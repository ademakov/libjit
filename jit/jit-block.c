/*
 * jit-block.c - Functions for manipulating blocks.
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
#include "jit-memory.h"

/*@

@cindex jit-block.h

@*/

int _jit_block_init(jit_function_t func)
{
	func->builder->entry = _jit_block_create(func, 0);
	if(!(func->builder->entry))
	{
		return 0;
	}
	func->builder->entry->entered_via_top = 1;
	func->builder->current_block = func->builder->entry;
	return 1;
}

void _jit_block_free(jit_function_t func)
{
	jit_block_t current = func->builder->first_block;
	jit_block_t next;
	jit_block_eh_t current_eh;
	jit_block_eh_t next_eh;
	while(current != 0)
	{
		next = current->next;
		jit_meta_destroy(&(current->meta));
		jit_free(current);
		current = next;
	}
	current_eh = func->builder->exception_handlers;
	while(current_eh != 0)
	{
		next_eh = current_eh->next;
		jit_free(current_eh);
		current_eh = next_eh;
	}
	func->builder->first_block = 0;
	func->builder->last_block = 0;
	func->builder->entry = 0;
	func->builder->current_block = 0;
	func->builder->exception_handlers = 0;
	func->builder->current_handler = 0;
}

jit_block_t _jit_block_create(jit_function_t func, jit_label_t *label)
{
	jit_block_t block;

	/* Allocate memory for the block */
	block = jit_cnew(struct _jit_block);
	if(!block)
	{
		return 0;
	}

	/* Initialize the block and set its label */
	block->func = func;
	block->first_insn = func->builder->num_insns;
	block->last_insn = block->first_insn - 1;
	if(label)
	{
		if(*label == jit_label_undefined)
		{
			*label = (func->builder->next_label)++;
		}
		block->label = *label;
		if(!_jit_block_record_label(block))
		{
			jit_free(block);
			return 0;
		}
	}
	else
	{
		block->label = jit_label_undefined;
	}

	/* Set the exception handling context for this block */
	block->block_eh = func->builder->current_handler;

	/* Add the block to the end of the function's list */
	block->next = 0;
	block->prev = func->builder->last_block;
	if(func->builder->last_block)
	{
		func->builder->last_block->next = block;
	}
	else
	{
		func->builder->first_block = block;
	}
	func->builder->last_block = block;
	return block;
}

int _jit_block_record_label(jit_block_t block)
{
	jit_builder_t builder = block->func->builder;
	jit_label_t num;
	jit_block_t *blocks;
	if(block->label >= builder->max_label_blocks)
	{
		num = builder->max_label_blocks;
		if(num < 64)
		{
			num = 64;
		}
		while(num <= block->label)
		{
			num *= 2;
		}
		blocks = (jit_block_t *)jit_realloc
			(builder->label_blocks, num * sizeof(jit_block_t));
		if(!blocks)
		{
			return 0;
		}
		jit_memzero(blocks + builder->max_label_blocks,
					sizeof(jit_block_t) * (num - builder->max_label_blocks));
		builder->label_blocks = blocks;
		builder->max_label_blocks = num;
	}
	builder->label_blocks[block->label] = block;
	return 1;
}

/*@
 * @deftypefun jit_function_t jit_block_get_function (jit_block_t block)
 * Get the function that a particular @code{block} belongs to.
 * @end deftypefun
@*/
jit_function_t jit_block_get_function(jit_block_t block)
{
	if(block)
	{
		return block->func;
	}
	else
	{
		return 0;
	}
}

/*@
 * @deftypefun jit_context_t jit_block_get_context (jit_block_t block)
 * Get the context that a particular @code{block} belongs to.
 * @end deftypefun
@*/
jit_context_t jit_block_get_context(jit_block_t block)
{
	if(block)
	{
		return block->func->context;
	}
	else
	{
		return 0;
	}
}

/*@
 * @deftypefun jit_label_t jit_block_get_label (jit_block_t block)
 * Get the label associated with a block.
 * @end deftypefun
@*/
jit_label_t jit_block_get_label(jit_block_t block)
{
	if(block)
	{
		return block->label;
	}
	else
	{
		return 0;
	}
}

