#include "pg_mustach.h"

#if __has_include("mustach/mustach-jansson.h")
#include "mustach/mustach-jansson.h"

int pg_mustach_process_jansson(const char *template, size_t length, const char *buffer, size_t buflen, FILE *file) {
    int rc;
    json_error_t error;
    json_t *root;
    if (!(root = json_loadb(buffer, buflen, JSON_DECODE_ANY, &error))) ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("!json_loadb and %s", error.text)));
    rc = mustach_jansson_file(template, length, root, Mustach_With_AllExtensions | Mustach_With_ErrorUndefined, file);
    json_decref(root);
    return rc;
}
#else
int pg_mustach_process_jansson(const char *template, size_t length, const char *buffer, size_t buflen, FILE *file) {
    ereport(ERROR, (errcode(ERRCODE_FEATURE_NOT_SUPPORTED), errmsg("!mustach_jansson")));
}
#endif
