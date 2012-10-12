
man_MANS        = libjit.3
EXTRA_SCRIPTS	= extract-docs.sh mkhtml.sh mkpdf.sh
EXTRA_DIST      = $(man_MANS) $(EXTRA_SCRIPTS)

info_TEXINFOS   = libjit.texi
libjit_TEXINFOS = \
	$(srcdir)/libjitext-apply.texi \
	$(srcdir)/libjitext-block.texi \
	$(srcdir)/libjitext-context.texi \
	$(srcdir)/libjitext-debugger.texi \
	$(srcdir)/libjitext-dump.texi \
	$(srcdir)/libjitext-dynlib.texi \
	$(srcdir)/libjitext-elf-read.texi \
	$(srcdir)/libjitext-except.texi \
	$(srcdir)/libjitext-function.texi \
	$(srcdir)/libjitext-init.texi \
	$(srcdir)/libjitext-insn.texi \
	$(srcdir)/libjitext-intrinsic.texi \
	$(srcdir)/libjitext-cpp-mangle.texi \
	$(srcdir)/libjitext-util.texi \
	$(srcdir)/libjitext-meta.texi \
	$(srcdir)/libjitext-reg-alloc.texi \
	$(srcdir)/libjitext-rules-interp.texi \
	$(srcdir)/libjitext-type.texi \
	$(srcdir)/libjitext-value.texi \
	$(srcdir)/libjitext-walk.texi \
	$(srcdir)/libjitext-plus-context.texi \
	$(srcdir)/libjitext-plus-function.texi \
	$(srcdir)/libjitext-plus-value.texi

$(srcdir)/libjitext-apply.texi: $(top_srcdir)/jit/jit-apply.c
	$(SHELL) $(srcdir)/extract-docs.sh $< >$@

$(srcdir)/libjitext-block.texi: $(top_srcdir)/jit/jit-block.c
	$(SHELL) $(srcdir)/extract-docs.sh $< >$@

$(srcdir)/libjitext-context.texi: $(top_srcdir)/jit/jit-context.c
	$(SHELL) $(srcdir)/extract-docs.sh $< >$@

$(srcdir)/libjitext-debugger.texi: $(top_srcdir)/jit/jit-debugger.c
	$(SHELL) $(srcdir)/extract-docs.sh $< >$@

$(srcdir)/libjitext-dump.texi: $(top_srcdir)/jit/jit-dump.c
	$(SHELL) $(srcdir)/extract-docs.sh $< >$@

$(srcdir)/libjitext-dynlib.texi: $(top_srcdir)/jitdynamic/jit-dynlib.c
	$(SHELL) $(srcdir)/extract-docs.sh $< >$@

$(srcdir)/libjitext-elf-read.texi: $(top_srcdir)/jit/jit-elf-read.c
	$(SHELL) $(srcdir)/extract-docs.sh $< >$@

$(srcdir)/libjitext-except.texi: $(top_srcdir)/jit/jit-except.c
	$(SHELL) $(srcdir)/extract-docs.sh $< >$@

$(srcdir)/libjitext-function.texi: $(top_srcdir)/jit/jit-function.c
	$(SHELL) $(srcdir)/extract-docs.sh $< >$@

$(srcdir)/libjitext-init.texi: $(top_srcdir)/jit/jit-init.c
	$(SHELL) $(srcdir)/extract-docs.sh $< >$@

$(srcdir)/libjitext-insn.texi: $(top_srcdir)/jit/jit-insn.c
	$(SHELL) $(srcdir)/extract-docs.sh $< >$@

$(srcdir)/libjitext-intrinsic.texi: $(top_srcdir)/jit/jit-intrinsic.c
	$(SHELL) $(srcdir)/extract-docs.sh $< >$@

$(srcdir)/libjitext-cpp-mangle.texi: $(top_srcdir)/jitdynamic/jit-cpp-mangle.c
	$(SHELL) $(srcdir)/extract-docs.sh $< >$@

$(srcdir)/libjitext-util.texi: $(top_srcdir)/jit/jit-util.c
	$(SHELL) $(srcdir)/extract-docs.sh $< >$@

$(srcdir)/libjitext-meta.texi: $(top_srcdir)/jit/jit-meta.c
	$(SHELL) $(srcdir)/extract-docs.sh $< >$@

$(srcdir)/libjitext-reg-alloc.texi: $(top_srcdir)/jit/jit-reg-alloc.c
	$(SHELL) $(srcdir)/extract-docs.sh $< >$@

$(srcdir)/libjitext-rules-interp.texi: $(top_srcdir)/jit/jit-rules-interp.c
	$(SHELL) $(srcdir)/extract-docs.sh $< >$@

$(srcdir)/libjitext-type.texi: $(top_srcdir)/jit/jit-type.c
	$(SHELL) $(srcdir)/extract-docs.sh $< >$@

$(srcdir)/libjitext-value.texi: $(top_srcdir)/jit/jit-value.c
	$(SHELL) $(srcdir)/extract-docs.sh $< >$@

$(srcdir)/libjitext-walk.texi: $(top_srcdir)/jit/jit-walk.c
	$(SHELL) $(srcdir)/extract-docs.sh $< >$@

$(srcdir)/libjitext-plus-context.texi: \
		$(top_srcdir)/jitplus/jit-plus-context.cpp
	$(SHELL) $(srcdir)/extract-docs.sh $< >$@

$(srcdir)/libjitext-plus-function.texi: \
		$(top_srcdir)/jitplus/jit-plus-function.cpp
	$(SHELL) $(srcdir)/extract-docs.sh $< >$@

$(srcdir)/libjitext-plus-value.texi: \
		$(top_srcdir)/jitplus/jit-plus-value.cpp
	$(SHELL) $(srcdir)/extract-docs.sh $< >$@

CLEANFILES = $(srcdir)/libjit.info $(srcdir)/libjit.info-* \
			 $(srcdir)/libjitext-*.texi
