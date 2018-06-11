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

#ifdef _JIT_FLOW_DEBUG
#include <jit/jit-dump.h>
#endif

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

/*
 * This function is based on the Iterative Data-Flow Analysis
 * algorithm from chapter 9 of "Engineering a Compiler"
 * by Keith D. Cooper and Linda Torczon.
 */
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

static void
insn_list_add(_jit_insn_list_t *list, jit_block_t block, jit_insn_t insn)
{
	_jit_insn_list_t entry = jit_new(struct _jit_insn_list);
	entry->block = block;
	entry->insn = insn;
	entry->next = *list;
	*list = entry;
}

static void
insn_list_remove(_jit_insn_list_t *list, jit_insn_t insn)
{
	_jit_insn_list_t curr = *list;
	while(curr)
	{
		if(curr->insn == insn)
		{
			*list = curr->next;
			jit_free(curr);
			return;
		}

		list = &curr->next;
		curr = curr->next;
	}
}

static jit_insn_t
insn_list_get_insn_from_block(_jit_insn_list_t list, jit_block_t block)
{
	while(list)
	{
		if(list->block == block)
		{
			return list->insn;
		}
		list = list->next;
	}
	return 0;
}

static _jit_live_range_t
create_live_range(jit_function_t func, jit_value_t value)
{
	_jit_live_range_t range;

	range = jit_new(struct _jit_live_range);
	range->value = value;
	range->starts = 0;
	range->ends = 0;

	range->value_next = value->live_ranges;
	value->live_ranges = range;
	range->func_next = func->live_ranges;
	func->live_ranges = range;

	_jit_bitset_init(&range->touched_block_starts);
	_jit_bitset_allocate(&range->touched_block_starts,
		func->builder->block_count);

	_jit_bitset_init(&range->touched_block_ends);
	_jit_bitset_allocate(&range->touched_block_ends,
		func->builder->block_count);

	return range;
}

static void
flood_fill_touched_succs(jit_block_t block, _jit_live_range_t range, jit_value_t value);

static void
flood_fill_touched_preds(jit_block_t block, _jit_live_range_t range, jit_value_t value)
{
	jit_block_t pred;
	int i;

	_jit_bitset_set_bit(&range->touched_block_starts, block->index);

	for(i = 0; i < block->num_preds; i++)
	{
		pred = block->preds[i]->src;
		if(_jit_bitset_test_bit(&range->touched_block_ends, pred->index))
		{
			continue;
		}

		flood_fill_touched_succs(pred, range, value);

		if(!_jit_bitset_test_bit(&pred->var_kills, value->index))
		{
			flood_fill_touched_preds(pred, range, value);
		}
	}
}

static void
flood_fill_touched_succs(jit_block_t block, _jit_live_range_t range, jit_value_t value)
{
	jit_block_t succ;
	int i;

	_jit_bitset_set_bit(&range->touched_block_ends, block->index);

	for(i = 0; i < block->num_succs; i++)
	{
		succ = block->succs[i]->dst;
		if(_jit_bitset_test_bit(&range->touched_block_starts, succ->index))
		{
			continue;
		}

		if(_jit_bitset_test_bit(&succ->upward_exposes, value->index))
		{
			flood_fill_touched_preds(succ, range, value);
		}

		if(_jit_bitset_test_bit(&succ->live_out, value->index)
			&& !_jit_bitset_test_bit(&succ->var_kills, value->index))
		{
			flood_fill_touched_succs(succ, range, value);
		}
	}
}

static void
handle_live_range_use(jit_block_t block, jit_insn_t insn, jit_value_t value)
{
	_jit_live_range_t range;
	jit_block_t pred;
	int i;

	if(!value || value->is_constant)
	{
		return;
	}

	for(range = value->live_ranges; range; range = range->value_next)
	{
		/* If the range does not touch the start of the current block, this
		   cannot be an end for it */
		if(!_jit_bitset_test_bit(&range->touched_block_starts, block->index))
		{
			continue;
		}

		if(_jit_bitset_test_bit(&range->touched_block_ends, block->index))
		{
			/* The range is alive at the end of the block. This is only an end
			   if it is restart later in this block */
			if(insn_list_get_insn_from_block(range->starts, block) == 0)
			{
				insn_list_add(&range->ends, block, insn);
			}
		}
		else if(insn_list_get_insn_from_block(range->ends, block) == 0)
		{
			/* This is the last instruction in the block which uses the range
			   thus is ends the range */
			insn_list_add(&range->ends, block, insn);
		}

		return;
	}

	/* There is no live range that matches, we have to create a new one and
	   compute the touched blocks */
	range = create_live_range(block->func, value);

	if(_jit_bitset_test_bit(&block->upward_exposes, value->index))
	{
		flood_fill_touched_preds(block, range, value);
	}
	if(_jit_bitset_test_bit(&block->live_out, value->index))
	{
		flood_fill_touched_succs(block, range, value);
	}
	else
	{
		insn_list_add(&range->ends, block, insn);
	}
}

