/*
 * jit-rules.c - Rules that define the characteristics of the back-end.
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

/*
 * The information blocks for all registers in the system.
 */
jit_reginfo_t const _jit_reg_info[JIT_NUM_REGS] = {JIT_REG_INFO};

int _jit_int_lowest_byte(void)
{
	union
	{
		unsigned char bytes[4];
		jit_int value;
	} volatile un;
	int posn;
	un.value = (jit_int)0x01020304;
	posn = 0;
	while(un.bytes[posn] != 0x04)
	{
		++posn;
	}
	return posn;
}

int _jit_int_lowest_short(void)
{
	union
	{
		unsigned char bytes[4];
		jit_int value;
	} volatile un;
	int posn;
	un.value = (jit_int)0x01020304;
	posn = 0;
	while(un.bytes[posn] != 0x03 && un.bytes[posn] != 0x04)
	{
		++posn;
	}
	return posn;
}

int _jit_nint_lowest_byte(void)
{
#ifdef JIT_NATIVE_INT32
	return _jit_int_lowest_byte();
#else
	union
	{
		unsigned char bytes[8];
		jit_long value;
	} volatile un;
	int posn;
	un.value = (jit_long)0x0102030405060708;
	posn = 0;
	while(un.bytes[posn] != 0x08)
	{
		++posn;
	}
	return posn;
#endif
}

int _jit_nint_lowest_short(void)
{
#ifdef JIT_NATIVE_INT32
	return _jit_int_lowest_short();
#else
	union
	{
		unsigned char bytes[8];
		jit_long value;
	} volatile un;
	int posn;
	un.value = (jit_long)0x0102030405060708;
	posn = 0;
	while(un.bytes[posn] != 0x07 && un.bytes[posn] != 0x08)
	{
		++posn;
	}
	return posn;
#endif
}

int _jit_nint_lowest_int(void)
{
#ifdef JIT_NATIVE_INT32
	return 0;
#else
	union
	{
		unsigned char bytes[8];
		jit_long value;
	} volatile un;
	int posn;
	un.value = (jit_long)0x0102030405060708;
	posn = 0;
	while(un.bytes[posn] <= 0x04)
	{
		++posn;
	}
	return posn;
#endif
}
