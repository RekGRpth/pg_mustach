#include <postgres.h>

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <catalog/pg_type.h>
#include <fmgr.h>
#include <miscadmin.h>

#include <utils/builtins.h>
#include <utils/guc.h>
#include <utils/hsearch.h>
#include <utils/jsonb.h>
#include <utils/memutils.h>
#if PG_VERSION_NUM >= 160000
#include <varatt.h>
#endif

#include <mustach/mustach.h>
#include <mustach/mustach-helpers.h>
#include <mustach/mustach-wrap.h>

#include "pg_whitelist.h"

#define EXTENSION(function) Datum (function)(PG_FUNCTION_ARGS); PG_FUNCTION_INFO_V1(function); Datum (function)(PG_FUNCTION_ARGS)

int mustach_process_jsonb(const char *template, size_t length, Jsonb *root, int flags, FILE *file, char **err);
int mustach_prepare_jsonb(const char *template, size_t length, int flags, mustach_template_t **templ);
int mustach_render_jsonb(mustach_template_t *templ, Jsonb *root, int flags, FILE *file);
void mustach_destroy_jsonb(mustach_template_t *templ);

PG_MODULE_MAGIC;

/* pg_whitelist's "privileged" caller is a superuser; anyone else must be
 * granted access explicitly via pg_mustach.whitelist. */
static bool pg_mustach_privileged(void) {
    return superuser();
}

/* {{>name}} partials that mustach-wrap.c can't resolve from the json data
 * fall back to reading "name" (and "name.mustache") as a local file path --
 * see get_partial_from_file() in mustach-wrap.c. Without this hook that
 * happens unconditionally, so any role with EXECUTE on mustach() could read
 * arbitrary server files via a crafted template, unlike the 3-arg mustach()
 * writing a file, which is already gated on superuser. Mirrors pg_curl's
 * pg_curl_privileged(): privileged (superuser) callers are admitted unless
 * pg_mustach.whitelist explicitly excludes the resolved path; unprivileged
 * callers are admitted only if it explicitly includes it. */
static int pg_mustach_get_partial(const char *name, mustach_sbuf_t *sbuf) {
    static char extension[] = ".mustache";
    bool privileged = pg_mustach_privileged();
    char path[PATH_MAX];
    char resolved[PATH_MAX];
    size_t length = strlen(name);
    if (length + sizeof extension > sizeof path) return MUSTACH_ERROR_TOO_BIG;
    memcpy(path, name, length);
    path[length] = 0;
    if (!realpath(path, resolved)) {
        memcpy(&path[length], extension, sizeof extension);
        if (!realpath(path, resolved)) return MUSTACH_ERROR_NOT_FOUND;
    }
    pg_whitelist_check_local(name, resolved, privileged);
    return mustach_read_file(path, sbuf) == MUSTACH_OK ? MUSTACH_OK : MUSTACH_ERROR_NOT_FOUND;
}

static int pg_mustach_flags = Mustach_With_AllExtensions;
static bool pg_mustach_transaction = true;

void _PG_init(void);
void _PG_init(void) {
    DefineCustomIntVariable("pg_mustach.flags", "Sets the flags (bitmask of the values returned by mustach_with_*() functions) controlling mustach rendering.", NULL, &pg_mustach_flags, Mustach_With_AllExtensions, 0, INT_MAX, PGC_USERSET, 0, NULL, NULL, NULL);
    DefineCustomBoolVariable("pg_mustach.transaction", "pg_mustach transaction", "Scope mustach_prepare()'d templates to the current transaction instead of the session?", &pg_mustach_transaction, true, PGC_USERSET, 0, NULL, NULL, NULL);
    pg_whitelist_init("pg_mustach.whitelist");
    mustach_wrap_get_partial = pg_mustach_get_partial;
}

EXTENSION(pg_mustach_with_allextensions) { PG_RETURN_INT32(Mustach_With_AllExtensions); }
EXTENSION(pg_mustach_with_colon) { PG_RETURN_INT32(Mustach_With_Colon); }
EXTENSION(pg_mustach_with_compare) { PG_RETURN_INT32(Mustach_With_Compare); }
EXTENSION(pg_mustach_with_emptytag) { PG_RETURN_INT32(Mustach_With_EmptyTag); }
EXTENSION(pg_mustach_with_equal) { PG_RETURN_INT32(Mustach_With_Equal); }
EXTENSION(pg_mustach_with_errorundefined) { PG_RETURN_INT32(Mustach_With_ErrorUndefined); }
EXTENSION(pg_mustach_with_escfirstcmp) { PG_RETURN_INT32(Mustach_With_EscFirstCmp); }
EXTENSION(pg_mustach_with_incpartial) { PG_RETURN_INT32(Mustach_With_IncPartial); }
EXTENSION(pg_mustach_with_jsonpointer) { PG_RETURN_INT32(Mustach_With_JsonPointer); }
EXTENSION(pg_mustach_with_noextensions) { PG_RETURN_INT32(Mustach_With_NoExtensions); }
EXTENSION(pg_mustach_with_objectiter) { PG_RETURN_INT32(Mustach_With_ObjectIter); }
EXTENSION(pg_mustach_with_partialdatafirst) { PG_RETURN_INT32(Mustach_With_PartialDataFirst); }
EXTENSION(pg_mustach_with_singledot) { PG_RETURN_INT32(Mustach_With_SingleDot); }

