/*
 * jit-interp.h - Bytecode interpreter for platforms without native support.
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

#if defined(__cplusplus)

/*
 * The JIT exception class.  Instances of this object are thrown
 * to simulate a JIT-level exception.
 */
class jit_exception
{
public:
	jit_exception(void *object) { this->object = object; }

	void *object;
};

#endif /* __cplusplus */

/*
 * Argument variable access opcodes.
 */
#define	JIT_OP_LDARG_SBYTE					(JIT_OP_NUM_OPCODES + 0x0000)
#define	JIT_OP_LDARG_UBYTE					(JIT_OP_NUM_OPCODES + 0x0001)
#define	JIT_OP_LDARG_SHORT					(JIT_OP_NUM_OPCODES + 0x0002)
#define	JIT_OP_LDARG_USHORT					(JIT_OP_NUM_OPCODES + 0x0003)
#define	JIT_OP_LDARG_INT					(JIT_OP_NUM_OPCODES + 0x0004)
#define	JIT_OP_LDARG_LONG					(JIT_OP_NUM_OPCODES + 0x0005)
#define	JIT_OP_LDARG_FLOAT32				(JIT_OP_NUM_OPCODES + 0x0006)
#define	JIT_OP_LDARG_FLOAT64				(JIT_OP_NUM_OPCODES + 0x0007)
#define	JIT_OP_LDARG_NFLOAT					(JIT_OP_NUM_OPCODES + 0x0008)
#define	JIT_OP_LDARG_STRUCT					(JIT_OP_NUM_OPCODES + 0x0009)
#define	JIT_OP_LDARGA						(JIT_OP_NUM_OPCODES + 0x000A)
#define	JIT_OP_STARG_BYTE					(JIT_OP_NUM_OPCODES + 0x000B)
#define	JIT_OP_STARG_SHORT					(JIT_OP_NUM_OPCODES + 0x000C)
#define	JIT_OP_STARG_INT					(JIT_OP_NUM_OPCODES + 0x000D)
#define	JIT_OP_STARG_LONG					(JIT_OP_NUM_OPCODES + 0x000E)
#define	JIT_OP_STARG_FLOAT32				(JIT_OP_NUM_OPCODES + 0x000F)
#define	JIT_OP_STARG_FLOAT64				(JIT_OP_NUM_OPCODES + 0x0010)
#define	JIT_OP_STARG_NFLOAT					(JIT_OP_NUM_OPCODES + 0x0011)
#define	JIT_OP_STARG_STRUCT					(JIT_OP_NUM_OPCODES + 0x0012)

/*
 * Local variable frame access opcodes.
 */
#define	JIT_OP_LDLOC_SBYTE					(JIT_OP_NUM_OPCODES + 0x0013)
#define	JIT_OP_LDLOC_UBYTE					(JIT_OP_NUM_OPCODES + 0x0014)
#define	JIT_OP_LDLOC_SHORT					(JIT_OP_NUM_OPCODES + 0x0015)
#define	JIT_OP_LDLOC_USHORT					(JIT_OP_NUM_OPCODES + 0x0016)
#define	JIT_OP_LDLOC_INT					(JIT_OP_NUM_OPCODES + 0x0017)
#define	JIT_OP_LDLOC_LONG					(JIT_OP_NUM_OPCODES + 0x0018)
#define	JIT_OP_LDLOC_FLOAT32				(JIT_OP_NUM_OPCODES + 0x0019)
#define	JIT_OP_LDLOC_FLOAT64				(JIT_OP_NUM_OPCODES + 0x001A)
#define	JIT_OP_LDLOC_NFLOAT					(JIT_OP_NUM_OPCODES + 0x001B)
#define	JIT_OP_LDLOC_STRUCT					(JIT_OP_NUM_OPCODES + 0x001C)
#define	JIT_OP_LDLOCA						(JIT_OP_NUM_OPCODES + 0x001D)
#define	JIT_OP_STLOC_BYTE					(JIT_OP_NUM_OPCODES + 0x001E)
#define	JIT_OP_STLOC_SHORT					(JIT_OP_NUM_OPCODES + 0x001F)
#define	JIT_OP_STLOC_INT					(JIT_OP_NUM_OPCODES + 0x0020)
#define	JIT_OP_STLOC_LONG					(JIT_OP_NUM_OPCODES + 0x0021)
#define	JIT_OP_STLOC_FLOAT32				(JIT_OP_NUM_OPCODES + 0x0022)
#define	JIT_OP_STLOC_FLOAT64				(JIT_OP_NUM_OPCODES + 0x0023)
#define	JIT_OP_STLOC_NFLOAT					(JIT_OP_NUM_OPCODES + 0x0024)
#define	JIT_OP_STLOC_STRUCT					(JIT_OP_NUM_OPCODES + 0x0025)

/*
 * Pointer check opcodes (interpreter only).
 */
#define	JIT_OP_CHECK_NULL_N					(JIT_OP_NUM_OPCODES + 0x0026)

/*
 * Stack management.
 */
#define	JIT_OP_POP							(JIT_OP_NUM_OPCODES + 0x0027)
#define	JIT_OP_POP_2						(JIT_OP_NUM_OPCODES + 0x0028)
#define	JIT_OP_POP_3						(JIT_OP_NUM_OPCODES + 0x0029)
#define	JIT_OP_PUSH_RETURN_INT				(JIT_OP_NUM_OPCODES + 0x002A)
#define	JIT_OP_PUSH_RETURN_LONG				(JIT_OP_NUM_OPCODES + 0x002B)
#define	JIT_OP_PUSH_RETURN_FLOAT32			(JIT_OP_NUM_OPCODES + 0x002C)
#define	JIT_OP_PUSH_RETURN_FLOAT64			(JIT_OP_NUM_OPCODES + 0x002D)
#define	JIT_OP_PUSH_RETURN_NFLOAT			(JIT_OP_NUM_OPCODES + 0x002E)
#define	JIT_OP_PUSH_RETURN_SMALL_STRUCT		(JIT_OP_NUM_OPCODES + 0x002F)
#define	JIT_OP_PUSH_RETURN_AREA_PTR			(JIT_OP_NUM_OPCODES + 0x0030)

/*
 * Nested function call handling.
 */
#define	JIT_OP_IMPORT_LOCAL					(JIT_OP_NUM_OPCODES + 0x0031)
#define	JIT_OP_IMPORT_ARG					(JIT_OP_NUM_OPCODES + 0x0032)

/*
 * Push constant values onto the stack.
 */
#define	JIT_OP_PUSH_CONST_INT				(JIT_OP_NUM_OPCODES + 0x0033)
#define	JIT_OP_PUSH_CONST_LONG				(JIT_OP_NUM_OPCODES + 0x0034)
#define	JIT_OP_PUSH_CONST_FLOAT32			(JIT_OP_NUM_OPCODES + 0x0035)
#define	JIT_OP_PUSH_CONST_FLOAT64			(JIT_OP_NUM_OPCODES + 0x0036)
#define	JIT_OP_PUSH_CONST_NFLOAT			(JIT_OP_NUM_OPCODES + 0x0037)

/*
 * Exception handling (interpreter-only).
 */
#define	JIT_OP_CALL_FINALLY					(JIT_OP_NUM_OPCODES + 0x0038)

/*
 * Marker opcode for the end of the interpreter-specific opcodes.
 */
#define	JIT_OP_END_MARKER					(JIT_OP_NUM_OPCODES + 0x003B)

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
