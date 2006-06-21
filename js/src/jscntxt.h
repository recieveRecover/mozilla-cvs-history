/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef jscntxt_h___
#define jscntxt_h___
/*
 * JS execution context.
 */
#include "jsarena.h" /* Added by JSIFY */
#include "jsclist.h"
#include "jslong.h"
#include "jsatom.h"
#include "jsconfig.h"
#include "jsdhash.h"
#include "jsgc.h"
#include "jsinterp.h"
#include "jsobj.h"
#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsregexp.h"

JS_BEGIN_EXTERN_C

#ifdef JS_THREADSAFE

/*
 * Structure uniquely representing a thread.  It holds thread-private data
 * that can be accessed without a global lock.
 */
struct JSThread {
    /* Linked list of all contexts active on this thread. */
    JSCList             contextList;

    /* Opaque thread-id, from NSPR's PR_GetCurrentThread(). */
    jsword              id;

    /* Thread-local gc free lists array. */
    JSGCThing           *gcFreeLists[GC_NUM_FREELISTS];

    /*
     * Thread-local version of JSRuntime.gcMallocBytes to avoid taking
     * locks on each JS_malloc.
     */
    uint32              gcMallocBytes;
};

extern void JS_DLL_CALLBACK
js_ThreadDestructorCB(void *ptr);

extern JSBool
js_SetContextThread(JSContext *cx);

extern void
js_ClearContextThread(JSContext *cx);

extern JSThread *
js_GetCurrentThread(JSRuntime *rt);

#endif /* JS_THREADSAFE */

typedef enum JSGCMode { JS_NO_GC, JS_MAYBE_GC, JS_FORCE_GC } JSGCMode;

typedef enum JSRuntimeState {
    JSRTS_DOWN,
    JSRTS_LAUNCHING,
    JSRTS_UP,
    JSRTS_LANDING
} JSRuntimeState;

typedef struct JSPropertyTreeEntry {
    JSDHashEntryHdr     hdr;
    JSScopeProperty     *child;
} JSPropertyTreeEntry;

/*
 * Forward declaration for opaque JSRuntime.nativeIteratorStates.
 */
typedef struct JSNativeIteratorState JSNativeIteratorState;

struct JSRuntime {
    /* Runtime state, synchronized by the stateChange/gcLock condvar/lock. */
    JSRuntimeState      state;

    /* Garbage collector state, used by jsgc.c. */
    JSGCArenaList       gcArenaList[GC_NUM_FREELISTS];
    JSDHashTable        gcRootsHash;
    JSDHashTable        *gcLocksHash;
    jsrefcount          gcKeepAtoms;
    uint32              gcBytes;
    uint32              gcLastBytes;
    uint32              gcMaxBytes;
    uint32              gcMaxMallocBytes;
    uint32              gcLevel;
    uint32              gcNumber;

    /*
     * NB: do not pack another flag here by claiming gcPadding unless the new
     * flag is written only by the GC thread.  Atomic updates to packed bytes
     * are not guaranteed, so stores issued by one thread may be lost due to
     * unsynchronized read-modify-write cycles on other threads.
     */
    JSPackedBool        gcPoke;
    JSPackedBool        gcRunning;
    JSPackedBool        gcClosePhase;
    uint8               gcPadding;

    JSGCCallback        gcCallback;
    uint32              gcMallocBytes;
    JSGCArena           *gcUnscannedArenaStackTop;
#ifdef DEBUG
    size_t              gcUnscannedBagSize;
#endif

    /*
     * API compatibility requires keeping GCX_PRIVATE bytes separate from the
     * original GC types' byte tally.  Otherwise embeddings that configure a
     * good limit for pre-GCX_PRIVATE versions of the engine will see memory
     * over-pressure too often, possibly leading to failed last-ditch GCs.
     *
     * The new XML GC-thing types do add to gcBytes, and they're larger than
     * the original GC-thing type size (8 bytes on most architectures).  So a
     * user who enables E4X may want to increase the maxbytes value passed to
     * JS_NewRuntime.  TODO: Note this in the API docs.
     */
    uint32              gcPrivateBytes;

