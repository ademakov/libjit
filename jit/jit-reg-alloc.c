/*
 * jit-reg-alloc.c - Register allocation routines for the JIT.
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
#include "jit-reg-alloc.h"
#include <jit/jit-dump.h>
#include <stdio.h>

/*@

The @code{libjit} library provides a number of functions for
performing register allocation within basic blocks so that you
mostly don't have to worry about it:

@*/

/*@
 * @deftypefun void _jit_regs_init_for_block (jit_gencode_t gen)
 * Initialize the register allocation state for a new block.
 * @end deftypefun
@*/
void _jit_regs_init_for_block(jit_gencode_t gen)
{
	int reg;
	gen->current_age = 1;
	for(reg = 0; reg < JIT_NUM_REGS; ++reg)
	{
		/* Clear everything except permanent and fixed registers */
		if(!jit_reg_is_used(gen->permanent, reg) &&
		   (_jit_reg_info[reg].flags & JIT_REG_FIXED) == 0)
		{
			gen->contents[reg].num_values = 0;
			gen->contents[reg].is_long_start = 0;
			gen->contents[reg].is_long_end = 0;
			gen->contents[reg].age = 0;
			gen->contents[reg].remap = -1;
			gen->contents[reg].used_for_temp = 0;
		}
		gen->stack_map[reg] = -1;
	}
	gen->inhibit = jit_regused_init;
}

/*@
 * @deftypefun int _jit_regs_needs_long_pair (jit_type_t type)
 * Determine if a type requires a long register pair.
 * @end deftypefun
@*/
int _jit_regs_needs_long_pair(jit_type_t type)
{
#if defined(JIT_NATIVE_INT32) && !defined(JIT_BACKEND_INTERP)
	type = jit_type_normalize(type);
	if(type)
	{
		if(type->kind == JIT_TYPE_LONG || type->kind == JIT_TYPE_ULONG)
		{
			return 1;
		}
	}
	return 0;
#else
	/* We don't register pairs on 64-bit platforms or the interpreter */
	return 0;
#endif
}

/*@
 * @deftypefun int _jit_regs_get_cpu (jit_gencode_t gen, int reg, int *other_reg)
 * Get the CPU register that corresponds to a pseudo register.
 * "other_reg" will be set to the other register in a pair,
 * or -1 if the register is not part of a pair.
 * @end deftypefun
@*/
int _jit_regs_get_cpu(jit_gencode_t gen, int reg, int *other_reg)
{
	int cpu_reg, other;
	cpu_reg = gen->contents[reg].remap;
	if(cpu_reg == -1)
	{
		cpu_reg = _jit_reg_info[reg].cpu_reg;
	}
	else
	{
		cpu_reg = _jit_reg_info[cpu_reg].cpu_reg;
	}
	if(gen->contents[reg].is_long_start)
	{
		other = _jit_reg_info[reg].other_reg;
		if(gen->contents[other].remap == -1)
		{
			other = _jit_reg_info[other].cpu_reg;
		}
		else
		{
			other = _jit_reg_info[gen->contents[other].remap].cpu_reg;
		}
	}
	else
	{
		other = -1;
	}
	if(other_reg)
	{
		*other_reg = other;
	}
	return cpu_reg;
}

/*
 * Dump debug information about the register allocation state.
 */
/*#define	JIT_REG_DEBUG	1*/
#ifdef JIT_REG_DEBUG
static void dump_regs(jit_gencode_t gen, const char *name)
{
	int reg;
	unsigned int index;
	printf("%s:\n", name);
	for(reg = 0; reg < JIT_NUM_REGS; ++reg)
	{
		if(gen->contents[reg].num_values == 0 &&
		   !(gen->contents[reg].used_for_temp) &&
		   gen->contents[reg].remap == -1)
		{
			continue;
		}
		printf("\t%s: ", _jit_reg_info[reg].name);
		if(gen->contents[reg].num_values > 0)
		{
			for(index = 0; index < gen->contents[reg].num_values; ++index)
			{
				if(index)
					fputs(", ", stdout);
				jit_dump_value(stdout, jit_value_get_function
										(gen->contents[reg].values[index]),
							   gen->contents[reg].values[index], 0);
			}
			if(gen->contents[reg].used_for_temp)
			{
				printf(", used_for_temp");
			}
		}
		else if(gen->contents[reg].used_for_temp)
		{
			printf("used_for_temp");
		}
		else
		{
			printf("free");
		}
		if(gen->contents[reg].remap != -1)
		{
			printf(", remap=%d", (int)(gen->contents[reg].remap));
		}
		for(index = 0; index < JIT_NUM_REGS; ++index)
		{
			if(gen->stack_map[index] == reg)
			{
				printf(", reverse_remap=%d", (int)index);
			}
		}
		putc('\n', stdout);
	}
}
#endif

/*
 * Spill all registers between two end points.
 */
static void spill_all_between(jit_gencode_t gen, int first, int last)
{
	int reg, posn, other_reg, real_reg;
	int first_stack_reg = 0;
	jit_value_t value;
	int value_used;

#ifdef JIT_REG_DEBUG
	dump_regs(gen, "enter spill_all_between");
#endif

	/* Handle the non-stack registers first, as they are easy to spill */
	for(reg = first; reg <= last; ++reg)
	{
		/* Skip this register if it is permanent or fixed */
		if(jit_reg_is_used(gen->permanent, reg) ||
		   (_jit_reg_info[reg].flags & JIT_REG_FIXED) != 0)
		{
			continue;
		}

		/* Remember this register if it is the start of a stack */
		if((_jit_reg_info[reg].flags & JIT_REG_START_STACK) != 0)
		{
			first_stack_reg = reg;
		}

		/* If this is a stack register, then we need to find the
		   register that contains the top-most stack position,
		   because we must spill stack registers from top down.
		   As we spill each one, something else will become the top */
		if((_jit_reg_info[reg].flags & JIT_REG_IN_STACK) != 0)
		{
			real_reg = gen->stack_map[first_stack_reg];
			if(real_reg == -1)
			{
				continue;
			}
		}
		else
		{
			real_reg = reg;
		}

		/* Skip this register if there is nothing in it */
		if(gen->contents[real_reg].num_values == 0 &&
		   !(gen->contents[real_reg].used_for_temp))
		{
			continue;
		}

		/* Get the other register in a long pair, if there is one */
		if(gen->contents[real_reg].is_long_start)
		{
			other_reg = _jit_reg_info[real_reg].other_reg;
		}
		else
		{
			other_reg = -1;
		}

		/* Spill all values that are associated with the register */
		value_used = 0;
		if(gen->contents[real_reg].num_values > 0)
		{
			for(posn = gen->contents[real_reg].num_values - 1;
				posn >= 0; --posn)
			{
				value = gen->contents[real_reg].values[posn];
				if(value->has_global_register)
				{
					if(!(value->in_global_register))
					{
						_jit_gen_spill_reg(gen, real_reg, other_reg, value);
						value->in_global_register = 1;
						value_used = 1;
					}
				}
				else if(!(value->in_frame))
				{
					if((_jit_reg_info[reg].flags & JIT_REG_IN_STACK) == 0)
					{
						_jit_gen_spill_reg(gen, real_reg, other_reg, value);
					}
					else
					{
						/* The back end needs to think that we are spilling
						   the first register in the stack, regardless of
						   what "real_reg" might happen to be */
						_jit_gen_spill_reg(gen, first_stack_reg, -1, value);
					}
					value->in_frame = 1;
					value_used = 1;
				}
				value->in_register = 0;
				value->reg = -1;
			}
		}

		/* Free the register */
		_jit_regs_free_reg(gen, real_reg, value_used);
	}

#ifdef JIT_REG_DEBUG
	dump_regs(gen, "leave spill_all_between");
#endif
}

/*
 * Spill a specific register.  If it is in a stack, then all registers
 * above the specific register must also be spilled.
 */
static void spill_register(jit_gencode_t gen, int reg)
{
	if((_jit_reg_info[reg].flags & JIT_REG_IN_STACK) == 0)
	{
		spill_all_between(gen, reg, reg);
	}
	else
	{
		int first_reg;
		reg = gen->contents[reg].remap;
		first_reg = reg;
		while((_jit_reg_info[first_reg].flags & JIT_REG_START_STACK) == 0)
		{
			--first_reg;
		}
		spill_all_between(gen, first_reg, reg);
	}
}

/*
 * Spill all stack registers of a specific type.
 */
static void spill_all_stack(jit_gencode_t gen, int reg)
{
	int first_reg;
	while((_jit_reg_info[reg].flags & JIT_REG_START_STACK) == 0)
	{
		--reg;
	}
	first_reg = reg;
	while((_jit_reg_info[reg].flags & JIT_REG_END_STACK) == 0)
	{
		++reg;
	}
	spill_all_between(gen, first_reg, reg);
}

/*@
 * @deftypefun void _jit_regs_spill_all (jit_gencode_t gen)
 * Spill all of the temporary registers to memory locations.
 * Normally used at the end of a block, but may also be used in
 * situations where a value must be in a certain register and
 * it is too hard to swap things around to put it there.
 * @end deftypefun
@*/
void _jit_regs_spill_all(jit_gencode_t gen)
{
	spill_all_between(gen, 0, JIT_NUM_REGS - 1);
}

/*
 * Free a register within a stack, and renumber the other stack registers
 * to compensate for the change.
 */
