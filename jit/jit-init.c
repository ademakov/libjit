/*
 * jit-init.c - Initialization routines for the JIT.
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

#include "jit-internal.h"
#include "jit-rules.h"

/*@
 * @deftypefun void jit_init (void)
 * This is normally the first function that you call when using
 * @code{libjit}.  It initializes the library and prepares for
 * JIT operations.
 *
 * The @code{jit_context_create} function also calls this, so you can
 * avoid using @code{jit_init} if @code{jit_context_create} is the first
 * JIT function that you use.
 *
 * It is safe to initialize the JIT multiple times.  Subsequent
 * initializations are quietly ignored.
 * @end deftypefun
@*/
void jit_init(void)
{
	/* Make sure that the thread subsystem is initialized */
	_jit_thread_init();

	/* Initialize the backend */
	_jit_init_backend();
}

/*@
 * @deftypefun int jit_uses_interpreter (void)
 * Determine if the JIT uses a fall-back interpreter to execute code
 * rather than generating and executing native code.  This can be
 * called prior to @code{jit_init}.
 * @end deftypefun
@*/
int jit_uses_interpreter(void)
{
#if defined(JIT_BACKEND_INTERP)
	return 1;
#else
	return 0;
#endif
}