/*@
 * @deftypefun jit_block_t jit_block_next (jit_function_t func, jit_block_t previous)
 * Iterate over the blocks in a function, in order of their creation.
 * The @code{previous} argument should be NULL on the first call.
 * This function will return NULL if there are no further blocks to iterate.
 * @end deftypefun
@*/
jit_block_t jit_block_next(jit_function_t func, jit_block_t previous)
{
	if(previous)
	{
		return previous->next;
	}
	else if(func && func->builder)
	{
		return func->builder->first_block;
	}
	else
	{
		return 0;
	}
}

/*@
 * @deftypefun jit_block_t jit_block_previous (jit_function_t func, jit_block_t previous)
 * Iterate over the blocks in a function, in reverse order of their creation.
 * The @code{previous} argument should be NULL on the first call.
 * This function will return NULL if there are no further blocks to iterate.
 * @end deftypefun
@*/
jit_block_t jit_block_previous(jit_function_t func, jit_block_t previous)
{
	if(previous)
	{
		return previous->prev;
	}
	else if(func && func->builder)
	{
		return func->builder->last_block;
	}
	else
	{
		return 0;
	}
}

/*@
 * @deftypefun jit_block_t jit_block_from_label (jit_function_t func, jit_label_t label)
 * Get the block that corresponds to a particular @code{label}.
 * Returns NULL if there is no block associated with the label.
 * @end deftypefun
@*/
jit_block_t jit_block_from_label(jit_function_t func, jit_label_t label)
{
	if(func && func->builder && label < func->builder->max_label_blocks)
	{
		return func->builder->label_blocks[label];
	}
	else
	{
		return 0;
	}
}

jit_insn_t _jit_block_add_insn(jit_block_t block)
{
	jit_builder_t builder = block->func->builder;
	jit_insn_t insn;
	int num;
	jit_insn_t *insns;

	/* Allocate the instruction from the builder's memory pool */
	insn = jit_memory_pool_alloc(&(builder->insn_pool), struct _jit_insn);
	if(!insn)
	{
		return 0;
	}

	/* Make space for the instruction in the function's instruction list */
	if(builder->num_insns >= builder->max_insns)
	{
		num = builder->max_insns * 2;
		if(num < 64)
		{
			num = 64;
		}
		insns = (jit_insn_t *)jit_realloc
			(builder->insns, num * sizeof(jit_insn_t));
		if(!insns)
		{
			return 0;
		}
		builder->insns = insns;
		builder->max_insns = num;
	}
	else
	{
		insns = builder->insns;
	}
	insns[builder->num_insns] = insn;
	block->last_insn = (builder->num_insns)++;

	/* Return the instruction, which is now ready to fill in */
	return insn;
}

jit_insn_t _jit_block_get_last(jit_block_t block)
{
	if(block->first_insn <= block->last_insn)
	{
		return block->func->builder->insns[block->last_insn];
	}
	else
	{
		return 0;
	}
}

/*@
 * @deftypefun int jit_block_set_meta (jit_block_t block, int type, {void *} data, jit_meta_free_func free_data)
 * Tag a block with some metadata.  Returns zero if out of memory.
 * If the @code{type} already has some metadata associated with it, then
 * the previous value will be freed.  Metadata may be used to store
 * dependency graphs, branch prediction information, or any other
 * information that is useful to optimizers or code generators.
 *
 * Metadata type values of 10000 or greater are reserved for internal use.
 * @end deftypefun
@*/
int jit_block_set_meta(jit_block_t block, int type, void *data,
					   jit_meta_free_func free_data)
{
	return jit_meta_set(&(block->meta), type, data, free_data, block->func);
}

/*@
 * @deftypefun {void *} jit_block_get_meta (jit_block_t block, int type)
 * Get the metadata associated with a particular tag.  Returns NULL
 * if @code{type} does not have any metadata associated with it.
 * @end deftypefun
@*/
void *jit_block_get_meta(jit_block_t block, int type)
{
	return jit_meta_get(block->meta, type);
}

/*@
 * @deftypefun void jit_block_free_meta (jit_block_t block, int type)
 * Free metadata of a specific type on a block.  Does nothing if
 * the @code{type} does not have any metadata associated with it.
 * @end deftypefun
@*/
void jit_block_free_meta(jit_block_t block, int type)
{
	jit_meta_free(&(block->meta), type);
}

