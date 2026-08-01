#include <postgres.h>

#include <lib/stringinfo.h>
#include <utils/builtins.h>
#include <utils/jsonb.h>
#include <utils/numeric.h>

#include <mustach/mustach.h>
#include <mustach/mustach-wrap.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int mustach_process_jsonb(const char *template, size_t length, Jsonb *root, int flags, FILE *file, char **err);

/* Not declared portably across PG versions (utils/fmgrprotos.h, which
 * carries these from PG12 on, doesn't exist before that): declare
 * these builtins ourselves rather than chase down whichever header
 * happens to expose them in a given version. */
extern Datum numeric_out(PG_FUNCTION_ARGS);
extern Datum numeric_float8(PG_FUNCTION_ARGS);

/* JsonContainerSize/IsObject/IsArray/IsScalar were only added in PG10; the
 * underlying header field and flag masks are stable since jsonb's PG9.4
 * introduction, so reimplement them portably instead of relying on those. */
#define PGM_JB_SIZE(jc)       ((jc)->header & JB_CMASK)
#define PGM_JB_IS_SCALAR(jc)  (((jc)->header & JB_FSCALAR) != 0)
#define PGM_JB_IS_OBJECT(jc)  (((jc)->header & JB_FOBJECT) != 0)
#define PGM_JB_IS_ARRAY(jc)   (((jc)->header & JB_FARRAY) != 0)

static JsonbValue *jb_lookup_key(JsonbContainer *c, const char *name, int namelen) {
#if PG_VERSION_NUM >= 130000
    return getKeyJsonValueFromContainer(c, name, namelen, NULL);
#else
    JsonbValue key;
    key.type = jbvString;
    key.val.string.val = (char *) name;
    key.val.string.len = namelen;
    return findJsonbValueFromContainer(c, JB_FOBJECT, &key);
#endif
}

typedef struct {
    enum { SEL_NULL, SEL_STRING, SEL_NUMERIC, SEL_BOOL, SEL_CONTAINER } kind;
    union {
        struct { const char *val; int len; } string;
        Numeric numeric;
        bool boolean;
        JsonbContainer *container;
    } v;
} jbsel;

struct frame {
    JsonbContainer *container; /* non-NULL while array-iterating; NULL for a one-shot or objiter frame */
    JsonbIterator *iter;       /* non-NULL while object-iterating */
    bool is_objiter;
    int index;
    int count;
    jbsel value;
    jbsel key;                 /* current key, meaningful only when is_objiter */
};

struct expl {
    Jsonb *root;
    jbsel selection;
    int depth;
    struct frame stack[MUSTACH_MAX_DEPTH];
};

static jbsel null_sel(void) {
    jbsel r;
    r.kind = SEL_NULL;
    return r;
}

static jbsel from_jsonbvalue(JsonbValue *v) {
    jbsel r;
    switch (v->type) {
    case jbvString: r.kind = SEL_STRING; r.v.string.val = v->val.string.val; r.v.string.len = v->val.string.len; break;
    case jbvNumeric: r.kind = SEL_NUMERIC; r.v.numeric = v->val.numeric; break;
    case jbvBool: r.kind = SEL_BOOL; r.v.boolean = v->val.boolean; break;
    case jbvBinary: r.kind = SEL_CONTAINER; r.v.container = v->val.binary.data; break;
    default: r = null_sel(); break;
    }
    return r;
}

static jbsel get_array_item(JsonbContainer *c, int i) {
    JsonbValue *v = getIthJsonbValueFromContainer(c, (uint32) i);
    return v ? from_jsonbvalue(v) : null_sel();
}

static bool get_object_field(JsonbContainer *c, const char *name, int namelen, jbsel *out) {
    JsonbValue *v = jb_lookup_key(c, name, namelen);
    if (!v) return false;
    *out = from_jsonbvalue(v);
    return true;
}

