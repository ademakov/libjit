/*
 * dpas-types.c - Special handling for Dynamic Pascal types.
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

#include "dpas-internal.h"
#include <stddef.h>

/*
 * Define some special integer types that are distinguished from normal ones.
 */
jit_type_t dpas_type_boolean;
jit_type_t dpas_type_cboolean;
jit_type_t dpas_type_char;
jit_type_t dpas_type_string;
jit_type_t dpas_type_address;
jit_type_t dpas_type_nil;
jit_type_t dpas_type_size_t;
jit_type_t dpas_type_ptrdiff_t;

/*
 * Register a predefined type within the global scope.
 */
static void register_type(const char *name, jit_type_t type)
{
	dpas_scope_add(dpas_scope_global(), name, type, DPAS_ITEM_TYPE, 0, 0,
	               "(builtin)", 1);
}

/*
 * Get an integer type of a specific size.
 */
static jit_type_t get_int_type(unsigned int size)
{
	if(size == sizeof(jit_int))
	{
		return jit_type_int;
	}
	else if(size == sizeof(jit_long))
	{
		return jit_type_long;
	}
	else if(size == sizeof(jit_nint))
	{
		return jit_type_nint;
	}
	else if(size == sizeof(jit_short))
	{
		return jit_type_short;
	}
	else if(size == sizeof(jit_sbyte))
	{
		return jit_type_sbyte;
	}
	else
	{
		return jit_type_int;
	}
}

/*
 * Get an unsigned integer type of a specific size.
 */
static jit_type_t get_uint_type(unsigned int size)
{
	if(size == sizeof(jit_uint))
	{
		return jit_type_uint;
	}
	else if(size == sizeof(jit_ulong))
	{
		return jit_type_ulong;
	}
	else if(size == sizeof(jit_nuint))
	{
		return jit_type_nuint;
	}
	else if(size == sizeof(jit_ushort))
	{
		return jit_type_ushort;
	}
	else if(size == sizeof(jit_ubyte))
	{
		return jit_type_ubyte;
	}
	else
	{
		return jit_type_uint;
	}
}

void dpas_init_types(void)
{
	jit_constant_t value;

	/*
	 * Create the special types.
	 */
	dpas_type_boolean = jit_type_create_tagged
		(jit_type_sys_int, DPAS_TAG_BOOLEAN, 0, 0, 1);
	dpas_type_cboolean = jit_type_create_tagged
		(jit_type_sys_char, DPAS_TAG_CBOOLEAN, 0, 0, 1);
	if(((char)0xFF) < 0)
	{
		dpas_type_char = jit_type_create_tagged
			(jit_type_sbyte, DPAS_TAG_CHAR, 0, 0, 1);
	}
	else
	{
		dpas_type_char = jit_type_create_tagged
			(jit_type_ubyte, DPAS_TAG_CHAR, 0, 0, 1);
	}
	dpas_type_string = jit_type_create_pointer(dpas_type_char, 1);
	dpas_type_address = jit_type_void_ptr;
	dpas_type_nil = jit_type_create_tagged
		(jit_type_void_ptr, DPAS_TAG_NIL, 0, 0, 1);
	dpas_type_size_t = get_uint_type(sizeof(size_t));
	dpas_type_ptrdiff_t = get_int_type(sizeof(ptrdiff_t));

	/*
	 * Register all of the builtin types.
	 */
	register_type("Boolean", dpas_type_boolean);
	register_type("CBoolean", dpas_type_cboolean);
	register_type("Char", dpas_type_char);
	register_type("String", dpas_type_string);
	register_type("Address", dpas_type_address);

	register_type("Integer", jit_type_int);
	register_type("Cardinal", jit_type_uint);
	register_type("Word", jit_type_uint);

	register_type("Byte", jit_type_ubyte);
	register_type("ByteInt", jit_type_sbyte);
	register_type("ByteWord", jit_type_ubyte);
	register_type("ByteCard", jit_type_ubyte);

	register_type("ShortInt", jit_type_short);
	register_type("ShortWord", jit_type_ushort);
	register_type("ShortCard", jit_type_ushort);

	register_type("MedInt", jit_type_nint);
	register_type("MedWord", jit_type_nuint);
	register_type("MedCard", jit_type_nuint);

	register_type("LongInt", jit_type_long);
	register_type("LongWord", jit_type_ulong);
	register_type("LongCard", jit_type_ulong);

	register_type("LongestInt", jit_type_long);
	register_type("LongestWord", jit_type_ulong);
	register_type("LongestCard", jit_type_ulong);

	register_type("PtrInt", jit_type_nint);
	register_type("PtrWord", jit_type_nuint);
	register_type("PtrCard", jit_type_nuint);

	register_type("SmallInt", jit_type_short);
	register_type("Comp", jit_type_long);

	register_type("ShortReal", jit_type_float32);
	register_type("Single", jit_type_float32);

	register_type("Real", jit_type_float64);
	register_type("Double", jit_type_float64);

	register_type("LongReal", jit_type_nfloat);
	register_type("Extended", jit_type_nfloat);

	register_type("PtrDiffType", dpas_type_ptrdiff_t);
	register_type("SizeType", dpas_type_size_t);

	register_type("SysInt", jit_type_sys_int);
	register_type("SysCard", jit_type_sys_uint);
	register_type("SysWord", jit_type_sys_uint);

	register_type("SysLongInt", jit_type_sys_long);
	register_type("SysLongCard", jit_type_sys_ulong);
	register_type("SysLongWord", jit_type_sys_ulong);

	register_type("SysLongestInt", jit_type_sys_longlong);
	register_type("SysLongestCard", jit_type_sys_ulonglong);
	register_type("SysLongestWord", jit_type_sys_ulonglong);

	/*
	 * Register the "True" and "False" constants.
	 */
	value.type = dpas_type_boolean;
	value.un.int_value = 1;
	dpas_scope_add_const(dpas_scope_global(), "True", &value, "(builtin)", 1);
	value.type = dpas_type_boolean;
	value.un.int_value = 0;
	dpas_scope_add_const(dpas_scope_global(), "False", &value, "(builtin)", 1);
}

