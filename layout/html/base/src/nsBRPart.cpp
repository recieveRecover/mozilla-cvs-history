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
#include "nsFrame.h"
#include "nsHTMLParts.h"
#include "nsHTMLTagContent.h"
#include "nsHTMLIIDs.h"
#include "nsIPresContext.h"
#include "nsBlockFrame.h"
#include "nsStyleConsts.h"
#include "nsHTMLAtoms.h"
#include "nsIStyleContext.h"
#include "nsIFontMetrics.h"
#include "nsIRenderingContext.h"

class BRFrame : public nsFrame
{
public:
  BRFrame(nsIContent* aContent, nsIFrame* aParentFrame);

  NS_IMETHOD Paint(nsIPresContext& aPresContext,
                   nsIRenderingContext& aRenderingContext,
                   const nsRect& aDirtyRect);
  NS_IMETHOD Reflow(nsIPresContext* aPresContext,
                    nsReflowMetrics& aDesiredSize,
                    const nsReflowState& aReflowState,
                    nsReflowStatus& aStatus);
  NS_IMETHOD GetReflowMetrics(nsIPresContext*  aPresContext,
                              nsReflowMetrics& aMetrics);

protected:
  virtual ~BRFrame();
};

BRFrame::BRFrame(nsIContent* aContent,
                 nsIFrame* aParentFrame)
  : nsFrame(aContent, aParentFrame)
{
}

BRFrame::~BRFrame()
{
}

NS_METHOD BRFrame::Paint(nsIPresContext& aPresContext,
                         nsIRenderingContext& aRenderingContext,
                         const nsRect& aDirtyRect)
{
  if (nsIFrame::GetShowFrameBorders()) {
    nsStyleColor* color = (nsStyleColor*)
      mStyleContext->GetData(eStyleStruct_Color);
    float p2t = aPresContext.GetPixelsToTwips();
    aRenderingContext.SetColor(color->mColor);
    aRenderingContext.FillRect(0, 0, nscoord(5 * p2t), mRect.height);
  }
  return NS_OK;
}

NS_METHOD BRFrame::GetReflowMetrics(nsIPresContext* aPresContext, nsReflowMetrics& aMetrics)
{
  // We have no width, but we're the height of the default font
  nsStyleFont* font =
    (nsStyleFont*)mStyleContext->GetData(eStyleStruct_Font);
  nsIFontMetrics* fm = aPresContext->GetMetricsFor(font->mFont);

  aMetrics.width = 0;
  if (nsIFrame::GetShowFrameBorders()) {
    // Reserve a tiny bit of space so that our frame won't be zero
    // sized so we get a chance to paint.
    aMetrics.width = 1;
  }

  aMetrics.height = fm->GetHeight();
  aMetrics.ascent = fm->GetMaxAscent();
  aMetrics.descent = fm->GetMaxDescent();
  NS_RELEASE(fm);

  // Get cached state for containing block frame
  nsLineLayout* lineLayoutState = nsnull;
  nsBlockReflowState* state =
    nsBlockFrame::FindBlockReflowState(aPresContext, this);
  if (nsnull != state) {
    lineLayoutState = state->mCurrentLine;
    if (nsnull != lineLayoutState) {
      lineLayoutState->mReflowResult =
        NS_LINE_LAYOUT_REFLOW_RESULT_BREAK_AFTER;
    }
  }

  return NS_OK;
}

NS_METHOD BRFrame::Reflow(nsIPresContext* aPresContext,
                          nsReflowMetrics& aDesiredSize,
                          const nsReflowState& aMaxSize,
                          nsReflowStatus& aStatus)
{
  // Get cached state for containing block frame
  nsLineLayout* lineLayoutState = nsnull;
  nsBlockReflowState* state =
    nsBlockFrame::FindBlockReflowState(aPresContext, this);
  if (nsnull != state) {
    lineLayoutState = state->mCurrentLine;
    if (nsnull != lineLayoutState) {
      lineLayoutState->mReflowResult =
        NS_LINE_LAYOUT_REFLOW_RESULT_BREAK_AFTER;
      lineLayoutState->LineBreak();
    }
  }

  GetReflowMetrics(aPresContext, aDesiredSize);
  aStatus = NS_FRAME_COMPLETE;
  return NS_OK;
}

