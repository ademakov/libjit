
lib_LTLIBRARIES = libjitplus.la

libjitplus_la_SOURCES = \
		jit-plus-context.cpp \
		jit-plus-function.cpp \
		jit-plus-value.cpp \
		jit-plus-jump-table.cpp

AM_CXXFLAGS = -I$(top_srcdir)/include -I$(top_builddir)/include -I. -I$(srcdir)

libjitplus_la_LDFLAGS = \
		-version-info $(LIBJIT_VERSION) \
		-no-undefined \
		-L$(top_builddir)/jit -L$(top_builddir)/jit/.libs -ljit $(LIB_STDCPP)
libjitplus_la_DEPENDENCIES = $(top_builddir)/jit/libjit.la
