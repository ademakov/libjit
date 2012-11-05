/*
 * jit-plus-function.cpp - C++ wrapper for JIT functions.
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
#include <config.h>
#ifdef HAVE_STDARG_H
	#include <stdarg.h>
#elif HAVE_VARARGS_H
	#include <varargs.h>
#endif

/*@

The @code{jit_function} class provides a C++ counterpart to the
C @code{jit_function_t} type.  @xref{Functions}, for more information
on creating and managing functions.

The @code{jit_function} class also provides a large number of methods
for creating the instructions within a function body.  @xref{Instructions},
for more information on creating and managing instructions.

@*/

#define	JITPP_MAPPING		20000

jit_type_t const jit_function::end_params = (jit_type_t)0;

/*@
 * @defop Constructor jit_function jit_function (jit_context& @var{context}, jit_type_t @var{signature})
 * Constructs a new function handler with the specified @var{signature} in
 * the given @var{context}.  It then calls @code{create(@var{signature})} to
 * create the actual function.
 * @end defop
@*/
jit_function::jit_function(jit_context& context, jit_type_t signature)
{
	// Save the context for the "create" method.
	this->context = context.raw();
	this->func = 0;

	// Create the function.
	create(signature);
}

/*@
 * @defop Constructor jit_function jit_function (jit_context& @var{context})
 * Constructs a new function handler in the specified @var{context}.
 * The actual function is not created until you call @code{create()}.
 * @end defop
@*/
jit_function::jit_function(jit_context& context)
{
	// Save the context, but don't create the function yet.
	this->context = context.raw();
	this->func = 0;
}

/*@
 * @defop Constructor jit_function jit_function (jit_function_t @var{func})
 * Constructs a new function handler and wraps it around the specified
 * raw C @code{jit_function_t} object.  This can be useful for layering
 * the C++ on-demand building facility on top of an existing C function.
 * @end defop
@*/
jit_function::jit_function(jit_function_t func)
{
	this->context = jit_function_get_context(func);
	this->func = func;
	if(func)
	{
		jit_context_build_start(context);
		jit_function_set_meta(func, JITPP_MAPPING, (void *)this, 0, 0);
		register_on_demand();
		jit_context_build_end(context);
	}
}

/*@
 * @defop Destructor jit_function ~jit_function ()
 * Destroy this function handler.  The raw function will persist
 * until the context is destroyed.
 * @end defop
@*/
jit_function::~jit_function()
{
	if(func)
	{
		jit_context_build_start(context);
		jit_function_free_meta(func, JITPP_MAPPING);
		jit_context_build_end(context);
	}
}

/*@
 * @deftypemethod jit_function jit_function_t raw () const
 * Get the raw C @code{jit_function_t} value that underlies this object.
 * @end deftypemethod
 *
 * @deftypemethod jit_function int is_valid () const
 * Determine if the raw C @code{jit_function_t} value that
 * underlies this object is valid.
 * @end deftypemethod
@*/

/*@
 * @deftypemethod jit_function {static jit_function *} from_raw (jit_function_t @var{func})
 * Find the C++ @code{jit_function} object that is associated with a
 * raw C @code{jit_function_t} pointer.  Returns NULL if there is
 * no such object.
 * @end deftypemethod
@*/
jit_function *jit_function::from_raw(jit_function_t func)
{
	return (jit_function *)jit_function_get_meta(func, JITPP_MAPPING);
}

/*@
 * @deftypemethod jit_function jit_type_t signature () const
 * Get the signature type for this function.
 * @end deftypemethod
@*/

/*@
 * @deftypemethod jit_function void create (jit_type_t @var{signature})
 * Create this function if it doesn't already exist.
 * @end deftypemethod
@*/
void jit_function::create(jit_type_t signature)
{
	// Bail out if the function is already created.
	if(func)
	{
		return;
	}

	// Lock down the context.
	jit_context_build_start(context);

	// Create the new function.
	func = jit_function_create(context, signature);
	if(!func)
	{
		jit_context_build_end(context);
		return;
	}

	// Store this object's pointer on the raw function so that we can
	// map the raw function back to this object later.
	jit_function_set_meta(func, JITPP_MAPPING, (void *)this, 0, 0);

	// Register us as the on-demand compiler.
	register_on_demand();

	// Unlock the context.
	jit_context_build_end(context);
}

/*@
 * @deftypemethod jit_function void create ()
 * Create this function if it doesn't already exist.  This version will
 * call the virtual @code{create_signature()} method to obtain the
 * signature from the subclass.
 * @end deftypemethod
@*/
void jit_function::create()
{
	if(!func)
	{
		jit_type_t signature = create_signature();
		create(signature);
		jit_type_free(signature);
	}
}

/*@
 * @deftypemethod jit_function int compile ()
 * Compile this function explicity.  You normally don't need to use this
 * method because the function will be compiled on-demand.  If you do
 * choose to build the function manually, then the correct sequence of
 * operations is as follows:
 *
 * @enumerate
 * @item
 * Invoke the @code{build_start} method to lock down the function builder.
 *
 * @item
 * Build the function by calling the value-related and instruction-related
 * methods within @code{jit_function}.
 *
 * @item
 * Compile the function with the @code{compile} method.
 *
 * @item
 * Unlock the function builder by invoking @code{build_end}.
 * @end enumerate
 * @end deftypemethod
 *
 * @deftypemethod jit_function int is_compiled () const
 * Determine if this function has already been compiled.
 * @end deftypemethod
@*/
int jit_function::compile()
{
	if(!func)
	{
		return 0;
	}
	else
	{
		return jit_function_compile(func);
	}
}

/*@
 * @deftypemethod jit_function void set_optimization_level (unsigned int @var{level})
 * @deftypemethodx jit_function {unsigned int} optimization_level () const
 * Set or get the optimization level for this function.
 * @end deftypemethod
 *
 * @deftypemethod jit_function {static unsigned int} max_optimization_level ()
 * Get the maximum optimization level for @code{libjit}.
 * @end deftypemethod
 *
 * @deftypemethod jit_function {void *} closure () const
 * @deftypemethodx jit_function {void *} vtable_pointer () const
 * Get the closure or vtable pointer form of this function.
 * @end deftypemethod
 *
 * @deftypemethod jit_function int apply (void** @var{args}, void* @var{result})
 * @deftypemethodx jit_function int apply (jit_type_t @var{signature}, void** @var{args}, void* @var{return_area})
 * Call this function, applying the specified arguments.
 * @end deftypemethod
@*/

