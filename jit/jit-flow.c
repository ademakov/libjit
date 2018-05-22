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

static int
value_list_has(_jit_value_list_t list, jit_value_t value)
{
	for(; list; list = list->next)
	{
		if(list->value == value)
			return 1;
	}

	return 0;
}

static int
value_list_add(_jit_value_list_t *list, jit_value_t value)
{
	if(value_list_has(*list, value))
		return 0;

	_jit_value_list_t entry = jit_malloc(sizeof(_jit_value_list_t));
	entry->value = value;
	entry->next = *list;
	*list = entry;
	return 1;
}

static void
value_list_free(_jit_value_list_t list)
{
	_jit_value_list_t next;

	while(list)
	{
		next = list->next;
		jit_free(list);
		list = next;
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
			if(!value_list_has(block->var_kills, insn->value1))
			{
				value_list_add(&block->upward_exposes, insn->value1);
			}
		}

		/* If value2 is a value not in VarKill add it to UEVar */
		if((flags & JIT_INSN_VALUE2_OTHER_FLAGS) == 0 && insn->value2
			&& !insn->value2->is_constant && insn->value2->is_local)
		{
			if(!value_list_has(block->var_kills, insn->value2))
			{
				value_list_add(&block->upward_exposes, insn->value2);
			}
		}

		/* If dest is a value add it to VarKill */
		if((flags & JIT_INSN_DEST_OTHER_FLAGS) == 0 && insn->dest
			&& !insn->dest->is_constant && insn->dest->is_local)
		{
			value_list_add(&block->var_kills, insn->dest);
		}
	}
}

static int
compute_live_out(jit_block_t block)
{
	int i;
	int changed = 0;
	jit_block_t succ;
	_jit_value_list_t curr;

	for(i = 0; i < block->num_succs; i++)
	{
		succ = block->succs[i]->dst;

		for(curr = succ->upward_exposes; curr; curr = curr->next)
		{
			if(value_list_add(&block->live_out, curr->value))
			{
				changed = 1;
			}
		}

		for(curr = succ->live_out; curr; curr = curr->next)
		{
			if(!value_list_has(succ->var_kills, curr->value)
				&& value_list_add(&block->live_out, curr->value))
			{
				changed = 1;
			}
		}
	}

	return changed;
}

void
_jit_function_compute_live_out(jit_function_t func)
{
	jit_block_t block;
	int i;
	int changed = 1;

	for(i = 0; i < func->builder->num_block_order; i++)
	{
		block = func->builder->block_order[i];
		block->has_live_out = 1;

		compute_kills_and_upward_exposes(block);
	}

	while(changed)
	{
		changed = 0;
		for(i = 0; i < func->builder->num_block_order; i++)
		{
			block = func->builder->block_order[i];
			
			if(compute_live_out(block))
				changed = 1;
		}
	}
}

void
_jit_block_free_live_out(jit_block_t block)
{
	value_list_free(block->upward_exposes);
	value_list_free(block->var_kills);
	value_list_free(block->live_out);
}
