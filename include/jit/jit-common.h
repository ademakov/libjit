/*
 * jit-common.h - Common type definitions that are used by the JIT.
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

#ifndef	_JIT_COMMON_H
#define	_JIT_COMMON_H

#include <jit/jit-defs.h>

#ifdef	__cplusplus
extern	"C" {
#endif

/*
 * Opaque structure that represents a context.
 */
typedef struct _jit_context *jit_context_t;

/*
 * Opaque structure that represents a function.
 */
typedef struct _jit_function *jit_function_t;

/*
 * Opaque type that represents the compiled form of a function.
 */
typedef void *jit_function_compiled_t;

/*
 * Opaque structure that represents a block.
 */
typedef struct _jit_block *jit_block_t;

/*
 * Opaque structure that represents an instruction.
 */
typedef struct _jit_insn *jit_insn_t;

/*
 * Opaque structure that represents a value.
 */
typedef struct _jit_value *jit_value_t;

/*
 * Opaque structure that represents a type descriptor.
 */
typedef struct _jit_type *jit_type_t;

/*
 * Opaque type that represents an exception stack trace.
 */
typedef struct jit_stack_trace *jit_stack_trace_t;

/*
 * Block label identifier.
 */
typedef jit_nuint jit_label_t;

/*
 * Value that represents an undefined label.
 */
#define	jit_label_undefined		((jit_label_t)~((jit_uint)0))

/*
 * Function that is used to free user-supplied metadata.
 */
typedef void (*jit_meta_free_func)(void *data);

/*
 * Function that is used to compile a function on demand.
 * Returns zero if the compilation process failed for some reason.
 */
typedef int (*jit_on_demand_func)(jit_function_t func);

#ifdef	__cplusplus
};
#endif

#endif /* _JIT_COMMON_H */
