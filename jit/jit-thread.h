/*
 * jit-thread.h - Internal thread management routines for libjit.
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

#ifndef	_JIT_THREAD_H
#define	_JIT_THREAD_H

#include <config.h>
#if defined(HAVE_PTHREAD_H) && defined(HAVE_LIBPTHREAD)
	#include <pthread.h>
#elif defined(JIT_WIN32_PLATFORM)
	#include <windows.h>
#endif

#ifdef	__cplusplus
extern	"C" {
#endif

/*
 * Determine the type of threading library that we are using.
 */
#if defined(HAVE_PTHREAD_H) && defined(HAVE_LIBPTHREAD)
	#define	JIT_THREADS_SUPPORTED	1
	#define	JIT_THREADS_PTHREAD		1
#elif defined(JIT_WIN32_PLATFORM)
	#define	JIT_THREADS_SUPPORTED	1
	#define	JIT_THREADS_WIN32		1
#else
	#define	JIT_THREADS_SUPPORTED	0
#endif

/*
 * Type that describes a thread's identifier, and the id comparison function.
 */
#if defined(JIT_THREADS_PTHREAD)
typedef pthread_t jit_thread_id_t;
#define	jit_thread_id_equal(x,y)	(pthread_equal((x), (y)))
#elif defined(JIT_THREADS_WIN32)
typedef HANDLE jit_thread_id_t;
#define	jit_thread_id_equal(x,y)	((x) == (y))
#else
typedef int jit_thread_id_t;
#define	jit_thread_id_equal(x,y)	((x) == (y))
#endif

/*
 * Control information that is associated with a thread.
 */
typedef struct jit_thread_control *jit_thread_control_t;

/*
 * Initialize the thread routines.  Ignored if called multiple times.
 */
void _jit_thread_init(void);

/*
 * Get the JIT control object for the current thread.
 */
jit_thread_control_t _jit_thread_get_control(void);

/*
 * Get the identifier for the current thread.
 */
jit_thread_id_t _jit_thread_current_id(void);

/*
 * Define the primitive mutex operations.
 */
#if defined(JIT_THREADS_PTHREAD)

typedef pthread_mutex_t jit_mutex_t;
#define	jit_mutex_create(mutex)		(pthread_mutex_init((mutex), 0))
#define	jit_mutex_destroy(mutex)	(pthread_mutex_destroy((mutex)))
#define	jit_mutex_lock(mutex)		(pthread_mutex_lock((mutex)))
#define	jit_mutex_unlock(mutex)		(pthread_mutex_unlock((mutex)))

#elif defined(JIT_THREADS_WIN32)

typedef CRITICAL_SECTION jit_mutex_t;
#define	jit_mutex_create(mutex)		(InitializeCriticalSection((mutex)))
#define	jit_mutex_destroy(mutex)	(DeleteCriticalSection((mutex)))
#define	jit_mutex_lock(mutex)		(EnterCriticalSection((mutex)))
#define	jit_mutex_unlock(mutex)		(LeaveCriticalSection((mutex)))

#else

typedef int jit_mutex_t;
#define	jit_mutex_create(mutex)		do { ; } while (0)
#define	jit_mutex_destroy(mutex)	do { ; } while (0)
#define	jit_mutex_lock(mutex)		do { ; } while (0)
#define	jit_mutex_unlock(mutex)		do { ; } while (0)

#endif

#ifdef	__cplusplus
};
#endif

#endif	/* _JIT_THREAD_H */
