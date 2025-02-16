## Our variables
EXTENSION = plmustache
EXTVERSION = 0.1
SED ?= sed

## PGXS variables
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)

MODULE_big = $(EXTENSION)
SRC = $(wildcard src/*.c)
OBJS = $(patsubst src/%.c, src/%.o, $(SRC))
SHLIB_LINK = -lmustach

PG_CFLAGS = -std=c11 -Wextra -Wall -Werror \
	-Wno-declaration-after-statement \
	-Wno-vla \
	-Wno-long-long \
	-Wno-missing-field-initializers
ifeq ($(COVERAGE), 1)
PG_CFLAGS += --coverage
endif

TESTS = $(wildcard test/sql/*.sql)
REGRESS = $(patsubst test/sql/%.sql,%,$(TESTS))
REGRESS_OPTS = --inputdir=test

DATA = $(wildcard *--*.sql)

EXTRA_CLEAN = $(EXTENSION)--$(EXTVERSION).sql $(EXTENSION).control

all: $(EXTENSION)--$(EXTVERSION).sql $(EXTENSION).control

$(EXTENSION)--$(EXTVERSION).sql: sql/$(EXTENSION).sql
	cp $< $@

$(EXTENSION).control:
	$(SED) "s/@EXTVERSION@/$(EXTVERSION)/g" $(EXTENSION).control.in > $@

include $(PGXS)
