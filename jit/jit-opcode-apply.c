/*
 * jit-opcode-apply.c - Constant folding.
 *
 * Copyright (C) 2008  Southern Storm Software, Pty Ltd.
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

/*
 * Signatures for the different intrinsics
 */
typedef jit_int (*jit_cf_i_i_func)(jit_int value);
typedef jit_int (*jit_cf_i_ii_func)(jit_int value1, jit_int value2);
typedef jit_int (*jit_cf_i_piii_func)(jit_int *result, jit_int value1, jit_int value2);
typedef jit_int (*jit_cf_i_iI_func)(jit_int value1, jit_uint value2);
typedef jit_int (*jit_cf_i_II_func)(jit_uint value1, jit_uint value2);
typedef jit_uint (*jit_cf_I_I_func)(jit_uint value);
typedef jit_uint (*jit_cf_I_II_func)(jit_uint value1, jit_uint value2);
typedef jit_int (*jit_cf_i_pIII_func)(jit_uint *result, jit_uint value1, jit_uint value2);
typedef jit_long (*jit_cf_l_l_func)(jit_long value);
typedef jit_long (*jit_cf_l_ll_func)(jit_long value1, jit_long value2);
typedef jit_int (*jit_cf_i_plll_func)(jit_long *result, jit_long value1, jit_long value2);
typedef jit_int (*jit_cf_i_l_func)(jit_long value);
typedef jit_int (*jit_cf_i_ll_func)(jit_long value1, jit_long value2);
typedef jit_long (*jit_cf_l_lI_func)(jit_long value1, jit_uint value2);
typedef jit_ulong (*jit_cf_L_L_func)(jit_ulong value);
typedef jit_ulong (*jit_cf_L_LL_func)(jit_ulong value1, jit_ulong value2);
typedef jit_int (*jit_cf_i_pLLL_func)(jit_ulong *result, jit_ulong value1, jit_ulong value2);
typedef jit_int (*jit_cf_i_LL_func)(jit_ulong value1, jit_ulong value2);
typedef jit_ulong (*jit_cf_L_LI_func)(jit_ulong value1, jit_uint value2);
typedef jit_float32 (*jit_cf_f_f_func)(jit_float32 value);
typedef jit_float32 (*jit_cf_f_ff_func)(jit_float32 value1, jit_float32 value2);
typedef jit_int (*jit_cf_i_f_func)(jit_float32 value);
typedef jit_int (*jit_cf_i_ff_func)(jit_float32 value1, jit_float32 value2);
typedef jit_float64 (*jit_cf_d_d_func)(jit_float64 value);
typedef jit_float64 (*jit_cf_d_dd_func)(jit_float64 value1, jit_float64 value2);
typedef jit_int (*jit_cf_i_d_func)(jit_float64 value);
typedef jit_int (*jit_cf_i_dd_func)(jit_float64 value1, jit_float64 value2);
typedef jit_nfloat (*jit_cf_D_D_func)(jit_nfloat value);
typedef jit_nfloat (*jit_cf_D_DD_func)(jit_nfloat value1, jit_nfloat value2);
typedef jit_int (*jit_cf_i_D_func)(jit_nfloat value);
typedef jit_int (*jit_cf_i_DD_func)(jit_nfloat value1, jit_nfloat value2);

/*
 * NOTE: The result type is already set in const_result.
 */
static int
apply_conv(jit_constant_t *const_result, jit_value_t value, int overflow_check)
{
	jit_type_t srctype;
	jit_constant_t const_value;

	srctype = jit_type_promote_int(jit_type_normalize(value->type));
	if(!srctype)
	{
		return 0;
	}
	const_value.type = srctype;
	switch(srctype->kind)
	{
		case JIT_TYPE_INT:
		{
			const_value.un.int_value = value->address;
		}
		break;

		case JIT_TYPE_UINT:
		{
			const_value.un.uint_value = value->address;
		}
		break;

		case JIT_TYPE_LONG:
		{
#ifdef JIT_NATIVE_INT64
			const_value.un.long_value = value->address;
#else
			const_value.un.long_value = *(jit_long *)(value->address);
#endif
		}
		break;

		case JIT_TYPE_ULONG:
		{
#ifdef JIT_NATIVE_INT64
			const_value.un.ulong_value = value->address;
#else
			const_value.un.ulong_value = *(jit_ulong *)(value->address);
#endif
		}
		break;

		case JIT_TYPE_FLOAT32:
		{
			const_value.un.float32_value = *(jit_float32 *)(value->address);
		}
		break;

		case JIT_TYPE_FLOAT64:
		{
			const_value.un.float64_value = *(jit_float64 *)(value->address);
		}
		break;

		case JIT_TYPE_NFLOAT:
		{
			const_value.un.nfloat_value = *(jit_nfloat *)(value->address);
		}
		break;

		default:
		{
			return 0;
		}
	}
	return jit_constant_convert(const_result, &const_value,
				    const_result->type, overflow_check);
}