    /*
     * Table for tracking objects of extended classes that have non-null close
     * hooks, and need the GC to perform two-phase finalization.
     */
    JSGCCloseTable      gcCloseTable;

#ifdef JS_GCMETER
    JSGCStats           gcStats;
#endif

    /* Literal table maintained by jsatom.c functions. */
    JSAtomState         atomState;

    /* Random number generator state, used by jsmath.c. */
    JSBool              rngInitialized;
    int64               rngMultiplier;
    int64               rngAddend;
    int64               rngMask;
    int64               rngSeed;
    jsdouble            rngDscale;

    /* Well-known numbers held for use by this runtime's contexts. */
    jsdouble            *jsNaN;
    jsdouble            *jsNegativeInfinity;
    jsdouble            *jsPositiveInfinity;

#ifdef JS_THREADSAFE
    JSLock              *deflatedStringCacheLock;
#endif
    JSHashTable         *deflatedStringCache;
#ifdef DEBUG
    uint32              deflatedStringCacheBytes;
#endif

    /* Empty string held for use by this runtime's contexts. */
    JSString            *emptyString;

    /* List of active contexts sharing this runtime; protected by gcLock. */
    JSCList             contextList;

    /* These are used for debugging -- see jsprvtd.h and jsdbgapi.h. */
    JSTrapHandler       interruptHandler;
    void                *interruptHandlerData;
    JSNewScriptHook     newScriptHook;
    void                *newScriptHookData;
    JSDestroyScriptHook destroyScriptHook;
    void                *destroyScriptHookData;
    JSTrapHandler       debuggerHandler;
    void                *debuggerHandlerData;
    JSSourceHandler     sourceHandler;
    void                *sourceHandlerData;
    JSInterpreterHook   executeHook;
    void                *executeHookData;
    JSInterpreterHook   callHook;
    void                *callHookData;
    JSObjectHook        objectHook;
    void                *objectHookData;
    JSTrapHandler       throwHook;
    void                *throwHookData;
    JSDebugErrorHook    debugErrorHook;
    void                *debugErrorHookData;

    /* More debugging state, see jsdbgapi.c. */
    JSCList             trapList;
    JSCList             watchPointList;

    /* Weak links to properties, indexed by quickened get/set opcodes. */
    /* XXX must come after JSCLists or MSVC alignment bug bites empty lists */
    JSPropertyCache     propertyCache;

    /* Client opaque pointer */
    void                *data;

#ifdef JS_THREADSAFE
    /* These combine to interlock the GC and new requests. */
    PRLock              *gcLock;
    PRCondVar           *gcDone;
    PRCondVar           *requestDone;
    uint32              requestCount;
    JSThread            *gcThread;

    /* Lock and owning thread pointer for JS_LOCK_RUNTIME. */
    PRLock              *rtLock;
#ifdef DEBUG
    jsword              rtLockOwner;
#endif

    /* Used to synchronize down/up state change; protected by gcLock. */
    PRCondVar           *stateChange;

    /* Used to serialize cycle checks when setting __proto__ or __parent__. */
    PRLock              *setSlotLock;
    PRCondVar           *setSlotDone;
    JSBool              setSlotBusy;
    JSScope             *setSlotScope;  /* deadlock avoidance, see jslock.c */

