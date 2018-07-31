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

/* num_regs[get_type_flags(value)] returns the amount of registers available in
   in the architecture which can hold @var{value} */
static jit_ubyte num_regs[6] = {0};

/* interference_map[i][j] is 1 if two values with type index i and j can
   reside in the same register and thus interfere */
static jit_ubyte interference_map[6][6] = {0};

#ifdef _JIT_GRAPH_REGALLOC_DEBUG
#include <jit/jit-dump.h>

void dump_live_range(jit_function_t func, _jit_live_range_t range)
{
	int i;
	_jit_live_range_t curr;

	i = 0;
	for(curr = func->live_ranges; curr; curr = curr->func_next)
	{
		if(curr == range)
		{
			break;
		}
		++i;
	}
	printf("LiveRange(#%d, ", i);

	if(range->value)
	{
		jit_dump_value(stdout, func, range->value, NULL);
	}
	else
	{
		printf("XX");
	}
	printf(", %d)", range->neighbor_count);
}
#endif

static int
is_local(_jit_live_range_t range)
{
	return range->starts != 0
		&& range->ends != 0
		&& range->starts->next == 0
		&& range->ends->next == 0
		&& range->starts->block == range->ends->block;
}

static int
is_dummy(_jit_live_range_t range)
{
	return is_local(range)
		&& range->ends->insn - range->starts->insn <= 1;
}

/* Returns a type index used with num_regs and interference_map and can be
   turned into a JIT_REG_ constant using get_type_flag */
int get_type_index_from_value(jit_value_t value)
{
	if(!value)
	{
		return 0;
	}

	switch(jit_type_remove_tags(value->type)->kind)
	{
	case JIT_TYPE_SBYTE:
	case JIT_TYPE_UBYTE:
	case JIT_TYPE_SHORT:
	case JIT_TYPE_USHORT:
	case JIT_TYPE_INT:
	case JIT_TYPE_UINT:
	case JIT_TYPE_NINT:
	case JIT_TYPE_NUINT:
	case JIT_TYPE_SIGNATURE:
	case JIT_TYPE_PTR:
		return 1;
	case JIT_TYPE_LONG:
	case JIT_TYPE_ULONG:
		return 2;
	case JIT_TYPE_FLOAT32:
		return 3;
	case JIT_TYPE_FLOAT64:
		return 4;
	case JIT_TYPE_NFLOAT:
		return 5;
	default:
		return 0;
	}
}

int get_type_flag_from_index(int index)
{
	switch(index)
	{
	case 1:
		return JIT_REG_WORD;
	case 2:
		return JIT_REG_LONG;
	case 3:
		return JIT_REG_FLOAT32;
	case 4:
		return JIT_REG_FLOAT64;
	case 5:
		return JIT_REG_NFLOAT;
	default:
		return 0;
	}
}

int get_type_index_from_range(_jit_live_range_t range)
{
	int i;

	if(range->value)
	{
		return get_type_index_from_value(range->value);
	}
	else
	{
		for(i = 1; i < 5; i++)
		{
			if(range->regflags & get_type_flag_from_index(i))
			{
				return i;
			}
		}
		return 0;
	}
}

int get_type_flag_from_range(_jit_live_range_t range)
{
	if(range->value)
	{
		return get_type_flag_from_index(
			get_type_index_from_value(range->value));
	}
	else
	{
		return range->regflags;
	}
}