/*@
 * @deftypemethod jit_function {static jit_type_t} signature_helper (jit_type_t @var{return_type}, ...)
 * You can call this method from @code{create_signature()} to help build the
 * correct signature for your function.  The first parameter is the return
 * type, following by zero or more types for the parameters.  The parameter
 * list is terminated with the special value @code{jit_function::end_params}.
 *
 * A maximum of 32 parameter types can be supplied, and the signature
 * ABI is always set to @code{jit_abi_cdecl}.
 * @end deftypemethod
@*/
jit_type_t jit_function::signature_helper(jit_type_t return_type, ...)
{
	va_list va;
	jit_type_t params[32];
	unsigned int num_params = 0;
	jit_type_t type;
#ifdef HAVE_STDARG_H
	va_start(va, return_type);
#else
	va_start(va);
#endif
	while(num_params < 32 && (type = va_arg(va, jit_type_t)) != 0)
	{
		params[num_params++] = type;
	}
	va_end(va);
	return jit_type_create_signature
		(jit_abi_cdecl, return_type, params, num_params, 1);
}

/*@
 * @deftypemethod jit_function void build ()
 * This method is called when the function has to be build on-demand,
 * or in response to an explicit @code{recompile} request.  You build the
 * function by calling the value-related and instruction-related
 * methods within @code{jit_function} that are described below.
 *
 * The default implementation of @code{build} will fail, so you must
 * override it if you didn't build the function manually and call
 * @code{compile}.
 * @end deftypemethod
@*/
void jit_function::build()
{
	// Normally overridden by subclasses.
	fail();
}

/*@
 * @deftypemethod jit_function jit_type_t create_signature ()
 * This method is called by @code{create()} to create the function's
 * signature.  The default implementation creates a signature that
 * returns @code{void} and has no parameters.
 * @end deftypemethod
@*/
jit_type_t jit_function::create_signature()
{
	// Normally overridden by subclasses.
	return signature_helper(jit_type_void, end_params);
}

/*@
 * @deftypemethod jit_function void fail ()
 * This method can be called by @code{build} to fail the on-demand
 * compilation process.  It throws an exception to unwind the build.
 * @end deftypemethod
@*/
void jit_function::fail()
{
	throw jit_build_exception(JIT_RESULT_COMPILE_ERROR);
}

/*@
 * @deftypemethod jit_function void out_of_memory ()
 * This method can be called by @code{build} to indicate that the on-demand
 * compilation process ran out of memory.  It throws an exception to
 * unwind the build.
 * @end deftypemethod
@*/
void jit_function::out_of_memory()
{
	throw jit_build_exception(JIT_RESULT_OUT_OF_MEMORY);
}

/*@
 * @deftypemethod jit_function void build_start ()
 * Start an explicit build process.  Not needed if you will be using
 * on-demand compilation.
 * @end deftypemethod
 *
 * @deftypemethod jit_function void build_end ()
 * End an explicit build process.
 * @end deftypemethod
@*/

#define	value_wrap(x)	\
			jit_value val((x)); \
			if(!(val.raw())) \
				out_of_memory(); \
			return val

/*@
 * @deftypemethod jit_function jit_value new_value (jit_type_t @var{type})
 * Create a new temporary value.  This is the C++ counterpart to
 * @code{jit_value_create}.
 * @end deftypemethod
@*/
jit_value jit_function::new_value(jit_type_t type)
{
	value_wrap(jit_value_create(func, type));
}

/*@
 * @deftypemethod jit_function jit_value new_constant (jit_sbyte @var{value}, jit_type_t @var{type})
 * @deftypemethodx jit_function jit_value new_constant (jit_ubyte @var{value}, jit_type_t @var{type})
 * @deftypemethodx jit_function jit_value new_constant (jit_short @var{value}, jit_type_t @var{type})
 * @deftypemethodx jit_function jit_value new_constant (jit_ushort @var{value}, jit_type_t @var{type})
 * @deftypemethodx jit_function jit_value new_constant (jit_int @var{value}, jit_type_t @var{type})
 * @deftypemethodx jit_function jit_value new_constant (jit_uint @var{value}, jit_type_t @var{type})
 * @deftypemethodx jit_function jit_value new_constant (jit_long @var{value}, jit_type_t @var{type})
 * @deftypemethodx jit_function jit_value new_constant (jit_ulong @var{value}, jit_type_t @var{type})
 * @deftypemethodx jit_function jit_value new_constant (jit_float32 @var{value}, jit_type_t @var{type})
 * @deftypemethodx jit_function jit_value new_constant (jit_float64 @var{value}, jit_type_t @var{type})
 * @deftypemethodx jit_function jit_value new_constant (jit_nfloat @var{value}, jit_type_t @var{type})
 * @deftypemethodx jit_function jit_value new_constant (void* @var{value}, jit_type_t @var{type})
 * @deftypemethodx jit_function jit_value new_constant (const jit_constant_t& @var{value})
 * Create constant values of various kinds.  @xref{Values}, for more
 * information on creating and managing constants.
 * @end deftypemethod
@*/
jit_value jit_function::new_constant(jit_sbyte value, jit_type_t type)
{
	if(!type)
	{
		type = jit_type_sbyte;
	}
	value_wrap(jit_value_create_nint_constant(func, type, (jit_nint)value));
}

jit_value jit_function::new_constant(jit_ubyte value, jit_type_t type)
{
	if(!type)
	{
		type = jit_type_ubyte;
	}
	value_wrap(jit_value_create_nint_constant(func, type, (jit_nint)value));
}

jit_value jit_function::new_constant(jit_short value, jit_type_t type)
{
	if(!type)
	{
		type = jit_type_short;
	}
	value_wrap(jit_value_create_nint_constant(func, type, (jit_nint)value));
}

jit_value jit_function::new_constant(jit_ushort value, jit_type_t type)
{
	if(!type)
	{
		type = jit_type_ushort;
	}
	value_wrap(jit_value_create_nint_constant(func, type, (jit_nint)value));
}

jit_value jit_function::new_constant(jit_int value, jit_type_t type)
{
	if(!type)
	{
		type = jit_type_int;
	}
	value_wrap(jit_value_create_nint_constant(func, type, (jit_nint)value));
}

