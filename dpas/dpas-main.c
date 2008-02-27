/*
 * dpas-main.c - Main entry point for the Dyanmic Pascal Compiler.
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
#include <config.h>
#include <stdlib.h>

/*
 * Command-line options.
 */
static char *progname = 0;
static char *filename = 0;
static char **include_dirs = 0;
static int num_include_dirs = 0;
static char **using_seen = 0;
static int num_using_seen = 0;
static int dont_fold = 0;

/*
 * Forward declarations.
 */
static void version(void);
static void usage(void);
static void add_include_dir(char *dir);
static void initialize(void);

int main(int argc, char **argv)
{
	FILE *file;

	/* Parse the command-line options */
	progname = argv[0];
	while(argc > 1 && argv[1][0] == '-' && argv[1][1] != '\0')
	{
		if(!jit_strncmp(argv[1], "-I", 2))
		{
			if(argv[1][2] == '\0')
			{
				++argv;
				--argc;
				if(argc > 1)
				{
					add_include_dir(argv[1]);
				}
				else
				{
					usage();
				}
			}
			else
			{
				add_include_dir(argv[1] + 2);
			}
		}
		else if(!jit_strcmp(argv[1], "-v") ||
		        !jit_strcmp(argv[1], "--version"))
		{
			version();
		}
		else if(!jit_strcmp(argv[1], "-d"))
		{
			dpas_dump_functions = 1;
		}
		else if(!jit_strcmp(argv[1], "-D"))
		{
			dpas_dump_functions = 2;
		}
		else if(!jit_strcmp(argv[1], "--dont-fold"))
		{
			dont_fold = 1;
		}
		else
		{
			usage();
		}
		++argv;
		--argc;
	}
	if(argc <= 1)
	{
		usage();
	}
	filename = argv[1];
	++argv;
	--argc;

	/* Add the system-wide include directories to the list */
#ifdef DPAS_INCLUDE_DIR
	add_include_dir(DPAS_INCLUDE_DIR);
#endif
	add_include_dir("/usr/local/share/dpas");
	add_include_dir("/usr/share/dpas");

	/* Initialize the pre-defined types, constants, procedures, etc */
	initialize();

	/* Load the specified program file */
	if(!jit_strcmp(filename, "-"))
	{
		dpas_load_file("(stdin)", stdin);
	}
	else if((file = fopen(filename, "r")) != NULL)
	{
		dpas_load_file(filename, file);
		fclose(file);
	}
	else
	{
		perror(filename);
		return 1;
	}

	/* Bail out if we had errors during the compilation phase */
	if(dpas_error_reported)
	{
		return 1;
	}

	/* Execute the program that we just compiled */
	if(!dpas_dump_functions)
	{
		if(!dpas_run_main_functions())
		{
			return 1;
		}
	}

	/* Done */
	return 0;
}

static void version(void)
{
	printf("Dynamic Pascal Version " VERSION "\n");
	printf("Copyright (c) 2004 Southern Storm Software, Pty Ltd.\n");
	exit(0);
}

static void usage(void)
{
	printf("Dynamic Pascal Version " VERSION "\n");
	printf("Copyright (c) 2004 Southern Storm Software, Pty Ltd.\n");
	printf("\n");
	printf("Usage: %s [-Idir] file.pas [args]\n", progname);
	exit(1);
}

static void add_include_dir(char *dir)
{
	include_dirs = (char **)jit_realloc
		(include_dirs, (num_include_dirs + 1) * sizeof(char *));
	if(!include_dirs)
	{
		dpas_out_of_memory();
	}
	include_dirs[num_include_dirs++] = dir;
}

void dpas_out_of_memory(void)
{
	fputs(progname, stderr);
	fputs(": virtual memory exhausted\n", stderr);
	exit(1);
}

void dpas_import(const char *name)
{
	int posn;
	char *pathname;
	FILE *file;

	/* Bail out if we've already included this name before */
	for(posn = 0; posn < num_using_seen; ++posn)
	{
		if(!jit_strcmp(using_seen[posn], name))
		{
			return;
		}
	}

	/* Add the name to the "seen" list */
	using_seen = (char **)jit_realloc
		(using_seen, (num_using_seen + 1) * sizeof(char *));
	if(!using_seen)
	{
		dpas_out_of_memory();
	}
	using_seen[num_using_seen] = jit_strdup(name);
	if(!(using_seen[num_using_seen]))
	{
		dpas_out_of_memory();
	}
	++num_using_seen;

	/* Look in the same directory as the including source file first */
	posn = jit_strlen(dpas_filename);
	while(posn > 0 && dpas_filename[posn - 1] != '/' &&
	      dpas_filename[posn - 1] != '\\')
	{
		--posn;
	}
	if(posn > 0)
	{
		pathname = (char *)jit_malloc(posn + jit_strlen(name) + 5);
		if(!pathname)
		{
			dpas_out_of_memory();
		}
		jit_strncpy(pathname, dpas_filename, posn);
		jit_strcpy(pathname + posn, name);
		jit_strcat(pathname, ".pas");
	}
	else
	{
		pathname = (char *)jit_malloc(jit_strlen(name) + 5);
		if(!pathname)
		{
			dpas_out_of_memory();
		}
		jit_strcpy(pathname, name);
		jit_strcat(pathname, ".pas");
	}
	if((file = fopen(pathname, "r")) != NULL)
	{
		dpas_load_file(pathname, file);
		fclose(file);
		jit_free(pathname);
		return;
	}
	jit_free(pathname);

	/* Scan the include directories looking for the name */
	for(posn = 0; posn < num_include_dirs; ++posn)
	{
		pathname = (char *)jit_malloc(jit_strlen(include_dirs[posn]) +
									  jit_strlen(name) + 6);
		if(!pathname)
		{
			dpas_out_of_memory();
		}
		jit_strcpy(pathname, include_dirs[posn]);
		jit_strcat(pathname, "/");
		jit_strcat(pathname, name);
		jit_strcat(pathname, ".pas");
		if((file = fopen(pathname, "r")) != NULL)
		{
			dpas_load_file(pathname, file);
			fclose(file);
			jit_free(pathname);
			return;
		}
		jit_free(pathname);
	}

	/* If we get here, then we could not find the specified module */
	fprintf(stderr, "%s:%ld: could not locate the module `%s'\n",
			dpas_filename, dpas_linenum, name);
	dpas_error_reported = 1;
}

/*
 * Initialize the system.
 */
static void initialize(void)
{
	jit_init();
	dpas_init_types();
	if(dont_fold)
	{
		jit_context_set_meta_numeric
			(dpas_current_context(), JIT_OPTION_DONT_FOLD, 1);
	}
}
