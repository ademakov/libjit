/*
 * jit-rules-alpha.c - Rules that define the characteristics of the alpha.
 *
 * Copyright (C) 2006  Southern Storm Software, Pty Ltd.
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
#include "jit-rules.h"
#include "jit-apply-rules.h"

#if defined(JIT_BACKEND_ALPHA)

#include "jit-elf-defs.h"
#include "jit-gen-alpha.h"
#include "jit-reg-alloc.h"
#include "jit-setjmp.h"
#include <stdio.h>

/*
 * _alpha_has_ieeefp()
 *
 * When the Alpha architecture's floating point unit was first designed, 
 * the designers traded performance for functionality. As a result, all 
 * Alpha systems below EV6 do not fully implement the IEEE floating 
 * point standard. For those earlier systems, there is no hardware 
 * support for denormalized numbers or exceptional IEEE values like not 
 * a number and positive/negative infinity. For systems without hardware
 * support, the kernal can assist, but more you'll need to add 
 * instructions to trap into the kernel. Use this function to determine
 * if hardware ieeefp is available.
 *
 * To get the kernel to assist when needed, use the following code:
 *
 *	if (!_alpha_has_ieeefp())
 *		alpha_trapb(inst);
 *
 * RETURN VALUE:
 *  - TRUE if the CPU fully supports IEEE floating-point (i.e. >=ev6)
 *  - FALSE if the CPU needs kernel assistance
 */
int _alpha_has_ieeefp() {
        unsigned long __implver;

	/*
	 * __implver - major version number of the processor
	 *
	 * (__implver == 0)	ev4 class processors
	 * (__implver == 1)	ev5 class processors
	 * (__implver == 2)	ev6 class processors
	 */
        __asm__ ("implver %0" : "=r"(__implver));
	return (__implver >= 2);
}

/*
 * Setup or teardown the alpha code output process.
 */
#define jit_cache_setup_output(needed)				\
	alpha_inst inst = (alpha_inst) gen->posn.ptr;		\
	if(!jit_cache_check_for_n(&(gen->posn), (needed))) {	\
		jit_cache_mark_full(&(gen->posn));		\
		return;						\
	}							\

#define jit_cache_end_output()  \
	gen->posn.ptr = (char*) inst

/*
 * Initialize the backend. This is normally used to configure registers 
 * that may not appear on all CPU's in a given family. For example, only 
 * some ARM cores have floating-point registers.
 */
void _jit_init_backend(void) {
	/* Nothing to do here */;
}

/*
 * Get the ELF machine and ABI type information for this platform. The 
 * machine field should be set to one of the EM_* values in 
 * jit-elf-defs.h. The abi field should be set to one of the ELFOSABI_* 
 * values in jit-elf-defs.h (ELFOSABI_SYSV will normally suffice if you 
 * are unsure). The abi_version field should be set to the ABI version, 
 * which is usually zero.
 */
void _jit_gen_get_elf_info(jit_elf_info_t *info) {
	info->machine = EM_ALPHA;
	info->abi = ELFOSABI_SYSV;
	info->abi_version = 0;
}

/*
 * Generate the prolog for a function into a previously-prepared buffer 
 * area of JIT_PROLOG_SIZE bytes in size. Returns the start of the 
 * prolog, which may be different than buf.
 *
 * This function is called at the end of the code generation process, 
 * not the beginning. At this point, it is known which callee save 
 * registers must be preserved, allowing the back end to output the 
 * most compact prolog possible. 
 */
