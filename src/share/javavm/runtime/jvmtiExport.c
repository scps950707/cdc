/*
 * @(#)jvmtiExport.c	1.7 06/10/31
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
 * Breakpoints, single-stepping and debugger event notification.
 * Debugger event notification/handling includes breakpoint, single-step
 * and exceptions.
 */

/*
 * This file is derived from the original CVM jvmdi.c file.  In addition
 * the 'jvmti' components of this file are derived from the J2SE
 * jvmtiExport.cpp class.  The primary purpose of this file is to
 * export VM events to external libraries.
 */

#ifdef CVM_JVMTI

#include "javavm/include/porting/ansi/stdarg.h"
#include "javavm/include/defs.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/globalroots.h"
#include "javavm/include/localroots.h"
#include "javavm/include/interpreter.h"
#include "javavm/include/basictypes.h"
#include "javavm/include/signature.h"
#include "javavm/include/globals.h"
#include "javavm/include/bag.h"
#include "javavm/include/common_exceptions.h"
#include "javavm/include/named_sys_monitor.h"
#include "generated/javavm/include/opcodes.h"
#include "generated/offsets/java_lang_Thread.h"
#include "generated/jni/java_lang_reflect_Modifier.h"
#include "javavm/export/jvm.h"
#include "javavm/export/jni.h"

#include "javavm/include/jvmtiExport.h"
#include "javavm/include/jvmtiDumper.h"
#include "javavm/include/jvmtiCapabilities.h"

#ifdef CVM_JIT
#include "javavm/include/jit/jitcodebuffer.h"
#endif

static jboolean _have_pending_compiled_method_unload_events;        
jvmtiPhase _jvmtiphase;
jlong clks_per_sec;
CVMBool _debuggerConnected = CVM_FALSE;

/* %comment: k001 */


/* (This is unused in CVM -- only used in unimplemented GetBytecodes) */
/* Defined in inline.c thru an include of "opcodes.length". This is dangerous
   as is. There should be a header file with this extern shared by both .c files */
/* extern const short opcode_length[256]; */


/* Convenience macros */
/* Purpose: Checks to see if it is safe to access object pointers directly. */
#define CVMjvmtiIsSafeToAccessDirectObject(ee_)       \
    (CVMD_isgcUnsafe(ee_) || CVMgcIsGCThread(ee_) ||  \
     CVMsysMutexIAmOwner(ee_, &CVMglobals.heapLock))

/* NOTE: "Frame pop sentinels" removed in CVM version of JVMTI
   because we don't have two return PC slots and mangling the only
   available one is incorrect if an exception is thrown. See
   CVMjvmtiNotifyDebuggerOfFramePop and jvmti_NotifyFramePop below. */

/* #define FRAME_POP_SENTINEL ((unsigned char *)1) */

#define INITIAL_BAG_SIZE 4

/* additional local frames to push to cover usage by client debugger */
#define LOCAL_FRAME_SLOP 10

#define JVMTI_EVENT_GLOBAL_MASK 0xf0000000
#define JVMTI_EVENT_THREAD_MASK 0x7fffffff

/* Purpose: Gets the ClassBlock from the specified java.lang.Class instance. */
const CVMClassBlock *
CVMjvmtiClassInstance2ClassBlock(CVMExecEnv *ee, CVMObject *obj)
{
    CVMJavaInt cbAddr;
    CVMClassBlock *cb;

    /* The following condition will have to be true to guarantee that the
       CVMObject *obj that is passed in remains valid for the duration of this
       function, and that we can access the
       CVMoffsetOfjava_lang_Class_classBlockPointer directly below: */
    CVMassert(CVMjvmtiIsSafeToAccessDirectObject(ee));

    CVMD_fieldReadInt(obj,
        CVMoffsetOfjava_lang_Class_classBlockPointer, cbAddr);
    cb = (CVMClassBlock*) cbAddr;
    CVMassert(cb != NULL);

    return cb;
}

void CVMjvmtiEnableThreadEvents(CVMExecEnv* ee, ThreadNode *node, 
				jvmtiEvent eventType, jboolean enabled) {

    CVMassert(JVMTI_IS_LOCKED(ee));

    /*
     * If state is changing, update the global eventEnable word
     * for this thread and the per-thread boolean flag.
     */
    if (node->eventEnabled[eventType] != enabled) {
	node->eventEnabled[eventType] = enabled;
    }
}

static jboolean threadEnabled(CVMExecEnv *ee, jint eventType) {
    return (ee == NULL) ?
	JNI_FALSE :
	(ee->_jvmti_event_enabled._enabled_bits &
	 (((jlong)1) << (eventType - TOTAL_MIN_EVENT_TYPE_VAL))) != 0;
}

#define GLOBALLY_ENABLED(jvmti_env, eventType)				\
    ((jvmti_env)->_env_event_enable._event_enabled._enabled_bits & (1 << (eventType - TOTAL_MIN_EVENT_TYPE_VAL)))

/*
 * This macro is used in notify_debugger_* to determine whether a JVMTI 
 * event should be generated. Note that this takes an expression for the thread node
 * which we should avoid evaluating twice. That's why threadEnabled above
 * is a function, not a macro
 *
 * Also the first eventEnable[eventType] is redundant, but it serves as
 * a quick filter of events that are completely unreported.
 */
#define MUST_NOTIFY(ee, eventType)					\
    (CVMglobals.jvmtiStatics._jvmti_env != NULL &&			\
     GLOBALLY_ENABLED(CVMglobals.jvmtiStatics._jvmti_env, eventType))

#define MUST_NOTIFY_THREAD(ee, eventType)				\
    (CVMglobals.jvmtiStatics._jvmti_env != NULL &&			\
     (GLOBALLY_ENABLED(CVMglobals.jvmtiStatics._jvmti_env, eventType) && \
      threadEnabled(ee, eventType)))

/* forward defs */
#ifdef JDK12
static void
handleExit(void);
#endif


/*
 * Initialize JVMTI - if and only if it hasn't been initialized.
 * Must be called before anything that accesses event structures.
 */
jvmtiError
CVMinitializeJVMTI() 
{
    CVMExecEnv *ee = CVMgetEE();
    CVMBool haveFailure = CVM_FALSE;

    /* %comment: k003 */
    if (CVMglobals.jvmtiStatics.jvmtiInitialized) {
	return JVMTI_ERROR_NONE;
    }

#ifdef JDK12
    CVMatExit(handleExit);
#endif

    CVMglobals.jvmtiStatics.breakpoints = 
	CVMbagCreateBag(sizeof(struct bkpt), INITIAL_BAG_SIZE);
    CVMglobals.jvmtiStatics.framePops = CVMbagCreateBag(
							sizeof(struct fpop), INITIAL_BAG_SIZE);
    CVMglobals.jvmtiStatics.watchedFieldModifications = 
	CVMbagCreateBag(sizeof(struct fieldWatch), INITIAL_BAG_SIZE);
    CVMglobals.jvmtiStatics.watchedFieldAccesses = 
	CVMbagCreateBag(sizeof(struct fieldWatch), INITIAL_BAG_SIZE);
    if (CVMglobals.jvmtiStatics.breakpoints == NULL || 
	CVMglobals.jvmtiStatics.framePops == NULL || 
	CVMglobals.jvmtiStatics.watchedFieldModifications == NULL || 
	CVMglobals.jvmtiStatics.watchedFieldAccesses == NULL) {
	return JVMTI_ERROR_OUT_OF_MEMORY;
    }

    /* NOTE: We must not trigger a GC while holding the thread lock.
       We are safe here because insertThread() can allocate global roots
       which can cause expansion of the global root stack, but not cause
       a GC. */
    CVM_THREAD_LOCK(ee);

    /* Log all thread that were created prior to JVMTI's initialization: */
    /* NOTE: We are only logging the pre-existing threads into the JVMTI
       threads list.  We don't send currently send JVMTI_EVENT_THREAD_START
       for these threads that started before JVMTI was initialized. */
    CVM_WALK_ALL_THREADS(ee, currentEE, {
	    jthread thread = CVMcurrentThreadICell(currentEE);
	    if (!haveFailure && !CVMID_icellIsNull(thread)) {
		ThreadNode *node = CVMjvmtiFindThread(ee, thread);
		if (node == NULL) {
		    node = CVMjvmtiInsertThread(ee, thread);
		    if (node == NULL) {
			haveFailure = CVM_TRUE;
		    }
		}
	    }
	});

    CVM_THREAD_UNLOCK(ee);

    /* Abort if we detected a failure while trying to log threads: */
    if (haveFailure) {
	return JVMTI_ERROR_OUT_OF_MEMORY;
    }
    initialize_capabilities();
    CVMglobals.jvmtiStatics.jvmtiInitialized = CVM_TRUE;
    /*    clks_per_sec = CVMgetClockTicks(); */
    return JVMTI_ERROR_NONE;
}
/*
CVMInt64 CVMjvmtiGetClockTicks() {
    return clks_per_sec;
}
*/

/*
 * These functions maintain the linked list of currently running threads. 
 */
ThreadNode *CVMjvmtiFindThread(CVMExecEnv* ee, CVMObjectICell* thread) {
    ThreadNode *node;
    CVMBool thrEq;

    JVMTI_LOCK(ee);

    /* cast away volatility */
    node = (ThreadNode *)CVMglobals.jvmtiStatics.threadList;  
    while (node != NULL) {
	CVMID_icellSameObject(ee, node->thread, thread, thrEq);
	if (thrEq) {
	    break;
	}
	node = node->next;
    }
    JVMTI_UNLOCK(ee);

    return node;
}

ThreadNode *CVMjvmtiInsertThread(CVMExecEnv* ee, CVMObjectICell* thread) {
    ThreadNode *node;

    /* NOTE: you could move the locking and unlocking inside the if
       clause in such a way as to avoid the problem with seizing both
       the debugger and global root lock at the same time, but it
       wouldn't solve the problem of removeThread, which is harder,
       nor the (potential) problem of the several routines which
       perform indirect memory accesses while holding the debugger
       lock; if there was ever a lock associated
       with those accesses there would be a problem. */
    JVMTI_LOCK(ee);

    node = (ThreadNode *)malloc(sizeof(*node));
    if (node != NULL) {
	memset(node, 0, sizeof(*node));
	node->thread = CVMID_getGlobalRoot(ee);
	if (node->thread == NULL) {
	    goto fail0;
	}
	node->lastDetectedException = CVMID_getGlobalRoot(ee);
	if (node->lastDetectedException == NULL) {
	    goto fail1;
	}

	CVMID_icellAssign(ee, node->thread, thread);

	/* cast away volatility */
	node->next = (ThreadNode *)CVMglobals.jvmtiStatics.threadList; 
	CVMglobals.jvmtiStatics.threadList = node;
    } 

 unlock:
    JVMTI_UNLOCK(ee);

    return node;

 fail1:
    CVMID_freeGlobalRoot(ee, node->thread);
 fail0:
    free(node);
    node = NULL;
    goto unlock;
}