static void free_stack_reg(jit_gencode_t gen, int reg)
{
	int remap;

#ifdef JIT_REG_DEBUG
	dump_regs(gen, "enter free_stack_reg");
#endif

	/* Shift everything after this register up by one position */
	remap = gen->contents[reg].remap;
	if((_jit_reg_info[remap].flags & JIT_REG_END_STACK) == 0)
	{
		++remap;
		for(;;)
		{
			if(gen->stack_map[remap] == -1)
			{
				/* There are no more active values in this stack */
				gen->stack_map[remap - 1] = -1;
				break;
			}
			else if((_jit_reg_info[remap].flags & JIT_REG_END_STACK) != 0)
			{
				/* This is the last register in the stack */
				--(gen->contents[gen->stack_map[remap]].remap);
				gen->stack_map[remap - 1] = gen->stack_map[remap];
				gen->stack_map[remap] = -1;
				break;
			}
			else
			{
				/* Shift this stack entry up by one */
				--(gen->contents[gen->stack_map[remap]].remap);
				gen->stack_map[remap - 1] = gen->stack_map[remap];
				++remap;
			}
		}
	}

	/* Clear the remapping for the register */
	gen->contents[reg].remap = -1;

#ifdef JIT_REG_DEBUG
	dump_regs(gen, "leave free_stack_reg");
#endif
}

/*
 * Make space for a new stack register in a particular stack.
 * Returns the pseudo register number of the newly allocated register.
 */
static int create_stack_reg(jit_gencode_t gen, int reg, int roll_down)
{
	int first_stack_reg;
	int temp_reg, remap;

#ifdef JIT_REG_DEBUG
	dump_regs(gen, "enter create_stack_reg");
#endif

	/* Find the first pseudo register in the stack */
	while((_jit_reg_info[reg].flags & JIT_REG_START_STACK) == 0)
	{
		--reg;
	}
	first_stack_reg = reg;

	/* Find a free pseudo register in the stack */
	for(;;)
	{
		if(gen->contents[reg].num_values == 0 &&
		   !(gen->contents[reg].used_for_temp))
		{
			break;
		}
		if((_jit_reg_info[reg].flags & JIT_REG_END_STACK) != 0)
		{
			/* None of the registers are free, so we have to spill them all */
			spill_all_between(gen, first_stack_reg, reg);
			reg = first_stack_reg;
			break;
		}
		++reg;
	}

	/* Roll the stack remappings down to make room at the top */
	if(roll_down)
	{
		temp_reg = first_stack_reg - 1;
		do
		{
			++temp_reg;
			remap = gen->contents[temp_reg].remap;
			if(remap != -1)
			{
				/* Change the register's position in the stack */
				gen->contents[temp_reg].remap = remap + 1;
				gen->stack_map[remap + 1] = temp_reg;

				/* Mark the rolled-down register position as touched */
				jit_reg_set_used(gen->touched, remap + 1);
			}
		}
		while((_jit_reg_info[temp_reg].flags & JIT_REG_END_STACK) == 0);
		gen->contents[reg].remap = first_stack_reg;
		gen->stack_map[first_stack_reg] = reg;
	}

	/* Mark the register as touched, in case it needs to be saved */
	jit_reg_set_used(gen->touched, reg);

#ifdef JIT_REG_DEBUG
	dump_regs(gen, "leave create_stack_reg");
#endif

	/* Return the free register to the caller */
	return reg;
}

/*
 * Free a register, and optionally spill its value.
 */
static void free_reg_and_spill
	(jit_gencode_t gen, int reg, int value_used, int spill)
{
	int other_reg, posn;
	jit_value_t value;

#ifdef JIT_REG_DEBUG
	dump_regs(gen, "enter free_reg_and_spill");
#endif

	/* Find the other register in a long pair */
	if(gen->contents[reg].is_long_start)
	{
		other_reg = _jit_reg_info[reg].other_reg;
		gen->contents[reg].is_long_start = 0;
		gen->contents[other_reg].is_long_end = 0;
	}
	else if(gen->contents[reg].is_long_end)
	{
		gen->contents[reg].is_long_end = 0;
		other_reg = reg;
		for(reg = 0; reg < JIT_NUM_REGS; ++reg)
		{
			if(other_reg == _jit_reg_info[reg].other_reg)
			{
				gen->contents[reg].is_long_start = 0;
				break;
			}
		}
	}
	else
	{
		other_reg = -1;
	}

	/* Spill the register's contents to the local variable frame */
	if(spill && gen->contents[reg].num_values > 0)
	{
		for(posn = gen->contents[reg].num_values - 1; posn >= 0; --posn)
		{
			value = gen->contents[reg].values[posn];
			if(value->has_global_register)
			{
				if(!(value->in_global_register))
				{
					_jit_gen_spill_reg(gen, reg, other_reg, value);
					value->in_global_register = 1;
					value_used = 1;
				}
			}
			else if(!(value->in_frame))
			{
				if((_jit_reg_info[reg].flags & JIT_REG_IN_STACK) == 0)
				{
					_jit_gen_spill_reg(gen, reg, other_reg, value);
				}
				else
				{
					_jit_gen_spill_reg
						(gen, gen->contents[reg].remap, -1, value);
				}
				value->in_frame = 1;
				value_used = 1;
			}
			value->in_register = 0;
			value->reg = -1;
		}
	}

	/* The registers do not contain values any more */
	gen->contents[reg].num_values = 0;
	gen->contents[reg].used_for_temp = 0;
	if(other_reg != -1)
	{
		gen->contents[other_reg].num_values = 0;
		gen->contents[other_reg].used_for_temp = 0;
	}

	/* If the registers are members of a stack, then readjust the
	   stack mappings to compensate for the change */
	if(gen->contents[reg].remap != -1)
	{
		free_stack_reg(gen, reg);
	}
	if(other_reg != -1 && gen->contents[other_reg].remap != -1)
	{
		free_stack_reg(gen, other_reg);
	}

#ifdef JIT_REG_DEBUG
	dump_regs(gen, "leave free_reg_and_spill");
#endif

	/* Free the register using CPU-specific code */
	_jit_gen_free_reg(gen, reg, other_reg, value_used);
}

/*@
 * @deftypefun void _jit_regs_want_reg (jit_gencode_t gen, int reg, int for_long)
 * Tell the register allocator that we want a particular register
 * for a specific purpose.  The current contents of the register
 * are spilled.  If @code{reg} is part of a register pair, then the
 * other register in the pair will also be spilled.  If @code{reg}
 * is a stack register, then it should be the first one.
 *
 * This is typically used for x86 instructions that require operands
 * to be in certain registers (especially multiplication and division),
 * and we want to make sure that the register is free before we clobber it.
 * It is also used to make space in the x86 FPU for floating-point returns.
 *
 * This may return a different pseudo register number if @code{reg}
 * was a member of a stack and some other register was made free.
 * @end deftypefun
@*/
int _jit_regs_want_reg(jit_gencode_t gen, int reg, int for_long)
{
	int other_reg;
	if((_jit_reg_info[reg].flags & JIT_REG_IN_STACK) == 0)
	{
		/* Spill an ordinary register and its pair register */
		free_reg_and_spill(gen, reg, 0, 1);
		if(for_long)
		{
			other_reg = _jit_reg_info[reg].other_reg;
			if(other_reg != -1)
			{
				free_reg_and_spill(gen, other_reg, 0, 1);
			}
		}
		else
		{
			other_reg = -1;
		}

		/* Mark the register as touched and return it */
		jit_reg_set_used(gen->touched, reg);
		if(other_reg != -1)
		{
			jit_reg_set_used(gen->touched, other_reg);
		}
		return reg;
	}
	else
	{
		/* If we want a stack register, all we have to do is roll
		   everything down to make room for the new value.  If the
		   stack is full, then we spill the entire stack */
		return create_stack_reg(gen, reg, 0);
	}
}

/*@
 * @deftypefun void _jit_regs_free_reg (jit_gencode_t gen, int reg, int value_used)
 * Free the contents of a pseudo register, without spilling.  Used when
 * the contents of a register becomes invalid.  If @code{value_used}
 * is non-zero, then it indicates that the value has already been
 * used.  On some systems, an explicit instruction is needed to free
 * a register whose value hasn't been used yet (e.g. x86 floating point
 * stack registers).
 * @end deftypefun
@*/
void _jit_regs_free_reg(jit_gencode_t gen, int reg, int value_used)
{
	free_reg_and_spill(gen, reg, value_used, 0);
}

/*@
 * @deftypefun void _jit_regs_set_value (jit_gencode_t gen, int reg, jit_value_t value, int still_in_frame)
 * Set pseudo register @code{reg} to record that it currently holds the
 * contents of @code{value}.  The value is assumed to already be in
 * the register and no spill occurs.  If @code{still_in_frame} is
 * non-zero, then the value is still in the stack frame; otherwise the
 * value is exclusively in the register.
 * @end deftypefun
@*/
void _jit_regs_set_value
	(jit_gencode_t gen, int reg, jit_value_t value, int still_in_frame)
{
	int other_reg;
	int first_stack_reg;

#ifdef JIT_REG_DEBUG
	dump_regs(gen, "enter set_value");
#endif

	/* Get the other register in a pair */
	if(_jit_regs_needs_long_pair(value->type))
	{
		other_reg = _jit_reg_info[reg].other_reg;
	}
	else
	{
		other_reg = -1;
	}

	/* Mark the register as touched */
	jit_reg_set_used(gen->touched, reg);
	if(other_reg != -1)
	{
		jit_reg_set_used(gen->touched, other_reg);
	}

	/* Adjust the allocation state to reflect that "reg" contains "value" */
	gen->contents[reg].values[0] = value;
	gen->contents[reg].num_values = 1;
	gen->contents[reg].age = gen->current_age;
	if(other_reg == -1)
	{
		gen->contents[reg].is_long_start = 0;
		gen->contents[reg].is_long_end = 0;
		gen->contents[reg].used_for_temp = 0;
	}
	else
	{
		gen->contents[reg].is_long_start = 1;
		gen->contents[reg].is_long_end = 0;
		gen->contents[reg].used_for_temp = 0;
		gen->contents[other_reg].num_values = 0;
		gen->contents[other_reg].is_long_start = 0;
		gen->contents[other_reg].is_long_end = 1;
		gen->contents[other_reg].age = gen->current_age;
		gen->contents[other_reg].used_for_temp = 0;
	}
	(gen->current_age)++;

	/* Set the stack mappings for this register */
	if((_jit_reg_info[reg].flags & JIT_REG_IN_STACK) != 0)
	{
		first_stack_reg = reg;
		while((_jit_reg_info[first_stack_reg].flags & JIT_REG_START_STACK) == 0)
		{
			--first_stack_reg;
		}
		gen->contents[reg].remap = first_stack_reg;
		gen->stack_map[first_stack_reg] = reg;
	}

#ifdef JIT_REG_DEBUG
	dump_regs(gen, "leave set_value");
#endif

	/* Adjust the value to reflect that it is in "reg", and maybe the frame */
	value->in_register = 1;
	if(value->has_global_register)
		value->in_global_register = still_in_frame;
	else
		value->in_frame = still_in_frame;
	value->reg = (short)reg;
}

