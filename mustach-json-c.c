#include <stdio.h>

int mustach_process_json_c(const char *template, size_t length, const char *str, size_t len, int flags, FILE *file, char **err);

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

int mustach_process_json_c(const char *template, size_t length, const char *str, size_t len, int flags, FILE *file, char **err) {
    enum json_tokener_error error = json_tokener_success;
    int rc = MUSTACH_ERROR_USER(1);
    struct json_object *root;
    if (!(root = json_tokener_parse_verbose_len(str, len, &error))) { *err = (char *)json_tokener_error_desc(error); goto ret; }
    rc = mustach_json_c_file(template, length, root, flags, file);
    json_object_put(root);
ret:
    fclose(file);
    return rc;
}
#else
#include <mustach/mustach.h>

int mustach_process_json_c(const char *template, size_t length, const char *str, size_t len, int flags, FILE *file, char **err) {
    *err = "!mustach_json_c";
    fclose(file);
    return MUSTACH_ERROR_USER(1);
}
#endif
