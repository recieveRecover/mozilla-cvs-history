/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#ifndef nsIWin32LocaleImpl_h__
#define nsIWin32LocaleImpl_h__


#include "nsISupports.h"
#include "nscore.h"
#include "nsString.h"
#include "nsILocale.h"
#include "nsIWin32Locale.h"
#include <windows.h>

class nsIWin32LocaleImpl: public nsIWin32Locale
{

	NS_DECL_ISUPPORTS

public:

	nsIWin32LocaleImpl(void);
	~nsIWin32LocaleImpl(void);

	NS_IMETHOD GetPlatformLocale(const nsString* locale,LCID* winLCID);
	NS_IMETHOD GetXPLocale(LCID winLCID,nsString* locale);

protected:
	inline PRBool	ParseLocaleString(const char* locale_string, char* language, char* country, char* region);

};

#endif