/*@
 * @deftypefun void _jit_regs_set_incoming (jit_gencode_t gen, int reg, jit_value_t value)
 * Set pseudo register @code{reg} to record that it currently holds the
 * contents of @code{value}.  If the register was previously in use,
 * then spill its value first.
 * @end deftypefun
@*/
void _jit_regs_set_incoming(jit_gencode_t gen, int reg, jit_value_t value)
{
	/* Eject any values that are currently in the register */
	reg = _jit_regs_want_reg(gen, reg, _jit_regs_needs_long_pair(value->type));

	/* Record that the value is in "reg", but not in the frame */
	_jit_regs_set_value(gen, reg, value, 0);
}

/*@
 * @deftypefun void _jit_regs_set_outgoing (jit_gencode_t gen, int reg, jit_value_t value)
 * Load the contents of @code{value} into pseudo register @code{reg},
 * spilling out the current contents.  This is used to set up outgoing
 * parameters for a function call.
 * @end deftypefun
@*/
void _jit_regs_set_outgoing(jit_gencode_t gen, int reg, jit_value_t value)
{
	int other_reg;
	int need_pair;

#ifdef JIT_BACKEND_X86
	jit_type_t type;
	type = jit_type_normalize(value->type);
	need_pair = 0;
	if(type)
	{
		/* We might need to put float values in register pairs under x86 */
		if(type->kind == JIT_TYPE_LONG || type->kind == JIT_TYPE_ULONG ||
		   type->kind == JIT_TYPE_FLOAT64 || type->kind == JIT_TYPE_NFLOAT)
		{
			need_pair = 1;
		}
	}
	if(value->in_register && value->reg == reg && !need_pair)
#else
	need_pair = _jit_regs_needs_long_pair(value->type);
	if(value->in_register && value->reg == reg)
#endif
	{
		/* The value is already in the register, but we may need to spill
		   if the frame copy is not up to date with the register */
		if(!(value->in_global_register) && !(value->in_frame) &&
		   !(value->is_temporary))
		{
			free_reg_and_spill(gen, reg, 1, 1);
		}

		/* The value is no longer "really" in the register.  A copy is
		   left behind, but the value itself reverts to the frame copy
		   as we are about to kill the registers in a function call */
		value->in_register = 0;
		value->reg = -1;
	}
	else
	{
		/* Force the value out of whatever register it is already in */
		_jit_regs_force_out(gen, value, 0);

		/* Reload the value into the specified register */
		if(need_pair)
		{
		#ifdef JIT_BACKEND_X86
			/* Long values in outgoing registers must be in ECX:EDX,
			   not in the ordinary register pairing of ECX:EBX */
			_jit_regs_want_reg(gen, reg, 0);
			other_reg = 2;
			_jit_regs_want_reg(gen, other_reg, 0);
		#else
			_jit_regs_want_reg(gen, reg, 1);
			other_reg = _jit_reg_info[reg].other_reg;
		#endif
			_jit_gen_load_value(gen, reg, other_reg, value);
			jit_reg_set_used(gen->inhibit, reg);
			jit_reg_set_used(gen->inhibit, other_reg);
		}
		else
		{
			_jit_regs_want_reg(gen, reg, 0);
			_jit_gen_load_value(gen, reg, -1, value);
			jit_reg_set_used(gen->inhibit, reg);
		}
	}
}

/*@
 * @deftypefun int _jit_regs_is_top (jit_gencode_t gen, jit_value_t value)
 * Determine if @code{value} is currently the in top-most position
 * in the appropriate register stack.  Always returns non-zero if
 * @code{value} is in a register, but that register is not part of a
 * register stack.  This is used to check if an operand value is
 * already in the right position for a unary operation.
 * @end deftypefun
@*/
int _jit_regs_is_top(jit_gencode_t gen, jit_value_t value)
{
	int reg, remap;
	if(!(value->in_register))
	{
		return 0;
	}
	reg = value->reg;
	if((_jit_reg_info[reg].flags & JIT_REG_IN_STACK) == 0)
	{
		return 1;
	}
	remap = gen->contents[reg].remap;
	if(remap != -1 && (_jit_reg_info[remap].flags & JIT_REG_START_STACK) != 0)
	{
		return 1;
	}
	return 0;
}

/*@
 * @deftypefun int _jit_regs_is_top_two (jit_gencode_t gen, jit_value_t value1, jit_value_t value2)
 * Determine if @code{value1} and @code{value2} are in the top two positions
 * in the appropriate register stack, and @code{value2} is above
 * @code{value1}.  Always returns non-zero if @code{value} and
 * @code{value2} are in registers, but those registers are not part
 * of a register stack.  This is used to check if the operand values
 * for a binary operation are already in the right positions.
 * @end deftypefun
@*/
int _jit_regs_is_top_two
	(jit_gencode_t gen, jit_value_t value1, jit_value_t value2)
{
	int reg, remap;
	if(!(value1->in_register) || !(value2->in_register))
	{
		return 0;
	}
	reg = value2->reg;
	if((_jit_reg_info[reg].flags & JIT_REG_IN_STACK) == 0)
	{
		reg = value1->reg;
		return ((_jit_reg_info[reg].flags & JIT_REG_IN_STACK) == 0);
	}
	remap = gen->contents[reg].remap;
	if(remap == -1 || (_jit_reg_info[remap].flags & JIT_REG_START_STACK) == 0)
	{
		return 0;
	}
	reg = value1->reg;
	if((_jit_reg_info[reg].flags & JIT_REG_IN_STACK) == 0)
	{
		return 1;
	}
	return (gen->contents[reg].remap == (remap + 1));
}

/*
 * Load a value into a register.
 */
static void load_value(jit_gencode_t gen, int reg, int other_reg,
					   jit_value_t value, int destroy)
{
	_jit_gen_load_value(gen, reg, other_reg, value);
	if(destroy || value->is_constant)
	{
		/* Mark the register as containing a temporary value */
		gen->contents[reg].used_for_temp = 1;
		jit_reg_set_used(gen->touched, reg);
		if(other_reg != -1)
		{
			gen->contents[reg].is_long_start = 1;
			gen->contents[other_reg].is_long_end = 1;
			gen->contents[other_reg].used_for_temp = 1;
			jit_reg_set_used(gen->touched, other_reg);
		}
	}
	else
	{
		/* Mark the register as containing the value we have loaded */
		if(value->has_global_register)
			_jit_regs_set_value(gen, reg, value, value->in_global_register);
		else
			_jit_regs_set_value(gen, reg, value, value->in_frame);
	}
}

/*
 * Find a free register to hold the contents of a value.
 */
static int free_register_for_value
	(jit_gencode_t gen, jit_value_t value, int *other_reg)
{
	int reg, type;
	int suitable_reg, need_pair;
	int suitable_age;

	/* Clear the other register before we start */
	*other_reg = -1;

	/* Determine if we need a long pair for this value */
	need_pair = _jit_regs_needs_long_pair(value->type);

	/* Determine the type of register that we need */
	switch(jit_type_normalize(value->type)->kind)
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
		{
			type = JIT_REG_WORD;
		}
		break;

		case JIT_TYPE_LONG:
		case JIT_TYPE_ULONG:
		{
			if(need_pair)
				type = JIT_REG_LONG;
			else
				type = JIT_REG_WORD;
		}
		break;

		case JIT_TYPE_FLOAT32:
		{
			type = JIT_REG_FLOAT32;
		}
		break;

		case JIT_TYPE_FLOAT64:
		{
			type = JIT_REG_FLOAT64;
		}
		break;

		case JIT_TYPE_NFLOAT:
		{
			type = JIT_REG_NFLOAT;
		}
		break;

		default: return -1;
	}

	/* Search for a free register, ignoring permanent global allocations.
	   We also keep track of the oldest suitable register that is not free */
	suitable_reg = -1;
	suitable_age = -1;
	for(reg = 0; reg < JIT_NUM_REGS; ++reg)
	{
		if((_jit_reg_info[reg].flags & type) != 0 &&
		   !jit_reg_is_used(gen->permanent, reg) &&
		   !jit_reg_is_used(gen->inhibit, reg))
		{
			if((_jit_reg_info[reg].flags & JIT_REG_IN_STACK) != 0)
			{
				/* We always load stack values to the top of the stack */
				reg = create_stack_reg(gen, reg, 1);
				return reg;
			}
			else if(!need_pair)
			{
				if(gen->contents[reg].num_values == 0 &&
				   !(gen->contents[reg].used_for_temp) &&
				   !(gen->contents[reg].is_long_end))
				{
					return reg;
				}
			}
			else
			{
				*other_reg = _jit_reg_info[reg].other_reg;
				if(gen->contents[reg].num_values == 0 &&
				   !(gen->contents[reg].used_for_temp) &&
				   gen->contents[*other_reg].num_values == 0 &&
				   !(gen->contents[*other_reg].used_for_temp))
				{
					return reg;
				}
			}
			if(suitable_reg == -1 || gen->contents[reg].age < suitable_age)
			{
				/* This is the oldest suitable register of this type */
				suitable_reg = reg;
				suitable_age = gen->contents[reg].age;
			}
		}
	}

	/* If there were no suitable registers at all, then fail */
	if(suitable_reg == -1)
	{
		return -1;
	}

	/* Eject the current contents of the register */
	reg = _jit_regs_want_reg(gen, suitable_reg, need_pair);
	if(need_pair)
	{
		*other_reg = _jit_reg_info[reg].other_reg;
	}
	return reg;
}

