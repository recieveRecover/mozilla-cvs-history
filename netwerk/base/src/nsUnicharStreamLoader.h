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

#ifndef nsUnicharStreamLoader_h__
#define nsUnicharStreamLoader_h__

#include "nsIUnicharStreamLoader.h"
#include "nsIStreamListener.h"
#include "nsCOMPtr.h"
#include "nsString.h"

class nsUnicharStreamLoader : public nsIUnicharStreamLoader,
                              public nsIStreamListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIUNICHARSTREAMLOADER
  NS_DECL_NSISTREAMOBSERVER
  NS_DECL_NSISTREAMLISTENER

  nsUnicharStreamLoader();
  virtual ~nsUnicharStreamLoader();

  static NS_METHOD
  Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

protected:
  nsCOMPtr<nsIUnicharStreamLoaderObserver> mObserver;
  nsString* mData;
///  nsCOMPtr<nsILoadGroup> mLoadGroup;
};

#endif // nsUnicharStreamLoader_h__
