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
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */



#include "nsCOMPtr.h"
#include "nsVoidArray.h"

#include "nsIContentViewer.h"
#include "nsIDocumentViewer.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMXULDocument.h"
#include "nsIDiskDocument.h"
#include "nsIDOMElement.h"
#include "nsIDOMSelection.h"
#include "nsIDOMAttr.h"
#include "nsIDOMXULCommandDispatcher.h"
#include "nsIScriptGlobalObject.h"
#include "nsIDOMWindow.h"

#include "nsIEditor.h"
#include "nsIHTMLEditor.h"
#include "nsITransactionManager.h"

#include "nsInterfaceState.h"



nsInterfaceState::nsInterfaceState()
:  mEditor(nsnull)
,  mChromeDoc(nsnull)
,  mDOMWindow(nsnull)
,  mBoldState(eStateUninitialized)
,  mItalicState(eStateUninitialized)
,  mUnderlineState(eStateUninitialized)
,  mDirtyState(eStateUninitialized)
,  mSelectionCollapsed(eStateUninitialized)
,  mFirstDoOfFirstUndo(PR_TRUE)
{
	NS_INIT_REFCNT();
}

nsInterfaceState::~nsInterfaceState()
{
}

NS_IMPL_ADDREF(nsInterfaceState);
NS_IMPL_RELEASE(nsInterfaceState);
NS_IMPL_QUERY_INTERFACE3(nsInterfaceState, nsIDOMSelectionListener, nsIDocumentStateListener, nsITransactionListener);

NS_IMETHODIMP
nsInterfaceState::Init(nsIHTMLEditor* aEditor, nsIDOMXULDocument *aChromeDoc)
{
  if (!aEditor)
    return NS_ERROR_INVALID_ARG;

  if (!aChromeDoc)
    return NS_ERROR_INVALID_ARG;

  mEditor = aEditor;		// no addreffing here
  mChromeDoc = aChromeDoc;
  return NS_OK;
}

NS_IMETHODIMP
nsInterfaceState::NotifyDocumentCreated()
{
  return NS_OK;
}

NS_IMETHODIMP
nsInterfaceState::TableCellNotification(nsIDOMNode* aNode, PRInt32 aOffset)
{
  //stub
  return NS_OK;
}

NS_IMETHODIMP
nsInterfaceState::NotifyDocumentWillBeDestroyed()
{
  return NS_OK;
}


NS_IMETHODIMP
nsInterfaceState::NotifyDocumentStateChanged(PRBool aNowDirty)
{
  // update document modified. We should have some other notifications for this too.
  return UpdateDirtyState(aNowDirty);
}

NS_IMETHODIMP
nsInterfaceState::NotifySelectionChanged()
{
  // if the selection state has changed, update stuff
  PRBool isCollapsed = SelectionIsCollapsed();
  if (isCollapsed != mSelectionCollapsed)
  {
    CallUpdateCommands(nsAutoString("select"));
    mSelectionCollapsed = isCollapsed;
  }
  
  return ForceUpdate();
}