/*
 * NOTE: value1 is guaranteed to be valid and a constant n entry of each
 * of the apply_* functions.
 * This is checked on entry of the public _jit_opcode_apply function.
 */
static int
apply_i_i(jit_constant_t *const_result, jit_value_t value1,
	  jit_cf_i_i_func intrinsic)
{
	if(value1->is_nint_constant)
	{
		const_result->un.int_value = (*intrinsic)(value1->address);
		return 1;
	}
	return 0;
}

static int
apply_i_ii(jit_constant_t *const_result,
	   jit_value_t value1, jit_value_t value2,
	   jit_cf_i_ii_func intrinsic)
{
	if(value1->is_nint_constant && value2 && value2->is_nint_constant)
	{
		const_result->un.int_value = (*intrinsic)(value1->address,
							  value2->address);
		return 1;
	}
	return 0;
}

static int
apply_i_piii(jit_constant_t *const_result,
	     jit_value_t value1, jit_value_t value2,
	     jit_cf_i_piii_func intrinsic)
{
	if(value1->is_nint_constant && value2 && value2->is_nint_constant)
	{
		return (*intrinsic)(&(const_result->un.int_value),
				    value1->address, value2->address);
	}
	return 0;
}

static int
apply_i_iI(jit_constant_t *const_result,
	   jit_value_t value1, jit_value_t value2,
	   jit_cf_i_iI_func intrinsic)
{
	if(value1->is_nint_constant && value2 && value2->is_nint_constant)
	{
		const_result->un.int_value = (*intrinsic)(value1->address,
							  value2->address);
		return 1;
	}
	return 0;
}

static int
apply_i_II(jit_constant_t *const_result,
	   jit_value_t value1, jit_value_t value2,
	   jit_cf_i_II_func intrinsic)
{
	if(value1->is_nint_constant && value2 && value2->is_nint_constant)
	{
		const_result->un.int_value = (*intrinsic)(value1->address, value2->address);
		return 1;
	}
	return 0;
}

static int
apply_I_I(jit_constant_t *const_result, jit_value_t value1,
	  jit_cf_I_I_func intrinsic)
{
	if(value1->is_nint_constant)
	{
		const_result->un.uint_value = (*intrinsic)(value1->address);
		return 1;
	}
	return 0;
}

static int
apply_I_II(jit_constant_t *const_result,
	   jit_value_t value1, jit_value_t value2,
	   jit_cf_I_II_func intrinsic)
{
	if(value1->is_nint_constant && value2 && value2->is_nint_constant)
	{
		const_result->un.uint_value = (*intrinsic)(value1->address, value2->address);
		return 1;
	}
	return 0;
}

static int
apply_i_pIII(jit_constant_t *const_result,
	     jit_value_t value1, jit_value_t value2,
	     jit_cf_i_pIII_func intrinsic)
{
	if(value1->is_nint_constant && value2 && value2->is_nint_constant)
	{
		return (*intrinsic)(&(const_result->un.uint_value),
				    value1->address, value2->address);
	}
	return 0;
}

static int
apply_l_l(jit_constant_t *const_result, jit_value_t value1,
	  jit_cf_l_l_func intrinsic)
{
#ifdef JIT_NATIVE_INT64
	const_result->un.long_value = (*intrinsic)(value1->address);
#else
	const_result->un.long_value = (*intrinsic)(*(jit_long *)(value1->address));
#endif
	return 1;
}

