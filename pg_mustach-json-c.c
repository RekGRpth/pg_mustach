#include "include.h"
#include <mustach/mustach-json-c.h>

void *pg_mustach_load_json_c(const char *json) {
    enum json_tokener_error error;
    struct json_object *root;
    if (!(root = json_tokener_parse_verbose(json, &error))) E("!json_tokener_parse and %s", json_tokener_error_desc(error));
    return root;
}

int pg_mustach_process_json_c(const char *template, void *root, FILE *file) {
    return mustach_json_c_file(template, root, Mustach_With_AllExtensions, file);
}

void pg_mustach_close_json_c(void *root) {
    if (!json_object_put(root)) E("!json_object_put");
}
