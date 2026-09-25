#include <postgres.h>

#include <lib/stringinfo.h>
#include <utils/builtins.h>
#include <utils/jsonb.h>
#include <utils/numeric.h>

#include <mustach/mustach.h>
#include <mustach/mustach-wrap.h>
#include <mustach/mustach-helpers.h>

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int mustach_process_jsonb(const char *template, size_t length, Jsonb *root, int flags, FILE *file, char **err);
int mustach_prepare_jsonb(const char *template, size_t length, int flags, mustach_template_t **templ);
int mustach_render_jsonb(mustach_template_t *templ, Jsonb *root, int flags, FILE *file);
void mustach_destroy_jsonb(mustach_template_t *templ);
int mustach_partial_from_data_jsonb(const char *name, mustach_sbuf_t *sbuf);

/* Not declared portably across PG versions (utils/fmgrprotos.h, which
 * carries these from PG12 on, doesn't exist before that): declare
 * these builtins ourselves rather than chase down whichever header
 * happens to expose them in a given version. */
extern Datum numeric_out(PG_FUNCTION_ARGS);
extern Datum numeric_in(PG_FUNCTION_ARGS);
extern Datum numeric_cmp(PG_FUNCTION_ARGS);

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

/* numeric_float8() hard-errors on overflow, which would abort rendering
 * for jsonb numbers outside double's range even though such numbers are
 * trivially non-zero. Go through numeric_out()'s text form instead:
 * strtod() saturates to +/-HUGE_VAL on overflow rather than erroring. */
static double numeric_to_double(Numeric n) {
    return strtod(DatumGetCString(DirectFunctionCall1(numeric_out, NumericGetDatum(n))), NULL);
}

/* Exact regardless of magnitude, unlike going through double: a number
 * that underflows to 0.0 (e.g. 1e-400) is still non-zero. */
static bool numeric_is_zero(Numeric n) {
    const char *s = DatumGetCString(DirectFunctionCall1(numeric_out, NumericGetDatum(n)));
    for (; *s; s++)
        if (*s >= '1' && *s <= '9')
            return false;
    return true;
}

/* Whether numeric_in() is certain to accept s, on every supported PG
 * version, without raising: plain decimal notation only (no NaN/Infinity,
 * nor PG16's non-decimal integers and underscores), and at most 1000
 * digits with a 3-digit exponent, well inside numeric's range everywhere
 * (older versions cap the exponent at +/-1000). */