    /*
     * State for sharing single-threaded scopes, once a second thread tries to
     * lock a scope.  The scopeSharingDone condvar is protected by rt->gcLock,
     * to minimize number of locks taken in JS_EndRequest.
     *
     * The scopeSharingTodo linked list is likewise "global" per runtime, not
     * one-list-per-context, to conserve space over all contexts, optimizing
     * for the likely case that scopes become shared rarely, and among a very
     * small set of threads (contexts).
     */
    PRCondVar           *scopeSharingDone;
    JSScope             *scopeSharingTodo;

/*
 * Magic terminator for the rt->scopeSharingTodo linked list, threaded through
 * scope->u.link.  This hack allows us to test whether a scope is on the list
 * by asking whether scope->u.link is non-null.  We use a large, likely bogus
 * pointer here to distinguish this value from any valid u.count (small int)
 * value.
 */
#define NO_SCOPE_SHARING_TODO   ((JSScope *) 0xfeedbeef)

    /*
     * The index for JSThread info, returned by PR_NewThreadPrivateIndex.
     * The value is visible and shared by all threads, but the data is
     * private to each thread.
     */
    PRUintn             threadTPIndex;
#endif /* JS_THREADSAFE */

    /*
     * Check property accessibility for objects of arbitrary class.  Used at
     * present to check f.caller accessibility for any function object f.
     */
    JSCheckAccessOp     checkObjectAccess;

    /* Security principals serialization support. */
    JSPrincipalsTranscoder principalsTranscoder;

    /* Optional hook to find principals for an object in this runtime. */
    JSObjectPrincipalsFinder findObjectPrincipals;

    /* Shared scope property tree, and allocator for its nodes. */
    JSDHashTable        propertyTreeHash;
    JSScopeProperty     *propertyFreeList;
    JSArenaPool         propertyArenaPool;

    /* Script filename table. */
    struct JSHashTable  *scriptFilenameTable;
    JSCList             scriptFilenamePrefixes;
#ifdef JS_THREADSAFE
    PRLock              *scriptFilenameTableLock;
#endif

    /* Number localization, used by jsnum.c */
    const char          *thousandsSeparator;
    const char          *decimalSeparator;
    const char          *numGrouping;

    /*
     * Weak references to lazily-created, well-known XML singletons.
     *
     * NB: Singleton objects must be carefully disconnected from the rest of
     * the object graph usually associated with a JSContext's global object,
     * including the set of standard class objects.  See jsxml.c for details.
     */
    JSObject            *anynameObject;
    JSObject            *functionNamespaceObject;

    /*
     * A helper list for the GC, so it can mark native iterator states. See
     * js_MarkNativeIteratorStates for details.
     */
    JSNativeIteratorState *nativeIteratorStates;

#ifdef DEBUG
    /* Function invocation metering. */
    jsrefcount          inlineCalls;
    jsrefcount          nativeCalls;
    jsrefcount          nonInlineCalls;
    jsrefcount          constructs;

    /* Scope lock and property metering. */
    jsrefcount          claimAttempts;
    jsrefcount          claimedScopes;
    jsrefcount          deadContexts;
    jsrefcount          deadlocksAvoided;
    jsrefcount          liveScopes;
    jsrefcount          sharedScopes;
    jsrefcount          totalScopes;
    jsrefcount          badUndependStrings;
    jsrefcount          liveScopeProps;
    jsrefcount          totalScopeProps;
    jsrefcount          livePropTreeNodes;
    jsrefcount          duplicatePropTreeNodes;
    jsrefcount          totalPropTreeNodes;
    jsrefcount          propTreeKidsChunks;
    jsrefcount          middleDeleteFixups;

    /* String instrumentation. */
    jsrefcount          liveStrings;
    jsrefcount          totalStrings;
    jsrefcount          liveDependentStrings;
    jsrefcount          totalDependentStrings;
    double              lengthSum;
    double              lengthSquaredSum;
    double              strdepLengthSum;
    double              strdepLengthSquaredSum;
#endif
};

#ifdef DEBUG
# define JS_RUNTIME_METER(rt, which)    JS_ATOMIC_INCREMENT(&(rt)->which)
# define JS_RUNTIME_UNMETER(rt, which)  JS_ATOMIC_DECREMENT(&(rt)->which)
#else
# define JS_RUNTIME_METER(rt, which)    /* nothing */
# define JS_RUNTIME_UNMETER(rt, which)  /* nothing */
#endif

