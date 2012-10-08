/*
 * jit-cache.h - Translated function cache implementation.
 *
 * Copyright (C) 2002, 2004, 2008  Southern Storm Software, Pty Ltd.
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

#ifndef	_JIT_CACHE_H
#define	_JIT_CACHE_H

#ifdef	__cplusplus
extern	"C" {
#endif

/*
 * Result values for "_jit_cache_start_function" and "_jit_cache_end_function".
 */
#define	JIT_CACHE_OK		0	/* Function is OK */
#define	JIT_CACHE_RESTART	1	/* Restart is required */
#define	JIT_CACHE_TOO_BIG	2	/* Function is too big for the cache */
#define JIT_CACHE_ERROR		3	/* Other error */

/*
 * Create a code cache.  Returns NULL if out of memory.
 * If "limit" is non-zero, then it specifies the maximum
 * size of the cache in bytes.  If "cache_page_size" is
 * non-zero, then it indicates the dafault/minimum cache
 * page size.  If "max_page_factor" is not zero, then it
 * indicates the maximum cache page size as multiple of
 * "max_page_factor" and "cache_page_size".
 */
jit_cache_t _jit_cache_create(long limit,
			      long cache_page_size,
			      int max_page_factor);

/*
 * Destroy a code cache.
 */
void _jit_cache_destroy(jit_cache_t cache);

/*
 * Request to allocate more code cache space.
 *
 * The "count" value should normally be zero except if just after the last
 * call a function is being recompiled again because it is still too big
 * to fit into the available cache space. In this case the "count" value
 * should gradually increase.
 */
void _jit_cache_extend(jit_cache_t cache, int count);

/*
 * Allocate a function information structure.
 */
jit_function_t _jit_cache_alloc_function(jit_cache_t cache);

/*
 * Release a function information structure.
 */
void _jit_cache_free_function(jit_cache_t cache, jit_function_t func);

/*
 * Start output of a function.
 */
int _jit_cache_start_function(jit_cache_t cache, jit_function_t func);

/*
 * End output of a function. 
 */
int _jit_cache_end_function(jit_cache_t cache, int result);

/*
 * Get the start address of memory available for function code generation.
 *
 * This function is only called betweed _jit_cache_start_function() and
 * corresponding _jit_cache_end_function() calls.
 *
 * Initially it should return the start address of the allocated memory.
 * Then the address may be moved forward with _jit_cache_set_code_break()
 * calls.
 */
void *_jit_cache_get_code_break(jit_cache_t cache);

/*
 * Set the address of memory yet available for function code generation.
 *
 * This function is only called betweed _jit_cache_start_function() and
 * corresponding _jit_cache_end_function() calls.
 *
 * The given address must be greater than or equal to its last value as
 * returned by the _jit_cache_get_code_break() call and also less than or
 * equal to the memory the address returned by _jit_cache_get_code_limit()
 * call.
 *
 * This function is to be used in two cases.  First, on the end of code
 * generation just before the _jit_cache_end_function() call. Second,
 * before allocating data with _jit_cache_alloc_data() calls. This lets
 * the cache know how much space was actually used and how much is still
 * free.
 */
void _jit_cache_set_code_break(jit_cache_t cache, void *ptr);

/*
 * Get the end address of memory available for function code generation.
 *
 * This function is only called betweed _jit_cache_start_function() and
 * corresponding _jit_cache_end_function() calls.
 *
 * The available memory may change if during code generation there were
 * _jit_cache_alloc_data() calls.  So after such calls available memory
 * should be rechecked.
 */
void *_jit_cache_get_code_limit(jit_cache_t cache);

/*
 * Allocate "size" bytes of memory in the data area.  Returns NULL if
 * there is insufficient space to satisfy the request.
 */
void *_jit_cache_alloc_data(jit_cache_t cache,
			    unsigned long size,
			    unsigned long align);

/*
 * Allocate memory for a trampoline.
 *
 * The required size and alignment can be determined with these functions:
 * jit_get_trampoline_size(), jit_get_trampoline_alignment().
 */
void *_jit_cache_alloc_trampoline(jit_cache_t cache);

/*
 * Free memory used by a trampoline.
 */
void _jit_cache_free_trampoline(jit_cache_t cache, void *trampoline);

/*
 * Allocate memory for a closure.
 * 
 * The required size and alignment can be determined with these functions:
 * jit_get_closure_size(), jit_get_closure_alignment().
 */
void *_jit_cache_alloc_closure(jit_cache_t cache);

/*
 * Free memory used by a closure.
 */
void _jit_cache_free_closure(jit_cache_t cache, void *closure);

/*
 * Find the method that is associated with a particular
 * program counter.  Returns NULL if the PC is not associated
 * with a method within the cache.
 */
jit_function_t _jit_cache_get_function(jit_cache_t cache, void *pc);


#ifdef	__cplusplus
};
#endif

#endif	/* _JIT_CACHE_H */
