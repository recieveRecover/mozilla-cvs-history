/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsGfxButtonControlFrame.h"
#include "nsIButton.h"
#include "nsWidgetsCID.h"
#include "nsIFontMetrics.h"
#include "nsFormControlFrame.h"
#include "nsISupportsArray.h"
#include "nsINameSpaceManager.h"
#include "nsContentCID.h"
#ifdef ACCESSIBILITY
#include "nsIAccessibilityService.h"
#endif
#include "nsIServiceManager.h"
#include "nsIDOMNode.h"
#include "nsLayoutAtoms.h"
#include "nsReflowPath.h"
// MouseEvent suppression in PP
#include "nsGUIEvent.h"

static NS_DEFINE_CID(kTextNodeCID,   NS_TEXTNODE_CID);

// Saving PresState
#include "nsIPresState.h"
#include "nsIHTMLContent.h"

const nscoord kSuggestedNotSet = -1;

nsGfxButtonControlFrame::nsGfxButtonControlFrame()
{
  mSuggestedWidth  = kSuggestedNotSet;
  mSuggestedHeight = kSuggestedNotSet;
}

nsresult
NS_NewGfxButtonControlFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsGfxButtonControlFrame* it = new (aPresShell) nsGfxButtonControlFrame;
  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  *aNewFrame = it;
  return NS_OK;
}
      
NS_IMETHODIMP
nsGfxButtonControlFrame::GetFrameType(nsIAtom** aType) const
{
  NS_PRECONDITION(nsnull != aType, "null OUT parameter pointer");
  *aType = nsLayoutAtoms::gfxButtonControlFrame; 
  NS_ADDREF(*aType);
  return NS_OK;
}

// Special check for the browse button of a file input.
//
// Since this is actually type "NS_FORM_INPUT_BUTTON", we
// can't tell if it is part of a file input by the type.
// We'll return PR_TRUE if either:
// (a) type is NS_FORM_BROWSE or
// (b) type is NS_FORM_INPUT_BUTTON and our parent is a file input
PRBool
nsGfxButtonControlFrame::IsFileBrowseButton(PRInt32 type)
{
  PRBool rv = PR_FALSE;
  if (NS_FORM_BROWSE == type) {
    rv = PR_TRUE;
  }
  else if (NS_FORM_INPUT_BUTTON == type) {
  
    // Check to see if parent is a file input
    nsresult result;
    nsCOMPtr<nsIContent> parentContent;
    result = mContent->GetParent(*getter_AddRefs(parentContent));
    if (NS_SUCCEEDED(result) && parentContent) {
      nsCOMPtr<nsIAtom> atom;
      result = parentContent->GetTag(*getter_AddRefs(atom));
      if (NS_SUCCEEDED(result) && atom) {
        if (atom.get() == nsHTMLAtoms::input) {

          // It's an input, is it a file input?
          nsAutoString value;
          if (NS_CONTENT_ATTR_HAS_VALUE == parentContent->GetAttr(kNameSpaceID_None, nsHTMLAtoms::type, value)) {
            if (value.EqualsIgnoreCase("file")) {
              rv = PR_TRUE;
            }
          }
        }
      }
    }
  }
  return rv;
}

/*
 * FIXME: this ::GetIID() method has no meaning in life and should be
 * removed.
 * Pierre Phaneuf <pp@ludusdesign.com>
 */
const nsIID&
nsGfxButtonControlFrame::GetIID()
{
  return NS_GET_IID(nsIButton);
}
  
const nsIID&
nsGfxButtonControlFrame::GetCID()
{
  static NS_DEFINE_IID(kButtonCID, NS_BUTTON_CID);
  return kButtonCID;
}

#ifdef DEBUG
NS_IMETHODIMP
nsGfxButtonControlFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("ButtonControl"), aResult);
}
#endif

NS_IMETHODIMP 
nsGfxButtonControlFrame::AddComputedBorderPaddingToDesiredSize(nsHTMLReflowMetrics& aDesiredSize,
                                                               const nsHTMLReflowState& aSuggestedReflowState)
{
  if (kSuggestedNotSet == mSuggestedWidth) {
    aDesiredSize.width  += aSuggestedReflowState.mComputedBorderPadding.left + aSuggestedReflowState.mComputedBorderPadding.right;
  }

  if (kSuggestedNotSet == mSuggestedHeight) {
    aDesiredSize.height += aSuggestedReflowState.mComputedBorderPadding.top + aSuggestedReflowState.mComputedBorderPadding.bottom;
  }
  return NS_OK;
}