/*@
 * @deftypefun int _jit_regs_load_value (jit_gencode_t gen, jit_value_t value, int destroy, int used_again)
 * Load a value into any register that is suitable and return that register.
 * If the value needs a long pair, then this will return the first register
 * in the pair.  Returns -1 if the value will not fit into any register.
 *
 * If @code{destroy} is non-zero, then we are about to destroy the register,
 * so the system must make sure that such destruction will not side-effect
 * @code{value} or any of the other values currently in that register.
 *
 * If @code{used_again} is non-zero, then it indicates that the value is
 * used again further down the block.
 * @end deftypefun
@*/
int _jit_regs_load_value
	(jit_gencode_t gen, jit_value_t value, int destroy, int used_again)
{
	int reg, other_reg, need_pair;

	/* Determine if we need a long pair for this value */
	need_pair = _jit_regs_needs_long_pair(value->type);

	/* If the value is already in a register, then try to use that register */
	if(value->in_register)
	{
		reg = value->reg;
		if(destroy)
		{
			if(gen->contents[reg].num_values == 1 &&
			   (value->in_frame || value->in_global_register || !used_again))
			{
				/* We are the only value in this register, and the
				   value is duplicated in the frame, or will never
				   be used again in this block.  In this case,
				   we can disassociate the register from the value
				   and just return the register as-is */
				value->in_register = 0;
				gen->contents[reg].num_values = 0;
				gen->contents[reg].used_for_temp = 1;
				gen->contents[reg].age = gen->current_age;
				if(need_pair)
				{
					other_reg = _jit_reg_info[reg].other_reg;
					gen->contents[other_reg].used_for_temp = 1;
					gen->contents[other_reg].age = gen->current_age;
				}
				++(gen->current_age);
				return reg;
			}
			else
			{
				/* We need to spill the register and then reload it */
				spill_register(gen, reg);
			}
		}
		else
		{
			if(gen->contents[reg].num_values == 1 &&
			   (value->in_frame || value->in_global_register || !used_again))
			{
				/* We are the only value in this register, and the
				   value is duplicated in the frame, or will never
				   be used again in this block.  In this case,
				   we can disassociate the register from the value
				   and just return the register as-is */
				value->in_register = 0;
				gen->contents[reg].num_values = 0;
				gen->contents[reg].used_for_temp = 1;
			}
			gen->contents[reg].age = gen->current_age;
			if(need_pair)
			{
				other_reg = _jit_reg_info[reg].other_reg;
				gen->contents[other_reg].age = gen->current_age;
			}
			++(gen->current_age);
			return reg;
		}
	}

	/* If the value is in a global register, and we are not going
	   to destroy the value, then use the global register itself.
	   This will avoid a redundant register copy operation */
	if(value->in_global_register && !destroy)
	{
		return value->global_reg;
	}

	/* Search for a free register to hold the value */
	reg = free_register_for_value(gen, value, &other_reg);
	load_value(gen, reg, other_reg, value, destroy);
	return reg;
}

/*@
 * @deftypefun int _jit_regs_dest_value (jit_gencode_t gen, jit_value_t value)
 * Get a new register to hold @code{value} as a destination.  This cannot
 * be used for stack register destinations (use @code{_jit_regs_new_top}
 * for that).
 * @end deftypefun
@*/
int _jit_regs_dest_value(jit_gencode_t gen, jit_value_t value)
{
	int reg, other_reg;

	/* If the value is exclusively in a register already, then use that */
	if(value->in_register)
	{
		reg = value->reg;
		if(gen->contents[reg].num_values == 1)
		{
			value->in_frame = 0;
			value->in_global_register = 0;
			return reg;
		}
		free_reg_and_spill(gen, reg, 0, 1);
	}

	/* Find a suitable register to hold the destination */
	reg = free_register_for_value(gen, value, &other_reg);
	_jit_regs_set_value(gen, reg, value, 0);
	return reg;
}

/*
 * Determine if "num" stack registers are free in a specific stack.
 */
static int stack_regs_free(jit_gencode_t gen, int reg, int num)
{
	int first_reg;

	/* Find the extents of the stack */
	while((_jit_reg_info[reg].flags & JIT_REG_START_STACK) == 0)
	{
		--reg;
	}
	first_reg = reg;
	while((_jit_reg_info[reg].flags & JIT_REG_END_STACK) == 0)
	{
		++reg;
	}

	/* Search for free registers */
	while(reg >= first_reg)
	{
		if(gen->contents[reg].num_values == 0 &&
		   !(gen->contents[reg].used_for_temp))
		{
			--num;
			if(num <= 0)
			{
				return 1;
			}
		}
		--reg;
	}
	return 0;
}

/*@
 * @deftypefun int _jit_regs_load_to_top (jit_gencode_t gen, jit_value_t value, int used_again, int type_reg)
 * Load the contents of @code{value} into a register that is guaranteed to
 * be at the top of its stack.  This is the preferred way to set up for a
 * unary operation on a stack-based architecture.  Returns the pseudo
 * register that contains the value.
 *
 * When @code{value} is loaded, the "destroy" flag is set so that the
 * unary operation will not affect the original contents of @code{value}.
 * The @code{used_again} flag indicates if @code{value} is used again
 * in the current basic block.
 *
 * The @code{type_reg} parameter should be set to the pseudo register
 * number of a suitable register.  This is used to determine which
 * register stack to use for the allocation.
 * @end deftypefun
@*/
int _jit_regs_load_to_top(jit_gencode_t gen, jit_value_t value, int used_again, int type_reg)
{
	int reg;

	/* Determine if the value is already in the top-most register */
	if(value->in_register)
	{
		reg = value->reg;
		if((_jit_reg_info[gen->contents[reg].remap].flags
				& JIT_REG_START_STACK) != 0)
		{
			if(value->in_frame || value->in_global_register || !used_again)
			{
				/* Disassociate the value from the register and return */
				value->in_register = 0;
				gen->contents[reg].num_values = 0;
				gen->contents[reg].used_for_temp = 1;
				gen->contents[reg].age = gen->current_age;
				++(gen->current_age);
				return reg;
			}
		}
		spill_all_stack(gen, type_reg);
	}

	/* If there are free registers of this type, then load the value now */
	if(stack_regs_free(gen, type_reg, 1))
	{
		return _jit_regs_load_value(gen, value, 1, used_again);
	}

	/* Spill the entire stack contents, to get things into a known state */
	spill_all_stack(gen, type_reg);

	/* Reload the value and return */
	return _jit_regs_load_value(gen, value, 1, used_again);
}

/*@
 * @deftypefun int _jit_regs_load_to_top_two (jit_gencode_t gen, jit_value_t value, jit_value_t value2, int used_again1, int used_again2, int type_reg)
 * Load the contents of @code{value} and @code{value2} into registers that
 * are guaranteed to be at the top of the relevant register stack.
 * This is the preferred way to set up for a binary operation on a
 * stack-based architecture.
 *
 * Returns the pseudo register that contains @code{value}.  The pseudo
 * register that contains @code{value2} is marked as free, because it is
 * assumed that the binary operation will immediately consume its value.
 *
 * When @code{value} are @code{value2} are loaded, the "destroy" flag is
 * set so that the binary operation will not affect their original contents.
 * The @code{used_again1} and @code{used_again2} flags indicate if
 * @code{value} and @code{value2} are used again in the current basic block.
 *
 * The @code{type_reg} parameter should be set to the pseudo register
 * number of a suitable register.  This is used to determine which
 * register stack to use for the allocation.
 * @end deftypefun
@*/
int _jit_regs_load_to_top_two
	(jit_gencode_t gen, jit_value_t value, jit_value_t value2,
	 int used_again1, int used_again2, int type_reg)
{
	int reg, reg2;

	/* Determine if the values are already in the top two registers */
	if(value->in_register && value2->in_register)
	{
		reg = value->reg;
		reg2 = value2->reg;
		if((_jit_reg_info[gen->contents[reg2].remap].flags
				& JIT_REG_START_STACK) != 0 &&
		   gen->contents[reg].remap == (gen->contents[reg2].remap + 1))
		{
			if((value->in_frame || value->in_global_register || !used_again1) &&
			   (value2->in_frame || value2->in_global_register || !used_again2))
			{
				/* Disassociate the values from the registers and return */
				free_stack_reg(gen, reg2);
				value->in_register = 0;
				value2->in_register = 0;
				gen->contents[reg].num_values = 0;
				gen->contents[reg].used_for_temp = 1;
				gen->contents[reg].age = gen->current_age;
				gen->contents[reg2].num_values = 0;
				gen->contents[reg2].used_for_temp = 0;
				gen->contents[reg2].age = gen->current_age;
				++(gen->current_age);
				return reg;
			}
		}
		spill_all_stack(gen, type_reg);
	}
	else if(value2->in_register && !(value->in_register))
	{
		/* We'll probably need to rearrange the stack, so spill first */
		spill_all_stack(gen, type_reg);
	}

	/* If there are free registers of this type, then load the values now */
	if(stack_regs_free(gen, type_reg, 2))
	{
		reg = _jit_regs_load_value(gen, value, 1, used_again1);
		reg2 = _jit_regs_load_value(gen, value2, 1, used_again2);
		free_stack_reg(gen, reg2);
		gen->contents[reg2].used_for_temp = 0;
		return reg;
	}

	/* Spill the entire stack contents, to get things into a known state */
	spill_all_stack(gen, type_reg);

	/* Reload the values and return */
	reg = _jit_regs_load_value(gen, value, 1, used_again1);
	reg2 = _jit_regs_load_value(gen, value2, 1, used_again2);
	free_stack_reg(gen, reg2);
	gen->contents[reg2].used_for_temp = 0;
	return reg;
}

