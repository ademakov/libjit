/*
 * jit-function.c - Functions for manipulating function blocks.
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
#include "jit-apply-func.h"
#include "jit-setjmp.h"
#ifdef _JIT_COMPILE_DEBUG
#include <stdio.h>
#endif

/*@
 * @deftypefun jit_function_t jit_function_create (jit_context_t @var{context}, jit_type_t @var{signature})
 * Create a new function block and associate it with a JIT context.
 * Returns NULL if out of memory.
 *
 * A function persists for the lifetime of its containing context.
 * It initially starts life in the "building" state, where the user
 * constructs instructions that represents the function body.
 * Once the build process is complete, the user calls
 * @code{jit_function_compile} to convert it into its executable form.
 *
 * It is recommended that you call @code{jit_context_build_start} before
 * calling @code{jit_function_create}, and then call
 * @code{jit_context_build_end} after you have called
 * @code{jit_function_compile}.  This will protect the JIT's internal
 * data structures within a multi-threaded environment.
 * @end deftypefun
@*/
jit_function_t jit_function_create(jit_context_t context, jit_type_t signature)
{
	jit_function_t func;
#if !defined(JIT_BACKEND_INTERP) && (defined(jit_redirector_size) || defined(jit_indirector_size))
	jit_cache_t cache;
#endif

	/* Allocate memory for the function and clear it */
	func = jit_cnew(struct _jit_function);
	if(!func)
	{
		return 0;
	}

#if !defined(JIT_BACKEND_INTERP) && (defined(jit_redirector_size) || defined(jit_indirector_size))
	/* TODO: if the function is destroyed the redirector and indirector memory
	   is leaked */

	/* We need the cache lock while we are allocating redirector and indirector */
	jit_mutex_lock(&(context->cache_lock));

	/* Get the method cache */
	cache = _jit_context_get_cache(context);
	if(!cache)
	{
		jit_mutex_unlock(&(context->cache_lock));
		jit_free(func);
		return 0;
	}

# if defined(jit_redirector_size)
	/* Allocate redirector buffer */
	func->redirector = _jit_cache_alloc_no_method(cache, jit_redirector_size, 1);
	if(!func->redirector)
	{
		jit_mutex_unlock(&(context->cache_lock));
		jit_free(func);
		return 0;
	}
# endif
# if defined(jit_indirector_size)
	/* Allocate indirector buffer */
	func->indirector = _jit_cache_alloc_no_method(cache, jit_indirector_size, 1);
	if(!func->indirector)
	{
		jit_mutex_unlock(&(context->cache_lock));
		jit_free(func);
		return 0;
	}
# endif

	jit_mutex_unlock(&(context->cache_lock));
#endif /* !defined(JIT_BACKEND_INTERP) && (defined(jit_redirector_size) || defined(jit_indirector_size)) */

	/* Initialize the function block */
	func->context = context;
	func->signature = jit_type_copy(signature);

#if !defined(JIT_BACKEND_INTERP) && defined(jit_redirector_size)
	/* If we aren't using interpretation, then point the function's
	   initial entry point at the redirector, which in turn will
	   invoke the on-demand compiler */
	func->entry_point = _jit_create_redirector
		(func->redirector, (void *) context->on_demand_driver,
		 func, jit_type_get_abi(signature));
	jit_flush_exec(func->redirector, jit_redirector_size);
#endif
# if !defined(JIT_BACKEND_INTERP) && defined(jit_indirector_size)
	_jit_create_indirector(func->indirector, (void**) &(func->entry_point));
	jit_flush_exec(func->indirector, jit_indirector_size);
# endif

	/* Add the function to the context list */
	func->next = 0;
	func->prev = context->last_function;
	if(context->last_function)
	{
		context->last_function->next = func;
	}
	else
	{
		context->functions = func;
	}
	context->last_function = func;

	/* Return the function to the caller */
	return func;
}

/*@
 * @deftypefun jit_function_t jit_function_create_nested (jit_context_t @var{context}, jit_type_t @var{signature}, jit_function_t @var{parent})
 * Create a new function block and associate it with a JIT context.
 * In addition, this function is nested inside the specified
 * @var{parent} function and is able to access its parent's
 * (and grandparent's) local variables.
 *
 * The front end is responsible for ensuring that the nested function can
 * never be called by anyone except its parent and sibling functions.
 * The front end is also responsible for ensuring that the nested function
 * is compiled before its parent.
 * @end deftypefun
@*/
jit_function_t jit_function_create_nested
		(jit_context_t context, jit_type_t signature, jit_function_t parent)
{
	jit_function_t func;
	func = jit_function_create(context, signature);
	if(!func)
	{
		return 0;
	}
	func->nested_parent = parent;
	return func;
}

