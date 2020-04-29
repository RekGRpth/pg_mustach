#include <postgres.h>

#include <catalog/pg_type.h>
#include <utils/builtins.h>

#include <mustach-json-c.h>

#define EXTENSION(function) Datum (function)(PG_FUNCTION_ARGS); PG_FUNCTION_INFO_V1(function); Datum (function)(PG_FUNCTION_ARGS)

PG_MODULE_MAGIC;

EXTENSION(text2mustach) {
    char *data;
    enum json_tokener_error jerr;
    struct json_object *jobj;
    struct json_tokener *tok;
    text *json;
    if (PG_ARGISNULL(0)) ereport(ERROR, (errmsg("json is null!")));
    if (PG_ARGISNULL(1)) ereport(ERROR, (errmsg("data is null!")));
    json = DatumGetTextP(PG_GETARG_DATUM(0));
    data = TextDatumGetCString(PG_GETARG_DATUM(1));
    if (!(tok = json_tokener_new())) ereport(ERROR, (errmsg("!json_tokener_new")));
    if (!(jobj = json_tokener_parse_ex(tok, VARDATA_ANY(json), VARSIZE_ANY_EXHDR(json)))) ereport(ERROR, (errmsg("!json_tokener_parse_ex")));
    if ((jerr = json_tokener_get_error(tok) != json_tokener_success) ereport(ERROR, (errmsg("json_tokener_get_error = %s", json_tokener_error_desc(jerr))));
    switch (PG_NARGS()) {
        case 2: {
            char *file;
            FILE *out;
            if (PG_ARGISNULL(2)) ereport(ERROR, (errmsg("file is null!")));
            file = TextDatumGetCString(PG_GETARG_DATUM(2));
            if (!(out = fopen(file, "wb"))) ereport(ERROR, (errmsg("!fopen")));
            pfree(file);
            if (fmustach_json_c(data, jobj, out)) ereport(ERROR, (errmsg("fmustach_json_c")));
            PG_RETURN_BOOL(true);
        } break;
        case 3: {
            char *result;
            size_t size;
            if (mustach_json_c(data, jobj, &result, &size)) ereport(ERROR, (errmsg("mustach_json_c")));
            text *res = cstring_to_text_with_len(result, size);
            free(result);
            PG_RETURN_TEXT_P(res);
        } break;
        default: ereport(ERROR, (errmsg("PG_NARGS must be 2 or 3")));
    }
    pfree(data);
    json_tokener_free(tok);
}