// Create the text content used as label for the button.
// The frame will be generated by the frame constructor.
NS_IMETHODIMP
nsGfxButtonControlFrame::CreateAnonymousContent(nsIPresContext* aPresContext,
                                                nsISupportsArray& aChildList)
{
  nsresult result;

  // Get the text from the "value" attribute.
  // If it is zero length, set it to a default value (localized)
  nsAutoString initvalue, value;
  result = GetValue(&initvalue);
  value = initvalue;
  if (result != NS_CONTENT_ATTR_HAS_VALUE && value.IsEmpty()) {
    // Generate localized label.
    // We can't make any assumption as to what the default would be
    // because the value is localized for non-english platforms, thus
    // it might not be the string "Reset", "Submit Query", or "Browse..."
    result = GetDefaultLabel(value);
    if (NS_FAILED(result)) {
      return result;
    }
  }

  // Compress whitespace out of label if needed.
  const nsStyleText* textStyle;
  GetStyleData(eStyleStruct_Text,  (const nsStyleStruct *&)textStyle);
  if (!textStyle->WhiteSpaceIsSignificant()) {
    value.CompressWhitespace();
  }

  // Add a child text content node for the label
  nsCOMPtr<nsIContent> labelContent(do_CreateInstance(kTextNodeCID,&result));
  if (NS_SUCCEEDED(result) && labelContent) {
    // set the value of the text node and add it to the child list
    mTextContent = do_QueryInterface(labelContent, &result);
    if (NS_SUCCEEDED(result) && mTextContent) {
      mTextContent->SetText(value.get(), value.Length(), PR_TRUE);
      aChildList.AppendElement(mTextContent);
    }
  }
  return result;
}

// Create the text content used as label for the button.
// The frame will be generated by the frame constructor.
NS_IMETHODIMP
nsGfxButtonControlFrame::CreateFrameFor(nsIPresContext*   aPresContext,
                                        nsIContent *      aContent,
                                        nsIFrame**        aFrame)
{
  nsIFrame * newFrame = nsnull;
  nsresult rv = NS_ERROR_FAILURE;

  if (aFrame) 
    *aFrame = nsnull; 

  nsCOMPtr<nsIContent> content(do_QueryInterface(mTextContent));
  if (aContent == content.get()) {
    nsCOMPtr<nsIPresShell> shell;
    aPresContext->GetShell(getter_AddRefs(shell));

    nsIFrame * parentFrame = mFrames.FirstChild();
    nsCOMPtr<nsIStyleContext> styleContext;
    parentFrame->GetStyleContext(getter_AddRefs(styleContext));

    rv = NS_NewTextFrame(shell, &newFrame);
    if (NS_FAILED(rv)) { return rv; }
    if (!newFrame)   { return NS_ERROR_NULL_POINTER; }
    nsCOMPtr<nsIStyleContext> textStyleContext;
    rv = aPresContext->ResolveStyleContextForNonElement(
                                             styleContext,
                                             getter_AddRefs(textStyleContext));
    if (NS_FAILED(rv))     { return rv; }
    if (!textStyleContext) { return NS_ERROR_NULL_POINTER; }

    if (styleContext) {
      // initialize the text frame
      newFrame->Init(aPresContext, content, parentFrame, textStyleContext, nsnull);
      newFrame->SetInitialChildList(aPresContext, nsnull, nsnull);
      rv = NS_OK;
    }
  }

  if (aFrame) {
    *aFrame = newFrame;
  }
  return rv;
}

#ifdef ACCESSIBILITY
NS_IMETHODIMP nsGfxButtonControlFrame::GetAccessible(nsIAccessible** aAccessible)
{
  nsCOMPtr<nsIAccessibilityService> accService = do_GetService("@mozilla.org/accessibilityService;1");

  if (accService) {
    return accService->CreateHTMLButtonAccessible(NS_STATIC_CAST(nsIFrame*, this), aAccessible);
  }

  return NS_ERROR_FAILURE;
}
#endif

// Frames are not refcounted, no need to AddRef
NS_IMETHODIMP
nsGfxButtonControlFrame::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  NS_ENSURE_ARG_POINTER(aInstancePtr);

  if (aIID.Equals(NS_GET_IID(nsIAnonymousContentCreator))) {
    *aInstancePtr = NS_STATIC_CAST(nsIAnonymousContentCreator*, this);
  }
else {
    return nsHTMLButtonControlFrame::QueryInterface(aIID, aInstancePtr);
  }

  return NS_OK;
}