unsigned int dpas_type_find_name(jit_type_t type, const char *name)
{
	unsigned int field = jit_type_num_fields(type);
	const char *fname;
	while(field > 0)
	{
		--field;
		fname = jit_type_get_name(type, field);
		if(fname && !jit_stricmp(fname, name))
		{
			return field;
		}
	}
	return JIT_INVALID_NAME;
}

jit_type_t dpas_type_get_field(jit_type_t type, const char *name,
							   jit_nint *offset)
{
	unsigned int field;
	const char *fname;
	jit_type_t field_type;
	type = jit_type_normalize(type);
	field = jit_type_num_fields(type);
	while(field > 0)
	{
		--field;
		fname = jit_type_get_name(type, field);
		field_type = jit_type_get_field(type, field);
		if(fname && !jit_stricmp(fname, name))
		{
			*offset = jit_type_get_offset(type, field);
			return field_type;
		}
		else if(!fname)
		{
			/* Probably a nested struct or union in a variant record */
			if(dpas_type_is_record(field_type))
			{
				field_type = dpas_type_get_field(field_type, name, offset);
				if(field_type != 0)
				{
					*offset += jit_type_get_offset(type, field);
					return field_type;
				}
			}
		}
	}
	return 0;
}

/*
 * Concatenate two strings.
 */
static char *concat_strings(char *str1, char *str2)
{
	char *str;
	if(!str1 || !str2)
	{
		dpas_out_of_memory();
	}
	str = (char *)jit_malloc(jit_strlen(str1) + jit_strlen(str2) + 1);
	if(!str)
	{
		dpas_out_of_memory();
	}
	jit_strcpy(str, str1);
	jit_strcat(str, str2);
	jit_free(str1);
	jit_free(str2);
	return str;
}

