/*
 * dpas-function.c - Special handling for Dynamic Pascal functions.
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

#include "dpas-internal.h"

static jit_context_t current_context;
static jit_function_t *function_stack = 0;
static int function_stack_size = 0;

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
