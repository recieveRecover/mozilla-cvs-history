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
#include "nsIDOMHTMLAreaElement.h"
#include "nsIScriptObjectOwner.h"
#include "nsIDOMEventReceiver.h"
#include "nsIHTMLContent.h"
#include "nsHTMLGenericContent.h"
#include "nsHTMLAtoms.h"
#include "nsHTMLIIDs.h"
#include "nsIStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIPresContext.h"

static NS_DEFINE_IID(kIDOMHTMLAreaElementIID, NS_IDOMHTMLAREAELEMENT_IID);

class nsHTMLArea : public nsIDOMHTMLAreaElement,
		   public nsIScriptObjectOwner,
		   public nsIDOMEventReceiver,
		   public nsIHTMLContent
{
public:
  nsHTMLArea(nsIAtom* aTag);
  ~nsHTMLArea();

  // nsISupports
  NS_DECL_ISUPPORTS

  // nsIDOMNode
  NS_IMPL_IDOMNODE_USING_GENERIC(mInner)

  // nsIDOMElement
  NS_IMPL_IDOMELEMENT_USING_GENERIC(mInner)

  // nsIDOMHTMLElement
  NS_IMPL_IDOMHTMLELEMENT_USING_GENERIC(mInner)

  // nsIDOMHTMLAreaElement
  NS_IMETHOD GetAccessKey(nsString& aAccessKey);
  NS_IMETHOD SetAccessKey(const nsString& aAccessKey);
  NS_IMETHOD GetAlt(nsString& aAlt);
  NS_IMETHOD SetAlt(const nsString& aAlt);
  NS_IMETHOD GetCoords(nsString& aCoords);
  NS_IMETHOD SetCoords(const nsString& aCoords);
  NS_IMETHOD GetHref(nsString& aHref);
  NS_IMETHOD SetHref(const nsString& aHref);
  NS_IMETHOD GetNoHref(PRBool* aNoHref);
  NS_IMETHOD SetNoHref(PRBool aNoHref);
  NS_IMETHOD GetShape(nsString& aShape);
  NS_IMETHOD SetShape(const nsString& aShape);
  NS_IMETHOD GetTabIndex(PRInt32* aTabIndex);
  NS_IMETHOD SetTabIndex(PRInt32 aTabIndex);
  NS_IMETHOD GetTarget(nsString& aTarget);
  NS_IMETHOD SetTarget(const nsString& aTarget);

  // nsIScriptObjectOwner
  NS_IMPL_ISCRIPTOBJECTOWNER_USING_GENERIC(mInner)

  // nsIDOMEventReceiver
  NS_IMPL_IDOMEVENTRECEIVER_USING_GENERIC(mInner)

  // nsIContent
  NS_IMPL_ICONTENT_USING_GENERIC(mInner)

  // nsIHTMLContent
  NS_IMPL_IHTMLCONTENT_USING_GENERIC(mInner)

protected:
  nsHTMLGenericLeafContent mInner;
};

nsresult
NS_NewHTMLArea(nsIHTMLContent** aInstancePtrResult, nsIAtom* aTag)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null ptr");
  if (nsnull == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }
  nsIHTMLContent* it = new nsHTMLArea(aTag);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return it->QueryInterface(kIHTMLContentIID, (void**) aInstancePtrResult);
}

nsHTMLArea::nsHTMLArea(nsIAtom* aTag)
{
  NS_INIT_REFCNT();
  mInner.Init(this, aTag);
}

nsHTMLArea::~nsHTMLArea()
{
}

NS_IMPL_ADDREF(nsHTMLArea)

NS_IMPL_RELEASE(nsHTMLArea)

nsresult
nsHTMLArea::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  NS_IMPL_HTML_CONTENT_QUERY_INTERFACE(aIID, aInstancePtr, this)
  if (aIID.Equals(kIDOMHTMLAreaElementIID)) {
    nsIDOMHTMLAreaElement* tmp = this;
    *aInstancePtr = (void*) tmp;
    AddRef();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

nsresult
nsHTMLArea::CloneNode(nsIDOMNode** aReturn)
{
  nsHTMLArea* it = new nsHTMLArea(mInner.mTag);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  mInner.CopyInnerTo(this, &it->mInner);
  return it->QueryInterface(kIDOMNodeIID, (void**) aReturn);
}

NS_IMPL_STRING_ATTR(nsHTMLArea, AccessKey, accesskey, eSetAttrNotify_None)
NS_IMPL_STRING_ATTR(nsHTMLArea, Alt, alt, eSetAttrNotify_None)
NS_IMPL_STRING_ATTR(nsHTMLArea, Coords, coords, eSetAttrNotify_None)
NS_IMPL_STRING_ATTR(nsHTMLArea, Href, href, eSetAttrNotify_None)
NS_IMPL_BOOL_ATTR(nsHTMLArea, NoHref, nohref, eSetAttrNotify_None)
NS_IMPL_STRING_ATTR(nsHTMLArea, Shape, shape, eSetAttrNotify_None)
NS_IMPL_INT_ATTR(nsHTMLArea, TabIndex, tabindex, eSetAttrNotify_None)
NS_IMPL_STRING_ATTR(nsHTMLArea, Target, target, eSetAttrNotify_None)

NS_IMETHODIMP
nsHTMLArea::StringToAttribute(nsIAtom* aAttribute,
                            const nsString& aValue,
                            nsHTMLValue& aResult)
{
  return NS_CONTENT_ATTR_NOT_THERE;
}

NS_IMETHODIMP
nsHTMLArea::AttributeToString(nsIAtom* aAttribute,
                            nsHTMLValue& aValue,
                            nsString& aResult) const
{
  return mInner.AttributeToString(aAttribute, aValue, aResult);
}

NS_IMETHODIMP
nsHTMLArea::MapAttributesInto(nsIStyleContext* aContext,
			      nsIPresContext* aPresContext)
{
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLArea::HandleDOMEvent(nsIPresContext& aPresContext,
			   nsEvent* aEvent,
			   nsIDOMEvent** aDOMEvent,
			   PRUint32 aFlags,
			   nsEventStatus& aEventStatus)
{
  return mInner.HandleDOMEvent(aPresContext, aEvent, aDOMEvent,
                               aFlags, aEventStatus);
}
