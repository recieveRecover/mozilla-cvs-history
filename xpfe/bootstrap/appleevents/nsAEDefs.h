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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *  Simon Fraser <sfraser@netscape.com>
 */


#ifndef nsAEDefs_h_
#define nsAEDefs_h_


#include <MacTypes.h>


typedef char	CStr255[256];		/* like Str255, except for C-format strings. */
typedef long TAEListIndex;    // a 1-based list index
typedef short TWindowKind;


enum {
	kAnyWindowKind = 99
};


typedef enum
{
	eSaveUnspecified,
	eSaveYes,
	eSaveNo,
	eSaveAsk
} TAskSave;


#if DEBUG

#define ASSERT(x,msg)	{ if (!(x)) { DebugStr("\p"msg); } }

#else

#define ASSERT(x)		((void) 0)

#endif




#endif // nsAEDefs_h_
