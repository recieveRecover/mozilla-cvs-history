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
#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIScriptObjectOwner.h"
#include "nsIDOMEventReceiver.h"
#include "nsIHTMLContent.h"
#include "nsGenericHTMLElement.h"
#include "nsHTMLAtoms.h"
#include "nsHTMLIIDs.h"
#include "nsIStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIPresContext.h"
#include "nsIHTMLAttributes.h"
#include "nsIFormControl.h"
#include "nsIForm.h"
#include "nsIWidget.h"
#include "nsITextWidget.h"
#include "nsICheckButton.h"
#include "nsIRadioButton.h"

// XXX align=left, hspace, vspace, border? other nav4 attrs

static NS_DEFINE_IID(kIDOMHTMLInputElementIID, NS_IDOMHTMLINPUTELEMENT_IID);
static NS_DEFINE_IID(kIDOMHTMLFormElementIID, NS_IDOMHTMLFORMELEMENT_IID);
static NS_DEFINE_IID(kIFormIID, NS_IFORM_IID);
static NS_DEFINE_IID(kIFormControlIID, NS_IFORMCONTROL_IID);
static NS_DEFINE_IID(kITextWidgetIID, NS_ITEXTWIDGET_IID);
static NS_DEFINE_IID(kIRadioIID, NS_IRADIOBUTTON_IID);
static NS_DEFINE_IID(kICheckButtonIID, NS_ICHECKBUTTON_IID);

class nsHTMLInputElement : public nsIDOMHTMLInputElement,
                           public nsIScriptObjectOwner,
                           public nsIDOMEventReceiver,
                           public nsIHTMLContent,
                           public nsIFormControl
{
public:
  nsHTMLInputElement(nsIAtom* aTag);
  ~nsHTMLInputElement();

  // nsISupports
  NS_DECL_ISUPPORTS

  // nsIDOMNode
  NS_IMPL_IDOMNODE_USING_GENERIC(mInner)

  // nsIDOMElement
  NS_IMPL_IDOMELEMENT_USING_GENERIC(mInner)

  // nsIDOMHTMLElement
  NS_IMPL_IDOMHTMLELEMENT_USING_GENERIC(mInner)

  // nsIDOMHTMLInputElement
  NS_IMETHOD GetDefaultValue(nsString& aDefaultValue);
  NS_IMETHOD SetDefaultValue(const nsString& aDefaultValue);
  NS_IMETHOD GetDefaultChecked(PRBool* aDefaultChecked);
  NS_IMETHOD SetDefaultChecked(PRBool aDefaultChecked);
  NS_IMETHOD GetForm(nsIDOMHTMLFormElement** aForm);
  NS_IMETHOD GetAccept(nsString& aAccept);
  NS_IMETHOD SetAccept(const nsString& aAccept);
  NS_IMETHOD GetAccessKey(nsString& aAccessKey);
  NS_IMETHOD SetAccessKey(const nsString& aAccessKey);
  NS_IMETHOD GetAlign(nsString& aAlign);
  NS_IMETHOD SetAlign(const nsString& aAlign);
  NS_IMETHOD GetAlt(nsString& aAlt);
  NS_IMETHOD SetAlt(const nsString& aAlt);
  NS_IMETHOD GetChecked(PRBool* aChecked);
  NS_IMETHOD SetChecked(PRBool aChecked);
  NS_IMETHOD GetDisabled(PRBool* aDisabled);
  NS_IMETHOD SetDisabled(PRBool aDisabled);
  NS_IMETHOD GetMaxLength(PRInt32* aMaxLength);
  NS_IMETHOD SetMaxLength(PRInt32 aMaxLength);
  NS_IMETHOD GetName(nsString& aName);
  NS_IMETHOD SetName(const nsString& aName);
  NS_IMETHOD GetReadOnly(PRBool* aReadOnly);
  NS_IMETHOD SetReadOnly(PRBool aReadOnly);
  NS_IMETHOD GetSize(nsString& aSize);
  NS_IMETHOD SetSize(const nsString& aSize);
  NS_IMETHOD GetSrc(nsString& aSrc);
  NS_IMETHOD SetSrc(const nsString& aSrc);
  NS_IMETHOD GetTabIndex(PRInt32* aTabIndex);
  NS_IMETHOD SetTabIndex(PRInt32 aTabIndex);
  NS_IMETHOD GetType(nsString& aType);
  NS_IMETHOD GetUseMap(nsString& aUseMap);
  NS_IMETHOD SetUseMap(const nsString& aUseMap);
  NS_IMETHOD GetValue(nsString& aValue);
  NS_IMETHOD SetValue(const nsString& aValue);
  NS_IMETHOD Blur();
  NS_IMETHOD Focus();
  NS_IMETHOD Select();
  NS_IMETHOD Click();

  // nsIScriptObjectOwner
  NS_IMPL_ISCRIPTOBJECTOWNER_USING_GENERIC(mInner)

  // nsIDOMEventReceiver
  NS_IMPL_IDOMEVENTRECEIVER_USING_GENERIC(mInner)

  // nsIContent
  NS_IMPL_ICONTENT_USING_GENERIC(mInner)

  // nsIHTMLContent
  NS_IMPL_IHTMLCONTENT_USING_GENERIC(mInner)

  // nsIFormControl
  NS_IMETHOD SetForm(nsIDOMHTMLFormElement* aForm);
  NS_IMETHOD GetType(PRInt32* aType);
  NS_IMETHOD SetWidget(nsIWidget* aWidget);
  NS_IMETHOD Init() { return NS_OK; }

protected:
  nsGenericHTMLLeafElement mInner;
  nsIWidget*               mWidget; // XXX this needs to go away when FindFrameWithContent is efficient
  nsIForm*                 mForm;
  PRInt32                  mType;
};

