EXTENSION = pg_mustach
MODULE_big = $(EXTENSION)

PG_CONFIG = pg_config
OBJS = $(EXTENSION).o pg_mustach-cjson.o pg_mustach-jansson.o pg_mustach-json-c.o
DATA = pg_mustach--1.0.sql

LIBS += -lmustach -lcjson -ljansson -ljson-c
SHLIB_LINK := $(LIBS)

PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)

$(OBJS): Makefile