/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla JavaScript code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1999-2001
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Brendan Eich <brendan@mozilla.org> (Original Author)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef pldhash_h___
#define pldhash_h___
/*
 * Double hashing, a la Knuth 6.
 * GENERATED BY js/src/plify_jsdhash.sed -- DO NOT EDIT!!!
 */
#include "prtypes.h"

PR_BEGIN_EXTERN_C

#ifdef DEBUG_brendan
#define PL_DHASHMETER 1
#endif

/* Maximum table size, do not equal or exceed (see min&maxAlphaFrac, below). */
#undef PL_DHASH_MAX_SIZE
#define PL_DHASH_MAX_SIZE       PR_BIT(24)

/* Minimum table size, or gross entry count (net is at most .75 loaded). */
#ifndef PL_DHASH_MIN_SIZE
#define PL_DHASH_MIN_SIZE 16
#elif (PL_DHASH_MIN_SIZE & (PL_DHASH_MIN_SIZE - 1)) != 0
#error "PL_DHASH_MIN_SIZE must be a power of two!"
#endif

/*
 * Multiplicative hash uses an unsigned 32 bit integer and the golden ratio,
 * expressed as a fixed-point 32-bit fraction.
 */
#define PL_DHASH_BITS           32
#define PL_DHASH_GOLDEN_RATIO   0x9E3779B9U

/* Primitive and forward-struct typedefs. */
typedef PRUint32                PLDHashNumber;
typedef struct PLDHashEntryHdr  PLDHashEntryHdr;
typedef struct PLDHashEntryStub PLDHashEntryStub;
typedef struct PLDHashTable     PLDHashTable;
typedef struct PLDHashTableOps  PLDHashTableOps;

/*
 * Table entry header structure.
 *
 * In order to allow in-line allocation of key and value, we do not declare
 * either here.  Instead, the API uses const void *key as a formal parameter,
 * and asks each entry for its key when necessary via a getKey callback, used
 * when growing or shrinking the table.  Other callback types are defined
 * below and grouped into the PLDHashTableOps structure, for single static
 * initialization per hash table sub-type.
 *
 * Each hash table sub-type should nest the PLDHashEntryHdr structure at the
 * front of its particular entry type.  The keyHash member contains the result
 * of multiplying the hash code returned from the hashKey callback (see below)
 * by PL_DHASH_GOLDEN_RATIO.  Its value is table size invariant.  keyHash is
 * maintained automatically by PL_DHashTableOperate -- users should never set
 * it, and its only uses should be via the entry macros below.
 *
 * The PL_DHASH_ENTRY_IS_LIVE macro tests whether entry is neither free nor
 * removed.  An entry may be either busy or free; if busy, it may be live or
 * removed.  Consumers of this API should not access members of entries that
 * are not live.
 */
struct PLDHashEntryHdr {
    PLDHashNumber       keyHash;        /* every entry must begin like this */
};

#define PL_DHASH_ENTRY_IS_FREE(entry)   ((entry)->keyHash == 0)
#define PL_DHASH_ENTRY_IS_BUSY(entry)   (!PL_DHASH_ENTRY_IS_FREE(entry))
#define PL_DHASH_ENTRY_IS_LIVE(entry)   ((entry)->keyHash >= 2)

