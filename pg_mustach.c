#include "include.h"

#include <catalog/pg_type.h>
#include <fmgr.h>
#include <mustach/mustach.h>
#include <utils/builtins.h>

#define EXTENSION(function) Datum (function)(PG_FUNCTION_ARGS); PG_FUNCTION_INFO_V1(function); Datum (function)(PG_FUNCTION_ARGS)

PG_MODULE_MAGIC;

static Datum pg_mustach_internal(FunctionCallInfo fcinfo, void *(*pg_mustach_load)(const char *data, size_t len), int (*pg_mustach_process)(const char *template, void *root, FILE *file), void (*pg_mustach_close)(void *root)) {
    char *data;
    char *template;
    FILE *file;
    size_t len;
    text *json;
    text *output;
    void *root;
    if (PG_ARGISNULL(0)) E("json is null!");
    if (PG_ARGISNULL(1)) E("template is null!");
    json = DatumGetTextP(PG_GETARG_DATUM(0));
    template = TextDatumGetCString(PG_GETARG_DATUM(1));
    root = pg_mustach_load(VARDATA_ANY(json), VARSIZE_ANY_EXHDR(json));
    switch (PG_NARGS()) {
        case 2: if (!(file = open_memstream(&data, &len))) E("!open_memstream"); break;
        case 3: {
            char *name;
            if (PG_ARGISNULL(2)) E("file is null!");
            name = TextDatumGetCString(PG_GETARG_DATUM(2));
            if (!(file = fopen(name, "wb"))) E("!fopen");
            pfree(name);
        } break;
        default: E("expect be 2 or 3 args");
    }
    switch (pg_mustach_process(template, root, file)) {
        case MUSTACH_OK: break;
        case MUSTACH_ERROR_SYSTEM: E("MUSTACH_ERROR_SYSTEM"); break;
        case MUSTACH_ERROR_UNEXPECTED_END: E("MUSTACH_ERROR_UNEXPECTED_END"); break;
        case MUSTACH_ERROR_EMPTY_TAG: E("MUSTACH_ERROR_EMPTY_TAG"); break;
        case MUSTACH_ERROR_TAG_TOO_LONG: E("MUSTACH_ERROR_TAG_TOO_LONG"); break;
        case MUSTACH_ERROR_BAD_SEPARATORS: E("MUSTACH_ERROR_BAD_SEPARATORS"); break;
        case MUSTACH_ERROR_TOO_DEEP: E("MUSTACH_ERROR_TOO_DEEP"); break;
        case MUSTACH_ERROR_CLOSING: E("MUSTACH_ERROR_CLOSING"); break;
        case MUSTACH_ERROR_BAD_UNESCAPE_TAG: E("MUSTACH_ERROR_BAD_UNESCAPE_TAG"); break;
        case MUSTACH_ERROR_INVALID_ITF: E("MUSTACH_ERROR_INVALID_ITF"); break;
        case MUSTACH_ERROR_ITEM_NOT_FOUND: E("MUSTACH_ERROR_ITEM_NOT_FOUND"); break;
        case MUSTACH_ERROR_PARTIAL_NOT_FOUND: E("MUSTACH_ERROR_PARTIAL_NOT_FOUND"); break;
        default: E("pg_mustach_process"); break;
    }
    pfree(template);
    pg_mustach_close(root);
    fclose(file);
    switch (PG_NARGS()) {
        case 2:
            output = cstring_to_text_with_len(data, len);
            free(data);
            PG_RETURN_TEXT_P(output);
            break;
        case 3: PG_RETURN_BOOL(true); break;
        default: E("expect be 2 or 3 args");
    }
}

EXTENSION(pg_mustach) { return pg_mustach_internal(fcinfo, pg_mustach_load_json_c, pg_mustach_process_json_c, pg_mustach_close_json_c); }

EXTENSION(pg_mustach_cjson) { return pg_mustach_internal(fcinfo, pg_mustach_load_cjson, pg_mustach_process_cjson, pg_mustach_close_cjson); }
EXTENSION(pg_mustach_jansson) { return pg_mustach_internal(fcinfo, pg_mustach_load_jansson, pg_mustach_process_jansson, pg_mustach_close_jansson); }
EXTENSION(pg_mustach_json_c) { return pg_mustach_internal(fcinfo, pg_mustach_load_json_c, pg_mustach_process_json_c, pg_mustach_close_json_c); }
