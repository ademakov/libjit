/*
 * jit-rules-interp.c - Rules that define the interpreter characteristics.
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
#include "jit-reg-alloc.h"

#if defined(JIT_BACKEND_INTERP)

#include "jit-interp.h"

/*@

The architecture definition rules for a CPU are placed into the files
@code{jit-rules-ARCH.h} and @code{jit-rules-ARCH.c}.  You should add
both of these files to @code{Makefile.am} in @code{libjit/jit}.

You will also need to edit @code{jit-rules.h} in two places.  First,
place detection logic at the top of the file to detect your platform
and define @code{JIT_BACKEND_ARCH} to 1.  Further down the file,
you should add the following two lines to the include file logic:

@example
#elif defined(JIT_BACKEND_ARCH)
#include "jit-rules-ARCH.h"
@end example

@subsection Defining the registers

Every rule header file needs to define the macro @code{JIT_REG_INFO} to
an array of values that represents the properties of the CPU's
registers.  The @code{_jit_reg_info} array is populated with
these values.  @code{JIT_NUM_REGS} defines the number of
elements in the array.  Each element in the array has the
following members:

@table @code
@item name
The name of the register.  This is used for debugging purposes.

@item cpu_reg
The raw CPU register number.  Registers in @code{libjit} are
referred to by their pseudo register numbers, corresponding to
their index within @code{JIT_REG_INFO}.  However, these pseudo
register numbers may not necessarily correspond to the register
numbers used by the actual CPU.  This field provides a mapping.

@item other_reg
The second pseudo register in a 64-bit register pair, or -1 if
the current register cannot be used as the first pseudo register
in a 64-bit register pair.  This field only has meaning on 32-bit
platforms, and should always be set to -1 on 64-bit platforms.

@item flags
Flag bits that describe the pseudo register's properties.
@end table

@noindent
The following flags may be present:

@table @code
@item JIT_REG_WORD
This register can hold an integer word value.

@item JIT_REG_LONG
This register can hold a 64-bit long value without needing a
second register.  Normally only used on 64-bit platforms.

@item JIT_REG_FLOAT32
This register can hold a 32-bit floating-point value.

@item JIT_REG_FLOAT64
This register can hold a 64-bit floating-point value.

@item JIT_REG_NFLOAT
This register can hold a native floating-point value.

@item JIT_REG_FRAME
This register holds the frame pointer.  You will almost always supply
@code{JIT_REG_FIXED} for this register.

@item JIT_REG_STACK_PTR
This register holds the stack pointer.  You will almost always supply
@code{JIT_REG_FIXED} for this register.

@item JIT_REG_FIXED
This register has a fixed meaning and cannot be used for general allocation.

@item JIT_REG_CALL_USED
This register will be destroyed by a function call.

@item JIT_REG_START_STACK
This register is the start of a range of registers that are used in a
stack-like arrangement.  Operations can typically only occur at the
top of the stack, and may automatically pop values as a side-effect
of the operation.  The stack continues until the next register that is
marked with @code{JIT_REG_END_STACK}.  The starting register must
also have the @code{JIT_REG_IN_STACK} flag set.

@item JIT_REG_END_STACK
This register is the end of a range of registers that are used in a
stack-like arrangement.  The ending register must also have the
@code{JIT_REG_IN_STACK} flag set.

@item JIT_REG_IN_STACK
This register is in a stack-like arrangement.  If neither
@code{JIT_REG_START_STACK} or @code{JIT_REG_END_STACK} is present,
then the register is in the "middle" of the stack.

@item JIT_REG_GLOBAL
This register is a candidate for global register allocation.
@end table

@subsection Other architecture macros

@noindent
The rule file may also have definitions of the following macros:

@table @code
@item JIT_NUM_GLOBAL_REGS
The number of registers that are used for global register allocation.
Set to zero if global register allocation should not be used.

@item JIT_ALWAYS_REG_REG
Define this to 1 if arithmetic operations must always be performed
on registers.  Define this to 0 if register/memory and memory/register
operations are possible.

@item JIT_PROLOG_SIZE
If defined, this indicates the maximum size of the function prolog.

@item JIT_FUNCTION_ALIGNMENT
This value indicates the alignment required for the start of a function.
e.g. define this to 32 if functions should be aligned on a 32-byte
boundary.

@item JIT_ALIGN_OVERRIDES
Define this to 1 if the platform allows reads and writes on
any byte boundary.  Define to 0 if only properly-aligned
memory accesses are allowed.  Normally only defined to 1 under x86.

@item jit_extra_gen_state
@itemx jit_extra_gen_init
@itemx jit_extra_gen_cleanup
The @code{jit_extra_gen_state} macro can be supplied to add extra fields
to the @code{struct jit_gencode} type in @code{jit-rules.h}, for
extra CPU-specific code generation state information.

The @code{jit_extra_gen_init} macro initializes this extra information,
and the @code{jit_extra_gen_cleanup} macro cleans it up when code
generation is complete.
@end table

@subsection Architecture-dependent functions

@*/

/*
 * Write an interpreter opcode to the cache.
 */
#define	jit_cache_opcode(posn,opcode)	\
			jit_cache_native((posn), (jit_nint)(opcode))

/*
 * Write "n" bytes to the cache, rounded up to a multiple of "void *".
 */
#define	jit_cache_add_n(posn,buf,size)	\
		do { \
			unsigned int __size = \
				((size) + sizeof(void *) - 1) & ~(sizeof(void *) - 1); \
			if(jit_cache_check_for_n((posn), __size)) \
			{ \
				jit_memcpy((posn)->ptr, (buf), (size)); \
				(posn)->ptr += __size; \
			} \
			else \
			{ \
				jit_cache_mark_full((posn)); \
			} \
		} while (0)

/*
 * Adjust the height of the working area.
 */
#define	adjust_working(gen,adjust)	\
		do { \
			(gen)->working_area += (adjust); \
			if((gen)->working_area > (gen)->max_working_area) \
			{ \
				(gen)->max_working_area = (gen)->working_area; \
			} \
		} while (0)

/*@
 * @deftypefun void _jit_init_backend (void)
 * Initialize the backend.  This is normally used to configure registers
 * that may not appear on all CPU's in a given family.  For example, only
 * some ARM cores have floating-point registers.
 * @end deftypefun
@*/
void _jit_init_backend(void)
{
	/* Nothing to do here for the interpreter */
}

/*@
 * @deftypefun void _jit_gen_get_elf_info ({jit_elf_info_t *} info)
 * Get the ELF machine and ABI type information for this platform.
 * The @code{machine} field should be set to one of the @code{EM_*}
 * values in @code{jit-elf-defs.h}.  The @code{abi} field should
 * be set to one of the @code{ELFOSABI_*} values in @code{jit-elf-defs.h}
 * (@code{ELFOSABI_SYSV} will normally suffice if you are unsure).
 * The @code{abi_version} field should be set to the ABI version,
 * which is usually zero.
 * @end deftypefun
@*/
void _jit_gen_get_elf_info(jit_elf_info_t *info)
{
	/* The interpreter's ELF machine type is defined to be "Lj",
	   which hopefully won't clash with any standard types */
	info->machine = 0x4C6A;
	info->abi = 0;
	info->abi_version = JIT_OPCODE_VERSION;
}

