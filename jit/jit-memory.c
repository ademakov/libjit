/*
 * jit-memory.c - Memory copy/set/compare routines.
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

#include "jit-memory.h"

/*
 * Undefine the macros in "jit-memory.h" so that we
 * can define the real function forms.
 */
#undef jit_memset
#undef jit_memcpy
#undef jit_memmove
#undef jit_memcmp
#undef jit_memchr

/*@
 * @section Memory set, copy, compare, etc
 * @cindex Memory operations
 *
 * The following functions are provided to set, copy, compare, and search
 * memory blocks.
@*/

/*@
 * @deftypefun {void *} jit_memset ({void *} dest, int ch, {unsigned int} len)
 * Set the @code{len} bytes at @code{dest} to the value @code{ch}.
 * Returns @code{dest}.
 * @end deftypefun
@*/
void *jit_memset(void *dest, int ch, unsigned int len)
{
#ifdef HAVE_MEMSET
	return memset(dest, ch, len);
#else
	unsigned char *d = (unsigned char *)dest;
	while(len > 0)
	{
		*d++ = (unsigned char)ch;
		--len;
	}
	return dest;
#endif
}

/*@
 * @deftypefun {void *} jit_memcpy ({void *} dest, {const void *} src, {unsigned int} len)
 * Copy the @code{len} bytes at @code{src} to @code{dest}.  Returns
 * @code{dest}.  The behavior is undefined if the blocks overlap
 * (use @code{jit_memmove} instead for that case).
 * @end deftypefun
@*/
void *jit_memcpy(void *dest, const void *src, unsigned int len)
{
#if defined(HAVE_MEMCPY)
	return memcpy(dest, src, len);
#elif defined(HAVE_BCOPY)
	bcopy(src, dest, len);
	return dest;
#else
	unsigned char *d = (unsigned char *)dest;
	const unsigned char *s = (const unsigned char *)src;
	while(len > 0)
	{
		*d++ = *s++;
		--len;
	}
	return dest;
#endif
}

/*@
 * @deftypefun {void *} jit_memmove ({void *} dest, {const void *} src, {unsigned int} len)
 * Copy the @code{len} bytes at @code{src} to @code{dest} and handle
 * overlapping blocks correctly.  Returns @code{dest}.
 * @end deftypefun
@*/
void *jit_memmove(void *dest, const void *src, unsigned int len)
{
#ifdef HAVE_MEMMOVE
	return memmove(dest, src, len);
#else
	unsigned char *d = (unsigned char *)dest;
	const unsigned char *s = (const unsigned char *)src;
	if(((const unsigned char *)d) < s)
	{
		while(len > 0)
		{
			*d++ = *s++;
			--len;
		}
	}
	else
	{
		d += len;
		s += len;
		while(len > 0)
		{
			*(--d) = *(--s);
			--len;
		}
	}
	return dest;
#endif
}

/*@
 * @deftypefun int jit_memcmp ({const void *} s1, {const void *} s2, {unsigned int} len)
 * Compare @code{len} bytes at @code{s1} and @code{s2}, returning a negative,
 * zero, or positive result depending upon their relationship.  It is
 * system-specific as to whether this function uses signed or unsigned
 * byte comparisons.
 * @end deftypefun
@*/
int jit_memcmp(const void *s1, const void *s2, unsigned int len)
{
#if defined(HAVE_MEMCMP)
	return memcmp(s1, s2, len);
#elif defined(HAVE_BCMP)
	return bcmp(s1, s2, len);
#else
	const unsigned char *str1 = (const unsigned char *)s1;
	const unsigned char *str2 = (const unsigned char *)s2;
	while(len > 0)
	{
		if(*str1 < *str2)
			return -1;
		else if(*str1 > *str2)
			return 1;
		++str1;
		++str2;
		--len;
	}
	return 0;
#endif
}

/*@
 * @deftypefun {void *} jit_memchr ({void *} str, int ch, {unsigned int} len)
 * Search the @code{len} bytes at @code{str} for the first instance of
 * the value @code{ch}.  Returns the location of @code{ch} if it was found,
 * or NULL if it was not found.
 * @end deftypefun
@*/
void *jit_memchr(const void *str, int ch, unsigned int len)
{
#ifdef HAVE_MEMCHR
	return memchr(str, ch, len);
#else
	const unsigned char *s = (const unsigned char *)str;
	while(len > 0)
	{
		if(*s == (unsigned char)ch)
		{
			return (void *)s;
		}
		++s;
		--len;
	}
	return (void *)0;
#endif
}
