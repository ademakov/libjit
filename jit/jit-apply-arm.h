/*
 * jit-apply-arm.h - Special definitions for ARM function application.
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

#ifndef	_JIT_APPLY_ARM_H
#define	_JIT_APPLY_ARM_H

/*
 * The maximum number of bytes that are needed to represent a closure,
 * and the alignment to use for the closure.
 */
#define	jit_closure_size		128
#define	jit_closure_align		16

/*
 * The number of bytes that are needed for a redirector stub.
 * This includes any extra bytes that are needed for alignment.
 */
#define	jit_redirector_size		128

#endif	/* _JIT_APPLY_ARM_H */
