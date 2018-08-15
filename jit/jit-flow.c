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
#include "jit-rules.h"
#include "jit-reg-alloc.h"
#include "jit-setjmp.h"
#include <assert.h>

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

void
_jit_insn_list_add(_jit_insn_list_t *list, jit_block_t block, jit_insn_t insn)
{
	_jit_insn_list_t entry = jit_new(struct _jit_insn_list);
	entry->block = block;
	entry->insn = insn;
	entry->next = *list;
	*list = entry;
}

void
_jit_insn_list_remove(_jit_insn_list_t *list, jit_insn_t insn)
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

jit_insn_t
_jit_insn_list_get_insn_from_block(_jit_insn_list_t list, jit_block_t block)
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

_jit_live_range_t
_jit_function_create_live_range(jit_function_t func,
	jit_value_t value)
{
	_jit_live_range_t range;

	range = jit_cnew(struct _jit_live_range);
	range->value = value;
	range->register_count = 1;
	range->func_next = 0;

	if(func->last_live_range == 0)
	{
		func->live_ranges = range;
		func->last_live_range = range;
	}
	else
	{
		func->last_live_range->func_next = range;
		func->last_live_range = range;
	}

	++func->live_range_count;

	if(value)
	{
		range->value_next = value->live_ranges;
		value->live_ranges = range;
	}
	else
	{
		range->value_next = 0;
	}

	_jit_bitset_init(&range->touched_block_starts);
	_jit_bitset_allocate(&range->touched_block_starts,
		func->builder->block_count);

	_jit_bitset_init(&range->touched_block_ends);
	_jit_bitset_allocate(&range->touched_block_ends,
		func->builder->block_count);

	_jit_bitset_init(&range->neighbors);

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
		if(!_jit_bitset_test_bit(&range->touched_block_ends, pred->index))
		{
			flood_fill_touched_succs(pred, range, value);
		}

		if(!_jit_bitset_test_bit(&range->touched_block_starts, pred->index)
			&& !_jit_bitset_test_bit(&pred->var_kills, value->index))
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
		if(!_jit_bitset_test_bit(&range->touched_block_starts, succ->index)
			&& _jit_bitset_test_bit(&succ->upward_exposes, value->index))
		{
			flood_fill_touched_preds(succ, range, value);
		}

		if(!_jit_bitset_test_bit(&range->touched_block_ends, succ->index)
			&& _jit_bitset_test_bit(&succ->live_out, value->index)
			&& !_jit_bitset_test_bit(&succ->var_kills, value->index))
		{
			flood_fill_touched_succs(succ, range, value);
		}
	}
}

static int
has_memory_type(jit_value_t value)
{
	switch(jit_type_remove_tags(value->type)->kind)
	{
	case JIT_TYPE_STRUCT:
	case JIT_TYPE_UNION:
	case JIT_TYPE_SIGNATURE:
		return 1;
	
	default:
		return 0;
	}
}

static _jit_live_range_t
handle_live_range_use(jit_block_t block,
	jit_insn_t prev, jit_insn_t insn, jit_value_t value)
{
	_jit_live_range_t range;

	if(!value || value->is_constant || has_memory_type(value))
	{
		return 0;
	}

	if(value->is_volatile || value->is_addressable)
	{
		/* TODO CAN_BE_IN_MEM */
		range = _jit_function_create_live_range(block->func, value);
		range->is_spill_range = 1;

		_jit_insn_list_add(&range->ends, block, insn);
		if(prev == 0)
		{
			_jit_insn_list_add(&range->starts, block, insn);
		}
		else
		{
			_jit_insn_list_add(&range->starts, block, prev);
		}

		return range;
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
			   if it is restarted later in this block */
			if(_jit_insn_list_get_insn_from_block(range->starts, block) == 0
				&& _jit_insn_list_get_insn_from_block(range->ends, block) == 0)
			{
				_jit_insn_list_add(&range->ends, block, insn);
			}
		}
		else if(_jit_insn_list_get_insn_from_block(range->ends, block) == 0)
		{
			/* This is the last instruction in the block which uses the range
			   thus it ends the range */
			_jit_insn_list_add(&range->ends, block, insn);
		}

		return range;
	}

	/* There is no live range that matches, we have to create a new one and
	   compute the touched blocks */
	range = _jit_function_create_live_range(block->func, value);

	if(_jit_bitset_test_bit(&block->upward_exposes, value->index))
	{
		flood_fill_touched_preds(block, range, value);
	}
	else
	{
		/* The live range does not really touch the start of the block.
		   The start will be found later and the bit cleared */
		_jit_bitset_set_bit(&range->touched_block_starts, block->index);
	}

	if(_jit_bitset_test_bit(&block->live_out, value->index))
	{
		flood_fill_touched_succs(block, range, value);
	}
	else
	{
		_jit_insn_list_add(&range->ends, block, insn);
	}

	return range;
}

