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
#ifndef nsHTMLDocument_h___
#define nsHTMLDocument_h___

#include "nsDocument.h"
#include "nsMarkupDocument.h"
#include "nsIHTMLDocument.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMNSHTMLDocument.h"
#include "nsIDOMNode.h"
#include "plhash.h"

class nsIHTMLStyleSheet;
class nsIHTMLCSSStyleSheet;
class nsContentList;
class nsIContentViewerContainer;
class nsIParser;
class BlockText;
class nsDOMStyleSheetCollection;

class nsHTMLDocument : public nsMarkupDocument, public nsIHTMLDocument, public nsIDOMHTMLDocument, public nsIDOMNSHTMLDocument {
public:
  nsHTMLDocument();
  virtual ~nsHTMLDocument();

  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr);

  NS_IMETHOD_(nsrefcnt) AddRef(void);
  NS_IMETHOD_(nsrefcnt) Release(void);

  NS_IMETHOD StartDocumentLoad(nsIURL* aUrl, 
                               nsIContentViewerContainer* aContainer,
                               nsIStreamListener** aDocListener,
                               const char* aCommand);

  NS_IMETHOD EndLoad();

  NS_IMETHOD AddImageMap(nsIImageMap* aMap);

  NS_IMETHOD GetImageMap(const nsString& aMapName, nsIImageMap** aResult);

  NS_IMETHOD GetAttributeStyleSheet(nsIHTMLStyleSheet** aStyleSheet);

  NS_IMETHOD GetDTDMode(nsDTDMode& aMode);
  NS_IMETHOD SetDTDMode(nsDTDMode aMode);

  // nsIDOMDocument interface
  NS_IMETHOD    GetDoctype(nsIDOMDocumentType** aDocumentType);
  NS_IMETHOD    GetImplementation(nsIDOMDOMImplementation** aImplementation)
  { return nsDocument::GetImplementation(aImplementation); }
  NS_IMETHOD    GetDocumentElement(nsIDOMElement** aDocumentElement)
  { return nsDocument::GetDocumentElement(aDocumentElement); }
  NS_IMETHOD    CreateCDATASection(const nsString& aData, nsIDOMCDATASection** aReturn);
  NS_IMETHOD    CreateEntityReference(const nsString& aName, nsIDOMEntityReference** aReturn);
  NS_IMETHOD    CreateDocumentFragment(nsIDOMDocumentFragment** aReturn);
  NS_IMETHOD    CreateComment(const nsString& aData, nsIDOMComment** aReturn);
  NS_IMETHOD    CreateProcessingInstruction(const nsString& aTarget, const nsString& aData, nsIDOMProcessingInstruction** aReturn);
  NS_IMETHOD    CreateAttribute(const nsString& aName, nsIDOMAttr** aReturn);
  NS_IMETHOD    CreateElement(const nsString& aTagName, 
                              nsIDOMElement** aReturn);
  NS_IMETHOD    CreateTextNode(const nsString& aData, nsIDOMText** aReturn);
  NS_IMETHOD    GetElementsByTagName(const nsString& aTagname, nsIDOMNodeList** aReturn)
  { return nsDocument::GetElementsByTagName(aTagname, aReturn); }

  // nsIDOMNode interface
  NS_FORWARD_IDOMNODE(nsDocument::)

  // nsIDOMHTMLDocument interface
  NS_DECL_IDOMHTMLDOCUMENT
  NS_DECL_IDOMNSHTMLDOCUMENT
  // the following is not part of nsIDOMHTMLDOCUMENT but allows the content sink to add forms
  NS_IMETHOD AddForm(nsIDOMHTMLFormElement* aForm);

  // From nsIScriptObjectOwner interface, implemented by nsDocument
  NS_IMETHOD GetScriptObject(nsIScriptContext *aContext, void** aScriptObject);

  /**
    * Finds text in content
   */
  NS_IMETHOD FindNext(const nsString &aSearchStr, PRBool aMatchCase, PRBool aSearchDown, PRBool &aIsFound);

protected:
  // Find/Search Method/Data member
  PRBool SearchBlock(BlockText    & aBlockText, 
                     nsString     & aStr,
                     nsIDOMNode * aCurrentBlock);

  PRBool NodeIsBlock(nsIDOMNode * aNode);
  nsIDOMNode * FindBlockParent(nsIDOMNode * aNode, 
                               PRBool aSkipThisContent = PR_FALSE);

  PRBool BuildBlockTraversing(nsIDOMNode   * aParent,
                              BlockText    & aBlockText,
                              nsIDOMNode * aCurrentBlock);

  PRBool BuildBlockFromContent(nsIDOMNode   * aNode,
                              BlockText    & aBlockText,
                              nsIDOMNode * aCurrentBlock);

  PRBool BuildBlockFromStack(nsIDOMNode * aParent,
                             BlockText  & aBlockText,
                             PRInt32      aStackInx,
                             nsIDOMNode * aCurrentBlock);

  // Search/Find Data Member
  nsIDOMNode ** mParentStack;
  nsIDOMNode ** mChildStack;
  PRInt32       mStackInx;

  nsString   * mSearchStr;
  PRInt32      mSearchDirection;

  PRInt32      mLastBlockSearchOffset;
  PRBool       mAdjustToEnd;

  nsIDOMNode * mHoldBlockContent;
  nsIDOMNode * mBodyContent;

  PRBool       mShouldMatchCase;
  PRBool       mIsPreTag;

protected:
  static PRIntn RemoveStrings(PLHashEntry *he, PRIntn i, void *arg);
  void RegisterNamedItems(nsIContent *aContent, PRBool aInForm);
  void DeleteNamedItems();
  nsIContent *MatchName(nsIContent *aContent, const nsString& aName);

  virtual void AddStyleSheetToSet(nsIStyleSheet* aSheet, nsIStyleSet* aSet);
  static PRBool MatchLinks(nsIContent *aContent);
  static PRBool MatchAnchors(nsIContent *aContent);

  nsIHTMLStyleSheet*    mAttrStyleSheet;
  nsIHTMLCSSStyleSheet* mStyleAttrStyleSheet;
  nsDTDMode mDTDMode;
  nsVoidArray mImageMaps;

  nsContentList *mImages;
  nsContentList *mApplets;
  nsContentList *mEmbeds;
  nsContentList *mLinks;
  nsContentList *mAnchors;
  nsContentList *mForms;
  nsDOMStyleSheetCollection *mDOMStyleSheets;
  
  PLHashTable *mNamedItems;

  nsIParser *mParser;
};

#endif /* nsHTMLDocument_h___ */
