/*
 * jit-reg-alloc.h - Register allocation routines for the JIT.
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

#ifndef	_JIT_REG_ALLOC_H
#define	_JIT_REG_ALLOC_H

#include "jit-rules.h"

#ifdef	__cplusplus
extern	"C" {
#endif

void _jit_regs_init_for_block(jit_gencode_t gen);
int _jit_regs_needs_long_pair(jit_type_t type);
int _jit_regs_get_cpu(jit_gencode_t gen, int reg, int *other_reg);
void _jit_regs_spill_all(jit_gencode_t gen);
int _jit_regs_want_reg(jit_gencode_t gen, int reg, int for_long);
void _jit_regs_free_reg(jit_gencode_t gen, int reg, int value_used);
void _jit_regs_set_value
	(jit_gencode_t gen, int reg, jit_value_t value, int still_in_frame);
void _jit_regs_set_incoming(jit_gencode_t gen, int reg, jit_value_t value);
void _jit_regs_set_outgoing(jit_gencode_t gen, int reg, jit_value_t value);
int _jit_regs_is_top(jit_gencode_t gen, jit_value_t value);
int _jit_regs_is_top_two
	(jit_gencode_t gen, jit_value_t value1, jit_value_t value2);
int _jit_regs_load_value
	(jit_gencode_t gen, jit_value_t value, int destroy, int used_again);
int _jit_regs_dest_value(jit_gencode_t gen, jit_value_t value);
int _jit_regs_load_to_top
	(jit_gencode_t gen, jit_value_t value, int used_again, int type_reg);
int _jit_regs_load_to_top_two
	(jit_gencode_t gen, jit_value_t value, jit_value_t value2,
	 int used_again1, int used_again2, int type_reg);
void _jit_regs_load_to_top_three
	(jit_gencode_t gen, jit_value_t value, jit_value_t value2,
	 jit_value_t value3, int used_again1, int used_again2,
	 int used_again3, int type_reg);
int _jit_regs_num_used(jit_gencode_t gen, int type_reg);
int _jit_regs_new_top(jit_gencode_t gen, jit_value_t value, int type_reg);
void _jit_regs_force_out(jit_gencode_t gen, jit_value_t value, int is_dest);
void _jit_regs_alloc_global(jit_gencode_t gen, jit_function_t func);
void _jit_regs_get_reg_pair(jit_gencode_t gen, int not_this1, int not_this2,
						    int not_this3, int *reg, int *reg2);

#ifdef	__cplusplus
};
#endif

#endif	/* _JIT_REG_ALLOC_H */
