/*
 * jit-rules-x86.c - Rules that define the characteristics of the x86.
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
#include "jit-rules.h"
#include "jit-apply-rules.h"

#if defined(JIT_BACKEND_X86)

#include "jit-gen-x86.h"

/*
 * Pseudo register numbers for the x86 registers.  These are not the
 * same as the CPU instruction register numbers.  The order of these
 * values must match the order in "JIT_REG_INFO".
 */
#define	X86_REG_EAX			0
#define	X86_REG_ECX			1
#define	X86_REG_EDX			2
#define	X86_REG_EBX			3
#define	X86_REG_ESI			4
#define	X86_REG_EDI			5
#define	X86_REG_EBP			6
#define	X86_REG_ESP			7
#define	X86_REG_ST0			8
#define	X86_REG_ST1			9
#define	X85_REG_ST2			10
#define	X86_REG_ST3			11
#define	X86_REG_ST4			12
#define	X86_REG_ST5			13
#define	X86_REG_ST6			14
#define	X86_REG_ST7			15

/*
 * Round a size up to a multiple of the stack word size.
 */
#define	ROUND_STACK(size)	\
		(((size) + (sizeof(void *) - 1)) & ~(sizeof(void *) - 1))

void _jit_init_backend(void)
{
	/* Nothing to do here for the x86 */
}

void _jit_gen_get_elf_info(jit_elf_info_t *info)
{
#ifdef JIT_NATIVE_INT32
	info->machine = 3;	/* EM_386 */
#else
	info->machine = 62;	/* EM_X86_64 */
#endif
#if JIT_APPLY_X86_FASTCALL == 0
	info->abi = 0;		/* ELFOSABI_SYSV */
#else
	info->abi = 186;	/* Private code, indicating STDCALL/FASTCALL support */
#endif
	info->abi_version = 0;
}

/*
 * Force values out of fastcall registers that cannot be easily
 * accessed in register form (i.e. long, float, and struct values).
 */
static int force_out_of_regs(jit_function_t func, jit_value_t param,
							 int num_regs, unsigned int num_stack_words)
{
	jit_value_t address;
	jit_value_t temp;
	jit_nint offset = 0;
	jit_nint frame_offset = 2 * sizeof(void *);

	/* Get the address of the parameter, to force it into the frame,
	   and to set up for the later "jit_insn_store_relative" calls */
	address = jit_insn_address_of(func, param);
	if(!address)
	{
		return 0;
	}

	/* Force the values out of the registers */
	while(num_regs > 0)
	{
		temp = jit_value_create(func, jit_type_void_ptr);
		if(!temp)
		{
			return 0;
		}
		if(num_regs == 2)
		{
			if(!jit_insn_incoming_reg(func, temp, X86_REG_ECX))
			{
				return 0;
			}
		}
		else
		{
			if(!jit_insn_incoming_reg(func, temp, X86_REG_EDX))
			{
				return 0;
			}
		}
		if(!jit_insn_store_relative(func, address, offset, temp))
		{
			return 0;
		}
		offset += sizeof(void *);
		--num_regs;
	}


	/* Force the rest of the value out of the incoming stack frame */
	while(num_stack_words > 0)
	{
		temp = jit_value_create(func, jit_type_void_ptr);
		if(!temp)
		{
			return 0;
		}
		if(!jit_insn_incoming_frame_posn(func, temp, frame_offset))
		{
			return 0;
		}
		if(!jit_insn_store_relative(func, address, offset, temp))
		{
			return 0;
		}
		offset += sizeof(void *);
		frame_offset += sizeof(void *);
		--num_stack_words;
	}
	return 1;
}

int _jit_create_entry_insns(jit_function_t func)
{
	jit_type_t signature = func->signature;
	jit_type_t type;
	int num_regs;
	jit_nint offset;
	jit_value_t value;
	unsigned int num_params;
	unsigned int param;
	unsigned int size;
	unsigned int num_stack_words;

	/* Reset the frame size for this function.  We start by assuming
	   that ESI, EDI, and EBX need to be saved in the local frame */
	func->builder->frame_size = 3 * sizeof(void *);

	/* Determine the number of registers to allocate to parameters */
#if JIT_APPLY_X86_FASTCALL == 1
	if(jit_type_get_abi(signature) == jit_abi_fastcall)
	{
		num_regs = 2;
	}
	else
#endif
	{
		num_regs = 0;
	}

	/* The starting parameter offset (saved ebp and return address on stack) */
	offset = 2 * sizeof(void *);

	/* If the function is nested, then we need an extra parameter
	   to pass the pointer to the parent's local variable frame */
	if(func->nested_parent)
	{
		if(num_regs > 0)
		{
			--num_regs;
		}
		else
		{
			offset += sizeof(void *);
		}
	}

	/* Allocate the structure return pointer */
	value = jit_value_get_struct_pointer(func);
	if(value)
	{
		if(num_regs == 2)
		{
			if(!jit_insn_incoming_reg(func, value, X86_REG_ECX))
			{
				return 0;
			}
			--num_regs;
		}
		else if(num_regs == 1)
		{
			if(!jit_insn_incoming_reg(func, value, X86_REG_EDX))
			{
				return 0;
			}
			--num_regs;
		}
		else
		{
			if(!jit_insn_incoming_frame_posn(func, value, offset))
			{
				return 0;
			}
			offset += sizeof(void *);
		}
	}

	/* Allocate the parameter offsets */
	num_params = jit_type_num_params(signature);
	for(param = 0; param < num_params; ++param)
	{
		value = jit_value_get_param(func, param);
		if(!value)
		{
			continue;
		}
		type = jit_type_normalize(jit_value_get_type(value));
		switch(type->kind)
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
				if(num_regs == 2)
				{
					if(!jit_insn_incoming_reg(func, value, X86_REG_ECX))
					{
						return 0;
					}
					--num_regs;
				}
				else if(num_regs == 1)
				{
					if(!jit_insn_incoming_reg(func, value, X86_REG_EDX))
					{
						return 0;
					}
					--num_regs;
				}
				else
				{
					if(!jit_insn_incoming_frame_posn(func, value, offset))
					{
						return 0;
					}
					offset += sizeof(void *);
				}
			}
			break;

			case JIT_TYPE_LONG:
			case JIT_TYPE_ULONG:
			case JIT_TYPE_FLOAT32:
			case JIT_TYPE_FLOAT64:
			case JIT_TYPE_NFLOAT:
			case JIT_TYPE_STRUCT:
			case JIT_TYPE_UNION:
			{
				/* Deal with the possibility that the value may be split
				   between registers and the stack */
				size = ROUND_STACK(jit_type_get_size(type));
				if(num_regs == 2)
				{
					if(size <= sizeof(void *))
					{
						if(!force_out_of_regs(func, value, 1, 0))
						{
							return 0;
						}
						--num_regs;
					}
					else
					{
						num_stack_words = (size / sizeof(void *)) - 2;
						if(!force_out_of_regs(func, value, 2, num_stack_words))
						{
							return 0;
						}
						num_regs = 0;
						offset += num_stack_words * sizeof(void *);
					}
				}
				else if(num_regs == 1)
				{
					num_stack_words = (size / sizeof(void *)) - 1;
					if(!force_out_of_regs(func, value, 1, num_stack_words))
					{
						return 0;
					}
					num_regs = 0;
					offset += num_stack_words * sizeof(void *);
				}
				else
				{
					if(!jit_insn_incoming_frame_posn(func, value, offset))
					{
						return 0;
					}
					offset += size;
				}
			}
			break;
		}
	}
	return 1;
}

