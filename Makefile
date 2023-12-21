EXTENSION = plmustache
EXTVERSION = 0.1

DATA = $(wildcard sql/*--*.sql)

MODULE_big = $(EXTENSION)
OBJS = src/plmustache.o

TESTS = $(wildcard test/sql/*.sql)
REGRESS = $(patsubst test/sql/%.sql,%,$(TESTS))
REGRESS_OPTS = --inputdir=test

PG_CONFIG = pg_config
SHLIB_LINK = -lmustach

PG_CFLAGS = -std=c99 -Wno-declaration-after-statement -Wall -Werror -Wshadow

PGXS := $(shell $(PG_CONFIG) --pgxs)

all: sql/$(EXTENSION)--$(EXTVERSION).sql $(EXTENSION).control

sql/$(EXTENSION)--$(EXTVERSION).sql: sql/$(EXTENSION).sql
	cp $< $@

$(EXTENSION).control:
	sed "s/@EXTVERSION@/$(EXTVERSION)/g" $(EXTENSION).control.in > $(EXTENSION).control

include $(PGXS)