jit_value jit_function::new_constant(jit_uint value, jit_type_t type)
{
	if(!type)
	{
		type = jit_type_uint;
	}
	value_wrap(jit_value_create_nint_constant(func, type, (jit_nint)value));
}

jit_value jit_function::new_constant(jit_long value, jit_type_t type)
{
	if(!type)
	{
		type = jit_type_long;
	}
	value_wrap(jit_value_create_long_constant(func, type, value));
}

jit_value jit_function::new_constant(jit_ulong value, jit_type_t type)
{
	if(!type)
	{
		type = jit_type_ulong;
	}
	value_wrap(jit_value_create_long_constant(func, type, (jit_long)value));
}

jit_value jit_function::new_constant(jit_float32 value, jit_type_t type)
{
	if(!type)
	{
		type = jit_type_float32;
	}
	value_wrap(jit_value_create_float32_constant(func, type, value));
}

jit_value jit_function::new_constant(jit_float64 value, jit_type_t type)
{
	if(!type)
	{
		type = jit_type_float64;
	}
	value_wrap(jit_value_create_float64_constant(func, type, value));
}

#ifndef JIT_NFLOAT_IS_DOUBLE

jit_value jit_function::new_constant(jit_nfloat value, jit_type_t type)
{
	if(!type)
	{
		type = jit_type_nfloat;
	}
	value_wrap(jit_value_create_nfloat_constant(func, type, value));
}

#endif

jit_value jit_function::new_constant(void *value, jit_type_t type)
{
	if(!type)
	{
		type = jit_type_void_ptr;
	}
	value_wrap(jit_value_create_nint_constant(func, type, (jit_nint)value));
}

jit_value jit_function::new_constant(const jit_constant_t& value)
{
	value_wrap(jit_value_create_constant(func, &value));
}

/*@
 * @deftypemethod jit_function jit_value get_param (unsigned int @var{param})
 * Get the value that corresponds to parameter @var{param}.
 * @end deftypemethod
@*/
jit_value jit_function::get_param(unsigned int param)
{
	value_wrap(jit_value_get_param(func, param));
}

/*@
 * @deftypemethod jit_function jit_value get_struct_pointer ()
 * Get the value that corresponds to the structure pointer parameter,
 * if this function has one.  Returns an empty value if it does not.
 * @end deftypemethod
@*/
jit_value jit_function::get_struct_pointer()
{
	value_wrap(jit_value_get_struct_pointer(func));
}

/*@
 * @deftypemethod jit_function jit_label new_label ()
 * Create a new label.  This is the C++ counterpart to
 * @code{jit_function_reserve_label}.
 * @end deftypemethod
@*/
jit_label jit_function::new_label()
{
	return jit_label(jit_function_reserve_label(func));
}