int _jit_function_ensure_builder(jit_function_t func)
{
	/* Handle the easy cases first */
	if(!func)
	{
		return 0;
	}
	if(func->builder)
	{
		return 1;
	}

	/* Allocate memory for the builder and clear it */
	func->builder = jit_cnew(struct _jit_builder);
	if(!(func->builder))
	{
		return 0;
	}

	/* Cache the value of the JIT_OPTION_POSITION_INDEPENDENT option */
	func->builder->position_independent
		= jit_context_get_meta_numeric(
			func->context, JIT_OPTION_POSITION_INDEPENDENT);

	/* Initialize the function builder */
	jit_memory_pool_init(&(func->builder->value_pool), struct _jit_value);
	jit_memory_pool_init(&(func->builder->insn_pool), struct _jit_insn);
	jit_memory_pool_init(&(func->builder->meta_pool), struct _jit_meta);

	/* Create the initial entry block */
	if(!_jit_block_init(func))
	{
		_jit_function_free_builder(func);
		return 0;
	}

	/* Create instructions to initialize the incoming arguments */
	if(!_jit_create_entry_insns(func))
	{
		_jit_function_free_builder(func);
		return 0;
	}

	/* The current position is where initialization code will be
	   inserted by "jit_insn_move_blocks_to_start" */
	func->builder->init_block = func->builder->current_block;
	func->builder->init_insn = func->builder->current_block->last_insn + 1;

	/* The builder is ready to go */
	return 1;
}

void _jit_function_free_builder(jit_function_t func)
{
	if(func->builder)
	{
		_jit_block_free(func);
		jit_memory_pool_free(&(func->builder->insn_pool), 0);
		jit_memory_pool_free(&(func->builder->value_pool), _jit_value_free);
		jit_memory_pool_free(&(func->builder->meta_pool), _jit_meta_free_one);
		jit_free(func->builder->param_values);
		jit_free(func->builder->insns);
		jit_free(func->builder->label_blocks);
		jit_free(func->builder);
		func->builder = 0;
	}
}

void _jit_function_destroy(jit_function_t func)
{
	if(!func)
	{
		return;
	}
	if(func->next)
	{
		func->next->prev = func->prev;
	}
	else
	{
		func->context->last_function = func->prev;
	}
	if(func->prev)
	{
		func->prev->next = func->next;
	}
	else
	{
		func->context->functions = func->next;
	}
	_jit_function_free_builder(func);
	jit_meta_destroy(&(func->meta));
	jit_type_free(func->signature);
	jit_free(func);
}

/*@
 * @deftypefun void jit_function_abandon (jit_function_t @var{func})
 * Abandon this function during the build process.  This should be called
 * when you detect a fatal error that prevents the function from being
 * properly built.  The @var{func} object is completely destroyed and
 * detached from its owning context.  The function is left alone if
 * it was already compiled.
 * @end deftypefun
@*/
void jit_function_abandon(jit_function_t func)
{
	if(func && func->builder)
	{
		if(func->is_compiled)
		{
			/* We already compiled this function previously, but we
			   have tried to recompile it with new contents.  Throw
			   away the builder, but keep the original version */
			_jit_function_free_builder(func);
		}
		else
		{
			/* This function was never compiled, so abandon entirely */
			_jit_function_destroy(func);
		}
	}
}

/*@
 * @deftypefun jit_context_t jit_function_get_context (jit_function_t @var{func})
 * Get the context associated with a function.
 * @end deftypefun
@*/
jit_context_t jit_function_get_context(jit_function_t func)
{
	if(func)
	{
		return func->context;
	}
	else
	{
		return 0;
	}
}

/*@
 * @deftypefun jit_type_t jit_function_get_signature (jit_function_t @var{func})
 * Get the signature associated with a function.
 * @end deftypefun
@*/
jit_type_t jit_function_get_signature(jit_function_t func)
{
	if(func)
	{
		return func->signature;
	}
	else
	{
		return 0;
	}
}

/*@
 * @deftypefun int jit_function_set_meta (jit_function_t @var{func}, int @var{type}, void *@var{data}, jit_meta_free_func @var{free_data}, int @var{build_only})
 * Tag a function with some metadata.  Returns zero if out of memory.
 *
 * Metadata may be used to store dependency graphs, branch prediction
 * information, or any other information that is useful to optimizers
 * or code generators.  It can also be used by higher level user code
 * to store information about the function that is specific to the
 * virtual machine or language.
 *
 * If the @var{type} already has some metadata associated with it, then
 * the previous value will be freed.
 *
 * If @var{build_only} is non-zero, then the metadata will be freed
 * when the function is compiled with @code{jit_function_compile}.
 * Otherwise the metadata will persist until the JIT context is destroyed,
 * or @code{jit_function_free_meta} is called for the specified @var{type}.
 *
 * Metadata type values of 10000 or greater are reserved for internal use.
 * @end deftypefun
@*/
int jit_function_set_meta(jit_function_t func, int type, void *data,
					      jit_meta_free_func free_data, int build_only)
{
	if(build_only)
	{
		if(!_jit_function_ensure_builder(func))
		{
			return 0;
		}
		return jit_meta_set(&(func->builder->meta), type, data,
							free_data, func);
	}
	else
	{
		return jit_meta_set(&(func->meta), type, data, free_data, 0);
	}
}

