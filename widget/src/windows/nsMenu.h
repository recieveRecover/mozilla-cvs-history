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

#ifndef nsMenu_h__
#define nsMenu_h__

#include "nsdefs.h"
#include "nsWindow.h"
#include "nsSwitchToUIThread.h"

#include "nsIMenu.h"

/**
 * Native Win32 button wrapper
 */

class nsMenu :  public nsWindow,
                public nsIMenu
{

public:
  nsMenu();
  virtual ~nsMenu();

  // nsISupports
  NS_IMETHOD_(nsrefcnt) AddRef();
  NS_IMETHOD_(nsrefcnt) Release();
  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);
  
  NS_IMETHOD   Create(nsIWidget *aParent,
                         const nsRect &aRect,
                         EVENT_CALLBACK aHandleEventFunction,
                         nsIDeviceContext *aContext,
                         nsIAppShell *aAppShell = nsnull,
                         nsIToolkit *aToolkit = nsnull,
                         nsWidgetInitData *aInitData = nsnull);
  NS_IMETHOD   Create(nsNativeWidget aParent,
                         const nsRect &aRect,
                         EVENT_CALLBACK aHandleEventFunction,
                         nsIDeviceContext *aContext,
                         nsIAppShell *aAppShell = nsnull,
                         nsIToolkit *aToolkit = nsnull,
                         nsWidgetInitData *aInitData = nsnull);

  // nsIMenu Methods
  NS_IMETHOD SetLabel(const nsString &aText);
  NS_IMETHOD GetLabel(nsString &aText);
  NS_IMETHOD AddItem(const nsString &aText);
  NS_IMETHOD AddItem(nsIMenuItem * aMenuItem);
  NS_IMETHOD AddMenu(nsIMenu * aMenu);
  NS_IMETHOD AddSeparator();
  NS_IMETHOD GetItemCount(PRUint32 &aCount);
  NS_IMETHOD GetItemAt(const PRUint32 aCount, nsIMenuItem *& aMenuItem);
  NS_IMETHOD InsertItemAt(const PRUint32 aCount, nsIMenuItem *& aMenuItem);
  NS_IMETHOD InsertItemAt(const PRUint32 aCount, const nsString & aMenuItemName);
  NS_IMETHOD InsertSeparator(const PRUint32 aCount);
  NS_IMETHOD RemoveItem(const PRUint32 aCount);
  NS_IMETHOD RemoveAll();

  virtual PRBool OnMove(PRInt32 aX, PRInt32 aY);
  virtual PRBool OnPaint();
  virtual PRBool OnResize(nsRect &aWindowRect);

protected:
  nsString mLabel;
  PRUint32 mNumMenuItems;

};

#endif // nsMenu_h__