/*@
 * @deftypemethod jit_function void insn_label (jit_label& @var{label})
 * @deftypemethodx jit_function void insn_new_block ()
 * @deftypemethodx jit_function jit_value insn_load (const jit_value& @var{value})
 * @deftypemethodx jit_function jit_value insn_dup (const jit_value& @var{value})
 * @deftypemethodx jit_function jit_value insn_load_small (const jit_value& @var{value})
 * @deftypemethodx jit_function void store (const jit_value& @var{dest}, const jit_value& @var{value})
 * @deftypemethodx jit_function jit_value insn_load_relative (const jit_value& @var{value}, jit_nint @var{offset}, jit_type_t @var{type})
 * @deftypemethodx jit_function void insn_store_relative (const jit_value& @var{dest}, jit_nint @var{offset}, const jit_value& @var{value})
 * @deftypemethodx jit_function jit_value insn_add_relative (const jit_value& @var{value}, jit_nint @var{offset})
 * @deftypemethodx jit_function jit_value insn_load_elem (const jit_value& @var{base_addr}, const jit_value& @var{index}, jit_type_t @var{elem_type})
 * @deftypemethodx jit_function jit_value insn_load_elem_address (const jit_value& @var{base_addr}, const jit_value& @var{index}, jit_type_t @var{elem_type})
 * @deftypemethodx jit_function void insn_store_elem (const jit_value& @var{base_addr}, const jit_value& @var{index}, const jit_value& @var{value})
 * @deftypemethodx jit_function void insn_check_null (const jit_value& @var{value})
 * @deftypemethodx jit_function jit_value insn_add (const jit_value& @var{value1}, const jit_value& @var{value2})
 * @deftypemethodx jit_function jit_value insn_add_ovf (const jit_value& @var{value1}, const jit_value& @var{value2})
 * @deftypemethodx jit_function jit_value insn_sub (const jit_value& @var{value1}, const jit_value& @var{value2})
 * @deftypemethodx jit_function jit_value insn_sub_ovf (const jit_value& @var{value1}, const jit_value& @var{value2})
 * @deftypemethodx jit_function jit_value insn_mul (const jit_value& @var{value1}, const jit_value& @var{value2})
 * @deftypemethodx jit_function jit_value insn_mul_ovf (const jit_value& @var{value1}, const jit_value& @var{value2})
 * @deftypemethodx jit_function jit_value insn_div (const jit_value& @var{value1}, const jit_value& @var{value2})
 * @deftypemethodx jit_function jit_value insn_rem (const jit_value& @var{value1}, const jit_value& @var{value2})
 * @deftypemethodx jit_function jit_value insn_rem_ieee (const jit_value& @var{value1}, const jit_value& @var{value2})
 * @deftypemethodx jit_function jit_value insn_neg (const jit_value& @var{value1})
 * @deftypemethodx jit_function jit_value insn_and (const jit_value& @var{value1}, const jit_value& @var{value2})
 * @deftypemethodx jit_function jit_value insn_or (const jit_value& @var{value1}, const jit_value& @var{value2})
 * @deftypemethodx jit_function jit_value insn_xor (const jit_value& @var{value1}, const jit_value& @var{value2})
 * @deftypemethodx jit_function jit_value insn_not (const jit_value& @var{value1})
 * @deftypemethodx jit_function jit_value insn_shl (const jit_value& @var{value1}, const jit_value& @var{value2})
 * @deftypemethodx jit_function jit_value insn_shr (const jit_value& @var{value1}, const jit_value& @var{value2})
 * @deftypemethodx jit_function jit_value insn_ushr (const jit_value& @var{value1}, const jit_value& @var{value2})
 * @deftypemethodx jit_function jit_value insn_sshr (const jit_value& @var{value1}, const jit_value& @var{value2})
 * @deftypemethodx jit_function jit_value insn_eq (const jit_value& @var{value1}, const jit_value& @var{value2})
 * @deftypemethodx jit_function jit_value insn_ne (const jit_value& @var{value1}, const jit_value& @var{value2})
 * @deftypemethodx jit_function jit_value insn_lt (const jit_value& @var{value1}, const jit_value& @var{value2})
 * @deftypemethodx jit_function jit_value insn_le (const jit_value& @var{value1}, const jit_value& @var{value2})
 * @deftypemethodx jit_function jit_value insn_gt (const jit_value& @var{value1}, const jit_value& @var{value2})
 * @deftypemethodx jit_function jit_value insn_ge (const jit_value& @var{value1}, const jit_value& @var{value2})
 * @deftypemethodx jit_function jit_value insn_cmpl (const jit_value& @var{value1}, const jit_value& @var{value2})
 * @deftypemethodx jit_function jit_value insn_cmpg (const jit_value& @var{value1}, const jit_value& @var{value2})
 * @deftypemethodx jit_function jit_value insn_to_bool (const jit_value& @var{value1})
 * @deftypemethodx jit_function jit_value insn_to_not_bool (const jit_value& @var{value1})
 * @deftypemethodx jit_function jit_value insn_acos (const jit_value& @var{value1})
 * @deftypemethodx jit_function jit_value insn_asin (const jit_value& @var{value1})
 * @deftypemethodx jit_function jit_value insn_atan (const jit_value& @var{value1})
 * @deftypemethodx jit_function jit_value insn_atan2 (const jit_value& @var{value1}, const jit_value& @var{value2})
 * @deftypemethodx jit_function jit_value insn_ceil (const jit_value& @var{value1})
 * @deftypemethodx jit_function jit_value insn_cos (const jit_value& @var{value1})
 * @deftypemethodx jit_function jit_value insn_cosh (const jit_value& @var{value1})
 * @deftypemethodx jit_function jit_value insn_exp (const jit_value& @var{value1})
 * @deftypemethodx jit_function jit_value insn_floor (const jit_value& @var{value1})
 * @deftypemethodx jit_function jit_value insn_log (const jit_value& @var{value1})
 * @deftypemethodx jit_function jit_value insn_log10 (const jit_value& @var{value1})
 * @deftypemethodx jit_function jit_value insn_pow (const jit_value& @var{value1}, const jit_value& @var{value2})
 * @deftypemethodx jit_function jit_value insn_rint (const jit_value& @var{value1})
 * @deftypemethodx jit_function jit_value insn_round (const jit_value& @var{value1})
 * @deftypemethodx jit_function jit_value insn_sin (const jit_value& @var{value1})
 * @deftypemethodx jit_function jit_value insn_sinh (const jit_value& @var{value1})
 * @deftypemethodx jit_function jit_value insn_sqrt (const jit_value& @var{value1})
 * @deftypemethodx jit_function jit_value insn_tan (const jit_value& @var{value1})
 * @deftypemethodx jit_function jit_value insn_tanh (const jit_value& @var{value1})
 * @deftypemethodx jit_function jit_value insn_trunc (const jit_value& @var{value1})
 * @deftypemethodx jit_function jit_value insn_is_nan (const jit_value& @var{value1})
 * @deftypemethodx jit_function jit_value insn_is_finite (const jit_value& @var{value1})
 * @deftypemethodx jit_function jit_value insn_is_inf (const jit_value& @var{value1})
 * @deftypemethodx jit_function jit_value insn_abs (const jit_value& @var{value1})
 * @deftypemethodx jit_function jit_value insn_min (const jit_value& @var{value1}, const jit_value& @var{value2})
 * @deftypemethodx jit_function jit_value insn_max (const jit_value& @var{value1}, const jit_value& @var{value2})
 * @deftypemethodx jit_function jit_value insn_sign (const jit_value& @var{value1})
 * @deftypemethodx jit_function void insn_branch (jit_label& @var{label})
 * @deftypemethodx jit_function void insn_branch_if (const jit_value& @var{value}, jit_label& @var{label})
 * @deftypemethodx jit_function void insn_branch_if_not (const jit_value& value, jit_label& @var{label})
 * @deftypemethodx jit_function jit_value insn_address_of (const jit_value& @var{value1})
 * @deftypemethodx jit_function jit_value insn_address_of_label (jit_label& @var{label})
 * @deftypemethodx jit_function jit_value insn_convert (const jit_value& @var{value}, jit_type_t @var{type}, int @var{overflow_check})
 * @deftypemethodx jit_function jit_value insn_call (const char* @var{name}, jit_function_t @var{jit_func}, jit_type_t @var{signature}, jit_value_t* @var{args}, unsigned int @var{num_args}, int @var{flags})
 * @deftypemethodx jit_function jit_value insn_call_indirect (const jit_value& @var{value}, jit_type_t @var{signature}, jit_value_t* @var{args}, unsigned int @var{num_args}, int @var{flags})
 * @deftypemethodx jit_function jit_value insn_call_indirect_vtable (const jit_value& @var{value}, jit_type_t @var{signature}, jit_value_t * @var{args}, unsigned int @var{num_args}, int @var{flags})
 * @deftypemethodx jit_function jit_value insn_call_native (const char* @var{name}, void* @var{native_func}, jit_type_t @var{signature}, jit_value_t* @var{args}, unsigned int @var{num_args}, int @var{flags})
 * @deftypemethodx jit_function jit_value insn_call_intrinsic (const char* @var{name}, void* @var{intrinsic_func}, const jit_intrinsic_descr_t& @var{descriptor}, const jit_value& @var{arg1}, const jit_value& @var{arg2})
 * @deftypemethodx jit_function void insn_incoming_reg (const jit_value& @var{value}, int @var{reg})
 * @deftypemethodx jit_function void insn_incoming_frame_posn (const jit_value& @var{value}, jit_nint @var{posn})
 * @deftypemethodx jit_function void insn_outgoing_reg (const jit_value& @var{value}, int @var{reg})
 * @deftypemethodx jit_function void insn_outgoing_frame_posn (const jit_value& @var{value}, jit_nint @var{posn})
 * @deftypemethodx jit_function void insn_return_reg (const jit_value& @var{value}, int @var{reg})
 * @deftypemethodx jit_function void insn_setup_for_nested (int @var{nested_level}, int @var{reg})
 * @deftypemethodx jit_function void insn_flush_struct (const jit_value& @var{value})
 * @deftypemethodx jit_function jit_value insn_import (jit_value @var{value})
 * @deftypemethodx jit_function void insn_push (const jit_value& @var{value})
 * @deftypemethodx jit_function void insn_push_ptr (const jit_value& @var{value}, jit_type_t @var{type})
 * @deftypemethodx jit_function void insn_set_param (const jit_value& @var{value}, jit_nint @var{offset})
 * @deftypemethodx jit_function void insn_set_param_ptr (const jit_value& @var{value}, jit_type_t @var{type}, jit_nint @var{offset})
 * @deftypemethodx jit_function void insn_push_return_area_ptr ()
 * @deftypemethodx jit_function void insn_return (const jit_value& @var{value})
 * @deftypemethodx jit_function void insn_return ()
 * @deftypemethodx jit_function void insn_return_ptr (const jit_value& @var{value}, jit_type_t @var{type})
 * @deftypemethodx jit_function void insn_default_return ()
 * @deftypemethodx jit_function void insn_throw (const jit_value& @var{value})
 * @deftypemethodx jit_function jit_value insn_get_call_stack ()
 * @deftypemethodx jit_function jit_value insn_thrown_exception ()
 * @deftypemethodx jit_function void insn_uses_catcher ()
 * @deftypemethodx jit_function jit_value insn_start_catcher ()
 * @deftypemethodx jit_function void insn_branch_if_pc_not_in_range (const jit_label& @var{start_label}, const jit_label& @var{end_label}, jit_label& @var{label})
 * @deftypemethodx jit_function void insn_rethrow_unhandled ()
 * @deftypemethodx jit_function void insn_start_finally (jit_label& @var{label})
 * @deftypemethodx jit_function void insn_return_from_finally ()
 * @deftypemethodx jit_function void insn_call_finally (jit_label& @var{label})
 * @deftypemethodx jit_function jit_value insn_start_filter (jit_label& @var{label}, jit_type_t @var{type})
 * @deftypemethodx jit_function void insn_return_from_filter (const jit_value& @var{value})
 * @deftypemethodx jit_function jit_value insn_call_filter (jit_label& @var{label}, const jit_value& @var{value}, jit_type_t @var{type})
 * @deftypemethodx jit_function void insn_memcpy (const jit_value& @var{dest}, const jit_value& @var{src}, const jit_value& @var{size})
 * @deftypemethodx jit_function void insn_memmove (const jit_value& @var{dest}, const jit_value& @var{src}, const jit_value& @var{size})
 * @deftypemethodx jit_function void jit_insn_memset (const jit_value& @var{dest}, const jit_value& @var{value}, const jit_value& @var{size})
 * @deftypemethodx jit_function jit_value jit_insn_alloca (const jit_value& @var{size})
 * @deftypemethodx jit_function void insn_move_blocks_to_end (const jit_label& @var{from_label}, const jit_label& @var{to_label})
 * @deftypemethodx jit_function void insn_move_blocks_to_start (const jit_label& @var{from_label}, const jit_label& @var{to_label})
 * @deftypemethodx jit_function void insn_mark_offset (jit_int @var{offset})
 * @deftypemethodx jit_function void insn_mark_breakpoint (jit_nint @var{data1}, jit_nint @var{data2})
 * Create instructions of various kinds.  @xref{Instructions}, for more
 * information on the individual instructions and their arguments.
 * @end deftypemethod
@*/

