
noinst_PROGRAMS = gen-apply gen-sel

gen_apply_SOURCES = gen-apply.c

gen_sel_SOURCES = gen-sel-parser.y \
				  gen-sel-scanner.l

AM_YFLAGS = -d

all-local: $(top_builddir)/jit/jit-apply-rules.h

$(top_builddir)/jit/jit-apply-rules.h: gen-apply$(EXEEXT)
	./gen-apply >$@

AM_CFLAGS = -I$(top_srcdir)/include -I$(top_builddir)/include \
			-I$(top_srcdir)/jit -I$(top_builddir)/jit

CLEANFILES = $(top_builddir)/jit/jit-apply-rules.h \
			 gen-sel-parser.c gen-sel-parser.h gen-sel-scanner.c
