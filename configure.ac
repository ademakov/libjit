dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.61)

AC_INIT([libjit], [0.1.3], [dotgnu-libjit@gnu.org])
AC_CONFIG_SRCDIR([include/jit/jit.h])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

dnl Determine the host system type.
AC_CANONICAL_HOST

dnl Initialize automake.
AM_INIT_AUTOMAKE([-Wall dist-bzip2])

dnl Set the version number for the shared libraries.
AC_SUBST(LIBJIT_VERSION)
LIBJIT_VERSION=0:1:0

dnl Determine the architecture.
AC_MSG_CHECKING([architecture])
AC_SUBST(JIT_ARCH)
case "$host" in
	i[[3456789]]86-*-*)
		JIT_ARCH=x86
		;;
	x86_64-*-*)
		JIT_ARCH=x86-64
		;;
	*)
		JIT_ARCH=generic
		;;
esac
AC_MSG_RESULT($JIT_ARCH)

dnl Turn off the cygwin library if building for Win32.
dnl Note: We have to include <stdlib.h> if we will be using "__int64"
dnl because otherwise the mingw32 compiler won't define it correctly.
AC_MSG_CHECKING([if building for some Win32 platform])
AC_SUBST(JIT_INT64_INCLUDE)
case "$host" in
	*-*-cygwin*)
		platform_win32=yes
		if test "x$CC" = "x" ; then
			CC="gcc -mno-cygwin"
		fi
		if test "x$CXX" = "x" ; then
			if test "x$CC" = "xcl" ; then
				CXX="cl"
			else
				CXX="g++ -mno-cygwin"
			fi
		fi
		suppress_libm=yes
		JIT_INT64_INCLUDE='#include <stdlib.h>'
		;;
	*-*-mingw*)
		platform_win32=yes
		if test "x$CC" = "xcl" ; then
			if test "x$CXX" = "x" ; then
				CXX="cl"
			fi
		fi
		suppress_libm=yes
		JIT_INT64_INCLUDE='#include <stdlib.h>'
		;;
	*)
		platform_win32=no
		suppress_libm=no
		JIT_INT64_INCLUDE=
		;;
esac
AC_MSG_RESULT($platform_win32)

dnl The "--enable-interpreter" option forces the use of the interpreter.
AC_ARG_ENABLE(interpreter,
AS_HELP_STRING([--enable-interpreter], [Enable the libjit interpreter]),
[case "${enableval}" in
  yes) interp=true ;;
  no)  interp=false ;;
  *) AC_MSG_ERROR(bad value ${enableval} for --enable-interpreter) ;;
esac], [interp=false])
if test x$interp = xtrue; then
	AC_DEFINE(USE_LIBJIT_INTERPRETER, 1, [Define if you want to use the libjit interpreter])
fi

dnl The "--enable-signals" option forces the use of the OS signals for exception handling.
AC_ARG_ENABLE(signals,
AS_HELP_STRING([--enable-signals], [Enable OS signal handling]),
[case "${enableval}" in
  yes) use_signals=true ;;
  no)  use_signals=false ;;
  *) AC_MSG_ERROR(bad value ${enableval} for --enable-signals) ;;
esac], [use_signals=false])
if test x$use_signals = xtrue; then
	AC_DEFINE(JIT_USE_SIGNALS, 1, [Define if you want to use the OS signals for exception handling])
fi

dnl The "--enable-long-double" option forces the use of long double for
dnl jit_nfloat.
AC_ARG_ENABLE(long-double,
AS_HELP_STRING([--enable-long-double], [Enable the use of long double for jit_nfloat]))

dnl Checks for programs.
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CXX
AM_PROG_AR
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_YACC
AM_PROG_LEX
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL

dnl Set the correct flags for compiling with MSVC.  "/QIfist" is needed
dnl on systems with both VC 6.0 and VC 7.0 installed: sometimes VC 7.0
dnl picks up the wrong intrinsic libraries (particularly for __ftol2).
dnl The "/EHs" option is required to enable exception handling in C++.
if test "x$CC" = "xcl" ; then
	CFLAGS="/nologo /QIfist"
	CXXFLAGS="/nologo /QIfist /EHs"
fi