void *_jit_gen_prolog(jit_gencode_t gen, jit_function_t func, void *buf) {
	unsigned int prolog[JIT_PROLOG_SIZE];
	alpha_inst inst = prolog;
	short int savereg_space = 0;
	unsigned char reg;

	/* NOT IMPLEMENTED YET */

	/* Determine which registers need to be preserved and push them onto the stack */
	for (reg = 0; reg < 32; reg++) {
		if(jit_reg_is_used(gen->touched, reg) && (_jit_reg_info[reg].flags & JIT_REG_CALL_USED) == 0) {
			/* store the register value on the stack */
			alpha_stq(inst,ALPHA_SP,reg,savereg_space);
			savereg_space -= 8;
		}
	}

	/* adjust the stack pointer to point to the 'top' of the stack */
	alpha_li(inst,ALPHA_AT,savereg_space);
	alpha_addq(inst,ALPHA_SP,ALPHA_AT,ALPHA_SP);

	/* TODO see if ALPHA_RA needs to be saved or was saved above -----^ */

	/* Copy the prolog into place and return the adjusted entry position */
	reg = (int)(inst - prolog);
	jit_memcpy(((unsigned char *)buf) + JIT_PROLOG_SIZE - reg, prolog, reg);
	return (void *)(((unsigned char *)buf) + JIT_PROLOG_SIZE - reg);
}

/*
 * Generate a function epilog, restoring the registers that were saved on entry to the 
 * function, and then returning.
 *
 * Only one epilog is generated per function. Functions with multiple jit_insn_return 
 * instructions will all jump to the common epilog. This is needed because the code 
 * generator may not know which callee save registers need to be restored by the 
 * epilog until the full function has been processed. 
 */
void _jit_gen_epilog(jit_gencode_t gen, jit_function_t func) {
	short int savereg_space = 0;
	unsigned char reg;

	alpha_inst inst;
	void **fixup, **next;

	/* NOT IMPLEMENTED YET */;

	inst = (alpha_inst) gen->posn.ptr;

	/* Determine which registers need to be restored when we return and restore them */
	for (reg = 0; reg < 32; reg++) {
		if (jit_reg_is_used(gen->touched, reg) && (_jit_reg_info[reg].flags & JIT_REG_CALL_USED) == 0) {
			/* store the register value on the stack */
			alpha_ldq(inst,reg,ALPHA_SP,savereg_space);
			savereg_space += 8;
		}
	}

	/* adjust the stack pointer to point to the 'top' of the stack */
	alpha_li(inst,ALPHA_AT,savereg_space);
	alpha_addq(inst,ALPHA_SP,ALPHA_AT,ALPHA_SP);

	fixup = (void **)(gen->epilog_fixup);
	while (fixup) {
		next     = (void **)(fixup[0]);
		fixup[0] = (void*) ((jit_nint) inst - (jit_nint) fixup - 4);
		fixup    = next;
	}

	/* Return from the current function */
	alpha_ret(inst,ALPHA_RA,1);
}

/*
 * Create instructions within func to clean up after a function call and 
 * to place the function's result into return_value. This should use 
 * jit_insn_pop_stack to pop values off the system stack and 
 * jit_insn_return_reg to tell libjit which register contains the return 
 * value. In the case of a void function, return_value will be NULL.
 *
 * Note: the argument values are passed again because it may not be 
 * possible to determine how many bytes to pop from the stack from the 
 * signature alone; especially if the called function is vararg. 
 */
int _jit_create_call_return_insns(jit_function_t func, jit_type_t signature, jit_value_t *args, unsigned int num_args, jit_value_t return_value, int is_nested) {
	/* NOT IMPLEMENTED YET */
	return 0;
}

/*
 * Place the indirect function pointer value into a suitable register or 
 * stack location for a subsequent indirect call.
 */
int _jit_setup_indirect_pointer(jit_function_t func, jit_value_t value) {
	/* NOT IMPLEMENTED YET */
	return 0;
}

/*
 * TODO: write what this function is supposed to do
 */
void _jit_gen_spill_top(jit_gencode_t gen, int reg, jit_value_t value, int pop) {
	/* NOT IMPLEMENTED YET */;
}

/*
 * TODO: write what this function is supposed to do
 */
