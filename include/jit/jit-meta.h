/*
 * jit-meta.h - Manipulate lists of metadata values.
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

#ifndef	_JIT_META_H
#define	_JIT_META_H

#ifdef	__cplusplus
extern	"C" {
#endif

typedef struct _jit_meta *jit_meta_t;

int jit_meta_set(jit_meta_t *list, int type, void *data,
				 jit_meta_free_func free_data, jit_function_t pool_owner);
void *jit_meta_get(jit_meta_t list, int type);
void jit_meta_free(jit_meta_t *list, int type);
void jit_meta_destroy(jit_meta_t *list);

#ifdef	__cplusplus
};
#endif

#endif /* _JIT_META_H */