int _jit_create_call_setup_insns
	(jit_function_t func, jit_type_t signature,
	 jit_value_t *args, unsigned int num_args,
	 int is_nested, int nesting_level, jit_value_t *struct_return)
{
	jit_type_t type = jit_type_get_return(signature);
	jit_value_t value;
	unsigned int size;
	unsigned int index;
	unsigned int num_stack_args;
	unsigned int word_regs;
	jit_value_t partial;

	/* Determine which values are going to end up in fastcall registers */
#if JIT_APPLY_X86_FASTCALL == 1
	if(jit_type_get_abi(signature) == jit_abi_fastcall)
	{
		word_regs = 0;
		if(func->nested_parent)
		{
			++word_regs;
		}
		if(jit_type_return_via_pointer(type))
		{
			++word_regs;
		}
		index = 0;
		partial = 0;
		while(index < num_args && word_regs < 2)
		{
			size = jit_type_get_size(jit_value_get_type(args[index]));
			size = ROUND_STACK(size) / sizeof(void *);
			if(size <= (2 - word_regs))
			{
				/* This argument will fit entirely in registers */
				word_regs += size;
				++index;
			}
			else
			{
				/* Partly in registers and partly on the stack.
				   We first copy it into a buffer that we can address */
				partial = jit_value_create
					(func, jit_value_get_type(args[index]));
				if(!partial)
				{
					return 0;
				}
				jit_value_set_addressable(partial);
				if(!jit_insn_store(func, partial, args[index]))
				{
					return 0;
				}
				++index;
				break;
			}
		}
		num_stack_args = num_args - index;
	}
	else
#endif
	{
		word_regs = 0;
		partial = 0;
		num_stack_args = num_args;
	}

	/* Push all of the stacked arguments in reverse order */
	while(num_stack_args > 0)
	{
		--num_stack_args;
		--num_args;
		if(!jit_insn_push(func, args[num_args]))
		{
			return 0;
		}
	}

	/* Handle a value that is partly on the stack and partly in registers */
	if(partial)
	{
		--num_args;
		index = (2 - word_regs) * sizeof(void *);
		size = ROUND_STACK(jit_type_get_size(jit_value_get_type(partial)));
		while(size > index)
		{
			size -= sizeof(void *);
			value = jit_value_create(func, jit_type_void_ptr);
			if(!value)
			{
				return 0;
			}
			value = jit_insn_load_relative
				(func, value, (jit_nint)size, jit_type_void_ptr);
			if(!value)
			{
				return 0;
			}
			if(!jit_insn_push(func, value))
			{
				return 0;
			}
		}
		while(size > 0)
		{
			size -= sizeof(void *);
			value = jit_value_create(func, jit_type_void_ptr);
			if(!value)
			{
				return 0;
			}
			value = jit_insn_load_relative
				(func, value, (jit_nint)size, jit_type_void_ptr);
			if(!value)
			{
				return 0;
			}
			if(word_regs == 2)
			{
				if(!jit_insn_outgoing_reg(func, value, X86_REG_EDX))
				{
					return 0;
				}
				--word_regs;
			}
			else
			{
				if(!jit_insn_outgoing_reg(func, value, X86_REG_ECX))
				{
					return 0;
				}
				--word_regs;
			}
		}
	}

	/* Push arguments that will end up entirely in registers */
	while(num_args > 0)
	{
		--num_args;
		size = jit_type_get_size(jit_value_get_type(args[num_args]));
		size = ROUND_STACK(size) / sizeof(void *);
		if(size == 2)
		{
			if(!jit_insn_outgoing_reg(func, args[num_args], X86_REG_ECX))
			{
				return 0;
			}
			word_regs = 0;
		}
		else if(word_regs == 2)
		{
			if(!jit_insn_outgoing_reg(func, args[num_args], X86_REG_EDX))
			{
				return 0;
			}
			--word_regs;
		}
		else
		{
			if(!jit_insn_outgoing_reg(func, args[num_args], X86_REG_ECX))
			{
				return 0;
			}
			--word_regs;
		}
	}

	/* Do we need to add a structure return pointer argument? */
	if(jit_type_return_via_pointer(type))
	{
		value = jit_value_create(func, type);
		if(!value)
		{
			return 0;
		}
		*struct_return = value;
		value = jit_insn_address_of(func, value);
		if(!value)
		{
			return 0;
		}
		if(word_regs == 2)
		{
			if(!jit_insn_outgoing_reg(func, value, X86_REG_EDX))
			{
				return 0;
			}
			--word_regs;
		}
		else if(word_regs == 1)
		{
			if(!jit_insn_outgoing_reg(func, value, X86_REG_ECX))
			{
				return 0;
			}
			--word_regs;
		}
		else
		{
			if(!jit_insn_push(func, value))
			{
				return 0;
			}
		}
	}
	else
	{
		*struct_return = 0;
	}

	/* Do we need to add nested function scope information? */
	if(is_nested)
	{
		if(word_regs > 0)
		{
			if(!jit_insn_setup_for_nested(func, nesting_level, X86_REG_ECX))
			{
				return 0;
			}
		}
		else
		{
			if(!jit_insn_setup_for_nested(func, nesting_level, -1))
			{
				return 0;
			}
		}
	}

	/* The call is ready to proceed */
	return 1;
}

