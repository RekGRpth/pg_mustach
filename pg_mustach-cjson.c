#include "pg_mustach.h"

#if __has_include("mustach/mustach-cjson.h")
#include "mustach/mustach-cjson.h"

int pg_mustach_process_cjson(const char *template, size_t length, const char *value, size_t buffer_length, FILE *file) {
    cJSON *root;
    int rc;
    if (!(root = cJSON_ParseWithLength(value, buffer_length))) E("!cJSON_ParseWithLength");
    rc = mustach_cJSON_file(template, length, root, Mustach_With_AllExtensions, file);
    cJSON_Delete(root);
    return rc;
}
#else
int pg_mustach_process_cjson(const char *template, size_t length, const char *value, size_t buffer_length, FILE *file) {
    E("!mustach_cjson");
}
#endif