/*@
 * @deftypefun {void *} jit_function_get_meta (jit_function_t @var{func}, int @var{type})
 * Get the metadata associated with a particular tag.  Returns NULL
 * if @var{type} does not have any metadata associated with it.
 * @end deftypefun
@*/
void *jit_function_get_meta(jit_function_t func, int type)
{
	void *data = jit_meta_get(func->meta, type);
	if(!data && func->builder)
	{
		data = jit_meta_get(func->builder->meta, type);
	}
	return data;
}

/*@
 * @deftypefun void jit_function_free_meta (jit_function_t @var{func}, int @var{type})
 * Free metadata of a specific type on a function.  Does nothing if
 * the @var{type} does not have any metadata associated with it.
 * @end deftypefun
@*/
void jit_function_free_meta(jit_function_t func, int type)
{
	jit_meta_free(&(func->meta), type);
	if(func->builder)
	{
		jit_meta_free(&(func->builder->meta), type);
	}
}

/*@
 * @deftypefun jit_function_t jit_function_next (jit_context_t @var{context}, jit_function_t @var{prev})
 * Iterate over the defined functions in creation order.  The @var{prev}
 * argument should be NULL on the first call.  Returns NULL at the end.
 * @end deftypefun
@*/
jit_function_t jit_function_next(jit_context_t context, jit_function_t prev)
{
	if(prev)
	{
		return prev->next;
	}
	else if(context)
	{
		return context->functions;
	}
	else
	{
		return 0;
	}
}

/*@
 * @deftypefun jit_function_t jit_function_previous (jit_context_t @var{context}, jit_function_t @var{prev})
 * Iterate over the defined functions in reverse creation order.
 * @end deftypefun
@*/
jit_function_t jit_function_previous(jit_context_t context, jit_function_t prev)
{
	if(prev)
	{
		return prev->prev;
	}
	else if(context)
	{
		return context->last_function;
	}
	else
	{
		return 0;
	}
}

/*@
 * @deftypefun jit_block_t jit_function_get_entry (jit_function_t @var{func})
 * Get the entry block for a function.  This is always the first block
 * created by @code{jit_function_create}.
 * @end deftypefun
@*/
jit_block_t jit_function_get_entry(jit_function_t func)
{
	if(func && func->builder)
	{
		return func->builder->entry;
	}
	else
	{
		return 0;
	}
}

/*@
 * @deftypefun jit_block_t jit_function_get_current (jit_function_t @var{func})
 * Get the current block for a function.  New blocks are created by
 * certain @code{jit_insn_xxx} calls.
 * @end deftypefun
@*/
jit_block_t jit_function_get_current(jit_function_t func)
{
	if(func && func->builder)
	{
		return func->builder->current_block;
	}
	else
	{
		return 0;
	}
}

/*@
 * @deftypefun jit_function_t jit_function_get_nested_parent (jit_function_t @var{func})
 * Get the nested parent for a function, or NULL if @var{func}
 * does not have a nested parent.
 * @end deftypefun
@*/
jit_function_t jit_function_get_nested_parent(jit_function_t func)
{
	if(func)
	{
		return func->nested_parent;
	}
	else
	{
		return 0;
	}
}

/*
 * Compile a single basic block within a function.
 */
