#include <postgres.h>

#include <catalog/pg_type.h>
#include <fmgr.h>

#include <utils/builtins.h>
#if PG_VERSION_NUM >= 160000
#include <varatt.h>
#endif

#include <mustach/mustach.h>
#include <mustach/mustach-wrap.h>

#define EXTENSION(function) Datum (function)(PG_FUNCTION_ARGS); PG_FUNCTION_INFO_V1(function); Datum (function)(PG_FUNCTION_ARGS)

int mustach_process_cjson(const char *template, size_t length, const char *value, size_t buffer_length, int flags, FILE *file, char **err);
int mustach_process_jansson(const char *template, size_t length, const char *buffer, size_t buflen, int flags, FILE *file, char **err);
int mustach_process_json_c(const char *template, size_t length, const char *str, size_t len, int flags, FILE *file, char **err);

PG_MODULE_MAGIC;

static int flags = Mustach_With_AllExtensions;

EXTENSION(pg_mustach_with_allextensions) { flags |= Mustach_With_AllExtensions; PG_RETURN_NULL(); }
EXTENSION(pg_mustach_with_colon) { flags |= Mustach_With_Colon; PG_RETURN_NULL(); }
EXTENSION(pg_mustach_with_compare) { flags |= Mustach_With_Compare; PG_RETURN_NULL(); }
EXTENSION(pg_mustach_with_emptytag) { flags |= Mustach_With_EmptyTag; PG_RETURN_NULL(); }
EXTENSION(pg_mustach_with_equal) { flags |= Mustach_With_Equal; PG_RETURN_NULL(); }
EXTENSION(pg_mustach_with_errorundefined) { flags |= Mustach_With_ErrorUndefined; PG_RETURN_NULL(); }
EXTENSION(pg_mustach_with_escfirstcmp) { flags |= Mustach_With_EscFirstCmp; PG_RETURN_NULL(); }
EXTENSION(pg_mustach_with_incpartial) { flags |= Mustach_With_IncPartial; PG_RETURN_NULL(); }
EXTENSION(pg_mustach_with_jsonpointer) { flags |= Mustach_With_JsonPointer; PG_RETURN_NULL(); }
EXTENSION(pg_mustach_with_noextensions) { flags = Mustach_With_NoExtensions; PG_RETURN_NULL(); }
EXTENSION(pg_mustach_with_objectiter) { flags |= Mustach_With_ObjectIter; PG_RETURN_NULL(); }
EXTENSION(pg_mustach_with_partialdatafirst) { flags |= Mustach_With_PartialDataFirst; PG_RETURN_NULL(); }
EXTENSION(pg_mustach_with_singledot) { flags |= Mustach_With_SingleDot; PG_RETURN_NULL(); }

static Datum pg_mustach(FunctionCallInfo fcinfo, int (*pg_mustach_process)(const char *template, size_t length, const char *data, size_t len, int flags, FILE *file, char **err)) {
    char *data = NULL;
    char *err;
    FILE *file;
    size_t len;
    text *json;
    text *output;
    text *template;
    if (PG_ARGISNULL(0)) ereport(ERROR, (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED), errmsg("mustach requires argument json")));
    if (PG_ARGISNULL(1)) ereport(ERROR, (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED), errmsg("mustach requires argument template")));
    json = PG_GETARG_TEXT_PP(0);
    template = PG_GETARG_TEXT_PP(1);
    switch (PG_NARGS()) {
        case 2: {
            if (!(file = open_memstream(&data, &len))) ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("!open_memstream")));
        } break;
        case 3: {
            char *name;
            if (PG_ARGISNULL(2)) ereport(ERROR, (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED), errmsg("mustach requires argument file")));
            name = TextDatumGetCString(PG_GETARG_DATUM(2));
            if (!(file = fopen(name, "wb"))) ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("!fopen")));
            pfree(name);
        } break;
        default: ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("expect be 2 or 3 args")));
    }
    switch (pg_mustach_process(VARDATA_ANY(template), VARSIZE_ANY_EXHDR(template), VARDATA_ANY(json), VARSIZE_ANY_EXHDR(json), flags, file, &err)) {
        case MUSTACH_OK: break;
        case MUSTACH_ERROR_SYSTEM: if (data) free(data); ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_SYSTEM"))); break;
        case MUSTACH_ERROR_UNEXPECTED_END: if (data) free(data); ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_UNEXPECTED_END"))); break;
        case MUSTACH_ERROR_EMPTY_TAG: if (data) free(data); ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_EMPTY_TAG"))); break;
#if MUSTACH_VERSION >= 200
        case MUSTACH_ERROR_TOO_BIG: if (data) free(data); ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_TOO_BIG"))); break;
#else
        case MUSTACH_ERROR_TAG_TOO_LONG: if (data) free(data); ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_TAG_TOO_LONG"))); break;
#endif
#if MUSTACH_VERSION >= 200
        case MUSTACH_ERROR_BAD_DELIMITER: if (data) free(data); ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_BAD_DELIMITER"))); break;
#else
        case MUSTACH_ERROR_BAD_SEPARATORS: if (data) free(data); ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_BAD_SEPARATORS"))); break;
#endif
        case MUSTACH_ERROR_TOO_DEEP: if (data) free(data); ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_TOO_DEEP"))); break;
        case MUSTACH_ERROR_CLOSING: if (data) free(data); ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_CLOSING"))); break;
        case MUSTACH_ERROR_BAD_UNESCAPE_TAG: if (data) free(data); ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_BAD_UNESCAPE_TAG"))); break;
        case MUSTACH_ERROR_INVALID_ITF: if (data) free(data); ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_INVALID_ITF"))); break;
#if MUSTACH_VERSION >= 200
        case MUSTACH_ERROR_NOT_FOUND: if (data) free(data); ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_NOT_FOUND"))); break;
#else
        case MUSTACH_ERROR_ITEM_NOT_FOUND: if (data) free(data); ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_ITEM_NOT_FOUND"))); break;
        case MUSTACH_ERROR_PARTIAL_NOT_FOUND: if (data) free(data); ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_PARTIAL_NOT_FOUND"))); break;
#endif
        case MUSTACH_ERROR_UNDEFINED_TAG: if (data) free(data); ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_UNDEFINED_TAG"))); break;
        case MUSTACH_ERROR_TOO_MUCH_NESTING: if (data) free(data); ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_TOO_MUCH_NESTING"))); break;
#if MUSTACH_VERSION >= 200
        case MUSTACH_ERROR_OUT_OF_MEMORY: if (data) free(data); ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_OUT_OF_MEMORY"))); break;
#endif
        default: if (data) free(data); ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("%s", err))); break;
    }
    PG_FREE_IF_COPY(json, 0);
    PG_FREE_IF_COPY(template, 1);
    switch (PG_NARGS()) {
        case 2:
            output = cstring_to_text_with_len(data, len);
            free(data);
            PG_RETURN_TEXT_P(output);
        case 3: PG_RETURN_BOOL(true);
        default: ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("expect be 2 or 3 args")));
    }
}

EXTENSION(pg_mustach_cjson) { return pg_mustach(fcinfo, mustach_process_cjson); }
EXTENSION(pg_mustach_jansson) { return pg_mustach(fcinfo, mustach_process_jansson); }
EXTENSION(pg_mustach_json_c) { return pg_mustach(fcinfo, mustach_process_json_c); }
