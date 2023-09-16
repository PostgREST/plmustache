EXTENSION = plmustache

DATA = $(wildcard sql/*--*.sql)

MODULE_big = $(EXTENSION)
OBJS = src/plmustache.o

TESTS = $(wildcard test/sql/*.sql)
REGRESS = $(patsubst test/sql/%.sql,%,$(TESTS))
REGRESS_OPTS = --inputdir=test

PG_CONFIG = pg_config
SHLIB_LINK = -lmustach

PG_CFLAGS = -std=c99 -Wall -Werror -Wshadow

PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