static void compile_block(jit_gencode_t gen, jit_function_t func,
						  jit_block_t block)
{
	jit_insn_iter_t iter;
	jit_insn_t insn;

#ifdef _JIT_COMPILE_DEBUG
	printf("Block #%d: %d\n", func->builder->block_count++, block->label);
#endif

	/* Iterate over all blocks in the function */
	jit_insn_iter_init(&iter, block);
	while((insn = jit_insn_iter_next(&iter)) != 0)
	{
#ifdef _JIT_COMPILE_DEBUG
		unsigned char *p1, *p2;
		p1 = gen->posn.ptr;
		printf("Insn: %5d, Opcode: 0x%04x\n", func->builder->insn_count++, insn->opcode);
		printf("Start of binary code: 0x%08x\n", p1);
#endif

		switch(insn->opcode)
		{
			case JIT_OP_NOP:		break;		/* Ignore NOP's */

			case JIT_OP_CHECK_NULL:
			{
				/* Determine if we can optimize the null check away */
				if(!_jit_insn_check_is_redundant(&iter))
				{
					_jit_gen_insn(gen, func, block, insn);
				}
			}
			break;

#ifndef JIT_BACKEND_INTERP
			case JIT_OP_INCOMING_REG:
			{
				/* Assign a register to an incoming value */
				_jit_regs_set_incoming
					(gen, (int)jit_value_get_nint_constant(insn->value2),
					 insn->value1);
				_jit_gen_insn(gen, func, block, insn);
			}
			break;
#endif

			case JIT_OP_INCOMING_FRAME_POSN:
			{
				/* Set the frame position for an incoming value */
				insn->value1->frame_offset =
					jit_value_get_nint_constant(insn->value2);
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
			}
			break;

#ifndef JIT_BACKEND_INTERP
			case JIT_OP_OUTGOING_REG:
			{
				/* Copy a value into an outgoing register */
				_jit_regs_set_outgoing
					(gen, (int)jit_value_get_nint_constant(insn->value2),
					 insn->value1);
			}
			break;
#endif

			case JIT_OP_OUTGOING_FRAME_POSN:
			{
				/* Set the frame position for an outgoing value */
				insn->value1->frame_offset =
					jit_value_get_nint_constant(insn->value2);
				insn->value1->in_register = 0;
				insn->value1->in_global_register = 0;
				insn->value1->in_frame = 0;
				insn->value1->has_frame_offset = 1;
				insn->value1->has_global_register = 0;
			}
			break;

#ifndef JIT_BACKEND_INTERP
			case JIT_OP_RETURN_REG:
			{
				/* Assign a register to a return value */
				_jit_regs_set_incoming
					(gen, (int)jit_value_get_nint_constant(insn->value2),
					 insn->value1);
				_jit_gen_insn(gen, func, block, insn);
			}
			break;
#endif

			case JIT_OP_MARK_OFFSET:
			{
				/* Mark the current code position as corresponding
				   to a particular bytecode offset */
				_jit_cache_mark_bytecode
					(&(gen->posn), (unsigned long)(long)
							jit_value_get_nint_constant(insn->value1));
			}
			break;

			default:
			{
				/* Generate code for the instruction with the back end */
				_jit_gen_insn(gen, func, block, insn);
			}
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
 * Reset value on restart.
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
 * Clean up the compilation state on restart.
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
 * Compile a function and return its entry point.
 */
static int
compile(jit_function_t func, void **entry_point)
{
	struct jit_gencode gen;
	jit_cache_t cache;
	unsigned char *start;
	unsigned char *end;
	jit_block_t block;
	int page_factor;
	int result;

	/* Initialize the code generation state */
	jit_memzero(&gen, sizeof(gen));
	page_factor = 0;
	start = 0;
	end = 0;

	/* Intuit "nothrow" and "noreturn" flags for this function */
	if(!(func->builder->may_throw))
	{
		func->no_throw = 1;
	}
	if(!(func->builder->ordinary_return))
	{
		func->no_return = 1;
	}

	/* Compute liveness and "next use" information for this function */
	_jit_function_compute_liveness(func);

	/* Allocate global registers to variables within the function */
#ifndef JIT_BACKEND_INTERP
	_jit_regs_alloc_global(&gen, func);
#endif

	/* We need the cache lock while we are compiling the function */
	jit_mutex_lock(&(func->context->cache_lock));

#ifdef _JIT_COMPILE_DEBUG
	printf("\n*** Start compilation ***\n\n");
	func->builder->block_count = 0;
	func->builder->insn_count = 0;
#endif

	/* Get the method cache */
	cache = _jit_context_get_cache(func->context);
	if(!cache)
	{
		jit_mutex_unlock(&(func->context->cache_lock));
		return 0;
	}

	/* Start function output to the cache */
	result = _jit_cache_start_method(cache, &(gen.posn),
					 page_factor++,
					 JIT_FUNCTION_ALIGNMENT, func);
	if (result == JIT_CACHE_RESTART)
	{
		/* No space left on the current cache page.  Allocate a new one. */
		result = _jit_cache_start_method(cache, &(gen.posn),
						 page_factor++,
						 JIT_FUNCTION_ALIGNMENT, func);
	}
	if (result != JIT_CACHE_OK)
	{
		/* Failed to allocate any cache space */
		jit_mutex_unlock(&(func->context->cache_lock));
		return 0;
	}

	for(;;)
	{
		start = gen.posn.ptr;

#ifdef jit_extra_gen_init
		/* Initialize information that may need to be reset each loop */
		jit_extra_gen_init(&gen);
#endif

#ifdef JIT_PROLOG_SIZE
		/* Output space for the function prolog */
		if(!jit_cache_check_for_n(&(gen.posn), JIT_PROLOG_SIZE))
		{
			/* No space left on the current cache page.  Restart. */
			jit_cache_mark_full(&(gen.posn));
			goto restart;
		}
		gen.posn.ptr += JIT_PROLOG_SIZE;
#endif

		/* Generate code for the blocks in the function */
		block = 0;
		while((block = jit_block_next(func, block)) != 0)
		{
			/* If this block is never entered, then discard it */
			if(!(block->entered_via_top) && !(block->entered_via_branch))
			{
				continue;
			}

			/* Notify the back end that the block is starting */
			_jit_gen_start_block(&gen, block);

#ifndef JIT_BACKEND_INTERP
			/* Clear the local register assignments */
			_jit_regs_init_for_block(&gen);
#endif

			/* Generate the block's code */
			compile_block(&gen, func, block);

#ifndef JIT_BACKEND_INTERP
			/* Spill all live register values back to their frame positions */
			_jit_regs_spill_all(&gen);
#endif

			/* Notify the back end that the block is finished */
			_jit_gen_end_block(&gen, block);

			/* Stop code generation if the cache page is full */
			if(_jit_cache_is_full(cache, &(gen.posn)))
			{
				/* No space left on the current cache page.  Restart. */
				goto restart;
			}
		}

		/* Output the function epilog.  All return paths will jump to here */
		_jit_gen_epilog(&gen, func);
		end = gen.posn.ptr;

#ifdef JIT_PROLOG_SIZE
		/* Back-patch the function prolog and get the real entry point */
		start = _jit_gen_prolog(&gen, func, start);
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

	restart:
		/* End the function's output process */
		result = _jit_cache_end_method(&(gen.posn));
		if(result != JIT_CACHE_RESTART)
		{
			break;
		}

		/* Clean up the compilation state before restart */
		cleanup_on_restart(&gen, func);

#ifdef _JIT_COMPILE_DEBUG
		printf("\n*** Restart compilation ***\n\n");
		func->builder->block_count = 0;
		func->builder->insn_count = 0;
#endif

		/* Restart function output to the cache */
		result = _jit_cache_start_method(cache, &(gen.posn),
						 page_factor,
						 JIT_FUNCTION_ALIGNMENT, func);
		if(result != JIT_CACHE_OK)
		{
#ifdef jit_extra_gen_cleanup
			/* Clean up the extra code generation state */
			jit_extra_gen_cleanup(gen);
#endif
			jit_mutex_unlock(&(func->context->cache_lock));
			return 0;
		}
		page_factor *= 2;
	}

#ifdef jit_extra_gen_cleanup
	/* Clean up the extra code generation state */
	jit_extra_gen_cleanup(gen);
#endif

	/* Bail out if we ran out of memory while translating the function */
	if(result != JIT_CACHE_OK)
	{
		jit_mutex_unlock(&(func->context->cache_lock));
		return 0;
	}

#ifndef JIT_BACKEND_INTERP
	/* Perform a CPU cache flush, to make the code executable */
	jit_flush_exec(start, (unsigned int)(end - start));
#endif

	/* The function has been compiled successfully */
	jit_mutex_unlock(&(func->context->cache_lock));

	/* Free the builder structure, which we no longer require */
	_jit_function_free_builder(func);

	/* Record the entry point */
	if(entry_point)
	{
		*entry_point = start;
	}

	return 1;
}

/*
 * Information that is stored for an exception region in the cache.
 */
typedef struct jit_cache_eh *jit_cache_eh_t;
struct jit_cache_eh
{
	jit_label_t		handler_label;
	unsigned char  *handler;
	jit_cache_eh_t	previous;
};

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
int jit_function_compile(jit_function_t func)
{
	int result;
	void *entry_point;

	/* Bail out if we have nothing to do */
	if(!func)
	{
		return 0;
	}
	if(func->is_compiled && !(func->builder))
	{
		/* The function is already compiled, and we don't need to recompile */
		return 1;
	}
	if(!(func->builder))
	{
		/* We don't have anything to compile at all */
		return 0;
	}

	/* Compile and record the entry point. */
	result = compile(func, &entry_point);
	if(result)
	{
		func->entry_point = entry_point;
		func->is_compiled = 1;
	}

	return result;
}

/*@
 * @deftypefun int jit_function_compile_entry (jit_function_t @var{func}, void **@var{entry_point})
 * Compile a function to its executable form but do not make it
 * available for invocation yet. It may be made available later
 * with @code{jit_function_setup_entry}.
 * @end deftypefun
@*/
int
jit_function_compile_entry(jit_function_t func, void **entry_point)
{
	/* Init entry_point */
	if(entry_point)
	{
		*entry_point = 0;
	}
	else
	{
		return 0;
	}

	/* Bail out if we have nothing to do */
	if(!func)
	{
		return 0;
	}
	if(func->is_compiled && !(func->builder))
	{
		/* The function is already compiled, and we don't need to recompile */
		*entry_point = func->entry_point;
		return 1;
	}
	if(!(func->builder))
	{
		/* We don't have anything to compile at all */
		return 0;
	}

	/* Compile and return the entry point. */
	return compile(func, entry_point);
}

/*@
 * @deftypefun int jit_function_setup_entry (jit_function_t @var{func}, void *@var{entry_point})
 * Make a function compiled with @code{jit_function_compile_entry}
 * available for invocation and free the resources used for
 * compilation. If @var{entry_point} is null then it only
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
 * @deftypefun int jit_function_is_compiled (jit_function_t @var{func})
 * Determine if a function has already been compiled.
 * @end deftypefun
@*/
int jit_function_is_compiled(jit_function_t func)
{
	if(func)
	{
		return func->is_compiled;
	}
	else
	{
		return 0;
	}
}

/*@
 * @deftypefun int jit_function_set_recompilable (jit_function_t @var{func})
 * Mark this function as a candidate for recompilation.  That is,
 * it is possible that we may call @code{jit_function_compile}
 * more than once, to re-optimize an existing function.
 *
 * It is very important that this be called before the first time that
 * you call @code{jit_function_compile}.  Functions that are recompilable
 * are invoked in a slightly different way to non-recompilable functions.
 * If you don't set this flag, then existing invocations of the function
 * may continue to be sent to the original compiled version, not the new
 * version.
 * @end deftypefun
@*/
void jit_function_set_recompilable(jit_function_t func)
{
	if(func)
	{
		func->is_recompilable = 1;
	}
}

/*@
 * @deftypefun void jit_function_clear_recompilable (jit_function_t @var{func})
 * Clear the recompilable flag on this function.  Normally you would use
 * this once you have decided that the function has been optimized enough,
 * and that you no longer intend to call @code{jit_function_compile} again.
 *
 * Future uses of the function with @code{jit_insn_call} will output a
 * direct call to the function, which is more efficient than calling
 * its recompilable version.  Pre-existing calls to the function may still
 * use redirection stubs, and will remain so until the pre-existing
 * functions are themselves recompiled.
 * @end deftypefun
@*/
void jit_function_clear_recompilable(jit_function_t func)
{
	if(func)
	{
		func->is_recompilable = 0;
	}
}

/*@
 * @deftypefun int jit_function_is_recompilable (jit_function_t @var{func})
 * Determine if this function is recompilable.
 * @end deftypefun
@*/
int jit_function_is_recompilable(jit_function_t func)
{
	if(func)
	{
		return func->is_recompilable;
	}
	else
	{
		return 0;
	}
}

#ifdef JIT_BACKEND_INTERP

/*
 * Closure handling function for "jit_function_to_closure".
 */
static void function_closure(jit_type_t signature, void *result,
							 void **args, void *user_data)
{
	if(!jit_function_apply((jit_function_t)user_data, args, result))
	{
		/* We cannot report the exception through the closure,
		   so we have no choice but to rethrow it up the stack */
		jit_exception_throw(jit_exception_get_last());
	}
}

#endif /* JIT_BACKEND_INTERP */

/*@
 * @deftypefun {void *} jit_function_to_closure (jit_function_t @var{func})
 * Convert a compiled function into a closure that can called directly
 * from C.  Returns NULL if out of memory, or if closures are not
 * supported on this platform.
 *
 * If the function has not been compiled yet, then this will return
 * a pointer to a redirector that will arrange for the function to be
 * compiled on-demand when it is called.
 *
 * Creating a closure for a nested function is not recommended as
 * C does not have any way to call such closures directly.
 * @end deftypefun
@*/
void *jit_function_to_closure(jit_function_t func)
{
	if(!func)
	{
		return 0;
	}
#ifdef JIT_BACKEND_INTERP
	return jit_closure_create(func->context, func->signature,
							  function_closure, (void *)func);
#else
	/* On native platforms, use the closure entry point */
	if(func->indirector && (!func->is_compiled || func->is_recompilable))
	{
		return func->indirector;
	}
	return func->entry_point;
#endif
}

/*@
 * @deftypefun jit_function_t jit_function_from_closure (jit_context_t @var{context}, void *@var{closure})
 * Convert a closure back into a function.  Returns NULL if the
 * closure does not correspond to a function in the specified context.
 * @end deftypefun
@*/
jit_function_t jit_function_from_closure(jit_context_t context, void *closure)
{
	void *cookie;
	if(!context || !(context->cache))
	{
		return 0;
	}
	return (jit_function_t)_jit_cache_get_method
		(context->cache, closure, &cookie);
}

/*@
 * @deftypefun jit_function_t jit_function_from_pc (jit_context_t @var{context}, void *@var{pc}, void **@var{handler})
 * Get the function that contains the specified program counter location.
 * Also return the address of the @code{catch} handler for the same location.
 * Returns NULL if the program counter does not correspond to a function
 * under the control of @var{context}.
 * @end deftypefun
@*/
jit_function_t jit_function_from_pc
	(jit_context_t context, void *pc, void **handler)
{
	jit_function_t func;
	void *cookie;

	/* Bail out if we don't have a function cache yet */
	if(!context || !(context->cache))
	{
		return 0;
	}

	/* Get the function and the exception handler cookie */
	func = (jit_function_t)_jit_cache_get_method(context->cache, pc, &cookie);
	if(!func)
	{
		return 0;
	}

	/* Convert the cookie into a handler address */
	if(handler)
	{
#if 0
		if(cookie)
		{
			*handler = ((jit_cache_eh_t)cookie)->handler;
		}
		else
		{
			*handler = 0;
		}
#else
		*handler = cookie;
#endif
	}
	return func;
}

/*@
 * @deftypefun {void *} jit_function_to_vtable_pointer (jit_function_t @var{func})
 * Return a pointer that is suitable for referring to this function
 * from a vtable.  Such pointers should only be used with the
 * @code{jit_insn_call_vtable} instruction.
 *
 * Using @code{jit_insn_call_vtable} is generally more efficient than
 * @code{jit_insn_call_indirect} for calling virtual methods.
 *
 * The vtable pointer might be the same as the closure, but this isn't
 * guaranteed.  Closures can be used with @code{jit_insn_call_indirect}.
 * @end deftypefun
@*/
void *jit_function_to_vtable_pointer(jit_function_t func)
{
#ifdef JIT_BACKEND_INTERP
	/* In the interpreted version, the function pointer is used in vtables */
	return func;
#else
	/* On native platforms, the closure entry point is the vtable pointer */
	if(!func)
	{
		return 0;
	}
	if(func->indirector && (!func->is_compiled || func->is_recompilable))
	{
		return func->indirector;
	}
	return func->entry_point;
#endif
}

/*@
 * @deftypefun jit_function_t jit_function_from_vtable_pointer (jit_context_t @var{context}, void *@var{vtable_pointer})
 * Convert a vtable_pointer back into a function.  Returns NULL if the
 * vtable_pointer does not correspond to a function in the specified context.
 * @end deftypefun
@*/
jit_function_t jit_function_from_vtable_pointer(jit_context_t context, void *vtable_pointer)
{
#ifdef JIT_BACKEND_INTERP
	/* In the interpreted version, the function pointer is used in vtables */
	jit_function_t func = (jit_function_t)vtable_pointer;

	if(func && func->context == context)
	{
		return func;
	}
	return 0;
#else
	void *cookie;
	if(!context || !(context->cache))
	{
		return 0;
	}
	return (jit_function_t)_jit_cache_get_method
		(context->cache, vtable_pointer, &cookie);
#endif
}

/*@
 * @deftypefun void jit_function_set_on_demand_compiler (jit_function_t @var{func}, jit_on_demand_func @var{on_demand})
 * Specify the C function to be called when @var{func} needs to be
 * compiled on-demand.  This should be set just after the function
 * is created, before any build or compile processes begin.
 *
 * You won't need an on-demand compiler if you always build and compile
 * your functions before you call them.  But if you can call a function
 * before it is built, then you must supply an on-demand compiler.
 *
 * When on-demand compilation is requested, @code{libjit} takes the following
 * actions:
 *
 * @enumerate
 * @item
 * The context is locked by calling @code{jit_context_build_start}.
 *
 * @item
 * If the function has already been compiled, @code{libjit} unlocks
 * the context and returns immediately.  This can happen because of race
 * conditions between threads: some other thread may have beaten us
 * to the on-demand compiler.
 *
 * @item
 * The user's on-demand compiler is called.  It is responsible for building
 * the instructions in the function's body.  It should return one of the
 * result codes @code{JIT_RESULT_OK}, @code{JIT_RESULT_COMPILE_ERROR},
 * or @code{JIT_RESULT_OUT_OF_MEMORY}.
 *
 * @item
 * If the user's on-demand function hasn't already done so, @code{libjit}
 * will call @code{jit_function_compile} to compile the function.
 *
 * @item
 * The context is unlocked by calling @code{jit_context_build_end} and
 * @code{libjit} jumps to the newly-compiled entry point.  If an error
 * occurs, a built-in exception of type @code{JIT_RESULT_COMPILE_ERROR}
 * or @code{JIT_RESULT_OUT_OF_MEMORY} will be thrown.
 * @end enumerate
 *
 * Normally you will need some kind of context information to tell you
 * which higher-level construct is being compiled.  You can use the
 * metadata facility to add this context information to the function
 * just after you create it with @code{jit_function_create}.
 * @end deftypefun
@*/
void
jit_function_set_on_demand_compiler(jit_function_t func, jit_on_demand_func on_demand)
{
	if(func)
	{
		func->on_demand = on_demand;
	}
}

/*@
 * @deftypefun jit_on_demand_func jit_function_get_on_demand_compiler (jit_function_t @var{func})
 * Returns function's on-demand compiler.
 * @end deftypefun
@*/
jit_on_demand_func
jit_function_get_on_demand_compiler(jit_function_t func)
{
	if(func)
	{
		return func->on_demand;
	}
	return 0;
}

void *_jit_function_compile_on_demand(jit_function_t func)
{
	void *entry = 0;
	int result = JIT_RESULT_OK;

	/* Lock down the context */
	jit_context_build_start(func->context);

	/* If we are already compiled, then bail out */
	if(func->is_compiled)
	{
		entry = func->entry_point;
		jit_context_build_end(func->context);
		return entry;
	}

	/* Call the user's on-demand compiler.  Bail out with an error
	   if the user didn't supply an on-demand compiler */
	if(func->on_demand)
	{
		result = (*(func->on_demand))(func);
		if(result == JIT_RESULT_OK)
		{
			/* Compile the function if the user didn't do so */
			if(!(func->is_compiled))
			{
				if(jit_function_compile(func))
				{
					entry = func->entry_point;
				}
				else
				{
					result = JIT_RESULT_OUT_OF_MEMORY;
				}
			}
			else
			{
				entry = func->entry_point;
			}
		}
		_jit_function_free_builder(func);
	}
	else
	{
		result = JIT_RESULT_COMPILE_ERROR;
	}

	/* Unlock the context and report the result */
	jit_context_build_end(func->context);
	if(result != JIT_RESULT_OK)
	{
		jit_exception_builtin(result);
	}
	return entry;
}

/*@
 * @deftypefun int jit_function_apply (jit_function_t @var{func}, void **@var{args}, void *@var{return_area})
 * Call the function @var{func} with the supplied arguments.  Each element
 * in @var{args} is a pointer to one of the arguments, and @var{return_area}
 * points to a buffer to receive the return value.  Returns zero if an
 * exception occurred.
 *
 * This is the primary means for executing a function from ordinary
 * C code without creating a closure first with @code{jit_function_to_closure}.
 * Closures may not be supported on all platforms, but function application
 * is guaranteed to be supported everywhere.
 *
 * Function applications acts as an exception blocker.  If any exceptions
 * occur during the execution of @var{func}, they won't travel up the
 * stack any further than this point.  This prevents ordinary C code
 * from being accidentally presented with a situation that it cannot handle.
 * This blocking protection is not present when a function is invoked
 * via its closure.
 * @end deftypefun
 *
 * @deftypefun int jit_function_apply_vararg (jit_function_t @var{func}, jit_type_t @var{signature}, void **@var{args}, void *@var{return_area})
 * Call the function @var{func} with the supplied arguments.  There may
 * be more arguments than are specified in the function's original signature,
 * in which case the additional values are passed as variable arguments.
 * This function is otherwise identical to @code{jit_function_apply}.
 * @end deftypefun
@*/
#if !defined(JIT_BACKEND_INTERP)
/* The interpreter version is in "jit-interp.cpp" */

int jit_function_apply(jit_function_t func, void **args, void *return_area)
{
	if(func)
	{
		return jit_function_apply_vararg
			(func, func->signature, args, return_area);
	}
	else
	{
		return jit_function_apply_vararg(func, 0, args, return_area);
	}
}

int jit_function_apply_vararg
	(jit_function_t func, jit_type_t signature, void **args, void *return_area)
{
	struct jit_backtrace call_trace;
	void *entry;
	jit_jmp_buf jbuf;

	/* Establish a "setjmp" point here so that we can unwind the
	   stack to this point when an exception occurs and then prevent
	   the exception from propagating further up the stack */
	_jit_unwind_push_setjmp(&jbuf);
	if(setjmp(jbuf.buf))
	{
		_jit_unwind_pop_setjmp();
		return 0;
	}

	/* Create a backtrace entry that blocks exceptions from
	   flowing further than this up the stack */
	_jit_backtrace_push(&call_trace, 0);

	/* Get the function's entry point */
	if(!func)
	{
		jit_exception_builtin(JIT_RESULT_NULL_FUNCTION);
		return 0;
	}
	if(func->nested_parent)
	{
		jit_exception_builtin(JIT_RESULT_CALLED_NESTED);
		return 0;
	}
	if(func->is_compiled)
	{
		entry = func->entry_point;
	}
	else
	{
		entry = (*func->context->on_demand_driver)(func);
	}

	/* Get the default signature if necessary */
	if(!signature)
	{
		signature = func->signature;
	}

	/* Clear the exception state */
	jit_exception_clear_last();

	/* Apply the function.  If it returns, then there is no exception */
	jit_apply(signature, func->entry_point, args,
			  jit_type_num_params(func->signature), return_area);

	/* Restore the backtrace and "setjmp" contexts and exit */
	_jit_unwind_pop_setjmp();
	return 1;
}

#endif /* !JIT_BACKEND_INTERP */

/*@
 * @deftypefun void jit_function_set_optimization_level (jit_function_t @var{func}, unsigned int @var{level})
 * Set the optimization level for @var{func}.  Increasing values indicate
 * that the @code{libjit} dynamic compiler should expend more effort to
 * generate better code for this function.  Usually you would increase
 * this value just before forcing @var{func} to recompile.
 *
 * When the optimization level reaches the value returned by
 * @code{jit_function_get_max_optimization_level()}, there is usually
 * little point in continuing to recompile the function because
 * @code{libjit} may not be able to do any better.
 *
 * The front end is usually responsible for choosing candidates for
 * function inlining.  If it has identified more such candidates, then
 * it may still want to recompile @var{func} again even once it has
 * reached the maximum optimization level.
 * @end deftypefun
@*/
void jit_function_set_optimization_level
	(jit_function_t func, unsigned int level)
{
	unsigned int max_level = jit_function_get_max_optimization_level();
	if(level > max_level)
	{
		level = max_level;
	}
	if(func)
	{
		func->optimization_level = (int)level;
	}
}

/*@
 * @deftypefun {unsigned int} jit_function_get_optimization_level (jit_function_t @var{func})
 * Get the current optimization level for @var{func}.
 * @end deftypefun
@*/
unsigned int jit_function_get_optimization_level(jit_function_t func)
{
	if(func)
	{
		return (unsigned int)(func->optimization_level);
	}
	else
	{
		return 0;
	}
}

/*@
 * @deftypefun {unsigned int} jit_function_get_max_optimization_level (void)
 * Get the maximum optimization level that is supported by @code{libjit}.
 * @end deftypefun
@*/
unsigned int jit_function_get_max_optimization_level(void)
{
	/* TODO - implement more than basic optimization */
	return 0;
}

/*@
 * @deftypefun {jit_label_t} jit_function_reserve_label (jit_function_t @var{func})
 * Allocate a new label for later use within the function @var{func}. Most
 * instructions that require a label could perform label allocation themselves.
 * A separate label allocation could be useful to fill a jump table with
 * identical entries.
 * @end deftypefun
@*/
jit_label_t
jit_function_reserve_label(jit_function_t func)
{
	/* Ensure that we have a function builder */
	if(!_jit_function_ensure_builder(func))
	{
		return jit_label_undefined;
	}

	return (func->builder->next_label)++;
}
