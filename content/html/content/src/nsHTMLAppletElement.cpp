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
#include "nsIDOMHTMLAppletElement.h"
#include "nsIScriptObjectOwner.h"
#include "nsIDOMEventReceiver.h"
#include "nsIHTMLContent.h"
#include "nsGenericHTMLElement.h"
#include "nsHTMLAtoms.h"
#include "nsHTMLIIDs.h"
#include "nsIStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIPresContext.h"

static NS_DEFINE_IID(kIDOMHTMLAppletElementIID, NS_IDOMHTMLAPPLETELEMENT_IID);

class nsHTMLAppletElement : public nsIDOMHTMLAppletElement,
                            public nsIScriptObjectOwner,
                            public nsIDOMEventReceiver,
                            public nsIHTMLContent
{
public:
  nsHTMLAppletElement(nsIAtom* aTag);
  ~nsHTMLAppletElement();

  // nsISupports
  NS_DECL_ISUPPORTS

  // nsIDOMNode
  NS_IMPL_IDOMNODE_USING_GENERIC(mInner)

  // nsIDOMElement
  NS_IMPL_IDOMELEMENT_USING_GENERIC(mInner)

  // nsIDOMHTMLElement
  NS_IMPL_IDOMHTMLELEMENT_USING_GENERIC(mInner)

  // nsIDOMHTMLAppletElement
  NS_IMETHOD GetAlign(nsString& aAlign);
  NS_IMETHOD SetAlign(const nsString& aAlign);
  NS_IMETHOD GetAlt(nsString& aAlt);
  NS_IMETHOD SetAlt(const nsString& aAlt);
  NS_IMETHOD GetArchive(nsString& aArchive);
  NS_IMETHOD SetArchive(const nsString& aArchive);
  NS_IMETHOD GetCode(nsString& aCode);
  NS_IMETHOD SetCode(const nsString& aCode);
  NS_IMETHOD GetCodeBase(nsString& aCodeBase);
  NS_IMETHOD SetCodeBase(const nsString& aCodeBase);
  NS_IMETHOD GetHeight(nsString& aHeight);
  NS_IMETHOD SetHeight(const nsString& aHeight);
  NS_IMETHOD GetHspace(nsString& aHspace);
  NS_IMETHOD SetHspace(const nsString& aHspace);
  NS_IMETHOD GetName(nsString& aName);
  NS_IMETHOD SetName(const nsString& aName);
  NS_IMETHOD GetObject(nsString& aObject);
  NS_IMETHOD SetObject(const nsString& aObject);
  NS_IMETHOD GetVspace(nsString& aVspace);
  NS_IMETHOD SetVspace(const nsString& aVspace);
  NS_IMETHOD GetWidth(nsString& aWidth);
  NS_IMETHOD SetWidth(const nsString& aWidth);

  // nsIScriptObjectOwner
  NS_IMPL_ISCRIPTOBJECTOWNER_USING_GENERIC(mInner)

  // nsIDOMEventReceiver
  NS_IMPL_IDOMEVENTRECEIVER_USING_GENERIC(mInner)

  // nsIContent
  NS_IMPL_ICONTENT_USING_GENERIC(mInner)

  // nsIHTMLContent
  NS_IMPL_IHTMLCONTENT_USING_GENERIC(mInner)

protected:
  nsHTMLGenericContainerContent mInner;
};

nsresult
NS_NewHTMLAppletElement(nsIHTMLContent** aInstancePtrResult, nsIAtom* aTag)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null ptr");
  if (nsnull == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }
  nsIHTMLContent* it = new nsHTMLAppletElement(aTag);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return it->QueryInterface(kIHTMLContentIID, (void**) aInstancePtrResult);
}

nsHTMLAppletElement::nsHTMLAppletElement(nsIAtom* aTag)
{
  NS_INIT_REFCNT();
  mInner.Init(this, aTag);
}

nsHTMLAppletElement::~nsHTMLAppletElement()
{
}

NS_IMPL_ADDREF(nsHTMLAppletElement)

NS_IMPL_RELEASE(nsHTMLAppletElement)

