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
#include "nsIHTMLStyleSheet.h"
#include "nsIArena.h"
#include "nsCRT.h"
#include "nsIAtom.h"
#include "nsIURL.h"
#include "nsISupportsArray.h"
#include "nsHashtable.h"
#include "nsIHTMLContent.h"
#include "nsIHTMLAttributes.h"
#include "nsIStyleRule.h"
#include "nsIFrame.h"
#include "nsIStyleContext.h"
#include "nsHTMLAtoms.h"
#include "nsIPresContext.h"
#include "nsILinkHandler.h"
#include "nsIDocument.h"
#include "nsIHTMLTableCellElement.h"
#include "nsTableColFrame.h"
#include "nsHTMLIIDs.h"
#include "nsIStyleFrameConstruction.h"
#include "nsHTMLParts.h"
#include "nsIPresShell.h"
#include "nsIViewManager.h"
#include "nsStyleConsts.h"
#include "nsTableOuterFrame.h"
#include "nsIXMLDocument.h"
#include "nsIWebShell.h"
#include "nsHTMLContainerFrame.h"
#include "nsINameSpaceManager.h"
#include "nsLayoutAtoms.h"

static NS_DEFINE_IID(kIHTMLStyleSheetIID, NS_IHTML_STYLE_SHEET_IID);
static NS_DEFINE_IID(kIStyleSheetIID, NS_ISTYLE_SHEET_IID);
static NS_DEFINE_IID(kIStyleRuleIID, NS_ISTYLE_RULE_IID);
static NS_DEFINE_IID(kIStyleFrameConstructionIID, NS_ISTYLE_FRAME_CONSTRUCTION_IID);
static NS_DEFINE_IID(kIHTMLTableCellElementIID, NS_IHTMLTABLECELLELEMENT_IID);
static NS_DEFINE_IID(kIXMLDocumentIID, NS_IXMLDOCUMENT_IID);
static NS_DEFINE_IID(kIWebShellIID, NS_IWEB_SHELL_IID);

class HTMLAnchorRule : public nsIStyleRule {
public:
  HTMLAnchorRule(nsIHTMLStyleSheet* aSheet);
  ~HTMLAnchorRule();

  NS_DECL_ISUPPORTS

  NS_IMETHOD Equals(const nsIStyleRule* aRule, PRBool& aValue) const;
  NS_IMETHOD HashValue(PRUint32& aValue) const;
  NS_IMETHOD GetStyleSheet(nsIStyleSheet*& aSheet) const;
  // Strength is an out-of-band weighting, always 0 here
  NS_IMETHOD GetStrength(PRInt32& aStrength) const;

  NS_IMETHOD MapStyleInto(nsIStyleContext* aContext, nsIPresContext* aPresContext);

  NS_IMETHOD List(FILE* out = stdout, PRInt32 aIndent = 0) const;

  nscolor             mColor;
  nsIHTMLStyleSheet*  mSheet;
};

HTMLAnchorRule::HTMLAnchorRule(nsIHTMLStyleSheet* aSheet)
  : mSheet(aSheet)
{
  NS_INIT_REFCNT();
}

HTMLAnchorRule::~HTMLAnchorRule()
{
}

NS_IMPL_ISUPPORTS(HTMLAnchorRule, kIStyleRuleIID);

NS_IMETHODIMP
HTMLAnchorRule::Equals(const nsIStyleRule* aRule, PRBool& aResult) const
{
  aResult = PRBool(this == aRule);
  return NS_OK;
}

NS_IMETHODIMP
HTMLAnchorRule::HashValue(PRUint32& aValue) const
{
  aValue = (PRUint32)(mColor);
  return NS_OK;
}

NS_IMETHODIMP
HTMLAnchorRule::GetStyleSheet(nsIStyleSheet*& aSheet) const
{
  NS_IF_ADDREF(mSheet);
  aSheet = mSheet;
  return NS_OK;
}

// Strength is an out-of-band weighting, always 0 here
NS_IMETHODIMP
HTMLAnchorRule::GetStrength(PRInt32& aStrength) const
{
  aStrength = 0;
  return NS_OK;
}

NS_IMETHODIMP
HTMLAnchorRule::MapStyleInto(nsIStyleContext* aContext, nsIPresContext* aPresContext)
{
  nsStyleColor* styleColor = (nsStyleColor*)(aContext->GetMutableStyleData(eStyleStruct_Color));

  if (nsnull != styleColor) {
    styleColor->mColor = mColor;
  }
  return NS_OK;
}

NS_IMETHODIMP
HTMLAnchorRule::List(FILE* out, PRInt32 aIndent) const
{
  return NS_OK;
}

// -----------------------------------------------------------

class AttributeKey: public nsHashKey
{
public:
  AttributeKey(nsMapAttributesFunc aMapFunc, nsIHTMLAttributes* aAttributes);
  virtual ~AttributeKey(void);

  PRBool      Equals(const nsHashKey* aOther) const;
  PRUint32    HashValue(void) const;
  nsHashKey*  Clone(void) const;

private:
  AttributeKey(void);
  AttributeKey(const AttributeKey& aCopy);
  AttributeKey& operator=(const AttributeKey& aCopy);

public:
  nsMapAttributesFunc mMapFunc;
  nsIHTMLAttributes*  mAttributes;
  PRUint32            mHashSet: 1;
  PRUint32            mHashCode: 31;
};

AttributeKey::AttributeKey(nsMapAttributesFunc aMapFunc, nsIHTMLAttributes* aAttributes)
  : mMapFunc(aMapFunc),
    mAttributes(aAttributes)
{
  NS_ADDREF(mAttributes);
  mHashSet = 0;
  mHashCode = 0;
}

AttributeKey::~AttributeKey(void)
{
  NS_RELEASE(mAttributes);
}

PRBool AttributeKey::Equals(const nsHashKey* aOther) const
{
  const AttributeKey* other = (const AttributeKey*)aOther;
  if (mMapFunc == other->mMapFunc) {
    PRBool  equals;
    mAttributes->Equals(other->mAttributes, equals);
    return equals;
  }
  return PR_FALSE;
}

PRUint32 AttributeKey::HashValue(void) const
{
  if (0 == mHashSet) {
    AttributeKey* self = (AttributeKey*)this; // break const
    PRUint32  hash;
    mAttributes->HashValue(hash);
    self->mHashCode = (0x7FFFFFFF & hash);
    self->mHashCode |= (0x7FFFFFFF & PRUint32(mMapFunc));
    self->mHashSet = 1;
  }
  return mHashCode;
}

nsHashKey* AttributeKey::Clone(void) const
{
  AttributeKey* clown = new AttributeKey(mMapFunc, mAttributes);
  if (nsnull != clown) {
    clown->mHashSet = mHashSet;
    clown->mHashCode = mHashCode;
  }
  return clown;
}

// -----------------------------------------------------------

// Structure used when constructing formatting object trees. Contains
// state information needed for absolutely positioned elements
struct nsAbsoluteItems {
  nsIFrame* const containingBlock;  // containing block for absolutely positioned elements
  nsIFrame*       childList;        // list of absolutely positioned child frames

  nsAbsoluteItems(nsIFrame* aContainingBlock);

  // Adds a child frame to the list of absolutely positioned child frames
  void  AddAbsolutelyPositionedChild(nsIFrame* aChildFrame);
};

nsAbsoluteItems::nsAbsoluteItems(nsIFrame* aContainingBlock)
  : containingBlock(aContainingBlock)
{
  childList = nsnull;
}

void
nsAbsoluteItems::AddAbsolutelyPositionedChild(nsIFrame* aChildFrame)
{
#ifdef NS_DEBUG
  nsIFrame* parent;
  aChildFrame->GetGeometricParent(parent);
  NS_PRECONDITION(parent == containingBlock, "bad geometric parent");
#endif

  if (nsnull == childList) {
    childList = aChildFrame;
  } else {
    // Get the last frane in the list
    nsIFrame* lastChild = nsnull;

    for (nsIFrame* f = childList; nsnull != f; f->GetNextSibling(f)) {
      lastChild = f;
    }

    lastChild->SetNextSibling(aChildFrame);
  }
}

// -----------------------------------------------------------

class HTMLStyleSheetImpl : public nsIHTMLStyleSheet,
                           public nsIStyleFrameConstruction {
public:
  void* operator new(size_t size);
  void* operator new(size_t size, nsIArena* aArena);
  void operator delete(void* ptr);

  HTMLStyleSheetImpl(void);

  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);
  NS_IMETHOD_(nsrefcnt) AddRef();
  NS_IMETHOD_(nsrefcnt) Release();

  // nsIStyleSheet api
  NS_IMETHOD GetURL(nsIURL*& aURL) const;
  NS_IMETHOD GetTitle(nsString& aTitle) const;
  NS_IMETHOD GetType(nsString& aType) const;
  NS_IMETHOD GetMediumCount(PRInt32& aCount) const;
  NS_IMETHOD GetMediumAt(PRInt32 aIndex, nsIAtom*& aMedium) const;

  NS_IMETHOD GetEnabled(PRBool& aEnabled) const;
  NS_IMETHOD SetEnabled(PRBool aEnabled);

  // style sheet owner info
  NS_IMETHOD GetParentSheet(nsIStyleSheet*& aParent) const;  // will be null
  NS_IMETHOD GetOwningDocument(nsIDocument*& aDocument) const;

  NS_IMETHOD SetOwningDocument(nsIDocument* aDocumemt);

  virtual PRInt32 RulesMatching(nsIPresContext* aPresContext,
                                nsIContent* aContent,
                                nsIStyleContext* aParentContext,
                                nsISupportsArray* aResults);

  virtual PRInt32 RulesMatching(nsIPresContext* aPresContext,
                                nsIContent* aParentContent,
                                nsIAtom* aPseudoTag,
                                nsIStyleContext* aParentContext,
                                nsISupportsArray* aResults);

  NS_IMETHOD Init(nsIURL* aURL, nsIDocument* aDocument);
  NS_IMETHOD SetLinkColor(nscolor aColor);
  NS_IMETHOD SetActiveLinkColor(nscolor aColor);
  NS_IMETHOD SetVisitedLinkColor(nscolor aColor);

  // Attribute management methods, aAttributes is an in/out param
  NS_IMETHOD SetAttributesFor(nsIHTMLContent* aContent, 
                              nsIHTMLAttributes*& aAttributes);
  NS_IMETHOD SetAttributeFor(nsIAtom* aAttribute, const nsString& aValue, 
                             nsIHTMLContent* aContent, 
                             nsIHTMLAttributes*& aAttributes);
  NS_IMETHOD SetAttributeFor(nsIAtom* aAttribute, const nsHTMLValue& aValue, 
                             nsIHTMLContent* aContent, 
                             nsIHTMLAttributes*& aAttributes);
  NS_IMETHOD UnsetAttributeFor(nsIAtom* aAttribute, nsIHTMLContent* aContent, 
                               nsIHTMLAttributes*& aAttributes);

  NS_IMETHOD ConstructRootFrame(nsIPresContext* aPresContext,
                                nsIContent*     aDocElement,
                                nsIFrame*&      aNewFrame);

  NS_IMETHODIMP ReconstructFrames(nsIPresContext* aPresContext,
                                  nsIContent*     aContent,
                                  nsIFrame*       aParentFrame,
                                  nsIFrame*       aFrameSubTree);

  NS_IMETHOD ContentAppended(nsIPresContext* aPresContext,
                             nsIContent*     aContainer,
                             PRInt32         aNewIndexInContainer);

  NS_IMETHOD ContentInserted(nsIPresContext* aPresContext,
                             nsIContent*     aContainer,
                             nsIContent*     aChild,
                             PRInt32         aIndexInContainer);

  NS_IMETHOD ContentReplaced(nsIPresContext* aPresContext,
                             nsIContent*     aContainer,
                             nsIContent*     aOldChild,
                             nsIContent*     aNewChild,
                             PRInt32         aIndexInContainer);

  NS_IMETHOD ContentRemoved(nsIPresContext* aPresContext,
                            nsIContent*     aContainer,
                            nsIContent*     aChild,
                            PRInt32         aIndexInContainer);

  NS_IMETHOD ContentChanged(nsIPresContext* aPresContext,
                            nsIContent* aContent,
                            nsISupports* aSubContent);
  NS_IMETHOD AttributeChanged(nsIPresContext* aPresContext,
                              nsIContent* aContent,
                              nsIAtom* aAttribute,
                              PRInt32 aHint);

  // Style change notifications
  NS_IMETHOD StyleRuleChanged(nsIPresContext* aPresContext,
                              nsIStyleSheet* aStyleSheet,
                              nsIStyleRule* aStyleRule,
                              PRInt32 aHint); // See nsStyleConsts fot hint values
  NS_IMETHOD StyleRuleAdded(nsIPresContext* aPresContext,
                            nsIStyleSheet* aStyleSheet,
                            nsIStyleRule* aStyleRule);
  NS_IMETHOD StyleRuleRemoved(nsIPresContext* aPresContext,
                              nsIStyleSheet* aStyleSheet,
                              nsIStyleRule* aStyleRule);

  virtual void List(FILE* out = stdout, PRInt32 aIndent = 0) const;

private: 
  // These are not supported and are not implemented! 
  HTMLStyleSheetImpl(const HTMLStyleSheetImpl& aCopy); 
  HTMLStyleSheetImpl& operator=(const HTMLStyleSheetImpl& aCopy); 

