/*
 * jit-bitset.h - Bitset routines for the JIT.
 *
 * Copyright (C) 2006  Southern Storm Software, Pty Ltd.
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

#ifndef	_JIT_BITSET_H
#define	_JIT_BITSET_H

#define _JIT_BITSET_WORD_BITS (8 * sizeof(_jit_bitset_word_t))

typedef unsigned long _jit_bitset_word_t;
typedef struct _jit_bitset _jit_bitset_t;

/* TODO: Use less space. Perhaps borrow bitmap from gcc. */
struct _jit_bitset
{
	int size;
	_jit_bitset_word_t *bits;
};

void _jit_bitset_init(_jit_bitset_t *bs);
int _jit_bitset_allocate(_jit_bitset_t *bs, int size);
int _jit_bitset_is_allocated(_jit_bitset_t *bs);
void _jit_bitset_free(_jit_bitset_t *bs);
void _jit_bitset_set_bit(_jit_bitset_t *bs, int bit);
void _jit_bitset_clear_bit(_jit_bitset_t *bs, int bit);
int _jit_bitset_test_bit(_jit_bitset_t *bs, int bit);
void _jit_bitset_clear(_jit_bitset_t *bs);
int _jit_bitset_empty(_jit_bitset_t *bs);
void _jit_bitset_add(_jit_bitset_t *dest, _jit_bitset_t *src);
void _jit_bitset_sub(_jit_bitset_t *dest, _jit_bitset_t *src);
int _jit_bitset_copy(_jit_bitset_t *dest, _jit_bitset_t *src);
int _jit_bitset_equal(_jit_bitset_t *bs1, _jit_bitset_t *bs2);

#endif