static int
do_dest_and_value2_interfere(jit_insn_t insn,
	_jit_live_range_t starting, _jit_live_range_t ending)
{
	return (insn->flags & JIT_INSN_DEST_INTERFERES_VALUE2)
		&& insn->dest_live == starting
		&& insn->value2_live == ending;
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

	if(touches_start && touches_end)
	{
		return 1;
	}
	if(touches_start)
	{
		insn = _jit_insn_list_get_insn_from_block(other->ends, block);
		if(insn != 0 && insn > local->starts->insn)
		{
			return 1;
		}
		if(insn == local->starts->insn
			&& do_dest_and_value2_interfere(insn, local, other))
		{
			return 1;
		}
	}
	if(touches_end)
	{
		insn = _jit_insn_list_get_insn_from_block(other->starts, block);
		if(insn != 0 && insn < local->ends->insn)
		{
			return 1;
		}
		if(insn == local->ends->insn
			&& do_dest_and_value2_interfere(insn, other, local))
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
	int a_index;
	int b_index;
	jit_block_t block;
	jit_insn_t start_a, start_b, end_a, end_b;

	if(a->value != 0 && a->value == b->value)
	{
		return 0;
	}

	if(a->value && b->value)
	{
		a_index = get_type_index_from_range(a);
		b_index = get_type_index_from_range(b);
		if(interference_map[a_index][b_index] == 0)
		{
			return 0;
		}
	}

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
		if(start_a >= start_b && start_a < end_b)
		{
			return 1;
		}
		if(start_b >= start_a && start_b < end_a)
		{
			return 1;
		}
		if(start_a == end_b
			&& do_dest_and_value2_interfere(start_a, a, b))
		{
			return 1;
		}
		if(start_b == end_a
			&& do_dest_and_value2_interfere(start_b, b, a))
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
				if(start_b < end_a)
				{
					return 1;
				}
			}
			else if(_jit_bitset_test_bit(&a->touched_block_ends, i)
				&& _jit_bitset_test_bit(&b->touched_block_starts, i))
			{
				start_a = _jit_insn_list_get_insn_from_block(a->starts, block);
				end_b = _jit_insn_list_get_insn_from_block(b->ends, block);
				if(start_a < end_b)
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

	for(a = func->live_ranges; a; a = a->func_next)
	{
		if(!_jit_bitset_is_allocated(&a->neighbors))
		{
			_jit_bitset_allocate(&a->neighbors, func->live_range_count);
		}
	}

#ifdef _JIT_GRAPH_REGALLOC_DEBUG
	printf("Interference graph:\n");
#endif

	i = 0;
	for(a = func->live_ranges; a; a = a->func_next)
	{
		j = i + 1;
		for(b = a->func_next; b; b = b->func_next)
		{
			if(check_interfering(func, a, b))
			{
				_jit_bitset_set_bit(&a->neighbors, j);
				_jit_bitset_set_bit(&b->neighbors, i);
				a->neighbor_count += b->register_count;
				b->neighbor_count += a->register_count;
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
	_jit_live_range_t curr)
{
	int i;
	for(i = 0; i < func->live_range_count; i++)
	{
		if(_jit_bitset_test_bit(&curr->neighbors, i))
		{
			ranges[i]->curr_neighbor_count -= curr->register_count;
		}
	}
}
int
_jit_regs_graph_simplify(jit_function_t func, _jit_live_range_t *ranges,
	_jit_live_range_t *stack)
{
	_jit_live_range_t curr;
	_jit_live_range_t spill_candidate;
	int type;
	int pos;

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
		for(curr = func->live_ranges; curr; curr = curr->func_next)
		{
			type = get_type_index_from_range(curr);
			if(!curr->on_stack && !curr->is_spilled && !curr->is_fixed
				&& curr->curr_neighbor_count < num_regs[type])
			{
				curr->on_stack = 1;
				stack[pos] = curr;
				decrement_neighbor_count(func, ranges, curr);
				break;
			}
		}

		if(curr)
		{
			/* We sucessfully added a live range to the stack */
			continue;
		}

		/* We did not find any live range with less then JIT_NUM_REGS
		   neighbors. We optimistically push one to the stack
		   TODO: base this decision on spill cost */
		spill_candidate = 0;
		for(curr = func->live_ranges; curr; curr = curr->func_next)
		{
			if(!curr->on_stack && !curr->is_spilled && !curr->is_fixed)
			{
				spill_candidate = curr;
				/* We prefer to optimistially push non-dummy live ranges as
				   spilling dummy ranges usually doesn't help the coloring
				   problem */
				if(!is_dummy(curr) && curr->value && !curr->value->is_constant)
				{
					break;
				}
			}
		}

		if(!spill_candidate)
		{
			/* everything is fixed, on stack or spilled already */
			break;
		}

#ifdef _JIT_GRAPH_REGALLOC_DEBUG
		printf("Optimistically pushing ");
		dump_live_range(func, spill_candidate);
		printf("\n");
#endif
		spill_candidate->on_stack = 1;
		stack[pos] = spill_candidate;
		decrement_neighbor_count(func, ranges, spill_candidate);
	}

	/* Put dummy live ranges on the stack last, ensuring they are colored first
	   and not spilled (again). */
	/*for(curr = func->live_ranges; curr; curr = curr->func_next)
	{
		if(is_dummy(curr))
		{
			curr->on_stack = 1;
			stack[pos] = curr;
			decrement_neighbor_count(func, ranges, curr);
			++pos;
		}
	}*/

	return pos - 1;
}

_jit_live_range_t
spill_live_range_in_insn(jit_function_t func, jit_block_t block,
	jit_insn_t prev, jit_insn_t insn, _jit_live_range_t range)
{
	_jit_live_range_t curr;
	_jit_live_range_t dummy;
	int i;
	int j;

	i = func->live_range_count;
	dummy = _jit_function_create_live_range(func, range->value);
	dummy->is_spill_range = 1;
	_jit_bitset_allocate(&dummy->neighbors, func->live_range_count * 3 / 2);

	_jit_insn_list_add(&dummy->ends, block, insn);
	if(prev == 0)
	{
		_jit_insn_list_add(&dummy->ends, block, insn);
	}
	else
	{
		_jit_insn_list_add(&dummy->starts, block, prev);
	}

	j = 0;
	for(curr = func->live_ranges; curr; curr = curr->func_next)
	{
		if(_jit_bitset_size(&curr->neighbors) < func->live_range_count)
		{
			_jit_bitset_resize(&curr->neighbors,
				func->live_range_count * 3 / 2);
		}

		if(dummy != curr && check_interfering(func, dummy, curr))
		{
			_jit_bitset_set_bit(&curr->neighbors, i);
			_jit_bitset_set_bit(&dummy->neighbors, j);
			curr->neighbor_count += dummy->register_count;
			dummy->neighbor_count += curr->register_count;
		}

		++j;
	}

#ifdef _JIT_GRAPH_REGALLOC_DEBUG
	printf("    - ");
	dump_live_range(func, dummy);
	printf("\n");
#endif
	return dummy;
}
void
spill_live_range_in_block(jit_function_t func, jit_block_t block,
	_jit_live_range_t range)
{
	jit_insn_iter_t iter;
	jit_insn_t prev;
	jit_insn_t insn;

	prev = 0;
	jit_insn_iter_init(&iter, block);
	while((insn = jit_insn_iter_next(&iter)) != NULL)
	{
		if(insn->dest_live == range)
		{
			if(insn->flags & JIT_INSN_DEST_CAN_BE_MEM)
			{
				insn->dest_live = 0;
			}
			else
			{
				insn->dest_live = spill_live_range_in_insn(func, block,
					prev, insn, range);
			}
		}
		else if(insn->value1_live == range)
		{
			if(insn->flags & JIT_INSN_VALUE1_CAN_BE_MEM)
			{
				insn->value1_live = 0;
			}
			else
			{
				insn->value1_live = spill_live_range_in_insn(func, block,
					prev, insn, range);
			}
		}
		else if(insn->value2_live == range)
		{
			if(insn->flags & JIT_INSN_VALUE2_CAN_BE_MEM)
			{
				insn->value2_live = 0;
			}
			else
			{
				insn->value2_live = spill_live_range_in_insn(func, block,
					prev, insn, range);
			}
		}

		prev = insn;
	}
}
void
spill_live_range(jit_function_t func, _jit_live_range_t *ranges,
	_jit_live_range_t range)
{
	jit_block_t block;
	int index;
	int i;

#ifdef _JIT_GRAPH_REGALLOC_DEBUG
	printf("Spilling ");
	dump_live_range(func, range);
	printf(" and creating:\n");
#endif

	for(i = 0; i < func->live_range_count; i++)
	{
		if(_jit_bitset_test_bit(&range->neighbors, i))
		{
			ranges[i]->neighbor_count -= range->register_count;
		}
	}

	if(is_local(range))
	{
		spill_live_range_in_block(func, range->starts->block, range);
	}
	else
	{
		for(i = 0; i < func->builder->num_block_order; i++)
		{
			block = func->builder->block_order[i];
			index = block->index;
			if(_jit_bitset_test_bit(&range->touched_block_starts, index)
				|| _jit_bitset_test_bit(&range->touched_block_ends, index))
			{
				spill_live_range_in_block(func, block, range);
			}
		}
	}

	range->is_spilled = 1;
}

int
_jit_regs_graph_select(jit_function_t func, _jit_live_range_t *ranges,
	_jit_live_range_t *stack, int pos)
{
	_jit_live_range_t curr;
	jit_nuint used;
	int preferred;
	int preferred_score;
	int preferred_is_global;
	int is_global;
	int flags;
	int type;
	int i;

	for(; pos >= 0; pos--)
	{
		curr = stack[pos];
		used = 0;

		for(i = 0; i < func->live_range_count; i++)
		{
			if(_jit_bitset_test_bit(&curr->neighbors, i)
				&& !ranges[i]->is_spilled)
			{
				used |= ranges[i]->colors;
			}
		}

		type = get_type_flag_from_range(curr);
		preferred = -1;
		preferred_score = -1;
		preferred_is_global = 0;
		for(i = 0; i < JIT_NUM_REGS; i++)
		{
			flags = jit_reg_flags(i);
			is_global = (flags & JIT_REG_GLOBAL) != 0;
			if(((1 << i) & used) == 0
				&& (flags & type) != 0
				&& (flags & JIT_REG_FIXED) == 0
				&& (curr->preferred_colors == 0
					|| curr->preferred_colors[i] >= preferred_score)
				&& (preferred == -1
					|| (curr->preferred_colors != 0
						&& curr->preferred_colors[i] > preferred_score)
					|| (preferred_is_global && !is_global)))
			{
				preferred = i;
				preferred_is_global = is_global;
				if(curr->preferred_colors)
				{
					preferred_score = curr->preferred_colors[i];
				}
			}
		}

		if(preferred == -1)
		{
			spill_live_range(func, ranges, curr);
			return 0;
		}
		curr->colors = 1 << preferred;
	}

	return 1;
}

/* This function initializes @var{interference_map} and @var{num_regs} */
void initialize_type_interference_and_num_regs()
{
	int flags;
	int reg;
	int i;
	int j;

	for(reg = 0; reg < JIT_NUM_REGS; reg++)
	{
		flags = jit_reg_flags(reg);

		for(i = 0; i < 6; i++)
		{
			if(flags & get_type_flag_from_index(i))
			{
				++num_regs[i];

				for(j = 0; j < 6; j++)
				{
					if(flags & get_type_flag_from_index(j))
					{
						interference_map[i][j] = 1;
					}
				}
			}
		}
	}
}
void
_jit_regs_graph_compute_coloring(jit_function_t func)
{
	_jit_live_range_t curr;
	_jit_live_range_t *stack;
	_jit_live_range_t *ranges;
	int pos;
	int i;	
#ifdef _JIT_GRAPH_REGALLOC_DEBUG
	int spill_count;
	spill_count = -1;
#endif

	if(num_regs[1] == 0)
	{
		initialize_type_interference_and_num_regs();
	}

	i = 0;
	stack = 0;
	ranges = 0;
	curr = func->live_ranges;

	_jit_regs_graph_build(func);
	_jit_regs_graph_coalesce(func);

	do
	{
		if(i < func->live_range_count)
		{
			jit_free(stack);
			stack = jit_malloc(func->live_range_count *
				sizeof(_jit_live_range_t));
			ranges = jit_realloc(ranges, func->live_range_count *
				sizeof(_jit_live_range_t));

			if(i != 0)
			{
				curr = curr->func_next;
			}
			for(; i < func->live_range_count; i++)
			{
				ranges[i] = curr;

				if(curr->func_next)
				{
					curr = curr->func_next;
				}
			}
		}
		
#ifdef _JIT_GRAPH_REGALLOC_DEBUG
		++spill_count;
#endif


		pos = _jit_regs_graph_simplify(func, ranges, stack);
	} while(!_jit_regs_graph_select(func, ranges, stack, pos));

	jit_free(stack);
	jit_free(ranges);

	func->registers_graph_allocated = 1;

#ifdef _JIT_GRAPH_REGALLOC_DEBUG
	printf("Register allocation finished after %d spills\n", spill_count);
	printf("Registers:\n");

	_jit_dump_live_ranges(func);
#endif
}

int find_reg_in_colors(jit_ulong colors)
{
	int i;
	for(i = 0; i < JIT_NUM_REGS; i++)
	{
		if(colors & (1 << i))
		{
			return i;
		}
	}

	return -1;
}

/* Initialize in_register and reg fields of each value */
void
_jit_regs_graph_init_for_block(jit_gencode_t gen, jit_function_t func,
	jit_block_t block)
{
	_jit_live_range_t curr;
	jit_value_t value;
	int i;
	jit_pool_block_t memblock = func->builder->value_pool.blocks;
	int num = (int)(func->builder->value_pool.elems_per_block);

	while(memblock != 0)
	{
		if(!(memblock->next))
		{
			num = (int)(func->builder->value_pool.elems_in_last);
		}

		for(i = 0; i < num; ++i)
		{
			value = (jit_value_t)(memblock->data + i * sizeof(struct _jit_value));
			value->in_register = 0;
			value->in_global_register = 0;

			if(!value->is_constant)
			{
				for(curr = value->live_ranges; curr; curr = curr->value_next)
				{
					if(_jit_bitset_test_bit(&curr->touched_block_starts,
						block->index)
						&& !curr->is_spilled)
					{
						value->in_register = 1;
						value->reg = find_reg_in_colors(curr->colors);
						break;
					}
				}
			}
		}
		memblock = memblock->next;
	}
}

void
init_value_for_insn(jit_gencode_t gen, jit_insn_t insn,
	short mask, jit_value_t value, _jit_live_range_t range)
{
	if((insn->flags & mask) != 0 || value == 0 || value->is_constant)
	{
		return;
	}

	if(range != 0 && !range->is_spill_range)
	{
		value->in_register = 1;
		value->reg = find_reg_in_colors(range->colors);
		jit_reg_set_used(gen->touched, value->reg);
	}
	else
	{
		value->in_register = 0;
	}
}
/* Set in_register and reg fields for values used in @var{insn} */
void
_jit_regs_graph_init_for_insn(jit_gencode_t gen, jit_function_t func,
	jit_insn_t insn)
{
	if(insn->opcode == JIT_OP_NOP)
	{
		return;
	}

	init_value_for_insn(gen, insn, JIT_INSN_DEST_OTHER_FLAGS,
		insn->dest, insn->dest_live);
	init_value_for_insn(gen, insn, JIT_INSN_VALUE1_OTHER_FLAGS,
		insn->value1, insn->value1_live);
	init_value_for_insn(gen, insn, JIT_INSN_VALUE2_OTHER_FLAGS,
		insn->value2, insn->value2_live);
}

int
same_reg(_jit_regs_t *regs, int a, int b)
{
	return regs->descs[a].value && regs->descs[b].value
		&& regs->descs[a].reg == regs->descs[b].reg
		&& regs->descs[a].other_reg == regs->descs[b].other_reg;
}
void
begin_value(jit_gencode_t gen, _jit_regs_t *regs, jit_insn_t insn,
	int i, jit_value_t value, _jit_live_range_t range)
{
	static const int other_masks[] = {
		JIT_INSN_DEST_OTHER_FLAGS,
		JIT_INSN_VALUE1_OTHER_FLAGS,
		JIT_INSN_VALUE2_OTHER_FLAGS
	};
	static const int mem_masks[] = {
		JIT_INSN_DEST_CAN_BE_MEM,
		JIT_INSN_VALUE1_CAN_BE_MEM,
		JIT_INSN_VALUE2_CAN_BE_MEM
	};

	if((insn->flags & other_masks[i]) != 0
		|| value == 0
		|| regs->descs[i].value == 0
		|| get_type_index_from_value(value) == 0)
	{
		return;
	}

	if(value->is_constant)
	{
		/* The value is a constant but required in a register */
		if(regs->descs[i].reg == -1)
		{
			assert(range != 0);
			regs->descs[i].reg = find_reg_in_colors(range->colors);
			/* TODO other_reg */
		}
		_jit_gen_load_value(gen, regs->descs[i].reg,
			regs->descs[i].other_reg, value);
	}
	else if(!value->in_register)
	{
		if((insn->flags & mem_masks[i]) != 0)
		{
			/* The value was spilled but the instruction supports it to be in
			   memory. No need to load it */
		}
		else
		{
			/* The range was spilled, we need to load the value */
			assert(range != 0 && range->is_spill_range);

			if(regs->descs[i].reg == -1)
			{
				regs->descs[i].reg = find_reg_in_colors(range->colors);
				/* TODO other_reg */
			}

			/* If the value is a destination of @var{insn} we do not need to
			   load it, but rather store it afterwords */
			if(i != 0 || (insn->flags & JIT_INSN_DEST_IS_VALUE) != 0)
			{
				_jit_gen_load_value(gen, regs->descs[i].reg,
					regs->descs[i].other_reg, value);
			}
		}
	}
	else
	{
		/* The value is already in a register */
		if(regs->descs[i].reg == -1)
		{
			regs->descs[i].reg = value->reg;
		}
		else if(regs->descs[i].reg != value->reg)
		{
			_jit_gen_load_value(gen, regs->descs[i].reg,
				regs->descs[i].other_reg, value);
		}
	}
}
void
_jit_regs_graph_begin(jit_gencode_t gen, _jit_regs_t *regs, jit_insn_t insn)
{
	_jit_live_range_t curr;
	int reg;
	int i;
	int ignore;

	ignore = -1;
	switch(insn->opcode)
	{
	case JIT_OP_INCOMING_REG:
	case JIT_OP_INCOMING_FRAME_POSN:
	case JIT_OP_RETURN_REG:
		ignore = 1;
		break;

	case JIT_OP_OUTGOING_REG:
	case JIT_OP_OUTGOING_FRAME_POSN:
		ignore = 2;
		break;
	}

	begin_value(gen, regs, insn, 0, insn->dest, insn->dest_live);

	if(ignore != 1)
	{
		begin_value(gen, regs, insn, 1, insn->value1, insn->value1_live);
	}
	if(ignore != 2)
	{
		begin_value(gen, regs, insn, 2, insn->value2, insn->value2_live);
	}

	if(!regs->ternary && !regs->free_dest)
	{
		/* The instruction outputs to regs->descs[1] */

		if(same_reg(regs, 0, 1))
		{
			/* The values are already in the correct registers */
		}
		else if(same_reg(regs, 0, 2))
		{
			assert(regs->commutative);

			reg = regs->descs[1].reg;
			regs->descs[1].reg = regs->descs[2].reg;
			regs->descs[2].reg = reg;

			reg = regs->descs[1].other_reg;
			regs->descs[1].other_reg = regs->descs[2].other_reg;
			regs->descs[2].other_reg = reg;
		}
		else if(regs->descs[0].value && regs->descs[1].value)
		{
			_jit_gen_load_value(gen, regs->descs[0].reg,
				regs->descs[0].other_reg, insn->value1);

			regs->descs[1].reg = regs->descs[0].reg;
			regs->descs[1].other_reg = regs->descs[0].other_reg;
		}
	}

	curr = insn->scratch_live;
	for(i = regs->num_scratch - 1; i >= 0; i--)
	{
		reg = find_reg_in_colors(curr->colors);
		assert(regs->scratch[i].reg == -1 || regs->scratch[i].reg == reg);
		regs->scratch[i].reg = reg;

		curr = curr->value_next;
	}
}

void
_jit_regs_graph_commit(jit_gencode_t gen, _jit_regs_t *regs, jit_insn_t insn)
{
	if((insn->flags & JIT_INSN_DEST_OTHER_FLAGS) == 0
		&& (insn->flags & JIT_INSN_DEST_IS_VALUE) == 0
		&& insn->dest != 0
		&& insn->dest_live != 0
		&& insn->dest_live->is_spill_range)
	{
		/* The instruction wrote to dest, which is a spilled value. We need to
		   save it from it's temporary register back to memory */
		_jit_gen_spill_reg(gen, regs->descs[0].reg, regs->descs[0].other_reg,
			insn->dest);
	}
}

void
_jit_regs_graph_set_incoming(jit_gencode_t gen, int reg, jit_value_t value)
{
	int tmp;

	if(!value->in_register)
	{
		_jit_gen_spill_reg(gen, reg, -1, value);
	}
	else if(value->reg != reg)
	{
		tmp = value->reg;
		value->reg = reg;
		_jit_gen_load_value(gen, tmp, -1, value);
		value->reg = tmp;
	}
}

void
_jit_regs_graph_set_outgoing(jit_gencode_t gen, int reg, jit_value_t value)
{
	if(!value->in_register || value->reg != reg)
	{
		_jit_gen_load_value(gen, reg, -1, value);
	}
}