protected:
  virtual ~HTMLStyleSheetImpl();

  NS_IMETHOD EnsureSingleAttributes(nsIHTMLAttributes*& aAttributes, 
                                    nsMapAttributesFunc aMapFunc,
                                    PRBool aCreate, 
                                    nsIHTMLAttributes*& aSingleAttrs);
  NS_IMETHOD UniqueAttributes(nsIHTMLAttributes*& aSingleAttrs,
                              nsMapAttributesFunc aMapFunc,
                              PRInt32 aAttrCount,
                              nsIHTMLAttributes*& aAttributes);

  nsresult ConstructFrame(nsIPresContext*  aPresContext,
                          nsIContent*      aContent,
                          nsIFrame*        aParentFrame,
                          nsAbsoluteItems& aAbsoluteItems,
                          nsIFrame*&       aFrameSubTree);

  nsresult ConstructDocElementFrame(nsIPresContext*  aPresContext,
                                    nsIContent*      aDocElement,
                                    nsIFrame*        aRootFrame,
                                    nsIStyleContext* aRootStyleContext,
                                    nsIFrame*&       aNewFrame);

  nsresult ConstructTableFrame(nsIPresContext*  aPresContext,
                               nsIContent*      aContent,
                               nsIFrame*        aParentFrame,
                               nsIStyleContext* aStyleContext,
                               nsAbsoluteItems& aAboluteItems,
                               nsIFrame*&       aNewFrame);

  nsresult ConstructTableCellFrame(nsIPresContext*  aPresContext,
                                   nsIContent*      aContent,
                                   nsIFrame*        aParentFrame,
                                   nsIStyleContext* aStyleContext,
                                   nsAbsoluteItems& aAbsoluteItems,
                                   nsIFrame*&       aNewFrame);

  nsresult CreatePlaceholderFrameFor(nsIPresContext*  aPresContext,
                                     nsIContent*      aContent,
                                     nsIFrame*        aFrame,
                                     nsIStyleContext* aStyleContext,
                                     nsIFrame*        aParentFrame,
                                     nsIFrame*&       aPlaceholderFrame);

  nsresult ConstructFrameByTag(nsIPresContext*  aPresContext,
                               nsIContent*      aContent,
                               nsIFrame*        aParentFrame,
                               nsIAtom*         aTag,
                               nsIStyleContext* aStyleContext,
                               nsAbsoluteItems& aAbsoluteItems,
                               nsIFrame*&       aNewFrame);

  nsresult ConstructFrameByDisplayType(nsIPresContext*       aPresContext,
                                       const nsStyleDisplay* aDisplay,
                                       nsIContent*           aContent,
                                       nsIFrame*             aParentFrame,
                                       nsIStyleContext*      aStyleContext,
                                       nsAbsoluteItems&      aAbsoluteItems,
                                       nsIFrame*&            aNewFrame);

  nsresult GetAdjustedParentFrame(nsIFrame*  aCurrentParentFrame, 
                                  PRUint8    aChildDisplayType,
                                  nsIFrame*& aNewParentFrame);


  nsresult ProcessChildren(nsIPresContext*  aPresContext,
                           nsIContent*      aContent,
                           nsIFrame*        aFrame,
                           nsAbsoluteItems& aAbsoluteItems,
                           nsIFrame*&       aChildList);

  nsresult CreateInputFrame(nsIContent* aContent, nsIFrame*& aFrame);

  PRBool IsScrollable(nsIPresContext* aPresContext, const nsStyleDisplay* aDisplay);

  nsIFrame* GetFrameFor(nsIPresShell* aPresShell, nsIPresContext* aPresContext,
                        nsIContent* aContent);

  nsIFrame* GetAbsoluteContainingBlock(nsIPresContext* aPresContext,
                                       nsIFrame*       aFrame);

protected:
  PRUint32 mInHeap : 1;
  PRUint32 mRefCnt : 31;

  nsIURL*             mURL;
  nsIDocument*        mDocument;
  HTMLAnchorRule*     mLinkRule;
  HTMLAnchorRule*     mVisitedRule;
  HTMLAnchorRule*     mActiveRule;
  nsHashtable         mAttrTable;
  nsIHTMLAttributes*  mRecycledAttrs;
  nsIFrame*           mInitialContainingBlock;
};


void* HTMLStyleSheetImpl::operator new(size_t size)
{
  HTMLStyleSheetImpl* rv = (HTMLStyleSheetImpl*) ::operator new(size);
#ifdef NS_DEBUG
  if (nsnull != rv) {
    nsCRT::memset(rv, 0xEE, size);
  }
#endif
  rv->mInHeap = 1;
  return (void*) rv;
}

void* HTMLStyleSheetImpl::operator new(size_t size, nsIArena* aArena)
{
  HTMLStyleSheetImpl* rv = (HTMLStyleSheetImpl*) aArena->Alloc(PRInt32(size));
#ifdef NS_DEBUG
  if (nsnull != rv) {
    nsCRT::memset(rv, 0xEE, size);
  }
#endif
  rv->mInHeap = 0;
  return (void*) rv;
}

void HTMLStyleSheetImpl::operator delete(void* ptr)
{
  HTMLStyleSheetImpl* sheet = (HTMLStyleSheetImpl*) ptr;
  if (nsnull != sheet) {
    if (sheet->mInHeap) {
      ::delete ptr;
    }
  }
}



HTMLStyleSheetImpl::HTMLStyleSheetImpl(void)
  : nsIHTMLStyleSheet(),
    mURL(nsnull),
    mDocument(nsnull),
    mLinkRule(nsnull),
    mVisitedRule(nsnull),
    mActiveRule(nsnull),
    mRecycledAttrs(nsnull),
    mInitialContainingBlock(nsnull)
{
  NS_INIT_REFCNT();
}

HTMLStyleSheetImpl::~HTMLStyleSheetImpl()
{
  NS_RELEASE(mURL);
  if (nsnull != mLinkRule) {
    mLinkRule->mSheet = nsnull;
    NS_RELEASE(mLinkRule);
  }
  if (nsnull != mVisitedRule) {
    mVisitedRule->mSheet = nsnull;
    NS_RELEASE(mVisitedRule);
  }
  if (nsnull != mActiveRule) {
    mActiveRule->mSheet = nsnull;
    NS_RELEASE(mActiveRule);
  }
  NS_IF_RELEASE(mRecycledAttrs);
}

NS_IMPL_ADDREF(HTMLStyleSheetImpl)
NS_IMPL_RELEASE(HTMLStyleSheetImpl)

nsresult HTMLStyleSheetImpl::QueryInterface(const nsIID& aIID,
                                            void** aInstancePtrResult)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null pointer");
  if (nsnull == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }
  static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
  if (aIID.Equals(kIHTMLStyleSheetIID)) {
    *aInstancePtrResult = (void*) ((nsIHTMLStyleSheet*)this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kIStyleSheetIID)) {
    *aInstancePtrResult = (void*) ((nsIStyleSheet*)this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kIStyleFrameConstructionIID)) {
    *aInstancePtrResult = (void*) ((nsIStyleFrameConstruction*)this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kISupportsIID)) {
    *aInstancePtrResult = (void*) this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

PRInt32 HTMLStyleSheetImpl::RulesMatching(nsIPresContext* aPresContext,
                                          nsIContent* aContent,
                                          nsIStyleContext* aParentContext,
                                          nsISupportsArray* aResults)
{
  NS_PRECONDITION(nsnull != aPresContext, "null arg");
  NS_PRECONDITION(nsnull != aContent, "null arg");
  NS_PRECONDITION(nsnull != aResults, "null arg");

  PRInt32 matchCount = 0;

  nsIHTMLContent* htmlContent;
  if (NS_OK == aContent->QueryInterface(kIHTMLContentIID, (void**)&htmlContent)) {
    nsIAtom*  tag;
    htmlContent->GetTag(tag);
    // if we have anchor colors, check if this is an anchor with an href
    if (tag == nsHTMLAtoms::a) {
      if ((nsnull != mLinkRule) || (nsnull != mVisitedRule) || (nsnull != mActiveRule)) {
        // test link state
        nsILinkHandler* linkHandler;

        if ((NS_OK == aPresContext->GetLinkHandler(&linkHandler)) &&
            (nsnull != linkHandler)) {
          nsAutoString base, href;  // XXX base??
          nsresult attrState = htmlContent->GetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::href, href);

          if (NS_CONTENT_ATTR_HAS_VALUE == attrState) {
            nsIURL* docURL = nsnull;
            nsIDocument* doc = nsnull;
            aContent->GetDocument(doc);
            if (nsnull != doc) {
              docURL = doc->GetDocumentURL();
              NS_RELEASE(doc);
            }

            nsAutoString absURLSpec;
            nsresult rv = NS_MakeAbsoluteURL(docURL, base, href, absURLSpec);
            NS_IF_RELEASE(docURL);

            nsLinkState  state;
            if (NS_OK == linkHandler->GetLinkState(absURLSpec, state)) {
              switch (state) {
                case eLinkState_Unvisited:
                  if (nsnull != mLinkRule) {
                    aResults->AppendElement(mLinkRule);
                    matchCount++;
                  }
                  break;
                case eLinkState_Visited:
                  if (nsnull != mVisitedRule) {
                    aResults->AppendElement(mVisitedRule);
                    matchCount++;
                  }
                  break;
                case eLinkState_Active:
                  if (nsnull != mActiveRule) {
                    aResults->AppendElement(mActiveRule);
                    matchCount++;
                  }
                  break;
              }
            }
          }
          NS_RELEASE(linkHandler);
        }
      }
    } // end A tag

    // just get the one and only style rule from the content
    nsIStyleRule* rule;
    htmlContent->GetContentStyleRule(rule);
    if (nsnull != rule) {
      aResults->AppendElement(rule);
      NS_RELEASE(rule);
      matchCount++;
    }

    NS_IF_RELEASE(tag);
    NS_RELEASE(htmlContent);
  }

  return matchCount;
}

PRInt32 HTMLStyleSheetImpl::RulesMatching(nsIPresContext* aPresContext,
                                          nsIContent* aParentContent,
                                          nsIAtom* aPseudoTag,
                                          nsIStyleContext* aParentContext,
                                          nsISupportsArray* aResults)
{
  // no pseudo frame style
  return 0;
}


  // nsIStyleSheet api
NS_IMETHODIMP
HTMLStyleSheetImpl::GetURL(nsIURL*& aURL) const
{
  NS_IF_ADDREF(mURL);
  aURL = mURL;
  return NS_OK;
}

NS_IMETHODIMP
HTMLStyleSheetImpl::GetTitle(nsString& aTitle) const
{
  aTitle.Truncate();
  aTitle.Append("Internal HTML Style Sheet");
  return NS_OK;
}

NS_IMETHODIMP
HTMLStyleSheetImpl::GetType(nsString& aType) const
{
  aType.Truncate();
  aType.Append("text/html");
  return NS_OK;
}

NS_IMETHODIMP
HTMLStyleSheetImpl::GetMediumCount(PRInt32& aCount) const
{
  aCount = 0;
  return NS_OK;
}

