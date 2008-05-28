/*
 * jit-alloc.c - Memory allocation routines.
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

#include "jit-internal.h"
#include <config.h>
#ifdef HAVE_STDLIB_H
	#include <stdlib.h>
#endif
#ifndef JIT_WIN32_PLATFORM
#ifdef HAVE_SYS_TYPES_H
	#include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
	#include <unistd.h>
#endif
#ifdef HAVE_SYS_MMAN_H
	#include <sys/mman.h>
#endif
#ifdef HAVE_FCNTL_H
	#include <fcntl.h>
#endif
#else /* JIT_WIN32_PLATFORM */
	#include <windows.h>
	#include <io.h>
#endif
#if defined(HAVE_SYS_MMAN_H) && defined(HAVE_MMAP) && defined(HAVE_MUNMAP)
/*
 * Make sure that "MAP_ANONYMOUS" is correctly defined, because it
 * may not exist on some variants of Unix.
 */
#ifndef MAP_ANONYMOUS
    #ifdef MAP_ANON
        #define MAP_ANONYMOUS        MAP_ANON
    #endif
#endif
#ifdef MAP_ANONYMOUS
#define JIT_USE_MMAP
#endif
#endif


/*@
 * @section Memory allocation
 *
 * The @code{libjit} library provides an interface to the traditional
 * system @code{malloc} routines.  All heap allocation in @code{libjit}
 * goes through these functions.  If you need to perform some other kind
 * of memory allocation, you can replace these functions with your
 * own versions.
@*/

/*@
 * @deftypefun {void *} jit_malloc (unsigned int @var{size})
 * Allocate @var{size} bytes of memory from the heap.
 * @end deftypefun
 *
 * @deftypefun {type *} jit_new (@var{type})
 * Allocate @code{sizeof(@var{type})} bytes of memory from the heap and
 * cast the return pointer to @code{@var{type} *}.  This is a macro that
 * wraps up the underlying @code{jit_malloc} function and is less
 * error-prone when allocating structures.
 * @end deftypefun
@*/
void *jit_malloc(unsigned int size)
{
	return malloc(size);
}

/*@
 * @deftypefun {void *} jit_calloc (unsigned int @var{num}, unsigned int @var{size})
 * Allocate @code{@var{num} * @var{size}} bytes of memory from the heap and clear
 * them to zero.
 * @end deftypefun
 *
 * @deftypefun {type *} jit_cnew (@var{type})
 * Allocate @code{sizeof(@var{type})} bytes of memory from the heap and
 * cast the return pointer to @code{@var{type} *}.  The memory is cleared
 * to zero.
 * @end deftypefun
@*/
void *jit_calloc(unsigned int num, unsigned int size)
{
	return calloc(num, size);
}

/*@
 * @deftypefun {void *} jit_realloc (void *@var{ptr}, unsigned int @var{size})
 * Re-allocate the memory at @var{ptr} to be @var{size} bytes in size.
 * The memory block at @var{ptr} must have been allocated by a previous
 * call to @code{jit_malloc}, @code{jit_calloc}, or @code{jit_realloc}.
 * @end deftypefun
@*/
void *jit_realloc(void *ptr, unsigned int size)
{
	return realloc(ptr, size);
}

/*@
 * @deftypefun void jit_free (void *@var{ptr})
 * Free the memory at @var{ptr}.  It is safe to pass a NULL pointer.
 * @end deftypefun
@*/
void jit_free(void *ptr)
{
	if(ptr)
	{
		free(ptr);
	}
}