/* Backend-local cache of templates parsed by mustach_prepare(), keyed by
 * tplname the same way pg_curl keys its named connections by conname: a
 * NULL tplname addresses a single unnamed default slot (mirroring
 * pg_curl's static "pg_curl" connection), any other tplname addresses an
 * entry in this hash. Session-scoped, same trust model as pg_curl's
 * connections: not persisted, not shared across backends, live until
 * mustach_forget() or backend exit. */
typedef struct {
    NameData tplname; // !!! always first !!! //
    mustach_template_t *templ;
} pg_mustach_prepared;

static HTAB *pg_mustach_prepared_hash = NULL;
static mustach_template_t *pg_mustach_default_templ = NULL;

static HTAB *pg_mustach_prepared_hash_get(void) {
    if (!pg_mustach_prepared_hash) {
        HASHCTL ctl;
        memset(&ctl, 0, sizeof(ctl));
        ctl.keysize = sizeof(NameData);
        ctl.entrysize = sizeof(pg_mustach_prepared);
#if PG_VERSION_NUM >= 140000
        pg_mustach_prepared_hash = hash_create("pg_mustach prepared templates", 16, &ctl, HASH_ELEM | HASH_STRINGS);
#else
        pg_mustach_prepared_hash = hash_create("pg_mustach prepared templates", 16, &ctl, HASH_ELEM);
#endif
    }
    return pg_mustach_prepared_hash;
}

#if PG_VERSION_NUM >= 90500
/* Mirrors pg_curl's pg_curl_global/pg_curl_global_init/pg_curl_global_cleanup:
 * a single reset callback, (re)armed on the context matching
 * pg_mustach.transaction, sweeps every prepared template away when that
 * context resets -- rather than one callback per template (which would need
 * MemoryContextUnregisterResetCallback to cancel cleanly on an explicit
 * mustach_forget(), and that call only exists since PG 19). Sweeping is
 * safe to do with a plain hash_search(HASH_REMOVE) while hash_seq_search()
 * is in progress -- deleting the currently-returned element mid-scan is
 * explicitly supported by dynahash. */
typedef struct {
    MemoryContext context;
    MemoryContextCallback cleanup;
} pg_mustach_global_t;

static pg_mustach_global_t pg_mustach_global = {0};

static void pg_mustach_global_cleanup(void *arg) {
    (void) arg;
    if (pg_mustach_default_templ) {
        mustach_destroy_jsonb(pg_mustach_default_templ);
        pg_mustach_default_templ = NULL;
    }
    if (pg_mustach_prepared_hash) {
        HASH_SEQ_STATUS status;
        pg_mustach_prepared *entry;
        hash_seq_init(&status, pg_mustach_prepared_hash);
        while ((entry = hash_seq_search(&status))) {
            mustach_destroy_jsonb(entry->templ);
            hash_search(pg_mustach_prepared_hash, NameStr(entry->tplname), HASH_REMOVE, NULL);
        }
    }
    pg_mustach_global.context = NULL;
}

static void pg_mustach_global_init(void) {
    if (pg_mustach_global.context) return;
    pg_mustach_global.context = pg_mustach_transaction ? TopTransactionContext : TopMemoryContext;
    pg_mustach_global.cleanup.func = pg_mustach_global_cleanup;
    MemoryContextRegisterResetCallback(pg_mustach_global.context, &pg_mustach_global.cleanup);
}
#else
static void pg_mustach_global_init(void) {
}
#endif

/* Resolve tplname (NULL for the unnamed default slot) to its prepared
 * template, erroring if none is prepared there yet. */
static mustach_template_t *pg_mustach_prepared_get(NameData *tplname) {
    pg_mustach_prepared *entry;
    bool found;
    if (!tplname) {
        if (!pg_mustach_default_templ) ereport(ERROR, (errcode(ERRCODE_UNDEFINED_OBJECT), errmsg("no prepared mustach template")));
        return pg_mustach_default_templ;
    }
    entry = hash_search(pg_mustach_prepared_hash_get(), NameStr(*tplname), HASH_FIND, &found);
    if (!found) ereport(ERROR, (errcode(ERRCODE_UNDEFINED_OBJECT), errmsg("unknown prepared mustach template \"%s\"", NameStr(*tplname))));
    return entry->templ;
}