/*
 * A PLDHashTable is currently 8 words (without the PL_DHASHMETER overhead)
 * on most architectures, and may be allocated on the stack or within another
 * structure or class (see below for the Init and Finish functions to use).
 *
 * To decide whether to use double hashing vs. chaining, we need to develop a
 * trade-off relation, as follows:
 *
 * Let alpha be the load factor, esize the entry size in words, count the
 * entry count, and pow2 the power-of-two table size in entries.
 *
 *   (PLDHashTable overhead)    > (PLHashTable overhead)
 *   (unused table entry space) > (malloc and .next overhead per entry) +
 *                                (buckets overhead)
 *   (1 - alpha) * esize * pow2 > 2 * count + pow2
 *
 * Notice that alpha is by definition (count / pow2):
 *
 *   (1 - alpha) * esize * pow2 > 2 * alpha * pow2 + pow2
 *   (1 - alpha) * esize        > 2 * alpha + 1
 *
 *   esize > (1 + 2 * alpha) / (1 - alpha)
 *
 * This assumes both tables must keep keyHash, key, and value for each entry,
 * where key and value point to separately allocated strings or structures.
 * If key and value can be combined into one pointer, then the trade-off is:
 *
 *   esize > (1 + 3 * alpha) / (1 - alpha)
 *
 * If the entry value can be a subtype of PLDHashEntryHdr, rather than a type
 * that must be allocated separately and referenced by an entry.value pointer
 * member, and provided key's allocation can be fused with its entry's, then
 * k (the words wasted per entry with chaining) is 4.
 *
 * To see these curves, feed gnuplot input like so:
 *
 *   gnuplot> f(x,k) = (1 + k * x) / (1 - x)
 *   gnuplot> plot [0:.75] f(x,2), f(x,3), f(x,4)
 *
 * For k of 2 and a well-loaded table (alpha > .5), esize must be more than 4
 * words for chaining to be more space-efficient than double hashing.
 *
 * Solving for alpha helps us decide when to shrink an underloaded table:
 *
 *   esize                     > (1 + k * alpha) / (1 - alpha)
 *   esize - alpha * esize     > 1 + k * alpha
 *   esize - 1                 > (k + esize) * alpha
 *   (esize - 1) / (k + esize) > alpha
 *
 *   alpha < (esize - 1) / (esize + k)
 *
 * Therefore double hashing should keep alpha >= (esize - 1) / (esize + k),
 * assuming esize is not too large (in which case, chaining should probably be
 * used for any alpha).  For esize=2 and k=3, we want alpha >= .2; for esize=3
 * and k=2, we want alpha >= .4.  For k=4, esize could be 6, and alpha >= .5
 * would still obtain.  See the PL_DHASH_MIN_ALPHA macro further below.
 *
 * The current implementation uses a constant .25 as alpha's lower bound when
 * deciding to shrink the table (while respecting PL_DHASH_MIN_SIZE).
 *
 * Note a qualitative difference between chaining and double hashing: under
 * chaining, entry addresses are stable across table shrinks and grows.  With
 * double hashing, you can't safely hold an entry pointer and use it after an
 * ADD or REMOVE operation, unless you sample table->generation before adding
 * or removing, and compare the sample after, dereferencing the entry pointer
 * only if table->generation has not changed.
 *
 * The moral of this story: there is no one-size-fits-all hash table scheme,
 * but for small table entry size, and assuming entry address stability is not
 * required, double hashing wins.
 */
struct PLDHashTable {
    PLDHashTableOps     *ops;           /* virtual operations, see below */
    void                *data;          /* ops- and instance-specific data */
    PRInt16             hashShift;      /* multiplicative hash shift */
    uint8               maxAlphaFrac;   /* 8-bit fixed point max alpha */
    uint8               minAlphaFrac;   /* 8-bit fixed point min alpha */
    PRUint32            entrySize;      /* number of bytes in an entry */
    PRUint32            entryCount;     /* number of entries in table */
    PRUint32            removedCount;   /* removed entry sentinels in table */
    PRUint32            generation;     /* entry storage generation number */
    char                *entryStore;    /* entry storage */
#ifdef PL_DHASHMETER
    struct PLDHashStats {
        PRUint32        searches;       /* total number of table searches */
        PRUint32        steps;          /* hash chain links traversed */
        PRUint32        hits;           /* searches that found key */
        PRUint32        misses;         /* searches that didn't find key */
        PRUint32        lookups;        /* number of PL_DHASH_LOOKUPs */
        PRUint32        addMisses;      /* adds that miss, and do work */
        PRUint32        addOverRemoved; /* adds that recycled a removed entry */
        PRUint32        addHits;        /* adds that hit an existing entry */
        PRUint32        addFailures;    /* out-of-memory during add growth */
        PRUint32        removeHits;     /* removes that hit, and do work */
        PRUint32        removeMisses;   /* useless removes that miss */
        PRUint32        removeFrees;    /* removes that freed entry directly */
        PRUint32        removeEnums;    /* removes done by Enumerate */
        PRUint32        grows;          /* table expansions */
        PRUint32        shrinks;        /* table contractions */
        PRUint32        compresses;     /* table compressions */
        PRUint32        enumShrinks;    /* contractions after Enumerate */
    } stats;
#endif
};

/*
 * Size in entries (gross, not net of free and removed sentinels) for table.
 * We store hashShift rather than sizeLog2 to optimize the collision-free case
 * in SearchTable.
 */
#define PL_DHASH_TABLE_SIZE(table)  PR_BIT(PL_DHASH_BITS - (table)->hashShift)

