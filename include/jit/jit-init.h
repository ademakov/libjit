/*
 * jit-init.h - Initialization routines for the JIT.
 *
 * Copyright (C) 2004  Southern Storm Software, Pty Ltd.
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

#ifndef	_JIT_INIT_H
#define	_JIT_INIT_H

#include <jit/jit-defs.h>

#ifdef	__cplusplus
extern	"C" {
#endif

void jit_init(void) JIT_NOTHROW;
int jit_uses_interpreter(void) JIT_NOTHROW;

#ifdef	__cplusplus
};
#endif

#endif /* _JIT_INIT_H */
