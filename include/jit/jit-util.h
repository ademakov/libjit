/*
 * jit-util.h - Utility functions.
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

#ifndef	_JIT_UTIL_H
#define	_JIT_UTIL_H

#include <jit/jit-common.h>

#ifdef	__cplusplus
extern	"C" {
#endif

/*
 * Memory allocation routines.
 */
void *jit_malloc(unsigned int size) JIT_NOTHROW;
void *jit_calloc(unsigned int num, unsigned int size) JIT_NOTHROW;
void *jit_realloc(void *ptr, unsigned int size) JIT_NOTHROW;
void jit_free(void *ptr) JIT_NOTHROW;
void *jit_malloc_exec(unsigned int size) JIT_NOTHROW;
void jit_free_exec(void *ptr, unsigned int size) JIT_NOTHROW;
void jit_flush_exec(void *ptr, unsigned int size) JIT_NOTHROW;
unsigned int jit_exec_page_size(void) JIT_NOTHROW;
#define	jit_new(type)		((type *)jit_malloc(sizeof(type)))
#define	jit_cnew(type)		((type *)jit_calloc(1, sizeof(type)))

/*
 * Memory set/copy/compare routines.
 */
void *jit_memset(void *dest, int ch, unsigned int len) JIT_NOTHROW;
void *jit_memcpy(void *dest, const void *src, unsigned int len) JIT_NOTHROW;
void *jit_memmove(void *dest, const void *src, unsigned int len) JIT_NOTHROW;
int   jit_memcmp(const void *s1, const void *s2, unsigned int len) JIT_NOTHROW;
void *jit_memchr(const void *str, int ch, unsigned int len) JIT_NOTHROW;

/*
 * String routines.
 */
unsigned int jit_strlen(const char *str) JIT_NOTHROW;
char *jit_strcpy(char *dest, const char *src) JIT_NOTHROW;
char *jit_strcat(char *dest, const char *src) JIT_NOTHROW;
char *jit_strncpy(char *dest, const char *src, unsigned int len) JIT_NOTHROW;
char *jit_strdup(const char *str) JIT_NOTHROW;
char *jit_strndup(const char *str, unsigned int len) JIT_NOTHROW;
int jit_strcmp(const char *str1, const char *str2) JIT_NOTHROW;
int jit_strncmp
	(const char *str1, const char *str2, unsigned int len) JIT_NOTHROW;
int jit_stricmp(const char *str1, const char *str2) JIT_NOTHROW;
int jit_strnicmp
	(const char *str1, const char *str2, unsigned int len) JIT_NOTHROW;
char *jit_strchr(const char *str, int ch) JIT_NOTHROW;
char *jit_strrchr(const char *str, int ch) JIT_NOTHROW;
int jit_sprintf(char *str, const char *format, ...) JIT_NOTHROW;
int jit_snprintf
	(char *str, unsigned int len, const char *format, ...) JIT_NOTHROW;

/*
 * Dynamic library routines.
 */
typedef void *jit_dynlib_handle_t;
jit_dynlib_handle_t jit_dynlib_open(const char *name) JIT_NOTHROW;
void jit_dynlib_close(jit_dynlib_handle_t handle) JIT_NOTHROW;
void *jit_dynlib_get_symbol
	(jit_dynlib_handle_t handle, const char *symbol) JIT_NOTHROW;
const char *jit_dynlib_get_suffix(void) JIT_NOTHROW;
void jit_dynlib_set_debug(int flag) JIT_NOTHROW;

/*
 * C++ name mangling definitions.
 */
#define	JIT_MANGLE_PUBLIC			0x0001
#define	JIT_MANGLE_PROTECTED		0x0002
#define	JIT_MANGLE_PRIVATE			0x0003
#define	JIT_MANGLE_STATIC			0x0008
#define	JIT_MANGLE_VIRTUAL			0x0010
#define	JIT_MANGLE_CONST			0x0020
#define	JIT_MANGLE_EXPLICIT_THIS	0x0040
#define	JIT_MANGLE_IS_CTOR			0x0080
#define	JIT_MANGLE_IS_DTOR			0x0100
#define	JIT_MANGLE_BASE				0x0200
char *jit_mangle_global_function
	(const char *name, jit_type_t signature, int form) JIT_NOTHROW;
char *jit_mangle_member_function
	(const char *class_name, const char *name,
	 jit_type_t signature, int form, int flags) JIT_NOTHROW;

#ifdef	__cplusplus
};
#endif

#endif /* _JIT_UTIL_H */