jboolean 
CVMjvmtiRemoveThread(CVMObjectICell* thread) {
    ThreadNode *previous = NULL;
    ThreadNode *node; 
    CVMExecEnv* ee = CVMgetEE();
    CVMBool thrEq;
    jboolean rc = JNI_FALSE;

    JVMTI_LOCK(ee);

    /* cast away volatility */
    node = (ThreadNode *)CVMglobals.jvmtiStatics.threadList;  
    while (node != NULL) {
	CVMID_icellSameObject(ee, node->thread, thread, thrEq);
	if (thrEq) {
	    int i;
	    if (previous == NULL) {
		CVMglobals.jvmtiStatics.threadList = node->next;
	    } else {
		previous->next = node->next;
	    }
	    for (i = 0; i <= JVMTI_MAX_EVENT_TYPE_VAL; i++) {
		CVMjvmtiEnableThreadEvents(ee, node, i, JNI_FALSE);
	    }
	    CVMID_freeGlobalRoot(ee, node->thread);
	    CVMID_freeGlobalRoot(ee, node->lastDetectedException);

	    free((void *)node);
	    rc = JNI_TRUE;
	    break;
	}
	previous = node;
	node = node->next;
    }
    JVMTI_UNLOCK(ee);

    return rc;
}

static void
unlock_monitors(CVMExecEnv* threadEE) {
    CVMOwnedMonitor *mon, *monNext;
    CVMFrame *currentFrame;
    CVMObjectICell objIcell;
    CVMObjectICell *objp = &objIcell;
    CVMExecEnv *ee =CVMgetEE();

    mon = threadEE->objLocksOwned;
    currentFrame = CVMeeGetCurrentFrame(threadEE);
    CVMassert(CVMframeIsJava(currentFrame));

    while (mon != NULL) {
	monNext = mon->next;
#ifdef CVM_DEBUG
	CVMassert(mon->state != CVM_OWNEDMON_FREE);
#endif
	if (mon->object != NULL) {
	    if (mon->frame == currentFrame) {
		CVMD_gcUnsafeExec(ee, {
			if(!CVMobjectTryUnlock(threadEE, mon->object)) {
			    CVMID_icellSetDirect(ee, objp, mon->object);
			    CVMobjectUnlock(threadEE, objp);
			}
		    });
	    }
	}
	mon = monNext;
    }
}

void
reportException(CVMExecEnv* ee, CVMUint8 *pc,
                CVMObjectICell* object, CVMFrame* frame) {
    JNIEnv *env = CVMexecEnv2JniEnv(ee);
    CVMMethodBlock* exceptionMb = frame->mb;
    CVMClassBlock* exceptionCb;
    CVMJavaLong catchLocation = 0L;
    CVMMethodBlock *catchMb = NULL;
    JvmtiEnv *jvmti_env;
    jvmtiEventException callback;
    CVMFrameIterator iter;

    if (exceptionMb == NULL) {
	return;
    }

    /* NOTE: MUST BE A JAVA METHOD */
    CVMassert(!CVMmbIs(exceptionMb, NATIVE));

    CVMID_objectGetClass(ee, object, exceptionCb);
    jvmti_env = CVMglobals.jvmtiStatics._jvmti_env;
    callback = jvmti_env->_event_callbacks.Exception;
    if (callback != NULL) {
	/* walk up the stack to see if this exception is caught anywhere. */

	CVMframeIterate(frame, &iter);

	while (CVMframeIterateNextSpecial(&iter, CVM_TRUE)) {
	    /* %comment: k004 */
	    /* skip transition frames, which can't catch exceptions */
	    if (CVMframeIterateCanHaveJavaCatchClause(&iter)) {
		catchMb = CVMframeIterateGetMb(&iter);
		/* %comment: k005 */
		if (catchMb != NULL) {
		    CVMUint8* pc = CVMframeIterateGetJavaPc(&iter);
		    CVMUint8* cpc =
			CVMgcSafeFindPCForException(ee, &iter,
						    exceptionCb,
						    pc);
		    if (cpc != NULL) {
			/* NOTE: MUST BE A JAVA METHOD */
			CVMassert(!CVMmbIs(catchMb, NATIVE));
			catchLocation = CVMint2Long(cpc - CVMmbJavaCode(catchMb));
			break;
		    } else {
			catchMb = NULL;
		    }
		}
	    }
	}
	(*callback)(&jvmti_env->_jvmti_external, env,
		    (*env)->NewLocalRef(env, CVMcurrentThreadICell(ee)), 
		    CVMmbMethodID(exceptionMb),
		    CVMint2Long(pc - CVMmbJavaCode(exceptionMb)),
		    (*env)->NewLocalRef(env, object), CVMmbMethodID(catchMb),
		    catchLocation);
    }

}

/* %comment: k006 */
void
CVMjvmtiPostExceptionEvent(CVMExecEnv* ee, CVMUint8 *pc,
			   CVMObjectICell* object)
{
    JNIEnv *env = CVMexecEnv2JniEnv(ee);
    ThreadNode *threadNode;
    CVMBool exceptionsEqual = CVM_FALSE;

    CHECK_PHASEV(JVMTI_PHASE_LIVE);
    /* This check is probably unnecessary */
    if (CVMcurrentThreadICell(ee) == NULL) {
	return;
    }
    if (!MUST_NOTIFY_THREAD(ee, JVMTI_EVENT_EXCEPTION)) {
	return;
    }
    threadNode = CVMjvmtiFindThread(ee, CVMcurrentThreadICell(ee));
    if (threadNode == NULL) {
	/* ignore any exceptions before thread start */
	return;
    }

    CVMID_icellSameObject(ee, threadNode->lastDetectedException,
			  object,
			  exceptionsEqual);

    if (!exceptionsEqual) {

	CVMFrame* frame = CVMeeGetCurrentFrame(ee);
	jobject pendingException;

	if ((*env)->PushLocalFrame(env, 5+LOCAL_FRAME_SLOP) < 0) {
	    return;
	}
        
	pendingException = (*env)->ExceptionOccurred(env);

	/*
	 * Save the pending exception so it does not get
	 * overwritten if CVMgcSafeFindPCForException() throws an
	 * exception. Also, in classloading builds,
	 * CVMgcSafeFindPCForException may cause loading of the
	 * exception class, which code expects no exception to
	 * have occurred upon entry.
	 */

	(*env)->ExceptionClear(env);

	reportException(ee, pc, pendingException, frame);

	/*
	 * Restore the exception state
	 */
	if (pendingException != NULL) {
	    (*env)->Throw(env, pendingException);
	} else {
	    (*env)->ExceptionClear(env);
	}

	/*
	 * This needs to be a global ref; otherwise, the detected
	 * exception could be cleared and collected in a native method
	 * causing later comparisons to be invalid.
	 */
	CVMID_icellAssign(ee, threadNode->lastDetectedException, object);

	(*env)->PopLocalFrame(env, 0);
    }
}

/*
 * This function is called by the interpreter whenever:
 * 1) The interpreter catches an exception, or
 * 2) The interpreter detects that a native method has returned
 *    without an exception set (i.e. the native method *may* have
 *    cleared an exception; cannot tell for sure)
 *
 * This function performs 2 tasks. It removes any exception recorded
 * as the last detected exception by notify_debugger_of_exception. It
 * also reports an event to the JVMTI client. (2) above implies that 
 * we need to make sure that an exception was actually cleared before
 * reporting the event. This 
 * can be done by checking whether there is currently a last detected
 * detected exception value saved for this thread.
 */
void
CVMjvmtiPostExceptionCatchEvent(CVMExecEnv* ee, CVMUint8 *pc,
				CVMObjectICell* object)
{
    ThreadNode *threadNode;
    JvmtiEnv *jvmti_env;
    jvmtiEventExceptionCatch callback;
    JNIEnv* env = CVMexecEnv2JniEnv(ee);
    CVMFrame* frame;
    CVMMethodBlock* mb;
    
    CHECK_PHASEV(JVMTI_PHASE_LIVE);
    /* This check is probably unnecessary */
    if (CVMcurrentThreadICell(ee) == NULL) {
	return;
    }

    if (!MUST_NOTIFY_THREAD(ee, JVMTI_EVENT_EXCEPTION_CATCH)) {
	return;
    }
    threadNode = CVMjvmtiFindThread(ee, CVMcurrentThreadICell(ee));
    if (threadNode == NULL) {
	/* ignore any exceptions before thread start */
	return;
    }

    frame = CVMeeGetCurrentFrame(ee);
    mb = frame->mb;

    if (mb == NULL) {
	return;
    }

    /* NOTE: MUST BE A JAVA METHOD */
    CVMassert(!CVMmbIs(mb, NATIVE));

    /*
     * Report the caught exception if it is being caught in Java code
     * or if it was caught in native code after its throw was reported 
     * earlier.
     */
    if ((object != NULL) || 
	!CVMID_icellIsNull(threadNode->lastDetectedException)) {
            
	if ((*env)->PushLocalFrame(env, 3+LOCAL_FRAME_SLOP) < 0) {
	    return;
	}

	jvmti_env = CVMglobals.jvmtiStatics._jvmti_env;
	callback = jvmti_env->_event_callbacks.ExceptionCatch;
	if (callback != NULL) {
	    (*callback)(&jvmti_env->_jvmti_external, env,
			(*env)->NewLocalRef(env, CVMcurrentThreadICell(ee)), 
			CVMmbMethodID(mb), CVMint2Long(pc - CVMmbJavaCode(mb)),
			(*env)->NewLocalRef(env, object));
	}

	(*env)->PopLocalFrame(env, 0);
    }
    CVMID_icellSetNull(threadNode->lastDetectedException);
}