dnl Check for file extensions.
AC_EXEEXT
AC_OBJEXT

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(string.h strings.h memory.h stdlib.h stdarg.h varargs.h)
AC_CHECK_HEADERS(tgmath.h math.h ieeefp.h pthread.h unistd.h sys/types.h)
AC_CHECK_HEADERS(sys/mman.h fcntl.h dlfcn.h sys/cygwin.h sys/stat.h)
AC_CHECK_HEADERS(time.h sys/time.h)

dnl A macro that helps detect the size of types in a cross-compile environment.
AC_DEFUN([AC_COMPILE_CHECK_SIZEOF],
[changequote(<<, >>)dnl
dnl The name to #define.
define(<<AC_TYPE_NAME>>, translit(sizeof_$1, [a-z *], [A-Z_P]))dnl
dnl The cache variable name.
define(<<AC_CV_NAME>>, translit(ac_cv_sizeof_$1, [ *], [_p]))dnl
changequote([, ])dnl
AC_MSG_CHECKING(size of $1)
AC_CACHE_VAL(AC_CV_NAME,
[for ac_size in 4 8 1 2 12 16 $2 ; do # List sizes in rough order of prevalence.
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include "confdefs.h"
#include <sys/types.h>
]], [[switch (0) case 0: case (sizeof ($1) == $ac_size):;]])], [AC_CV_NAME=$ac_size], [])
  if test x$AC_CV_NAME != x ; then break; fi
done
])
if test x$AC_CV_NAME = x ; then
  AC_CV_NAME=0
fi
AC_MSG_RESULT($AC_CV_NAME)
AC_DEFINE_UNQUOTED(AC_TYPE_NAME, $AC_CV_NAME, [The number of bytes in type $1])
undefine([AC_TYPE_NAME])dnl
undefine([AC_CV_NAME])dnl
])

dnl A macro that detects if "char" is unsigned in a cross-compile environment.
AC_DEFUN([AC_COMPILE_CHAR_UNSIGNED],
[AC_MSG_CHECKING(if char is unsigned)
AC_CACHE_VAL(ac_cv_c_char_unsigned,
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include "confdefs.h"
]], [[switch (-1) case -1: case (char)255:;]])], [ac_cv_c_char_unsigned=yes], []))
if test x$ac_cv_c_char_unsigned = x ; then
  ac_cv_c_char_unsigned=no
fi
AC_MSG_RESULT($ac_cv_c_char_unsigned)
if test $ac_cv_c_char_unsigned = yes && test "$GCC" != yes; then
  AC_DEFINE(__CHAR_UNSIGNED__, 1, [Define to 1 if "char" is unsigned])
fi
])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_COMPILE_CHAR_UNSIGNED
AC_COMPILE_CHECK_SIZEOF(char, 1)
AC_COMPILE_CHECK_SIZEOF(short, 2)
AC_COMPILE_CHECK_SIZEOF(int, 4)
AC_COMPILE_CHECK_SIZEOF(long, 4)
AC_COMPILE_CHECK_SIZEOF(long long, 8)
AC_COMPILE_CHECK_SIZEOF(__int64, 8)
AC_COMPILE_CHECK_SIZEOF(float, 4)
AC_COMPILE_CHECK_SIZEOF(double, 8)
AC_COMPILE_CHECK_SIZEOF(long double, 12)
AC_COMPILE_CHECK_SIZEOF(void *, 4)

dnl Determine the types to use for specific sizes of integers and floats.
AC_SUBST(JITINT8)
AC_SUBST(JITUINT8)
AC_SUBST(JITINT16)
AC_SUBST(JITINT32)
AC_SUBST(JITINT64)
AC_SUBST(JITINT64CXX)
AC_SUBST(JITNATIVEINT)
AC_SUBST(JITFLOAT32)
AC_SUBST(JITFLOAT64)
AC_SUBST(JITNATIVEFLOAT)
AC_SUBST(JITNATIVEINTDEFINE)
AC_SUBST(JITNFLOATISDOUBLE)
AC_MSG_CHECKING(for the 8-bit integer types)
if test "$ac_cv_sizeof_char" = 1 ; then
	if test "x$ac_cv_c_char_unsigned" = "xyes" ; then
		JITINT8="signed char"
	else
		JITINT8=char
	fi
	JITUINT8="unsigned char"
elif test "$ac_cv_sizeof_short" = 1 ; then
	JITINT8=short
	JITUINT8="unsigned short"