NS_IMETHODIMP nsInterfaceState::WillDo(nsITransactionManager *aManager,
  nsITransaction *aTransaction, PRBool *aInterrupt)
{
  *aInterrupt = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP nsInterfaceState::DidDo(nsITransactionManager *aManager,
  nsITransaction *aTransaction, nsresult aDoResult)
{
  // only need to update if the status of the Undo menu item changes.
  PRInt32 undoCount;
  aManager->GetNumberOfUndoItems(&undoCount);
  if (undoCount == 1)
  {
    if (mFirstDoOfFirstUndo)
      CallUpdateCommands(nsAutoString("undo"));
    mFirstDoOfFirstUndo = PR_FALSE;
  }

  return NS_OK;
}

NS_IMETHODIMP nsInterfaceState::WillUndo(nsITransactionManager *aManager,
  nsITransaction *aTransaction, PRBool *aInterrupt)
{
  *aInterrupt = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP nsInterfaceState::DidUndo(nsITransactionManager *aManager,
  nsITransaction *aTransaction, nsresult aUndoResult)
{
  PRInt32 undoCount;
  aManager->GetNumberOfUndoItems(&undoCount);
  if (undoCount == 0)
    mFirstDoOfFirstUndo = PR_TRUE;    // reset the state for the next do

  CallUpdateCommands(nsAutoString("undo"));
  return NS_OK;
}

NS_IMETHODIMP nsInterfaceState::WillRedo(nsITransactionManager *aManager,
  nsITransaction *aTransaction, PRBool *aInterrupt)
{
  *aInterrupt = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP nsInterfaceState::DidRedo(nsITransactionManager *aManager,  
  nsITransaction *aTransaction, nsresult aRedoResult)
{
  CallUpdateCommands(nsAutoString("undo"));
  return NS_OK;
}

NS_IMETHODIMP nsInterfaceState::WillBeginBatch(nsITransactionManager *aManager, PRBool *aInterrupt)
{
  *aInterrupt = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP nsInterfaceState::DidBeginBatch(nsITransactionManager *aManager, nsresult aResult)
{
  return NS_OK;
}

NS_IMETHODIMP nsInterfaceState::WillEndBatch(nsITransactionManager *aManager, PRBool *aInterrupt)
{
  *aInterrupt = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP nsInterfaceState::DidEndBatch(nsITransactionManager *aManager, nsresult aResult)
{
  return NS_OK;
}

NS_IMETHODIMP nsInterfaceState::WillMerge(nsITransactionManager *aManager,
        nsITransaction *aTopTransaction, nsITransaction *aTransactionToMerge, PRBool *aInterrupt)
{
  *aInterrupt = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP nsInterfaceState::DidMerge(nsITransactionManager *aManager,
  nsITransaction *aTopTransaction, nsITransaction *aTransactionToMerge,
                    PRBool aDidMerge, nsresult aMergeResult)
{
  return NS_OK;
}


nsresult nsInterfaceState::CallUpdateCommands(const nsString& aCommand)
{
  if (!mDOMWindow)
  {
    nsCOMPtr<nsIEditor> editor = do_QueryInterface(mEditor);
    if (!editor) return NS_ERROR_FAILURE;

    nsCOMPtr<nsIDOMDocument> domDoc;
    editor->GetDocument(getter_AddRefs(domDoc));
    if (!domDoc) return NS_ERROR_FAILURE;

    nsCOMPtr<nsIDocument> theDoc = do_QueryInterface(domDoc);
    if (!theDoc) return NS_ERROR_FAILURE;

    nsCOMPtr<nsIScriptGlobalObject> scriptGlobalObject;
    theDoc->GetScriptGlobalObject(getter_AddRefs(scriptGlobalObject));

    nsCOMPtr<nsIDOMWindow> domWindow = do_QueryInterface(scriptGlobalObject);
    if (!domWindow) return NS_ERROR_FAILURE;
    mDOMWindow = domWindow;
  }
  
  return mDOMWindow->UpdateCommands(aCommand);
}

NS_IMETHODIMP
nsInterfaceState::ForceUpdate()
{
  nsresult  rv;
  
  // we don't really care if any of these fail.
  
  // update bold
  rv = UpdateTextState("b", "Editor:Bold", "bold", mBoldState);
  // update italic
  rv = UpdateTextState("i", "Editor:Italic", "italic", mItalicState);
  // update underline
  rv = UpdateTextState("u", "Editor:Underline", "underline", mUnderlineState);
  
  // update the paragraph format popup
  rv = UpdateParagraphState("Editor:Paragraph:Format", "format", mParagraphFormat);
  // udpate the font face
  rv = UpdateFontFace("Editor:Font:Face", "font", mFontString);
  
  // TODO: FINISH FONT FACE AND ADD FONT SIZE ("Editor:Font:Size", "fontsize", mFontSize)

  // update the list buttons
  rv = UpdateListState("Editor:Paragraph:ListType");

  return NS_OK;
}

PRBool
nsInterfaceState::SelectionIsCollapsed()
{
  nsresult rv;
  // we don't care too much about failures here.
  nsCOMPtr<nsIEditor> editor = do_QueryInterface(mEditor, &rv);
  if (NS_SUCCEEDED(rv))
  {
    nsCOMPtr<nsIDOMSelection> domSelection;
    rv = editor->GetSelection(getter_AddRefs(domSelection));
    if (NS_SUCCEEDED(rv))
    {    
      PRBool selectionCollapsed = PR_FALSE;
      rv = domSelection->GetIsCollapsed(&selectionCollapsed);
      return selectionCollapsed;
    }
  }
  return PR_FALSE;
}

nsresult
nsInterfaceState::UpdateParagraphState(const char* observerName, const char* attributeName, nsString& ioParaFormat)
{
  nsStringArray tagList;
  
  mEditor->GetParagraphTags(&tagList);

  PRInt32 numTags = tagList.Count();
  nsAutoString  thisTag;
  //Note: If numTags == 0, we probably have a text node not in a container
  //  (directly under <body>). Consider it normal
  if (numTags > 0)
  {
    // This will never show the "mixed state"
    // TODO: Scan list of tags and if any are different, set to "mixed"
    tagList.StringAt(0, thisTag);
  }
  if (thisTag != mParagraphFormat)
  {
    nsresult rv = SetNodeAttribute(observerName, attributeName, thisTag);
    if (NS_FAILED(rv)) return rv;
    mParagraphFormat = thisTag;
  }
  return NS_OK;
}

nsresult
nsInterfaceState::UpdateListState(const char* observerName)
{
  nsresult  rv = NS_ERROR_NO_INTERFACE;

  nsCOMPtr<nsIDOMSelection>  domSelection;
  {
    nsCOMPtr<nsIEditor> editor = do_QueryInterface(mEditor);
    editor->GetSelection(getter_AddRefs(domSelection));
  }
  
  nsCOMPtr<nsIDOMNode>       domNode;
  if (domSelection)
    domSelection->GetAnchorNode(getter_AddRefs(domNode));
  
  // tagStr will hold the list state when we're done.
  nsAutoString  tagStr("ol");
  nsCOMPtr<nsIDOMElement> parentElement;
  rv = mEditor->GetElementOrParentByTagName(tagStr, domNode, getter_AddRefs(parentElement));
  if (NS_FAILED(rv)) return rv;  

  if (!parentElement)
  {
    tagStr = "ul";
    rv = mEditor->GetElementOrParentByTagName(tagStr, domNode, getter_AddRefs(parentElement));
    if (NS_FAILED(rv)) return rv;
    
    if (!parentElement)
      tagStr = "";
  }
  
  if (tagStr != mListTag)
  {
    rv = SetNodeAttribute(observerName, "format", tagStr);
    mListTag = tagStr;
  }

  return rv;
}

nsresult
nsInterfaceState::UpdateFontFace(const char* observerName, const char* attributeName, nsString& ioFontString)
{
  nsresult  rv;
  
  PRBool    firstOfSelectionHasProp = PR_FALSE;
  PRBool    anyOfSelectionHasProp = PR_FALSE;
  PRBool    allOfSelectionHasProp = PR_FALSE;

  nsCOMPtr<nsIAtom> styleAtom = getter_AddRefs(NS_NewAtom("font"));
  nsAutoString faceStr("face");
  nsAutoString thisFace;
  
  // Use to test for "Default Fixed Width"
  nsCOMPtr<nsIAtom> fixedStyleAtom = getter_AddRefs(NS_NewAtom("tt"));

  PRBool testBoolean;
  
  rv = mEditor->GetInlineProperty(styleAtom, &faceStr, &thisFace, firstOfSelectionHasProp, anyOfSelectionHasProp, allOfSelectionHasProp);
  if( !anyOfSelectionHasProp )
  {
    // No font face set -- check for "tt"
    rv = mEditor->GetInlineProperty(fixedStyleAtom, nsnull, nsnull, firstOfSelectionHasProp, anyOfSelectionHasProp, allOfSelectionHasProp);
    testBoolean = anyOfSelectionHasProp;
    if (anyOfSelectionHasProp)
      thisFace = "tt";
  }

  // TODO: HANDLE "MIXED" STATE
  if (thisFace != mFontString)
  {
    rv = SetNodeAttribute(observerName, faceStr.GetBuffer(), thisFace);
    if (NS_SUCCEEDED(rv))
      mFontString = thisFace;
  }
  return rv;
}


nsresult
nsInterfaceState::UpdateTextState(const char* tagName, const char* observerName, const char* attributeName, PRInt8& ioState)
{
  nsresult  rv;
    
  nsCOMPtr<nsIAtom> styleAtom = getter_AddRefs(NS_NewAtom(tagName));

  PRBool testBoolean;
  PRBool firstOfSelectionHasProp = PR_FALSE;
  PRBool anyOfSelectionHasProp = PR_FALSE;
  PRBool allOfSelectionHasProp = PR_FALSE;

  rv = mEditor->GetInlineProperty(styleAtom, nsnull, nsnull, firstOfSelectionHasProp, anyOfSelectionHasProp, allOfSelectionHasProp);
  testBoolean = allOfSelectionHasProp;			// change this to alter the behaviour

  if (NS_FAILED(rv)) return rv;

  if (testBoolean != ioState)
  {
    rv = SetNodeAttribute(observerName, attributeName, testBoolean ? "true" : "false");
	  if (NS_FAILED(rv))
	    return rv;
	  
	  ioState = testBoolean;
  }
  
  return rv;
}

nsresult
nsInterfaceState::UpdateDirtyState(PRBool aNowDirty)
{
  if (mDirtyState != aNowDirty)
  {
    nsresult rv;	// = SetNodeAttribute("Editor:Save", "disabled", aNowDirty ? "true" : "false");

    rv = SetNodeAttribute("Editor:Save", "disabled", aNowDirty ? "false" : "true");
	  if (NS_FAILED(rv)) return rv;

    mDirtyState = aNowDirty;
  }
  
  return NS_OK;  
}


nsresult
nsInterfaceState::SetNodeAttribute(const char* nodeID, const char* attributeName, const nsString& newValue)
{
    nsresult rv = NS_OK;

  if (!mChromeDoc)
    return NS_ERROR_NOT_INITIALIZED;

  nsCOMPtr<nsIDOMElement> elem;
  rv = mChromeDoc->GetElementById( nodeID, getter_AddRefs(elem) );
  if (NS_FAILED(rv) || !elem) return rv;
  
  return elem->SetAttribute(attributeName, newValue);
}


nsresult
nsInterfaceState::UnsetNodeAttribute(const char* nodeID, const char* attributeName)
{
    nsresult rv = NS_OK;

  if (!mChromeDoc)
    return NS_ERROR_NOT_INITIALIZED;

  nsCOMPtr<nsIDOMElement> elem;
  rv = mChromeDoc->GetElementById( nodeID, getter_AddRefs(elem) );
  if (NS_FAILED(rv) || !elem) return rv;
  
  return elem->RemoveAttribute(attributeName);
}


nsresult NS_NewInterfaceState(nsIHTMLEditor* aEditor, nsIDOMXULDocument* aChromeDoc, nsIDOMSelectionListener** aInstancePtrResult)
{
  nsInterfaceState* newThang = new nsInterfaceState;
  if (!newThang)
    return NS_ERROR_OUT_OF_MEMORY;

  *aInstancePtrResult = nsnull;
  nsresult rv = newThang->Init(aEditor, aChromeDoc);
  if (NS_FAILED(rv))
  {
    delete newThang;
    return rv;
  }
      
  return newThang->QueryInterface(NS_GET_IID(nsIDOMSelectionListener), (void **)aInstancePtrResult);
}