#define JS_KEEP_ATOMS(rt)   JS_ATOMIC_INCREMENT(&(rt)->gcKeepAtoms);
#define JS_UNKEEP_ATOMS(rt) JS_ATOMIC_DECREMENT(&(rt)->gcKeepAtoms);

#if JS_HAS_LVALUE_RETURN
/*
 * Values for the cx->rval2set flag byte.  This flag tells whether cx->rval2
 * is unset (CLEAR), set to a jsval (VALUE) naming a property in the object
 * referenced by cx->fp->rval, or set to a jsid (ITERKEY) result of a native
 * iterator's it.next() call (where the return value of it.next() is the next
 * value in the iteration).
 *
 * The ITERKEY case is just an optimization for native iterators, as general
 * iterators can return an array of length 2 to return a [key, value] pair.
 */
enum { JS_RVAL2_CLEAR, JS_RVAL2_VALUE, JS_RVAL2_ITERKEY };
#endif

#ifdef JS_ARGUMENT_FORMATTER_DEFINED
/*
 * Linked list mapping format strings for JS_{Convert,Push}Arguments{,VA} to
 * formatter functions.  Elements are sorted in non-increasing format string
 * length order.
 */
struct JSArgumentFormatMap {
    const char          *format;
    size_t              length;
    JSArgumentFormatter formatter;
    JSArgumentFormatMap *next;
};
#endif

struct JSStackHeader {
    uintN               nslots;
    JSStackHeader       *down;
};

#define JS_STACK_SEGMENT(sh)    ((jsval *)(sh) + 2)

/*
 * Key and entry types for the JSContext.resolvingTable hash table, typedef'd
 * here because all consumers need to see these declarations (and not just the
 * typedef names, as would be the case for an opaque pointer-to-typedef'd-type
 * declaration), along with cx->resolvingTable.
 */
typedef struct JSResolvingKey {
    JSObject            *obj;
    jsid                id;
} JSResolvingKey;

typedef struct JSResolvingEntry {
    JSDHashEntryHdr     hdr;
    JSResolvingKey      key;
    uint32              flags;
} JSResolvingEntry;

#define JSRESFLAG_LOOKUP        0x1     /* resolving id from lookup */
#define JSRESFLAG_WATCH         0x2     /* resolving id from watch */

typedef struct JSLocalRootChunk JSLocalRootChunk;

#define JSLRS_CHUNK_SHIFT       8
#define JSLRS_CHUNK_SIZE        JS_BIT(JSLRS_CHUNK_SHIFT)
#define JSLRS_CHUNK_MASK        JS_BITMASK(JSLRS_CHUNK_SHIFT)

struct JSLocalRootChunk {
    jsval               roots[JSLRS_CHUNK_SIZE];
    JSLocalRootChunk    *down;
};

typedef struct JSLocalRootStack {
    uint32              scopeMark;
    uint32              rootCount;
    JSLocalRootChunk    *topChunk;
    JSLocalRootChunk    firstChunk;
} JSLocalRootStack;

#define JSLRS_NULL_MARK ((uint32) -1)

typedef struct JSTempValueRooter JSTempValueRooter;

/*
 * Context-linked stack of temporary GC roots.
 *
 * If count is -1, then u.value contains the single value to root.  Otherwise
 * u.array points to a stack-allocated vector of jsvals.  Note that the vector
 * may have length 0 or 1 for full generality, so we need -1 to discriminate
 * the union.
 *
 * To root a single GC-thing pointer, which need not be tagged and stored as a
 * jsval, use JS_PUSH_SINGLE_TEMP_ROOT.  The (jsval)(val) cast works because a
 * GC-thing is aligned on a 0 mod 8 boundary, and object has the 0 jsval tag.
 * So any GC-thing may be tagged as if it were an object and untagged, if it's
 * then used only as an opaque pointer until discriminated by other means than
 * tag bits (this is how the GC mark function uses its |thing| parameter -- it
 * consults GC-thing flags stored separately from the thing to decide the type
 * of thing).
 *
 * If you need to protect a result value that flows out of a C function across
 * several layers of other functions, use the js_LeaveLocalRootScopeWithResult
 * internal API (see further below) instead.
 */