elif test "$ac_cv_sizeof_int" = 1 ; then
	JITINT8=int
	JITUINT8="unsigned int"
elif test "$ac_cv_sizeof_long" = 1 ; then
	JITINT8=long
	JITUINT8="unsigned long"
else
	AC_MSG_ERROR(unknown)
fi
AC_MSG_RESULT([$JITINT8, $JITUINT8])
AC_MSG_CHECKING(for the 16-bit integer types)
if test "$ac_cv_sizeof_short" = 2 ; then
	JITINT16=short
elif test "$ac_cv_sizeof_int" = 2 ; then
	JITINT16=int
elif test "$ac_cv_sizeof_long" = 2 ; then
	JITINT16=long
else
	AC_MSG_ERROR(unknown)
fi
AC_MSG_RESULT([$JITINT16, unsigned $JITINT16])
AC_MSG_CHECKING(for the 32-bit integer types)
if test "$ac_cv_sizeof_int" = 4 ; then
	JITINT32=int
elif test "$ac_cv_sizeof_long" = 4 ; then
	JITINT32=long
elif test "$ac_cv_sizeof_short" = 4 ; then
	JITINT32=short
else
	AC_MSG_ERROR(unknown)
fi
AC_MSG_RESULT([$JITINT32, unsigned $JITINT32])
AC_MSG_CHECKING(for the 64-bit integer types)
JITINT64CXX=
if test "$ac_cv_sizeof___int64" = 8 ; then
	if test "x$JIT_INT64_INCLUDE" = "x" ; then
		JITINT64='long long'
	else
		dnl __int64 doesn't work with g++, although it does with gcc.
		JITINT64='__int64'
		JITINT64CXX='long long'
	fi
elif test "$ac_cv_sizeof_int" = 8 ; then
	JITINT64=int
elif test "$ac_cv_sizeof_long" = 8 ; then
	JITINT64=long
elif test "$ac_cv_sizeof_long_long" = 8 ; then
	JITINT64='long long'
else
	AC_MSG_ERROR(unknown)
fi
if test "x$JITINT64CXX" = "x" ; then
	JITINT64CXX="$JITINT64"
fi
AC_MSG_RESULT([$JITINT64, unsigned $JITINT64])
AC_MSG_CHECKING(for the native integer types)
if test "$ac_cv_sizeof_void_p" = 4 ; then
	JITNATIVEINT="$JITINT32"
	JITNATIVEINTDEFINE="JIT_NATIVE_INT32"
elif test "$ac_cv_sizeof_void_p" = 8 ; then
	JITNATIVEINT="$JITINT64"
	JITNATIVEINTDEFINE="JIT_NATIVE_INT64"
else
	AC_MSG_ERROR(unknown)
fi
AC_MSG_RESULT([$JITNATIVEINT, unsigned $JITNATIVEINT])
AC_MSG_CHECKING(for the 32-bit floating-point type)
if test "$ac_cv_sizeof_float" = 4 ; then
	JITFLOAT32=float
elif test "$ac_cv_sizeof_double" = 4 ; then
	JITFLOAT32=double
else
	AC_MSG_ERROR(unknown)
fi
AC_MSG_RESULT($JITFLOAT32)
AC_MSG_CHECKING(for the 64-bit floating-point type)
if test "$ac_cv_sizeof_float" = 8 ; then
	JITFLOAT64=float
elif test "$ac_cv_sizeof_double" = 8 ; then
	JITFLOAT64=double
else
	AC_MSG_ERROR(unknown)
fi
AC_MSG_RESULT($JITFLOAT64)
AC_MSG_CHECKING(for the native floating-point type)
JITNFLOATISDOUBLE=''
if test "$ac_cv_sizeof_long_double" != 0 ; then
	dnl MSVC's "long double" is the same as "double", so we make sure
	dnl to preserve compatibility between MSVC and gcc unless the
	dnl --enable-long-double option is provided.
	if test "x$enable_long_double" = "xyes" ; then
		JITNATIVEFLOAT='long double'
	elif test "x$enable_long_double" = "xno" -o "$ac_cv_sizeof_long_double" = "$ac_cv_sizeof_double" -o "x$platform_win32" = "xyes" ; then
		JITNATIVEFLOAT='double'
		JITNFLOATISDOUBLE='#define JIT_NFLOAT_IS_DOUBLE 1'
	else
		JITNATIVEFLOAT='long double'
	fi