void
CVMjvmtiPostSingleStepEvent(CVMExecEnv* ee, CVMUint8 *pc)
{
    /*
     * The interpreter notifies us only for threads that have stepping enabled,
     * so we don't do any checks of the global or per-thread event
     * enable flags here.
     */
    CHECK_PHASEV(JVMTI_PHASE_LIVE);

    if (CVMglobals.jvmtiStatics.jvmtiInitialized) {
	CVMFrame* frame = CVMeeGetCurrentFrame(ee);
	CVMMethodBlock* mb = frame->mb;
	JNIEnv* env = CVMexecEnv2JniEnv(ee);
	JvmtiEnv *jvmti_env;
	jvmtiEventSingleStep callback;

	if (CVMframeIsTransition(frame) || mb == NULL) {
	    return;
	}

	/* NOTE: MUST BE A JAVA METHOD */
	CVMassert(!CVMmbIs(mb, NATIVE));

	if ((*env)->PushLocalFrame(env, 2+LOCAL_FRAME_SLOP) < 0) {
	    return;
	}
	jvmti_env = CVMglobals.jvmtiStatics._jvmti_env;
	callback = jvmti_env->_event_callbacks.SingleStep;
	if (callback != NULL) {
	    (*callback)(&jvmti_env->_jvmti_external, env,
			(*env)->NewLocalRef(env, CVMcurrentThreadICell(ee)), 
			CVMmbMethodID(mb),
			CVMint2Long(pc - CVMmbJavaCode(mb)));
	}

	(*env)->PopLocalFrame(env, 0);
    }
    if (ee->_jvmti_event_enabled._enabled_bits & NEED_FRAME_POP_BIT) {
	unlock_monitors(ee);
	CVMpopFrame(&ee->interpreterStack, CVMeeGetCurrentFrame(ee));
    }
}

void
notify_debugger_of_breakpoint(CVMExecEnv* ee, CVMUint8 *pc)
{

    CHECK_PHASEV(JVMTI_PHASE_LIVE);
    if (MUST_NOTIFY_THREAD(ee, JVMTI_EVENT_BREAKPOINT)) {
	CVMFrame* frame = CVMeeGetCurrentFrame(ee);
	CVMMethodBlock* mb = frame->mb;
	JNIEnv *env = CVMexecEnv2JniEnv(ee);
	JvmtiEnv *jvmti_env;
	jvmtiEventBreakpoint callback;

	if (mb == NULL) {
	    return;
	}
	/* NOTE: MUST BE A JAVA METHOD */
	CVMassert(!CVMmbIs(mb, NATIVE));
	if ((*env)->PushLocalFrame(env, 2+LOCAL_FRAME_SLOP) < 0) {
	    return;
	}
	jvmti_env = CVMglobals.jvmtiStatics._jvmti_env;
	callback = jvmti_env->_event_callbacks.Breakpoint;
	if (callback != NULL) {
	    (*callback)(&jvmti_env->_jvmti_external, env,
			(*env)->NewLocalRef(env, CVMcurrentThreadICell(ee)), 
			CVMmbMethodID(mb),
			CVMint2Long(pc - CVMmbJavaCode(mb)));
	}
  
	(*env)->PopLocalFrame(env, 0);
    }
    if (ee->_jvmti_event_enabled._enabled_bits & NEED_FRAME_POP_BIT) {
	unlock_monitors(ee);
	CVMpopFrame(&ee->interpreterStack, CVMeeGetCurrentFrame(ee));
    }
}

void
CVMjvmtiPostThreadStartEvent(CVMExecEnv* ee, CVMObjectICell* thread)
{
    JNIEnv *env = CVMexecEnv2JniEnv(ee);
    JvmtiEnv *jvmti_env;
    jvmtiEventThreadStart callback;

    /*
     * Look for existing thread info for this thread. If there is 
     * a ThreadNode already, it just means that it's a debugger thread
     * started by jvmti_RunDebugThread; if not, we create the ThreadNode
     * here. 
     */
    ThreadNode *node = CVMjvmtiFindThread(ee, thread);
    if (node == NULL) {
	node = CVMjvmtiInsertThread(ee, thread);
	if (node == NULL) {
	    (*env)->FatalError(env, "internal allocation error in JVMTI");
	}
    }

    jvmti_env = CVMglobals.jvmtiStatics._jvmti_env;
    CHECK_PHASEV2(JVMTI_PHASE_LIVE, JVMTI_PHASE_START);
    if (CVMglobals.jvmtiStatics.jvmtiInitialized) {
	if ((*env)->PushLocalFrame(env, 1+LOCAL_FRAME_SLOP) < 0) {
	    return;
	}
	callback = jvmti_env->_event_callbacks.ThreadStart;
	if (callback != NULL) {
	    (*callback)(&jvmti_env->_jvmti_external, env,
			(*env)->NewLocalRef(env, CVMcurrentThreadICell(ee)));
	}

	(*env)->PopLocalFrame(env, 0);
    }
    jvmti_recompute_thread_enabled(ee, &jvmti_env->_env_event_enable);
}

void
CVMjvmtiPostThreadEndEvent(CVMExecEnv* ee, CVMObjectICell* thread)
{
    JNIEnv *env = CVMexecEnv2JniEnv(ee);
    CHECK_PHASEV2(JVMTI_PHASE_LIVE, JVMTI_PHASE_START);
    if (MUST_NOTIFY_THREAD(ee, JVMTI_EVENT_THREAD_END)) {
	JvmtiEnv *jvmti_env;
	jvmtiEventThreadEnd callback;

	if ((*env)->PushLocalFrame(env, 1+LOCAL_FRAME_SLOP) < 0) {
	    goto forgetThread;
	}
	jvmti_env = CVMglobals.jvmtiStatics._jvmti_env;
	callback = jvmti_env->_event_callbacks.ThreadEnd;
	if (callback != NULL) {
	    (*callback)(&jvmti_env->_jvmti_external, env,
			(*env)->NewLocalRef(env, CVMcurrentThreadICell(ee)));
	}
	(*env)->PopLocalFrame(env, 0);
    }

 forgetThread:
    CVMjvmtiRemoveThread(thread);
}

void
CVMjvmtiPostFieldAccessEvent(CVMExecEnv* ee, CVMObjectICell* obj,
			     CVMFieldBlock* fb)
{
    struct fieldWatch *fwfb;

    CHECK_PHASEV(JVMTI_PHASE_LIVE);
    JVMTI_LOCK(ee);
    fwfb = CVMbagFind(CVMglobals.jvmtiStatics.watchedFieldAccesses, fb);
    JVMTI_UNLOCK(ee);          

    if (fwfb != NULL &&
	MUST_NOTIFY(ee, JVMTI_EVENT_FIELD_ACCESS)) {
	CVMFrame* frame = CVMeeGetCurrentFrame(ee);
	CVMMethodBlock* mb;
	jlocation location;
	JNIEnv* env = CVMexecEnv2JniEnv(ee);
	JvmtiEnv *jvmti_env;
	jvmtiEventFieldAccess callback;

	/* We may have come through native code - remove any
	 * empty frames.
	 */

	/* %comment: k007 */

	frame = CVMgetCallerFrame(frame, 0);
	if (frame == NULL || (mb = frame->mb) == NULL) {
	    return;
	}
	if (CVMframeIsInterpreter(frame)) {
	    CVMInterpreterFrame* interpFrame = CVMgetInterpreterFrame(frame);
	    location = CVMint2Long(CVMframePc(interpFrame) - CVMmbJavaCode(mb));
	} else {
	    location = CVMint2Long(-1);
	}

	if ((*env)->PushLocalFrame(env, 4+LOCAL_FRAME_SLOP) < 0) {
	    return;
	}
	jvmti_env = CVMglobals.jvmtiStatics._jvmti_env;
	callback = jvmti_env->_event_callbacks.FieldAccess;
	if (callback != NULL) {
	    (*callback)(&jvmti_env->_jvmti_external, env,
			(*env)->NewLocalRef(env, CVMcurrentThreadICell(ee)), 
			CVMmbMethodID(mb), location,
			(*env)->NewLocalRef(env,
					    CVMcbJavaInstance(CVMfbClassBlock(fb))),
			(obj == NULL ? NULL :
			 (*env)->NewLocalRef(env, obj)), fb);
	}
	(*env)->PopLocalFrame(env, 0);
    }
    if (ee->_jvmti_event_enabled._enabled_bits & NEED_FRAME_POP_BIT) {
	unlock_monitors(ee);
	CVMpopFrame(&ee->interpreterStack, CVMeeGetCurrentFrame(ee));
    }
}

void
CVMjvmtiPostFieldModificationEvent(CVMExecEnv* ee, CVMObjectICell* obj,
				   CVMFieldBlock* fb, jvalue jval)
{
    struct fieldWatch *fwfb;

    CHECK_PHASEV(JVMTI_PHASE_LIVE);
    JVMTI_LOCK(ee);
    fwfb = CVMbagFind(CVMglobals.jvmtiStatics.watchedFieldModifications, fb);
    JVMTI_UNLOCK(ee);          

    if (fwfb != NULL &&
	MUST_NOTIFY(ee, JVMTI_EVENT_FIELD_MODIFICATION)) {
	CVMFrame* frame = CVMeeGetCurrentFrame(ee);
	CVMMethodBlock* mb;
	jlocation location;
	JNIEnv* env = CVMexecEnv2JniEnv(ee);
	CVMFieldTypeID tid = CVMfbNameAndTypeID(fb);
	char sig_type;
	JvmtiEnv *jvmti_env;
	jvmtiEventFieldModification callback;

	if (CVMtypeidIsPrimitive(tid)) {
	    sig_type = CVMterseTypePrimitiveSignatures[CVMtypeidGetType(tid)];
	} else if (CVMtypeidIsArray(tid)) {
	    sig_type = CVM_SIGNATURE_ARRAY;
	} else {
	    sig_type = CVM_SIGNATURE_CLASS;
	}

	/* We may have come through native code - remove any
	 * empty frames.
	 */

	/* %comment: k007 */

	frame = CVMgetCallerFrame(frame, 0);
	if (frame == NULL || (mb = frame->mb) == NULL) {
	    return;
	}
	if (CVMframeIsInterpreter(frame)) {
	    CVMInterpreterFrame* interpFrame = CVMgetInterpreterFrame(frame);
	    location = CVMint2Long(CVMframePc(interpFrame) - CVMmbJavaCode(mb));
	} else {
	    location = CVMint2Long(-1);
	}

	if ((*env)->PushLocalFrame(env, 5+LOCAL_FRAME_SLOP) < 0) {
	    return;
	}
	if ((sig_type == CVM_SIGNATURE_CLASS) ||
	    (sig_type == CVM_SIGNATURE_ARRAY)) {
	    jval.l = (*env)->NewLocalRef(env, jval.l);
	}

	jvmti_env = CVMglobals.jvmtiStatics._jvmti_env;
	callback = jvmti_env->_event_callbacks.FieldModification;
	if (callback != NULL) {
	    (*callback)(&jvmti_env->_jvmti_external, env,
			(*env)->NewLocalRef(env, CVMcurrentThreadICell(ee)), 
			CVMmbMethodID(mb), location,
			(*env)->NewLocalRef(env, CVMcbJavaInstance(CVMfbClassBlock(fb))),
			(obj == NULL? NULL : 
			 (*env)->NewLocalRef(env, obj)),
			fb, sig_type, jval);
	}
	(*env)->PopLocalFrame(env, 0);
    }
    if (ee->_jvmti_event_enabled._enabled_bits & NEED_FRAME_POP_BIT) {
	unlock_monitors(ee);
	CVMpopFrame(&ee->interpreterStack, CVMeeGetCurrentFrame(ee));
    }
}