static _jit_live_range_t
handle_live_range_start(jit_block_t block,
	jit_insn_t prev, jit_insn_t insn, jit_value_t value)
{
	_jit_live_range_t range;
	jit_insn_t end;

	if(!value || value->is_constant || has_memory_type(value))
	{
		return 0;
	}

	if(value->is_volatile || value->is_addressable)
	{
		/* TODO CAN_BE_IN_MEM */
		range = _jit_function_create_live_range(block->func, value);
		range->is_spill_range = 1;

		_jit_insn_list_add(&range->ends, block, insn);
		if(prev == 0)
		{
			_jit_insn_list_add(&range->starts, block, insn);
		}
		else
		{
			_jit_insn_list_add(&range->starts, block, prev);
		}

		return range;
	}

	for(range = value->live_ranges; range; range = range->value_next)
	{
		end = _jit_insn_list_get_insn_from_block(range->ends, block);
		if(end != 0
			&& _jit_insn_list_get_insn_from_block(range->starts, block) == 0
			&& !_jit_bitset_test_bit(&range->touched_block_ends, block->index))
		{
			if(range->starts == 0 && range->ends->next == 0)
			{
				/* range is a local life range with one start (here) and one
				   end (@var{end}) */
				_jit_insn_list_add(&range->starts, block, insn);
				_jit_bitset_clear(&range->touched_block_starts);
				_jit_bitset_clear(&range->touched_block_ends);
				return range;
			}
			else
			{
				/* @var{end} and insn are together a local live range however
				   @var{range} is a different one */
				_jit_insn_list_remove(&range->ends, end);

				range = _jit_function_create_live_range(block->func, value);
				_jit_insn_list_add(&range->starts, block, insn);
				_jit_insn_list_add(&range->ends, block, end);
				return range;
			}
		}

		/* If the range does not touch the end of the current block, this
		   cannot be a start for it */
		if(!_jit_bitset_test_bit(&range->touched_block_ends, block->index))
		{
			continue;
		}

		_jit_insn_list_add(&range->starts, block, insn);
		return range;
	}

	/* There is no live range that matches, we have to create a new one and
	   compute the touched blocks */
	range = _jit_function_create_live_range(block->func, value);
	_jit_insn_list_add(&range->starts, block, insn);

	if(_jit_bitset_test_bit(&block->live_out, value->index))
	{
		flood_fill_touched_succs(block, range, value);
	}

	return range;
}

void
_jit_function_compute_live_ranges(jit_function_t func)
{
	jit_block_t block;
	jit_insn_iter_t iter;
	jit_insn_t prev;
	jit_insn_t insn;
	int flags;

	for(block = func->builder->entry_block; block; block = block->next)
	{
		prev = 0;
		jit_insn_iter_init_last(&iter, block);
		while((insn = jit_insn_iter_previous(&iter)) != 0)
		{
			/* Skip NOP instructions, which may have arguments left
			   over from when the instruction was replaced, but which
			   are not relevant to our data flow analysis */
			if(insn->opcode == JIT_OP_NOP)
			{
				prev = insn;
				continue;
			}

			flags = insn->flags;

			if((flags & JIT_INSN_DEST_OTHER_FLAGS) == 0)
			{
				if((flags & JIT_INSN_DEST_IS_VALUE) == 0)
				{
					insn->dest_live = handle_live_range_start(block,
						prev, insn, insn->dest);
				}
				else
				{
					/* The destination is actually a source value for this
					   instruction (e.g. JIT_OP_STORE_RELATIVE_*) */
					insn->dest_live = handle_live_range_use(block,
						prev, insn, insn->dest);
				}
			}

			if((flags & JIT_INSN_VALUE1_OTHER_FLAGS) == 0)
			{
				insn->value1_live = handle_live_range_use(block,
					prev, insn, insn->value1);
			}

			if((flags & JIT_INSN_VALUE2_OTHER_FLAGS) == 0)
			{
				insn->value2_live = handle_live_range_use(block,
					prev, insn, insn->value2);
			}

			prev = insn;
		}
	}
}