static char *type_name(const char *embed_name, jit_type_t type)
{
	char *temp;
	dpas_subrange *range;
	char *name;
	if(jit_type_is_primitive(type))
	{
		if(type == jit_type_void)
		{
			temp = jit_strdup("void");
		}
		else if(type == jit_type_sbyte)
		{
			temp = jit_strdup("ByteInt");
		}
		else if(type == jit_type_ubyte)
		{
			temp = jit_strdup("Byte");
		}
		else if(type == jit_type_short)
		{
			temp = jit_strdup("ShortInt");
		}
		else if(type == jit_type_ushort)
		{
			temp = jit_strdup("ShortCard");
		}
		else if(type == jit_type_int)
		{
			temp = jit_strdup("Integer");
		}
		else if(type == jit_type_uint)
		{
			temp = jit_strdup("Cardinal");
		}
		else if(type == jit_type_long)
		{
			temp = jit_strdup("LongInt");
		}
		else if(type == jit_type_ulong)
		{
			temp = jit_strdup("LongCard");
		}
		else if(type == jit_type_float32)
		{
			temp = jit_strdup("ShortReal");
		}
		else if(type == jit_type_float64)
		{
			temp = jit_strdup("Real");
		}
		else if(type == jit_type_nfloat)
		{
			temp = jit_strdup("LongReal");
		}
		else
		{
			temp = jit_strdup("unknown-primitive-type");
		}
	}
	else if(jit_type_is_struct(type) || jit_type_is_union(type))
	{
		/* Shouldn't happen: record types should be tagged with a name */
		temp = jit_strdup("unknown-struct-or-union");
	}
	else if(jit_type_is_signature(type))
	{
		char *temp;
		jit_type_t return_type, param_type;
		unsigned int param, num_params;
		const char *param_name;
		return_type = jit_type_get_return(type);
		if(return_type == jit_type_void)
		{
			temp = jit_strdup("procedure");
		}
		else
		{
			temp = jit_strdup("function");
		}
		if(embed_name)
		{
			temp = concat_strings(temp, jit_strdup(" "));
			temp = concat_strings(temp, jit_strdup(embed_name));
		}
		num_params = jit_type_num_params(type);
		if(num_params > 0)
		{
			temp = concat_strings(temp, jit_strdup("("));
			for(param = 0; param < num_params; ++param)
			{
				if(param > 0)
				{
					temp = concat_strings(temp, jit_strdup(", "));
				}
				param_type = jit_type_get_param(type, param);
				param_name = jit_type_get_name(type, param);
				temp = concat_strings
					(temp, type_name(param_name, param_type));
			}
			temp = concat_strings(temp, jit_strdup(")"));
		}
		if(return_type != jit_type_void)
		{
			temp = concat_strings(temp, jit_strdup(" : "));
			temp = concat_strings(temp, type_name(0, return_type));
		}
		return temp;
	}
	else if(jit_type_is_pointer(type))
	{
		if(jit_type_get_ref(type) == dpas_type_char)
		{
			temp = jit_strdup("String");
		}
		else if(jit_type_get_ref(type) == jit_type_void)
		{
			temp = jit_strdup("Address");
		}
		else
		{
			temp = concat_strings
				(jit_strdup("^"), type_name(0, jit_type_get_ref(type)));
		}
	}
	else if(jit_type_is_tagged(type))
	{
		switch(jit_type_get_tagged_kind(type))
		{
			case DPAS_TAG_BOOLEAN:		temp = jit_strdup("Boolean"); break;
			case DPAS_TAG_CBOOLEAN:		temp = jit_strdup("CBoolean"); break;
			case DPAS_TAG_CHAR:			temp = jit_strdup("Char"); break;
			case DPAS_TAG_NIL:			temp = jit_strdup("nil"); break;

			case DPAS_TAG_NAME:
			{
				name = (char *)jit_type_get_tagged_data(type);
				if(name)
				{
					temp = jit_strdup(name);
				}
				else
				{
					temp = jit_strdup("anonymous_record");
				}
			}
			break;

			case DPAS_TAG_VAR:
			{
				return concat_strings
					(jit_strdup("var "),
					 type_name(embed_name, jit_type_get_ref
					 	(jit_type_get_tagged_type(type))));
			}
			/* Not reached */

			case DPAS_TAG_SUBRANGE:
			{
				range = (dpas_subrange *)jit_type_get_tagged_data(type);
				temp = concat_strings
					(dpas_constant_name(&(range->first)),
					 concat_strings
					 	(jit_strdup(".."),
						 dpas_constant_name(&(range->last))));
			}
			break;

			case DPAS_TAG_ENUM:
			{
				name = ((dpas_enum *)jit_type_get_tagged_data(type))->name;
				if(name)
				{
					temp = jit_strdup(name);
				}
				else
				{
					temp = jit_strdup("anonymous_enum");
				}
			}
			break;

			case DPAS_TAG_SET:
			{
				temp = concat_strings
					(jit_strdup("set of "),
					 type_name(0, (jit_type_t)jit_type_get_tagged_data(type)));
			}
			break;

			case DPAS_TAG_ARRAY:
			{
				int dim;
				jit_type_t array_type;
				dpas_array *array_info = (dpas_array *)
					jit_type_get_tagged_data(type);
				temp = jit_strdup("array [");
				for(dim = 0; dim < array_info->num_bounds; ++dim)
				{
					if(dim != 0)
					{
						temp = concat_strings(temp, jit_strdup(", "));
					}
					if(array_info->bounds[dim])
					{
						temp = concat_strings
							(temp, type_name(0, array_info->bounds[dim]));
					}
					else
					{
						/* User-supplied type which can't be used as a bound */
						temp = concat_strings(temp, jit_strdup("0 .. 0"));
					}
				}
				temp = concat_strings(temp, jit_strdup("] of "));
				array_type = jit_type_get_tagged_type(type);
				temp = concat_strings
					(temp, type_name(0, jit_type_get_field(array_type, 0)));
			}
			break;

			case DPAS_TAG_CONFORMANT_ARRAY:
			{
				int dim;
				jit_type_t array_type;
				dpas_conformant_array *array_info;
				array_info = (dpas_conformant_array *)
					jit_type_get_tagged_data(type);
				if(array_info->is_packed)
				{
					temp = jit_strdup("packed array [");
				}
				else
				{
					temp = jit_strdup("array [");
				}
				for(dim = 1; dim < array_info->num_bounds; ++dim)
				{
					temp = concat_strings(temp, jit_strdup(","));
				}
				temp = concat_strings(temp, jit_strdup("] of "));
				array_type = jit_type_get_ref(jit_type_get_tagged_type(type));
				temp = concat_strings(temp, type_name(0, array_type));
				if(embed_name)
				{
					temp = concat_strings
						(concat_strings(jit_strdup("var "),
										jit_strdup(embed_name)),
						 concat_strings(jit_strdup(" : "), temp));
				}
				else
				{
					temp = concat_strings(jit_strdup("var "), temp);
				}
				return temp;
			}
			/* Not reached */

			default: temp = jit_strdup("unknown-tagged-type"); break;
		}
	}
	else
	{
		/* We shouldn't get here */
		temp = jit_strdup("unknown-jit-type");
	}
	if(!temp)
	{
		dpas_out_of_memory();
	}
	if(embed_name)
	{
		temp = concat_strings(concat_strings
					(jit_strdup(embed_name), jit_strdup(" : ")), temp);
	}
	return temp;
}

