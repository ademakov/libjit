/*
 * jit-rules-alpha.c - Rules that define the characteristics of the alpha.
 *
 * Copyright (C) 2006  Southern Storm Software, Pty Ltd.
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
 * Round a size up to a multiple of the stack word size.
 */
#define ROUND_STACK(size)       \
	(((size) + (sizeof(alpha_inst) - 1)) & ~(sizeof(alpha_inst) - 1))


/*
 * Setup or teardown the alpha code output process.
 */
#define jit_cache_setup_output(needed)			\
	alpha_inst inst = (alpha_inst) gen->ptr;	\
	_jit_gen_check_space(gen, (needed))

#define jit_cache_end_output()  \
	gen->ptr = (unsigned char*) inst

/*
 * Load the instruction pointer from the generation context.
 */
#define jit_gen_load_inst_ptr(gen,inst) \
	inst = (alpha_inst) (gen)->ptr;

/*
 * Save the instruction pointer back to the generation context.
 */
#define jit_gen_save_inst_ptr(gen,inst) \
	(gen)->ptr = (unsigned char *) inst;

static _jit_regclass_t *alpha_reg;
static _jit_regclass_t *alpha_freg;

/*
 * Initialize the backend. This is normally used to configure registers 
 * that may not appear on all CPU's in a given family. For example, only 
 * some ARM cores have floating-point registers.
 */
void _jit_init_backend(void) {
	alpha_reg = _jit_regclass_create("reg", JIT_REG_WORD | JIT_REG_LONG, 18,
		ALPHA_T0, ALPHA_T1, ALPHA_T2, ALPHA_T3,  ALPHA_T4,  ALPHA_T5,
		ALPHA_T6, ALPHA_T7, ALPHA_T8, ALPHA_T9, ALPHA_T10, ALPHA_T11,
		ALPHA_S0, ALPHA_S1, ALPHA_S2, ALPHA_S3,  ALPHA_S4,  ALPHA_S5
	);

	alpha_freg = _jit_regclass_create("freg", JIT_REG_FLOAT32 | JIT_REG_FLOAT64 | JIT_REG_NFLOAT, 8,
		ALPHA_FS0, ALPHA_FS1, ALPHA_FS2, ALPHA_FS3, ALPHA_FS4,
		ALPHA_FS5, ALPHA_FS6, ALPHA_FS7, ALPHA_FT0, ALPHA_FT1,
		ALPHA_FT2, ALPHA_FT3, ALPHA_FT4, ALPHA_FT5
	);
}

#define TODO()          \
        do { \
                fprintf(stderr, "TODO at %s, %d\n", __FILE__, (int)__LINE__); \
        } while (0)


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

	/* Compute and load the global pointer (2 instruction) */
        alpha_ldah(inst,ALPHA_GP,ALPHA_PV,0);
        alpha_lda( inst,ALPHA_GP,ALPHA_GP,0);

	/* Allocate space for a new stack frame. (1 instruction) */
	alpha_lda(inst,ALPHA_SP,ALPHA_SP,-(2*8));

	/* Save the return address. (1 instruction) */
	alpha_stq(inst,ALPHA_RA,ALPHA_SP,0*8);

	/* Save the frame pointer. (1 instruction) */
	alpha_stq(inst,ALPHA_FP,ALPHA_SP,1*8);

	/* Set the frame pointer (1 instruction) */
	alpha_mov(inst,ALPHA_SP,ALPHA_FP);

	/* Force any pending hardware exceptions to be raised. (1 instruction) */
	alpha_trapb(inst);

	/* Copy the prolog into place and return the entry position */
	jit_memcpy(buf, prolog, JIT_PROLOG_SIZE);
	return (void *) buf;
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
	void **fixup, **next;

	jit_cache_setup_output(20);

	/* Perform fixups on any blocks that jump to the epilog */
	fixup = (void **)(gen->epilog_fixup);
	while (fixup) {
		alpha_inst code = (alpha_inst) fixup;
		next     = (void **)(fixup[0]);

		_alpha_li64(code,ALPHA_AT,(long int)inst);
		alpha_jmp(code,ALPHA_ZERO,ALPHA_AT,1);

		fixup    = next;
	}

	/* Set the stack pointer (1 instruction) */
	alpha_mov(inst,ALPHA_FP,ALPHA_SP);

	/* Restore the return address. (1 instruction) */
	alpha_ldq(inst,ALPHA_RA,ALPHA_SP,0*8);

	/* Restore the frame pointer. (1 instruction) */
	alpha_ldq(inst,ALPHA_FP,ALPHA_SP,1*8);

	/* Restore the stack pointer (1 instruction) */
	alpha_lda(inst,ALPHA_SP,ALPHA_SP,16*8);

	/* Force any pending hardware exceptions to be raised. (1 instruction) */
	alpha_trapb(inst);

	/* Return from the current function (1 instruction) */
	alpha_ret(inst,ALPHA_RA,1);

	jit_cache_end_output();
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
	jit_type_t return_type;
	int ptr_return;

	return_type = jit_type_normalize(jit_type_get_return(signature));
	ptr_return  = jit_type_return_via_pointer(return_type);

	/* Bail out now if we don't need to worry about return values */
	if (!return_value || ptr_return) {
		return 0;
	}

	/*
	 * Structure values must be flushed into the frame, and
	 * everything else ends up in a register
	 */
	if (jit_type_is_struct(return_type) || jit_type_is_union(return_type)) {
		if (!jit_insn_flush_struct(func, return_value)) {
			return 0;
		}
	} else if (return_type->kind == JIT_TYPE_FLOAT32 || return_type->kind == JIT_TYPE_FLOAT64 || return_type->kind == JIT_TYPE_NFLOAT) {
		if (!jit_insn_return_reg(func, return_value, 32 /* fv0 */)) {
			return 0;
		}
	} else if (return_type->kind != JIT_TYPE_VOID) {
		if (!jit_insn_return_reg(func, return_value, 0 /* v0 */)) {
			return 0;
		}
	}

	/* Everything is back where it needs to be */
	return 1;
}

