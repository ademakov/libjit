/*
 * jit-memory-arena.c - Memory manager based on a virtual memory arena.
 *
 * Copyright (C) 2020  Aleksey Demakov
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

#include <jit/jit-vmem.h>
#include "jit-internal.h"

#define JIT_MEMORY_NODE_SIZE ((jit_uint) 64)


typedef struct jit_memory_arena *jit_memory_arena_t;
typedef struct jit_memory_block *jit_memory_block_t;

typedef enum
{
	JIT_MEMORY_FREE,
	JIT_MEMORY_META,
	JIT_MEMORY_CODE,
	JIT_MEMORY_DATA,
	JIT_MEMORY_HUGE_CODE,
	JIT_MEMORY_HUGE_DATA,
} jit_memory_type_t;

struct jit_memory_arena
{
	unsigned char *free_addr;
	unsigned char *base_addr;
	jit_uint free_size;
	jit_uint full_size;

	jit_uint default_block_size;
	jit_uint first_code_block_size;
	jit_uint default_code_block_size;
	jit_uint first_data_block_size;
	jit_uint default_data_block_size;
};

struct jit_memory_node
{
	jit_memory_type_t type;
	jit_uint size;
	unsigned char *end[];
};

struct jit_memory_block
{
	unsigned char *start;
	unsigned char *end;

	jit_memory_type_t type;
};

static unsigned char *
arena_reserve(jit_memory_arena_t arena, jit_uint size)
{
	unsigned char *addr = jit_vmem_reserve(size);

	arena->free_addr = addr;
	arena->base_addr = addr;
	if(addr)
	{
		arena->free_size = size;
		arena->full_size = size;
	}
	else
	{
	  	arena->free_size = 0;
		arena->full_size = 0;
	}

	return addr;
}

static void
arena_release(jit_memory_arena_t arena)
{
	jit_vmem_release(arena->base_addr, arena->full_size);
}

static unsigned char *
arena_commit(jit_memory_arena_t arena, jit_uint size, jit_prot_t prot)
{
	unsigned char *addr;

	if(arena->free_size < size)
	{
		return 0;
	}

	addr = arena->free_addr;
	if(!jit_vmem_commit(addr, size, prot))
	{
		return 0;
	}

	arena->free_addr += size;
	arena->free_size -= size;

	return addr;
}

static jit_memory_arena_t
arena_create(jit_context_t context)
{
	return 0;
}

static void
arena_destroy(jit_memory_arena_t cache)
{
}

static int
arena_extend(jit_memory_arena_t cache, int count)
{
	return JIT_MEMORY_ERROR;
}

static jit_function_t
arena_alloc_function(jit_memory_arena_t cache)
{
	return 0;
}

static void
arena_free_function(jit_memory_arena_t cache, jit_function_t func)
{
}

static int
arena_start_function(jit_memory_arena_t cache, jit_function_t func)
{
	return JIT_MEMORY_ERROR;
}

static int
arena_end_function(jit_memory_arena_t cache, int result)
{
	return JIT_MEMORY_ERROR;
}

static void *
arena_get_code_break(jit_memory_arena_t cache)
{
	return 0;
}

static void
arena_set_code_break(jit_memory_arena_t cache, void *ptr)
{
}

static void *
arena_get_code_limit(jit_memory_arena_t cache)
{
	return 0;
}

static void *
arena_alloc_data(jit_memory_arena_t cache, unsigned long size, unsigned long align)
{
	return 0;
}

static void *
arena_alloc_trampoline(jit_memory_arena_t cache)
{
	return 0;
}

static void
arena_free_trampoline(jit_memory_arena_t cache, void *trampoline)
{
	/* not supported yet */
}

static void *
arena_alloc_closure(jit_memory_arena_t cache)
{
	return 0;
}

static void
arena_free_closure(jit_memory_arena_t cache, void *closure)
{
	/* not supported yet */
}

static void *
arena_find_function_info(jit_memory_arena_t cache, void *pc)
{
	return 0;
}

static jit_function_t
arena_get_function(jit_memory_arena_t cache, void *func_info)
{
	if(func_info)
	{
	}
	return 0;
}

static void *
arena_get_function_start(jit_memory_context_t memctx, void *func_info)
{
	if(func_info)
	{
	}
	return 0;
}

static void *
arena_get_function_end(jit_memory_context_t memctx, void *func_info)
{
	if(func_info)
	{
	}
	return 0;
}

jit_memory_manager_t
jit_memory_arena_memory_manager(void)
{
	static const struct jit_memory_manager mm = {

		(jit_memory_context_t (*)(jit_context_t))
		&arena_create,

		(void (*)(jit_memory_context_t))
		&arena_destroy,

		(jit_function_info_t (*)(jit_memory_context_t, void *))
		&arena_find_function_info,

		(jit_function_t (*)(jit_memory_context_t, jit_function_info_t))
		&arena_get_function,

		(void * (*)(jit_memory_context_t, jit_function_info_t))
		&arena_get_function_start,

		(void * (*)(jit_memory_context_t, jit_function_info_t))
		&arena_get_function_end,

		(jit_function_t (*)(jit_memory_context_t))
		&arena_alloc_function,

		(void (*)(jit_memory_context_t, jit_function_t))
		&arena_free_function,

		(int (*)(jit_memory_context_t, jit_function_t))
		&arena_start_function,

		(int (*)(jit_memory_context_t, int))
		&arena_end_function,

		(int (*)(jit_memory_context_t, int))
		&arena_extend,

		(void * (*)(jit_memory_context_t))
		&arena_get_code_limit,

		(void * (*)(jit_memory_context_t))
		&arena_get_code_break,

		(void (*)(jit_memory_context_t, void *))
		&arena_set_code_break,

		(void * (*)(jit_memory_context_t))
		&arena_alloc_trampoline,

		(void (*)(jit_memory_context_t, void *))
		&arena_free_trampoline,

		(void * (*)(jit_memory_context_t))
		&arena_alloc_closure,

		(void (*)(jit_memory_context_t, void *))
		&arena_free_closure,

		(void * (*)(jit_memory_context_t, jit_size_t, jit_size_t))
		&arena_alloc_data
	};
	return &mm;
}
