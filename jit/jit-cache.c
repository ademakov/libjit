/*
 * jit-cache.c - Translated function cache implementation.
 *
 * Copyright (C) 2002, 2003, 2008  Southern Storm Software, Pty Ltd.
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

/*
See the bottom of this file for documentation on the cache system.
*/

#include "jit-internal.h"
#include "jit-cache.h"
#include "jit-apply-func.h"

#ifdef	__cplusplus
extern	"C" {
#endif

/*
 * Tune the default size of a cache page.  Memory is allocated from
 * the system in chunks of this size.
 */
#ifndef	JIT_CACHE_PAGE_SIZE
#define	JIT_CACHE_PAGE_SIZE		(64 * 1024)
#endif

/*
 * Tune the maximum size of a cache page.  The size of a page might be
 * up to (JIT_CACHE_PAGE_SIZE * JIT_CACHE_MAX_PAGE_FACTOR).  This will
 * also determine the maximum method size that can be translated.
 */
#ifndef JIT_CACHE_MAX_PAGE_FACTOR
#define JIT_CACHE_MAX_PAGE_FACTOR	1024
#endif

/*
 * Structure of a debug information header for a method.
 * This header is followed by the debug data, which is
 * stored as compressed metadata integers.
 */
typedef struct jit_cache_debug *jit_cache_debug_t;
struct jit_cache_debug
{
	jit_cache_debug_t next;			/* Next block for the method */

};

/*
 * Method information block, organised as a red-black tree node.
 * There may be more than one such block associated with a method
 * if the method contains exception regions.
 */
typedef struct jit_cache_method *jit_cache_method_t;
struct jit_cache_method
{
	void			*method;	/* Method containing the region */
	void			*cookie;	/* Cookie value for the region */
	unsigned char		*start;		/* Start of the region */
	unsigned char		*end;		/* End of the region */
	jit_cache_debug_t	debug;		/* Debug information for method */
	jit_cache_method_t	left;		/* Left sub-tree and red/black bit */
	jit_cache_method_t	right;		/* Right sub-tree */

};

/*
 * Structure of the page list entry.
 */
struct jit_cache_page
{
	void			*page;		/* Page memory */
	long			factor;		/* Page size factor */
};

/*
 * Structure of the method cache.
 */
#define	JIT_CACHE_DEBUG_SIZE		64
struct jit_cache
{
	struct jit_cache_page	*pages;		/* List of pages currently in the cache */
	unsigned long		numPages;	/* Number of pages currently in the cache */
	unsigned long		maxNumPages;	/* Maximum number of pages that could be in the list */
	unsigned long		pageSize;	/* Default size of a page for allocation */
	unsigned int		maxPageFactor;	/* Maximum page size factor */
	unsigned char		*freeStart;	/* Start of the current free region */
	unsigned char		*freeEnd;	/* End of the current free region */
	long			pagesLeft;	/* Number of pages left to allocate */
	jit_cache_method_t	method;		/* Information for the current method */
	struct jit_cache_method	head;		/* Head of the lookup tree */
	struct jit_cache_method	nil;		/* Nil pointer for the lookup tree */
	unsigned char		*start;		/* Start of the current method */
	unsigned char		debugData[JIT_CACHE_DEBUG_SIZE];
	int			debugLen;	/* Length of temporary debug data */
	jit_cache_debug_t	firstDebug;	/* First debug block for method */
	jit_cache_debug_t	lastDebug;	/* Last debug block for method */

};

/*
 * Compress a "long" value so that it takes up less bytes.
 * This is used to store offsets within functions and
 * debug line numbers, which are usually small integers.
 */
static int CompressInt(unsigned char *buf, long data)
{
	if(data >= 0)
	{
		if(data < (long)0x40)
		{
			buf[0] = (unsigned char)(data << 1);
			return 1;
		}
		else if(data < (long)(1 << 13))
		{
			buf[0] = (unsigned char)(((data >> 7) & 0x3F) | 0x80);
			buf[1] = (unsigned char)(data << 1);
			return 2;
		}
		else if(data < (long)(1L << 28))
		{
			buf[0] = (unsigned char)((data >> 23) | 0xC0);
			buf[1] = (unsigned char)(data >> 15);
			buf[2] = (unsigned char)(data >> 7);
			buf[3] = (unsigned char)(data << 1);
			return 4;
		}
		else
		{
			buf[0] = (unsigned char)0xE0;
			buf[1] = (unsigned char)(data >> 23);
			buf[2] = (unsigned char)(data >> 15);
			buf[3] = (unsigned char)(data >> 7);
			buf[4] = (unsigned char)(data << 1);
			return 5;
		}
	}
	else
	{
		if(data >= ((long)-0x40))
		{
			buf[0] = ((((unsigned char)(data << 1)) & 0x7E) | 0x01);
			return 1;
		}
		else if(data >= ((long)-(1 << 13)))
		{
			buf[0] = (unsigned char)(((data >> 7) & 0x3F) | 0x80);
			buf[1] = (unsigned char)((data << 1) | 0x01);
			return 2;
		}
		else if(data >= ((long)-(1L << 29)))
		{
			buf[0] = (unsigned char)(((data >> 23) & 0x1F) | 0xC0);
			buf[1] = (unsigned char)(data >> 15);
			buf[2] = (unsigned char)(data >> 7);
			buf[3] = (unsigned char)((data << 1) | 0x01);
			return 4;
		}
		else
		{
			buf[0] = (unsigned char)0xE1;
			buf[1] = (unsigned char)(data >> 23);
			buf[2] = (unsigned char)(data >> 15);
			buf[3] = (unsigned char)(data >> 7);
			buf[4] = (unsigned char)((data << 1) | 0x01);
			return 5;
		}
	}
}

/*
 * Control data structure that is used by "UncompressInt".
 */
typedef struct
{
	const unsigned char	*data;		/* Current data position */
	unsigned long		 len;		/* Length remaining to read */
	int			 error;		/* Set to non-zero if error encountered */

} UncompressReader;

/*
 * Uncompress a value that was compressed by "CompressInt".
 */
static long UncompressInt(UncompressReader *meta)
{
	unsigned char ch;
	unsigned char ch2;
	unsigned char ch3;
	unsigned char ch4;
	unsigned long value;

	if(meta->len > 0)
	{
		ch = *((meta->data)++);
		--(meta->len);
		if((ch & 0x80) == 0x00)
		{
			/* One-byte form of the item */
			if((ch & 0x01) == 0x00)
				return (long)(ch >> 1);
			else
				return (long)(signed char)((ch >> 1) | 0xC0);
		}
		else if((ch & 0xC0) == 0x80)
		{
			/* Two-byte form of the item */
			if(meta->len > 0)
			{
				--(meta->len);
				value = (((unsigned long)(ch & 0x3F)) << 8) |
					     ((unsigned long)(*((meta->data)++)));
				if((value & 0x01) == 0x00)
					return (long)(value >> 1);
				else
					return (long)(jit_int)((value >> 1) | 0xFFFFE000);
			}
			else
			{
				meta->error = 1;
				return 0;
			}
		}
		else if((ch & 0xE0) == 0xC0)
		{
			/* Four-byte form of the item */
			if(meta->len >= 3)
			{
				ch2 = meta->data[0];
				ch3 = meta->data[1];
				ch4 = meta->data[2];
				meta->len -= 3;
				meta->data += 3;
				value = (((unsigned long)(ch & 0x1F)) << 24) |
					    (((unsigned long)ch2) << 16) |
					    (((unsigned long)ch3) << 8) |
					     ((unsigned long)ch4);
				if((value & 0x01) == 0x00)
					return (long)(value >> 1);
				else
					return (long)(jit_int)((value >> 1) | 0xF0000000);
			}
			else
			{
				meta->len = 0;
				meta->error = 1;
				return 0;
			}
		}
		else
		{
			/* Five-byte form of the item */
			if(meta->len >= 4)
			{
				ch  = meta->data[0];
				ch2 = meta->data[1];
				ch3 = meta->data[2];
				ch4 = meta->data[3];
				meta->len -= 4;
				meta->data += 4;
				value = (((unsigned long)ch) << 24) |
					    (((unsigned long)ch2) << 16) |
					    (((unsigned long)ch3) << 8) |
					     ((unsigned long)ch4);
				return (long)(jit_int)value;
			}
			else
			{
				meta->len = 0;
				meta->error = 1;
				return 0;
			}
		}
	}
	else
	{
		meta->error = 1;
		return 0;
	}
}

/*
 * Allocate a cache page and add it to the cache.
 */
static void AllocCachePage(jit_cache_t cache, int factor)
{
	long num;
	unsigned char *ptr;
	struct jit_cache_page *list;

	/* The minimum page factor is 1 */
	if(factor <= 0)
	{
		factor = 1;
	}

	/* If too big a page is requested, then bail out */
	if(((unsigned int) factor) > cache->maxPageFactor)
	{
		goto failAlloc;
	}

	/* If the page limit is hit, then bail out */
	if(cache->pagesLeft >= 0 && cache->pagesLeft < factor)
	{
		goto failAlloc;
	}

	/* Try to allocate a physical page */
	ptr = (unsigned char *) jit_malloc_exec((unsigned int) cache->pageSize * factor);
	if(!ptr)
	{
		goto failAlloc;
	}

	/* Add the page to the page list.  We keep this in an array
	   that is separate from the pages themselves so that we don't
	   have to "touch" the pages to free them.  Touching the pages
	   may cause them to be swapped in if they are currently out.
	   There's no point doing that if we are trying to free them */
	if(cache->numPages == cache->maxNumPages)
	{
		if(cache->numPages == 0)
		{
			num = 16;
		}
		else
		{
			num = cache->numPages * 2;
		}
		if(cache->pagesLeft > 0 && num > (cache->numPages + cache->pagesLeft - factor + 1))
		{
			num = cache->numPages + cache->pagesLeft - factor + 1;
		}

		list = (struct jit_cache_page *) jit_realloc(cache->pages,
							     sizeof(struct jit_cache_page) * num);
		if(!list)
		{
			jit_free_exec(ptr, cache->pageSize * factor);
		failAlloc:
			cache->freeStart = 0;
			cache->freeEnd = 0;
			return;
		}

		cache->maxNumPages = num;
		cache->pages = list;
	}
	cache->pages[cache->numPages].page = ptr;
	cache->pages[cache->numPages].factor = factor;
	++(cache->numPages);

	/* Adjust te number of pages left before we hit the limit */
	if(cache->pagesLeft > 0)
	{
		cache->pagesLeft -= factor;
	}

	/* Set up the working region within the new page */
	cache->freeStart = ptr;
	cache->freeEnd = ptr + (int) cache->pageSize * factor;
}

/*
 * Get or set the sub-trees of a node.
 */
#define	GetLeft(node)	\
	((jit_cache_method_t)(((jit_nuint)((node)->left)) & ~((jit_nuint)1)))
#define	GetRight(node)	((node)->right)
#define	SetLeft(node,value)	\
	((node)->left = (jit_cache_method_t)(((jit_nuint)(value)) | \
						(((jit_nuint)((node)->left)) & ((jit_nuint)1))))
#define	SetRight(node,value)	\
			((node)->right = (value))

/*
 * Get or set the red/black state of a node.
 */
#define	GetRed(node)	\
	((((jit_nuint)((node)->left)) & ((jit_nuint)1)) != 0)
#define	SetRed(node)	\
	((node)->left = (jit_cache_method_t)(((jit_nuint)((node)->left)) | \
									     ((jit_nuint)1)))
#define	SetBlack(node)	\
	((node)->left = (jit_cache_method_t)(((jit_nuint)((node)->left)) & \
									    ~((jit_nuint)1)))

/*
 * Compare a key against a node, being careful of sentinel nodes.
 */
static int CacheCompare(jit_cache_t cache, unsigned char *key,
						jit_cache_method_t node)
{
	if(node == &(cache->nil) || node == &(cache->head))
	{
		/* Every key is greater than the sentinel nodes */
		return 1;
	}
	else
	{
		/* Compare a regular node */
		if(key < node->start)
		{
			return -1;
		}
		else if(key > node->start)
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}
}

/*
 * Rotate a sub-tree around a specific node.
 */
static jit_cache_method_t CacheRotate(jit_cache_t cache, unsigned char *key,
								      jit_cache_method_t around)
{
	jit_cache_method_t child, grandChild;
	int setOnLeft;
	if(CacheCompare(cache, key, around) < 0)
	{
		child = GetLeft(around);
		setOnLeft = 1;
	}
	else
	{
		child = GetRight(around);
		setOnLeft = 0;
	}
	if(CacheCompare(cache, key, child) < 0)
	{
		grandChild = GetLeft(child);
		SetLeft(child, GetRight(grandChild));
		SetRight(grandChild, child);
	}
	else
	{
		grandChild = GetRight(child);
		SetRight(child, GetLeft(grandChild));
		SetLeft(grandChild, child);
	}
	if(setOnLeft)
	{
		SetLeft(around, grandChild);
	}
	else
	{
		SetRight(around, grandChild);
	}
	return grandChild;
}

/*
 * Split a red-black tree at the current position.
 */
#define	Split()		\
			do { \
				SetRed(temp); \
				SetBlack(GetLeft(temp)); \
				SetBlack(GetRight(temp)); \
				if(GetRed(parent)) \
				{ \
					SetRed(grandParent); \
					if((CacheCompare(cache, key, grandParent) < 0) != \
							(CacheCompare(cache, key, parent) < 0)) \
					{ \
						parent = CacheRotate(cache, key, grandParent); \
					} \
					temp = CacheRotate(cache, key, greatGrandParent); \
					SetBlack(temp); \
				} \
			} while (0)

/*
 * Add a method region block to the red-black lookup tree
 * that is associated with a method cache.
 */
static void AddToLookupTree(jit_cache_t cache, jit_cache_method_t method)
{
	unsigned char *key = method->start;
	jit_cache_method_t temp;
	jit_cache_method_t greatGrandParent;
	jit_cache_method_t grandParent;
	jit_cache_method_t parent;
	jit_cache_method_t nil = &(cache->nil);
	int cmp;

	/* Search for the insert position */
	temp = &(cache->head);
	greatGrandParent = temp;
	grandParent = temp;
	parent = temp;
	while(temp != nil)
	{
		/* Adjust our ancestor pointers */
		greatGrandParent = grandParent;
		grandParent = parent;
		parent = temp;

		/* Compare the key against the current node */
		cmp = CacheCompare(cache, key, temp);
		if(cmp == 0)
		{
			/* This is a duplicate, which normally shouldn't happen.
			   If it does happen, then ignore the node and bail out */
			return;
		}
		else if(cmp < 0)
		{
			temp = GetLeft(temp);
		}
		else
		{
			temp = GetRight(temp);
		}

		/* Do we need to split this node? */
		if(GetRed(GetLeft(temp)) && GetRed(GetRight(temp)))
		{
			Split();
		}
	}

	/* Insert the new node into the current position */
	method->left = (jit_cache_method_t)(((jit_nuint)nil) | ((jit_nuint)1));
	method->right = nil;
	if(CacheCompare(cache, key, parent) < 0)
	{
		SetLeft(parent, method);
	}
	else
	{
		SetRight(parent, method);
	}
	Split();
	SetBlack(cache->head.right);
}

/*
 * Flush the current debug buffer.
 */
static void FlushCacheDebug(jit_cache_posn *posn)
{
	jit_cache_t cache = posn->cache;
	jit_cache_debug_t debug;

	/* Allocate a new jit_cache_debug structure to hold the data */
	debug = _jit_cache_alloc(posn,
		(unsigned long)(sizeof(struct jit_cache_debug) + cache->debugLen));
	if(!debug)
	{
		cache->debugLen = 0;
		return;
	}

	/* Copy the temporary debug data into the new structure */
	jit_memcpy(debug + 1, cache->debugData, cache->debugLen);

	/* Link the structure into the debug list */
	debug->next = 0;
	if(cache->lastDebug)
	{
		cache->lastDebug->next = debug;
	}
	else
	{
		cache->firstDebug = debug;
	}
	cache->lastDebug = debug;

	/* Reset the temporary debug buffer */
	cache->debugLen = 0;
}

/*
 * Write a debug pair to the cache.  The pair (-1, -1)
 * terminates the debug information for a method.
 */
static void WriteCacheDebug(jit_cache_posn *posn, long offset, long nativeOffset)
{
	jit_cache_t cache = posn->cache;

	/* Write the two values to the temporary debug buffer */
	cache->debugLen += CompressInt
		(cache->debugData + cache->debugLen, offset);
	cache->debugLen += CompressInt
		(cache->debugData + cache->debugLen, nativeOffset);
	if((cache->debugLen + 5 * 2 + 1) > (int)(sizeof(cache->debugData)))
	{
		/* Overflow occurred: write -2 to mark the end of this buffer */
		cache->debugLen += CompressInt
				(cache->debugData + cache->debugLen, -2);

		/* Flush the debug data that we have collected so far */
		FlushCacheDebug(posn);
	}
}

jit_cache_t _jit_cache_create(long limit, long cache_page_size, int max_page_factor)
{
	jit_cache_t cache;
	unsigned long exec_page_size;

	/* Allocate space for the cache control structure */
	if((cache = (jit_cache_t )jit_malloc(sizeof(struct jit_cache))) == 0)
	{
		return 0;
	}

	/* determine the default cache page size */
	exec_page_size = jit_exec_page_size();
	if(cache_page_size <= 0)
	{
		cache_page_size = JIT_CACHE_PAGE_SIZE;
	}
	if(cache_page_size < exec_page_size)
	{
		cache_page_size = exec_page_size;
	}
	else
	{
		cache_page_size = (cache_page_size / exec_page_size) * exec_page_size;
	}

	/* determine the maximum page size factor */
	if(max_page_factor <= 0)
	{
		max_page_factor = JIT_CACHE_MAX_PAGE_FACTOR;
	}

	/* Initialize the rest of the cache fields */
	cache->pages = 0;
	cache->numPages = 0;
	cache->maxNumPages = 0;
	cache->pageSize = cache_page_size;
	cache->maxPageFactor = max_page_factor;
	cache->freeStart = 0;
	cache->freeEnd = 0;
	if(limit > 0)
	{
		cache->pagesLeft = limit / cache_page_size;
		if(cache->pagesLeft < 1)
		{
			cache->pagesLeft = 1;
		}
	}
	else
	{
		cache->pagesLeft = -1;
	}
	cache->method = 0;
	cache->nil.method = 0;
	cache->nil.cookie = 0;
	cache->nil.start = 0;
	cache->nil.end = 0;
	cache->nil.debug = 0;
	cache->nil.left = &(cache->nil);
	cache->nil.right = &(cache->nil);
	cache->head.method = 0;
	cache->head.cookie = 0;
	cache->head.start = 0;
	cache->head.end = 0;
	cache->head.debug = 0;
	cache->head.left = 0;
	cache->head.right = &(cache->nil);
	cache->start = 0;
	cache->debugLen = 0;
	cache->firstDebug = 0;
	cache->lastDebug = 0;

	/* Allocate the initial cache page */
	AllocCachePage(cache, 0);
	if(!cache->freeStart)
	{
		_jit_cache_destroy(cache);
		return 0;
	}

	/* Ready to go */
	return cache;
}

void _jit_cache_destroy(jit_cache_t cache)
{
	unsigned long page;

	/* Free all of the cache pages */
	for(page = 0; page < cache->numPages; ++page)
	{
		jit_free_exec(cache->pages[page].page,
			      cache->pageSize * cache->pages[page].factor);
	}
	if(cache->pages)
	{
		jit_free(cache->pages);
	}

	/* Free the cache object itself */
	jit_free(cache);
}

int _jit_cache_is_full(jit_cache_t cache, jit_cache_posn *posn)
{
	return (!cache->freeStart || (posn && posn->ptr >= posn->limit));
}

int _jit_cache_start_method(jit_cache_t cache,
			    jit_cache_posn *posn,
			    int page_factor,
			    int align,
			    void *method)
{
	unsigned char *ptr;

	/* Do we need to allocate a new cache page? */
	if(page_factor > 0)
	{
		AllocCachePage(cache, page_factor);
	}

	/* Bail out if the cache is already full */
	if(!cache->freeStart)
	{
		return JIT_CACHE_TOO_BIG;
	}

	/* Set up the initial cache position */
	posn->cache = cache;
	posn->ptr = cache->freeStart;
	posn->limit = cache->freeEnd;

	/* Align the method start */
	ptr = posn->ptr;
	if(align > 1)
	{
		ptr = (unsigned char *)(((jit_nuint) (ptr + align - 1)) & ~((jit_nuint) (align - 1)));
	}
	if(ptr >= posn->limit)
	{
		/* There is insufficient space in this page */
		posn->ptr = posn->limit;
		return JIT_CACHE_RESTART;
	}
#ifdef jit_should_pad
	if(ptr > posn->ptr)
	{
		_jit_pad_buffer(posn->ptr, ptr - posn->ptr);
	}
#endif
	posn->ptr = ptr;

	/* Allocate memory for the method information block */
	cache->method = (jit_cache_method_t) _jit_cache_alloc(posn, sizeof(struct jit_cache_method));
	if(!cache->method)
	{
		/* There is insufficient space in this page */
		return JIT_CACHE_RESTART;
	}
	cache->method->method = method;
	cache->method->cookie = 0;
	cache->method->start = posn->ptr;
	cache->method->end = posn->ptr;
	cache->method->debug = 0;
	cache->method->left = 0;
	cache->method->right = 0;

	/* Store the method start address */
	cache->start = posn->ptr;

	/* Clear the debug data */
	cache->debugLen = 0;
	cache->firstDebug = 0;
	cache->lastDebug = 0;

	return JIT_CACHE_OK;
}

int _jit_cache_end_method(jit_cache_posn *posn)
{
	jit_cache_t cache = posn->cache;
	jit_cache_method_t method;
	jit_cache_method_t next;

	/* Determine if we ran out of space while writing the method */
	if(posn->ptr >= posn->limit)
	{
		/* If we had a newly allocated page then it has to be freed
		   to let allocate another new page of appropriate size. */
		if((cache->freeStart == ((unsigned char *)(cache->pages[cache->numPages - 1].page)))
		    && (cache->freeEnd
			== (cache->freeStart + (cache->pageSize * cache->pages[cache->numPages - 1].factor))))
		{
			--(cache->numPages);
			jit_free_exec(cache->pages[cache->numPages].page,
				      cache->pageSize * cache->pages[cache->numPages].factor);
			if (cache->pagesLeft >= 0)
			{
				cache->pagesLeft += cache->pages[cache->numPages].factor;
			}
			cache->freeStart = 0;
			cache->freeEnd = 0;
		}
		return JIT_CACHE_RESTART;
	}

	/* Terminate the debug information and flush it */
	if(cache->firstDebug || cache->debugLen)
	{
		WriteCacheDebug(posn, -1, -1);
		if(cache->debugLen)
		{
			FlushCacheDebug(posn);
		}
	}

	/* Flush the position information back to the cache */
	cache->freeStart = posn->ptr;
	cache->freeEnd = posn->limit;

	/* Update the last method region block and then
	   add all method regions to the lookup tree */
	method = cache->method;
	if(method)
	{
		method->end = posn->ptr;
		do
		{
			method->debug = cache->firstDebug;
			next = method->right;
			AddToLookupTree(cache, method);
			method = next;
		}
		while(method != 0);
		cache->method = 0;
	}

	/* The method is ready to go */
	return JIT_CACHE_OK;
}

void *_jit_cache_alloc(jit_cache_posn *posn, unsigned long size)
{
	unsigned char *ptr;

	/* Bail out if the request is too big to ever be satisfiable */
	if(size > (unsigned long)(posn->limit - posn->ptr))
	{
		posn->ptr = posn->limit;
		return 0;
	}

	/* Allocate memory from the top of the free region, so that it
	   does not overlap with the method code being written at the
	   bottom of the free region */
	ptr = (unsigned char *)(((jit_nuint)(posn->limit - size)) &
		                    ~(((jit_nuint)JIT_BEST_ALIGNMENT) - 1));
	if(ptr < posn->ptr)
	{
		/* When we aligned the block, it caused an overflow */
		posn->ptr = posn->limit;
		return 0;
	}

	/* Allocate the block and return it */
	posn->limit = ptr;
	return (void *)ptr;
}

void *_jit_cache_alloc_no_method
	(jit_cache_t cache, unsigned long size, unsigned long align)
{
	unsigned char *ptr;

	/* Bail out if the request is too big to ever be satisfiable */
	if(size > (unsigned long)(cache->freeEnd - cache->freeStart))
	{
		AllocCachePage(cache, 0);
		if(size > (unsigned long)(cache->freeEnd - cache->freeStart))
		{
			return 0;
		}
	}

	/* Allocate memory from the top of the free region, so that it
	   does not overlap with the method code being written at the
	   bottom of the free region */
	ptr = (unsigned char *)(((jit_nuint)(cache->freeEnd - size)) &
		                    ~(((jit_nuint)align) - 1));
	if(ptr < cache->freeStart)
	{
		/* When we aligned the block, it caused an overflow */
		return 0;
	}

	/* Allocate the block and return it */
	cache->freeEnd = ptr;
	return (void *)ptr;
}

void _jit_cache_align(jit_cache_posn *posn, int align, int diff, int nop)
{
	jit_nuint current;
	jit_nuint next;

	/* Determine the location of the next alignment boundary */
	if(align <= 1)
	{
		align = 1;
	}
	current = (jit_nuint)(posn->ptr);
	next = (current + ((jit_nuint)align) - 1) &
		   ~(((jit_nuint)align) - 1);
	if(current == next || (next - current) >= (jit_nuint)diff)
	{
		return;
	}

	/* Detect overflow of the free memory region */
	if(next > ((jit_nuint)(posn->limit)))
	{
		posn->ptr = posn->limit;
		return;
	}

#ifndef jit_should_pad
	/* Fill from "current" to "next" with nop bytes */
	while(current < next)
	{
		*((posn->ptr)++) = (unsigned char)nop;
		++current;
	}
#else
	/* Use CPU-specific padding, because it may be more efficient */
	_jit_pad_buffer((unsigned char *)current, (int)(next - current));
#endif
}

void _jit_cache_mark_bytecode(jit_cache_posn *posn, unsigned long offset)
{
	WriteCacheDebug(posn, (long)offset,
				    (long)(posn->ptr - posn->cache->start));
}

void _jit_cache_new_region(jit_cache_posn *posn, void *cookie)
{
	jit_cache_method_t method;
	jit_cache_method_t newMethod;

	/* Fetch the current method information block */
	method = posn->cache->method;
	if(!method)
	{
		return;
	}

	/* If the current region starts here, then simply update it */
	if(method->start == posn->ptr)
	{
		method->cookie = cookie;
		return;
	}

	/* Close off the current method region */
	method->end = posn->ptr;

	/* Allocate a new method region block and initialise it */
	newMethod = (jit_cache_method_t)
		_jit_cache_alloc(posn, sizeof(struct jit_cache_method));
	if(!newMethod)
	{
		return;
	}
	newMethod->method = method->method;
	newMethod->cookie = cookie;
	newMethod->start = posn->ptr;
	newMethod->end = posn->ptr;

	/* Attach the new region to the cache */
	newMethod->left = 0;
	newMethod->right = method;
	posn->cache->method = newMethod;
}

void _jit_cache_set_cookie(jit_cache_posn *posn, void *cookie)
{
	if(posn->cache->method)
	{
		posn->cache->method->cookie = cookie;
	}
}

void *_jit_cache_get_method(jit_cache_t cache, void *pc, void **cookie)
{
	jit_cache_method_t node = cache->head.right;
	while(node != &(cache->nil))
	{
		if(((unsigned char *)pc) < node->start)
		{
			node = GetLeft(node);
		}
		else if(((unsigned char *)pc) >= node->end)
		{
			node = GetRight(node);
		}
		else
		{
			if(cookie)
			{
				*cookie = node->cookie;
			}
			return node->method;
		}
	}
	return 0;
}

/*
 * Add a parent pointer to a list.  Returns null if out of memory.
 */
static int add_parent(jit_cache_method_t *parent_buf,
					  jit_cache_method_t **parents,
					  int *num_parents, int *max_parents,
					  jit_cache_method_t node)
{
	jit_cache_method_t *new_list;
	if(*num_parents >= *max_parents)
	{
		if(*parents == parent_buf)
		{
			new_list = (jit_cache_method_t *)jit_malloc
				((*max_parents * 2) * sizeof(jit_cache_method_t));
			if(new_list)
			{
				jit_memcpy(new_list, *parents,
						   (*num_parents) * sizeof(jit_cache_method_t));
			}
		}
		else
		{
			new_list = (jit_cache_method_t *)jit_realloc
				(*parents, (*max_parents * 2) * sizeof(jit_cache_method_t));
		}
		if(!new_list)
		{
			return 0;
		}
		*parents = new_list;
		*max_parents *= 2;
	}
	(*parents)[*num_parents] = node;
	++(*num_parents);
	return 1;
}

void *_jit_cache_get_start_method(jit_cache_t cache, void *pc)
{
	/* TODO: This function is not currently aware of multiple regions. */
	jit_cache_method_t node = cache->head.right;
	while(node != &(cache->nil))
	{
		if(((unsigned char *)pc) < node->start)
		{
			node = GetLeft(node);
		}
		else if(((unsigned char *)pc) >= node->end)
		{
			node = GetRight(node);
		}
		else
		{
			return node->start;
		}
	}
	return 0;
}

void *_jit_cache_get_end_method(jit_cache_t cache, void *pc)
{
	jit_cache_method_t parent_buf[16];
	jit_cache_method_t *parents = parent_buf;
	int num_parents = 0;
	int max_parents = 16;
	jit_cache_method_t node = cache->head.right;
	jit_cache_method_t last;
	jit_cache_method_t parent;
	void *method;
	while(node != &(cache->nil))
	{
		if(((unsigned char *)pc) < node->start)
		{
			if(!add_parent(parent_buf, &parents, &num_parents,
			   &max_parents, node))
			{
				break;
			}
			node = GetLeft(node);
		}
		else if(((unsigned char *)pc) >= node->end)
		{
			if(!add_parent(parent_buf, &parents, &num_parents,
			   &max_parents, node))
			{
				break;
			}
			node = GetRight(node);
		}
		else
		{
			/* This is the node that contains the starting position.
			   We now need to do an inorder traversal from this point
			   to find the last node that mentions this method */
			method = node->method;
			last = node;
			do
			{
				if(GetRight(node) != &(cache->nil))
				{
					/* Move down the left-most sub-tree of the right */
					if(!add_parent(parent_buf, &parents, &num_parents,
					   &max_parents, node))
					{
						goto failed;
					}
					node = GetRight(node);
					while(GetLeft(node) != &(cache->nil))
					{
						if(!add_parent(parent_buf, &parents, &num_parents,
						   &max_parents, node))
						{
							goto failed;
						}
						node = GetLeft(node);
					}
				}
				else
				{
					/* Find a parent or other ancestor that contains this
					   node within its left sub-tree */
					for(;;)
					{
						if(num_parents == 0)
						{
							node = 0;
							break;
						}
						parent = parents[--num_parents];
						if(GetLeft(parent) == node)
						{
							/* We are on our parent's left, so next is parent */
							node = parent;
							break;
						}
						node = parent;
					}
					if(!node)
					{
						/* We reached the root of the tree */
						break;
					}
				}
				if(node->method == method)
				{
					last = node;
				}
			}
			while(node->method == method);
			if(parents != parent_buf)
			{
				jit_free(parents);
			}
			return last->end;
		}
	}
failed:
	if(parents != parent_buf)
	{
		jit_free(parents);
	}
	return 0;
}

/*
 * Count the number of methods in a sub-tree.
 */
static unsigned long CountMethods(jit_cache_method_t node,
								  jit_cache_method_t nil,
								  void **prev)
{
	unsigned long num;

	/* Bail out if we've reached a leaf */
	if(node == nil)
	{
		return 0;
	}

	/* Count the number of methods in the left sub-tree */
	num = CountMethods(GetLeft(node), nil, prev);

	/* Process the current node */
	if(node->method != 0 && node->method != *prev)
	{
		++num;
		*prev = node->method;
	}

	/* Count the number of methods in the right sub-tree */
	return num + CountMethods(GetRight(node), nil, prev);
}

/*
 * Fill a list with methods.
 */
static unsigned long FillMethodList(void **list,
									jit_cache_method_t node,
								    jit_cache_method_t nil,
								    void **prev)
{
	unsigned long num;

	/* Bail out if we've reached a leaf */
	if(node == nil)
	{
		return 0;
	}

	/* Process the methods in the left sub-tree */
	num = FillMethodList(list, GetLeft(node), nil, prev);

	/* Process the current node */
	if(node->method != 0 && node->method != *prev)
	{
		list[num] = node->method;
		++num;
		*prev = node->method;
	}

	/* Process the methods in the right sub-tree */
	return num + FillMethodList(list + num, GetRight(node), nil, prev);
}

void **_jit_cache_get_method_list(jit_cache_t cache)
{
	void *prev;
	unsigned long num;
	void **list;

	/* Count the number of distinct methods in the tree */
	prev = 0;
	num = CountMethods(cache->head.right, &(cache->nil), &prev);

	/* Allocate a list to hold all of the method descriptors */
	list = (void **)jit_malloc((num + 1) * sizeof(void *));
	if(!list)
	{
		return 0;
	}

	/* Fill the list with methods and then return it */
	prev = 0;
	FillMethodList(list, cache->head.right, &(cache->nil), &prev);
	list[num] = 0;
	return list;
}

/*
 * Temporary structure for iterating over a method's debug list.
 */
typedef struct
{
	jit_cache_debug_t	list;
	UncompressReader	reader;

} jit_cache_debug_iter;

/*
 * Initialize a debug information list iterator for a method.
 */
static void InitDebugIter(jit_cache_debug_iter *iter,
						  jit_cache_t cache, void *start)
{
	jit_cache_method_t node = cache->head.right;
	while(node != &(cache->nil))
	{
		if(((unsigned char *)start) < node->start)
		{
			node = GetLeft(node);
		}
		else if(((unsigned char *)start) >= node->end)
		{
			node = GetRight(node);
		}
		else
		{
			iter->list = node->debug;
			if(iter->list)
			{
				iter->reader.data = (unsigned char *)(iter->list + 1);
				iter->reader.len = JIT_CACHE_DEBUG_SIZE;
				iter->reader.error = 0;
			}
			return;
		}
	}
	iter->list = 0;
}

/*
 * Get the next debug offset pair from a debug information list.
 * Returns non-zero if OK, or zero at the end of the list.
 */
static int GetNextDebug(jit_cache_debug_iter *iter, unsigned long *offset,
						unsigned long *nativeOffset)
{
	long value;
	while(iter->list)
	{
		value = UncompressInt(&(iter->reader));
		if(value == -1)
		{
			return 0;
		}
		else if(value != -2)
		{
			*offset = (unsigned long)value;
			*nativeOffset = (unsigned long)(UncompressInt(&(iter->reader)));
			return 1;
		}
		iter->list = iter->list->next;
		if(iter->list)
		{
			iter->reader.data = (unsigned char *)(iter->list + 1);
			iter->reader.len = JIT_CACHE_DEBUG_SIZE;
			iter->reader.error = 0;
		}
	}
	return 0;
}

unsigned long _jit_cache_get_native(jit_cache_t cache, void *start,
						   			unsigned long offset, int exact)
{
	jit_cache_debug_iter iter;
	unsigned long ofs, nativeOfs;
	unsigned long prevNativeOfs = JIT_CACHE_NO_OFFSET;

	/* Search for the bytecode offset */
	InitDebugIter(&iter, cache, start);
	while(GetNextDebug(&iter, &ofs, &nativeOfs))
	{
		if(exact)
		{
			if(ofs == offset)
			{
				return nativeOfs;
			}
		}
		else if(ofs > offset)
		{
			return prevNativeOfs;
		}
		prevNativeOfs = nativeOfs;
	}

	return exact ? JIT_CACHE_NO_OFFSET : prevNativeOfs;
}

unsigned long _jit_cache_get_bytecode(jit_cache_t cache, void *start,
							 		  unsigned long offset, int exact)
{
	jit_cache_debug_iter iter;
	unsigned long ofs, nativeOfs;
	unsigned long prevOfs = JIT_CACHE_NO_OFFSET;

	/* Search for the native offset */
	InitDebugIter(&iter, cache, start);
	while(GetNextDebug(&iter, &ofs, &nativeOfs))
	{
		if(exact)
		{
			if(nativeOfs == offset)
			{
				return ofs;
			}
		}
		else if(nativeOfs > offset)
		{
			return prevOfs;
		}
		prevOfs = ofs;
	}

	return exact ? JIT_CACHE_NO_OFFSET : prevOfs;
}

unsigned long _jit_cache_get_size(jit_cache_t cache)
{
	return (cache->numPages * cache->pageSize) -
		   (cache->freeEnd - cache->freeStart);
}

/*

Using the cache
---------------

To output the code for a method, first call _jit_cache_start_method:

	jit_cache_posn posn;
	int result;

	result = _jit_cache_start_method(cache, &posn, factor,
					 METHOD_ALIGNMENT, method);

"factor" is used to control cache space allocation for the method.
The cache space is allocated by pages.  The value 0 indicates that
the method has to use the space left after the last allocation.
The value 1 or more indicates that the method has to start on a
newly allocated space that must contain the specified number of
consecutive pages.

"METHOD_ALIGNMENT" is used to align the start of the method on an
appropriate boundary for the target CPU.  Use the value 1 if no
special alignment is required.  Note: this value is a hint to the
cache - it may alter the alignment value.

"method" is a value that uniquely identifies the method that is being
translated.  Usually this is the "jit_function_t" pointer.

The function initializes the "posn" structure to point to the start
and end of the space available for the method output.  The function
returns one of three result codes:

	JIT_CACHE_OK       The function call was successful.
	JIT_CACHE_RESTART  The cache does not currently have enough
	                   space to fit any method.  This code may
			   only be returned if the "factor" value
			   was 0.  In this case it is necessary to
			   restart the method output process by
			   calling _jit_cache_start_method again
			   with a bigger "factor" value.
	JIT_CACHE_TOO_BIG  The cache does not have any space left
	                   for allocation.  In this case a restart
			   won't help.

To write code to the method, use the following:

	jit_cache_byte(&posn, value);
	jit_cache_word16(&posn, value);
	jit_cache_word32(&posn, value);
	jit_cache_native(&posn, value);
	jit_cache_word64(&posn, value);

These macros write the value to cache and then update the current
position.  If the macros detect the end of the avaialable space,
they will flag overflow, but otherwise do nothing (overflow is
flagged when posn->ptr == posn->limit).  The current position
in the method can be obtained using "jit_cache_get_posn".

Some CPU optimization guides recommend that labels should be aligned.
This can be achieved using _jit_cache_align.

Once the method code has been output, call _jit_cache_end_method to finalize
the process.  This function returns one of two result codes:

	JIT_CACHE_OK       The method output process was successful.
	JIT_CACHE_RESTART  The cache space overflowed. It is necessary
	                   to restart the method output process by
			   calling _jit_cache_start_method again
			   with a bigger "factor" value.

The caller should repeatedly translate the method while _jit_cache_end_method
continues to return JIT_CACHE_END_RESTART.  Normally there will be no
more than a single request to restart, but the caller should not rely
upon this.  The cache algorithm guarantees that the restart loop will
eventually terminate.

Cache data structure
--------------------

The cache consists of one or more "cache pages", which contain method
code and auxillary data.  The default size for a cache page is 64k
(JIT_CACHE_PAGE_SIZE).  The size is adjusted to be a multiple
of the system page size (usually 4k), and then stored in "pageSize".

Method code is written into a cache page starting at the bottom of the
page, and growing upwards.  Auxillary data is written into a cache page
starting at the top of the page, and growing downwards.  When the two
regions meet, a new cache page is allocated and the process restarts.

To allow methods bigger than a single cache page it is possible to
allocate a block of consecutive pages as a single unit. The method
code and auxillary data is written to such a multiple-page block in
the same manner as into an ordinary page.

Each method has one or more jit_cache_method auxillary data blocks associated
with it.  These blocks indicate the start and end of regions within the
method.  Normally these regions correspond to exception "try" blocks, or
regular code between "try" blocks.

The jit_cache_method blocks are organised into a red-black tree, which
is used to perform fast lookups by address (_jit_cache_get_method).  These
lookups are used when walking the stack during exceptions or security
processing.

Each method can also have offset information associated with it, to map
between native code addresses and offsets within the original bytecode.
This is typically used to support debugging.  Offset information is stored
as auxillary data, attached to the jit_cache_method block.

Threading issues
----------------

Writing a method to the cache, querying a method by address, or querying
offset information for a method, are not thread-safe.  The caller should
arrange for a cache lock to be acquired prior to performing these
operations.

Executing methods from the cache is thread-safe, as the method code is
fixed in place once it has been written.

Note: some CPU's require that a special cache flush instruction be
performed before executing method code that has just been written.
This is especially important in SMP environments.  It is the caller's
responsibility to perform this flush operation.

We do not provide locking or CPU flush capabilities in the cache
implementation itself, because the caller may need to perform other
duties before flushing the CPU cache or releasing the lock.

The following is the recommended way to map an "jit_function_t" pointer
to a starting address for execution:

	Look in "jit_function_t" to see if we already have a starting address.
		If so, then bail out.
	Acquire the cache lock.
	Check again to see if we already have a starting address, just
		in case another thread got here first.  If so, then release
		the cache lock and bail out.
	Translate the method.
	Update the "jit_function_t" structure to contain the starting address.
	Force a CPU cache line flush.
	Release the cache lock.

Why aren't methods flushed when the cache fills up?
---------------------------------------------------

In this cache implementation, methods are never "flushed" when the
cache becomes full.  Instead, all translation stops.  This is not a bug.
It is a feature.

In a multi-threaded environment, it is impossible to know if some
other thread is executing the code of a method that may be a candidate
for flushing.  Impossible that is unless one introduces a huge number
of read-write locks, one per method, to prevent a method from being
flushed.  The read locks must be acquired on entry to a method, and
released on exit.  The write locks are acquired prior to translation.

The overhead of introducing all of these locks and the associated cache
data structures is very high.  The only safe thing to do is to assume
that once a method has been translated, its code must be fixed in place
for all time.

We've looked at the code for other Free Software and Open Source JIT's,
and they all use a constantly-growing method cache.  No one has found
a solution to this problem, it seems.  Suggestions are welcome.

To prevent the cache from chewing up all of system memory, it is possible
to set a limit on how far it will grow.  Once the limit is reached, out
of memory will be reported and there is no way to recover.

*/

#ifdef	__cplusplus
};
#endif
