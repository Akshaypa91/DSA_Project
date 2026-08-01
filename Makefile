# MiniGit - build configuration
#
#   make            build ./mini_git
#   make test       build and run the end-to-end test suite
#   make debug      build with ASan + UBSan
#   make analyze    syntax/warning-only pass over every source file
#   make clean      remove build artefacts

CC       ?= cc
CSTD     ?= -std=c11
WARN      = -Wall -Wextra -Wpedantic -Wshadow -Wstrict-prototypes \
            -Wmissing-prototypes -Wpointer-arith -Wcast-qual \
            -Wwrite-strings -Wno-unused-parameter
OPT         ?= -O2
CFLAGS_EXTRA ?=
CFLAGS      += $(CSTD) $(WARN) $(OPT) -Iinclude -MMD -MP $(CFLAGS_EXTRA)
LDFLAGS     +=

TARGET    = mini_git
SRCDIR    = src
BUILDDIR  = build

SRCS      = $(wildcard $(SRCDIR)/*.c)
OBJS      = $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SRCS))
DEPS      = $(OBJS:.o=.d)

.PHONY: all test debug analyze clean install help

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR):
	@mkdir -p $(BUILDDIR)

# Address + undefined-behaviour sanitizers, no optimisation.
debug:
	@$(MAKE) --no-print-directory clean
	@$(MAKE) --no-print-directory OPT="-O0 -g3 -fsanitize=address,undefined -fno-omit-frame-pointer" \
		LDFLAGS="-fsanitize=address,undefined"

test: $(TARGET)
	@sh tests/run_tests.sh

analyze:
	@$(CC) $(CSTD) $(WARN) -Iinclude -fsyntax-only $(SRCS) && echo "syntax OK"

install: $(TARGET)
	install -d $(DESTDIR)/usr/local/bin
	install -m 0755 $(TARGET) $(DESTDIR)/usr/local/bin/$(TARGET)

clean:
	@rm -rf $(BUILDDIR) $(TARGET) $(TARGET).exe
	@echo "cleaned"

help:
	@echo "targets: all test debug analyze install clean"

-include $(DEPS)