void jit_function::insn_label(jit_label& label)
{
	if(!jit_insn_label(func, label.rawp()))
	{
		out_of_memory();
	}
}

void jit_function::insn_new_block()
{
	if(!jit_insn_new_block(func))
	{
		out_of_memory();
	}
}

jit_value jit_function::insn_load(const jit_value& value)
{
	value_wrap(jit_insn_load(func, value.raw()));
}

jit_value jit_function::insn_dup(const jit_value& value)
{
	value_wrap(jit_insn_dup(func, value.raw()));
}

jit_value jit_function::insn_load_small(const jit_value& value)
{
	value_wrap(jit_insn_load_small(func, value.raw()));
}

void jit_function::store(const jit_value& dest, const jit_value& value)
{
	if(!jit_insn_store(func, dest.raw(), value.raw()))
	{
		out_of_memory();
	}
}

jit_value jit_function::insn_load_relative
	(const jit_value& value, jit_nint offset, jit_type_t type)
{
	value_wrap(jit_insn_load_relative(func, value.raw(), offset, type));
}

void jit_function::insn_store_relative
	(const jit_value& dest, jit_nint offset, const jit_value& value)
{
	if(!jit_insn_store_relative(func, dest.raw(), offset, value.raw()))
	{
		out_of_memory();
	}
}

jit_value jit_function::insn_add_relative
	(const jit_value& value, jit_nint offset)
{
	value_wrap(jit_insn_add_relative(func, value.raw(), offset));
}

jit_value jit_function::insn_load_elem
	(const jit_value& base_addr, const jit_value& index,
	 jit_type_t elem_type)
{
	value_wrap(jit_insn_load_elem
		(func, base_addr.raw(), index.raw(), elem_type));
}

jit_value jit_function::insn_load_elem_address
	(const jit_value& base_addr, const jit_value& index,
	 jit_type_t elem_type)
{
	value_wrap(jit_insn_load_elem_address
		(func, base_addr.raw(), index.raw(), elem_type));
}

void jit_function::insn_store_elem
	(const jit_value& base_addr, const jit_value& index,
	 const jit_value& value)
{
	if(!jit_insn_store_elem(func, base_addr.raw(), index.raw(), value.raw()))
	{
		out_of_memory();
	}
}

void jit_function::insn_check_null(const jit_value& value)
{
	if(!jit_insn_check_null(func, value.raw()))
	{
		out_of_memory();
	}
}

jit_value jit_function::insn_add
	(const jit_value& value1, const jit_value& value2)
{
	value_wrap(jit_insn_add(func, value1.raw(), value2.raw()));
}

jit_value jit_function::insn_add_ovf
	(const jit_value& value1, const jit_value& value2)
{
	value_wrap(jit_insn_add_ovf(func, value1.raw(), value2.raw()));
}

