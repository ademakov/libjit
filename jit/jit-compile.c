/*
 * jit-compile.c - Function compilation.
 *
 * Copyright (C) 2004, 2006-2008  Southern Storm Software, Pty Ltd.
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
#include "jit-memory.h"
#include "jit-rules.h"
#include "jit-reg-alloc.h"
#include "jit-setjmp.h"
#ifdef _JIT_COMPILE_DEBUG
# include <jit/jit-dump.h>
# include <stdio.h>
#endif

/*
 * Misc data needed for compilation
 */
typedef struct
{
	jit_function_t		func;

	jit_cache_t		cache;
	int			cache_locked;
	int 			cache_started;

	int			restart;
	int			page_factor;

	void			*code_start;
	void			*code_end;

	struct jit_gencode	gen;

} _jit_compile_t;

#define _JIT_RESULT_TO_OBJECT(x)	((void *) ((jit_nint) (x) - JIT_RESULT_OK))
#define _JIT_RESULT_FROM_OBJECT(x)	((jit_nint) ((void *) (x)) + JIT_RESULT_OK)

/*
 * This exception handler overrides a user-defined handler during compilation.
 */
static void *
internal_exception_handler(int exception_type)
{
	return _JIT_RESULT_TO_OBJECT(exception_type);
}

/*
 * Optimize a function.
 */
static void
optimize(jit_function_t func)
{
	if(func->is_optimized || func->optimization_level == JIT_OPTLEVEL_NONE)
	{
		/* The function is already optimized or does not need optimization */
		return;
	}

	/* Build control flow graph */
	_jit_block_build_cfg(func);

	/* Eliminate useless control flow */
	_jit_block_clean_cfg(func);

	/* Optimization is done */
	func->is_optimized = 1;
}

/*@
 * @deftypefun int jit_optimize (jit_function_t @var{func})
 * Optimize a function by analyzing and transforming its intermediate
 * representation. If the function was already compiled or optimized,
 * then do nothing.
 *
 * Returns @code{JIT_RESUlT_OK} on success, otherwise it might return
 * @code{JIT_RESULT_OUT_OF_MEMORY}, @code{JIT_RESULT_COMPILE_ERROR} or
 * possibly some other more specific @code{JIT_RESULT_} code.
 *
 * Normally this function should not be used because @code{jit_compile}
 * performs all the optimization anyway.  However it might be useful for
 * debugging to verify the effect of the @code{libjit} code optimization.
 * This might be done, for instance, by calling @code{jit_dump_function}
 * before and after @code{jit_optimize}.
 * @end deftypefun
@*/
int
jit_optimize(jit_function_t func)
{
	jit_jmp_buf jbuf;
	jit_exception_func handler;

	/* Bail out on invalid parameter */
	if(!func)
	{
		return JIT_RESULT_NULL_FUNCTION;
	}

	/* Bail out if there is nothing to do here */
	if(!func->builder)
	{
		if(func->is_compiled)
		{
			/* The function is already compiled and we can't optimize it */
			return JIT_RESULT_OK;
		}
		else
		{
			/* We don't have anything to optimize at all */
			return JIT_RESULT_NULL_FUNCTION;
		}
	}

	/* Override user's exception handler */
	handler = jit_exception_set_handler(internal_exception_handler);

	/* Establish a "setjmp" point here so that we can unwind the
	   stack to this point when an exception occurs and then prevent
	   the exception from propagating further up the stack */
	_jit_unwind_push_setjmp(&jbuf);
	if(setjmp(jbuf.buf))
	{
		_jit_unwind_pop_setjmp();
		jit_exception_set_handler(handler);
		return _JIT_RESULT_FROM_OBJECT(jit_exception_get_last_and_clear());
	}

	/* Perform the optimizations */
	optimize(func);

	/* Restore the "setjmp" contexts and exit */
	_jit_unwind_pop_setjmp();
	jit_exception_set_handler(handler);
	return JIT_RESULT_OK;
}

/*
 * Mark the current position with a bytecode offset value.
 */
