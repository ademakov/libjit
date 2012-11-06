/*
 * dpas-scope.c - Scope handling for Dynamic Pascal.
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

/*
 * Internal structure of a scope item.
 */
struct dpas_scope_item
{
	int					kind;
	char			   *name;
	jit_type_t			type;
	void			   *info;
	jit_meta_free_func	free_info;
	char			   *filename;
	long				linenum;
	dpas_scope_item_t	next;
};

/*
 * Internal structure of a scope.
 */
struct dpas_scope
{
	dpas_scope_t		parent;
	dpas_scope_item_t	first;
	dpas_scope_item_t	last;
	dpas_scope_item_t	first_with;
	dpas_scope_item_t	last_with;
	int					level;
};

dpas_scope_t dpas_scope_create(dpas_scope_t parent)
{
	dpas_scope_t scope;
	scope = jit_new(struct dpas_scope);
	if(!scope)
	{
		dpas_out_of_memory();
	}
	scope->parent = parent;
	scope->first = 0;
	scope->last = 0;
	scope->first_with = 0;
	scope->last_with = 0;
	if(parent)
	{
		scope->level = parent->level + 1;
	}
	else
	{
		scope->level = 0;
	}
	return scope;
}

void dpas_scope_destroy(dpas_scope_t scope)
{
	if(scope)
	{
		dpas_scope_item_t item, next;
		item = scope->first;
		while(item != 0)
		{
			next = item->next;
			if(item->name)
			{
				jit_free(item->name);
			}
			if(item->filename)
			{
				jit_free(item->filename);
			}
			if(item->free_info)
			{
				(*(item->free_info))(item->info);
			}
			jit_type_free(item->type);
			jit_free(item);
			item = next;
		}
		item = scope->first_with;
		while(item != 0)
		{
			next = item->next;
			if(item->name)
			{
				jit_free(item->name);
			}
			if(item->filename)
			{
				jit_free(item->filename);
			}
			if(item->free_info)
			{
				(*(item->free_info))(item->info);
			}
			jit_type_free(item->type);
			jit_free(item);
			item = next;
		}
		jit_free(scope);
	}
}

dpas_scope_item_t dpas_scope_lookup(dpas_scope_t scope,
									const char *name, int up)
{
	dpas_scope_item_t item;
	int check_with = 1;
	while(scope != 0)
	{
		/* Search through the "with" items for a field match */
		if(check_with)
		{
			item = scope->first_with;
			if(!item)
			{
				/* We are exiting from the top-most "with" scope
				   in the current procedure.  Ignore the rest of
				   the "with" scopes on the stack because we only
				   care about local/global variables from now on */
				check_with = 0;
			}
			while(item != 0)
			{
				if(dpas_type_find_name(item->type, name) != JIT_INVALID_NAME)
				{
					return item;
				}
				item = item->next;
			}
		}

		/* Search the regular items for a match */
		item = scope->first;
		while(item != 0)
		{
			if(!jit_stricmp(item->name, name))
			{
				return item;
			}
			item = item->next;
		}

		/* Move up to the parent scope if necessary */
		if(!up)
		{
			break;
		}
		scope = scope->parent;
	}
	return 0;
}

/*
 * Add an item to a scope list.
 */
static void scope_add(dpas_scope_item_t *first, dpas_scope_item_t *last,
					  const char *name, jit_type_t type, int kind,
					  void *info, jit_meta_free_func free_info,
					  char *filename, long linenum)
{
	dpas_scope_item_t item;
	item = jit_new(struct dpas_scope_item);
	if(!item)
	{
		dpas_out_of_memory();
	}
	if(name)
	{
		item->name = jit_strdup(name);
		if(!(item->name))
		{
			dpas_out_of_memory();
		}
	}
	else
	{
		item->name = 0;
	}
	item->type = jit_type_copy(type);
	item->kind = kind;
	item->info = info;
	item->free_info = free_info;
	if(filename)
	{
		item->filename = jit_strdup(filename);
		if(!(item->filename))
		{
			dpas_out_of_memory();
		}
	}
	else
	{
		item->filename = 0;
	}
	item->linenum = linenum;
	item->next = 0;
	if(*last)
	{
		(*last)->next = item;
	}
	else
	{
		*first = item;
	}
	*last = item;
}

