/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla FastLoad code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 2001 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 *   Brendan Eich <brendan@mozilla.org> (original author)
 */

#include <string.h>
#include "prtypes.h"
#include "nscore.h"
#include "nsDebug.h"
#include "nsMemory.h"

#include "nsIComponentManager.h"
#include "nsISeekableStream.h"
#include "nsISerializable.h"
#include "nsIStreamBufferAccess.h"

#include "nsBinaryStream.h"
#include "nsFastLoadFile.h"

#ifdef DEBUG_brendan
# define METERING
# define DEBUG_MUX
#endif

#ifdef METERING
# define METER(x)       x
#else
# define METER(x)       /* nothing */
#endif

#ifdef DEBUG_MUX
# include <stdio.h>
# include <stdarg.h>

static void trace_mux(char mode, const char *format, ...)
{
    va_list ap;
    static FILE *tfp;
    if (!tfp) {
        char tfn[16];
        sprintf(tfn, "/tmp/mux.%ctrace", mode);
        tfp = fopen(tfn, "w");
        if (!tfp)
            return;
        setvbuf(tfp, NULL, _IOLBF, 0);
    }
    va_start(ap, format);
    vfprintf(tfp, format, ap);
    va_end(ap);
}

# define TRACE_MUX(args) trace_mux args
#else
# define TRACE_MUX(args) /* nothing */
#endif

/*
 * Fletcher's 16-bit checksum, using 32-bit two's-complement arithmetic.
 */
#define FOLD_ONES_COMPLEMENT_CARRY(X)   ((X) = ((X) & 0xffff) + ((X) >> 16))
#define ONES_COMPLEMENT_ACCUMULATE(X,Y) (X) += (Y); if ((X) & 0x80000000)     \
                                        FOLD_ONES_COMPLEMENT_CARRY(X)
#define FLETCHER_ACCUMULATE(A,B,U)      ONES_COMPLEMENT_ACCUMULATE(A, U);     \
                                        ONES_COMPLEMENT_ACCUMULATE(B, A)

PR_IMPLEMENT(PRUint32)
NS_AccumulateFastLoadChecksum(PRUint32 *aChecksum,
                              const PRUint8* aBuffer,
                              PRUint32 aLength,
                              PRBool aLastBuffer)
{
    PRUint32 C = *aChecksum;
    PRUint32 A = C & 0xffff;
    PRUint32 B = C >> 16;

    PRUint16 U = 0;
    if (aLength >= 4) {
        PRBool odd = PRWord(aBuffer) & 1;
        switch (PRWord(aBuffer) & 3) {
          case 3:
            U = (aBuffer[0] << 8) | aBuffer[1];
            FLETCHER_ACCUMULATE(A, B, U);
            U = aBuffer[2];
            aBuffer += 3;
            aLength -= 3;
            break;

          case 2:
            U = (aBuffer[0] << 8) | aBuffer[1];
            FLETCHER_ACCUMULATE(A, B, U);
            U = 0;
            aBuffer += 2;
            aLength -= 2;
            break;

          case 1:
            U = *aBuffer++;
            aLength--;
            break;
        }

        PRUint32 W;
        if (odd) {
            while (aLength > 3) {
                W = *NS_REINTERPRET_CAST(const PRUint32*, aBuffer);
                U <<= 8;
#ifdef IS_BIG_ENDIAN
                U |= W >> 24;
                FLETCHER_ACCUMULATE(A, B, U);
                U = PRUint16(W >> 8);
                FLETCHER_ACCUMULATE(A, B, U);
                U = W & 0xff;
#else
                U |= W & 0xff;
                FLETCHER_ACCUMULATE(A, B, U);
                U = PRUint16(W >> 8);
                U = NS_SWAP16(U);
                FLETCHER_ACCUMULATE(A, B, U);
                U = W >> 24;
#endif
                aBuffer += 4;
                aLength -= 4;
            }
            aBuffer--;      // we're odd, we didn't checksum the last byte
            aLength++;
        } else {
            while (aLength > 3) {
                W = *NS_REINTERPRET_CAST(const PRUint32*, aBuffer);
#ifdef IS_BIG_ENDIAN
                U = W >> 16;
                FLETCHER_ACCUMULATE(A, B, U);
                U = PRUint16(W);
                FLETCHER_ACCUMULATE(A, B, U);
#else
                U = NS_SWAP16(W);
                FLETCHER_ACCUMULATE(A, B, U);
                U = W >> 16;
                U = NS_SWAP16(W);
                FLETCHER_ACCUMULATE(A, B, U);
#endif
                aBuffer += 4;
                aLength -= 4;
            }
        }
    }

    if (aLastBuffer) {
        NS_ASSERTION(aLength <= 4, "aLength botch");
        switch (aLength) {
          case 4:
            U = (aBuffer[0] << 8) | aBuffer[1];
            FLETCHER_ACCUMULATE(A, B, U);
            U = (aBuffer[2] << 8) | aBuffer[3];
            FLETCHER_ACCUMULATE(A, B, U);
            break;

          case 3:
            U = (aBuffer[0] << 8) | aBuffer[1];
            FLETCHER_ACCUMULATE(A, B, U);
            U = aBuffer[2];
            FLETCHER_ACCUMULATE(A, B, U);
            break;

          case 2:
            U = (aBuffer[0] << 8) | aBuffer[1];
            FLETCHER_ACCUMULATE(A, B, U);
            break;

          case 1:
            U = aBuffer[0];
            FLETCHER_ACCUMULATE(A, B, U);
            break;
        }

        aLength = 0;
    }

    while (A >> 16)
        FOLD_ONES_COMPLEMENT_CARRY(A);
    while (B >> 16)
        FOLD_ONES_COMPLEMENT_CARRY(B);

    *aChecksum = (B << 16) | A;
    return aLength;
}

PR_IMPLEMENT(PRUint32)
NS_AddFastLoadChecksums(PRUint32 sum1, PRUint32 sum2, PRUint32 sum2ByteCount)
{
    PRUint32 A1 = sum1 & 0xffff;
    PRUint32 B1 = sum1 >> 16;

    PRUint32 A2 = sum2 & 0xffff;
    PRUint32 B2 = sum2 >> 16;

    PRUint32 A = A1 + A2;
    while (A >> 16)
        FOLD_ONES_COMPLEMENT_CARRY(A);

    PRUint32 B = B2;
    for (PRUint32 n = (sum2ByteCount + 1) / 2; n != 0; n--)
        ONES_COMPLEMENT_ACCUMULATE(B, B1);
    while (B >> 16)
        FOLD_ONES_COMPLEMENT_CARRY(B);

    return (B << 16) | A;
}

#undef FOLD_ONES_COMPLEMENT_CARRY
#undef ONES_COMPLEMENT_ACCUMULATE
#undef FLETCHER_ACCUMULATE

static const char magic[] = MFL_FILE_MAGIC;

// -------------------------- nsFastLoadFileReader --------------------------

nsID nsFastLoadFileReader::nsFastLoadFooter::gDummyID;
nsFastLoadFileReader::nsObjectMapEntry nsFastLoadFileReader::nsFastLoadFooter::gDummySharpObjectEntry; 

NS_IMPL_ISUPPORTS_INHERITED3(nsFastLoadFileReader,
                             nsBinaryInputStream,
                             nsIObjectInputStream,
                             nsIFastLoadFileControl,
                             nsISeekableStream)

MOZ_DECL_CTOR_COUNTER(nsFastLoadFileReader)

nsresult
nsFastLoadFileReader::ReadHeader(nsFastLoadHeader *aHeader)
{
    nsresult rv;
    PRUint32 bytesRead;

    rv = Read(NS_REINTERPRET_CAST(char*, aHeader), sizeof *aHeader, &bytesRead);
    if (NS_FAILED(rv)) return rv;

    if (bytesRead != sizeof *aHeader ||
        memcmp(aHeader->mMagic, magic, MFL_FILE_MAGIC_SIZE)) {
        return NS_ERROR_UNEXPECTED;
    }

    aHeader->mChecksum     = NS_SWAP32(aHeader->mChecksum);
    aHeader->mVersion      = NS_SWAP32(aHeader->mVersion);
    aHeader->mFooterOffset = NS_SWAP32(aHeader->mFooterOffset);
    aHeader->mFileSize     = NS_SWAP32(aHeader->mFileSize);

    return NS_OK;
}

// nsIFastLoadFileControl methods:

NS_IMETHODIMP
nsFastLoadFileReader::GetChecksum(PRUint32 *aChecksum)
{
    *aChecksum = mHeader.mChecksum;
    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadFileReader::SetChecksum(PRUint32 aChecksum)
{
    mHeader.mChecksum = aChecksum;
    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadFileReader::ComputeChecksum(PRUint32 *aResult)
{
    *aResult = mHeader.mChecksum;
    return NS_OK;
#if 0
    PRUint32 saveChecksum = mHeader.mChecksum;
    mHeader.mChecksum = 0;
    NS_AccumulateFastLoadChecksum(&mHeader.mChecksum,
                                  NS_REINTERPRET_CAST(PRUint8*, &mHeader),
                                  sizeof mHeader,
                                  PR_FALSE);

    nsCOMPtr<nsIStreamBufferAccess>
        bufferAccess(do_QueryInterface(mInputStream));
    if (bufferAccess)
        bufferAccess->DisableBuffering();

    nsCOMPtr<nsISeekableStream> seekable(do_QueryInterface(mInputStream));
    nsresult rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET,
                                 sizeof mHeader);
    if (NS_SUCCEEDED(rv)) {
        char buf[MFL_CHECKSUM_BUFSIZE];
        PRUint32 len, rem = 0;

        for (;;) {
            rv = mInputStream->Read(buf + rem, sizeof buf - rem, &len);
            if (NS_FAILED(rv) || len == 0)
                break;

            len += rem;
            rem = NS_AccumulateFastLoadChecksum(&mHeader.mChecksum,
                                                NS_REINTERPRET_CAST(PRUint8*,
                                                                    buf),
                                                len,
                                                PR_FALSE);
            if (rem != 0)
                memcpy(buf, buf + len - rem, rem);
        }

        if (rem != 0) {
            NS_AccumulateFastLoadChecksum(&mHeader.mChecksum,
                                          NS_REINTERPRET_CAST(PRUint8*, buf),
                                          rem,
                                          PR_TRUE);
        }
    }

    if (bufferAccess)
        bufferAccess->EnableBuffering();

    *aResult = mHeader.mChecksum;
    mHeader.mChecksum = saveChecksum;
    return rv;
#endif
}

NS_IMETHODIMP
nsFastLoadFileReader::GetDependencies(nsICollection* *aDependencies)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

struct nsDocumentMapEntry : public PLDHashEntryHdr {
    const char* mURISpec;               // key, must come first
    PRUint32    mInitialSegmentOffset;  // offset of URI's first segment in file
};

struct nsDocumentMapReadEntry : public nsDocumentMapEntry {
    PRUint32    mNextSegmentOffset;     // offset of URI's next segment to read
    PRUint32    mBytesLeft;             // bytes remaining in current segment
    PRUint32    mSaveOffset;            // in case demux schedule differs from
                                        // mux schedule
};

PR_STATIC_CALLBACK(PRBool)
docmap_MatchEntry(PLDHashTable *aTable,
                  const PLDHashEntryHdr *aHdr,
                  const void *aKey)
{
    const nsDocumentMapEntry* entry =
        NS_STATIC_CAST(const nsDocumentMapEntry*, aHdr);
    const char* spec = NS_REINTERPRET_CAST(const char*, aKey);

    return strcmp(entry->mURISpec, spec) == 0;
}

PR_STATIC_CALLBACK(void)
docmap_ClearEntry(PLDHashTable *aTable, PLDHashEntryHdr *aHdr)
{
    nsDocumentMapEntry* entry = NS_STATIC_CAST(nsDocumentMapEntry*, aHdr);

    nsMemory::Free((void*) entry->mURISpec);
    PL_DHashClearEntryStub(aTable, aHdr);
}

static PLDHashTableOps docmap_DHashTableOps = {
    PL_DHashAllocTable,
    PL_DHashFreeTable,
    PL_DHashGetKeyStub,
    PL_DHashStringKey,
    docmap_MatchEntry,
    PL_DHashMoveEntryStub,
    docmap_ClearEntry,
    PL_DHashFinalizeStub,
    NULL
};

// An nsObjectMapEntry holds a strong reference to an XPCOM object, unless the
// mObject member, when cast to NSFastLoadOID, has its MFL_OBJECT_DEF_TAG bit
// set.  NB: we rely on the fact that an nsISupports* is never an odd pointer.
struct nsObjectMapEntry : public PLDHashEntryHdr {
    nsISupports*            mObject;        // key, must come first
};

// Fast mapping from URI object pointer back to spec-indexed document info.
struct nsURIMapReadEntry : public nsObjectMapEntry {
    nsDocumentMapReadEntry* mDocMapEntry;
};

PR_STATIC_CALLBACK(void)
objmap_ClearEntry(PLDHashTable *aTable, PLDHashEntryHdr *aHdr)
{
    nsObjectMapEntry* entry = NS_STATIC_CAST(nsObjectMapEntry*, aHdr);

    // Ignore tagged object ids stored as object pointer keys (the updater
    // code does this).
    if ((NSFastLoadOID(NS_PTR_TO_INT32(entry->mObject)) & MFL_OBJECT_DEF_TAG) == 0)
        NS_IF_RELEASE(entry->mObject);
    PL_DHashClearEntryStub(aTable, aHdr);
}

static PLDHashTableOps objmap_DHashTableOps = {
    PL_DHashAllocTable,
    PL_DHashFreeTable,
    PL_DHashGetKeyStub,
    PL_DHashVoidPtrKeyStub,
    PL_DHashMatchEntryStub,
    PL_DHashMoveEntryStub,
    objmap_ClearEntry,
    PL_DHashFinalizeStub,
    NULL
};

NS_IMETHODIMP
nsFastLoadFileReader::StartMuxedDocument(nsISupports* aURI, const char* aURISpec)
{
    nsDocumentMapReadEntry* docMapEntry =
        NS_STATIC_CAST(nsDocumentMapReadEntry*,
                       PL_DHashTableOperate(&mFooter.mDocumentMap, aURISpec,
                                            PL_DHASH_LOOKUP));
    if (PL_DHASH_ENTRY_IS_FREE(docMapEntry))
        return NS_ERROR_NOT_AVAILABLE;

    nsCOMPtr<nsISupports> key(do_QueryInterface(aURI));
    nsURIMapReadEntry* uriMapEntry =
        NS_STATIC_CAST(nsURIMapReadEntry*,
                       PL_DHashTableOperate(&mFooter.mURIMap, key,
                                            PL_DHASH_ADD));
    if (!uriMapEntry)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ASSERTION(uriMapEntry->mDocMapEntry == nsnull,
                 "URI mapped to two different specs?");
    if (uriMapEntry->mDocMapEntry)
        return NS_ERROR_UNEXPECTED;

    uriMapEntry->mObject = key;
    NS_ADDREF(uriMapEntry->mObject);
    uriMapEntry->mDocMapEntry = docMapEntry;
    TRACE_MUX(('r', "start %p (%p) %s\n", aURI, key.get(), aURISpec));
    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadFileReader::SelectMuxedDocument(nsISupports* aURI)
{
    nsresult rv;

    // Find the given URI's entry and select it for more reading.
    nsCOMPtr<nsISupports> key(do_QueryInterface(aURI));
    nsURIMapReadEntry* uriMapEntry =
        NS_STATIC_CAST(nsURIMapReadEntry*,
                       PL_DHashTableOperate(&mFooter.mURIMap, key,
                                            PL_DHASH_LOOKUP));

    // If the URI isn't in the map, return NS_ERROR_NOT_AVAILABLE so the
    // FastLoad service can try for a file update.
    if (PL_DHASH_ENTRY_IS_FREE(uriMapEntry))
        return NS_ERROR_NOT_AVAILABLE;

    // If we're interrupting another document's segment, save its offset so
    // we can seek back when it's reselected.
    nsDocumentMapReadEntry* docMapEntry = mCurrentDocumentMapEntry;
    if (docMapEntry && docMapEntry->mBytesLeft) {
        rv = Tell(&docMapEntry->mSaveOffset);
        if (NS_FAILED(rv)) return rv;
    }

    // It turns out we get a fair amount of redundant select calls, thanks to
    // non-blocking hunks of data from the parser that are devoid of scripts.
    // As more data gets FastLoaded, the number of these useless selects will
    // decline.
    docMapEntry = uriMapEntry->mDocMapEntry;
    if (docMapEntry == mCurrentDocumentMapEntry) {
        TRACE_MUX(('r', "select prev %s same as current!\n",
                   docMapEntry->mURISpec));
    }

    // Invariant: docMapEntry->mBytesLeft implies docMapEntry->mSaveOffset has
    // been set non-zero by the Tell call above.
    if (docMapEntry->mBytesLeft) {
        NS_ASSERTION(docMapEntry->mSaveOffset != 0,
                     "reselecting from multiplex at unsaved offset?");

        // Don't call our Seek wrapper, as it clears mCurrentDocumentMapEntry.
        nsCOMPtr<nsISeekableStream> seekable(do_QueryInterface(mInputStream));
        rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET,
                            docMapEntry->mSaveOffset);
        if (NS_FAILED(rv)) return rv;
    }

    mCurrentDocumentMapEntry = docMapEntry;
#ifdef DEBUG_MUX
    PRUint32 currentSegmentOffset;
    Tell(&currentSegmentOffset);
    trace_mux('r', "select %p (%p) offset %lu\n",
              aURI, key.get(), currentSegmentOffset);
#endif
    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadFileReader::EndMuxedDocument(nsISupports* aURI)
{
    nsCOMPtr<nsISupports> key(do_QueryInterface(aURI));
    nsURIMapReadEntry* uriMapEntry =
        NS_STATIC_CAST(nsURIMapReadEntry*,
                       PL_DHashTableOperate(&mFooter.mURIMap, key,
                                            PL_DHASH_LOOKUP));

    // If the URI isn't in the map, return NS_ERROR_NOT_AVAILABLE so the
    // FastLoad service can try for a file update.
    if (PL_DHASH_ENTRY_IS_FREE(uriMapEntry))
        return NS_ERROR_NOT_AVAILABLE;

    // Shrink the table if half the entries are removed sentinels.
    PRUint32 size = PR_BIT(mFooter.mURIMap.sizeLog2);
    if (mFooter.mURIMap.removedCount >= (size >> 2))
        PL_DHashTableOperate(&mFooter.mURIMap, key, PL_DHASH_REMOVE);
    else
        PL_DHashTableRawRemove(&mFooter.mURIMap, uriMapEntry);

    TRACE_MUX(('r', "end %p (%p)\n", aURI, key.get()));
    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadFileReader::Read(char* aBuffer, PRUint32 aCount, PRUint32 *aBytesRead)
{
    nsresult rv;

    nsDocumentMapReadEntry* entry = mCurrentDocumentMapEntry;
    if (entry && entry->mBytesLeft == 0) {
        // Don't call our Seek wrapper, as it clears mCurrentDocumentMapEntry.
        nsCOMPtr<nsISeekableStream> seekable(do_QueryInterface(mInputStream));

        // Loop to handle empty segments, which may be generated by the
        // writer, given Start A; Start B; Select A; Select B; write B data;
        // multiplexing schedules, which do tend to occur given non-blocking
        // i/o with LIFO scheduling.  XXXbe investigate LIFO issues
        do {
            // Check for unexpected end of multiplexed stream.
            NS_ASSERTION(entry->mNextSegmentOffset != 0,
                         "document demuxed from FastLoad file more than once?");
            if (entry->mNextSegmentOffset == 0)
                return NS_ERROR_UNEXPECTED;

            rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET,
                                entry->mNextSegmentOffset);
            if (NS_FAILED(rv)) return rv;

            // Clear mCurrentDocumentMapEntry temporarily to avoid recursion.
            mCurrentDocumentMapEntry = nsnull;

            rv = Read32(&entry->mNextSegmentOffset);
            if (NS_SUCCEEDED(rv))
                rv = Read32(&entry->mBytesLeft);

            mCurrentDocumentMapEntry = entry;
            if (NS_FAILED(rv)) return rv;

            NS_ASSERTION(entry->mBytesLeft >= 8, "demux segment length botch!");
            entry->mBytesLeft -= 8;
        } while (entry->mBytesLeft == 0);
    }

    rv = mInputStream->Read(aBuffer, aCount, aBytesRead);

    if (NS_SUCCEEDED(rv) && entry) {
        NS_ASSERTION(entry->mBytesLeft >= *aBytesRead, "demux underflow!");
        entry->mBytesLeft -= *aBytesRead;

#ifdef NS_DEBUG
        // Invariant: !entry->mBytesLeft implies entry->mSaveOffset == 0.
        if (entry->mBytesLeft == 0)
            entry->mSaveOffset = 0;
#endif
    }
    return rv;
}

nsresult
nsFastLoadFileReader::ReadFooter(nsFastLoadFooter *aFooter)
{
    nsresult rv;

    rv = ReadFooterPrefix(aFooter);
    if (NS_FAILED(rv)) return rv;

    aFooter->mIDMap = new nsID[aFooter->mNumIDs];
    if (!aFooter->mIDMap)
        return NS_ERROR_OUT_OF_MEMORY;

    PRUint32 i, n;
    for (i = 0, n = aFooter->mNumIDs; i < n; i++) {
        rv = ReadSlowID(&aFooter->mIDMap[i]);
        if (NS_FAILED(rv)) return rv;
    }

    aFooter->mObjectMap = new nsObjectMapEntry[aFooter->mNumSharpObjects];
    if (!aFooter->mObjectMap)
        return NS_ERROR_OUT_OF_MEMORY;

    for (i = 0, n = aFooter->mNumSharpObjects; i < n; i++) {
        nsObjectMapEntry* entry = &aFooter->mObjectMap[i];

        rv = ReadSharpObjectInfo(entry);
        if (NS_FAILED(rv)) return rv;

        entry->mReadObject = nsnull;
        entry->mSkipOffset = 0;
    }

    if (!PL_DHashTableInit(&aFooter->mDocumentMap, &docmap_DHashTableOps,
                           (void *)this, sizeof(nsDocumentMapReadEntry),
                           PL_DHASH_MIN_SIZE)) {
        aFooter->mDocumentMap.ops = nsnull;
        return NS_ERROR_OUT_OF_MEMORY;
    }

    if (!PL_DHashTableInit(&aFooter->mURIMap, &objmap_DHashTableOps,
                           (void *)this, sizeof(nsURIMapReadEntry),
                           PL_DHASH_MIN_SIZE)) {
        aFooter->mURIMap.ops = nsnull;
        return NS_ERROR_OUT_OF_MEMORY;
    }

    for (i = 0, n = aFooter->mNumMuxedDocuments; i < n; i++) {
        nsFastLoadMuxedDocumentInfo info;

        rv = ReadMuxedDocumentInfo(&info);
        if (NS_FAILED(rv)) return rv;

        nsDocumentMapReadEntry* entry =
            NS_STATIC_CAST(nsDocumentMapReadEntry*,
                           PL_DHashTableOperate(&aFooter->mDocumentMap,
                                                info.mURISpec,
                                                PL_DHASH_ADD));
        if (!entry) {
            nsMemory::Free((void*) info.mURISpec);
            return NS_ERROR_OUT_OF_MEMORY;
        }

        NS_ASSERTION(!entry->mURISpec, "duplicate URISpec in MuxedDocumentMap");
        entry->mURISpec = info.mURISpec;
        entry->mInitialSegmentOffset = info.mInitialSegmentOffset;
        entry->mNextSegmentOffset = info.mInitialSegmentOffset;
        entry->mBytesLeft = 0;
        entry->mSaveOffset = 0;
    }

    for (i = 0, n = aFooter->mNumDependencies; i < n; i++) {
        char* s;
        rv = ReadStringZ(&s);
        if (NS_FAILED(rv)) return rv;

        if (!aFooter->AppendDependency(s, PR_FALSE)) {
            nsMemory::Free(s);
            return NS_ERROR_OUT_OF_MEMORY;
        }
    }
    return NS_OK;
}

nsresult
nsFastLoadFileReader::ReadFooterPrefix(nsFastLoadFooterPrefix *aFooterPrefix)
{
    nsresult rv;

    rv = Read32(&aFooterPrefix->mNumIDs);
    if (NS_FAILED(rv)) return rv;

    rv = Read32(&aFooterPrefix->mNumSharpObjects);
    if (NS_FAILED(rv)) return rv;

    rv = Read32(&aFooterPrefix->mNumMuxedDocuments);
    if (NS_FAILED(rv)) return rv;

    rv = Read32(&aFooterPrefix->mNumDependencies);
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}

nsresult
nsFastLoadFileReader::ReadSlowID(nsID *aID)
{
    nsresult rv;

    rv = Read32(&aID->m0);
    if (NS_FAILED(rv)) return rv;

    rv = Read16(&aID->m1);
    if (NS_FAILED(rv)) return rv;

    rv = Read16(&aID->m2);
    if (NS_FAILED(rv)) return rv;

    PRUint32 bytesRead;
    rv = Read(NS_REINTERPRET_CAST(char*, aID->m3), sizeof aID->m3, &bytesRead);
    if (NS_FAILED(rv)) return rv;

    if (bytesRead != sizeof aID->m3)
        return NS_ERROR_FAILURE;
    return NS_OK;
}

nsresult
nsFastLoadFileReader::ReadFastID(NSFastLoadID *aID)
{
    nsresult rv = Read32(aID);
    if (NS_SUCCEEDED(rv))
        *aID ^= MFL_ID_XOR_KEY;
    return rv;
}

nsresult
nsFastLoadFileReader::ReadSharpObjectInfo(nsFastLoadSharpObjectInfo *aInfo)
{
    nsresult rv;

    rv = Read32(&aInfo->mCIDOffset);
    if (NS_FAILED(rv)) return rv;

    rv = Read16(&aInfo->mStrongRefCnt);
    if (NS_FAILED(rv)) return rv;

    rv = Read16(&aInfo->mWeakRefCnt);
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}

nsresult
nsFastLoadFileReader::ReadMuxedDocumentInfo(nsFastLoadMuxedDocumentInfo *aInfo)
{
    nsresult rv;

    char *spec;
    rv = ReadStringZ(&spec);
    if (NS_FAILED(rv)) return rv;

    rv = Read32(&aInfo->mInitialSegmentOffset);
    if (NS_FAILED(rv)) {
        nsMemory::Free((void*) spec);
        return rv;
    }

    aInfo->mURISpec = spec;
    return NS_OK;
}

nsresult
nsFastLoadFileReader::Open()
{
    nsCOMPtr<nsISeekableStream> seekable(do_QueryInterface(mInputStream));
    if (!seekable)
        return NS_ERROR_UNEXPECTED;

    nsresult rv;

    // Don't bother buffering the header, as we immediately seek to EOF.
    nsCOMPtr<nsIStreamBufferAccess>
        bufferAccess(do_QueryInterface(mInputStream));
    if (bufferAccess)
        bufferAccess->DisableBuffering();

    rv = ReadHeader(&mHeader);

    if (bufferAccess)
        bufferAccess->EnableBuffering();
    if (NS_FAILED(rv)) return rv;

    if (mHeader.mVersion != MFL_FILE_VERSION)
        return NS_ERROR_UNEXPECTED;
    if (mHeader.mFooterOffset == 0)
        return NS_ERROR_UNEXPECTED;

    rv = seekable->Seek(nsISeekableStream::NS_SEEK_END, 0);
    if (NS_FAILED(rv)) return rv;

    PRUint32 fileSize;
    rv = seekable->Tell(&fileSize);
    if (NS_FAILED(rv)) return rv;

    if (fileSize != mHeader.mFileSize)
        return NS_ERROR_UNEXPECTED;

    rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET,
                        PRInt32(mHeader.mFooterOffset));
    if (NS_FAILED(rv)) return rv;

    rv = ReadFooter(&mFooter);
    if (NS_FAILED(rv)) return rv;

    return seekable->Seek(nsISeekableStream::NS_SEEK_SET,
                          sizeof(nsFastLoadHeader));
}

NS_IMETHODIMP
nsFastLoadFileReader::Close()
{
#ifdef DEBUG_brendan
    PRUint32 strongTotal = 0, weakTotal = 0;
#endif

    // Give up our strong "keepalive" references, in case not all objects that
    // were deserialized were fully re-connected.  This happens for sure when
    // nsFastLoadFileUpdater has to deserialize sharp objects from its aReader
    // constructor parameter in order to map object info by object address, in
    // order to write a valid file footer when the update process completes.
    //
    // XXXbe get rid of strongTotal, weakTotal, and the warnings
    for (PRUint32 i = 0, n = mFooter.mNumSharpObjects; i < n; i++) {
        nsObjectMapEntry* entry = &mFooter.mObjectMap[i];

#ifdef DEBUG_brendan
        strongTotal += entry->mStrongRefCnt;
        weakTotal += entry->mWeakRefCnt;
#endif

        entry->mReadObject = nsnull;
    }

#ifdef DEBUG_brendan
    if (strongTotal != 0)
        NS_WARNING("failed to deserialize all strong refs from FastLoad file");
    if (weakTotal != 0)
        NS_WARNING("failed to deserialize all weak refs from FastLoad file");
#endif

    return mInputStream->Close();
}

nsresult
nsFastLoadFileReader::DeserializeObject(nsISupports* *aObject)
{
    nsresult rv;
    NSFastLoadID fastCID;

    rv = ReadFastID(&fastCID);
    if (NS_FAILED(rv)) return rv;

    const nsID& slowCID = mFooter.GetID(fastCID);
    nsCOMPtr<nsISupports> object(do_CreateInstance(slowCID, &rv));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsISerializable> serializable(do_QueryInterface(object));
    if (!serializable)
        return NS_ERROR_FAILURE;

    rv = serializable->Read(this);
    if (NS_FAILED(rv)) return rv;

    *aObject = object;
    NS_ADDREF(*aObject);
    return NS_OK;
}

nsresult
nsFastLoadFileReader::ReadObject(PRBool aIsStrongRef, nsISupports* *aObject)
{
    nsresult rv;
    NSFastLoadOID oid;

    rv = Read32(&oid);
    if (NS_FAILED(rv)) return rv;
    oid ^= MFL_OID_XOR_KEY;

    nsObjectMapEntry* entry = (oid != MFL_DULL_OBJECT_OID)
                              ? &mFooter.GetSharpObjectEntry(oid)
                              : nsnull;
    nsCOMPtr<nsISupports> object;

    if (!entry) {
        // A very dull object, defined at point of single (strong) reference.
        NS_ASSERTION(aIsStrongRef, "dull object read via weak ref!");

        rv = DeserializeObject(getter_AddRefs(object));
        if (NS_FAILED(rv)) return rv;
    } else {
        NS_ASSERTION((oid & MFL_WEAK_REF_TAG) ==
                     (aIsStrongRef ? 0 : MFL_WEAK_REF_TAG),
                     "strong vs. weak ref deserialization mismatch!");

        // Check whether we've already deserialized the object for this OID.
        object = entry->mReadObject;
        if (!object) {
            nsCOMPtr<nsISeekableStream> seekable(do_QueryInterface(mInputStream));
            PRUint32 saveOffset;
            nsDocumentMapReadEntry* saveDocMapEntry = nsnull;

            rv = seekable->Tell(&saveOffset);
            if (NS_FAILED(rv)) return rv;

            if (entry->mCIDOffset != saveOffset) {
                // We skipped deserialization of this object from its position
                // earlier in the input stream, presumably due to the reference
                // there being an nsFastLoadPtr or some such thing.  Seek back
                // and read it now.
                NS_ASSERTION(entry->mCIDOffset < saveOffset,
                             "out of order object?!");

                // Ape our Seek wrapper by clearing mCurrentDocumentMapEntry.
                // This allows for a skipped object to be referenced from two
                // or more multiplexed documents in the FastLoad file.
                saveDocMapEntry = mCurrentDocumentMapEntry;
                mCurrentDocumentMapEntry = nsnull;
                rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET,
                                    entry->mCIDOffset);
                if (NS_FAILED(rv)) return rv;
            }

            rv = DeserializeObject(getter_AddRefs(object));
            if (NS_FAILED(rv)) return rv;

            if (entry->mCIDOffset != saveOffset) {
                // Save the "skip offset" in case we need to skip this object
                // definition when reading forward, later on.
                rv = seekable->Tell(&entry->mSkipOffset);
                if (NS_FAILED(rv)) return rv;

                // Restore stream offset and mCurrentDocumentMapEntry in case
                // we're still reading forward through a part of the multiplex
                // to get object definitions eagerly.
                rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET, saveOffset);
                if (NS_FAILED(rv)) return rv;
                mCurrentDocumentMapEntry = saveDocMapEntry;
            }

            // Save object until all refs have been deserialized.
            entry->mReadObject = object;
        } else {
            // What if we are at a definition that's already been read?  This
            // case arises when a sharp object's def is serialized before its
            // refs, while a non-defining ref is deserialized before the def.
            // We must skip over the object definition.
            if (oid & MFL_OBJECT_DEF_TAG) {
                NS_ASSERTION(entry->mSkipOffset != 0, "impossible! see above");
                nsCOMPtr<nsISeekableStream> seekable(do_QueryInterface(mInputStream));
                rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET,
                                    entry->mSkipOffset);
                if (NS_FAILED(rv)) return rv;
            }
        }

        if (aIsStrongRef) {
            NS_ASSERTION(entry->mStrongRefCnt != 0, "mStrongRefCnt underflow!");
            entry->mStrongRefCnt--;
        } else {
            NS_ASSERTION(entry->mWeakRefCnt != 0, "mWeakRefCnt underflow!");
            entry->mWeakRefCnt--;
        }

        if (entry->mStrongRefCnt == 0 && entry->mWeakRefCnt == 0)
            entry->mReadObject = nsnull;
    }

    if (oid & MFL_QUERY_INTERFACE_TAG) {
        NSFastLoadID iid;
        rv = ReadFastID(&iid);
        if (NS_FAILED(rv)) return rv;

        rv = object->QueryInterface(mFooter.GetID(iid),
                                    NS_REINTERPRET_CAST(void**, aObject));
        if (NS_FAILED(rv)) return rv;
    } else {
        *aObject = object;
        NS_ADDREF(*aObject);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadFileReader::ReadID(nsID *aResult)
{
    nsresult rv;
    NSFastLoadID fastID;

    rv = ReadFastID(&fastID);
    if (NS_FAILED(rv)) return rv;

    *aResult = mFooter.GetID(fastID);
    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadFileReader::Seek(PRInt32 aWhence, PRInt32 aOffset)
{
    mCurrentDocumentMapEntry = nsnull;
    nsCOMPtr<nsISeekableStream> seekable(do_QueryInterface(mInputStream));
    return seekable->Seek(aWhence, aOffset);
}

NS_IMETHODIMP
nsFastLoadFileReader::Tell(PRUint32 *aResult)
{
    nsCOMPtr<nsISeekableStream> seekable(do_QueryInterface(mInputStream));
    return seekable->Tell(aResult);
}

NS_IMETHODIMP
nsFastLoadFileReader::SetEOF()
{
    nsCOMPtr<nsISeekableStream> seekable(do_QueryInterface(mInputStream));
    return seekable->SetEOF();
}

NS_COM nsresult
NS_NewFastLoadFileReader(nsIObjectInputStream* *aResult,
                         nsIInputStream* aSrcStream)
{
    nsFastLoadFileReader* reader = new nsFastLoadFileReader(aSrcStream);
    if (!reader)
        return NS_ERROR_OUT_OF_MEMORY;

    // Stabilize reader's refcnt.
    nsCOMPtr<nsIObjectInputStream> stream(reader);

    nsresult rv = reader->Open();
    if (NS_FAILED(rv))
        return rv;

    *aResult = stream;
    NS_ADDREF(*aResult);
    return NS_OK;
}

// -------------------------- nsFastLoadFileWriter --------------------------

NS_IMPL_ISUPPORTS_INHERITED3(nsFastLoadFileWriter,
                             nsBinaryOutputStream,
                             nsIObjectOutputStream,
                             nsIFastLoadFileControl,
                             nsISeekableStream)

MOZ_DECL_CTOR_COUNTER(nsFastLoadFileWriter)

struct nsIDMapEntry : public PLDHashEntryHdr {
    NSFastLoadID    mFastID;            // 1 + nsFastLoadFooter::mIDMap index
    nsID            mSlowID;            // key, used by PLDHashTableOps below
};

PR_STATIC_CALLBACK(const void *)
idmap_GetKey(PLDHashTable *aTable, PLDHashEntryHdr *aHdr)
{
    nsIDMapEntry* entry = NS_STATIC_CAST(nsIDMapEntry*, aHdr);

    return &entry->mSlowID;
}

PR_STATIC_CALLBACK(PLDHashNumber)
idmap_HashKey(PLDHashTable *aTable, const void *aKey)
{
    const nsID *idp = NS_REINTERPRET_CAST(const nsID*, aKey);

    return idp->m0;
}

PR_STATIC_CALLBACK(PRBool)
idmap_MatchEntry(PLDHashTable *aTable,
                const PLDHashEntryHdr *aHdr,
                const void *aKey)
{
    const nsIDMapEntry* entry = NS_STATIC_CAST(const nsIDMapEntry*, aHdr);
    const nsID *idp = NS_REINTERPRET_CAST(const nsID*, aKey);

    return memcmp(&entry->mSlowID, idp, sizeof(nsID)) == 0;
}

static PLDHashTableOps idmap_DHashTableOps = {
    PL_DHashAllocTable,
    PL_DHashFreeTable,
    idmap_GetKey,
    idmap_HashKey,
    idmap_MatchEntry,
    PL_DHashMoveEntryStub,
    PL_DHashClearEntryStub,
    PL_DHashFinalizeStub,
    NULL
};

nsresult
nsFastLoadFileWriter::MapID(const nsID& aSlowID, NSFastLoadID *aResult)
{
    nsIDMapEntry* entry =
        NS_STATIC_CAST(nsIDMapEntry*,
                       PL_DHashTableOperate(&mIDMap, &aSlowID, PL_DHASH_ADD));
    if (!entry)
        return NS_ERROR_OUT_OF_MEMORY;

    if (entry->mFastID == 0) {
        entry->mFastID = mIDMap.entryCount;
        entry->mSlowID = aSlowID;
    }

    *aResult = entry->mFastID;
    return NS_OK;
}

nsresult
nsFastLoadFileWriter::WriteHeader(nsFastLoadHeader *aHeader)
{
    nsresult rv;
    PRUint32 bytesWritten;

    rv = Write(aHeader->mMagic, MFL_FILE_MAGIC_SIZE, &bytesWritten);
    if (NS_FAILED(rv)) return rv;

    if (bytesWritten != MFL_FILE_MAGIC_SIZE)
        return NS_ERROR_FAILURE;

    rv = Write32(aHeader->mChecksum);
    if (NS_FAILED(rv)) return rv;

    rv = Write32(aHeader->mVersion);
    if (NS_FAILED(rv)) return rv;

    rv = Write32(aHeader->mFooterOffset);
    if (NS_FAILED(rv)) return rv;

    rv = Write32(aHeader->mFileSize);
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}

// nsIFastLoadFileControl methods:

NS_IMETHODIMP
nsFastLoadFileWriter::GetChecksum(PRUint32 *aChecksum)
{
    if (mHeader.mChecksum == 0)
        return NS_ERROR_NOT_AVAILABLE;
    *aChecksum = mHeader.mChecksum;
    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadFileWriter::SetChecksum(PRUint32 aChecksum)
{
    mHeader.mChecksum = aChecksum;
    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadFileWriter::ComputeChecksum(PRUint32 *aResult)
{
    return GetChecksum(aResult);
}

NS_IMETHODIMP
nsFastLoadFileWriter::GetDependencies(nsICollection* *aDependencies)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

struct nsDocumentMapWriteEntry : public nsDocumentMapEntry {
    PRUint32    mCurrentSegmentOffset;      // last written segment's offset
};

// Fast mapping from URI object pointer back to spec-indexed document info.
struct nsURIMapWriteEntry : public nsObjectMapEntry {
    nsDocumentMapWriteEntry* mDocMapEntry;
};

NS_IMETHODIMP
nsFastLoadFileWriter::StartMuxedDocument(nsISupports* aURI,
                                         const char* aURISpec)
{
    // Save mDocumentMap table generation and mCurrentDocumentMapEntry key in
    // case the hash table grows during the PL_DHASH_ADD operation.
    PRUint32 saveGeneration = 0;
    const char* saveURISpec = nsnull;
    if (mCurrentDocumentMapEntry) {
        saveGeneration = mDocumentMap.generation;
        saveURISpec = mCurrentDocumentMapEntry->mURISpec;
    }

    nsDocumentMapWriteEntry* docMapEntry =
        NS_STATIC_CAST(nsDocumentMapWriteEntry*,
                       PL_DHashTableOperate(&mDocumentMap, aURISpec,
                                            PL_DHASH_ADD));
    if (!docMapEntry)
        return NS_ERROR_OUT_OF_MEMORY;

    // If the generation number changed, refresh mCurrentDocumentMapEntry.
    if (mCurrentDocumentMapEntry && mDocumentMap.generation != saveGeneration) {
        mCurrentDocumentMapEntry =
            NS_STATIC_CAST(nsDocumentMapWriteEntry*,
                           PL_DHashTableOperate(&mDocumentMap, saveURISpec,
                                                PL_DHASH_LOOKUP));
        NS_ASSERTION(PL_DHASH_ENTRY_IS_BUSY(mCurrentDocumentMapEntry),
                     "mCurrentDocumentMapEntry lost during table growth?!");
    }

    NS_ASSERTION(docMapEntry->mURISpec == nsnull,
                 "redundant multiplexed document?");
    if (docMapEntry->mURISpec)
        return NS_ERROR_UNEXPECTED;

    void* spec = nsMemory::Clone(aURISpec, strlen(aURISpec) + 1);
    if (!spec)
        return NS_ERROR_OUT_OF_MEMORY;
    docMapEntry->mURISpec = NS_REINTERPRET_CAST(const char*, spec);

    nsCOMPtr<nsISupports> key(do_QueryInterface(aURI));
    nsURIMapWriteEntry* uriMapEntry =
        NS_STATIC_CAST(nsURIMapWriteEntry*,
                       PL_DHashTableOperate(&mURIMap, key, PL_DHASH_ADD));
    if (!uriMapEntry)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ASSERTION(uriMapEntry->mDocMapEntry == nsnull,
                 "URI mapped to two different specs?");
    if (uriMapEntry->mDocMapEntry)
        return NS_ERROR_UNEXPECTED;

    uriMapEntry->mObject = key;
    NS_ADDREF(uriMapEntry->mObject);
    uriMapEntry->mDocMapEntry = docMapEntry;
    TRACE_MUX(('w', "start %p (%p) %s\n", aURI, key.get(), aURISpec));
    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadFileWriter::SelectMuxedDocument(nsISupports* aURI)
{
    // Avoid repeatedly QI'ing to nsISeekableStream as we tell and seek.
    nsCOMPtr<nsISeekableStream> seekable(do_QueryInterface(mOutputStream));

    // Capture the current file offset (XXXbe maintain our own via Write?)
    nsresult rv;
    PRUint32 currentSegmentOffset;
    rv = seekable->Tell(&currentSegmentOffset);
    if (NS_FAILED(rv)) return rv;

    // Look for an existing entry keyed by aURI, added by StartMuxedDocument.
    nsCOMPtr<nsISupports> key(do_QueryInterface(aURI));
    nsURIMapWriteEntry* uriMapEntry =
        NS_STATIC_CAST(nsURIMapWriteEntry*,
                       PL_DHashTableOperate(&mURIMap, key, PL_DHASH_LOOKUP));
    NS_ASSERTION(PL_DHASH_ENTRY_IS_BUSY(uriMapEntry),
                 "SelectMuxedDocument without prior StartMuxedDocument?");
    if (PL_DHASH_ENTRY_IS_FREE(uriMapEntry))
        return NS_ERROR_UNEXPECTED;

    nsDocumentMapWriteEntry* docMapEntry = uriMapEntry->mDocMapEntry;

    // If there is a muxed document segment open, close it now by setting its
    // length, stored in the second PRUint32 of the segment.
    nsDocumentMapWriteEntry* prevDocMapEntry = mCurrentDocumentMapEntry;
    if (prevDocMapEntry) {
        if (prevDocMapEntry == docMapEntry) {
            TRACE_MUX(('w', "select prev %s same as current!\n",
                       prevDocMapEntry->mURISpec));
            return NS_OK;
        }

        PRUint32 prevSegmentOffset = prevDocMapEntry->mCurrentSegmentOffset;
        TRACE_MUX(('w', "select prev %s offset %lu\n",
                   prevDocMapEntry->mURISpec, prevSegmentOffset));

        rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET,
                            prevSegmentOffset + 4);
        if (NS_FAILED(rv)) return rv;

        // The length counts all bytes in the segment, including the header
        // that contains [nextSegmentOffset, length].
        rv = Write32(currentSegmentOffset - prevSegmentOffset);
        if (NS_FAILED(rv)) return rv;

        // Seek back to the current offset only if we are not going to seek
        // back to *this* entry's last "current" segment offset and write its
        // next segment offset at the first PRUint32 of the segment.
        if (!docMapEntry->mInitialSegmentOffset) {
            rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET,
                                currentSegmentOffset);
            if (NS_FAILED(rv)) return rv;
        }
    }

    // If this entry was newly added, set its key and initial segment offset.
    // Otherwise, seek back to write the next segment offset of the previous
    // segment for this document in the multiplex.
    if (!docMapEntry->mInitialSegmentOffset) {
        docMapEntry->mInitialSegmentOffset = currentSegmentOffset;
    } else {
        rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET,
                            docMapEntry->mCurrentSegmentOffset);
        if (NS_FAILED(rv)) return rv;

        rv = Write32(currentSegmentOffset);
        if (NS_FAILED(rv)) return rv;

        rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET,
                            currentSegmentOffset);
        if (NS_FAILED(rv)) return rv;
    }

    // Update this document's current segment offset so we can later fix its
    // next segment offset (unless it is last, in which case we leave the zero
    // placeholder as a terminator).
    docMapEntry->mCurrentSegmentOffset = currentSegmentOffset;

    rv = Write32(0);    // nextSegmentOffset placeholder
    if (NS_FAILED(rv)) return rv;

    rv = Write32(0);    // length placeholder
    if (NS_FAILED(rv)) return rv;

    mCurrentDocumentMapEntry = docMapEntry;
    TRACE_MUX(('w', "select %p (%p) offset %lu\n",
               aURI, key.get(), currentSegmentOffset));
    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadFileWriter::EndMuxedDocument(nsISupports* aURI)
{
    nsCOMPtr<nsISupports> key(do_QueryInterface(aURI));
#ifdef NS_DEBUG
    nsURIMapWriteEntry* uriMapEntry =
        NS_STATIC_CAST(nsURIMapWriteEntry*,
                       PL_DHashTableOperate(&mURIMap, key, PL_DHASH_LOOKUP));
    NS_ASSERTION(uriMapEntry && uriMapEntry->mDocMapEntry,
                 "unknown aURI passed to EndMuxedDocument!");
#endif

    PL_DHashTableOperate(&mURIMap, key, PL_DHASH_REMOVE);
    TRACE_MUX(('w', "end %p (%p)\n", aURI, key.get()));
    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadFileWriter::Write(const char* aBuffer, PRUint32 aCount,
                            PRUint32 *aBytesWritten)
{
    nsresult rv;

    rv = mOutputStream->Write(aBuffer, aCount, aBytesWritten);

    if (NS_SUCCEEDED(rv)) {
        PRUint32 bytesToCheck = *aBytesWritten;

        while (bytesToCheck != 0) {
            PRUint32 bytesThatFit = bytesToCheck;
            PRUint32 nextCursor = mChecksumCursor + bytesThatFit;
            PRInt32 didntFit = PRInt32(nextCursor - MFL_CHECKSUM_BUFSIZE);
            if (didntFit > 0)
                bytesThatFit -= didntFit;

            memcpy(mChecksumBuffer + mChecksumCursor, aBuffer, bytesThatFit);
            mChecksumCursor += bytesThatFit;
            if (mChecksumCursor == MFL_CHECKSUM_BUFSIZE) {
                nextCursor =
                    NS_AccumulateFastLoadChecksum(&mHeader.mChecksum,
                                                  mChecksumBuffer,
                                                  MFL_CHECKSUM_BUFSIZE,
                                                  PR_FALSE);
                mCheckedByteCount += MFL_CHECKSUM_BUFSIZE - nextCursor;

                NS_ASSERTION(nextCursor == 0, "odd mChecksumBuffer alignment?");
                if (nextCursor != 0) {
                    memcpy(mChecksumBuffer,
                           mChecksumBuffer + MFL_CHECKSUM_BUFSIZE - nextCursor,
                           nextCursor);
                }
            }

            mChecksumCursor = nextCursor;
            bytesToCheck -= bytesThatFit;
        }
    }

    return rv;
}

nsresult
nsFastLoadFileWriter::WriteFooterPrefix(const nsFastLoadFooterPrefix& aFooterPrefix)
{
    nsresult rv;

    rv = Write32(aFooterPrefix.mNumIDs);
    if (NS_FAILED(rv)) return rv;

    rv = Write32(aFooterPrefix.mNumSharpObjects);
    if (NS_FAILED(rv)) return rv;

    rv = Write32(aFooterPrefix.mNumMuxedDocuments);
    if (NS_FAILED(rv)) return rv;

    rv = Write32(aFooterPrefix.mNumDependencies);
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}

nsresult
nsFastLoadFileWriter::WriteSlowID(const nsID& aID)
{
    nsresult rv;

    rv = Write32(aID.m0);
    if (NS_FAILED(rv)) return rv;

    rv = Write16(aID.m1);
    if (NS_FAILED(rv)) return rv;

    rv = Write16(aID.m2);
    if (NS_FAILED(rv)) return rv;

    PRUint32 bytesWritten;
    rv = Write(NS_REINTERPRET_CAST(const char*, aID.m3), sizeof aID.m3,
               &bytesWritten);
    if (NS_FAILED(rv)) return rv;

    if (bytesWritten != sizeof aID.m3)
        return NS_ERROR_FAILURE;
    return NS_OK;
}

nsresult
nsFastLoadFileWriter::WriteFastID(NSFastLoadID aID)
{
    return Write32(aID ^ MFL_ID_XOR_KEY);
}

nsresult
nsFastLoadFileWriter::WriteSharpObjectInfo(const nsFastLoadSharpObjectInfo& aInfo)
{
    nsresult rv;

    rv = Write32(aInfo.mCIDOffset);
    if (NS_FAILED(rv)) return rv;

    rv = Write16(aInfo.mStrongRefCnt);
    if (NS_FAILED(rv)) return rv;

    rv = Write16(aInfo.mWeakRefCnt);
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}

nsresult
nsFastLoadFileWriter::WriteMuxedDocumentInfo(const nsFastLoadMuxedDocumentInfo& aInfo)
{
    nsresult rv;

    rv = WriteStringZ(aInfo.mURISpec);
    if (NS_FAILED(rv)) return rv;

    rv = Write32(aInfo.mInitialSegmentOffset);
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}

PLDHashOperator PR_CALLBACK
nsFastLoadFileWriter::IDMapEnumerate(PLDHashTable *aTable,
                                     PLDHashEntryHdr *aHdr,
                                     PRUint32 aNumber,
                                     void *aData)
{
    nsIDMapEntry* entry = NS_STATIC_CAST(nsIDMapEntry*, aHdr);
    PRUint32 index = entry->mFastID - 1;
    nsID* vector = NS_REINTERPRET_CAST(nsID*, aData);

    NS_ASSERTION(index < aTable->entryCount, "bad nsIDMap index!");
    vector[index] = entry->mSlowID;
    return PL_DHASH_NEXT;
}

struct nsSharpObjectMapEntry : public nsObjectMapEntry {
    NSFastLoadOID               mOID;
    nsFastLoadSharpObjectInfo   mInfo;
};

PLDHashOperator PR_CALLBACK
nsFastLoadFileWriter::ObjectMapEnumerate(PLDHashTable *aTable,
                                         PLDHashEntryHdr *aHdr,
                                         PRUint32 aNumber,
                                         void *aData)
{
    nsSharpObjectMapEntry* entry = NS_STATIC_CAST(nsSharpObjectMapEntry*, aHdr);
    PRUint32 index = MFL_OID_TO_SHARP_INDEX(entry->mOID);
    nsFastLoadSharpObjectInfo* vector =
        NS_REINTERPRET_CAST(nsFastLoadSharpObjectInfo*, aData);

    NS_ASSERTION(index < aTable->entryCount, "bad nsObjectMap index!");
    vector[index] = entry->mInfo;

#ifdef NS_DEBUG
    NS_ASSERTION(entry->mInfo.mStrongRefCnt, "no strong ref in serialization!");

    if ((NSFastLoadOID(NS_PTR_TO_INT32(entry->mObject)) & MFL_OBJECT_DEF_TAG) == 0) {
        nsrefcnt rc = entry->mObject->AddRef();
        NS_ASSERTION(entry->mInfo.mStrongRefCnt <= rc - 2,
                     "too many strong refs in serialization");
        entry->mObject->Release();
    }
#endif

    // Ignore tagged object ids stored as object pointer keys (the updater
    // code does this).
    if ((NSFastLoadOID(NS_PTR_TO_INT32(entry->mObject)) & MFL_OBJECT_DEF_TAG) == 0)
        NS_RELEASE(entry->mObject);

    return PL_DHASH_NEXT;
}

PLDHashOperator PR_CALLBACK
nsFastLoadFileWriter::DocumentMapEnumerate(PLDHashTable *aTable,
                                           PLDHashEntryHdr *aHdr,
                                           PRUint32 aNumber,
                                           void *aData)
{
    nsFastLoadFileWriter* writer =
        NS_REINTERPRET_CAST(nsFastLoadFileWriter*, aTable->data);
    nsDocumentMapWriteEntry* entry =
        NS_STATIC_CAST(nsDocumentMapWriteEntry*, aHdr);
    nsresult* rvp = NS_REINTERPRET_CAST(nsresult*, aData);

    nsFastLoadMuxedDocumentInfo info;
    info.mURISpec = entry->mURISpec;
    info.mInitialSegmentOffset = entry->mInitialSegmentOffset;
    *rvp = writer->WriteMuxedDocumentInfo(info);

    return NS_FAILED(*rvp) ? PL_DHASH_STOP : PL_DHASH_NEXT;
}

nsresult
nsFastLoadFileWriter::WriteFooter()
{
    nsresult rv;
    PRUint32 i, count;

    nsFastLoadFooterPrefix footerPrefix;
    footerPrefix.mNumIDs = mIDMap.entryCount;
    footerPrefix.mNumSharpObjects = mObjectMap.entryCount;
    footerPrefix.mNumMuxedDocuments = mDocumentMap.entryCount;
    footerPrefix.mNumDependencies = mDependencies.Count();

    rv = WriteFooterPrefix(footerPrefix);
    if (NS_FAILED(rv)) return rv;

    // Enumerate mIDMap into a vector indexed by mFastID and write it.
    nsID* idvec = new nsID[footerPrefix.mNumIDs];
    if (!idvec)
        return NS_ERROR_OUT_OF_MEMORY;

    count = PL_DHashTableEnumerate(&mIDMap, IDMapEnumerate, idvec);
    NS_ASSERTION(count == footerPrefix.mNumIDs, "bad mIDMap enumeration!");
    for (i = 0; i < count; i++) {
        rv = WriteSlowID(idvec[i]);
        if (NS_FAILED(rv)) break;
    }

    delete[] idvec;
    if (NS_FAILED(rv)) return rv;

    // Enumerate mObjectMap into a vector indexed by mOID and write it.
    nsFastLoadSharpObjectInfo* objvec =
        new nsFastLoadSharpObjectInfo[footerPrefix.mNumSharpObjects];
    if (!objvec)
        return NS_ERROR_OUT_OF_MEMORY;

    count = PL_DHashTableEnumerate(&mObjectMap, ObjectMapEnumerate, objvec);
    NS_ASSERTION(count == footerPrefix.mNumSharpObjects,
                 "bad mObjectMap enumeration!");
    for (i = 0; i < count; i++) {
        rv = WriteSharpObjectInfo(objvec[i]);
        if (NS_FAILED(rv)) break;
    }

    delete[] objvec;
    if (NS_FAILED(rv)) return rv;

    // Enumerate mDocumentMap, writing nsFastLoadMuxedDocumentInfo records
    count = PL_DHashTableEnumerate(&mDocumentMap, DocumentMapEnumerate, &rv);
    if (NS_FAILED(rv)) return rv;

    NS_ASSERTION(count == footerPrefix.mNumMuxedDocuments,
                 "bad mDocumentMap enumeration!");

    // Write out make-like file dependencies.
    count = footerPrefix.mNumDependencies;
    for (i = 0; i < count; i++) {
        const char* s =
          NS_REINTERPRET_CAST(const char*, mDependencies.ElementAt(PRInt32(i)));
        rv = WriteStringZ(s);
        if (NS_FAILED(rv)) return rv;
    }

    return NS_OK;
}

nsresult
nsFastLoadFileWriter::Init()
{
    if (!PL_DHashTableInit(&mIDMap, &idmap_DHashTableOps, (void *)this,
                           sizeof(nsIDMapEntry), PL_DHASH_MIN_SIZE)) {
        mIDMap.ops = nsnull;
        return NS_ERROR_OUT_OF_MEMORY;
    }

    if (!PL_DHashTableInit(&mObjectMap, &objmap_DHashTableOps, (void *)this,
                           sizeof(nsSharpObjectMapEntry), PL_DHASH_MIN_SIZE)) {
        mObjectMap.ops = nsnull;
        return NS_ERROR_OUT_OF_MEMORY;
    }

    if (!PL_DHashTableInit(&mDocumentMap, &docmap_DHashTableOps, (void *)this,
                           sizeof(nsDocumentMapWriteEntry),
                           PL_DHASH_MIN_SIZE)) {
        mDocumentMap.ops = nsnull;
        return NS_ERROR_OUT_OF_MEMORY;
    }

    if (!PL_DHashTableInit(&mURIMap, &objmap_DHashTableOps, (void *)this,
                           sizeof(nsURIMapWriteEntry), PL_DHASH_MIN_SIZE)) {
        mURIMap.ops = nsnull;
        return NS_ERROR_OUT_OF_MEMORY;
    }

    return NS_OK;
}

nsresult
nsFastLoadFileWriter::Open()
{
    nsCOMPtr<nsISeekableStream> seekable(do_QueryInterface(mOutputStream));
    if (!seekable)
        return NS_ERROR_UNEXPECTED;

    nsresult rv;

    rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET,
                        sizeof(nsFastLoadHeader));
    if (NS_FAILED(rv)) return rv;

    return Init();
}

NS_IMETHODIMP
nsFastLoadFileWriter::Close()
{
    nsresult rv;

    memcpy(mHeader.mMagic, magic, MFL_FILE_MAGIC_SIZE);
    mHeader.mChecksum = 0;
    mHeader.mVersion = MFL_FILE_VERSION;

    nsCOMPtr<nsISeekableStream> seekable(do_QueryInterface(mOutputStream));

    rv = seekable->Tell(&mHeader.mFooterOffset);
    if (NS_FAILED(rv)) return rv;

    // If there is a muxed document segment open, close it now by setting its
    // length, stored in the second PRUint32 of the segment.
    if (mCurrentDocumentMapEntry) {
        PRUint32 currentSegmentOffset =
            mCurrentDocumentMapEntry->mCurrentSegmentOffset;
        rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET,
                            currentSegmentOffset + 4);
        if (NS_FAILED(rv)) return rv;

        rv = Write32(mHeader.mFooterOffset - currentSegmentOffset);
        if (NS_FAILED(rv)) return rv;

        // Seek back to the current offset to write the footer.
        rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET,
                            mHeader.mFooterOffset);
        if (NS_FAILED(rv)) return rv;

        mCurrentDocumentMapEntry = nsnull;
    }

    rv = WriteFooter();
    if (NS_FAILED(rv)) return rv;

    rv = seekable->Tell(&mHeader.mFileSize);
    if (NS_FAILED(rv)) return rv;

    /* Sum any bytes left in the checksum buffer. */
    if (mChecksumCursor != 0) {
        NS_AccumulateFastLoadChecksum(&mHeader.mChecksum,
                                      mChecksumBuffer,
                                      mChecksumCursor,
                                      PR_TRUE);
        mCheckedByteCount += mChecksumCursor;
    }

    PRUint32 headChecksum;
    NS_AccumulateFastLoadChecksum(&headChecksum,
                                  NS_REINTERPRET_CAST(PRUint8*, &mHeader),
                                  sizeof mHeader,
                                  PR_FALSE);

    mHeader.mChecksum =
        NS_AddFastLoadChecksums(headChecksum, mHeader.mChecksum,
                                mCheckedByteCount);

    rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET, 0);
    if (NS_FAILED(rv)) return rv;

    rv = WriteHeader(&mHeader);
    if (NS_FAILED(rv)) return rv;

    return mOutputStream->Close();
}