// construction, destruction

nsresult
NS_NewHTMLInputElement(nsIHTMLContent** aInstancePtrResult, nsIAtom* aTag)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null ptr");
  if (nsnull == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }
  nsIHTMLContent* it = new nsHTMLInputElement(aTag);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return it->QueryInterface(kIHTMLContentIID, (void**) aInstancePtrResult);
}

nsHTMLInputElement::nsHTMLInputElement(nsIAtom* aTag)
{
  NS_INIT_REFCNT();
  mInner.Init(this, aTag);
  mType = NS_FORM_INPUT_TEXT; // default value
  mForm = nsnull;
  mWidget = nsnull;
  nsTraceRefcnt::Create((nsIFormControl*)this, "nsHTMLFormControlElement", __FILE__, __LINE__);
}

nsHTMLInputElement::~nsHTMLInputElement()
{
  NS_IF_RELEASE(mWidget);
  if (nsnull != mForm) {
    // prevent mForm from decrementing its ref count on us
    mForm->RemoveElement(this, PR_FALSE); 
    NS_RELEASE(mForm);
  }
  nsTraceRefcnt::Destroy((nsIFormControl*)this, __FILE__, __LINE__);
}

// nsISupports

NS_IMETHODIMP_(nsrefcnt) 
nsHTMLInputElement::AddRef(void)
{
  nsTraceRefcnt::AddRef((nsIFormControl*)this, mRefCnt+1, __FILE__, __LINE__);
  PRInt32 refCnt = mRefCnt;  // debugging 
  return ++mRefCnt; 
}


