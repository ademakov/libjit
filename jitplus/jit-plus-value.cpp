/*
 * jit-plus-value.cpp - C++ wrapper for JIT values.
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

#include <jit/jit-plus.h>

/*@

The @code{jit_value} class provides a C++ counterpart to the
@code{jit_value_t} type.  Values normally result by calling methods
on the @code{jit_function} class during the function building process.
@xref{Values}, for more information on creating and managing values.

@defop Constructor jit_value jit_value ()
Construct an empty value.
@end defop

@defop Constructor jit_value jit_value (jit_value_t @var{value})
Construct a value by wrapping up a raw C @code{jit_value_t} object.
@end defop

@defop Constructor jit_value jit_value (const jit_value& @var{value})
Create a copy of @var{value}.
@end defop

@defop Destructor jit_value ~jit_value ()
Destroy the C++ value wrapper, but leave the underlying raw C
value alone.
@end defop

@defop Operator jit_value {jit_value& operator=} (const jit_value& @var{value})
Copy @code{jit_value} objects.
@end defop

@deftypemethod jit_value jit_value_t raw () const
Get the raw C @code{jit_value_t} value that underlies this object.
@end deftypemethod

@deftypemethod jit_value int is_valid () const
Determine if this @code{jit_value} object contains a valid raw
C @code{jit_value_t} value.
@end deftypemethod

@deftypemethod jit_value int is_temporary () const
@deftypemethodx jit_value int is_local () const
@deftypemethodx jit_value int is_constant () const
Determine if this @code{jit_value} is temporary, local, or constant.
@end deftypemethod

@deftypemethod jit_value void set_volatile ()
@deftypemethodx jit_value int is_volatile () const
Set or check the "volatile" state on this value.
@end deftypemethod

@deftypemethod jit_value void set_addressable ()
@deftypemethodx jit_value int is_addressable () const
Set or check the "addressable" state on this value.
@end deftypemethod

@deftypemethod jit_value jit_type_t type () const
Get the type of this value.
@end deftypemethod

@deftypemethod jit_value jit_function_t function () const
@deftypemethodx jit_value jit_block_t block () const
@deftypemethodx jit_value jit_context_t context () const
Get the owning function, block, or context for this value.
@end deftypemethod

@deftypemethod jit_value jit_constant_t constant () const
@deftypemethodx jit_value jit_nint nint_constant () const
@deftypemethodx jit_value jit_long long_constant () const
@deftypemethodx jit_value jit_float32 float32_constant () const
@deftypemethodx jit_value jit_float64 float64_constant () const
@deftypemethodx jit_value jit_nfloat nfloat_constant () const
Extract the constant stored in this value.
@end deftypemethod

@defop Operator jit_value {jit_value operator+} (const jit_value& @var{value1}, const jit_value& @var{value2})
@defopx Operator jit_value {jit_value operator-} (const jit_value& @var{value1}, const jit_value& @var{value2})
@defopx Operator jit_value {jit_value operator*} (const jit_value& @var{value1}, const jit_value& @var{value2})
@defopx Operator jit_value {jit_value operator/} (const jit_value& @var{value1}, const jit_value& @var{value2})
@defopx Operator jit_value {jit_value operator%} (const jit_value& @var{value1}, const jit_value& @var{value2})
@defopx Operator jit_value {jit_value operator-} (const jit_value& @var{value1})
@defopx Operator jit_value {jit_value operator&} (const jit_value& @var{value1}, const jit_value& @var{value2})
@defopx Operator jit_value {jit_value operator|} (const jit_value& @var{value1}, const jit_value& @var{value2})
@defopx Operator jit_value {jit_value operator^} (const jit_value& @var{value1}, const jit_value& @var{value2})
@defopx Operator jit_value {jit_value operator~} (const jit_value& @var{value1})
@defopx Operator jit_value {jit_value operator<<} (const jit_value& @var{value1}, const jit_value& @var{value2})
@defopx Operator jit_value {jit_value operator>>} (const jit_value& @var{value1}, const jit_value& @var{value2})
@defopx Operator jit_value {jit_value operator==} (const jit_value& @var{value1}, const jit_value& @var{value2})
@defopx Operator jit_value {jit_value operator!=} (const jit_value& @var{value1}, const jit_value& @var{value2})
@defopx Operator jit_value {jit_value operator<} (const jit_value& @var{value1}, const jit_value& @var{value2})
@defopx Operator jit_value {jit_value operator<=} (const jit_value& @var{value1}, const jit_value& @var{value2})
@defopx Operator jit_value {jit_value operator>} (const jit_value& @var{value1}, const jit_value& @var{value2})
@defopx Operator jit_value {jit_value operator>=} (const jit_value& @var{value1}, const jit_value& @var{value2})
Generate an arithmetic, bitwise, or comparison instruction based on
one or two @code{jit_value} objects.  These operators are shortcuts
for calling @code{insn_add}, @code{insn_sub}, etc on the
@code{jit_function} object.
@end defop

@*/