_jit_live_range_t
create_dummy_live_range(jit_function_t func, jit_block_t block,
	jit_insn_t prev, jit_insn_t curr, jit_value_t value)
{
	_jit_live_range_t range;
	range = _jit_function_create_live_range(func, value);

	_jit_insn_list_add(&range->ends, block, curr);
	if(prev == 0)
	{
		_jit_insn_list_add(&range->starts, block, curr);
	}
	else
	{
		_jit_insn_list_add(&range->starts, block, prev);
	}

	return range;
}
_jit_live_range_t
create_fixed_live_range(jit_function_t func, jit_block_t block,
	jit_insn_t prev, jit_insn_t curr,
	jit_value_t value, jit_ulong colors)
{
	_jit_live_range_t range;
	int i;

	range = create_dummy_live_range(func, block, prev, curr, value);
	range->is_fixed = 1;
	range->colors = colors;

	range->register_count = 0;
	for(i = 0; i < JIT_NUM_REGS; i++)
	{
		if(colors & ((jit_ulong)1 << i))
		{
			++range->register_count;
		}
	}

	return range;
}
void
create_scratch_live_range(jit_function_t func, jit_block_t block,
	jit_insn_t prev, jit_insn_t curr, int regclass)
{
	_jit_live_range_t range;
	range = create_dummy_live_range(func, block, prev, curr, 0);
	range->value_next = curr->scratch_live;
	range->regflags = _jit_regclass_info[regclass]->flags;
	curr->scratch_live = range;
}

void
increment_preferred_color(jit_function_t func, jit_block_t block,
	jit_insn_t prev, jit_insn_t insn,
	_jit_live_range_t range, int reg, int reg_other)
{
	int colors;

	if(range == 0
		|| reg == _JIT_REG_USAGE_UNNAMED
		|| reg == _JIT_REG_USAGE_UNNUSED)
	{
		return;
	}

	if(range->preferred_colors == 0)
	{
		range->preferred_colors = jit_calloc(JIT_NUM_REGS, sizeof(jit_ushort));
	}
	++range->preferred_colors[reg];

	colors = (jit_ulong)1 << reg;
	if(reg_other != _JIT_REG_USAGE_UNNUSED)
	{
		assert(reg_other != _JIT_REG_USAGE_UNNAMED);
		colors |= (jit_ulong)1 << reg_other;
	}

	/* Create a dummy range which reserves the register in case @var{range} gets
	   colored with a different register */
	create_fixed_live_range(func, block, prev, insn, range->value, colors);
}

void
handle_constant_in_reg(jit_function_t func, jit_block_t block,
	jit_insn_t prev, jit_insn_t insn, jit_value_t value,
	int reg, int reg_other, _jit_live_range_t *out)
{
	int colors;

	if(reg == _JIT_REG_USAGE_UNNUSED || value == 0 || !value->is_constant)
	{
		return;
	}

	if(reg == _JIT_REG_USAGE_UNNAMED)
	{
		*out = create_dummy_live_range(func, block, prev, insn, value);
	}
	else
	{
		colors = (jit_ulong)1 << reg;
		if(reg_other != _JIT_REG_USAGE_UNNUSED)
		{
			assert(reg_other != _JIT_REG_USAGE_UNNAMED);
			colors |= (jit_ulong)1 << reg_other;
		}
		*out = create_fixed_live_range(func, block, prev, insn, value, colors);
	}
}