int _jit_setup_indirect_pointer(jit_function_t func, jit_value_t value)
{
	return jit_insn_outgoing_reg(func, value, X86_REG_EAX);
}

int _jit_create_call_return_insns
	(jit_function_t func, jit_type_t signature,
	 jit_value_t *args, unsigned int num_args,
	 jit_value_t return_value, int is_nested)
{
	jit_nint pop_bytes;
	unsigned int size;
	jit_type_t return_type;
	int ptr_return;

	/* Calculate the number of bytes that we need to pop */
	return_type = jit_type_normalize(jit_type_get_return(signature));
	ptr_return = jit_type_return_via_pointer(return_type);
#if JIT_APPLY_X86_FASTCALL == 1
	if(jit_type_get_abi(signature) == jit_abi_stdcall ||
	   jit_type_get_abi(signature) == jit_abi_fastcall)
	{
		/* STDCALL and FASTCALL functions pop their own arguments */
		pop_bytes = 0;
	}
	else
#endif
	{
		pop_bytes = 0;
		while(num_args > 0)
		{
			--num_args;
			size = jit_type_get_size(jit_value_get_type(args[num_args]));
			pop_bytes += ROUND_STACK(size);
		}
#if JIT_APPLY_X86_POP_STRUCT_RETURN == 1
		if(ptr_return && is_nested)
		{
			/* Note: we only need this for nested functions, because
			   regular functions will pop the structure return for us */
			pop_bytes += sizeof(void *);
		}
#else
		if(ptr_return)
		{
			pop_bytes += sizeof(void *);
		}
#endif
		if(is_nested)
		{
			pop_bytes += sizeof(void *);
		}
	}

	/* Pop the bytes from the system stack */
	if(pop_bytes > 0)
	{
		if(!jit_insn_pop_stack(func, pop_bytes))
		{
			return 0;
		}
	}

	/* Bail out now if we don't need to worry about return values */
	if(!return_value || ptr_return)
	{
		return 0;
	}

	/* Structure values must be flushed into the frame, and
	   everything else ends up in a register */
	if(jit_type_is_struct(return_type) || jit_type_is_union(return_type))
	{
		if(!jit_insn_flush_struct(func, return_value))
		{
			return 0;
		}
	}
	else if(return_type == jit_type_float32 ||
			return_type == jit_type_float64 ||
			return_type == jit_type_nfloat)
	{
		if(!jit_insn_return_reg(func, return_value, X86_REG_ST0))
		{
			return 0;
		}
	}
	else if(return_type->kind != JIT_TYPE_VOID)
	{
		if(!jit_insn_return_reg(func, return_value, X86_REG_EAX))
		{
			return 0;
		}
	}

	/* Everything is back where it needs to be */
	return 1;
}

int _jit_opcode_is_supported(int opcode)
{
	switch(opcode)
	{
		/* TODO: float and long opcodes */

		/*
		 * Conversion opcodes.
		 */
		case JIT_OP_TRUNC_SBYTE:
		case JIT_OP_TRUNC_UBYTE:
		case JIT_OP_TRUNC_SHORT:
		case JIT_OP_TRUNC_USHORT:

		/*
		 * Arithmetic opcodes.
		 */
		case JIT_OP_IADD:
		case JIT_OP_ISUB:
		case JIT_OP_IMUL:
		case JIT_OP_INEG:

		/*
		 * Bitwise opcodes.
		 */
		case JIT_OP_IAND:
		case JIT_OP_IOR:
		case JIT_OP_IXOR:
		case JIT_OP_INOT:
		case JIT_OP_ISHL:
		case JIT_OP_ISHR:
		case JIT_OP_ISHR_UN:

		/*
		 * Branch opcodes.
		 */
		case JIT_OP_BR:
		case JIT_OP_BR_IFALSE:
		case JIT_OP_BR_ITRUE:
		case JIT_OP_BR_IEQ:
		case JIT_OP_BR_INE:
		case JIT_OP_BR_ILT:
		case JIT_OP_BR_ILT_UN:
		case JIT_OP_BR_ILE:
		case JIT_OP_BR_ILE_UN:
		case JIT_OP_BR_IGT:
		case JIT_OP_BR_IGT_UN:
		case JIT_OP_BR_IGE:
		case JIT_OP_BR_IGE_UN:			return 1;
	}
	return 0;
}