jit_value jit_function::insn_sub
	(const jit_value& value1, const jit_value& value2)
{
	value_wrap(jit_insn_sub(func, value1.raw(), value2.raw()));
}

jit_value jit_function::insn_sub_ovf
	(const jit_value& value1, const jit_value& value2)
{
	value_wrap(jit_insn_sub_ovf(func, value1.raw(), value2.raw()));
}

jit_value jit_function::insn_mul
	(const jit_value& value1, const jit_value& value2)
{
	value_wrap(jit_insn_mul(func, value1.raw(), value2.raw()));
}

jit_value jit_function::insn_mul_ovf
	(const jit_value& value1, const jit_value& value2)
{
	value_wrap(jit_insn_mul_ovf(func, value1.raw(), value2.raw()));
}

jit_value jit_function::insn_div
	(const jit_value& value1, const jit_value& value2)
{
	value_wrap(jit_insn_div(func, value1.raw(), value2.raw()));
}

jit_value jit_function::insn_rem
	(const jit_value& value1, const jit_value& value2)
{
	value_wrap(jit_insn_rem(func, value1.raw(), value2.raw()));
}

jit_value jit_function::insn_rem_ieee
	(const jit_value& value1, const jit_value& value2)
{
	value_wrap(jit_insn_rem_ieee(func, value1.raw(), value2.raw()));
}

jit_value jit_function::insn_neg(const jit_value& value1)
{
	value_wrap(jit_insn_neg(func, value1.raw()));
}

jit_value jit_function::insn_and
	(const jit_value& value1, const jit_value& value2)
{
	value_wrap(jit_insn_and(func, value1.raw(), value2.raw()));
}

jit_value jit_function::insn_or
	(const jit_value& value1, const jit_value& value2)
{
	value_wrap(jit_insn_or(func, value1.raw(), value2.raw()));
}

jit_value jit_function::insn_xor
	(const jit_value& value1, const jit_value& value2)
{
	value_wrap(jit_insn_xor(func, value1.raw(), value2.raw()));
}

jit_value jit_function::insn_not(const jit_value& value1)
{
	value_wrap(jit_insn_not(func, value1.raw()));
}

jit_value jit_function::insn_shl
	(const jit_value& value1, const jit_value& value2)
{
	value_wrap(jit_insn_shl(func, value1.raw(), value2.raw()));
}

jit_value jit_function::insn_shr
	(const jit_value& value1, const jit_value& value2)
{
	value_wrap(jit_insn_shr(func, value1.raw(), value2.raw()));
}

jit_value jit_function::insn_ushr
	(const jit_value& value1, const jit_value& value2)
{
	value_wrap(jit_insn_ushr(func, value1.raw(), value2.raw()));
}

jit_value jit_function::insn_sshr
	(const jit_value& value1, const jit_value& value2)
{
	value_wrap(jit_insn_sshr(func, value1.raw(), value2.raw()));
}

jit_value jit_function::insn_eq
	(const jit_value& value1, const jit_value& value2)
{
	value_wrap(jit_insn_eq(func, value1.raw(), value2.raw()));
}

jit_value jit_function::insn_ne
	(const jit_value& value1, const jit_value& value2)
{
	value_wrap(jit_insn_ne(func, value1.raw(), value2.raw()));
}

jit_value jit_function::insn_lt
	(const jit_value& value1, const jit_value& value2)
{
	value_wrap(jit_insn_lt(func, value1.raw(), value2.raw()));
}

jit_value jit_function::insn_le
	(const jit_value& value1, const jit_value& value2)
{
	value_wrap(jit_insn_le(func, value1.raw(), value2.raw()));
}

jit_value jit_function::insn_gt
	(const jit_value& value1, const jit_value& value2)
{
	value_wrap(jit_insn_gt(func, value1.raw(), value2.raw()));
}

jit_value jit_function::insn_ge
	(const jit_value& value1, const jit_value& value2)
{
	value_wrap(jit_insn_ge(func, value1.raw(), value2.raw()));
}

jit_value jit_function::insn_cmpl
	(const jit_value& value1, const jit_value& value2)
{
	value_wrap(jit_insn_cmpl(func, value1.raw(), value2.raw()));
}

jit_value jit_function::insn_cmpg
	(const jit_value& value1, const jit_value& value2)
{
	value_wrap(jit_insn_cmpg(func, value1.raw(), value2.raw()));
}

jit_value jit_function::insn_to_bool(const jit_value& value1)
{
	value_wrap(jit_insn_to_bool(func, value1.raw()));
}

jit_value jit_function::insn_to_not_bool(const jit_value& value1)
{
	value_wrap(jit_insn_to_not_bool(func, value1.raw()));
}

jit_value jit_function::insn_acos(const jit_value& value1)
{
	value_wrap(jit_insn_acos(func, value1.raw()));
}

jit_value jit_function::insn_asin(const jit_value& value1)
{
	value_wrap(jit_insn_asin(func, value1.raw()));
}

jit_value jit_function::insn_atan(const jit_value& value1)
{
	value_wrap(jit_insn_atan(func, value1.raw()));
}

jit_value jit_function::insn_atan2
	(const jit_value& value1, const jit_value& value2)
{
	value_wrap(jit_insn_atan2(func, value1.raw(), value2.raw()));
}

jit_value jit_function::insn_ceil(const jit_value& value1)
{
	value_wrap(jit_insn_ceil(func, value1.raw()));
}

jit_value jit_function::insn_cos(const jit_value& value1)
{
	value_wrap(jit_insn_cos(func, value1.raw()));
}

jit_value jit_function::insn_cosh(const jit_value& value1)
{
	value_wrap(jit_insn_cosh(func, value1.raw()));
}

jit_value jit_function::insn_exp(const jit_value& value1)
{
	value_wrap(jit_insn_exp(func, value1.raw()));
}

jit_value jit_function::insn_floor(const jit_value& value1)
{
	value_wrap(jit_insn_floor(func, value1.raw()));
}

jit_value jit_function::insn_log(const jit_value& value1)
{
	value_wrap(jit_insn_log(func, value1.raw()));
}

jit_value jit_function::insn_log10(const jit_value& value1)
{
	value_wrap(jit_insn_log10(func, value1.raw()));
}

