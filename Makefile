# SPDX-License-Identifier: GPL-3.0-only
#
# nfuspire/Makefile
#
# Copyright (C) Emily <info@emy.sh>

TARGET = nfuspire
CC ?= gcc
INCLUDE_DIR = $(CURDIR)/include
BUILD_DIR ?= .build
OBJ_DIR ?= $(BUILD_DIR)/obj

CFLAGS = -I$(INCLUDE_DIR) -std=gnu23 -DFUSE_USE_VERSION=31 -Wall \
	-Wextra -Werror -Werror=vla $(shell pkg-config fuse3 --cflags) \
	$(shell pkg-config libnspire --cflags)
LFLAGS = -Wl,--fatal-warnings -Wl,--warn-common \
	$(shell pkg-config fuse3 --libs) $(shell pkg-config libnspire --libs)
CLANG_FORMAT_FLAGS =  --Werror --dry-run --ferror-limit=5

PREFIX ?= /usr

SOURCES = $(wildcard src/*.c)
HEADERS = $(wildcard $(INCLUDE_DIR)/nfuspire/*.h)

OBJCC=${patsubst %.c,$(OBJ_DIR)/%.o,$(notdir $(SOURCES))}

.PHONY: all
all: check $(SOURCES) $(TARGET)

$(OBJ_DIR)/:
	mkdir -p $@

$(OBJ_DIR)/%.o: src/%.c $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJ_DIR)/ $(OBJCC)
	$(CC) $(LFLAGS) $(OBJCC) -o $(TARGET)

.PHONY: check
check:
	clang-format $(CLANG_FORMAT_FLAGS) $(HEADERS) $(SOURCES)

.PHONY: install
install: $(TARGET)
	install -Dm 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)

.PHONY: uninstall
uninstall:
	rm $(DESTDIR)$(PREFIX)/bin/$(TARGET)

.PHONY: clean
clean:
	rm -rf $(OBJ_DIR)
	rm -f $(TARGET)