/*@
 * @deftypefun void _jit_regs_load_to_top_three (jit_gencode_t gen, jit_value_t value, jit_value_t value2, jit_value_t value3, int used_again1, int used_again2, int used_again3, int type_reg)
 * Load three values to the top of a register stack.  The values are assumed
 * to be popped by the subsequent operation.  This is used by the interpreted
 * back end for things like array stores, that need three values but all
 * of them are discarded after the operation.
 * @end deftypefun
@*/
void _jit_regs_load_to_top_three
	(jit_gencode_t gen, jit_value_t value, jit_value_t value2,
	 jit_value_t value3, int used_again1, int used_again2,
	 int used_again3, int type_reg)
{
	int reg, reg2, reg3;

	/* Determine if the values are already in the top three registers */
	if(value->in_register && value2->in_register && value3->in_register)
	{
		reg = value->reg;
		reg2 = value2->reg;
		reg3 = value3->reg;
		if((_jit_reg_info[gen->contents[reg2].remap].flags
				& JIT_REG_START_STACK) != 0 &&
		   gen->contents[reg].remap == (gen->contents[reg2].remap + 1) &&
		   gen->contents[reg2].remap == (gen->contents[reg3].remap + 1))
		{
			if((value->in_frame || value->in_global_register || !used_again1) &&
			   (value2->in_frame || value2->in_global_register ||
			   			!used_again2) &&
			   (value3->in_frame || value3->in_global_register || !used_again3))
			{
				/* Disassociate the values from the registers and return */
				free_stack_reg(gen, reg);
				free_stack_reg(gen, reg2);
				free_stack_reg(gen, reg3);
				value->in_register = 0;
				value2->in_register = 0;
				value3->in_register = 0;
				gen->contents[reg].used_for_temp = 0;
				gen->contents[reg2].used_for_temp = 0;
				gen->contents[reg3].used_for_temp = 0;
				return;
			}
		}
	}

	/* Spill everything out, so that we know where things are */
	spill_all_stack(gen, type_reg);

	/* Load the three values that we want onto the stack */
	reg = _jit_regs_load_value(gen, value, 1, used_again1);
	reg2 = _jit_regs_load_value(gen, value2, 1, used_again2);
	reg3 = _jit_regs_load_value(gen, value3, 1, used_again3);
	gen->contents[reg].used_for_temp = 0;
	gen->contents[reg2].used_for_temp = 0;
	gen->contents[reg3].used_for_temp = 0;
}

/*@
 * @deftypefun int _jit_regs_num_used (jit_gencode_t gen, int type_reg)
 * Get the number of stack registers in use within the register stack
 * indicated by @code{type_reg}.
 * @end deftypefun
@*/
int _jit_regs_num_used(jit_gencode_t gen, int type_reg)
{
	int count;
	while((_jit_reg_info[type_reg].flags & JIT_REG_START_STACK) == 0)
	{
		--type_reg;
	}
	count = 0;
	for(;;)
	{
		if(gen->contents[type_reg].num_values > 0 ||
		   gen->contents[type_reg].used_for_temp)
		{
			++count;
		}
		if((_jit_reg_info[type_reg].flags & JIT_REG_END_STACK) == 0)
		{
			break;
		}
		++type_reg;
	}
	return count;
}

/*@
 * @deftypefun int _jit_regs_new_top (jit_gencode_t gen, jit_value_t value, int type_reg)
 * Record that the top of the stack indicated by @code{type_reg} now
 * contains @code{value}.  This is slightly different from
 * @code{_jit_regs_set_value}, in that the register wasn't previously
 * allocated to a temporary operand value.  Returns the actual stack
 * register that contains @code{value}.
 * @end deftypefun
@*/
int _jit_regs_new_top(jit_gencode_t gen, jit_value_t value, int type_reg)
{
	int reg;
	
	/* Create space for the value at the top of the stack */
	reg = create_stack_reg(gen, type_reg, 1);

	/* Record the "value" is now in this register */
	value->in_register = 1;
	value->in_frame = 0;
	value->in_global_register = 0;
	value->reg = reg;
	gen->contents[reg].values[0] = value;
	gen->contents[reg].num_values = 1;

	/* Return the allocated register to the caller */
	return reg;
}

/*@
 * @deftypefun void _jit_regs_force_out (jit_gencode_t gen, jit_value_t value, int is_dest)
 * If @code{value} is currently in a register, then force its value out
 * into the stack frame.  The @code{is_dest} flag indicates that the value
 * will be a destination, so we don't care about the original value.
 * @end deftypefun
@*/
void _jit_regs_force_out(jit_gencode_t gen, jit_value_t value, int is_dest)
{
	if(value->in_register)
	{
		if((_jit_reg_info[value->reg].flags & JIT_REG_IN_STACK) == 0)
		{
			free_reg_and_spill(gen, value->reg, 0, !is_dest);
		}
		else
		{
			/* Always do a spill for a stack register */
			spill_register(gen, value->reg);
		}
	}
}

/*
 * Minimum number of times a candidate must be used before it
 * is considered worthy of putting in a global register.
 */
#define	JIT_MIN_USED		3

/*@
 * @deftypefun void _jit_regs_alloc_global (jit_gencode_t gen, jit_function_t func)
 * Perform global register allocation on the values in @code{func}.
 * This is called during function compilation just after variable
 * liveness has been computed.
 * @end deftypefun
@*/
void _jit_regs_alloc_global(jit_gencode_t gen, jit_function_t func)
{
#if JIT_NUM_GLOBAL_REGS != 0
	jit_value_t candidates[JIT_NUM_GLOBAL_REGS];
	int num_candidates = 0;
	int index, reg, posn, num;
	jit_pool_block_t block;
	jit_value_t value, temp;

	/* If the function has a "try" block, then don't do global allocation
	   as the "longjmp" for exception throws will wipe out global registers */
	if(func->has_try)
	{
		return;
	}

	/* If the current function involves a tail call, then we don't do
	   global register allocation and we also prevent the code generator
	   from using any of the callee-saved registers.  This simplifies
	   tail calls, which don't have to worry about restoring such registers */
	if(func->builder->has_tail_call)
	{
		for(reg = 0; reg < JIT_NUM_REGS; ++reg)
		{
			if((_jit_reg_info[reg].flags &
					(JIT_REG_FIXED | JIT_REG_CALL_USED)) == 0)
			{
				jit_reg_set_used(gen->permanent, reg);
			}
		}
		return;
	}

	/* Scan all values within the function, looking for the most used.
	   We will replace this with a better allocation strategy later */
	block = func->builder->value_pool.blocks;
	num = (int)(func->builder->value_pool.elems_per_block);
	while(block != 0)
	{
		if(!(block->next))
		{
			num = (int)(func->builder->value_pool.elems_in_last);
		}
		for(posn = 0; posn < num; ++posn)
		{
			value = (jit_value_t)(block->data + posn *
								  sizeof(struct _jit_value));
			if(value->global_candidate && value->usage_count >= JIT_MIN_USED &&
			   !(value->is_addressable) && !(value->is_volatile))
			{
				/* Insert this candidate into the list, ordered on count */
				index = 0;
				while(index < num_candidates &&
				      value->usage_count <= candidates[index]->usage_count)
				{
					++index;
				}
				while(index < num_candidates)
				{
					temp = candidates[index];
					candidates[index] = value;
					value = temp;
					++index;
				}
				if(index < JIT_NUM_GLOBAL_REGS)
				{
					candidates[num_candidates++] = value;
				}
			}
		}
		block = block->next;
	}

	/* Allocate registers to the candidates.  We allocate from the top-most
	   register in the allocation order, because some architectures like
	   PPC require global registers to be saved top-down for efficiency */
	reg = JIT_NUM_REGS - 1;
	for(index = 0; index < num_candidates; ++index)
	{
		while(reg >= 0 && (_jit_reg_info[reg].flags & JIT_REG_GLOBAL) == 0)
		{
			--reg;
		}
		candidates[index]->has_global_register = 1;
		candidates[index]->global_reg = (short)reg;
		jit_reg_set_used(gen->touched, reg);
		jit_reg_set_used(gen->permanent, reg);
		--reg;
	}

#endif
}

/*
 * @deftypefun void _jit_regs_get_reg_pair (jit_gencode_t gen, int not_this1, int not_this2, int not_this3, {int *} reg, {int *} reg2)
 * Get a register pair for temporary operations on "long" values.
 * @end deftypefun
 */
void _jit_regs_get_reg_pair(jit_gencode_t gen, int not_this1, int not_this2,
						    int not_this3, int *reg, int *reg2)
{
	int index;
	for(index = 0; index < 8; ++index)
	{
		if((_jit_reg_info[index].flags & JIT_REG_WORD) == 0 ||
		   jit_reg_is_used(gen->permanent, index))
		{
			continue;
		}
		if(index != not_this1 && index != not_this2 &&
		   index != not_this3)
		{
			break;
		}
	}
	*reg = index;
	_jit_regs_want_reg(gen, index, 0);
	if(!reg2)
	{
		return;
	}
	for(; index < 8; ++index)
	{
		if((_jit_reg_info[index].flags & JIT_REG_WORD) == 0 ||
		   jit_reg_is_used(gen->permanent, index))
		{
			continue;
		}
		if(index != not_this1 && index != not_this2 &&
		   index != not_this3 && index != *reg)
		{
			break;
		}
	}
	if(index >= 8)
	{
		*reg2 = -1;
	}
	else
	{
		*reg2 = index;
		_jit_regs_want_reg(gen, index, 0);
	}
}

/*
 * New Reg Alloc API
 */