/*@
 * @deftypefun int jit_block_is_reachable (jit_block_t block)
 * Determine if a block is reachable from some other point in
 * its function.  Unreachable blocks can be discarded in their
 * entirety.  If the JIT is uncertain as to whether a block is
 * reachable, or it does not wish to perform expensive flow
 * analysis to find out, then it will err on the side of caution
 * and assume that it is reachable.
 * @end deftypefun
@*/
int jit_block_is_reachable(jit_block_t block)
{
	return (block->entered_via_top || block->entered_via_branch);
}

/*@
 * @deftypefun int jit_block_ends_in_dead (jit_block_t block)
 * Determine if a block ends in a "dead" marker.  That is, control
 * will not fall out through the end of the block.
 * @end deftypefun
@*/
int jit_block_ends_in_dead(jit_block_t block)
{
	return block->ends_in_dead;
}

/*
 * Determine if a block is empty or is never entered.
 */
static int block_is_empty_or_dead(jit_block_t block)
{
	if(block->first_insn > block->last_insn)
	{
		return 1;
	}
	else if(!(block->entered_via_top) && !(block->entered_via_branch))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

/*
 * Determine if the next block after "block" has "label".
 */
static int block_branches_to_next(jit_block_t block, jit_label_t label)
{
	jit_insn_t insn;
	block = block->next;
	while(block != 0)
	{
		if(block->label == label)
		{
			return 1;
		}
		if(!block_is_empty_or_dead(block))
		{
			if(block->first_insn < block->last_insn)
			{
				/* This block contains more than one instruction, so the
				   first cannot be an unconditional branch */
				break;
			}
			else
			{
				insn = block->func->builder->insns[block->first_insn];
				if(insn->opcode == JIT_OP_BR)
				{
					/* If the instruction branches to its next block,
					   then it is equivalent to an empty block.  If it
					   does not, then we have to stop scanning here */
					if(!block_branches_to_next
							(block, (jit_label_t)(insn->dest)))
					{
						return 0;
					}
				}
				else
				{
					/* The block does not contain an unconditional branch */
					break;
				}
			}
		}
		block = block->next;
	}
	return 0;
}

void _jit_block_peephole_branch(jit_block_t block)
{
	jit_insn_t insn;
	jit_insn_t new_insn;
	jit_label_t label;
	jit_block_t new_block;
	int count;

	/* Bail out if the last instruction is not actually a branch */
	insn = _jit_block_get_last(block);
	if(!insn || insn->opcode < JIT_OP_BR || insn->opcode > JIT_OP_BR_NFGE_INV)
	{
		return;
	}

	/* Thread unconditional branches.  We stop if we jump back to the
	   starting block, or follow more than 32 links.  This is to prevent
	   infinite loops in situations like "while true do nothing" */
	label = (jit_label_t)(insn->dest);
	count = 32;
	while(label != block->label && count > 0)
	{
		new_block = jit_block_from_label(block->func, label);
		while(new_block != 0 && block_is_empty_or_dead(new_block))
		{
			/* Skip past empty blocks */
			new_block = new_block->next;
		}
		if(!new_block)
		{
			break;
		}
		if(new_block->first_insn < new_block->last_insn)
		{
			/* There is more than one instruction in this block,
			   so the first instruction cannot be a branch */
			break;
		}
		new_insn = new_block->func->builder->insns[new_block->first_insn];
		if(new_insn->opcode != JIT_OP_BR)
		{
			/* The target block does not contain an unconditional branch */
			break;
		}
		label = (jit_label_t)(new_insn->dest);
		--count;
	}
	insn->dest = (jit_value_t)label;

	/* Determine if we are branching to the immediately following block */
	if(block_branches_to_next(block, label))
	{
		/* Remove the branch instruction, because it has no effect.
		   It doesn't matter if the branch is unconditional or
		   conditional.  Any side-effects in a conditional expression
		   would have already been computed by now.  Expressions without
		   side-effects will be optimized away by liveness analysis */
		--(block->last_insn);
	}
}