// Psuedo-tag used as flag between WriteSingleRefObject and WriteCommon.
#define MFL_SINGLE_REF_PSEUDO_TAG       PR_BIT(MFL_OBJECT_TAG_BITS)

nsresult
nsFastLoadFileWriter::WriteObjectCommon(nsISupports* aObject,
                                        PRBool aIsStrongRef,
                                        PRUint32 aTags)
{
    nsrefcnt rc;
    nsresult rv;

    NS_ASSERTION((NSFastLoadOID(NS_PTR_TO_INT32(aObject)) & MFL_OBJECT_DEF_TAG) == 0,
                 "odd nsISupports*, oh no!");

    // Here be manual refcounting dragons!
    rc = aObject->AddRef();
    NS_ASSERTION(rc != 0, "bad refcnt when writing aObject!");

    NSFastLoadOID oid;

    if (rc == 2 && (aTags & MFL_SINGLE_REF_PSEUDO_TAG)) {
        // Dull object: only one strong ref and no weak refs in serialization.
        // Conservative: we don't trust the caller if there are more than two
        // refs (one from the AddRef above, one from the data structure that's
        // being serialized).
        oid = MFL_DULL_OBJECT_OID;
        aObject->Release();
    } else {
        // Object is presumed to be multiply connected through some combo of
        // strong and weak refs.  Hold onto it via mObjectMap.
        nsSharpObjectMapEntry* entry =
            NS_STATIC_CAST(nsSharpObjectMapEntry*,
                           PL_DHashTableOperate(&mObjectMap, aObject,
                                                PL_DHASH_ADD));
        if (!entry) {
            aObject->Release();
            return NS_ERROR_OUT_OF_MEMORY;
        }

        if (!entry->mObject) {
            // First time we've seen this object address: add it to mObjectMap
            // and serialize the object at the current stream offset.
            PRUint32 thisOffset;
            rv = Tell(&thisOffset);
            if (NS_FAILED(rv)) {
                aObject->Release();
                return rv;
            }

            // NB: aObject was already held, and mObject is a raw nsISupports*.
            entry->mObject = aObject;

            oid = (mObjectMap.entryCount << MFL_OBJECT_TAG_BITS);
            entry->mOID = oid;

            // NB: the (32-bit, fast) CID and object data follow the OID.
            entry->mInfo.mCIDOffset = thisOffset + sizeof(oid);
            entry->mInfo.mStrongRefCnt = aIsStrongRef ? 1 : 0;
            entry->mInfo.mWeakRefCnt   = aIsStrongRef ? 0 : 1;

            oid |= MFL_OBJECT_DEF_TAG;
        } else {
            // Already serialized, recover oid and update the desired refcnt.
            oid = entry->mOID;
            if (aIsStrongRef) {
                entry->mInfo.mStrongRefCnt++;
                NS_ASSERTION(entry->mInfo.mStrongRefCnt != 0,
                             "mStrongRefCnt overflow");
            } else {
                entry->mInfo.mWeakRefCnt++;
                NS_ASSERTION(entry->mInfo.mWeakRefCnt != 0,
                             "mWeakRefCnt overflow");
            }

            aObject->Release();
        }
    }

    if (!aIsStrongRef)
        oid |= MFL_WEAK_REF_TAG;
    oid |= (aTags & MFL_QUERY_INTERFACE_TAG);

    rv = Write32(oid ^ MFL_OID_XOR_KEY);
    if (NS_FAILED(rv)) return rv;

    if (oid & MFL_OBJECT_DEF_TAG) {
        nsCOMPtr<nsIClassInfo> classInfo(do_QueryInterface(aObject));
        nsCOMPtr<nsISerializable> serializable(do_QueryInterface(aObject));
        if (!classInfo || !serializable)
            return NS_ERROR_FAILURE;

        nsCID slowCID;
        rv = classInfo->GetClassIDNoAlloc(&slowCID);
        if (NS_FAILED(rv)) return rv;

        NSFastLoadID fastCID;
        rv = MapID(slowCID, &fastCID);
        if (NS_FAILED(rv)) return rv;

        rv = WriteFastID(fastCID);
        if (NS_FAILED(rv)) return rv;

        rv = serializable->Write(this);
        if (NS_FAILED(rv)) return rv;
    }

    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadFileWriter::WriteObject(nsISupports* aObject, PRBool aIsStrongRef)
{
#ifdef NS_DEBUG
    nsCOMPtr<nsISupports> rootObject(do_QueryInterface(aObject));

    NS_ASSERTION(rootObject.get() == aObject,
                 "bad call to WriteObject -- call WriteCompoundObject!");
#endif

    return WriteObjectCommon(aObject, aIsStrongRef, 0);
}

NS_IMETHODIMP
nsFastLoadFileWriter::WriteSingleRefObject(nsISupports* aObject)
{
#ifdef NS_DEBUG
    nsCOMPtr<nsISupports> rootObject(do_QueryInterface(aObject));

    NS_ASSERTION(rootObject.get() == aObject,
                 "bad call to WriteObject -- call WriteCompoundObject!");
#endif

    return WriteObjectCommon(aObject, PR_TRUE, MFL_SINGLE_REF_PSEUDO_TAG);
}

NS_IMETHODIMP
nsFastLoadFileWriter::WriteCompoundObject(nsISupports* aObject,
                                          const nsIID& aIID,
                                          PRBool aIsStrongRef)
{
    nsresult rv;
    nsCOMPtr<nsISupports> rootObject(do_QueryInterface(aObject));

#ifdef NS_DEBUG
    nsCOMPtr<nsISupports> roundtrip;
    rootObject->QueryInterface(aIID, getter_AddRefs(roundtrip));

    NS_ASSERTION(rootObject.get() != aObject,
                 "wasteful call to WriteCompoundObject -- call WriteObject!");
    NS_ASSERTION(roundtrip.get() == aObject,
                 "bad aggregation or multiple inheritance detected by call to "
                 "WriteCompoundObject!");
#endif

    rv = WriteObjectCommon(rootObject, aIsStrongRef, MFL_QUERY_INTERFACE_TAG);
    if (NS_FAILED(rv)) return rv;

    NSFastLoadID iid;
    rv = MapID(aIID, &iid);
    if (NS_FAILED(rv)) return rv;

    return WriteFastID(iid);
}

NS_IMETHODIMP
nsFastLoadFileWriter::WriteID(const nsID& aID)
{
    nsresult rv;
    NSFastLoadID fastID;

    rv = MapID(aID, &fastID);
    if (NS_FAILED(rv)) return rv;

    return WriteFastID(fastID);
}

NS_IMETHODIMP
nsFastLoadFileWriter::Seek(PRInt32 aWhence, PRInt32 aOffset)
{
    mCurrentDocumentMapEntry = nsnull;
    nsCOMPtr<nsISeekableStream> seekable(do_QueryInterface(mOutputStream));
    return seekable->Seek(aWhence, aOffset);
}

NS_IMETHODIMP
nsFastLoadFileWriter::Tell(PRUint32 *aResult)
{
    nsCOMPtr<nsISeekableStream> seekable(do_QueryInterface(mOutputStream));
    return seekable->Tell(aResult);
}

NS_IMETHODIMP
nsFastLoadFileWriter::SetEOF()
{
    nsCOMPtr<nsISeekableStream> seekable(do_QueryInterface(mOutputStream));
    return seekable->SetEOF();
}

NS_COM nsresult
NS_NewFastLoadFileWriter(nsIObjectOutputStream* *aResult,
                         nsIOutputStream* aDestStream)
{
    nsFastLoadFileWriter* writer = new nsFastLoadFileWriter(aDestStream);
    if (!writer)
        return NS_ERROR_OUT_OF_MEMORY;

    // Stabilize writer's refcnt.
    nsCOMPtr<nsIObjectOutputStream> stream(writer);

    nsresult rv = writer->Open();
    if (NS_FAILED(rv))
        return rv;

    *aResult = stream;
    NS_ADDREF(*aResult);
    return NS_OK;
}

// -------------------------- nsFastLoadFileUpdater --------------------------

NS_IMPL_ISUPPORTS_INHERITED0(nsFastLoadFileUpdater,
                             nsFastLoadFileWriter)

PLDHashOperator PR_CALLBACK
nsFastLoadFileUpdater::CopyReadDocumentMapEntryToUpdater(PLDHashTable *aTable,
                                                         PLDHashEntryHdr *aHdr,
                                                         PRUint32 aNumber,
                                                         void *aData)
{
    nsDocumentMapReadEntry* readEntry =
        NS_STATIC_CAST(nsDocumentMapReadEntry*, aHdr);
    nsFastLoadFileUpdater* updater =
        NS_REINTERPRET_CAST(nsFastLoadFileUpdater*, aData);

    void* spec = nsMemory::Clone(readEntry->mURISpec,
                                 strlen(readEntry->mURISpec) + 1);
    if (!spec)
        return PL_DHASH_STOP;

    nsDocumentMapWriteEntry* writeEntry =
        NS_STATIC_CAST(nsDocumentMapWriteEntry*,
                       PL_DHashTableOperate(&updater->mDocumentMap, spec,
                                            PL_DHASH_ADD));
    if (!writeEntry) {
        nsMemory::Free(spec);
        return PL_DHASH_STOP;
    }

    writeEntry->mURISpec = NS_REINTERPRET_CAST(const char*, spec);
    writeEntry->mInitialSegmentOffset = readEntry->mInitialSegmentOffset;
    writeEntry->mCurrentSegmentOffset = 0;
    return PL_DHASH_NEXT;
}

nsresult
nsFastLoadFileUpdater::Open(nsFastLoadFileReader* aReader)
{
    nsCOMPtr<nsISeekableStream> seekable(do_QueryInterface(mOutputStream));
    if (!seekable)
        return NS_ERROR_UNEXPECTED;

    nsresult rv;
    rv = nsFastLoadFileWriter::Init();
    if (NS_FAILED(rv)) return rv;

    PRUint32 i, n;

    // Map from dense, zero-based, uint32 NSFastLoadID in reader to 16-byte
    // nsID in updater.
    nsID* readIDMap = aReader->mFooter.mIDMap;
    for (i = 0, n = aReader->mFooter.mNumIDs; i < n; i++) {
        NSFastLoadID fastID;
        rv = MapID(readIDMap[i], &fastID);
        NS_ASSERTION(fastID == i + 1, "huh?");
        if (NS_FAILED(rv)) return rv;
    }

    // Map from reader dense, zero-based MFL_OID_TO_SHARP_INDEX(oid) to sharp
    // object offset and refcnt information in updater.
    PRUint32 saveReadOffset;
    rv = aReader->Tell(&saveReadOffset);
    if (NS_FAILED(rv)) return rv;

    nsFastLoadFileReader::nsObjectMapEntry* readObjectMap =
        aReader->mFooter.mObjectMap;
    for (i = 0, n = aReader->mFooter.mNumSharpObjects; i < n; i++) {
        nsFastLoadFileReader::nsObjectMapEntry* readEntry = &readObjectMap[i];

        nsISupports* obj = readEntry->mReadObject;
        NSFastLoadOID oid = MFL_SHARP_INDEX_TO_OID(i);
        void* key = obj
                    ? NS_REINTERPRET_CAST(void*, obj)
                    : NS_REINTERPRET_CAST(void*, (oid | MFL_OBJECT_DEF_TAG));

        nsSharpObjectMapEntry* writeEntry =
            NS_STATIC_CAST(nsSharpObjectMapEntry*,
                           PL_DHashTableOperate(&mObjectMap, key,
                                                PL_DHASH_ADD));
        if (!writeEntry)
            return NS_ERROR_OUT_OF_MEMORY;

        // Hold the object if there is one, so that objmap_ClearEntry can
        // release the reference.
        NS_IF_ADDREF(obj);
        writeEntry->mObject = NS_REINTERPRET_CAST(nsISupports*, key);
        writeEntry->mOID = oid;
        writeEntry->mInfo = *NS_STATIC_CAST(nsFastLoadSharpObjectInfo*,
                                            readEntry);
    }

    rv = aReader->Seek(nsISeekableStream::NS_SEEK_SET, saveReadOffset);
    if (NS_FAILED(rv)) return rv;

    // Copy URI spec string and initial segment offset in FastLoad file from
    // nsDocumentMapReadEntry in reader to mDocumentMapWriteEntry in updater.
    // If we didn't enumerate all entries, we ran out of memory.
    n = PL_DHashTableEnumerate(&aReader->mFooter.mDocumentMap,
                               CopyReadDocumentMapEntryToUpdater,
                               this);
    if (n != aReader->mFooter.mDocumentMap.entryCount)
        return NS_ERROR_OUT_OF_MEMORY;

    // Copy source filename dependencies from reader to updater.
    nsFastLoadDependencyArray& readDeps = aReader->mFooter.mDependencies;
    for (i = 0, n = readDeps.Count(); i < n; i++) {
        const char* s = NS_REINTERPRET_CAST(const char*,
                                            readDeps.ElementAt(PRInt32(i)));
        if (!mDependencies.AppendDependency(s))
            return NS_ERROR_OUT_OF_MEMORY;
    }

    // Seek to the reader's footer offset so we overwrite the footer.  First,
    // update the header to have a zero mFooterOffset, which will invalidate
    // the FastLoad file on next startup read attempt, should we crash before
    // completing this update.
    rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET,
                        offsetof(nsFastLoadHeader, mFooterOffset));
    if (NS_FAILED(rv)) return rv;

    rv = Write32(0);
    if (NS_FAILED(rv)) return rv;

    rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET,
                        aReader->mHeader.mFooterOffset);
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}

NS_COM nsresult
NS_NewFastLoadFileUpdater(nsIObjectOutputStream* *aResult,
                          nsIOutputStream* aOutputStream,
                          nsFastLoadFileReader* aReader)
{
    nsFastLoadFileUpdater* updater = new nsFastLoadFileUpdater(aOutputStream);
    if (!updater)
        return NS_ERROR_OUT_OF_MEMORY;

    // Stabilize updater's refcnt.
    nsCOMPtr<nsIObjectOutputStream> stream(updater);

    nsresult rv = updater->Open(aReader);
    if (NS_FAILED(rv))
        return rv;

    *aResult = stream;
    NS_ADDREF(*aResult);
    return NS_OK;
}