/*@
 * @deftypefun int _jit_create_entry_insns (jit_function_t func)
 * Create instructions in the entry block to initialize the
 * registers and frame offsets that contain the parameters.
 * Returns zero if out of memory.
 *
 * This function is called when a builder is initialized.  It should
 * scan the signature and decide which register or frame position
 * contains each of the parameters and then call either
 * @code{jit_insn_incoming_reg} or @code{jit_insn_incoming_frame_posn}
 * to notify @code{libjit} of the location.
 * @end deftypefun
@*/
int _jit_create_entry_insns(jit_function_t func)
{
	jit_type_t signature = func->signature;
	jit_type_t type;
	jit_nint offset;
	jit_value_t value;
	unsigned int num_params;
	unsigned int param;

	/* Reset the frame size for this function */
	func->builder->frame_size = 0;

	/* The starting parameter offset.  We use negative offsets to indicate
	   an offset into the "args" block, and positive offsets to indicate
	   an offset into the "frame" block.  The negative values will be
	   flipped when we output the argument opcodes for interpretation */
	offset = -1;

	/* If the function is nested, then we need two extra parameters
	   to pass the pointer to the parent's local variables and arguments */
	if(func->nested_parent)
	{
		offset -= 2;
	}

	/* Allocate the structure return pointer */
	value = jit_value_get_struct_pointer(func);
	if(value)
	{
		if(!jit_insn_incoming_frame_posn(func, value, offset))
		{
			return 0;
		}
		--offset;
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
			{
				if(!jit_insn_incoming_frame_posn
						(func, value, offset - _jit_int_lowest_byte()))
				{
					return 0;
				}
				--offset;
			}
			break;

			case JIT_TYPE_SHORT:
			case JIT_TYPE_USHORT:
			{
				if(!jit_insn_incoming_frame_posn
						(func, value, offset - _jit_int_lowest_short()))
				{
					return 0;
				}
				--offset;
			}
			break;

			case JIT_TYPE_INT:
			case JIT_TYPE_UINT:
			case JIT_TYPE_NINT:
			case JIT_TYPE_NUINT:
			case JIT_TYPE_SIGNATURE:
			case JIT_TYPE_PTR:
			case JIT_TYPE_LONG:
			case JIT_TYPE_ULONG:
			case JIT_TYPE_FLOAT32:
			case JIT_TYPE_FLOAT64:
			case JIT_TYPE_NFLOAT:
			{
				if(!jit_insn_incoming_frame_posn(func, value, offset))
				{
					return 0;
				}
				--offset;
			}
			break;

			case JIT_TYPE_STRUCT:
			case JIT_TYPE_UNION:
			{
				if(!jit_insn_incoming_frame_posn(func, value, offset))
				{
					return 0;
				}
				offset -= JIT_NUM_ITEMS_IN_STRUCT(jit_type_get_size(type));
			}
			break;
		}
	}
	return 1;
}

/*@
 * @deftypefun int _jit_create_call_setup_insns (jit_function_t func, jit_type_t signature, {jit_value_t *} args, {unsigned int} num_args, int is_nested, int nested_level, jit_value_t *struct_return, int flags)
 * Create instructions within @code{func} necessary to set up for a
 * function call to a function with the specified @code{signature}.
 * Use @code{jit_insn_push} to push values onto the system stack,
 * or @code{jit_insn_outgoing_reg} to copy values into call registers.
 *
 * If @code{is_nested} is non-zero, then it indicates that we are calling a
 * nested function within the current function's nested relationship tree.
 * The @code{nested_level} value will be -1 to call a child, zero to call a
 * sibling of @code{func}, 1 to call a sibling of the parent, 2 to call
 * a sibling of the grandparent, etc.  The @code{jit_insn_setup_for_nested}
 * instruction should be used to create the nested function setup code.
 *
 * If the function returns a structure by pointer, then @code{struct_return}
 * must be set to a new local variable that will contain the returned
 * structure.  Otherwise it should be set to NULL.
 * @end deftypefun
@*/
int _jit_create_call_setup_insns
	(jit_function_t func, jit_type_t signature,
	 jit_value_t *args, unsigned int num_args,
	 int is_nested, int nested_level, jit_value_t *struct_return, int flags)
{
	jit_type_t type;
	jit_type_t vtype;
	jit_value_t value;
	unsigned int arg_num;
	jit_nint offset;
	jit_nuint size;

	/* Regular or tail call? */
	if((flags & JIT_CALL_TAIL) == 0)
	{
		/* Push all of the arguments in reverse order */
		while(num_args > 0)
		{
			--num_args;
			type = jit_type_normalize(jit_type_get_param(signature, num_args));
			if(type->kind == JIT_TYPE_STRUCT || type->kind == JIT_TYPE_UNION)
			{
				/* If the value is a pointer, then we are pushing a structure
				   argument by pointer rather than by local variable */
				vtype = jit_type_normalize(jit_value_get_type(args[num_args]));
				if(vtype->kind <= JIT_TYPE_MAX_PRIMITIVE)
				{
					if(!jit_insn_push_ptr(func, args[num_args], type))
					{
						return 0;
					}
					continue;
				}
			}
			if(!jit_insn_push(func, args[num_args]))
			{
				return 0;
			}
		}

		/* Do we need to add a structure return pointer argument? */
		type = jit_type_get_return(signature);
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
			if(!jit_insn_push(func, value))
			{
				return 0;
			}
		}
		else if((flags & JIT_CALL_NATIVE) != 0)
		{
			/* Native calls always return a return area pointer */
			if(!jit_insn_push_return_area_ptr(func))
			{
				return 0;
			}
			*struct_return = 0;
		}
		else
		{
			*struct_return = 0;
		}

		/* Do we need to add nested function scope information? */
		if(is_nested)
		{
			if(!jit_insn_setup_for_nested(func, nested_level, -1))
			{
				return 0;
			}
		}
	}
	else
	{
		/* Copy the arguments into our own parameter slots */
		offset = -1;
		if(func->nested_parent)
		{
			offset -= 2;
		}
		type = jit_type_get_return(signature);
		if(jit_type_return_via_pointer(type))
		{
			--offset;
		}
		for(arg_num = 0; arg_num < num_args; ++arg_num)
		{
			type = jit_type_get_param(signature, arg_num);
			value = jit_value_create(func, type);
			if(!value)
			{
				return 0;
			}
			if(!jit_insn_outgoing_frame_posn(func, value, offset))
			{
				return 0;
			}
			type = jit_type_normalize(type);
			size = jit_type_get_size(type);
			offset -= (jit_nint)(JIT_NUM_ITEMS_IN_STRUCT(size));
			if(type->kind == JIT_TYPE_STRUCT || type->kind == JIT_TYPE_UNION)
			{
				/* If the value is a pointer, then we are pushing a structure
				   argument by pointer rather than by local variable */
				vtype = jit_type_normalize(jit_value_get_type(args[arg_num]));
				if(vtype->kind <= JIT_TYPE_MAX_PRIMITIVE)
				{
					value = jit_insn_address_of(func, value);
					if(!value)
					{
						return 0;
					}
					if(!jit_insn_memcpy
							(func, value, args[arg_num],
							 jit_value_create_nint_constant
								(func, jit_type_nint, (jit_nint)size)))
					{
						return 0;
					}
					continue;
				}
			}
			if(!jit_insn_store(func, value, args[arg_num]))
			{
				return 0;
			}
		}
		*struct_return = 0;
	}

	/* The call is ready to proceed */
	return 1;
}

