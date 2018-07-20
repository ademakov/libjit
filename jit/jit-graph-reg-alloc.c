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
#include "jit-reg-alloc.h"
#include <assert.h>

#ifdef _JIT_GRAPH_REGALLOC_DEBUG
#include <jit/jit-dump.h>
#endif

static int
is_local(_jit_live_range_t range)
{
	return range->starts != NULL
		&& range->ends != NULL
		&& range->starts->next == NULL
		&& range->ends->next == NULL
		&& range->starts->block == range->ends->block;
}

static int
does_local_range_interfere_with(_jit_live_range_t local,
	_jit_live_range_t other)
{
	int touches_start;
	int touches_end;
	jit_block_t block;
	jit_insn_t insn;

	block = local->starts->block;
	touches_start = _jit_bitset_test_bit(&other->touched_block_starts,
		block->index);
	touches_end = _jit_bitset_test_bit(&other->touched_block_ends,
		block->index);

	if(touches_start)
	{
		insn = _jit_insn_list_get_insn_from_block(other->ends, block);
		if(insn != 0 && insn->index > local->starts->insn->index)
		{
			return 1;
		}
	}
	if(touches_end)
	{
		insn = _jit_insn_list_get_insn_from_block(other->starts, block);
		if(insn != 0 && insn->index < local->ends->insn->index)
		{
			return 1;
		}
	}
	
	return 0;
}

static int
check_interfering(jit_function_t func,
	_jit_live_range_t a, _jit_live_range_t b)
{
	int i;
	int a_is_local;
	int b_is_local;
	jit_block_t block;
	jit_insn_t start_a, start_b, end_a, end_b;

	if(_jit_bitset_test(&a->touched_block_starts, &b->touched_block_starts)
		|| _jit_bitset_test(&a->touched_block_ends, &b->touched_block_ends))
	{
		return 1;
	}

	a_is_local = is_local(a);
	b_is_local = is_local(b);
	if(a_is_local && b_is_local)
	{
		block = a->starts->block;
		if(block != b->starts->block)
		{
			return 0;
		}

		start_a = a->starts->insn;
		start_b = b->starts->insn;
		end_a = a->ends->insn;
		end_b = b->ends->insn;
		if(start_a->index >= start_b->index && start_a->index < end_b->index)
		{
			return 1;
		}
		if(start_b->index >= start_a->index && start_b->index < end_a->index)
		{
			return 1;
		}

		return 0;
	}
	else if(a_is_local)
	{
		return does_local_range_interfere_with(a, b);
	}
	else if(b_is_local)
	{
		return does_local_range_interfere_with(b, a);
	}
	else
	{
		i = 0;
		for(block = func->builder->entry_block; block; block = block->next)
		{
			if(_jit_bitset_test_bit(&a->touched_block_starts, i)
				&& _jit_bitset_test_bit(&b->touched_block_ends, i))
			{
				end_a = _jit_insn_list_get_insn_from_block(a->ends, block);
				start_b = _jit_insn_list_get_insn_from_block(b->starts, block);
				if(start_b->index < end_a->index)
				{
					return 1;
				}
			}
			else if(_jit_bitset_test_bit(&a->touched_block_ends, i)
				&& _jit_bitset_test_bit(&b->touched_block_starts, i))
			{
				start_a = _jit_insn_list_get_insn_from_block(a->starts, block);
				end_b = _jit_insn_list_get_insn_from_block(b->ends, block);
				if(start_a->index < end_b->index)
				{
					return 1;
				}
			}

			++i;
		}

		return 0;
	}
}

void
_jit_regs_graph_build(jit_function_t func)
{
	_jit_live_range_t a;
	_jit_live_range_t b;
	int i;
	int j;

#ifdef _JIT_GRAPH_REGALLOC_DEBUG
	printf("Interference graph:\n");
#endif

	i = 0;
	for(a = func->live_ranges; a; a = a->func_next)
	{
		if(!_jit_bitset_is_allocated(&a->neighbors))
		{
			_jit_bitset_allocate(&a->neighbors, func->live_range_count);
		}

		j = i + 1;
		for(b = a->func_next; b; b = b->func_next)
		{
			if(!_jit_bitset_is_allocated(&b->neighbors))
			{
				_jit_bitset_allocate(&b->neighbors, func->live_range_count);
			}

			if(check_interfering(func, a, b))
			{
				_jit_bitset_set_bit(&a->neighbors, j);
				_jit_bitset_set_bit(&b->neighbors, i);
				++a->neighbor_count;
				++b->neighbor_count;

#ifdef _JIT_GRAPH_REGALLOC_DEBUG
				printf("    LiveRange(#%d, ", i);
				if(a->value == NULL)
				{
					printf("XX");
				}
				else
				{
					jit_dump_value(stdout, func, a->value, NULL);
				}
				printf(") <-> LiveRange(#%d, ", j);
				if(b->value == NULL)
				{
					printf("XX");
				}
				else
				{
					jit_dump_value(stdout, func, b->value, NULL);
				}
				printf(")\n");
#endif
			}

			++j;
		}

		++i;
	}

#ifdef _JIT_GRAPH_REGALLOC_DEBUG
	printf("\n");
#endif
}

