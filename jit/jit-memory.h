/*
 * jit-memory.h - Memory copy/set/compare routines.
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

#ifndef	_JIT_MEMORY_H
#define	_JIT_MEMORY_H

#include <config.h>
#ifdef HAVE_STRING_H
	#include <string.h>
#else
	#ifdef HAVE_STRINGS_H
		#include <strings.h>
	#endif
#endif
#ifdef HAVE_MEMORY_H
	#include <memory.h>
#endif

#ifdef	__cplusplus
extern	"C" {
#endif

/*
 * Macros that replace the routines in <jit/jit-util.h>
 * with direct calls on the underlying library functions.
 */
#ifdef HAVE_MEMSET
	#define	jit_memset(dest,ch,len)	(memset((dest), (ch), (len)))
	#define	jit_memzero(dest,len)	(memset((dest), 0, (len)))
#else
	#ifdef HAVE_BZERO
		#define	jit_memzero(dest,len)	(bzero((char *)(dest), (len)))
	#else
		#define	jit_memzero(dest,len)	(jit_memset((char *)(dest), 0, (len)))
	#endif
#endif
#ifdef HAVE_MEMCPY
	#define	jit_memcpy(dest,src,len)	(memcpy((dest), (src), (len)))
#endif
#ifdef HAVE_MEMMOVE
	#define	jit_memmove(dest,src,len)	(memmove((dest), (src), (len)))
#endif
#ifdef HAVE_MEMCMP
	#define	jit_memcmp(s1,s2,len)		(memcmp((s1), (s2), (len)))
#else
	#ifdef HAVE_BCMP
		#define	jit_memcmp(s1,s2,len)	\
					(bcmp((char *)(s1), (char *)(s2), (len)))
	#endif
#endif
#ifdef HAVE_MEMCHR
	#define	jit_memchr(str,ch,len)	(memchr((str), (ch), (len)))
#endif

#ifdef	__cplusplus
};
#endif

#endif	/* _JIT_MEMORY_H */
