#include <postgres.h>

#include <catalog/pg_type.h>
extern text *cstring_to_text(const char *s);
extern text *cstring_to_text_with_len(const char *s, int len);
extern char *text_to_cstring(const text *t);
extern void text_to_cstring_buffer(const text *src, char *dst, size_t dst_len);
#define CStringGetTextDatum(s) PointerGetDatum(cstring_to_text(s))
#define TextDatumGetCString(d) text_to_cstring((text *) DatumGetPointer(d))
#include <mustach/mustach-json-c.h>

#define EXTENSION(function) Datum (function)(PG_FUNCTION_ARGS); PG_FUNCTION_INFO_V1(function); Datum (function)(PG_FUNCTION_ARGS)

PG_MODULE_MAGIC;


EXTENSION(json2mustach) {
    char *json;
    char *template;
    struct json_object *object;
    if (PG_ARGISNULL(0)) ereport(ERROR, (errmsg("json is null!")));
    if (PG_ARGISNULL(1)) ereport(ERROR, (errmsg("template is null!")));
    json = TextDatumGetCString(PG_GETARG_DATUM(0));
    template = TextDatumGetCString(PG_GETARG_DATUM(1));
    if (!(object = json_tokener_parse(json))) ereport(ERROR, (errmsg("!json_tokener_parse")));
    switch (PG_NARGS()) {
        case 3: {
            char *file;
            FILE *out;
            if (PG_ARGISNULL(2)) ereport(ERROR, (errmsg("file is null!")));
            file = TextDatumGetCString(PG_GETARG_DATUM(2));
            if (!(out = fopen(file, "wb"))) ereport(ERROR, (errmsg("!fopen")));
            pfree(file);
            if (fmustach_json_c(template, object, out)) ereport(ERROR, (errmsg("fmustach_json_c")));
            PG_RETURN_BOOL(true);
        } break;
        case 2: {
            char *result;
            size_t size;
            text *res;
            if (mustach_json_c(template, object, &result, &size)) ereport(ERROR, (errmsg("mustach_json_c")));
            res = cstring_to_text_with_len(result, size);
            free(result);
            PG_RETURN_TEXT_P(res);
        } break;
        default: ereport(ERROR, (errmsg("PG_NARGS must be 2 or 3")));
    }
    pfree(json);
    pfree(template);
    if (!json_object_put(object)) ereport(ERROR, (errmsg("!json_object_put")));
}
