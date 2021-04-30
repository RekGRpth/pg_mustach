#include "include.h"
#include <mustach/mustach-json-c.h>

void *pg_mustach_load_json_c(const char *str, size_t len) {
    enum json_tokener_error error;
    struct json_object *root;
    struct json_tokener *tok;
    if (!(tok = json_tokener_new())) E("!json_tokener_new");
    do root = json_tokener_parse_ex(tok, str, len); while ((error = json_tokener_get_error(tok)) == json_tokener_continue);
    if (error != json_tokener_success) E("!json_tokener_parse_ex and %s", json_tokener_error_desc(error));
    if (json_tokener_get_parse_end(tok) < len) E("json_tokener_get_parse_end < %li", len);
    json_tokener_free(tok);
    return root;
}

int pg_mustach_process_json_c(const char *template, void *root, FILE *file) {
    return mustach_json_c_file(template, root, Mustach_With_AllExtensions, file);
}

void pg_mustach_close_json_c(void *root) {
    if (!json_object_put(root)) E("!json_object_put");
}