void
mark_offset(jit_gencode_t gen, jit_function_t func, unsigned long offset)
{
	unsigned long native_offset = gen->posn.ptr - func->start;
	if(!_jit_varint_encode_uint(&gen->offset_encoder, (jit_uint) offset))
	{
		jit_exception_builtin(JIT_RESULT_OUT_OF_MEMORY);
	}
	if(!_jit_varint_encode_uint(&gen->offset_encoder, (jit_uint) native_offset))
	{
		jit_exception_builtin(JIT_RESULT_OUT_OF_MEMORY);
	}
}

/*
 * Compile a single basic block within a function.
 */
static void
compile_block(jit_gencode_t gen, jit_function_t func, jit_block_t block)
{
	jit_insn_iter_t iter;
	jit_insn_t insn;

#ifdef _JIT_COMPILE_DEBUG
	printf("Block #%d: %d\n\n", func->builder->block_count++, block->label);
#endif

	/* Iterate over all blocks in the function */
	jit_insn_iter_init(&iter, block);
	while((insn = jit_insn_iter_next(&iter)) != 0)
	{
#ifdef _JIT_COMPILE_DEBUG
		unsigned char *p1, *p2;
		p1 = gen->posn.ptr;
		printf("Insn #%d: ", func->builder->insn_count++);
		jit_dump_insn(stdout, func, insn);
		printf("\nStart of binary code: 0x%08x\n", p1);
#endif

		switch(insn->opcode)
		{
		case JIT_OP_NOP:
			/* Ignore NOP's */
			break;

		case JIT_OP_CHECK_NULL:
			/* Determine if we can optimize the null check away */
			if(!_jit_insn_check_is_redundant(&iter))
			{
				_jit_gen_insn(gen, func, block, insn);
			}
			break;

#ifndef JIT_BACKEND_INTERP
		case JIT_OP_CALL:
		case JIT_OP_CALL_TAIL:
		case JIT_OP_CALL_INDIRECT:
		case JIT_OP_CALL_INDIRECT_TAIL:
		case JIT_OP_CALL_VTABLE_PTR:
		case JIT_OP_CALL_VTABLE_PTR_TAIL:
		case JIT_OP_CALL_EXTERNAL:
		case JIT_OP_CALL_EXTERNAL_TAIL:
			/* Spill all caller-saved registers before a call */
			_jit_regs_spill_all(gen);
			/* Generate code for the instruction with the back end */
			_jit_gen_insn(gen, func, block, insn);
			/* Free outgoing registers if any */
			_jit_regs_clear_all_outgoing(gen);
			break;
#endif

#ifndef JIT_BACKEND_INTERP
		case JIT_OP_INCOMING_REG:
			/* Assign a register to an incoming value */
			_jit_regs_set_incoming(gen,
					       (int)jit_value_get_nint_constant(insn->value2),
					       insn->value1);
			/* Generate code for the instruction with the back end */
			_jit_gen_insn(gen, func, block, insn);
			break;
#endif

		case JIT_OP_INCOMING_FRAME_POSN:
			/* Set the frame position for an incoming value */
			insn->value1->frame_offset = jit_value_get_nint_constant(insn->value2);
			insn->value1->in_register = 0;
			insn->value1->has_frame_offset = 1;
			if(insn->value1->has_global_register)
			{
				insn->value1->in_global_register = 1;
				_jit_gen_load_global(gen, insn->value1->global_reg, insn->value1);
			}
			else
			{
				insn->value1->in_frame = 1;
			}
			break;

#ifndef JIT_BACKEND_INTERP
		case JIT_OP_OUTGOING_REG:
			/* Copy a value into an outgoing register */
			_jit_regs_set_outgoing(gen,
					       (int)jit_value_get_nint_constant(insn->value2),
					       insn->value1);
			break;
#endif

		case JIT_OP_OUTGOING_FRAME_POSN:
			/* Set the frame position for an outgoing value */
			insn->value1->frame_offset = jit_value_get_nint_constant(insn->value2);
			insn->value1->in_register = 0;
			insn->value1->in_global_register = 0;
			insn->value1->in_frame = 0;
			insn->value1->has_frame_offset = 1;
			insn->value1->has_global_register = 0;
			break;

#ifndef JIT_BACKEND_INTERP
		case JIT_OP_RETURN_REG:
			/* Assign a register to a return value */
			_jit_regs_set_incoming(gen,
					       (int)jit_value_get_nint_constant(insn->value2),
					       insn->value1);
			/* Generate code for the instruction with the back end */
			_jit_gen_insn(gen, func, block, insn);
			break;
#endif

		case JIT_OP_MARK_OFFSET:
			/* Mark the current code position as corresponding
			   to a particular bytecode offset */
			mark_offset(gen, func, (unsigned long)(long)jit_value_get_nint_constant(insn->value1));
			break;

		default:
			/* Generate code for the instruction with the back end */
			_jit_gen_insn(gen, func, block, insn);
			break;
		}

#ifdef _JIT_COMPILE_DEBUG
		p2 = gen->posn.ptr;
		printf("Length of binary code: %d\n\n", p2 - p1);
		fflush(stdout);
#endif
	}
}

