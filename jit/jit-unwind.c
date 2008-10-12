/*
 * jit-unwind.c - Routines for performing stack unwinding.
 *
 * Copyright (C) 2008  Southern Storm Software, Pty Ltd.
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
#include "jit-cache.h"
#include "jit-rules.h"
#include "jit-apply-rules.h"
#include <jit/jit-unwind.h>
#include <jit/jit-walk.h>

int
jit_unwind_init(jit_unwind_context_t *unwind, jit_context_t context)
{
#if defined(JIT_BACKEND_INTERP) || JIT_APPLY_BROKEN_FRAME_BUILTINS != 0
	jit_thread_control_t control;

	control = _jit_thread_get_control();
	if(!control)
	{
		return 0;
	}

	unwind->frame = control->backtrace_head;
#elif JIT_FAST_GET_CURRENT_FRAME != 0
	unwind->frame = jit_get_next_frame_address(jit_get_current_frame());
#else
	unwind->frame = jit_get_frame_address(1);
#endif

	unwind->context = context;
	unwind->cache = 0;

#ifdef _JIT_ARCH_UNWIND_INIT
	_JIT_ARCH_UNWIND_INIT(unwind);
#endif

	return (unwind->frame != 0);
}

void
jit_unwind_free(jit_unwind_context_t *unwind)
{
#ifdef _JIT_ARCH_UNWIND_FREE
	_JIT_ARCH_UNWIND_FREE(unwind);
#endif
}

int
jit_unwind_next(jit_unwind_context_t *unwind)
{
#if defined(_JIT_ARCH_UNWIND_NEXT) || defined(_JIT_ARCH_UNWIND_NEXT_PRE)
	jit_function_t func;
#endif

	if(!unwind || !unwind->frame)
	{
		return 0;
	}

	unwind->cache = 0;

#if defined(JIT_BACKEND_INTERP) || JIT_APPLY_BROKEN_FRAME_BUILTINS != 0
	unwind->frame =  ((jit_backtrace_t) unwind->frame)->parent;
	return (unwind->frame != 0);
#else

#ifdef _JIT_ARCH_UNWIND_NEXT_PRE
	func = jit_unwind_get_function(unwind);
	if(func)
	{
		_JIT_ARCH_UNWIND_NEXT_PRE(unwind, func);
	}
#endif

	unwind->frame = jit_get_next_frame_address(unwind->frame);
	if(!unwind->frame)
	{
		return 0;
	}

#ifdef _JIT_ARCH_UNWIND_NEXT
	func = jit_unwind_get_function(unwind);
	if(func)
	{
		_JIT_ARCH_UNWIND_NEXT(unwind, func);
	}
#endif

	return 1;
#endif
}

int
jit_unwind_next_pc(jit_unwind_context_t *unwind)
{
	if(!unwind || !unwind->frame)
	{
		return 0;
	}

	unwind->cache = 0;

#if defined(JIT_BACKEND_INTERP) || JIT_APPLY_BROKEN_FRAME_BUILTINS != 0
	unwind->frame =  ((jit_backtrace_t) unwind->frame)->parent;
#else
	unwind->frame = jit_get_next_frame_address(unwind->frame);
#endif
	return (unwind->frame != 0);
}

void *
jit_unwind_get_pc(jit_unwind_context_t *unwind)
{
	if(!unwind || !unwind->frame)
	{
		return 0;
	}

#if defined(JIT_BACKEND_INTERP) || JIT_APPLY_BROKEN_FRAME_BUILTINS != 0
	return ((jit_backtrace_t) unwind->frame)->pc;
#else
	return jit_get_return_address(unwind->frame);
#endif
}

int
jit_unwind_jump(jit_unwind_context_t *unwind, void *pc)
{
#ifdef _JIT_ARCH_UNWIND_JUMP
	if(!unwind || !unwind->frame || !pc)
	{
		return 0;
	}

	return _JIT_ARCH_UNWIND_JUMP(unwind, pc);
#else
	return 0;
#endif
}

jit_function_t
jit_unwind_get_function(jit_unwind_context_t *unwind)
{
	if(!unwind || !unwind->frame || !unwind->context)
	{
		return 0;
	}

	if(!unwind->cache)
	{
		jit_cache_t cache = _jit_context_get_cache(unwind->context);
		void *pc = jit_unwind_get_pc(unwind);
		unwind->cache = (jit_function_t)
			_jit_cache_get_method(cache, pc, NULL);
	}

	return (jit_function_t) unwind->cache;
}

unsigned int
jit_unwind_get_offset(jit_unwind_context_t *unwind)
{
	jit_function_t func;
	jit_cache_t cache;
	void *pc, *start;

	if(!unwind || !unwind->frame || !unwind->context)
	{
		return JIT_NO_OFFSET;
	}

	pc = jit_unwind_get_pc(unwind);
	if(!pc)
	{
		return JIT_NO_OFFSET;
	}

	func = jit_unwind_get_function(unwind);
	if(!func)
	{
		return JIT_NO_OFFSET;
	}

	cache = _jit_context_get_cache(unwind->context);

#ifdef JIT_PROLOG_SIZE
	start = _jit_cache_get_start_method(cache, func->entry_point);
#else
	start = func->entry_point;
#endif

	return _jit_cache_get_bytecode(cache, start, pc - start, 0);
}