NS_IMETHODIMP
HTMLStyleSheetImpl::GetMediumAt(PRInt32 aIndex, nsIAtom*& aMedium) const
{
  aMedium = nsnull;
  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP
HTMLStyleSheetImpl::GetEnabled(PRBool& aEnabled) const
{
  aEnabled = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
HTMLStyleSheetImpl::SetEnabled(PRBool aEnabled)
{ // these can't be disabled
  return NS_OK;
}

NS_IMETHODIMP
HTMLStyleSheetImpl::GetParentSheet(nsIStyleSheet*& aParent) const
{
  aParent = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
HTMLStyleSheetImpl::GetOwningDocument(nsIDocument*& aDocument) const
{
  NS_IF_ADDREF(mDocument);
  aDocument = mDocument;
  return NS_OK;
}

NS_IMETHODIMP
HTMLStyleSheetImpl::SetOwningDocument(nsIDocument* aDocument)
{
  mDocument = aDocument; // not refcounted
  return NS_OK;
}

NS_IMETHODIMP HTMLStyleSheetImpl::Init(nsIURL* aURL, nsIDocument* aDocument)
{
  NS_PRECONDITION(aURL && aDocument, "null ptr");
  if (! aURL || ! aDocument)
    return NS_ERROR_NULL_POINTER;

  if (mURL || mDocument)
    return NS_ERROR_ALREADY_INITIALIZED;

  mDocument = aDocument; // not refcounted!
  mURL = aURL;
  NS_ADDREF(mURL);
  return NS_OK;
}

NS_IMETHODIMP HTMLStyleSheetImpl::SetLinkColor(nscolor aColor)
{
  if (nsnull == mLinkRule) {
    mLinkRule = new HTMLAnchorRule(this);
    if (nsnull == mLinkRule) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    NS_ADDREF(mLinkRule);
  }
  mLinkRule->mColor = aColor;
  return NS_OK;
}

NS_IMETHODIMP HTMLStyleSheetImpl::SetActiveLinkColor(nscolor aColor)
{
  if (nsnull == mActiveRule) {
    mActiveRule = new HTMLAnchorRule(this);
    if (nsnull == mActiveRule) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    NS_ADDREF(mActiveRule);
  }
  mActiveRule->mColor = aColor;
  return NS_OK;
}

NS_IMETHODIMP HTMLStyleSheetImpl::SetVisitedLinkColor(nscolor aColor)
{
  if (nsnull == mVisitedRule) {
    mVisitedRule = new HTMLAnchorRule(this);
    if (nsnull == mVisitedRule) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    NS_ADDREF(mVisitedRule);
  }
  mVisitedRule->mColor = aColor;
  return NS_OK;
}

NS_IMETHODIMP HTMLStyleSheetImpl::SetAttributesFor(nsIHTMLContent* aContent, nsIHTMLAttributes*& aAttributes)
{
  nsIHTMLAttributes*  attrs = aAttributes;

  if (nsnull != attrs) {
    nsMapAttributesFunc mapFunc;
    aContent->GetAttributeMappingFunction(mapFunc);
    AttributeKey  key(mapFunc, attrs);
    nsIHTMLAttributes* sharedAttrs = (nsIHTMLAttributes*)mAttrTable.Get(&key);
    if (nsnull == sharedAttrs) {  // we have a new unique set
      mAttrTable.Put(&key, attrs);
    }
    else {  // found existing set
      if (sharedAttrs != aAttributes) {
        aAttributes->ReleaseContentRef();
        NS_RELEASE(aAttributes);  // release content's ref

        aAttributes = sharedAttrs;
        aAttributes->AddContentRef();
        NS_ADDREF(aAttributes); // add ref for content
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP HTMLStyleSheetImpl::SetAttributeFor(nsIAtom* aAttribute, 
                                                  const nsString& aValue,
                                                  nsIHTMLContent* aContent, 
                                                  nsIHTMLAttributes*& aAttributes)
{
  nsresult            result = NS_OK;
  nsIHTMLAttributes*  attrs;
  nsMapAttributesFunc mapFunc;

  aContent->GetAttributeMappingFunction(mapFunc);

  result = EnsureSingleAttributes(aAttributes, mapFunc, PR_TRUE, attrs);

  if ((NS_OK == result) && (nsnull != attrs)) {
    PRInt32 count;
    attrs->SetAttribute(aAttribute, aValue, count);

    result = UniqueAttributes(attrs, mapFunc, count, aAttributes);
  }

  return result;
}

NS_IMETHODIMP
HTMLStyleSheetImpl::EnsureSingleAttributes(nsIHTMLAttributes*& aAttributes, 
                                           nsMapAttributesFunc aMapFunc,
                                           PRBool aCreate, 
                                           nsIHTMLAttributes*& aSingleAttrs)
{
  nsresult  result = NS_OK;
  PRInt32   contentRefCount;

  if (nsnull == aAttributes) {
    if (PR_TRUE == aCreate) {
      if (nsnull != mRecycledAttrs) {
        aSingleAttrs = mRecycledAttrs;
        mRecycledAttrs = nsnull;
        aSingleAttrs->SetMappingFunction(aMapFunc);
      }
      else {
        result = NS_NewHTMLAttributes(&aSingleAttrs, this, aMapFunc);
      }
    }
    else {
      aSingleAttrs = nsnull;
    }
    contentRefCount = 0;
  }
  else {
    aSingleAttrs = aAttributes;
    aSingleAttrs->GetContentRefCount(contentRefCount);
    NS_ASSERTION(0 < contentRefCount, "bad content ref count");
  }

  if (NS_OK == result) {
    if (1 < contentRefCount) {  // already shared, copy it
      result = aSingleAttrs->Clone(&aSingleAttrs);
      if (NS_OK != result) {
        aSingleAttrs = nsnull;
        return result;
      }
      contentRefCount = 0;
      aAttributes->ReleaseContentRef();
      NS_RELEASE(aAttributes);
    }
    else {  // one content ref, ok to use, remove from table because hash may change
      if (1 == contentRefCount) {
        AttributeKey  key(aMapFunc, aSingleAttrs);
        mAttrTable.Remove(&key);
        NS_ADDREF(aSingleAttrs); // add a local ref so we match up 
      }
    }
    // at this point, content ref count is 0 or 1, and we hold a local ref
    // attrs is also unique and not in the table
    NS_ASSERTION(((0 == contentRefCount) && (nsnull == aAttributes)) ||
                 ((1 == contentRefCount) && (aSingleAttrs == aAttributes)), "this is broken");
  }
  return result;
}

NS_IMETHODIMP
HTMLStyleSheetImpl::UniqueAttributes(nsIHTMLAttributes*& aSingleAttrs,
                                     nsMapAttributesFunc aMapFunc,
                                     PRInt32 aAttrCount,
                                     nsIHTMLAttributes*& aAttributes)
{
  nsresult result = NS_OK;

  if (0 < aAttrCount) {
    AttributeKey  key(aMapFunc, aSingleAttrs);
    nsIHTMLAttributes* sharedAttrs = (nsIHTMLAttributes*)mAttrTable.Get(&key);
    if (nsnull == sharedAttrs) {  // we have a new unique set
      mAttrTable.Put(&key, aSingleAttrs);
      if (aSingleAttrs != aAttributes) {
        NS_ASSERTION(nsnull == aAttributes, "this is broken");
        aAttributes = aSingleAttrs;
        aAttributes->AddContentRef(); 
        NS_ADDREF(aAttributes); // add ref for content
      }
    }
    else {  // found existing set
      NS_ASSERTION (sharedAttrs != aAttributes, "should never happen");
      if (nsnull != aAttributes) {
        aAttributes->ReleaseContentRef();
        NS_RELEASE(aAttributes);  // release content's ref
      }
      aAttributes = sharedAttrs;
      aAttributes->AddContentRef();
      NS_ADDREF(aAttributes); // add ref for content

      if (nsnull == mRecycledAttrs) {
        mRecycledAttrs = aSingleAttrs;
        NS_ADDREF(mRecycledAttrs);
        mRecycledAttrs->Reset();
      }
    }
  }
  else {  // no attributes to store
    if (nsnull != aAttributes) {
      aAttributes->ReleaseContentRef();
      NS_RELEASE(aAttributes);
    }
    if ((nsnull != aSingleAttrs) && (nsnull == mRecycledAttrs)) {
      mRecycledAttrs = aSingleAttrs;
      NS_ADDREF(mRecycledAttrs);
      mRecycledAttrs->Reset();
    }
  }
  NS_IF_RELEASE(aSingleAttrs);
  return result;
}



NS_IMETHODIMP HTMLStyleSheetImpl::SetAttributeFor(nsIAtom* aAttribute, 
                                                  const nsHTMLValue& aValue,
                                                  nsIHTMLContent* aContent, 
                                                  nsIHTMLAttributes*& aAttributes)
{
  nsresult            result = NS_OK;
  nsIHTMLAttributes*  attrs;
  PRBool              hasValue = PRBool(eHTMLUnit_Null != aValue.GetUnit());
  nsMapAttributesFunc mapFunc;

  aContent->GetAttributeMappingFunction(mapFunc);

  result = EnsureSingleAttributes(aAttributes, mapFunc, hasValue, attrs);

  if ((NS_OK == result) && (nsnull != attrs)) {
    PRInt32 count;
    attrs->SetAttribute(aAttribute, aValue, count);

    result = UniqueAttributes(attrs, mapFunc, count, aAttributes);
  }

  return result;
}

NS_IMETHODIMP HTMLStyleSheetImpl::UnsetAttributeFor(nsIAtom* aAttribute,
                                                    nsIHTMLContent* aContent, 
                                                    nsIHTMLAttributes*& aAttributes)
{
  nsresult            result = NS_OK;
  nsIHTMLAttributes*  attrs;
  nsMapAttributesFunc mapFunc;

  aContent->GetAttributeMappingFunction(mapFunc);

  result = EnsureSingleAttributes(aAttributes, mapFunc, PR_FALSE, attrs);

  if ((NS_OK == result) && (nsnull != attrs)) {
    PRInt32 count;
    attrs->UnsetAttribute(aAttribute, count);

    result = UniqueAttributes(attrs, mapFunc, count, aAttributes);
  }

  return result;

}

/**
 * Request to process the child content elements and create frames.
 *
 * @param   aContent the content object whose child elements to process
 * @param   aFrame the the associated with aContent. This will be the
 *            parent frame (both content and geometric) for the flowed
 *            child frames
 * @param   aState the state information associated with the current process
 * @param   aChildList an OUT parameter. This is the list of flowed child
 *            frames that should go in the principal child list
 */
nsresult
HTMLStyleSheetImpl::ProcessChildren(nsIPresContext*  aPresContext,
                                    nsIContent*      aContent,
                                    nsIFrame*        aFrame,
                                    nsAbsoluteItems& aAbsoluteItems,
                                    nsIFrame*&       aChildList)
{
  // Initialize OUT parameter
  aChildList = nsnull;

  // Iterate the child content objects and construct a frame
  nsIFrame* lastChildFrame = nsnull;
  PRInt32   count;

  aContent->ChildCount(count);
  for (PRInt32 i = 0; i < count; i++) {
    nsIContent* childContent;
    aContent->ChildAt(i, childContent);

    if (nsnull != childContent) {
      nsIFrame* childFrame;

      // Construct a child frame
      ConstructFrame(aPresContext, childContent, aFrame, aAbsoluteItems, childFrame);
      NS_RELEASE(childContent);

      if (nsnull != childFrame) {
        // Append the child frame to the list of child frames
        if (nsnull == aChildList) {
          aChildList = childFrame;
        } else {
          lastChildFrame->SetNextSibling(childFrame);
        }
        lastChildFrame = childFrame;
      }
    }
  }

  return NS_OK;
}

nsresult
HTMLStyleSheetImpl::CreateInputFrame(nsIContent* aContent, nsIFrame*& aFrame)
{
  nsresult  rv;

  // Figure out which type of input frame to create
  nsAutoString  val;
  if (NS_OK == aContent->GetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::type, val)) {
    if (val.EqualsIgnoreCase("submit")) {
      rv = NS_NewButtonControlFrame(aFrame);
    }
    else if (val.EqualsIgnoreCase("reset")) {
      rv = NS_NewButtonControlFrame(aFrame);
    }
    else if (val.EqualsIgnoreCase("button")) {
      rv = NS_NewButtonControlFrame(aFrame);
    }
    else if (val.EqualsIgnoreCase("checkbox")) {
      rv = NS_NewCheckboxControlFrame(aFrame);
    }
    else if (val.EqualsIgnoreCase("file")) {
      rv = NS_NewFileControlFrame(aFrame);
    }
    else if (val.EqualsIgnoreCase("hidden")) {
      rv = NS_NewButtonControlFrame(aFrame);
    }
    else if (val.EqualsIgnoreCase("image")) {
      rv = NS_NewImageControlFrame(aFrame);
    }
    else if (val.EqualsIgnoreCase("password")) {
      rv = NS_NewTextControlFrame(aFrame);
    }
    else if (val.EqualsIgnoreCase("radio")) {
      rv = NS_NewRadioControlFrame(aFrame);
    }
    else if (val.EqualsIgnoreCase("text")) {
      rv = NS_NewTextControlFrame(aFrame);
    }
    else {
      rv = NS_NewTextControlFrame(aFrame);
    }
  } else {
    rv = NS_NewTextControlFrame(aFrame);
  }

  return rv;
}

nsresult
HTMLStyleSheetImpl::ConstructTableFrame(nsIPresContext*  aPresContext,
                                        nsIContent*      aContent,
                                        nsIFrame*        aParentFrame,
                                        nsIStyleContext* aStyleContext,
                                        nsAbsoluteItems& aAbsoluteItems,
                                        nsIFrame*&       aNewFrame)
{
  nsIFrame* childList;
  nsIFrame* innerFrame;
  nsIFrame* innerChildList = nsnull;
  nsIFrame* captionFrame = nsnull;

  // Create an anonymous table outer frame which holds the caption and the
  // table frame
  NS_NewTableOuterFrame(aNewFrame);

  // Init the table outer frame and see if we need to create a view, e.g.
  // the frame is absolutely positioned
  aNewFrame->Init(*aPresContext, aContent, aParentFrame, aParentFrame, aStyleContext);
  nsHTMLContainerFrame::CreateViewForFrame(*aPresContext, aNewFrame,
                                           aStyleContext, PR_FALSE);

  // Create the inner table frame
  NS_NewTableFrame(innerFrame);
  childList = innerFrame;

  // Have the inner table frame use a pseudo style context based on the outer table frame's
  /* XXX: comment this back in and use this as the inner table's p-style asap
  nsIStyleContext *innerTableStyleContext = 
    aPresContext->ResolvePseudoStyleContextFor (aContent, 
                                                nsHTMLAtoms::tablePseudo,
                                                aStyleContext);
  */
  innerFrame->Init(*aPresContext, aContent, aNewFrame, aNewFrame, aStyleContext);
  // this should be "innerTableStyleContext" but I haven't tested that thoroughly yet

  // Iterate the child content
  nsIFrame* lastChildFrame = nsnull;
  PRInt32   count;
  aContent->ChildCount(count);
  for (PRInt32 i = 0; i < count; i++) {
    nsIFrame* grandChildList=nsnull;  // to be used only when pseudoframes need to be created
    nsIContent* childContent;
    aContent->ChildAt(i, childContent);

    if (nsnull != childContent) {
      nsIFrame*         frame = nsnull;
      nsIStyleContext*  childStyleContext;

      // Resolve the style context
      childStyleContext = aPresContext->ResolveStyleContextFor(childContent, aStyleContext);

      // See how it should be displayed
      const nsStyleDisplay* styleDisplay = (const nsStyleDisplay*)
        childStyleContext->GetStyleData(eStyleStruct_Display);

      switch (styleDisplay->mDisplay) {
      case NS_STYLE_DISPLAY_TABLE_CAPTION:
        // Have we already created a caption? If so, ignore this caption
        if (nsnull == captionFrame) {
          NS_NewAreaFrame(captionFrame, NS_BODY_NO_AUTO_MARGINS);
          captionFrame->Init(*aPresContext, childContent, aNewFrame,
                             aNewFrame, childStyleContext);
          // Process the caption's child content and set the initial child list
          nsIFrame* captionChildList;
          ProcessChildren(aPresContext, childContent, captionFrame,
                          aAbsoluteItems, captionChildList);
          captionFrame->SetInitialChildList(*aPresContext, nsnull, captionChildList);

          // Prepend the caption frame to the outer frame's child list
          innerFrame->SetNextSibling(captionFrame);
        }
        break;

      case NS_STYLE_DISPLAY_TABLE_HEADER_GROUP:
      case NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP:
      case NS_STYLE_DISPLAY_TABLE_ROW_GROUP:
        NS_NewTableRowGroupFrame(frame);
        frame->Init(*aPresContext, childContent, innerFrame, innerFrame,
                    childStyleContext);
        break;

      case NS_STYLE_DISPLAY_TABLE_ROW:
      {
        // When we're done here, "frame" will be the pseudo rowgroup frame and will be dealt with normally after the swtich.
        // The row itself will have already been dealt with
        nsIStyleContext*  rowStyleContext;
        nsIStyleContext*  rowGroupStyleContext;
        rowGroupStyleContext = aPresContext->ResolvePseudoStyleContextFor (childContent, 
                                                                           nsHTMLAtoms::columnPseudo,
                                                                           aStyleContext);
        nsStyleDisplay *rowGroupDisplay = (nsStyleDisplay *)rowGroupStyleContext->GetMutableStyleData(eStyleStruct_Display);
        rowGroupDisplay->mDisplay = NS_STYLE_DISPLAY_TABLE_ROW_GROUP;
        NS_NewTableRowGroupFrame(frame);
        NS_RELEASE(childStyleContext); // we can't use this resolved style, so get rid of it
        childStyleContext = rowGroupStyleContext;  // the row group style context will get set to childStyleContext after the switch ends.
        frame->Init(*aPresContext, childContent, innerFrame,
                    innerFrame, childStyleContext);
        // need to resolve the style context for the column again to be sure it's a child of the colgroup style context
        rowStyleContext = aPresContext->ResolveStyleContextFor(childContent, rowGroupStyleContext);
        nsIFrame *rowFrame;
        NS_NewTableRowFrame(rowFrame);
        rowFrame->Init(*aPresContext, childContent, frame, frame, rowStyleContext); 
        rowFrame->SetInitialChildList(*aPresContext, nsnull, nsnull);
        grandChildList = rowFrame;
        break;
      }
/*
      case NS_STYLE_DISPLAY_TABLE_CELL:
      {
        // When we're done here, "frame" will be the pseudo rowgroup frame and will be dealt with normally after the swtich.
        // The row itself will have already been dealt with
        nsIStyleContext*  cellStyleContext;
        nsIStyleContext*  rowStyleContext;
        nsIStyleContext*  rowGroupStyleContext;
        rowGroupStyleContext = aPresContext->ResolvePseudoStyleContextFor (childContent, 
                                                                           nsHTMLAtoms::columnPseudo,
                                                                           aStyleContext);
        nsStyleDisplay *rowGroupDisplay = (nsStyleDisplay *)rowGroupStyleContext->GetMutableStyleData(eStyleStruct_Display);
        rowGroupDisplay->mDisplay = NS_STYLE_DISPLAY_TABLE_ROW_GROUP;
        NS_NewTableRowGroupFrame(childContent, innerFrame, frame);
        NS_RELEASE(childStyleContext); // we can't use this resolved style, so get rid of it
        childStyleContext = rowGroupStyleContext;  // the row group style context will get set to childStyleContext after the switch ends.
        // need to resolve the style context for the column again to be sure it's a child of the colgroup style context
        rowStyleContext = aPresContext->ResolveStyleContextFor(childContent, rowGroupStyleContext);
        nsIFrame *rowFrame;
        NS_NewTableRowFrame(childContent, frame, rowFrame);
        rowFrame->SetInitialChildList(*aPresContext, nsnull, nsnull);
        rowFrame->SetStyleContext(aPresContext, rowStyleContext); 
        grandChildList = rowFrame;
        break;
      }
*/
      case NS_STYLE_DISPLAY_TABLE_COLUMN:
      {
        //XXX: Peter -  please code review and remove this comment when all is well.
        // When we're done here, "frame" will be the pseudo colgroup frame and will be dealt with normally after the swtich.
        // The column itself will have already been dealt with
        nsIStyleContext*  colStyleContext;
        nsIStyleContext*  colGroupStyleContext;
        colGroupStyleContext = aPresContext->ResolvePseudoStyleContextFor (childContent, 
                                                                           nsHTMLAtoms::columnPseudo,
                                                                           aStyleContext);
        nsStyleDisplay *colGroupDisplay = (nsStyleDisplay *)colGroupStyleContext->GetMutableStyleData(eStyleStruct_Display);
        colGroupDisplay->mDisplay = NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP;
        NS_NewTableColGroupFrame(frame);
        NS_RELEASE(childStyleContext);  // we can't use this resolved style, so get rid of it
        childStyleContext = colGroupStyleContext;  // the col group style context will get set to childStyleContext after the switch ends.
        frame->Init(*aPresContext, childContent, innerFrame, innerFrame, childStyleContext);
        // need to resolve the style context for the column again to be sure it's a child of the colgroup style context
        colStyleContext = aPresContext->ResolveStyleContextFor(childContent, colGroupStyleContext);
        nsIFrame *colFrame;
        NS_NewTableColFrame(colFrame);
        colFrame->Init(*aPresContext, childContent, frame, frame, colStyleContext); 
        colFrame->SetInitialChildList(*aPresContext, nsnull, nsnull);
        grandChildList = colFrame;
        break;
      }

      case NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP:
        NS_NewTableColGroupFrame(frame);
        frame->Init(*aPresContext, childContent, innerFrame, innerFrame, childStyleContext);
        break;

      default:
        // XXX For the time being ignore everything else. We should deal with
        // things like table cells and create anonymous frames...
        break;
      }

      // If it's not a caption frame, then link the frame into the inner
      // frame's child list
      if (nsnull != frame) {
        // Process the children, and set the frame's initial child list
        nsIFrame* childChildList;
        if (nsnull==grandChildList) {
          ProcessChildren(aPresContext, childContent, frame, aAbsoluteItems,
                          childChildList);
          grandChildList = childChildList;
        } else {
          ProcessChildren(aPresContext, childContent, grandChildList,
                          aAbsoluteItems, childChildList);
          grandChildList->SetInitialChildList(*aPresContext, nsnull, childChildList);
        }
        frame->SetInitialChildList(*aPresContext, nsnull, grandChildList);
  
        // Link the frame into the child list
        if (nsnull == lastChildFrame) {
          innerChildList = frame;
        } else {
          lastChildFrame->SetNextSibling(frame);
        }
        lastChildFrame = frame;
      }

      NS_RELEASE(childStyleContext);
      NS_RELEASE(childContent);
    }
  }

  // Set the inner table frame's list of initial child frames
  innerFrame->SetInitialChildList(*aPresContext, nsnull, innerChildList);

  // Set the anonymous table outer frame's initial child list
  aNewFrame->SetInitialChildList(*aPresContext, nsnull, childList);
  return NS_OK;
}

nsresult
HTMLStyleSheetImpl::ConstructTableCellFrame(nsIPresContext*  aPresContext,
                                            nsIContent*      aContent,
                                            nsIFrame*        aParentFrame,
                                            nsIStyleContext* aStyleContext,
                                            nsAbsoluteItems& aAbsoluteItems,
                                            nsIFrame*&       aNewFrame)
{
  nsresult  rv;

  // Create a table cell frame
  rv = NS_NewTableCellFrame(aNewFrame);
  if (NS_SUCCEEDED(rv)) {
    // Initialize the table cell frame
    aNewFrame->Init(*aPresContext, aContent, aParentFrame, aParentFrame, aStyleContext);

    // Create an area frame that will format the cell's content
    nsIFrame*   cellBodyFrame;

    rv = NS_NewAreaFrame(cellBodyFrame, NS_BODY_NO_AUTO_MARGINS);
    if (NS_FAILED(rv)) {
      aNewFrame->DeleteFrame(*aPresContext);
      aNewFrame = nsnull;
      return rv;
    }
  
    // Resolve pseudo style and initialize the body cell frame
    nsIStyleContext*  bodyPseudoStyle = aPresContext->ResolvePseudoStyleContextFor(aContent,
                                          nsHTMLAtoms::cellContentPseudo, aStyleContext);
    cellBodyFrame->Init(*aPresContext, aContent, aNewFrame, aNewFrame, bodyPseudoStyle);
    NS_RELEASE(bodyPseudoStyle);

    // Process children and set the body cell frame's initial child list
    nsIFrame* childList;
    rv = ProcessChildren(aPresContext, aContent, cellBodyFrame, aAbsoluteItems,
                         childList);
    if (NS_SUCCEEDED(rv)) {
      cellBodyFrame->SetInitialChildList(*aPresContext, nsnull, childList);
    }

    // Set the table cell frame's initial child list
    aNewFrame->SetInitialChildList(*aPresContext, nsnull, cellBodyFrame);
  }

  return rv;
}

nsresult
HTMLStyleSheetImpl::ConstructDocElementFrame(nsIPresContext*  aPresContext,
                                             nsIContent*      aDocElement,
                                             nsIFrame*        aRootFrame,
                                             nsIStyleContext* aRootStyleContext,
                                             nsIFrame*&       aNewFrame)
{
  // See if we're paginated
  if (aPresContext->IsPaginated()) {
    nsIFrame* scrollFrame;
    nsIFrame* pageSequenceFrame;

    // Create a page sequence frame and wrap it in a scroll frame. Let the
    // scroll frame and page sequence frame share the same style context as
    // the root frame.
    // XXX We only need the scroll frame if it's print preview...
    NS_NewScrollFrame(scrollFrame);
    scrollFrame->Init(*aPresContext, nsnull, aRootFrame, aRootFrame,
                      aRootStyleContext);

    // The page sequence frame needs a view, because it's a scrolled frame
    NS_NewSimplePageSequenceFrame(pageSequenceFrame);
    pageSequenceFrame->Init(*aPresContext, nsnull, scrollFrame, scrollFrame,
                            aRootStyleContext);
    nsHTMLContainerFrame::CreateViewForFrame(*aPresContext, pageSequenceFrame,
                                             aRootStyleContext, PR_TRUE);

    // Create the first page
    nsIFrame* pageFrame;
    NS_NewPageFrame(pageFrame);

    // Initialize the page and force it to have a view. This makes printing of
    // the pages easier and faster.
    // XXX Use a PAGE style context...
    pageFrame->Init(*aPresContext, nsnull, pageSequenceFrame, pageSequenceFrame,
                    aRootStyleContext);
    nsHTMLContainerFrame::CreateViewForFrame(*aPresContext, pageFrame,
                                             aRootStyleContext, PR_TRUE);

    // Resolve the style context for the document element
    nsIStyleContext*  styleContext;
    styleContext = aPresContext->ResolveStyleContextFor(aDocElement, aRootStyleContext);

    // Create an area frame for the document element
    nsIFrame* areaFrame;

    NS_NewAreaFrame(areaFrame, 0);
    areaFrame->Init(*aPresContext, aDocElement, pageFrame, pageFrame, styleContext);
    NS_RELEASE(styleContext);

    // The area frame is the "initial containing block"
    mInitialContainingBlock = areaFrame;

    // Process the child content
    nsAbsoluteItems  absoluteItems(areaFrame);
    nsIFrame*        childList;

    ProcessChildren(aPresContext, aDocElement, areaFrame, absoluteItems, childList);
    
    // Set the initial child lists
    areaFrame->SetInitialChildList(*aPresContext, nsnull, childList);
    if (nsnull != absoluteItems.childList) {
      areaFrame->SetInitialChildList(*aPresContext, nsLayoutAtoms::absoluteList,
                                     absoluteItems.childList);
    }
    pageFrame->SetInitialChildList(*aPresContext, nsnull, areaFrame);
    pageSequenceFrame->SetInitialChildList(*aPresContext, nsnull, pageFrame);
    scrollFrame->SetInitialChildList(*aPresContext, nsnull, pageSequenceFrame);

    // Return the scroll frame as the frame sub-tree
    aNewFrame = scrollFrame;
  
  } else {
    // Resolve the style context for the document element
    nsIStyleContext*  styleContext;
    styleContext = aPresContext->ResolveStyleContextFor(aDocElement, aRootStyleContext);
  
    // Unless the 'overflow' policy forbids scrolling, wrap the frame in a
    // scroll frame.
    nsIFrame*             scrollFrame = nsnull;
    PRInt32               scrolling = -1;
    const nsStyleDisplay* display = (const nsStyleDisplay*)
      styleContext->GetStyleData(eStyleStruct_Display);
  
    // XXX Check the webshell and see if scrolling is enabled there. This needs
    // to go away...
    nsISupports* container;
    if (nsnull != aPresContext) {
      aPresContext->GetContainer(&container);
      if (nsnull != container) {
        nsIWebShell* webShell = nsnull;
        container->QueryInterface(kIWebShellIID, (void**) &webShell);
        if (nsnull != webShell) {
          webShell->GetScrolling(scrolling);
          NS_RELEASE(webShell);
        }
        NS_RELEASE(container);
      }
    }
    if (-1 == scrolling) {
      scrolling = display->mOverflow;
    }
  
    if (NS_STYLE_OVERFLOW_HIDDEN != scrolling) {
      NS_NewScrollFrame(scrollFrame);
      scrollFrame->Init(*aPresContext, aDocElement, aRootFrame, aRootFrame,
                        styleContext);
  
      // The scrolled frame gets a pseudo element style context
      nsIStyleContext*  scrolledPseudoStyle =
        aPresContext->ResolvePseudoStyleContextFor(nsnull,
                                                   nsHTMLAtoms::scrolledContentPseudo,
                                                   styleContext);
      NS_RELEASE(styleContext);
      styleContext = scrolledPseudoStyle;
    }

    // Create an area frame for the document element. This serves as the
    // "initial containing block"
    nsIFrame* areaFrame;

    // XXX Until we clean up how painting damage is handled, we need to use the
    // flag that says that this is the body...
    NS_NewAreaFrame(areaFrame, NS_BODY_THE_BODY);
    nsIFrame* parentFrame = scrollFrame ? scrollFrame : aRootFrame;
    areaFrame->Init(*aPresContext, aDocElement, parentFrame, parentFrame,
                    styleContext);
    nsHTMLContainerFrame::CreateViewForFrame(*aPresContext, areaFrame,
                                             styleContext, PR_FALSE);
    NS_RELEASE(styleContext);

    // The area frame is the "initial containing block"
    mInitialContainingBlock = areaFrame;
    
    // Process the child content
    nsAbsoluteItems  absoluteItems(areaFrame);
    nsIFrame*        childList;

    ProcessChildren(aPresContext, aDocElement, areaFrame, absoluteItems, childList);
    
    // Set the initial child lists
    areaFrame->SetInitialChildList(*aPresContext, nsnull, childList);
    if (nsnull != absoluteItems.childList) {
      areaFrame->SetInitialChildList(*aPresContext, nsLayoutAtoms::absoluteList,
                                     absoluteItems.childList);
    }
    if (nsnull != scrollFrame) {
      scrollFrame->SetInitialChildList(*aPresContext, nsnull, areaFrame);
    }

    aNewFrame = scrollFrame ? scrollFrame : areaFrame;
  }

  return NS_OK;
}

NS_IMETHODIMP
HTMLStyleSheetImpl::ConstructRootFrame(nsIPresContext* aPresContext,
                                       nsIContent*     aDocElement,
                                       nsIFrame*&      aNewFrame)
{
#ifdef NS_DEBUG
    nsIDocument*  doc;
    nsIContent*   rootContent;

    // Verify that the content object is really the root content object
    aDocElement->GetDocument(doc);
    rootContent = doc->GetRootContent();
    NS_RELEASE(doc);
    NS_ASSERTION(rootContent == aDocElement, "unexpected content");
    NS_RELEASE(rootContent);
#endif

  nsIFrame*         rootFrame;
  nsIStyleContext*  rootPseudoStyle;

  // Create the root frame
  NS_NewRootFrame(rootFrame);
  
  // Create a pseudo element style context
  rootPseudoStyle = aPresContext->ResolvePseudoStyleContextFor(nsnull, 
                      nsHTMLAtoms::rootPseudo, nsnull);

  // Initialize the root frame. It has a NULL content object
  rootFrame->Init(*aPresContext, nsnull, nsnull, nsnull, rootPseudoStyle);

  // Bind the root frame to the root view
  nsIPresShell*   presShell = aPresContext->GetShell();
  nsIViewManager* viewManager = presShell->GetViewManager();
  nsIView*        rootView;
  
  NS_RELEASE(presShell);
  viewManager->GetRootView(rootView);
  rootFrame->SetView(rootView);
  NS_RELEASE(viewManager);
  
  // Create frames for the document element and its child elements
  nsIFrame* docElementFrame;
  ConstructDocElementFrame(aPresContext, aDocElement, rootFrame,
                           rootPseudoStyle, docElementFrame);
  NS_RELEASE(rootPseudoStyle);

  // Set the root frame's initial child list
  rootFrame->SetInitialChildList(*aPresContext, nsnull, docElementFrame);

  aNewFrame = rootFrame;
  return NS_OK;  
}

nsresult
HTMLStyleSheetImpl::CreatePlaceholderFrameFor(nsIPresContext*  aPresContext,
                                              nsIContent*      aContent,
                                              nsIFrame*        aFrame,
                                              nsIStyleContext* aStyleContext,
                                              nsIFrame*        aParentFrame,
                                              nsIFrame*&       aPlaceholderFrame)
{
  nsIFrame* placeholderFrame;
  nsresult  rv;

  rv = NS_NewEmptyFrame(&placeholderFrame);

  if (NS_SUCCEEDED(rv)) {
    // The placeholder frame gets a pseudo style context
    nsIStyleContext*  placeholderPseudoStyle =
      aPresContext->ResolvePseudoStyleContextFor(aContent,
                      nsHTMLAtoms::placeholderPseudo, aStyleContext);
    placeholderFrame->Init(*aPresContext, aContent, aParentFrame,
                           aParentFrame, placeholderPseudoStyle);
    NS_RELEASE(placeholderPseudoStyle);
  
    // Add mapping from absolutely positioned frame to its placeholder frame
    nsIPresShell* presShell = aPresContext->GetShell();
    presShell->SetPlaceholderFrameFor(aFrame, placeholderFrame);
    NS_RELEASE(presShell);
  
    aPlaceholderFrame = placeholderFrame;
  }

  return rv;
}

nsresult
HTMLStyleSheetImpl::ConstructFrameByTag(nsIPresContext*  aPresContext,
                                        nsIContent*      aContent,
                                        nsIFrame*        aParentFrame,
                                        nsIAtom*         aTag,
                                        nsIStyleContext* aStyleContext,
                                        nsAbsoluteItems& aAbsoluteItems,
                                        nsIFrame*&       aNewFrame)
{
  PRBool    processChildren = PR_FALSE;  // whether we should process child content
  nsresult  rv = NS_OK;
  PRBool    isAbsolutelyPositioned = PR_FALSE;

  // Initialize OUT parameter
  aNewFrame = nsnull;

  if (nsnull == aTag) {
    rv = NS_NewTextFrame(aNewFrame);
  }
  else {
    nsIHTMLContent *htmlContent;

    // Ignore the tag if it's not HTML content
    rv = aContent->QueryInterface(kIHTMLContentIID, (void **)&htmlContent);
    if (NS_SUCCEEDED(rv)) {
      // See if the element is absolutely positioned
      const nsStylePosition* position = (const nsStylePosition*)
        aStyleContext->GetStyleData(eStyleStruct_Position);
      if (NS_STYLE_POSITION_ABSOLUTE == position->mPosition) {
        isAbsolutelyPositioned = PR_TRUE;
      }

      // Create a frame based on the tag
      if (nsHTMLAtoms::img == aTag) {
        rv = NS_NewImageFrame(aNewFrame);
      }
      else if (nsHTMLAtoms::hr == aTag) {
        rv = NS_NewHRFrame(aNewFrame);
      }
      else if (nsHTMLAtoms::br == aTag) {
        rv = NS_NewBRFrame(aNewFrame);
      }
      else if (nsHTMLAtoms::wbr == aTag) {
        rv = NS_NewWBRFrame(aNewFrame);
      }
      else if (nsHTMLAtoms::input == aTag) {
        rv = CreateInputFrame(aContent, aNewFrame);
      }
      else if (nsHTMLAtoms::textarea == aTag) {
        rv = NS_NewTextControlFrame(aNewFrame);
      }
      else if (nsHTMLAtoms::select == aTag) {
        rv = NS_NewSelectControlFrame(aNewFrame);
      }
      else if (nsHTMLAtoms::applet == aTag) {
        rv = NS_NewObjectFrame(aNewFrame);
      }
      else if (nsHTMLAtoms::embed == aTag) {
        rv = NS_NewObjectFrame(aNewFrame);
      }
      else if (nsHTMLAtoms::fieldset == aTag) {
        rv = NS_NewFieldSetFrame(aNewFrame);
        processChildren = PR_TRUE;
      }
      else if (nsHTMLAtoms::legend == aTag) {
        rv = NS_NewLegendFrame(aNewFrame);
        processChildren = PR_TRUE;
        isAbsolutelyPositioned = PR_FALSE;  // don't absolutely position
      }
      else if (nsHTMLAtoms::object == aTag) {
        rv = NS_NewObjectFrame(aNewFrame);
        //rv = NS_NewObjectFrame(aContent, aParentFrame, aNewFrame);
        nsIFrame *blockFrame;
        NS_NewBlockFrame(blockFrame, 0);
        blockFrame->Init(*aPresContext, aContent, aNewFrame, aNewFrame,
                         aStyleContext);
        aNewFrame = blockFrame;
        processChildren = PR_TRUE;
      }
      else if (nsHTMLAtoms::form == aTag) {
        rv = NS_NewFormFrame(aNewFrame);
      }
      else if (nsHTMLAtoms::frameset == aTag) {
        rv = NS_NewHTMLFramesetFrame(aNewFrame);
        isAbsolutelyPositioned = PR_FALSE;  // don't absolutely position
      }
      else if (nsHTMLAtoms::iframe == aTag) {
        rv = NS_NewHTMLFrameOuterFrame(aNewFrame);
      }
      else if (nsHTMLAtoms::spacer == aTag) {
        rv = NS_NewSpacerFrame(aNewFrame);
        isAbsolutelyPositioned = PR_FALSE;  // don't absolutely position
      }
      else if (nsHTMLAtoms::button == aTag) {
        rv = NS_NewHTMLButtonControlFrame(aNewFrame);
        processChildren = PR_TRUE;
      }
      else if (nsHTMLAtoms::label == aTag) {
        rv = NS_NewLabelFrame(aNewFrame);
        processChildren = PR_TRUE;
      }
      NS_RELEASE(htmlContent);
    }
  }

  // If we succeeded in creating a frame then initialize it, process its
  // children (if requested), and set the initial child list
  if (NS_SUCCEEDED(rv) && (nsnull != aNewFrame)) {
    nsIFrame* geometricParent = isAbsolutelyPositioned ? aAbsoluteItems.containingBlock :
                                                         aParentFrame;
    aNewFrame->Init(*aPresContext, aContent, geometricParent, aParentFrame,
                    aStyleContext);

    // See if we need to create a view, e.g. the frame is absolutely positioned
    nsHTMLContainerFrame::CreateViewForFrame(*aPresContext, aNewFrame,
                                             aStyleContext, PR_FALSE);

    // Process the child content if requested
    nsIFrame* childList = nsnull;
    if (processChildren) {
      rv = ProcessChildren(aPresContext, aContent, aNewFrame, aAbsoluteItems,
                           childList);
    }

    // Set the frame's initial child list
    aNewFrame->SetInitialChildList(*aPresContext, nsnull, childList);

    // If the frame is absolutely positioned then create a placeholder frame
    if (isAbsolutelyPositioned) {
      nsIFrame* placeholderFrame;

      CreatePlaceholderFrameFor(aPresContext, aContent, aNewFrame, aStyleContext,
                                aParentFrame, placeholderFrame);

      // Add the absolutely positioned frame to its containing block's list
      // of child frames
      aAbsoluteItems.AddAbsolutelyPositionedChild(aNewFrame);

      // Add the placeholder frame to the flow
      aNewFrame = placeholderFrame;
    }
  }

  return rv;
}

nsresult
HTMLStyleSheetImpl::ConstructFrameByDisplayType(nsIPresContext*       aPresContext,
                                                const nsStyleDisplay* aDisplay,
                                                nsIContent*           aContent,
                                                nsIFrame*             aParentFrame,
                                                nsIStyleContext*      aStyleContext,
                                                nsAbsoluteItems&      aAbsoluteItems,
                                                nsIFrame*&            aNewFrame)
{
  const nsStylePosition* position = (const nsStylePosition*)
                           aStyleContext->GetStyleData(eStyleStruct_Position);
  PRBool                 isAbsolutelyPositioned = PR_FALSE;
  PRBool                 isBlock = aDisplay->IsBlockLevel();
  nsresult               rv = NS_OK;

  // Initialize OUT parameter
  aNewFrame = nsnull;

  // The frame is also a block if it's an inline frame that's floated or
  // absolutely positioned
  if ((NS_STYLE_DISPLAY_INLINE == aDisplay->mDisplay) &&
      ((NS_STYLE_FLOAT_NONE != aDisplay->mFloats) ||
       (NS_STYLE_POSITION_ABSOLUTE == position->mPosition))) {
    isBlock = PR_TRUE;
  }

  // If the frame is a block-level frame and is scrollable then wrap it
  // in a scroll frame.
  // XXX Applies to replaced elements, too, but how to tell if the element
  // is replaced?
  // XXX Ignore tables for the time being
  if ((isBlock && (aDisplay->mDisplay != NS_STYLE_DISPLAY_TABLE)) &&
      IsScrollable(aPresContext, aDisplay)) {

    // See if it's absolutely positioned
    isAbsolutelyPositioned = NS_STYLE_POSITION_ABSOLUTE == position->mPosition;

    // Create a scroll frame
    nsIFrame* scrollFrame;
    NS_NewScrollFrame(scrollFrame);

    // Initialize it
    nsIFrame* geometricParent = isAbsolutelyPositioned ? aAbsoluteItems.containingBlock :
                                                         aParentFrame;
    scrollFrame->Init(*aPresContext, aContent, geometricParent, aParentFrame,
                      aStyleContext);

    // The scroll frame gets the original style context, and the scrolled
    // frame gets a SCROLLED-CONTENT pseudo element style context that
    // inherits the background properties
    nsIStyleContext*  scrolledPseudoStyle = aPresContext->ResolvePseudoStyleContextFor
                        (aContent, nsHTMLAtoms::scrolledContentPseudo, aStyleContext);

    // Create an area container for the frame
    nsIFrame* scrolledFrame;
    NS_NewAreaFrame(scrolledFrame, NS_BODY_SHRINK_WRAP);

    // Initialize the frame and force it to have a view
    scrolledFrame->Init(*aPresContext, aContent, scrollFrame, scrollFrame,
                       scrolledPseudoStyle);
    nsHTMLContainerFrame::CreateViewForFrame(*aPresContext, scrolledFrame,
                                             scrolledPseudoStyle, PR_TRUE);
    NS_RELEASE(scrolledPseudoStyle);

    // Process children
    if (isAbsolutelyPositioned) {
      nsAbsoluteItems  absoluteItems(scrolledFrame);
      nsIFrame*        childList;
      ProcessChildren(aPresContext, aContent, scrolledFrame, absoluteItems,
                      childList);
  
      // Set the initial child lists
      scrolledFrame->SetInitialChildList(*aPresContext, nsnull, childList);
      if (nsnull != absoluteItems.childList) {
        scrolledFrame->SetInitialChildList(*aPresContext, nsLayoutAtoms::absoluteList,
                                           absoluteItems.childList);
      }

    } else {
      nsIFrame* childList;
      ProcessChildren(aPresContext, aContent, scrolledFrame, aAbsoluteItems,
                      childList);
  
      // Set the initial child lists
      scrolledFrame->SetInitialChildList(*aPresContext, nsnull, childList);
    }
    scrollFrame->SetInitialChildList(*aPresContext, nsnull, scrolledFrame);
    aNewFrame = scrollFrame;

  // See if the frame is absolutely positioned
  } else if ((NS_STYLE_POSITION_ABSOLUTE == position->mPosition) &&
             ((NS_STYLE_DISPLAY_BLOCK == aDisplay->mDisplay) ||
              (NS_STYLE_DISPLAY_INLINE == aDisplay->mDisplay) ||
              (NS_STYLE_DISPLAY_LIST_ITEM == aDisplay->mDisplay))) {

    isAbsolutelyPositioned = PR_TRUE;

    // Create an area frame
    NS_NewAreaFrame(aNewFrame, NS_BODY_SHRINK_WRAP);
    aNewFrame->Init(*aPresContext, aContent, aAbsoluteItems.containingBlock,
                    aParentFrame, aStyleContext);

    // Create a view
    nsHTMLContainerFrame::CreateViewForFrame(*aPresContext, aNewFrame,
                                             aStyleContext, PR_FALSE);

    // Process the child content
    nsAbsoluteItems  absoluteItems(aNewFrame);
    nsIFrame*        childList = nsnull;
    ProcessChildren(aPresContext, aContent, aNewFrame, absoluteItems, childList);

    // Set the frame's initial child list
    aNewFrame->SetInitialChildList(*aPresContext, nsnull, childList);
    if (nsnull != absoluteItems.childList) {
      aNewFrame->SetInitialChildList(*aPresContext, nsLayoutAtoms::absoluteList,
                                     absoluteItems.childList);
    }

  // See if the frame is floated, and it's a block or inline frame
  } else if ((NS_STYLE_FLOAT_NONE != aDisplay->mFloats) &&
             ((NS_STYLE_DISPLAY_BLOCK == aDisplay->mDisplay) ||
              (NS_STYLE_DISPLAY_INLINE == aDisplay->mDisplay) ||
              (NS_STYLE_DISPLAY_LIST_ITEM == aDisplay->mDisplay))) {

    // Create an area frame
    NS_NewAreaFrame(aNewFrame, NS_BODY_SHRINK_WRAP);

    // Initialize the frame
    aNewFrame->Init(*aPresContext, aContent, aParentFrame, aParentFrame,
                    aStyleContext);

    // See if we need to create a view
    nsHTMLContainerFrame::CreateViewForFrame(*aPresContext, aNewFrame,
                                             aStyleContext, PR_FALSE);

    // Process the child content
    nsIFrame* childList = nsnull;
    ProcessChildren(aPresContext, aContent, aNewFrame, aAbsoluteItems, childList);

    // Set the frame's initial child list
    aNewFrame->SetInitialChildList(*aPresContext, nsnull, childList);

  } else {
    PRBool  processChildren = PR_FALSE;  // whether we should process child content

    // Use the 'display' property to chose a frame type
    switch (aDisplay->mDisplay) {
    case NS_STYLE_DISPLAY_BLOCK:
    case NS_STYLE_DISPLAY_LIST_ITEM:
    case NS_STYLE_DISPLAY_RUN_IN:
    case NS_STYLE_DISPLAY_COMPACT:
      rv = NS_NewBlockFrame(aNewFrame, 0);
      processChildren = PR_TRUE;
      break;
  
    case NS_STYLE_DISPLAY_INLINE:
      rv = NS_NewInlineFrame(aNewFrame);
      processChildren = PR_TRUE;
      break;
  
    case NS_STYLE_DISPLAY_TABLE:
      rv = ConstructTableFrame(aPresContext, aContent, aParentFrame, aStyleContext,
                               aAbsoluteItems, aNewFrame);
      // Note: table construction function takes care of initializing the frame,
      // processing children, and setting the initial child list
      return rv;
  
    case NS_STYLE_DISPLAY_TABLE_ROW_GROUP:
    case NS_STYLE_DISPLAY_TABLE_HEADER_GROUP:
    case NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP:
      // XXX We should check for being inside of a table. If there's a missing
      // table then create an anonynmous table frame
      // XXX: see ConstructTableFrame for a prototype of how this should be done,
      //      and propagate similar logic to other table elements
      {
        nsIFrame *parentFrame;
        rv = GetAdjustedParentFrame(aParentFrame, aDisplay->mDisplay, parentFrame);
        if (NS_SUCCEEDED(rv))
        {
          rv = NS_NewTableRowGroupFrame(aNewFrame);
          processChildren = PR_TRUE;
        }
      }
      break;
  
    case NS_STYLE_DISPLAY_TABLE_COLUMN:
      // XXX We should check for being inside of a table column group...
      rv = NS_NewTableColFrame(aNewFrame);
      processChildren = PR_TRUE;
      break;
  
    case NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP:
      // XXX We should check for being inside of a table...
      {
        nsIFrame *parentFrame;
        rv = GetAdjustedParentFrame(aParentFrame, aDisplay->mDisplay, parentFrame);
        if (NS_SUCCEEDED(rv))
        {
          rv = NS_NewTableColGroupFrame(aNewFrame);
          processChildren = PR_TRUE;
        }
      }
      break;
  
    case NS_STYLE_DISPLAY_TABLE_ROW:
      // XXX We should check for being inside of a table row group...
      rv = NS_NewTableRowFrame(aNewFrame);
      processChildren = PR_TRUE;
      break;
  
    case NS_STYLE_DISPLAY_TABLE_CELL:
      // XXX We should check for being inside of a table row frame...
      rv = ConstructTableCellFrame(aPresContext, aContent, aParentFrame,
                                   aStyleContext, aAbsoluteItems, aNewFrame);
      // Note: table construction function takes care of initializing the frame,
      // processing children, and setting the initial child list
      return rv;
  
    case NS_STYLE_DISPLAY_TABLE_CAPTION:
      // XXX We should check for being inside of a table row frame...
      rv = NS_NewAreaFrame(aNewFrame, NS_BODY_NO_AUTO_MARGINS);
      processChildren = PR_TRUE;
      break;
  
    default:
      // Don't create any frame for content that's not displayed...
      break;
    }

    // If we succeeded in creating a frame then initialize the frame and
    // process children if requested
    if (NS_SUCCEEDED(rv) && (nsnull != aNewFrame)) {
      aNewFrame->Init(*aPresContext, aContent, aParentFrame, aParentFrame,
                      aStyleContext);

      // See if we need to create a view, e.g. the frame is absolutely positioned
      nsHTMLContainerFrame::CreateViewForFrame(*aPresContext, aNewFrame,
                                               aStyleContext, PR_FALSE);

      // Process the child content if requested
      nsIFrame* childList = nsnull;
      if (processChildren) {
        rv = ProcessChildren(aPresContext, aContent, aNewFrame, aAbsoluteItems,
                             childList);
      }

      // Set the frame's initial child list
      aNewFrame->SetInitialChildList(*aPresContext, nsnull, childList);
    }
  }

  // If the frame is absolutely positioned then create a placeholder frame
  if (isAbsolutelyPositioned) {
    nsIFrame* placeholderFrame;

    CreatePlaceholderFrameFor(aPresContext, aContent, aNewFrame, aStyleContext,
                              aParentFrame, placeholderFrame);

    // Add the absolutely positioned frame to its containing block's list
    // of child frames
    aAbsoluteItems.AddAbsolutelyPositionedChild(aNewFrame);

    // Add the placeholder frame to the flow
    aNewFrame = placeholderFrame;
  }

  return rv;
}

nsresult 
HTMLStyleSheetImpl::GetAdjustedParentFrame(nsIFrame*  aCurrentParentFrame, 
                                           PRUint8    aChildDisplayType, 
                                           nsIFrame*& aNewParentFrame)
{
  NS_PRECONDITION(nsnull!=aCurrentParentFrame, "bad arg aCurrentParentFrame");

  nsresult rv=NS_OK;
  // by default, the new parent frame is the given current parent frame
  aNewParentFrame=aCurrentParentFrame;
  if (nsnull!=aCurrentParentFrame)
  {
    const nsStyleDisplay* currentParentDisplay;
    aCurrentParentFrame->GetStyleData(eStyleStruct_Display, (const nsStyleStruct *&)currentParentDisplay);
    if (NS_STYLE_DISPLAY_TABLE==currentParentDisplay->mDisplay)
    {
      if (NS_STYLE_DISPLAY_TABLE_CAPTION!=aChildDisplayType)
      {
        nsIFrame *innerTableFrame=nsnull;
        aCurrentParentFrame->FirstChild(nsnull, innerTableFrame);
        if (nsnull!=innerTableFrame)
        {
          const nsStyleDisplay* innerTableDisplay;
          innerTableFrame->GetStyleData(eStyleStruct_Display, (const nsStyleStruct *&)innerTableDisplay);
          if (NS_STYLE_DISPLAY_TABLE==innerTableDisplay->mDisplay)
          { // we were given the outer table frame, use the inner table frame
            aNewParentFrame=innerTableFrame;
          }
          // else we were already given the inner table frame
        }
        // else the current parent has no children and cannot be an outer table frame
      }
      // else the child is a caption and really belongs to the outer table frame
    }
    else
    { 
      if (NS_STYLE_DISPLAY_TABLE_CAPTION     ==aChildDisplayType ||
          NS_STYLE_DISPLAY_TABLE_ROW_GROUP   ==aChildDisplayType ||
          NS_STYLE_DISPLAY_TABLE_HEADER_GROUP==aChildDisplayType ||
          NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP==aChildDisplayType ||
          NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP==aChildDisplayType)
      { // we need to create an anonymous table frame (outer and inner) around the new frame
        NS_NOTYETIMPLEMENTED("anonymous table frame as parent of table content not yet implemented.");
        rv = NS_ERROR_NOT_IMPLEMENTED;
      }
    }
  }
  else
    rv = NS_ERROR_NULL_POINTER;

  NS_POSTCONDITION(nsnull!=aNewParentFrame, "bad result null aNewParentFrame");
  return rv;
}

PRBool
HTMLStyleSheetImpl::IsScrollable(nsIPresContext*       aPresContext,
                                 const nsStyleDisplay* aDisplay)
{
  // For the time being it's scrollable if the overflow property is auto or
  // scroll, regardless of whether the width or height is fixed in size
  if ((NS_STYLE_OVERFLOW_SCROLL == aDisplay->mOverflow) ||
      (NS_STYLE_OVERFLOW_AUTO == aDisplay->mOverflow)) {
    return PR_TRUE;
  }

  return PR_FALSE;
}

nsresult
HTMLStyleSheetImpl::ConstructFrame(nsIPresContext*  aPresContext,
                                   nsIContent*      aContent,
                                   nsIFrame*        aParentFrame,
                                   nsAbsoluteItems& aAbsoluteItems,
                                   nsIFrame*&       aFrameSubTree)
{
  NS_PRECONDITION(nsnull != aParentFrame, "no parent frame");

  nsresult  rv;

  // Initialize OUT paremeter
  aFrameSubTree = nsnull;

  // Get the element's tag
  nsIAtom*  tag;
  aContent->GetTag(tag);

  // Resolve the style context based on the content object and the parent
  // style context
  nsIStyleContext* styleContext;
  nsIStyleContext* parentStyleContext;

  aParentFrame->GetStyleContext(parentStyleContext);
  if (nsnull == tag) {
    // Use a special pseudo element style context for text
    nsIContent* parentContent = nsnull;
    if (nsnull != aParentFrame) {
      aParentFrame->GetContent(parentContent);
    }
    styleContext = aPresContext->ResolvePseudoStyleContextFor(parentContent, 
                                                              nsHTMLAtoms::textPseudo, 
                                                              parentStyleContext);
    rv = (nsnull == styleContext) ? NS_ERROR_OUT_OF_MEMORY : NS_OK;
    NS_IF_RELEASE(parentContent);
  } else {
    styleContext = aPresContext->ResolveStyleContextFor(aContent, parentStyleContext);
    rv = (nsnull == styleContext) ? NS_ERROR_OUT_OF_MEMORY : NS_OK;
  }
  NS_IF_RELEASE(parentStyleContext);

  if (NS_SUCCEEDED(rv)) {
    // Pre-check for display "none" - if we find that, don't create
    // any frame at all
    const nsStyleDisplay* display = (const nsStyleDisplay*)
      styleContext->GetStyleData(eStyleStruct_Display);

    if (NS_STYLE_DISPLAY_NONE != display->mDisplay) {
      // Handle specific frame types
      rv = ConstructFrameByTag(aPresContext, aContent, aParentFrame, tag,
                               styleContext, aAbsoluteItems, aFrameSubTree);

      if (NS_SUCCEEDED(rv) && (nsnull == aFrameSubTree)) {
        // When there is no explicit frame to create, assume it's a
        // container and let display style dictate the rest
        rv = ConstructFrameByDisplayType(aPresContext, display, aContent, aParentFrame,
                                         styleContext, aAbsoluteItems, aFrameSubTree);
      }
    }
    NS_RELEASE(styleContext);
  }
  
  NS_IF_RELEASE(tag);
  return rv;
}

NS_IMETHODIMP  
HTMLStyleSheetImpl::ReconstructFrames(nsIPresContext* aPresContext,
                                      nsIContent*     aContent,
                                      nsIFrame*       aParentFrame,
                                      nsIFrame*       aFrameSubTree)
{
  nsresult rv = NS_OK;

  nsIDocument* document;
  rv = aContent->GetDocument(document);
  if (NS_FAILED(rv)) {
    return rv;
  }
  
  // XXX Currently we only know how to do this for XML documents
  if (nsnull != document) {
    nsIPresShell* shell = aPresContext->GetShell();
    nsIXMLDocument* xmlDocument;
    rv = document->QueryInterface(kIXMLDocumentIID, (void **)&xmlDocument);

    if (NS_SUCCEEDED(rv)) {
      nsIReflowCommand* reflowCmd;

      rv = NS_NewHTMLReflowCommand(&reflowCmd, aParentFrame,
                                   nsIReflowCommand::FrameRemoved,
                                   aFrameSubTree);
      
      if (NS_SUCCEEDED(rv)) {
        shell->AppendReflowCommand(reflowCmd);
        NS_RELEASE(reflowCmd);
        
        nsIFrame *newChild;
        // XXX See ConstructXMLContents for an explanation of why
        // we create this pseudostyle
        nsIStyleContext*  rootPseudoStyle;
        rootPseudoStyle = aPresContext->ResolvePseudoStyleContextFor(nsnull, 
                      nsHTMLAtoms::rootPseudo, nsnull);
        
        rv = ConstructDocElementFrame(aPresContext, aContent,
                                      aParentFrame, rootPseudoStyle, newChild);

        if (NS_SUCCEEDED(rv)) {
          rv = NS_NewHTMLReflowCommand(&reflowCmd, aParentFrame,
                                       nsIReflowCommand::FrameInserted,
                                       newChild);
          
          if (NS_SUCCEEDED(rv)) {
            shell->AppendReflowCommand(reflowCmd);
            NS_RELEASE(reflowCmd);
          }
        }
        NS_IF_RELEASE(rootPseudoStyle);
      }
      NS_RELEASE(xmlDocument);
    }  
    NS_RELEASE(document);
    NS_RELEASE(shell);
  }

  return rv;

}

nsIFrame*
HTMLStyleSheetImpl::GetFrameFor(nsIPresShell* aPresShell, nsIPresContext* aPresContext,
                                nsIContent* aContent)
{
  nsIFrame* frame = aPresShell->FindFrameWithContent(aContent);

  if (nsnull != frame) {
    // If the primary frame is a scroll frame, then get the scrolled frame.
    // That's the frame that gets the reflow command
    const nsStyleDisplay* display;
    frame->GetStyleData(eStyleStruct_Display, (const nsStyleStruct*&)display);

    if (display->IsBlockLevel() && IsScrollable(aPresContext, display)) {
      frame->FirstChild(nsnull, frame);
    }
  }

  return frame;
}

nsIFrame*
HTMLStyleSheetImpl::GetAbsoluteContainingBlock(nsIPresContext* aPresContext,
                                               nsIFrame*       aFrame)
{
  // For the time being just return the initial containing block
  NS_PRECONDITION(nsnull != mInitialContainingBlock, "no initial containing block");
  return mInitialContainingBlock;
  
  // XXX TROY
#if 0
  // Look for a containing frame that is absolutely positioned. If we don't
  // find one then use the initial containg block which is the BODY
  nsIFrame* lastFrame = (nsIFrame*)this;
  nsIFrame* result;

  GetContentParent(result);
  while (nsnull != result) {
    const nsStylePosition* position;

    // Get the style data
    result->GetStyleData(eStyleStruct_Position, (const nsStyleStruct*&)position);

    if (position->mPosition == NS_STYLE_POSITION_ABSOLUTE) {
      // XXX This needs cleaning up...
      // Make sure the frame supports the nsIAbsoluteItems interface. If not,
      // walk the geometric parent hierarchy and find the nearest one that does...
      nsIAbsoluteItems* interface;
      while ((nsnull != result) &&
             NS_FAILED(result->QueryInterface(kIAbsoluteItemsIID, (void**)&interface))) {
        result->GetGeometricParent(result);
      }
      break;
    }

    // Get the next contentual parent
    lastFrame = result;
    result->GetContentParent(result);
  }
#endif
}

NS_IMETHODIMP
HTMLStyleSheetImpl::ContentAppended(nsIPresContext* aPresContext,
                                    nsIContent*     aContainer,
                                    PRInt32         aNewIndexInContainer)
{
  nsIPresShell* shell = aPresContext->GetShell();
  nsIFrame*     parentFrame = GetFrameFor(shell, aPresContext, aContainer);

#ifdef NS_DEBUG
  if (nsnull == parentFrame) {
    NS_WARNING("Ignoring content-appended notification with no matching frame...");
  }
#endif
  if (nsnull != parentFrame) {
    // Get the parent frame's last-in-flow
    nsIFrame* nextInFlow = parentFrame;
    while (nsnull != nextInFlow) {
      parentFrame->GetNextInFlow(nextInFlow);
      if (nsnull != nextInFlow) {
        parentFrame = nextInFlow;
      }
    }

    // Get the containing block for absolutely positioned elements
    nsIFrame* absoluteContainingBlock = GetAbsoluteContainingBlock(aPresContext,
                                                                   parentFrame);

    // Create some new frames
    PRInt32         count;
    nsIFrame*       lastChildFrame = nsnull;
    nsIFrame*       firstAppendedFrame = nsnull;
    nsAbsoluteItems absoluteItems(absoluteContainingBlock);

    aContainer->ChildCount(count);

    for (PRInt32 i = aNewIndexInContainer; i < count; i++) {
      nsIContent* child;
      nsIFrame*   frame;

      aContainer->ChildAt(i, child);
      ConstructFrame(aPresContext, child, parentFrame, absoluteItems, frame);

      if (nsnull != frame) {
        // Link the frame into the child frame list
        if (nsnull == lastChildFrame) {
          firstAppendedFrame = frame;
        } else {
          lastChildFrame->SetNextSibling(frame);
        }

        lastChildFrame = frame;
      }
      NS_RELEASE(child);
    }

    // Adjust parent frame for table inner/outer frame
    // we need to do this here because we need both the parent frame and the constructed frame
    nsresult result = NS_OK;
    nsIFrame *adjustedParentFrame=parentFrame;
    if (nsnull!=firstAppendedFrame)
    {
      const nsStyleDisplay* firstAppendedFrameDisplay;
      firstAppendedFrame->GetStyleData(eStyleStruct_Display, (const nsStyleStruct *&)firstAppendedFrameDisplay);
      result = GetAdjustedParentFrame(parentFrame, firstAppendedFrameDisplay->mDisplay, adjustedParentFrame);
    }

    // Notify the parent frame with a reflow command, passing it the list of
    // new frames.
    if (NS_SUCCEEDED(result)) {
      nsIReflowCommand* reflowCmd;
      result = NS_NewHTMLReflowCommand(&reflowCmd, adjustedParentFrame,
                                       nsIReflowCommand::FrameAppended,
                                       firstAppendedFrame);
      if (NS_SUCCEEDED(result)) {
        shell->AppendReflowCommand(reflowCmd);
        NS_RELEASE(reflowCmd);
      }

      // If there are new absolutely positioned child frames then send a reflow
      // command for them, too.
      // XXX We can't just assume these frames are being appended, we need to
      // determine where in the list they should be inserted...
      if (nsnull != absoluteItems.childList) {
        result = NS_NewHTMLReflowCommand(&reflowCmd, absoluteItems.containingBlock,
                                         nsIReflowCommand::FrameAppended,
                                         absoluteItems.childList);
        if (NS_SUCCEEDED(result)) {
          reflowCmd->SetChildListName(nsLayoutAtoms::absoluteList);
          shell->AppendReflowCommand(reflowCmd);
          NS_RELEASE(reflowCmd);
        }
      }
    }
  }

  NS_RELEASE(shell);
  return NS_OK;
}

static nsIFrame*
FindPreviousSibling(nsIPresShell* aPresShell,
                    nsIContent*   aContainer,
                    PRInt32       aIndexInContainer)
{
  nsIFrame* prevSibling = nsnull;

  // Note: not all content objects are associated with a frame so
  // keep looking until we find a previous frame
  for (PRInt32 index = aIndexInContainer - 1; index >= 0; index--) {
    nsIContent* precedingContent;
    aContainer->ChildAt(index, precedingContent);
    prevSibling = aPresShell->FindFrameWithContent(precedingContent);
    NS_RELEASE(precedingContent);

    if (nsnull != prevSibling) {
      // The frame may have a next-in-flow. Get the last-in-flow
      nsIFrame* nextInFlow;
      do {
        prevSibling->GetNextInFlow(nextInFlow);
        if (nsnull != nextInFlow) {
          prevSibling = nextInFlow;
        }
      } while (nsnull != nextInFlow);

      break;
    }
  }

  return prevSibling;
}

static nsIFrame*
FindNextSibling(nsIPresShell* aPresShell,
                nsIContent*   aContainer,
                PRInt32       aIndexInContainer)
{
  nsIFrame* nextSibling = nsnull;

  // Note: not all content objects are associated with a frame so
  // keep looking until we find a next frame
  PRInt32 count;
  aContainer->ChildCount(count);
  for (PRInt32 index = aIndexInContainer + 1; index < count; index++) {
    nsIContent* nextContent;
    aContainer->ChildAt(index, nextContent);
    nextSibling = aPresShell->FindFrameWithContent(nextContent);
    NS_RELEASE(nextContent);

    if (nsnull != nextSibling) {
      // The frame may have a next-in-flow. Get the last-in-flow
      nsIFrame* nextInFlow;
      do {
        nextSibling->GetNextInFlow(nextInFlow);
        if (nsnull != nextInFlow) {
          nextSibling = nextInFlow;
        }
      } while (nsnull != nextInFlow);

      break;
    }
  }

  return nextSibling;
}

NS_IMETHODIMP
HTMLStyleSheetImpl::ContentInserted(nsIPresContext* aPresContext,
                                    nsIContent*     aContainer,
                                    nsIContent*     aChild,
                                    PRInt32         aIndexInContainer)
{
  nsIPresShell* shell = aPresContext->GetShell();

  // Find the frame that precedes the insertion point.
  nsIFrame* prevSibling = FindPreviousSibling(shell, aContainer, aIndexInContainer);
  nsIFrame* nextSibling = nsnull;
  PRBool    isAppend = PR_FALSE;

  // If there is no previous sibling, then find the frame that follows
  if (nsnull == prevSibling) {
    nextSibling = FindNextSibling(shell, aContainer, aIndexInContainer);
  }

  // Get the geometric parent.
  nsIFrame* parentFrame;
  if ((nsnull == prevSibling) && (nsnull == nextSibling)) {
    // No previous or next sibling so treat this like an appended frame.
    // XXX This won't always be true if there's auto-generated before/after
    // content
    isAppend = PR_TRUE;
    parentFrame = shell->FindFrameWithContent(aContainer);

  } else {
    // Use the prev sibling if we have it; otherwise use the next sibling.
    // Note that we use the content parent, and not the geometric parent,
    // in case the frame has been moved out of the flow...
    if (nsnull != prevSibling) {
      prevSibling->GetContentParent(parentFrame);
    } else {
      nextSibling->GetContentParent(parentFrame);
    }
  }

  // Construct a new frame
  nsresult  rv = NS_OK;
  if (nsnull != parentFrame) {
    // Get the containing block for absolutely positioned elements
    nsIFrame* absoluteContainingBlock = GetAbsoluteContainingBlock(aPresContext,
                                                                   parentFrame);

    nsAbsoluteItems absoluteItems(absoluteContainingBlock);
    nsIFrame*       newFrame;
    rv = ConstructFrame(aPresContext, aChild, parentFrame, absoluteItems, newFrame);

    if (NS_SUCCEEDED(rv) && (nsnull != newFrame)) {
      nsIReflowCommand* reflowCmd = nsnull;

      // Notify the parent frame with a reflow command.
      if (isAppend) {
        // Generate a FrameAppended reflow command
        rv = NS_NewHTMLReflowCommand(&reflowCmd, parentFrame,
                                     nsIReflowCommand::FrameAppended, newFrame);
      } else {
        // Generate a FrameInserted reflow command
        rv = NS_NewHTMLReflowCommand(&reflowCmd, parentFrame, newFrame, prevSibling);
      }
      if (NS_SUCCEEDED(rv)) {
        shell->AppendReflowCommand(reflowCmd);
        NS_RELEASE(reflowCmd);
      }

      // If there are new absolutely positioned child frames then send a reflow
      // command for them, too.
      // XXX We can't just assume these frames are being appended, we need to
      // determine where in the list they should be inserted...
      if (nsnull != absoluteItems.childList) {
        rv = NS_NewHTMLReflowCommand(&reflowCmd, absoluteItems.containingBlock,
                                     nsIReflowCommand::FrameAppended,
                                     absoluteItems.childList);
        if (NS_SUCCEEDED(rv)) {
          reflowCmd->SetChildListName(nsLayoutAtoms::absoluteList);
          shell->AppendReflowCommand(reflowCmd);
          NS_RELEASE(reflowCmd);
        }
      }
    }
  }

  NS_RELEASE(shell);
  return rv;
}

NS_IMETHODIMP
HTMLStyleSheetImpl::ContentReplaced(nsIPresContext* aPresContext,
                                    nsIContent*     aContainer,
                                    nsIContent*     aOldChild,
                                    nsIContent*     aNewChild,
                                    PRInt32         aIndexInContainer)
{
  // XXX For now, do a brute force remove and insert.
  nsresult res = ContentRemoved(aPresContext, aContainer, 
                                aOldChild, aIndexInContainer);
  if (NS_OK == res) {
    res = ContentInserted(aPresContext, aContainer, 
                          aNewChild, aIndexInContainer);
  }

  return res;
}

NS_IMETHODIMP
HTMLStyleSheetImpl::ContentRemoved(nsIPresContext* aPresContext,
                                   nsIContent*     aContainer,
                                   nsIContent*     aChild,
                                   PRInt32         aIndexInContainer)
{
  nsIPresShell* shell = aPresContext->GetShell();
  nsresult      rv = NS_OK;

  // Find the child frame
  nsIFrame* childFrame = shell->FindFrameWithContent(aChild);

  if (nsnull != childFrame) {
    // Get the parent frame.
    // Note that we use the content parent, and not the geometric parent,
    // in case the frame has been moved out of the flow...
    nsIFrame* parentFrame;
    childFrame->GetContentParent(parentFrame);
    NS_ASSERTION(nsnull != parentFrame, "null content parent frame");

    // Notify the parent frame with a reflow command.
    nsIReflowCommand* reflowCmd;
    rv = NS_NewHTMLReflowCommand(&reflowCmd, parentFrame,
                                 nsIReflowCommand::FrameRemoved, childFrame);

    if (NS_SUCCEEDED(rv)) {
      shell->AppendReflowCommand(reflowCmd);
      NS_RELEASE(reflowCmd);
    }
  }

  NS_RELEASE(shell);
  return rv;
}


static void
ApplyRenderingChangeToTree(nsIPresContext* aPresContext,
                           nsIFrame* aFrame)
{
  nsIViewManager* viewManager = nsnull;

  // Trigger rendering updates by damaging this frame and any
  // continuations of this frame.
  while (nsnull != aFrame) {
    // Get the frame's bounding rect
    nsRect r;
    aFrame->GetRect(r);
    r.x = 0;
    r.y = 0;

    // Get view if this frame has one and trigger an update. If the
    // frame doesn't have a view, find the nearest containing view
    // (adjusting r's coordinate system to reflect the nesting) and
    // update there.
    nsIView* view;
    aFrame->GetView(view);
    if (nsnull != view) {
    } else {
      nsPoint offset;
      aFrame->GetOffsetFromView(offset, view);
      NS_ASSERTION(nsnull != view, "no view");
      r += offset;
    }
    if (nsnull == viewManager) {
      view->GetViewManager(viewManager);
    }
    const nsStyleColor* color;
    aFrame->GetStyleData(eStyleStruct_Color, (const nsStyleStruct*&) color);
    viewManager->SetViewOpacity(view, color->mOpacity);
    viewManager->UpdateView(view, r, NS_VMREFRESH_NO_SYNC);

    aFrame->GetNextInFlow(aFrame);
  }

  if (nsnull != viewManager) {
    viewManager->Composite();
    NS_RELEASE(viewManager);
  }
}

static void
StyleChangeReflow(nsIPresContext* aPresContext,
                  nsIFrame* aFrame,
                  nsIAtom * aAttribute)
{
  nsIPresShell* shell;
  shell = aPresContext->GetShell();
    
  nsIReflowCommand* reflowCmd;
  nsresult rv = NS_NewHTMLReflowCommand(&reflowCmd, aFrame,
                                        nsIReflowCommand::StyleChanged,
                                        nsnull,
                                        aAttribute);
  if (NS_SUCCEEDED(rv)) {
    shell->AppendReflowCommand(reflowCmd);
    NS_RELEASE(reflowCmd);
  }

  NS_RELEASE(shell);
}

NS_IMETHODIMP
HTMLStyleSheetImpl::ContentChanged(nsIPresContext* aPresContext,
                                   nsIContent*  aContent,
                                   nsISupports* aSubContent)
{
  nsIPresShell* shell = aPresContext->GetShell();
  nsresult      rv = NS_OK;

  // Find the child frame
  nsIFrame* frame = shell->FindFrameWithContent(aContent);

  // Notify the first frame that maps the content. It will generate a reflow
  // command

  // It's possible the frame whose content changed isn't inserted into the
  // frame hierarchy yet, or that there is no frame that maps the content
  if (nsnull != frame) {
#if 0
    NS_FRAME_LOG(NS_FRAME_TRACE_CALLS,
       ("nsHTMLStyleSheet::ContentChanged: content=%p[%s] subcontent=%p frame=%p",
        aContent, ContentTag(aContent, 0),
        aSubContent, frame));
#endif
    frame->ContentChanged(aPresContext, aContent, aSubContent);
  }

  NS_RELEASE(shell);
  return rv;
}
    
NS_IMETHODIMP
HTMLStyleSheetImpl::AttributeChanged(nsIPresContext* aPresContext,
                                     nsIContent* aContent,
                                     nsIAtom* aAttribute,
                                     PRInt32 aHint)
{
  nsresult  result = NS_OK;

  nsIPresShell* shell = aPresContext->GetShell();
  nsIFrame* frame = shell->FindFrameWithContent(aContent);

  if (nsnull != frame) {
    PRBool  restyle = PR_FALSE;
    PRBool  reflow  = PR_FALSE;
    PRBool  reframe = PR_FALSE;
    PRBool  render  = PR_FALSE;

#if 0
    NS_FRAME_LOG(NS_FRAME_TRACE_CALLS,
       ("HTMLStyleSheet::AttributeChanged: content=%p[%s] frame=%p",
        aContent, ContentTag(aContent, 0), frame));
#endif

    // the style tag has its own interpretation based on aHint 
    if ((nsHTMLAtoms::style != aAttribute) && (NS_STYLE_HINT_UNKNOWN == aHint)) { 
      nsIHTMLContent* htmlContent;
      result = aContent->QueryInterface(kIHTMLContentIID, (void**)&htmlContent);
  
      if (NS_OK == result) { 
        // Get style hint from HTML content object. 
        htmlContent->GetStyleHintForAttributeChange(aAttribute, &aHint);
        NS_RELEASE(htmlContent); 
      } 
    } 

    switch (aHint) {
      default:
      case NS_STYLE_HINT_UNKNOWN:
      case NS_STYLE_HINT_FRAMECHANGE:
        reframe = PR_TRUE;
      case NS_STYLE_HINT_REFLOW:
        reflow = PR_TRUE;
      case NS_STYLE_HINT_VISUAL:
        render = PR_TRUE;
      case NS_STYLE_HINT_CONTENT:
        restyle = PR_TRUE;
        break;
      case NS_STYLE_HINT_AURAL:
        break;
    }

    // apply changes
    if (PR_TRUE == restyle) {
      nsIStyleContext* frameContext;
      frame->GetStyleContext(frameContext);
      NS_ASSERTION(nsnull != frameContext, "frame must have style context");
      if (nsnull != frameContext) {
        nsIStyleContext*  parentContext = frameContext->GetParent();
        frame->ReResolveStyleContext(aPresContext, parentContext);
        NS_IF_RELEASE(parentContext);
        NS_RELEASE(frameContext);
      }
    }
    if (PR_TRUE == reframe) {
      NS_NOTYETIMPLEMENTED("frame change reflow");
    }
    else if (PR_TRUE == reflow) {
      StyleChangeReflow(aPresContext, frame, aAttribute);
    }
    else if (PR_TRUE == render) {
      ApplyRenderingChangeToTree(aPresContext, frame);
    }
    else {  // let the frame deal with it, since we don't know how to
      frame->AttributeChanged(aPresContext, aContent, aAttribute, aHint);
    }
  }

  NS_RELEASE(shell);
  return result;
}

  // Style change notifications
NS_IMETHODIMP
HTMLStyleSheetImpl::StyleRuleChanged(nsIPresContext* aPresContext,
                                     nsIStyleSheet* aStyleSheet,
                                     nsIStyleRule* aStyleRule,
                                     PRInt32 aHint)
{
  nsIPresShell* shell = aPresContext->GetShell();
  nsIFrame* frame = shell->GetRootFrame();

  PRBool reframe  = PR_FALSE;
  PRBool reflow   = PR_FALSE;
  PRBool render   = PR_FALSE;
  PRBool restyle  = PR_FALSE;
  switch (aHint) {
    default:
    case NS_STYLE_HINT_UNKNOWN:
    case NS_STYLE_HINT_FRAMECHANGE:
      reframe = PR_TRUE;
    case NS_STYLE_HINT_REFLOW:
      reflow = PR_TRUE;
    case NS_STYLE_HINT_VISUAL:
      render = PR_TRUE;
    case NS_STYLE_HINT_CONTENT:
      restyle = PR_TRUE;
      break;
    case NS_STYLE_HINT_AURAL:
      break;
  }

  if (restyle) {
    nsIStyleContext* sc;
    frame->GetStyleContext(sc);
    sc->RemapStyle(aPresContext);
    NS_RELEASE(sc);
  }

  // XXX hack, skip the root and scrolling frames
  frame->FirstChild(nsnull, frame);
  frame->FirstChild(nsnull, frame);
  if (reframe) {
    NS_NOTYETIMPLEMENTED("frame change reflow");
  }
  else if (reflow) {
    StyleChangeReflow(aPresContext, frame, nsnull);
  }
  else if (render) {
    ApplyRenderingChangeToTree(aPresContext, frame);
  }

  NS_RELEASE(shell);
  
  return NS_OK;
}

NS_IMETHODIMP
HTMLStyleSheetImpl::StyleRuleAdded(nsIPresContext* aPresContext,
                                   nsIStyleSheet* aStyleSheet,
                                   nsIStyleRule* aStyleRule)
{
  return NS_OK;
}

NS_IMETHODIMP
HTMLStyleSheetImpl::StyleRuleRemoved(nsIPresContext* aPresContext,
                                     nsIStyleSheet* aStyleSheet,
                                     nsIStyleRule* aStyleRule)
{
  return NS_OK;
}


void HTMLStyleSheetImpl::List(FILE* out, PRInt32 aIndent) const
{
  PRUnichar* buffer;

  // Indent
  for (PRInt32 index = aIndent; --index >= 0; ) fputs("  ", out);

  fputs("HTML Style Sheet: ", out);
  mURL->ToString(&buffer);
  fputs(buffer, out);
  fputs("\n", out);
  delete buffer;
}

// XXX For convenience and backwards compatibility
NS_HTML nsresult
NS_NewHTMLStyleSheet(nsIHTMLStyleSheet** aInstancePtrResult, nsIURL* aURL, 
                     nsIDocument* aDocument)
{
  nsresult rv;
  nsIHTMLStyleSheet* sheet;
  if (NS_FAILED(rv = NS_NewHTMLStyleSheet(&sheet)))
    return rv;

  if (NS_FAILED(rv = sheet->Init(aURL, aDocument))) {
    NS_RELEASE(sheet);
    return rv;
  }

  *aInstancePtrResult = sheet;
  return NS_OK;
}


NS_HTML nsresult
NS_NewHTMLStyleSheet(nsIHTMLStyleSheet** aInstancePtrResult)
{
  if (aInstancePtrResult == nsnull) {
    return NS_ERROR_NULL_POINTER;
  }

  HTMLStyleSheetImpl  *it = new HTMLStyleSheetImpl();

  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(it);
  *aInstancePtrResult = it;
  return NS_OK;
}