//----------------------------------------------------------------------

class BRPart : public nsHTMLTagContent {
public:
  BRPart(nsIAtom* aTag);

  virtual void SetAttribute(nsIAtom* aAttribute, const nsString& aValue);

  virtual nsContentAttr GetAttribute(nsIAtom* aAttribute,
                                     nsHTMLValue& aResult) const;

  virtual void UnsetAttribute(nsIAtom* aAttribute);


  virtual nsresult CreateFrame(nsIPresContext* aPresContext,
                               nsIFrame* aParentFrame,
                               nsIStyleContext* aStyleContext,
                               nsIFrame*& aResult);

  PRInt32 GetClear() {
    return mClear;
  }

  void SetClear(PRInt32 aValue) {
    mClear = aValue;
  }

protected:
  virtual ~BRPart();
  virtual nsContentAttr AttributeToString(nsIAtom* aAttribute,
                                          nsHTMLValue& aValue,
                                          nsString& aResult) const;

  PRInt32 mClear;
};

BRPart::BRPart(nsIAtom* aTag)
  : nsHTMLTagContent(aTag)
{
}

BRPart::~BRPart()
{
}

nsresult
BRPart::CreateFrame(nsIPresContext*  aPresContext,
                    nsIFrame*        aParentFrame,
                    nsIStyleContext* aStyleContext,
                    nsIFrame*&       aResult)
{
  nsIFrame* frame = new BRFrame(this, aParentFrame);
  if (nsnull == frame) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  frame->SetStyleContext(aPresContext, aStyleContext);
  aResult = frame;
  return NS_OK;
}

//----------------------------------------------------------------------
// Attributes

static nsHTMLTagContent::EnumTable kClearTable[] = {
  { "left", NS_STYLE_CLEAR_LEFT },
  { "right", NS_STYLE_CLEAR_RIGHT },
  { "all", NS_STYLE_CLEAR_LEFT_AND_RIGHT },
  { "both", NS_STYLE_CLEAR_LEFT_AND_RIGHT },
  { 0 }
};

void BRPart::SetAttribute(nsIAtom* aAttribute, const nsString& aString)
{
  if (aAttribute == nsHTMLAtoms::clear) {
    nsHTMLValue value;
    if (ParseEnumValue(aString, kClearTable, value)) {
      mClear = value.GetIntValue();
    }
    else {
      mClear = NS_STYLE_CLEAR_NONE;
    }
    return;
  }
  nsHTMLTagContent::SetAttribute(aAttribute, aString);
}

nsContentAttr BRPart::GetAttribute(nsIAtom* aAttribute,
                                   nsHTMLValue& aResult) const
{
  nsContentAttr ca = eContentAttr_NotThere;
  if (aAttribute == nsHTMLAtoms::clear) {
    aResult.Reset();
    if (NS_STYLE_CLEAR_NONE != mClear) {
      aResult.SetIntValue(mClear, eHTMLUnit_Enumerated);
      ca = eContentAttr_HasValue;
    }
  }
  else {
    ca = nsHTMLTagContent::GetAttribute(aAttribute, aResult);
  }
  return ca;
}

void BRPart::UnsetAttribute(nsIAtom* aAttribute)
{
  if (aAttribute == nsHTMLAtoms::clear) {
    mClear = NS_STYLE_CLEAR_NONE;
  }
}

nsContentAttr BRPart::AttributeToString(nsIAtom* aAttribute,
                                        nsHTMLValue& aValue,
                                        nsString& aResult) const
{
  if (aAttribute == nsHTMLAtoms::clear) {
    if ((eHTMLUnit_Enumerated == aValue.GetUnit()) &&
        (NS_STYLE_CLEAR_NONE != aValue.GetIntValue())) {
      EnumValueToString(aValue, kClearTable, aResult);
      return eContentAttr_HasValue;
    }
  }
  return eContentAttr_NotThere;
}

//----------------------------------------------------------------------

nsresult
NS_NewHTMLBreak(nsIHTMLContent** aInstancePtrResult, nsIAtom* aTag)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null ptr");
  if (nsnull == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }
  nsIHTMLContent* it = new BRPart(aTag);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return it->QueryInterface(kIHTMLContentIID, (void**) aInstancePtrResult);
}