static int
apply_l_ll(jit_constant_t *const_result,
	   jit_value_t value1, jit_value_t value2,
	   jit_cf_l_ll_func intrinsic)
{
	if(value2 && value2->is_constant)
	{
#ifdef JIT_NATIVE_INT64
		const_result->un.long_value = (*intrinsic)(value1->address,
							   value2->address);
#else
		const_result->un.long_value = (*intrinsic)(*(jit_long *)(value1->address),
							   *(jit_long *)(value2->address));
#endif
		return 1;
	}
	return 0;
}

static int
apply_i_plll(jit_constant_t *const_result,
	     jit_value_t value1, jit_value_t value2,
	     jit_cf_i_plll_func intrinsic)
{
	if(value2 && value2->is_constant)
	{
#ifdef JIT_NATIVE_INT64
		return (*intrinsic)(&(const_result->un.long_value),
				    value1->address, value2->address);
#else
		return (*intrinsic)(&(const_result->un.long_value),
				    *(jit_long *)(value1->address),
				    *(jit_long *)(value2->address));
#endif
	}
	return 0;
}

static int
apply_i_l(jit_constant_t *const_result,
	  jit_value_t value, jit_cf_i_l_func intrinsic)
{
#ifdef JIT_NATIVE_INT64
	const_result->un.int_value = (*intrinsic)(value->address);
#else
	const_result->un.int_value = (*intrinsic)(*(jit_long *)(value->address));
#endif
	return 1;
}

static int
apply_i_ll(jit_constant_t *const_result,
	   jit_value_t value1, jit_value_t value2,
	   jit_cf_i_ll_func intrinsic)
{
	if(value2 && value2->is_constant)
	{
#ifdef JIT_NATIVE_INT64
		const_result->un.int_value = (*intrinsic)(value1->address,
							  value2->address);
#else
		const_result->un.int_value = (*intrinsic)(*(jit_long *)(value1->address),
							  *(jit_long *)(value2->address));
#endif
		return 1;
	}
	return 0;
}

static int
apply_l_lI(jit_constant_t *const_result,
	   jit_value_t value1, jit_value_t value2,
	   jit_cf_l_lI_func intrinsic)
{
	if(value2 && value2->is_nint_constant)
	{
#ifdef JIT_NATIVE_INT64
		const_result->un.long_value = (*intrinsic)(value1->address,
							   value2->address);
#else
		const_result->un.long_value = (*intrinsic)(*(jit_long *)(value1->address),
							   value2->address);
#endif
		return 1;
	}
	return 0;
}

static int
apply_L_L(jit_constant_t *const_result, jit_value_t value,
	  jit_cf_L_L_func intrinsic)
{
#ifdef JIT_NATIVE_INT64
	const_result->un.ulong_value = (*intrinsic)(value->address);
#else
	const_result->un.ulong_value = (*intrinsic)(*(jit_ulong *)(value->address));
#endif
	return 1;
}

static int
apply_L_LL(jit_constant_t *const_result,
	   jit_value_t value1, jit_value_t value2,
	   jit_cf_L_LL_func intrinsic)
{
	if(value2 && value2->is_constant)
	{
#ifdef JIT_NATIVE_INT64
		const_result->un.ulong_value = (*intrinsic)(value1->address,
							    value2->address);
#else
		const_result->un.ulong_value = (*intrinsic)(*(jit_ulong *)(value1->address),
							    *(jit_ulong *)(value2->address));
#endif
		return 1;
	}
	return 0;
}

static int
apply_i_pLLL(jit_constant_t *const_result,
	     jit_value_t value1, jit_value_t value2,
	     jit_cf_i_pLLL_func intrinsic)
{
	if(value2 && value2->is_constant)
	{
#ifdef JIT_NATIVE_INT64
		return (*intrinsic)(&(const_result->un.ulong_value),
				    value1->address, value2->address);
#else
		return (*intrinsic)(&(const_result->un.ulong_value),
				    *(jit_ulong *)(value1->address),
				    *(jit_ulong *)(value2->address));
#endif
	}
	return 0;
}

static int
apply_i_LL(jit_constant_t *const_result,
	   jit_value_t value1, jit_value_t value2,
	   jit_cf_i_LL_func intrinsic)
{
	if(value2 && value2->is_constant)
	{
#ifdef JIT_NATIVE_INT64
		const_result->un.int_value = (*intrinsic)(value1->address,
							  value2->address);
#else
		const_result->un.int_value = (*intrinsic)(*(jit_ulong *)(value1->address),
							  *(jit_ulong *)(value2->address));
#endif
		return 1;
	}
	return 0;
}