char *dpas_type_name(jit_type_t type)
{
	char *temp = type_name(0, type);
	if(!temp)
	{
		dpas_out_of_memory();
	}
	return temp;
}

char *dpas_type_name_with_var(const char *name, jit_type_t type)
{
	char *temp = type_name(name, type);
	if(!temp)
	{
		dpas_out_of_memory();
	}
	return temp;
}

jit_type_t dpas_promote_type(jit_type_t type)
{
	if(jit_type_get_tagged_kind(type) == DPAS_TAG_SUBRANGE)
	{
		/* Convert subrange types into their real integer counterparts */
		type = jit_type_get_tagged_type(type);
	}
	if(type == jit_type_sbyte || type == jit_type_ubyte ||
	   type == jit_type_short || type == jit_type_ushort)
	{
		return jit_type_int;
	}
	else if(type == jit_type_nint)
	{
		if(sizeof(jit_nint) == sizeof(jit_int))
		{
			return jit_type_int;
		}
		else
		{
			return jit_type_long;
		}
	}
	else if(type == jit_type_nuint)
	{
		if(sizeof(jit_nuint) == sizeof(jit_uint))
		{
			return jit_type_uint;
		}
		else
		{
			return jit_type_ulong;
		}
	}
	else if(type == jit_type_float32 || type == jit_type_float64)
	{
		return jit_type_nfloat;
	}
	else
	{
		return type;
	}
}

jit_type_t dpas_common_type(jit_type_t type1, jit_type_t type2, int int_only)
{
	type1 = dpas_promote_type(type1);
	type2 = dpas_promote_type(type2);
	if(type1 == type2)
	{
		if(int_only && type1 == jit_type_nfloat)
		{
			return 0;
		}
		return type1;
	}
	if(type1 == jit_type_int)
	{
		if(type2 == jit_type_uint)
		{
			return jit_type_int;
		}
		else if(type2 == jit_type_long || type2 == jit_type_ulong)
		{
			return jit_type_long;
		}
		else if(type2 == jit_type_nfloat)
		{
			if(!int_only)
			{
				return jit_type_nfloat;
			}
		}
	}
	else if(type1 == jit_type_uint)
	{
		if(type2 == jit_type_int)
		{
			return jit_type_int;
		}
		else if(type2 == jit_type_long)
		{
			return jit_type_long;
		}
		else if(type2 == jit_type_ulong)
		{
			return jit_type_ulong;
		}
		else if(type2 == jit_type_nfloat)
		{
			if(!int_only)
			{
				return jit_type_nfloat;
			}
		}
	}
	else if(type1 == jit_type_long)
	{
		if(type2 == jit_type_int || type2 == jit_type_uint ||
		   type2 == jit_type_ulong)
		{
			return jit_type_long;
		}
		else if(type2 == jit_type_nfloat)
		{
			if(!int_only)
			{
				return jit_type_nfloat;
			}
		}
	}
	else if(type1 == jit_type_ulong)
	{
		if(type2 == jit_type_int || type2 == jit_type_long)
		{
			return jit_type_long;
		}
		else if(type2 == jit_type_uint)
		{
			return jit_type_ulong;
		}
		else if(type2 == jit_type_nfloat)
		{
			if(!int_only)
			{
				return jit_type_nfloat;
			}
		}
	}
	else if(type1 == jit_type_nfloat)
	{
		if(type2 == jit_type_int || type2 == jit_type_uint ||
		   type2 == jit_type_long || type2 == jit_type_ulong)
		{
			if(!int_only)
			{
				return jit_type_nfloat;
			}
		}
	}
	return 0;
}