/*
 * Place the indirect function pointer value into a suitable register or 
 * stack location for a subsequent indirect call.
 */
int _jit_setup_indirect_pointer(jit_function_t func, jit_value_t value) {
	TODO();
	return 0;
}

void _jit_gen_exch_top(jit_gencode_t gen, int reg) {
	/* not used by alpha */;
}

void _jit_gen_move_top(jit_gencode_t gen, int reg) {
	/* not used by alpha */;
}

void _jit_gen_spill_top(jit_gencode_t gen, int reg, jit_value_t value, int pop) {
	/* not used by alpha */;
}

void _jit_gen_spill_global(jit_gencode_t gen, int reg, jit_value_t value) {
	/* not used by alpha */;
}

/*
 * Generate instructions to spill a pseudo register to the local variable frame. If 
 * other_reg is not -1, then it indicates the second register in a 64-bit register pair.
 *
 * This function will typically call _jit_gen_fix_value to fix the value's frame 
 * position, and will then generate the appropriate spill instructions. 
 */
void _jit_gen_spill_reg(jit_gencode_t gen, int reg, int other_reg, jit_value_t value) {
	int offset;

	/* Make sure that we have sufficient space */
	jit_cache_setup_output(32);

	/* If the value is associated with a global register, then copy to that */
	if (value->has_global_register) {
		alpha_mov(inst,_jit_reg_info[reg].cpu_reg,_jit_reg_info[value->global_reg].cpu_reg);
		jit_cache_end_output();
		return;
	}

	/* Fix the value in place within the local variable frame */
	_jit_gen_fix_value(value);

	/* Output an appropriate instruction to spill the value */
	offset = (int)(value->frame_offset);

	if (reg < 32) {		/* if integer register */
		alpha_stq(inst,reg,ALPHA_FP,offset);
		if (other_reg != -1) {
			offset += sizeof(void *);
			alpha_stq(inst,other_reg,ALPHA_FP,offset);
		}
	} else /* floating point register */ {
		/* TODO requires floating point support */
	}

	jit_cache_end_output();
	return;
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
	block->address = (void *)(gen->ptr);

	/* If this block has pending fixups, then apply them now */
	fixup = (void **)(block->fixup_list);

	while (fixup) {
		alpha_inst code = (alpha_inst) fixup;
		next     = (void **)(fixup[0]);

		_alpha_li64(code,ALPHA_AT,(long int)(gen->ptr));
		alpha_jmp(code,ALPHA_ZERO,ALPHA_AT,1);

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
	short int offset;

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

	} else {

		/* Fix the position of the value in the stack frame */
		_jit_gen_fix_value(value);
		offset = (int)(value->frame_offset);

		/* Load the value into the specified register */
		switch (jit_type_normalize(value->type)->kind) {

			case JIT_TYPE_SBYTE:
/* TODO add alpha_ldb		alpha_ldb(inst,_jit_reg_info[reg].cpu_reg,ALPHA_SP,offset);
 */				break;
			case JIT_TYPE_UBYTE:
				alpha_ldbu(inst,_jit_reg_info[reg].cpu_reg,ALPHA_SP,offset);
				break;
			case JIT_TYPE_SHORT:
/* TODO add alpha_ldw		alpha_ldw(inst,_jit_reg_info[reg].cpu_reg,ALPHA_SP,offset);
 */				break;
			case JIT_TYPE_USHORT:
				alpha_ldwu(inst,_jit_reg_info[reg].cpu_reg,ALPHA_SP,offset);
				break;
			case JIT_TYPE_INT:
				alpha_ldl(inst,_jit_reg_info[reg].cpu_reg,ALPHA_SP,offset);
				break;
			case JIT_TYPE_UINT:
/* TODO add alpha_ldlu		alpha_ldlu(inst,_jit_reg_info[reg].cpu_reg,ALPHA_SP,offset);
*/				break;
			case JIT_TYPE_LONG:
				alpha_ldq(inst,_jit_reg_info[reg].cpu_reg,ALPHA_SP,offset);
				break;
			case JIT_TYPE_ULONG:
/* TODO add alpha_ldqu		alpha_ldqu(inst,_jit_reg_info[reg].cpu_reg,ALPHA_SP,offset);
*/				break;

			/* TODO requires floating-point support */
			case JIT_TYPE_FLOAT32:
			case JIT_TYPE_FLOAT64:
			case JIT_TYPE_NFLOAT:
				break;
		}
	}

	jit_cache_end_output();
}

