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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */
#include "nsIDOMHTMLFrameElement.h"
#include "nsIScriptObjectOwner.h"
#include "nsIDOMEventReceiver.h"
#include "nsIHTMLContent.h"
#include "nsGenericHTMLElement.h"
#include "nsHTMLAtoms.h"
#include "nsHTMLIIDs.h"
#include "nsIStyleContext.h"
#include "nsIMutableStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIPresContext.h"
#include "nsIPresShell.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIWebNavigation.h"
#include "nsIChromeEventHandler.h"
#include "nsDOMError.h"


class nsHTMLFrameElement : public nsGenericHTMLLeafElement,
                           public nsIDOMHTMLFrameElement,
                           public nsIChromeEventHandler
{
public:
  nsHTMLFrameElement();
  virtual ~nsHTMLFrameElement();

  // nsISupports
  NS_DECL_ISUPPORTS_INHERITED

  // nsIDOMNode
  NS_FORWARD_IDOMNODE_NO_CLONENODE(nsGenericHTMLLeafElement::)

  // nsIDOMElement
  NS_FORWARD_IDOMELEMENT(nsGenericHTMLLeafElement::)

  // nsIDOMHTMLElement
  NS_FORWARD_IDOMHTMLELEMENT(nsGenericHTMLLeafElement::)

  // nsIDOMHTMLFrameElement
  NS_DECL_IDOMHTMLFRAMEELEMENT

  // nsIChromeEventHandler
  NS_DECL_NSICHROMEEVENTHANDLER

  NS_IMETHOD StringToAttribute(nsIAtom* aAttribute,
                               const nsAReadableString& aValue,
                               nsHTMLValue& aResult);
  NS_IMETHOD AttributeToString(nsIAtom* aAttribute,
                               const nsHTMLValue& aValue,
                               nsAWritableString& aResult) const;
  NS_IMETHOD SizeOf(nsISizeOfHandler* aSizer, PRUint32* aResult) const;
};

nsresult
NS_NewHTMLFrameElement(nsIHTMLContent** aInstancePtrResult,
                       nsINodeInfo *aNodeInfo)
{
  NS_ENSURE_ARG_POINTER(aInstancePtrResult);

  nsHTMLFrameElement* it = new nsHTMLFrameElement();

  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsresult rv = it->Init(aNodeInfo);

  if (NS_FAILED(rv)) {
    delete it;

    return rv;
  }

  *aInstancePtrResult = NS_STATIC_CAST(nsIHTMLContent *, it);
  NS_ADDREF(*aInstancePtrResult);

  return NS_OK;
}


nsHTMLFrameElement::nsHTMLFrameElement()
{
}

nsHTMLFrameElement::~nsHTMLFrameElement()
{
}


NS_IMPL_ADDREF_INHERITED(nsHTMLFrameElement, nsGenericElement);
NS_IMPL_RELEASE_INHERITED(nsHTMLFrameElement, nsGenericElement);

NS_IMPL_HTMLCONTENT_QI2(nsHTMLFrameElement, nsGenericHTMLLeafElement,
                        nsIDOMHTMLFrameElement, nsIChromeEventHandler);


nsresult
nsHTMLFrameElement::CloneNode(PRBool aDeep, nsIDOMNode** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = nsnull;

  nsHTMLFrameElement* it = new nsHTMLFrameElement();

  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsCOMPtr<nsIDOMNode> kungFuDeathGrip(it);

  nsresult rv = it->Init(mNodeInfo);

  if (NS_FAILED(rv))
    return rv;

  CopyInnerTo(this, it, aDeep);

  *aReturn = NS_STATIC_CAST(nsIDOMNode *, it);

  NS_ADDREF(*aReturn);

  return NS_OK;
}


NS_IMPL_STRING_ATTR(nsHTMLFrameElement, FrameBorder, frameborder)
NS_IMPL_STRING_ATTR(nsHTMLFrameElement, LongDesc, longdesc)
NS_IMPL_STRING_ATTR(nsHTMLFrameElement, MarginHeight, marginheight)
NS_IMPL_STRING_ATTR(nsHTMLFrameElement, MarginWidth, marginwidth)
NS_IMPL_STRING_ATTR(nsHTMLFrameElement, Name, name)
NS_IMPL_BOOL_ATTR(nsHTMLFrameElement, NoResize, noresize)
NS_IMPL_STRING_ATTR(nsHTMLFrameElement, Scrolling, scrolling)
NS_IMPL_STRING_ATTR(nsHTMLFrameElement, Src, src)


