#include "pg_mustach.h"

#include <catalog/pg_type.h>
#include <fmgr.h>
#include <mustach/mustach.h>
#include <utils/builtins.h>

#define EXTENSION(function) Datum (function)(PG_FUNCTION_ARGS); PG_FUNCTION_INFO_V1(function); Datum (function)(PG_FUNCTION_ARGS)

PG_MODULE_MAGIC;

static Datum pg_mustach(FunctionCallInfo fcinfo, int (*pg_mustach_process)(const char *template, size_t length, const char *data, size_t len, FILE *file)) {
    char *data;
    FILE *file;
    size_t len;
    text *json;
    text *output;
    text *template;
    if (PG_ARGISNULL(0)) ereport(ERROR, (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED), errmsg("handlebars requires argument json")));
    if (PG_ARGISNULL(1)) ereport(ERROR, (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED), errmsg("handlebars requires argument template")));
    json = DatumGetTextP(PG_GETARG_DATUM(0));
    template = DatumGetTextP(PG_GETARG_DATUM(1));
    switch (PG_NARGS()) {
        case 2: if (!(file = open_memstream(&data, &len)))ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("!open_memstream"))); break;
        case 3: {
            char *name;
            if (PG_ARGISNULL(2)) ereport(ERROR, (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED), errmsg("handlebars requires argument file")));
            name = TextDatumGetCString(PG_GETARG_DATUM(2));
            if (!(file = fopen(name, "wb"))) ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("!fopen")));
            pfree(name);
        } break;
        default: ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("expect be 2 or 3 args")));
    }
    switch (pg_mustach_process(VARDATA_ANY(template), VARSIZE_ANY_EXHDR(template), VARDATA_ANY(json), VARSIZE_ANY_EXHDR(json), file)) {
        case MUSTACH_OK: break;
        case MUSTACH_ERROR_SYSTEM: ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_SYSTEM"))); break;
        case MUSTACH_ERROR_UNEXPECTED_END: ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_UNEXPECTED_END"))); break;
        case MUSTACH_ERROR_EMPTY_TAG: ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_EMPTY_TAG"))); break;
        case MUSTACH_ERROR_TAG_TOO_LONG: ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_TAG_TOO_LONG"))); break;
        case MUSTACH_ERROR_BAD_SEPARATORS: ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_BAD_SEPARATORS"))); break;
        case MUSTACH_ERROR_TOO_DEEP: ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_TOO_DEEP"))); break;
        case MUSTACH_ERROR_CLOSING: ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_CLOSING"))); break;
        case MUSTACH_ERROR_BAD_UNESCAPE_TAG: ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_BAD_UNESCAPE_TAG"))); break;
        case MUSTACH_ERROR_INVALID_ITF: ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_INVALID_ITF"))); break;
        case MUSTACH_ERROR_ITEM_NOT_FOUND: ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_ITEM_NOT_FOUND"))); break;
        case MUSTACH_ERROR_PARTIAL_NOT_FOUND: ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("MUSTACH_ERROR_PARTIAL_NOT_FOUND"))); break;
        default: ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("pg_mustach_process"))); break;
    }
    fclose(file);
    switch (PG_NARGS()) {
        case 2:
            output = cstring_to_text_with_len(data, len);
            free(data);
            PG_RETURN_TEXT_P(output);
            break;
        case 3: PG_RETURN_BOOL(true); break;
        default: ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("expect be 2 or 3 args")));
    }
}

EXTENSION(pg_mustach_cjson) { return pg_mustach(fcinfo, pg_mustach_process_cjson); }
EXTENSION(pg_mustach_jansson) { return pg_mustach(fcinfo, pg_mustach_process_jansson); }
EXTENSION(pg_mustach_json_c) { return pg_mustach(fcinfo, pg_mustach_process_json_c); }