/*
 * Table space at entryStore is allocated and freed using these callbacks.
 * The allocator should return null on error only (not if called with nbytes
 * equal to 0; but note that pldhash.c code will never call with 0 nbytes).
 */
typedef void *
(* PR_CALLBACK PLDHashAllocTable)(PLDHashTable *table, PRUint32 nbytes);

typedef void
(* PR_CALLBACK PLDHashFreeTable) (PLDHashTable *table, void *ptr);

/*
 * When a table grows or shrinks, each entry is queried for its key using this
 * callback.  NB: in that event, entry is not in table any longer; it's in the
 * old entryStore vector, which is due to be freed once all entries have been
 * moved via moveEntry callbacks.
 */
typedef const void *
(* PR_CALLBACK PLDHashGetKey)    (PLDHashTable *table, PLDHashEntryHdr *entry);

/*
 * Compute the hash code for a given key to be looked up, added, or removed
 * from table.  A hash code may have any PLDHashNumber value.
 */
typedef PLDHashNumber
(* PR_CALLBACK PLDHashHashKey)   (PLDHashTable *table, const void *key);

/*
 * Compare the key identifying entry in table with the provided key parameter.
 * Return PR_TRUE if keys match, PR_FALSE otherwise.
 */
typedef PRBool
(* PR_CALLBACK PLDHashMatchEntry)(PLDHashTable *table,
                                      const PLDHashEntryHdr *entry,
                                      const void *key);

/*
 * Copy the data starting at from to the new entry storage at to.  Do not add
 * reference counts for any strong references in the entry, however, as this
 * is a "move" operation: the old entry storage at from will be freed without
 * any reference-decrementing callback shortly.
 */
typedef void
(* PR_CALLBACK PLDHashMoveEntry)(PLDHashTable *table,
                                     const PLDHashEntryHdr *from,
                                     PLDHashEntryHdr *to);

/*
 * Clear the entry and drop any strong references it holds.  This callback is
 * invoked during a PL_DHASH_REMOVE operation (see below for operation codes),
 * but only if the given key is found in the table.
 */
typedef void
(* PR_CALLBACK PLDHashClearEntry)(PLDHashTable *table,
                                      PLDHashEntryHdr *entry);

/*
 * Called when a table (whether allocated dynamically by itself, or nested in
 * a larger structure, or allocated on the stack) is finished.  This callback
 * allows table->ops-specific code to finalize table->data.
 */
typedef void
(* PR_CALLBACK PLDHashFinalize)  (PLDHashTable *table);

/*
 * Initialize a new entry, apart from keyHash.  This function is called when
 * PL_DHashTableOperate's PL_DHASH_ADD case finds no existing entry for the
 * given key, and must add a new one.  At that point, entry->keyHash is not
 * set yet, to avoid claiming the last free entry in a severely overloaded
 * table.
 */
typedef void
(* PR_CALLBACK PLDHashInitEntry)(PLDHashTable *table,
                                     PLDHashEntryHdr *entry,
                                     const void *key);

/*
 * Finally, the "vtable" structure for PLDHashTable.  The first eight hooks
 * must be provided by implementations; they're called unconditionally by the
 * generic pldhash.c code.  Hooks after these may be null.
 *
 * Summary of allocation-related hook usage with C++ placement new emphasis:
 *  allocTable          Allocate raw bytes with malloc, no ctors run.
 *  freeTable           Free raw bytes with free, no dtors run.
 *  initEntry           Call placement new using default key-based ctor.
 *  moveEntry           Call placement new using copy ctor, run dtor on old
 *                      entry storage.
 *  clearEntry          Run dtor on entry.
 *  finalize            Stub unless table->data was initialized and needs to
 *                      be finalized.
 *
 * Note the reason why initEntry is optional: the default hooks (stubs) clear
 * entry storage:  On successful PL_DHashTableOperate(tbl, key, PL_DHASH_ADD),
 * the returned entry pointer addresses an entry struct whose keyHash member
 * has been set non-zero, but all other entry members are still clear (null).
 * PL_DHASH_ADD callers can test such members to see whether the entry was
 * newly created by the PL_DHASH_ADD call that just succeeded.  If placement
 * new or similar initialization is required, define an initEntry hook.  Of
 * course, the clearEntry hook must zero or null appropriately.
 */