jit_type_t dpas_create_subrange(jit_type_t underlying, dpas_subrange *values)
{
	dpas_subrange *copy;
	jit_type_t type;
	copy = jit_new(dpas_subrange);
	if(!copy)
	{
		dpas_out_of_memory();
	}
	*copy = *values;
	type = jit_type_create_tagged(underlying, DPAS_TAG_SUBRANGE,
								  copy, jit_free, 1);
	if(!type)
	{
		dpas_out_of_memory();
	}
	return type;
}

/*
 * Free the information block for an enumerated type.
 */
static void free_enum(void *info)
{
	jit_free(((dpas_enum *)info)->name);
	jit_free(info);
}

jit_type_t dpas_create_enum(jit_type_t underlying, int num_elems)
{
	dpas_enum *enum_info;
	jit_type_t type;
	enum_info = jit_new(dpas_enum);
	if(!enum_info)
	{
		dpas_out_of_memory();
	}
	enum_info->name = 0;
	enum_info->num_elems = num_elems;
	type = jit_type_create_tagged(underlying, DPAS_TAG_ENUM,
								  enum_info, free_enum, 1);
	if(!type)
	{
		dpas_out_of_memory();
	}
	return type;
}

jit_nuint dpas_type_range_size(jit_type_t type)
{
	if(jit_type_get_tagged_kind(type) == DPAS_TAG_ENUM)
	{
		return (jit_nuint)(jit_nint)
			(((dpas_enum *)jit_type_get_tagged_data(type))->num_elems);
	}
	else if(jit_type_get_tagged_kind(type) == DPAS_TAG_SUBRANGE &&
			jit_type_get_tagged_type(type) == jit_type_int)
	{
		return (jit_nuint)(jit_nint)
			(((dpas_subrange *)jit_type_get_tagged_data(type))
					->last.un.int_value -
			 ((dpas_subrange *)jit_type_get_tagged_data(type))
					->first.un.int_value + 1);
	}
	else
	{
		return 0;
	}
}

jit_type_t dpas_create_array(jit_type_t *bounds, int num_bounds,
							 jit_type_t elem_type)
{
	jit_type_t type;
	jit_type_t tagged;
	dpas_array *info;
	jit_nuint size;
	int posn;

	/* Create a struct type, with the element type in the first field */
	type = jit_type_create_struct(&elem_type, 1, 0);
	if(!type)
	{
		dpas_out_of_memory();
	}

	/* Tag the structure with the array bound information */
	info = (dpas_array *)jit_malloc(sizeof(dpas_array));
	if(!info)
	{
		dpas_out_of_memory();
	}
	info->bounds = bounds;
	info->num_bounds = num_bounds;
	tagged = jit_type_create_tagged(type, DPAS_TAG_ARRAY, info, jit_free, 0);

	/* Infer the total size of the array */
	size = jit_type_get_size(elem_type);
	for(posn = 0; posn < num_bounds; ++posn)
	{
		size *= dpas_type_range_size(bounds[posn]);
	}

	/* If the array is empty, then allocate space for one element
	   so that the type's total size is non-zero */
	if(!size)
	{
		size = jit_type_get_size(elem_type);
	}

	/* Set the array's final size and alignment */
	jit_type_set_size_and_alignment
		(type, size, jit_type_get_alignment(elem_type));

	/* Return the tagged version of the array to the caller */
	return tagged;
}

jit_type_t dpas_create_conformant_array(jit_type_t elem_type, int num_bounds,
							 			int is_packed)
{
	jit_type_t type;
	dpas_conformant_array *info;

	/* A conformant array is actually a pointer to the first element */
	type = jit_type_create_pointer(elem_type, 0);
	if(!type)
	{
		dpas_out_of_memory();
	}

	/* Tag the pointer so that the rest of the system knows what it is */
	info = jit_new(dpas_conformant_array);
	if(!info)
	{
		dpas_out_of_memory();
	}
	info->num_bounds = num_bounds;
	info->is_packed = is_packed;
	type = jit_type_create_tagged(type, DPAS_TAG_CONFORMANT_ARRAY,
								  info, jit_free, 0);
	if(!type)
	{
		dpas_out_of_memory();
	}
	return type;
}