void _jit_gen_spill_global(jit_gencode_t gen, int reg, jit_value_t value) {
	/* NOT IMPLEMENTED YET */;
}

/*
 * Generate instructions to spill a pseudo register to the local variable frame. If 
 * other_reg is not -1, then it indicates the second register in a 64-bit register pair.
 *
 * This function will typically call _jit_gen_fix_value to fix the value's frame 
 * position, and will then generate the appropriate spill instructions. 
 */
void _jit_gen_spill_reg(jit_gencode_t gen, int reg, int other_reg, jit_value_t value) {
	/* NOT IMPLEMENTED YET */;
}

/*
 * Generate instructions to free a register without spilling its value. This is called 
 * when a register's contents become invalid, or its value is no longer required. If 
 * value_used is set to a non-zero value, then it indicates that the register's value 
 * was just used. Otherwise, there is a value in the register but it was never used.
 *
 * On most platforms, this function won't need to do anything to free the register. 
 * But some do need to take explicit action. For example, x86 needs an explicit 
 * instruction to remove a floating-point value from the FPU's stack if its value has 
 * not been used yet. 
 */
void _jit_gen_free_reg(jit_gencode_t gen, int reg, int other_reg, int value_used) {
	/* Nothing to do here */;
}

/*
 * Not all CPU's support all arithmetic, conversion, bitwise, or comparison operators 
 * natively. For example, most ARM platforms need to call out to helper functions to 
 * perform floating-point.
 *
 * If this function returns zero, then jit-insn.c will output a call to an intrinsic 
 * function that is equivalent to the desired opcode. This is how you tell libjit that 
 * you cannot handle the opcode natively.
 *
 * This function can also help you develop your back end incrementally. Initially, you 
 * can report that only integer operations are supported, and then once you have them 
 * working you can move on to the floating point operations. 
 *
 * Since alpha processors below ev6 need help with floating-point, we'll use the 
 * intrinsic floating-point functions on this systems.
 */
int _jit_opcode_is_supported(int opcode) {

	switch(opcode) {
		#define JIT_INCLUDE_SUPPORTED
		#include "jit-rules-alpha.inc"
		#undef JIT_INCLUDE_SUPPORTED
	}

	return 0;
}

/*
 * Determine if type is a candidate for allocation within global registers.
 */
int _jit_gen_is_global_candidate(jit_type_t type) {

	switch(jit_type_remove_tags(type)->kind) {
		case JIT_TYPE_INT:
		case JIT_TYPE_UINT:
		case JIT_TYPE_NINT:
		case JIT_TYPE_NUINT:
		case JIT_TYPE_PTR:
		case JIT_TYPE_SIGNATURE:
			return 1;
	}

	return 0;
}

/*
 * Called to notify the back end that the start of a basic block has been reached.
 */
void _jit_gen_start_block(jit_gencode_t gen, jit_block_t block) {
	void **fixup, **next;

	/* Set the address of this block */
	block->address = (void *)(gen->posn.ptr);

	/* If this block has pending fixups, then apply them now */
	fixup = (void **)(block->fixup_list);

	while (fixup) {
		next     = (void **)(fixup[0]);
		fixup[0] = (void*) ((jit_nint) block->address - (jit_nint) fixup - 4);
		fixup    = next;
	}

	block->fixup_list = 0;
}

/*
 * Called to notify the back end that the end of a basic block has been reached.
 */
void _jit_gen_end_block(jit_gencode_t gen, jit_block_t block) {
	/* Nothing to do here */;
}

/*
 * Generate instructions to load a value into a register. The value will either be a 
 * constant or a slot in the frame. You should fix frame slots with 
 * _jit_gen_fix_value.
 */
