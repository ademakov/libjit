/*
 * dpas-types.h - Type handling for Dynamic Pascal.
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

#ifndef	_DPAS_TYPES_H
#define	_DPAS_TYPES_H

#ifdef	__cplusplus
extern	"C" {
#endif

/*
 * Special type tags that are used in this language.
 */
#define	DPAS_TAG_BOOLEAN			1
#define	DPAS_TAG_CBOOLEAN			2
#define	DPAS_TAG_CHAR				3
#define	DPAS_TAG_NIL				4
#define	DPAS_TAG_NAME				5
#define	DPAS_TAG_VAR				6
#define	DPAS_TAG_SUBRANGE			7
#define	DPAS_TAG_ENUM				8
#define	DPAS_TAG_SET				9
#define	DPAS_TAG_ARRAY				10
#define	DPAS_TAG_CONFORMANT_ARRAY	11

/*
 * Define some special integer types that are distinguished from normal ones.
 */
extern jit_type_t dpas_type_boolean;
extern jit_type_t dpas_type_cboolean;
extern jit_type_t dpas_type_char;
extern jit_type_t dpas_type_string;
extern jit_type_t dpas_type_address;
extern jit_type_t dpas_type_nil;
extern jit_type_t dpas_type_size_t;
extern jit_type_t dpas_type_ptrdiff_t;

/*
 * Information block for a subrange.
 */
typedef struct
{
	jit_constant_t	first;
	jit_constant_t	last;

} dpas_subrange;

/*
 * Information block for an enumeration.
 */
typedef struct
{
	char		   *name;
	int				num_elems;

} dpas_enum;

/*
 * Information block for an array.
 */
typedef struct
{
	jit_type_t	   *bounds;
	int				num_bounds;

} dpas_array;

/*
 * Information block for a conformant array.
 */
typedef struct
{
	int				num_bounds;
	int				is_packed;

} dpas_conformant_array;

/*
 * Initialize the standard types.
 */
void dpas_init_types(void);

/*
 * Find a field within a record type.  This is similar to
 * "jit_type_find_name", except that it ignores case.
 */
unsigned int dpas_type_find_name(jit_type_t type, const char *name);

/*
 * Get the type and offset of a field within a record type.
 */
jit_type_t dpas_type_get_field(jit_type_t type, const char *name,
							   jit_nint *offset);

/*
 * Get the name of a type, for printing in error messages.
 */
char *dpas_type_name(jit_type_t type);

/*
 * Get the name of a type, combined with a variable name.
 */
char *dpas_type_name_with_var(const char *name, jit_type_t type);

/*
 * Promote a numeric type to its best form for arithmetic.  e.g. byte -> int.
 */
jit_type_t dpas_promote_type(jit_type_t type);

/*
 * Infer a common type for a binary operation.  Returns NULL if not possible.
 */
jit_type_t dpas_common_type(jit_type_t type1, jit_type_t type2, int int_only);

/*
 * Create a subrange type.
 */
jit_type_t dpas_create_subrange(jit_type_t underlying, dpas_subrange *values);

/*
 * Create an enumerated type.
 */
jit_type_t dpas_create_enum(jit_type_t underlying, int num_elems);

/*
 * Get the size of a range type that is being used for an array bound.
 */
jit_nuint dpas_type_range_size(jit_type_t type);

/*
 * Create an array type.
 */
jit_type_t dpas_create_array(jit_type_t *bounds, int num_bounds,
							 jit_type_t elem_type);

/*
 * Create a conformant array type.
 */
jit_type_t dpas_create_conformant_array(jit_type_t elem_type, int num_bounds,
							 			int is_packed);

/*
 * Get an array's element type.
 */
jit_type_t dpas_type_get_elem(jit_type_t type);

/*
 * Get an array's rank.
 */
int dpas_type_get_rank(jit_type_t type);

/*
 * Set the name of an enumerated or record type.  Ignored for other types,
 * or types that already have a name.
 */
void dpas_type_set_name(jit_type_t type, const char *name);

/*
 * Convert a constant value into a new type.
 */
void dpas_convert_constant(jit_constant_t *result, jit_constant_t *from,
						   jit_type_t to_type);

/*
 * Get the name form of a constant value.
 */
char *dpas_constant_name(jit_constant_t *value);

/*
 * Determine if a type is set-compatible.  It must be an enumerated
 * or subrange type with 32 or less members.
 */
int dpas_is_set_compatible(jit_type_t type);

/*
 * Determine if a type is numeric, integer, or boolean.
 */
int dpas_type_is_numeric(jit_type_t type);
int dpas_type_is_integer(jit_type_t type);
int dpas_type_is_boolean(jit_type_t type);

/*
 * Determine if a type is a record.
 */
int dpas_type_is_record(jit_type_t type);

/*
 * Determine if a type is an array.
 */
int dpas_type_is_array(jit_type_t type);
int dpas_type_is_conformant_array(jit_type_t type);

/*
 * Determine if a type is a "var" parameter, and return its element type.
 */
jit_type_t dpas_type_is_var(jit_type_t type);

/*
 * Determine if two types are identical.
 */
int dpas_type_identical(jit_type_t type1, jit_type_t type2, int normalize);

#ifdef	__cplusplus
};
#endif

#endif	/* _DPAS_TYPES_H */
