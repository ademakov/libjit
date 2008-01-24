/*
 * jit-interp.h - Bytecode interpreter for platforms without native support.
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

#ifndef	_JIT_INTERP_H
#define	_JIT_INTERP_H

#include "jit-internal.h"
#include "jit-apply-rules.h"

#ifdef	__cplusplus
extern	"C" {
#endif

/*
 * Structure of a stack item.
 */
typedef union
{
	jit_int		int_value;
	jit_uint	uint_value;
	jit_long	long_value;
	jit_ulong	ulong_value;
	jit_float32	float32_value;
	jit_float64	float64_value;
	jit_nfloat	nfloat_value;
	void	   *ptr_value;
#if JIT_APPLY_MAX_STRUCT_IN_REG != 0
	char		struct_value[JIT_APPLY_MAX_STRUCT_IN_REG];
#endif

} jit_item;

/*
 * Number of items that make up a struct or union value on the stack.
 */
#define	JIT_NUM_ITEMS_IN_STRUCT(size)		\
	(((size) + (sizeof(jit_item) - 1)) / sizeof(jit_item))

/*
 * Information that is prefixed to a function that describes
 * its interpretation context.  The code starts just after this.
 */
typedef struct jit_function_interp *jit_function_interp_t;
struct jit_function_interp
{
	/* The function that this structure is associated with */
	jit_function_t	func;

	/* Size of the argument area to allocate, in bytes */
	unsigned int	args_size;

	/* Size of the local stack frame to allocate, in bytes */
	unsigned int	frame_size;

	/* Size of the working stack area of the frame, in items */
	unsigned int	working_area;
};

/*
 * Get the size of the "jit_function_interp" structure, rounded
 * up to a multiple of "void *".
 */
#define	jit_function_interp_size	\
			((sizeof(struct jit_function_interp) + sizeof(void *) - 1) & \
			 ~(sizeof(void *) - 1))

/*
 * Get the entry point for a function, from its "jit_function_interp_t" block.
 */
#define	jit_function_interp_entry_pc(info)	\
			((void **)(((unsigned char *)(info)) + jit_function_interp_size))

/*
 * Argument variable access opcodes.
 */
#define	JIT_OP_LDA_0_SBYTE			(JIT_OP_NUM_OPCODES + 0x0000)
#define	JIT_OP_LDA_0_UBYTE			(JIT_OP_NUM_OPCODES + 0x0001)
#define	JIT_OP_LDA_0_SHORT			(JIT_OP_NUM_OPCODES + 0x0002)
#define	JIT_OP_LDA_0_USHORT			(JIT_OP_NUM_OPCODES + 0x0003)
#define	JIT_OP_LDA_0_INT			(JIT_OP_NUM_OPCODES + 0x0004)
#define	JIT_OP_LDA_0_LONG			(JIT_OP_NUM_OPCODES + 0x0005)
#define	JIT_OP_LDA_0_FLOAT32			(JIT_OP_NUM_OPCODES + 0x0006)
#define	JIT_OP_LDA_0_FLOAT64			(JIT_OP_NUM_OPCODES + 0x0007)
#define	JIT_OP_LDA_0_NFLOAT			(JIT_OP_NUM_OPCODES + 0x0008)
#define	JIT_OP_LDAA_0				(JIT_OP_NUM_OPCODES + 0x0009)
#define	JIT_OP_LDA_1_SBYTE			(JIT_OP_NUM_OPCODES + 0x000a)
#define	JIT_OP_LDA_1_UBYTE			(JIT_OP_NUM_OPCODES + 0x000b)
#define	JIT_OP_LDA_1_SHORT			(JIT_OP_NUM_OPCODES + 0x000c)
#define	JIT_OP_LDA_1_USHORT			(JIT_OP_NUM_OPCODES + 0x000d)
#define	JIT_OP_LDA_1_INT			(JIT_OP_NUM_OPCODES + 0x000e)
#define	JIT_OP_LDA_1_LONG			(JIT_OP_NUM_OPCODES + 0x000f)
#define	JIT_OP_LDA_1_FLOAT32			(JIT_OP_NUM_OPCODES + 0x0010)
#define	JIT_OP_LDA_1_FLOAT64			(JIT_OP_NUM_OPCODES + 0x0011)
#define	JIT_OP_LDA_1_NFLOAT			(JIT_OP_NUM_OPCODES + 0x0012)
#define	JIT_OP_LDAA_1				(JIT_OP_NUM_OPCODES + 0x0013)
#define	JIT_OP_LDA_2_SBYTE			(JIT_OP_NUM_OPCODES + 0x0014)
#define	JIT_OP_LDA_2_UBYTE			(JIT_OP_NUM_OPCODES + 0x0015)
#define	JIT_OP_LDA_2_SHORT			(JIT_OP_NUM_OPCODES + 0x0016)
#define	JIT_OP_LDA_2_USHORT			(JIT_OP_NUM_OPCODES + 0x0017)
#define	JIT_OP_LDA_2_INT			(JIT_OP_NUM_OPCODES + 0x0018)
#define	JIT_OP_LDA_2_LONG			(JIT_OP_NUM_OPCODES + 0x0019)
#define	JIT_OP_LDA_2_FLOAT32			(JIT_OP_NUM_OPCODES + 0x001a)
#define	JIT_OP_LDA_2_FLOAT64			(JIT_OP_NUM_OPCODES + 0x001b)
#define	JIT_OP_LDA_2_NFLOAT			(JIT_OP_NUM_OPCODES + 0x001c)
#define	JIT_OP_LDAA_2				(JIT_OP_NUM_OPCODES + 0x001d)
#define	JIT_OP_STA_0_BYTE			(JIT_OP_NUM_OPCODES + 0x001e)
#define	JIT_OP_STA_0_SHORT			(JIT_OP_NUM_OPCODES + 0x001f)
#define	JIT_OP_STA_0_INT			(JIT_OP_NUM_OPCODES + 0x0020)
#define	JIT_OP_STA_0_LONG			(JIT_OP_NUM_OPCODES + 0x0021)
#define	JIT_OP_STA_0_FLOAT32			(JIT_OP_NUM_OPCODES + 0x0022)
#define	JIT_OP_STA_0_FLOAT64			(JIT_OP_NUM_OPCODES + 0x0023)
#define	JIT_OP_STA_0_NFLOAT			(JIT_OP_NUM_OPCODES + 0x0024)

