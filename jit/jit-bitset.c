/*
 * jit-bitset.h - Bitset routines for the JIT.
 *
 * Copyright (C) 2006  Southern Storm Software, Pty Ltd.
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
#include "jit-bitset.h"

void
_jit_bitset_init(_jit_bitset_t *bs)
{
	bs->size = 0;
	bs->bits = 0;
}

int
_jit_bitset_allocate(_jit_bitset_t *bs, int size)
{
	if(size > 0)
	{
		size = (size + _JIT_BITSET_WORD_BITS - 1) / _JIT_BITSET_WORD_BITS;
		bs->size = size;
		bs->bits = jit_calloc(size, sizeof(_jit_bitset_word_t));
		if(!bs->bits)
		{
			return 0;
		}
	}
	else
	{
		bs->size = 0;
		bs->bits = 0;
	}
	return 1;
}

int
_jit_bitset_resize(_jit_bitset_t *bs, int size)
{
	int old_size;

	if(size > 0)
	{
		old_size = bs->size;
		size = (size + _JIT_BITSET_WORD_BITS - 1) / _JIT_BITSET_WORD_BITS;
		bs->size = size;
		bs->bits = jit_realloc(bs->bits, size * sizeof(_jit_bitset_word_t));
		if(!bs->bits)
		{
			return 0;
		}
		if(size > old_size)
		{
			memset(bs->bits + old_size, 0, size - old_size);
		}
	}
	else
	{
		bs->size = 0;
		bs->bits = 0;
	}
	return 1;
}

int
 _jit_bitset_is_allocated(_jit_bitset_t *bs)
{
	return (bs->bits != 0);
}

void
_jit_bitset_free(_jit_bitset_t *bs)
{
	if(bs->bits)
	{
		jit_free(bs->bits);
		bs->size = 0;
		bs->bits = 0;
	}
}

int
_jit_bitset_size(_jit_bitset_t *bs)
{
	return bs->size * _JIT_BITSET_WORD_BITS;
}

void
_jit_bitset_set_bit(_jit_bitset_t *bs, int bit)
{
	int word;
	_jit_bitset_word_t pattern;
	word = bit / _JIT_BITSET_WORD_BITS;
	pattern = (_jit_bitset_word_t)1 << (bit % _JIT_BITSET_WORD_BITS);
	bs->bits[word] |= pattern;
}

void
_jit_bitset_clear_bit(_jit_bitset_t *bs, int bit)
{
	int word;
	_jit_bitset_word_t pattern;
	word = bit / _JIT_BITSET_WORD_BITS;
	pattern = (_jit_bitset_word_t)1 << (bit % _JIT_BITSET_WORD_BITS);
	bs->bits[word] &= ~pattern;
}

int
_jit_bitset_test_bit(_jit_bitset_t *bs, int bit)
{
	int word;
	_jit_bitset_word_t pattern;
	word = bit / _JIT_BITSET_WORD_BITS;
	pattern = (_jit_bitset_word_t)1 << (bit % _JIT_BITSET_WORD_BITS);
	return (bs->bits[word] & pattern) != 0;
}

void
_jit_bitset_clear(_jit_bitset_t *bs)
{
	int i;
	for(i = 0; i < bs->size; i++)
	{
		bs->bits[i] = 0;
	}
}

int
_jit_bitset_empty(_jit_bitset_t *bs)
{
	int i;
	for(i = 0; i < bs->size; i++)
	{
		if(bs->bits[i])
		{
			return 0;
		}
	}
	return 1;
}

void
_jit_bitset_add(_jit_bitset_t *dest, _jit_bitset_t *src)
{
	int i;
	for(i = 0; i < dest->size; i++)
	{
		dest->bits[i] |= src->bits[i];
	}
}

void
_jit_bitset_sub(_jit_bitset_t *dest, _jit_bitset_t *src)
{
	int i;
	for(i = 0; i < dest->size; i++)
	{
		dest->bits[i] &= ~src->bits[i];
	}
}

int
_jit_bitset_copy(_jit_bitset_t *dest, _jit_bitset_t *src)
{
	int i;
	int changed;

	changed = 0;
	for(i = 0; i < dest->size; i++)
	{
		if(dest->bits[i] != src->bits[i])
		{
			dest->bits[i] = src->bits[i];
			changed = 1;
		}
	}
	return changed;
}

int
_jit_bitset_equal(_jit_bitset_t *bs1, _jit_bitset_t *bs2)
{
	int i;
	for(i = 0; i < bs1->size; i++)
	{
		if(bs1->bits[i] != bs2->bits[i])
		{
			return 0;
		}
	}
	return 1;
}

int
_jit_bitset_test(_jit_bitset_t *bs1, _jit_bitset_t *bs2)
{
	int i;
	for(i = 0; i < bs1->size; i++)
	{
		if((bs1->bits[i] & bs2->bits[i]) != 0)
		{
			return 1;
		}
	}
	return 0;
}

int
_jit_bitset_contains(_jit_bitset_t *outer, _jit_bitset_t *inner)
{
	int i;
	for(i = 0; i < outer->size; i++)
	{
		if(((outer->bits[i] ^ inner->bits[i]) & inner->bits[i]) != 0)
		{
			return 0;
		}
	}
	return 1;
}
