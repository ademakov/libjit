
noinst_PROGRAMS = gen-apply

gen_apply_SOURCES = gen-apply.c

all-local: $(top_builddir)/jit/jit-apply-rules.h

$(top_builddir)/jit/jit-apply-rules.h: gen-apply$(EXEEXT)
	./gen-apply >$@

AM_CFLAGS = -I$(top_srcdir)/include -I$(top_builddir)/include \
			-I$(top_srcdir)/jit -I$(top_builddir)/jit

CLEANFILES = $(top_builddir)/jit/jit-apply-rules.h