static bool numeric_is_zero(Numeric n) {
    return DatumGetFloat8(DirectFunctionCall1(numeric_float8, NumericGetDatum(n))) == 0.0;
}

static bool objiter_advance(struct frame *f) {
    JsonbValue kv, vv;
    if (JsonbIteratorNext(&f->iter, &kv, true) != WJB_KEY)
        return false;
    f->key = from_jsonbvalue(&kv);
    JsonbIteratorNext(&f->iter, &vv, true);
    f->value = from_jsonbvalue(&vv);
    return true;
}

static int start(void *closure) {
    struct expl *e = closure;
    JsonbContainer *rc = &e->root->root;
    e->depth = 0;
    e->stack[0].container = NULL;
    e->stack[0].is_objiter = false;
    e->stack[0].value = PGM_JB_IS_SCALAR(rc) ? get_array_item(rc, 0) : (jbsel) { .kind = SEL_CONTAINER, .v.container = rc };
    e->selection = e->stack[0].value;
    return MUSTACH_OK;
}

static int compare(void *closure, const char *value) {
    struct expl *e = closure;
    jbsel *o = &e->selection;
    double d;
    size_t vlen, minlen;
    int c;
    switch (o->kind) {
    case SEL_NUMERIC:
        d = DatumGetFloat8(DirectFunctionCall1(numeric_float8, NumericGetDatum(o->v.numeric))) - atof(value);
        return d < 0 ? -1 : d > 0 ? 1 : 0;
    case SEL_STRING:
        vlen = strlen(value);
        minlen = (size_t) o->v.string.len < vlen ? (size_t) o->v.string.len : vlen;
        c = minlen ? memcmp(o->v.string.val, value, minlen) : 0;
        if (c == 0) c = (int) o->v.string.len - (int) vlen;
        return c < 0 ? -1 : c > 0 ? 1 : 0;
    case SEL_BOOL:
        return strcmp(o->v.boolean ? "true" : "false", value);
    case SEL_NULL:
        return strcmp("null", value);
    default:
        return 1;
    }
}

static int sel(void *closure, const char *name) {
    struct expl *e = closure;
    jbsel o;
    int i, r = 0;
    if (name == NULL) {
        o = e->stack[e->depth].value;
        r = 1;
    } else {
        for (i = e->depth; i >= 0 && !r; i--) {
            jbsel cur = e->stack[i].value;
            if (cur.kind == SEL_CONTAINER && PGM_JB_IS_OBJECT(cur.v.container))
                r = get_object_field(cur.v.container, name, (int) strlen(name), &o);
        }
        if (!r) o = null_sel();
    }
    e->selection = o;
    return r;
}

static int subsel(void *closure, const char *name) {
    struct expl *e = closure;
    jbsel o = null_sel();
    int r = 0;
    if (e->selection.kind == SEL_CONTAINER) {
        JsonbContainer *c = e->selection.v.container;
        if (PGM_JB_IS_OBJECT(c))
            r = get_object_field(c, name, (int) strlen(name), &o);
        else if (PGM_JB_IS_ARRAY(c) && *name) {
            char *end;
            long idx = strtol(name, &end, 10);
            if (!*end && idx >= 0 && idx < PGM_JB_SIZE(c)) {
                o = get_array_item(c, (int) idx);
                r = 1;
            }
        }
    }
    if (r) e->selection = o;
    return r;
}

