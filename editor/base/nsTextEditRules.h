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

#ifndef nsTextEditRules_h__
#define nsTextEditRules_h__

#include "nsIEditRules.h"
#include "nsIEditor.h"
#include "nsCOMPtr.h"


/** Object that encapsulates HTML text-specific editing rules.
  *  
  * To be a good citizen, edit rules must live by these restrictions:
  * 1. All data manipulation is through the editor.  
  *    Content nodes in the document tree must <B>not</B> be manipulated directly.
  *    Content nodes in document fragments that are not part of the document itself
  *    may be manipulated at will.  Operations on document fragments must <B>not</B>
  *    go through the editor.
  * 2. Selection must not be explicitly set by the rule method.  
  *    Any manipulation of Selection must be done by the editor.
  */
class nsTextEditRules  : public nsIEditRules
{
public:

  NS_DECL_ISUPPORTS

  nsTextEditRules();
  virtual ~nsTextEditRules();

  NS_IMETHOD Init(nsIEditor *aEditor, nsIEditRules *aNextRule);

  NS_IMETHOD WillInsertBreak(nsIDOMSelection *aSelection, PRBool *aCancel);
  NS_IMETHOD DidInsertBreak(nsIDOMSelection *aSelection, nsresult aResult);
  NS_IMETHOD GetInsertBreakTag(nsIAtom **aTag);

protected:

  nsCOMPtr<nsIEditor> mEditor;
  nsCOMPtr<nsIEditRules> mNextRule;  

};

#endif //nsTextEditRules_h__