/*@
 * @deftypefun int _jit_setup_indirect_pointer (jit_function_t func, jit_value_t value)
 * Place the indirect function pointer @code{value} into a suitable register
 * or stack location for a subsequent indirect call.
 * @end deftypefun
@*/
int _jit_setup_indirect_pointer(jit_function_t func, jit_value_t value)
{
	return jit_insn_push(func, value);
}

/*@
 * @deftypefun int _jit_create_call_return_insns (jit_function_t func, jit_type_t signature, jit_value_t *args, unsigned int num_args, jit_value_t return_value, int is_nested)
 * Create instructions within @code{func} to clean up after a function call
 * and to place the function's result into @code{return_value}.
 * This should use @code{jit_insn_pop_stack} to pop values off the system
 * stack and @code{jit_insn_return_reg} to tell @code{libjit} which
 * register contains the return value.  In the case of a @code{void}
 * function, @code{return_value} will be NULL.
 *
 * Note: the argument values are passed again because it may not be possible
 * to determine how many bytes to pop from the stack from the @code{signature}
 * alone; especially if the called function is vararg.
 * @end deftypefun
@*/
int _jit_create_call_return_insns
	(jit_function_t func, jit_type_t signature,
	 jit_value_t *args, unsigned int num_args,
	 jit_value_t return_value, int is_nested)
{
	jit_nint pop_items;
	unsigned int size;
	jit_type_t return_type;
	int ptr_return;

	/* Calculate the number of items that we need to pop */
	pop_items = 0;
	while(num_args > 0)
	{
		--num_args;
		size = jit_type_get_size(jit_value_get_type(args[num_args]));
		pop_items += JIT_NUM_ITEMS_IN_STRUCT(size);
	}
	return_type = jit_type_normalize(jit_type_get_return(signature));
	ptr_return = jit_type_return_via_pointer(return_type);
	if(ptr_return)
	{
		++pop_items;
	}
	if(is_nested)
	{
		/* The interpreter needs two arguments for the parent frame info */
		pop_items += 2;
	}

	/* Pop the items from the system stack */
	if(pop_items > 0)
	{
		if(!jit_insn_pop_stack(func, pop_items))
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
	   everything else ends up in the top-most stack register */
	if(jit_type_is_struct(return_type) || jit_type_is_union(return_type))
	{
		if(!jit_insn_flush_struct(func, return_value))
		{
			return 0;
		}
	}
	else if(return_type->kind != JIT_TYPE_VOID)
	{
		if(!jit_insn_return_reg(func, return_value, 0))
		{
			return 0;
		}
	}

	/* Everything is back where it needs to be */
	return 1;
}

/*@
 * @deftypefun int _jit_opcode_is_supported (int opcode)
 * Not all CPU's support all arithmetic, conversion, bitwise, or
 * comparison operators natively.  For example, most ARM platforms
 * need to call out to helper functions to perform floating-point.
 *
 * If this function returns zero, then @code{jit-insn.c} will output a
 * call to an intrinsic function that is equivalent to the desired opcode.
 * This is how you tell @code{libjit} that you cannot handle the
 * opcode natively.
 *
 * This function can also help you develop your back end incrementally.
 * Initially, you can report that only integer operations are supported,
 * and then once you have them working you can move on to the floating point
 * operations.
 * @end deftypefun
@*/
int _jit_opcode_is_supported(int opcode)
{
	/* We support all opcodes in the interpreter */
	return 1;
}

/*
 * Calculate the size of the argument area for an interpreted function.
 */
unsigned int _jit_interp_calculate_arg_size
		(jit_function_t func, jit_type_t signature)
{
	unsigned int size = 0;
	jit_type_t type;
	unsigned int num_params;
	unsigned int param;

	/* Determine if we need nested parameter information */
	if(func->nested_parent)
	{
		size += 2 * sizeof(jit_item);
	}

	/* Determine if we need a structure pointer argument */
	type = jit_type_get_return(signature);
	if(jit_type_return_via_pointer(type))
	{
		size += sizeof(jit_item);
	}

	/* Calculate the total size of the regular arguments */
	num_params = jit_type_num_params(signature);
	for(param = 0; param < num_params; ++param)
	{
		type = jit_type_normalize(jit_type_get_param(signature, param));
		if(type->kind == JIT_TYPE_STRUCT || type->kind == JIT_TYPE_UNION)
		{
			size += JIT_NUM_ITEMS_IN_STRUCT(jit_type_get_size(type)) *
					sizeof(jit_item);
		}
		else
		{
			size += sizeof(jit_item);
		}
	}

	/* Return the final size to the caller */
	return size;
}

/*@
 * @deftypefun {void *} _jit_gen_prolog (jit_gencode_t gen, jit_function_t func, {void *} buf)
 * Generate the prolog for a function into a previously-prepared
 * buffer area of @code{JIT_PROLOG_SIZE} bytes in size.  Returns
 * the start of the prolog, which may be different than @code{buf}.
 *
 * This function is called at the end of the code generation process,
 * not the beginning.  At this point, it is known which callee save
 * registers must be preserved, allowing the back end to output the
 * most compact prolog possible.
 * @end deftypefun
@*/
void *_jit_gen_prolog(jit_gencode_t gen, jit_function_t func, void *buf)
{
	/* Output the jit_function_interp structure at the beginning */
	jit_function_interp_t interp = (jit_function_interp_t)buf;
	unsigned int max_working_area =
		gen->max_working_area + gen->extra_working_space;
	interp->func = func;
	interp->args_size = _jit_interp_calculate_arg_size(func, func->signature);
	interp->frame_size =
		(func->builder->frame_size + max_working_area) * sizeof(jit_item);
	interp->working_area = max_working_area;
	return buf;
}

/*@
 * @deftypefun void _jit_gen_epilog (jit_gencode_t gen, jit_function_t func)
 * Generate a function epilog, restoring the registers that
 * were saved on entry to the function, and then returning.
 *
 * Only one epilog is generated per function.  Functions with multiple
 * @code{jit_insn_return} instructions will all jump to the common epilog.
 * This is needed because the code generator may not know which callee
 * save registers need to be restored by the epilog until the full function
 * has been processed.
 * @end deftypefun
@*/
void _jit_gen_epilog(jit_gencode_t gen, jit_function_t func)
{
	/* The interpreter doesn't use epilogs */
}

/*@
 * @deftypefun {void *} _jit_gen_redirector (jit_gencode_t gen, jit_function_t func)
 * Generate code for a redirector, which makes an indirect jump
 * to the contents of @code{func->entry_point}.  Redirectors
 * are used on recompilable functions in place of the regular
 * entry point.  This allows @code{libjit} to redirect existing
 * calls to the new version after recompilation.
 * @end deftypefun
@*/
void *_jit_gen_redirector(jit_gencode_t gen, jit_function_t func)
{
	/* The interpreter doesn't need redirectors */
	return 0;
}

/*@
 * @deftypefun void _jit_gen_spill_reg (jit_gencode_t gen, int reg, int other_reg, jit_value_t value)
 * Generate instructions to spill a pseudo register to the local
 * variable frame.  If @code{other_reg} is not -1, then it indicates
 * the second register in a 64-bit register pair.
 *
 * This function will typically call @code{_jit_gen_fix_value} to
 * fix the value's frame position, and will then generate the
 * appropriate spill instructions.
 * @end deftypefun
@*/
void _jit_gen_spill_reg(jit_gencode_t gen, int reg,
						int other_reg, jit_value_t value)
{
	int opcode;
	jit_nint offset;

	/* Fix the value in place within the local variable frame */
	_jit_gen_fix_value(value);

	/* Output an appropriate instruction to spill the value */
	offset = value->frame_offset;
	if(offset >= 0)
	{
		opcode = _jit_store_opcode(JIT_OP_STLOC_BYTE, 0, value->type);
	}
	else
	{
		opcode = _jit_store_opcode(JIT_OP_STARG_BYTE, 0, value->type);
		offset = -(offset + 1);
	}
	jit_cache_opcode(&(gen->posn), opcode);
	jit_cache_native(&(gen->posn), offset);

	/* Adjust the working area to account for the popped value */
	adjust_working(gen, -1);
}

/*@
 * @deftypefun void _jit_gen_free_reg (jit_gencode_t gen, int reg, int other_reg, int value_used)
 * Generate instructions to free a register without spilling its value.
 * This is called when a register's contents become invalid, or its
 * value is no longer required.  If @code{value_used} is set to a non-zero
 * value, then it indicates that the register's value was just used.
 * Otherwise, there is a value in the register but it was never used.
 *
 * On most platforms, this function won't need to do anything to free
 * the register.  But some do need to take explicit action.  For example,
 * x86 needs an explicit instruction to remove a floating-point value
 * from the FPU's stack if its value has not been used yet.
 * @end deftypefun
@*/
void _jit_gen_free_reg(jit_gencode_t gen, int reg,
					   int other_reg, int value_used)
{
	/* If the value wasn't used, then pop it from the stack.
	   Registers are always freed from the top down */
	if(!value_used)
	{
		jit_cache_opcode(&(gen->posn), JIT_OP_POP);
		adjust_working(gen, -1);
	}
}

/*@
 * @deftypefun void _jit_gen_load_value (jit_gencode_t gen, int reg, int other_reg, jit_value_t value)
 * Generate instructions to load a value into a register.  The value will
 * either be a constant or a slot in the frame.  You should fix frame slots
 * with @code{_jit_gen_fix_value}.
 * @end deftypefun
@*/
void _jit_gen_load_value
	(jit_gencode_t gen, int reg, int other_reg, jit_value_t value)
{
	int opcode;
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
				jit_cache_opcode(&(gen->posn), JIT_OP_PUSH_CONST_INT);
				jit_cache_native(&(gen->posn), (jit_nint)(value->address));
			}
			break;

			case JIT_TYPE_LONG:
			case JIT_TYPE_ULONG:
			{
				jit_long long_value;
				long_value = jit_value_get_long_constant(value);
				jit_cache_opcode(&(gen->posn), JIT_OP_PUSH_CONST_LONG);
			#ifdef JIT_NATIVE_INT64
				jit_cache_native(&(gen->posn), long_value);
			#else
				jit_cache_add_n(&(gen->posn), &long_value, sizeof(long_value));
			#endif
			}
			break;

			case JIT_TYPE_FLOAT32:
			{
				jit_float32 float32_value;
				float32_value = jit_value_get_float32_constant(value);
				jit_cache_opcode(&(gen->posn), JIT_OP_PUSH_CONST_FLOAT32);
				jit_cache_add_n
					(&(gen->posn), &float32_value, sizeof(float32_value));
			}
			break;

			case JIT_TYPE_FLOAT64:
			{
				jit_float64 float64_value;
				float64_value = jit_value_get_float64_constant(value);
				jit_cache_opcode(&(gen->posn), JIT_OP_PUSH_CONST_FLOAT64);
				jit_cache_add_n
					(&(gen->posn), &float64_value, sizeof(float64_value));
			}
			break;

			case JIT_TYPE_NFLOAT:
			{
				jit_nfloat nfloat_value;
				nfloat_value = jit_value_get_nfloat_constant(value);
				jit_cache_opcode(&(gen->posn), JIT_OP_PUSH_CONST_NFLOAT);
				jit_cache_add_n
					(&(gen->posn), &nfloat_value, sizeof(nfloat_value));
			}
			break;
		}
	}
	else
	{
		/* Fix the position of the value in the stack frame */
		_jit_gen_fix_value(value);

		/* Generate a local or argument access opcode, as appropriate */
		if(value->frame_offset >= 0)
		{
			/* Load a local variable value onto the stack */
			opcode = _jit_load_opcode
				(JIT_OP_LDLOC_SBYTE, value->type, value, 0);
			jit_cache_opcode(&(gen->posn), opcode);
			jit_cache_native(&(gen->posn), value->frame_offset);
		}
		else
		{
			/* Load an argument value onto the stack */
			opcode = _jit_load_opcode
				(JIT_OP_LDARG_SBYTE, value->type, value, 0);
			jit_cache_opcode(&(gen->posn), opcode);
			jit_cache_native(&(gen->posn), -(value->frame_offset + 1));
		}
	}

	/* We have one more value on the stack */
	adjust_working(gen, 1);
}