static bool numeric_is_decimal(const char *s) {
    int digits = 0;
    int exponent_digits = 0;
    while (isspace((unsigned char) *s)) s++;
    if (*s == '+' || *s == '-') s++;
    for (; isdigit((unsigned char) *s); s++) digits++;
    if (*s == '.')
        for (s++; isdigit((unsigned char) *s); s++) digits++;
    if (!digits || digits > 1000) return false;
    if (*s == 'e' || *s == 'E') {
        s++;
        if (*s == '+' || *s == '-') s++;
        for (; isdigit((unsigned char) *s); s++)
            if (++exponent_digits > 3) return false;
        if (!exponent_digits) return false;
    }
    while (isspace((unsigned char) *s)) s++;
    return !*s;
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
        /* Exactly, as numeric, whenever value is a number: going through
         * double would call 12345678901234567890 equal to ...891, or 10
         * not greater than 9.99999999999999999. Anything else keeps the
         * double comparison against atof(value) it always got. */
        if (numeric_is_decimal(value)) {
            Datum n = DirectFunctionCall3(numeric_in, CStringGetDatum(value), ObjectIdGetDatum(InvalidOid), Int32GetDatum(-1));
            c = DatumGetInt32(DirectFunctionCall2(numeric_cmp, NumericGetDatum(o->v.numeric), n));
            return c < 0 ? -1 : c > 0 ? 1 : 0;
        }
        d = numeric_to_double(o->v.numeric) - atof(value);
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

/* Same callbacks, minus start: rendering through this picks up an
 * in-progress render's state (current section context included) as is,
 * instead of resetting it back to the root. */
static const struct mustach_wrap_itf mustach_jsonb_lookup_itf = {
    .start = NULL,
    .stop = NULL,
    .compare = compare,
    .sel = sel,
    .subsel = subsel,
    .enter = enter,
    .next = next,
    .leave = leave,
    .get = get
};

/* mustach_wrap_get_partial is a global hook with no closure, so the render
 * in progress is published here for mustach_partial_from_data_jsonb(). */
static struct expl *current_expl = NULL;
static int current_flags = 0;

/* Look name up in the json data of the render in progress the same way
 * mustach-wrap.c's own get_partial_buf() would (via getoptional(): same key
 * splitting, json pointer, compare and objiter handling, from the current
 * section context), by rendering "{{&name}}" against that render's state
 * with ErrorUndefined, so a missing name fails instead of rendering empty.
 * Delimiters are switched to \1 and \2 first, so a name containing "}}"
 * (possible under custom delimiters) can't break the tag. Returns 1 with
 * sbuf filled (malloc'd, released by mustach via freecb) if found. */
int mustach_partial_from_data_jsonb(const char *name, mustach_sbuf_t *sbuf) {
    static const char head[] = "{{=\1 \2=}}\1&";
    size_t length = strlen(name);
    size_t size;
    char *template;
    char *result;
    int rc;
    if (!current_expl || strpbrk(name, "\1\2")) return 0;
    template = palloc(sizeof head + length);
    memcpy(template, head, sizeof head - 1);
    memcpy(template + sizeof head - 1, name, length);
    template[sizeof head - 1 + length] = '\2';
    rc = mustach_wrap_mem(template, sizeof head + length, &mustach_jsonb_lookup_itf, current_expl, current_flags | Mustach_With_ErrorUndefined, &result, &size);
    pfree(template);
    if (rc != MUSTACH_OK) return 0;
    sbuf->value = result;
    sbuf->length = size;
    sbuf->freecb = free;
    return 1;
}

int mustach_process_jsonb(const char *template, size_t length, Jsonb *root, int flags, FILE *file, char **err) {
    struct expl e;
    int rc;
    /* A zero length means "unknown, NUL-terminated" to mustach, which
     * the text datum's contents aren't, so an empty template would
     * otherwise be read past its end. */
    if (!length) template = "";
    e.root = root;
    current_expl = &e;
    current_flags = flags;
    rc = mustach_wrap_file(template, length, &mustach_jsonb_wrap_itf, &e, flags, file);
    current_expl = NULL;
    return rc;
}

/* mustach_make_template() doesn't copy the template text: the built
 * mustach_template_t keeps references into it (see mustach2.c's "the
 * reference text" comment on struct mustach_template.sbuf), so the buffer
 * must outlive every render done through the returned template. Hand it
 * our own malloc'd copy with a freecb; mustach_destroy_template() releases
 * it via that freecb, so we never touch/free it ourselves afterward. */
int mustach_prepare_jsonb(const char *template, size_t length, int flags, mustach_template_t **templ) {
    mustach_sbuf_t sbuf = MUSTACH_SBUF_INIT;
    /* NUL-terminated: a zero sbuf.length means "unknown, use strlen()" to
     * mustach (mustach_sbuf_length()), so an empty template would
     * otherwise be read past its end. */
    char *copy = malloc(length + 1);
    int buildflags;
    if (!copy)
        return MUSTACH_ERROR_SYSTEM;
    memcpy(copy, template, length);
    copy[length] = '\0';
    sbuf.value = copy;
    sbuf.length = length;
    sbuf.freecb = free;
    buildflags = 0;
    if (flags & Mustach_With_Colon)
        buildflags |= Mustach_Build_With_Colon;
    if (flags & Mustach_With_EmptyTag)
        buildflags |= Mustach_Build_With_EmptyTag;
    return mustach_make_template(templ, buildflags, &sbuf, NULL);
}

int mustach_render_jsonb(mustach_template_t *templ, Jsonb *root, int flags, FILE *file) {
    struct expl e;
    int rc;
    e.root = root;
    current_expl = &e;
    current_flags = flags;
    rc = mustach_wrap_apply(templ, &mustach_jsonb_wrap_itf, &e, flags, mustach_fwrite_cb, NULL, file);
    current_expl = NULL;
    return rc;
}

void mustach_destroy_jsonb(mustach_template_t *templ) {
    mustach_destroy_template(templ, NULL, NULL);
}