static int
apply_L_LI(jit_constant_t *const_result,
	   jit_value_t value1, jit_value_t value2,
	   jit_cf_L_LI_func intrinsic)
{
	if(value2 && value2->is_nint_constant)
	{
#ifdef JIT_NATIVE_INT64
		const_result->un.ulong_value = (*intrinsic)(value1->address,
							    value2->address);
#else
		const_result->un.ulong_value = (*intrinsic)(*(jit_ulong *)(value1->address),
							    value2->address);
#endif
		return 1;
	}
	return 0;
}

static int
apply_f_f(jit_constant_t *const_result, jit_value_t value,
	  jit_cf_f_f_func intrinsic)
{
	const_result->un.float32_value = (*intrinsic)(*(jit_float32 *)(value->address));
	return 1;
}

static int
apply_f_ff(jit_constant_t *const_result,
	   jit_value_t value1, jit_value_t value2,
	   jit_cf_f_ff_func intrinsic)
{
	if(value2 && value2->is_constant)
	{
		const_result->un.float32_value = (*intrinsic)(*(jit_float32 *)(value1->address),
							      *(jit_float32 *)(value2->address));
		return 1;
	}
	return 0;
}

static int
apply_i_f(jit_constant_t *const_result, jit_value_t value,
	  jit_cf_i_f_func intrinsic)
{
	const_result->un.int_value = (*intrinsic)(*(jit_float32 *)(value->address));
	return 1;
}

static int
apply_i_ff(jit_constant_t *const_result,
	   jit_value_t value1, jit_value_t value2,
	   jit_cf_i_ff_func intrinsic)
{
	if(value2 && value2->is_constant)
	{
		const_result->un.int_value = (*intrinsic)(*(jit_float32 *)(value1->address),
							  *(jit_float32 *)(value2->address));
		return 1;
	}
	return 0;
}

static int
apply_d_d(jit_constant_t *const_result, jit_value_t value,
	  jit_cf_d_d_func intrinsic)
{
	const_result->un.float64_value = (*intrinsic)(*(jit_float64 *)(value->address));
	return 1;
}

static int
apply_d_dd(jit_constant_t *const_result,
	   jit_value_t value1, jit_value_t value2,
	   jit_cf_d_dd_func intrinsic)
{
	if(value2 && value2->is_constant)
	{
		const_result->un.float64_value = (*intrinsic)(*(jit_float64 *)(value1->address),
							      *(jit_float64 *)(value2->address));
		return 1;
	}
	return 0;
}

static int
apply_i_d(jit_constant_t *const_result, jit_value_t value,
	  jit_cf_i_d_func intrinsic)
{
	const_result->un.int_value = (*intrinsic)(*(jit_float64 *)(value->address));
	return 1;
}

static int
apply_i_dd(jit_constant_t *const_result,
	   jit_value_t value1, jit_value_t value2,
	   jit_cf_i_dd_func intrinsic)
{
	if(value2 && value2->is_constant)
	{
		const_result->un.int_value = (*intrinsic)(*(jit_float64 *)(value1->address),
							  *(jit_float64 *)(value2->address));
		return 1;
	}
	return 0;
}

static int
apply_D_D(jit_constant_t *const_result, jit_value_t value,
	  jit_cf_D_D_func intrinsic)
{
	const_result->un.nfloat_value = (*intrinsic)(*(jit_nfloat *)(value->address));
	return 1;
}

static int
apply_D_DD(jit_constant_t *const_result,
	   jit_value_t value1, jit_value_t value2,
	   jit_cf_D_DD_func intrinsic)
{
	if(value2 && value2->is_constant)
	{
		const_result->un.nfloat_value = (*intrinsic)(*(jit_nfloat *)(value1->address),
							     *(jit_nfloat *)(value2->address));
		return 1;
	}
	return 0;
}

static int
apply_i_D(jit_constant_t *const_result,
	  jit_value_t value, jit_cf_i_D_func intrinsic)
{
	const_result->un.int_value = (*intrinsic)(*(jit_nfloat *)(value->address));
	return 1;
}