void
_jit_function_add_instruction_live_ranges(jit_function_t func)
{
	jit_block_t block;
	jit_insn_iter_t iter;
	jit_insn_t insn;
	jit_insn_t prev;
	_jit_regclass_t *regclass;
	int i;
	int j;
	int skip;
	jit_ulong colors;
	struct jit_insn_register_usage regmap;

	skip = 0;
	for(block = func->builder->entry_block; block; block = block->next)
	{
		jit_insn_iter_init(&iter, block);
		prev = 0;

		while((insn = jit_insn_iter_next(&iter)) != 0)
		{
			if(insn->opcode == JIT_OP_NOP)
			{
				prev = insn;
				continue;
			}

			/* Handle opcode specialities */
			switch(insn->opcode)
			{
			case JIT_OP_CALL:
			case JIT_OP_CALL_TAIL:
			case JIT_OP_CALL_INDIRECT:
			case JIT_OP_CALL_INDIRECT_TAIL:
			case JIT_OP_CALL_VTABLE_PTR:
			case JIT_OP_CALL_VTABLE_PTR_TAIL:
			case JIT_OP_CALL_EXTERNAL:
			case JIT_OP_CALL_EXTERNAL_TAIL:
				colors = 0;
				for(i = 0; i < JIT_NUM_REGS; i++)
				{
					if((jit_reg_flags(i) & JIT_REG_GLOBAL) == 0)
					{
						colors |= (jit_ulong)1 << i;
					}
				}
				create_fixed_live_range(func, block, 0, insn, 0, colors);
				break;

			case JIT_OP_INCOMING_REG:
			case JIT_OP_RETURN_REG:
				increment_preferred_color(func, block, prev, insn,
					insn->dest_live,
					(int)jit_value_get_nint_constant(insn->value1),
					_JIT_REG_USAGE_UNNUSED);
				skip = 1;
				break;

			case JIT_OP_OUTGOING_REG:
				increment_preferred_color(func, block, prev, insn,
					insn->value1_live,
					(int)jit_value_get_nint_constant(insn->value2),
					_JIT_REG_USAGE_UNNUSED);
				skip = 1;
				break;

			case JIT_OP_OUTGOING_FRAME_POSN:
			case JIT_OP_INCOMING_FRAME_POSN:
				skip = 1;
				break;
			}

			if(skip)
			{
				skip = 0;
				prev = insn;
				continue;
			}

			memset(&regmap, 0, sizeof(regmap));
			regmap.dest = _JIT_REG_USAGE_UNNUSED;
			regmap.value1 = _JIT_REG_USAGE_UNNUSED;
			regmap.value2 = _JIT_REG_USAGE_UNNUSED;
			regmap.dest_other = _JIT_REG_USAGE_UNNUSED;
			regmap.value1_other = _JIT_REG_USAGE_UNNUSED;
			regmap.value2_other = _JIT_REG_USAGE_UNNUSED;
			_jit_insn_get_register_usage(insn, &regmap);

			for(i = 0; i < JIT_NUM_REG_CLASSES; i++)
			{
				for(j = regmap.unnamed[i]; j > 0; j--)
				{
					create_scratch_live_range(func, block, prev, insn, i);
				}
			}

			/* create tiny live ranges for clobbered values */
			if(regmap.clobber != 0)
			{
				create_fixed_live_range(func, block, prev, insn,
					0, regmap.clobber);
			}
			if(regmap.early_clobber != 0)
			{
				create_fixed_live_range(func, block, prev, insn,
					0, regmap.early_clobber);
			}
			if(regmap.clobbered_classes != 0)
			{
				for(i = 0; i < JIT_NUM_REG_CLASSES; i++)
				{
					if((regmap.clobbered_classes & ((jit_ulong)1 << i)) == 0)
					{
						continue;
					}

					regclass = _jit_regclass_info[i];
					if(regclass == 0)
					{
						continue;
					}

					colors = 0;
					for(j = 0; j < regclass->num_regs; j++)
					{
						colors |= (jit_ulong)1 << regclass->regs[j];
					}
				}

				create_fixed_live_range(func, block, prev, insn,
					0, colors);
			}

			increment_preferred_color(func, block, prev, insn,
				insn->dest_live, regmap.dest, regmap.dest_other);
			increment_preferred_color(func, block, prev, insn,
				insn->value1_live, regmap.value1, regmap.value1_other);
			increment_preferred_color(func, block, prev, insn,
				insn->value2_live, regmap.value2, regmap.value2_other);

			handle_constant_in_reg(func, block, prev, insn, insn->dest,
				regmap.dest, regmap.dest_other, &insn->dest_live);
			handle_constant_in_reg(func, block, prev, insn, insn->value1,
				regmap.value1, regmap.value1_other, &insn->value1_live);
			handle_constant_in_reg(func, block, prev, insn, insn->value2,
				regmap.value2, regmap.value2_other, &insn->value2_live);

			insn->flags |= regmap.flags;

			prev = insn;
		}
	}

#ifdef _JIT_FLOW_DEBUG
	jit_dump_live_ranges(stdout, func);
#endif
}
