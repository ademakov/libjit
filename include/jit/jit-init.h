/*
 * jit-init.h - Initialization routines for the JIT.
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
