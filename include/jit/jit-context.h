/*
 * jit-context.h - Functions for manipulating JIT contexts.
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

#ifndef	_JIT_CONTEXT_H
#define	_JIT_CONTEXT_H

#include <jit/jit-common.h>

#ifdef	__cplusplus
extern	"C" {
#endif

jit_context_t jit_context_create(void) JIT_NOTHROW;
void jit_context_destroy(jit_context_t context) JIT_NOTHROW;
int jit_context_supports_threads(jit_context_t context) JIT_NOTHROW;
void jit_context_build_start(jit_context_t context) JIT_NOTHROW;
void jit_context_build_end(jit_context_t context) JIT_NOTHROW;
int jit_context_set_meta
	(jit_context_t context, int type, void *data,
	 jit_meta_free_func free_data) JIT_NOTHROW;
int jit_context_set_meta_numeric
	(jit_context_t context, int type, jit_nuint data) JIT_NOTHROW;
void *jit_context_get_meta(jit_context_t context, int type) JIT_NOTHROW;
jit_nuint jit_context_get_meta_numeric
	(jit_context_t context, int type) JIT_NOTHROW;
void jit_context_free_meta(jit_context_t context, int type) JIT_NOTHROW;
void jit_context_enable_all_breakpoints
		(jit_context_t context, int flag) JIT_NOTHROW;
int jit_context_all_breakpoints_enabled(jit_context_t context) JIT_NOTHROW;

/*
 * Standard meta values for builtin configurable options.
 */
#define	JIT_OPTION_CACHE_LIMIT		10000
#define	JIT_OPTION_CACHE_PAGE_SIZE	10001
#define	JIT_OPTION_PRE_COMPILE		10002
#define	JIT_OPTION_DONT_FOLD		10003
#define	JIT_OPTION_DEBUG_HOOK		10004

/*
 * Prototype for debug hook functions.
 */
typedef void (*jit_debug_hook_func)
	(jit_function_t func, jit_nint data1, jit_nint data2);

#ifdef	__cplusplus
};
#endif

#endif /* _JIT_CONTEXT_H */
