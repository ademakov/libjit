/*
 * jit-type.h - Functions for manipulating type descriptors.
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

#ifndef	_JIT_TYPE_H
#define	_JIT_TYPE_H

#include <jit/jit-common.h>

#ifdef	__cplusplus
extern	"C" {
#endif

/*
 * Pre-defined type descriptors.
 */
extern jit_type_t const jit_type_void;
extern jit_type_t const jit_type_sbyte;
extern jit_type_t const jit_type_ubyte;
extern jit_type_t const jit_type_short;
extern jit_type_t const jit_type_ushort;
extern jit_type_t const jit_type_int;
extern jit_type_t const jit_type_uint;
extern jit_type_t const jit_type_nint;
extern jit_type_t const jit_type_nuint;
extern jit_type_t const jit_type_long;
extern jit_type_t const jit_type_ulong;
extern jit_type_t const jit_type_float32;
extern jit_type_t const jit_type_float64;
extern jit_type_t const jit_type_nfloat;
extern jit_type_t const jit_type_void_ptr;

/*
 * Type descriptors for the system "char", "int", "long", etc types.
 * These are defined to one of the above values.
 */
extern jit_type_t const jit_type_sys_bool;
extern jit_type_t const jit_type_sys_char;
extern jit_type_t const jit_type_sys_schar;
extern jit_type_t const jit_type_sys_uchar;
extern jit_type_t const jit_type_sys_short;
extern jit_type_t const jit_type_sys_ushort;
extern jit_type_t const jit_type_sys_int;
extern jit_type_t const jit_type_sys_uint;
extern jit_type_t const jit_type_sys_long;
extern jit_type_t const jit_type_sys_ulong;
extern jit_type_t const jit_type_sys_longlong;
extern jit_type_t const jit_type_sys_ulonglong;
extern jit_type_t const jit_type_sys_float;
extern jit_type_t const jit_type_sys_double;
extern jit_type_t const jit_type_sys_long_double;

/*
 * Special tag types.
 */
#define	JIT_TYPETAG_NAME			10000
#define	JIT_TYPETAG_STRUCT_NAME		10001
#define	JIT_TYPETAG_CONST			10002
#define	JIT_TYPETAG_VOLATILE		10003
#define	JIT_TYPETAG_REFERENCE		10004
#define	JIT_TYPETAG_OUTPUT			10005
#define	JIT_TYPETAG_RESTRICT		10006
#define	JIT_TYPETAG_SYS_BOOL		10007
#define	JIT_TYPETAG_SYS_CHAR		10008
#define	JIT_TYPETAG_SYS_SCHAR		10009
#define	JIT_TYPETAG_SYS_UCHAR		10010
#define	JIT_TYPETAG_SYS_SHORT		10011
#define	JIT_TYPETAG_SYS_USHORT		10012
#define	JIT_TYPETAG_SYS_INT			10013
#define	JIT_TYPETAG_SYS_UINT		10014
#define	JIT_TYPETAG_SYS_LONG		10015
#define	JIT_TYPETAG_SYS_ULONG		10016
#define	JIT_TYPETAG_SYS_LONGLONG	10017
#define	JIT_TYPETAG_SYS_ULONGLONG	10018
#define	JIT_TYPETAG_SYS_FLOAT		10019
#define	JIT_TYPETAG_SYS_DOUBLE		10020
#define	JIT_TYPETAG_SYS_LONGDOUBLE	10021

/*
 * ABI types for function signatures.
 */
typedef enum
{
	jit_abi_cdecl,			/* Native C calling conventions */
	jit_abi_vararg,			/* Native C with optional variable arguments */
	jit_abi_stdcall,		/* Win32 STDCALL (same as cdecl if not Win32) */
	jit_abi_fastcall		/* Win32 FASTCALL (same as cdecl if not Win32) */

} jit_abi_t;

/*
 * External function declarations.
 */
