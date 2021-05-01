#include "include.h"
#include <mustach/mustach-jansson.h>

int pg_mustach_process_jansson(const char *template, size_t length, const char *buffer, size_t buflen, FILE *file) {
    int rc;
    json_error_t error;
    json_t *root;
    if (!(root = json_loadb(buffer, buflen, JSON_DECODE_ANY, &error))) E("!json_loadb and %s", error.text);
    rc = mustach_jansson_file(template, length, root, Mustach_With_AllExtensions, file);
    json_decref(root);
    return rc;
}
