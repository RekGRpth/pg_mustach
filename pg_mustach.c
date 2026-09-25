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
int mustach_partial_from_data_jsonb(const char *name, mustach_sbuf_t *sbuf);

PG_MODULE_MAGIC;

/* pg_whitelist's "privileged" caller is a superuser; anyone else must be
 * granted access explicitly via pg_mustach.whitelist. */
static bool pg_mustach_privileged(void) {
    return superuser();
}

static int pg_mustach_flags = Mustach_With_AllExtensions;

/* pg_whitelist_check_local(), but answering false where it would raise its
 * permission-denied ERROR. Catching that without a subtransaction is fine:
 * pg_whitelist_check_local() holds no resources or locks, only palloc's,
 * and anything but the denial itself is re-thrown untouched. */
static bool pg_mustach_whitelisted(const char *name, const char *resolved, bool privileged) {
    MemoryContext context = CurrentMemoryContext;
    volatile bool allowed = true;
    PG_TRY(); {
        pg_whitelist_check_local(name, resolved, privileged);
    } PG_CATCH(); {
        ErrorData *edata;
        MemoryContextSwitchTo(context);
        edata = CopyErrorData();
        if (edata->sqlerrcode != ERRCODE_INSUFFICIENT_PRIVILEGE) PG_RE_THROW();
        FlushErrorState();
        FreeErrorData(edata);
        allowed = false;
    } PG_END_TRY();
    return allowed;
}

/* {{>name}} partials: mustach-wrap.c by itself resolves them from the json
 * data and from reading "name" (or "name.mustache") as a local file path --
 * see get_partial_buf() there -- the latter unconditionally, so any role
 * with EXECUTE on mustach() could read arbitrary server files via a crafted
 * template, unlike the 3-arg mustach() writing a file, which is already
 * gated on superuser. This hook replaces that lookup entirely (it never
 * returns MUSTACH_ERROR_NOT_FOUND, which would fall back to it), keeping
 * its order -- json data first under Mustach_With_PartialDataFirst, the
 * file first otherwise -- with file access gated like pg_curl's
 * pg_curl_privileged(): privileged (superuser) callers are admitted unless
 * pg_mustach.whitelist explicitly excludes the resolved path; unprivileged
 * callers are admitted only if it explicitly includes it. A file that's
 * there but not admitted only raises ERROR when the json data can't provide
 * the partial either -- relative names resolve against the data directory,
 * so e.g. {{>base}} would otherwise hit PGDATA/base instead of "base" in
 * the data. Nothing found anywhere renders empty, as mustach-wrap.c does. */
static int pg_mustach_get_partial(const char *name, mustach_sbuf_t *sbuf) {
    static char extension[] = ".mustache";
    bool data_first = (pg_mustach_flags & Mustach_With_PartialDataFirst) != 0;
    bool privileged = pg_mustach_privileged();
    bool found;
    char path[PATH_MAX];
    char resolved[PATH_MAX];
    size_t length = strlen(name);
    if (data_first && mustach_partial_from_data_jsonb(name, sbuf)) return MUSTACH_OK;
    if (length + sizeof extension > sizeof path) return MUSTACH_ERROR_TOO_BIG;
    memcpy(path, name, length);
    path[length] = 0;
    found = realpath(path, resolved) != NULL;
    if (!found) {
        memcpy(&path[length], extension, sizeof extension);
        found = realpath(path, resolved) != NULL;
    }
    if (found) {
        if (!pg_mustach_whitelisted(name, resolved, privileged)) {
            if (!data_first && mustach_partial_from_data_jsonb(name, sbuf)) return MUSTACH_OK;
            pg_whitelist_check_local(name, resolved, privileged); /* raises the denial */
        }
        if (mustach_read_file(resolved, sbuf) == MUSTACH_OK) return MUSTACH_OK;
    }
    if (!data_first && mustach_partial_from_data_jsonb(name, sbuf)) return MUSTACH_OK;
    sbuf->value = "";
    return MUSTACH_OK;
}
static bool pg_mustach_transaction = true;

void _PG_init(void);
void _PG_init(void) {
    DefineCustomIntVariable("pg_mustach.flags", "Sets the flags (bitmask of the values returned by mustach_with_*() functions) controlling mustach rendering.", NULL, &pg_mustach_flags, Mustach_With_AllExtensions, 0, INT_MAX, PGC_USERSET, 0, NULL, NULL, NULL);
#if PG_VERSION_NUM >= 90500
    DefineCustomBoolVariable("pg_mustach.transaction", "pg_mustach transaction", "Scope mustach_template()'d templates to the current transaction instead of the session?", &pg_mustach_transaction, true, PGC_USERSET, 0, NULL, NULL, NULL);
#endif
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

/* Backend-local cache of templates parsed by mustach_template(), keyed by
 * tplname the same way pg_curl keys its named connections by conname: a
 * NULL tplname addresses a single unnamed default slot (mirroring
 * pg_curl's static "pg_curl" connection), any other tplname addresses an
 * entry in this hash. Session-scoped, same trust model as pg_curl's
 * connections: not persisted, not shared across backends, live until
 * mustach_free() or backend exit. */
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
 * mustach_free(), and that call only exists since PG 19). Sweeping is
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
/* No MemoryContextRegisterResetCallback before PG 9.5: pg_mustach.transaction
 * isn't even registered as a GUC there (see _PG_init), and prepared
 * templates are always session-lifetime -- only mustach_free() (or the
 * session ending) removes them. See expected/transaction_1.out. */
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

