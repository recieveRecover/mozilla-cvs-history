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
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#include "nsListBox.h"

NS_IMPL_ADDREF(nsListBox)
NS_IMPL_RELEASE(nsListBox)

NS_METHOD nsListBox::SetMultipleSelection(PRBool aMultipleSelections) 
{
  return NS_OK;
}

NS_METHOD nsListBox::PreCreateWidget(nsWidgetInitData *aInitData)
{
  return NS_OK;
}

NS_METHOD nsListBox::AddItemAt(nsString &aItem, PRInt32 aPosition)
{
  return NS_OK;
}

PRInt32  nsListBox::FindItem(nsString &aItem, PRInt32 aStartPos)
{
  return 0;
}

PRInt32  nsListBox::GetItemCount()
{
  return 0;
}

PRBool  nsListBox::RemoveItemAt(PRInt32 aPosition)
{
  return PR_FALSE;
}

PRBool nsListBox::GetItemAt(nsString& anItem, PRInt32 aPosition)
{
  return PR_FALSE;
}

NS_METHOD nsListBox::GetSelectedItem(nsString& aItem)
{
  return NS_OK;
}

PRInt32 nsListBox::GetSelectedIndex()
{
  return 0;
}

NS_METHOD nsListBox::SelectItem(PRInt32 aPosition)
{
  return NS_OK;
}

PRInt32 nsListBox::GetSelectedCount()
{
  return 0;
}

NS_METHOD nsListBox::GetSelectedIndices(PRInt32 aIndices[], PRInt32 aSize)
{
  return NS_OK;
}

NS_METHOD nsListBox::SetSelectedIndices(PRInt32 aIndices[], PRInt32 aSize)
{
  return NS_OK;
}

NS_METHOD nsListBox::Deselect()
{
  return NS_OK;
}

nsListBox::nsListBox() : nsWidget(), nsIListWidget(), nsIListBox()
{
  NS_INIT_REFCNT();
}

nsListBox::~nsListBox()
{
}


nsresult nsListBox::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  nsresult result = nsWidget::QueryInterface(aIID, aInstancePtr);
  
  static NS_DEFINE_IID(kInsListBoxIID, NS_ILISTBOX_IID);
  static NS_DEFINE_IID(kInsListWidgetIID, NS_ILISTWIDGET_IID);
  if (result == NS_NOINTERFACE) {
    if (aIID.Equals(kInsListBoxIID)) {
      *aInstancePtr = (void*) ((nsIListBox*)this);
      NS_ADDREF_THIS();
      result = NS_OK;
    }
    else if (aIID.Equals(kInsListWidgetIID)) {
      *aInstancePtr = (void*) ((nsIListWidget*)this);
      NS_ADDREF_THIS();
      result = NS_OK;
    }
  }
  return result;
}

PRBool nsListBox::AutoErase()
{
  return(PR_TRUE);
}


