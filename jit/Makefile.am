
lib_LTLIBRARIES = libjit.la

libjit_la_SOURCES = \
		jit-alloc.c \
		jit-apply.c \
		jit-apply-func.h \
		jit-apply-arm.h \
		jit-apply-arm.c \
		jit-apply-x86.h \
		jit-apply-x86.c \
		jit-block.c \
		jit-cache.c \
		jit-context.c \
		jit-dump.c \
		jit-elf-defs.h \
		jit-elf-read.c \
		jit-elf-write.c \
		jit-except.c \
		jit-function.c \
		jit-gen-arm.h \
		jit-gen-arm.c \
		jit-gen-x86.h \
		jit-insn.c \
		jit-init.c \
		jit-internal.h \
		jit-interp.c \
		jit-intrinsic.c \
		jit-live.c \
		jit-memory.c \
		jit-memory.h \
		jit-meta.c \
		jit-objmodel.c \
		jit-opcode.c \
		jit-pool.c \
		jit-reg-alloc.h \
		jit-reg-alloc.c \
		jit-rules.h \
		jit-rules.c \
		jit-rules-interp.c \
		jit-rules-arm.h \
		jit-rules-arm.c \
		jit-rules-x86.h \
		jit-rules-x86.c \
		jit-string.c \
		jit-symbol.c \
		jit-thread.c \
		jit-type.c \
		jit-value.c \
		jit-walk.c

AM_CFLAGS = -I$(top_srcdir)/include -I$(top_builddir)/include -I. -I$(srcdir)

libjit_la_LDFLAGS = -version-info $(LIBJIT_VERSION)

jit-interp.lo: jit-interp-labels.h

jit-interp-labels.h: $(top_srcdir)/include/jit/jit-opcode.h \
		$(top_srcdir)/jit/jit-interp.h $(srcdir)/mklabel.sh
	$(SHELL) $(srcdir)/mklabel.sh "$(AWK)" \
		$(top_srcdir)/include/jit/jit-opcode.h \
		$(top_srcdir)/jit/jit-interp.h >jit-interp-labels.h

jit-rules-x86.lo: jit-rules-x86.slc

jit-rules-x86.slc: jit-rules-x86.sel $(top_builddir)/tools/gen-sel$(EXEEXT)
	$(top_builddir)/tools/gen-sel$(EXEEXT) $(srcdir)/jit-rules-x86.sel \
			>jit-rules-x86.slc

jit-rules-arm.lo: jit-rules-arm.slc

jit-rules-arm.slc: jit-rules-arm.sel $(top_builddir)/tools/gen-sel$(EXEEXT)
	$(top_builddir)/tools/gen-sel$(EXEEXT) $(srcdir)/jit-rules-arm.sel \
			>jit-rules-arm.slc

CLEANFILES = jit-interp-labels.h jit-rules-x86.slc jit-rules-arm.slc
