/*
 * cfg-tests.c - Simple CFG tests
 *
 * Copyright (C) 2020 Free Software Foundation
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

#include <jit/jit.h>
#include "unit-tests.h"

/* Make a block like

   x = INCOMING
   if INCOMING != 0 then goto .L0
   goto .L1
   .L0:
   x = 23
   .L1:
   return x

   Then, check that the optimized CFG removes the unnecessary block,
   inverting the condition.  */

static void test_block_removal(void)
{
	jit_init();
	jit_context_t ctx = jit_context_create ();

	jit_type_t params[1] = { jit_type_sys_int };
	jit_type_t sig = jit_type_create_signature (jit_abi_cdecl,
						    jit_type_sys_int,
						    params, 1, 1);

	jit_label_t l0 = jit_label_undefined;
	jit_label_t l1 = jit_label_undefined;

	jit_function_t func = jit_function_create (ctx, sig);
	jit_value_t incoming = jit_value_get_param (func, 0);

	jit_value_t x = jit_value_create (func, jit_type_int);
	jit_insn_store (func, x, incoming);

	jit_value_t zero = jit_value_create_nint_constant (func,
							   jit_type_sys_int,
							   0);
	jit_value_t compare = jit_insn_ne (func, x, zero);
	jit_block_t saved_block = jit_function_get_current (func);
	jit_insn_branch_if (func, compare, &l1);

	jit_insn_branch (func, &l0);

	jit_insn_label (func, &l1);
	jit_value_t twenty_three
	  = jit_value_create_nint_constant (func, jit_type_sys_int, 23);
	jit_insn_store (func, x, twenty_three);

	jit_insn_label (func, &l0);
	jit_insn_return (func, x);

	/* Test that optimization removes the unnecessary block.  We
	   do this by examining the instruction rather than the CFG,
	   because there didn't seem to be a reliable way to do the
	   latter.  */
	unsigned max = jit_function_get_max_optimization_level ();
	jit_function_set_optimization_level (func, max);
	// jit_dump_function (stderr, func, "test_block_removal");
	jit_function_compile (func);
	jit_insn_iter_t iter;
	jit_insn_iter_init_last (&iter, saved_block);
	jit_insn_t insn = jit_insn_iter_previous (&iter);
	CHECK (insn != NULL);
	CHECK (jit_insn_get_opcode (insn) == JIT_OP_BR_IEQ);

	/* Test that the result is still correct.  */
	int result = -1;
	int arg = 0;
	void *args[] = { &arg };
	CHECK (jit_function_apply (func, args, &result));
	CHECK (result == 0);

	arg = 72;
	CHECK (jit_function_apply (func, args, &result));
	CHECK (result == 23);
}

int main()
{
	test_block_removal ();

	return 0;
}