struct JSTempValueRooter {
    JSTempValueRooter   *down;
    jsint               count;
    union {
        jsval           value;
        jsval           *array;
    } u;
};

#define JS_PUSH_TEMP_ROOT_COMMON(cx,tvr)                                      \
    JS_BEGIN_MACRO                                                            \
        JS_ASSERT((cx)->tempValueRooters != (tvr));                           \
        (tvr)->down = (cx)->tempValueRooters;                                 \
        (cx)->tempValueRooters = (tvr);                                       \
    JS_END_MACRO

#define JS_PUSH_SINGLE_TEMP_ROOT(cx,val,tvr)                                  \
    JS_BEGIN_MACRO                                                            \
        JS_PUSH_TEMP_ROOT_COMMON(cx, tvr);                                    \
        (tvr)->count = -1;                                                    \
        (tvr)->u.value = (jsval)(val);                                        \
    JS_END_MACRO

#define JS_PUSH_TEMP_ROOT(cx,cnt,arr,tvr)                                     \
    JS_BEGIN_MACRO                                                            \
        JS_PUSH_TEMP_ROOT_COMMON(cx, tvr);                                    \
        (tvr)->count = (cnt);                                                 \
        (tvr)->u.array = (arr);                                               \
    JS_END_MACRO

#define JS_POP_TEMP_ROOT(cx,tvr)                                              \
    JS_BEGIN_MACRO                                                            \
        JS_ASSERT((cx)->tempValueRooters == (tvr));                           \
        (cx)->tempValueRooters = (tvr)->down;                                 \
    JS_END_MACRO

#define JS_TEMP_ROOT_EVAL(cx,cnt,val,expr)                                    \
    JS_BEGIN_MACRO                                                            \
        JSTempValueRooter tvr;                                                \
        JS_PUSH_TEMP_ROOT(cx, cnt, val, &tvr);                                \
        (expr);                                                               \
        JS_POP_TEMP_ROOT(cx, &tvr);                                           \
    JS_END_MACRO

struct JSContext {
    /* JSRuntime contextList linkage. */
    JSCList             links;

    /* Interpreter activation count. */
    uintN               interpLevel;

    /* Limit pointer for checking stack consumption during recursion. */
    jsuword             stackLimit;

    /* Runtime version control identifier and equality operators. */
    uint16              version;
    jsbytecode          jsop_eq;
    jsbytecode          jsop_ne;

    /* Data shared by threads in an address space. */
    JSRuntime           *runtime;

    /* Stack arena pool and frame pointer register. */
    JSArenaPool         stackPool;
    JSStackFrame        *fp;

    /* Temporary arena pool used while compiling and decompiling. */
    JSArenaPool         tempPool;

    /* Top-level object and pointer to top stack frame's scope chain. */
    JSObject            *globalObject;

    /* Most recently created things by type, members of the GC's root set. */
    JSGCThing           *newborn[GCX_NTYPES];

    /* Atom root for the last-looked-up atom on this context. */
    JSAtom              *lastAtom;

    /* Root for the result of the most recent js_InternalInvoke call. */
    jsval               lastInternalResult;

    /* Regular expression class statics (XXX not shared globally). */
    JSRegExpStatics     regExpStatics;

    /* State for object and array toSource conversion. */
    JSSharpObjectMap    sharpObjectMap;

