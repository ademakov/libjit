/*
 * jit-arch-x86.h - Architecture-specific definitions.
 *
 * Copyright (C) 2006  Southern Storm Software, Pty Ltd.
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

#ifndef	_JIT_ARCH_X86_H
#define	_JIT_ARCH_X86_H

/*
 * If defined _JIT_ARCH_GET_CURRENT_FRAME() macro assigns the current frame
 * pointer to the supplied argument that has to be a void pointer.
 */
#if defined(__GNUC__)
#define _JIT_ARCH_GET_CURRENT_FRAME(f)		\
	do {					\
		register void *__f asm("ebp");	\
		f = __f;			\
	} while(0)
#elif defined(_MSC_VER) && defined(_M_IX86)
#define	_JIT_ARCH_GET_CURRENT_FRAME(f)		\
	do {					\
		__asm				\
		{				\
			mov dword ptr f, ebp	\
		}				\
	} while(0)
#else
#undef _JIT_ARCH_GET_CURRENT_FRAME
#endif

#endif /* _JIT_ARCH_X86_H */