/*
 * Local variable frame access opcodes.
 */
#define	JIT_OP_LDL_0_SBYTE			(JIT_OP_NUM_OPCODES + 0x0025)
#define	JIT_OP_LDL_0_UBYTE			(JIT_OP_NUM_OPCODES + 0x0026)
#define	JIT_OP_LDL_0_SHORT			(JIT_OP_NUM_OPCODES + 0x0027)
#define	JIT_OP_LDL_0_USHORT			(JIT_OP_NUM_OPCODES + 0x0028)
#define	JIT_OP_LDL_0_INT			(JIT_OP_NUM_OPCODES + 0x0029)
#define	JIT_OP_LDL_0_LONG			(JIT_OP_NUM_OPCODES + 0x002a)
#define	JIT_OP_LDL_0_FLOAT32			(JIT_OP_NUM_OPCODES + 0x002b)
#define	JIT_OP_LDL_0_FLOAT64			(JIT_OP_NUM_OPCODES + 0x002c)
#define	JIT_OP_LDL_0_NFLOAT			(JIT_OP_NUM_OPCODES + 0x002d)
#define	JIT_OP_LDLA_0				(JIT_OP_NUM_OPCODES + 0x002e)
#define	JIT_OP_LDL_1_SBYTE			(JIT_OP_NUM_OPCODES + 0x002f)
#define	JIT_OP_LDL_1_UBYTE			(JIT_OP_NUM_OPCODES + 0x0030)
#define	JIT_OP_LDL_1_SHORT			(JIT_OP_NUM_OPCODES + 0x0031)
#define	JIT_OP_LDL_1_USHORT			(JIT_OP_NUM_OPCODES + 0x0032)
#define	JIT_OP_LDL_1_INT			(JIT_OP_NUM_OPCODES + 0x0033)
#define	JIT_OP_LDL_1_LONG			(JIT_OP_NUM_OPCODES + 0x0034)
#define	JIT_OP_LDL_1_FLOAT32			(JIT_OP_NUM_OPCODES + 0x0035)
#define	JIT_OP_LDL_1_FLOAT64			(JIT_OP_NUM_OPCODES + 0x0036)
#define	JIT_OP_LDL_1_NFLOAT			(JIT_OP_NUM_OPCODES + 0x0037)
#define	JIT_OP_LDLA_1				(JIT_OP_NUM_OPCODES + 0x0038)
#define	JIT_OP_LDL_2_SBYTE			(JIT_OP_NUM_OPCODES + 0x0039)
#define	JIT_OP_LDL_2_UBYTE			(JIT_OP_NUM_OPCODES + 0x003a)
#define	JIT_OP_LDL_2_SHORT			(JIT_OP_NUM_OPCODES + 0x003b)
#define	JIT_OP_LDL_2_USHORT			(JIT_OP_NUM_OPCODES + 0x003c)
#define	JIT_OP_LDL_2_INT			(JIT_OP_NUM_OPCODES + 0x003d)
#define	JIT_OP_LDL_2_LONG			(JIT_OP_NUM_OPCODES + 0x003e)
#define	JIT_OP_LDL_2_FLOAT32			(JIT_OP_NUM_OPCODES + 0x003f)
#define	JIT_OP_LDL_2_FLOAT64			(JIT_OP_NUM_OPCODES + 0x0040)
#define	JIT_OP_LDL_2_NFLOAT			(JIT_OP_NUM_OPCODES + 0x0041)
#define	JIT_OP_LDLA_2				(JIT_OP_NUM_OPCODES + 0x0042)
#define	JIT_OP_STL_0_BYTE			(JIT_OP_NUM_OPCODES + 0x0043)
#define	JIT_OP_STL_0_SHORT			(JIT_OP_NUM_OPCODES + 0x0044)
#define	JIT_OP_STL_0_INT			(JIT_OP_NUM_OPCODES + 0x0045)
#define	JIT_OP_STL_0_LONG			(JIT_OP_NUM_OPCODES + 0x0046)
#define	JIT_OP_STL_0_FLOAT32			(JIT_OP_NUM_OPCODES + 0x0047)
#define	JIT_OP_STL_0_FLOAT64			(JIT_OP_NUM_OPCODES + 0x0048)
#define	JIT_OP_STL_0_NFLOAT			(JIT_OP_NUM_OPCODES + 0x0049)

