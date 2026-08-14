$(OBJS): Makefile
DATA = pg_mustach--1.0.sql pg_mustach--1.0--2.0.sql pg_mustach--2.0.sql pg_mustach--2.0--3.0.sql pg_mustach--3.0.sql pg_mustach--3.0--4.0.sql pg_mustach--4.0.sql
EXTENSION = pg_mustach
MODULE_big = $(EXTENSION)
OBJS = $(EXTENSION).o mustach-jsonb.o pg_whitelist/pg_whitelist.o
PG_CONFIG = pg_config
PG_CPPFLAGS = -Ipg_whitelist
PGXS = $(shell $(PG_CONFIG) --pgxs)
REGRESS = $(patsubst sql/%.sql,%,$(TESTS))
SHLIB_LINK = -lmustach
TESTS = $(wildcard sql/*.sql)
include $(PGXS)