struct PLDHashTableOps {
    /* Mandatory hooks.  All implementations must provide these. */
    PLDHashAllocTable   allocTable;
    PLDHashFreeTable    freeTable;
    PLDHashGetKey       getKey;
    PLDHashHashKey      hashKey;
    PLDHashMatchEntry   matchEntry;
    PLDHashMoveEntry    moveEntry;
    PLDHashClearEntry   clearEntry;
    PLDHashFinalize     finalize;

    /* Optional hooks start here.  If null, these are not called. */
    PLDHashInitEntry    initEntry;
};

/*
 * Default implementations for the above ops.
 */
PR_EXTERN(void *)
PL_DHashAllocTable(PLDHashTable *table, PRUint32 nbytes);

PR_EXTERN(void)
PL_DHashFreeTable(PLDHashTable *table, void *ptr);

PR_EXTERN(PLDHashNumber)
PL_DHashStringKey(PLDHashTable *table, const void *key);

/* A minimal entry contains a keyHash header and a void key pointer. */
struct PLDHashEntryStub {
    PLDHashEntryHdr hdr;
    const void      *key;
};

PR_EXTERN(const void *)
PL_DHashGetKeyStub(PLDHashTable *table, PLDHashEntryHdr *entry);

PR_EXTERN(PLDHashNumber)
PL_DHashVoidPtrKeyStub(PLDHashTable *table, const void *key);

PR_EXTERN(PRBool)
PL_DHashMatchEntryStub(PLDHashTable *table,
                       const PLDHashEntryHdr *entry,
                       const void *key);

PR_EXTERN(void)
PL_DHashMoveEntryStub(PLDHashTable *table,
                      const PLDHashEntryHdr *from,
                      PLDHashEntryHdr *to);

PR_EXTERN(void)
PL_DHashClearEntryStub(PLDHashTable *table, PLDHashEntryHdr *entry);

PR_EXTERN(void)
PL_DHashFinalizeStub(PLDHashTable *table);

/*
 * If you use PLDHashEntryStub or a subclass of it as your entry struct, and
 * if your entries move via memcpy and clear via memset(0), you can use these
 * stub operations.
 */
PR_EXTERN(PLDHashTableOps *)
PL_DHashGetStubOps(void);

/*
 * Dynamically allocate a new PLDHashTable using malloc, initialize it using
 * PL_DHashTableInit, and return its address.  Return null on malloc failure.
 * Note that the entry storage at table->entryStore will be allocated using
 * the ops->allocTable callback.
 */
PR_EXTERN(PLDHashTable *)
PL_NewDHashTable(PLDHashTableOps *ops, void *data, PRUint32 entrySize,
                 PRUint32 capacity);

/*
 * Finalize table's data, free its entry storage (via table->ops->freeTable),
 * and return the memory starting at table to the malloc heap.
 */
PR_EXTERN(void)
PL_DHashTableDestroy(PLDHashTable *table);

/*
 * Initialize table with ops, data, entrySize, and capacity.  Capacity is a
 * guess for the smallest table size at which the table will usually be less
 * than 75% loaded (the table will grow or shrink as needed; capacity serves
 * only to avoid inevitable early growth from PL_DHASH_MIN_SIZE).
 */
PR_EXTERN(PRBool)
PL_DHashTableInit(PLDHashTable *table, PLDHashTableOps *ops, void *data,
                  PRUint32 entrySize, PRUint32 capacity);

/*
 * Set maximum and minimum alpha for table.  The defaults are 0.75 and .25.
 * maxAlpha must be in [0.5, 0.9375] for the default PL_DHASH_MIN_SIZE; or if
 * MinSize=PL_DHASH_MIN_SIZE <= 256, in [0.5, (float)(MinSize-1)/MinSize]; or
 * else in [0.5, 255.0/256].  minAlpha must be in [0, maxAlpha / 2), so that
 * we don't shrink on next remove after growing a table upon adding an entry
 * that brings entryCount past maxAlpha * tableSize.
 */
PR_IMPLEMENT(void)
PL_DHashTableSetAlphaBounds(PLDHashTable *table,
                            float maxAlpha,
                            float minAlpha);

/*
 * Call this macro with k, the number of pointer-sized words wasted per entry
 * under chaining, to compute the minimum alpha at which double hashing still
 * beats chaining.
 */
#define PL_DHASH_MIN_ALPHA(table, k)                                          \
    ((float)((table)->entrySize / sizeof(void *) - 1)                         \
     / ((table)->entrySize / sizeof(void *) + (k)))

/*
 * Finalize table's data, free its entry storage using table->ops->freeTable,
 * and leave its members unchanged from their last live values (which leaves
 * pointers dangling).  If you want to burn cycles clearing table, it's up to
 * your code to call memset.
 */
