/*
 * jit-opcode.c - Information about all of the JIT opcodes.
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

#include "jit-internal.h"
#include "jit-rules.h"

#define	F_(dest,src1,src2)	\
	(JIT_OPCODE_DEST_##dest | JIT_OPCODE_SRC1_##src1 | JIT_OPCODE_SRC2_##src2)
#define	O_(dest,src1,src2,oper)	\
	(JIT_OPCODE_DEST_##dest | JIT_OPCODE_SRC1_##src1 | \
	 JIT_OPCODE_SRC2_##src2 | JIT_OPCODE_OPER_##oper)
#define	B_(src1,src2)	\
	(JIT_OPCODE_IS_BRANCH | JIT_OPCODE_SRC1_##src1 | JIT_OPCODE_SRC2_##src2)
#define	A_(src1,src2,oper)	\
	(JIT_OPCODE_IS_BRANCH | JIT_OPCODE_SRC1_##src1 | \
	 JIT_OPCODE_SRC2_##src2 | JIT_OPCODE_OPER_##oper)
#if defined(JIT_BACKEND_INTERP)
	#define	NINT_ARG			JIT_OPCODE_NINT_ARG
	#define	NINT_ARG_TWO		JIT_OPCODE_NINT_ARG_TWO
	#define	INDIRECT_ARGS		JIT_OPCODE_CALL_INDIRECT_ARGS
#else
	#define	NINT_ARG			0
	#define	NINT_ARG_TWO		0
	#define	INDIRECT_ARGS		0
#endif

jit_opcode_info_t const jit_opcodes[JIT_OP_NUM_OPCODES] = {

	/*
	 * Simple opcodes.
	 */
	{"nop",							F_(EMPTY, EMPTY, EMPTY)},

	/*
 	 * Conversion opcodes.
 	 */
	{"trunc_sbyte",					F_(INT, INT, EMPTY)},
	{"trunc_ubyte",					F_(INT, INT, EMPTY)},
	{"trunc_short",					F_(INT, INT, EMPTY)},
	{"trunc_ushort",				F_(INT, INT, EMPTY)},
	{"trunc_int",					F_(INT, INT, EMPTY)},
	{"trunc_uint",					F_(INT, INT, EMPTY)},
	{"check_sbyte",					F_(INT, INT, EMPTY)},
	{"check_ubyte",					F_(INT, INT, EMPTY)},
	{"check_short",					F_(INT, INT, EMPTY)},
	{"check_ushort",				F_(INT, INT, EMPTY)},
	{"check_int",					F_(INT, INT, EMPTY)},
	{"check_uint",					F_(INT, INT, EMPTY)},
	{"low_word",					F_(INT, LONG, EMPTY)},
	{"expand_int",					F_(LONG, INT, EMPTY)},
	{"expand_uint",					F_(LONG, INT, EMPTY)},
	{"check_low_word",				F_(INT, LONG, EMPTY)},
	{"check_signed_low_word",		F_(INT, LONG, EMPTY)},
	{"check_long",					F_(LONG, LONG, EMPTY)},
	{"check_ulong",					F_(LONG, LONG, EMPTY)},
	{"nfloat_to_int",				F_(INT, NFLOAT, EMPTY)},
	{"nfloat_to_uint",				F_(INT, NFLOAT, EMPTY)},
	{"nfloat_to_long",				F_(LONG, NFLOAT, EMPTY)},
	{"nfloat_to_ulong",				F_(LONG, NFLOAT, EMPTY)},
	{"check_nfloat_to_int",			F_(INT, NFLOAT, EMPTY)},
	{"check_nfloat_to_uint",		F_(INT, NFLOAT, EMPTY)},
	{"check_nfloat_to_long",		F_(LONG, NFLOAT, EMPTY)},
	{"check_nfloat_to_ulong",		F_(LONG, NFLOAT, EMPTY)},
	{"int_to_nfloat",				F_(NFLOAT, INT, EMPTY)},
	{"uint_to_nfloat",				F_(NFLOAT, INT, EMPTY)},
	{"long_to_nfloat",				F_(NFLOAT, LONG, EMPTY)},
	{"ulong_to_nfloat",				F_(NFLOAT, LONG, EMPTY)},
	{"nfloat_to_float32",			F_(FLOAT32, NFLOAT, EMPTY)},
	{"nfloat_to_float64",			F_(FLOAT64, NFLOAT, EMPTY)},
	{"float32_to_nfloat",			F_(NFLOAT, FLOAT32, EMPTY)},
	{"float64_to_nfloat",			F_(NFLOAT, FLOAT64, EMPTY)},

	/*
	 * Arithmetic opcodes.
	 */
	{"iadd",						O_(INT, INT, INT, ADD)},
	{"iadd_ovf",					F_(INT, INT, INT)},
	{"iadd_ovf_un",					F_(INT, INT, INT)},
	{"isub",						O_(INT, INT, INT, SUB)},
	{"isub_ovf",					F_(INT, INT, INT)},
	{"isub_ovf_un",					F_(INT, INT, INT)},
	{"imul",						O_(INT, INT, INT, MUL)},
	{"imul_ovf",					F_(INT, INT, INT)},
	{"imul_ovf_un",					F_(INT, INT, INT)},
	{"idiv",						O_(INT, INT, INT, DIV)},
	{"idiv_un",						F_(INT, INT, INT)},
	{"irem",						O_(INT, INT, INT, REM)},
	{"irem_un",						F_(INT, INT, INT)},
	{"ineg",						O_(INT, INT, EMPTY, NEG)},
	{"ladd",						O_(LONG, LONG, LONG, ADD)},
	{"ladd_ovf",					F_(LONG, LONG, LONG)},
	{"ladd_ovf_un",					F_(LONG, LONG, LONG)},
	{"lsub",						O_(LONG, LONG, LONG, SUB)},
	{"lsub_ovf",					F_(LONG, LONG, LONG)},
	{"lsub_ovf_un",					F_(LONG, LONG, LONG)},
	{"lmul",						O_(LONG, LONG, LONG, MUL)},
	{"lmul_ovf",					F_(LONG, LONG, LONG)},
	{"lmul_ovf_un",					F_(LONG, LONG, LONG)},
	{"ldiv",						O_(LONG, LONG, LONG, DIV)},
	{"ldiv_un",						F_(LONG, LONG, LONG)},
	{"lrem",						O_(LONG, LONG, LONG, REM)},
	{"lrem_un",						F_(LONG, LONG, LONG)},
	{"lneg",						O_(LONG, LONG, EMPTY, NEG)},
	{"fadd",						O_(FLOAT32, FLOAT32, FLOAT32, ADD)},
	{"fsub",						O_(FLOAT32, FLOAT32, FLOAT32, SUB)},
	{"fmul",						O_(FLOAT32, FLOAT32, FLOAT32, MUL)},
	{"fdiv",						O_(FLOAT32, FLOAT32, FLOAT32, DIV)},
	{"frem",						O_(FLOAT32, FLOAT32, FLOAT32, REM)},
	{"frem_ieee",					F_(FLOAT32, FLOAT32, FLOAT32)},
	{"fneg",						O_(FLOAT32, FLOAT32, EMPTY, NEG)},
	{"dadd",						O_(FLOAT64, FLOAT64, FLOAT64, ADD)},
	{"dsub",						O_(FLOAT64, FLOAT64, FLOAT64, SUB)},
	{"dmul",						O_(FLOAT64, FLOAT64, FLOAT64, MUL)},
	{"ddiv",						O_(FLOAT64, FLOAT64, FLOAT64, DIV)},
	{"drem",						O_(FLOAT64, FLOAT64, FLOAT64, REM)},
	{"drem_ieee",					F_(FLOAT64, FLOAT64, FLOAT64)},
	{"dneg",						O_(FLOAT64, FLOAT64, EMPTY, NEG)},
	{"nfadd",						O_(NFLOAT, NFLOAT, NFLOAT, ADD)},
	{"nfsub",						O_(NFLOAT, NFLOAT, NFLOAT, SUB)},
	{"nfmul",						O_(NFLOAT, NFLOAT, NFLOAT, MUL)},
	{"nfdiv",						O_(NFLOAT, NFLOAT, NFLOAT, DIV)},
	{"nfrem",						O_(NFLOAT, NFLOAT, NFLOAT, REM)},
	{"nfrem_ieee",					F_(NFLOAT, NFLOAT, NFLOAT)},
	{"nfneg",						O_(NFLOAT, NFLOAT, EMPTY, NEG)},

	/*
	 * Bitwise opcodes.
 	 */
	{"iand",						O_(INT, INT, INT, AND)},
	{"ior",							O_(INT, INT, INT, OR)},
	{"ixor",						O_(INT, INT, INT, XOR)},
	{"inot",						O_(INT, INT, EMPTY, NOT)},
	{"ishl",						O_(INT, INT, INT, SHL)},
	{"ishr",						O_(INT, INT, INT, SHR)},
	{"ishr_un",						O_(INT, INT, INT, SHR_UN)},
	{"land",						O_(LONG, LONG, LONG, AND)},
	{"lor",							O_(LONG, LONG, LONG, OR)},
	{"lxor",						O_(LONG, LONG, LONG, XOR)},
	{"lnot",						O_(LONG, LONG, EMPTY, NOT)},
	{"lshl",						O_(LONG, LONG, INT, SHL)},
	{"lshr",						O_(LONG, LONG, INT, SHR)},
	{"lshr_un",						O_(LONG, LONG, INT, SHR_UN)},

	/*
	 * Branch opcodes.
	 */
	{"br",							B_(EMPTY, EMPTY)},
	{"br_ifalse",					B_(INT, EMPTY)},
	{"br_itrue",					B_(INT, EMPTY)},
	{"br_ieq",						A_(INT, INT, EQ)},
	{"br_ine",						A_(INT, INT, NE)},
	{"br_ilt",						A_(INT, INT, LT)},
	{"br_ilt_un",					B_(INT, INT)},
	{"br_ile",						A_(INT, INT, LE)},
	{"br_ile_un",					B_(INT, INT)},
	{"br_igt",						A_(INT, INT, GT)},
	{"br_igt_un",					B_(INT, INT)},
	{"br_ige",						A_(INT, INT, GE)},
	{"br_ige_un",					B_(INT, INT)},
	{"br_lfalse",					B_(LONG, EMPTY)},
	{"br_ltrue",					B_(LONG, EMPTY)},
	{"br_leq",						A_(LONG, LONG, EQ)},
	{"br_lne",						A_(LONG, LONG, NE)},
	{"br_llt",						A_(LONG, LONG, LT)},
	{"br_llt_un",					B_(LONG, LONG)},
	{"br_lle",						A_(LONG, LONG, LE)},
	{"br_lle_un",					B_(LONG, LONG)},
	{"br_lgt",						A_(LONG, LONG, GT)},
	{"br_lgt_un",					B_(LONG, LONG)},
	{"br_lge",						A_(LONG, LONG, GE)},
	{"br_lge_un",					B_(LONG, LONG)},
	{"br_feq",						A_(FLOAT32, FLOAT32, EQ)},
	{"br_fne",						A_(FLOAT32, FLOAT32, NE)},
	{"br_flt",						A_(FLOAT32, FLOAT32, LT)},
	{"br_fle",						A_(FLOAT32, FLOAT32, LE)},
	{"br_fgt",						A_(FLOAT32, FLOAT32, GT)},
	{"br_fge",						A_(FLOAT32, FLOAT32, GE)},
	{"br_feq_inv",					B_(FLOAT32, FLOAT32)},
	{"br_fne_inv",					B_(FLOAT32, FLOAT32)},
	{"br_flt_inv",					B_(FLOAT32, FLOAT32)},
	{"br_fle_inv",					B_(FLOAT32, FLOAT32)},
	{"br_fgt_inv",					B_(FLOAT32, FLOAT32)},
	{"br_fge_inv",					B_(FLOAT32, FLOAT32)},
	{"br_deq",						A_(FLOAT64, FLOAT64, EQ)},
	{"br_dne",						A_(FLOAT64, FLOAT64, NE)},
	{"br_dlt",						A_(FLOAT64, FLOAT64, LT)},
	{"br_dle",						A_(FLOAT64, FLOAT64, LE)},
	{"br_dgt",						A_(FLOAT64, FLOAT64, GT)},
	{"br_dge",						A_(FLOAT64, FLOAT64, GE)},
	{"br_deq_inv",					B_(FLOAT64, FLOAT64)},
	{"br_dne_inv",					B_(FLOAT64, FLOAT64)},
	{"br_dlt_inv",					B_(FLOAT64, FLOAT64)},
	{"br_dle_inv",					B_(FLOAT64, FLOAT64)},
	{"br_dgt_inv",					B_(FLOAT64, FLOAT64)},
	{"br_dge_inv",					B_(FLOAT64, FLOAT64)},
	{"br_nfeq",						A_(NFLOAT, NFLOAT, EQ)},
	{"br_nfne",						A_(NFLOAT, NFLOAT, NE)},
	{"br_nflt",						A_(NFLOAT, NFLOAT, LT)},
	{"br_nfle",						A_(NFLOAT, NFLOAT, LE)},
	{"br_nfgt",						A_(NFLOAT, NFLOAT, GT)},
	{"br_nfge",						A_(NFLOAT, NFLOAT, GE)},
	{"br_nfeq_inv",					B_(NFLOAT, NFLOAT)},
	{"br_nfne_inv",					B_(NFLOAT, NFLOAT)},
	{"br_nflt_inv",					B_(NFLOAT, NFLOAT)},
	{"br_nfle_inv",					B_(NFLOAT, NFLOAT)},
	{"br_nfgt_inv",					B_(NFLOAT, NFLOAT)},
	{"br_nfge_inv",					B_(NFLOAT, NFLOAT)},

	/*
	 * Comparison opcodes.
	 */
	{"icmp",						F_(INT, INT, INT)},
	{"icmp_un",						F_(INT, INT, INT)},
	{"lcmp",						F_(INT, LONG, LONG)},
	{"lcmp_un",						F_(INT, LONG, LONG)},
	{"fcmpl",						F_(INT, FLOAT32, FLOAT32)},
	{"fcmpg",						F_(INT, FLOAT32, FLOAT32)},
	{"dcmpl",						F_(INT, FLOAT64, FLOAT64)},
	{"dcmpg",						F_(INT, FLOAT64, FLOAT64)},
	{"nfcmpl",						F_(INT, NFLOAT, NFLOAT)},
	{"nfcmpg",						F_(INT, NFLOAT, NFLOAT)},
	{"ieq",							O_(INT, INT, INT, EQ)},
	{"ine",							O_(INT, INT, INT, NE)},
	{"ilt",							O_(INT, INT, INT, LT)},
	{"ilt_un",						F_(INT, INT, INT)},
	{"ile",							O_(INT, INT, INT, LE)},
	{"ile_un",						F_(INT, INT, INT)},
	{"igt",							O_(INT, INT, INT, GT)},
	{"igt_un",						F_(INT, INT, INT)},
	{"ige",							O_(INT, INT, INT, GE)},
	{"ige_un",						F_(INT, INT, INT)},
	{"leq",							O_(INT, LONG, LONG, EQ)},
	{"lne",							O_(INT, LONG, LONG, NE)},
	{"llt",							O_(INT, LONG, LONG, LT)},
	{"llt_un",						F_(INT, LONG, LONG)},
	{"lle",							O_(INT, LONG, LONG, LE)},
	{"lle_un",						F_(INT, LONG, LONG)},
	{"lgt",							O_(INT, LONG, LONG, GT)},
	{"lgt_un",						F_(INT, LONG, LONG)},
	{"lge",							O_(INT, LONG, LONG, GE)},
	{"lge_un",						F_(INT, LONG, LONG)},
	{"feq",							O_(INT, FLOAT32, FLOAT32, EQ)},
	{"fne",							O_(INT, FLOAT32, FLOAT32, NE)},
	{"flt",							O_(INT, FLOAT32, FLOAT32, LT)},
	{"fle",							O_(INT, FLOAT32, FLOAT32, LE)},
	{"fgt",							O_(INT, FLOAT32, FLOAT32, GT)},
	{"fge",							O_(INT, FLOAT32, FLOAT32, GE)},
	{"feq_inv",						F_(INT, FLOAT32, FLOAT32)},
	{"fne_inv",						F_(INT, FLOAT32, FLOAT32)},
	{"flt_inv",						F_(INT, FLOAT32, FLOAT32)},
	{"fle_inv",						F_(INT, FLOAT32, FLOAT32)},
	{"fgt_inv",						F_(INT, FLOAT32, FLOAT32)},
	{"fge_inv",						F_(INT, FLOAT32, FLOAT32)},
	{"deq",							O_(INT, FLOAT64, FLOAT64, EQ)},
	{"dne",							O_(INT, FLOAT64, FLOAT64, NE)},
	{"dlt",							O_(INT, FLOAT64, FLOAT64, LT)},
	{"dle",							O_(INT, FLOAT64, FLOAT64, LE)},
	{"dgt",							O_(INT, FLOAT64, FLOAT64, GT)},
	{"dge",							O_(INT, FLOAT64, FLOAT64, GE)},
	{"deq_inv",						F_(INT, FLOAT64, FLOAT64)},
	{"dne_inv",						F_(INT, FLOAT64, FLOAT64)},
	{"dlt_inv",						F_(INT, FLOAT64, FLOAT64)},
	{"dle_inv",						F_(INT, FLOAT64, FLOAT64)},
	{"dgt_inv",						F_(INT, FLOAT64, FLOAT64)},
	{"dge_inv",						F_(INT, FLOAT64, FLOAT64)},
	{"nfeq",						O_(INT, NFLOAT, NFLOAT, EQ)},
	{"nfne",						O_(INT, NFLOAT, NFLOAT, NE)},
	{"nflt",						O_(INT, NFLOAT, NFLOAT, LT)},
	{"nfle",						O_(INT, NFLOAT, NFLOAT, LE)},
	{"nfgt",						O_(INT, NFLOAT, NFLOAT, GT)},
	{"nfge",						O_(INT, NFLOAT, NFLOAT, GE)},
	{"nfeq_inv",					F_(INT, NFLOAT, NFLOAT)},
	{"nfne_inv",					F_(INT, NFLOAT, NFLOAT)},
	{"nflt_inv",					F_(INT, NFLOAT, NFLOAT)},
	{"nfle_inv",					F_(INT, NFLOAT, NFLOAT)},
	{"nfgt_inv",					F_(INT, NFLOAT, NFLOAT)},
	{"nfge_inv",					F_(INT, NFLOAT, NFLOAT)},
	{"is_fnan",						F_(INT, FLOAT32, EMPTY)},
	{"is_finf",						F_(INT, FLOAT32, EMPTY)},
	{"is_ffinite",					F_(INT, FLOAT32, EMPTY)},
	{"is_dnan",						F_(INT, FLOAT64, EMPTY)},
	{"is_dinf",						F_(INT, FLOAT64, EMPTY)},
	{"is_dfinite",					F_(INT, FLOAT64, EMPTY)},
	{"is_nfnan",					F_(INT, NFLOAT, EMPTY)},
	{"is_nfinf",					F_(INT, NFLOAT, EMPTY)},
	{"is_nffinite",					F_(INT, NFLOAT, EMPTY)},

	/*
	 * Mathematical functions.
	 */
	{"facos",						F_(FLOAT32, FLOAT32, EMPTY)},
	{"fasin",						F_(FLOAT32, FLOAT32, EMPTY)},
	{"fatan",						F_(FLOAT32, FLOAT32, EMPTY)},
	{"fatan2",						F_(FLOAT32, FLOAT32, FLOAT32)},
	{"fceil",						F_(FLOAT32, FLOAT32, EMPTY)},
	{"fcos",						F_(FLOAT32, FLOAT32, EMPTY)},
	{"fcosh",						F_(FLOAT32, FLOAT32, EMPTY)},
	{"fexp",						F_(FLOAT32, FLOAT32, EMPTY)},
	{"ffloor",						F_(FLOAT32, FLOAT32, EMPTY)},
	{"flog",						F_(FLOAT32, FLOAT32, EMPTY)},
	{"flog10",						F_(FLOAT32, FLOAT32, EMPTY)},
	{"fpow",						F_(FLOAT32, FLOAT32, FLOAT32)},
	{"frint",						F_(FLOAT32, FLOAT32, EMPTY)},
	{"fround",						F_(FLOAT32, FLOAT32, EMPTY)},
	{"fsin",						F_(FLOAT32, FLOAT32, EMPTY)},
	{"fsinh",						F_(FLOAT32, FLOAT32, EMPTY)},
	{"fsqrt",						F_(FLOAT32, FLOAT32, EMPTY)},
	{"ftan",						F_(FLOAT32, FLOAT32, EMPTY)},
	{"ftanh",						F_(FLOAT32, FLOAT32, EMPTY)},
	{"dacos",						F_(FLOAT64, FLOAT64, EMPTY)},
	{"dasin",						F_(FLOAT64, FLOAT64, EMPTY)},
	{"datan",						F_(FLOAT64, FLOAT64, EMPTY)},
	{"datan2",						F_(FLOAT64, FLOAT64, FLOAT64)},
	{"dceil",						F_(FLOAT64, FLOAT64, EMPTY)},
	{"dcos",						F_(FLOAT64, FLOAT64, EMPTY)},
	{"dcosh",						F_(FLOAT64, FLOAT64, EMPTY)},
	{"dexp",						F_(FLOAT64, FLOAT64, EMPTY)},
	{"dfloor",						F_(FLOAT64, FLOAT64, EMPTY)},
	{"dlog",						F_(FLOAT64, FLOAT64, EMPTY)},
	{"dlog10",						F_(FLOAT64, FLOAT64, EMPTY)},
	{"dpow",						F_(FLOAT64, FLOAT64, FLOAT64)},
	{"drint",						F_(FLOAT64, FLOAT64, EMPTY)},
	{"dround",						F_(FLOAT64, FLOAT64, EMPTY)},
	{"dsin",						F_(FLOAT64, FLOAT64, EMPTY)},
	{"dsinh",						F_(FLOAT64, FLOAT64, EMPTY)},
	{"dsqrt",						F_(FLOAT64, FLOAT64, EMPTY)},
	{"dtan",						F_(FLOAT64, FLOAT64, EMPTY)},
	{"dtanh",						F_(FLOAT64, FLOAT64, EMPTY)},
	{"nfacos",						F_(NFLOAT, NFLOAT, EMPTY)},
	{"nfasin",						F_(NFLOAT, NFLOAT, EMPTY)},
	{"nfatan",						F_(NFLOAT, NFLOAT, EMPTY)},
	{"nfatan2",						F_(NFLOAT, NFLOAT, NFLOAT)},
	{"nfceil",						F_(NFLOAT, NFLOAT, EMPTY)},
	{"nfcos",						F_(NFLOAT, NFLOAT, EMPTY)},
	{"nfcosh",						F_(NFLOAT, NFLOAT, EMPTY)},
	{"nfexp",						F_(NFLOAT, NFLOAT, EMPTY)},
	{"nffloor",						F_(NFLOAT, NFLOAT, EMPTY)},
	{"nflog",						F_(NFLOAT, NFLOAT, EMPTY)},
	{"nflog10",						F_(NFLOAT, NFLOAT, EMPTY)},
	{"nfpow",						F_(NFLOAT, NFLOAT, NFLOAT)},
	{"nfrint",						F_(NFLOAT, NFLOAT, EMPTY)},
	{"nfround",						F_(NFLOAT, NFLOAT, EMPTY)},
	{"nfsin",						F_(NFLOAT, NFLOAT, EMPTY)},
	{"nfsinh",						F_(NFLOAT, NFLOAT, EMPTY)},
	{"nfsqrt",						F_(NFLOAT, NFLOAT, EMPTY)},
	{"nftan",						F_(NFLOAT, NFLOAT, EMPTY)},
	{"nftanh",						F_(NFLOAT, NFLOAT, EMPTY)},

	/*
	 * Absolute, minimum, maximum, and sign.
	 */
	{"iabs",						F_(INT, INT, EMPTY)},
	{"labs",						F_(LONG, LONG, EMPTY)},
	{"fabs",						F_(FLOAT32, FLOAT32, EMPTY)},
	{"dabs",						F_(FLOAT64, FLOAT64, EMPTY)},
	{"nfabs",						F_(NFLOAT, NFLOAT, EMPTY)},
	{"imin",						F_(INT, INT, INT)},
	{"imin_un",						F_(INT, INT, INT)},
	{"lmin",						F_(LONG, LONG, LONG)},
	{"lmin_un",						F_(LONG, LONG, LONG)},
	{"fmin",						F_(FLOAT32, FLOAT32, FLOAT32)},
	{"dmin",						F_(FLOAT64, FLOAT64, FLOAT64)},
	{"nfmin",						F_(NFLOAT, NFLOAT, NFLOAT)},
	{"imax",						F_(INT, INT, INT)},
	{"imax_un",						F_(INT, INT, INT)},
	{"lmax",						F_(LONG, LONG, LONG)},
	{"lmax_un",						F_(LONG, LONG, LONG)},
	{"fmax",						F_(FLOAT32, FLOAT32, FLOAT32)},
	{"dmax",						F_(FLOAT64, FLOAT64, FLOAT64)},
	{"nfmax",						F_(NFLOAT, NFLOAT, NFLOAT)},
	{"isign",						F_(INT, INT, EMPTY)},
	{"lsign",						F_(INT, LONG, EMPTY)},
	{"fsign",						F_(INT, FLOAT32, EMPTY)},
	{"dsign",						F_(INT, FLOAT64, EMPTY)},
	{"nfsign",						F_(INT, NFLOAT, EMPTY)},

	/*
	 * Pointer check opcodes.
	 */
	{"check_null",					F_(EMPTY, PTR, EMPTY)},

	/*
	 * Function calls.
	 */
	{"call",						JIT_OPCODE_IS_CALL},
	{"call_tail",					JIT_OPCODE_IS_CALL},
	{"call_indirect",				F_(EMPTY, PTR, EMPTY) | INDIRECT_ARGS},
	{"call_indirect_tail",			F_(EMPTY, PTR, EMPTY) | INDIRECT_ARGS},
	{"call_vtable_ptr",				F_(EMPTY, PTR, EMPTY)},
	{"call_vtable_ptr_tail",		F_(EMPTY, PTR, EMPTY)},
	{"call_external",				JIT_OPCODE_IS_CALL_EXTERNAL},
	{"call_external_tail",			JIT_OPCODE_IS_CALL_EXTERNAL},
	{"return",						F_(EMPTY, EMPTY, EMPTY)},
	{"return_int",					F_(EMPTY, INT, EMPTY)},
	{"return_long",					F_(EMPTY, LONG, EMPTY)},
	{"return_float32",				F_(EMPTY, FLOAT32, EMPTY)},
	{"return_float64",				F_(EMPTY, FLOAT64, EMPTY)},
	{"return_nfloat",				F_(EMPTY, NFLOAT, EMPTY)},
	{"return_small_struct",			F_(EMPTY, PTR, PTR) | NINT_ARG},
	{"setup_for_nested",			F_(EMPTY, INT, EMPTY)},
	{"setup_for_sibling",			F_(EMPTY, INT, INT) | NINT_ARG},
	{"import",						F_(PTR, ANY, INT)},

	/*
	 * Exception handling.
	 */
	{"throw",						F_(EMPTY, PTR, EMPTY)},
	{"rethrow",						F_(EMPTY, PTR, EMPTY)},
	{"load_pc",						F_(PTR, EMPTY, EMPTY)},
	{"load_exception_pc",			F_(PTR, EMPTY, EMPTY)},
	{"enter_finally",				F_(EMPTY, EMPTY, EMPTY)},
	{"leave_finally",				F_(EMPTY, EMPTY, EMPTY)},
	{"call_finally",				B_(EMPTY, EMPTY)},
	{"enter_filter",				F_(ANY, EMPTY, EMPTY)},
	{"leave_filter",				F_(EMPTY, ANY, EMPTY)},
	{"call_filter",					B_(ANY, EMPTY)},
	{"call_filter_return",			F_(ANY, EMPTY, EMPTY)},
	{"address_of_label",			JIT_OPCODE_IS_ADDROF_LABEL},

	/*
	 * Data manipulation.
	 */
	{"copy_load_sbyte",				F_(INT, INT, EMPTY)},
	{"copy_load_ubyte",				F_(INT, INT, EMPTY)},
	{"copy_load_short",				F_(INT, INT, EMPTY)},
	{"copy_load_ushort",			F_(INT, INT, EMPTY)},
	{"copy_int",					O_(INT, INT, EMPTY, COPY)},
	{"copy_long",					O_(LONG, LONG, EMPTY, COPY)},
	{"copy_float32",				O_(FLOAT32, FLOAT32, EMPTY, COPY)},
	{"copy_float64",				O_(FLOAT64, FLOAT64, EMPTY, COPY)},
	{"copy_nfloat",					O_(NFLOAT, NFLOAT, EMPTY, COPY)},
	{"copy_struct",					O_(PTR, PTR, EMPTY, COPY) | NINT_ARG},
	{"copy_store_byte",				F_(INT, INT, EMPTY)},
	{"copy_store_short",			F_(INT, INT, EMPTY)},
	{"address_of",					O_(PTR, ANY, EMPTY, ADDRESS_OF)},

	/*
	 * Incoming registers, outgoing registers, and stack pushes.
	 */
	{"incoming_reg",				JIT_OPCODE_IS_REG},
	{"incoming_frame_posn",			F_(EMPTY, ANY, INT)},
	{"outgoing_reg",				JIT_OPCODE_IS_REG},
	{"outgoing_frame_posn",			F_(EMPTY, ANY, INT)},
	{"return_reg",					JIT_OPCODE_IS_REG},
	{"push_int",					F_(EMPTY, INT, EMPTY)},
	{"push_long",					F_(EMPTY, LONG, EMPTY)},
	{"push_float32",				F_(EMPTY, FLOAT32, EMPTY)},
	{"push_float64",				F_(EMPTY, FLOAT64, EMPTY)},
	{"push_nfloat",					F_(EMPTY, NFLOAT, EMPTY)},
	{"push_struct",					F_(EMPTY, ANY, PTR) | NINT_ARG},
	{"pop_stack",					F_(EMPTY, INT, EMPTY) | NINT_ARG},
	{"flush_small_struct",			F_(EMPTY, ANY, EMPTY) | NINT_ARG},
	{"set_param_int",				F_(EMPTY, INT, PTR)},
	{"set_param_long",				F_(EMPTY, LONG, PTR)},
	{"set_param_float32",			F_(EMPTY, FLOAT32, PTR)},
	{"set_param_float64",			F_(EMPTY, FLOAT64, PTR)},
	{"set_param_nfloat",			F_(EMPTY, NFLOAT, PTR)},
	{"set_param_struct",			F_(PTR, PTR, PTR)},
	{"push_return_area_ptr",		F_(EMPTY, EMPTY, EMPTY)},

	/*
	 * Pointer-relative loads and stores.
	 */
	{"load_relative_sbyte",			F_(INT, PTR, INT) | NINT_ARG},
	{"load_relative_ubyte",			F_(INT, PTR, INT) | NINT_ARG},
	{"load_relative_short",			F_(INT, PTR, INT) | NINT_ARG},
	{"load_relative_ushort",		F_(INT, PTR, INT) | NINT_ARG},
	{"load_relative_int",			F_(INT, PTR, INT) | NINT_ARG},
	{"load_relative_long",			F_(LONG, PTR, INT) | NINT_ARG},
	{"load_relative_float32",		F_(FLOAT32, PTR, INT) | NINT_ARG},
	{"load_relative_float64",		F_(FLOAT64, PTR, INT) | NINT_ARG},
	{"load_relative_nfloat",		F_(NFLOAT, PTR, INT) | NINT_ARG},
	{"load_relative_struct",		F_(ANY, PTR, INT) | NINT_ARG_TWO},
	{"store_relative_byte",			F_(PTR, INT, INT) | NINT_ARG},
	{"store_relative_short",		F_(PTR, INT, INT) | NINT_ARG},
	{"store_relative_int",			F_(PTR, INT, INT) | NINT_ARG},
	{"store_relative_long",			F_(PTR, LONG, INT) | NINT_ARG},
	{"store_relative_float32",		F_(PTR, FLOAT32, INT) | NINT_ARG},
	{"store_relative_float64",		F_(PTR, FLOAT64, INT) | NINT_ARG},
	{"store_relative_nfloat",		F_(PTR, NFLOAT, INT) | NINT_ARG},
	{"store_relative_struct",		F_(PTR, ANY, INT) | NINT_ARG_TWO},
	{"add_relative",				F_(PTR, PTR, INT) | NINT_ARG},

	/*
	 * Array element loads and stores.
	 */
	{"load_element_sbyte",			F_(INT, PTR, INT)},
	{"load_element_ubyte",			F_(INT, PTR, INT)},
	{"load_element_short",			F_(INT, PTR, INT)},
	{"load_element_ushort",			F_(INT, PTR, INT)},
	{"load_element_int",			F_(INT, PTR, INT)},
	{"load_element_long",			F_(LONG, PTR, INT)},
	{"load_element_float32",		F_(FLOAT32, PTR, INT)},
	{"load_element_float64",		F_(FLOAT64, PTR, INT)},
	{"load_element_nfloat",			F_(NFLOAT, PTR, INT)},
	{"store_element_byte",			F_(PTR, INT, INT)},
	{"store_element_short",			F_(PTR, INT, INT)},
	{"store_element_int",			F_(PTR, INT, INT)},
	{"store_element_long",			F_(PTR, INT, LONG)},
	{"store_element_float32",		F_(PTR, INT, FLOAT32)},
	{"store_element_float64",		F_(PTR, INT, FLOAT64)},
	{"store_element_nfloat",		F_(PTR, INT, NFLOAT)},

	/*
	 * Block operations.
	 */
	{"memcpy",						F_(PTR, PTR, PTR)},
	{"memmove",						F_(PTR, PTR, PTR)},
	{"memset",						F_(PTR, INT, PTR)},

	/*
	 * Allocate memory from the stack.
	 */
	{"alloca",						F_(PTR, PTR, EMPTY)},

	/*
	 * Debugging support.
	 */
	{"mark_offset",					F_(EMPTY, INT, EMPTY)},
	{"mark_breakpoint",				F_(EMPTY, PTR, PTR)},

	/*
	 * Switch statement support.
	 */
	{"jump_table",				F_(ANY, PTR, INT)|JIT_OPCODE_IS_JUMP_TABLE},
};