nsresult
nsHTMLAppletElement::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  NS_IMPL_HTML_CONTENT_QUERY_INTERFACE(aIID, aInstancePtr, this)
  if (aIID.Equals(kIDOMHTMLAppletElementIID)) {
    nsIDOMHTMLAppletElement* tmp = this;
    *aInstancePtr = (void*) tmp;
    mRefCnt++;
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

nsresult
nsHTMLAppletElement::CloneNode(nsIDOMNode** aReturn)
{
  nsHTMLAppletElement* it = new nsHTMLAppletElement(mInner.mTag);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  mInner.CopyInnerTo(this, &it->mInner);
  return it->QueryInterface(kIDOMNodeIID, (void**) aReturn);
}

NS_IMPL_STRING_ATTR(nsHTMLAppletElement, Align, align, eSetAttrNotify_Reflow)
NS_IMPL_STRING_ATTR(nsHTMLAppletElement, Alt, alt, eSetAttrNotify_None)
NS_IMPL_STRING_ATTR(nsHTMLAppletElement, Archive, archive, eSetAttrNotify_Restart)
NS_IMPL_STRING_ATTR(nsHTMLAppletElement, Code, code, eSetAttrNotify_Restart)
NS_IMPL_STRING_ATTR(nsHTMLAppletElement, CodeBase, codebase, eSetAttrNotify_Restart)
NS_IMPL_STRING_ATTR(nsHTMLAppletElement, Height, height, eSetAttrNotify_Reflow)
NS_IMPL_STRING_ATTR(nsHTMLAppletElement, Hspace, hspace, eSetAttrNotify_Reflow)
NS_IMPL_STRING_ATTR(nsHTMLAppletElement, Name, name, eSetAttrNotify_Restart)
NS_IMPL_STRING_ATTR(nsHTMLAppletElement, Object, object, eSetAttrNotify_None)
NS_IMPL_STRING_ATTR(nsHTMLAppletElement, Vspace, vspace, eSetAttrNotify_Reflow)
NS_IMPL_STRING_ATTR(nsHTMLAppletElement, Width, width, eSetAttrNotify_Reflow)

NS_IMETHODIMP
nsHTMLAppletElement::StringToAttribute(nsIAtom* aAttribute,
                                       const nsString& aValue,
                                       nsHTMLValue& aResult)
{
  if (aAttribute == nsHTMLAtoms::align) {
    if (nsHTMLGenericContent::ParseAlignValue(aValue, aResult)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (nsHTMLGenericContent::ParseImageAttribute(aAttribute,
                                                     aValue, aResult)) {
    return NS_CONTENT_ATTR_HAS_VALUE;
  }
  return NS_CONTENT_ATTR_NOT_THERE;
}

NS_IMETHODIMP
nsHTMLAppletElement::AttributeToString(nsIAtom* aAttribute,
                                       nsHTMLValue& aValue,
                                       nsString& aResult) const
{
  if (aAttribute == nsHTMLAtoms::align) {
    if (eHTMLUnit_Enumerated == aValue.GetUnit()) {
      nsHTMLGenericContent::AlignValueToString(aValue, aResult);
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (nsHTMLGenericContent::ImageAttributeToString(aAttribute,
                                                        aValue, aResult)) {
    return NS_CONTENT_ATTR_HAS_VALUE;
  }
  return mInner.AttributeToString(aAttribute, aValue, aResult);
}

NS_IMETHODIMP
nsHTMLAppletElement::MapAttributesInto(nsIStyleContext* aContext,
                                       nsIPresContext* aPresContext)
{
  mInner.MapImageAlignAttributeInto(aContext, aPresContext);
  mInner.MapImageAttributesInto(aContext, aPresContext);
  mInner.MapImageBorderAttributesInto(aContext, aPresContext, nsnull);
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLAppletElement::HandleDOMEvent(nsIPresContext& aPresContext,
                                    nsEvent* aEvent,
                                    nsIDOMEvent** aDOMEvent,
                                    PRUint32 aFlags,
                                    nsEventStatus& aEventStatus)
{
  return mInner.HandleDOMEvent(aPresContext, aEvent, aDOMEvent,
                               aFlags, aEventStatus);
}