/*
 * Load constant values.
 */
#define	JIT_OP_LDC_0_INT			(JIT_OP_NUM_OPCODES + 0x004a)
#define	JIT_OP_LDC_1_INT			(JIT_OP_NUM_OPCODES + 0x004b)
#define	JIT_OP_LDC_2_INT			(JIT_OP_NUM_OPCODES + 0x004c)
#define	JIT_OP_LDC_0_LONG			(JIT_OP_NUM_OPCODES + 0x004d)
#define	JIT_OP_LDC_1_LONG			(JIT_OP_NUM_OPCODES + 0x004e)
#define	JIT_OP_LDC_2_LONG			(JIT_OP_NUM_OPCODES + 0x004f)
#define	JIT_OP_LDC_0_FLOAT32			(JIT_OP_NUM_OPCODES + 0x0050)
#define	JIT_OP_LDC_1_FLOAT32			(JIT_OP_NUM_OPCODES + 0x0051)
#define	JIT_OP_LDC_2_FLOAT32			(JIT_OP_NUM_OPCODES + 0x0052)
#define	JIT_OP_LDC_0_FLOAT64			(JIT_OP_NUM_OPCODES + 0x0053)
#define	JIT_OP_LDC_1_FLOAT64			(JIT_OP_NUM_OPCODES + 0x0054)
#define	JIT_OP_LDC_2_FLOAT64			(JIT_OP_NUM_OPCODES + 0x0055)
#define	JIT_OP_LDC_0_NFLOAT			(JIT_OP_NUM_OPCODES + 0x0056)
#define	JIT_OP_LDC_1_NFLOAT			(JIT_OP_NUM_OPCODES + 0x0057)
#define	JIT_OP_LDC_2_NFLOAT			(JIT_OP_NUM_OPCODES + 0x0058)

/*
 * Load return value.
 */
#define	JIT_OP_LDR_0_INT			(JIT_OP_NUM_OPCODES + 0x0059)
#define	JIT_OP_LDR_0_LONG			(JIT_OP_NUM_OPCODES + 0x005a)
#define	JIT_OP_LDR_0_FLOAT32			(JIT_OP_NUM_OPCODES + 0x005b)
#define	JIT_OP_LDR_0_FLOAT64			(JIT_OP_NUM_OPCODES + 0x005c)
#define	JIT_OP_LDR_0_NFLOAT			(JIT_OP_NUM_OPCODES + 0x005d)

/*
 * Stack management.
 */
#define	JIT_OP_POP				(JIT_OP_NUM_OPCODES + 0x005e)
#define	JIT_OP_POP_2				(JIT_OP_NUM_OPCODES + 0x005f)
#define	JIT_OP_POP_3				(JIT_OP_NUM_OPCODES + 0x0060)

/*
 * Nested function call handling.
 */
#define	JIT_OP_IMPORT_LOCAL			(JIT_OP_NUM_OPCODES + 0x0061)
#define	JIT_OP_IMPORT_ARG			(JIT_OP_NUM_OPCODES + 0x0062)

/*
 * Marker opcode for the end of the interpreter-specific opcodes.
 */
#define	JIT_OP_END_MARKER			(JIT_OP_NUM_OPCODES + 0x0063)

/*
 * Number of interpreter-specific opcodes.
 */
#define	JIT_OP_NUM_INTERP_OPCODES	\
		(JIT_OP_END_MARKER + 1 - JIT_OP_NUM_OPCODES)

/*
 * Opcode version.  Should be increased whenever new opcodes
 * are added to this list or the public list in "jit-opcode.h".
 * This value is written to ELF binaries, to ensure that code
 * for one version of libjit is not inadvertantly used in another.
 */
#define	JIT_OPCODE_VERSION					0

/*
 * Additional opcode definition flags.
 */
#define	JIT_OPCODE_INTERP_ARGS_MASK			0x7E000000
#define	JIT_OPCODE_NINT_ARG					0x02000000
#define	JIT_OPCODE_NINT_ARG_TWO				0x04000000
#define	JIT_OPCODE_CONST_LONG				0x06000000
#define	JIT_OPCODE_CONST_FLOAT32			0x08000000
#define	JIT_OPCODE_CONST_FLOAT64			0x0A000000
#define	JIT_OPCODE_CONST_NFLOAT				0x0C000000
#define	JIT_OPCODE_CALL_INDIRECT_ARGS		0x0E000000

#ifdef	__cplusplus
};
#endif

#endif /* _JIT_INTERP_H */