#if defined(JIT_BACKEND_INTERP)

jit_opcode_info_t const _jit_interp_opcodes[JIT_OP_NUM_INTERP_OPCODES] = {

	/*
	 * Argument variable access opcodes.
	 */
	{"lda_0_sbyte",			JIT_OPCODE_NINT_ARG},
	{"lda_0_ubyte",			JIT_OPCODE_NINT_ARG},
	{"lda_0_short",			JIT_OPCODE_NINT_ARG},
	{"lda_0_ushort",		JIT_OPCODE_NINT_ARG},
	{"lda_0_int",			JIT_OPCODE_NINT_ARG},
	{"lda_0_long",			JIT_OPCODE_NINT_ARG},
	{"lda_0_float32",		JIT_OPCODE_NINT_ARG},
	{"lda_0_float64",		JIT_OPCODE_NINT_ARG},
	{"lda_0_nfloat",		JIT_OPCODE_NINT_ARG},
	{"ldaa_0",			JIT_OPCODE_NINT_ARG},
	{"lda_1_sbyte",			JIT_OPCODE_NINT_ARG},
	{"lda_1_ubyte",			JIT_OPCODE_NINT_ARG},
	{"lda_1_short",			JIT_OPCODE_NINT_ARG},
	{"lda_1_ushort",		JIT_OPCODE_NINT_ARG},
	{"lda_1_int",			JIT_OPCODE_NINT_ARG},
	{"lda_1_long",			JIT_OPCODE_NINT_ARG},
	{"lda_1_float32",		JIT_OPCODE_NINT_ARG},
	{"lda_1_float64",		JIT_OPCODE_NINT_ARG},
	{"lda_1_nfloat",		JIT_OPCODE_NINT_ARG},
	{"ldaa_1",			JIT_OPCODE_NINT_ARG},
	{"lda_2_sbyte",			JIT_OPCODE_NINT_ARG},
	{"lda_2_ubyte",			JIT_OPCODE_NINT_ARG},
	{"lda_2_short",			JIT_OPCODE_NINT_ARG},
	{"lda_2_ushort",		JIT_OPCODE_NINT_ARG},
	{"lda_2_int",			JIT_OPCODE_NINT_ARG},
	{"lda_2_long",			JIT_OPCODE_NINT_ARG},
	{"lda_2_float32",		JIT_OPCODE_NINT_ARG},
	{"lda_2_float64",		JIT_OPCODE_NINT_ARG},
	{"lda_2_nfloat",		JIT_OPCODE_NINT_ARG},
	{"ldaa_2",			JIT_OPCODE_NINT_ARG},
	{"sta_0_byte",			JIT_OPCODE_NINT_ARG},
	{"sta_0_short",			JIT_OPCODE_NINT_ARG},
	{"sta_0_int",			JIT_OPCODE_NINT_ARG},
	{"sta_0_long",			JIT_OPCODE_NINT_ARG},
	{"sta_0_float32",		JIT_OPCODE_NINT_ARG},
	{"sta_0_float64",		JIT_OPCODE_NINT_ARG},
	{"sta_0_nfloat",		JIT_OPCODE_NINT_ARG},

	/*
	 * Local variable frame access opcodes.
	 */
	{"ldl_0_sbyte",			JIT_OPCODE_NINT_ARG},
	{"ldl_0_ubyte",			JIT_OPCODE_NINT_ARG},
	{"ldl_0_short",			JIT_OPCODE_NINT_ARG},
	{"ldl_0_ushort",		JIT_OPCODE_NINT_ARG},
	{"ldl_0_int",			JIT_OPCODE_NINT_ARG},
	{"ldl_0_long",			JIT_OPCODE_NINT_ARG},
	{"ldl_0_float32",		JIT_OPCODE_NINT_ARG},
	{"ldl_0_float64",		JIT_OPCODE_NINT_ARG},
	{"ldl_0_nfloat",		JIT_OPCODE_NINT_ARG},
	{"ldla_0",			JIT_OPCODE_NINT_ARG},
	{"ldl_1_sbyte",			JIT_OPCODE_NINT_ARG},
	{"ldl_1_ubyte",			JIT_OPCODE_NINT_ARG},
	{"ldl_1_short",			JIT_OPCODE_NINT_ARG},
	{"ldl_1_ushort",		JIT_OPCODE_NINT_ARG},
	{"ldl_1_int",			JIT_OPCODE_NINT_ARG},
	{"ldl_1_long",			JIT_OPCODE_NINT_ARG},
	{"ldl_1_float32",		JIT_OPCODE_NINT_ARG},
	{"ldl_1_float64",		JIT_OPCODE_NINT_ARG},
	{"ldl_1_nfloat",		JIT_OPCODE_NINT_ARG},
	{"ldla_1",			JIT_OPCODE_NINT_ARG},
	{"ldl_2_sbyte",			JIT_OPCODE_NINT_ARG},
	{"ldl_2_ubyte",			JIT_OPCODE_NINT_ARG},
	{"ldl_2_short",			JIT_OPCODE_NINT_ARG},
	{"ldl_2_ushort",		JIT_OPCODE_NINT_ARG},
	{"ldl_2_int",			JIT_OPCODE_NINT_ARG},
	{"ldl_2_long",			JIT_OPCODE_NINT_ARG},
	{"ldl_2_float32",		JIT_OPCODE_NINT_ARG},
	{"ldl_2_float64",		JIT_OPCODE_NINT_ARG},
	{"ldl_2_nfloat",		JIT_OPCODE_NINT_ARG},
	{"ldla_2",			JIT_OPCODE_NINT_ARG},
	{"stl_0_byte",			JIT_OPCODE_NINT_ARG},
	{"stl_0_short",			JIT_OPCODE_NINT_ARG},
	{"stl_0_int",			JIT_OPCODE_NINT_ARG},
	{"stl_0_long",			JIT_OPCODE_NINT_ARG},
	{"stl_0_float32",		JIT_OPCODE_NINT_ARG},
	{"stl_0_float64",		JIT_OPCODE_NINT_ARG},
	{"stl_0_nfloat",		JIT_OPCODE_NINT_ARG},

	/*
	 * Load constant values.
	 */
	{"ldc_0_int",			JIT_OPCODE_NINT_ARG},
	{"ldc_1_int",			JIT_OPCODE_NINT_ARG},
	{"ldc_2_int",			JIT_OPCODE_NINT_ARG},
	{"ldc_0_long",			JIT_OPCODE_CONST_LONG},
	{"ldc_1_long",			JIT_OPCODE_CONST_LONG},
	{"ldc_2_long",			JIT_OPCODE_CONST_LONG},
	{"ldc_0_float32",		JIT_OPCODE_CONST_FLOAT32},
	{"ldc_1_float32",		JIT_OPCODE_CONST_FLOAT32},
	{"ldc_2_float32",		JIT_OPCODE_CONST_FLOAT32},
	{"ldc_0_float64",		JIT_OPCODE_CONST_FLOAT64},
	{"ldc_1_float64",		JIT_OPCODE_CONST_FLOAT64},
	{"ldc_2_float64",		JIT_OPCODE_CONST_FLOAT64},
	{"ldc_0_nfloat",		JIT_OPCODE_CONST_NFLOAT},
	{"ldc_1_nfloat",		JIT_OPCODE_CONST_NFLOAT},
	{"ldc_2_nfloat",		JIT_OPCODE_CONST_NFLOAT},

	/*
	 * Load return value.
	 */
	{"ldr_0_int",			0},
	{"ldr_0_long",			0},
	{"ldr_0_float32",		0},
	{"ldr_0_float64",		0},
	{"ldr_0_nfloat",		0},

	/*
	 * Stack management.
	 */
	{"pop",				0},
	{"pop_2",			0},
	{"pop_3",			0},

	/*
	 * Nested function call handling.
	 */
	{"import_local",		JIT_OPCODE_NINT_ARG_TWO},
	{"import_arg",			JIT_OPCODE_NINT_ARG_TWO},

	/*
	 * Marker opcode for the end of a function.
	 */
	{"end_marker",			0},
};

#endif /* JIT_BACKEND_INTERP */
