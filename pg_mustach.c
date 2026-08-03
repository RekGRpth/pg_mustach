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
#include <utils/jsonb.h>
#if PG_VERSION_NUM >= 160000
#include <varatt.h>
#endif

#include <mustach/mustach.h>
#include <mustach/mustach-helpers.h>
#include <mustach/mustach-wrap.h>

#include "pg_whitelist.h"

#define EXTENSION(function) Datum (function)(PG_FUNCTION_ARGS); PG_FUNCTION_INFO_V1(function); Datum (function)(PG_FUNCTION_ARGS)

int mustach_process_jsonb(const char *template, size_t length, Jsonb *root, int flags, FILE *file, char **err);

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

void _PG_init(void);
void _PG_init(void) {
    DefineCustomIntVariable("pg_mustach.flags", "Sets the flags (bitmask of the values returned by mustach_with_*() functions) controlling mustach rendering.", NULL, &pg_mustach_flags, Mustach_With_AllExtensions, 0, INT_MAX, PGC_USERSET, 0, NULL, NULL, NULL);
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

EXTENSION(pg_mustach_jsonb) {
    char *data = NULL;
    char *err = NULL;
    char *name = NULL;
    FILE *file;
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
    switch (mustach_process_jsonb(VARDATA_ANY(template), VARSIZE_ANY_EXHDR(template), json, pg_mustach_flags, file, &err)) {
        case MUSTACH_OK: break;
        case MUSTACH_ERROR_SYSTEM: if (data) free(data); if (name) unlink(name); ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_SYSTEM"))); break;
        case MUSTACH_ERROR_UNEXPECTED_END: if (data) free(data); if (name) unlink(name); ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_UNEXPECTED_END"))); break;
        case MUSTACH_ERROR_EMPTY_TAG: if (data) free(data); if (name) unlink(name); ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_EMPTY_TAG"))); break;
#if MUSTACH_VERSION >= 200
        case MUSTACH_ERROR_TOO_BIG: if (data) free(data); if (name) unlink(name); ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_TOO_BIG"))); break;
#else
        case MUSTACH_ERROR_TAG_TOO_LONG: if (data) free(data); if (name) unlink(name); ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_TAG_TOO_LONG"))); break;
#endif
#if MUSTACH_VERSION >= 200
        case MUSTACH_ERROR_BAD_DELIMITER: if (data) free(data); if (name) unlink(name); ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_BAD_DELIMITER"))); break;
#else
        case MUSTACH_ERROR_BAD_SEPARATORS: if (data) free(data); if (name) unlink(name); ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_BAD_SEPARATORS"))); break;
#endif
        case MUSTACH_ERROR_TOO_DEEP: if (data) free(data); if (name) unlink(name); ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_TOO_DEEP"))); break;
        case MUSTACH_ERROR_CLOSING: if (data) free(data); if (name) unlink(name); ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_CLOSING"))); break;
        case MUSTACH_ERROR_BAD_UNESCAPE_TAG: if (data) free(data); if (name) unlink(name); ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_BAD_UNESCAPE_TAG"))); break;
        case MUSTACH_ERROR_INVALID_ITF: if (data) free(data); if (name) unlink(name); ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_INVALID_ITF"))); break;
#if MUSTACH_VERSION >= 200
        case MUSTACH_ERROR_NOT_FOUND: if (data) free(data); if (name) unlink(name); ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_NOT_FOUND"))); break;
#else
        case MUSTACH_ERROR_ITEM_NOT_FOUND: if (data) free(data); if (name) unlink(name); ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_ITEM_NOT_FOUND"))); break;
        case MUSTACH_ERROR_PARTIAL_NOT_FOUND: if (data) free(data); if (name) unlink(name); ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_PARTIAL_NOT_FOUND"))); break;
#endif
        case MUSTACH_ERROR_UNDEFINED_TAG: if (data) free(data); if (name) unlink(name); ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_UNDEFINED_TAG"))); break;
        case MUSTACH_ERROR_TOO_MUCH_NESTING: if (data) free(data); if (name) unlink(name); ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_TOO_MUCH_NESTING"))); break;
#if MUSTACH_VERSION >= 200
        case MUSTACH_ERROR_OUT_OF_MEMORY: if (data) free(data); if (name) unlink(name); ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_OUT_OF_MEMORY"))); break;
#endif
        default: if (data) free(data); if (name) unlink(name); ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("%s", err ? err : "unknown mustach error"))); break;
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
