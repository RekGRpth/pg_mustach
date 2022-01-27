#ifndef _PG_MUSTACH_H
#define _PG_MUSTACH_H

#include <postgres.h>

int pg_mustach_process_cjson(const char *template, size_t length, const char *value, size_t buffer_length, int flags, FILE *file);
int pg_mustach_process_jansson(const char *template, size_t length, const char *buffer, size_t buflen, int flags, FILE *file);
int pg_mustach_process_json_c(const char *template, size_t length, const char *str, size_t len, int flags, FILE *file);

#endif // _PG_MUSTACH_H
