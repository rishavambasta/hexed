CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -O3 -g -D_GNU_SOURCE
GTK_CFLAGS := $(shell pkg-config --cflags gtk+-3.0)
GTK_LIBS := $(shell pkg-config --libs gtk+-3.0) -lm

SRCDIR = src
COREDIR = $(SRCDIR)/core
UIDIR = $(SRCDIR)/ui
BINDIR = bin
BUILDDIR = build

CORE_SRCS = $(wildcard $(COREDIR)/*.c)
UI_SRCS = $(wildcard $(UIDIR)/*.c)
MAIN_SRC = $(SRCDIR)/main.c

CORE_OBJS = $(patsubst $(COREDIR)/%.c,$(BUILDDIR)/core/%.o,$(CORE_SRCS))
UI_OBJS = $(patsubst $(UIDIR)/%.c,$(BUILDDIR)/ui/%.o,$(UI_SRCS))
MAIN_OBJ = $(BUILDDIR)/main.o

TARGET = $(BINDIR)/hexed
ALIAS = $(BINDIR)/hxd

.PHONY: all clean test appimage-x86_64 appimage-aarch64 win64-installer package-all

all: $(TARGET) $(ALIAS)

$(TARGET): $(CORE_OBJS) $(UI_OBJS) $(MAIN_OBJ) | $(BINDIR)
	$(CC) $^ $(GTK_LIBS) -o $@

$(ALIAS): $(TARGET)
	ln -sf hexed $(ALIAS)


$(BUILDDIR)/core/%.o: $(COREDIR)/%.c | $(BUILDDIR)/core
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c $< -o $@

$(BUILDDIR)/ui/%.o: $(UIDIR)/%.c | $(BUILDDIR)/ui
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c $< -o $@

$(BUILDDIR)/main.o: $(MAIN_SRC) | $(BUILDDIR)
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c $< -o $@

$(BINDIR) $(BUILDDIR) $(BUILDDIR)/core $(BUILDDIR)/ui:
	mkdir -p $@

test: $(CORE_OBJS)
	$(CC) $(CFLAGS) tests/test_core_all.c $(CORE_OBJS) -lm -o $(BUILDDIR)/test_core_all
	./$(BUILDDIR)/test_core_all
	$(CC) $(CFLAGS) tests/test_piece_table.c $(BUILDDIR)/core/piece_table.o -o $(BUILDDIR)/test_piece_table
	./$(BUILDDIR)/test_piece_table
	$(CC) $(CFLAGS) tests/test_hex_records.c $(BUILDDIR)/core/intel_hex.o $(BUILDDIR)/core/srec.o $(BUILDDIR)/core/piece_table.o -o $(BUILDDIR)/test_hex_records
	./$(BUILDDIR)/test_hex_records
	$(CC) $(CFLAGS) tests/test_performance.c $(BUILDDIR)/core/piece_table.o -o $(BUILDDIR)/test_performance
	./$(BUILDDIR)/test_performance
	$(CC) $(CFLAGS) tests/test_new_features.c $(CORE_OBJS) -lm -o $(BUILDDIR)/test_new_features
	./$(BUILDDIR)/test_new_features

clean:
	rm -rf $(BUILDDIR) $(BINDIR)

appimage-x86_64: $(TARGET)
	./packaging/linux/build_appimage_x86_64.sh

appimage-aarch64:
	./packaging/linux/build_appimage_aarch64.sh

win64-installer:
	./packaging/windows/build_installer.sh

package-all: appimage-x86_64 appimage-aarch64 win64-installer