// Get the function that owns a pair of values.  It will choose
// the function for the first value, unless it is NULL (e.g. for
// global values).  In that case, it will choose the function
// for the second value.
static inline jit_function_t value_owner
	(const jit_value& value1, const jit_value& value2)
{
	jit_function_t func = jit_value_get_function(value1.raw());
	if(func)
	{
		return func;
	}
	else
	{
		return jit_value_get_function(value2.raw());
	}
}

jit_value operator+(const jit_value& value1, const jit_value& value2)
{
	return jit_value(jit_insn_add(value_owner(value1, value2),
								  value1.raw(), value2.raw()));
}

jit_value operator-(const jit_value& value1, const jit_value& value2)
{
	return jit_value(jit_insn_sub(value_owner(value1, value2),
								  value1.raw(), value2.raw()));
}

jit_value operator*(const jit_value& value1, const jit_value& value2)
{
	return jit_value(jit_insn_mul(value_owner(value1, value2),
								  value1.raw(), value2.raw()));
}

jit_value operator/(const jit_value& value1, const jit_value& value2)
{
	return jit_value(jit_insn_div(value_owner(value1, value2),
								  value1.raw(), value2.raw()));
}

jit_value operator%(const jit_value& value1, const jit_value& value2)
{
	return jit_value(jit_insn_rem(value_owner(value1, value2),
								  value1.raw(), value2.raw()));
}

jit_value operator-(const jit_value& value1)
{
	return jit_value(jit_insn_neg(jit_value_get_function(value1.raw()),
								  value1.raw()));
}

jit_value operator&(const jit_value& value1, const jit_value& value2)
{
	return jit_value(jit_insn_and(value_owner(value1, value2),
								  value1.raw(), value2.raw()));
}

jit_value operator|(const jit_value& value1, const jit_value& value2)
{
	return jit_value(jit_insn_or(value_owner(value1, value2),
								 value1.raw(), value2.raw()));
}

jit_value operator^(const jit_value& value1, const jit_value& value2)
{
	return jit_value(jit_insn_xor(value_owner(value1, value2),
								  value1.raw(), value2.raw()));
}

jit_value operator~(const jit_value& value1)
{
	return jit_value(jit_insn_not(jit_value_get_function(value1.raw()),
								  value1.raw()));
}

jit_value operator<<(const jit_value& value1, const jit_value& value2)
{
	return jit_value(jit_insn_shl(value_owner(value1, value2),
								  value1.raw(), value2.raw()));
}

jit_value operator>>(const jit_value& value1, const jit_value& value2)
{
	return jit_value(jit_insn_shr(value_owner(value1, value2),
								  value1.raw(), value2.raw()));
}

jit_value operator==(const jit_value& value1, const jit_value& value2)
{
	return jit_value(jit_insn_eq(value_owner(value1, value2),
								 value1.raw(), value2.raw()));
}

jit_value operator!=(const jit_value& value1, const jit_value& value2)
{
	return jit_value(jit_insn_ne(value_owner(value1, value2),
								 value1.raw(), value2.raw()));
}

jit_value operator<(const jit_value& value1, const jit_value& value2)
{
	return jit_value(jit_insn_lt(value_owner(value1, value2),
								 value1.raw(), value2.raw()));
}

jit_value operator<=(const jit_value& value1, const jit_value& value2)
{
	return jit_value(jit_insn_le(value_owner(value1, value2),
								 value1.raw(), value2.raw()));
}

jit_value operator>(const jit_value& value1, const jit_value& value2)
{
	return jit_value(jit_insn_gt(value_owner(value1, value2),
								 value1.raw(), value2.raw()));
}

jit_value operator>=(const jit_value& value1, const jit_value& value2)
{
	return jit_value(jit_insn_ge(value_owner(value1, value2),
								 value1.raw(), value2.raw()));
}
