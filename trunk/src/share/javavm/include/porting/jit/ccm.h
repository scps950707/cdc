/*
 * @(#)ccm.h	1.17 06/10/10
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

/*
 * Porting layer for CCM functions.
 */

#ifndef _INCLUDED_CCM_H
#define _INCLUDED_CCM_H

/*
 * Start and end of code we copy into the code cache for faster access.
 */
extern char CVMCCMcodeCacheCopyStart;
extern char CVMCCMcodeCacheCopyEnd;

/*
 * The following #define/#undef are used to determine whether to include the
 * shard C implementations of these CCM runtime helpers in the build
 * or not.  If #undef'ed, then the shared C version will be built in.  If
 * #define'd, then a cpu specific implementation will be used instead.
 *
 * The assumption is that none will be defined. If you provide a cpu 
 * specific implementation for a CCM function, then #define the
 * appropriate symbol in ccm_cpu.h
 */

/*
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_IDIV
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_IREM
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_LMUL
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_LDIV
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_LREM
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_LNEG
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_LSHL
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_LSHR
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_LUSHR
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_LAND
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_LOR
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_LXOR
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_FADD
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_FSUB
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_FMUL
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_FDIV
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_FREM
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_FNEG
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_DADD
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_DSUB
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_DMUL
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_DDIV
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_DREM
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_DNEG
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_I2L
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_I2F
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_I2D
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_L2I
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_L2F
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_L2D
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_F2I
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_F2L
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_F2D
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_D2I
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_D2L
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_D2F
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_LCMP
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_FCMP
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_DCMPL
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_DCMPG
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_THROW_CLASS
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_THROW_OBJECT
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_CHECKCAST
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_CHECK_ARRAY_ASSIGNABLE
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_INSTANCEOF
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_LOOKUP_INTEFACE_MB
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_NEW
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_NEWARRAY
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_ANEWARRAY
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_MULTIANEWARRAY
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_RUN_CLASS_INITIALIZER
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_RESOLVE_CLASS_BLOCK
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_RESOLVE_ARRAY_CLASS_BLOCK
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_RESOLVE_GETFIELD_OFFSET
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_RESOLVE_PUTFIELD_OFFSET
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_RESOLVE_METHOD_BLOCK
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_RESOLVE_SPECIAL_METHOD_BLOCK
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_RESOLVE_METHOD_TABLE_OFFSET
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_RESOLVE_NEW_CLASS_BLOCK_AND_CLINIT
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_RESOLVE_GETSTATIC_FB_AND_CLINIT
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_RESOLVE_PUTSTATIC_FB_AND_CLINIT
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_RESOLVE_STATIC_MB_AND_CLINIT
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_MONITOR_ENTER
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_MONITOR_EXIT
#undef  CVMCCM_HAVE_PLATFORM_SPECIFIC_GC_RENDEZVOUS
*/

/*
 * The following #define/#undef are used to determine whether to include the
 * shared C implementations of these CCM intrinsic helpers in the build
 * or not.  If #undef'ed, then the shared C version will be built in.  If
 * #define'd, then the target platform gets to define its own.  If the
 * the target platform does not define its own version, then the respective
 * method will not be regarded as an intrinsic.
 * NOTE: #undef the respective symbol is also a method for choosing not to
 *       implement the corresponding method as an intrinsic in a port.  The
 *       target platform is not required to provide one.  This just prevents
 *       the shared version from built in.
 * NOTE: The target platform can always add intrinsics that are not in this
 *       shared list as well.
 */
/*
#undef  CVMCCM_DISABLE_SHARED_STRICTMATH_COS_INTRINSIC
#undef  CVMCCM_DISABLE_SHARED_STRICTMATH_SIN_INTRINSIC
#undef  CVMCCM_DISABLE_SHARED_STRICTMATH_TAN_INTRINSIC
#undef  CVMCCM_DISABLE_SHARED_STRICTMATH_ACOS_INTRINSIC
#undef  CVMCCM_DISABLE_SHARED_STRICTMATH_ASIN_INTRINSIC
#undef  CVMCCM_DISABLE_SHARED_STRICTMATH_ATAN_INTRINSIC
#undef  CVMCCM_DISABLE_SHARED_STRICTMATH_EXP_INTRINSIC
#undef  CVMCCM_DISABLE_SHARED_STRICTMATH_LOG_INTRINSIC
#undef  CVMCCM_DISABLE_SHARED_STRICTMATH_SQRT_INTRINSIC
#undef  CVMCCM_DISABLE_SHARED_STRICTMATH_CEIL_INTRINSIC
#undef  CVMCCM_DISABLE_SHARED_STRICTMATH_FLOOR_INTRINSIC
#undef  CVMCCM_DISABLE_SHARED_STRICTMATH_ATAN2_INTRINSIC
#undef  CVMCCM_DISABLE_SHARED_STRICTMATH_POW_INTRINSIC
#undef  CVMCCM_DISABLE_SHARED_STRICTMATH_IEEEREMAINDER_INTRINSIC
#undef  CVMCCM_DISABLE_SHARED_STRICTMATH_RINT_INTRINSIC
#undef  CVMCCM_DISABLE_SHARED_STRING_CHARAT_INTRINSIC
#undef  CVMCCM_DISABLE_SHARED_STRING_COMPARETO_INTRINSIC
#undef  CVMCCM_DISABLE_SHARED_STRING_EQUALS_INTRINSIC
#undef  CVMCCM_DISABLE_SHARED_STRING_GETCHARS_INTRINSIC
#undef  CVMCCM_DISABLE_SHARED_SYSTEM_CURRENTTIMEMILLIS_INTRINSIC
#undef  CVMCCM_DISABLE_SHARED_SYSTEM_ARRAYCOPY_INTRINSIC
#undef  CVMCCM_DISABLE_SHARED_THREAD_CURRENTTHREAD_INTRINSIC

#undef  CVMCCM_DISABLE_SHARED_CVM_COPYBOOLEANARRAY_INTRINSIC
#undef  CVMCCM_DISABLE_SHARED_CVM_COPYBYTEARRAY_INTRINSIC
#undef  CVMCCM_DISABLE_SHARED_CVM_COPYSHORTARRAY_INTRINSIC
#undef  CVMCCM_DISABLE_SHARED_CVM_COPYCHARARRAY_INTRINSIC
#undef  CVMCCM_DISABLE_SHARED_CVM_COPYINTARRAY_INTRINSIC
#undef  CVMCCM_DISABLE_SHARED_CVM_COPYLONGARRAY_INTRINSIC
#undef  CVMCCM_DISABLE_SHARED_CVM_COPYFLOATARRAY_INTRINSIC
#undef  CVMCCM_DISABLE_SHARED_CVM_COPYDOUBLEARRAY_INTRINSIC
#undef  CVMCCM_DISABLE_SHARED_CVM_COPYOBJECTARRAY_INTRINSIC
*/

/* Invokers of methods from compiled code */
extern void
CVMCCMinvokeJNIMethod(CVMMethodBlock*);

extern void
CVMCCMinvokeCNIMethod(CVMMethodBlock*);

extern void
CVMCCMletInterpreterDoInvoke(CVMMethodBlock*);

extern void
CVMCCMletInterpreterDoInvokeWithoutFlushRetAddr(CVMMethodBlock*);

#ifdef CVM_JIT_PATCHED_METHOD_INVOCATIONS
extern void
CVMCCMinvokeVirtual();
#endif

#endif /* _INCLUDED_CCM_H */
