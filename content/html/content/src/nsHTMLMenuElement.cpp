/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are Copyright (C) 1998
 * Netscape Communications Corporation.  All Rights Reserved.
 */
#include "nsIDOMHTMLMenuElement.h"
#include "nsIScriptObjectOwner.h"
#include "nsIDOMEventReceiver.h"
#include "nsIHTMLContent.h"
#include "nsGenericHTMLElement.h"
#include "nsHTMLAtoms.h"
#include "nsHTMLIIDs.h"
#include "nsIStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIPresContext.h"

// XXX nav4 has type= start= (same as OL/UL)

extern nsGenericHTMLElement::EnumTable kListTypeTable[];

static NS_DEFINE_IID(kIDOMHTMLMenuElementIID, NS_IDOMHTMLMENUELEMENT_IID);

class nsHTMLMenuElement : public nsIDOMHTMLMenuElement,
                          public nsIScriptObjectOwner,
                          public nsIDOMEventReceiver,
                          public nsIHTMLContent
{
public:
  nsHTMLMenuElement(nsIAtom* aTag);
  ~nsHTMLMenuElement();

  // nsISupports
  NS_DECL_ISUPPORTS

  // nsIDOMNode
  NS_IMPL_IDOMNODE_USING_GENERIC(mInner)

  // nsIDOMElement
  NS_IMPL_IDOMELEMENT_USING_GENERIC(mInner)

  // nsIDOMHTMLElement
  NS_IMPL_IDOMHTMLELEMENT_USING_GENERIC(mInner)

  // nsIDOMHTMLMenuElement
  NS_IMETHOD GetCompact(PRBool* aCompact);
  NS_IMETHOD SetCompact(PRBool aCompact);

  // nsIScriptObjectOwner
  NS_IMPL_ISCRIPTOBJECTOWNER_USING_GENERIC(mInner)

  // nsIDOMEventReceiver
  NS_IMPL_IDOMEVENTRECEIVER_USING_GENERIC(mInner)

  // nsIContent
  NS_IMPL_ICONTENT_USING_GENERIC(mInner)

  // nsIHTMLContent
  NS_IMPL_IHTMLCONTENT_USING_GENERIC(mInner)

protected:
  nsGenericHTMLContainerElement mInner;
};

nsresult
NS_NewHTMLMenuElement(nsIHTMLContent** aInstancePtrResult, nsIAtom* aTag)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null ptr");
  if (nsnull == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }
  nsIHTMLContent* it = new nsHTMLMenuElement(aTag);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return it->QueryInterface(kIHTMLContentIID, (void**) aInstancePtrResult);
}

nsHTMLMenuElement::nsHTMLMenuElement(nsIAtom* aTag)
{
  NS_INIT_REFCNT();
  mInner.Init(this, aTag);
}

nsHTMLMenuElement::~nsHTMLMenuElement()
{
}

NS_IMPL_ADDREF(nsHTMLMenuElement)

NS_IMPL_RELEASE(nsHTMLMenuElement)

nsresult
nsHTMLMenuElement::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  NS_IMPL_HTML_CONTENT_QUERY_INTERFACE(aIID, aInstancePtr, this)
  if (aIID.Equals(kIDOMHTMLMenuElementIID)) {
    nsIDOMHTMLMenuElement* tmp = this;
    *aInstancePtr = (void*) tmp;
    mRefCnt++;
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

nsresult
nsHTMLMenuElement::CloneNode(nsIDOMNode** aReturn)
{
  nsHTMLMenuElement* it = new nsHTMLMenuElement(mInner.mTag);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  mInner.CopyInnerTo(this, &it->mInner);
  return it->QueryInterface(kIDOMNodeIID, (void**) aReturn);
}

NS_IMPL_BOOL_ATTR(nsHTMLMenuElement, Compact, compact, eSetAttrNotify_Reflow)

NS_IMETHODIMP
nsHTMLMenuElement::StringToAttribute(nsIAtom* aAttribute,
                                     const nsString& aValue,
                                     nsHTMLValue& aResult)
{
  if (aAttribute == nsHTMLAtoms::type) {
    if (!nsGenericHTMLElement::ParseEnumValue(aValue, kListTypeTable,
                                              aResult)) {
      aResult.SetIntValue(NS_STYLE_LIST_STYLE_BASIC, eHTMLUnit_Enumerated);
    }
    return NS_CONTENT_ATTR_HAS_VALUE;
  }
  if (aAttribute == nsHTMLAtoms::start) {
    nsGenericHTMLElement::ParseValue(aValue, 1, aResult, eHTMLUnit_Integer);
    return NS_CONTENT_ATTR_HAS_VALUE;
  }
  if (aAttribute == nsHTMLAtoms::compact) {
    aResult.SetEmptyValue();
    return NS_CONTENT_ATTR_NO_VALUE;
  }
  return NS_CONTENT_ATTR_NOT_THERE;
}

NS_IMETHODIMP
nsHTMLMenuElement::AttributeToString(nsIAtom* aAttribute,
                                     nsHTMLValue& aValue,
                                     nsString& aResult) const
{
  if (aAttribute == nsHTMLAtoms::type) {
    nsGenericHTMLElement::EnumValueToString(aValue, kListTypeTable, aResult);
    return NS_CONTENT_ATTR_HAS_VALUE;
  }
  return mInner.AttributeToString(aAttribute, aValue, aResult);
}

NS_IMETHODIMP
nsHTMLMenuElement::MapAttributesInto(nsIStyleContext* aContext,
                                     nsIPresContext* aPresContext)
{
  if (nsnull != mInner.mAttributes) {
    nsHTMLValue value;
    nsStyleList* list = (nsStyleList*)
      aContext->GetMutableStyleData(eStyleStruct_List);

    // type: enum
    GetAttribute(nsHTMLAtoms::type, value);
    if (value.GetUnit() == eHTMLUnit_Enumerated) {
      list->mListStyleType = value.GetIntValue();
    }

    // compact: empty
    GetAttribute(nsHTMLAtoms::compact, value);
    if (value.GetUnit() == eHTMLUnit_Empty) {
      // XXX set
    }
  }
  return mInner.MapAttributesInto(aContext, aPresContext);
}

NS_IMETHODIMP
nsHTMLMenuElement::HandleDOMEvent(nsIPresContext& aPresContext,
                                  nsEvent* aEvent,
                                  nsIDOMEvent** aDOMEvent,
                                  PRUint32 aFlags,
                                  nsEventStatus& aEventStatus)
{
  return mInner.HandleDOMEvent(aPresContext, aEvent, aDOMEvent,
                               aFlags, aEventStatus);
}