elif test "$ac_cv_sizeof_double" != 0 ; then
	JITNATIVEFLOAT=double
	JITNFLOATISDOUBLE='#define JIT_NFLOAT_IS_DOUBLE 1'
elif test "$ac_cv_sizeof_float" != 0 ; then
	JITNATIVEFLOAT=float
else
	AC_MSG_ERROR(unknown)
fi
AC_MSG_RESULT($JITNATIVEFLOAT)

dnl Check to see if we are using gcc or not.
if test x$GCC = xyes ; then
	CFLAGS="$CFLAGS -Wall"
fi
if test x$GXX = xyes ; then
	CXXFLAGS="$CXXFLAGS -Wall"
fi

dnl If CFLAGS contains "-fomit-frame-pointer", then remove it.
dnl We need to have frame pointers to perform stack walking.
case "x$CFLAGS" in
	*-fomit-frame-pointer*)
		CFLAGS=`echo "$CFLAGS" | sed 's/-fomit-frame-pointer//'` ;;
	*)	;;
esac
case "x$CXXFLAGS" in
	*-fomit-frame-pointer*)
		CXXFLAGS=`echo "$CXXFLAGS" | sed 's/-fomit-frame-pointer//'` ;;
	*)	;;
esac

dnl Add "-fno-omit-frame-pointer" to the CFLAGS because current gcc versions
dnl have no frame pointers by default on some archs.
if test x$GCC = xyes ; then
	CFLAGS="$CFLAGS -fno-omit-frame-pointer"
	CXXFLAGS="$CXXFLAGS -fno-omit-frame-pointer"
fi

dnl Find the option to use to turn on C++ exception handling.
AC_CACHE_CHECK(for C++ exception handling option, ac_cv_prog_cxx_exceptions,
[echo 'int main(int argc, char **argv){try { throw 1; } catch(int i) { return i; } return 0;}' > conftest.c
if test -z "`${CXX-c++} -o conftest conftest.c 2>&1`"; then
  ac_cv_prog_cxx_exceptions='none needed'
else
  if test -z "`${CXX-c++} -fexceptions -o conftest conftest.c 2>&1`"; then
    ac_cv_prog_cxx_exceptions=-fexceptions
  else
    if test -z "`${CXX-c++} -fhandle-exceptions -o conftest conftest.c 2>&1`"; then
      ac_cv_prog_cxx_exceptions=-fhandle-exceptions
    else
      ac_cv_prog_cxx_exceptions='none needed'
    fi
  fi
fi
rm -f conftest*
])
if test "x$ac_cv_prog_cxx_exceptions" != "xnone needed" ; then
	CXXFLAGS="$ac_cv_prog_cxx_exceptions $CXXFLAGS"
fi

dnl Determine if the C++ compiler understands the "throw()" idiom.
AC_CACHE_CHECK(for C++ throw() idiom, ac_cv_prog_cxx_throw_idiom,
[echo 'extern "C" void func(void) throw(); int main(int argc, char **argv){return 0;}' > conftest.c
if test -z "`${CXX-c++} -o conftest conftest.c 2>&1`"; then
  ac_cv_prog_cxx_throw_idiom=yes
else
  ac_cv_prog_cxx_throw_idiom=no
fi
rm -f conftest*
])
AC_SUBST(JITTHROWIDIOM)
if test "x$ac_cv_prog_cxx_throw_idiom" = "xyes" ; then
	JITTHROWIDIOM='throw()'
else
	JITTHROWIDIOM=''
fi

dnl Determine if the C compiler understands the "-fno-gcse" option.
dnl We will get better code in the interpreter if we use this option.
AC_CACHE_CHECK(for -fno-gcse option, ac_cv_prog_no_gcse,
[echo 'int main(int argc, char **argv){ return 0;}' > conftest.c
if test -z "`${CC-cc} -fno-gcse -o conftest conftest.c 2>&1`"; then
  ac_cv_prog_no_gcse=yes
else
  ac_cv_prog_no_gcse=no
fi
rm -f conftest*
])
if test "x$ac_cv_prog_no_gcse" = "xyes" ; then
	CFLAGS="-fno-gcse $CFLAGS"
fi