/*@
 * @deftypefun {void *} jit_malloc_exec (unsigned int @var{size})
 * Allocate a block of memory that is read/write/executable.  Such blocks
 * are used to store JIT'ed code, function closures, and other trampolines.
 * The size should be a multiple of @code{jit_exec_page_size()}.
 *
 * This will usually be identical to @code{jit_malloc}.  However,
 * some systems may need special handling to create executable code
 * segments, so this function must be used instead.
 *
 * You must never mix regular and executable segment allocation.  That is,
 * do not use @code{jit_free} to free the result of @code{jit_malloc_exec}.
 * @end deftypefun
@*/
void *jit_malloc_exec(unsigned int size)
{
#if defined(JIT_WIN32_PLATFORM)
	return VirtualAlloc(NULL, size,
			    MEM_COMMIT | MEM_RESERVE,
			    PAGE_EXECUTE_READWRITE);
#elif defined(JIT_USE_MMAP)
	void *ptr = mmap(0, size,
			 PROT_READ | PROT_WRITE | PROT_EXEC,
			 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if(ptr == (void *)-1)
	{
		return (void *)0;
	}
	return ptr;
#else
	return malloc(size);
#endif
}

/*@
 * @deftypefun void jit_free_exec (void *@var{ptr}, unsigned int @var{size})
 * Free a block of memory that was previously allocated by
 * @code{jit_malloc_exec}.  The @var{size} must be identical to the
 * original allocated size, as some systems need to know this information
 * to be able to free the block.
 * @end deftypefun
@*/
void jit_free_exec(void *ptr, unsigned int size)
{
	if(ptr)
	{
#if defined(JIT_WIN32_PLATFORM)
		VirtualFree(ptr, 0, MEM_RELEASE);
#elif defined(JIT_USE_MMAP)
		munmap(ptr, size);
#else
		free(ptr);
#endif
	}
}

/*@
 * @deftypefun void jit_flush_exec (void *@var{ptr}, unsigned int @var{size})
 * Flush the contents of the block at @var{ptr} from the CPU's
 * data and instruction caches.  This must be used after the code is
 * written to an executable code segment, but before the code is
 * executed, to prepare it for execution.
 * @end deftypefun
@*/
void jit_flush_exec(void *ptr, unsigned int size)
{

#define ROUND_BEG_PTR(p) \
	((void *)((((jit_nuint)(p)) / CLSIZE) * CLSIZE))
#define ROUND_END_PTR(p,s) \
	((void *)(((((jit_nuint)(p)) + (s) + CLSIZE - 1)/CLSIZE)*CLSIZE))

#if defined(__GNUC__)
#if defined(PPC)

#define CLSIZE 4
	/* Flush the CPU cache on PPC platforms */
	register unsigned char *p;
	register unsigned char *end;

	/* Flush the data out of the data cache */
	p   = ROUND_BEG_PTR (ptr);
	end = ROUND_END_PTR (p, size);
	while (p < end)
	{
		__asm__ __volatile__ ("dcbst 0,%0" :: "r"(p));
		p += CLSIZE;
	}
	__asm__ __volatile__ ("sync");

	/* Invalidate the cache lines in the instruction cache */
	p = ROUND_BEG_PTR (ptr);
	while (p < end)
	{
		__asm__ __volatile__ ("icbi 0,%0; isync" :: "r"(p));
		p += CLSIZE;
	}
	__asm__ __volatile__ ("isync");

#elif defined(__sparc)

#define CLSIZE 4

	/* Flush the CPU cache on sparc platforms */
	register unsigned char *p   = ROUND_BEG_PTR (ptr);
	register unsigned char *end = ROUND_END_PTR (p, size);
	__asm__ __volatile__ ("stbar");
	while (p < end)
	{
		__asm__ __volatile__ ("flush %0" :: "r"(p));
		p += CLSIZE;
	}
	__asm__ __volatile__ ("nop; nop; nop; nop; nop");

#elif (defined(__arm__) || defined(__arm)) && defined(linux)

	/* ARM Linux has a "cacheflush" system call */
	/* R0 = start of range, R1 = end of range, R3 = flags */
	/* flags = 0 indicates data cache, flags = 1 indicates both caches */
	__asm __volatile ("mov r0, %0\n"
	                  "mov r1, %1\n"
					  "mov r2, %2\n"
					  "swi 0x9f0002       @ sys_cacheflush"
					  : /* no outputs */
					  : "r" (ptr),
					    "r" (((int)ptr) + (int)size),
						"r" (0)
					  : "r0", "r1", "r3" );

#elif (defined(__ia64) || defined(__ia64__)) && defined(linux)
#define CLSIZE 32
	register unsigned char *p   = ROUND_BEG_PTR (ptr);
	register unsigned char *end = ROUND_END_PTR (p, size);
	while(p < end)
	{
		asm volatile("fc %0" :: "r"(p));
		p += CLSIZE;
	}
	asm volatile(";;sync.i;;srlz.i;;");
#endif
#endif	/* __GNUC__ */
}

/*@
 * @deftypefun {unsigned int} jit_exec_page_size (void)
 * Get the page allocation size for the system.  This is the preferred
 * unit when making calls to @code{jit_malloc_exec}.  It is not
 * required that you supply a multiple of this size when allocating,
 * but it can lead to better performance on some systems.
 * @end deftypefun
@*/
unsigned int jit_exec_page_size(void)
{
#ifndef JIT_WIN32_PLATFORM
	/* Get the page size using a Unix-like sequence */
	#ifdef HAVE_GETPAGESIZE
		return (unsigned long)getpagesize();
	#else
		#ifdef NBPG
			return NBPG;
		#else
			#ifdef PAGE_SIZE
				return PAGE_SIZE;
			#else
				return 4096;
			#endif
		#endif
	#endif
#else
	/* Get the page size from a Windows-specific API */
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	return (unsigned long)(sysInfo.dwPageSize);
#endif
}
