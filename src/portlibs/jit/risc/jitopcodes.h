/*
 * @(#)jitopcodes.h	1.82 06/10/10
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.  
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER  
 *   
 * This program is free software; you can redistribute it and/or  
 * modify it under the terms of the GNU General Public License version  
 * 2 only, as published by the Free Software Foundation.   
 *   
 * This program is distributed in the hope that it will be useful, but  
 * WITHOUT ANY WARRANTY; without even the implied warranty of  
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU  
 * General Public License version 2 for more details (a copy is  
 * included at /legal/license.txt).   
 *   
 * You should have received a copy of the GNU General Public License  
 * version 2 along with this work; if not, write to the Free Software  
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  
 * 02110-1301 USA   
 *   
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa  
 * Clara, CA 95054 or visit www.sun.com if you need additional  
 * information or have any questions. 
 *
 */

#ifndef _INCLUDED_JITOPCODES_H
#define _INCLUDED_JITOPCODES_H

#include "javavm/include/jit/jitir.h"

/* Define additional node typetags that will be used by the grammar rules: */
#define CVMJITCG_TYPEID_64BITS_VOLATILE (CVMJIT_TYPEID_LAST_PREDEFINED_TYPE + 1)

/*
 * CVMJITgetMassagedIROpcode - Massage IR opcodes to forms that fit
 * the needs of the jit backend.
 */
extern int
CVMJITgetMassagedIROpcode(CVMJITCompilationContext *con, CVMJITIRNode *ip);

/*
 * This file contains the shorthand names for opcode values that the
 * IR-to-codegen parser will use.
 *
 * In order to make IR parsing easier and faster, it is based
 * totally on syntax. Sometimes, we want to look at semantics, too.
 * This is what CVMGCGetMassagedIROpcode does for us: it maps
 * the IR node opcode space into a slightly different space. For example,
 * many machines can use small constants as immediate operands, where
 * the meaning of "small" is very machine dependent. On the other hand,
 * sometimes the difference between data types is uninteresting, if all
 * we care about is the size.
 *
 * This is the version common to RISC CPUs.
 */

#define CVMJIT_ENCODE_LUNARY(opcodeTag) \
	CVMJIT_TYPE_ENCODE(opcodeTag, CVMJIT_UNARY_NODE, CVM_TYPEID_LONG)
#define CVMJIT_ENCODE_LBINARY(opcodeTag) \
	CVMJIT_TYPE_ENCODE(opcodeTag, CVMJIT_BINARY_NODE, CVM_TYPEID_LONG)
#define CVMJIT_ENCODE_FUNARY(opcodeTag) \
	CVMJIT_TYPE_ENCODE(opcodeTag, CVMJIT_UNARY_NODE, CVM_TYPEID_FLOAT)
#define CVMJIT_ENCODE_FBINARY(opcodeTag) \
	CVMJIT_TYPE_ENCODE(opcodeTag, CVMJIT_BINARY_NODE, CVM_TYPEID_FLOAT)
#define CVMJIT_ENCODE_DUNARY(opcodeTag) \
	CVMJIT_TYPE_ENCODE(opcodeTag, CVMJIT_UNARY_NODE, CVM_TYPEID_DOUBLE)
#define CVMJIT_ENCODE_DBINARY(opcodeTag) \
	CVMJIT_TYPE_ENCODE(opcodeTag, CVMJIT_BINARY_NODE, CVM_TYPEID_DOUBLE)

/* The first several are standard. We can break this up soon */

