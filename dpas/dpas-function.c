/*
 * dpas-function.c - Special handling for Dynamic Pascal functions.
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

#include "dpas-internal.h"

static jit_context_t current_context;
static jit_function_t *function_stack = 0;
static int function_stack_size = 0;
static jit_function_t *main_list = 0;
static int main_list_size = 0;

jit_context_t dpas_current_context(void)
{
	if(!current_context)
	{
		current_context = jit_context_create();
		if(!current_context)
		{
			dpas_out_of_memory();
		}
	}
	return current_context;
}

jit_function_t dpas_current_function(void)
{
	if(function_stack_size > 0)
	{
		return function_stack[function_stack_size - 1];
	}
	else
	{
		/* We are probably compiling the "main" method for this module */
		jit_type_t signature = jit_type_create_signature
			(jit_abi_cdecl, jit_type_void, 0, 0, 1);
		if(!signature)
		{
			dpas_out_of_memory();
		}
		return dpas_new_function(signature);
	}
}

jit_function_t dpas_new_function(jit_type_t signature)
{
	jit_function_t func;
	func = jit_function_create(dpas_current_context(), signature);
	if(!func)
	{
		dpas_out_of_memory();
	}
	function_stack = (jit_function_t *)jit_realloc
		(function_stack, sizeof(jit_function_t) * (function_stack_size + 1));
	if(!function_stack)
	{
		dpas_out_of_memory();
	}
	function_stack[function_stack_size++] = func;
	return func;
}

void dpas_pop_function(void)
{
	if(function_stack_size > 0)
	{
		--function_stack_size;
	}
}

int dpas_function_is_nested(void)
{
	return (function_stack_size > 1);
}

dpas_semvalue dpas_lvalue_to_rvalue(dpas_semvalue value)
{
	if(dpas_sem_is_lvalue_ea(value))
	{
		jit_type_t type = dpas_sem_get_type(value);
		jit_value_t rvalue = dpas_sem_get_value(value);
		rvalue = jit_insn_load_relative
			(dpas_current_function(), rvalue, 0, type);
		if(!rvalue)
		{
			dpas_out_of_memory();
		}
		dpas_sem_set_rvalue(value, type, rvalue);
	}
	return value;
}

void dpas_add_main_function(jit_function_t func)
{
	main_list = (jit_function_t *)jit_realloc
		(main_list, sizeof(jit_function_t) * (main_list_size + 1));
	if(!main_list)
	{
		dpas_out_of_memory();
	}
	main_list[main_list_size++] = func;
}

int dpas_run_main_functions(void)
{
	int index;
	for(index = 0; index < main_list_size; ++index)
	{
		if(!jit_function_apply(main_list[index], 0, 0))
		{
			fprintf(stderr, "Exception 0x%lx thrown past top level\n",
					(long)(jit_nint)(jit_exception_get_last()));
			return 0;
		}
	}
	return 1;
}