static int enter(void *closure, int objiter) {
    struct expl *e = closure;
    jbsel o;
    struct frame *f;
    JsonbValue tmp;

    if (++e->depth >= MUSTACH_MAX_DEPTH)
        return MUSTACH_ERROR_TOO_DEEP;

    o = e->selection;
    f = &e->stack[e->depth];
    f->is_objiter = false;
    f->container = NULL;

    if (objiter) {
        if (o.kind != SEL_CONTAINER || !PGM_JB_IS_OBJECT(o.v.container) || PGM_JB_SIZE(o.v.container) == 0)
            goto not_entering;
        f->iter = JsonbIteratorInit(o.v.container);
        JsonbIteratorNext(&f->iter, &tmp, false); /* consume WJB_BEGIN_OBJECT */
        f->is_objiter = true;
        if (!objiter_advance(f))
            goto not_entering;
    } else if (o.kind == SEL_CONTAINER && PGM_JB_IS_ARRAY(o.v.container)) {
        if (PGM_JB_SIZE(o.v.container) == 0)
            goto not_entering;
        f->container = o.v.container;
        f->index = 0;
        f->count = PGM_JB_SIZE(o.v.container);
        f->value = get_array_item(f->container, 0);
    } else if ((o.kind == SEL_CONTAINER && PGM_JB_IS_OBJECT(o.v.container) && PGM_JB_SIZE(o.v.container) > 0)
            || (o.kind == SEL_BOOL && o.v.boolean)
            || (o.kind == SEL_STRING && o.v.string.len > 0)
            || (o.kind == SEL_NUMERIC && !numeric_is_zero(o.v.numeric))) {
        f->value = o;
    } else
        goto not_entering;
    return 1;

not_entering:
    e->depth--;
    return 0;
}

static int next(void *closure) {
    struct expl *e = closure;
    struct frame *f;
    if (e->depth <= 0)
        return MUSTACH_ERROR_CLOSING;
    f = &e->stack[e->depth];
    if (f->is_objiter)
        return objiter_advance(f) ? 1 : 0;
    if (f->container != NULL) {
        if (++f->index >= f->count)
            return 0;
        f->value = get_array_item(f->container, f->index);
        return 1;
    }
    return 0; /* one-shot frame: body already rendered once by enter() */
}

static int leave(void *closure) {
    struct expl *e = closure;
    if (e->depth <= 0)
        return MUSTACH_ERROR_CLOSING;
    e->depth--;
    return 0;
}

static int get(void *closure, struct mustach_sbuf *sbuf, int key) {
    struct expl *e = closure;
    if (key) {
        int d;
        jbsel *k = NULL;
        for (d = e->depth; d >= 0; d--)
            if (e->stack[d].is_objiter) { k = &e->stack[d].key; break; }
        if (k && k->kind == SEL_STRING) {
            sbuf->value = k->v.string.val;
            sbuf->length = (size_t) k->v.string.len;
        } else {
            sbuf->value = "";
            sbuf->length = 0;
        }
        return 1;
    }
    switch (e->selection.kind) {
    case SEL_STRING:
        sbuf->value = e->selection.v.string.val;
        sbuf->length = (size_t) e->selection.v.string.len;
        break;
    case SEL_NULL:
        sbuf->value = "";
        sbuf->length = 0;
        break;
    case SEL_BOOL:
        sbuf->value = e->selection.v.boolean ? "true" : "false";
        sbuf->length = 0;
        break;
    case SEL_NUMERIC:
        sbuf->value = DatumGetCString(DirectFunctionCall1(numeric_out, NumericGetDatum(e->selection.v.numeric)));
        sbuf->length = 0;
        break;
    case SEL_CONTAINER: {
        StringInfoData buf;
        initStringInfo(&buf);
        JsonbToCString(&buf, e->selection.v.container, -1);
        sbuf->value = buf.data;
        sbuf->length = (size_t) buf.len;
        break;
    }
    }
    return 1;
}

static const struct mustach_wrap_itf mustach_jsonb_wrap_itf = {
    .start = start,
    .stop = NULL,
    .compare = compare,
    .sel = sel,
    .subsel = subsel,
    .enter = enter,
    .next = next,
    .leave = leave,
    .get = get
};

int mustach_process_jsonb(const char *template, size_t length, Jsonb *root, int flags, FILE *file, char **err) {
    struct expl e;
    int rc;
    e.root = root;
    rc = mustach_wrap_file(template, length, &mustach_jsonb_wrap_itf, &e, flags, file);
    fclose(file);
    return rc;
}