#define NULLCHECK       CVMJIT_ENCODE_NULL_CHECK
#define ISEQUENCE_R	CVMJIT_ENCODE_SEQUENCE_R(CVM_TYPEID_INT)
#define LSEQUENCE_R	CVMJIT_ENCODE_SEQUENCE_R(CVM_TYPEID_LONG)
#define FSEQUENCE_R	CVMJIT_ENCODE_SEQUENCE_R(CVM_TYPEID_FLOAT)
#define DSEQUENCE_R	CVMJIT_ENCODE_SEQUENCE_R(CVM_TYPEID_DOUBLE)
#define VSEQUENCE_R	CVMJIT_ENCODE_SEQUENCE_R(CVM_TYPEID_NONE)
#define ISEQUENCE_L	CVMJIT_ENCODE_SEQUENCE_L(CVM_TYPEID_INT)
#define LSEQUENCE_L	CVMJIT_ENCODE_SEQUENCE_L(CVM_TYPEID_LONG)
#define FSEQUENCE_L	CVMJIT_ENCODE_SEQUENCE_L(CVM_TYPEID_FLOAT)
#define DSEQUENCE_L	CVMJIT_ENCODE_SEQUENCE_L(CVM_TYPEID_DOUBLE)
#define VSEQUENCE_L	CVMJIT_ENCODE_SEQUENCE_L(CVM_TYPEID_NONE)
#define VENDINLINING	CVMJIT_ENCODE_END_INLINING_LEAF
#define LOCAL32		CVMJIT_ENCODE_ILOCAL
#define LOCAL64		CVMJIT_ENCODE_LOCAL(CVM_TYPEID_LONG)
#define STATIC32	CVMJIT_ENCODE_STATIC_FIELD_REF(CVM_TYPEID_INT)
#define STATIC64	CVMJIT_ENCODE_STATIC_FIELD_REF(CVM_TYPEID_LONG)
#define STATIC64VOL	CVMJIT_ENCODE_STATIC_FIELD_REF(CVMJITCG_TYPEID_64BITS_VOLATILE)
#define INDEX   	CVMJIT_ENCODE_INDEX
#define BOUNDS_CHECK	CVMJIT_ENCODE_BOUNDS_CHECK
#define FIELDREFOBJ     CVMJIT_ENCODE_FIELD_REF(CVM_TYPEID_OBJ)
#define FIELDREF32	CVMJIT_ENCODE_FIELD_REF(CVM_TYPEID_INT)
#define FIELDREF64	CVMJIT_ENCODE_FIELD_REF(CVM_TYPEID_LONG)
#define FIELDREF64VOL	CVMJIT_ENCODE_FIELD_REF(CVMJITCG_TYPEID_64BITS_VOLATILE)
#define EXCEPTION_OBJECT CVMJIT_ENCODE_EXCEPTION_OBJECT
#define DEFINE_VALUE32	CVMJIT_ENCODE_DEFINE(CVM_TYPEID_INT)
#define DEFINE_VALUE64	CVMJIT_ENCODE_DEFINE(CVM_TYPEID_LONG)
#define FDEFINE		CVMJIT_ENCODE_DEFINE(CVM_TYPEID_FLOAT)
#define DDEFINE		CVMJIT_ENCODE_DEFINE(CVM_TYPEID_DOUBLE)
#define LOAD_PHIS       CVMJIT_ENCODE_LOAD_PHIS
#define RELEASE_PHIS    CVMJIT_ENCODE_RELEASE_PHIS
#define ICONSTANT32	CVMJIT_ENCODE_CONST_JAVA_NUMERIC32(CVM_TYPEID_INT)
#define ALENGTH		CVMJIT_ENCODE_ARRAY_LENGTH
#define INEG32		CVMJIT_ENCODE_IUNARY(CVMJIT_NEG)
#define NOT32	        CVMJIT_ENCODE_IUNARY(CVMJIT_NOT)
#define INT2BIT32	CVMJIT_ENCODE_IUNARY(CVMJIT_INT2BIT)
#define IADD32		CVMJIT_ENCODE_IBINARY(CVMJIT_ADD)
#define ISUB32		CVMJIT_ENCODE_IBINARY(CVMJIT_SUB)
#define IMUL32		CVMJIT_ENCODE_IBINARY(CVMJIT_MUL)
#define IDIV32		CVMJIT_ENCODE_IBINARY(CVMJIT_DIV)
#define IREM32		CVMJIT_ENCODE_IBINARY(CVMJIT_REM)
#define AND32		CVMJIT_ENCODE_IBINARY(CVMJIT_AND)
#define OR32		CVMJIT_ENCODE_IBINARY(CVMJIT_OR)
#define XOR32		CVMJIT_ENCODE_IBINARY(CVMJIT_XOR)
#define SLL32		CVMJIT_ENCODE_IBINARY(CVMJIT_SHL)
#define SRL32		CVMJIT_ENCODE_IBINARY(CVMJIT_USHR)
#define SRA32		CVMJIT_ENCODE_IBINARY(CVMJIT_SHR)

