/*
 * jit-cpp-mangle.c - Perform C++ name mangling.
 *
 * Copyright (C) 2004  Southern Storm Software, Pty Ltd.
 *
 * This file is part of the libjit library.
 *
 * The libjit library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * The libjit library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the libjit library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <jit/jit-dynamic.h>
#include <jit/jit.h>
#include <config.h>
#include <stdio.h>

/*@

@cindex Name mangling

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
static char const b36chars[36] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

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
	char		  **names;
	unsigned int	num_names;
	unsigned int	max_names;
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
	mangler->names = 0;
	mangler->num_names = 0;
	mangler->max_names = 0;
}

/*
 * End a mangling operation, and return the final string.
 */
static char *end_mangler(jit_mangler_t mangler)
{
	unsigned int index;
	for(index = 0; index < mangler->num_names; ++index)
	{
		jit_free(mangler->names[index]);
	}
	jit_free(mangler->names);
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
 * Add a name to the name list in "mangler".  Returns the index
 * of a previous occurrence, or -1 if there was no previous version.
 */
static int add_name(jit_mangler_t mangler, const char *name,
					unsigned int name_len)
{
	unsigned int index;
	unsigned int len;
	char **new_names;
	for(index = 0; index < mangler->num_names; ++index)
	{
		len = jit_strlen(mangler->names[index]);
		if(len == name_len && !jit_strncmp(name, mangler->names[index], len))
		{
			return (int)index;
		}
	}
	if(mangler->num_names >= mangler->max_names)
	{
		if(mangler->out_of_memory)
		{
			return -1;
		}
		new_names = (char **)jit_realloc
			(mangler->names, (mangler->num_names + 8));
		if(!new_names)
		{
			mangler->out_of_memory = 1;
			return -1;
		}
		mangler->names = new_names;
		mangler->max_names += 8;
	}
	mangler->names[mangler->num_names] = jit_strndup(name, name_len);
	if(!(mangler->names[mangler->num_names]))
	{
		mangler->out_of_memory = 1;
	}
	else
	{
		++(mangler->num_names);
	}
	return -1;
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
	switch(jit_type_get_kind(type))
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
	int kind = jit_type_get_kind(jit_type_remove_tags(type));
	if(kind == JIT_TYPE_UBYTE ||
	   kind == JIT_TYPE_USHORT ||
	   kind == JIT_TYPE_UINT ||
	   kind == JIT_TYPE_NUINT ||
	   kind == JIT_TYPE_ULONG)
	{
		return 1;
	}
	return 0;
}

/*
 * Forward declarations.
 */
static void mangle_type_gcc2(jit_mangler_t mangler, jit_type_t type);
static void mangle_type_gcc3(jit_mangler_t mangler, jit_type_t type);

/*
 * Special prefixes for gcc 2.x rules.
 */
#define	GCC2_CTOR_PREFIX		"__"
#define	GCC2_DTOR_PREFIX		"_._"	/* Could be "_$_" on some systems */

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
 * Mangle a qualified name, using gcc 2.x rules.
 */
static void mangle_name_gcc2(jit_mangler_t mangler, const char *name)
{
	unsigned int len;
	unsigned int posn;
	unsigned int index;
	unsigned int count;
	char buf[32];

	/* Bail out if we don't have a name at all */
	if(!name)
	{
		return;
	}

	/* Count the number of components */
	len = jit_strlen(name);
	count = 1;
	for(posn = 0; posn < len; ++posn)
	{
		if(name[posn] == '.')
		{
			++count;
		}
		else if(name[posn] == ':')
		{
			if((posn + 1) < len && name[posn + 1] == ':')
			{
				++count;
				++posn;
			}
		}
	}

	/* Output the component count */
	if(count > 9)
	{
		add_ch(mangler, 'Q');
		add_ch(mangler, '_');
		sprintf(buf, "%u", count);
		add_string(mangler, buf);
		add_ch(mangler, '_');
	}
	else if(count > 1)
	{
		add_ch(mangler, 'Q');
		add_ch(mangler, (int)('0' + count));
	}

	/* Output the components in the name */
	posn = 0;
	while(posn < len)
	{
		index = posn;
		while(index < len)
		{
			if(name[index] == '.' || name[index] == ':')
			{
				break;
			}
			++index;
		}
		sprintf(buf, "%u", index - posn);
		add_string(mangler, buf);
		while(posn < index)
		{
			add_ch(mangler, name[posn++]);
		}
		if(posn < len && name[posn] == ':')
		{
			if((posn + 1) < len && name[posn + 1] == ':')
			{
				posn += 2;
			}
			else
			{
				++posn;
			}
		}
		else if(posn < len && name[posn] == '.')
		{
			++posn;
		}
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
	kind = jit_type_get_kind(type);
	if(kind >= JIT_TYPE_SBYTE && kind <= JIT_TYPE_ULONG)
	{
		type = fix_system_types(type);
	}
	switch(kind)
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
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_UNION_NAME:
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_ENUM_NAME:
		{
			/* Output the qualified name of the type */
			mangle_name_gcc2
				(mangler, (const char *)jit_type_get_tagged_data(type));
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
 * Mangle a substitution reference.
 */
static void mangle_substitution_gcc3(jit_mangler_t mangler, int name_index)
{
	char buf[32];
	unsigned int index;
	add_ch(mangler, 'S');
	if(name_index > 0)
	{
		--name_index;
		index = sizeof(buf) - 1;
		buf[index] = '\0';
		while(name_index != 0)
		{
			buf[--index] = b36chars[name_index % 36];
			name_index /= 36;
		}
		while(index == (sizeof(buf) - 1))
		{
			buf[--index] = '0';
		}
		add_string(mangler, buf + index);
	}
	add_ch(mangler, '_');
}

/*
 * Mangle a qualified name, using gcc 3.x rules.
 */
static void mangle_name_gcc3(jit_mangler_t mangler, const char *name,
							 const char *member_name)
{
	unsigned int len;
	unsigned int posn;
	unsigned int index;
	int name_index;
	int name_index2;
	char buf[32];
	int multiple;
	if(!name)
	{
		return;
	}
	len = jit_strlen(name);
	name_index = add_name(mangler, name, len);
	if(name_index != -1)
	{
		/* We have a substitution for the whole name */
		mangle_substitution_gcc3(mangler, name_index);
		return;
	}
	multiple = (jit_strchr(name, '.') != 0 || jit_strchr(name, ':') != 0 ||
				member_name != 0);
	if(multiple)
	{
		add_ch(mangler, 'N');
	}
	posn = 0;
	name_index = -1;
	while(posn < len)
	{
		/* Extract the next component */
		index = posn;
		while(index < len)
		{
			if(name[index] == '.' || name[index] == ':')
			{
				break;
			}
			++index;
		}

		/* Determine if we have a substitution for the current prefix */
		name_index2 = add_name(mangler, name, index);
		if(name_index2 != -1)
		{
			name_index = name_index2;
			posn = index;
		}
		else
		{
			if(name_index != -1)
			{
				mangle_substitution_gcc3(mangler, name_index);
				name_index = -1;
			}
			sprintf(buf, "%u", index - posn);
			add_string(mangler, buf);
			while(posn < index)
			{
				add_ch(mangler, name[posn++]);
			}
		}

		/* Move on to the next component */
		if(posn < len && name[posn] == ':')
		{
			if((posn + 1) < len && name[posn + 1] == ':')
			{
				posn += 2;
			}
			else
			{
				++posn;
			}
		}
		else if(posn < len && name[posn] == '.')
		{
			++posn;
		}
	}
	if(member_name)
	{
		add_len_string(mangler, member_name);
	}
	if(multiple)
	{
		add_ch(mangler, 'E');
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
	kind = jit_type_get_kind(type);
	if(kind >= JIT_TYPE_SBYTE && kind <= JIT_TYPE_ULONG)
	{
		type = fix_system_types(type);
	}
	switch(kind)
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
			if(is_unsigned(type))
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
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_UNION_NAME:
		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_ENUM_NAME:
		{
			/* Output the qualified name of the type */
			mangle_name_gcc3
				(mangler, (const char *)jit_type_get_tagged_data(type), 0);
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
 * Mangle a qualified name, using MSVC 6.0 rules.
 */
static void mangle_name_msvc6(jit_mangler_t mangler, const char *name)
{
	unsigned int len;
	unsigned int posn;
	unsigned int index;
	int name_index;
	int output_at;
	if(!name)
	{
		return;
	}
	len = jit_strlen(name);
	while(len > 0)
	{
		posn = len - 1;
		while(posn > 0 && name[posn] != '.' && name[posn] != ':')
		{
			--posn;
		}
		++posn;
		name_index = add_name(mangler, name + posn, len - posn);
		if(name_index == -1 || name_index > 9)
		{
			for(index = posn; index < len; ++index)
			{
				add_ch(mangler, name[index]);
			}
			output_at = 1;
		}
		else
		{
			add_ch(mangler, '0' + name_index);
			output_at = 0;
		}
		if(posn > 0 && name[posn - 1] == ':')
		{
			--posn;
			if(posn > 0 && name[posn - 1] == ':')
			{
				--posn;
			}
		}
		else if(posn > 0 && name[posn - 1] == '.')
		{
			--posn;
		}
		len = posn;
		if(len > 0 && output_at)
		{
			add_ch(mangler, '@');
		}
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
	kind = jit_type_get_kind(type);
	if(kind >= JIT_TYPE_SBYTE && kind <= JIT_TYPE_ULONG)
	{
		type = fix_system_types(type);
	}
	switch(kind)
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
		{
			add_ch(mangler, 'V');
			mangle_name_msvc6
				(mangler, (const char *)jit_type_get_tagged_data(type));
			add_string(mangler, "@@");
		}
		break;

		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_STRUCT_NAME:
		{
			add_ch(mangler, 'U');
			mangle_name_msvc6
				(mangler, (const char *)jit_type_get_tagged_data(type));
			add_string(mangler, "@@");
		}
		break;

		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_UNION_NAME:
		{
			add_ch(mangler, 'T');
			mangle_name_msvc6
				(mangler, (const char *)jit_type_get_tagged_data(type));
			add_string(mangler, "@@");
		}
		break;

		case JIT_TYPE_FIRST_TAGGED + JIT_TYPETAG_ENUM_NAME:
		{
			add_ch(mangler, 'W');
			add_ch(mangler, (int)('0' + jit_type_get_size(type)));
			mangle_name_msvc6
				(mangler, (const char *)jit_type_get_tagged_data(type));
			add_string(mangler, "@@");
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
 * @deftypefun {char *} jit_mangle_global_function (const char *@var{name}, jit_type_t @var{signature}, int @var{form})
 * Mangle the name of a global C++ function using the specified @var{form}.
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

/*@
 * @deftypefun {char *} jit_mangle_member_function (const char *@var{class_name}, const char *@var{name}, jit_type_t @var{signature}, int @var{form}, int @var{flags})
 * Mangle the name of a C++ member function using the specified @var{form}.
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
 * @vindex JIT_MANGLE_VIRTUAL
 * @item JIT_MANGLE_VIRTUAL
 * The method is a virtual instance method.  If neither
 * @code{JIT_MANGLE_STATIC} nor @code{JIT_MANGLE_VIRTUAL} are supplied,
 * then the method is assumed to be a non-virtual instance method.
 *
 * @vindex JIT_MANGLE_CONST
 * @item JIT_MANGLE_CONST
 * The method is an instance method with the @code{const} qualifier.
 *
 * @vindex JIT_MANGLE_EXPLICIT_THIS
 * @item JIT_MANGLE_EXPLICIT_THIS
 * The @var{signature} includes an extra pointer parameter at the start
 * that indicates the type of the @code{this} pointer.  This parameter won't
 * be included in the final mangled name.
 *
 * @vindex JIT_MANGLE_IS_CTOR
 * @item JIT_MANGLE_IS_CTOR
 * The method is a constructor.  The @var{name} parameter will be ignored.
 *
 * @vindex JIT_MANGLE_IS_DTOR
 * @item JIT_MANGLE_IS_DTOR
 * The method is a destructor.  The @var{name} parameter will be ignored.
 *
 * @vindex JIT_MANGLE_BASE
 * @item JIT_MANGLE_BASE
 * Fetch the "base" constructor or destructor entry point, rather than
 * the "complete" entry point.
 * @end table
 *
 * The @var{class_name} may include namespace and nested parent qualifiers
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
			if((flags & JIT_MANGLE_IS_CTOR) != 0)
			{
				add_string(&mangler, GCC2_CTOR_PREFIX);
				mangle_name_gcc2(&mangler, class_name);
				mangle_signature_gcc2(&mangler, signature);
			}
			else if((flags & JIT_MANGLE_IS_DTOR) != 0)
			{
				add_string(&mangler, GCC2_DTOR_PREFIX);
				mangle_name_gcc2(&mangler, class_name);
			}
			else
			{
				add_string(&mangler, name);
				add_string(&mangler, "__");
				mangle_signature_gcc2(&mangler, signature);
			}
		}
		break;
	#endif

	#ifdef MANGLING_FORM_GCC_3
		case MANGLING_FORM_GCC_3:
		{
			if((flags & JIT_MANGLE_IS_CTOR) != 0)
			{
				add_string(&mangler, "_Z");
				if((flags & JIT_MANGLE_BASE) != 0)
				{
					mangle_name_gcc3(&mangler, class_name, "C2");
				}
				else
				{
					mangle_name_gcc3(&mangler, class_name, "C1");
				}
				mangle_signature_gcc3(&mangler, signature);
			}
			else if((flags & JIT_MANGLE_IS_DTOR) != 0)
			{
				add_string(&mangler, "_Z");
				if((flags & JIT_MANGLE_BASE) != 0)
				{
					mangle_name_gcc3(&mangler, class_name, "D2");
				}
				else
				{
					mangle_name_gcc3(&mangler, class_name, "D1");
				}
				mangle_signature_gcc3(&mangler, signature);
			}
			else
			{
				add_string(&mangler, "_Z");
				mangle_name_gcc3(&mangler, class_name, name);
				mangle_signature_gcc3(&mangler, signature);
			}
		}
		break;
	#endif

	#ifdef MANGLING_FORM_MSVC_6
		case MANGLING_FORM_MSVC_6:
		{
			if((flags & JIT_MANGLE_IS_CTOR) != 0)
			{
				add_string(&mangler, "??0");
				mangle_name_msvc6(&mangler, class_name);
			}
			else if((flags & JIT_MANGLE_IS_DTOR) != 0)
			{
				add_string(&mangler, "??1");
				mangle_name_msvc6(&mangler, class_name);
			}
			else
			{
				add_ch(&mangler, '?');
				add_string(&mangler, name);
				add_ch(&mangler, '@');
				mangle_name_msvc6(&mangler, class_name);
			}
			add_string(&mangler, "@@");
			if((flags & 0x07) == JIT_MANGLE_PROTECTED)
			{
				if((flags & JIT_MANGLE_STATIC) != 0)
				{
					/* static protected */
					add_ch(&mangler, 'K');
				}
				else if((flags & JIT_MANGLE_VIRTUAL) != 0)
				{
					/* virtual protected */
					add_ch(&mangler, 'M');
				}
				else
				{
					/* instance protected */
					add_ch(&mangler, 'I');
				}
			}
			else if((flags & 0x07) == JIT_MANGLE_PRIVATE)
			{
				if((flags & JIT_MANGLE_STATIC) != 0)
				{
					/* static private */
					add_ch(&mangler, 'C');
				}
				else if((flags & JIT_MANGLE_VIRTUAL) != 0)
				{
					/* virtual private */
					add_ch(&mangler, 'E');
				}
				else
				{
					/* instance private */
					add_ch(&mangler, 'A');
				}
			}
			else
			{
				if((flags & JIT_MANGLE_STATIC) != 0)
				{
					/* static public */
					add_ch(&mangler, 'S');
				}
				else if((flags & JIT_MANGLE_VIRTUAL) != 0)
				{
					/* virtual public */
					add_ch(&mangler, 'U');
				}
				else
				{
					/* instance public */
					add_ch(&mangler, 'Q');
				}
			}
			if((flags & JIT_MANGLE_STATIC) == 0)
			{
				if((flags & JIT_MANGLE_CONST) != 0)
				{
					add_ch(&mangler, 'B');
				}
				else
				{
					add_ch(&mangler, 'A');
				}
			}
			mangle_signature_msvc6
				(&mangler, signature,
			     (flags & (JIT_MANGLE_IS_CTOR | JIT_MANGLE_IS_DTOR)) == 0,
				 (flags & JIT_MANGLE_STATIC) == 0,
				 (flags & JIT_MANGLE_EXPLICIT_THIS) != 0);
		}
		break;
	#endif
	}
	return end_mangler(&mangler);
}