jit_value jit_function::insn_pow
	(const jit_value& value1, const jit_value& value2)
{
	value_wrap(jit_insn_pow(func, value1.raw(), value2.raw()));
}

jit_value jit_function::insn_rint(const jit_value& value1)
{
	value_wrap(jit_insn_rint(func, value1.raw()));
}

jit_value jit_function::insn_round(const jit_value& value1)
{
	value_wrap(jit_insn_round(func, value1.raw()));
}

jit_value jit_function::insn_sin(const jit_value& value1)
{
	value_wrap(jit_insn_sin(func, value1.raw()));
}

jit_value jit_function::insn_sinh(const jit_value& value1)
{
	value_wrap(jit_insn_sinh(func, value1.raw()));
}

jit_value jit_function::insn_sqrt(const jit_value& value1)
{
	value_wrap(jit_insn_sqrt(func, value1.raw()));
}

jit_value jit_function::insn_tan(const jit_value& value1)
{
	value_wrap(jit_insn_tan(func, value1.raw()));
}

jit_value jit_function::insn_tanh(const jit_value& value1)
{
	value_wrap(jit_insn_tanh(func, value1.raw()));
}

jit_value jit_function::insn_trunc(const jit_value& value1)
{
	value_wrap(jit_insn_trunc(func, value1.raw()));
}

jit_value jit_function::insn_is_nan(const jit_value& value1)
{
	value_wrap(jit_insn_is_nan(func, value1.raw()));
}

jit_value jit_function::insn_is_finite(const jit_value& value1)
{
	value_wrap(jit_insn_is_finite(func, value1.raw()));
}

jit_value jit_function::insn_is_inf(const jit_value& value1)
{
	value_wrap(jit_insn_is_inf(func, value1.raw()));
}

jit_value jit_function::insn_abs(const jit_value& value1)
{
	value_wrap(jit_insn_abs(func, value1.raw()));
}

jit_value jit_function::insn_min
	(const jit_value& value1, const jit_value& value2)
{
	value_wrap(jit_insn_min(func, value1.raw(), value2.raw()));
}

jit_value jit_function::insn_max
	(const jit_value& value1, const jit_value& value2)
{
	value_wrap(jit_insn_max(func, value1.raw(), value2.raw()));
}

jit_value jit_function::insn_sign(const jit_value& value1)
{
	value_wrap(jit_insn_sign(func, value1.raw()));
}

void jit_function::insn_branch(jit_label& label)
{
	if(!jit_insn_branch(func, label.rawp()))
	{
		out_of_memory();
	}
}

void jit_function::insn_branch_if(const jit_value& value, jit_label& label)
{
	if(!jit_insn_branch_if(func, value.raw(), label.rawp()))
	{
		out_of_memory();
	}
}

void jit_function::insn_branch_if_not(const jit_value& value, jit_label& label)
{
	if(!jit_insn_branch_if_not(func, value.raw(), label.rawp()))
	{
		out_of_memory();
	}
}

void jit_function::insn_jump_table(const jit_value& value, jit_jump_table& jump_table)
{
	if(!jit_insn_jump_table(func, value.raw(), jump_table.raw(), jump_table.size()))
	{
		out_of_memory();
	}
}

jit_value jit_function::insn_address_of(const jit_value& value1)
{
	value_wrap(jit_insn_address_of(func, value1.raw()));
}

jit_value jit_function::insn_address_of_label(jit_label& label)
{
	value_wrap(jit_insn_address_of_label(func, label.rawp()));
}

jit_value jit_function::insn_convert
	(const jit_value& value, jit_type_t type, int overflow_check)
{
	value_wrap(jit_insn_convert(func, value.raw(), type, overflow_check));
}

jit_value jit_function::insn_call
	(const char *name, jit_function_t jit_func,
	 jit_type_t signature, jit_value_t *args, unsigned int num_args,
	 int flags)
{
	value_wrap(jit_insn_call
		(func, name, jit_func, signature, args, num_args, flags));
}

jit_value jit_function::insn_call_indirect
	(const jit_value& value, jit_type_t signature,
 	 jit_value_t *args, unsigned int num_args, int flags)
{
	value_wrap(jit_insn_call_indirect
		(func, value.raw(), signature, args, num_args, flags));
}

jit_value jit_function::insn_call_indirect_vtable
	(const jit_value& value, jit_type_t signature,
 	 jit_value_t *args, unsigned int num_args, int flags)
{
	value_wrap(jit_insn_call_indirect_vtable
		(func, value.raw(), signature, args, num_args, flags));
}

jit_value jit_function::insn_call_native
	(const char *name, void *native_func, jit_type_t signature,
     jit_value_t *args, unsigned int num_args, int flags)
{
	value_wrap(jit_insn_call_native
		(func, name, native_func, signature, args, num_args, flags));
}

jit_value jit_function::insn_call_intrinsic
	(const char *name, void *intrinsic_func,
	 const jit_intrinsic_descr_t& descriptor,
	 const jit_value& arg1, const jit_value& arg2)
{
	value_wrap(jit_insn_call_intrinsic
		(func, name, intrinsic_func, &descriptor, arg1.raw(), arg2.raw()));
}

void jit_function::insn_incoming_reg(const jit_value& value, int reg)
{
	if(!jit_insn_incoming_reg(func, value.raw(), reg))
	{
		out_of_memory();
	}
}

void jit_function::insn_incoming_frame_posn
	(const jit_value& value, jit_nint posn)
{
	if(!jit_insn_incoming_frame_posn(func, value.raw(), posn))
	{
		out_of_memory();
	}
}

void jit_function::insn_outgoing_reg(const jit_value& value, int reg)
{
	if(!jit_insn_outgoing_reg(func, value.raw(), reg))
	{
		out_of_memory();
	}
}

void jit_function::insn_outgoing_frame_posn
	(const jit_value& value, jit_nint posn)
{
	if(!jit_insn_outgoing_frame_posn(func, value.raw(), posn))
	{
		out_of_memory();
	}
}

void jit_function::insn_return_reg(const jit_value& value, int reg)
{
	if(!jit_insn_return_reg(func, value.raw(), reg))
	{
		out_of_memory();
	}
}

void jit_function::insn_setup_for_nested(int nested_level, int reg)
{
}

void jit_function::insn_flush_struct(const jit_value& value)
{
	if(!jit_insn_flush_struct(func, value.raw()))
	{
		out_of_memory();
	}
}