jit_type_t dpas_type_get_elem(jit_type_t type)
{
	if(jit_type_get_tagged_kind(type) == DPAS_TAG_ARRAY)
	{
		return jit_type_get_field(jit_type_normalize(type), 0);
	}
	else if(jit_type_get_tagged_kind(type) == DPAS_TAG_CONFORMANT_ARRAY)
	{
		return jit_type_get_ref(jit_type_normalize(type));
	}
	else
	{
		return 0;
	}
}

int dpas_type_get_rank(jit_type_t type)
{
	if(jit_type_get_tagged_kind(type) == DPAS_TAG_ARRAY)
	{
		return ((dpas_array *)jit_type_get_tagged_data(type))->num_bounds;
	}
	else if(jit_type_get_tagged_kind(type) == DPAS_TAG_CONFORMANT_ARRAY)
	{
		return ((dpas_conformant_array *)
					jit_type_get_tagged_data(type))->num_bounds;
	}
	else
	{
		return 1;
	}
}

void dpas_type_set_name(jit_type_t type, const char *name)
{
	char *copy;
	dpas_enum *enum_info;
	if(jit_type_get_tagged_kind(type) == DPAS_TAG_NAME)
	{
		if(!jit_type_get_tagged_data(type))
		{
			copy = jit_strdup(name);
			if(!copy)
			{
				dpas_out_of_memory();
			}
			jit_type_set_tagged_data(type, copy, jit_free);
		}
	}
	else if(jit_type_get_tagged_kind(type) == DPAS_TAG_ENUM)
	{
		enum_info = (dpas_enum *)jit_type_get_tagged_data(type);
		if(!(enum_info->name))
		{
			copy = jit_strdup(name);
			if(!copy)
			{
				dpas_out_of_memory();
			}
			enum_info->name = copy;
		}
	}
}

void dpas_convert_constant(jit_constant_t *result, jit_constant_t *from,
						   jit_type_t to_type)
{
	jit_type_t from_type = dpas_promote_type(from->type);
	to_type = dpas_promote_type(to_type);
	result->type = to_type;
	result->un = from->un;
	if(to_type == jit_type_int)
	{
		if(from_type == jit_type_int)
		{
			result->un.int_value = jit_int_to_int(from->un.int_value);
		}
		else if(from_type == jit_type_uint)
		{
			result->un.int_value = jit_uint_to_int(from->un.uint_value);
		}
		else if(from_type == jit_type_long)
		{
			result->un.int_value = jit_long_to_int(from->un.long_value);
		}
		else if(from_type == jit_type_ulong)
		{
			result->un.int_value = jit_ulong_to_int(from->un.ulong_value);
		}
		else if(from_type == jit_type_nfloat)
		{
			result->un.int_value = jit_nfloat_to_int(from->un.nfloat_value);
		}
	}
	else if(to_type == jit_type_uint)
	{
		if(from_type == jit_type_int)
		{
			result->un.uint_value = jit_int_to_uint(from->un.int_value);
		}
		else if(from_type == jit_type_uint)
		{
			result->un.uint_value = jit_uint_to_uint(from->un.uint_value);
		}
		else if(from_type == jit_type_long)
		{
			result->un.uint_value = jit_long_to_uint(from->un.long_value);
		}
		else if(from_type == jit_type_ulong)
		{
			result->un.uint_value = jit_ulong_to_uint(from->un.ulong_value);
		}
		else if(from_type == jit_type_nfloat)
		{
			result->un.uint_value = jit_nfloat_to_uint(from->un.nfloat_value);
		}
	}
	else if(to_type == jit_type_long)
	{
		if(from_type == jit_type_int)
		{
			result->un.long_value = jit_int_to_long(from->un.int_value);
		}
		else if(from_type == jit_type_uint)
		{
			result->un.long_value = jit_uint_to_long(from->un.uint_value);
		}
		else if(from_type == jit_type_long)
		{
			result->un.long_value = jit_long_to_long(from->un.long_value);
		}
		else if(from_type == jit_type_ulong)
		{
			result->un.long_value = jit_ulong_to_long(from->un.ulong_value);
		}
		else if(from_type == jit_type_nfloat)
		{
			result->un.long_value = jit_nfloat_to_long(from->un.nfloat_value);
		}
	}
	else if(to_type == jit_type_ulong)
	{
		if(from_type == jit_type_int)
		{
			result->un.ulong_value = jit_int_to_ulong(from->un.int_value);
		}
		else if(from_type == jit_type_uint)
		{
			result->un.ulong_value = jit_uint_to_ulong(from->un.uint_value);
		}
		else if(from_type == jit_type_long)
		{
			result->un.ulong_value = jit_long_to_ulong(from->un.long_value);
		}
		else if(from_type == jit_type_ulong)
		{
			result->un.ulong_value = jit_ulong_to_ulong(from->un.ulong_value);
		}
		else if(from_type == jit_type_nfloat)
		{
			result->un.ulong_value = jit_nfloat_to_ulong(from->un.nfloat_value);
		}
	}
	else if(to_type == jit_type_nfloat)
	{
		if(from_type == jit_type_int)
		{
			result->un.nfloat_value = jit_int_to_nfloat(from->un.int_value);
		}
		else if(from_type == jit_type_uint)
		{
			result->un.nfloat_value = jit_uint_to_nfloat(from->un.uint_value);
		}
		else if(from_type == jit_type_long)
		{
			result->un.nfloat_value = jit_long_to_nfloat(from->un.long_value);
		}
		else if(from_type == jit_type_ulong)
		{
			result->un.nfloat_value =
				jit_ulong_to_nfloat(from->un.ulong_value);
		}
		else if(from_type == jit_type_nfloat)
		{
			result->un.nfloat_value = from->un.nfloat_value;
		}
	}
}