/*
 * Reset value on codegen restart.
 */
static void
reset_value(jit_value_t value)
{
	value->reg = -1;
	value->in_register = 0;
	value->in_global_register = 0;
	value->in_frame = 0;
}

/*
 * Clean up the compilation state on codegen restart.
 */
static void
cleanup_on_restart(jit_gencode_t gen, jit_function_t func)
{
	jit_block_t block;
	jit_insn_iter_t iter;
	jit_insn_t insn;

	block = 0;
	while((block = jit_block_next(func, block)) != 0)
	{
		/* Clear the block addresses and fixup lists */
		block->address = 0;
		block->fixup_list = 0;
		block->fixup_absolute_list = 0;

		/* Reset values referred to by block instructions */
		jit_insn_iter_init(&iter, block);
		while((insn = jit_insn_iter_next(&iter)) != 0)
		{
			if(insn->dest && (insn->flags & JIT_INSN_DEST_OTHER_FLAGS) == 0)
			{
				reset_value(insn->dest);
			}
			if(insn->value1 && (insn->flags & JIT_INSN_VALUE1_OTHER_FLAGS) == 0)
			{
				reset_value(insn->value1);
			}
			if(insn->value2 && (insn->flags & JIT_INSN_VALUE2_OTHER_FLAGS) == 0)
			{
				reset_value(insn->value2);
			}
		}
	}

	/* Reset values referred to by builder */
	if(func->builder->setjmp_value)
	{
		reset_value(func->builder->setjmp_value);
	}
	if(func->builder->parent_frame)
	{
		reset_value(func->builder->parent_frame);
	}

	/* Reset the "touched" registers mask. The first time compilation
	   might have followed wrong code paths and thus allocated wrong
	   registers. */
	if(func->builder->has_tail_call)
	{
		/* For functions with tail calls _jit_regs_alloc_global()
		   does not allocate any global registers. The "permanent"
		   mask has all global registers set to prevent their use. */
		gen->touched = jit_regused_init;
	}
	else
	{
		gen->touched = gen->permanent;
	}

	/* Reset the epilog fixup list */
	gen->epilog_fixup = 0;
}

/*
 * Acquire the code cache.
 */
static void
cache_acquire(_jit_compile_t *state)
{
	/* Acquire the cache lock */
	jit_mutex_lock(&state->func->context->cache_lock);

	/* Remember that the lock is acquired */
	state->cache_locked = 1;

	/* Get the method cache */
	state->cache = _jit_context_get_cache(state->func->context);
	if(!state->cache)
	{
		jit_exception_builtin(JIT_RESULT_OUT_OF_MEMORY);
	}
}

/*
 * Release the code cache.
 */
static void
cache_release(_jit_compile_t *state)
{
	/* Release the lock if it was previously acquired */
	if(state->cache_locked)
	{
		jit_mutex_unlock(&state->func->context->cache_lock);
		state->cache_locked = 0;
	}
}

/*
 * Allocate some space in the code cache.
 */
static void
cache_alloc(_jit_compile_t *state)
{
	int result;

	/* First try with the current cache page */
	result = _jit_cache_start_method(state->cache,
					 &state->gen.posn,
					 state->page_factor++,
					 JIT_FUNCTION_ALIGNMENT,
					 state->func);
	if(result == JIT_CACHE_RESTART)
	{
		/* No space left on the current cache page.  Allocate a new one. */
		result = _jit_cache_start_method(state->cache,
						 &state->gen.posn,
						 state->page_factor++,
						 JIT_FUNCTION_ALIGNMENT,
						 state->func);
	}
	if(result != JIT_CACHE_OK)
	{
		/* Failed to allocate any cache space */
		jit_exception_builtin(JIT_RESULT_OUT_OF_MEMORY);
	}

	/* Prepare the bytecode offset encoder */
	_jit_varint_init_encoder(&state->gen.offset_encoder);

	/* On success remember the cache state */
	state->cache_started = 1;
}

