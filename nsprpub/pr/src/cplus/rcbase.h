/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* 
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
 * The Original Code is the Netscape security libraries.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are 
 * Copyright (C) 1998-2000 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s):
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL"), in which case the provisions of the GPL are applicable 
 * instead of those above.  If you wish to allow use of your 
 * version of this file only under the terms of the GPL and not to
 * allow others to use your version of this file under the MPL,
 * indicate your decision by deleting the provisions above and
 * replace them with the notice and other provisions required by
 * the GPL.  If you do not delete the provisions above, a recipient
 * may use your version of this file under either the MPL or the
 * GPL.
 */

/*
** RCBase.h - Mixin class for NSPR C++ wrappers
*/

#if defined(_RCRUNTIME_H)
#else
#define _RCRUNTIME_H

#include <prerror.h>

/*
** Class: RCBase (mixin)
**
** Generally mixed into every base class. The functions in this class are all
** static. Therefore this entire class is just syntatic sugar. It gives the
** illusion that errors (in particular) are retrieved via the same object
** that just reported a failure. It also (unfortunately) might lead one to
** believe that the errors are persistent in that object. They're not.
*/

class PR_IMPLEMENT(RCBase)
{
public:
    virtual ~RCBase();

    static void AbortSelf();

    static PRErrorCode GetError();
    static PRInt32 GetOSError();

    static PRSize GetErrorTextLength();
    static PRSize CopyErrorText(char *text);

    static void SetError(PRErrorCode error, PRInt32 oserror);
    static void SetErrorText(PRSize textLength, const char *text);

protected:
    RCBase() { }
};  /* RCObject */

inline PRErrorCode RCBase::GetError() { return PR_GetError(); }
inline PRInt32 RCBase::GetOSError() { return PR_GetOSError(); }

#endif  /* defined(_RCRUNTIME_H) */

/* rcbase.h */
