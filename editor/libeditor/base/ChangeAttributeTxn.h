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

#ifndef ChangeAttributeTxn_h__
#define ChangeAttributeTxn_h__

#include "EditTxn.h"
#include "nsCOMPtr.h"
#include "nsIDOMElement.h"
#include "nsIEditor.h"

#define CHANGE_ATTRIBUTE_TXN_IID \
{/* 97818860-ac48-11d2-86d8-000064657374 */ \
0x97818860, 0xac48, 0x11d2, \
{0x86, 0xd8, 0x0, 0x0, 0x64, 0x65, 0x73, 0x74} }

/**
 * A transaction that changes an attribute of a content node. 
 * This transaction covers add, remove, and change attribute.
 */
class ChangeAttributeTxn : public EditTxn
{
public:

  /** Initialize the transaction.
    * @param aEditor the object providing core editing operations
    * @param aNode   the node whose attribute will be changed
    * @param aAttribute the name of the attribute to change
    * @param aValue     the new value for aAttribute, if aRemoveAttribute is false
    * @param aRemoveAttribute if PR_TRUE, remove aAttribute from aNode
    */
  NS_IMETHOD Init(nsIEditor      *aEditor,
                  nsIDOMElement  *aNode,
                  const nsString& aAttribute,
                  const nsString& aValue,
                  PRBool aRemoveAttribute);

private:
  ChangeAttributeTxn();

public:

  NS_IMETHOD Do(void);

  NS_IMETHOD Undo(void);

  NS_IMETHOD Redo(void);

  NS_IMETHOD Merge(PRBool *aDidMerge, nsITransaction *aTransaction);

  NS_IMETHOD Write(nsIOutputStream *aOutputStream);

  NS_IMETHOD GetUndoString(nsString **aString);

  NS_IMETHOD GetRedoString(nsString **aString);

protected:

  /** the editor that created this transaction */
  nsCOMPtr<nsIEditor> mEditor;
  
  /** the element to operate upon */
  nsCOMPtr<nsIDOMElement> mElement;
  
  /** the attribute to change */
  nsString mAttribute;

  /** the value to set the attribute to (ignored if mRemoveAttribute==PR_TRUE) */
  nsString mValue;

  /** the value to set the attribute to for undo */
  nsString mUndoValue;

  /** PR_TRUE if the mAttribute was set on mElement at the time of execution */
  PRBool   mAttributeWasSet;

  /** PR_TRUE if the operation is to remove mAttribute from mElement */
  PRBool   mRemoveAttribute;

  friend class TransactionFactory;
};

#endif