#if NOT_USED
/*
 * Align the method code on a particular boundary if the
 * difference between the current position and the aligned
 * boundary is less than "diff".  The "nop" value is used
 * to pad unused bytes.
 */
static void
cache_align(jit_cache_posn *posn, int align, int diff, int nop)
{
	jit_nuint current;
	jit_nuint next;

	/* Determine the location of the next alignment boundary */
	if(align <= 1)
	{
		align = 1;
	}
	current = (jit_nuint)(posn->ptr);
	next = (current + ((jit_nuint)align) - 1) &
		   ~(((jit_nuint)align) - 1);
	if(current == next || (next - current) >= (jit_nuint)diff)
	{
		return;
	}

	/* Detect overflow of the free memory region */
	if(next > ((jit_nuint)(posn->limit)))
	{
		posn->ptr = posn->limit;
		return;
	}

#ifndef jit_should_pad
	/* Fill from "current" to "next" with nop bytes */
	while(current < next)
	{
		*((posn->ptr)++) = (unsigned char)nop;
		++current;
	}
#else
	/* Use CPU-specific padding, because it may be more efficient */
	_jit_pad_buffer((unsigned char *)current, (int)(next - current));
#endif
}
#endif


/*
 * End function output to the cache.
 */
static void
cache_flush(_jit_compile_t *state)
{
	int result;

	if(state->cache_started)
	{
		state->cache_started = 0;

		/* End the function's output process */
		result = _jit_cache_end_method(&state->gen.posn, JIT_CACHE_OK);
		if(result != JIT_CACHE_OK)
		{
			if(result == JIT_CACHE_RESTART)
			{
				/* Throw an internal exception that causes
				   a larger code space to be allocated and
				   the code generation to restart */
				jit_exception_builtin(JIT_RESULT_CACHE_FULL);
			}
			else
			{
				/* Throw exception that indicates failure
				   to allocate enough code space */
				jit_exception_builtin(JIT_RESULT_OUT_OF_MEMORY);
			}
		}

#ifndef JIT_BACKEND_INTERP
		/* On success perform a CPU cache flush, to make the code executable */
		jit_flush_exec(state->code_start,
			       (unsigned int)(state->code_end - state->code_start));
#endif

		/* Terminate the debug information and flush it */
		if(!_jit_varint_encode_end(&state->gen.offset_encoder))
		{
			jit_exception_builtin(JIT_RESULT_OUT_OF_MEMORY);
		}
		state->func->bytecode_offset = _jit_varint_get_data(&state->gen.offset_encoder);
	}
}

/*
 * Release the allocated cache space.
 */
static void
cache_abort(_jit_compile_t *state)
{
	if(state->cache_started)
	{
		state->cache_started = 0;

		/* Release the cache space */
		_jit_cache_end_method(&state->gen.posn, JIT_CACHE_RESTART);

		/* Free encoded bytecode offset data */
		_jit_varint_free_data(_jit_varint_get_data(&state->gen.offset_encoder));
	}
}

/*
 * Allocate more space in the code cache.
 */
static void
cache_realloc(_jit_compile_t *state)
{
	int result;

	/* Release the allocated cache space */
	cache_abort(state);

	/* Allocate a new cache page with the size that grows
	   by factor of 2 on each reallocation */
	result = _jit_cache_start_method(state->cache,
					 &state->gen.posn,
					 state->page_factor,
					 JIT_FUNCTION_ALIGNMENT,
					 state->func);
	if(result != JIT_CACHE_OK)
	{
		/* Failed to allocate enough cache space */
		jit_exception_builtin(JIT_RESULT_OUT_OF_MEMORY);
	}

	state->page_factor *= 2;

	/* Prepare the bytecode offset encoder */
	_jit_varint_init_encoder(&state->gen.offset_encoder);

	/* On success remember the cache state */
	state->cache_started = 1;
}

