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
void *jit_malloc(unsigned int size);
void *jit_calloc(unsigned int num, unsigned int size);
void *jit_realloc(void *ptr, unsigned int size);
void jit_free(void *ptr);
void *jit_malloc_exec(unsigned int size);
void jit_free_exec(void *ptr, unsigned int size);
void jit_flush_exec(void *ptr, unsigned int size);
unsigned int jit_exec_page_size(void);
#define	jit_new(type)		((type *)jit_malloc(sizeof(type)))
#define	jit_cnew(type)		((type *)jit_calloc(1, sizeof(type)))

/*
 * Memory set/copy/compare routines.
 */
void *jit_memset(void *dest, int ch, unsigned int len);
void *jit_memcpy(void *dest, const void *src, unsigned int len);
void *jit_memmove(void *dest, const void *src, unsigned int len);
int   jit_memcmp(const void *s1, const void *s2, unsigned int len);
void *jit_memchr(const void *str, int ch, unsigned int len);

/*
 * String routines.  Note: jit_stricmp uses fixed ASCII rules for case
 * comparison, whereas jit_stricoll uses localized rules.
 */
unsigned int jit_strlen(const char *str);
char *jit_strcpy(char *dest, const char *src);
char *jit_strcat(char *dest, const char *src);
char *jit_strncpy(char *dest, const char *src, unsigned int len);
char *jit_strdup(const char *str);
char *jit_strndup(const char *str, unsigned int len);
int jit_strcmp(const char *str1, const char *str2);
int jit_strncmp(const char *str1, const char *str2, unsigned int len);
int jit_stricmp(const char *str1, const char *str2);
int jit_strnicmp(const char *str1, const char *str2, unsigned int len);
int jit_strcoll(const char *str1, const char *str2);
int jit_strncoll(const char *str1, const char *str2, unsigned int len);
int jit_stricoll(const char *str1, const char *str2);
int jit_strnicoll(const char *str1, const char *str2, unsigned int len);
char *jit_strchr(const char *str, int ch);
char *jit_strrchr(const char *str, int ch);
int jit_sprintf(char *str, const char *format, ...);
int jit_snprintf(char *str, unsigned int len, const char *format, ...);

/*
 * Dynamic library routines.
 */
typedef void *jit_dynlib_handle_t;
jit_dynlib_handle_t jit_dynlib_open(const char *name);
void jit_dynlib_close(jit_dynlib_handle_t handle);
void *jit_dynlib_get_symbol(jit_dynlib_handle_t handle, const char *symbol);
const char *jit_dynlib_get_suffix(void);
void jit_dynlib_set_debug(int flag);

#ifdef	__cplusplus
};
#endif

#endif /* _JIT_UTIL_H */