/*@
 * @deftypefun void _jit_gen_load_global (jit_gencode_t gen, jit_value_t value)
 * Load the contents of @code{value} into its corresponding global register.
 * This is used at the head of a function to pull parameters out of stack
 * slots into their global register copies.
 * @end deftypefun
@*/
void _jit_gen_load_global(jit_gencode_t gen, jit_value_t value)
{
	/* Global registers are not used in the interpreted back end */
}

/*@
 * @deftypefun void _jit_gen_fix_value (jit_value_t value)
 * Fix the position of a value within the local variable frame.
 * If it doesn't already have a position, then assign one for it.
 * @end deftypefun
@*/
void _jit_gen_fix_value(jit_value_t value)
{
	if(!(value->has_frame_offset) && !(value->is_constant))
	{
		jit_nint size = (jit_nint)
			(JIT_NUM_ITEMS_IN_STRUCT(jit_type_get_size(value->type)));
		value->frame_offset = value->block->func->builder->frame_size;
		value->block->func->builder->frame_size += size;
		value->has_frame_offset = 1;
	}
}

/*
 * Record that a destination is now in a particular register.
 */
static void record_dest(jit_gencode_t gen, jit_insn_t insn, int reg)
{
	if(insn->dest)
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
	else
	{
		/* This is a note, with the result left on the stack */
		_jit_regs_free_reg(gen, reg, 1);
	}
}