PR_EXTERN(void)
PL_DHashTableFinish(PLDHashTable *table);

/*
 * To consolidate keyHash computation and table grow/shrink code, we use a
 * single entry point for lookup, add, and remove operations.  The operation
 * codes are declared here, along with codes returned by PLDHashEnumerator
 * functions, which control PL_DHashTableEnumerate's behavior.
 */
typedef enum PLDHashOperator {
    PL_DHASH_LOOKUP = 0,        /* lookup entry */
    PL_DHASH_ADD = 1,           /* add entry */
    PL_DHASH_REMOVE = 2,        /* remove entry, or enumerator says remove */
    PL_DHASH_NEXT = 0,          /* enumerator says continue */
    PL_DHASH_STOP = 1           /* enumerator says stop */
} PLDHashOperator;

/*
 * To lookup a key in table, call:
 *
 *  entry = PL_DHashTableOperate(table, key, PL_DHASH_LOOKUP);
 *
 * If PL_DHASH_ENTRY_IS_BUSY(entry) is true, key was found and it identifies
 * entry.  If PL_DHASH_ENTRY_IS_FREE(entry) is true, key was not found.
 *
 * To add an entry identified by key to table, call:
 *
 *  entry = PL_DHashTableOperate(table, key, PL_DHASH_ADD);
 *
 * If entry is null upon return, the table is severely overloaded, and new
 * memory can't be allocated for new entry storage via table->ops->allocTable.
 * Otherwise, entry->keyHash has been set so that PL_DHASH_ENTRY_IS_BUSY(entry)
 * is true, and it is up to the caller to initialize the key and value parts
 * of the entry sub-type, if they have not been set already (i.e. if entry was
 * not already in the table).
 *
 * To remove an entry identified by key from table, call:
 *
 *  (void) PL_DHashTableOperate(table, key, PL_DHASH_REMOVE);
 *
 * If key's entry is found, it is cleared (via table->ops->clearEntry) and
 * the entry is marked so that PL_DHASH_ENTRY_IS_FREE(entry).  This operation
 * returns null unconditionally; you should ignore its return value.
 */
PR_EXTERN(PLDHashEntryHdr *)
PL_DHashTableOperate(PLDHashTable *table, const void *key, PLDHashOperator op);

/*
 * Remove an entry already accessed via LOOKUP or ADD.
 *
 * NB: this is a "raw" or low-level routine, intended to be used only where
 * the inefficiency of a full PL_DHashTableOperate (which rehashes in order
 * to find the entry given its key) is not tolerable.  This function does not
 * shrink the table if it is underloaded.  It does not update stats #ifdef
 * PL_DHASHMETER, either.
 */
PR_EXTERN(void)
PL_DHashTableRawRemove(PLDHashTable *table, PLDHashEntryHdr *entry);

/*
 * Enumerate entries in table using etor:
 *
 *   count = PL_DHashTableEnumerate(table, etor, arg);
 *
 * PL_DHashTableEnumerate calls etor like so:
 *
 *   op = etor(table, entry, number, arg);
 *
 * where number is a zero-based ordinal assigned to live entries according to
 * their order in table->entryStore.
 *
 * The return value, op, is treated as a set of flags.  If op is PL_DHASH_NEXT,
 * then continue enumerating.  If op contains PL_DHASH_REMOVE, then clear (via
 * table->ops->clearEntry) and free entry.  Then we check whether op contains
 * PL_DHASH_STOP; if so, stop enumerating and return the number of live entries
 * that were enumerated so far.  Return the total number of live entries when
 * enumeration completes normally.
 *
 * If etor calls PL_DHashTableOperate on table, it must return PL_DHASH_STOP;
 * otherwise undefined behavior results.
 */
typedef PLDHashOperator
(* PR_CALLBACK PLDHashEnumerator)(PLDHashTable *table, PLDHashEntryHdr *hdr,
                                      PRUint32 number, void *arg);

PR_EXTERN(PRUint32)
PL_DHashTableEnumerate(PLDHashTable *table, PLDHashEnumerator etor, void *arg);

#ifdef PL_DHASHMETER
#include <stdio.h>

PR_EXTERN(void)
PL_DHashTableDumpMeter(PLDHashTable *table, PLDHashEnumerator dump, FILE *fp);
#endif

PR_END_EXTERN_C

#endif /* pldhash_h___ */
