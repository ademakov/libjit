/*
 * dpas-scope.h - Scope handling for Dynamic Pascal.
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

#ifndef	_DPAS_SCOPE_H
#define	_DPAS_SCOPE_H

#ifdef	__cplusplus
extern	"C" {
#endif

/*
 * Kinds of items that may be stored in a scope.
 */
#define	DPAS_ITEM_TYPE				1
#define	DPAS_ITEM_VARIABLE			2
#define	DPAS_ITEM_GLOBAL_VARIABLE	3
#define	DPAS_ITEM_CONSTANT			4
#define	DPAS_ITEM_PROCEDURE			5
#define	DPAS_ITEM_WITH				6
#define	DPAS_ITEM_FUNC_RETURN		7

/*
 * Opaque type that represents a scope.
 */
typedef struct dpas_scope *dpas_scope_t;

/*
 * Opaque type that represents a scope item.
 */
typedef struct dpas_scope_item *dpas_scope_item_t;

/*
 * Create a new scope, with a specified parent scope.  If the parent
 * scope is NULL, then this creates the global scope.
 */
dpas_scope_t dpas_scope_create(dpas_scope_t parent);

/*
 * Destroy a scope that is no longer required.
 */
void dpas_scope_destroy(dpas_scope_t scope);

/*
 * Look up a name within a scope.  If "up" is non-zero, then also
 * check the parent scopes.
 */
dpas_scope_item_t dpas_scope_lookup(dpas_scope_t scope,
									const char *name, int up);

/*
 * Add an entry to a scope.
 */
void dpas_scope_add(dpas_scope_t scope, const char *name, jit_type_t type,
					int kind, void *info, jit_meta_free_func free_info,
					char *filename, long linenum);

/*
 * Add a "with" field declaration to a scope.
 */
void dpas_scope_add_with(dpas_scope_t scope, jit_type_t type,
						 void *with_info, jit_meta_free_func free_info);

/*
 * Add a constant value to a scope.
 */
void dpas_scope_add_const(dpas_scope_t scope, const char *name,
						  jit_constant_t *value, char *filename, long linenum);

/*
 * Search a scope to look for undefined record types that were
 * referenced by a construct of the form "^name".
 */
void dpas_scope_check_undefined(dpas_scope_t scope);

/*
 * Get the name associated with a scope item.
 */
const char *dpas_scope_item_name(dpas_scope_item_t item);

/*
 * Get the kind associated with a scope item.
 */
int dpas_scope_item_kind(dpas_scope_item_t item);

/*
 * Get the type associated with a scope item.
 */
jit_type_t dpas_scope_item_type(dpas_scope_item_t item);

/*
 * Get the information block associated with a scope item.
 */
void *dpas_scope_item_info(dpas_scope_item_t item);

/*
 * Set the information block associated with a scope item.
 */
void dpas_scope_item_set_info(dpas_scope_item_t item, void *info);

/*
 * Get the filename associated with a scope item.
 */
const char *dpas_scope_item_filename(dpas_scope_item_t item);

/*
 * Get the line number associated with a scope item.
 */
long dpas_scope_item_linenum(dpas_scope_item_t item);

/*
 * Get the level associated with the current scope (global is zero).
 */
int dpas_scope_level(dpas_scope_t scope);

/*
 * Get the current scope.
 */
dpas_scope_t dpas_scope_current(void);

/*
 * Get the global scope.
 */
dpas_scope_t dpas_scope_global(void);

/*
 * Create a new scope and push into it.
 */
dpas_scope_t dpas_scope_push(void);

/*
 * Pop the current scope level and destroy it.
 */
void dpas_scope_pop(void);

/*
 * Determine if the current scope is the module-global scope.
 */
int dpas_scope_is_module(void);

#ifdef	__cplusplus
};
#endif

#endif	/* _DPAS_SCOPE_H */