    /* Argument formatter support for JS_{Convert,Push}Arguments{,VA}. */
    JSArgumentFormatMap *argumentFormatMap;

    /* Last message string and trace file for debugging. */
    char                *lastMessage;
#ifdef DEBUG
    void                *tracefp;
#endif

    /* Per-context optional user callbacks. */
    JSBranchCallback    branchCallback;
    JSErrorReporter     errorReporter;

    /* Client opaque pointer */
    void                *data;

    /* GC and thread-safe state. */
    JSStackFrame        *dormantFrameChain; /* dormant stack frame to scan */
#ifdef JS_THREADSAFE
    JSThread            *thread;
    jsrefcount          requestDepth;
    JSScope             *scopeToShare;      /* weak reference, see jslock.c */
    JSScope             *lockedSealedScope; /* weak ref, for low-cost sealed
                                               scope locking */
    JSCList             threadLinks;        /* JSThread contextList linkage */

#define CX_FROM_THREAD_LINKS(tl) \
    ((JSContext *)((char *)(tl) - offsetof(JSContext, threadLinks)))
#endif

#if JS_HAS_LVALUE_RETURN
    /*
     * Secondary return value from native method called on the left-hand side
     * of an assignment operator.  The native should store the object in which
     * to set a property in *rval, and return the property's id expressed as a
     * jsval by calling JS_SetCallReturnValue2(cx, idval).
     */
    jsval               rval2;
    uint8               rval2set;
#endif

#if JS_HAS_XML_SUPPORT
    /*
     * Bit-set formed from binary exponentials of the XML_* tiny-ids defined
     * for boolean settings in jsxml.c, plus an XSF_CACHE_VALID bit.  Together
     * these act as a cache of the boolean XML.ignore* and XML.prettyPrinting
     * property values associated with this context's global object.
     */
    uint8               xmlSettingFlags;
#endif

    /*
     * True if creating an exception object, to prevent runaway recursion.
     * NB: creatingException packs with rval2set, #if JS_HAS_LVALUE_RETURN;
     * with xmlSettingFlags, #if JS_HAS_XML_SUPPORT; and with throwing below.
     */
    JSPackedBool        creatingException;

    /*
     * Exception state -- the exception member is a GC root by definition.
     * NB: throwing packs with creatingException and rval2set, above.
     */
    JSPackedBool        throwing;           /* is there a pending exception? */
    jsval               exception;          /* most-recently-thrown exception */
    /* Flag to indicate that we run inside gcCallback(cx, JSGC_MARK_END). */
    JSPackedBool        insideGCMarkCallback;

    /* Per-context options. */
    uint32              options;            /* see jsapi.h for JSOPTION_* */

    /* Locale specific callbacks for string conversion. */
    JSLocaleCallbacks   *localeCallbacks;

    /*
     * cx->resolvingTable is non-null and non-empty if we are initializing
     * standard classes lazily, or if we are otherwise recursing indirectly
     * from js_LookupProperty through a JSClass.resolve hook.  It is used to
     * limit runaway recursion (see jsapi.c and jsobj.c).
     */
    JSDHashTable        *resolvingTable;

    /* PDL of stack headers describing stack slots not rooted by argv, etc. */
    JSStackHeader       *stackHeaders;

    /* Optional stack of heap-allocated scoped local GC roots. */
    JSLocalRootStack    *localRootStack;

    /* Stack of thread-stack-allocated temporary GC roots. */
    JSTempValueRooter   *tempValueRooters;

    /* Iterator cache to speed up native default for-in loop case. */
    JSObject            *cachedIterObj;

#ifdef GC_MARK_DEBUG
    /* Top of the GC mark stack. */
    void                *gcCurrentMarkNode;
#endif
};

#define JS_THREAD_ID(cx)            ((cx)->thread ? (cx)->thread->id : 0)