/*
 * Prepare data needed for code generation.
 */
static void
codegen_prepare(_jit_compile_t *state)
{
	/* Intuit "nothrow" and "noreturn" flags for this function */
	if(!state->func->builder->may_throw)
	{
		state->func->no_throw = 1;
	}
	if(!state->func->builder->ordinary_return)
	{
		state->func->no_return = 1;
	}

	/* Compute liveness and "next use" information for this function */
	_jit_function_compute_liveness(state->func);

	/* Allocate global registers to variables within the function */
#ifndef JIT_BACKEND_INTERP
	_jit_regs_alloc_global(&state->gen, state->func);
#endif
}

/*
 * Run codegen.
 */
static void
codegen(_jit_compile_t *state)
{
	jit_function_t func = state->func;
	struct jit_gencode *gen = &state->gen;
	jit_block_t block;

	state->code_start = gen->posn.ptr;

#ifdef JIT_PROLOG_SIZE
	/* Output space for the function prolog */
	_jit_cache_check_space(&gen->posn, JIT_PROLOG_SIZE);
	gen->posn.ptr += JIT_PROLOG_SIZE;
#endif

	/* Generate code for the blocks in the function */
	block = 0;
	while((block = jit_block_next(func, block)) != 0)
	{
		/* Notify the back end that the block is starting */
		_jit_gen_start_block(gen, block);

#ifndef JIT_BACKEND_INTERP
		/* Clear the local register assignments */
		_jit_regs_init_for_block(gen);
#endif

		/* Generate the block's code */
		compile_block(gen, func, block);

#ifndef JIT_BACKEND_INTERP
		/* Spill all live register values back to their frame positions */
		_jit_regs_spill_all(gen);
#endif

		/* Notify the back end that the block is finished */
		_jit_gen_end_block(gen, block);

		/* Stop code generation if the cache page is full */
		if(_jit_cache_is_full(state->cache, &gen->posn))
		{
			/* No space left on the current cache page.  Restart. */
			jit_exception_builtin(JIT_RESULT_CACHE_FULL);
		}
	}

	/* Output the function epilog.  All return paths will jump to here */
	_jit_gen_epilog(gen, func);
	state->code_end = gen->posn.ptr;

#ifdef JIT_PROLOG_SIZE
	/* Back-patch the function prolog and get the real entry point */
	state->code_start = _jit_gen_prolog(gen, func, state->code_start);
#endif

#if !defined(JIT_BACKEND_INTERP) && (!defined(jit_redirector_size) || !defined(jit_indirector_size))
	/* If the function is recompilable, then we need an extra entry
	   point to properly redirect previous references to the function */
	if(func->is_recompilable && !func->indirector)
	{
		/* TODO: use _jit_create_indirector() instead of
		   _jit_gen_redirector() as both do the same. */
		func->indirector = _jit_gen_redirector(&gen, func);
	}
#endif
}

/*
 * Compile a function and return its entry point.
 */
