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
 * Copyright (C) 2001 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *   Johnny Stenback <jst@netscape.com> (original author)
 */
#ifndef nsDOMScriptableHelper_h___
#define nsDOMScriptableHelper_h___

#include "nsIXPCScriptable.h"

class nsDOMScriptableHelper : public nsIXPCScriptable
{
public:
  // nsISupports
  NS_DECL_ISUPPORTS

  // nsIXPCScriptable
  XPC_DECLARE_IXPCSCRIPTABLE

  nsDOMScriptableHelper();
  virtual ~nsDOMScriptableHelper();

  static nsresult GetDefaultHelper(void **aHelper);
  static nsresult GetHTMLDocHelper(void **aHelper);
  static nsresult GetHTMLFormHelper(void **aHelper);

protected:
  PRBool DidDefineStaticJSIds()
  {
    return sLocation_id;
  }

  nsresult DefineStaticJSIds(JSContext *cx);
  static nsresult GetHelperBody(nsDOMScriptableHelper *aHelper,
                                nsIXPCScriptable **aStaticHolder);

  static jsid sLocation_id;

  static nsIXPConnect* sXPConnect;

  static PRUint32 sInstanceCount;
};

#endif /* nsDOMScriptableHelper_h___ */
