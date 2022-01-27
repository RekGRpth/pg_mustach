#include "pg_mustach.h"

#if __has_include("mustach/mustach-json-c.h")
#include "mustach/mustach-json-c.h"

int pg_mustach_process_json_c(const char *template, size_t length, const char *str, size_t len, int flags, FILE *file) {
    enum json_tokener_error error;
    int rc;
    struct json_object *root;
    struct json_tokener *tok;
    if (!(tok = json_tokener_new())) ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("!json_tokener_new")));
    do root = json_tokener_parse_ex(tok, str, len); while ((error = json_tokener_get_error(tok)) == json_tokener_continue);
    if (error != json_tokener_success) ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("!json_tokener_parse_ex"), errdetail("%s", json_tokener_error_desc(error))));
    if (json_tokener_get_parse_end(tok) < len) ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("json_tokener_get_parse_end < %li", len)));
    json_tokener_free(tok);
    rc = mustach_json_c_file(template, length, root, flags, file);
    if (!json_object_put(root)) ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("!json_object_put")));
    return rc;
}
#else
int pg_mustach_process_json_c(const char *template, size_t length, const char *str, size_t len, int flags, FILE *file) {
    ereport(ERROR, (errcode(ERRCODE_FEATURE_NOT_SUPPORTED), errmsg("!mustach_json_c")));
}
#endif
