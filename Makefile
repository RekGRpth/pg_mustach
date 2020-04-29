EXTENSION = pg_mustach
MODULE_big = $(EXTENSION)

PG_CONFIG = pg_config
OBJS = $(EXTENSION).o
DATA = pg_mustach--1.0.sql

LIBS += -lmustach
SHLIB_LINK := $(LIBS)

PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)

$(OBJS): Makefile