// Initially we hardcoded the default strings here.
// Next, we used html.css to store the default label for various types
// of buttons. (nsGfxButtonControlFrame::DoNavQuirksReflow rev 1.20)
// However, since html.css is not internationalized, we now grab the default
// label from a string bundle as is done for all other UI strings.
// See bug 16999 for further details.
nsresult
nsGfxButtonControlFrame::GetDefaultLabel(nsString& aString) 
{
  const char * propname = nsFormControlHelper::GetHTMLPropertiesFileName();
  nsresult rv = NS_OK;
  PRInt32 type;
  GetType(&type);
  if (type == NS_FORM_INPUT_RESET) {
    rv = nsFormControlHelper::GetLocalizedString(propname, NS_LITERAL_STRING("Reset").get(), aString);
  } 
  else if (type == NS_FORM_INPUT_SUBMIT) {
    rv = nsFormControlHelper::GetLocalizedString(propname, NS_LITERAL_STRING("Submit").get(), aString);
  } 
  else if (IsFileBrowseButton(type)) {
    rv = nsFormControlHelper::GetLocalizedString(propname, NS_LITERAL_STRING("Browse").get(), aString);
  }
  else {
    aString.Assign(NS_LITERAL_STRING(""));
    rv = NS_OK;
  }
  return rv;
}

NS_IMETHODIMP
nsGfxButtonControlFrame::AttributeChanged(nsIPresContext* aPresContext,
                                       nsIContent*     aChild,
                                       PRInt32         aNameSpaceID,
                                       nsIAtom*        aAttribute,
                                       PRInt32         aModType, 
                                       PRInt32         aHint)
{
  nsresult rv = NS_OK;

  // If the value attribute is set, update the text of the label
  if (nsHTMLAtoms::value == aAttribute) {
    nsAutoString value;
    if (mTextContent && mContent) {
      if (NS_CONTENT_ATTR_HAS_VALUE != mContent->GetAttr(kNameSpaceID_None, nsHTMLAtoms::value, value)) {
        value.Truncate();
      }
      rv = mTextContent->SetText(value.get(), value.Length(), PR_TRUE);
    } else {
      rv = NS_ERROR_UNEXPECTED;
    }

  // defer to HTMLButtonControlFrame
  } else {
    rv = nsHTMLButtonControlFrame::AttributeChanged(aPresContext, aChild, aNameSpaceID, aAttribute, aModType, aHint);
  }
  return rv;
}

NS_IMETHODIMP 
nsGfxButtonControlFrame::Reflow(nsIPresContext*          aPresContext, 
                                nsHTMLReflowMetrics&     aDesiredSize,
                                const nsHTMLReflowState& aReflowState, 
                                nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsGfxButtonControlFrame", aReflowState.reason);
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);

  if ((kSuggestedNotSet != mSuggestedWidth) || 
      (kSuggestedNotSet != mSuggestedHeight)) {
    nsHTMLReflowState suggestedReflowState(aReflowState);

      // Honor the suggested width and/or height.
    if (kSuggestedNotSet != mSuggestedWidth) {
      suggestedReflowState.mComputedWidth = mSuggestedWidth;
    }

    if (kSuggestedNotSet != mSuggestedHeight) {
      suggestedReflowState.mComputedHeight = mSuggestedHeight;
    }

    return nsHTMLButtonControlFrame::Reflow(aPresContext, aDesiredSize, suggestedReflowState, aStatus);

  }
  
  // Normal reflow.

  return nsHTMLButtonControlFrame::Reflow(aPresContext, aDesiredSize, aReflowState, aStatus);
}

NS_IMETHODIMP 
nsGfxButtonControlFrame::SetSuggestedSize(nscoord aWidth, nscoord aHeight)
{
  mSuggestedWidth = aWidth;
  mSuggestedHeight = aHeight;
  //mState |= NS_FRAME_IS_DIRTY;
  return NS_OK;
}

NS_IMETHODIMP
nsGfxButtonControlFrame::HandleEvent(nsIPresContext* aPresContext, 
                                      nsGUIEvent*     aEvent,
                                      nsEventStatus*  aEventStatus)
{
  // temp fix until Bug 124990 gets fixed
  PRBool isPaginated = PR_FALSE;
  aPresContext->IsPaginated(&isPaginated);
  if (isPaginated && NS_IS_MOUSE_EVENT(aEvent)) {
    return NS_OK;
  }
  // Override the HandleEvent to prevent the nsFrame::HandleEvent
  // from being called. The nsFrame::HandleEvent causes the button label
  // to be selected (Drawn with an XOR rectangle over the label)
 
  // Do nothing here, nsHTMLInputElement::HandleDOMEvent
  // takes cares of calling MouseClicked for us.

  // do we have user-input style?
  const nsStyleUserInterface* uiStyle;
  GetStyleData(eStyleStruct_UserInterface,  (const nsStyleStruct *&)uiStyle);
  if (uiStyle->mUserInput == NS_STYLE_USER_INPUT_NONE || uiStyle->mUserInput == NS_STYLE_USER_INPUT_DISABLED)
    return nsFrame::HandleEvent(aPresContext, aEvent, aEventStatus);
  
  return NS_OK;
}
