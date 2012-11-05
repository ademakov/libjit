/*

Tutorial 1 - mul_add

Builds and compiles the following function:

int mul_add(int x, int y, int z)
{
	return x * y + z;
}

*/

#include <stdio.h>
#include <jit/jit.h>

int main(int argc, char **argv)
{
	jit_context_t context;
	jit_type_t params[3];
	jit_type_t signature;
	jit_function_t function;
	jit_value_t x, y, z;
	jit_value_t temp1, temp2;
	jit_int arg1, arg2, arg3;
	void *args[3];
	jit_int result;

	/* Create a context to hold the JIT's primary state */
	context = jit_context_create();

	/* Lock the context while we build and compile the function */
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

	/* Construct the function body */
	x = jit_value_get_param(function, 0);
	y = jit_value_get_param(function, 1);
	z = jit_value_get_param(function, 2);
	temp1 = jit_insn_mul(function, x, y);
	temp2 = jit_insn_add(function, temp1, z);
	jit_insn_return(function, temp2);

	/* Compile the function */
	jit_function_compile(function);

	/* Unlock the context */
	jit_context_build_end(context);

	/* Execute the function and print the result */
	arg1 = 3;
	arg2 = 5;
	arg3 = 2;
	args[0] = &arg1;
	args[1] = &arg2;
	args[2] = &arg3;
	jit_function_apply(function, args, &result);
	printf("mul_add(3, 5, 2) = %d\n", (int)result);

	/* Clean up */
	jit_context_destroy(context);

	/* Finished */
	return 0;
}