void
CVMjvmtiPostClassLoadEvent(CVMExecEnv* ee, CVMObjectICell* clazz)
{
    CHECK_PHASEV2(JVMTI_PHASE_LIVE, JVMTI_PHASE_START);
    if (MUST_NOTIFY_THREAD(ee, JVMTI_EVENT_CLASS_LOAD)) {
	JNIEnv *env = CVMexecEnv2JniEnv(ee);
	JvmtiEnv *jvmti_env;
	jvmtiEventClassLoad callback;

	if ((*env)->PushLocalFrame(env, 2+LOCAL_FRAME_SLOP) < 0) {
	    return;
	}
	jvmti_env = CVMglobals.jvmtiStatics._jvmti_env;
	callback = jvmti_env->_event_callbacks.ClassLoad;
	if (callback != NULL) {
	    (*callback)(&jvmti_env->_jvmti_external, env,
			(*env)->NewLocalRef(env, CVMcurrentThreadICell(ee)), 
			(*env)->NewLocalRef(env, clazz));
	}
	(*env)->PopLocalFrame(env, 0);
    }
}


void
CVMjvmtiPostClassPrepareEvent(CVMExecEnv* ee, CVMObjectICell* clazz)
{
    CHECK_PHASEV2(JVMTI_PHASE_LIVE, JVMTI_PHASE_START);
    if (MUST_NOTIFY_THREAD(ee, JVMTI_EVENT_CLASS_PREPARE)) {
	JNIEnv *env = CVMexecEnv2JniEnv(ee);
	JvmtiEnv *jvmti_env;
	jvmtiEventClassPrepare callback;

	if ((*env)->PushLocalFrame(env, 2+LOCAL_FRAME_SLOP) < 0) {
	    return;
	}
	jvmti_env = CVMglobals.jvmtiStatics._jvmti_env;
	callback = jvmti_env->_event_callbacks.ClassPrepare;
	if (callback != NULL) {
	    (*callback)(&jvmti_env->_jvmti_external, env,
			(*env)->NewLocalRef(env, CVMcurrentThreadICell(ee)), 
			(*env)->NewLocalRef(env, clazz));
	}
	(*env)->PopLocalFrame(env, 0);
    }
}

void
reportFrameEvent(CVMExecEnv* ee, jint kind, CVMFrame *targetFrame,
		 CVMBool is_exception, jvalue value)
{
    JNIEnv *env = CVMexecEnv2JniEnv(ee);
    CVMObjectICell* thread = CVMcurrentThreadICell(ee);
    CVMMethodBlock* mb = targetFrame->mb;
    JvmtiEnv *jvmti_env;

    if (mb == NULL) {
	return;
    }

    jvmti_env = CVMglobals.jvmtiStatics._jvmti_env;
    switch(kind) {
    case JVMTI_EVENT_FRAME_POP:
	{
	    jvmtiEventFramePop callback;
	    callback = jvmti_env->_event_callbacks.FramePop;
	    if (callback != NULL) {
		(*callback)(&jvmti_env->_jvmti_external, env,
			    (*env)->NewLocalRef(env, thread),
			    CVMmbMethodID(mb), is_exception);
	    }
	}
	break;
    case JVMTI_EVENT_METHOD_ENTRY:
	{
	    jvmtiEventMethodEntry callback;
	    callback = jvmti_env->_event_callbacks.MethodEntry;
	    if (callback != NULL) {
		(*callback)(&jvmti_env->_jvmti_external, env,
			    (*env)->NewLocalRef(env, thread),
			    CVMmbMethodID(mb));
	    }
	}
	break;
    case JVMTI_EVENT_METHOD_EXIT:
	{
	    /* NOTE: need to pass return value */
	    jvmtiEventMethodExit callback;
	    callback = jvmti_env->_event_callbacks.MethodExit;
	    if (callback != NULL) {
		(*callback)(&jvmti_env->_jvmti_external, env,
			    (*env)->NewLocalRef(env, thread),
			    CVMmbMethodID(mb), is_exception, value);
	    }
	}
	break;
    }

}

void
CVMjvmtiPostFramePopEvent(CVMExecEnv* ee, CVMBool isRef,
			  CVMBool is_exception, jlong val)
{
    CVMFrame* frame = CVMeeGetCurrentFrame(ee);
    CVMFrame* framePrev;
    jvalue value;
    JNIEnv *env = CVMexecEnv2JniEnv(ee);

    /* %comment: k008 */

    CHECK_PHASEV(JVMTI_PHASE_LIVE);
    if (frame == NULL) {
	return;
    }

    framePrev = CVMframePrev(frame);

    if (framePrev == NULL) {
	return;
    }

    /* NOTE: This is needed by the code below, but wasn't
       present in the 1.2 sources */
    if (!CVMframeIsInterpreter(framePrev)) {
	return;
    }

    if ((*env)->PushLocalFrame(env, 2+LOCAL_FRAME_SLOP) < 0) {
	return;
    }

    if (isRef) {
	/* return value was stashed in ee->miscICell */
	value.l = (*env)->NewLocalRef(env, ee->miscICell);
    } else {
	value.j = val;
    }
    /* NOTE: the JDK has two slots for the return PC; one is the
       "lastpc" and one is the "returnpc". The JDK's version of the
       JVMTI inserts a "frame pop sentinel" as the returnpc in
       jvmti_NotifyFramePop. The munging of the return PC is
       unnecessary and is merely an optimization to avoid performing a
       hash table lookup each time this function is called by the
       interpreter loop. Unfortunately, in CVM this optimization is
       also incorrect. We only have one return PC slot, and if an
       exception is thrown the fillInStackTrace code will not know
       that it may have to undo these sentinels, or how to do so.

       We could get around this by setting a bit in CVMFrame; since
       pointers on a 32-bit architecture are 4-byte aligned, we could
       use the bit just above the "special handling bit". However, for
       now, we're going to do the slow hashtable lookup each time. */
    {
	struct fpop *fp = NULL;
	CVMBool gotFramePop = CVM_FALSE;

	JVMTI_LOCK(ee);
	/* It seems that this gets called before JVMTI is initialized */
	if (CVMglobals.jvmtiStatics.framePops != NULL) {
	    fp = CVMbagFind(CVMglobals.jvmtiStatics.framePops, frame);
	}

	if (fp != NULL) {
	    /* Found a frame pop */
	    /* fp now points to randomness */
	    CVMbagDelete(CVMglobals.jvmtiStatics.framePops, fp); 
	    gotFramePop = CVM_TRUE;
	}
	JVMTI_UNLOCK(ee);

	if (gotFramePop) {
	    if (!CVMexceptionOccurred(ee) &&
		MUST_NOTIFY(ee, JVMTI_EVENT_FRAME_POP)) {
		reportFrameEvent(ee, JVMTI_EVENT_FRAME_POP, frame,
				 is_exception, value);
	    }
	}
    }
    if (!CVMexceptionOccurred(ee) && 
	MUST_NOTIFY(ee, JVMTI_EVENT_METHOD_EXIT)) {
	reportFrameEvent(ee, JVMTI_EVENT_METHOD_EXIT, frame, is_exception, value);
    }
    (*env)->PopLocalFrame(env, 0);
}

void
CVMjvmtiPostFramePushEvent(CVMExecEnv* ee)
{
    CVMFrame* frame = CVMeeGetCurrentFrame(ee);
    JNIEnv *env = CVMexecEnv2JniEnv(ee);
    jvalue val;
    val.j = 0L;
    CHECK_PHASEV(JVMTI_PHASE_LIVE);
    if ((*env)->PushLocalFrame(env, 2+LOCAL_FRAME_SLOP) < 0) {
	return;
    }
    if (MUST_NOTIFY(ee, JVMTI_EVENT_METHOD_ENTRY)) {
	reportFrameEvent(ee, JVMTI_EVENT_METHOD_ENTRY, frame, CVM_FALSE, val);
    }
    (*env)->PopLocalFrame(env, 0);
}

/*
 * This function is called near the end of the VM initialization process.
 * It triggers the sending of an event to the JVMTI client. At this point
 * The JVMTI client is free to use all of JNI and JVMTI.
 */
void
CVMjvmtiPostVmStartEvent(CVMExecEnv* ee)
{    
    JvmtiEnv *jvmti_env;
    jvmtiEventVMStart callback;
    JNIEnv *env = CVMexecEnv2JniEnv(ee);

    CHECK_PHASEV2(JVMTI_PHASE_LIVE, JVMTI_PHASE_START);
    jvmti_env = CVMglobals.jvmtiStatics._jvmti_env;
    if (jvmti_env != NULL) {
	callback = jvmti_env->_event_callbacks.VMStart;
	if (callback != NULL) {
	    (*callback)(&jvmti_env->_jvmti_external, env);
	}
    }
}

/*
 * This function is called at the end of the VM initialization process.
 * It triggers the sending of an event to the JVMTI client. At this point
 * The JVMTI client is free to use all of JNI and JVMTI.
 */
void
CVMjvmtiPostVmInitEvent(CVMExecEnv* ee)
{    
    JvmtiEnv *jvmti_env;
    jvmtiEventVMInit callback;
    JNIEnv *env = CVMexecEnv2JniEnv(ee);

    CHECK_PHASEV(JVMTI_PHASE_LIVE);
    if ((*env)->PushLocalFrame(env, 1+LOCAL_FRAME_SLOP) < 0) {
	return;
    }
    jvmti_env = CVMglobals.jvmtiStatics._jvmti_env;
    if (jvmti_env != NULL) {
	callback = jvmti_env->_event_callbacks.VMInit;
	if (callback != NULL) {
	    (*callback)(&jvmti_env->_jvmti_external, env,
			(*env)->NewLocalRef(env, CVMcurrentThreadICell(ee)));
	}
    }
    (*env)->PopLocalFrame(env, 0);
}

void
CVMjvmtiPostVmExitEvent(CVMExecEnv* ee)
{    
    CHECK_PHASEV(JVMTI_PHASE_LIVE);
    if (CVMglobals.jvmtiStatics.jvmtiInitialized) {
	JNIEnv *env = CVMexecEnv2JniEnv(ee);
	JvmtiEnv *jvmti_env;
	jvmtiEventVMDeath callback;

	if ((*env)->PushLocalFrame(env, 0+LOCAL_FRAME_SLOP) < 0) {
	    return;
	}
	jvmti_env = CVMglobals.jvmtiStatics._jvmti_env;
	callback = jvmti_env->_event_callbacks.VMDeath;
	if (callback != NULL) {
	    (*callback)(&jvmti_env->_jvmti_external, env);
	}
	(*env)->PopLocalFrame(env, 0);
    }
}

