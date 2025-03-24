SRC_DIR = src
BUILD_DIR ?= build

# the `-Wno`s quiet C90 warnings
PG_CFLAGS = -std=c11 -Wextra -Wall -Werror \
	-Wno-declaration-after-statement \
	-Wno-vla \
	-Wno-long-long \
	-Wno-missing-field-initializers
ifeq ($(COVERAGE), 1)
PG_CFLAGS += --coverage
endif

EXTENSION = plmustache
EXTVERSION = 0.1
SED ?= sed

DATA = $(wildcard sql/*--*.sql)

EXTRA_CLEAN = sql/$(EXTENSION)--$(EXTVERSION).sql $(EXTENSION).control

TESTS = $(wildcard test/sql/*.sql)
REGRESS = $(patsubst test/sql/%.sql,%,$(TESTS))
REGRESS_OPTS = --inputdir=test

MODULE_big = $(EXTENSION)
SRC = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRC))

PG_CONFIG = pg_config
SHLIB_LINK = -lmustach

# Find <curl/curl.h> from system headers
PG_CPPFLAGS := $(CPPFLAGS) -DEXTVERSION=\"$(EXTVERSION)\"

build: $(BUILD_DIR)/$(EXTENSION).so $(BUILD_DIR)/$(EXTENSION)--$(EXTVERSION).sql $(BUILD_DIR)/$(EXTENSION).control

$(BUILD_DIR)/.gitignore:
	mkdir -p $(BUILD_DIR)
	echo "*" > $(BUILD_DIR)/.gitignore

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(BUILD_DIR)/.gitignore
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/$(EXTENSION)--$(EXTVERSION).sql: sql/$(EXTENSION).sql
	cp $< $@

$(BUILD_DIR)/$(EXTENSION).control:
	$(SED) "s/@EXTVERSION@/$(EXTVERSION)/g" $(EXTENSION).control.in > $@

$(BUILD_DIR)/$(EXTENSION).so: $(EXTENSION).so
	mv $? $@

PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)

.PHONY: test
test:
	make installcheck
