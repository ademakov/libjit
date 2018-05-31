/*
 * jit-block.c - Functions for manipulating blocks.
 *
 * Copyright (C) 2004  Southern Storm Software, Pty Ltd.
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

/*
 * This file is based on the Iterative Data-Flow Analysis
 * algorithm from chapter 9 of "Engineering a Compiler"
 * by Keith D. Cooper and Linda Torczon.
*/

static void
handle_source_value(jit_block_t block, jit_value_t value)
{
	if(!_jit_bitset_test_bit(&block->var_kills, value->index))
	{
		_jit_bitset_set_bit(&block->upward_exposes, value->index);
	}
}

static void
compute_kills_and_upward_exposes(jit_block_t block)
{
	jit_insn_iter_t iter;
	jit_insn_t insn;
	int flags;

	jit_insn_iter_init(&iter, block);
	while((insn = jit_insn_iter_next(&iter)) != 0)
	{
		/* Skip NOP instructions, which may have arguments left
		   over from when the instruction was replaced, but which
		   are not relevant to our data flow analysis */
		if(insn->opcode == JIT_OP_NOP)
		{
			continue;
		}

		flags = insn->flags;

		/* If value1 is a value not in VarKill add it to UEVar */
		if((flags & JIT_INSN_VALUE1_OTHER_FLAGS) == 0 && insn->value1
			&& !insn->value1->is_constant && insn->value1->is_local)
		{
			handle_source_value(block, insn->value1);
		}

		/* If value2 is a value not in VarKill add it to UEVar */
		if((flags & JIT_INSN_VALUE2_OTHER_FLAGS) == 0 && insn->value2
			&& !insn->value2->is_constant && insn->value2->is_local)
		{
			handle_source_value(block, insn->value2);
		}

		/* If dest is a destination value add it to VarKill
		   If it's a sourcevalue and not in VarKill add it to UEVar */
		if((flags & JIT_INSN_DEST_OTHER_FLAGS) == 0 && insn->dest
			&& !insn->dest->is_constant && insn->dest->is_local)
		{
			if((flags & JIT_INSN_DEST_IS_VALUE) == 0)
			{
				_jit_bitset_set_bit(&block->var_kills, insn->dest->index);
			}
			else
			{
				/* The destination is actually a source value for this
				   instruction (e.g. JIT_OP_STORE_RELATIVE_*) */
				handle_source_value(block, insn->dest);
			}
		}
	}
}

static void
compute_initial_live_out(jit_block_t block)
{
	int i;
	jit_block_t succ;

	for(i = 0; i < block->num_succs; i++)
	{
		succ = block->succs[i]->dst;

		_jit_bitset_add(&block->live_out, &succ->upward_exposes);
	}
}

/* Computes the LiveOut set of @var{block}
 *
 * LiveIn(m) is the list of all variables used before set in m
 * and all variables in LiveOut(m) which are never set in m.
 *
 * LiveOut(i) is the union of all of i's successor blocks LiveIn lists.
 * i.e. LiveOut(i) = union(LiveIn(m) foreach m in successors(i))
 */
static int
compute_live_out(jit_block_t block, _jit_bitset_t *tmp)
{
	int i;
	int changed = 0;
	jit_block_t succ;

	for(i = 0; i < block->num_succs; i++)
	{
		succ = block->succs[i]->dst;

		_jit_bitset_copy(tmp, &succ->live_out);
		_jit_bitset_sub(tmp, &succ->var_kills);

		if(!_jit_bitset_contains(&block->live_out, tmp))
		{
			changed = 1;
			_jit_bitset_add(&block->live_out, tmp);
		}
	}

	return changed;
}

void
_jit_function_compute_live_out(jit_function_t func)
{
	jit_block_t block;
	_jit_bitset_t tmp;
	int i;
	int changed = 1;
	int value_count = func->builder->value_count;

	/* Compute the UEVar and VarKill sets for each block */
	for(block = func->builder->entry_block; block; block = block->next)
	{
		if(_jit_bitset_is_allocated(&block->live_out))
		{
			if(_jit_bitset_size(&block->live_out) == value_count)
			{
				_jit_bitset_clear(&block->upward_exposes);
				_jit_bitset_clear(&block->var_kills);
				_jit_bitset_clear(&block->live_out);
			}
			else
			{
				_jit_block_free_live_out(block);
			}
		}

		if(!_jit_bitset_is_allocated(&block->live_out))
		{
			_jit_bitset_allocate(&block->upward_exposes, value_count);
			_jit_bitset_allocate(&block->var_kills, value_count);
			_jit_bitset_allocate(&block->live_out, value_count);
		}

		compute_kills_and_upward_exposes(block);
	}

	for(block = func->builder->entry_block; block; block = block->next)
	{
		compute_initial_live_out(block);
	}

	_jit_bitset_allocate(&tmp, value_count);

	/* Compute LiveOut sets for each block */
	while(changed)
	{
		changed = 0;
		for(i = 0; i < func->builder->num_block_order; i++)
		{
			block = func->builder->block_order[i];
			
			if(compute_live_out(block, &tmp))
				changed = 1;
		}
	}

	_jit_bitset_free(&tmp);
}

void
_jit_block_free_live_out(jit_block_t block)
{
	_jit_bitset_free(&block->upward_exposes);
	_jit_bitset_free(&block->var_kills);
	_jit_bitset_free(&block->live_out);
}

int
_jit_value_in_live_out(jit_block_t block, jit_value_t value)
{
	if(_jit_bitset_is_allocated(&block->live_out))
		return _jit_bitset_test_bit(&block->live_out, value->index);
	else
		return 1;
}
