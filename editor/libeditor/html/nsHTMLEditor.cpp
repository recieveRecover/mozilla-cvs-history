/* -*- Mode: C++ tab-width: 2 indent-tabs-mode: nil c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL") you may not use this file except in
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

#include "nsTextEditor.h"
#include "nsHTMLEditor.h"
#include "nsHTMLEditRules.h"
#include "nsEditorEventListeners.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMRange.h"
#include "nsIDOMDocument.h"
#include "nsIDOMEventReceiver.h" 
#include "nsIDOMKeyListener.h" 
#include "nsIDOMMouseListener.h"
#include "nsIDOMSelection.h"
#include "nsIDOMHTMLAnchorElement.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsIEnumerator.h"
#include "nsEditorCID.h"

#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsITableCellLayout.h"   //For GetColIndexForCell


static NS_DEFINE_IID(kIDOMEventReceiverIID, NS_IDOMEVENTRECEIVER_IID);
static NS_DEFINE_IID(kIDOMMouseListenerIID, NS_IDOMMOUSELISTENER_IID);
static NS_DEFINE_IID(kIDOMKeyListenerIID,   NS_IDOMKEYLISTENER_IID);

static NS_DEFINE_CID(kEditorCID,      NS_EDITOR_CID);
static NS_DEFINE_IID(kIEditorIID,     NS_IEDITOR_IID);
static NS_DEFINE_IID(kISupportsIID,   NS_ISUPPORTS_IID);

static NS_DEFINE_IID(kITextEditorIID, NS_ITEXTEDITOR_IID);
static NS_DEFINE_CID(kTextEditorCID,  NS_TEXTEDITOR_CID);
static NS_DEFINE_IID(kIHTMLEditorIID, NS_IHTMLEDITOR_IID);
static NS_DEFINE_CID(kHTMLEditorCID,  NS_HTMLEDITOR_CID);

#ifdef NS_DEBUG
static PRBool gNoisy = PR_FALSE;
#else
static const PRBool gNoisy = PR_FALSE;
#endif


nsHTMLEditor::nsHTMLEditor()
{
// Done in nsEditor
// NS_INIT_REFCNT();
}

nsHTMLEditor::~nsHTMLEditor()
{
  //the autopointers will clear themselves up. 
}

// Adds appropriate AddRef, Release, and QueryInterface methods for derived class
//NS_IMPL_ISUPPORTS_INHERITED(nsHTMLEditor, nsTextEditor, nsIHTMLEditor)

//NS_IMPL_ADDREF_INHERITED(Class, Super)
NS_IMETHODIMP_(nsrefcnt) nsHTMLEditor::AddRef(void)
{
  return nsTextEditor::AddRef();
}

//NS_IMPL_RELEASE_INHERITED(Class, Super)
NS_IMETHODIMP_(nsrefcnt) nsHTMLEditor::Release(void)
{
  return nsTextEditor::Release();
}

//NS_IMPL_QUERY_INTERFACE_INHERITED(Class, Super, AdditionalInterface)
NS_IMETHODIMP nsHTMLEditor::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (!aInstancePtr) return NS_ERROR_NULL_POINTER;
 
  if (aIID.Equals(nsIHTMLEditor::GetIID())) {
    *aInstancePtr = NS_STATIC_CAST(nsIHTMLEditor*, this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return nsTextEditor::QueryInterface(aIID, aInstancePtr);
}


NS_IMETHODIMP nsHTMLEditor::Init(nsIDOMDocument *aDoc, nsIPresShell *aPresShell)
{
  NS_PRECONDITION(nsnull!=aDoc && nsnull!=aPresShell, "bad arg");
  nsresult result=NS_ERROR_NULL_POINTER;
  if ((nsnull!=aDoc) && (nsnull!=aPresShell))
  {
    return nsTextEditor::Init(aDoc, aPresShell);
  }
  return result;
}

void  nsHTMLEditor::InitRules()
{
// instantiate the rules for this text editor
// XXX: we should be told which set of rules to instantiate
  mRules =  new nsHTMLEditRules();
  mRules->Init(this);
}

NS_IMETHODIMP nsHTMLEditor::SetTextProperty(nsIAtom *aProperty, 
                                            const nsString *aAttribute,
                                            const nsString *aValue)
{
  return nsTextEditor::SetTextProperty(aProperty, aAttribute, aValue);
}

NS_IMETHODIMP nsHTMLEditor::GetTextProperty(nsIAtom *aProperty, 
                                            const nsString *aAttribute, 
                                            const nsString *aValue,
                                            PRBool &aFirst, PRBool &aAny, PRBool &aAll)
{
  return nsTextEditor::GetTextProperty(aProperty, aAttribute, aValue, aFirst, aAny, aAll);
}

NS_IMETHODIMP nsHTMLEditor::RemoveTextProperty(nsIAtom *aProperty, const nsString *aAttribute)
{
  return nsTextEditor::RemoveTextProperty(aProperty, aAttribute);
}

NS_IMETHODIMP nsHTMLEditor::DeleteSelection(nsIEditor::Direction aDir)
{
  return nsTextEditor::DeleteSelection(aDir);
}

NS_IMETHODIMP nsHTMLEditor::InsertText(const nsString& aStringToInsert)
{
  return nsTextEditor::InsertText(aStringToInsert);
}

NS_IMETHODIMP nsHTMLEditor::InsertBreak()
{
  nsresult result;
  if (!mRules) { return NS_ERROR_NOT_INITIALIZED; }

  nsCOMPtr<nsIDOMSelection> selection;
  PRBool cancel= PR_FALSE;

  result = nsEditor::BeginTransaction();
  if (NS_FAILED(result)) { return result; }

  // pre-process
  nsEditor::GetSelection(getter_AddRefs(selection));
  nsTextRulesInfo ruleInfo(nsHTMLEditRules::kInsertBreak);
  result = mRules->WillDoAction(selection, &ruleInfo, &cancel);
  if ((PR_FALSE==cancel) && (NS_SUCCEEDED(result)))
  {
    // create the new BR node
    nsCOMPtr<nsIDOMNode> newNode;
    nsAutoString tag("BR");
    result = nsEditor::DeleteSelectionAndCreateNode(tag, getter_AddRefs(newNode));
    if (NS_SUCCEEDED(result) && newNode)
    {
      // set the selection to the new node
      nsCOMPtr<nsIDOMNode>parent;
      result = newNode->GetParentNode(getter_AddRefs(parent));
      if (NS_SUCCEEDED(result) && parent)
      {
        PRInt32 offsetInParent=-1;  // we use the -1 as a marker to see if we need to compute this or not
        nsCOMPtr<nsIDOMNode>nextNode;
        newNode->GetNextSibling(getter_AddRefs(nextNode));
        if (nextNode)
        {
          nsCOMPtr<nsIDOMCharacterData>nextTextNode;
          nextTextNode = do_QueryInterface(nextNode);
          if (!nextTextNode) {
            nextNode = do_QueryInterface(newNode);
          }
          else { 
            offsetInParent=0; 
          }
        }
        else {
          nextNode = do_QueryInterface(newNode);
        }
        result = nsEditor::GetSelection(getter_AddRefs(selection));
        if (NS_SUCCEEDED(result))
        {
          if (-1==offsetInParent) 
          {
            nextNode->GetParentNode(getter_AddRefs(parent));
            result = nsIEditorSupport::GetChildOffset(nextNode, parent, offsetInParent);
            if (NS_SUCCEEDED(result)) {
              selection->Collapse(parent, offsetInParent+1);  // +1 to insert just after the break
            }
          }
          else
          {
            selection->Collapse(nextNode, offsetInParent);
          }
        }
      }
    }
    // post-process, always called if WillInsertBreak didn't return cancel==PR_TRUE
    result = mRules->DidDoAction(selection, &ruleInfo, result);
  }
  nsresult endTxnResult = nsEditor::EndTransaction();  // don't return this result!
  NS_ASSERTION ((NS_SUCCEEDED(endTxnResult)), "bad end transaction result");

// XXXX: Horrible hack! We are doing this because
// of an error in Gecko which is not rendering the
// document after a change via the DOM - gpk 2/13/99
  // BEGIN HACK!!!
  HACKForceRedraw();
  // END HACK

  return result;
}

// Methods shared with the base editor.
// Note: We could call each of these via nsTextEditor -- is that better?
NS_IMETHODIMP nsHTMLEditor::EnableUndo(PRBool aEnable)
{
  return nsTextEditor::EnableUndo(aEnable);
}

NS_IMETHODIMP nsHTMLEditor::Undo(PRUint32 aCount)
{
  return nsTextEditor::Undo(aCount);
}

NS_IMETHODIMP nsHTMLEditor::CanUndo(PRBool &aIsEnabled, PRBool &aCanUndo)
{
  return nsTextEditor::CanUndo(aIsEnabled, aCanUndo);
}

NS_IMETHODIMP nsHTMLEditor::Redo(PRUint32 aCount)
{
  return nsTextEditor::Redo(aCount);
}

NS_IMETHODIMP nsHTMLEditor::CanRedo(PRBool &aIsEnabled, PRBool &aCanRedo)
{
  return nsTextEditor::CanRedo(aIsEnabled, aCanRedo);
}

NS_IMETHODIMP nsHTMLEditor::BeginTransaction()
{
  return nsTextEditor::BeginTransaction();
}

NS_IMETHODIMP nsHTMLEditor::EndTransaction()
{
  return nsTextEditor::EndTransaction();
}

NS_IMETHODIMP nsHTMLEditor::MoveSelectionUp(nsIAtom *aIncrement, PRBool aExtendSelection)
{
  return nsTextEditor::MoveSelectionUp(aIncrement, aExtendSelection);
}

NS_IMETHODIMP nsHTMLEditor::MoveSelectionDown(nsIAtom *aIncrement, PRBool aExtendSelection)
{
  return nsTextEditor::MoveSelectionDown(aIncrement, aExtendSelection);
}

NS_IMETHODIMP nsHTMLEditor::MoveSelectionNext(nsIAtom *aIncrement, PRBool aExtendSelection)
{
  return nsTextEditor::MoveSelectionNext(aIncrement, aExtendSelection);
}

NS_IMETHODIMP nsHTMLEditor::MoveSelectionPrevious(nsIAtom *aIncrement, PRBool aExtendSelection)
{
  return nsTextEditor::MoveSelectionPrevious(aIncrement, aExtendSelection);
}

NS_IMETHODIMP nsHTMLEditor::SelectNext(nsIAtom *aIncrement, PRBool aExtendSelection) 
{
  return nsTextEditor::SelectNext(aIncrement, aExtendSelection);
}

NS_IMETHODIMP nsHTMLEditor::SelectPrevious(nsIAtom *aIncrement, PRBool aExtendSelection)
{
  return nsTextEditor::SelectPrevious(aIncrement, aExtendSelection);
}

NS_IMETHODIMP nsHTMLEditor::SelectAll()
{
  return nsTextEditor::SelectAll();
}

NS_IMETHODIMP nsHTMLEditor::ScrollUp(nsIAtom *aIncrement)
{
  return nsTextEditor::ScrollUp(aIncrement);
}

NS_IMETHODIMP nsHTMLEditor::ScrollDown(nsIAtom *aIncrement)
{
  return nsTextEditor::ScrollDown(aIncrement);
}

NS_IMETHODIMP nsHTMLEditor::ScrollIntoView(PRBool aScrollToBegin)
{
  return nsTextEditor::ScrollIntoView(aScrollToBegin);
}

NS_IMETHODIMP nsHTMLEditor::Cut()
{
  return nsTextEditor::Cut();
}

NS_IMETHODIMP nsHTMLEditor::Copy()
{
  return nsTextEditor::Copy();
}

NS_IMETHODIMP nsHTMLEditor::Paste()
{
  return nsTextEditor::Paste();
}

NS_IMETHODIMP nsHTMLEditor::Insert(nsString& aInputString)
{
  return nsTextEditor::Insert(aInputString);
}

NS_IMETHODIMP nsHTMLEditor::OutputText(nsString& aOutputString)
{
  return nsTextEditor::OutputText(aOutputString);
}

NS_IMETHODIMP nsHTMLEditor::OutputHTML(nsString& aOutputString)
{
  return nsTextEditor::OutputHTML(aOutputString);
}

//================================================================
// HTML Editor methods
//
// Note: Table Editing methods are implemented in EditTable.cpp
//

NS_IMETHODIMP 
nsHTMLEditor::AddBlockParent(nsString& aParentTag)
{

  if (gNoisy) 
  { 
    char *tag = aParentTag.ToNewCString();
    printf("---------- nsHTMLEditor::AddBlockParent %s ----------\n", tag); 
    delete [] tag;
  }
  
  nsresult result=NS_ERROR_NOT_INITIALIZED;
  nsCOMPtr<nsIDOMSelection>selection;
  result = nsEditor::GetSelection(getter_AddRefs(selection));
  if ((NS_SUCCEEDED(result)) && selection)
  {
    // set the block parent for all selected ranges
    nsEditor::BeginTransaction();
    nsCOMPtr<nsIEnumerator> enumerator;
    enumerator = do_QueryInterface(selection);
    if (enumerator)
    {
      enumerator->First(); 
      nsISupports *currentItem;
      result = enumerator->CurrentItem(&currentItem);
      if ((NS_SUCCEEDED(result)) && (nsnull!=currentItem))
      {
        nsCOMPtr<nsIDOMRange> range( do_QueryInterface(currentItem) );
        nsCOMPtr<nsIDOMNode>commonParent;
        result = range->GetCommonParent(getter_AddRefs(commonParent));
        if ((NS_SUCCEEDED(result)) && commonParent)
        {
          PRInt32 startOffset, endOffset;
          range->GetStartOffset(&startOffset);
          range->GetEndOffset(&endOffset);
          nsCOMPtr<nsIDOMNode> startParent;  nsCOMPtr<nsIDOMNode> endParent;
          range->GetStartParent(getter_AddRefs(startParent));
          range->GetEndParent(getter_AddRefs(endParent));
          if (startParent.get()==endParent.get()) 
          { // the range is entirely contained within a single node
            // commonParent==aStartParent, so get the "real" parent of the selection
            result = ReParentContentOfNode(startParent, aParentTag);
          }
          else
          {
            nsCOMPtr<nsIDOMNode> startGrandParent;
            startParent->GetParentNode(getter_AddRefs(startGrandParent));
            nsCOMPtr<nsIDOMNode> endGrandParent;
            endParent->GetParentNode(getter_AddRefs(endGrandParent));
            if (NS_SUCCEEDED(result))
            {
              PRBool canCollapse = PR_FALSE;
              if (endGrandParent.get()==startGrandParent.get())
              {
                result = IntermediateNodesAreInline(range, startParent, startOffset, 
                                                    endParent, endOffset, 
                                                    canCollapse);
              }
              if (NS_SUCCEEDED(result)) 
              {
                if (PR_TRUE==canCollapse)
                { // the range is between 2 nodes that have a common (immediate) grandparent,
                  // and any intermediate nodes are just inline style nodes
                  result = ReParentContentOfNode(startParent, aParentTag);
                }
                else
                { // the range is between 2 nodes that have no simple relationship
                  result = ReParentContentOfRange(range, aParentTag);
                }
              }
            }
          }
          if (NS_SUCCEEDED(result))
          { // compute a range for the selection
            // don't want to actually do anything with selection, because
            // we are still iterating through it.  Just want to create and remember
            // an nsIDOMRange, and later add the range to the selection after clearing it.
            // XXX: I'm blocked here because nsIDOMSelection doesn't provide a mechanism
            //      for setting a compound selection yet.
          }
        }
      }
    }
    nsEditor::EndTransaction();
    if (NS_SUCCEEDED(result))
    { // set the selection
      // XXX: can't do anything until I can create ranges
    }
  }
  if (gNoisy) {printf("Finished nsHTMLEditor::AddBlockParent with this content:\n"); DebugDumpContent(); } // DEBUG

  return result;
}

NS_IMETHODIMP 
nsHTMLEditor::ReParentContentOfNode(nsIDOMNode *aNode, nsString &aParentTag)
{
  if (!aNode) { return NS_ERROR_NULL_POINTER; }
  // find the current block parent
  nsCOMPtr<nsIDOMElement>blockParentElement;
  nsresult result = nsTextEditor::GetBlockParent(aNode, getter_AddRefs(blockParentElement));
  if ((NS_SUCCEEDED(result)) && blockParentElement)
  {
    nsCOMPtr<nsIDOMNode> newParentNode;
    nsCOMPtr<nsIDOMNode> blockParentNode = do_QueryInterface(blockParentElement);
    // select all children of block parent, if the block parent is not the body
    nsAutoString parentTag;
    blockParentElement->GetTagName(parentTag);
    nsAutoString bodyTag = "body";
    if (PR_TRUE==parentTag.EqualsIgnoreCase(bodyTag))
    {
      // if aNode is a text node, we have <BODY><TEXT>text.
      // re-parent <TEXT> into a new <aTag> at the offset of <TEXT> in <BODY>
      nsCOMPtr<nsIDOMCharacterData> nodeAsText = do_QueryInterface(aNode);
      if (nodeAsText)
      {
        result = ReParentBlockContent(aNode, aParentTag, blockParentNode, parentTag, 
                                      getter_AddRefs(newParentNode));
      }
      else
      { // this is the case of an insertion point between 2 non-text objects
        PRInt32 offsetInParent=0;
        result = nsIEditorSupport::GetChildOffset(aNode, blockParentNode, offsetInParent);
        NS_ASSERTION((NS_SUCCEEDED(result)), "bad result from GetChildOffset");
        // otherwise, just create the block parent at the selection
        result = nsTextEditor::CreateNode(aParentTag, blockParentNode, offsetInParent, 
                                          getter_AddRefs(newParentNode));
        // XXX: need to create a bogus text node inside this new block!
        //      that means, I need to generalize bogus node handling
      }
    }
    else
    { 
      // for the selected block content, replace blockParentNode with a new node of the correct type
      if (PR_FALSE==parentTag.EqualsIgnoreCase(aParentTag))
      {
        DebugDumpContent(); // DEBUG
        result = ReParentBlockContent(aNode, aParentTag, blockParentNode, parentTag, 
                                      getter_AddRefs(newParentNode));
        if (NS_SUCCEEDED(result) && newParentNode)
        {
          PRBool hasChildren;
          blockParentNode->HasChildNodes(&hasChildren);
          if (PR_FALSE==hasChildren)
          {
            result = nsEditor::DeleteNode(blockParentNode);
            if (gNoisy) 
            {
              printf("deleted old block parent node %p\n", blockParentNode.get());
              DebugDumpContent(); // DEBUG
            }
          }
        }
      }
      else { // otherwise, it's a no-op
        if (gNoisy) { printf("AddBlockParent is a no-op for this collapsed selection.\n"); }
      }
    }
  }
  return result;
}


NS_IMETHODIMP 
nsHTMLEditor::ReParentBlockContent(nsIDOMNode  *aNode, 
                                   nsString    &aParentTag,
                                   nsIDOMNode  *aBlockParentNode,
                                   nsString    &aBlockParentTag,
                                   nsIDOMNode **aNewParentNode)
{
  if (!aNode || !aBlockParentNode || !aNewParentNode) { return NS_ERROR_NULL_POINTER; }
  nsCOMPtr<nsIDOMNode>ancestor;
  nsresult result = aNode->GetParentNode(getter_AddRefs(ancestor));
  nsCOMPtr<nsIDOMNode>previousAncestor = do_QueryInterface(aNode);
  while (NS_SUCCEEDED(result) && ancestor)
  {
    nsCOMPtr<nsIDOMElement>ancestorElement = do_QueryInterface(ancestor);
    nsAutoString ancestorTag;
    ancestorElement->GetTagName(ancestorTag);
    if (ancestorTag.EqualsIgnoreCase(aBlockParentTag))
    {
      break;  // previousAncestor will contain the node to operate on
    }
    previousAncestor = do_QueryInterface(ancestor);
    result = ancestorElement->GetParentNode(getter_AddRefs(ancestor));
  }
  // now, previousAncestor is the node we are operating on
  nsCOMPtr<nsIDOMNode>leftNode, rightNode;
  result = GetBlockDelimitedContent(aBlockParentNode, 
                                    previousAncestor, 
                                    getter_AddRefs(leftNode), 
                                    getter_AddRefs(rightNode));
  if ((NS_SUCCEEDED(result)) && leftNode && rightNode)
  {
    PRInt32 offsetInParent=0;
    if (aBlockParentTag.EqualsIgnoreCase("body"))
    {
      result = nsIEditorSupport::GetChildOffset(leftNode, aBlockParentNode, offsetInParent);
      NS_ASSERTION((NS_SUCCEEDED(result)), "bad result from GetChildOffset");
      result = nsTextEditor::CreateNode(aParentTag, aBlockParentNode, offsetInParent, aNewParentNode);
    }
    else
    {
      nsCOMPtr<nsIDOMNode> grandParent;
      result = aBlockParentNode->GetParentNode(getter_AddRefs(grandParent));
      if ((NS_SUCCEEDED(result)) && grandParent)
      {
        result = nsIEditorSupport::GetChildOffset(leftNode, aBlockParentNode, offsetInParent);
        NS_ASSERTION((NS_SUCCEEDED(result)), "bad result from GetChildOffset");
        result = nsTextEditor::CreateNode(aParentTag, aBlockParentNode, offsetInParent, aNewParentNode);
      }
    }
    if ((NS_SUCCEEDED(result)) && *aNewParentNode)
    { // move all the children/contents of aBlockParentNode to aNewParentNode
      nsCOMPtr<nsIDOMNode>childNode = do_QueryInterface(rightNode);
      nsCOMPtr<nsIDOMNode>previousSiblingNode;
      while (NS_SUCCEEDED(result) && childNode)
      {
        childNode->GetPreviousSibling(getter_AddRefs(previousSiblingNode));
        // explicitly delete of childNode from it's current parent
        // can't just rely on DOM semantics of InsertNode doing the delete implicitly, doesn't undo! 
        result = nsEditor::DeleteNode(childNode); 
        if (NS_SUCCEEDED(result))
        {
          result = nsEditor::InsertNode(childNode, *aNewParentNode, 0);
          if (gNoisy) 
          {
            printf("re-parented sibling node %p\n", childNode.get());
          }
        }
        if (childNode==leftNode || rightNode==leftNode) {
          break;
        }
        childNode = do_QueryInterface(previousSiblingNode);
      } // end while loop 
    }
  }
  return result;
}

NS_IMETHODIMP 
nsHTMLEditor::ReParentContentOfRange(nsIDOMRange *aRange, nsString &aParentTag)
{
  nsresult result=NS_ERROR_NOT_IMPLEMENTED;
  printf("transformations over complex selections not implemented.\n");
  return result;
}


NS_IMETHODIMP 
nsHTMLEditor::RemoveBlockParent()
{
  nsresult result = NS_ERROR_NOT_IMPLEMENTED;
  printf("remove block parent not implemented.\n");
  return result;
}

NS_IMETHODIMP
nsHTMLEditor::InsertLink(nsString& aURL)
{
  nsresult res = nsEditor::BeginTransaction();

  // Find out if the selection is collapsed:
  nsCOMPtr<nsIDOMSelection> selection;
  res = GetSelection(getter_AddRefs(selection));
  if (!NS_SUCCEEDED(res) || !selection)
  {
#ifdef DEBUG_akkana
    printf("Can't get selection!");
#endif
    return res;
  }
  PRBool isCollapsed;
  res = selection->GetIsCollapsed(&isCollapsed);
  if (!NS_SUCCEEDED(res))
    isCollapsed = PR_TRUE;

  // Temporary: we need to save the contents of the selection,
  // then insert them back in as the child of the newly created
  // anchor node in order to put the link around the selection.
  // This will require copying the selection into a document fragment,
  // then splicing the document fragment back into the tree after the
  // new anchor node has been put in place.  As a temporary solution,
  // Copy/Paste does this for us in the text case
  // (and eventually in all cases).
  if (!isCollapsed)
    (void)Copy();

  nsCOMPtr<nsIDOMNode> newNode;
  nsAutoString tag("A");
  res = nsEditor::DeleteSelectionAndCreateNode(tag, getter_AddRefs(newNode));
  if (!NS_SUCCEEDED(res) || !newNode)
    return res;

  nsCOMPtr<nsIDOMHTMLAnchorElement> anchor (do_QueryInterface(newNode));
  if (!anchor)
  {
#ifdef DEBUG_akkana
    printf("Not an anchor element\n");
#endif
    return NS_NOINTERFACE;
  }

  res = anchor->SetHref(aURL);
  if (!NS_SUCCEEDED(res))
  {
#ifdef DEBUG_akkana
    printf("SetHref failed");
#endif
    return res;
  }

  // Set the selection to the node we just inserted:
  res = GetSelection(getter_AddRefs(selection));
  if (!NS_SUCCEEDED(res) || !selection)
  {
#ifdef DEBUG_akkana
    printf("Can't get selection!");
#endif
    return res;
  }
  res = selection->Collapse(newNode, 0);
  if (!NS_SUCCEEDED(res))
  {
#ifdef DEBUG_akkana
    printf("Couldn't collapse");
#endif
    return res;
  }

  // If we weren't collapsed, paste the old selection back in under the link:
  if (!isCollapsed)
    (void)Paste();
  // Otherwise (we were collapsed) insert some bogus text in
  // so the link will be visible
  else
  {
    nsString link("[***]");
    (void) InsertText(link);   // ignore return value -- we don't care
  }

  nsEditor::EndTransaction();  // don't return this result!

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLEditor::InsertImage(nsString& aURL,
                          nsString& aWidth, nsString& aHeight,
                          nsString& aHspace, nsString& aVspace,
                          nsString& aBorder,
                          nsString& aAlt,
                          nsString& aAlignment)
{
  nsresult res;
  nsCOMPtr<nsIDOMNode> newNode;

  nsCOMPtr<nsIDOMDocument>doc;
  res = GetDocument(getter_AddRefs(doc));
  if (NS_SUCCEEDED(res))
  {
    nsAutoString tag("IMG");
    nsCOMPtr<nsIDOMElement>newElement;
    res = doc->CreateElement(tag, getter_AddRefs(newElement));
    if (NS_SUCCEEDED(res) && newElement)
    {
      newNode = do_QueryInterface(newElement);
      nsCOMPtr<nsIDOMHTMLImageElement> image (do_QueryInterface(newNode));
      // Set all the attributes now, before we insert into the tree:
      if (image)
      {
        if (NS_SUCCEEDED(res = image->SetSrc(aURL)))
          if (NS_SUCCEEDED(res = image->SetWidth(aWidth)))
            if (NS_SUCCEEDED(res = image->SetHeight(aHeight)))
              if (NS_SUCCEEDED(res = image->SetAlt(aAlt)))
                if (NS_SUCCEEDED(res = image->SetBorder(aBorder)))
                  if (NS_SUCCEEDED(res = image->SetAlign(aAlignment)))
                    if (NS_SUCCEEDED(res = image->SetHspace(aHspace)))
                      if (NS_SUCCEEDED(res = image->SetVspace(aVspace)))
                        ;
      }
    }
  }

  // If any of these failed, then don't insert the new node into the tree
  if (!NS_SUCCEEDED(res))
  {
#ifdef DEBUG_akkana
    printf("Some failure creating the new image node\n");
#endif
    return res;
  }

  //
  // Now we're ready to insert the new image node:
  // Starting now, don't return without ending the transaction!
  //
  (void)nsEditor::BeginTransaction();

  nsCOMPtr<nsIDOMNode> parentNode;
  PRInt32 offsetOfNewNode;
  res = nsEditor::DeleteSelectionAndPrepareToCreateNode(parentNode,
                                                        offsetOfNewNode);
  if (NS_SUCCEEDED(res))
  {
    // and insert it into the right place in the tree:
    res = InsertNode(newNode, parentNode, offsetOfNewNode);
  }

  (void)nsEditor::EndTransaction();  // don't return this result!

  return res;
}