/*
 * Set assigned and clobbered flags for register.
 */
static void
set_register_bits(_jit_regs_t *regs, _jit_regdesc_t *desc, int output)
{
	int clobber;
	if(desc->reg >= 0)
	{
		clobber = desc->clobber;
		if(output && desc->value->in_register && desc->value->reg == desc->reg)
		{
			clobber = 0;
		}

		jit_reg_set_used(regs->assigned, desc->reg);
		if(clobber)
		{
			jit_reg_set_used(regs->clobber, desc->reg);
		}
		if(desc->other_reg >= 0)
		{
			jit_reg_set_used(regs->assigned, desc->other_reg);
			if(clobber)
			{
				jit_reg_set_used(regs->clobber, desc->other_reg);
			}
		}
	}
}

/*
 * Determine the type of register that we need.
 */
static int
get_register_type(jit_value_t value, int need_pair)
{
	switch(jit_type_normalize(value->type)->kind)
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
		return JIT_REG_WORD;

	case JIT_TYPE_LONG:
	case JIT_TYPE_ULONG:
		return need_pair ? JIT_REG_LONG : JIT_REG_WORD;

	case JIT_TYPE_FLOAT32:
		return JIT_REG_FLOAT32;

	case JIT_TYPE_FLOAT64:
		return JIT_REG_FLOAT64;

	case JIT_TYPE_NFLOAT:
		return JIT_REG_NFLOAT;
	}

	return 0;
}

/*
 * Assign diplicate input value to the same register if possible.
 * The first value has to be already assigned. The second value
 * is assigned to the same register if it is equal to the first
 * and neither of them is clobbered.
 */
static void
reuse_duplicate_value(_jit_regdesc_t *desc1, _jit_regdesc_t *desc2)
{
	if(desc1->value == desc2->value
	   && desc1->reg >= 0 && desc2->reg < 0
	   && !desc1->clobber && !desc2->clobber)
	{
		desc2->reg = desc1->reg;
		desc2->other_reg = desc1->other_reg;
	}
}

/*
 * Assign value to the register it is already in if possible. This is the case
 * if the register is not already assigned to and one of the following is true:
 *  1. The value is output and it the only value in register.
 *  2. The value is input and it is not clobbered.
 *  3. The value is input and it the only value in register, it is clobbered
 *     but not used afterwards.
 *  4. The value is input and it is clobbered even if we do not use its
 *     register. This might be because the instruction clobbers all or
 *     some registers (see _jit_regs_clobber_all(), jit_regs_clobber()).
 * NOTE: In the last case it might be possible to find a not clobbered register
 * where the value could be moved to so that the original copy can be used for
 * input without spilling. However probaly this corner case is not worth the
 * effort.
 */
static void
reuse_register(jit_gencode_t gen, _jit_regs_t *regs, _jit_regdesc_t *desc, int output)
{
	int reg;
	int other_reg;

	reg = -1;
	other_reg = -1;
	if(desc->value->in_register)
	{
		reg = desc->value->reg;

		if(_jit_regs_needs_long_pair(desc->value->type))
		{
			other_reg = _jit_reg_info[reg].other_reg;
		}
		else
		{
			other_reg = -1;
		}
	}
	else if(desc->value->in_global_register
		&& (regs->is_ternary
		    || desc->value == regs->descs[0].value
		    || desc->value != regs->descs[1].value))
	{
		/* It is safe to use the global register directly when it
		   is not a binary operation which output value is going
		   to override the original one. */
		reg = desc->value->global_reg;
		other_reg = -1;
	}

	if(reg < 0)
	{
		return;
	}

	if(jit_reg_is_used(regs->assigned, reg))
	{
		return;
	}
	if(output)
	{
		if(gen->contents[reg].num_values > 1)
		{
			return;
		}
	}
	else
	{
		if(desc->clobber
		   && (desc->live || desc->used || gen->contents[reg].num_values > 1)
		   && !jit_reg_is_used(regs->clobber, reg))
		{
			return;
		}
	}

	if(other_reg >= 0)
	{
		if(jit_reg_is_used(regs->assigned, other_reg))
		{
			return;
		}

		if(output)
		{
			if(gen->contents[other_reg].num_values > 1)
			{
				return;
			}
		}
		else
		{
			if(desc->clobber
			   && (desc->live || desc->used || gen->contents[other_reg].num_values > 1)
			   && !jit_reg_is_used(regs->clobber, other_reg))
			{
				return;
			}
		}
	}

	desc->reg = reg;
	desc->other_reg = other_reg;
	set_register_bits(regs, desc, output);
}

/*
 * Assign value to a register that is cheapest to use. We are here either
 * because the value is not in register or it is but the register will be
 * clobbered so the reuse_register() function failed to assign the register.
 *
 * Depending on the value location and on the presence of a free register we
 * do one of the following:
 *
 * 1. The value is in register and there is a free register.
 *    In this case we will generate a reg-to-reg move.
 * 2. The value is in register and there are no free regsiters.
 *    In this case we will generate a spill if the register is dirty.
 * 3. The value is in frame and there is a free register.
 *    In this case we will generate a value load.
 * 4. The value is in frame and there are no free registers.
 *    In this case we choose a register to evict. We will generate a
 *    spill for the dirty register and load the value there.
 *
 * In the last case we choose the register using the following rules:
 *
 * 1. Choose clean registers over dirty.
 * 2. Choose registers that contain global values over those that don't.
 * 2. Choose old registers over new.
 *
 * NOTE: A register is clean if the value it contains has not changed since
 * it was loaded form the frame. Otherwise it is dirty. There is no need to
 * spill clean registers. Dirty registers has to be spilled.
 *
 * TODO: build use lists in CFG and choose the register on the basis of next
 * value use instead of the register age.
 *
 */
static int
use_cheapest_register(jit_gencode_t gen, _jit_regs_t *regs, _jit_regdesc_t *desc, int output)
{
	int type;
	int need_pair;
	int reg;
	int other_reg;
	int cost;
	int free_reg;
	int suitable_reg;
	int suitable_cost;
	int suitable_age;
	int index;

	if(desc)
	{
		type = get_register_type(desc->value, need_pair);
		if(!type)
		{
			return -1;
		}

		need_pair = _jit_regs_needs_long_pair(desc->value->type);
	}
	else
	{
		type = JIT_REG_WORD;
		need_pair = 0;
	}

	free_reg = -1;
	suitable_reg = -1;
	suitable_cost = 0;
	suitable_age = -1;
	for(reg = 0; reg < JIT_NUM_REGS; ++reg)
	{
		if((_jit_reg_info[reg].flags & type) == 0)
		{
			continue;
		}

		if((_jit_reg_info[reg].flags & JIT_REG_IN_STACK) != 0)
		{
			/* TODO: Support stack registers. */
			return -1;
		}

		if(need_pair)
		{
			other_reg = _jit_reg_info[reg].other_reg;
		}
		else
		{
			other_reg = -1;
		}

		if(jit_reg_is_used(gen->permanent, reg)
		   || jit_reg_is_used(gen->inhibit, reg)
		   || jit_reg_is_used(regs->assigned, reg))
		{
			continue;
		}
		if(other_reg >= 0)
		{
			if(jit_reg_is_used(gen->permanent, other_reg)
			   || jit_reg_is_used(gen->inhibit, other_reg)
			   || jit_reg_is_used(regs->assigned, other_reg))
			{
				continue;
			}
		}

		if(gen->contents[reg].num_values == 0
		   && !gen->contents[reg].is_long_end
		   && (other_reg < 0 || gen->contents[other_reg].num_values == 0))
		{
			free_reg = reg;
			break;
		}

		cost = 0;
		for(index = 0; index < gen->contents[reg].num_values; index++)
		{
			if(gen->contents[reg].values[index]->has_global_register)
			{
				if(!gen->contents[reg].values[index]->in_global_register)
				{
					cost += 1;
				}
			}
			else
			{
				if(!gen->contents[reg].values[index]->in_frame)
				{
					cost += 3;
				}
			}
		}
		if(other_reg >= 0)
		{
			for(index = 0; index < gen->contents[other_reg].num_values; index++)
			{
				if(gen->contents[other_reg].values[index]->has_global_register)
				{
					if(!gen->contents[other_reg].values[index]->in_global_register)
					{
						cost += 1;
					}
				}
				else
				{
					if(!gen->contents[other_reg].values[index]->in_frame)
					{
						cost += 3;
					}
				}
			}
		}

		if(suitable_reg == -1
		   || cost < suitable_cost
		   || (cost == suitable_cost && gen->contents[reg].age < suitable_age))
		{
			/* This is the oldest suitable register of this type */
			suitable_reg = reg;
			suitable_cost = cost;
			suitable_age = gen->contents[reg].age;
		}
	}

	if(desc && desc->value->in_register && free_reg < 0)
	{
		/* Case 2. */
		reg = desc->value->reg;
	}
	else if(free_reg >= 0)
	{
		/* Cases 1 and 3. */
		reg = free_reg;
	}
	else
	{
		/* Case 4. */
		reg = suitable_reg;
	}

	if(desc && reg >= 0)
	{
		if(need_pair)
		{
			other_reg = _jit_reg_info[reg].other_reg;
		}
		else
		{
			other_reg = -1;
		}

		desc->reg = reg;
		desc->other_reg = other_reg;
		set_register_bits(regs, desc, output);
	}

	return reg;
}