/*@
 * @deftypefun void _jit_gen_insn (jit_gencode_t gen, jit_function_t func, jit_block_t block, jit_insn_t insn)
 * Generate native code for the specified @code{insn}.  This function should
 * call the appropriate register allocation routines, output the instruction,
 * and then arrange for the result to be placed in an appropriate register
 * or memory destination.
 * @end deftypefun
@*/
void _jit_gen_insn(jit_gencode_t gen, jit_function_t func,
				   jit_block_t block, jit_insn_t insn)
{
	int reg;
	jit_label_t label;
	void **pc;
	jit_nint offset;
	jit_nint size;

	switch(insn->opcode)
	{
		case JIT_OP_BR:
		case JIT_OP_CALL_FINALLY:
		{
			/* Unconditional branch */
			_jit_regs_spill_all(gen);
			label = (jit_label_t)(insn->dest);
		branch:
			pc = (void **)(gen->posn.ptr);
			jit_cache_opcode(&(gen->posn), insn->opcode);
			block = jit_block_from_label(func, label);
			if(!block)
			{
				break;
			}
			if(block->address)
			{
				/* We already know the address of the block */
				jit_cache_native
					(&(gen->posn), ((void **)(block->address)) - pc);
			}
			else
			{
				/* Record this position on the block's fixup list */
				jit_cache_native(&(gen->posn), block->fixup_list);
				block->fixup_list = (void *)pc;
			}
		}
		break;

		case JIT_OP_BR_IFALSE:
		case JIT_OP_BR_ITRUE:
		case JIT_OP_BR_LFALSE:
		case JIT_OP_BR_LTRUE:
		case JIT_OP_CALL_FILTER:
		{
			/* Unary branch */
			label = (jit_label_t)(insn->dest);
			if(!_jit_regs_is_top(gen, insn->value1) ||
			   _jit_regs_num_used(gen, 0) != 1)
			{
				_jit_regs_spill_all(gen);
			}
			reg = _jit_regs_load_to_top
				(gen, insn->value1, (insn->flags & JIT_INSN_VALUE1_LIVE), 0);
			_jit_regs_free_reg(gen, reg, 1);
			goto branch;
		}
		/* Not reached */

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
		case JIT_OP_BR_LEQ:
		case JIT_OP_BR_LNE:
		case JIT_OP_BR_LLT:
		case JIT_OP_BR_LLT_UN:
		case JIT_OP_BR_LLE:
		case JIT_OP_BR_LLE_UN:
		case JIT_OP_BR_LGT:
		case JIT_OP_BR_LGT_UN:
		case JIT_OP_BR_LGE:
		case JIT_OP_BR_LGE_UN:
		case JIT_OP_BR_FEQ:
		case JIT_OP_BR_FNE:
		case JIT_OP_BR_FLT:
		case JIT_OP_BR_FLE:
		case JIT_OP_BR_FGT:
		case JIT_OP_BR_FGE:
		case JIT_OP_BR_FEQ_INV:
		case JIT_OP_BR_FNE_INV:
		case JIT_OP_BR_FLT_INV:
		case JIT_OP_BR_FLE_INV:
		case JIT_OP_BR_FGT_INV:
		case JIT_OP_BR_FGE_INV:
		case JIT_OP_BR_DEQ:
		case JIT_OP_BR_DNE:
		case JIT_OP_BR_DLT:
		case JIT_OP_BR_DLE:
		case JIT_OP_BR_DGT:
		case JIT_OP_BR_DGE:
		case JIT_OP_BR_DEQ_INV:
		case JIT_OP_BR_DNE_INV:
		case JIT_OP_BR_DLT_INV:
		case JIT_OP_BR_DLE_INV:
		case JIT_OP_BR_DGT_INV:
		case JIT_OP_BR_DGE_INV:
		case JIT_OP_BR_NFEQ:
		case JIT_OP_BR_NFNE:
		case JIT_OP_BR_NFLT:
		case JIT_OP_BR_NFLE:
		case JIT_OP_BR_NFGT:
		case JIT_OP_BR_NFGE:
		case JIT_OP_BR_NFEQ_INV:
		case JIT_OP_BR_NFNE_INV:
		case JIT_OP_BR_NFLT_INV:
		case JIT_OP_BR_NFLE_INV:
		case JIT_OP_BR_NFGT_INV:
		case JIT_OP_BR_NFGE_INV:
		{
			/* Binary branch */
			label = (jit_label_t)(insn->dest);
			if(!_jit_regs_is_top_two(gen, insn->value1, insn->value2) ||
			   _jit_regs_num_used(gen, 0) != 2)
			{
				_jit_regs_spill_all(gen);
			}
			reg = _jit_regs_load_to_top_two
				(gen, insn->value1, insn->value2,
				 (insn->flags & JIT_INSN_VALUE1_LIVE),
				 (insn->flags & JIT_INSN_VALUE2_LIVE), 0);
			_jit_regs_free_reg(gen, reg, 1);
			goto branch;
		}
		/* Not reached */

		case JIT_OP_ADDRESS_OF_LABEL:
		{
			/* Get the address of a particular label */
			if(_jit_regs_num_used(gen, 0) >= JIT_NUM_REGS)
			{
				_jit_regs_spill_all(gen);
			}
			reg = _jit_regs_new_top(gen, insn->dest, 0);
			adjust_working(gen, 1);
			label = (jit_label_t)(insn->value1);
			pc = (void **)(gen->posn.ptr);
			jit_cache_opcode(&(gen->posn), insn->opcode);
			block = jit_block_from_label(func, label);
			if(!block)
			{
				break;
			}
			if(block->address)
			{
				/* We already know the address of the block */
				jit_cache_native
					(&(gen->posn), ((void **)(block->address)) - pc);
			}
			else
			{
				/* Record this position on the block's fixup list */
				jit_cache_native(&(gen->posn), block->fixup_list);
				block->fixup_list = (void *)pc;
			}
		}
		break;

		case JIT_OP_CALL:
		case JIT_OP_CALL_TAIL:
		{
			/* Call a function, whose pointer is supplied explicitly */
			jit_cache_opcode(&(gen->posn), insn->opcode);
			jit_cache_native(&(gen->posn), (jit_nint)(insn->dest));
		}
		break;

		case JIT_OP_CALL_INDIRECT:
		{
			/* Call a function, whose pointer is supplied on the stack */
			jit_cache_opcode(&(gen->posn), insn->opcode);
			jit_cache_native(&(gen->posn), (jit_nint)(insn->value2));
			jit_cache_native(&(gen->posn), (jit_nint)
					(jit_type_num_params((jit_type_t)(insn->value2))));
			adjust_working(gen, -1);
		}
		break;

		case JIT_OP_CALL_VTABLE_PTR:
		{
			/* Call a function, whose vtable pointer is supplied on the stack */
			jit_cache_opcode(&(gen->posn), insn->opcode);
			adjust_working(gen, -1);
		}
		break;

		case JIT_OP_CALL_EXTERNAL:
		{
			/* Call a native function, whose pointer is supplied explicitly */
			jit_cache_opcode(&(gen->posn), insn->opcode);
			jit_cache_native(&(gen->posn), (jit_nint)(insn->value2));
			jit_cache_native(&(gen->posn), (jit_nint)(insn->dest));
			jit_cache_native(&(gen->posn), (jit_nint)
					(jit_type_num_params((jit_type_t)(insn->value2))));
		}
		break;

		case JIT_OP_RETURN:
		{
			/* Return from the current function with no result */
			_jit_regs_spill_all(gen);
			jit_cache_opcode(&(gen->posn), JIT_OP_RETURN);
		}
		break;

		case JIT_OP_RETURN_INT:
		case JIT_OP_RETURN_LONG:
		case JIT_OP_RETURN_FLOAT32:
		case JIT_OP_RETURN_FLOAT64:
		case JIT_OP_RETURN_NFLOAT:
		{
			/* Return from the current function with a specific result */
			if(!_jit_regs_is_top(gen, insn->value1) ||
			   _jit_regs_num_used(gen, 0) != 1)
			{
				_jit_regs_spill_all(gen);
			}
			reg = _jit_regs_load_to_top(gen, insn->value1, 0, 0);
			jit_cache_opcode(&(gen->posn), insn->opcode);
			_jit_regs_free_reg(gen, reg, 1);
		}
		break;

		case JIT_OP_RETURN_SMALL_STRUCT:
		{
			/* Return from current function with a small structure result */
			if(!_jit_regs_is_top(gen, insn->value1) ||
			   _jit_regs_num_used(gen, 0) != 1)
			{
				_jit_regs_spill_all(gen);
			}
			reg = _jit_regs_load_to_top(gen, insn->value1, 0, 0);
			jit_cache_opcode(&(gen->posn), insn->opcode);
			jit_cache_native(&(gen->posn),
							 jit_value_get_nint_constant(insn->value2));
			_jit_regs_free_reg(gen, reg, 1);
		}
		break;

		case JIT_OP_SETUP_FOR_NESTED:
		{
			/* Set up to call a nested child */
			jit_cache_opcode(&(gen->posn), insn->opcode);
			adjust_working(gen, 2);
		}
		break;

		case JIT_OP_SETUP_FOR_SIBLING:
		{
			/* Set up to call a nested sibling */
			jit_cache_opcode(&(gen->posn), insn->opcode);
			jit_cache_native(&(gen->posn),
							 jit_value_get_nint_constant(insn->value1));
			adjust_working(gen, 2);
		}
		break;

		case JIT_OP_IMPORT:
		{
			/* Import a local variable from an outer nested scope */
			if(_jit_regs_num_used(gen, 0) >= JIT_NUM_REGS)
			{
				_jit_regs_spill_all(gen);
			}
			_jit_gen_fix_value(insn->value1);
			if(insn->value1->frame_offset >= 0)
			{
				jit_cache_opcode(&(gen->posn), JIT_OP_IMPORT_LOCAL);
				jit_cache_native(&(gen->posn), insn->value1->frame_offset);
				jit_cache_native(&(gen->posn),
								 jit_value_get_nint_constant(insn->value2));
			}
			else
			{
				jit_cache_opcode(&(gen->posn), JIT_OP_IMPORT_ARG);
				jit_cache_native
					(&(gen->posn), -(insn->value1->frame_offset + 1));
				jit_cache_native(&(gen->posn),
								 jit_value_get_nint_constant(insn->value2));
			}
			reg = _jit_regs_new_top(gen, insn->dest, 0);
			adjust_working(gen, 1);
		}
		break;

		case JIT_OP_THROW:
		{
			/* Throw an exception */
			reg = _jit_regs_load_to_top
				(gen, insn->value1,
				 (insn->flags & (JIT_INSN_VALUE1_NEXT_USE |
				 				 JIT_INSN_VALUE1_LIVE)), 0);
			jit_cache_opcode(&(gen->posn), insn->opcode);
			_jit_regs_free_reg(gen, reg, 1);
		}
		break;

		case JIT_OP_LOAD_PC:
		case JIT_OP_LOAD_EXCEPTION_PC:
		{
			/* Load the current program counter onto the stack */
			if(_jit_regs_num_used(gen, 0) >= JIT_NUM_REGS)
			{
				_jit_regs_spill_all(gen);
			}
			jit_cache_opcode(&(gen->posn), insn->opcode);
			reg = _jit_regs_new_top(gen, insn->dest, 0);
			adjust_working(gen, 1);
		}
		break;

		case JIT_OP_CALL_FILTER_RETURN:
		{
			/* The top of stack currently contains "dest" */
			_jit_regs_set_value(gen, 0, insn->dest, 0);
			adjust_working(gen, 1);
		}
		break;

		case JIT_OP_ENTER_FINALLY:
		{
			/* Record that the finally return address is on the stack */
			++(gen->extra_working_space);
		}
		break;

		case JIT_OP_LEAVE_FINALLY:
		{
			/* Leave a finally clause */
			jit_cache_opcode(&(gen->posn), insn->opcode);
		}
		break;

		case JIT_OP_ENTER_FILTER:
		{
			/* The top of stack contains "dest" and a return address */
			++(gen->extra_working_space);
			_jit_regs_set_value(gen, 0, insn->dest, 0);
			adjust_working(gen, 1);
		}
		break;

		case JIT_OP_LEAVE_FILTER:
		{
			/* Leave a filter clause, returning a particular value */
			if(!_jit_regs_is_top(gen, insn->value1) ||
			   _jit_regs_num_used(gen, 0) != 1)
			{
				_jit_regs_spill_all(gen);
			}
			reg = _jit_regs_load_to_top(gen, insn->value1, 0, 0);
			jit_cache_opcode(&(gen->posn), insn->opcode);
			_jit_regs_free_reg(gen, reg, 1);
		}
		break;

		case JIT_OP_RETURN_REG:
		{
			/* Push a function return value back onto the stack */
			switch(jit_type_normalize(insn->value1->type)->kind)
			{
				case JIT_TYPE_SBYTE:
				case JIT_TYPE_UBYTE:
				case JIT_TYPE_SHORT:
				case JIT_TYPE_USHORT:
				case JIT_TYPE_INT:
				case JIT_TYPE_UINT:
				{
					jit_cache_opcode(&(gen->posn), JIT_OP_PUSH_RETURN_INT);
					adjust_working(gen, 1);
				}
				break;

				case JIT_TYPE_LONG:
				case JIT_TYPE_ULONG:
				{
					jit_cache_opcode(&(gen->posn), JIT_OP_PUSH_RETURN_LONG);
					adjust_working(gen, 1);
				}
				break;

				case JIT_TYPE_FLOAT32:
				{
					jit_cache_opcode(&(gen->posn), JIT_OP_PUSH_RETURN_FLOAT32);
					adjust_working(gen, 1);
				}
				break;

				case JIT_TYPE_FLOAT64:
				{
					jit_cache_opcode(&(gen->posn), JIT_OP_PUSH_RETURN_FLOAT64);
					adjust_working(gen, 1);
				}
				break;

				case JIT_TYPE_NFLOAT:
				{
					jit_cache_opcode(&(gen->posn), JIT_OP_PUSH_RETURN_NFLOAT);
					adjust_working(gen, 1);
				}
				break;
			}
		}
		break;

		case JIT_OP_COPY_LOAD_SBYTE:
		case JIT_OP_COPY_LOAD_UBYTE:
		case JIT_OP_COPY_LOAD_SHORT:
		case JIT_OP_COPY_LOAD_USHORT:
		case JIT_OP_COPY_INT:
		case JIT_OP_COPY_LONG:
		case JIT_OP_COPY_FLOAT32:
		case JIT_OP_COPY_FLOAT64:
		case JIT_OP_COPY_NFLOAT:
		case JIT_OP_COPY_STRUCT:
		case JIT_OP_COPY_STORE_BYTE:
		case JIT_OP_COPY_STORE_SHORT:
		{
			/* Copy a value from one temporary variable to another */
			reg = _jit_regs_load_to_top
				(gen, insn->value1,
				 (insn->flags & (JIT_INSN_VALUE1_NEXT_USE |
				 				 JIT_INSN_VALUE1_LIVE)), 0);
			record_dest(gen, insn, reg);
		}
		break;

		case JIT_OP_ADDRESS_OF:
		{
			/* Get the address of a local variable */
			if(_jit_regs_num_used(gen, 0) >= JIT_NUM_REGS)
			{
				_jit_regs_spill_all(gen);
			}
			_jit_gen_fix_value(insn->value1);
			if(insn->value1->frame_offset >= 0)
			{
				jit_cache_opcode(&(gen->posn), JIT_OP_LDLOCA);
				jit_cache_native(&(gen->posn), insn->value1->frame_offset);
			}
			else
			{
				jit_cache_opcode(&(gen->posn), JIT_OP_LDARGA);
				jit_cache_native
					(&(gen->posn), -(insn->value1->frame_offset + 1));
			}
			reg = _jit_regs_new_top(gen, insn->dest, 0);
			adjust_working(gen, 1);
		}
		break;

		case JIT_OP_PUSH_INT:
		case JIT_OP_PUSH_LONG:
		case JIT_OP_PUSH_FLOAT32:
		case JIT_OP_PUSH_FLOAT64:
		case JIT_OP_PUSH_NFLOAT:
		{
			/* Push an item onto the stack, ready for a function call */
			if(!_jit_regs_is_top(gen, insn->value1) ||
			   _jit_regs_num_used(gen, 0) != 1)
			{
				_jit_regs_spill_all(gen);
			}
			reg = _jit_regs_load_to_top
				(gen, insn->value1,
				 (insn->flags & (JIT_INSN_VALUE1_NEXT_USE |
				 				 JIT_INSN_VALUE1_LIVE)), 0);
			_jit_regs_free_reg(gen, reg, 1);
		}
		break;

		case JIT_OP_PUSH_STRUCT:
		{
			/* Load the pointer value to the top of the stack */
			if(!_jit_regs_is_top(gen, insn->value1) ||
			   _jit_regs_num_used(gen, 0) != 1)
			{
				_jit_regs_spill_all(gen);
			}
			reg = _jit_regs_load_to_top
				(gen, insn->value1,
				 (insn->flags & (JIT_INSN_VALUE1_NEXT_USE |
				 				 JIT_INSN_VALUE1_LIVE)), 0);
			_jit_regs_free_reg(gen, reg, 1);

			/* Push the structure at the designated pointer */
			size = jit_value_get_nint_constant(insn->value2);
			jit_cache_opcode(&(gen->posn), insn->opcode);
			jit_cache_native(&(gen->posn), size);
			adjust_working(gen, JIT_NUM_ITEMS_IN_STRUCT(size) - 1);
		}
		break;

		case JIT_OP_PUSH_RETURN_AREA_PTR:
		{
			/* Push the address of the interpreter's return area */
			_jit_regs_spill_all(gen);
			jit_cache_opcode(&(gen->posn), insn->opcode);
			adjust_working(gen, 1);
		}
		break;

		case JIT_OP_POP_STACK:
		{
			/* Pop parameter values from the stack after a function returns */
			size = jit_value_get_nint_constant(insn->value1);
			if(size == 1)
			{
				jit_cache_opcode(&(gen->posn), JIT_OP_POP);
			}
			else if(size == 2)
			{
				jit_cache_opcode(&(gen->posn), JIT_OP_POP_2);
			}
			else if(size == 3)
			{
				jit_cache_opcode(&(gen->posn), JIT_OP_POP_3);
			}
			else if(size != 0)
			{
				jit_cache_opcode(&(gen->posn), JIT_OP_POP_STACK);
				jit_cache_native(&(gen->posn), size);
			}
		}
		break;

		case JIT_OP_FLUSH_SMALL_STRUCT:
		{
			/* Flush a small structure return value back into the frame */
			_jit_gen_fix_value(insn->value1);
			if(insn->value1->frame_offset >= 0)
			{
				jit_cache_opcode(&(gen->posn), JIT_OP_LDLOCA);
				jit_cache_native(&(gen->posn), insn->value1->frame_offset);
			}
			else
			{
				jit_cache_opcode(&(gen->posn), JIT_OP_LDARGA);
				jit_cache_native
					(&(gen->posn), -(insn->value1->frame_offset + 1));
			}
			jit_cache_opcode(&(gen->posn), JIT_OP_PUSH_RETURN_SMALL_STRUCT);
			jit_cache_native
				(&(gen->posn), jit_type_get_size(insn->value1->type));
			adjust_working(gen, 2);
			jit_cache_opcode(&(gen->posn), JIT_OP_STORE_RELATIVE_STRUCT);
			jit_cache_native(&(gen->posn), 0);
			jit_cache_native
				(&(gen->posn), jit_type_get_size(insn->value1->type));
			adjust_working(gen, -2);
		}
		break;

		case JIT_OP_LOAD_RELATIVE_SBYTE:
		case JIT_OP_LOAD_RELATIVE_UBYTE:
		case JIT_OP_LOAD_RELATIVE_SHORT:
		case JIT_OP_LOAD_RELATIVE_USHORT:
		case JIT_OP_LOAD_RELATIVE_INT:
		case JIT_OP_LOAD_RELATIVE_LONG:
		case JIT_OP_LOAD_RELATIVE_FLOAT32:
		case JIT_OP_LOAD_RELATIVE_FLOAT64:
		case JIT_OP_LOAD_RELATIVE_NFLOAT:
		{
			/* Load a value from a relative pointer */
			reg = _jit_regs_load_to_top
				(gen, insn->value1,
				 (insn->flags & (JIT_INSN_VALUE1_NEXT_USE |
				 				 JIT_INSN_VALUE1_LIVE)), 0);
			offset = jit_value_get_nint_constant(insn->value2);
			jit_cache_opcode(&(gen->posn), insn->opcode);
			jit_cache_native(&(gen->posn), offset);
			record_dest(gen, insn, reg);
		}
		break;

		case JIT_OP_LOAD_RELATIVE_STRUCT:
		{
			/* Load a structured value from a relative pointer */
			reg = _jit_regs_load_to_top
				(gen, insn->value1,
				 (insn->flags & (JIT_INSN_VALUE1_NEXT_USE |
				 				 JIT_INSN_VALUE1_LIVE)), 0);
			offset = jit_value_get_nint_constant(insn->value2);
			size = (jit_nint)jit_type_get_size(jit_value_get_type(insn->dest));
			jit_cache_opcode(&(gen->posn), insn->opcode);
			jit_cache_native(&(gen->posn), offset);
			jit_cache_native(&(gen->posn), size);
			size = JIT_NUM_ITEMS_IN_STRUCT(size);
			record_dest(gen, insn, reg);
			adjust_working(gen, size - 1);
		}
		break;

		case JIT_OP_STORE_RELATIVE_BYTE:
		case JIT_OP_STORE_RELATIVE_SHORT:
		case JIT_OP_STORE_RELATIVE_INT:
		case JIT_OP_STORE_RELATIVE_LONG:
		case JIT_OP_STORE_RELATIVE_FLOAT32:
		case JIT_OP_STORE_RELATIVE_FLOAT64:
		case JIT_OP_STORE_RELATIVE_NFLOAT:
		{
			/* Store a value to a relative pointer */
			reg = _jit_regs_load_to_top_two
				(gen, insn->dest, insn->value1,
				 (insn->flags & (JIT_INSN_DEST_NEXT_USE |
				 				 JIT_INSN_DEST_LIVE)),
				 (insn->flags & (JIT_INSN_VALUE1_NEXT_USE |
				 				 JIT_INSN_VALUE1_LIVE)), 0);
			offset = jit_value_get_nint_constant(insn->value2);
			jit_cache_opcode(&(gen->posn), insn->opcode);
			jit_cache_native(&(gen->posn), offset);
			_jit_regs_free_reg(gen, reg, 1);
			adjust_working(gen, -2);
		}
		break;

		case JIT_OP_STORE_RELATIVE_STRUCT:
		{
			/* Store a structured value to a relative pointer */
			reg = _jit_regs_load_to_top_two
				(gen, insn->dest, insn->value1,
				 (insn->flags & (JIT_INSN_DEST_NEXT_USE |
				 				 JIT_INSN_DEST_LIVE)),
				 (insn->flags & (JIT_INSN_VALUE1_NEXT_USE |
				 				 JIT_INSN_VALUE1_LIVE)), 0);
			offset = jit_value_get_nint_constant(insn->value2);
			size = (jit_nint)jit_type_get_size
				(jit_value_get_type(insn->value1));
			jit_cache_opcode(&(gen->posn), insn->opcode);
			jit_cache_native(&(gen->posn), offset);
			jit_cache_native(&(gen->posn), size);
			_jit_regs_free_reg(gen, reg, 1);
			size = JIT_NUM_ITEMS_IN_STRUCT(size);
			adjust_working(gen, -(size + 1));
		}
		break;

		case JIT_OP_ADD_RELATIVE:
		{
			/* Add a relative offset to a pointer */
			reg = _jit_regs_load_to_top
				(gen, insn->value1,
				 (insn->flags & (JIT_INSN_VALUE1_NEXT_USE |
				 				 JIT_INSN_VALUE1_LIVE)), 0);
			offset = jit_value_get_nint_constant(insn->value2);
			if(offset != 0)
			{
				jit_cache_opcode(&(gen->posn), insn->opcode);
				jit_cache_native(&(gen->posn), offset);
			}
			record_dest(gen, insn, reg);
		}
		break;

		case JIT_OP_MARK_BREAKPOINT:
		{
			/* Mark the current location as a potential breakpoint */
			jit_cache_opcode(&(gen->posn), insn->opcode);
			jit_cache_native(&(gen->posn), insn->value1->address);
			jit_cache_native(&(gen->posn), insn->value2->address);
		}
		break;

		default:
		{
			/* Whatever opcodes are left are ordinary operators,
			   and the interpreter's opcode is identical to the JIT's */
			if(insn->value2 && (insn->flags & JIT_INSN_DEST_IS_VALUE) != 0)
			{
				/* Generate code for a ternary operator with no real dest */
				_jit_regs_load_to_top_three
					(gen, insn->dest, insn->value1, insn->value2,
					 (insn->flags & (JIT_INSN_DEST_NEXT_USE |
					 				 JIT_INSN_DEST_LIVE)),
					 (insn->flags & (JIT_INSN_VALUE1_NEXT_USE |
					 				 JIT_INSN_VALUE1_LIVE)),
					 (insn->flags & (JIT_INSN_VALUE2_NEXT_USE |
					 				 JIT_INSN_VALUE2_LIVE)), 0);
				jit_cache_opcode(&(gen->posn), insn->opcode);
				adjust_working(gen, -3);
			}
			else if(insn->value2)
			{
				/* Generate code for a binary operator */
				reg = _jit_regs_load_to_top_two
					(gen, insn->value1, insn->value2,
					 (insn->flags & (JIT_INSN_VALUE1_NEXT_USE |
					 				 JIT_INSN_VALUE1_LIVE)),
					 (insn->flags & (JIT_INSN_VALUE2_NEXT_USE |
					 				 JIT_INSN_VALUE2_LIVE)), 0);
				jit_cache_opcode(&(gen->posn), insn->opcode);
				adjust_working(gen, -1);
				if(insn->dest)
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
				else
				{
					/* This is a note, with the result left on the stack */
					_jit_regs_free_reg(gen, reg, 1);
				}
			}
			else
			{
				/* Generate code for a unary operator */
				reg = _jit_regs_load_to_top
					(gen, insn->value1,
					 (insn->flags & (JIT_INSN_VALUE1_NEXT_USE |
					 				 JIT_INSN_VALUE1_LIVE)), 0);
				jit_cache_opcode(&(gen->posn), insn->opcode);
				if(insn->dest)
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
				else
				{
					/* This is a note, with the result left on the stack */
					_jit_regs_free_reg(gen, reg, 1);
				}
			}
		}
		break;
	}
}

