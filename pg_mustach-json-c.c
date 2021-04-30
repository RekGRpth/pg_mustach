#include "include.h"
#include <mustach/mustach-json-c.h>

int pg_mustach_process_json_c(const char *template, const char *str, size_t len, FILE *file) {
    enum json_tokener_error error;
    int rc;
    struct json_object *root;
    struct json_tokener *tok;
    if (!(tok = json_tokener_new())) E("!json_tokener_new");
    do root = json_tokener_parse_ex(tok, str, len); while ((error = json_tokener_get_error(tok)) == json_tokener_continue);
    if (error != json_tokener_success) E("!json_tokener_parse_ex and %s", json_tokener_error_desc(error));
    if (json_tokener_get_parse_end(tok) < len) E("json_tokener_get_parse_end < %li", len);
    json_tokener_free(tok);
    rc = mustach_json_c_file(template, root, Mustach_With_AllExtensions, file);
    if (!json_object_put(root)) E("!json_object_put");
    return rc;
}
