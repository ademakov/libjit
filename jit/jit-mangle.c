/*
 * jit-mangle.c - Perform C++ name mangling.
 *
 * Copyright (C) 2004  Southern Storm Software, Pty Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "jit-internal.h"
#include <config.h>
#include <stdio.h>

/*@

@cindex jit-mangle.h

Sometimes you want to retrieve a C++ method from a dynamic library
using @code{jit_dynlib_get_symbol}.  Unfortunately, C++ name mangling
rules differ from one system to another, making this process very
error-prone.

The functions that follow try to help.  They aren't necessarily fool-proof,
but they should work in the most common cases.  The only alternative is
to wrap your C++ library with C functions, so that the names are predictable.

The basic idea is that you supply a description of the C++ method that
you wish to access, and these functions return a number of candidate forms
that you can try with @code{jit_dynlib_get_symbol}.  If one form fails,
you move on and try the next form, until either symbol lookup succeeds
or until all forms have been exhausted.

@noindent
The following code demonstrates how to resolve a global function:

@example
jit_dynlib_handle_t handle;
jit_type_t signature;
int form = 0;
void *address = 0;
char *mangled;

while((mangled = jit_mangle_global_function
             ("foo", signature, form)) != 0)
@{
    address = jit_dynlib_get_symbol(handle, mangled);
    if(address != 0)
    @{
        break;
    @}
    jit_free(mangled);
    ++form;
@}

if(address)
@{
    printf("%s = 0x%lx\n", mangled, (long)address);
@}
else
@{
    printf("could not resolve foo\n");
@}
@end example

This mechanism typically cannot be used to obtain the entry points for
@code{inline} methods.  You will need to make other arrangements to
simulate the behaviour of inline methods, or recompile your dynamic C++
library in a mode that explicitly exports inlines.

C++ method names are very picky about types.  On 32-bit systems,
@code{int} and @code{long} are the same size, but they are mangled
to different characters.  To ensure that the correct function is
picked, you should use @code{jit_type_sys_int}, @code{jit_type_sys_long}, etc
instead of the platform independent types.  If you do use a platform
independent type like @code{jit_type_int}, this library will try to
guess which system type you mean, but the guess will most likely be wrong.

@*/

/*
 * Useful encoding characters.
 */
static char const hexchars[16] = "0123456789ABCDEF";

/*
 * Name mangling output context.
 */
typedef struct jit_mangler *jit_mangler_t;
struct jit_mangler
{
	char		   *buf;
	unsigned int	buf_len;
	unsigned int	buf_max;
	int				out_of_memory;
};

/*
 * Initialize a mangling context.
 */
static void init_mangler(jit_mangler_t mangler)
{
	mangler->buf = 0;
	mangler->buf_len = 0;
	mangler->buf_max = 0;
	mangler->out_of_memory = 0;
}

/*
 * End a mangling operation, and return the final string.
 */
static char *end_mangler(jit_mangler_t mangler)
{
	if(!(mangler->buf) || mangler->out_of_memory)
	{
		jit_free(mangler->buf);
		return 0;
	}
	return mangler->buf;
}

/*
 * Add a character to a mangling buffer.
 */
static void add_ch(jit_mangler_t mangler, int ch)
{
	char *new_buf;
	if(mangler->buf_len >= mangler->buf_max)
	{
		if(mangler->out_of_memory)
		{
			return;
		}
		new_buf = (char *)jit_realloc
			(mangler->buf, mangler->buf_len + 32);
		if(!new_buf)
		{
			mangler->out_of_memory = 1;
			return;
		}
		mangler->buf = new_buf;
		mangler->buf_max += 32;
	}
	mangler->buf[(mangler->buf_len)++] = (char)ch;
}

/*
 * Add a string to a mangling buffer.
 */
static void add_string(jit_mangler_t mangler, const char *str)
{
	while(*str != '\0')
	{
		add_ch(mangler, *str++);
	}
}

/*
 * Add a length-prefixed string to a mangling buffer.
 */
