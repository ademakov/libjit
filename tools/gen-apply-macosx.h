/*
 * gen-apply-macosx.h - MacOSX-specific definitions.
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

#ifndef	_GEN_APPLY_MACOSX_H
#define	_GEN_APPLY_MACOSX_H

#ifdef	__cplusplus
extern "C" {
#endif

#define JIT_APPLY_NUM_WORD_REGS 8
#define JIT_APPLY_NUM_FLOAT_REGS 13
#define JIT_APPLY_PASS_STACK_FLOAT_AS_DOUBLE 0
#define JIT_APPLY_PASS_STACK_FLOAT_AS_NFLOAT 0
#define JIT_APPLY_PASS_STACK_DOUBLE_AS_NFLOAT 0
#define JIT_APPLY_PASS_STACK_NFLOAT_AS_DOUBLE 1
#define JIT_APPLY_PASS_REG_FLOAT_AS_DOUBLE 1
#define JIT_APPLY_PASS_REG_FLOAT_AS_NFLOAT 0
#define JIT_APPLY_PASS_REG_DOUBLE_AS_NFLOAT 0
#define JIT_APPLY_PASS_REG_NFLOAT_AS_DOUBLE 1
#define JIT_APPLY_RETURN_FLOAT_AS_DOUBLE 1
#define JIT_APPLY_RETURN_FLOAT_AS_NFLOAT 0
#define JIT_APPLY_RETURN_DOUBLE_AS_NFLOAT 0
#define JIT_APPLY_RETURN_NFLOAT_AS_DOUBLE 1
#define JIT_APPLY_FLOATS_IN_WORD_REGS 0
#define JIT_APPLY_RETURN_FLOATS_AFTER 8
#define JIT_APPLY_VARARGS_ON_STACK 0
#define JIT_APPLY_STRUCT_RETURN_SPECIAL_REG 0
#define JIT_APPLY_STRUCT_REG_OVERLAPS_WORD_REG 0
#define JIT_APPLY_ALIGN_LONG_REGS 1
#define JIT_APPLY_ALIGN_LONG_STACK 1
#define JIT_APPLY_CAN_SPLIT_LONG 0
#define JIT_APPLY_STRUCT_RETURN_IN_REG \
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
#define JIT_APPLY_MAX_STRUCT_IN_REG 0
#define JIT_APPLY_X86_FASTCALL 0
#define JIT_APPLY_PARENT_FRAME_OFFSET 0
#define JIT_APPLY_RETURN_ADDRESS_OFFSET 0
#define JIT_APPLY_BROKEN_FRAME_BUILTINS 1
#define JIT_APPLY_X86_POP_STRUCT_RETURN 0
#define JIT_APPLY_PAD_FLOAT_REGS 1

#define JIT_APPLY_NUM_DOUBLE_REGS 0
#define JIT_APPLY_NUM_NFLOAT_REGS 0
#define JIT_APPLY_DOUBLES_IN_WORD_REGS 0
#define JIT_APPLY_NFLOATS_IN_WORD_REGS 0
#define JIT_APPLY_RETURN_DOUBLES_AFTER 0
#define JIT_APPLY_RETURN_NFLOATS_AFTER 0

#ifdef	__cplusplus
};
#endif

#endif	/* _GEN_APPLY_MACOSX_H */
