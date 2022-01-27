#include <stdio.h>

int mustach_process_jansson(const char *template, size_t length, const char *buffer, size_t buflen, int flags, FILE *file, char **err);

#if __has_include("mustach/mustach-jansson.h")
#include "mustach/mustach-jansson.h"

int mustach_process_jansson(const char *template, size_t length, const char *buffer, size_t buflen, int flags, FILE *file, char **err) {
    int rc = MUSTACH_ERROR_USER(1);
    json_error_t error;
    json_t *root;
    if (!(root = json_loadb(buffer, buflen, JSON_DECODE_ANY, &error))) { *err = (char *)error.text; goto ret; }
    rc = mustach_jansson_file(template, length, root, flags, file);
    json_decref(root);
ret:
    fclose(file);
    return rc;
}
#else
int mustach_process_jansson(const char *template, size_t length, const char *buffer, size_t buflen, int flags, FILE *file, char **err) {
    *err = "!mustach_jansson";
    fclose(file);
    return MUSTACH_ERROR_USER(1);
}
#endif
