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
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#ifndef nsINntpUrl_h___
#define nsINntpUrl_h___

#include "nscore.h"
#include "nsIURL.h"

#include "nsISupports.h"

/* include all of our event sink interfaces */
#include "nsINNTPNewsgroupList.h"
#include "nsINNTPArticleList.h"
#include "nsINNTPHost.h"
#include "nsINNTPNewsgroup.h"
#include "nsINNTPNewsgroupPost.h"
#include "nsIMsgOfflineNewsState.h"

/* BDD12930-A682-11d2-804C-006008128C4E */

#define NS_INNTPURL_IID                         \
{ 0xbdd12930, 0xa682, 0x11d2,                   \
    { 0x80, 0x4c, 0x0, 0x60, 0x8, 0x12, 0x8c, 0x4e } }

class nsINntpUrl : public nsIURL
{
public:
    static const nsIID& IID() { static nsIID iid = NS_INNTPURL_IID; return iid; }

	/////////////////////////////////////////////////////////////////////////////// 
	// Getters and Setters for the news specific event sinks to bind to to your url
	///////////////////////////////////////////////////////////////////////////////

	NS_IMETHOD SetNntpHost (nsINNTPHost * newsHost) = 0;
	NS_IMETHOD GetNntpHost (nsINNTPHost ** newsHost) const = 0;

	NS_IMETHOD SetNntpArticleList (nsINNTPArticleList * articleList) = 0;
	NS_IMETHOD GetNntpArticleList (nsINNTPArticleList ** articleList) const = 0;

	NS_IMETHOD SetNewsgroup (nsINNTPNewsgroup * newsgroup) = 0;
	NS_IMETHOD GetNewsgroup (nsINNTPNewsgroup ** newsgroup) const = 0;

	NS_IMETHOD SetOfflineNewsState (nsIMsgOfflineNewsState * offlineNews) = 0;
	NS_IMETHOD GetOfflineNewsState (nsIMsgOfflineNewsState ** offlineNews) const = 0;

	NS_IMETHOD SetNewsgroupList (nsINNTPNewsgroupList * xoverParser) = 0;
	NS_IMETHOD GetNewsgroupList (nsINNTPNewsgroupList ** xoverParser) const = 0;

	// mscott: this interface really belongs in nsIURL and I will move it there after talking
	// it over with core netlib. This error message replaces the err_msg which was in the 
	// old URL_struct. Also, it should probably be a nsString or a PRUnichar *. I don't know what
	// XP_GetString is going to return in mozilla. 

	NS_IMETHOD SetErrorMessage (char * errorMessage) = 0;
	// caller must free using PR_FREE
	NS_IMETHOD GetErrorMessage (char ** errorMessage) const = 0;

    NS_IMETHOD SetMessageToPost (nsINNTPNewsgroupPost *post) = 0;
    NS_IMETHOD GetMessageToPost (nsINNTPNewsgroupPost **post) = 0;

};

#endif /* nsIHttpURL_h___ */
