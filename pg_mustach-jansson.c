#include "include.h"
#include <mustach/mustach-jansson.h>

void *pg_mustach_load_jansson(const char *json) {
    json_error_t error;
    json_t *root;
    if (!(root = json_loads(json, 0, &error))) E("!json_loads and %s", error.text);
    return root;
}

int pg_mustach_process_jansson(const char *template, void *root, FILE *file) {
    return mustach_jansson_file(template, root, Mustach_With_AllExtensions, file);
}

void pg_mustach_close_jansson(void *root) {
    json_decref(root);
}