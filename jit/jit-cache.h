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
 * Opaque method cache type.
 */
typedef struct jit_cache *jit_cache_t;

/*
 * Writing position within a cache.
 */
typedef struct
{
	jit_cache_t    cache;			/* Cache this position is attached to */
	unsigned char  *ptr;			/* Current code pointer */
	unsigned char  *limit;			/* Limit of the current page */

} jit_cache_posn;

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
 * Determine if the cache is full.  The "posn" value should
 * be supplied while translating a method, or be NULL otherwise.
 */
int _jit_cache_is_full(jit_cache_t cache, jit_cache_posn *posn);

/*
 * Return values for "_jit_cache_start_method" and "_jit_cache_end_method".
 */
#define	JIT_CACHE_OK		0		/* Function is OK */
#define	JIT_CACHE_RESTART	1		/* Restart is required */
#define	JIT_CACHE_TOO_BIG	2		/* Function is too big for the cache */

/*
 * Start output of a method, returning a cache position.
 * The "page_factor" value should be equal to zero unless
 * the method is being recompiled because it did not fit
 * into the current page. In the later case the value
 * should gradually increase until either the methods fits
 * or the maximum page factor value is exceeded.
 * The "align" value indicates the default alignment for
 * the start of the method.  The "cookie" value is a
 * cookie for referring to the method.  Returns the
 * method entry point, or NULL if the cache is full.
 */
int _jit_cache_start_method(jit_cache_t cache,
			    jit_cache_posn *posn,
			    int page_factor,
			    int align,
			    void *cookie);

/*
 * End output of a method.  Returns zero if a restart.
 */
int _jit_cache_end_method(jit_cache_posn *posn);

/*
 * Allocate "size" bytes of storage in the method cache's
 * auxillary data area.  Returns NULL if insufficient space
 * to satisfy the request.  It may be possible to satisfy
 * the request after a restart.
 */
void *_jit_cache_alloc(jit_cache_posn *posn, unsigned long size);

/*
 * Allocate "size" bytes of storage when we aren't currently
 * translating a method.
 */
void *_jit_cache_alloc_no_method
	(jit_cache_t cache, unsigned long size, unsigned long align);

/*
 * Align the method code on a particular boundary if the
 * difference between the current position and the aligned
 * boundary is less than "diff".  The "nop" value is used
 * to pad unused bytes.
 */
void _jit_cache_align(jit_cache_posn *posn, int align, int diff, int nop);

/*
 * Mark the current position with a bytecode offset value.
 */
void _jit_cache_mark_bytecode(jit_cache_posn *posn, unsigned long offset);

/*
 * Change to a new exception region within the current method.
 * The cookie will typically be NULL if no exception region.
 */
void _jit_cache_new_region(jit_cache_posn *posn, void *cookie);

/*
 * Set the exception region cookie for the current region.
 */
void _jit_cache_set_cookie(jit_cache_posn *posn, void *cookie);

/*
 * Find the method that is associated with a particular
 * program counter.  Returns NULL if the PC is not associated
 * with a method within the cache.  The exception region
 * cookie is returned in "*cookie", if "cookie" is not NULL.
 */
void *_jit_cache_get_method(jit_cache_t cache, void *pc, void **cookie);

/*
 * Get the start of a method with a particular starting PC.
 * Returns NULL if the PC could not be located.
 * NOTE: This function is not currently aware of the
 * possibility of multiple regions per function. To ensure
 * correct results the ``pc'' argument has to be in the
 * first region.
 */
void *_jit_cache_get_start_method(jit_cache_t cache, void *pc);

/*
 * Get the end of a method with a particular starting PC.
 * Returns NULL if the PC could not be located.
 */
void *_jit_cache_get_end_method(jit_cache_t cache, void *pc);

/*
 * Get a list of all method that are presently in the cache.
 * The list is terminated by a NULL, and must be free'd with
 * "ILFree".  Returns NULL if out of memory.
 */
