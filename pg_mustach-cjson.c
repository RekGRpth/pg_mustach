#include "include.h"
#include <mustach/mustach-cjson.h>

void *pg_mustach_load_cjson(const char *json) {
    cJSON *root;
    if (!(root = cJSON_Parse(json))) E("!cJSON_Parse");
    return root;
}

int pg_mustach_process_cjson(const char *template, void *root, FILE *file) {
    return mustach_cJSON_file(template, root, Mustach_With_AllExtensions, file);
}

void pg_mustach_close_cjson(void *root) {
    cJSON_Delete(root);
}
