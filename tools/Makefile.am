
noinst_PROGRAMS = gen-apply gen-rules gen-ops

noinst_HEADERS = gen-apply-macosx.h

gen_apply_SOURCES = gen-apply.c

gen_rules_SOURCES = gen-rules-parser.y gen-rules-scanner.l

gen_ops_SOURCES = gen-ops-scanner.l gen-ops-parser.y

AM_YFLAGS = -d

gen-rules-scanner.l: gen-rules-parser.c

gen-ops-scanner.l: gen-ops-parser.c

all-local: $(top_builddir)/jit/jit-apply-rules.h

$(top_builddir)/jit/jit-apply-rules.h: gen-apply$(EXEEXT)
	./gen-apply >$@

AM_CFLAGS = -I$(top_srcdir)/include -I$(top_builddir)/include \
		-I$(top_srcdir)/jit -I$(top_builddir)/jit

CLEANFILES = $(top_builddir)/jit/jit-apply-rules.h \
	gen-rules-parser.c gen-rules-parser.h gen-rules-scanner.c