NS_IMETHODIMP
nsHTMLInputElement::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  NS_IMPL_HTML_CONTENT_QUERY_INTERFACE(aIID, aInstancePtr, this)
  if (aIID.Equals(kIDOMHTMLInputElementIID)) {
    *aInstancePtr = (void*)(nsIDOMHTMLInputElement*) this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  else if (aIID.Equals(kIFormControlIID)) {
    *aInstancePtr = (void*)(nsIFormControl*) this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

NS_IMETHODIMP_(nsrefcnt)
nsHTMLInputElement::Release()
{
  nsTraceRefcnt::Release((nsIFormControl*)this, mRefCnt-1, __FILE__, __LINE__);
  --mRefCnt;
	if (mRefCnt <= 0) {
    delete this;                                       
    return 0;                                          
  } else if ((1 == mRefCnt) && mForm) { 
    mRefCnt = 0;
    delete this;
    return 0;
  } else {
    return mRefCnt;
  }
}

// nsIDOMHTMLInputElement

nsresult
nsHTMLInputElement::CloneNode(PRBool aDeep, nsIDOMNode** aReturn)
{
  nsHTMLInputElement* it = new nsHTMLInputElement(mInner.mTag);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  mInner.CopyInnerTo(this, &it->mInner);
  return it->QueryInterface(kIDOMNodeIID, (void**) aReturn);
}

NS_IMETHODIMP
nsHTMLInputElement::GetForm(nsIDOMHTMLFormElement** aForm)
{
  nsresult result = NS_OK;
  *aForm = nsnull;
  if (nsnull != mForm) {
    nsIDOMHTMLFormElement* formElem = nsnull;
    result = mForm->QueryInterface(kIDOMHTMLFormElementIID, (void**)&formElem);
    if (NS_OK == result) {
      *aForm = formElem;
    }
  }
  return result;
}

NS_IMPL_STRING_ATTR(nsHTMLInputElement, DefaultValue, defaultvalue)
NS_IMPL_BOOL_ATTR(nsHTMLInputElement, DefaultChecked, defaultchecked)
NS_IMPL_STRING_ATTR(nsHTMLInputElement, Accept, accept)
NS_IMPL_STRING_ATTR(nsHTMLInputElement, AccessKey, accesskey)
NS_IMPL_STRING_ATTR(nsHTMLInputElement, Align, align)
NS_IMPL_STRING_ATTR(nsHTMLInputElement, Alt, alt)
//NS_IMPL_BOOL_ATTR(nsHTMLInputElement, Checked, checked)
NS_IMPL_BOOL_ATTR(nsHTMLInputElement, Disabled, disabled)
NS_IMPL_INT_ATTR(nsHTMLInputElement, MaxLength, maxlength)
NS_IMPL_STRING_ATTR(nsHTMLInputElement, Name, name)
NS_IMPL_BOOL_ATTR(nsHTMLInputElement, ReadOnly, readonly)
NS_IMPL_STRING_ATTR(nsHTMLInputElement, Size, size)
NS_IMPL_STRING_ATTR(nsHTMLInputElement, Src, src)
NS_IMPL_INT_ATTR(nsHTMLInputElement, TabIndex, tabindex)
NS_IMPL_STRING_ATTR(nsHTMLInputElement, UseMap, usemap)
//NS_IMPL_STRING_ATTR(nsHTMLInputElement, Value, value)

NS_IMETHODIMP
nsHTMLInputElement::GetType(nsString& aValue)
{
  mInner.GetAttribute(nsHTMLAtoms::type, aValue);
  return NS_OK;
}

NS_IMETHODIMP 
nsHTMLInputElement::GetValue(nsString& aValue)
{
  PRInt32 type;
  GetType(&type);
  if (NS_FORM_INPUT_TEXT == type) {
    if (nsnull != mWidget) {
      nsITextWidget* text = nsnull;
      if (NS_OK == mWidget->QueryInterface(kITextWidgetIID,(void**)&text)) {
        PRUint32 size;
        text->GetText(aValue,0,size); 
        NS_RELEASE(text);
        return NS_OK;
      }
    }
  } 

  return mInner.GetAttribute(nsHTMLAtoms::value, aValue);
}


NS_IMETHODIMP 
nsHTMLInputElement::SetValue(const nsString& aValue)
{
  PRInt32 type;
  GetType(&type);
  if (NS_FORM_INPUT_TEXT == type) {
    if (nsnull != mWidget) {
      nsITextWidget* text = nsnull;
      if (NS_OK == mWidget->QueryInterface(kITextWidgetIID,(void**)&text)) {
        PRUint32 size;
        text->SetText(aValue,size); 
        NS_RELEASE(text);
        return NS_OK;
      }
    }
  }

  return mInner.SetAttribute(nsHTMLAtoms::value, aValue, PR_TRUE); 
}

NS_IMETHODIMP 
nsHTMLInputElement::GetChecked(PRBool* aValue)
{
  PRInt32 type;
  GetType(&type);
  if (NS_FORM_INPUT_CHECKBOX == type) {
    if (nsnull != mWidget) {
      nsICheckButton* checkbox = nsnull;
      if (NS_OK == mWidget->QueryInterface(kICheckButtonIID,(void**)&checkbox)) {
        checkbox->GetState(*aValue); 
        NS_RELEASE(checkbox);
        return NS_OK;
      }
    }
  } else if (NS_FORM_INPUT_RADIO == type) {
    if (nsnull != mWidget) {
      nsIRadioButton* radio = nsnull;
      if (NS_OK == mWidget->QueryInterface(kIRadioIID,(void**)&radio)) {
        radio->GetState(*aValue); 
        NS_RELEASE(radio);
        return NS_OK;
      }
    }
  } 
  
  nsHTMLValue val;                                                 
  nsresult rv = mInner.GetAttribute(nsHTMLAtoms::checked, val);       
  *aValue = (NS_CONTENT_ATTR_NOT_THERE != rv);                        
  return NS_OK;                                                     
}


NS_IMETHODIMP 
nsHTMLInputElement::SetChecked(PRBool aValue)
{
  PRInt32 type;
  GetType(&type);
  if (NS_FORM_INPUT_CHECKBOX == type) {
    if (nsnull != mWidget) {
      nsICheckButton* checkbox = nsnull;
      if (NS_OK == mWidget->QueryInterface(kICheckButtonIID,(void**)&checkbox)) {
        checkbox->SetState(aValue); 
        NS_RELEASE(checkbox);
        return NS_OK;
      }
    }
  } else if (NS_FORM_INPUT_RADIO == type) {
    if (nsnull != mWidget) {
      nsIRadioButton* radio = nsnull;
      if (NS_OK == mWidget->QueryInterface(kIRadioIID,(void**)&radio)) {
        radio->SetState(aValue); 
        NS_RELEASE(radio);
        return NS_OK;
      }
    }
  } 
  
  nsAutoString empty;                                               
  if (aValue) {                                                     
    return mInner.SetAttribute(nsHTMLAtoms::checked, empty, PR_TRUE); 
  } else {                                                            
    mInner.UnsetAttribute(nsHTMLAtoms::checked, PR_TRUE);             
    return NS_OK;                                                   
  }                                                                 
}

NS_IMETHODIMP
nsHTMLInputElement::Blur()
{
  if (nsnull != mWidget) {
    nsIWidget *mParentWidget = mWidget->GetParent();
    if (nsnull != mParentWidget) {
      mParentWidget->SetFocus();
      NS_RELEASE(mParentWidget);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLInputElement::Focus()
{
  if (nsnull != mWidget) {
    mWidget->SetFocus();
  }
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLInputElement::Select()
{
  if ((NS_FORM_INPUT_TEXT == mType) && (nsnull != mWidget)) {
    nsITextWidget *textWidget;
    if (NS_OK == mWidget->QueryInterface(kITextWidgetIID, (void**)&textWidget)) {
      textWidget->SelectAll();
      NS_RELEASE(textWidget);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLInputElement::Click()
{
  //XXX TBI
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHTMLInputElement::HandleDOMEvent(nsIPresContext& aPresContext,
                            nsEvent* aEvent,
                            nsIDOMEvent** aDOMEvent,
                            PRUint32 aFlags,
                            nsEventStatus& aEventStatus)
{
  return mInner.HandleDOMEvent(aPresContext, aEvent, aDOMEvent,
                               aFlags, aEventStatus);
}

// nsIHTMLContent

static nsGenericHTMLElement::EnumTable kInputTypeTable[] = {
  { "browse", NS_FORM_BROWSE }, // XXX not valid html, but it is convient
  { "button", NS_FORM_INPUT_BUTTON },
  { "checkbox", NS_FORM_INPUT_CHECKBOX },
  { "file", NS_FORM_INPUT_FILE },
  { "hidden", NS_FORM_INPUT_HIDDEN },
  { "reset", NS_FORM_INPUT_RESET },
  { "image", NS_FORM_INPUT_IMAGE },
  { "password", NS_FORM_INPUT_PASSWORD },
  { "radio", NS_FORM_INPUT_RADIO },
  { "submit", NS_FORM_INPUT_SUBMIT },
  { "text", NS_FORM_INPUT_TEXT },
  { 0 }
};

NS_IMETHODIMP
nsHTMLInputElement::StringToAttribute(nsIAtom* aAttribute,
                               const nsString& aValue,
                               nsHTMLValue& aResult)
{
  if (aAttribute == nsHTMLAtoms::type) {
    nsGenericHTMLElement::EnumTable *table = kInputTypeTable;
    while (nsnull != table->tag) { 
      if (aValue.EqualsIgnoreCase(table->tag)) {
        aResult.SetIntValue(table->value, eHTMLUnit_Enumerated);
        mType = table->value;  // set the type of this input 
        return NS_CONTENT_ATTR_HAS_VALUE;
      }
      table++;
    }
  }
  else if (aAttribute == nsHTMLAtoms::checked) {
    aResult.SetEmptyValue();
    return NS_CONTENT_ATTR_HAS_VALUE;
  }
  else if (aAttribute == nsHTMLAtoms::disabled) {
    aResult.SetEmptyValue();
    return NS_CONTENT_ATTR_HAS_VALUE;
  }
  else if (aAttribute == nsHTMLAtoms::readonly) {
    aResult.SetEmptyValue();
    return NS_CONTENT_ATTR_HAS_VALUE;
  }
  else if (aAttribute == nsHTMLAtoms::width) {
    nsGenericHTMLElement::ParseValueOrPercent(aValue, aResult,
                                              eHTMLUnit_Pixel);
    return NS_CONTENT_ATTR_HAS_VALUE;
  } 
  else if (aAttribute == nsHTMLAtoms::height) {
    nsGenericHTMLElement::ParseValueOrPercent(aValue, aResult,
                                              eHTMLUnit_Pixel);
    return NS_CONTENT_ATTR_HAS_VALUE;
  } 
  else if (aAttribute == nsHTMLAtoms::maxlength) {
    nsGenericHTMLElement::ParseValue(aValue, 0, aResult, eHTMLUnit_Integer);
    return NS_CONTENT_ATTR_HAS_VALUE;
  }
  else if (aAttribute == nsHTMLAtoms::size) {
    nsGenericHTMLElement::ParseValue(aValue, 0, aResult, eHTMLUnit_Integer);
    return NS_CONTENT_ATTR_HAS_VALUE;
  }
  else if (aAttribute == nsHTMLAtoms::tabindex) {
    nsGenericHTMLElement::ParseValue(aValue, 0, aResult, eHTMLUnit_Integer);
    return NS_CONTENT_ATTR_HAS_VALUE;
  }
  else if (aAttribute == nsHTMLAtoms::align) {
    if (nsGenericHTMLElement::ParseFormAlignValue(aValue, aResult)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  return NS_CONTENT_ATTR_NOT_THERE;
}

NS_IMETHODIMP
nsHTMLInputElement::AttributeToString(nsIAtom* aAttribute,
                               nsHTMLValue& aValue,
                               nsString& aResult) const
{
  if (aAttribute == nsHTMLAtoms::type) {
    if (eHTMLUnit_Enumerated == aValue.GetUnit()) {
      nsGenericHTMLElement::EnumValueToString(aValue, kInputTypeTable, aResult);
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (aAttribute == nsHTMLAtoms::align) {
    if (eHTMLUnit_Enumerated == aValue.GetUnit()) {
      nsGenericHTMLElement::FormAlignValueToString(aValue, aResult);
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  return mInner.AttributeToString(aAttribute, aValue, aResult);
}

static void
MapAttributesInto(nsIHTMLAttributes* aAttributes,
                  nsIStyleContext* aContext,
                  nsIPresContext* aPresContext)
{
  nsHTMLValue value;

  aAttributes->GetAttribute(nsHTMLAtoms::align, value);
  if (eHTMLUnit_Enumerated == value.GetUnit()) {
    nsStyleDisplay* display = (nsStyleDisplay*)
      aContext->GetMutableStyleData(eStyleStruct_Display);
    nsStyleText* text = (nsStyleText*)
      aContext->GetMutableStyleData(eStyleStruct_Text);
    switch (value.GetIntValue()) {
    case NS_STYLE_TEXT_ALIGN_LEFT:
      display->mFloats = NS_STYLE_FLOAT_LEFT;
      break;
    case NS_STYLE_TEXT_ALIGN_RIGHT:
      display->mFloats = NS_STYLE_FLOAT_RIGHT;
      break;
    default:
      text->mVerticalAlign.SetIntValue(value.GetIntValue(), eStyleUnit_Enumerated);
      break;
    }
  }

  aAttributes->GetAttribute(nsHTMLAtoms::type, value);
  if (eHTMLUnit_Enumerated == value.GetUnit()) {  
    switch (value.GetIntValue()) {
      case NS_FORM_INPUT_CHECKBOX:
      case NS_FORM_INPUT_RADIO: 
      {
        float p2t = aPresContext->GetPixelsToTwips();
        nscoord pad = NSIntPixelsToTwips(3, p2t);

        // add left and right padding around the radio button via css
        nsStyleSpacing* spacing = (nsStyleSpacing*) aContext->GetMutableStyleData(eStyleStruct_Spacing);
        if (eStyleUnit_Null == spacing->mMargin.GetLeftUnit()) {
          nsStyleCoord left(pad);
          spacing->mMargin.SetLeft(left);
        }
        if (eStyleUnit_Null == spacing->mMargin.GetRightUnit()) {
          nsStyleCoord right(NSIntPixelsToTwips(5, p2t));
          spacing->mMargin.SetRight(right);
        }
        // add bottom padding if backward mode
        // XXX why isn't this working?
        nsCompatibility mode;
        aPresContext->GetCompatibilityMode(mode);
        if (eCompatibility_NavQuirks == mode) {
          if (eStyleUnit_Null == spacing->mMargin.GetBottomUnit()) {
            nsStyleCoord bottom(pad);
            spacing->mMargin.SetBottom(bottom);
          }
        }
        break;
      }
      case NS_FORM_INPUT_IMAGE:
      {
        // Apply the image border as well. For form elements the color is
        // always forced to blue.
        static nscolor blue[4] = {
          NS_RGB(0, 0, 255),
          NS_RGB(0, 0, 255),
          NS_RGB(0, 0, 255),
          NS_RGB(0, 0, 255)
        };
        nsGenericHTMLElement::MapImageBorderAttributesInto(aAttributes, aContext, aPresContext, blue);
        nsGenericHTMLElement::MapImageAttributesInto(aAttributes, aContext, aPresContext);
        break;
      }
    }
  }
  nsGenericHTMLElement::MapCommonAttributesInto(aAttributes, aContext, aPresContext);
}

NS_IMETHODIMP
nsHTMLInputElement::GetAttributeMappingFunction(nsMapAttributesFunc& aMapFunc) const
{
  aMapFunc = &MapAttributesInto;
  return NS_OK;
}

// nsIFormControl

// An important assumption is that if aForm is null, the previous mForm will not be released
// This allows nsHTMLFormElement to deal with circular references.
NS_IMETHODIMP
nsHTMLInputElement::SetForm(nsIDOMHTMLFormElement* aForm)
{
  nsresult result = NS_OK;
	if (nsnull == aForm) {
    mForm = nsnull;
    return NS_OK;
  } else {
    NS_IF_RELEASE(mForm);
    nsIFormControl* formControl = nsnull;
    result = QueryInterface(kIFormControlIID, (void**)&formControl);
    if ((NS_OK == result) && formControl) {
      result = aForm->QueryInterface(kIFormIID, (void**)&mForm); // keep the ref
      if ((NS_OK == result) && mForm) {
        mForm->AddElement(formControl);
      }
      NS_RELEASE(formControl);
    }
  }
  return result;
}

NS_IMETHODIMP
nsHTMLInputElement::GetType(PRInt32* aType)
{
  if (aType) {
    *aType = mType;
    return NS_OK;
  } else {
    return NS_FORM_NOTOK;
  }
}

NS_IMETHODIMP
nsHTMLInputElement::SetWidget(nsIWidget* aWidget)
{
  if (aWidget != mWidget) {
	  NS_IF_RELEASE(mWidget);
    NS_IF_ADDREF(aWidget);
    mWidget = aWidget;
  }
  return NS_OK;
}