static void
set_register(jit_gencode_t gen, _jit_regdesc_t *desc, int still_in_frame)
{
	int reg, other_reg;
	int first_stack_reg;

	reg = desc->reg;
	other_reg = desc->other_reg;

	if(desc->value->has_global_register && desc->value->global_reg == reg)
	{
		desc->value->in_register = 0;
		desc->value->in_global_register = 1;
		return;
	}

	gen->contents[reg].values[0] = desc->value;
	gen->contents[reg].num_values = 1;
	gen->contents[reg].age = gen->current_age;
	gen->contents[reg].used_for_temp = 0;
	gen->contents[reg].is_long_end = 0;
	if(other_reg == -1)
	{
		gen->contents[reg].is_long_start = 0;
	}
	else
	{
		gen->contents[reg].is_long_start = 1;
		gen->contents[other_reg].num_values = 0;
		gen->contents[other_reg].age = gen->current_age;
		gen->contents[other_reg].used_for_temp = 0;
		gen->contents[other_reg].is_long_start = 0;
		gen->contents[other_reg].is_long_end = 1;
	}

	/* Set the stack mappings for this register */
	if((_jit_reg_info[reg].flags & JIT_REG_IN_STACK) != 0)
	{
		first_stack_reg = reg;
		while((_jit_reg_info[first_stack_reg].flags & JIT_REG_START_STACK) == 0)
		{
			--first_stack_reg;
		}
		gen->contents[reg].remap = first_stack_reg;
		gen->stack_map[first_stack_reg] = desc->reg;
	}

	/* Adjust the value to reflect that it is in "reg", and maybe the frame */
	desc->value->in_register = 1;
	if(desc->value->has_global_register)
		desc->value->in_global_register = still_in_frame;
	else
		desc->value->in_frame = still_in_frame;
	desc->value->reg = reg;
}

static void
load_single(jit_gencode_t gen, _jit_regdesc_t *desc)
{
	if(!desc->value)
	{
		return;
	}

	if(desc->value->in_register)
	{
		if(desc->value->reg != desc->reg)
		{
			if(gen->contents[desc->reg].num_values > 0)
			{
				spill_register(gen, desc->reg);
			}
			if(desc->other_reg >= 0 && gen->contents[desc->other_reg].num_values > 0)
			{
				spill_register(gen, desc->other_reg);
			}
			_jit_gen_load_value(gen, desc->reg, desc->other_reg, desc->value);
		}
	}
	else if(desc->value->in_global_register)
	{
		if(desc->value->global_reg != desc->reg)
		{
			if(gen->contents[desc->reg].num_values > 0)
			{
				spill_register(gen, desc->reg);
			}
			if(desc->other_reg >= 0 && gen->contents[desc->other_reg].num_values > 0)
			{
				spill_register(gen, desc->other_reg);
			}
			_jit_gen_load_value(gen, desc->reg, desc->other_reg, desc->value);
			set_register(gen, desc, 1);
		}
	}
	else
	{
		if(gen->contents[desc->reg].num_values > 0)
		{
			spill_register(gen, desc->reg);
		}
		if(desc->other_reg >= 0 && gen->contents[desc->other_reg].num_values > 0)
		{
			spill_register(gen, desc->other_reg);
		}
		_jit_gen_load_value(gen, desc->reg, desc->other_reg, desc->value);
		set_register(gen, desc, 1);
	}
}

static void
load_couple(jit_gencode_t gen, _jit_regdesc_t *desc1, _jit_regdesc_t *desc2)
{
	int reg2, other_reg2;

	if(!desc1->value || !desc1->value->in_register || desc1->value->reg == desc1->reg)
	{
		load_single(gen, desc2);
		load_single(gen, desc1);
	}
	else if(!desc2->value || !desc2->value->in_register || desc2->value->reg == desc2->reg)
	{
		load_single(gen, desc1);
		load_single(gen, desc2);
	}
	else
	{
		reg2 = desc2->value->reg;
		if(gen->contents[reg2].is_long_start)
		{
			other_reg2 = _jit_reg_info[reg2].other_reg;
		}
		else
		{
			other_reg2 = -1;
		}

		if(desc1->reg != reg2 && desc1->other_reg != reg2
		   && (other_reg2 < 0
		       || (desc1->reg != other_reg2 && desc1->other_reg != other_reg2)))
		{
			load_single(gen, desc1);
			load_single(gen, desc2);
		}
		else
		{
			load_single(gen, desc2);
			load_single(gen, desc1);
		}
	}
}

static void
load_triple(jit_gencode_t gen, _jit_regdesc_t *desc1, _jit_regdesc_t *desc2, _jit_regdesc_t *desc3)
{
	int reg1, other_reg1;
	int reg2, other_reg2;
	int reg3, other_reg3;

	if(!desc1->value || !desc1->value->in_register || desc1->value->reg == desc1->reg)
	{
		load_couple(gen, desc2, desc3);
		load_single(gen, desc1);
	}
	else if(!desc2->value || !desc2->value->in_register || desc2->value->reg == desc2->reg)
	{
		load_couple(gen, desc1, desc3);
		load_single(gen, desc2);
	}
	else if(!desc3->value || !desc3->value->in_register || desc3->value->reg == desc3->reg)
	{
		load_couple(gen, desc1, desc2);
		load_single(gen, desc3);
	}
	else
	{
		reg1 = desc1->value->reg;
		if(gen->contents[reg1].is_long_start)
		{
			other_reg1 = _jit_reg_info[reg1].other_reg;
		}
		else
		{
			other_reg1 = -1;
		}

		reg2 = desc2->value->reg;
		if(gen->contents[reg2].is_long_start)
		{
			other_reg2 = _jit_reg_info[reg2].other_reg;
		}
		else
		{
			other_reg2 = -1;
		}

		reg3 = desc3->value->reg;
		if(gen->contents[reg3].is_long_start)
		{
			other_reg3 = _jit_reg_info[reg3].other_reg;
		}
		else
		{
			other_reg3 = -1;
		}

		if(desc1->reg != reg2 && desc1->other_reg != reg2
		   && desc1->reg != reg3 && desc1->other_reg != reg3
		   && (other_reg2 < 0
		       || (desc1->reg != other_reg2 && desc1->other_reg != other_reg2))
		   && (other_reg3 < 0
		       || (desc1->reg != other_reg3 && desc1->other_reg != other_reg3)))
		{
			load_single(gen, desc1);
			load_couple(gen, desc2, desc3);
		}
		else if(desc2->reg != reg1 && desc2->other_reg != reg1
			&& desc2->reg != reg3 && desc2->other_reg != reg3
			&& (other_reg1 < 0
			    || (desc2->reg != other_reg1 && desc2->other_reg != other_reg1))
			&& (other_reg3 < 0
			    || (desc2->reg != other_reg3 && desc2->other_reg != other_reg3)))
		{
			load_single(gen, desc2);
			load_couple(gen, desc1, desc3);
		}
		else
		{
			load_single(gen, desc3);
			load_couple(gen, desc1, desc2);
		}
	}
}

void
_jit_regs_init(_jit_regs_t *regs, int is_ternary, int is_commutative)
{
	int i;

	regs->is_ternary = is_ternary;
	regs->is_commutative = is_commutative;

	for(i = 0; i < _JIT_REGS_VALUE_MAX; i++)
	{
		regs->descs[i].value = 0;
		regs->descs[i].reg = -1;
		regs->descs[i].other_reg = -1;
		regs->descs[i].clobber = 0;
		regs->descs[i].live = 0;
		regs->descs[i].used = 0;
	}
	regs->num_descs = 0;

	for(i = 0; i < _JIT_REGS_SCRATCH_MAX; i++)
	{
		regs->scratch[1] = -1;
	}
	regs->num_scratch = 0;

	regs->assigned = jit_regused_init;
	regs->clobber = jit_regused_init;
}

void
_jit_regs_set_dest(_jit_regs_t *regs, jit_insn_t insn, int clobber, int reg, int other_reg)
{
	if(regs->num_descs < 1)
	{
		regs->num_descs = 1;
	}

	regs->descs[0].value = insn->dest;
	if(reg >= 0)
	{
		regs->descs[0].reg = reg;
		regs->descs[0].other_reg = other_reg;
	}
	if(!regs->is_ternary || clobber)
	{
		regs->descs[0].clobber = 1;
	}
	if((insn->flags & JIT_INSN_DEST_LIVE) != 0)
	{
		regs->descs[0].live = 1;
	}
	if((insn->flags & JIT_INSN_DEST_NEXT_USE) != 0)
	{
		regs->descs[0].used = 1;
	}
}

void
_jit_regs_set_value1(_jit_regs_t *regs, jit_insn_t insn, int clobber, int reg, int other_reg)
{
	if(regs->num_descs < 2)
	{
		regs->num_descs = 2;
	}

	regs->descs[1].value = insn->value1;
	if(reg >= 0)
	{
		regs->descs[1].reg = reg;
		regs->descs[1].other_reg = other_reg;
	}
	if(clobber)
	{
		regs->descs[1].clobber = 1;
	}
	if((insn->flags & JIT_INSN_VALUE1_LIVE) != 0)
	{
		regs->descs[1].live = 1;
	}
	if((insn->flags & JIT_INSN_VALUE1_NEXT_USE) != 0)
	{
		regs->descs[1].used = 1;
	}
}

void
_jit_regs_set_value2(_jit_regs_t *regs, jit_insn_t insn, int clobber, int reg, int other_reg)
{
	if(regs->num_descs < 3)
	{
		regs->num_descs = 3;
	}

	regs->descs[2].value = insn->value2;
	if(reg >= 0)
	{
		regs->descs[2].reg = reg;
		regs->descs[2].other_reg = other_reg;
	}
	if(clobber)
	{
		regs->descs[2].clobber = 1;
	}
	if((insn->flags & JIT_INSN_VALUE2_LIVE) != 0)
	{
		regs->descs[2].live = 1;
	}
	if((insn->flags & JIT_INSN_VALUE2_NEXT_USE) != 0)
	{
		regs->descs[2].used = 1;
	}
}

void
_jit_regs_set_scratch(_jit_regs_t *regs, int reg)
{
	if(regs->num_scratch < _JIT_REGS_SCRATCH_MAX)
	{
		regs->scratch[regs->num_scratch++] = reg;
	}
}

