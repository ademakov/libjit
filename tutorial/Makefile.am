
noinst_PROGRAMS = t1 t2 t3 t4 t5

t1_SOURCES = t1.c
t1_LDADD = $(top_builddir)/jit/libjit.la
t1_DEPENDENCIES = $(top_builddir)/jit/libjit.la

t2_SOURCES = t2.c
t2_LDADD = $(top_builddir)/jit/libjit.la
t2_DEPENDENCIES = $(top_builddir)/jit/libjit.la

t3_SOURCES = t3.c
t3_LDADD = $(top_builddir)/jit/libjit.la
t3_DEPENDENCIES = $(top_builddir)/jit/libjit.la

t4_SOURCES = t4.cpp
t4_LDADD = $(top_builddir)/jitplus/libjitplus.la $(top_builddir)/jit/libjit.la
t4_DEPENDENCIES = $(top_builddir)/jitplus/libjitplus.la \
				  $(top_builddir)/jit/libjit.la

t5_SOURCES = t5.c
t5_LDADD = $(top_builddir)/jit/libjit.la
t5_DEPENDENCIES = $(top_builddir)/jit/libjit.la

AM_CFLAGS = -I$(top_srcdir)/include -I$(top_builddir)/include -I. -I$(srcdir)
AM_CXXFLAGS = -I$(top_srcdir)/include -I$(top_builddir)/include -I. -I$(srcdir)
