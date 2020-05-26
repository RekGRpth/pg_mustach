#include <postgres.h>

#include <catalog/pg_type.h>
extern text *cstring_to_text(const char *s);
extern text *cstring_to_text_with_len(const char *s, int len);
extern char *text_to_cstring(const text *t);
extern void text_to_cstring_buffer(const text *src, char *dst, size_t dst_len);
#define CStringGetTextDatum(s) PointerGetDatum(cstring_to_text(s))
#define TextDatumGetCString(d) text_to_cstring((text *) DatumGetPointer(d))
#include <mustach/mustach.h>
#include <mustach/mustach-json-c.h>

#define EXTENSION(function) Datum (function)(PG_FUNCTION_ARGS); PG_FUNCTION_INFO_V1(function); Datum (function)(PG_FUNCTION_ARGS)

PG_MODULE_MAGIC;

EXTENSION(json2mustach) {
    char *file;
    char *json;
    char *output_data;
    char *template;
    enum json_tokener_error error;
    FILE *out;
    size_t output_len;
    struct json_object *object;
    text *output;
    if (PG_ARGISNULL(0)) ereport(ERROR, (errmsg("json is null!")));
    if (PG_ARGISNULL(1)) ereport(ERROR, (errmsg("template is null!")));
    json = TextDatumGetCString(PG_GETARG_DATUM(0));
    template = TextDatumGetCString(PG_GETARG_DATUM(1));
    if (!(object = json_tokener_parse_verbose(json, &error))) ereport(ERROR, (errmsg("!json_tokener_parse and %s", json_tokener_error_desc(error))));
    switch (PG_NARGS()) {
        case 2: if (!(out = open_memstream(&output_data, &output_len))) ereport(ERROR, (errmsg("!open_memstream"))); break;
        case 3:
            if (PG_ARGISNULL(2)) ereport(ERROR, (errmsg("file is null!")));
            file = TextDatumGetCString(PG_GETARG_DATUM(2));
            if (!(out = fopen(file, "wb"))) ereport(ERROR, (errmsg("!fopen")));
            pfree(file);
            break;
        default: ereport(ERROR, (errmsg("expect be 2 or 3 args")));
    }
    switch (fmustach_json_c(template, object, out)) {
        case MUSTACH_OK: break;
        case MUSTACH_ERROR_SYSTEM: ereport(ERROR, (errmsg("MUSTACH_ERROR_SYSTEM"))); break;
        case MUSTACH_ERROR_UNEXPECTED_END: ereport(ERROR, (errmsg("MUSTACH_ERROR_UNEXPECTED_END"))); break;
        case MUSTACH_ERROR_EMPTY_TAG: ereport(ERROR, (errmsg("MUSTACH_ERROR_EMPTY_TAG"))); break;
        case MUSTACH_ERROR_TAG_TOO_LONG: ereport(ERROR, (errmsg("MUSTACH_ERROR_TAG_TOO_LONG"))); break;
        case MUSTACH_ERROR_BAD_SEPARATORS: ereport(ERROR, (errmsg("MUSTACH_ERROR_BAD_SEPARATORS"))); break;
        case MUSTACH_ERROR_TOO_DEEP: ereport(ERROR, (errmsg("MUSTACH_ERROR_TOO_DEEP"))); break;
        case MUSTACH_ERROR_CLOSING: ereport(ERROR, (errmsg("MUSTACH_ERROR_CLOSING"))); break;
        case MUSTACH_ERROR_BAD_UNESCAPE_TAG: ereport(ERROR, (errmsg("MUSTACH_ERROR_BAD_UNESCAPE_TAG"))); break;
        case MUSTACH_ERROR_INVALID_ITF: ereport(ERROR, (errmsg("MUSTACH_ERROR_INVALID_ITF"))); break;
        case MUSTACH_ERROR_ITEM_NOT_FOUND: ereport(ERROR, (errmsg("MUSTACH_ERROR_ITEM_NOT_FOUND"))); break;
        case MUSTACH_ERROR_PARTIAL_NOT_FOUND: ereport(ERROR, (errmsg("MUSTACH_ERROR_PARTIAL_NOT_FOUND"))); break;
        default: ereport(ERROR, (errmsg("fmustach_json_c"))); break;
    }
    pfree(json);
    pfree(template);
    if (!json_object_put(object)) ereport(ERROR, (errmsg("!json_object_put")));
    fclose(out);
    switch (PG_NARGS()) {
        case 2:
            output = cstring_to_text_with_len(output_data, output_len);
            free(output_data);
            PG_RETURN_TEXT_P(output);
            break;
        case 3: PG_RETURN_BOOL(true); break;
        default: ereport(ERROR, (errmsg("expect be 2 or 3 args")));
    }
}