dnl Determine if the C++ compiler understands the "-lstdc++" option.
dnl Needed to force the shared libraries to link in the C++ code.
AC_CACHE_CHECK(for -lstdc++ option, ac_cv_prog_stdcpp,
[echo 'int main(int argc, char **argv){ return 0;}' > conftest.c
if test -z "`${CXX-c++} -o conftest conftest.c -lstdc++ 2>&1`"; then
  ac_cv_prog_stdcpp=yes
else
  ac_cv_prog_stdcpp=no
fi
rm -f conftest*
])
AC_SUBST(LIB_STDCPP)
if test "x$ac_cv_prog_stdcpp" = "xyes" ; then
	LIB_STDCPP="-lstdc++"
else
	LIB_STDCPP=""
fi

dnl Check for computed goto support in the compiler.
AC_MSG_CHECKING(for computed goto support)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[
	static void *labels[] = {&&label0, &&label1, &&label2};
	unsigned char *pc = 0;
	goto *labels[*pc];
	label0: ;
	label1: ;
	label2: ;
]])], [AC_DEFINE(HAVE_COMPUTED_GOTO, 1, Define if you have support for computed gotos) compgoto=yes], [compgoto=no])
AC_MSG_RESULT($compgoto)
AC_MSG_CHECKING(for pic computed goto support)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[
	static int labelOffsets[] =
		{&&label0 - &&label0, &&label1 - &&label0, &&label2 - &&label0};
	unsigned char *pc = 0;
	goto *(&&label0 + labelOffsets[*pc]);
	label0: ;
	label1: ;
	label2: ;
]])], [AC_DEFINE(HAVE_PIC_COMPUTED_GOTO, 1, Define if you have PIC support for computed gotos) piccompgoto=yes], [piccompgoto=no])
AC_MSG_RESULT($piccompgoto)

dnl Check for building on a multi os system
if test x$GCC = xyes ; then
	multi_os_directory=`$CC -print-multi-os-directory`
	case $multi_os_directory in
	  .) ;; # Avoid trailing /.
	  *) libdir=$libdir/$multi_os_directory ;;
	esac
fi
AC_SUBST(libdir)

dnl Checks for library functions.
if test "x$suppress_libm" = "xno" ; then
	AC_CHECK_LIB(m, sin)
fi
if test "x$platform_win32" = "xno" ; then
	AC_CHECK_LIB(dl, dlopen)
	AC_CHECK_LIB(pthread, pthread_create)
fi
AC_FUNC_MEMCMP
AC_CHECK_FUNCS(memset memcmp memchr memcpy memmove bcopy bzero bcmp)
AC_CHECK_FUNCS(strlen strcpy strcat strncpy strcmp strncmp)
AC_CHECK_FUNCS(strchr strrchr vsprintf vsnprintf _vsnprintf getpagesize)
AC_CHECK_FUNCS(isnan isinf finite fmod remainder drem ceil floor)
AC_CHECK_FUNCS(acos asin atan atan2 cos cosh exp log log10 pow)
AC_CHECK_FUNCS(sin sinh sqrt tan tanh)
AC_CHECK_FUNCS(isnanf isinff finitef fmodf remainderf dremf ceilf floorf)
AC_CHECK_FUNCS(acosf asinf atanf atan2f cosf coshf expf logf log10f powf)
AC_CHECK_FUNCS(sinf sinhf sqrtf tanf tanhf)
AC_CHECK_FUNCS(isnanl isinfl finitel fmodl remainderl dreml ceill floorl)
AC_CHECK_FUNCS(acosl asinl atanl atan2l cosl coshl expl logl log10l powl)
AC_CHECK_FUNCS(sinl sinhl sqrtl tanl tanhl)
AC_CHECK_FUNCS(trunc truncf truncl)
AC_CHECK_FUNCS(roundf round roundl rint rintf rintl)
AC_CHECK_FUNCS(dlopen cygwin_conv_to_win32_path mmap munmap mprotect)
AC_CHECK_FUNCS(sigsetjmp __sigsetjmp _setjmp)
AC_FUNC_ALLOCA

AC_CONFIG_FILES([
  Makefile
  config/Makefile
  include/Makefile
  include/jit/Makefile
  include/jit/jit-defs.h
  tools/Makefile
  jit/Makefile
  jitdynamic/Makefile
  jitplus/Makefile
  dpas/Makefile
  tutorial/Makefile
  tests/Makefile
  doc/Makefile])
AC_OUTPUT