/* Cleanup for an ERROR raised while rendering (e.g. pg_whitelist denying a
 * partial from pg_mustach_get_partial(), or any palloc/jsonb failure), which
 * longjmps past the rc != MUSTACH_OK path: without it the FILE (and its fd)
 * would leak for the rest of the session, and a half-written O_EXCL target
 * would stay behind and block every retry with the same path. fclose() must
 * come first, since for open_memstream() it's what finalizes *data. */
static void pg_mustach_abort(FILE *file, char **data, const char *name) {
    fclose(file);
    if (*data) free(*data);
    if (name) unlink(name);
}

/* Close file once rendering returned rc, and fail the call if either the
 * render or the close did. fclose() is where buffered output actually
 * reaches the target file, and for open_memstream() where *data gets
 * finalized, so its failure (e.g. ENOSPC) must not be reported as success
 * over a truncated file or a NULL *data. On failure *data is freed and the
 * target file removed before raising the ERROR. */
static void pg_mustach_close(FILE *file, int rc, const char *err, char **data, const char *name) {
    int close_errno = fclose(file) ? errno : 0;
    if (rc == MUSTACH_OK && !close_errno) return;
    if (*data) free(*data);
    if (name) unlink(name);
    pg_mustach_check(rc, err);
    errno = close_errno;
    if (name) ereport(ERROR, (errcode_for_file_access(), errmsg("could not write file \"%s\": %m", name)));
    ereport(ERROR, (errcode(ERRCODE_OUT_OF_MEMORY), errmsg("could not write mustach output: %m")));
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
    PG_TRY(); {
        rc = mustach_process_jsonb(VARDATA_ANY(template), VARSIZE_ANY_EXHDR(template), json, pg_mustach_flags, file, &err);
    } PG_CATCH(); {
        pg_mustach_abort(file, &data, name);
        PG_RE_THROW();
    } PG_END_TRY();
    pg_mustach_close(file, rc, err, &data, name);
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

EXTENSION(pg_mustach_template) {
    text *template;
    mustach_template_t *templ;
    NameData *tplname;
    int rc;
    if (PG_ARGISNULL(0)) ereport(ERROR, (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED), errmsg("mustach_template requires argument template")));
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

EXTENSION(pg_mustach_json) {
    Jsonb *json;
    mustach_template_t *templ;
    char *data = NULL;
    char *name = NULL;
    size_t len;
    FILE *file;
    text *output;
    int rc;
    if (PG_ARGISNULL(0)) ereport(ERROR, (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED), errmsg("mustach_json requires argument json")));
    pg_mustach_global_init();
#if PG_VERSION_NUM >= 110000
    json = PG_GETARG_JSONB_P(0);
#else
    json = PG_GETARG_JSONB(0);
#endif
    switch (PG_NARGS()) {
        case 2: {
            templ = pg_mustach_prepared_get(PG_TPLNAME(1));
            if (!(file = open_memstream(&data, &len))) ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("!open_memstream")));
        } break;
        case 3: {
            int fd;
            int open_errno;
            if (PG_ARGISNULL(1)) ereport(ERROR, (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED), errmsg("mustach_json requires argument file")));
            templ = pg_mustach_prepared_get(PG_TPLNAME(2));
            if (!superuser())
                ereport(ERROR, (errcode(ERRCODE_INSUFFICIENT_PRIVILEGE), errmsg("permission denied to write server file"), errdetail("Only superusers may write files with mustach_json.")));
            name = TextDatumGetCString(PG_GETARG_DATUM(1));
            fd = open(name, O_WRONLY | O_CREAT | O_EXCL, 0666);
            open_errno = errno;
            if (fd < 0) ereport(ERROR, (errcode(open_errno == EEXIST ? ERRCODE_DUPLICATE_FILE : ERRCODE_INTERNAL_ERROR), errmsg(open_errno == EEXIST ? "mustach target file already exists" : "!open")));
            if (!(file = fdopen(fd, "wb"))) { close(fd); unlink(name); ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("!fdopen"))); }
        } break;
        default: ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("expect be 2 or 3 args")));
    }
    PG_TRY(); {
        rc = mustach_render_jsonb(templ, json, pg_mustach_flags, file);
    } PG_CATCH(); {
        pg_mustach_abort(file, &data, name);
        PG_RE_THROW();
    } PG_END_TRY();
    pg_mustach_close(file, rc, NULL, &data, name);
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

EXTENSION(pg_mustach_free) {
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