#define PG_TPLNAME(arg) (PG_ARGISNULL(arg) ? NULL : PG_GETARG_NAME(arg))

static void pg_mustach_check(int rc, const char *err) {
    switch (rc) {
        case MUSTACH_OK: break;
        case MUSTACH_ERROR_SYSTEM: ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_SYSTEM"))); break;
        case MUSTACH_ERROR_UNEXPECTED_END: ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_UNEXPECTED_END"))); break;
        case MUSTACH_ERROR_EMPTY_TAG: ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_EMPTY_TAG"))); break;
#if MUSTACH_VERSION >= 200
        case MUSTACH_ERROR_TOO_BIG: ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_TOO_BIG"))); break;
#else
        case MUSTACH_ERROR_TAG_TOO_LONG: ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_TAG_TOO_LONG"))); break;
#endif
#if MUSTACH_VERSION >= 200
        case MUSTACH_ERROR_BAD_DELIMITER: ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_BAD_DELIMITER"))); break;
#else
        case MUSTACH_ERROR_BAD_SEPARATORS: ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_BAD_SEPARATORS"))); break;
#endif
        case MUSTACH_ERROR_TOO_DEEP: ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_TOO_DEEP"))); break;
        case MUSTACH_ERROR_CLOSING: ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_CLOSING"))); break;
        case MUSTACH_ERROR_BAD_UNESCAPE_TAG: ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_BAD_UNESCAPE_TAG"))); break;
        case MUSTACH_ERROR_INVALID_ITF: ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_INVALID_ITF"))); break;
#if MUSTACH_VERSION >= 200
        case MUSTACH_ERROR_NOT_FOUND: ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_NOT_FOUND"))); break;
#else
        case MUSTACH_ERROR_ITEM_NOT_FOUND: ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_ITEM_NOT_FOUND"))); break;
        case MUSTACH_ERROR_PARTIAL_NOT_FOUND: ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_PARTIAL_NOT_FOUND"))); break;
#endif
        case MUSTACH_ERROR_UNDEFINED_TAG: ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_UNDEFINED_TAG"))); break;
        case MUSTACH_ERROR_TOO_MUCH_NESTING: ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_TOO_MUCH_NESTING"))); break;
#if MUSTACH_VERSION >= 200
        case MUSTACH_ERROR_OUT_OF_MEMORY: ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_OUT_OF_MEMORY"))); break;
#endif
        default: ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("%s", err ? err : "unknown mustach error"))); break;
    }
}

EXTENSION(pg_mustach) {
    char *data = NULL;
    char *err = NULL;
    char *name = NULL;
    FILE *file;
    int rc;
    size_t len;
    Jsonb *json;
    text *output;
    text *template;
    if (PG_ARGISNULL(0)) ereport(ERROR, (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED), errmsg("mustach requires argument json")));
    if (PG_ARGISNULL(1)) ereport(ERROR, (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED), errmsg("mustach requires argument template")));
#if PG_VERSION_NUM >= 110000
    json = PG_GETARG_JSONB_P(0);
#else
    json = PG_GETARG_JSONB(0);
#endif
    template = PG_GETARG_TEXT_PP(1);
    switch (PG_NARGS()) {
        case 2: {
            if (!(file = open_memstream(&data, &len))) ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("!open_memstream")));
        } break;
        case 3: {
            int fd;
            int open_errno;
            if (PG_ARGISNULL(2)) ereport(ERROR, (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED), errmsg("mustach requires argument file")));
            if (!superuser())
                ereport(ERROR, (errcode(ERRCODE_INSUFFICIENT_PRIVILEGE), errmsg("permission denied to write server file"), errdetail("Only superusers may write files with mustach.")));
            name = TextDatumGetCString(PG_GETARG_DATUM(2));
            fd = open(name, O_WRONLY | O_CREAT | O_EXCL, 0666);
            open_errno = errno;
            if (fd < 0) ereport(ERROR, (errcode(open_errno == EEXIST ? ERRCODE_DUPLICATE_FILE : ERRCODE_INTERNAL_ERROR), errmsg(open_errno == EEXIST ? "mustach target file already exists" : "!open")));
            if (!(file = fdopen(fd, "wb"))) { close(fd); unlink(name); ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("!fdopen"))); }
        } break;
        default: ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("expect be 2 or 3 args")));
    }
    rc = mustach_process_jsonb(VARDATA_ANY(template), VARSIZE_ANY_EXHDR(template), json, pg_mustach_flags, file, &err);
    if (rc != MUSTACH_OK) {
        if (data) free(data);
        if (name) unlink(name);
        pg_mustach_check(rc, err);
    }
    PG_FREE_IF_COPY(json, 0);
    PG_FREE_IF_COPY(template, 1);
    switch (PG_NARGS()) {
        case 2:
            output = cstring_to_text_with_len(data, len);
            free(data);
            PG_RETURN_TEXT_P(output);
        case 3: if (name) pfree(name); PG_RETURN_BOOL(true);
        default: ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("expect be 2 or 3 args")));
    }
}