jit_value jit_function::insn_import(jit_value value)
{
	value_wrap(jit_insn_import(func, value.raw()));
}

void jit_function::insn_push(const jit_value& value)
{
	if(!jit_insn_push(func, value.raw()))
	{
		out_of_memory();
	}
}

void jit_function::insn_push_ptr(const jit_value& value, jit_type_t type)
{
	if(!jit_insn_push_ptr(func, value.raw(), type))
	{
		out_of_memory();
	}
}

void jit_function::insn_set_param(const jit_value& value, jit_nint offset)
{
	if(!jit_insn_set_param(func, value.raw(), offset))
	{
		out_of_memory();
	}
}

void jit_function::insn_set_param_ptr
	(const jit_value& value, jit_type_t type, jit_nint offset)
{
	if(!jit_insn_set_param_ptr(func, value.raw(), type, offset))
	{
		out_of_memory();
	}
}

void jit_function::insn_push_return_area_ptr()
{
	if(!jit_insn_push_return_area_ptr(func))
	{
		out_of_memory();
	}
}

void jit_function::insn_return(const jit_value& value)
{
	if(!jit_insn_return(func, value.raw()))
	{
		out_of_memory();
	}
}

void jit_function::insn_return()
{
	if(!jit_insn_return(func, 0))
	{
		out_of_memory();
	}
}

void jit_function::insn_return_ptr(const jit_value& value, jit_type_t type)
{
	if(!jit_insn_return_ptr(func, value.raw(), type))
	{
		out_of_memory();
	}
}

void jit_function::insn_default_return()
{
	if(!jit_insn_default_return(func))
	{
		out_of_memory();
	}
}

void jit_function::insn_throw(const jit_value& value)
{
	if(!jit_insn_throw(func, value.raw()))
	{
		out_of_memory();
	}
}

jit_value jit_function::insn_get_call_stack()
{
	value_wrap(jit_insn_get_call_stack(func));
}

jit_value jit_function::insn_thrown_exception()
{
	value_wrap(jit_insn_thrown_exception(func));
}

void jit_function::insn_uses_catcher()
{
	if(!jit_insn_uses_catcher(func))
	{
		out_of_memory();
	}
}

jit_value jit_function::insn_start_catcher()
{
	value_wrap(jit_insn_start_catcher(func));
}

void jit_function::insn_branch_if_pc_not_in_range
	(const jit_label& start_label, const jit_label& end_label,
	 jit_label& label)
{
	if(!jit_insn_branch_if_pc_not_in_range
			(func, start_label.raw(), end_label.raw(), label.rawp()))
	{
		out_of_memory();
	}
}

void jit_function::insn_rethrow_unhandled()
{
	if(!jit_insn_rethrow_unhandled(func))
	{
		out_of_memory();
	}
}

void jit_function::insn_start_finally(jit_label& label)
{
	if(!jit_insn_start_finally(func, label.rawp()))
	{
		out_of_memory();
	}
}

void jit_function::insn_return_from_finally()
{
	if(!jit_insn_return_from_finally(func))
	{
		out_of_memory();
	}
}

void jit_function::insn_call_finally(jit_label& label)
{
	if(!jit_insn_call_finally(func, label.rawp()))
	{
		out_of_memory();
	}
}

jit_value jit_function::insn_start_filter(jit_label& label, jit_type_t type)
{
	value_wrap(jit_insn_start_filter(func, label.rawp(), type));
}

void jit_function::insn_return_from_filter(const jit_value& value)
{
	if(!jit_insn_return_from_filter(func, value.raw()))
	{
		out_of_memory();
	}
}

jit_value jit_function::insn_call_filter
	(jit_label& label, const jit_value& value, jit_type_t type)
{
	value_wrap(jit_insn_call_filter(func, label.rawp(), value.raw(), type));
}

void jit_function::insn_memcpy
	(const jit_value& dest, const jit_value& src, const jit_value& size)
{
	if(!jit_insn_memcpy(func, dest.raw(), src.raw(), size.raw()))
	{
		out_of_memory();
	}
}

void jit_function::insn_memmove
	(const jit_value& dest, const jit_value& src, const jit_value& size)
{
	if(!jit_insn_memmove(func, dest.raw(), src.raw(), size.raw()))
	{
		out_of_memory();
	}
}

void jit_function::insn_memset
	(const jit_value& dest, const jit_value& value, const jit_value& size)
{
	if(!jit_insn_memset(func, dest.raw(), value.raw(), size.raw()))
	{
		out_of_memory();
	}
}

jit_value jit_function::insn_alloca(const jit_value& size)
{
	value_wrap(jit_insn_alloca(func, size.raw()));
}

void jit_function::insn_move_blocks_to_end
	(const jit_label& from_label, const jit_label& to_label)
{
	if(!jit_insn_move_blocks_to_end(func, from_label.raw(), to_label.raw()))
	{
		out_of_memory();
	}
}

void jit_function::insn_move_blocks_to_start
	(const jit_label& from_label, const jit_label& to_label)
{
	if(!jit_insn_move_blocks_to_start(func, from_label.raw(), to_label.raw()))
	{
		out_of_memory();
	}
}

void jit_function::insn_mark_offset(jit_int offset)
{
	if(!jit_insn_mark_offset(func, offset))
	{
		out_of_memory();
	}
}

void jit_function::insn_mark_breakpoint(jit_nint data1, jit_nint data2)
{
	if(!jit_insn_mark_breakpoint(func, data1, data2))
	{
		out_of_memory();
	}
}

void jit_function::register_on_demand()
{
	jit_function_set_on_demand_compiler(func, on_demand_compiler);
}

int jit_function::on_demand_compiler(jit_function_t func)
{
	jit_function *func_object;

	// Get the object that corresponds to the raw function.
	func_object = from_raw(func);
	if(!func_object)
	{
		return JIT_RESULT_COMPILE_ERROR;
	}

	// Attempt to build the function's contents.
	try
	{
		func_object->build();
		if(!jit_insn_default_return(func))
		{
			func_object->out_of_memory();
		}
		return JIT_RESULT_OK;
	}
	catch(jit_build_exception e)
	{
		return e.result;
	}
}

void jit_function::free_mapping(void *data)
{
	// If we were called during the context's shutdown,
	// then the raw function pointer is no longer valid.
	((jit_function *)data)->func = 0;
}