jit_type_t jit_type_copy(jit_type_t type) JIT_NOTHROW;
void jit_type_free(jit_type_t type) JIT_NOTHROW;
jit_type_t jit_type_create_struct
	(jit_type_t *fields, unsigned int num_fields, int incref) JIT_NOTHROW;
jit_type_t jit_type_create_union
	(jit_type_t *fields, unsigned int num_fields, int incref) JIT_NOTHROW;
jit_type_t jit_type_create_signature
	(jit_abi_t abi, jit_type_t return_type, jit_type_t *params,
	 unsigned int num_params, int incref) JIT_NOTHROW;
jit_type_t jit_type_create_pointer(jit_type_t type, int incref) JIT_NOTHROW;
jit_type_t jit_type_create_tagged
	(jit_type_t type, int kind, void *data,
	 jit_meta_free_func free_func, int incref) JIT_NOTHROW;
int jit_type_set_names
	(jit_type_t type, char **names, unsigned int num_names) JIT_NOTHROW;
void jit_type_set_size_and_alignment
	(jit_type_t type, jit_nint size, jit_nint alignment) JIT_NOTHROW;
void jit_type_set_offset
	(jit_type_t type, unsigned int field_index, jit_nuint offset) JIT_NOTHROW;
jit_nuint jit_type_get_size(jit_type_t type) JIT_NOTHROW;
jit_nuint jit_type_get_alignment(jit_type_t type) JIT_NOTHROW;
unsigned int jit_type_num_fields(jit_type_t type) JIT_NOTHROW;
jit_type_t jit_type_get_field
	(jit_type_t type, unsigned int field_index) JIT_NOTHROW;
jit_nuint jit_type_get_offset
	(jit_type_t type, unsigned int field_index) JIT_NOTHROW;
const char *jit_type_get_name(jit_type_t type, unsigned int index) JIT_NOTHROW;
#define	JIT_INVALID_NAME	(~((unsigned int)0))
unsigned int jit_type_find_name(jit_type_t type, const char *name) JIT_NOTHROW;
unsigned int jit_type_num_params(jit_type_t type) JIT_NOTHROW;
jit_type_t jit_type_get_return(jit_type_t type) JIT_NOTHROW;
jit_type_t jit_type_get_param
	(jit_type_t type, unsigned int param_index) JIT_NOTHROW;
jit_abi_t jit_type_get_abi(jit_type_t type) JIT_NOTHROW;
jit_type_t jit_type_get_ref(jit_type_t type) JIT_NOTHROW;
jit_type_t jit_type_get_tagged_type(jit_type_t type) JIT_NOTHROW;
void jit_type_set_tagged_type
	(jit_type_t type, jit_type_t underlying, int incref) JIT_NOTHROW;
int jit_type_get_tagged_kind(jit_type_t type) JIT_NOTHROW;
void *jit_type_get_tagged_data(jit_type_t type) JIT_NOTHROW;
void jit_type_set_tagged_data
	(jit_type_t type, void *data, jit_meta_free_func free_func) JIT_NOTHROW;
int jit_type_is_primitive(jit_type_t type) JIT_NOTHROW;
int jit_type_is_struct(jit_type_t type) JIT_NOTHROW;
int jit_type_is_union(jit_type_t type) JIT_NOTHROW;
int jit_type_is_signature(jit_type_t type) JIT_NOTHROW;
int jit_type_is_pointer(jit_type_t type) JIT_NOTHROW;
int jit_type_is_tagged(jit_type_t type) JIT_NOTHROW;
jit_nuint jit_type_best_alignment(void) JIT_NOTHROW;
jit_type_t jit_type_normalize(jit_type_t type) JIT_NOTHROW;
jit_type_t jit_type_remove_tags(jit_type_t type) JIT_NOTHROW;
jit_type_t jit_type_promote_int(jit_type_t type) JIT_NOTHROW;
int jit_type_return_via_pointer(jit_type_t type) JIT_NOTHROW;
int jit_type_has_tag(jit_type_t type, int kind) JIT_NOTHROW;

#ifdef	__cplusplus
};
#endif

#endif /* _JIT_TYPE_H */
