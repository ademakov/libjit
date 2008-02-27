/*
 * dpas-semantics.h - Semantic value handling for Dynamic Pascal.
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

#ifndef	_DPAS_SEMANTICS_H
#define	_DPAS_SEMANTICS_H

#ifdef	__cplusplus
extern	"C" {
#endif

/*
 * Structure of a semantic value.
 */
typedef struct
{
	int			kind__;
	jit_type_t	type__;
	jit_value_t	value__;

} dpas_semvalue;

/*
 * Semantic value kinds.
 */
#define	DPAS_SEM_LVALUE			(1 << 0)
#define	DPAS_SEM_RVALUE			(1 << 1)
#define	DPAS_SEM_TYPE			(1 << 2)
#define	DPAS_SEM_PROCEDURE		(1 << 3)
#define	DPAS_SEM_ERROR			(1 << 4)
#define	DPAS_SEM_RETURN			(1 << 5)
#define	DPAS_SEM_LVALUE_EA		(1 << 6)
#define	DPAS_SEM_VOID			(1 << 7)
#define	DPAS_SEM_BUILTIN		(1 << 8)

/*
 * Set a semantic value to an l-value.
 */
#define	dpas_sem_set_lvalue(sem,type,value)	\
		do { \
			(sem).kind__ = DPAS_SEM_LVALUE | DPAS_SEM_RVALUE; \
			(sem).type__ = (type); \
			if(!(value)) \
			{ \
				dpas_out_of_memory(); \
			} \
			(sem).value__ = (value); \
		} while (0)

/*
 * Set a semantic value to an l-value that is actually an effective
 * address that must be dereferenced to get the actual l-value.
 */
#define	dpas_sem_set_lvalue_ea(sem,type,value)	\
		do { \
			(sem).kind__ = DPAS_SEM_LVALUE_EA | DPAS_SEM_RVALUE; \
			(sem).type__ = (type); \
			if(!(value)) \
			{ \
				dpas_out_of_memory(); \
			} \
			(sem).value__ = (value); \
		} while (0)

/*
 * Set a semantic value to an r-value.
 */
#define	dpas_sem_set_rvalue(sem,type,value)	\
		do { \
			(sem).kind__ = DPAS_SEM_RVALUE; \
			(sem).type__ = (type); \
			if(!(value)) \
			{ \
				dpas_out_of_memory(); \
			} \
			(sem).value__ = (value); \
		} while (0)

/*
 * Set a semantic value to a type.
 */
#define	dpas_sem_set_type(sem,type)	\
		do { \
			(sem).kind__ = DPAS_SEM_TYPE; \
			(sem).type__ = (type); \
			(sem).value__ = 0; \
		} while (0)

/*
 * Set a semantic value to a procedure/function reference.
 */
#define	dpas_sem_set_procedure(sem,type,item)	\
		do { \
			(sem).kind__ = DPAS_SEM_PROCEDURE; \
			(sem).type__ = (type); \
			(sem).value__ = (jit_value_t)(item); \
		} while (0)

/*
 * Set a semantic value to an error.
 */
#define	dpas_sem_set_error(sem)	\
		do { \
			(sem).kind__ = DPAS_SEM_ERROR; \
			(sem).type__ = 0; \
			(sem).value__ = 0; \
		} while (0)

/*
 * Set a semantic value to indicate the return slot.
 */
#define	dpas_sem_set_return(sem,type)	\
		do { \
			(sem).kind__ = DPAS_SEM_RETURN; \
			(sem).type__ = (type); \
			(sem).value__ = 0; \
		} while (0)

/*
 * Set a semantic value to indicate "void" for a procedure return.
 */
#define	dpas_sem_set_void(sem)	\
		do { \
			(sem).kind__ = DPAS_SEM_VOID; \
			(sem).type__ = jit_type_void; \
			(sem).value__ = 0; \
		} while (0)

/*
 * Set a semantic value to indicate a builtin function.
 */
#define	dpas_sem_set_builtin(sem,value)	\
		do { \
			(sem).kind__ = DPAS_SEM_BUILTIN; \
			(sem).type__ = jit_type_void; \
			(sem).value__ = (jit_value_t)(jit_nint)(value); \
		} while (0)

/*
 * Determine if a semantic value has a specific kind.
 */
#define	dpas_sem_is_lvalue(sem)		(((sem).kind__ & DPAS_SEM_LVALUE) != 0)
#define	dpas_sem_is_lvalue_ea(sem)	(((sem).kind__ & DPAS_SEM_LVALUE_EA) != 0)
#define	dpas_sem_is_rvalue(sem)		(((sem).kind__ & DPAS_SEM_RVALUE) != 0)
#define	dpas_sem_is_type(sem)		(((sem).kind__ & DPAS_SEM_TYPE) != 0)
#define	dpas_sem_is_procedure(sem)	(((sem).kind__ & DPAS_SEM_PROCEDURE) != 0)
#define	dpas_sem_is_error(sem)		(((sem).kind__ & DPAS_SEM_ERROR) != 0)
#define	dpas_sem_is_return(sem)		(((sem).kind__ & DPAS_SEM_RETURN) != 0)
#define	dpas_sem_is_void(sem)		(((sem).kind__ & DPAS_SEM_VOID) != 0)
#define	dpas_sem_is_builtin(sem)	(((sem).kind__ & DPAS_SEM_BUILTIN) != 0)

/*
 * Extract the type information from a semantic value.
 */
#define	dpas_sem_get_type(sem)		((sem).type__)

/*
 * Extract the value information from a semantic value.
 */
#define	dpas_sem_get_value(sem)		((sem).value__)

/*
 * Extract the procedure information from a semantic value.
 */
#define	dpas_sem_get_procedure(sem)	((dpas_scope_item_t)((sem).value__))

/*
 * Extract the builtin function information from a semantic value.
 */
#define	dpas_sem_get_builtin(sem)	((int)(jit_nint)((sem).value__))

/*
 * Convert an l-value effective address into a plain r-value.
 */
dpas_semvalue dpas_lvalue_to_rvalue(dpas_semvalue value);

#ifdef	__cplusplus
};
#endif

#endif	/* _DPAS_SEMANTICS_H */
