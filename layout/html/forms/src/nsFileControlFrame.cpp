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

#include "nsFileControlFrame.h"
#include "nsFormFrame.h"
#include "nsButtonControlFrame.h"
#include "nsTextControlFrame.h"
#include "nsIContent.h"
#include "prtypes.h"
#include "nsIAtom.h"
#include "nsIPresContext.h"
#include "nsIHTMLContent.h"
#include "nsHTMLIIDs.h"
#include "nsHTMLAtoms.h"
#include "nsIFileWidget.h"
#include "nsITextWidget.h"
#include "nsWidgetsCID.h"
#include "nsRepository.h"
#include "nsIView.h"
#include "nsHTMLParts.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIFormControl.h"
#include "nsINameSpaceManager.h"

// XXX make this pixels
#define CONTROL_SPACING 40  

static NS_DEFINE_IID(kCFileWidgetCID, NS_FILEWIDGET_CID);
static NS_DEFINE_IID(kIFileWidgetIID, NS_IFILEWIDGET_IID);
static NS_DEFINE_IID(kITextWidgetIID, NS_ITEXTWIDGET_IID);
static NS_DEFINE_IID(kIFormControlFrameIID, NS_IFORMCONTROLFRAME_IID);

nsresult
NS_NewFileControlFrame(nsIFrame*& aResult)
{
  aResult = new nsFileControlFrame;
  if (nsnull == aResult) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return NS_OK;
}

nsFileControlFrame::nsFileControlFrame()
  : nsHTMLContainerFrame()
{
  mTextFrame   = nsnull;
  mBrowseFrame = nsnull;
  mFormFrame   = nsnull;
}

nsresult
nsFileControlFrame::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  NS_PRECONDITION(0 != aInstancePtr, "null ptr");
  if (NULL == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(kIFormControlFrameIID)) {
    *aInstancePtr = (void*) ((nsIFormControlFrame*) this);
    return NS_OK;
  }
  return nsHTMLContainerFrame::QueryInterface(aIID, aInstancePtr);
}

PRBool
nsFileControlFrame::IsSuccessful(nsIFormControlFrame* aSubmitter)
{
  nsAutoString name;
  return (NS_CONTENT_ATTR_HAS_VALUE == GetName(&name));
}

void 
nsFileControlFrame::Reset()
{
  if (mTextFrame) {
    mTextFrame->Reset();
  }
}

NS_IMETHODIMP 
nsFileControlFrame::GetType(PRInt32* aType) const
{
  *aType = NS_FORM_INPUT_FILE;
  return NS_OK;
}

// XXX this should be removed when nsView exposes it
nsIWidget*
GetWindowTemp(nsIView *aView)
{
  nsIWidget *window = nsnull;

  nsIView *ancestor = aView;
  while (nsnull != ancestor) {
    ancestor->GetWidget(window);
	  if (nsnull != window) {
	    return window;
	  }
	  ancestor->GetParent(ancestor);
  }
  return nsnull;
}


void 
nsFileControlFrame::SetFocus(PRBool aOn, PRBool aRepaint)
{
  if (mTextFrame) {
    mTextFrame->SetFocus(aOn, aRepaint);
  }
}

// this is in response to the MouseClick from the containing browse button
// XXX still need to get filters from accept attribute
void nsFileControlFrame::MouseClicked(nsIPresContext* aPresContext)
{
  nsIView* textView;
  mTextFrame->GetView(textView);
  if (nsnull == textView) {
    return;
  }
  nsIWidget* widget;
  mTextFrame->GetWidget(&widget);
  if (!widget) {
    return;
  }
 
  nsITextWidget* textWidget;
  nsresult result = widget->QueryInterface(kITextWidgetIID, (void**)&textWidget);
  if (NS_OK != result) {
    NS_RELEASE(widget);
    return;
  }
  
  nsIView*   parentView;
  textView->GetParent(parentView);
  nsIWidget* parentWidget = GetWindowTemp(parentView);
 
  nsIFileWidget *fileWidget;

  nsString title("FileWidget Title <here> mode = save");
  nsRepository::CreateInstance(kCFileWidgetCID, nsnull, kIFileWidgetIID, (void**)&fileWidget);
  
  nsString titles[] = {"all files"};
  nsString filters[] = {"*.*"};
  fileWidget->SetFilterList(1, titles, filters);

  fileWidget->Create(parentWidget, title, eMode_load, nsnull, nsnull);
  result = fileWidget->Show();

  if (result) {
    PRUint32 size;
    nsString fileName;
    fileWidget->GetFile(fileName);
    textWidget->SetText(fileName,size);
  }
  NS_RELEASE(fileWidget);
  NS_RELEASE(parentWidget);
  NS_RELEASE(textWidget);
  NS_RELEASE(widget);
}


