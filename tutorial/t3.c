/*

Tutorial 3 - compiling on-demand

Builds and compiles the following function:

int mul_add(int x, int y, int z)
{
	return x * y + z;
}

Differs from Tutorial 1 in that this version only builds the function
when it is called, not at startup time.

*/

#include <stdio.h>
#include <jit/jit.h>

int compile_mul_add(jit_function_t function)
{
	jit_value_t x, y, z;
	jit_value_t temp1, temp2;

	printf("Compiling mul_add on demand\n");

	x = jit_value_get_param(function, 0);
	y = jit_value_get_param(function, 1);
	z = jit_value_get_param(function, 2);

	temp1 = jit_insn_mul(function, x, y);
	temp2 = jit_insn_add(function, temp1, z);

	jit_insn_return(function, temp2);
	return 1;
}

int main(int argc, char **argv)
{
	jit_context_t context;
	jit_type_t params[3];
	jit_type_t signature;
	jit_function_t function;
	jit_int arg1, arg2, arg3;
	void *args[3];
	jit_int result;

	/* Create a context to hold the JIT's primary state */
	context = jit_context_create();

	/* Lock the context while we construct the function */
	jit_context_build_start(context);

	/* Build the function signature */
	params[0] = jit_type_int;
	params[1] = jit_type_int;
	params[2] = jit_type_int;
	signature = jit_type_create_signature
		(jit_abi_cdecl, jit_type_int, params, 3, 1);

	/* Create the function object */
	function = jit_function_create(context, signature);
	jit_type_free(signature);

	/* Make the function recompilable */
	jit_function_set_recompilable(function);

	/* Set the on-demand compiler for "mul_add" */
	jit_function_set_on_demand_compiler(function, compile_mul_add);

	/* Unlock the context.  It will be automatically locked for
	   us when the on-demand compiler is called */
	jit_context_build_end(context);

	/* Execute the function and print the result.  This will arrange
	   to call the on-demand compiler to build the function's body */
	arg1 = 3;
	arg2 = 5;
	arg3 = 2;
	args[0] = &arg1;
	args[1] = &arg2;
	args[2] = &arg3;
	jit_function_apply(function, args, &result);
	printf("mul_add(3, 5, 2) = %d\n", (int)result);

	/* Execute the function again, to demonstrate that the
	   on-demand compiler is not invoked a second time */
	arg1 = 13;
	arg2 = 5;
	arg3 = 7;
	args[0] = &arg1;
	args[1] = &arg2;
	args[2] = &arg3;
	jit_function_apply(function, args, &result);
	printf("mul_add(13, 5, 7) = %d\n", (int)result);

	/* Force the function to be recompiled.  Normally we'd use another
	   on-demand compiler with greater optimization capabilities */
	jit_context_build_start(context);
	jit_function_get_on_demand_compiler(function)(function);
	jit_function_compile(function);
	jit_context_build_end(context);

	/* Execute the function a third time, after it is recompiled */
	arg1 = 2;
	arg2 = 18;
	arg3 = -3;
	args[0] = &arg1;
	args[1] = &arg2;
	args[2] = &arg3;
	jit_function_apply(function, args, &result);
	printf("mul_add(2, 18, -3) = %d\n", (int)result);

	/* Clean up */
	jit_context_destroy(context);

	/* Finished */
	return 0;
}
