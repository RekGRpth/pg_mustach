#ifndef _PG_MUSTACH_H
#define _PG_MUSTACH_H

#include <postgres.h>

#define D1(...) ereport(DEBUG1, (errmsg(__VA_ARGS__)))
#define D2(...) ereport(DEBUG2, (errmsg(__VA_ARGS__)))
#define D3(...) ereport(DEBUG3, (errmsg(__VA_ARGS__)))
#define D4(...) ereport(DEBUG4, (errmsg(__VA_ARGS__)))
#define D5(...) ereport(DEBUG5, (errmsg(__VA_ARGS__)))
#define E(...) ereport(ERROR, (errmsg(__VA_ARGS__)))
#define F(...) ereport(FATAL, (errmsg(__VA_ARGS__)))
#define I(...) ereport(INFO, (errmsg(__VA_ARGS__)))
#define L(...) ereport(LOG, (errmsg(__VA_ARGS__)))
#define N(...) ereport(NOTICE, (errmsg(__VA_ARGS__)))
#define W(...) ereport(WARNING, (errmsg(__VA_ARGS__)))

int pg_mustach_process_cjson(const char *template, size_t length, const char *value, size_t buffer_length, FILE *file);
int pg_mustach_process_jansson(const char *template, size_t length, const char *buffer, size_t buflen, FILE *file);
int pg_mustach_process_json_c(const char *template, size_t length, const char *str, size_t len, FILE *file);

#endif // _PG_MUSTACH_H
