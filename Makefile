TOP_DIR=$(shell pwd)
SRCDIR:=$(TOP_DIR)
OBJDIR:=$(TOP_DIR)
TARGET_S:=example-server
TARGET_C:=example-client
CC?=gcc

# CFLAGS+=-Os -Wall -Wextra
CFLAGS+=-Os -std=gnu99 -g3 -Wmissing-declarations -Wno-unused-parameter
CFLAGS+=-Wextra -Werror=implicit-function-declaration
CFLAGS+=-Wformat -Werror=format-security -Werror=format-nonliteral
CFLAGS+=-I$(TOP_DIR)/lib/inc
CFLAGS+=-I$(TOP_DIR)/lib/src
# LDFLAGS+=-static

LIBSRCS:=$(wildcard $(SRCDIR)/lib/src/*.c)
SOURCES_S:=$(wildcard $(SRCDIR)/$(TARGET_S).c $(LIBSRCS))
SOURCES_C:=$(wildcard $(SRCDIR)/$(TARGET_C).c $(LIBSRCS))
OBJECTS_S:=$(patsubst %.c, %.o, $(SOURCES_S))
OBJECTS_C:=$(patsubst %.c, %.o, $(SOURCES_C))

$(info "SOURCES_S=$(SOURCES_S)")
$(info "SOURCES_C=$(SOURCES_C)")
$(info "OBJECTS_S=$(OBJECTS_S)")
$(info "OBJECTS_C=$(OBJECTS_C)")

all: $(TARGET_S) $(TARGET_C)

$(TARGET_S): $(OBJECTS_S)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LIBS) -o $@

$(TARGET_C): $(OBJECTS_C)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LIBS) -o $@

clean:
	rm -rf $(TARGET_S) $(TARGET_C)
	find $(TOP_DIR) -iname "*.o" | xargs rm -f

lint:
	find $(SRCDIR) -iname "*.[ch]" | xargs clang-format -i