/*@
 * @deftypefun void _jit_gen_start_block (jit_gencode_t gen, jit_block_t block)
 * Called to notify the back end that the start of a basic block
 * has been reached.
 * @end deftypefun
@*/
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
		next = (void **)(fixup[1]);
		fixup[1] = (void *)(jit_nint)(((void **)(block->address)) - fixup);
		fixup = next;
	}
	block->fixup_list = 0;

	/* If this is the exception catcher block, then we need to update
	   the exception cookie for the function to point to here */
	if(block->label == block->func->builder->catcher_label &&
	   block->func->has_try)
	{
		_jit_cache_set_cookie(&(gen->posn), block->address);
	}
}

/*@
 * @deftypefun void _jit_gen_end_block (jit_gencode_t gen)
 * Called to notify the back end that the end of a basic block
 * has been reached.
 * @end deftypefun
@*/
void _jit_gen_end_block(jit_gencode_t gen, jit_block_t block)
{
	/* Reset the working area size to zero for the next block */
	gen->working_area = 0;
}

/*@
 * @deftypefun int _jit_gen_is_global_candidate (jit_type_t type)
 * Determine if @code{type} is a candidate for allocation within
 * global registers.
 * @end deftypefun
@*/
int _jit_gen_is_global_candidate(jit_type_t type)
{
	/* Global register allocation is not used by the interpreter */
	return 0;
}

#endif /* JIT_BACKEND_INTERP */
