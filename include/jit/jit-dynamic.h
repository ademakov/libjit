/*
 * jit-dynamic.h - Managing dynamic libraries and cross-language invocation.
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

#ifndef	_JIT_DYNAMIC_H
#define	_JIT_DYNAMIC_H

#include <jit/jit-common.h>

#ifdef	__cplusplus
extern	"C" {
#endif

/*
 * Dynamic library routines.
 */
typedef void *jit_dynlib_handle_t;
jit_dynlib_handle_t jit_dynlib_open(const char *name) JIT_NOTHROW;
void jit_dynlib_close(jit_dynlib_handle_t handle) JIT_NOTHROW;
void *jit_dynlib_get_symbol
	(jit_dynlib_handle_t handle, const char *symbol) JIT_NOTHROW;
const char *jit_dynlib_get_suffix(void) JIT_NOTHROW;
void jit_dynlib_set_debug(int flag) JIT_NOTHROW;

/*
 * C++ name mangling definitions.
 */
#define	JIT_MANGLE_PUBLIC			0x0001
#define	JIT_MANGLE_PROTECTED		0x0002
#define	JIT_MANGLE_PRIVATE			0x0003
#define	JIT_MANGLE_STATIC			0x0008
#define	JIT_MANGLE_VIRTUAL			0x0010
#define	JIT_MANGLE_CONST			0x0020
#define	JIT_MANGLE_EXPLICIT_THIS	0x0040
#define	JIT_MANGLE_IS_CTOR			0x0080
#define	JIT_MANGLE_IS_DTOR			0x0100
#define	JIT_MANGLE_BASE				0x0200
char *jit_mangle_global_function
	(const char *name, jit_type_t signature, int form) JIT_NOTHROW;
char *jit_mangle_member_function
	(const char *class_name, const char *name,
	 jit_type_t signature, int form, int flags) JIT_NOTHROW;

#ifdef	__cplusplus
};
#endif

#endif /* _JIT_DYNAMIC_H */