#ifdef JDK12

void
handleExit(void)
{
    CVMjvmtiPostVmExitEvent(CVMgetEE());
}

#endif

/**
 * Return the underlying opcode at the specified address.
 * Notify of the breakpoint, if it is still there and 'notify' is true.
 * For use outside jvmti.
 */
CVMUint8
CVMjvmtiGetBreakpointOpcode(CVMExecEnv* ee, CVMUint8 *pc, CVMBool notify)
{
    struct bkpt *bp;
    int rv;

    JVMTI_LOCK(ee);
    bp = CVMbagFind(CVMglobals.jvmtiStatics.breakpoints, pc);
    if (bp == NULL) {
	rv = *pc;  
    } else {
	rv = bp->opcode;
    }
    JVMTI_UNLOCK(ee);
    
    CVMassert(rv != opc_breakpoint);

    if (notify && bp != NULL) {
	notify_debugger_of_breakpoint(ee, pc);
    }
    return rv;
}

/* The opcode at the breakpoint has changed. For example, it
 * has been quickened. Update the opcode stored in the breakpoint table.
 */
CVMBool
CVMjvmtiSetBreakpointOpcode(CVMExecEnv* ee, CVMUint8 *pc, CVMUint8 opcode)
{
    struct bkpt *bp;

    CVMassert(JVMTI_IS_LOCKED(ee));
    bp = CVMbagFind(CVMglobals.jvmtiStatics.breakpoints, pc);
    CVMassert(bp != NULL);
    bp->opcode = opcode;
    return CVM_TRUE;
}

/*  Profiling support */

/* Purpose: Posts a JVMTI_CLASS_LOAD_HOOK event. */
/* NOTE: Called while GC safe. */
void CVMjvmtiPostClassLoadHookEvent(jclass klass, CVMClassLoaderICell *loader,
				    const char *class_name,
				    jobject protection_domain,
				    CVMInt32 bufferLength, CVMUint8 *buffer,
				    CVMInt32 *new_bufferLength,
				    CVMUint8 **new_buffer)
{
    CVMExecEnv *ee = CVMgetEE();
    jvmtiEventClassFileLoadHook callback;
    JNIEnv *env = CVMexecEnv2JniEnv(ee);
    JvmtiEnv *jvmti_env;

    if (CVMjvmtiGetPhase() != JVMTI_PHASE_PRIMORDIAL &&
	CVMjvmtiGetPhase() != JVMTI_PHASE_ONLOAD &&
	CVMjvmtiGetPhase() != JVMTI_PHASE_LIVE) {
	return;
    }
    CVMassert(!CVMgcIsGCThread(ee));
    CVMassert(CVMD_isgcSafe(ee));
    CVMassert(!ee->hasPostedExitEvents);

    if ((*env)->PushLocalFrame(env, 1+LOCAL_FRAME_SLOP) < 0) {
	return;
    }
    /* Fill event info and notify the profiler: */
    jvmti_env = CVMglobals.jvmtiStatics._jvmti_env;
    if (jvmti_env != NULL) {
	callback = jvmti_env->_event_callbacks.ClassFileLoadHook;
	if (callback != NULL) {
	    (*callback)(&jvmti_env->_jvmti_external, env,
			klass, loader,
			class_name, protection_domain,
			bufferLength, buffer,
			new_bufferLength, new_buffer);
	}
    }
}

/* Purpose: Posts a JVMTI_EVENT_COMPILED_METHOD_LOAD event. */
/* IAI-07: Notify JVMTI of compilations and decompilations. */
void CVMjvmtiPostCompiledMethodLoadEvent(CVMExecEnv *ee, CVMMethodBlock *mb)
{
#ifndef CVM_JIT
    CVMassert(CVM_FALSE);
#else
    jvmtiEventCompiledMethodLoad callback;
    CVMCompiledMethodDescriptor *cmd = CVMmbCmd(mb);
    JvmtiEnv *jvmti_env = CVMglobals.jvmtiStatics._jvmti_env;

#ifdef CVM_DEBUG_CLASSINFO
    int i;
    CVMCompiledPcMap * maps = CVMcmdCompiledPcMaps(cmd);
    int map_entries;
    jvmtiAddrLocationMap *addrMap = NULL;
    jvmtiError err;
    CHECK_PHASEV(JVMTI_PHASE_LIVE);

    /* cmd: javapc<->compiled pc */
    map_entries = CVMcmdNumPcMaps(cmd);
    if (map_entries <= 0) {
        goto fillfields;
    }

    if (jvmti_env == NULL) {
	goto failed;
    }
    err = CVMjvmtiAllocate(map_entries * sizeof(jvmtiAddrLocationMap),
			   (unsigned char **)&addrMap);
    if (err != JVMTI_ERROR_NONE) {
        goto failed;
    }

    for (i = 0; i < map_entries; i++) {
        addrMap[i].start_address = CVMcmdCodeBufAddr(cmd) + maps[i].compiledPc;
	addrMap[i].location = maps[i].javaPc;
    }

#else
    CHECK_PHASEV(JVMTI_PHASE_LIVE);
#endif /* CVM_DEBUG_CLASSINFO */

 fillfields:
    if (jvmti_env != NULL) {
	callback = jvmti_env->_event_callbacks.CompiledMethodLoad;
	if (callback != NULL) {
	    (*callback)(&jvmti_env->_jvmti_external, CVMmbMethodID(mb),
			CVMcmdCodeBufSize(cmd), CVMcmdCodeBufAddr(cmd),
			map_entries, addrMap, NULL);
	}
    }

 doCleanUpAndExit:
    if (addrMap != NULL) {
	CVMjvmtiDeallocate((unsigned char *)addrMap);
    }
    return;

 failed:
    goto doCleanUpAndExit;
#endif /* CVM_JIT */
}

/* Purpose: Posts a JVMTI_EVENT_COMPILED_METHOD_UNLOAD event. */
/* IAI-07: Notify JVMTI of compilations and decompilations. */
void CVMjvmtiPostCompiledMethodUnloadEvent(CVMExecEnv *ee, CVMMethodBlock* mb)
{
#ifndef CVM_JIT
    CVMassert(CVM_FALSE);
#else
    jvmtiEventCompiledMethodUnload callback;
    JvmtiEnv *jvmti_env = CVMglobals.jvmtiStatics._jvmti_env;
    CVMCompiledMethodDescriptor *cmd = CVMmbCmd(mb);
    CHECK_PHASEV(JVMTI_PHASE_LIVE);

    if (jvmti_env != NULL) {
	callback = jvmti_env->_event_callbacks.CompiledMethodUnload;
	if (callback != NULL) {
	    (*callback)(&jvmti_env->_jvmti_external, CVMmbMethodID(mb),
			CVMcmdCodeBufAddr(cmd));
	}
    }
#endif /* CVM_JIT */
}

/* Purpose: Posts the JVMTI_EVENT_DATA_DUMP_REQUEST event. */
/* NOTE: Called while GC safe. */
void CVMjvmtiPostDataDumpRequest(void)
{
    jvmtiEventDataDumpRequest callback;
    JvmtiEnv *jvmti_env = CVMglobals.jvmtiStatics._jvmti_env;
    CHECK_PHASEV(JVMTI_PHASE_LIVE);

    if (jvmti_should_post_data_dump()) {
	if (jvmti_env != NULL) {
	    callback = jvmti_env->_event_callbacks.DataDumpRequest;
	    if (callback != NULL) {
		(*callback)(&jvmti_env->_jvmti_external);
	    }
	}
    }
}

/*
 * Purpose: Posts the JVMTI_EVENT_GARBAGE_COLLECTION_START event just before
 * GC starts.
 * NOTE: Called while GC safe before GC thread requests all threads to be
 * safe.
 */
void CVMjvmtiPostGCStartEvent(void)
{
    jvmtiEventGarbageCollectionStart callback;
    JvmtiEnv *jvmti_env = CVMglobals.jvmtiStatics._jvmti_env;
    CHECK_PHASEV(JVMTI_PHASE_LIVE);

    CVMassert(!CVMgcIsGCThread(CVMgetEE()));
    CVMassert(CVMD_isgcSafe(CVMgetEE()));
    if (jvmti_env != NULL) {
	callback = jvmti_env->_event_callbacks.GarbageCollectionStart;
	if (callback != NULL) {
	    (*callback)(&jvmti_env->_jvmti_external);
	}
    }
}

/*
 * Purpose: Posts the JVMTI_EVENT_GARBAGE_COLLECTION_FINISH event just after
 * GC finishes.
 * NOTE: Called while GC safe before GC thread allows all threads to become
 * unsafe again.
 */
void CVMjvmtiPostGCFinishEvent(void)
{
    jvmtiEventGarbageCollectionFinish callback;
    JvmtiEnv *jvmti_env = CVMglobals.jvmtiStatics._jvmti_env;

    CHECK_PHASEV(JVMTI_PHASE_LIVE);
    CVMassert(CVMgcIsGCThread(CVMgetEE()));
    CVMassert(CVMD_isgcSafe(CVMgetEE()));
    if (jvmti_env != NULL) {
	callback = jvmti_env->_event_callbacks.GarbageCollectionFinish;
	if (callback != NULL) {
	    (*callback)(&jvmti_env->_jvmti_external);
	}
    }
}

#ifdef CVM_CLASSLIB_JCOV
/*
 * Purpose: Posts a JVMTI_EVENT_CLASS_LOAD_HOOK for a preloaded class.
 * NOTE: This function is used to simulate loading of classes that were
 *       preloaded.  If exceptions are encountered while doing this, the
 *       exceptions will be cleared and discarded.
 */
static void
CVMjvmtiPostStartupClassLoadHookEvents(CVMExecEnv *ee,
                                       CVMClassBlock *cb, void *data)
{
    char *classname;
    CVMClassPath *path = (CVMClassPath *)data;
    CVMUint8 *classfile;
    CVMUint32 classfileSize;

    CVMassert(!CVMexceptionOccurred(ee));

    classname = CVMtypeidClassNameToAllocatedCString(CVMcbClassName(cb));
    if (classname == NULL) {
        return;
    }

    classfile = CVMclassLoadSystemClassFileFromPath(ee, classname,
                                                    path, &classfileSize);
    if (CVMexceptionOccurred(ee)) {
        CVMclearLocalException(ee);
        free(classname);
        return;
    }

    if (jvmti_should_post_class_file_load_hook()) {
        CVMUint8 *buffer = (CVMUint8 *)classfile;
        CVMUint32 bufferSize = classfileSize;
	CVMUint8 *new_buffer = NULL;
	CVMInt32 new_bufferLength = 0;

        CVMjvmtiPostClassLoadHookEvent(NULL, classname, bufferSize, buffer,
                                       &new_bufferLength, &new_buffer);

        /* If the new_buffer != NULL, then the profiler agent must have
           replaced it with an instrumented version.  We should clean this up
           properly even though we can't use it.
        */
        if (new_buffer != NULL) {
	    JvmtiEnv *jvmti_env = CVMglobals.jvmtiStatics._jvmti_env;
            CVMjvmtiDeallocate(buffer);
        }
    }

    /* Clean-up resources and return: */
    CVMclassReleaseSystemClassFile(ee, classfile);
    free(classname);
}
#endif