static void add_len_string(jit_mangler_t mangler, const char *str)
{
	char buf[32];
	sprintf(buf, "%u", jit_strlen(str));
	add_string(mangler, buf);
	add_string(mangler, str);
}

/*
 * Get a system integer type of a particular size.
 */
static jit_type_t get_system_type(jit_type_t type, int size, int is_signed)
{
	if(size == sizeof(int))
	{
		if(is_signed)
			return jit_type_sys_int;
		else
			return jit_type_sys_uint;
	}
	else if(size == sizeof(long))
	{
		if(is_signed)
			return jit_type_sys_long;
		else
			return jit_type_sys_ulong;
	}
	else if(size == sizeof(jit_long))
	{
		if(is_signed)
			return jit_type_sys_longlong;
		else
			return jit_type_sys_ulonglong;
	}
	else if(size == sizeof(short))
	{
		if(is_signed)
			return jit_type_sys_short;
		else
			return jit_type_sys_ushort;
	}
	else if(size == sizeof(char))
	{
	#ifdef __CHAR_UNSIGNED__
		if(is_signed)
			return jit_type_sys_schar;
		else
			return jit_type_sys_char;
	#else
		if(is_signed)
			return jit_type_sys_char;
		else
			return jit_type_sys_uchar;
	#endif
	}
	else
	{
		return type;
	}
}

/*
 * Convert a fixed-sized integer type into a system-specific type.
 */
static jit_type_t fix_system_types(jit_type_t type)
{
	if(!type)
	{
		return 0;
	}
	switch(type->kind)
	{
		case JIT_TYPE_SBYTE:
			return get_system_type(type, sizeof(jit_sbyte), 1);
		case JIT_TYPE_UBYTE:
			return get_system_type(type, sizeof(jit_ubyte), 0);
		case JIT_TYPE_SHORT:
			return get_system_type(type, sizeof(jit_short), 1);
		case JIT_TYPE_USHORT:
			return get_system_type(type, sizeof(jit_ushort), 0);
		case JIT_TYPE_INT:
			return get_system_type(type, sizeof(jit_int), 1);
		case JIT_TYPE_UINT:
			return get_system_type(type, sizeof(jit_uint), 0);
		case JIT_TYPE_NINT:
			return get_system_type(type, sizeof(jit_nint), 1);
		case JIT_TYPE_NUINT:
			return get_system_type(type, sizeof(jit_nuint), 0);
		case JIT_TYPE_LONG:
			return get_system_type(type, sizeof(jit_long), 1);
		case JIT_TYPE_ULONG:
			return get_system_type(type, sizeof(jit_long), 0);
	}
	return type;
}

/*
 * Determine if a type is an unsigned integer value.
 */
static int is_unsigned(jit_type_t type)
{
	type = jit_type_remove_tags(type);
	if(type)
	{
		if(type->kind == JIT_TYPE_UBYTE ||
		   type->kind == JIT_TYPE_USHORT ||
		   type->kind == JIT_TYPE_UINT ||
		   type->kind == JIT_TYPE_NUINT ||
		   type->kind == JIT_TYPE_ULONG)
		{
			return 1;
		}
	}
	return 0;
}

/*
 * Forward declarations.
 */
static void mangle_type_gcc2(jit_mangler_t mangler, jit_type_t type);
static void mangle_type_gcc3(jit_mangler_t mangler, jit_type_t type);

/*
 * Mangle a function signature, using gcc 2.x rules.
 */
static void mangle_signature_gcc2(jit_mangler_t mangler, jit_type_t type)
{
	unsigned int num_params;
	unsigned int param;
	num_params = jit_type_num_params(type);
	if(num_params == 0 && jit_type_get_abi(type) != jit_abi_vararg)
	{
		add_ch(mangler, 'v');
	}
	for(param = 0; param < num_params; ++param)
	{
		mangle_type_gcc2(mangler, jit_type_get_param(type, param));
	}
	if(jit_type_get_abi(type) == jit_abi_vararg)
	{
		add_ch(mangler, 'e');
	}
}

