/*
 * jit-dynlib.c - Dynamic library support routines.
 *
 * Copyright (C) 2001-2004  Southern Storm Software, Pty Ltd.
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

#include <jit/jit-dynamic.h>
#include <jit/jit-util.h>
#include <stdio.h>
#include <config.h>
#ifdef JIT_WIN32_PLATFORM
	#include <windows.h>
	#ifndef JIT_WIN32_NATIVE
		#ifdef HAVE_SYS_CYGWIN_H
			#include <sys/cygwin.h>
		#endif
	#endif
#else
#ifdef HAVE_DLFCN_H
	#include <dlfcn.h>
#endif
#endif

/*@

@section Dynamic libraries
@cindex Dynamic libraries

The following routines are supplied to help load and inspect dynamic
libraries.  They should be used in place of the traditional
@code{dlopen}, @code{dlclose}, and @code{dlsym} functions, which
are not portable across operating systems.

You must include @code{<jit/jit-dynamic.h>} to use these routines,
and then link with @code{-ljitdynamic -ljit}.

@deftypefun jit_dynlib_handle_t jit_dynlib_open (const char *@var{name})
Opens the dynamic library called @var{name}, returning a handle for it.
@end deftypefun

@deftypefun void jit_dynlib_close (jit_dynlib_handle_t @var{handle})
Close a dynamic library.
@end deftypefun

@deftypefun {void *} jit_dynlib_get_symbol (jit_dynlib_handle_t @var{handle}, const char *@var{symbol})
Retrieve the symbol @var{symbol} from the specified dynamic library.
Returns NULL if the symbol could not be found.  This will try both
non-prefixed and underscore-prefixed forms of @var{symbol} on platforms
where it makes sense to do so, so there is no need for the caller
to perform prefixing.
@end deftypefun

@deftypefun void jit_dynlib_set_debug (int @var{flag})
Enable or disable additional debug messages to stderr.  Debugging is
disabled by default.  Normally the dynamic library routines will silently
report errors via NULL return values, leaving reporting up to the caller.
However, it can be useful to turn on additional diagnostics when tracking
down problems with dynamic loading.
@end deftypefun

@deftypefun {const char *} jit_dynlib_get_suffix (void)
Get the preferred dynamic library suffix for this platform.
Usually something like @code{so}, @code{dll}, or @code{dylib}.
@end deftypefun

@*/

