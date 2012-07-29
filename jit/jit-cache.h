/*
 * jit-cache.h - Translated method cache implementation.
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
 * Create a method cache.  Returns NULL if out of memory.
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
 * Destroy a method cache.
 */
void _jit_cache_destroy(jit_cache_t cache);

/*
 * Start output of a function.  The "restart_count" value should be
 * equal to zero unless the method is being recompiled because it did
 * not fit last time into the memory. In the later case the value
 * should gradually increase until either the methods fits or the
 * maximum restart_count value is exceeded.
 */
int _jit_cache_start_function(jit_cache_t cache,
			      jit_function_t func,
			      int restart_count);

/*
 * End output of a function.   Returns zero if a restart is needed.
 */
int _jit_cache_end_function(jit_cache_t cache, int result);

/*
 * Get the boundary between used and free code space.
 */
void *_jit_cache_get_code_break(jit_cache_t cache);

/*
 * Set the boundary between used and free code space.
 */
void _jit_cache_set_code_break(jit_cache_t cache, void *ptr);

/*
 * Get the end address of the free code space.
 */
void *_jit_cache_get_code_limit(jit_cache_t cache);

/*
 * Allocate "size" bytes of storage in the function cache's
 * auxiliary data area.  Returns NULL if insufficient space
 * to satisfy the request.  It may be possible to satisfy
 * the request after a restart.
 */
void *_jit_cache_alloc_data(jit_cache_t cache,
			    unsigned long size,
			    unsigned long align);

/*
 * Allocate "size" bytes of storage when we aren't currently
 * translating a method.
 */
void *_jit_cache_alloc_no_method
	(jit_cache_t cache, unsigned long size, unsigned long align);

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