/*
 * Slightly more readable macros for testing per-context option settings (also
 * to hide bitset implementation detail).
 *
 * JSOPTION_XML must be handled specially in order to propagate from compile-
 * to run-time (from cx->options to script->version/cx->version).  To do that,
 * we copy JSOPTION_XML from cx->options into cx->version as JSVERSION_HAS_XML
 * whenever options are set, and preserve this XML flag across version number
 * changes done via the JS_SetVersion API.
 *
 * But when executing a script or scripted function, the interpreter changes
 * cx->version, including the XML flag, to script->version.  Thus JSOPTION_XML
 * is a compile-time option that causes a run-time version change during each
 * activation of the compiled script.  That version change has the effect of
 * changing JS_HAS_XML_OPTION, so that any compiling done via eval enables XML
 * support.  If an XML-enabled script or function calls a non-XML function,
 * the flag bit will be cleared during the callee's activation.
 *
 * Note that JS_SetVersion API calls never pass JSVERSION_HAS_XML or'd into
 * that API's version parameter.
 *
 * Note also that script->version must contain this XML option flag in order
 * for XDR'ed scripts to serialize and deserialize with that option preserved
 * for detection at run-time.  We can't copy other compile-time options into
 * script->version because that would break backward compatibility (certain
 * other options, e.g. JSOPTION_VAROBJFIX, are analogous to JSOPTION_XML).
 */
#define JS_HAS_OPTION(cx,option)        (((cx)->options & (option)) != 0)
#define JS_HAS_STRICT_OPTION(cx)        JS_HAS_OPTION(cx, JSOPTION_STRICT)
#define JS_HAS_WERROR_OPTION(cx)        JS_HAS_OPTION(cx, JSOPTION_WERROR)
#define JS_HAS_COMPILE_N_GO_OPTION(cx)  JS_HAS_OPTION(cx, JSOPTION_COMPILE_N_GO)
#define JS_HAS_ATLINE_OPTION(cx)        JS_HAS_OPTION(cx, JSOPTION_ATLINE)

#define JSVERSION_MASK                  0x0FFF  /* see JSVersion in jspubtd.h */
#define JSVERSION_HAS_XML               0x1000  /* flag induced by XML option */

#define JSVERSION_NUMBER(cx)            ((cx)->version & JSVERSION_MASK)
#define JS_HAS_XML_OPTION(cx)           ((cx)->version & JSVERSION_HAS_XML || \
                                         JSVERSION_NUMBER(cx) >= JSVERSION_1_6)

#define JS_HAS_NATIVE_BRANCH_CALLBACK_OPTION(cx)                              \
    JS_HAS_OPTION(cx, JSOPTION_NATIVE_BRANCH_CALLBACK)

/*
 * Wrappers for the JSVERSION_IS_* macros from jspubtd.h taking JSContext *cx
 * and masking off the XML flag and any other high order bits.
 */
#define JS_VERSION_IS_ECMA(cx)          JSVERSION_IS_ECMA(JSVERSION_NUMBER(cx))

/*
 * Common subroutine of JS_SetVersion and js_SetVersion, to update per-context
 * data that depends on version.
 */
extern void
js_OnVersionChange(JSContext *cx);

/*
 * Unlike the JS_SetVersion API, this function stores JSVERSION_HAS_XML and
 * any future non-version-number flags induced by compiler options.
 */
extern void
js_SetVersion(JSContext *cx, JSVersion version);

/*
 * Create and destroy functions for JSContext, which is manually allocated
 * and exclusively owned.
 */
extern JSContext *
js_NewContext(JSRuntime *rt, size_t stackChunkSize);

extern void
js_DestroyContext(JSContext *cx, JSGCMode gcmode);

/*
 * Return true if cx points to a context in rt->contextList, else return false.
 * NB: the caller (see jslock.c:ClaimScope) must hold rt->gcLock.
 */
extern JSBool
js_ValidContextPointer(JSRuntime *rt, JSContext *cx);