static char *format_integer(char *buf, int is_neg, jit_ulong value)
{
	buf += 64;
	*(--buf) = '\0';
	if(value == 0)
	{
		*(--buf) = '0';
	}
	else
	{
		while(value != 0)
		{
			*(--buf) = '0' + (int)(value % 10);
			value /= 10;
		}
	}
	if(is_neg)
	{
		*(--buf) = '-';
	}
	return buf;
}

char *dpas_constant_name(jit_constant_t *value)
{
	jit_type_t type;
	char buf[64];
	char *result;
	if(value->type == dpas_type_nil)
	{
		return jit_strdup("nil");
	}
	else if(jit_type_is_pointer(value->type) &&
			jit_type_get_ref(value->type) == dpas_type_char)
	{
		return concat_strings
			(concat_strings(jit_strdup("\""),
							jit_strdup((char *)(value->un.ptr_value))),
			 jit_strdup("\""));
	}
	type = dpas_promote_type(value->type);
	if(type == jit_type_int)
	{
		if(value->un.int_value < 0)
		{
			result = format_integer
				(buf, 1, (jit_ulong)(-((jit_long)(value->un.int_value))));
		}
		else
		{
			result = format_integer
				(buf, 0, (jit_ulong)(jit_long)(value->un.int_value));
		}
	}
	else if(type == jit_type_uint)
	{
		result = format_integer
			(buf, 0, (jit_ulong)(value->un.uint_value));
	}
	else if(type == jit_type_long)
	{
		if(value->un.int_value < 0)
		{
			result = format_integer
				(buf, 1, (jit_ulong)(-(value->un.long_value)));
		}
		else
		{
			result = format_integer
				(buf, 0, (jit_ulong)(value->un.long_value));
		}
	}
	else if(type == jit_type_ulong)
	{
		result = format_integer(buf, 0, value->un.ulong_value);
	}
	else if(type == jit_type_nfloat)
	{
		sprintf(buf, "%g", (double)(value->un.nfloat_value));
		result = buf;
	}
	else
	{
		result = "unknown constant";
	}
	return jit_strdup(result);
}

int dpas_is_set_compatible(jit_type_t type)
{
	if(jit_type_get_tagged_kind(type) == DPAS_TAG_ENUM)
	{
		return (((dpas_enum *)jit_type_get_tagged_data(type))->num_elems <= 32);
	}
	else if(jit_type_get_tagged_kind(type) == DPAS_TAG_SUBRANGE)
	{
		dpas_subrange *range;
		if(jit_type_get_tagged_type(type) != jit_type_int)
		{
			return 0;
		}
		range = (dpas_subrange *)jit_type_get_tagged_data(type);
		if(range->first.un.int_value < 0 || range->first.un.int_value > 31 ||
		   range->last.un.int_value < 0 || range->last.un.int_value > 31)
		{
			return 0;
		}
		return 1;
	}
	return 0;
}

int dpas_type_is_numeric(jit_type_t type)
{
	if(type == jit_type_sbyte || type == jit_type_ubyte ||
	   type == jit_type_short || type == jit_type_ushort ||
	   type == jit_type_int || type == jit_type_uint ||
	   type == jit_type_long || type == jit_type_ulong ||
	   type == jit_type_float32 || type == jit_type_float64 ||
	   type == jit_type_nfloat)
	{
		return 1;
	}
	if(jit_type_get_tagged_kind(type) == DPAS_TAG_SUBRANGE)
	{
		return 1;
	}
	return 0;
}