void *_jit_gen_prolog(jit_gencode_t gen, jit_function_t func, void *buf)
{
	unsigned char prolog[JIT_PROLOG_SIZE];
	unsigned char *inst = prolog;
	int reg;
	unsigned int saved;

	/* Push ebp onto the stack */
	x86_push_reg(inst, X86_EBP);

	/* Initialize EBP for the current frame */
	x86_mov_reg_reg(inst, X86_EBP, X86_ESP, sizeof(void *));

	/* Save registers that we need to preserve */
	saved = 0;
	for(reg = 0; reg <= 7; ++reg)
	{
		if(jit_reg_is_used(gen->touched, reg) &&
		   (_jit_reg_info[reg].flags & JIT_REG_CALL_USED) == 0)
		{
			x86_push_reg(inst, _jit_reg_info[reg].cpu_reg);
			saved += sizeof(void *);
		}
	}

	/* Allocate space for the local variable frame.  Subtract off
	   the space for the registers that we just saved */
	if((func->builder->frame_size - saved) > 0)
	{
		x86_alu_reg_imm(inst, X86_SUB, X86_ESP,
			 			(int)(func->builder->frame_size - saved));
	}

	/* Copy the prolog into place and return the adjusted entry position */
	reg = (int)(inst - prolog);
	jit_memcpy(((unsigned char *)buf) + JIT_PROLOG_SIZE - reg, prolog, reg);
	return (void *)(((unsigned char *)buf) + JIT_PROLOG_SIZE - reg);
}

void _jit_gen_epilog(jit_gencode_t gen, jit_function_t func)
{
	jit_nint pop_bytes = 0;
	int num_regs, reg;
	unsigned char *inst;
	int struct_return_offset = 0;

	/* Bail out if there is insufficient space for the epilog */
	if(!jit_cache_check_for_n(&(gen->posn), 32))
	{
		jit_cache_mark_full(&(gen->posn));
		return;
	}

#if JIT_APPLY_X86_FASTCALL == 1
	/* Determine the number of parameter bytes to pop when we return */
	{
		jit_type_t signature;
		unsigned int num_params;
		unsigned int param;
		signature = func->signature;
		if(jit_type_abi(signature) == jit_type_stdcall ||
		   jit_type_abi(signature) == jit_type_fastcall)
		{
			if(func->nested_parent)
			{
				pop_bytes += sizeof(void *);
			}
			if(jit_type_return_via_pointer(jit_type_get_return(signature)))
			{
				struct_return_offset = 2 * sizeof(void *) + pop_bytes;
				pop_bytes += sizeof(void *);
			}
			num_params = jit_type_num_params(signature);
			for(param = 0; param < num_params; ++param)
			{
				pop_bytes += ROUND_STACK
						(jit_type_get_size
							(jit_type_get_param(signature, param)));
			}
			if(jit_type_abi(signature) == jit_type_fastcall)
			{
				/* The first two words are in fastcall registers */
				if(pop_bytes > (2 * sizeof(void *)))
				{
					pop_bytes -= 2 * sizeof(void *);
				}
				else
				{
					pop_bytes = 0;
				}
				struct_return_offset = 0;
			}
		}
		else if(!(func->nested_parent) &&
				jit_type_return_via_pointer(jit_type_get_return(signature)))
		{
#if JIT_APPLY_X86_POP_STRUCT_RETURN == 1
			pop_bytes += sizeof(void *);
#endif
			struct_return_offset = 2 * sizeof(void *);
		}
	}
#else
	{
		/* We only need to pop structure pointers in non-nested functions */
		jit_type_t signature;
		signature = func->signature;
		if(!(func->nested_parent) &&
		   jit_type_return_via_pointer(jit_type_get_return(signature)))
		{
#if JIT_APPLY_X86_POP_STRUCT_RETURN == 1
			pop_bytes += sizeof(void *);
#endif
			struct_return_offset = 2 * sizeof(void *);
		}
	}
#endif

	/* If we are returning a structure via a pointer, then copy
	   the pointer value into EAX when we return */
	if(struct_return_offset != 0)
	{
		x86_mov_reg_membase(inst, X86_EAX, X86_EBP, struct_return_offset, 4);
	}

	/* Determine the number of callee save registers on the stack */
	num_regs = 0;
	for(reg = 7; reg >= 0; --reg)
	{
		if(jit_reg_is_used(gen->touched, reg) &&
		   (_jit_reg_info[reg].flags & JIT_REG_CALL_USED) == 0)
		{
			++num_regs;
		}
	}

	/* Pop the local stack frame, and get back to the callee save regs */
	inst = gen->posn.ptr;
	if(num_regs == 0)
	{
		x86_mov_reg_reg(inst, X86_ESP, X86_EBP, sizeof(void *));
	}
	else
	{
		x86_lea_membase(inst, X86_ESP, X86_EBP, -(sizeof(void *) * num_regs));
	}

	/* Pop the callee save registers that we used */
	for(reg = 7; reg >= 0; --reg)
	{
		if(jit_reg_is_used(gen->touched, reg) &&
		   (_jit_reg_info[reg].flags & JIT_REG_CALL_USED) == 0)
		{
			x86_pop_reg(inst, _jit_reg_info[reg].cpu_reg);
		}
	}

	/* Pop the saved copy of ebp */
	x86_pop_reg(inst, X86_EBP);

	/* Return from the current function */
	if(pop_bytes > 0)
	{
		x86_ret_imm(inst, pop_bytes);
	}
	else
	{
		x86_ret(inst);
	}
	gen->posn.ptr = inst;
}

void *_jit_gen_redirector(jit_gencode_t gen, jit_function_t func)
{
	void *ptr, *entry;
	if(!jit_cache_check_for_n(&(gen->posn), 8))
	{
		jit_cache_mark_full(&(gen->posn));
		return 0;
	}
	ptr = (void *)&(func->entry_point);
	entry = gen->posn.ptr;
	x86_jump_mem(gen->posn.ptr, ptr);
	return entry;
}

