/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Communicator client code, 
 * released March 31, 1998. 
 *
 * The Initial Developer of the Original Code is Netscape Communications 
 * Corporation.  Portions created by Netscape are 
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 *
 * Contributors:
 *     Douglas Turner <dougt@netscape.com>
 *     Daniel Veditz <dveditz@netscape.com>
 */

#ifndef nsTopProgressNotifier_h__
#define nsTopProgressNotifier_h__

#include "nsIXPINotifier.h"
#include "nsVector.h"


class nsTopProgressNotifier : public nsIXPINotifier
{
    public:

        nsTopProgressNotifier();
        virtual ~nsTopProgressNotifier();

        long RegisterNotifier(nsIXPINotifier * newNotifier);
        void UnregisterNotifier(long id);
        void SetActiveNotifier(nsIXPINotifier *aNotifier) 
            { mActive = aNotifier; }

        NS_DECL_ISUPPORTS

        // implements nsIXPINotifier
        NS_IMETHOD BeforeJavascriptEvaluation(const PRUnichar *URL);
        NS_IMETHOD AfterJavascriptEvaluation(const PRUnichar *URL);
        NS_IMETHOD InstallStarted(const PRUnichar* URL, const PRUnichar* UIPackageName);
        NS_IMETHOD ItemScheduled(const  PRUnichar* message );
        NS_IMETHOD FinalizeProgress(const PRUnichar* message, PRInt32 itemNum, PRInt32 totNum );
        NS_IMETHOD FinalStatus(const PRUnichar* URL, PRInt32 status);
        NS_IMETHOD LogComment(const PRUnichar* comment);
   
   private:
        nsVector        *mNotifiers;
        nsIXPINotifier  *mActive;

};

#endif