#define INEG64		CVMJIT_ENCODE_LUNARY(CVMJIT_NEG)
#define IADD64		CVMJIT_ENCODE_LBINARY(CVMJIT_ADD)
#define ISUB64		CVMJIT_ENCODE_LBINARY(CVMJIT_SUB)
#define IMUL64		CVMJIT_ENCODE_LBINARY(CVMJIT_MUL)
#define IDIV64		CVMJIT_ENCODE_LBINARY(CVMJIT_DIV)
#define IREM64		CVMJIT_ENCODE_LBINARY(CVMJIT_REM)
#define AND64		CVMJIT_ENCODE_LBINARY(CVMJIT_AND)
#define OR64		CVMJIT_ENCODE_LBINARY(CVMJIT_OR)
#define XOR64		CVMJIT_ENCODE_LBINARY(CVMJIT_XOR)
#define SLL64		CVMJIT_ENCODE_LBINARY(CVMJIT_SHL)
#define SRL64		CVMJIT_ENCODE_LBINARY(CVMJIT_USHR)
#define SRA64		CVMJIT_ENCODE_LBINARY(CVMJIT_SHR)

#define FNEG            CVMJIT_ENCODE_FUNARY(CVMJIT_NEG)
#define FADD            CVMJIT_ENCODE_FBINARY(CVMJIT_ADD)
#define FSUB            CVMJIT_ENCODE_FBINARY(CVMJIT_SUB)
#define FMUL            CVMJIT_ENCODE_FBINARY(CVMJIT_MUL)
#define FDIV            CVMJIT_ENCODE_FBINARY(CVMJIT_DIV)
#define FREM            CVMJIT_ENCODE_FBINARY(CVMJIT_REM)

#define DNEG            CVMJIT_ENCODE_DUNARY(CVMJIT_NEG)
#define DADD            CVMJIT_ENCODE_DBINARY(CVMJIT_ADD)
#define DSUB            CVMJIT_ENCODE_DBINARY(CVMJIT_SUB)
#define DMUL            CVMJIT_ENCODE_DBINARY(CVMJIT_MUL)
#define DDIV            CVMJIT_ENCODE_DBINARY(CVMJIT_DIV)
#define DREM            CVMJIT_ENCODE_DBINARY(CVMJIT_REM)

#define INSTANCEOF	CVMJIT_ENCODE_INSTANCEOF

#ifdef CVM_JIT_USE_FP_HARDWARE
#define FINVOKE		CVMJIT_ENCODE_INVOKE(CVM_TYPEID_FLOAT)
#define DINVOKE		CVMJIT_ENCODE_INVOKE(CVM_TYPEID_DOUBLE)
#endif
#define IINVOKE		CVMJIT_ENCODE_INVOKE(CVM_TYPEID_INT)
#define LINVOKE		CVMJIT_ENCODE_INVOKE(CVM_TYPEID_LONG)
#define VINVOKE		CVMJIT_ENCODE_INVOKE(CVM_TYPEID_VOID)

#define VINTRINSIC      CVMJIT_ENCODE_INTRINSIC(CVM_TYPEID_VOID)
#define INTRINSIC32     CVMJIT_ENCODE_INTRINSIC(CVM_TYPEID_INT)
#define INTRINSIC64     CVMJIT_ENCODE_INTRINSIC(CVM_TYPEID_LONG)
#define IARG            CVMJIT_ENCODE_IARG(CVM_TYPEID_VOID)
#define NULL_IARG       CVMJIT_ENCODE_NULL_IARG

/* conversion operators. */
#define I2C             CVMJIT_TYPE_ENCODE(CVMJIT_CONVERT_I2C, \
                                CVMJIT_UNARY_NODE, CVM_TYPEID_INT)
#define I2S             CVMJIT_TYPE_ENCODE(CVMJIT_CONVERT_I2S, \
                                CVMJIT_UNARY_NODE, CVM_TYPEID_INT)
#define I2B             CVMJIT_TYPE_ENCODE(CVMJIT_CONVERT_I2B, \
                                CVMJIT_UNARY_NODE, CVM_TYPEID_INT)
#define I2L		CVMJIT_TYPE_ENCODE(CVMJIT_CONVERT_INTEGER, \
				CVMJIT_UNARY_NODE, CVM_TYPEID_LONG)
#define I2F		CVMJIT_TYPE_ENCODE(CVMJIT_CONVERT_INTEGER, \
				CVMJIT_UNARY_NODE, CVM_TYPEID_FLOAT)