/*
 * Mangle a type, using gcc 2.x rules.
 */
static void mangle_type_gcc2(jit_mangler_t mangler, jit_type_t type)
{
	int kind;

	/* Bail out if the type is invalid */
	if(!type)
	{
		return;
	}

	/* Handle "const", "unsigned", "volatile", and "restrict" prefixes */
	if(jit_type_has_tag(type, JIT_TYPETAG_CONST))
	{
		add_ch(mangler, 'C');
	}
	if(is_unsigned(type) && !jit_type_has_tag(type, JIT_TYPETAG_SYS_CHAR))
	{
		add_ch(mangler, 'U');
	}
	if(jit_type_has_tag(type, JIT_TYPETAG_VOLATILE))
	{
		add_ch(mangler, 'V');
	}
	if(jit_type_has_tag(type, JIT_TYPETAG_RESTRICT))
	{
		add_ch(mangler, 'u');
	}

	/* Strip the prefixes that we just output, together with tag kinds
	   that we don't handle specially ourselves */
	while(jit_type_is_tagged(type))
	{
		kind = jit_type_get_tagged_kind(type);
		if(kind == JIT_TYPETAG_CONST ||
		   kind == JIT_TYPETAG_VOLATILE ||
		   kind == JIT_TYPETAG_RESTRICT)
		{
			type = jit_type_get_tagged_type(type);
		}
		else if(kind < JIT_TYPETAG_NAME ||
				kind > JIT_TYPETAG_SYS_LONGDOUBLE)
		{
			type = jit_type_get_tagged_type(type);
		}
		else
		{
			break;
		}
	}

	/* Handle the inner-most part of the type */
	if(type->kind >= JIT_TYPE_SBYTE && type->kind <= JIT_TYPE_ULONG)
	{
		type = fix_system_types(type);
	}
	switch(type->kind)
	{
		case JIT_TYPE_VOID:			add_ch(mangler, 'v'); break;

		case JIT_TYPE_SBYTE:
		case JIT_TYPE_UBYTE:
		case JIT_TYPE_SHORT:
		case JIT_TYPE_USHORT:
		case JIT_TYPE_INT:
		case JIT_TYPE_UINT:
		case JIT_TYPE_NINT:
		case JIT_TYPE_NUINT:
		case JIT_TYPE_LONG:
		case JIT_TYPE_ULONG:
		{
			/* Will only happen if the primitive numeric type
			   does not correspond to one of the system types */
			jit_nuint size = jit_type_get_size(type);
			add_ch(mangler, 'I');
			add_ch(mangler, hexchars[(size >> 4) & 0x0F]);
			add_ch(mangler, hexchars[size & 0x0F]);
		}
		break;

		case JIT_TYPE_FLOAT32:		add_ch(mangler, 'f'); break;
		case JIT_TYPE_FLOAT64:		add_ch(mangler, 'd'); break;
	#ifdef JIT_NFLOAT_IS_DOUBLE
		case JIT_TYPE_NFLOAT:		add_ch(mangler, 'd'); break;
	#else
		case JIT_TYPE_NFLOAT:		add_ch(mangler, 'r'); break;
	#endif

		case JIT_TYPE_STRUCT:
		case JIT_TYPE_UNION:
		{
			/* These should have been tagged with a name */
			add_ch(mangler, '?');
		}
		break;

		case JIT_TYPE_SIGNATURE:
		{
			add_ch(mangler, 'F');
			mangle_signature_gcc2(mangler, type);
			add_ch(mangler, '_');
			mangle_type_gcc2(mangler, jit_type_get_return(type));
		}
		break;

		case JIT_TYPE_PTR:
		{
			add_ch(mangler, 'P');
			mangle_type_gcc2(mangler, jit_type_get_ref(type));
		}
		break;

		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_NAME:
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_STRUCT_NAME:
		{
			/* Output the qualified name of the type */
			/* TODO */
		}
		break;

		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_REFERENCE:
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_OUTPUT:
		{
			add_ch(mangler, 'R');
			mangle_type_gcc2
				(mangler, jit_type_get_ref(jit_type_remove_tags(type)));
		}
		break;

		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_SYS_BOOL:
			add_ch(mangler, 'b'); break;
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_SYS_CHAR:
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_SYS_UCHAR:
			add_ch(mangler, 'c'); break;
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_SYS_SCHAR:
			add_ch(mangler, 'S');
			add_ch(mangler, 'c'); break;
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_SYS_SHORT:
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_SYS_USHORT:
			add_ch(mangler, 's'); break;
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_SYS_INT:
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_SYS_UINT:
			add_ch(mangler, 'i'); break;
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_SYS_LONG:
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_SYS_ULONG:
			add_ch(mangler, 'l'); break;
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_SYS_LONGLONG:
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_SYS_ULONGLONG:
			add_ch(mangler, 'x'); break;
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_SYS_FLOAT:
			add_ch(mangler, 'f'); break;
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_SYS_DOUBLE:
			add_ch(mangler, 'd'); break;
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_SYS_LONGDOUBLE:
			add_ch(mangler, 'r'); break;

		default: break;
	}
}

