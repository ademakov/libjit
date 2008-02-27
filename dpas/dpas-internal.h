/*
 * dpas-internal.h - Internal definitions for the Dynamic Pascal compiler.
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

#ifndef	_DPAS_INTERNAL_H
#define	_DPAS_INTERNAL_H

#include <jit/jit.h>
#include <stdio.h>

#include "dpas-scope.h"
#include "dpas-types.h"
#include "dpas-semantics.h"

#ifdef	__cplusplus
extern	"C" {
#endif

/*
 * Current filename and line number.
 */
extern char *dpas_filename;
extern long dpas_linenum;

/*
 * Flag that indicates that functions should be dumped as they are compiled.
 */
extern int dpas_dump_functions;

/*
 * Information about a parameter list (also used for record fields).
 */
typedef struct
{
	char		  **names;
	jit_type_t	   *types;
	int				len;
	jit_abi_t		abi;

} dpas_params;

/*
 * Flag that is set when an error is encountered.
 */
extern int dpas_error_reported;

/*
 * Function that is called when the system runs out of memory.
 */
void dpas_out_of_memory(void);

/*
 * Process an "import" clause within a program.
 */
void dpas_import(const char *name);

/*
 * Load the contents of a source file.
 */
void dpas_load_file(char *filename, FILE *file);

/*
 * Report an error on the current line.
 */
void dpas_error(const char *format, ...);

/*
 * Report a warning on the current line.
 */
void dpas_warning(const char *format, ...);

/*
 * Report an error on a specific line.
 */
void dpas_error_on_line(const char *filename, long linenum,
                        const char *format, ...);

/*
 * Get the JIT context that we are using to compile functions.
 */
jit_context_t dpas_current_context(void);

/*
 * Get the current function that is being compiled.  Returns NULL
 * if we are currently at the global level.
 */
jit_function_t dpas_current_function(void);

/*
 * Create a new function and push it onto the context stack.
 * The function is initialized to read parameters that are
 * compatible with the supplied signature.
 */
jit_function_t dpas_new_function(jit_type_t signature);

/*
 * Pop out of the current function.
 */
void dpas_pop_function(void);

/*
 * Determine if the current function is nested.
 */
int dpas_function_is_nested(void);

/*
 * Determine if a name corresponds to a builtin function,
 * and get its builtin identifier.  Returns zero if not a builtin.
 */
int dpas_is_builtin(const char *name);

/*
 * Expand a builtin function reference.
 */
dpas_semvalue dpas_expand_builtin
	(int identifier, dpas_semvalue *args, int num_args);

/*
 * Add the current function to the execution list.  Used for
 * "main" functions within each compiled module.
 */
void dpas_add_main_function(jit_function_t func);

/*
 * Run the "main" functions in the order in which they were compiled.
 * Returns zero on an exception.
 */
int dpas_run_main_functions(void);

#ifdef	__cplusplus
};
#endif

#endif	/* _DPAS_INTERNAL_H */