#ifdef	__cplusplus
extern	"C" {
#endif

static int dynlib_debug = 0;

void jit_dynlib_set_debug(int flag)
{
	dynlib_debug = flag;
}

#if defined(__APPLE__) && defined(__MACH__)	/* MacOS X */

#include <mach-o/dyld.h>

jit_dynlib_handle_t jit_dynlib_open(const char *name)
{
	NSObjectFileImage file;
	NSObjectFileImageReturnCode result;
	NSModule module;
	void *image;
	const char *msg;

	/* Attempt to open the dylib file */
	result = NSCreateObjectFileImageFromFile(name, &file);
	if(result == NSObjectFileImageInappropriateFile)
	{
		/* May be an image, and not a bundle */
		image = (void *)NSAddImage(name, NSADDIMAGE_OPTION_RETURN_ON_ERROR);
		if(image)
		{
			return image;
		}
	}
	if(result != NSObjectFileImageSuccess)
	{
		switch(result)
		{
			case NSObjectFileImageFailure:
				msg = " (NSObjectFileImageFailure)"; break;
			case NSObjectFileImageInappropriateFile:
				msg = " (NSObjectFileImageInappropriateFile)"; break;
			case NSObjectFileImageArch:
				msg = " (NSObjectFileImageArch)"; break;
			case NSObjectFileImageFormat:
				msg = " (NSObjectFileImageFormat)"; break;
			case NSObjectFileImageAccess:
				msg = " (NSObjectFileImageAccess)"; break;
			default:
				msg = ""; break;
		}
		if(dynlib_debug)
		{
			fprintf(stderr, "%s: could not load dynamic library%s\n",
					name, msg);
		}
		return 0;
	}

	/* Link the module dependencies */
	module = NSLinkModule(file, name,
						  NSLINKMODULE_OPTION_BINDNOW |
						  NSLINKMODULE_OPTION_PRIVATE |
						  NSLINKMODULE_OPTION_RETURN_ON_ERROR);
	return (void *)module;
}

void jit_dynlib_close(jit_dynlib_handle_t handle)
{
	if((((struct mach_header *)handle)->magic == MH_MAGIC) ||
	   (((struct mach_header *)handle)->magic == MH_CIGAM))
	{
		/* Cannot remove dynamic images once they've been loaded */
		return;
	}
	NSUnLinkModule((NSModule)handle, NSUNLINKMODULE_OPTION_NONE);
}

static void *GetSymbol(jit_dynlib_handle_t handle, const char *symbol)
{
	NSSymbol sym;

	/* We have to use a different lookup approach for images and modules */
	if((((struct mach_header *)handle)->magic == MH_MAGIC) ||
	   (((struct mach_header *)handle)->magic == MH_CIGAM))
	{
		if(NSIsSymbolNameDefinedInImage((struct mach_header *)handle, symbol))
		{
			sym = NSLookupSymbolInImage((struct mach_header *)handle, symbol,
						NSLOOKUPSYMBOLINIMAGE_OPTION_BIND |
						NSLOOKUPSYMBOLINIMAGE_OPTION_RETURN_ON_ERROR);
		}
		else
		{
			sym = 0;
		}
	}
	else
	{
		sym = NSLookupSymbolInModule((NSModule)handle, symbol);
	}

	/* Did we find the symbol? */
	if(sym == 0)
	{
		return 0;
	}

	/* Convert the symbol into the address that we require */
	return (void *)NSAddressOfSymbol(sym);
}

void *jit_dynlib_get_symbol(jit_dynlib_handle_t handle, const char *symbol)
{
	void *value = GetSymbol(handle, (char *)symbol);
	char *newName;
	if(value)
	{
		return value;
	}
	newName = (char *)jit_malloc(jit_strlen(symbol) + 2);
	if(newName)
	{
		/* Try again with '_' prepended to the name */
		newName[0] = '_';
		jit_strcpy(newName + 1, symbol);
		value = GetSymbol(handle, newName);
		if(value)
		{
			jit_free(newName);
			return value;
		}
		jit_free(newName);
	}
	if(dynlib_debug)
	{
		fprintf(stderr, "%s: could not find the specified symbol\n", symbol);
	}
	return 0;
}

const char *jit_dynlib_get_suffix(void)
{
	return "dylib";
}

#elif defined(JIT_WIN32_PLATFORM)	/* Native Win32 or Cygwin */

jit_dynlib_handle_t jit_dynlib_open(const char *name)
{
	void *libHandle;
	char *newName = 0;

#if defined(JIT_WIN32_CYGWIN) && defined(HAVE_SYS_CYGWIN_H) && \
    defined(HAVE_CYGWIN_CONV_TO_WIN32_PATH)

	/* Use Cygwin to expand the path */
	{
		char buf[4096];
		if(cygwin_conv_to_win32_path(name, buf) == 0)
		{
			newName = jit_strdup(buf);
			if(!newName)
			{
				return 0;
			}
		}
	}

#endif

	/* Attempt to load the library */
	libHandle = (void *)LoadLibrary((newName ? newName : name));
	if(libHandle == 0)
	{
		if(dynlib_debug)
		{
			fprintf(stderr, "%s: could not load dynamic library\n",
					(newName ? newName : name));
		}
		if(newName)
		{
			jit_free(newName);
		}
		return 0;
	}
	if(newName)
	{
		jit_free(newName);
	}
	return libHandle;
}

void jit_dynlib_close(jit_dynlib_handle_t handle)
{
	FreeLibrary((HINSTANCE)handle);
}

void *jit_dynlib_get_symbol(jit_dynlib_handle_t handle, const char *symbol)
{
	void *procAddr;
	procAddr = (void *)GetProcAddress((HINSTANCE)handle, symbol);
	if(procAddr == 0)
	{
		if(dynlib_debug)
		{
			fprintf(stderr, "%s: could not resolve symbol", symbol);
		}
		return 0;
	}
	return procAddr;
}

const char *jit_dynlib_get_suffix(void)
{
	return "dll";
}

#elif defined(HAVE_DLFCN_H) && defined(HAVE_DLOPEN)

jit_dynlib_handle_t jit_dynlib_open(const char *name)
{
	jit_dynlib_handle_t handle;
	const char *error;
	handle = (jit_dynlib_handle_t)dlopen(name, RTLD_LAZY | RTLD_GLOBAL);
	if(!handle)
	{
		/* If the name does not start with "lib" and does not
		   contain a path, then prepend "lib" and try again */
		if(jit_strncmp(name, "lib", 3) != 0)
		{
			error = name;
			while(*error != '\0' && *error != '/' && *error != '\\')
			{
				++error;
			}
			if(*error == '\0')
			{
				/* Try adding "lib" to the start */
				char *temp = (char *)jit_malloc(jit_strlen(name) + 4);
				if(temp)
				{
					jit_strcpy(temp, "lib");
					jit_strcat(temp, name);
					handle = dlopen(temp, RTLD_LAZY | RTLD_GLOBAL);
					jit_free(temp);
					if(handle)
					{
						return handle;
					}
				}

				/* Reload the original error state */
				handle = dlopen(name, RTLD_LAZY | RTLD_GLOBAL);
			}
		}

		/* Report the error, or just clear the error state */
		if(dynlib_debug)
		{
			error = dlerror();
			fprintf(stderr, "%s: %s\n", name,
					(error ? error : "could not load dynamic library"));
		}
		else
		{
			dlerror();
		}
		return 0;
	}
	else
	{
		return handle;
	}
}

void jit_dynlib_close(jit_dynlib_handle_t handle)
{
	dlclose(handle);
}

void *jit_dynlib_get_symbol(jit_dynlib_handle_t handle, const char *symbol)
{
	void *value = dlsym(handle, (char *)symbol);
	const char *error = dlerror();
	char *newName;
	if(error == 0)
	{
		return value;
	}
	newName = (char *)jit_malloc(jit_strlen(symbol) + 2);
	if(newName)
	{
		/* Try again with '_' prepended to the name in case
		   we are running on a system with a busted "dlsym" */
		newName[0] = '_';
		jit_strcpy(newName + 1, symbol);
		value = dlsym(handle, newName);
		error = dlerror();
		if(error == 0)
		{
			jit_free(newName);
			return value;
		}
		jit_free(newName);
	}
	if(dynlib_debug)
	{
		fprintf(stderr, "%s: %s\n", symbol, error);
	}
	return 0;
}

const char *jit_dynlib_get_suffix(void)
{
	return "so";
}

#else	/* No dynamic library support */

jit_dynlib_handle_t jit_dynlib_open(const char *name)
{
	if(dynlib_debug)
	{
		fprintf(stderr, "%s: dynamic libraries are not available\n", name);
	}
	return 0;
}

void jit_dynlib_close(jit_dynlib_handle_t handle)
{
}

void *jit_dynlib_get_symbol(jit_dynlib_handle_t handle, const char *symbol)
{
	return 0;
}

const char *jit_dynlib_get_suffix(void)
{
	return "so";
}

#endif	/* No dynamic library support */

#ifdef	__cplusplus
};
#endif
