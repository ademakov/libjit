/*
 * jit-plus-jump-table.cpp - C++ wrapper for JIT jump tables.
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

#include <jit/jit-plus.h>

jit_jump_table::jit_jump_table(int size)
{
	try
	{
		labels = new jit_label_t[size];
	}
	catch(...)
	{
		throw jit_build_exception(JIT_RESULT_OUT_OF_MEMORY);
	}
	for (int i = 0; i < size; i++)
	{
		labels[i] = jit_label_undefined;
	}
	num_labels = size;
}

jit_jump_table::~jit_jump_table()
{
	delete [] labels;
}

jit_label
jit_jump_table::get(int index)
{
	if (index < 0 || index >= num_labels)
	{
		throw jit_build_exception(JIT_RESULT_COMPILE_ERROR);
	}
	return jit_label(labels[index]);
}

void
jit_jump_table::set(int index, jit_label label)
{
	if (index < 0 || index >= num_labels)
	{
		throw jit_build_exception(JIT_RESULT_COMPILE_ERROR);
	}
	labels[index] = label.raw();
}
