/*
 * unit-tests.h - Defines for unit tests.
 *
 * Copyright (C) 2020 Free Software Foundation
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

#ifndef _JIT_TESTS_UNIT_TESTS_H
#define _JIT_TESTS_UNIT_TESTS_H

#include <stdio.h>
#include <stdlib.h>

/* Convenience.  */
#include <jit/jit-dump.h>

#define CHECK(val)							\
	do {								\
		if (!(val)) {						\
			fprintf (stderr, "%s:%d: error: test failure\n", \
				 __FILE__, __LINE__);			\
			exit (1);					\
		}							\
	} while (0)

#endif /* _JIT_TESTS_UNIT_TESTS_H */
