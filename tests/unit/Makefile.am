## Unit tests.

jitlib = $(top_builddir)/jit/libjit.la

AM_CFLAGS = -I$(top_srcdir)/include -I$(top_builddir)/include

check_PROGRAMS = cfg-tests
TESTS = $(check_PROGRAMS)

cfg_tests_SOURCES = cfg-tests.c
cfg_tests_LDADD = $(jitlib)

# It's enough for this program to compile.
check-local:
	$(srcdir)/make-includes-test $(top_srcdir)/jit test-includes.c
	$(CC) $(AM_CFLAGS) -c test-includes.c