int dpas_type_is_integer(jit_type_t type)
{
	if(type == jit_type_sbyte || type == jit_type_ubyte ||
	   type == jit_type_short || type == jit_type_ushort ||
	   type == jit_type_int || type == jit_type_uint ||
	   type == jit_type_long || type == jit_type_ulong)
	{
		return 1;
	}
	if(jit_type_get_tagged_kind(type) == DPAS_TAG_SUBRANGE)
	{
		return 1;
	}
	return 0;
}

int dpas_type_is_boolean(jit_type_t type)
{
	return (type == dpas_type_boolean || type == dpas_type_cboolean);
}

int dpas_type_is_record(jit_type_t type)
{
	type = jit_type_normalize(type);
	return (jit_type_is_struct(type) || jit_type_is_union(type));
}

int dpas_type_is_array(jit_type_t type)
{
	return (jit_type_get_tagged_kind(type) == DPAS_TAG_ARRAY);
}

int dpas_type_is_conformant_array(jit_type_t type)
{
	return (jit_type_get_tagged_kind(type) == DPAS_TAG_CONFORMANT_ARRAY);
}

jit_type_t dpas_type_is_var(jit_type_t type)
{
	if(jit_type_is_tagged(type) &&
	   jit_type_get_tagged_kind(type) == DPAS_TAG_VAR)
	{
		return jit_type_get_ref(jit_type_normalize(type));
	}
	return 0;
}

int dpas_type_identical(jit_type_t type1, jit_type_t type2, int normalize)
{
	if(normalize)
	{
		type1 = jit_type_normalize(type1);
		type2 = jit_type_normalize(type2);
	}
	if(jit_type_get_kind(type1) != jit_type_get_kind(type2))
	{
#ifdef JIT_NFLOAT_IS_DOUBLE
		if((jit_type_get_kind(type1) == JIT_TYPE_FLOAT64 ||
		    jit_type_get_kind(type1) == JIT_TYPE_NFLOAT) &&
		   (jit_type_get_kind(type2) == JIT_TYPE_FLOAT64 ||
		    jit_type_get_kind(type2) == JIT_TYPE_NFLOAT))
		{
			return 1;
		}
#endif
		return 0;
	}
	switch(jit_type_get_kind(type1))
	{
		case JIT_TYPE_STRUCT:
		case JIT_TYPE_UNION:
		{
			if(jit_type_get_size(type1) != jit_type_get_size(type2))
			{
				return 0;
			}
		}
		break;

		case JIT_TYPE_SIGNATURE:
		{
			/* TODO */
		}
		break;

		case JIT_TYPE_PTR:
		{
			return dpas_type_identical
				(jit_type_get_ref(type1), jit_type_get_ref(type2), 0);
		}
		/* Not reached */

		case JIT_TYPE_FIRST_TAGGED + DPAS_TAG_NAME:
		{
			if(jit_stricmp((char *)jit_type_get_tagged_data(type1),
						   (char *)jit_type_get_tagged_data(type2)) != 0)
			{
				return 0;
			}
		}
		break;

		case JIT_TYPE_FIRST_TAGGED + DPAS_TAG_VAR:
		{
			return dpas_type_identical
				(jit_type_get_tagged_type(type1),
				 jit_type_get_tagged_type(type2), 0);
		}
		/* Not reached */

		case JIT_TYPE_FIRST_TAGGED + DPAS_TAG_SUBRANGE:
		{
		}
		break;

		case JIT_TYPE_FIRST_TAGGED + DPAS_TAG_ENUM:
		{
			if(jit_stricmp
				(((dpas_enum *)jit_type_get_tagged_data(type1))->name,
				 ((dpas_enum *)jit_type_get_tagged_data(type2))->name) != 0)
			{
				return 0;
			}
		}
		break;

		case JIT_TYPE_FIRST_TAGGED + DPAS_TAG_SET:
		{
			return dpas_type_identical
				((jit_type_t)jit_type_get_tagged_data(type1),
				 (jit_type_t)jit_type_get_tagged_data(type2), 0);
		}
		/* Not reached */

		case JIT_TYPE_FIRST_TAGGED + DPAS_TAG_ARRAY:
		{
			/* TODO */
		}
		break;

		case JIT_TYPE_FIRST_TAGGED + DPAS_TAG_CONFORMANT_ARRAY:
		{
			/* TODO */
		}
		break;
	}
	return 1;
}