/*
 * Setup or teardown the x86 code output process.
 */
#define	jit_cache_setup_output(needed)	\
	unsigned char *inst = gen->posn.ptr; \
	if(!jit_cache_check_for_n(&(gen->posn), (needed))) \
	{ \
		jit_cache_mark_full(&(gen->posn)); \
		return; \
	}
#define	jit_cache_end_output()	\
	gen->posn.ptr = inst

void _jit_gen_spill_reg(jit_gencode_t gen, int reg,
						int other_reg, jit_value_t value)
{
	int offset;

	/* Make sure that we have sufficient space */
	jit_cache_setup_output(16);

	/* Fix the value in place within the local variable frame */
	_jit_gen_fix_value(value);

	/* Output an appropriate instruction to spill the value */
	offset = (int)(value->frame_offset);
	if(reg < X86_REG_ST0)
	{
		/* Spill a word register.  If the value is smaller than a word,
		   then we write the entire word.  The local stack frame is
		   allocated such that the extra bytes will be simply ignored */
		reg = _jit_reg_info[reg].cpu_reg;
		x86_mov_membase_reg(inst, X86_EBP, offset, reg, 4);
		if(other_reg != -1)
		{
			/* Spill the other word register in a pair */
			reg = _jit_reg_info[other_reg].cpu_reg;
			offset += sizeof(void *);
			x86_mov_membase_reg(inst, X86_EBP, offset, reg, 4);
		}
	}
	else
	{
		/* Spill the top of the floating-point register stack */
		switch(jit_type_normalize(value->type)->kind)
		{
			case JIT_TYPE_FLOAT32:
			{
				x86_fst_membase(inst, X86_EBP, offset, 0, 1);
			}
			break;

			case JIT_TYPE_FLOAT64:
			{
				x86_fst_membase(inst, X86_EBP, offset, 1, 1);
			}
			break;

			case JIT_TYPE_NFLOAT:
			{
				x86_fst80_membase(inst, X86_EBP, offset);
			}
			break;
		}
	}

	/* End the code output process */
	jit_cache_end_output();
}

void _jit_gen_free_reg(jit_gencode_t gen, int reg,
					   int other_reg, int value_used)
{
	/* We only need to take explicit action if we are freeing a
	   floating-point register whose value hasn't been used yet */
	if(!value_used && reg >= X86_REG_ST0 && reg <= X86_REG_ST7)
	{
		if(jit_cache_check_for_n(&(gen->posn), 2))
		{
			x86_fstp(gen->posn.ptr, reg - X86_REG_ST0);
		}
		else
		{
			jit_cache_mark_full(&(gen->posn));
		}
	}
}

void _jit_gen_load_value
	(jit_gencode_t gen, int reg, int other_reg, jit_value_t value)
{
	void *ptr;
	int offset;

	/* Make sure that we have sufficient space */
	jit_cache_setup_output(16);

	if(value->is_constant)
	{
		/* Determine the type of constant to be loaded */
		switch(jit_type_normalize(value->type)->kind)
		{
			case JIT_TYPE_SBYTE:
			case JIT_TYPE_UBYTE:
			case JIT_TYPE_SHORT:
			case JIT_TYPE_USHORT:
			case JIT_TYPE_INT:
			case JIT_TYPE_UINT:
			{
				x86_mov_reg_imm(inst, _jit_reg_info[reg].cpu_reg,
								(jit_nint)(value->address));
			}
			break;

			case JIT_TYPE_LONG:
			case JIT_TYPE_ULONG:
			{
				jit_long long_value;
				long_value = jit_value_get_long_constant(value);
			#ifdef JIT_NATIVE_INT64
				x86_mov_reg_imm(inst, _jit_reg_info[reg].cpu_reg,
								(jit_nint)long_value);
			#else
				x86_mov_reg_imm(inst, _jit_reg_info[reg].cpu_reg,
								(jit_int)long_value);
				x86_mov_reg_imm(inst, _jit_reg_info[other_reg].cpu_reg,
								(jit_int)(long_value >> 32));
			#endif
			}
			break;

			case JIT_TYPE_FLOAT32:
			{
				jit_float32 float32_value;
				float32_value = jit_value_get_float32_constant(value);
				if(!jit_cache_check_for_n(&(gen->posn), 32))
				{
					jit_cache_mark_full(&(gen->posn));
					return;
				}
				ptr = _jit_cache_alloc(&(gen->posn), sizeof(jit_float32));
				jit_memcpy(ptr, &float32_value, sizeof(float32_value));
				x86_fld(inst, ptr, 0);
			}
			break;

			case JIT_TYPE_FLOAT64:
			{
				jit_float64 float64_value;
				float64_value = jit_value_get_float64_constant(value);
				if(!jit_cache_check_for_n(&(gen->posn), 32))
				{
					jit_cache_mark_full(&(gen->posn));
					return;
				}
				ptr = _jit_cache_alloc(&(gen->posn), sizeof(jit_float64));
				jit_memcpy(ptr, &float64_value, sizeof(float64_value));
				x86_fld(inst, ptr, 1);
			}
			break;

			case JIT_TYPE_NFLOAT:
			{
				jit_nfloat nfloat_value;
				nfloat_value = jit_value_get_nfloat_constant(value);
				if(!jit_cache_check_for_n(&(gen->posn), 32))
				{
					jit_cache_mark_full(&(gen->posn));
					return;
				}
				ptr = _jit_cache_alloc(&(gen->posn), sizeof(jit_nfloat));
				jit_memcpy(ptr, &nfloat_value, sizeof(nfloat_value));
				x86_fld80_mem(inst, ptr);
			}
			break;
		}
	}
	else
	{
		/* Fix the position of the value in the stack frame */
		_jit_gen_fix_value(value);
		offset = (int)(value->frame_offset);

		/* Load the value into the specified register */
		switch(jit_type_normalize(value->type)->kind)
		{
			case JIT_TYPE_SBYTE:
			{
				x86_widen_membase(inst, _jit_reg_info[reg].cpu_reg,
								  X86_EBP, offset, 1, 0);
			}
			break;

			case JIT_TYPE_UBYTE:
			{
				x86_widen_membase(inst, _jit_reg_info[reg].cpu_reg,
								  X86_EBP, offset, 0, 0);
			}
			break;

			case JIT_TYPE_SHORT:
			{
				x86_widen_membase(inst, _jit_reg_info[reg].cpu_reg,
								  X86_EBP, offset, 1, 1);
			}
			break;

			case JIT_TYPE_USHORT:
			{
				x86_widen_membase(inst, _jit_reg_info[reg].cpu_reg,
								  X86_EBP, offset, 0, 1);
			}
			break;

			case JIT_TYPE_INT:
			case JIT_TYPE_UINT:
			{
				x86_mov_reg_membase(inst, _jit_reg_info[reg].cpu_reg,
									X86_EBP, offset, 4);
			}
			break;

			case JIT_TYPE_LONG:
			case JIT_TYPE_ULONG:
			{
			#ifdef JIT_NATIVE_INT64
				x86_mov_reg_membase(inst, _jit_reg_info[reg].cpu_reg,
									X86_EBP, offset, 8);
			#else
				x86_mov_reg_membase(inst, _jit_reg_info[reg].cpu_reg,
									X86_EBP, offset, 4);
				x86_mov_reg_membase(inst, _jit_reg_info[other_reg].cpu_reg,
									X86_EBP, offset + 4, 4);
			#endif
			}
			break;

			case JIT_TYPE_FLOAT32:
			{
				x86_fld_membase(inst, X86_EBP, offset, 0);
			}
			break;

			case JIT_TYPE_FLOAT64:
			{
				x86_fld_membase(inst, X86_EBP, offset, 1);
			}
			break;

			case JIT_TYPE_NFLOAT:
			{
				x86_fld80_membase(inst, X86_EBP, offset);
			}
			break;
		}
	}

	/* End the code output process */
	jit_cache_end_output();
}