void
_jit_regs_graph_coalesce(jit_function_t func)
{
	/* TODO */
}

static void
decrement_neighbor_count(jit_function_t func, _jit_live_range_t *ranges,
	_jit_live_range_t curr, int i)
{
	int j;
	for(j = 0; j < func->live_range_count; j++)
	{
		if(_jit_bitset_test_bit(&curr->neighbors, j))
		{
			--ranges[i]->curr_neighbor_count;
		}
	}
}
void
_jit_regs_graph_simplify(jit_function_t func, _jit_live_range_t *ranges,
	_jit_live_range_t *stack)
{
	_jit_live_range_t curr;
	int pos;
	int i;

	for(curr = func->live_ranges; curr; curr = curr->func_next)
	{
		curr->on_stack = 0;
		curr->curr_neighbor_count = curr->neighbor_count;

		if(!curr->is_fixed)
		{
			curr->colors = 0;
		}
	}

	for(pos = 0; pos < func->live_range_count; pos++)
	{
		i = 0;
		for(curr = func->live_ranges; curr; curr = curr->func_next)
		{
			if(!curr->on_stack && curr->curr_neighbor_count < JIT_NUM_REGS)
			{
				curr->on_stack = 1;
				stack[pos] = curr;
				decrement_neighbor_count(func, ranges, curr, i);
				break;
			}

			++i;
		}

		if(i == func->live_range_count)
		{
			/* We did not find any live range with less then JIT_NUM_REGS
			   neighbors. We optimistically push one to the stack
			   TODO: base this decision on spill cost */
			i = 0;
			for(curr = func->live_ranges; curr; curr = curr->func_next)
			{
				if(!curr->on_stack)
				{
					curr->on_stack = 1;
					stack[pos] = curr;
					decrement_neighbor_count(func, ranges, curr, i);
					break;
				}

				++i;
			}
		}
	}
}

int
_jit_regs_graph_select(jit_function_t func, _jit_live_range_t *ranges,
	_jit_live_range_t *stack)
{
	_jit_live_range_t curr;
	jit_nuint used;
	jit_ushort preferred_score;
	int preferred;
	int pos;
	int i;

#ifdef _JIT_GRAPH_REGALLOC_DEBUG
	printf("Registers:\n");
#endif

	for(pos = func->live_range_count - 1; pos >= 0; pos--)
	{
		curr = stack[pos];
		used = 0;

		for(i = 0; i < func->live_range_count; i++)
		{
			if(_jit_bitset_test_bit(&curr->neighbors, i))
			{
				used |= ranges[i]->colors;
			}
		}

		if(curr->is_fixed)
		{
			if((curr->colors & used) != 0)
			{
				/* TODO spill @var{curr} */
				assert(0);
				return 0;
			}
		}
		else
		{
			preferred = -1;
			preferred_score = 0;
			for(i = 0; i < JIT_NUM_REGS; i++)
			{
				if((curr->preferred_colors == 0
					|| curr->preferred_colors[i] >= preferred_score)
					&& ((1 << i) & used) == 0)
				{
					preferred = i;
					if(curr->preferred_colors)
					{
						preferred_score = curr->preferred_colors[i];
					}
					else
					{
						break;
					}
				}
			}

			if(preferred == -1)
			{
				/* TODO spill @var{curr} */
				assert(0);
				return 0;
			}
			curr->colors = 1 << preferred;
		}

#ifdef _JIT_GRAPH_REGALLOC_DEBUG
		for(i = 0; i < func->live_range_count; i++)
		{
			if(ranges[i] == curr)
			{
				break;
			}
		}
		printf("    LiveRange(#%d, ", i);

		if(curr->value)
		{
			jit_dump_value(stdout, func, curr->value, NULL);
		}
		else
		{
			printf("XX");
		}
		printf("): ");

		for(i = 0; i < JIT_NUM_REGS; i++)
		{
			if(curr->colors & ((jit_nuint)1 << i))
			{
				printf("%%%s, ", jit_reg_name(i));
			}
		}
		printf("\n");
#endif
	}

#ifdef _JIT_GRAPH_REGALLOC_DEBUG
	printf("\n");
#endif
	return 1;
}

void
_jit_regs_graph_compute_coloring(jit_function_t func)
{
	_jit_live_range_t curr;
	_jit_live_range_t *stack;
	_jit_live_range_t *ranges;
	int i;

	stack = jit_malloc(func->live_range_count * 
		sizeof(_jit_live_range_t));

	ranges = jit_malloc(func->live_range_count * 
		sizeof(_jit_live_range_t));

	curr = func->live_ranges;
	for(i = 0; i < func->live_range_count; i++)
	{
		ranges[i] = curr;
		curr = curr->func_next;
	}

	_jit_regs_graph_build(func);
	_jit_regs_graph_coalesce(func);

	do
	{
		_jit_regs_graph_simplify(func, ranges, stack);
	} while(!_jit_regs_graph_select(func, ranges, stack));
}
