/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1999 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#ifndef _nsMsgThread_H
#define _nsMsgThread_H

#include "nsIMsgThread.h"
#include "nsString.h"
#include "MailNewsTypes.h"
#include "mdb.h"

class nsIMdbTable;
class nsIMsgDBHdr;
class nsMsgDatabase;

class nsMsgThread : public nsIMsgThread {
public:
	nsMsgThread();
	nsMsgThread(nsMsgDatabase *db, nsIMdbTable *table);
	virtual ~nsMsgThread();

	friend class nsMsgThreadEnumerator;

    NS_DECL_ISUPPORTS
    NS_DECL_NSIMSGTHREAD

	// non-interface methods
	PRBool TryReferenceThreading(nsIMsgDBHdr *newHeader);
    nsIMdbTable		*GetMDBTable() {return m_mdbTable;}
	nsIMdbRow		*GetMetaRow() {return m_metaRow;}
	nsMsgDatabase	*m_mdbDB ;

protected:

	void				Init();
	virtual nsresult	InitCachedValues();
	nsresult			ChangeChildCount(PRInt32 delta);
	nsresult			ChangeUnreadChildCount(PRInt32 delta);
	nsresult			RemoveChild(nsMsgKey msgKey);
	nsresult			SetThreadRootKey(nsMsgKey threadRootKey);
	nsresult			GetChildHdrForKey(nsMsgKey desiredKey, 
												nsIMsgDBHdr **result, PRInt32 *resultIndex); 
	nsresult			ReparentChildrenOf(nsMsgKey oldParent, nsMsgKey newParent, nsIDBChangeAnnouncer *announcer);

	nsMsgKey		m_threadKey; 
	PRUint32		m_numChildren;		
	PRUint32		m_numUnreadChildren;	
	PRUint32		m_flags;
    nsIMdbTable		*m_mdbTable;
	nsIMdbRow		*m_metaRow;
	PRBool			m_cachedValuesInitialized;
	nsMsgKey		m_threadRootKey;
};

#endif