void _jit_gen_fix_value(jit_value_t value)
{
	if(!(value->has_frame_offset) && !(value->is_constant))
	{
		jit_nint size = (jit_nint)(ROUND_STACK(jit_type_get_size(value->type)));
		value->block->func->builder->frame_size += size;
		value->frame_offset = -(value->block->func->builder->frame_size);
		value->has_frame_offset = 1;
	}
}

/*
 * Set up for a unary 32-bit integer operator.
 */
static int setup_for_int_unary(jit_gencode_t gen, jit_insn_t insn)
{
	return _jit_regs_load_value
		(gen, insn->value1, 1,
		 (insn->flags & (JIT_INSN_VALUE1_NEXT_USE |
		 				 JIT_INSN_VALUE1_LIVE)));
}

/*
 * Store the destination for a 32-bit integer operator.
 */
static void store_for_int(jit_gencode_t gen, jit_insn_t insn, int reg)
{
	if((insn->flags & JIT_INSN_DEST_NEXT_USE) != 0)
	{
		/* Record that the destination is in "reg" */
		_jit_regs_set_value(gen, reg, insn->dest, 0);
	}
	else
	{
		/* No next use, so store to the destination */
		_jit_gen_spill_reg(gen, reg, -1, insn->dest);
		insn->dest->in_frame = 1;
		_jit_regs_free_reg(gen, reg, 1);
	}
}

/*
 * Set up for a binary 32-bit integer operator.
 */
static int setup_for_int_binary(jit_gencode_t gen, jit_insn_t insn, int *reg2)
{
	int reg;
	reg = _jit_regs_load_value
		(gen, insn->value1, 1,
		 (insn->flags & (JIT_INSN_VALUE1_NEXT_USE |
		 				 JIT_INSN_VALUE1_LIVE)));
	*reg2 = _jit_regs_load_value
		(gen, insn->value2, 0,
		 (insn->flags & (JIT_INSN_VALUE1_NEXT_USE |
		 				 JIT_INSN_VALUE1_LIVE)));
	return reg;
}

/*
 * Set up for a binary 32-bit integer operator, where reg2 may be destroyed.
 */
static int setup_for_int_binary_destroy
	(jit_gencode_t gen, jit_insn_t insn, int *reg2)
{
	int reg;
	reg = _jit_regs_load_value
		(gen, insn->value1, 1,
		 (insn->flags & (JIT_INSN_VALUE1_NEXT_USE |
		 				 JIT_INSN_VALUE1_LIVE)));
	*reg2 = _jit_regs_load_value
		(gen, insn->value2, 1,
		 (insn->flags & (JIT_INSN_VALUE1_NEXT_USE |
		 				 JIT_INSN_VALUE1_LIVE)));
	return reg;
}

/*
 * Widen a byte register.
 */
static unsigned char *widen_byte(unsigned char *inst, int reg, int isSigned)
{
	if(reg == X86_EAX || reg == X86_EBX || reg == X86_ECX || reg == X86_EDX)
	{
		x86_widen_reg(inst, reg, reg, isSigned, 0);
	}
	else
	{
		x86_push_reg(inst, X86_EAX);
		x86_mov_reg_reg(inst, X86_EAX, reg, 4);
		x86_widen_reg(inst, reg, X86_EAX, isSigned, 0);
		x86_pop_reg(inst, X86_EAX);
	}
	return inst;
}

