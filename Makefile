$(OBJS): Makefile
DATA = pg_mustach--1.0.sql
EXTENSION = pg_mustach
MODULE_big = $(EXTENSION)
OBJS = $(EXTENSION).o pg_mustach-cjson.o pg_mustach-jansson.o pg_mustach-json-c.o
PG_CONFIG = pg_config
PGXS = $(shell $(PG_CONFIG) --pgxs)
REGRESS = $(patsubst sql/%.sql,%,$(TESTS))
SHLIB_LINK = -lmustach
TESTS = $(wildcard sql/*.sql)
include $(PGXS)