/*
 * Mangle a function signature, using gcc 3.x rules.
 */
static void mangle_signature_gcc3(jit_mangler_t mangler, jit_type_t type)
{
	unsigned int num_params;
	unsigned int param;
	num_params = jit_type_num_params(type);
	if(num_params == 0 && jit_type_get_abi(type) != jit_abi_vararg)
	{
		add_ch(mangler, 'v');
	}
	for(param = 0; param < num_params; ++param)
	{
		mangle_type_gcc3(mangler, jit_type_get_param(type, param));
	}
	if(jit_type_get_abi(type) == jit_abi_vararg)
	{
		add_ch(mangler, 'z');
	}
}

/*
 * Mangle a type, using gcc 3.x rules.
 */
static void mangle_type_gcc3(jit_mangler_t mangler, jit_type_t type)
{
	int kind;

	/* Bail out if the type is invalid */
	if(!type)
	{
		return;
	}

	/* Handle "const", "volatile", and "restrict" prefixes */
	if(jit_type_has_tag(type, JIT_TYPETAG_RESTRICT))
	{
		add_ch(mangler, 'r');
	}
	if(jit_type_has_tag(type, JIT_TYPETAG_VOLATILE))
	{
		add_ch(mangler, 'V');
	}
	if(jit_type_has_tag(type, JIT_TYPETAG_CONST))
	{
		add_ch(mangler, 'K');
	}

	/* Strip the prefixes that we just output, together with tag kinds
	   that we don't handle specially ourselves */
	while(jit_type_is_tagged(type))
	{
		kind = jit_type_get_tagged_kind(type);
		if(kind == JIT_TYPETAG_CONST ||
		   kind == JIT_TYPETAG_VOLATILE ||
		   kind == JIT_TYPETAG_RESTRICT)
		{
			type = jit_type_get_tagged_type(type);
		}
		else if(kind < JIT_TYPETAG_NAME ||
				kind > JIT_TYPETAG_SYS_LONGDOUBLE)
		{
			type = jit_type_get_tagged_type(type);
		}
		else
		{
			break;
		}
	}

	/* Handle the inner-most part of the type */
	if(type->kind >= JIT_TYPE_SBYTE && type->kind <= JIT_TYPE_ULONG)
	{
		type = fix_system_types(type);
	}
	switch(type->kind)
	{
		case JIT_TYPE_VOID:			add_ch(mangler, 'v'); break;

		case JIT_TYPE_SBYTE:
		case JIT_TYPE_UBYTE:
		case JIT_TYPE_SHORT:
		case JIT_TYPE_USHORT:
		case JIT_TYPE_INT:
		case JIT_TYPE_UINT:
		case JIT_TYPE_NINT:
		case JIT_TYPE_NUINT:
		case JIT_TYPE_LONG:
		case JIT_TYPE_ULONG:
		{
			/* Will only happen if the primitive numeric type
			   does not correspond to one of the system types */
			jit_nuint size = jit_type_get_size(type);
			if(is_unsigned)
				add_string(mangler, "uU");
			else
				add_string(mangler, "uI");
			add_ch(mangler, hexchars[(size >> 4) & 0x0F]);
			add_ch(mangler, hexchars[size & 0x0F]);
		}
		break;

		case JIT_TYPE_FLOAT32:		add_ch(mangler, 'f'); break;
		case JIT_TYPE_FLOAT64:		add_ch(mangler, 'd'); break;
	#ifdef JIT_NFLOAT_IS_DOUBLE
		case JIT_TYPE_NFLOAT:		add_ch(mangler, 'd'); break;
	#else
		case JIT_TYPE_NFLOAT:		add_ch(mangler, 'e'); break;
	#endif

		case JIT_TYPE_STRUCT:
		case JIT_TYPE_UNION:
		{
			/* These should have been tagged with a name */
			add_ch(mangler, '?');
		}
		break;

		case JIT_TYPE_SIGNATURE:
		{
			add_ch(mangler, 'F');
			mangle_type_gcc3(mangler, jit_type_get_return(type));
			mangle_signature_gcc3(mangler, type);
			add_ch(mangler, 'E');
		}
		break;

		case JIT_TYPE_PTR:
		{
			add_ch(mangler, 'P');
			mangle_type_gcc3(mangler, jit_type_get_ref(type));
		}
		break;

		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_NAME:
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_STRUCT_NAME:
		{
			/* Output the qualified name of the type */
			/* TODO */
		}
		break;

		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_REFERENCE:
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_OUTPUT:
		{
			add_ch(mangler, 'R');
			mangle_type_gcc3
				(mangler, jit_type_get_ref(jit_type_remove_tags(type)));
		}
		break;

		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_SYS_BOOL:
			add_ch(mangler, 'b'); break;
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_SYS_CHAR:
			add_ch(mangler, 'c'); break;
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_SYS_SCHAR:
			add_ch(mangler, 'a'); break;
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_SYS_UCHAR:
			add_ch(mangler, 'h'); break;
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_SYS_SHORT:
			add_ch(mangler, 's'); break;
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_SYS_USHORT:
			add_ch(mangler, 't'); break;
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_SYS_INT:
			add_ch(mangler, 'i'); break;
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_SYS_UINT:
			add_ch(mangler, 'j'); break;
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_SYS_LONG:
			add_ch(mangler, 'l'); break;
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_SYS_ULONG:
			add_ch(mangler, 'm'); break;
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_SYS_LONGLONG:
			add_ch(mangler, 'x'); break;
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_SYS_ULONGLONG:
			add_ch(mangler, 'y'); break;
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_SYS_FLOAT:
			add_ch(mangler, 'f'); break;
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_SYS_DOUBLE:
			add_ch(mangler, 'd'); break;
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_SYS_LONGDOUBLE:
			add_ch(mangler, 'e'); break;

		default: break;
	}
}