void
_jit_regs_clobber(_jit_regs_t *regs, int reg)
{
	jit_reg_set_used(regs->clobber, reg);
}

void
_jit_regs_clobber_all(_jit_regs_t *regs)
{
	regs->clobber = jit_regused_init_used;
}

int
_jit_regs_dest(_jit_regs_t *regs)
{
	int reg;
	reg = regs->descs[0].reg;
	if(reg >= 0)
	{
		return _jit_reg_info[reg].cpu_reg;
	}
	return -1;
}

int
_jit_regs_value1(_jit_regs_t *regs)
{
	int reg;
	reg = regs->descs[1].reg;
	if(reg >= 0)
	{
		return _jit_reg_info[reg].cpu_reg;
	}
	return -1;
}

int
_jit_regs_value2(_jit_regs_t *regs)
{
	int reg;
	reg = regs->descs[2].reg;
	if(reg >= 0)
	{
		return _jit_reg_info[reg].cpu_reg;
	}
	return -1;
}

int
_jit_regs_dest_other(_jit_regs_t *regs)
{
	int reg;
	reg = regs->descs[0].other_reg;
	if(reg >= 0)
	{
		return _jit_reg_info[reg].cpu_reg;
	}
	return -1;
}

int
_jit_regs_value1_other(_jit_regs_t *regs)
{
	int reg;
	reg = regs->descs[1].other_reg;
	if(reg >= 0)
	{
		return _jit_reg_info[reg].cpu_reg;
	}
	return -1;
}

int
_jit_regs_value2_other(_jit_regs_t *regs)
{
	int reg;
	reg = regs->descs[2].other_reg;
	if(reg >= 0)
	{
		return _jit_reg_info[reg].cpu_reg;
	}
	return -1;
}

int
_jit_regs_scratch(_jit_regs_t *regs, int index)
{
	int reg;
	if(index < regs->num_scratch && index >= 0)
	{
		reg = regs->scratch[index];
		if(reg >= 0)
		{
			return _jit_reg_info[reg].cpu_reg;
		}
	}
	return -1;
}

int
_jit_regs_assign(jit_gencode_t gen, _jit_regs_t *regs)
{
	_jit_regdesc_t *desc1;
	_jit_regdesc_t *desc2;
	_jit_regdesc_t *desc3;
	_jit_regdesc_t tmp;
	int index;

	desc1 = &regs->descs[0];
	desc2 = &regs->descs[1];
	desc3 = &regs->descs[2];

	/* If the operation is not ternary its output clobbers the first input value. */
	if(!regs->is_ternary && desc1->value && desc2->value)
	{
		/* If the operation is commutative choose which value is cheaper to clobber. */
		if(regs->is_commutative && desc3->value)
		{
			if((desc2->value != desc1->value && desc3->value == desc1->value)
			   || (desc2->live && !desc3->live && !desc2->used && !desc3->used)
			   || (desc2->used && !desc3->used)
			   || (!desc2->value->in_frame && desc3->value->in_frame))
			{
				tmp = *desc2;
				*desc2 = *desc3;
				*desc3 = tmp;
			}
		}

		if(desc1->value != desc2->value)
		{
			desc2->clobber = 1;
		}
	}

	/* Process pre-assigned registers. */

	if(desc1->reg >= 0)
	{
		if(regs->is_ternary)
		{
			set_register_bits(regs, desc1, 0);
		}
		else
		{
			set_register_bits(regs, desc1, 1);
		}
	}
	if(desc2->reg >= 0)
	{
		set_register_bits(regs, desc2, 0);
	}
	if(desc3->reg >= 0)
	{
		set_register_bits(regs, desc3, 0);
	}

	for(index = 0; index < regs->num_scratch; index++)
	{
		if(regs->scratch[index] >= 0)
		{
			jit_reg_set_used(regs->assigned, regs->scratch[index]);
			jit_reg_set_used(regs->clobber, regs->scratch[index]);
		}
	}

	/* For values that are already in registers try to keep them there. */

	if(!regs->is_ternary && desc1->value && desc2->value)
	{
		if(desc1->reg < 0 && desc2->reg < 0)
		{
			/* If input value is in register and it will not be
			   used again we can save one move by placing output
			   value into that register. */
			if(!(desc2->live || desc2->used))
			{
				reuse_register(gen, regs, desc2, 0);
				if(desc2->reg >= 0)
				{
					desc1->reg = desc2->reg;
					desc1->other_reg = desc2->other_reg;
					set_register_bits(regs, desc1, 1);
				}
			}
			if(desc1->reg < 0)
			{
				reuse_register(gen, regs, desc1, 1);
				if(desc1->reg >= 0)
				{
					desc2->reg = desc1->reg;
					desc2->other_reg = desc1->other_reg;
					set_register_bits(regs, desc2, 0);
				}
			}
		}
	}
	else
	{
		if(desc1->value && desc1->reg < 0)
		{
			reuse_register(gen, regs, desc1, 0);
		}
		if(desc2->value && desc2->reg < 0)
		{
			reuse_register(gen, regs, desc2, 0);
		}
	}
	if(desc3->value && desc3->reg < 0)
	{
		reuse_register(gen, regs, desc3, 0);
	}

	/* Assign the remaining registers. */

	if(regs->is_ternary)
	{
		if(desc1->value && desc1->reg < 0)
		{
			use_cheapest_register(gen, regs, desc1, 0);
			if(desc1->reg < 0)
			{
				return 0;
			}
		}

		reuse_duplicate_value(desc1, desc2);
		reuse_duplicate_value(desc1, desc3);

		if(desc2->value && desc2->reg < 0)
		{
			use_cheapest_register(gen, regs, desc2, 0);
			if(desc2->reg < 0)
			{
				return 0;
			}
		}
	}
	else
	{
		if(desc1->value && desc1->reg < 0)
		{
			if(desc2->reg >= 0)
			{
				desc1->reg = desc2->reg;
				desc1->other_reg = desc2->other_reg;
				set_register_bits(regs, desc1, 1);
			}
			else
			{
				use_cheapest_register(gen, regs, desc1, 1);
				if(desc1->reg < 0)
				{
					return 0;
				}
			}
		}

		if(desc2->value && desc2->reg < 0)
		{
			if(desc1->reg >= 0)
			{
				desc2->reg = desc1->reg;
				desc2->other_reg = desc1->other_reg;
				set_register_bits(regs, desc2, 0);
			}
			else
			{
				use_cheapest_register(gen, regs, desc2, 0);
				if(desc2->reg < 0)
				{
					return 0;
				}
			}
		}
	}

	reuse_duplicate_value(desc2, desc3);

	if(desc3->value && desc3->reg < 0)
	{
		use_cheapest_register(gen, regs, desc3, 0);
		if(desc3->reg < 0)
		{
			return 0;
		}
	}

	for(index = 0; index < regs->num_scratch; index++)
	{
		if(regs->scratch[index] < 0)
		{
			regs->scratch[index] = use_cheapest_register(gen, regs, 0, 0);
			jit_reg_set_used(regs->assigned, regs->scratch[index]);
			jit_reg_set_used(regs->clobber, regs->scratch[index]);
		}
	}

	return 1;
}

int
_jit_regs_gen(jit_gencode_t gen, _jit_regs_t *regs)
{
	_jit_regdesc_t *desc1;
	_jit_regdesc_t *desc2;
	_jit_regdesc_t *desc3;
	int reg;

	desc1 = &regs->descs[0];
	desc2 = &regs->descs[1];
	desc3 = &regs->descs[2];

	/* Load values. */
	if(regs->is_ternary)
	{
		load_triple(gen, desc1, desc2, desc3);
	}
	else
	{
		if(desc1->value)
		{
			/* To avoid spilling the value that we are about to
			   change pretend that its current content is already
			   in the frame. The correct flags will be set by the
			   _jit_regs_commit() function. */
			if(desc1->value->has_global_register)
			{
				desc1->value->in_global_register = 1;
			}
			else
			{
				desc1->value->in_frame = 1;
			}
		}

		load_couple(gen, desc2, desc3);
	}

	/* Spill clobbered registers. */
	for(reg = 0; reg < JIT_NUM_REGS; reg++)
	{
		if(jit_reg_is_used(regs->clobber, reg))
		{
			if(jit_reg_is_used(gen->permanent, reg))
			{
				/* Oops, global register. */
				_jit_gen_spill_global(gen, gen->contents[reg].values[0]);
			}
			else
			{
				spill_register(gen, reg);
			}
		}
	}

	return 1;
}

void
_jit_regs_commit(jit_gencode_t gen, _jit_regs_t *regs)
{
	_jit_regdesc_t *desc1;
	int reg;

	desc1 = &regs->descs[0];

	/* If the output register is used in this basic block remember it.
	   Otherwise spill it. */
	if(!regs->is_ternary && desc1->value)
	{
		if(desc1->used)
		{
			set_register(gen, desc1, 0);
		}
		else
		{
			if(desc1->value->has_global_register)
			{
				if(desc1->value->global_reg != desc1->reg)
				{
					_jit_gen_spill_reg(gen, desc1->reg, desc1->other_reg, desc1->value);
				}
				desc1->value->in_register = 0;
				desc1->value->in_global_register = 1;
			}
			else
			{
				_jit_gen_spill_reg(gen, desc1->reg, desc1->other_reg, desc1->value);
				desc1->value->in_register = 0;
				desc1->value->in_frame = 1;
			}
		}

		jit_reg_clear_used(regs->clobber, desc1->reg);
		if(desc1->other_reg >= 0)
		{
			jit_reg_clear_used(regs->clobber, desc1->other_reg);
		}
	}

	/* Load clobbered global registers. */
	for(reg = 0; reg < JIT_NUM_REGS; reg++)
	{
		if(jit_reg_is_used(regs->clobber, reg) && jit_reg_is_used(gen->permanent, reg))
		{
			_jit_gen_load_global(gen, gen->contents[reg].values[0]);
		}
	}
}