void _jit_gen_load_global(jit_gencode_t gen, int reg, jit_value_t value) {
	TODO();
}


/*
 * Generate code for a redirector, which makes an indirect jump to the contents of 
 * func->entry_point. Redirectors are used on recompilable functions in place of the 
 * regular entry point. This allows libjit to redirect existing calls to the new 
 * version after recompilation.
 */
void *_jit_gen_redirector(jit_gencode_t gen, jit_function_t func) {
	void *ptr, *entry;
	alpha_inst inst = (alpha_inst) gen->ptr;

	_jit_gen_check_space(gen, 8*6);

	ptr = (void *)&(func->entry_point);
	entry = gen->ptr;

	alpha_call(inst, ptr);

	return entry;
}

/*
 * Generate native code for the specified insn. This function should call the 
 * appropriate register allocation routines, output the instruction, and then arrange 
 * for the result to be placed in an appropriate register or memory destination.
 */
void _jit_gen_insn(jit_gencode_t gen, jit_function_t func, jit_block_t block, jit_insn_t insn) {

	switch (insn->opcode) {
		#define JIT_INCLUDE_RULES
			#include "jit-rules-alpha.inc"
		#undef JIT_INCLUDE_RULES

		default:
			fprintf(stderr, "TODO(%x) at %s, %d\n", (int)(insn->opcode), __FILE__, (int)__LINE__);
			break;
	}
}

void _jit_gen_fix_value(jit_value_t value) {

	if (!(value->has_frame_offset) && !(value->is_constant)) {
		jit_nint size = (jit_nint)(ROUND_STACK(jit_type_get_size(value->type)));
		value->block->func->builder->frame_size += size;
		value->frame_offset = -(value->block->func->builder->frame_size);
		value->has_frame_offset = 1;
	}
}

/*
 * Output a branch instruction.
 */
void alpha_output_branch(jit_function_t func, alpha_inst inst, int opcode, jit_insn_t insn, int reg) {
	jit_block_t block;

	if (!(block = jit_block_from_label(func, (jit_label_t)(insn->dest))))
		return;

	if (block->address) {
		/* We already know the address of the block */
		short offset = ((unsigned long) block->address - (unsigned long) inst);
		alpha_encode_branch(inst,opcode,reg,offset);

	} else {
		long *addr = (void*) inst;

		/* Output a placeholder and record on the block's fixup list */
		*addr = (long) block->fixup_list;
		inst++; inst++;

		_jit_pad_buffer((unsigned char*)inst,6);
	}
}

/*
 * Jump to the current function's epilog.
 */
void jump_to_epilog(jit_gencode_t gen, alpha_inst inst, jit_block_t block) {
	long *addr = (void*) inst;

	/*
	 * If the epilog is the next thing that we will output,
	 * then fall through to the epilog directly.
	 */
	if(_jit_block_is_final(block))
	{
		return;
	}

	/*
	 * fixups are slightly strange for the alpha port. On alpha you 
	 * cannot use an address stored in memory for jumps. The address 
	 * has to stored in a register.
	 *
	 * The fixups need the address stored in memory so that they can
	 * be 'fixed up' later. So what we do here is output the address
	 * and some nops. When it gets 'fixed up' we replace the address
	 * and 4 no-ops with opcodes to load the address into a register.
	 * Then we overwrite the last no-op with a jump opcode.
	 */

	/* Output a placeholder for the jump and add it to the fixup list */
	*addr = (long) gen->epilog_fixup;
	inst++; inst++;

	_jit_pad_buffer((unsigned char*)inst,6); /* to be overwritten later with jmp */

	(gen)->ptr = (unsigned char*) inst;
}

#endif /* JIT_BACKEND_ALPHA */
