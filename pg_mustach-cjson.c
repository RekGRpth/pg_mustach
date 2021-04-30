#include "include.h"
#include <mustach/mustach-cjson.h>

int pg_mustach_process_cjson(const char *template, const char *value, size_t buffer_length, FILE *file) {
    cJSON *root;
    int rc;
    if (!(root = cJSON_ParseWithLength(value, buffer_length))) E("!cJSON_ParseWithLength");
    rc = mustach_cJSON_file(template, root, Mustach_With_AllExtensions, file);
    cJSON_Delete(root);
    return rc;
}
