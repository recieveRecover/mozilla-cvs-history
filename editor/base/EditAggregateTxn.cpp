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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998-1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#include "EditAggregateTxn.h"
#include "nsCOMPtr.h"
#include "nsIDOMNode.h"
#include "nsVoidArray.h"

EditAggregateTxn::EditAggregateTxn()
  : EditTxn()
{
  // base class does this: NS_INIT_REFCNT();
  nsresult res = NS_NewISupportsArray(getter_AddRefs(mChildren));
  NS_POSTCONDITION(NS_SUCCEEDED(res), "EditAggregateTxn failed in constructor");
  SetTransactionDescriptionID( kTransactionID );
  /* log description initialized in parent constructor */
}

EditAggregateTxn::~EditAggregateTxn()
{
  // nsISupportsArray cleans up array for us at destruct time
}

NS_IMETHODIMP EditAggregateTxn::Do(void)
{
  nsresult result=NS_OK;  // it's legal (but not very useful) to have an empty child list
  if (mChildren)
  {
    PRInt32 i;
    PRUint32 count;
    mChildren->Count(&count);
    for (i=0; i<((PRInt32)count); i++)
    {
      nsCOMPtr<nsISupports> isupports = (dont_AddRef)(mChildren->ElementAt(i));
      nsCOMPtr<nsITransaction> txn ( do_QueryInterface(isupports) );
      if (!txn) { return NS_ERROR_NULL_POINTER; }
      result = txn->Do();
      if (NS_FAILED(result))
        break;
    }  
  }
  return result;
}

NS_IMETHODIMP EditAggregateTxn::Undo(void)
{
  nsresult result=NS_OK;  // it's legal (but not very useful) to have an empty child list
  if (mChildren)
  {
    PRInt32 i;
    PRUint32 count;
    mChildren->Count(&count);
    // undo goes through children backwards
    for (i=count-1; i>=0; i--)
    {
      nsCOMPtr<nsISupports> isupports = (dont_AddRef)(mChildren->ElementAt(i));
      nsCOMPtr<nsITransaction> txn ( do_QueryInterface(isupports) );
      if (!txn) { return NS_ERROR_NULL_POINTER; }
      result = txn->Undo();
      if (NS_FAILED(result))
        break;
    }  
  }
  return result;
}

NS_IMETHODIMP EditAggregateTxn::Redo(void)
{
  nsresult result=NS_OK;  // it's legal (but not very useful) to have an empty child list
  if (mChildren)
  {
    PRInt32 i;
    PRUint32 count;
    mChildren->Count(&count);
    for (i=0; i<((PRInt32)count); i++)
    {
      nsCOMPtr<nsISupports> isupports = (dont_AddRef)(mChildren->ElementAt(i));
      nsCOMPtr<nsITransaction> txn ( do_QueryInterface(isupports) );
      if (!txn) { return NS_ERROR_NULL_POINTER; }
      result = txn->Redo();
      if (NS_FAILED(result))
        break;
    }  
  }
  return result;
}

NS_IMETHODIMP EditAggregateTxn::GetIsTransient(PRBool *aIsTransient)
{
  if (nsnull!=aIsTransient)
    *aIsTransient = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP EditAggregateTxn::Merge(PRBool *aDidMerge, nsITransaction *aTransaction)
{
  nsresult result=NS_OK;  // it's legal (but not very useful) to have an empty child list
  if (nsnull!=aDidMerge)
    *aDidMerge=PR_FALSE;
  if (mChildren)
  {
    PRInt32 i=0;
    PRUint32 count;
    mChildren->Count(&count);
    NS_ASSERTION(count>0, "bad count");
    if (0<count)
    {
      nsCOMPtr<nsISupports> isupports = (dont_AddRef)(mChildren->ElementAt(i));
      nsCOMPtr<nsITransaction> txn ( do_QueryInterface(isupports) );
      if (!txn) { return NS_ERROR_NULL_POINTER; }
      result = txn->Merge(aDidMerge, aTransaction);
    }
  }
  return result;

}

NS_IMETHODIMP EditAggregateTxn::Write(nsIOutputStream *aOutputStream)
{
  return NS_OK;
}

NS_IMETHODIMP EditAggregateTxn::GetUndoString(nsString *aString)
{
  if (nsnull!=aString)
    *aString="";
  return NS_OK;
}

NS_IMETHODIMP EditAggregateTxn::GetRedoString(nsString *aString)
{
  if (nsnull!=aString)
    *aString="";
  return NS_OK;
}

NS_IMETHODIMP EditAggregateTxn::AppendChild(EditTxn *aTxn)
{
  if (mChildren && aTxn)
  {
    // aaahhhh! broken interfaces drive me crazy!!!
    nsCOMPtr<nsISupports> isupports;
    aTxn->QueryInterface(NS_GET_IID(nsISupports), getter_AddRefs(isupports));
    mChildren->AppendElement(isupports);
    return NS_OK;
  }
  return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP EditAggregateTxn::GetCount(PRUint32 *aCount)
{
  if (!aCount) {
    return NS_ERROR_NULL_POINTER;
  }
  *aCount=0;
  if (mChildren) {
    mChildren->Count(aCount);
  }
  return NS_OK;
}

NS_IMETHODIMP EditAggregateTxn::GetTxnAt(PRInt32 aIndex, EditTxn **aTxn)
{
  // preconditions
  NS_PRECONDITION(aTxn, "null out param");
  NS_PRECONDITION(mChildren, "bad internal state");

  if (!aTxn) {
    return NS_ERROR_NULL_POINTER;
  }
  *aTxn = nsnull; // initialize out param as soon as we know it's a valid pointer
  if (!mChildren) {
    return NS_ERROR_UNEXPECTED;
  }

  // get the transaction at aIndex
  PRUint32 txnCount;
  mChildren->Count(&txnCount);
  if (0>aIndex || ((PRInt32)txnCount)<=aIndex) {
    return NS_ERROR_UNEXPECTED;
  }
  nsCOMPtr<nsISupports> isupports = (dont_AddRef)(mChildren->ElementAt(aIndex));
  // ugh, this is all wrong - what a mess we have with editor transaction interfaces
  isupports->QueryInterface(EditTxn::GetCID(), (void**)aTxn);
  if (!*aTxn)
    return NS_ERROR_UNEXPECTED;
  return NS_OK;
}


NS_IMETHODIMP EditAggregateTxn::SetName(nsIAtom *aName)
{
  mName = do_QueryInterface(aName);
  return NS_OK;
}

NS_IMETHODIMP EditAggregateTxn::GetName(nsIAtom **aName)
{
  if (aName)
  {
    if (mName)
    {
      *aName = mName;
      NS_ADDREF(*aName);
      return NS_OK;
    }
  }
  return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP_(nsrefcnt) EditAggregateTxn::AddRef(void)
{
  return EditTxn::AddRef();
}

//NS_IMPL_RELEASE_INHERITED(Class, Super)
NS_IMETHODIMP_(nsrefcnt) EditAggregateTxn::Release(void)
{
  return EditTxn::Release();
}

//NS_IMPL_QUERY_INTERFACE_INHERITED(Class, Super, AdditionalInterface)
NS_IMETHODIMP EditAggregateTxn::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (!aInstancePtr) return NS_ERROR_NULL_POINTER;
 
  if (aIID.Equals(EditAggregateTxn::GetCID())) {
    *aInstancePtr = NS_STATIC_CAST(EditAggregateTxn*, this);
//    *aInstancePtr = (nsISupports*)(EditAggregateTxn*)(this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return EditTxn::QueryInterface(aIID, aInstancePtr);
}