#define I2D		CVMJIT_TYPE_ENCODE(CVMJIT_CONVERT_INTEGER, \
				CVMJIT_UNARY_NODE, CVM_TYPEID_DOUBLE)
#define L2I		CVMJIT_TYPE_ENCODE(CVMJIT_CONVERT_LONG, \
				CVMJIT_UNARY_NODE, CVM_TYPEID_INT)
#define L2F		CVMJIT_TYPE_ENCODE(CVMJIT_CONVERT_LONG, \
				CVMJIT_UNARY_NODE, CVM_TYPEID_FLOAT)
#define L2D		CVMJIT_TYPE_ENCODE(CVMJIT_CONVERT_LONG, \
				CVMJIT_UNARY_NODE, CVM_TYPEID_DOUBLE)
#define F2D             CVMJIT_TYPE_ENCODE(CVMJIT_CONVERT_FLOAT, \
                                CVMJIT_UNARY_NODE, CVM_TYPEID_DOUBLE)
#define F2I             CVMJIT_TYPE_ENCODE(CVMJIT_CONVERT_FLOAT, \
                                CVMJIT_UNARY_NODE, CVM_TYPEID_INT)
#define F2L             CVMJIT_TYPE_ENCODE(CVMJIT_CONVERT_FLOAT, \
                                CVMJIT_UNARY_NODE, CVM_TYPEID_LONG)
#define D2F             CVMJIT_TYPE_ENCODE(CVMJIT_CONVERT_DOUBLE, \
                                CVMJIT_UNARY_NODE, CVM_TYPEID_FLOAT)
#define D2I             CVMJIT_TYPE_ENCODE(CVMJIT_CONVERT_DOUBLE, \
                                CVMJIT_UNARY_NODE, CVM_TYPEID_INT)
#define D2L             CVMJIT_TYPE_ENCODE(CVMJIT_CONVERT_DOUBLE, \
                                CVMJIT_UNARY_NODE, CVM_TYPEID_LONG)

/*
 * Address-like constant's
 */
#define METHOD_BLOCK	    CVMJIT_ENCODE_CONST_MB
#define CLASS_BLOCK  	    CVMJIT_ENCODE_CONST_CB
#define STRING_ICELL_CONST  CVMJIT_ENCODE_CONST_STRING_ICELL
#define NEW_OBJECT	    CVMJIT_ENCODE_NEW_OBJECT
#define NEW_ARRAY_REF	    CVMJIT_ENCODE_NEW_ARRAY_REF
#define MULTI_NEW_ARRAY_REF CVMJIT_ENCODE_MULTIANEW_ARRAY
#define NEW_ARRAY_BASIC	    CVMJIT_ENCODE_NEW_ARRAY_BASIC(CVM_T_INT)
#define GET_VTBL	    CVMJIT_ENCODE_GET_VTBL
#define GET_ITBL	    CVMJIT_ENCODE_GET_ITBL
#define FETCH_MB_FROM_VTABLE CVMJIT_ENCODE_FETCH_MB_FROM_VTABLE
#define FETCH_MB_FROM_ITABLE CVMJIT_ENCODE_FETCH_MB_FROM_INTERFACETABLE

/* #ifdef IAI_VIRTUAL_INLINE_CB_TEST */
#define FETCH_VCB CVMJIT_ENCODE_FETCH_VCB
#define FETCH_MB_FROM_VTABLE_OUTOFLINE	 \
	CVMJIT_ENCODE_FETCH_MB_FROM_VTABLE_OUTOFLINE	
#define MB_TEST_OUTOFLINE	CVMJIT_ENCODE_MB_TEST_OUTOFLINE
/* #endif */

/* BCOND binary nodes */
#define BCOND_INT      	CVMJIT_ENCODE_BCOND(CVM_TYPEID_INT)
#define BCOND_LONG      CVMJIT_ENCODE_BCOND(CVM_TYPEID_LONG)
#define BCOND_FLOAT     CVMJIT_ENCODE_BCOND(CVM_TYPEID_FLOAT)
#define BCOND_DOUBLE    CVMJIT_ENCODE_BCOND(CVM_TYPEID_DOUBLE)

