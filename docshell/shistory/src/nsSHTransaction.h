/* -*- Mode: IDL; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * The Original Code is the Mozilla browser.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications, Inc.  Portions created by Netscape are
 * Copyright (C) 1999, Mozilla.  All Rights Reserved.
 * 
 * Contributor(s):
 *   Radha Kulkarni <radha@netscape.com>
 */

#ifndef nsSHTransaction_h
#define nsSHTransaction_h

#include "nsISHTransaction.h"
#include "nsISHEntry.h"

class nsSHTransaction: public nsISHTransaction
{
public:
	NS_DECL_ISUPPORTS
	NS_DECL_NSISHTRANSACTION

	nsSHTransaction();

protected:
	virtual ~nsSHTransaction();


private:
    friend NS_IMETHODIMP
		NS_NewSHTransaction(nsISupports * aOuter, REFNSIID aIID, void** aResult);
	nsresult SetChild(nsISHTransaction * aChild);
	nsresult SetParent(nsISHTransaction * aParent);
	nsresult SetCurrentSHistoryEntry(nsISHEntry * aSHEntry);

    nsISHEntry *  mSHEntry;
	/* Weak reference to parent */
	nsISHTransaction * mParent;
	nsISHTransaction * mChild;
	nsISHTransaction * mLRVList;
};


#endif   /* nsSHTransaction_h */
