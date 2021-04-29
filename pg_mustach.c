#include <postgres.h>

#include <catalog/pg_type.h>
#include <fmgr.h>
extern text *cstring_to_text(const char *s);
extern text *cstring_to_text_with_len(const char *s, int len);
extern char *text_to_cstring(const text *t);
extern void text_to_cstring_buffer(const text *src, char *dst, size_t dst_len);
#define CStringGetTextDatum(s) PointerGetDatum(cstring_to_text(s))
#define TextDatumGetCString(d) text_to_cstring((text *) DatumGetPointer(d))
#include <mustach/mustach.h>
#include <mustach/mustach-json-c.h>

#define EXTENSION(function) Datum (function)(PG_FUNCTION_ARGS); PG_FUNCTION_INFO_V1(function); Datum (function)(PG_FUNCTION_ARGS)

#define FORMAT_0(fmt, ...) "%s(%s:%d): %s", __func__, __FILE__, __LINE__, fmt
#define FORMAT_1(fmt, ...) "%s(%s:%d): " fmt,  __func__, __FILE__, __LINE__
#define GET_FORMAT(fmt, ...) GET_FORMAT_PRIVATE(fmt, 0, ##__VA_ARGS__, 1, \
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
    1, 1, 1, 1, 1, 1, 1, 1, 1, 0)
#define GET_FORMAT_PRIVATE(fmt, \
      _0,  _1,  _2,  _3,  _4,  _5,  _6,  _7,  _8,  _9, \
     _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, \
     _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, \
     _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, \
     _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, \
     _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, \
     _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, \
     _70, format, ...) FORMAT_ ## format(fmt)

#define D1(fmt, ...) ereport(DEBUG1, (errmsg(GET_FORMAT(fmt, ##__VA_ARGS__), ##__VA_ARGS__)))
#define D2(fmt, ...) ereport(DEBUG2, (errmsg(GET_FORMAT(fmt, ##__VA_ARGS__), ##__VA_ARGS__)))
#define D3(fmt, ...) ereport(DEBUG3, (errmsg(GET_FORMAT(fmt, ##__VA_ARGS__), ##__VA_ARGS__)))
#define D4(fmt, ...) ereport(DEBUG4, (errmsg(GET_FORMAT(fmt, ##__VA_ARGS__), ##__VA_ARGS__)))
#define D5(fmt, ...) ereport(DEBUG5, (errmsg(GET_FORMAT(fmt, ##__VA_ARGS__), ##__VA_ARGS__)))
#define E(fmt, ...) ereport(ERROR, (errmsg(GET_FORMAT(fmt, ##__VA_ARGS__), ##__VA_ARGS__)))
#define F(fmt, ...) ereport(FATAL, (errmsg(GET_FORMAT(fmt, ##__VA_ARGS__), ##__VA_ARGS__)))
#define I(fmt, ...) ereport(INFO, (errmsg(GET_FORMAT(fmt, ##__VA_ARGS__), ##__VA_ARGS__)))
#define L(fmt, ...) ereport(LOG, (errmsg(GET_FORMAT(fmt, ##__VA_ARGS__), ##__VA_ARGS__)))
#define N(fmt, ...) ereport(NOTICE, (errmsg(GET_FORMAT(fmt, ##__VA_ARGS__), ##__VA_ARGS__)))
#define W(fmt, ...) ereport(WARNING, (errmsg(GET_FORMAT(fmt, ##__VA_ARGS__), ##__VA_ARGS__)))

PG_MODULE_MAGIC;

EXTENSION(pg_mustach) {
    char *json;
    char *output_data;
    char *template;
    enum json_tokener_error error;
    FILE *file;
    size_t output_len;
    struct json_object *root;
    text *output;
    if (PG_ARGISNULL(0)) E("json is null!");
    if (PG_ARGISNULL(1)) E("template is null!");
    json = TextDatumGetCString(PG_GETARG_DATUM(0));
    template = TextDatumGetCString(PG_GETARG_DATUM(1));
    if (!(root = json_tokener_parse_verbose(json, &error))) E("!json_tokener_parse and %s", json_tokener_error_desc(error));
    switch (PG_NARGS()) {
        case 2: if (!(file = open_memstream(&output_data, &output_len))) E("!open_memstream"); break;
        case 3: {
            char *name;
            if (PG_ARGISNULL(2)) E("file is null!");
            name = TextDatumGetCString(PG_GETARG_DATUM(2));
            if (!(file = fopen(name, "wb"))) E("!fopen");
            pfree(name);
        } break;
        default: E("expect be 2 or 3 args");
    }
    switch (mustach_json_c_file(template, root, -1, file)) {
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
        default: E("mustach_json_c_file"); break;
    }
    pfree(json);
    pfree(template);
    if (!json_object_put(root)) E("!json_object_put");
    fclose(file);
    switch (PG_NARGS()) {
        case 2:
            output = cstring_to_text_with_len(output_data, output_len);
            free(output_data);
            PG_RETURN_TEXT_P(output);
            break;
        case 3: PG_RETURN_BOOL(true); break;
        default: E("expect be 2 or 3 args");
    }
}