/*
 * Shift the contents of a register.
 */
static unsigned char *shift_reg(unsigned char *inst, int opc, int reg, int reg2)
{
	if(reg2 == X86_ECX)
	{
		/* The shift value is already in ECX */
		x86_shift_reg(inst, opc, reg1);
	}
	else if(reg1 == X86_ECX)
	{
		/* The value to be shifted is in ECX, so swap the order */
		x86_xchg_reg_reg(inst, reg1, reg2, 4);
		x86_shift_reg(inst, opc, reg2);
		x86_mov_reg_reg(inst, reg1, reg2, 4);
	}
	else
	{
		/* Save ECX, perform the shift, and then restore ECX */
		x86_push_reg(inst, X86_ECX);
		x86_mov_reg_reg(inst, X86_ECX, reg2, 4);
		x86_shift_reg(inst, opc, reg1);
		x86_pop_reg(inst, X86_ECX);
	}
	return inst;
}

void _jit_gen_insn(jit_gencode_t gen, jit_function_t func,
				   jit_block_t block, jit_insn_t insn)
{
	int reg, reg2;
	switch(insn->opcode)
	{
		/* TODO: lots of optimizations to do here */

		case JIT_OP_TRUNC_SBYTE:
		{
			/* Truncate a value to a signed 8-byte integer */
			reg = setup_for_int_unary(gen, insn);
			{
				jit_cache_setup_output(16);
				inst = widen_byte(inst, _jit_reg_info[reg].cpu_reg, 1);
				jit_cache_end_output();
			}
			store_for_int(gen, insn, reg);
		}
		break;

		case JIT_OP_TRUNC_UBYTE:
		{
			/* Truncate a value to an unsigned 8-byte integer */
			reg = setup_for_int_unary(gen, insn);
			{
				jit_cache_setup_output(16);
				inst = widen_byte(inst, _jit_reg_info[reg].cpu_reg, 0);
				jit_cache_end_output();
			}
			store_for_int(gen, insn, reg);
		}
		break;

		case JIT_OP_TRUNC_SHORT:
		{
			/* Truncate a value to a signed 16-byte integer */
			reg = setup_for_int_unary(gen, insn);
			{
				jit_cache_setup_output(16);
				x86_widen_reg(inst, _jit_reg_info[reg].cpu_reg,
							  _jit_reg_info[reg].cpu_reg, 1, 1);
				jit_cache_end_output();
			}
			store_for_int(gen, insn, reg);
		}
		break;

		case JIT_OP_TRUNC_USHORT:
		{
			/* Truncate a value to an unsigned 16-byte integer */
			reg = setup_for_int_unary(gen, insn);
			{
				jit_cache_setup_output(16);
				x86_widen_reg(inst, _jit_reg_info[reg].cpu_reg,
							  _jit_reg_info[reg].cpu_reg, 0, 1);
				jit_cache_end_output();
			}
			store_for_int(gen, insn, reg);
		}
		break;

		case JIT_OP_IADD:
		{
			/* Add two signed 32-bit integers */
			reg = setup_for_int_binary(gen, insn, &reg2);
			{
				jit_cache_setup_output(16);
				x86_alu_reg_reg(inst, X86_ADD, _jit_reg_info[reg].cpu_reg,
							    _jit_reg_info[reg2].cpu_reg);
				jit_cache_end_output();
			}
			store_for_int(gen, insn, reg);
		}
		break;

		case JIT_OP_ISUB:
		{
			/* Subtract two signed 32-bit integers */
			reg = setup_for_int_binary(gen, insn, &reg2);
			{
				jit_cache_setup_output(16);
				x86_alu_reg_reg(inst, X86_SUB, _jit_reg_info[reg].cpu_reg,
							    _jit_reg_info[reg2].cpu_reg);
				jit_cache_end_output();
			}
			store_for_int(gen, insn, reg);
		}
		break;

		case JIT_OP_IMUL:
		{
			/* Multiply two signed 32-bit integers.  We spill first
			   so that the reload will hopefully end up in EAX and ECX,
			   and EDX will be free for us to destroy */
			_jit_regs_spill_all(gen);
			reg = setup_for_int_binary(gen, insn, &reg2);
			{
				jit_cache_setup_output(16);
				x86_mul_reg(inst, _jit_reg_info[reg2].cpu_reg, 1);
				jit_cache_end_output();
			}
			store_for_int(gen, insn, reg);
		}
		break;

		case JIT_OP_INEG:
		{
			/* Negate a signed 32-bit integer */
			reg = setup_for_int_unary(gen, insn);
			{
				jit_cache_setup_output(16);
				x86_neg_reg(inst, _jit_reg_info[reg].cpu_reg);
				jit_cache_end_output();
			}
			store_for_int(gen, insn, reg);
		}
		break;

		case JIT_OP_IAND:
		{
			/* Bitwise AND two 32-bit integer values */
			reg = setup_for_int_binary(gen, insn, &reg2);
			{
				jit_cache_setup_output(16);
				x86_alu_reg_reg(inst, X86_AND, _jit_reg_info[reg].cpu_reg,
							    _jit_reg_info[reg2].cpu_reg);
				jit_cache_end_output();
			}
			store_for_int(gen, insn, reg);
		}
		break;

		case JIT_OP_IOR:
		{
			/* Bitwise OR two 32-bit integer values */
			reg = setup_for_int_binary(gen, insn, &reg2);
			{
				jit_cache_setup_output(16);
				x86_alu_reg_reg(inst, X86_OR, _jit_reg_info[reg].cpu_reg,
							    _jit_reg_info[reg2].cpu_reg);
				jit_cache_end_output();
			}
			store_for_int(gen, insn, reg);
		}
		break;

		case JIT_OP_IXOR:
		{
			/* Bitwise XOR two 32-bit integer values */
			reg = setup_for_int_binary(gen, insn, &reg2);
			{
				jit_cache_setup_output(16);
				x86_alu_reg_reg(inst, X86_XOR, _jit_reg_info[reg].cpu_reg,
							    _jit_reg_info[reg2].cpu_reg);
				jit_cache_end_output();
			}
			store_for_int(gen, insn, reg);
		}
		break;

		case JIT_OP_INOT:
		{
			/* Bitwise NOT a 32-bit integer value */
			reg = setup_for_int_unary(gen, insn);
			{
				jit_cache_setup_output(16);
				x86_not_reg(inst, _jit_reg_info[reg].cpu_reg);
				jit_cache_end_output();
			}
			store_for_int(gen, insn, reg);
		}
		break;

		case JIT_OP_ISHL:
		{
			/* Shift a 32-bit integer value left */
			reg = setup_for_int_binary_destroy(gen, insn, &reg2);
			{
				jit_cache_setup_output(16);
				inst = shift_reg(inst, X86_SHL, _jit_reg_info[reg].cpu_reg,
						         _jit_reg_info[reg2].cpu_reg);
				jit_cache_end_output();
			}
			store_for_int(gen, insn, reg);
		}
		break;

		case JIT_OP_ISHR:
		{
			/* Shift a 32-bit integer value right, with sign-extend */
			reg = setup_for_int_binary_destroy(gen, insn, &reg2);
			{
				jit_cache_setup_output(16);
				inst = shift_reg(inst, X86_SAR, _jit_reg_info[reg].cpu_reg,
						         _jit_reg_info[reg2].cpu_reg);
				jit_cache_end_output();
			}
			store_for_int(gen, insn, reg);
		}
		break;

		case JIT_OP_ISHR_UN:
		{
			/* Shift a 32-bit integer value right, with zero-extend */
			reg = setup_for_int_binary_destroy(gen, insn, &reg2);
			{
				jit_cache_setup_output(16);
				inst = shift_reg(inst, X86_SHR, _jit_reg_info[reg].cpu_reg,
						         _jit_reg_info[reg2].cpu_reg);
				jit_cache_end_output();
			}
			store_for_int(gen, insn, reg);
		}
		break;

		case JIT_OP_BR:
		case JIT_OP_BR_IFALSE:
		case JIT_OP_BR_ITRUE:
		case JIT_OP_BR_IEQ:
		case JIT_OP_BR_INE:
		case JIT_OP_BR_ILT:
		case JIT_OP_BR_ILT_UN:
		case JIT_OP_BR_ILE:
		case JIT_OP_BR_ILE_UN:
		case JIT_OP_BR_IGT:
		case JIT_OP_BR_IGT_UN:
		case JIT_OP_BR_IGE:
		case JIT_OP_BR_IGE_UN:
			/* TODO */
	}
}