void _jit_gen_load_value(jit_gencode_t gen, int reg, int other_reg, jit_value_t value) {

	/* Make sure that we have sufficient space */
	jit_cache_setup_output(32);

	if (value->is_constant) {

		/* Determine the type of constant to be loaded */
		switch(jit_type_normalize(value->type)->kind) {

			case JIT_TYPE_SBYTE:
			case JIT_TYPE_UBYTE:
			case JIT_TYPE_SHORT:
			case JIT_TYPE_USHORT:
			case JIT_TYPE_INT:
			case JIT_TYPE_UINT:
			case JIT_TYPE_LONG:
			case JIT_TYPE_ULONG:
				alpha_li(inst,_jit_reg_info[reg].cpu_reg,(jit_nint)(value->address));
				break;

			/*
			TODO (needs floating-point
			case JIT_TYPE_FLOAT32:
			case JIT_TYPE_FLOAT64:
			case JIT_TYPE_NFLOAT:
				break;
			*/

			default:
				break;

		}
	} else if (value->in_register || value->in_global_register) {

		/* mov from value->reg to _jit_reg_info[reg].cpu_reg */
		alpha_mov(inst,value->reg,_jit_reg_info[reg].cpu_reg);

	} /* TODO else load from mem */

	jit_cache_end_output();
}

void _jit_gen_load_global(jit_gencode_t gen, int reg, jit_value_t value) {
/*
	NOT IMPLEMENTED YET!
	jit_cache_setup_output();
	jit_cache_end_output();
*/
}


/*
 * Generate code for a redirector, which makes an indirect jump to the contents of 
 * func->entry_point. Redirectors are used on recompilable functions in place of the 
 * regular entry point. This allows libjit to redirect existing calls to the new 
 * version after recompilation.
 */
void *_jit_gen_redirector(jit_gencode_t gen, jit_function_t func) {
	/* NOT IMPLEMENTED YET */
	return NULL;
}

/*
 * Generate native code for the specified insn. This function should call the 
 * appropriate register allocation routines, output the instruction, and then arrange 
 * for the result to be placed in an appropriate register or memory destination.
 */
void _jit_gen_insn(jit_gencode_t gen, jit_function_t func, jit_block_t block, jit_insn_t insn) {
	/* NOT IMPLEMENTED YET */;
}

/*
 * TODO: write what this function is supposed to do
 */
void _jit_gen_exch_top(jit_gencode_t gen, int reg, int pop) {
	/* NOT IMPLEMENTED YET */;
}

/*
 * Output a branch instruction.
 */
void alpha_output_branch(jit_function_t func, alpha_inst inst, int opcode, jit_insn_t insn, int reg) {
	jit_block_t block;
	short int offset;

	if (!(block = jit_block_from_label(func, (jit_label_t)(insn->dest))))
		return;

	if (block->address) {
		/* We already know the address of the block */

		offset = ((unsigned long) block->address - (unsigned long) inst);
		alpha_encode_branch(inst,opcode,reg,offset);

	} else {
		/* Output a placeholder and record on the block's fixup list */

		if(block->fixup_list) {
			offset = (short int) ((unsigned long int) inst - (unsigned long int) block->fixup_list);
		} else {
			offset = 0;
		}

		alpha_encode_branch(inst,opcode,reg,offset);
		block->fixup_list = inst - 4;
	}
}

/*
 * Jump to the current function's epilog.
 */
void alpha_jump_to_epilog(jit_gencode_t gen, alpha_inst inst, jit_block_t block) {
	short int offset;

	/*
	 * If the epilog is the next thing that we will output,
	 * then fall through to the epilog directly.
	 */

	block = block->next;

	while (!block && block->first_insn > block->last_insn)
		block = block->next;

	if (!block)
		return;

	/* Output a placeholder for the jump and add it to the fixup list */
	if (gen->epilog_fixup) {
		offset = (short int) ((unsigned long int) inst - (unsigned long int) gen->epilog_fixup);
	} else {
		offset = 0;
	}

	alpha_br(inst, ALPHA_ZERO, offset);
	gen->epilog_fixup = inst - 4;
}

#endif /* JIT_BACKEND_ALPHA */