#if defined(JIT_WIN32_PLATFORM)

/*
 * Forward declaration.
 */
static void mangle_type_msvc6(jit_mangler_t mangler, jit_type_t type);

/*
 * Mangle a function signature, using MSVC 6.0 rules.
 */
static void mangle_signature_msvc6(jit_mangler_t mangler, jit_type_t type,
								   int output_return, int is_this_call,
								   int has_explicit_this)
{
	unsigned int num_params;
	unsigned int param;
	jit_abi_t abi = jit_type_get_abi(type);
	if(is_this_call)
	{
		add_ch(mangler, 'E');
	}
	else if(abi == jit_abi_stdcall)
	{
		add_ch(mangler, 'G');
	}
	else if(abi == jit_abi_fastcall)
	{
		add_ch(mangler, 'I');
	}
	else
	{
		add_ch(mangler, 'A');
	}
	if(output_return)
	{
		/* Ordinary function with an explicit return type */
		mangle_type_msvc6(mangler, jit_type_get_return(type));
	}
	else
	{
		/* Constructor or destructor, with no explicit return type */
		add_ch(mangler, '@');
	}
	num_params = jit_type_num_params(type);
	if(num_params == 0 && abi != jit_abi_vararg)
	{
		add_ch(mangler, 'X');
		add_ch(mangler, 'Z');
		return;
	}
	for(param = (has_explicit_this ? 1 : 0); param < num_params; ++param)
	{
		mangle_type_msvc6(mangler, jit_type_get_param(type, param));
	}
	if(abi == jit_abi_vararg)
	{
		add_ch(mangler, 'Z');
		add_ch(mangler, 'Z');
	}
	else
	{
		add_ch(mangler, '@');
		add_ch(mangler, 'Z');
	}
}

