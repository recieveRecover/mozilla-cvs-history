/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#include "nsTimerMotif.h"

// #include "nsITimerCallback.h"
// #include "nsCRT.h"
// #include "prlog.h"
// #include <stdio.h>
// #include <limits.h>
// #include <X11/Intrinsic.h>

static NS_DEFINE_IID(kITimerIID, NS_ITIMER_IID);

// Hack for now. This is Bad because it creates a dependency between the widget
// library and this library. This needs to be replaced with having code 
// to pass an interface which can be queried for the app context.
XtAppContext gAppContext;

extern void nsTimerExpired(XtPointer aCallData);


void nsTimerMotif::FireTimeout()
{
  if (mFunc != NULL) {
    (*mFunc)(this, mClosure);
  }
  else if (mCallback != NULL) {
    mCallback->Notify(this); // Fire the timer
  }

// Always repeating here

// if (mRepeat)
//  mTimerId = XtAppAddTimeOut(gAppContext, GetDelay(),(XtTimerCallbackProc)nsTimerExpired, this);
}


nsTimerMotif::nsTimerMotif()
{
  NS_INIT_REFCNT();
  mFunc = NULL;
  mCallback = NULL;
  mNext = NULL;
  mTimerId = 0;
  mDelay = 0;
  mClosure = NULL;
}

nsTimerMotif::~nsTimerMotif()
{
}

nsresult 
nsTimerMotif::Init(nsTimerCallbackFunc aFunc,
                void *aClosure,
//              PRBool aRepeat, 
                PRUint32 aDelay)
{
    mFunc = aFunc;
    mClosure = aClosure;
    // mRepeat = aRepeat;

    mTimerId = XtAppAddTimeOut(gAppContext, aDelay,(XtTimerCallbackProc)nsTimerExpired, this);

    return Init(aDelay);
}

nsresult 
nsTimerMotif::Init(nsITimerCallback *aCallback,
//              PRBool aRepeat, 
                PRUint32 aDelay)
{
    mCallback = aCallback;
    // mRepeat = aRepeat;

    mTimerId = XtAppAddTimeOut(gAppContext, aDelay, (XtTimerCallbackProc)nsTimerExpired, this);

    return Init(aDelay);
}

nsresult
nsTimerMotif::Init(PRUint32 aDelay)
{
    mDelay = aDelay;
    NS_ADDREF(this);

    return NS_OK;
}

NS_IMPL_ISUPPORTS(nsTimerMotif, kITimerIID)


void
nsTimerMotif::Cancel()
{
  XtRemoveTimeOut(mTimerId);
}

void nsTimerExpired(XtPointer aCallData)
{
  nsTimerMotif* timer = (nsTimerMotif *)aCallData;
  timer->FireTimeout();
}

nsresult NS_NewTimer(nsITimer** aInstancePtrResult)
{
    NS_PRECONDITION(nsnull != aInstancePtrResult, "null ptr");
    if (nsnull == aInstancePtrResult) {
      return NS_ERROR_NULL_POINTER;
    }  

    nsTimerMotif *timer = new nsTimerMotif();
    if (nsnull == timer) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    return timer->QueryInterface(kITimerIID, (void **) aInstancePtrResult);
}