static int
apply_i_DD(jit_constant_t *const_result,
	   jit_value_t value1, jit_value_t value2,
	   jit_cf_i_DD_func intrinsic)
{
	if(value2 && value2->is_constant)
	{
		const_result->un.int_value = (*intrinsic)(*(jit_nfloat *)(value1->address),
							  *(jit_nfloat *)(value2->address));
		return 1;
	}
	return 0;
}

static jit_value_t
apply_opcode(jit_function_t func, const _jit_intrinsic_info_t *opcode_info,
	     jit_type_t dest_type, jit_value_t value1, jit_value_t value2)
{
	jit_constant_t const_result;
	int result = 0;

	const_result.type = dest_type;
	switch(opcode_info->signature)
	{
		case JIT_SIG_i_i:
		{
			result = apply_i_i
				 (&const_result, value1,
				 (jit_cf_i_i_func)opcode_info->intrinsic);
		}
		break;

		case JIT_SIG_i_ii:
		{
			result = apply_i_ii
				 (&const_result, value1, value2,
				 (jit_cf_i_ii_func)opcode_info->intrinsic);
		}
		break;

		case JIT_SIG_i_piii:
		{
			result = apply_i_piii
				 (&const_result, value1, value2,
				 (jit_cf_i_piii_func)opcode_info->intrinsic);
		}
		break;

		case JIT_SIG_i_iI:
		{
			result = apply_i_iI
				 (&const_result, value1, value2,
				 (jit_cf_i_iI_func)opcode_info->intrinsic);
		}
		break;

		case JIT_SIG_i_II:
		{
			result = apply_i_II
				 (&const_result, value1, value2,
				 (jit_cf_i_II_func)opcode_info->intrinsic);
		}
		break;

		case JIT_SIG_I_I:
		{
			result = apply_I_I
				 (&const_result, value1,
				 (jit_cf_I_I_func)opcode_info->intrinsic);
		}
		break;

		case JIT_SIG_I_II:
		{
			result = apply_I_II
				 (&const_result, value1, value2,
				 (jit_cf_I_II_func)opcode_info->intrinsic);
		}
		break;

		case JIT_SIG_i_pIII:
		{
			result = apply_i_pIII
				 (&const_result, value1, value2,
				 (jit_cf_i_pIII_func)opcode_info->intrinsic);
		}
		break;

		case JIT_SIG_l_l:
		{
			result = apply_l_l
				 (&const_result, value1,
				 (jit_cf_l_l_func)opcode_info->intrinsic);
		}
		break;

		case JIT_SIG_l_ll:
		{
			result = apply_l_ll
				 (&const_result, value1, value2,
				 (jit_cf_l_ll_func)opcode_info->intrinsic);
		}
		break;

		case JIT_SIG_i_plll:
		{
			result = apply_i_plll
				 (&const_result, value1, value2,
				 (jit_cf_i_plll_func)opcode_info->intrinsic);
		}
		break;

		case JIT_SIG_i_l:
		{
			result = apply_i_l
				 (&const_result, value1,
				 (jit_cf_i_l_func)opcode_info->intrinsic);
		}
		break;

		case JIT_SIG_i_ll:
		{
			result = apply_i_ll
				 (&const_result, value1, value2,
				 (jit_cf_i_ll_func)opcode_info->intrinsic);
		}
		break;

		case JIT_SIG_l_lI:
		{
			result = apply_l_lI
				 (&const_result, value1, value2,
				 (jit_cf_l_lI_func)opcode_info->intrinsic);
		}
		break;

		case JIT_SIG_L_L:
		{
			result = apply_L_L
				 (&const_result, value1,
				 (jit_cf_L_L_func)opcode_info->intrinsic);
		}
		break;

		case JIT_SIG_L_LL:
		{
			result = apply_L_LL
				 (&const_result, value1, value2,
				 (jit_cf_L_LL_func)opcode_info->intrinsic);
		}
		break;

		case JIT_SIG_i_pLLL:
		{
			result = apply_i_pLLL
				 (&const_result, value1, value2,
				 (jit_cf_i_pLLL_func)opcode_info->intrinsic);
		}
		break;

		case JIT_SIG_i_LL:
		{
			result = apply_i_LL
				 (&const_result, value1, value2,
				 (jit_cf_i_LL_func)opcode_info->intrinsic);
		}
		break;

		case JIT_SIG_L_LI:
		{
			result = apply_L_LI
				 (&const_result, value1, value2,
				 (jit_cf_L_LI_func)opcode_info->intrinsic);
		}
		break;

		case JIT_SIG_f_f:
		{
			result = apply_f_f
				 (&const_result, value1,
				 (jit_cf_f_f_func)opcode_info->intrinsic);
		}
		break;

		case JIT_SIG_f_ff:
		{
			result = apply_f_ff
				 (&const_result, value1, value2,
				 (jit_cf_f_ff_func)opcode_info->intrinsic);
		}
		break;

		case JIT_SIG_i_f:
		{
			result = apply_i_f
				 (&const_result, value1,
				 (jit_cf_i_f_func)opcode_info->intrinsic);
		}
		break;

		case JIT_SIG_i_ff:
		{
			result = apply_i_ff
				 (&const_result, value1, value2,
				 (jit_cf_i_ff_func)opcode_info->intrinsic);
		}
		break;

		case JIT_SIG_d_d:
		{
			result = apply_d_d
				 (&const_result, value1,
				 (jit_cf_d_d_func)opcode_info->intrinsic);
		}
		break;

		case JIT_SIG_d_dd:
		{
			result = apply_d_dd
				 (&const_result, value1, value2,
				 (jit_cf_d_dd_func)opcode_info->intrinsic);
		}
		break;

		case JIT_SIG_i_d:
		{
			result = apply_i_d
				 (&const_result, value1,
				 (jit_cf_i_d_func)opcode_info->intrinsic);
		}
		break;

		case JIT_SIG_i_dd:
		{
			result = apply_i_dd
				 (&const_result, value1, value2,
				 (jit_cf_i_dd_func)opcode_info->intrinsic);
		}
		break;

		case JIT_SIG_D_D:
		{
			result = apply_D_D
				 (&const_result, value1,
				 (jit_cf_D_D_func)opcode_info->intrinsic);
		}
		break;

		case JIT_SIG_D_DD:
		{
			result = apply_D_DD
				 (&const_result, value1, value2,
				 (jit_cf_D_DD_func)opcode_info->intrinsic);
		}
		break;

		case JIT_SIG_i_D:
		{
			result = apply_i_D
				 (&const_result, value1,
				 (jit_cf_i_D_func)opcode_info->intrinsic);
		}
		break;

		case JIT_SIG_i_DD:
		{
			result = apply_i_DD
				 (&const_result, value1, value2,
				 (jit_cf_i_DD_func)opcode_info->intrinsic);
		}
		break;

		case JIT_SIG_conv:
		{
			result = apply_conv(&const_result, value1, 0);
		}
		break;

		case JIT_SIG_conv_ovf:
		{
			result = apply_conv(&const_result, value1, 1);
		}
		break;
	}
	if(result)
	{
		return jit_value_create_constant(func, &const_result);
	}
	return 0;
}