void **_jit_cache_get_method_list(jit_cache_t cache);

/*
 * Get the native offset that is associated with a bytecode
 * offset within a method.  The value "start" indicates the
 * entry point for the method.  Returns JIT_CACHE_NO_OFFSET
 * if the native offset could not be determined.
 */
#define	JIT_CACHE_NO_OFFSET		(~((unsigned long)0))
unsigned long _jit_cache_get_native(jit_cache_t cache, void *start,
						   			unsigned long offset, int exact);

/*
 * Get the bytecode offset that is associated with a native
 * offset within a method.  The value "start" indicates the
 * entry point for the method.  Returns JIT_CACHE_NO_OFFSET
 * if the bytecode offset could not be determined.
 */
unsigned long _jit_cache_get_bytecode(jit_cache_t cache, void *start,
							 		  unsigned long offset, int exact);

/*
 * Get the number of bytes currently in use in the method cache.
 */
unsigned long _jit_cache_get_size(jit_cache_t cache);

/*
 * Convert a return address into a program counter value
 * that can be used with "_jit_cache_get_method".  Normally
 * return addresses point to the next instruction after
 * an instruction that falls within a method region.  This
 * macro corrects for the "off by 1" address.
 */
#define	jit_cache_return_to_pc(addr)	\
			((void *)(((unsigned char *)(addr)) - 1))

/*
 * Output a single byte to the current method.
 */
#define	jit_cache_byte(posn,value)	\
			do { \
				if((posn)->ptr < (posn)->limit) \
				{ \
					*(((posn)->ptr)++) = (unsigned char)(value); \
				} \
			} while (0)

/*
 * Output a 16-bit word to the current method.
 */
#define	jit_cache_word16(posn,value)	\
			do { \
				if(((posn)->ptr + 1) < (posn)->limit) \
				{ \
					*((jit_ushort *)((posn)->ptr)) = (jit_ushort)(value); \
					(posn)->ptr += 2; \
				} \
				else \
				{ \
					(posn)->ptr = (posn)->limit; \
				} \
			} while (0)

/*
 * Output a 32-bit word to the current method.
 */
#define	jit_cache_word32(posn,value)	\
			do { \
				if(((posn)->ptr + 3) < (posn)->limit) \
				{ \
					*((jit_uint *)((posn)->ptr)) = (jit_uint)(value); \
					(posn)->ptr += 4; \
				} \
				else \
				{ \
					(posn)->ptr = (posn)->limit; \
				} \
			} while (0)

/*
 * Output a native word to the current method.
 */
#define	jit_cache_native(posn,value)	\
			do { \
				if(((posn)->ptr + sizeof(jit_nuint) - 1) < (posn)->limit) \
				{ \
					*((jit_nuint *)((posn)->ptr)) = (jit_nuint)(value); \
					(posn)->ptr += sizeof(jit_nuint); \
				} \
				else \
				{ \
					(posn)->ptr = (posn)->limit; \
				} \
			} while (0)

/*
 * Output a 64-bit word to the current method.
 */
#define	jit_cache_word64(posn,value)	\
			do { \
				if(((posn)->ptr + 7) < (posn)->limit) \
				{ \
					*((jit_ulong *)((posn)->ptr)) = (jit_ulong)(value); \
					(posn)->ptr += 8; \
				} \
				else \
				{ \
					(posn)->ptr = (posn)->limit; \
				} \
			} while (0)

/*
 * Get the output position within the current method.
 */
#define	jit_cache_get_posn(posn)	((posn)->ptr)

/*
 * Determine if there is sufficient space for N bytes in the current method.
 */
#define	jit_cache_check_for_n(posn,n)	\
				(((posn)->ptr + (n)) <= (posn)->limit)

/*
 * Mark the cache as full.
 */
#define	jit_cache_mark_full(posn)	\
			do { \
				(posn)->ptr = (posn)->limit; \
			} while (0)

#ifdef	__cplusplus
};
#endif

#endif	/* _JIT_CACHE_H */