NS_IMETHODIMP
nsHTMLFrameElement::GetContentDocument(nsIDOMDocument** aContentDocument)
{
  NS_ENSURE_ARG_POINTER(aContentDocument);

  *aContentDocument = nsnull;

  NS_ENSURE_TRUE(mDocument, NS_OK);

  nsCOMPtr<nsIPresShell> presShell;

  presShell = dont_AddRef(mDocument->GetShellAt(0));
  NS_ENSURE_TRUE(presShell, NS_OK);

  nsCOMPtr<nsISupports> tmp;

  presShell->GetSubShellFor(this, getter_AddRefs(tmp));
  NS_ENSURE_TRUE(tmp, NS_OK);

  nsCOMPtr<nsIWebNavigation> webNav = do_QueryInterface(tmp);
  NS_ENSURE_TRUE(webNav, NS_OK);

  nsCOMPtr<nsIDOMDocument> domDoc;

  webNav->GetDocument(getter_AddRefs(domDoc));

  *aContentDocument = domDoc;

  NS_IF_ADDREF(*aContentDocument);

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLFrameElement::SetContentDocument(nsIDOMDocument* aContentDocument)
{
  return NS_ERROR_DOM_INVALID_MODIFICATION_ERR;
}

NS_IMETHODIMP
nsHTMLFrameElement::StringToAttribute(nsIAtom* aAttribute,
                                      const nsAReadableString& aValue,
                                      nsHTMLValue& aResult)
{
  if (aAttribute == nsHTMLAtoms::bordercolor) {
    if (ParseColor(aValue, mDocument, aResult)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (aAttribute == nsHTMLAtoms::frameborder) {
    // XXX need to check for correct mode
    if (ParseFrameborderValue(PR_FALSE, aValue, aResult)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (aAttribute == nsHTMLAtoms::marginwidth) {
    if (ParseValueOrPercent(aValue, aResult, eHTMLUnit_Pixel)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (aAttribute == nsHTMLAtoms::marginheight) {
    if (ParseValueOrPercent(aValue, aResult, eHTMLUnit_Pixel)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (aAttribute == nsHTMLAtoms::noresize) {
    aResult.SetEmptyValue();
    return NS_CONTENT_ATTR_HAS_VALUE;
  }
  else if (aAttribute == nsHTMLAtoms::scrolling) {
    if (ParseScrollingValue(PR_FALSE, aValue, aResult)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }

  return NS_CONTENT_ATTR_NOT_THERE;
}

NS_IMETHODIMP
nsHTMLFrameElement::AttributeToString(nsIAtom* aAttribute,
                                      const nsHTMLValue& aValue,
                                      nsAWritableString& aResult) const
{
  if (aAttribute == nsHTMLAtoms::frameborder) {
    FrameborderValueToString(PR_FALSE, aValue, aResult);
    return NS_CONTENT_ATTR_HAS_VALUE;
  } 
  else if (aAttribute == nsHTMLAtoms::scrolling) {
    ScrollingValueToString(PR_FALSE, aValue, aResult);
    return NS_CONTENT_ATTR_HAS_VALUE;
  }

  return nsGenericHTMLLeafElement::AttributeToString(aAttribute, aValue,
                                                     aResult);
}

NS_IMETHODIMP
nsHTMLFrameElement::SizeOf(nsISizeOfHandler* aSizer, PRUint32* aResult) const
{
  *aResult = sizeof(*this) + BaseSizeOf(aSizer);

  return NS_OK;
}


//*****************************************************************************
// nsHTMLFrameElement::nsIChromeEventHandler
//*****************************************************************************   

NS_IMETHODIMP
nsHTMLFrameElement::HandleChromeEvent(nsIPresContext* aPresContext,
                                      nsEvent* aEvent,
                                      nsIDOMEvent** aDOMEvent,
                                      PRUint32 aFlags, 
                                      nsEventStatus* aEventStatus)
{
  return HandleDOMEvent(aPresContext, aEvent, aDOMEvent, aFlags,aEventStatus);
}
