
noinst_PROGRAMS = t1 t2 t3 t4

CCLD = $(CXX)

t1_SOURCES = t1.c
t1_LDADD = $(top_builddir)/jit/libjit.a
t1_DEPENDENCIES = $(top_builddir)/jit/libjit.a

t2_SOURCES = t2.c
t2_LDADD = $(top_builddir)/jit/libjit.a
t2_DEPENDENCIES = $(top_builddir)/jit/libjit.a

t3_SOURCES = t3.c
t3_LDADD = $(top_builddir)/jit/libjit.a
t3_DEPENDENCIES = $(top_builddir)/jit/libjit.a

t4_SOURCES = t4.cpp
t4_LDADD = $(top_builddir)/jitplus/libjitplus.a $(top_builddir)/jit/libjit.a
t4_DEPENDENCIES = $(top_builddir)/jitplus/libjitplus.a \
				  $(top_builddir)/jit/libjit.a

AM_CFLAGS = -I$(top_srcdir)/include -I$(top_builddir)/include -I. -I$(srcdir)
AM_CXXFLAGS = -I$(top_srcdir)/include -I$(top_builddir)/include -I. -I$(srcdir)