static void
handle_live_range_start(jit_block_t block, jit_insn_t insn, jit_value_t value)
{
	_jit_live_range_t range;
	jit_block_t succ;
	jit_insn_t end;
	int i;

	if(!value || value->is_constant)
	{
		return;
	}

	for(range = value->live_ranges; range; range = range->value_next)
	{
		end = insn_list_get_insn_from_block(range->ends, block);
		if(end != 0 && insn_list_get_insn_from_block(range->starts, block) == 0)
		{
			/* range is a local life range with one start (here) and one end */
			if(range->starts == 0 && range->ends->next == 0)
			{
				insn_list_add(&range->starts, block, insn);
				_jit_bitset_clear(&range->touched_block_starts);
				_jit_bitset_clear(&range->touched_block_ends);
			}
			else
			{
				insn_list_remove(&range->ends, end);

				range = create_live_range(block->func, value);
				insn_list_add(&range->starts, block, insn);
				insn_list_add(&range->ends, block, end);
			}
			return;
		}

		/* If the range does not touch the end of the current block, this
		   cannot be a start for it */
		if(!_jit_bitset_test_bit(&range->touched_block_ends, block->index))
		{
			continue;
		}

		insn_list_add(&range->starts, block, insn);
		return;
	}

	/* There is no live range that matches, we have to create a new one and
	   compute the touched blocks */
	range = create_live_range(block->func, value);
	insn_list_add(&range->starts, block, insn);

	if(_jit_bitset_test_bit(&block->live_out, value->index))
	{
		flood_fill_touched_succs(block, range, value);
	}
}

void _jit_function_compute_live_ranges(jit_function_t func)
{
	jit_block_t block;
	jit_insn_iter_t iter;
	jit_insn_t insn;
	int flags;
#ifdef _JIT_FLOW_DEBUG
	_jit_live_range_t range;
	_jit_insn_list_t curr;
	int i;
#endif

	for(block = func->builder->entry_block; block; block = block->next)
	{
		jit_insn_iter_init_last(&iter, block);
		while((insn = jit_insn_iter_previous(&iter)) != 0)
		{
			/* Skip NOP instructions, which may have arguments left
			   over from when the instruction was replaced, but which
			   are not relevant to our data flow analysis */
			if(insn->opcode == JIT_OP_NOP)
			{
				continue;
			}

			flags = insn->flags;

			if((flags & JIT_INSN_VALUE1_OTHER_FLAGS) == 0)
			{
				handle_live_range_use(block, insn, insn->value1);
			}

			if((flags & JIT_INSN_VALUE2_OTHER_FLAGS) == 0)
			{
				handle_live_range_use(block, insn, insn->value2);
			}

			if((flags & JIT_INSN_DEST_OTHER_FLAGS) == 0)
			{
				if((flags & JIT_INSN_DEST_IS_VALUE) == 0)
				{
					handle_live_range_start(block, insn, insn->dest);
				}
				else
				{
					/* The destination is actually a source value for this
					   instruction (e.g. JIT_OP_STORE_RELATIVE_*) */
					handle_live_range_use(block, insn, insn->dest);
				}
			}
		}
	}

#ifdef _JIT_FLOW_DEBUG
	i = 0;
	for(range = func->live_ranges; range; range = range->func_next)
	{
		printf("Live range %d for value ", i++);
		jit_dump_value(stdout, func, range->value, NULL);

		printf("\n    Starts:");
		for(curr = range->starts; curr; curr = curr->next)
		{
			printf("\n        ");
			jit_dump_insn(stdout, func, curr->insn);
		}
		printf("\n");

		printf("    Ends:");
		for(curr = range->ends; curr; curr = curr->next)
		{
			printf("\n        ");
			jit_dump_insn(stdout, func, curr->insn);
		}
		printf("\n\n");
	}
#endif
}
