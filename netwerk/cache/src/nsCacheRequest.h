/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is nsCacheRequest.h, released February 22, 2001.
 * 
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 2001 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s): 
 *    Gordon Sheridan, 22-February-2001
 */

#ifndef _nsCacheRequest_h_
#define _nsCacheRequest_h_

#include "nspr.h"
#include "nsCOMPtr.h"
#include "nsICache.h"
#include "nsICacheListener.h"
#include "nsIEventQueue.h"


class nsCacheRequest : public PRCList
{
private:
    friend class nsCacheService;
    friend class nsCacheEntry;

    nsCacheRequest( nsCString *           key, 
                    nsICacheListener *    listener,
                    nsCacheAccessMode     accessRequested,
                    PRBool                streamBased,
                    nsCacheStoragePolicy  storagePolicy)

        : mKey(key),
          mInfo(0),
          mListener(listener),
          mEventQ(nsnull),
          mLock(nsnull),
          mCondVar(nsnull)
    {
        PR_INIT_CLIST(this);
        SetAccessRequested(accessRequested);
        if (streamBased)  MarkStreamBased();
        SetStoragePolicy(storagePolicy);
        MarkWaitingForValidation();
    }
    
    ~nsCacheRequest()
    {
        delete mKey;
        if (mLock)    PR_DestroyLock(mLock);
        if (mCondVar) PR_DestroyCondVar(mCondVar);
        NS_ASSERTION(PR_CLIST_IS_EMPTY(this), "request still on a list");
    }
    
    /**
     * Simple Accessors
     */
    enum CacheRequestInfo {
        eAccessRequestedMask       = 0xFF000000,
        eStoragePolicyMask         = 0x00FF0000,
        eStreamBasedMask           = 0x00000010,
        eWaitingForValidationMask  = 0x00000001
    };

    void SetAccessRequested(nsCacheAccessMode mode)
    {
        NS_ASSERTION(mode <= 0xFF, "too many bits in nsCacheAccessMode");
        mInfo &= ~eAccessRequestedMask;
        mInfo |= mode << 24;
    }

    nsCacheAccessMode AccessRequested()
    {
        return (nsCacheAccessMode)((mInfo >> 24) & 0xFF);
    }

    void MarkStreamBased()      { mInfo |=  eStreamBasedMask; }
    PRBool IsStreamBased()      { return (mInfo & eStreamBasedMask) != 0; }

    void SetStoragePolicy(nsCacheStoragePolicy policy)
    {
        NS_ASSERTION(policy <= 0xFF, "too many bits in nsCacheStoragePolicy");
        mInfo &= ~eStoragePolicyMask;  // clear storage policy bits
        mInfo |= policy << 16;         // or in new bits
    }

    nsCacheStoragePolicy StoragePolicy()
    {
        return (nsCacheStoragePolicy)((mInfo >> 16) & 0xFF);
    }

    void   MarkWaitingForValidation() { mInfo |=  eWaitingForValidationMask; }
    void   DoneWaitingForValidation() { mInfo &= ~eWaitingForValidationMask; }
    PRBool WaitingForValidation()
    {
        return (mInfo & eWaitingForValidationMask) != 0;
    }

    nsresult
    WaitForValidation(void)
    {
        if (!WaitingForValidation()) {   // flag already cleared
            MarkWaitingForValidation();  // set up for next time
            return NS_OK;                // early exit;
        }

        if (!mLock) {
            mLock = PR_NewLock();
            if (!mLock) return NS_ERROR_OUT_OF_MEMORY;

            NS_ASSERTION(!mCondVar,"we have mCondVar, but didn't have mLock?");
            mCondVar = PR_NewCondVar(mLock);
            if (!mCondVar) {
                PR_DestroyLock(mLock);
                return NS_ERROR_OUT_OF_MEMORY;
            }
        }
        PRStatus status;
        PR_Lock(mLock);
        while (WaitingForValidation() && (status == PR_SUCCESS) ) {
            status = PR_WaitCondVar(mCondVar, PR_INTERVAL_NO_TIMEOUT);
        }
        MarkWaitingForValidation();  // set up for next time
        PR_Unlock(mLock);
        
        NS_ASSERTION(status == PR_SUCCESS, "PR_WaitCondVar() returned PR_FAILURE?");
        if (status == PR_FAILURE)
            return NS_ERROR_UNEXPECTED;

        return NS_OK;
    }

    void WakeUp(void) {
        DoneWaitingForValidation();
        if (mLock) {
        PR_Lock(mLock);
        PR_NotifyCondVar(mCondVar);
        PR_Unlock(mLock);
        }
    }

    /**
     * Data members
     */
    nsCString *                mKey;
    PRUint32                   mInfo;
    nsCOMPtr<nsICacheListener> mListener;
    nsCOMPtr<nsIEventQueue>    mEventQ;
    PRLock *                   mLock;
    PRCondVar *                mCondVar;
};

#endif // _nsCacheRequest_h_