void dpas_scope_add(dpas_scope_t scope, const char *name, jit_type_t type,
					int kind, void *info, jit_meta_free_func free_info,
					char *filename, long linenum)
{
	scope_add(&(scope->first), &(scope->last),
			  name, type, kind, info, free_info, filename, linenum);
}

void dpas_scope_add_with(dpas_scope_t scope, jit_type_t type,
						 void *with_info, jit_meta_free_func free_info)
{
	scope_add(&(scope->first_with), &(scope->last_with),
			  0, type, DPAS_ITEM_WITH, with_info, free_info, 0, 0);
}

void dpas_scope_add_const(dpas_scope_t scope, const char *name,
						  jit_constant_t *value, char *filename, long linenum)
{
	jit_constant_t *new_value;
	new_value = jit_new(jit_constant_t);
	if(!new_value)
	{
		dpas_out_of_memory();
	}
	*new_value = *value;
	scope_add(&(scope->first), &(scope->last), name, value->type,
			  DPAS_ITEM_CONSTANT, new_value, jit_free, filename, linenum);
}

void dpas_scope_check_undefined(dpas_scope_t scope)
{
	dpas_scope_item_t item = scope->first;
	jit_type_t type, new_type;
	while(item != 0)
	{
		if(item->kind == DPAS_ITEM_TYPE)
		{
			type = item->type;
			if(jit_type_get_tagged_kind(type) == DPAS_TAG_NAME &&
			   jit_type_get_tagged_type(type) == 0)
			{
				dpas_error_on_line
					(item->filename, item->linenum,
					 "forward-referenced record type `%s' was not "
					 "declared", item->name);
				new_type = jit_type_create_struct(0, 0, 0);
				if(!new_type)
				{
					dpas_out_of_memory();
				}
				jit_type_set_tagged_type(type, new_type, 0);
			}
		}
		item = item->next;
	}
}

const char *dpas_scope_item_name(dpas_scope_item_t item)
{
	return item->name;
}

int dpas_scope_item_kind(dpas_scope_item_t item)
{
	return item->kind;
}

jit_type_t dpas_scope_item_type(dpas_scope_item_t item)
{
	return item->type;
}

void *dpas_scope_item_info(dpas_scope_item_t item)
{
	return item->info;
}

void dpas_scope_item_set_info(dpas_scope_item_t item, void *info)
{
	item->info = info;
}

const char *dpas_scope_item_filename(dpas_scope_item_t item)
{
	return item->filename;
}

long dpas_scope_item_linenum(dpas_scope_item_t item)
{
	return item->linenum;
}

int dpas_scope_level(dpas_scope_t scope)
{
	return scope->level;
}

/*
 * The global and current scopes.
 */
static dpas_scope_t global_scope = 0;
static dpas_scope_t current_scope = 0;

dpas_scope_t dpas_scope_current(void)
{
	if(!current_scope)
	{
		/* Create the global scope for the first time */
		global_scope = dpas_scope_create(0);

		/* Create a wrapper around the global scope to contain
		   the program-private definitions.  This allows the
		   program to override the global scope if it wants to */
		current_scope = dpas_scope_create(global_scope);
	}
	return current_scope;
}

dpas_scope_t dpas_scope_global(void)
{
	if(!current_scope)
	{
		dpas_scope_current();
	}
	return global_scope;
}

dpas_scope_t dpas_scope_push(void)
{
	current_scope = dpas_scope_create(dpas_scope_current());
	return current_scope;
}

void dpas_scope_pop(void)
{
	dpas_scope_t scope = dpas_scope_current();
	if(scope->parent && scope->parent != global_scope)
	{
		current_scope = scope->parent;
		dpas_scope_destroy(scope);
	}
}

int dpas_scope_is_module(void)
{
	return (dpas_scope_current()->parent == dpas_scope_global());
}