static int
compile(_jit_compile_t *state, jit_function_t func)
{
	jit_exception_func handler;
	jit_jmp_buf jbuf;
	int result;

	/* Initialize compilation state */
	jit_memzero(state, sizeof(_jit_compile_t));
	state->func = func;

	/* Replace user's exception handler with internal handler */
	handler = jit_exception_set_handler(internal_exception_handler);

	/* Establish a "setjmp" point here so that we can unwind the
	   stack to this point when an exception occurs and then prevent
	   the exception from propagating further up the stack */
	_jit_unwind_push_setjmp(&jbuf);

 restart:
	/* Handle compilation exceptions */
	if(setjmp(jbuf.buf))
	{
		result = _JIT_RESULT_FROM_OBJECT(jit_exception_get_last_and_clear());
		if(result == JIT_RESULT_CACHE_FULL)
		{
			/* Restart code generation after the cache-full condition */
			state->restart = 1;
			goto restart;
		}

		/* Release allocated cache space and exit */
		cache_abort(state);
		goto exit;
	}

	if(state->restart == 0)
	{
		/* Start compilation */

		/* Perform machine-independent optimizations */
		optimize(state->func);

		/* Prepare data needed for code generation */
		codegen_prepare(state);

		/* Allocate some cache */
		cache_acquire(state);
		cache_alloc(state);
	}
	else
	{
		/* Restart compilation */

		/* Clean up the compilation state */
		cleanup_on_restart(&state->gen, state->func);

		/* Allocate more cache */
		cache_realloc(state);
	}

#ifdef _JIT_COMPILE_DEBUG
	if(state->restart == 0)
	{
		printf("\n*** Start code generation ***\n\n");
	}
	else
	{
		printf("\n*** Restart code generation ***\n\n");
	}
	state->func->builder->block_count = 0;
	state->func->builder->insn_count = 0;
#endif

#ifdef jit_extra_gen_init
	/* Initialize information that may need to be reset both
	   on start and restart */
	jit_extra_gen_init(&state->gen);
#endif

	/* Perform code generation */
	codegen(state);

#ifdef jit_extra_gen_cleanup
	/* Clean up the extra code generation state */
	jit_extra_gen_cleanup(&state->gen);
#endif

	/* End the function's output process */
	cache_flush(state);

	/* Compilation done, no exceptions occured */
	result = JIT_RESULT_OK;

 exit:
	/* Release the cache */
	cache_release(state);

	/* Restore the "setjmp" context */
	_jit_unwind_pop_setjmp();

	/* Restore user's exception handler */
	jit_exception_set_handler(handler);

	return result;
}

/*@
 * @deftypefun int jit_compile (jit_function_t @var{func})
 * Compile a function to its executable form.  If the function was
 * already compiled, then do nothing.  Returns zero on error.
 *
 * If an error occurs, you can use @code{jit_function_abandon} to
 * completely destroy the function.  Once the function has been compiled
 * successfully, it can no longer be abandoned.
 *
 * Sometimes you may wish to recompile a function, to apply greater
 * levels of optimization the second time around.  You must call
 * @code{jit_function_set_recompilable} before you compile the function
 * the first time.  On the second time around, build the function's
 * instructions again, and call @code{jit_compile} a second time.
 * @end deftypefun
@*/
int
jit_compile(jit_function_t func)
{
	_jit_compile_t state;
	int result;

	/* Bail out on invalid parameter */
	if(!func)
	{
		return JIT_RESULT_NULL_FUNCTION;
	}

	/* Bail out if there is nothing to do here */
	if(!func->builder)
	{
		if(func->is_compiled)
		{
			/* The function is already compiled, and we don't need to recompile */
			return JIT_RESULT_OK;
		}
		else
		{
			/* We don't have anything to compile at all */
			return JIT_RESULT_NULL_FUNCTION;
		}
	}

	/* Compile and record the entry point */
	result = compile(&state, func);
	if(result == JIT_RESULT_OK)
	{
		func->entry_point = state.code_start;
		func->is_compiled = 1;

		/* Free the builder structure, which we no longer require */
		_jit_function_free_builder(func);
	}

	return result;
}

/*@
 * @deftypefun int jit_compile_entry (jit_function_t @var{func}, void **@var{entry_point})
 * Compile a function to its executable form but do not make it
 * available for invocation yet.  It may be made available later
 * with @code{jit_function_setup_entry}.
 * @end deftypefun
@*/
int
jit_compile_entry(jit_function_t func, void **entry_point)
{
	_jit_compile_t state;
	int result;

	/* Init entry_point */
	if(entry_point)
	{
		*entry_point = 0;
	}
	else
	{
		return JIT_RESULT_NULL_REFERENCE;
	}

	/* Bail out on invalid parameter */
	if(!func)
	{
		return JIT_RESULT_NULL_FUNCTION;
	}

	/* Bail out if there is nothing to do here */
	if(!func->builder)
	{
		if(func->is_compiled)
		{
			/* The function is already compiled, and we don't need to recompile */
			*entry_point = func->entry_point;
			return JIT_RESULT_OK;
		}
		else
		{
			/* We don't have anything to compile at all */
			return JIT_RESULT_NULL_FUNCTION;
		}
	}

	/* Compile and return the entry point */
	result = compile(&state, func);
	if(result == JIT_RESULT_OK)
	{
		*entry_point = state.code_start;
	}

	return result;
}