/*
 * If unlocked, acquire and release rt->gcLock around *iterp update; otherwise
 * the caller must be holding rt->gcLock.
 */
extern JSContext *
js_ContextIterator(JSRuntime *rt, JSBool unlocked, JSContext **iterp);

/*
 * JSClass.resolve and watchpoint recursion damping machinery.
 */
extern JSBool
js_StartResolving(JSContext *cx, JSResolvingKey *key, uint32 flag,
                  JSResolvingEntry **entryp);

extern void
js_StopResolving(JSContext *cx, JSResolvingKey *key, uint32 flag,
                 JSResolvingEntry *entry, uint32 generation);

/*
 * Local root set management.
 *
 * NB: the jsval parameters below may be properly tagged jsvals, or GC-thing
 * pointers cast to (jsval).  This relies on JSObject's tag being zero, but
 * on the up side it lets us push int-jsval-encoded scopeMark values on the
 * local root stack.
 */
extern JSBool
js_EnterLocalRootScope(JSContext *cx);

#define js_LeaveLocalRootScope(cx) \
    js_LeaveLocalRootScopeWithResult(cx, JSVAL_NULL)

extern void
js_LeaveLocalRootScopeWithResult(JSContext *cx, jsval rval);

extern void
js_ForgetLocalRoot(JSContext *cx, jsval v);

extern int
js_PushLocalRoot(JSContext *cx, JSLocalRootStack *lrs, jsval v);

extern void
js_MarkLocalRoots(JSContext *cx, JSLocalRootStack *lrs);

/*
 * Report an exception, which is currently realized as a printf-style format
 * string and its arguments.
 */
typedef enum JSErrNum {
#define MSG_DEF(name, number, count, exception, format) \
    name = number,
#include "js.msg"
#undef MSG_DEF
    JSErr_Limit
} JSErrNum;

extern const JSErrorFormatString *
js_GetErrorMessage(void *userRef, const char *locale, const uintN errorNumber);

#ifdef va_start
extern JSBool
js_ReportErrorVA(JSContext *cx, uintN flags, const char *format, va_list ap);

extern JSBool
js_ReportErrorNumberVA(JSContext *cx, uintN flags, JSErrorCallback callback,
                       void *userRef, const uintN errorNumber,
                       JSBool charArgs, va_list ap);

extern JSBool
js_ExpandErrorArguments(JSContext *cx, JSErrorCallback callback,
                        void *userRef, const uintN errorNumber,
                        char **message, JSErrorReport *reportp,
                        JSBool *warningp, JSBool charArgs, va_list ap);
#endif

extern void
js_ReportOutOfMemory(JSContext *cx);

/*
 * Report an exception using a previously composed JSErrorReport.
 * XXXbe remove from "friend" API
 */
extern JS_FRIEND_API(void)
js_ReportErrorAgain(JSContext *cx, const char *message, JSErrorReport *report);

extern void
js_ReportIsNotDefined(JSContext *cx, const char *name);

extern JSErrorFormatString js_ErrorFormatString[JSErr_Limit];

/*
 * See JS_SetThreadStackLimit in jsapi.c, where we check that the stack grows
 * in the expected direction.  On Unix-y systems, JS_STACK_GROWTH_DIRECTION is
 * computed on the build host by jscpucfg.c and written into jsautocfg.h.  The
 * macro is hardcoded in jscpucfg.h on Windows and Mac systems (for historical
 * reasons pre-dating autoconf usage).
 */
#if JS_STACK_GROWTH_DIRECTION > 0
# define JS_CHECK_STACK_SIZE(cx, lval)  ((jsuword)&(lval) < (cx)->stackLimit)
#else
# define JS_CHECK_STACK_SIZE(cx, lval)  ((jsuword)&(lval) > (cx)->stackLimit)
#endif

JS_END_EXTERN_C

#endif /* jscntxt_h___ */