/* Purpose: Posts a JVMTI_EVENT_CLASS_LOAD for a missed class. */
static void
CVMjvmtiPostStartupClassEvents(CVMExecEnv *ee, CVMClassBlock *cb, void *data)
{
    /* If an exception occurred during event notification, then abort: */
    if (CVMexceptionOccurred(ee)) {
        return;
    }

    /* Ignore primitive types: */
    if (CVMcbIs(cb, PRIMITIVE) ) {
        return;
    }

#ifdef CVM_CLASSLIB_JCOV
    if (jvmti_should_post_class_file_load_hook()) {
        CVMjvmtiPostStartupClassLoadHookEvents(ee, cb,
                                               &CVMglobals.bootClassPath);
        /* If an exception occurred during event notification, then abort.
           We cannot go on with more event notifications: */
        if (CVMexceptionOccurred(ee)) {
            return;
        }
    }
#endif
    if (jvmti_should_post_class_load()) {
        CVMjvmtiPostClassLoadEvent(ee, CVMcbJavaInstance(cb));
    }
}

/* Purpose: Posts a JVMTI_EVENT_VM_OBJECT_ALLOC event. */
/* NOTE: Called while GC safe. */
static void CVMjvmtiPostVMObjectAllocEvent(CVMExecEnv *ee,
					   CVMObject *obj)
                                          
{
    const CVMClassBlock *cb;
    CVMUint32 size;
    jvmtiEventVMObjectAlloc callback;
    JvmtiEnv *jvmti_env = CVMglobals.jvmtiStatics._jvmti_env;
    JNIEnv *env = CVMexecEnv2JniEnv(ee);
    jobject thisObj;
    CHECK_PHASEV(JVMTI_PHASE_LIVE);

    CVMassert(CVMD_isgcSafe(ee));

    if (obj == NULL) {
        return;
    }
    cb = CVMobjectGetClass(obj);
    size = CVMobjectSizeGivenClass(obj, cb);

    if (jvmti_env != NULL) {
	if ((*env)->PushLocalFrame(env, 3) < 0) {
	    return;
	}
	CVMID_localrootBegin(ee); {
	    CVMID_localrootDeclare(CVMObjectICell, tempObj);
	    CVMD_gcUnsafeExec(ee, {
		    CVMID_icellSetDirect(ee, tempObj, obj);
		});
	    thisObj = (*env)->NewLocalRef(env, tempObj);
	} CVMID_localrootEnd();
	callback = jvmti_env->_event_callbacks.VMObjectAlloc;
	if (callback != NULL) {
	    (*callback)(&jvmti_env->_jvmti_external, env,
			(*env)->NewLocalRef(env, CVMcurrentThreadICell(ee)),
			thisObj,
			(*env)->NewLocalRef(env, CVMcbJavaInstance(cb)),
			size);
	}
	(*env)->PopLocalFrame(env, 0);
    }
}

/* Purpose: Posts a JVMTI_EVENT_VM_OBJECT_ALLOC for a missed object. */
static CVMBool
CVMjvmtiPostStartupObjectEvents(CVMObject *obj, CVMClassBlock *cb,
                                CVMUint32 size, void *data)
{
    if (CVMobjectGetClass(obj) != CVMsystemClass(java_lang_Class)) {
        if (jvmti_should_post_vm_object_alloc()) {
            CVMExecEnv *ee = (CVMExecEnv *)data;
            CVMjvmtiPostVMObjectAllocEvent(ee, obj);
            /* If an exception occurred during event notification, then abort.
               We cannot go on with more event notifications: */
            if (CVMexceptionOccurred(ee)) {
                return CVM_FALSE; /* Abort scan. */
            }
        }
    }
    return CVM_TRUE; /* OK to continue scan. */
}

/*
 * Purpose: Posts startup events required by JVMTI.  This includes defining
 * events which transpired prior to the completion of VM
 * initialization.
 * NOTE: Called while GC safe.
 */
void CVMjvmtiPostStartUpEvents(CVMExecEnv *ee)
{
    CVMObjectICell *threadICell = CVMcurrentThreadICell(ee);

    /* NOTE: It is not trivial to reconstruct the order in which all the
       defining events prior to this point has occurred.  Also, these events
       don't exist for preloaded objects.  The following is a gross
       approximation which will hopefully lessen the need missed events.
       This is a temporary measure until a need for a more comprehensive
       solution is found, or until a way to compute the order is found. */

    CVMassert(CVMD_isgcSafe(ee));

    /* 1. Post the current Thread Start Event first because all events will
       make reference to this thread: */
    if (jvmti_should_post_thread_life()) {
	CVMjvmtiPostThreadStartEvent(ee, threadICell);
	if (CVMexceptionOccurred(ee)) {
	    return;
	}
    }

    /* 2. Post Class Load Events: */
    if (jvmti_should_post_class_load()
#ifdef CVM_CLASSLIB_JCOV
        || jvmti_should_post_class_file_load_hook()
#endif
        ) {
        /* Posts a JVMTI_EVENT_CLASS_LOAD event for each ROMized class: */
        CVMclassIterateAllClasses(ee, CVMjvmtiPostStartupClassEvents, 0);
        if (CVMexceptionOccurred(ee)) {
            return;
        }
    }

    /* 3. Post Object Allocation Events: */
    if (jvmti_should_post_vm_object_alloc()) {
#ifdef CVM_JIT
	CVMsysMutexLock(ee, &CVMglobals.jitLock);
#endif
        CVMsysMutexLock(ee, &CVMglobals.heapLock);
	CVMpreloaderIteratePreloadedObjects(ee,
					    CVMjvmtiPostStartupObjectEvents, (void *)ee);
	if (!CVMexceptionOccurred(ee)) {
	    CVMgcimplIterateHeap(ee, CVMjvmtiPostStartupObjectEvents,
				 (void *)ee);
	}
        CVMsysMutexUnlock(ee, &CVMglobals.heapLock);
#ifdef CVM_JIT
	CVMsysMutexUnlock(ee, &CVMglobals.jitLock);
#endif
        if (CVMexceptionOccurred(ee)) {
            return;
        }
    }

    /* 4. Post other Thread Start Events: */
    if (jvmti_should_post_thread_life()) {
	/* try to avoid holding a lock across an event */
	int count = 0;
	int i;
	CVMObjectICell **threads;
#ifdef CVM_JIT
	CVMsysMutexLock(ee, &CVMglobals.jitLock);
#endif
        CVMsysMutexLock(ee, &CVMglobals.heapLock);
        CVMsysMutexLock(ee, &CVMglobals.threadLock);
        CVM_WALK_ALL_THREADS(ee, threadEE, {
		if (ee != threadEE && !CVMexceptionOccurred(ee)) {
		    count++;
		}
	    });
	CVMjvmtiAllocate(count * sizeof(jthread), (unsigned char **)&threads);
	i = 0;
        CVM_WALK_ALL_THREADS(ee, threadEE, {
		if (ee != threadEE && !CVMexceptionOccurred(ee)) {
		    threads[i++] = CVMcurrentThreadICell(threadEE);
		}
	    });
        CVMsysMutexUnlock(ee, &CVMglobals.threadLock);
        CVMsysMutexUnlock(ee, &CVMglobals.heapLock);
#ifdef CVM_JIT
	CVMsysMutexUnlock(ee, &CVMglobals.jitLock);
#endif
	for (i = 0; i < count; i++) {
	    CVMjvmtiPostThreadStartEvent(ee, threads[i]);
	}
        if (CVMexceptionOccurred(ee)) {
            return;
        }
    }
}

void
CVMjvmtiPostNativeMethodBind(CVMExecEnv* ee, CVMMethodBlock *mb,
			     CVMUint8 *nativeCode, CVMUint8 **new_nativeCode)
{    
    JvmtiEnv *jvmti_env;
    jvmtiEventNativeMethodBind callback;
    JNIEnv *env = CVMexecEnv2JniEnv(ee);

    if (CVMjvmtiGetPhase() != JVMTI_PHASE_PRIMORDIAL &&
	CVMjvmtiGetPhase() != JVMTI_PHASE_ONLOAD &&
	CVMjvmtiGetPhase() != JVMTI_PHASE_LIVE) {
	return;
    }
    jvmti_env = CVMglobals.jvmtiStatics._jvmti_env;
    if (jvmti_env != NULL) {
	callback = jvmti_env->_event_callbacks.NativeMethodBind;
	if (callback != NULL) {
	    if ((*env)->PushLocalFrame(env, 1) < 0) {
		return;
	    }
	    (*callback)(&jvmti_env->_jvmti_external, env,
			(*env)->NewLocalRef(env, CVMcurrentThreadICell(ee)),
			CVMmbMethodID(mb), nativeCode,
			(void **)new_nativeCode);
	    (*env)->PopLocalFrame(env, 0);
	}
    }
}

/* Purpose: Posts a JVMTI_EVENT_MONITOR_CONTENDED_ENTER */
/* NOTE: Called while GC safe. */
void CVMjvmtiPostMonitorContendedEnterEvent(CVMExecEnv *ee,
                                            CVMProfiledMonitor *pm)
{
    JvmtiEnv *jvmti_env;
    jvmtiEventMonitorContendedEnter callback;
    JNIEnv *env = CVMexecEnv2JniEnv(ee);
    jobject monitor_object;

    CVMassert(!CVMgcIsGCThread(ee));
    CVMassert(CVMD_isgcSafe(ee));
    CHECK_PHASEV(JVMTI_PHASE_LIVE);

    if (!MUST_NOTIFY_THREAD(ee, JVMTI_EVENT_MONITOR_CONTENDED_ENTER)) {
	return;
    }
    jvmti_env = CVMglobals.jvmtiStatics._jvmti_env;
    if (jvmti_env != NULL) {
	callback = jvmti_env->_event_callbacks.MonitorContendedEnter;
	if (callback != NULL) {
	    CVMObjMonitor *mon = CVMcastProfiledMonitor2ObjMonitor(pm);
	    if ((*env)->PushLocalFrame(env, 2) < 0) {
		return;
	    }
	    CVMID_localrootBegin(ee); {
		CVMID_localrootDeclare(CVMObjectICell, tempObj);
		CVMD_gcUnsafeExec(ee, {
			CVMID_icellSetDirect(ee, tempObj, mon->obj);
		    });
		monitor_object = (*env)->NewLocalRef(env, tempObj);
	    } CVMID_localrootEnd();

	    (*callback)(&jvmti_env->_jvmti_external, env,
			(*env)->NewLocalRef(env, CVMcurrentThreadICell(ee)),
			monitor_object);
	    (*env)->PopLocalFrame(env, 0);
	}
    }
}