/* generic. should be typeless. */
#define TABLESWITCH     CVMJIT_ENCODE_TABLESWITCH
#define LOOKUPSWITCH    CVMJIT_ENCODE_LOOKUPSWITCH
#define CHECKCAST	CVMJIT_ENCODE_CHECKCAST

#define JSR		CVMJIT_ENCODE_BRANCH(CVMJIT_JSR)
#define JSR_RETURN_ADDRESS CVMJIT_ENCODE_JSR_RETURN_ADDRESS
#define RET		CVMJIT_ENCODE_RET

#define GOTO		CVMJIT_ENCODE_BRANCH(CVMJIT_GOTO)
#define RETURN		CVMJIT_ENCODE_RETURN
#define IRETURN		CVMJIT_ENCODE_IRETURN
#define FRETURN		CVMJIT_ENCODE_FRETURN
#define ARETURN		CVMJIT_ENCODE_ARETURN
#define LRETURN         CVMJIT_ENCODE_LRETURN
#define DRETURN         CVMJIT_ENCODE_DRETURN
#define ATHROW		CVMJIT_ENCODE_ATHROW

/* for some things, we care only about size,
   so simplify by throwing away type info */
#define FETCH32		CVMJIT_ENCODE_FETCH(CVM_TYPEID_INT)
#define FETCH64		CVMJIT_ENCODE_FETCH(CVM_TYPEID_LONG)

#define IDENT32		CVMJIT_ENCODE_IUNARY(CVMJIT_IDENTITY)
#define IDENT64		CVMJIT_ENCODE_LUNARY(CVMJIT_IDENTITY)
#define FIDENT		CVMJIT_ENCODE_FUNARY(CVMJIT_IDENTITY)
#define DIDENT		CVMJIT_ENCODE_DUNARY(CVMJIT_IDENTITY)

#define USED32		CVMJIT_ENCODE_USED(CVM_TYPEID_INT)
#define USED64		CVMJIT_ENCODE_USED(CVM_TYPEID_LONG)
#define FUSED		CVMJIT_ENCODE_USED(CVM_TYPEID_FLOAT)
#define DUSED		CVMJIT_ENCODE_USED(CVM_TYPEID_DOUBLE)

/* for some other things, we care much more about specific values */
/* Use values that will not be in the usual opcode space */
#define ICONST_32	ICONSTANT32
#define ICONST_64	CVMJIT_ENCODE_CONST_JAVA_NUMERIC64(CVM_TYPEID_LONG)

/* some things seem to have no type at all. */
#define	ASSIGN		CVMJIT_ENCODE_ASSIGN(CVM_TYPEID_NONE)

#ifdef CVM_JIT_USE_FP_HARDWARE
#define FPARAMETER	CVMJIT_ENCODE_PARAMETER(CVM_TYPEID_FLOAT)
#define DPARAMETER	CVMJIT_ENCODE_PARAMETER(CVM_TYPEID_DOUBLE)
#endif
#define IPARAMETER	CVMJIT_ENCODE_PARAMETER(CVM_TYPEID_INT)
#define LPARAMETER	CVMJIT_ENCODE_PARAMETER(CVM_TYPEID_LONG)
#define NULL_PARAMETER	CVMJIT_ENCODE_NULL_PARAMETER

#define MONITOR_ENTER	CVMJIT_ENCODE_MONITOR_ENTER
#define MONITOR_EXIT	CVMJIT_ENCODE_MONITOR_EXIT

#define MAP_PC		CVMJIT_ENCODE_MAP_PC

#define BEGININLINING        CVMJIT_ENCODE_BEGIN_INLINING
#define OUTOFLINEINVOKE      CVMJIT_ENCODE_OUTOFLINE_INVOKE

#define RESOLVE_REF     CVMJIT_ENCODE_RESOLVE_REFERENCE
#define CHECKINIT       CVMJIT_ENCODE_CLASS_CHECKINIT

#define LCMP            CVMJIT_ENCODE_IBINARY(CVMJIT_LCMP)
#define FCMPL           CVMJIT_ENCODE_IBINARY(CVMJIT_FCMPL)
#define FCMPG           CVMJIT_ENCODE_IBINARY(CVMJIT_FCMPG)
#define DCMPL           CVMJIT_ENCODE_IBINARY(CVMJIT_DCMPL)
#define DCMPG           CVMJIT_ENCODE_IBINARY(CVMJIT_DCMPG)

#endif /* _INCLUDED_JITOPCODES_H */