void _jit_gen_start_block(jit_gencode_t gen, jit_block_t block)
{
	void **fixup;
	void **next;

	/* Set the address of this block */
	block->address = (void *)(gen->posn.ptr);

	/* If this block has pending fixups, then apply them now */
	fixup = (void **)(block->fixup_list);
	while(fixup != 0)
	{
		next = (void **)(fixup[0]);
		fixup[0] = (void *)
			(((jit_nint)(block->address)) - ((jit_nint)fixup) - 4);
		fixup = next;
	}
	block->fixup_list = 0;
}

void _jit_gen_end_block(jit_gencode_t gen, jit_block_t block)
{
	/* Nothing to do here for x86 */
}

void _jit_gen_call_finally
	(jit_gencode_t gen, jit_function_t func, jit_label_t label)
{
	/* TODO */
}

void _jit_gen_unwind_stack(void *stacktop, void *catch_pc, void *object)
{
	void *frame;

	/* Fetch the proper EBP value from the stack, just before
	   where we need to unwind back to */
	frame = ((void **)stacktop)[-2];

	/* Unwind the stack and jump to "catch_pc" */
#if defined(__GNUC__)
	__asm__ (
		"movl %0, %%edx\n\t"
		"movl %1, %%eax\n\t"
		"movl %2, %%ecx\n\t"
		"movl %3, %%ebx\n\t"
		"movl %%ebx, %%ebp\n\t"
		"movl %%edx, %%esp\n\t"
		"jmp *(%%eax)\n\t"
		: : "m"(stacktop), "m"(catch_pc), "m"(object), "m"(frame)
		: "eax", "ebx", "ecx", "edx"
	);
#elif defined(_MSC_VER)
	__asm {
		mov edx, dword ptr stacktop
		mov eax, dword ptr catch_pc
		mov ecx, dword ptr object
		mov ebx, dword ptr frame
		mov ebp, ebx
		mov esp, edx
		jmp [eax]
	}
#else
	#error "Don't know how to unwind the stack under x86"
#endif
}

void _jit_unwind_stack(void *frame, void *pc, void *object)
{
#if defined(__GNUC__)
	__asm__ (
		"movl %0, %%edx\n\t"
		"movl %1, %%eax\n\t"
		"movl %2, %%ecx\n\t"
		"movl %%edx, %%ebp\n\t"
		"popl %%ebp\n\t"
		"popl %%edx\n\t"
		"jmp *%%eax\n\t"
		: : "m"(frame), "m"(pc), "m"(object)
		: "eax", "ecx", "edx"
	);
#elif defined(_MSC_VER)
	__asm {
		mov edx, dword ptr frame
		mov eax, dword ptr pc
		mov ecx, dword ptr object
		mov ebp, edx
		pop ebp
		pop edx
		jmp eax
	}
#else
	#error "Don't know how to unwind the stack on x86 platforms"
#endif
}

#endif /* JIT_BACKEND_X86 */