/* Purpose: Posts a  JVMTI_EVENT_MONITOR_CONTENDED_ENTERED event. */
/* NOTE: Called while GC safe. */
void CVMjvmtiPostMonitorContendedEnteredEvent(CVMExecEnv *ee,
                                              CVMProfiledMonitor *pm)
{
    JvmtiEnv *jvmti_env;
    jvmtiEventMonitorContendedEntered callback;
    JNIEnv *env = CVMexecEnv2JniEnv(ee);
    jobject monitor_object;

    CVMassert(!CVMgcIsGCThread(ee));
    CVMassert(CVMD_isgcSafe(ee));

    CHECK_PHASEV(JVMTI_PHASE_LIVE);
    if (!MUST_NOTIFY_THREAD(ee, JVMTI_EVENT_MONITOR_CONTENDED_ENTERED)) {
	return;
    }
    jvmti_env = CVMglobals.jvmtiStatics._jvmti_env;
    if (jvmti_env != NULL) {
	callback = jvmti_env->_event_callbacks.MonitorContendedEntered;
	if (callback != NULL) {
	    CVMObjMonitor *mon = CVMcastProfiledMonitor2ObjMonitor(pm);
	    if ((*env)->PushLocalFrame(env, 2) < 0) {
		return;
	    }
	    CVMID_localrootBegin(ee); {
		CVMID_localrootDeclare(CVMObjectICell, tempObj);
		CVMD_gcUnsafeExec(ee, {
			CVMID_icellSetDirect(ee, tempObj, mon->obj);
		    });
		monitor_object = (*env)->NewLocalRef(env, tempObj);
	    } CVMID_localrootEnd();
	    (*callback)(&jvmti_env->_jvmti_external, env,
			(*env)->NewLocalRef(env, CVMcurrentThreadICell(ee)),
			monitor_object);
	    (*env)->PopLocalFrame(env, 0);
	}
    }
}

/* Purpose: Posts a JVMTI_EVENT_MONITOR_WAIT_EVENT */
/* NOTE: Called while GC safe. */
void CVMjvmtiPostMonitorWaitEvent(CVMExecEnv *ee,
				  jobject obj, jlong millis)
{
    JvmtiEnv *jvmti_env;
    jvmtiEventMonitorWait callback;
    JNIEnv *env = CVMexecEnv2JniEnv(ee);

    CVMassert(!CVMgcIsGCThread(ee));
    CVMassert(CVMD_isgcSafe(ee));

    CHECK_PHASEV(JVMTI_PHASE_LIVE);
    if (!MUST_NOTIFY_THREAD(ee, JVMTI_EVENT_MONITOR_WAIT)) {
	return;
    }
    jvmti_env = CVMglobals.jvmtiStatics._jvmti_env;
    if (jvmti_env != NULL) {
	callback = jvmti_env->_event_callbacks.MonitorWait;
	if (callback != NULL) {
	    if ((*env)->PushLocalFrame(env, 2) < 0) {
		return;
	    }
	    (*callback)(&jvmti_env->_jvmti_external, env,
			(*env)->NewLocalRef(env, CVMcurrentThreadICell(ee)),
			(*env)->NewLocalRef(env, obj), millis);
	    (*env)->PopLocalFrame(env, 0);
	}
    }
}

/* Purpose: Posts a JVMTI_EVENT_MONITOR_WAITED_EVENT */
/* NOTE: Called while GC safe. */
void CVMjvmtiPostMonitorWaitedEvent(CVMExecEnv *ee,
				  jobject obj, CVMBool timedout)
{
    JvmtiEnv *jvmti_env;
    jvmtiEventMonitorWaited callback;
    JNIEnv *env = CVMexecEnv2JniEnv(ee);

    CVMassert(!CVMgcIsGCThread(ee));
    CVMassert(CVMD_isgcSafe(ee));

    CHECK_PHASEV(JVMTI_PHASE_LIVE);
    if (!MUST_NOTIFY_THREAD(ee, JVMTI_EVENT_MONITOR_WAITED)) {
	return;
    }
    jvmti_env = CVMglobals.jvmtiStatics._jvmti_env;
    if (jvmti_env != NULL) {
	callback = jvmti_env->_event_callbacks.MonitorWaited;
	if (callback != NULL) {
	    if ((*env)->PushLocalFrame(env, 2) < 0) {
		return;
	    }
	    (*callback)(&jvmti_env->_jvmti_external, env,
			(*env)->NewLocalRef(env, CVMcurrentThreadICell(ee)),
			(*env)->NewLocalRef(env, obj), timedout);
	    (*env)->PopLocalFrame(env, 0);
	}
    }
}

void CVMjvmtiPostObjectFreeEvent(CVMObject *obj)
{
    JvmtiEnv *jvmti_env;
    jvmtiEventObjectFree callback;
    CVMExecEnv *ee = CVMgetEE();
    int hash;
    jlong tag;

    CVMassert(CVMD_isgcSafe(ee));
    CVMassert(CVMgcIsGCThread(ee));

    CHECK_PHASEV(JVMTI_PHASE_LIVE);
    jvmti_env = CVMglobals.jvmtiStatics._jvmti_env;
    if (jvmti_env != NULL) {
	callback = jvmti_env->_event_callbacks.ObjectFree;
	if (callback != NULL) {
	    /*	    if ((*env)->PushLocalFrame(env, 2) < 0) {
		return;
	    }
	    CVMID_localrootBegin(ee); {
		CVMID_localrootDeclare(CVMObjectICell, tempObj);
		CVMD_gcUnsafeExec(ee, {
			CVMID_icellSetDirect(ee, tempObj, obj);
		    });
		object = (*env)->NewLocalRef(env, tempObj);
	    } CVMID_localrootEnd();
	    */
	    hash = CVMobjectGetHashNoSet(ee, obj);
	    if (hash == CVM_OBJECT_NO_HASH) {
		return;
	    }
	    CVMjvmtiTagGetTagGC(obj, &tag);
	    if (tag == 0) {
		return;
	    }
	    (*callback)(&jvmti_env->_jvmti_external, tag);
	    /*	    (*env)->PopLocalFrame(env, 0); */
	}
    }
}

_jvmtiExports jvmtiExports;

void jvmti_set_can_get_source_debug_extension(jboolean on) {
    jvmtiExports._can_get_source_debug_extension = (on != 0);
}
void jvmti_set_can_examine_or_deopt_anywhere(jboolean on) {
    jvmtiExports._can_examine_or_deopt_anywhere = (on != 0);
}
void jvmti_set_can_maintain_original_method_order(jboolean on) {
    jvmtiExports._can_maintain_original_method_order = (on != 0);
}
void jvmti_set_can_post_interpreter_events(jboolean on) {
    jvmtiExports._can_post_interpreter_events = (on != 0);
}
void jvmti_set_can_hotswap_or_post_breakpoint(jboolean on) {
    jvmtiExports._can_hotswap_or_post_breakpoint = (on != 0);
}
void jvmti_set_can_modify_any_class(jboolean on) {
    jvmtiExports._can_modify_any_class = (on != 0);
}
void jvmti_set_can_walk_any_space(jboolean on) {
    jvmtiExports._can_walk_any_space = (on != 0);
}
void jvmti_set_can_access_local_variables(jboolean on) {
    jvmtiExports._can_access_local_variables = (on != 0);
}
void jvmti_set_can_post_exceptions(jboolean on) {
    jvmtiExports._can_post_exceptions = (on != 0);
}
void jvmti_set_can_post_breakpoint(jboolean on) {
    jvmtiExports._can_post_breakpoint = (on != 0);
}
void jvmti_set_can_post_field_access(jboolean on) {
    jvmtiExports._can_post_field_access = (on != 0);
}
void jvmti_set_can_post_field_modification(jboolean on) {
    jvmtiExports._can_post_field_modification = (on != 0);
}
void jvmti_set_can_post_method_entry(jboolean on) {
    jvmtiExports._can_post_method_entry = (on != 0);
}
void jvmti_set_can_post_method_exit(jboolean on) {
    jvmtiExports._can_post_method_exit = (on != 0);
}
void jvmti_set_can_pop_frame(jboolean on) {
    jvmtiExports._can_pop_frame = (on != 0);
}
void jvmti_set_can_force_early_return(jboolean on) {
    jvmtiExports._can_force_early_return = (on != 0);
}


void jvmti_set_should_post_single_step(jboolean on) {
    jvmtiExports._should_post_single_step = on;
}
void jvmti_set_should_post_field_access(jboolean on) {
    jvmtiExports._should_post_field_access = on;
}
void jvmti_set_should_post_field_modification(jboolean on) {
    jvmtiExports._should_post_field_modification = on;
}
void jvmti_set_should_post_class_load(jboolean on) {
    jvmtiExports._should_post_class_load = on;
}
void jvmti_set_should_post_class_prepare(jboolean on) {
    jvmtiExports._should_post_class_prepare = on;
}
void jvmti_set_should_post_class_unload(jboolean on) {
    jvmtiExports._should_post_class_unload = on;
}
void jvmti_set_should_post_class_file_load_hook(jboolean on) {
    jvmtiExports._should_post_class_file_load_hook = on;
}   
void jvmti_set_should_post_native_method_bind(jboolean on) {
    jvmtiExports._should_post_native_method_bind = on;
}
void jvmti_set_should_post_compiled_method_load(jboolean on) {
    jvmtiExports._should_post_compiled_method_load = on;
}
void jvmti_set_should_post_compiled_method_unload(jboolean on) {
    jvmtiExports._should_post_compiled_method_unload = on;
}
void jvmti_set_should_post_dynamic_code_generated(jboolean on) {
    jvmtiExports._should_post_dynamic_code_generated = on;
}   
void jvmti_set_should_post_monitor_contended_enter(jboolean on) {
    jvmtiExports._should_post_monitor_contended_enter = on;
}
void jvmti_set_should_post_monitor_contended_entered(jboolean on) {
    jvmtiExports._should_post_monitor_contended_entered = on;
}
void jvmti_set_should_post_monitor_wait(jboolean on) {
    jvmtiExports._should_post_monitor_wait = on;
}
void jvmti_set_should_post_monitor_waited(jboolean on) {
    jvmtiExports._should_post_monitor_waited = on;
}
void jvmti_set_should_post_garbage_collection_start(jboolean on) {
    jvmtiExports._should_post_garbage_collection_start = on;
}
void jvmti_set_should_post_garbage_collection_finish(jboolean on) {
    jvmtiExports._should_post_garbage_collection_finish = on;
}
void jvmti_set_should_post_data_dump(jboolean on) {
    jvmtiExports._should_post_data_dump = on;
}   
void jvmti_set_should_post_object_free(jboolean on) {
    jvmtiExports._should_post_object_free = on;
}
void jvmti_set_should_post_vm_object_alloc(jboolean on) {
    jvmtiExports._should_post_vm_object_alloc = on;
}    

