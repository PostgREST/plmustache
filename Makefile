EXTENSION = plmustache
EXTVERSION = 0.1

DATA = $(wildcard sql/*--*.sql)

MODULE_big = $(EXTENSION)
OBJS = src/plmustache.o src/observation.o

TESTS = $(wildcard test/sql/*.sql)
REGRESS = $(patsubst test/sql/%.sql,%,$(TESTS))
REGRESS_OPTS = --inputdir=test

PG_CONFIG = pg_config
SHLIB_LINK = -lmustach

PG_CFLAGS = -std=c99 -Wno-declaration-after-statement -Wall -Werror -Wshadow

PGXS := $(shell $(PG_CONFIG) --pgxs)

all: sql/$(EXTENSION)--$(EXTVERSION).sql $(EXTENSION).control

.PHONY: clean_generated
clean_generated:
	rm -f $(EXTENSION).control
	rm -f sql/$(EXTENSION)--$(EXTVERSION).sql

# extra dep for clean target in pgxs.mk
clean: clean_generated

sql/$(EXTENSION)--$(EXTVERSION).sql: sql/$(EXTENSION).sql
	cp $< $@

$(EXTENSION).control:
	sed "s/@EXTVERSION@/$(EXTVERSION)/g" $(EXTENSION).control.in > $(EXTENSION).control

include $(PGXS)