/*
 * Mangle a type, using MSVC 6.0 rules.
 */
static void mangle_type_msvc6(jit_mangler_t mangler, jit_type_t type)
{
	int kind;
	jit_type_t sub_type;

	/* Bail out if the type is invalid */
	if(!type)
	{
		return;
	}

	/* Strip tag kinds that we don't handle specially ourselves */
	while(jit_type_is_tagged(type))
	{
		kind = jit_type_get_tagged_kind(type);
		if(kind < JIT_TYPETAG_NAME ||
		   kind > JIT_TYPETAG_SYS_LONGDOUBLE)
		{
			type = jit_type_get_tagged_type(type);
		}
		else
		{
			break;
		}
	}

	/* Handle the inner-most part of the type */
	if(type->kind >= JIT_TYPE_SBYTE && type->kind <= JIT_TYPE_ULONG)
	{
		type = fix_system_types(type);
	}
	switch(type->kind)
	{
		case JIT_TYPE_VOID:			add_ch(mangler, 'X'); break;

		case JIT_TYPE_SBYTE:
		case JIT_TYPE_UBYTE:
		case JIT_TYPE_SHORT:
		case JIT_TYPE_USHORT:
		case JIT_TYPE_INT:
		case JIT_TYPE_UINT:
		case JIT_TYPE_NINT:
		case JIT_TYPE_NUINT:
		case JIT_TYPE_LONG:
		case JIT_TYPE_ULONG:
		{
			/* Shouldn't happen, as "fix_system_types" resolved them above */
		}
		break;

		case JIT_TYPE_FLOAT32:		add_ch(mangler, 'M'); break;
		case JIT_TYPE_FLOAT64:		add_ch(mangler, 'N'); break;
	#ifdef JIT_NFLOAT_IS_DOUBLE
		case JIT_TYPE_NFLOAT:		add_ch(mangler, 'N'); break;
	#else
		case JIT_TYPE_NFLOAT:		add_ch(mangler, 'O'); break;
	#endif

		case JIT_TYPE_STRUCT:
		case JIT_TYPE_UNION:
		{
			/* These should have been tagged with a name */
			add_ch(mangler, '?');
		}
		break;

		case JIT_TYPE_SIGNATURE:
		{
			add_string(mangler, "P6");
			mangle_signature_msvc6(mangler, type, 1, 0, 0);
		}
		break;

		case JIT_TYPE_PTR:
		{
			add_ch(mangler, 'P');
			sub_type = jit_type_get_ref(type);
			if(jit_type_has_tag(sub_type, JIT_TYPETAG_CONST))
			{
				if(jit_type_has_tag(sub_type, JIT_TYPETAG_VOLATILE))
				{
					add_ch(mangler, 'D');
				}
				else
				{
					add_ch(mangler, 'B');
				}
			}
			else if(jit_type_has_tag(sub_type, JIT_TYPETAG_VOLATILE))
			{
				add_ch(mangler, 'C');
			}
			else
			{
				add_ch(mangler, 'A');
			}
			mangle_type_msvc6(mangler, sub_type);
		}
		break;

		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_NAME:
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_STRUCT_NAME:
		{
			/* Output the qualified name of the type */
			/* TODO */
		}
		break;

		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_REFERENCE:
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_OUTPUT:
		{
			add_ch(mangler, 'A');
			sub_type = jit_type_get_ref(jit_type_remove_tags(type));
			if(jit_type_has_tag(sub_type, JIT_TYPETAG_CONST))
			{
				if(jit_type_has_tag(sub_type, JIT_TYPETAG_VOLATILE))
				{
					add_ch(mangler, 'D');
				}
				else
				{
					add_ch(mangler, 'B');
				}
			}
			else if(jit_type_has_tag(sub_type, JIT_TYPETAG_VOLATILE))
			{
				add_ch(mangler, 'C');
			}
			else
			{
				add_ch(mangler, 'A');
			}
			mangle_type_msvc6(mangler, sub_type);
		}
		break;

		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_CONST:
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_VOLATILE:
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_RESTRICT:
		{
			/* These are handled in the pointer and reference cases */
			mangle_type_msvc6(mangler, jit_type_get_tagged_type(type));
		}
		break;

		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_SYS_BOOL:
			add_ch(mangler, 'D'); break;
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_SYS_CHAR:
			add_ch(mangler, 'D'); break;
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_SYS_SCHAR:
			add_ch(mangler, 'C'); break;
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_SYS_UCHAR:
			add_ch(mangler, 'E'); break;
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_SYS_SHORT:
			add_ch(mangler, 'F'); break;
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_SYS_USHORT:
			add_ch(mangler, 'G'); break;
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_SYS_INT:
			add_ch(mangler, 'H'); break;
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_SYS_UINT:
			add_ch(mangler, 'I'); break;
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_SYS_LONG:
			add_ch(mangler, 'J'); break;
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_SYS_ULONG:
			add_ch(mangler, 'K'); break;
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_SYS_LONGLONG:
			add_string(mangler, "_J"); break;
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_SYS_ULONGLONG:
			add_string(mangler, "_K"); break;
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_SYS_FLOAT:
			add_ch(mangler, 'M'); break;
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_SYS_DOUBLE:
			add_ch(mangler, 'N'); break;
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_SYS_LONGDOUBLE:
			add_ch(mangler, 'O'); break;

		default: break;
	}
}

