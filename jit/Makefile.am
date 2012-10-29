BUILT_SOURCES = jit-opcode.c jit-interp-opcode.h jit-interp-opcode.c

lib_LTLIBRARIES = libjit.la

libjit_la_SOURCES = \
	jit-alloc.c \
	jit-apply.c \
	jit-apply-func.h \
	jit-apply-alpha.h \
	jit-apply-alpha.c \
	jit-apply-arm.h \
	jit-apply-arm.c \
	jit-apply-x86.h \
	jit-apply-x86.c \
	jit-apply-x86-64.h \
	jit-apply-x86-64.c \
	jit-bitset.h \
	jit-bitset.c \
	jit-block.c \
	jit-compile.c \
	jit-config.h \
	jit-context.c \
	jit-cpuid-x86.h \
	jit-cpuid-x86.c \
	jit-debugger.c \
	jit-dump.c \
	jit-elf-defs.h \
	jit-elf-read.c \
	jit-elf-write.c \
	jit-except.c \
	jit-function.c \
	jit-gen-alpha.h \
	jit-gen-arm.h \
	jit-gen-arm.c \
	jit-gen-x86.h \
	jit-gen-x86-64.h \
	jit-insn.c \
	jit-init.c \
	jit-internal.h \
	jit-interp.h \
	jit-interp.c \
	jit-interp-opcode.h \
	jit-interp-opcode.c \
	jit-intrinsic.c \
	jit-live.c \
	jit-memory.c \
	jit-memory-cache.c \
	jit-meta.c \
	jit-opcode-apply.c \
	jit-objmodel.c \
	jit-opcode.c \
	jit-pool.c \
	jit-reg-alloc.h \
	jit-reg-alloc.c \
	jit-reg-class.h \
	jit-reg-class.c \
	jit-rules.h \
	jit-rules.c \
	jit-rules-interp.c \
	jit-rules-interp.h \
	jit-rules-alpha.h \
	jit-rules-alpha.c \
	jit-rules-arm.h \
	jit-rules-arm.c \
	jit-rules-x86.h \
	jit-rules-x86.c \
	jit-rules-x86-64.h \
	jit-rules-x86-64.c \
	jit-setjmp.h \
	jit-signal.c \
	jit-symbol.c \
	jit-thread.c \
	jit-thread.h \
	jit-type.c \
	jit-unwind.c \
	jit-util.c \
	jit-value.c \
	jit-varint.h \
	jit-varint.c \
	jit-vmem.c \
	jit-walk.c

EXTRA_DIST = \
	mklabel.sh \
	jit-rules-alpha.ins \
	jit-rules-arm.ins \
	jit-rules-x86.ins \
	jit-rules-x86-64.ins

AM_CFLAGS = -I$(top_srcdir)/include -I$(top_builddir)/include -I. -I$(srcdir)

libjit_la_LDFLAGS = -version-info $(LIBJIT_VERSION) -no-undefined

jit-interp.lo: jit-interp-labels.h

jit-interp-labels.h: $(top_srcdir)/include/jit/jit-opcode.h \
		$(top_srcdir)/jit/jit-interp-opcode.h $(srcdir)/mklabel.sh
	$(SHELL) $(srcdir)/mklabel.sh "$(AWK)" \
		$(top_srcdir)/include/jit/jit-opcode.h \
		$(top_srcdir)/jit/jit-interp-opcode.h >jit-interp-labels.h

jit-rules-x86.lo: jit-rules-x86.inc

jit-rules-x86.inc: jit-rules-x86.ins $(top_builddir)/tools/gen-rules$(EXEEXT)
	$(top_builddir)/tools/gen-rules$(EXEEXT) $(srcdir)/jit-rules-x86.ins \
			>jit-rules-x86.inc

jit-rules-arm.lo: jit-rules-arm.inc

jit-rules-arm.inc: jit-rules-arm.ins $(top_builddir)/tools/gen-rules$(EXEEXT)
	$(top_builddir)/tools/gen-rules$(EXEEXT) $(srcdir)/jit-rules-arm.ins \
			>jit-rules-arm.inc

jit-rules-alpha.lo: jit-rules-alpha.inc

jit-rules-alpha.inc: jit-rules-alpha.ins $(top_builddir)/tools/gen-rules$(EXEEXT)
	$(top_builddir)/tools/gen-rules$(EXEEXT) $(srcdir)/jit-rules-alpha.ins \
			>jit-rules-alpha.inc

jit-rules-x86-64.lo: jit-rules-x86-64.inc

jit-rules-x86-64.inc: jit-rules-x86-64.ins $(top_builddir)/tools/gen-rules$(EXEEXT)
	$(top_builddir)/tools/gen-rules$(EXEEXT) $(srcdir)/jit-rules-x86-64.ins \
			>jit-rules-x86-64.inc

jit-opcode.c: $(top_srcdir)/config/jit-opcodes.ops
	$(top_builddir)/tools/gen-ops -T $(top_srcdir)/config/jit-opcodes.ops >jit-opcode.c

jit-interp-opcode.h: $(top_srcdir)/config/jit-interp-opcodes.ops
	$(top_builddir)/tools/gen-ops -H $(top_srcdir)/config/jit-interp-opcodes.ops >jit-interp-opcode.h

jit-interp-opcode.c: $(top_srcdir)/config/jit-interp-opcodes.ops
	$(top_builddir)/tools/gen-ops -T $(top_srcdir)/config/jit-interp-opcodes.ops >jit-interp-opcode.c

CLEANFILES = \
	jit-interp-labels.h \
	jit-rules-alpha.inc \
	jit-rules-arm.inc \
	jit-rules-x86.inc \
	jit-rules-x86-64.inc
