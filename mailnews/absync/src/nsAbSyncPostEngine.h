/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
#ifndef nsAbSyncPostEngine_h_
#define nsAbSyncPostEngine_h_

#include "nsCOMPtr.h"
#include "nsIInputStream.h"
#include "nsIStreamListener.h"
#include "nsFileStream.h"
#include "nsIAbSyncPostEngine.h"
#include "nsIInterfaceRequestor.h"
#include "nsCURILoader.h"
#include "nsIURIContentListener.h"
#include "nsIURI.h"
#include "nsIAbSyncMojo.h"
#include "nsIChannel.h"

//
// Callback declarations for URL completion
//
// For completion of send/message creation operations...
//
typedef nsresult (*nsPostCompletionCallback ) (nsresult aStatus,
                                               const char *aContentType,
                                               const char *aCharset,
                                               PRInt32 totalSize, const PRUnichar* aMsg, 
                                               void *tagData);

class nsAbSyncPostEngine : public nsIAbSyncPostEngine, public nsIStreamListener, public nsIURIContentListener, public nsIInterfaceRequestor { 
public: 
  nsAbSyncPostEngine();
  virtual ~nsAbSyncPostEngine();

  /* this macro defines QueryInterface, AddRef and Release for this class */
  NS_DECL_ISUPPORTS
  NS_DECL_NSIABSYNCPOSTENGINE

  // 
  // This is the output stream where the stream converter will write processed data after 
  // conversion. 
  // 
  NS_IMETHOD StillRunning(PRBool *running);

  NS_IMETHOD FireURLRequest(nsIURI *aURL, const char *postData);

  NS_IMETHOD KickTheSyncOperation();

  static NS_METHOD    Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

  // Methods for nsIStreamListener
  NS_DECL_NSISTREAMLISTENER
  // Methods for nsIStreamObserver
  NS_DECL_NSISTREAMOBSERVER
  NS_DECL_NSIURICONTENTLISTENER
  NS_DECL_NSIINTERFACEREQUESTOR

  PRInt32                         mPostEngineState;
  PRInt32                         mTransactionID;

private:
  // Handy methods for listeners...
  nsresult        DeleteListeners();
  nsresult        NotifyListenersOnStartAuthOperation(void);
  nsresult        NotifyListenersOnStopAuthOperation(nsresult aStatus, const PRUnichar *aMsg, const char *aCookie);
  nsresult        NotifyListenersOnStartSending(PRInt32 aTransactionID, PRUint32 aMsgSize);
  nsresult        NotifyListenersOnProgress(PRInt32 aTransactionID, PRUint32 aProgress, PRUint32 aProgressMax);
  nsresult        NotifyListenersOnStatus(PRInt32 aTransactionID, PRUnichar *aMsg);
  nsresult        NotifyListenersOnStopSending(PRInt32 aTransactionID, nsresult aStatus, 
                                               const PRUnichar *aMsg, char *aProtocolResponse);

  PRBool                          mStillRunning;  // Are we still running?

  PRInt32                         mTotalWritten;      // Size counter variable
  nsString                        mProtocolResponse;  // Protocol response

  char                            *mContentType;  // The content type retrieved from the server
  char                            *mCharset;      // The charset retrieved from the server

  nsCOMPtr<nsISupports>           mLoadCookie;    // load cookie used by the uri loader when we post the url
  char                            *mCookie;
  char                            *mUser;
  char                            *mAuthSpec;

  PRInt32                         mMessageSize;   // Size of POST request...

  nsIAbSyncPostListener           **mListenerArray;
  PRInt32                         mListenerArrayCount;

  // Since we need to do authentication a bit differently, do it here!
  PRBool                          mAuthenticationRunning;
  nsCOMPtr<nsIAbSyncMojo>         mSyncMojo;
  nsCOMPtr<nsIChannel>            mChannel;

  char                            *mSyncProtocolRequest;
  char                            *mSyncProtocolRequestPrefix;

  char                            *mMojoSyncSpec;
  PRInt32                         mMojoSyncPort;
}; 

#endif /* nsAbSyncPostEngine_h_ */