#endif /* JIT_WIN32_PLATFORM */

/*
 * Name mangling forms, in the order in which they should be tried.
 * We try to arrange for the most likely to be tried first.
 */
#if defined(JIT_WIN32_PLATFORM)
	#define	MANGLING_FORM_MSVC_6		0
	#if defined(__GNUC__) && (__GNUC__ >= 3)
		#define	MANGLING_FORM_GCC_3		1
		#define	MANGLING_FORM_GCC_2		2
	#else
		#define	MANGLING_FORM_GCC_2		1
		#define	MANGLING_FORM_GCC_3		2
	#endif
#elif defined(__GNUC__) && (__GNUC__ >= 3)
	#define	MANGLING_FORM_GCC_3			0
	#define	MANGLING_FORM_GCC_2			1
#else
	#define	MANGLING_FORM_GCC_2			0
	#define	MANGLING_FORM_GCC_3			1
#endif

/*@
 * @deftypefun {char *} jit_mangle_global_function ({const char *} name, jit_type_t signature, int form)
 * Mangle the name of a global C++ function using the specified @code{form}.
 * Returns NULL if out of memory, or if the form is not supported.
 * @end deftypefun
@*/
char *jit_mangle_global_function
	(const char *name, jit_type_t signature, int form)
{
	struct jit_mangler mangler;
	init_mangler(&mangler);
	switch(form)
	{
	#ifdef MANGLING_FORM_GCC_2
		case MANGLING_FORM_GCC_2:
		{
			add_string(&mangler, name);
			add_string(&mangler, "__F");
			mangle_signature_gcc2(&mangler, signature);
		}
		break;
	#endif

	#ifdef MANGLING_FORM_GCC_3
		case MANGLING_FORM_GCC_3:
		{
			add_string(&mangler, "_Z");
			add_len_string(&mangler, name);
			mangle_signature_gcc3(&mangler, signature);
		}
		break;
	#endif

	#ifdef MANGLING_FORM_MSVC_6
		case MANGLING_FORM_MSVC_6:
		{
			add_ch(&mangler, '?');
			add_string(&mangler, name);
			add_string(&mangler, "@@Y");
			mangle_signature_msvc6(&mangler, signature, 1, 0, 0);
		}
		break;
	#endif
	}
	return end_mangler(&mangler);
}

