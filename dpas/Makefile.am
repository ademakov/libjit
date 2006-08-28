
noinst_PROGRAMS = dpas

dpas_SOURCES = \
	dpas-internal.h \
	dpas-main.c \
	dpas-builtin.c \
	dpas-function.c \
	dpas-parser.y \
	dpas-scanner.l \
	dpas-semantics.h \
	dpas-scope.c \
	dpas-scope.h \
	dpas-types.c \
	dpas-types.h

AM_YFLAGS = -d

dpas_LDADD = $(top_builddir)/jit/libjit.la
dpas_DEPENDENCIES = $(top_builddir)/jit/libjit.la

AM_CFLAGS = -I$(top_srcdir)/include -I$(top_builddir)/include -I. -I$(srcdir) \
			-DDPAS_INCLUDE_DIR=\"$(datadir)/dpas\"

dpas-scanner.l: dpas-parser.c

CLEANFILES = dpas-parser.c dpas-scanner.c dpas-parser.h
