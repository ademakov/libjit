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


/*
 * New Reg Alloc API
 */

/*
 * The maximum number of values per instruction.
 */
#define _JIT_REGS_VALUE_MAX		3

/*
 * The maximum number of temporaries per instruction.
 */
#define _JIT_REGS_SCRATCH_MAX		6

/*
 * Flags for _jit_regs_init().
 */
#define _JIT_REGS_CLOBBER_ALL		0x0001
#define _JIT_REGS_TERNARY		0x0002
#define _JIT_REGS_BRANCH		0x0004
#define _JIT_REGS_COPY			0x0008
#define _JIT_REGS_STACK			0x0010
#define _JIT_REGS_X87_ARITH		0x0020
#define _JIT_REGS_COMMUTATIVE		0x0040
#define _JIT_REGS_REVERSIBLE		0x0080

/*
 * Flags for _jit_regs_init_dest(), _jit_regs_init_value1(), and
 * _jit_regs_init_value2().
 */
#define _JIT_REGS_CLOBBER		0x0001
#define _JIT_REGS_EARLY_CLOBBER		0x0002

/*
 * Flags returned by _jit_regs_select_insn().
 */
#define _JIT_REGS_NO_POP		0x0001
#define _JIT_REGS_REVERSE_DEST		0x0002
#define _JIT_REGS_REVERSE_ARGS		0x0004

/*
 * Contains register assignment data for single operand.
 */
typedef struct
{
	jit_value_t	value;
	int		reg;
	int		other_reg;
	int		stack_reg;
	jit_regused_t	regset;
	unsigned	live : 1;
	unsigned	used : 1;
	unsigned	clobber : 1;
	unsigned	early_clobber : 1;
	unsigned	duplicate : 1;
	unsigned	save : 1;
	unsigned	load : 1;
	unsigned	copy : 1;
	unsigned	kill : 1;

} _jit_regdesc_t;

/*
 * Contains scratch register assignment data.
 */
typedef struct
{
	int		reg;
	jit_regused_t	regset;

} _jit_scratch_t;

/*
 * Contains register assignment data for instruction.
 */
typedef struct
{
	unsigned	clobber_all : 1;
	unsigned	ternary : 1;
	unsigned	branch : 1;
	unsigned	copy : 1;
	unsigned	commutative : 1;
	unsigned	on_stack : 1;
	unsigned	x87_arith : 1;
	unsigned	reversible : 1;

	unsigned	no_pop : 1;
	unsigned	reverse_dest : 1;
	unsigned	reverse_args : 1;

	_jit_regdesc_t	descs[_JIT_REGS_VALUE_MAX];
	_jit_scratch_t	scratch[_JIT_REGS_SCRATCH_MAX];
	int		num_scratch;

	jit_regused_t	assigned;
	jit_regused_t	clobber;
	jit_regused_t	spill;

	int		stack_start;
	int		current_stack_top;
	int		wanted_stack_count;
	int		loaded_stack_count;
} _jit_regs_t;

void _jit_regs_init(_jit_regs_t *regs, int flags);
void _jit_regs_init_dest(_jit_regs_t *regs, jit_insn_t insn, int flags);
void _jit_regs_init_value1(_jit_regs_t *regs, jit_insn_t insn, int flags);
void _jit_regs_init_value2(_jit_regs_t *regs, jit_insn_t insn, int flags);

void _jit_regs_set_dest(_jit_regs_t *regs, int reg, int other_reg);
void _jit_regs_set_value1(_jit_regs_t *regs, int reg, int other_reg);
void _jit_regs_set_value2(_jit_regs_t *regs, int reg, int other_reg);
void _jit_regs_add_scratch(_jit_regs_t *regs, int reg);
void _jit_regs_set_clobber(_jit_regs_t *regs, int reg);

void _jit_regs_set_dest_from(_jit_regs_t *regs, jit_regused_t regset);
void _jit_regs_set_value1_from(_jit_regs_t *regs, jit_regused_t regset);
void _jit_regs_set_value2_from(_jit_regs_t *regs, jit_regused_t regset);
void _jit_regs_add_scratch_from(_jit_regs_t *regs, jit_regused_t regset);

int _jit_regs_assign(jit_gencode_t gen, _jit_regs_t *regs);
int _jit_regs_gen(jit_gencode_t gen, _jit_regs_t *regs);
int _jit_regs_select(_jit_regs_t *regs);
void _jit_regs_commit(jit_gencode_t gen, _jit_regs_t *regs);

int _jit_regs_dest(_jit_regs_t *regs);
int _jit_regs_value1(_jit_regs_t *regs);
int _jit_regs_value2(_jit_regs_t *regs);
int _jit_regs_dest_other(_jit_regs_t *regs);
int _jit_regs_value1_other(_jit_regs_t *regs);
int _jit_regs_value2_other(_jit_regs_t *regs);
int _jit_regs_scratch(_jit_regs_t *regs, int index);

int _jit_regs_lookup(char *name);

#ifdef	__cplusplus
};
#endif

#endif	/* _JIT_REG_ALLOC_H */