#define	JIT_MANGLE_PUBLIC			0x0001
#define	JIT_MANGLE_PROTECTED		0x0002
#define	JIT_MANGLE_PRIVATE			0x0003
#define	JIT_MANGLE_STATIC			0x0000
#define	JIT_MANGLE_INSTANCE			0x0008
#define	JIT_MANGLE_VIRTUAL			0x0010
#define	JIT_MANGLE_CONST			0x0020
#define	JIT_MANGLE_EXPLICIT_THIS	0x0040
#define	JIT_MANGLE_IS_CTOR			0x0080
#define	JIT_MANGLE_IS_DTOR			0x0100

/*@
 * @deftypefun {char *} jit_mangle_member_function ({const char *} class_name, {const char *} name, jit_type_t signature, int form, int flags)
 * Mangle the name of a C++ member function using the specified @code{form}.
 * Returns NULL if out of memory, or if the form is not supported.
 * The following flags may be specified to modify the mangling rules:
 *
 * @table @code
 * @vindex JIT_MANGLE_PUBLIC
 * @item JIT_MANGLE_PUBLIC
 * The method has @code{public} access within its containing class.
 *
 * @vindex JIT_MANGLE_PROTECTED
 * @item JIT_MANGLE_PROTECTED
 * The method has @code{protected} access within its containing class.
 *
 * @vindex JIT_MANGLE_PRIVATE
 * @item JIT_MANGLE_PRIVATE
 * The method has @code{private} access within its containing class.
 *
 * @vindex JIT_MANGLE_STATIC
 * @item JIT_MANGLE_STATIC
 * The method is @code{static}.
 *
 * @vindex JIT_MANGLE_INSTANCE
 * @item JIT_MANGLE_INSTANCE
 * The method is a non-virtual instance method.
 *
 * @vindex JIT_MANGLE_VIRTUAL
 * @item JIT_MANGLE_VIRTUAL
 * The method is a virtual instance method.
 *
 * @vindex JIT_MANGLE_CONST
 * @item JIT_MANGLE_CONST
 * The method is an instance method with the @code{const} qualifier.
 *
 * @vindex JIT_MANGLE_EXPLICIT_THIS
 * @item JIT_MANGLE_EXPLICIT_THIS
 * The @code{signature} includes an extra pointer parameter at the start
 * that indicates the type of the @code{this} pointer.  This parameter won't
 * be included in the final mangled name.
 *
 * @vindex JIT_MANGLE_IS_CTOR
 * @item JIT_MANGLE_IS_CTOR
 * The method is a constructor.  The @code{name} parameter will be ignored.
 *
 * @vindex JIT_MANGLE_IS_DTOR
 * @item JIT_MANGLE_IS_DTOR
 * The method is a destructor.  The @code{name} parameter will be ignored.
 * @end table
 *
 * The @code{class_name} may include namespace and nested parent qualifiers
 * by separating them with @code{::} or @code{.}.  Class names that involve
 * template parameters are not supported yet.
 * @end deftypefun
@*/
char *jit_mangle_member_function
	(const char *class_name, const char *name,
	 jit_type_t signature, int form, int flags)
{
	struct jit_mangler mangler;
	init_mangler(&mangler);
	switch(form)
	{
	#ifdef MANGLING_FORM_GCC_2
		case MANGLING_FORM_GCC_2:
		{
			/* TODO */
		}
		break;
	#endif

	#ifdef MANGLING_FORM_GCC_3
		case MANGLING_FORM_GCC_3:
		{
			/* TODO */
		}
		break;
	#endif

	#ifdef MANGLING_FORM_MSVC_6
		case MANGLING_FORM_MSVC_6:
		{
			/* TODO */
		}
		break;
	#endif
	}
	return end_mangler(&mangler);
}
