/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#ifndef _nsIMsgFilterService_H_
#define _nsIMsgFilterService_H_

// The filter service is used to acquire and manipulate filter lists.
#define NS_IMSGFILTERSERVICE_IID                         \
{ 0x5cbb0700, 0x04bc, 0x11d3,                 \
    { 0xa5, 0x0a, 0x0, 0x60, 0xb0, 0xfc, 0x04, 0xb7 } }

// 5cbb0700-04bc-11d3-a50a-0060b0fc04b7

#include "nsISupports.h"

class nsIMsgFilterList;

class nsIMsgFilterService : public nsISupports
{

public:
    static const nsIID& GetIID() { static nsIID iid = NS_IMSGFILTERSERVICE_IID; return iid; }

/* clients call OpenFilterList to get a handle to a FilterList, of existing nsMsgFilter *.
	These are manipulated by the front end as a result of user interaction
   with dialog boxes. To apply the new list call MSG_CloseFilterList.

*/
	NS_IMETHOD OpenFilterList(nsFileSpec *filterFile, nsIMsgFilterList **filterList) = 0;
	NS_IMETHOD CloseFilterList(nsIMsgFilterList *filterList) = 0;
	NS_IMETHOD	SaveFilterList(nsIMsgFilterList *filterList) = 0;	/* save without deleting */
	NS_IMETHOD CancelFilterList(nsIMsgFilterList *filterList) = 0;

};

#endif  // _nsIMsgFilterService_H_