void jvmti_set_should_post_thread_life(jboolean on) {
    jvmtiExports._should_post_thread_life = on;
}
void jvmti_set_should_clean_up_heap_objects(jboolean on) {
    jvmtiExports._should_clean_up_heap_objects = on;
}

jlong
jvmti_get_thread_event_enabled(CVMExecEnv *ee) {
    return ee->_jvmti_event_enabled._enabled_bits;
}

void
jvmti_set_should_post_any_thread_event(CVMExecEnv *ee, jlong enabled) {
    ee->_jvmti_event_enabled._enabled_bits = enabled;
}

/*
 * CompiledMethodUnload events are reported from the VM thread so they
 * are collected in lists (of jmethodID/addresses) and the events are posted later
 * from threads posting CompieldMethodLoad or DynamicCodeGenerated events.

 * tests if there are CompiledMethodUnload events pending
 */
jboolean have_pending_compiled_method_unload_events() { 
    return _have_pending_compiled_method_unload_events; 
}

void set_has_redefined_a_class() {
    jvmtiExports._has_redefined_a_class = CVM_TRUE;
}

jboolean has_redefined_a_class() {
    return jvmtiExports._has_redefined_a_class;
}

jboolean all_dependencies_are_recorded() {
    return jvmtiExports._all_dependencies_are_recorded;
}

void jvmti_set_all_dependencies_are_recorded(jboolean on) {
    jvmtiExports._all_dependencies_are_recorded = (on != 0);
}

jboolean jvmti_can_get_source_debug_extension() {
    return jvmtiExports._can_get_source_debug_extension;
}

/* BP, expression stack, hotswap, interp_only, local_var, monitor info */
jboolean jvmti_can_examine_or_deopt_anywhere() {
    return jvmtiExports._can_examine_or_deopt_anywhere;
}

/* JVMTI spec requires this, does this matter for JVMTI? */
jboolean jvmti_can_maintain_original_method_order() {
    return jvmtiExports._can_maintain_original_method_order;
}

/* any of single-step, method-entry/exit, frame-pop, and field-access/modification */
jboolean jvmti_can_post_interpreter_events() {
    return jvmtiExports._can_post_interpreter_events;
}

jboolean jvmti_can_hotswap_or_post_breakpoint() {
    return jvmtiExports._can_hotswap_or_post_breakpoint;
}

jboolean jvmti_can_modify_any_class() {
    return jvmtiExports._can_modify_any_class;
}

jboolean jvmti_can_walk_any_space() {
    return jvmtiExports._can_walk_any_space;
}

/* can retrieve frames, set/get local variables or hotswap */
jboolean jvmti_can_access_local_variables() {
    return jvmtiExports._can_access_local_variables;
}

/* throw or catch */
jboolean jvmti_can_post_exceptions() {
    return jvmtiExports._can_post_exceptions;
}

jboolean jvmti_can_post_breakpoint() {
    return jvmtiExports._can_post_breakpoint;
}
jboolean jvmti_can_post_field_access() {
    return jvmtiExports._can_post_field_access;
}
jboolean jvmti_can_post_field_modification() {
    return jvmtiExports._can_post_field_modification;
}
jboolean jvmti_can_post_method_entry() {
    return jvmtiExports._can_post_method_entry;
}
jboolean jvmti_can_post_method_exit() {
    return jvmtiExports._can_post_method_exit;
}
jboolean jvmti_can_pop_frame() {
    return jvmtiExports._can_pop_frame;
}
jboolean jvmti_can_force_early_return() {
    return jvmtiExports._can_force_early_return;
}


/*
 * the below maybe don't have to be (but are for now) fixed conditions here 
 * any events can be enabled
 */
jboolean jvmti_should_post_thread_life() {
    return jvmtiExports._should_post_thread_life;
}


/* ------ DYNAMIC conditions here ------------ */

jboolean jvmti_should_post_single_step() {
    return jvmtiExports._should_post_single_step;
}
jboolean jvmti_should_post_field_access() {
    return jvmtiExports._should_post_field_access;
}
jboolean jvmti_should_post_field_modification() {
    return jvmtiExports._should_post_field_modification;
}
jboolean jvmti_should_post_class_load() {
    return jvmtiExports._should_post_class_load;
}
jboolean jvmti_should_post_class_prepare() {
    return jvmtiExports._should_post_class_prepare;
}
jboolean jvmti_should_post_class_unload() {
    return jvmtiExports._should_post_class_unload;
}
jboolean jvmti_should_post_class_file_load_hook() {
    return jvmtiExports._should_post_class_file_load_hook;
}
jboolean jvmti_should_post_native_method_bind() {
    return jvmtiExports._should_post_native_method_bind;
}
jboolean jvmti_should_post_compiled_method_load() {
    return jvmtiExports._should_post_compiled_method_load;
}
jboolean jvmti_should_post_compiled_method_unload() {
    return jvmtiExports._should_post_compiled_method_unload;
}
jboolean jvmti_should_post_dynamic_code_generated() {
    return jvmtiExports._should_post_dynamic_code_generated;
}
jboolean jvmti_should_post_monitor_contended_enter() {
    return jvmtiExports._should_post_monitor_contended_enter;
}
jboolean jvmti_should_post_monitor_contended_entered() {
    return jvmtiExports._should_post_monitor_contended_entered;
}
jboolean jvmti_should_post_monitor_wait() {
    return jvmtiExports._should_post_monitor_wait;
}
jboolean jvmti_should_post_monitor_waited() {
    return jvmtiExports._should_post_monitor_waited;
}
jboolean jvmti_should_post_data_dump() {
    return jvmtiExports._should_post_data_dump;
}
jboolean jvmti_should_post_garbage_collection_start() {
    return jvmtiExports._should_post_garbage_collection_start;
}
jboolean jvmti_should_post_garbage_collection_finish() {
    return jvmtiExports._should_post_garbage_collection_finish;
}
jboolean jvmti_should_post_object_free() {
    return jvmtiExports._should_post_object_free;
}
jboolean jvmti_should_post_vm_object_alloc() {
    return jvmtiExports._should_post_vm_object_alloc;
}

/* we are holding objects on the heap - need to talk to GC - e.g. breakpoint info */
jboolean jvmti_should_clean_up_heap_objects() {
    return jvmtiExports._should_clean_up_heap_objects;
}

/* field access management */
CVMAddr  get_field_access_count_addr();

/* field modification management */
CVMAddr  get_field_modification_count_addr();

/* single stepping management methods */
void at_single_stepping_point(jthread *thread, CVMMethodBlock method, CVMAddr location);
void expose_single_stepping(jthread *thread);
jboolean hide_single_stepping(jthread *thread);

/* Methods that notify the debugger that something interesting has happened in the VM. */

/*
 * post a DynamicCodeGenerated event while holding locks in the VM. Any event
 * posted using this function is recorded by the enclosing event collector
 * -- JvmtiDynamicCodeEventCollector.
 */

/* Collects vm internal objects for later event posting. */
/*
  void vm_object_alloc_event_collector(oop object) {
  if (should_post_vm_object_alloc()) {
  record_vm_internal_object_allocation(object);
  }      
  }
*/
/* attach support */
jint load_agent_library(AttachOperation* op, char* out);

/* ----------------- */

jboolean is_jvmti_version(jint version) {
    return (version & JVMTI_VERSION_MASK) == JVMTI_VERSION_VALUE;
}
jint get_jvmti_interface(JavaVM *jvm, void **penv, jint version);


void
CVMjvmtiEnterPrimordialPhase() {
  
    _jvmtiphase = JVMTI_PHASE_PRIMORDIAL;
}

void
CVMjvmtiEnterStartPhase() {
    _jvmtiphase = JVMTI_PHASE_START;
}

void
CVMjvmtiEnterOnloadPhase() {
    _jvmtiphase = JVMTI_PHASE_ONLOAD;
}

void
CVMjvmtiEnterLivePhase() {
    _jvmtiphase = JVMTI_PHASE_LIVE;
}

void
CVMjvmtiEnterDeadPhase() {
    _jvmtiphase = JVMTI_PHASE_DEAD;
}

jvmtiPhase
CVMjvmtiGetPhase() {
    return _jvmtiphase;
}

jvmtiError
CVMjvmtiAllocate(jlong size, unsigned char **mem) {
    JvmtiEnv *jvmti_env = CVMglobals.jvmtiStatics._jvmti_env;
    CVMassert((jvmti_env != NULL));
    CVMassert((jvmti_env->_jvmti_external != NULL));
    jvmti_env->_jvmti_external->Allocate(&jvmti_env->_jvmti_external, size, mem);
    CVMassert(*mem != NULL);
    return JVMTI_ERROR_NONE;
}

jvmtiError
CVMjvmtiDeallocate(unsigned char *mem) {
    JvmtiEnv *jvmti_env = CVMglobals.jvmtiStatics._jvmti_env;
    CVMassert((jvmti_env != NULL));
    CVMassert((jvmti_env->_jvmti_external != NULL));
    jvmti_env->_jvmti_external->Deallocate(&jvmti_env->_jvmti_external, mem);
    return JVMTI_ERROR_NONE;
}

CVMBool
CVMjvmtiClassBeingRedefined(CVMExecEnv *ee, CVMClassBlock *cb) {

    ThreadNode *node;
    if (CVMglobals.jvmtiStatics.jvmtiInitialized) {
	node = CVMjvmtiFindThread(ee, CVMcurrentThreadICell(ee));
	return node == NULL ? CVM_FALSE : (node->redefine_cb == cb);
    }
    return CVM_FALSE;
}

void
CVMjvmtiSetDebuggerConnected(CVMBool connected) {
    _debuggerConnected = connected;
}

CVMBool
CVMjvmtiDebuggerConnected() {
    return _debuggerConnected;
}

#endif /* CVM_JVMTI */