EXTENSION(pg_mustach_prepare) {
    text *template;
    mustach_template_t *templ;
    NameData *tplname;
    int rc;
    if (PG_ARGISNULL(0)) ereport(ERROR, (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED), errmsg("mustach_prepare requires argument template")));
    pg_mustach_global_init();
    template = PG_GETARG_TEXT_PP(0);
    tplname = PG_TPLNAME(1);
    rc = mustach_prepare_jsonb(VARDATA_ANY(template), VARSIZE_ANY_EXHDR(template), pg_mustach_flags, &templ);
    if (rc != MUSTACH_OK) pg_mustach_check(rc, NULL);
    PG_FREE_IF_COPY(template, 0);
    if (!tplname) {
        if (pg_mustach_default_templ) mustach_destroy_jsonb(pg_mustach_default_templ);
        pg_mustach_default_templ = templ;
    } else {
        pg_mustach_prepared *entry;
        bool found;
        entry = hash_search(pg_mustach_prepared_hash_get(), NameStr(*tplname), HASH_ENTER, &found);
        if (found) mustach_destroy_jsonb(entry->templ);
        entry->templ = templ;
    }
    PG_RETURN_VOID();
}

EXTENSION(pg_mustach_render) {
    Jsonb *json;
    mustach_template_t *templ;
    char *data = NULL;
    char *name = NULL;
    size_t len;
    FILE *file;
    text *output;
    int rc;
    if (PG_ARGISNULL(0)) ereport(ERROR, (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED), errmsg("mustach_render requires argument json")));
    pg_mustach_global_init();
    json = PG_GETARG_JSONB_P(0);
    switch (PG_NARGS()) {
        case 2: {
            templ = pg_mustach_prepared_get(PG_TPLNAME(1));
            if (!(file = open_memstream(&data, &len))) ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("!open_memstream")));
        } break;
        case 3: {
            int fd;
            int open_errno;
            if (PG_ARGISNULL(1)) ereport(ERROR, (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED), errmsg("mustach_render requires argument file")));
            templ = pg_mustach_prepared_get(PG_TPLNAME(2));
            if (!superuser())
                ereport(ERROR, (errcode(ERRCODE_INSUFFICIENT_PRIVILEGE), errmsg("permission denied to write server file"), errdetail("Only superusers may write files with mustach_render.")));
            name = TextDatumGetCString(PG_GETARG_DATUM(1));
            fd = open(name, O_WRONLY | O_CREAT | O_EXCL, 0666);
            open_errno = errno;
            if (fd < 0) ereport(ERROR, (errcode(open_errno == EEXIST ? ERRCODE_DUPLICATE_FILE : ERRCODE_INTERNAL_ERROR), errmsg(open_errno == EEXIST ? "mustach target file already exists" : "!open")));
            if (!(file = fdopen(fd, "wb"))) { close(fd); unlink(name); ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("!fdopen"))); }
        } break;
        default: ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("expect be 2 or 3 args")));
    }
    rc = mustach_render_jsonb(templ, json, pg_mustach_flags, file);
    fclose(file);
    if (rc != MUSTACH_OK) {
        if (data) free(data);
        if (name) unlink(name);
        pg_mustach_check(rc, NULL);
    }
    PG_FREE_IF_COPY(json, 0);
    switch (PG_NARGS()) {
        case 2:
            output = cstring_to_text_with_len(data, len);
            free(data);
            PG_RETURN_TEXT_P(output);
        case 3: if (name) pfree(name); PG_RETURN_BOOL(true);
        default: ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("expect be 2 or 3 args")));
    }
}

EXTENSION(pg_mustach_forget) {
    NameData *tplname = PG_TPLNAME(0);
    bool found;
    pg_mustach_global_init();
    if (!tplname) {
        found = pg_mustach_default_templ != NULL;
        if (found) {
            mustach_destroy_jsonb(pg_mustach_default_templ);
            pg_mustach_default_templ = NULL;
        }
    } else {
        pg_mustach_prepared *entry = hash_search(pg_mustach_prepared_hash_get(), NameStr(*tplname), HASH_FIND, &found);
        if (found) {
            mustach_destroy_jsonb(entry->templ);
            hash_search(pg_mustach_prepared_hash_get(), NameStr(*tplname), HASH_REMOVE, NULL);
        }
    }
    PG_RETURN_BOOL(found);
}
