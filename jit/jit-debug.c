/*
 * jit-debug.c - Debug support routines for the JIT.
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

/*@

The @code{libjit} library provides a very simple breakpoint mechanism.
Upon reaching each breakpoint in a function, the global debug hook
is called.  It is up to the debug hook to decide whether to stop execution
or to ignore the breakpoint.

Typically, the debug hook will inspect a table to determine which
breakpoints were actually selected by the user in a debugger's user
interface.  The debug hook may even evaluate a complicated expression,
taking the function, current thread, and the value of local variables
into account, to make the decision.

The global debug hook is set using @code{jit_context_set_meta} with a
type argument of @code{JIT_OPTION_DEBUG_HOOK}.  It must have the
following prototype:

@example
void hook(jit_function_t func, jit_nint data1, jit_nint data2);
@end example

The @code{func} argument indicates the function that the breakpoint
occurred within.  The @code{data1} and @code{data2} arguments are
those supplied to @code{jit_insn_mark_breakpoint}.  The debugger can use
these values to indicate information about the breakpoint's type
and location.

If the hook decides to stop at the breakpoint, it can call the debugger
immediately.  Or the hook can send a message to a separate debugger
thread and wait for an indication that it is time to continue.

Debug hooks can be used for other purposes besides breakpoint debugging.
A program could be instrumented with hooks that tally up the number
of times that each function is called, or which profile the amount of
time spent in each function.

@*/

/*@
 * @deftypefun int jit_insn_mark_breakpoint (jit_function_t func, jit_nint data1, jit_nint data2)
 * Mark the current position in @code{func} as corresponding to a breakpoint
 * location.  When a break occurs, the global debug hook is called with
 * @code{func}, @code{data1}, and @code{data2} as arguments.
 *
 * Some platforms may have a performance penalty for inserting breakpoint
 * locations, even if the breakpoint is never enabled.  Correctness is
 * considered more important than performance where debugging is concerned.
 * @end deftypefun
@*/

/*@
 * @deftypefun void jit_context_enable_all_breakpoints (jit_context_t context, int flag)
 * Enable or disable all breakpoints in all functions within @code{context}.
 * This is typically used to implement a "single step" facility.
 * @end deftypefun
@*/
void jit_context_enable_all_breakpoints(jit_context_t context, int flag)
{
	if(context)
	{
		context->breakpoints_enabled = flag;
	}
}

/*@
 * @deftypefun int jit_context_all_breakpoints_enabled (jit_context_t context)
 * Determine if all breakpoints within @code{context} are enabled.
 * @end deftypefun
@*/
int jit_context_all_breakpoints_enabled(jit_context_t context)
{
	if(context)
	{
		return context->breakpoints_enabled;
	}
	else
	{
		return 0;
	}
}

/*@
 * @deftypefun void jit_function_enable_breakpoints (jit_function_t func)
 * Enable or disable all breakpoints in the specified function.
 * @end deftypefun
@*/
void jit_function_enable_breakpoints(jit_function_t func, int flag)
{
	if(func)
	{
		func->breakpoints_enabled = flag;
	}
}

/*@
 * @deftypefun int jit_function_breakpoints_enabled (jit_function_t func)
 * Determine if breakpoints are enabled on the specified function.
 * @end deftypefun
@*/
int jit_function_breakpoints_enabled(jit_function_t func)
{
	if(func)
	{
		return func->breakpoints_enabled;
	}
	else
	{
		return 0;
	}
}
