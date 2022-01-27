#include "pg_mustach.h"

#if __has_include("mustach/mustach-json-c.h")
#include "mustach/mustach-json-c.h"

static struct json_object *json_tokener_parse_verbose_len(const char *str, size_t len, enum json_tokener_error *error) {
    struct json_tokener *tok;
    struct json_object *obj;
    if (!(tok = json_tokener_new())) return NULL;
    obj = json_tokener_parse_ex(tok, str, len);
    *error = tok->err;
    if (tok->err != json_tokener_success || json_tokener_get_parse_end(tok) != len) {
        if (obj) json_object_put(obj);
        obj = NULL;
    }
    json_tokener_free(tok);
    return obj;
}

int pg_mustach_process_json_c(const char *template, size_t length, const char *str, size_t len, int flags, FILE *file) {
    enum json_tokener_error error = json_tokener_success;
    int rc;
    struct json_object *root;
    if (!(root = json_tokener_parse_verbose_len(str, len, &error))) { fclose(file); ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("!json_tokener_parse_verbose_len"), errdetail("%s", json_tokener_error_desc(error)))); }
    rc = mustach_json_c_file(template, length, root, flags, file);
    json_object_put(root);
    fclose(file);
    return rc;
}
#else
int pg_mustach_process_json_c(const char *template, size_t length, const char *str, size_t len, int flags, FILE *file) {
    fclose(file);
    ereport(ERROR, (errcode(ERRCODE_FEATURE_NOT_SUPPORTED), errmsg("!mustach_json_c")));
}
#endif