/*@
 * @deftypefun int jit_function_setup_entry (jit_function_t @var{func}, void *@var{entry_point})
 * Make a function compiled with @code{jit_function_compile_entry}
 * available for invocation and free the resources used for
 * compilation.  If @var{entry_point} is null then it only
 * frees the resources.
 * @end deftypefun
@*/
void
jit_function_setup_entry(jit_function_t func, void *entry_point)
{
	/* Bail out if we have nothing to do */
	if(!func)
	{
		return;
	}
	/* Record the entry point */
	if(entry_point)
	{
		func->entry_point = entry_point;
		func->is_compiled = 1;
	}
	_jit_function_free_builder(func);
}

/*@
 * @deftypefun int jit_function_compile (jit_function_t @var{func})
 * Compile a function to its executable form.  If the function was
 * already compiled, then do nothing.  Returns zero on error.
 *
 * If an error occurs, you can use @code{jit_function_abandon} to
 * completely destroy the function.  Once the function has been compiled
 * successfully, it can no longer be abandoned.
 *
 * Sometimes you may wish to recompile a function, to apply greater
 * levels of optimization the second time around.  You must call
 * @code{jit_function_set_recompilable} before you compile the function
 * the first time.  On the second time around, build the function's
 * instructions again, and call @code{jit_function_compile}
 * a second time.
 * @end deftypefun
@*/
int
jit_function_compile(jit_function_t func)
{
	return (JIT_RESULT_OK == jit_compile(func));
}

/*@
 * @deftypefun int jit_function_compile_entry (jit_function_t @var{func}, void **@var{entry_point})
 * Compile a function to its executable form but do not make it
 * available for invocation yet.  It may be made available later
 * with @code{jit_function_setup_entry}.
 * @end deftypefun
@*/
int
jit_function_compile_entry(jit_function_t func, void **entry_point)
{
	return (JIT_RESULT_OK == jit_compile_entry(func, entry_point));
}

void *
_jit_function_compile_on_demand(jit_function_t func)
{
	_jit_compile_t state;
	int result;

	/* Lock down the context */
	jit_context_build_start(func->context);

	/* Fast return if we are already compiled */
	if(func->is_compiled)
	{
		jit_context_build_end(func->context);
		return func->entry_point;
	}

	if(!func->on_demand)
	{
		/* Bail out with an error if the user didn't supply an
		   on-demand compiler */
		result = JIT_RESULT_COMPILE_ERROR;
	}
	else
	{
		/* Call the user's on-demand compiler. */
		result = (func->on_demand)(func);
		if(result == JIT_RESULT_OK && !func->is_compiled)
		{
			/* Compile the function if the user didn't do so */
			result = compile(&state, func);
			if(result == JIT_RESULT_OK)
			{
				func->entry_point = state.code_start;
				func->is_compiled = 1;
			}
		}
		_jit_function_free_builder(func);
	}

	/* Unlock the context and report the result */
	jit_context_build_end(func->context);
	if(result != JIT_RESULT_OK)
	{
		jit_exception_builtin(result);
		/* Normally this should be unreachable but just in case... */
		return 0;
	}

	return func->entry_point;
}

#define	JIT_CACHE_NO_OFFSET		(~((unsigned long)0))

unsigned long
_jit_function_get_bytecode(jit_function_t func, void *pc, int exact)
{
	unsigned long offset = JIT_CACHE_NO_OFFSET;
	jit_cache_t cache;
	void *start;
	unsigned long native_offset;
	jit_varint_decoder_t decoder;
	jit_uint off, noff;

	cache = _jit_context_get_cache(func->context);

#ifdef JIT_PROLOG_SIZE
	start = func->start;
#else
	start = func->entry_point;
#endif

	native_offset = pc - start;

	_jit_varint_init_decoder(&decoder, func->bytecode_offset);
	for(;;)
	{
		off = _jit_varint_decode_uint(&decoder);
		noff = _jit_varint_decode_uint(&decoder);
		if(_jit_varint_decode_end(&decoder))
		{
			if(exact)
			{
				offset = JIT_CACHE_NO_OFFSET;
			}
			break;
		}
		if(noff >= native_offset)
		{
			if(noff == native_offset)
			{
				offset = off;
			}
			else if (exact)
			{
				offset = JIT_CACHE_NO_OFFSET;
			}
			break;
		}
		offset = off;
	}

	return offset;
}