NS_IMETHODIMP nsFileControlFrame::Reflow(nsIPresContext&          aPresContext, 
                                         nsHTMLReflowMetrics&     aDesiredSize,
                                         const nsHTMLReflowState& aReflowState, 
                                         nsReflowStatus&          aStatus)
{
  PRInt32 numChildren = LengthOf(mFirstChild);
  
  nsIFrame* childFrame;
  if (0 == numChildren) {
    // XXX This code should move to Init(), someday when the frame construction
    // changes are all done and Init() is always getting called...
    PRBool disabled = nsFormFrame::GetDisabled(this);
    nsIHTMLContent* text = nsnull;
    nsIAtom* tag = NS_NewAtom("text");
    NS_NewHTMLInputElement(&text, tag);
    text->SetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::type, nsAutoString("text"), PR_FALSE);
    if (disabled) {
      text->SetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::disabled, nsAutoString("1"), PR_FALSE);  // XXX this should use an "empty" bool value
    }
    NS_NewTextControlFrame(childFrame);
    childFrame->Init(aPresContext, text, this, this, mStyleContext);
    mTextFrame = (nsTextControlFrame*)childFrame;
    mFirstChild = childFrame;

    nsIHTMLContent* browse = nsnull;
    tag = NS_NewAtom("browse");
    NS_NewHTMLInputElement(&browse, tag);
    browse->SetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::type, nsAutoString("browse"), PR_FALSE);
    if (disabled) {
      browse->SetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::disabled, nsAutoString("1"), PR_FALSE);  // XXX should be "empty"
    }
    NS_NewButtonControlFrame(childFrame);
    ((nsButtonControlFrame*)childFrame)->SetFileControlFrame(this);
    mBrowseFrame = (nsButtonControlFrame*)childFrame;
    childFrame->Init(aPresContext, browse, this, this, mStyleContext);

    mFirstChild->SetNextSibling(childFrame);

    NS_RELEASE(text);
    NS_RELEASE(browse);
  }

  nsSize maxSize = aReflowState.maxSize;
  nsHTMLReflowMetrics desiredSize = aDesiredSize;
  aDesiredSize.width = CONTROL_SPACING; 
  aDesiredSize.height = 0;
  childFrame = mFirstChild;
  nsPoint offset(0,0);
  while (nsnull != childFrame) {  // reflow, place, size the children
    nsHTMLReflowState   reflowState(aPresContext, childFrame, aReflowState,
                                    maxSize);
    nsIHTMLReflow*      htmlReflow;

    if (NS_OK == childFrame->QueryInterface(kIHTMLReflowIID, (void**)&htmlReflow)) {
      htmlReflow->WillReflow(aPresContext);
      nsresult result = htmlReflow->Reflow(aPresContext, desiredSize, reflowState, aStatus);
      NS_ASSERTION(NS_FRAME_IS_COMPLETE(aStatus), "bad status");
      nsRect rect(offset.x, offset.y, desiredSize.width, desiredSize.height);
      childFrame->SetRect(rect);
      maxSize.width  -= desiredSize.width;
      aDesiredSize.width  += desiredSize.width; 
      aDesiredSize.height = desiredSize.height;
      childFrame->GetNextSibling(childFrame);
      offset.x += desiredSize.width + CONTROL_SPACING;
    }
  }

  aDesiredSize.ascent = aDesiredSize.height;
  aDesiredSize.descent = 0;

  if (nsnull != aDesiredSize.maxElementSize) {
//XXX    aDesiredSize.AddBorderPaddingToMaxElementSize(borderPadding);
    aDesiredSize.maxElementSize->width = aDesiredSize.width;
	  aDesiredSize.maxElementSize->height = aDesiredSize.height;
  }

  aStatus = NS_FRAME_COMPLETE;
  return NS_OK;
}

PRIntn
nsFileControlFrame::GetSkipSides() const
{
  return 0;
}


NS_IMETHODIMP
nsFileControlFrame::GetName(nsString* aResult)
{
  nsresult result = NS_FORM_NOTOK;
  if (mContent) {
    nsIHTMLContent* formControl = nsnull;
    result = mContent->QueryInterface(kIHTMLContentIID, (void**)&formControl);
    if ((NS_OK == result) && formControl) {
      nsHTMLValue value;
      result = formControl->GetHTMLAttribute(nsHTMLAtoms::name, value);
      if (NS_CONTENT_ATTR_HAS_VALUE == result) {
        if (eHTMLUnit_String == value.GetUnit()) {
          value.GetStringValue(*aResult);
        }
      }
      NS_RELEASE(formControl);
    }
  }
  return result;
}

PRInt32 
nsFileControlFrame::GetMaxNumValues()
{
  return 1;
}
  
PRBool
nsFileControlFrame::GetNamesValues(PRInt32 aMaxNumValues, PRInt32& aNumValues,
                                   nsString* aValues, nsString* aNames)
{
  nsAutoString name;
  nsresult result = GetName(&name);
  if ((aMaxNumValues <= 0) || (NS_CONTENT_ATTR_HAS_VALUE != result)) {
    return PR_FALSE;
  }

  // use our name and the text widgets value 
  aNames[0] = name;
  nsresult status = PR_FALSE;
  nsIWidget*  widget;
  nsITextWidget* textWidget;
  mTextFrame->GetWidget(&widget);
  if (widget && (NS_OK == widget->QueryInterface(kITextWidgetIID, (void**)&textWidget))) {
    PRUint32 actualSize;
    textWidget->GetText(aValues[0], 0, actualSize);
    aNumValues = 1;
    NS_RELEASE(textWidget);
    status = PR_TRUE;
  }
  NS_IF_RELEASE(widget);
  return status;
}

NS_IMETHODIMP
nsFileControlFrame::GetFrameName(nsString& aResult) const
{
  return MakeFrameName("FileControl", aResult);
}
