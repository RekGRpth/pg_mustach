#include <stdio.h>

int mustach_process_cjson(const char *template, size_t length, const char *value, size_t buffer_length, int flags, FILE *file, char **err);

#if __has_include("mustach/mustach-cjson.h")
#include "mustach/mustach-cjson.h"

int mustach_process_cjson(const char *template, size_t length, const char *value, size_t buffer_length, int flags, FILE *file, char **err) {
    cJSON *root;
    int rc = MUSTACH_ERROR_USER(1);
    if (!(root = cJSON_ParseWithLength(value, buffer_length))) { *err = (char *)cJSON_GetErrorPtr(); goto ret; }
    rc = mustach_cJSON_file(template, length, root, flags, file);
    cJSON_Delete(root);
ret:
    fclose(file);
    return rc;
}
#else
int mustach_process_cjson(const char *template, size_t length, const char *value, size_t buffer_length, int flags, FILE *file, char **err) {
    *err = "!mustach_cjson";
    fclose(file);
    return MUSTACH_ERROR_USER(1);
}
#endif