jit_value_t
_jit_opcode_apply(jit_function_t func, jit_uint opcode, jit_type_t dest_type,
		  jit_value_t value1, jit_value_t value2)
{
	const _jit_intrinsic_info_t *opcode_info;

	if(!func || opcode >= JIT_OP_NUM_OPCODES)
	{
		return 0;
	}
	if((value1 == 0) || !(value1->is_constant))
	{
		return 0;
	}
	opcode_info = &(_jit_intrinsics[opcode]);
	if((opcode_info->flags & _JIT_INTRINSIC_FLAG_MASK) ==
	   _JIT_INTRINSIC_FLAG_NOT)
	{
		jit_value_t value;
		
		opcode = opcode_info->flags & ~_JIT_INTRINSIC_FLAG_MASK;
		if(opcode >= JIT_OP_NUM_OPCODES)
		{
			return 0;
		}
		opcode_info = &(_jit_intrinsics[opcode]);
		value = apply_opcode(func, opcode_info, dest_type, value1, value2);
		if(value)
		{
			/*
			 * We have to apply a logical not to the constant
			 * jit_int result value.
			 */
			value->address = !(value->address);
			return value;
		}
	}
	else if((opcode_info->flags & _JIT_INTRINSIC_FLAG_MASK) ==
	      _JIT_INTRINSIC_FLAG_NONE)
	{
		return apply_opcode(func, opcode_info, dest_type, value1, value2);
	}
	return 0;
}

