
lib_LTLIBRARIES = libjitdynamic.la

libjitdynamic_la_SOURCES = \
		jit-dynlib.c \
		jit-cpp-mangle.c

AM_CFLAGS = -I$(top_srcdir)/include -I$(top_builddir)/include \
			-I. -I$(srcdir) -I$(top_srcdir)/jit -I$(top_builddir)/jit

libjitdynamic_la_LDFLAGS = -version-info $(LIBJIT_VERSION) \
						   -L$(top_builddir)/jit/.libs -ljit $(LIB_STDCPP)
libjitdynamic_la_DEPENDENCIES = $(top_builddir)/jit/libjit.la
