/*
 * jit-live.c - Liveness analysis for function bodies.
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

/*
 * Compute liveness information for a basic block.
 */
static void compute_liveness_for_block(jit_block_t block)
{
	jit_insn_iter_t iter;
	jit_insn_t insn;
	jit_value_t dest;
	jit_value_t value1;
	jit_value_t value2;
	int flags;

	/* Scan backwards to compute the liveness flags */
	jit_insn_iter_init_last(&iter, block);
	while((insn = jit_insn_iter_previous(&iter)) != 0)
	{
		/* Skip NOP instructions, which may have arguments left
		   over from when the instruction was replaced, but which
		   are not relevant to our liveness analysis */
		if(insn->opcode == JIT_OP_NOP)
		{
			continue;
		}

		/* Fetch the value parameters to this instruction */
		flags = insn->flags;
		if((flags & JIT_INSN_DEST_OTHER_FLAGS) == 0)
		{
			dest = insn->dest;
			if(dest && dest->is_constant)
			{
				dest = 0;
			}
		}
		else
		{
			dest = 0;
		}
		if((flags & JIT_INSN_VALUE1_OTHER_FLAGS) == 0)
		{
			value1 = insn->value1;
			if(value1 && value1->is_constant)
			{
				value1 = 0;
			}
		}
		else
		{
			value1 = 0;
		}
		if((flags & JIT_INSN_VALUE2_OTHER_FLAGS) == 0)
		{
			value2 = insn->value2;
			if(value2 && value2->is_constant)
			{
				value2 = 0;
			}
		}
		else
		{
			value2 = 0;
		}

		/* Record the liveness information in the instruction flags */
		flags &= ~JIT_INSN_LIVENESS_FLAGS;
		if(dest)
		{
			if(dest->live)
			{
				flags |= JIT_INSN_DEST_LIVE;
			}
			if(dest->next_use)
			{
				flags |= JIT_INSN_DEST_NEXT_USE;
			}
		}
		if(value1)
		{
			if(value1->live)
			{
				flags |= JIT_INSN_VALUE1_LIVE;
			}
			if(value1->next_use)
			{
				flags |= JIT_INSN_VALUE1_NEXT_USE;
			}
		}
		if(value2)
		{
			if(value2->live)
			{
				flags |= JIT_INSN_VALUE2_LIVE;
			}
			if(value2->next_use)
			{
				flags |= JIT_INSN_VALUE2_NEXT_USE;
			}
		}
		insn->flags = (short)flags;

		/* Set the destination to "not live, no next use" */
		if(dest)
		{
			if((flags & JIT_INSN_DEST_IS_VALUE) == 0)
			{
				if(!(dest->next_use) && !(dest->live))
				{
					/* There is no next use of this value and it is not
					   live on exit from the block.  So we can discard
					   the entire instruction as it will have no effect */
					insn->opcode = (short)JIT_OP_NOP;
					continue;
				}
				dest->live = 0;
				dest->next_use = 0;
			}
			else
			{
				/* The destination is actually a source value for this
				   instruction (e.g. JIT_OP_STORE_RELATIVE_*) */
				dest->live = 1;
				dest->next_use = 1;
			}
		}

		/* Set value1 and value2 to "live, next use" */
		if(value1)
		{
			value1->live = 1;
			value1->next_use = 1;
		}
		if(value2)
		{
			value2->live = 1;
			value2->next_use = 1;
		}
	}

	/* Re-scan the block to reset the liveness flags on all non-temporaries
	   because we need them in the original state for the next block */
	jit_insn_iter_init_last(&iter, block);
	while((insn = jit_insn_iter_previous(&iter)) != 0)
	{
		flags = insn->flags;
		if((flags & JIT_INSN_DEST_OTHER_FLAGS) == 0)
		{
			dest = insn->dest;
			if(dest && !(dest->is_constant) && !(dest->is_temporary))
			{
				dest->live = 1;
				dest->next_use = 0;
			}
		}
		if((flags & JIT_INSN_VALUE1_OTHER_FLAGS) == 0)
		{
			value1 = insn->value1;
			if(value1 && !(value1->is_constant) && !(value1->is_temporary))
			{
				value1->live = 1;
				value1->next_use = 0;
			}
		}
		if((flags & JIT_INSN_VALUE2_OTHER_FLAGS) == 0)
		{
			value2 = insn->value2;
			if(value2 && !(value2->is_constant) && !(value2->is_temporary))
			{
				value2->live = 1;
				value2->next_use = 0;
			}
		}
	}
}

void _jit_function_compute_liveness(jit_function_t func)
{
	jit_block_t block = func->builder->first_block;
	while(block != 0)
	{
		compute_liveness_for_block(block);
		block = block->next;
	}
}
