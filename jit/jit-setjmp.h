/*
 * jit-setjmp.h - Support definitions that use "setjmp" for exceptions.
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

#ifndef	_JIT_SETJMP_H
#define	_JIT_SETJMP_H

#include <setjmp.h>

#ifdef	__cplusplus
extern	"C" {
#endif

/*
 * Jump buffer structure, with link.
 */
typedef struct jit_jmp_buf
{
	jmp_buf				buf;
	jit_backtrace_t		trace;
	struct jit_jmp_buf *parent;

} jit_jmp_buf;

/*
 * Push a "setjmp" buffer onto the current thread's unwind stck.
 */
void _jit_unwind_push_setjmp(jit_jmp_buf *jbuf);

/*
 * Pop the top-most "setjmp" buffer from the current thread's unwind stack.
 */
void _jit_unwind_pop_setjmp(void);

#ifdef	__cplusplus
};
#endif

#endif	/* _JIT_SETJMP_H */
