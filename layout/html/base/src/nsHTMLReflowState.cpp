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
#include "nsCOMPtr.h"
#include "nsIStyleContext.h"
#include "nsStyleConsts.h"
#include "nsFrame.h"
#include "nsIContent.h"
#include "nsHTMLAtoms.h"
#include "nsIPresContext.h"
#include "nsIPresShell.h"
#include "nsLayoutAtoms.h"
#include "nsIRenderingContext.h"
#include "nsIFontMetrics.h"
#include "nsBlockFrame.h"
#include "nsLineBox.h"
#include "nsImageFrame.h"

#ifdef NS_DEBUG
#undef NOISY_VERTICAL_ALIGN
#else
#undef NOISY_VERTICAL_ALIGN
#endif

// Initialize a <b>root</b> reflow state with a rendering context to
// use for measuring things.
nsHTMLReflowState::nsHTMLReflowState(nsIPresContext*      aPresContext,
                                     nsIFrame*            aFrame,
                                     nsReflowReason       aReason,
                                     nsIRenderingContext* aRenderingContext,
                                     const nsSize&        aAvailableSpace)
  : mReflowDepth(0)
{
  NS_PRECONDITION(nsnull != aRenderingContext, "no rendering context");

  parentReflowState = nsnull;
  frame = aFrame;
  reason = aReason;
  reflowCommand = nsnull;
  availableWidth = aAvailableSpace.width;
  availableHeight = aAvailableSpace.height;
  rendContext = aRenderingContext;
  mSpaceManager = nsnull;
  mLineLayout = nsnull;
  isTopOfPage = PR_FALSE;
  Init(aPresContext);
}

// Initialize a <b>root</b> reflow state for an <b>incremental</b>
// reflow.
nsHTMLReflowState::nsHTMLReflowState(nsIPresContext*      aPresContext,
                                     nsIFrame*            aFrame,
                                     nsIReflowCommand&    aReflowCommand,
                                     nsIRenderingContext* aRenderingContext,
                                     const nsSize&        aAvailableSpace)
  : mReflowDepth(0)
{
  NS_PRECONDITION(nsnull != aRenderingContext, "no rendering context");  

  reason = eReflowReason_Incremental;
  parentReflowState = nsnull;
  frame = aFrame;
  reflowCommand = &aReflowCommand;
  availableWidth = aAvailableSpace.width;
  availableHeight = aAvailableSpace.height;
  rendContext = aRenderingContext;
  mSpaceManager = nsnull;
  mLineLayout = nsnull;
  isTopOfPage = PR_FALSE;
  Init(aPresContext);
}

// Initialize a reflow state for a child frames reflow. Some state
// is copied from the parent reflow state; the remaining state is
// computed.
nsHTMLReflowState::nsHTMLReflowState(nsIPresContext*          aPresContext,
                                     const nsHTMLReflowState& aParentReflowState,
                                     nsIFrame*                aFrame,
                                     const nsSize&            aAvailableSpace,
                                     nsReflowReason           aReason)
  : mReflowDepth(aParentReflowState.mReflowDepth + 1)
{
  parentReflowState = &aParentReflowState;
  frame = aFrame;
  reason = aReason;
  reflowCommand = (reason == eReflowReason_Incremental)
    ? aParentReflowState.reflowCommand
    : nsnull;
  availableWidth = aAvailableSpace.width;
  availableHeight = aAvailableSpace.height;

  rendContext = aParentReflowState.rendContext;
  mSpaceManager = aParentReflowState.mSpaceManager;
  mLineLayout = aParentReflowState.mLineLayout;
  isTopOfPage = aParentReflowState.isTopOfPage;

  Init(aPresContext);
}

// Same as the previous except that the reason is taken from the
// parent's reflow state.
nsHTMLReflowState::nsHTMLReflowState(nsIPresContext*          aPresContext,
                                     const nsHTMLReflowState& aParentReflowState,
                                     nsIFrame*                aFrame,
                                     const nsSize&            aAvailableSpace)
  : mReflowDepth(aParentReflowState.mReflowDepth + 1)
{
  parentReflowState = &aParentReflowState;
  frame = aFrame;
  reason = aParentReflowState.reason;
  reflowCommand = aParentReflowState.reflowCommand;
  availableWidth = aAvailableSpace.width;
  availableHeight = aAvailableSpace.height;

  rendContext = aParentReflowState.rendContext;
  mSpaceManager = aParentReflowState.mSpaceManager;
  mLineLayout = aParentReflowState.mLineLayout;
  isTopOfPage = aParentReflowState.isTopOfPage;

  Init(aPresContext);
}

// Version that species the containing block width and height
nsHTMLReflowState::nsHTMLReflowState(nsIPresContext*          aPresContext,
                                     const nsHTMLReflowState& aParentReflowState,
                                     nsIFrame*                aFrame,
                                     const nsSize&            aAvailableSpace,
                                     nscoord                  aContainingBlockWidth,
                                     nscoord                  aContainingBlockHeight)
  : mReflowDepth(aParentReflowState.mReflowDepth + 1)
{
  parentReflowState = &aParentReflowState;
  frame = aFrame;
  reason = aParentReflowState.reason;
  reflowCommand = aParentReflowState.reflowCommand;
  availableWidth = aAvailableSpace.width;
  availableHeight = aAvailableSpace.height;

  rendContext = aParentReflowState.rendContext;
  mSpaceManager = aParentReflowState.mSpaceManager;
  mLineLayout = aParentReflowState.mLineLayout;
  isTopOfPage = aParentReflowState.isTopOfPage;

  Init(aPresContext, aContainingBlockWidth, aContainingBlockHeight);
}

void
nsHTMLReflowState::Init(nsIPresContext* aPresContext,
                        nscoord         aContainingBlockWidth,
                        nscoord         aContainingBlockHeight)
{
  mCompactMarginWidth = 0;
  mAlignCharOffset = 0;
  mUseAlignCharOffset = 0;
  mIsAutoWidthPctBase = PR_FALSE;

  frame->GetStyleData(eStyleStruct_Position,
                      (const nsStyleStruct*&)mStylePosition);
  frame->GetStyleData(eStyleStruct_Display,
                      (const nsStyleStruct*&)mStyleDisplay);
  frame->GetStyleData(eStyleStruct_Spacing,
                      (const nsStyleStruct*&)mStyleSpacing);
  mFrameType = DetermineFrameType(frame, mStylePosition, mStyleDisplay);
  InitConstraints(aPresContext, aContainingBlockWidth, aContainingBlockHeight);
}

const nsHTMLReflowState*
nsHTMLReflowState::GetContainingBlockReflowState(const nsHTMLReflowState* aParentRS)
{
  while (nsnull != aParentRS) {
    if (nsnull != aParentRS->frame) {
      PRBool isContainingBlock;
      // XXX This needs to go and we need to start using the info in the
      // reflow state...
      nsresult rv = aParentRS->frame->IsPercentageBase(isContainingBlock);
      if (NS_SUCCEEDED(rv) && isContainingBlock) {
        // a block inside a table cell needs to use the table cell
        if (aParentRS->parentReflowState) {
          nsCOMPtr<nsIAtom> fType;
          aParentRS->parentReflowState->frame->GetFrameType(getter_AddRefs(fType));
          if (nsLayoutAtoms::tableCellFrame == fType.get()) {
            aParentRS = aParentRS->parentReflowState;
          }
        }
        return aParentRS;
      }
    }
    aParentRS = aParentRS->parentReflowState;
  }
  return nsnull;
}

const nsHTMLReflowState*
nsHTMLReflowState::GetPageBoxReflowState(const nsHTMLReflowState* aParentRS)
{
  // XXX write me as soon as we can ask a frame if it's a page frame...
  return nsnull;
}

nscoord
nsHTMLReflowState::GetContainingBlockContentWidth(const nsHTMLReflowState* aParentRS)
{
  nscoord width = 0;
  const nsHTMLReflowState* rs =
    GetContainingBlockReflowState(aParentRS);
  if (nsnull != rs) {
    return aParentRS->mComputedWidth;
  }
  return width;
}

nsCSSFrameType
nsHTMLReflowState::DetermineFrameType(nsIFrame* aFrame)
{
  const nsStylePosition* stylePosition;
  aFrame->GetStyleData(eStyleStruct_Position,
                       (const nsStyleStruct*&)stylePosition);
  const nsStyleDisplay* styleDisplay;
  aFrame->GetStyleData(eStyleStruct_Display,
                       (const nsStyleStruct*&)styleDisplay);
  return DetermineFrameType(aFrame, stylePosition, styleDisplay);
}

nsCSSFrameType
nsHTMLReflowState::DetermineFrameType(nsIFrame* aFrame,
                                      const nsStylePosition* aPosition,
                                      const nsStyleDisplay* aDisplay)
{
  nsCSSFrameType frameType;

  // Get the frame state
  nsFrameState  frameState;
  aFrame->GetFrameState(&frameState);
  
  // Section 9.7 of the CSS2 spec indicates that absolute position
  // takes precedence over float which takes precedence over display.
  // Make sure the frame was actually moved out of the flow, and don't
  // just assume what the style says
  if (frameState & NS_FRAME_OUT_OF_FLOW) {
    if (aPosition->IsAbsolutelyPositioned()) {
      frameType = NS_CSS_FRAME_TYPE_ABSOLUTE;
    }
    else if (NS_STYLE_FLOAT_NONE != aDisplay->mFloats) {
      frameType = NS_CSS_FRAME_TYPE_FLOATING;
    }
  }
  else {
    switch (aDisplay->mDisplay) {
    case NS_STYLE_DISPLAY_BLOCK:
    case NS_STYLE_DISPLAY_LIST_ITEM:
    case NS_STYLE_DISPLAY_TABLE:
    case NS_STYLE_DISPLAY_TABLE_CAPTION:
      frameType = NS_CSS_FRAME_TYPE_BLOCK;
      break;

    case NS_STYLE_DISPLAY_INLINE:
    case NS_STYLE_DISPLAY_MARKER:
    case NS_STYLE_DISPLAY_INLINE_TABLE:
      frameType = NS_CSS_FRAME_TYPE_INLINE;
      break;

    case NS_STYLE_DISPLAY_RUN_IN:
    case NS_STYLE_DISPLAY_COMPACT:
      // XXX need to look ahead at the frame's sibling
      frameType = NS_CSS_FRAME_TYPE_BLOCK;
      break;

    case NS_STYLE_DISPLAY_TABLE_CELL:
    case NS_STYLE_DISPLAY_TABLE_ROW_GROUP:
    case NS_STYLE_DISPLAY_TABLE_COLUMN:
    case NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP:
    case NS_STYLE_DISPLAY_TABLE_HEADER_GROUP:
    case NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP:
    case NS_STYLE_DISPLAY_TABLE_ROW:
      frameType = NS_CSS_FRAME_TYPE_INTERNAL_TABLE;
      break;

    case NS_STYLE_DISPLAY_NONE:
    default:
      frameType = NS_CSS_FRAME_TYPE_UNKNOWN;
      break;
    }
  }

  // See if the frame is replaced
  if (frameState & NS_FRAME_REPLACED_ELEMENT) {
    frameType = NS_FRAME_REPLACED(frameType);
  }

  return frameType;
}

void
nsHTMLReflowState::ComputeRelativeOffsets(const nsHTMLReflowState* cbrs,
                                          nscoord aContainingBlockWidth,
                                          nscoord aContainingBlockHeight)
{
  nsStyleCoord  coord;

  // Compute the 'left' and 'right' values. 'Left' moves the boxes to the right,
  // and 'right' moves the boxes to the left. The computed values are always:
  // left=-right
  PRBool  leftIsAuto = eStyleUnit_Auto == mStylePosition->mOffset.GetLeftUnit();
  PRBool  rightIsAuto = eStyleUnit_Auto == mStylePosition->mOffset.GetRightUnit();

  // Check for percentage based values and an unconstrained containing
  // block width. Treat them like 'auto'
  if (NS_UNCONSTRAINEDSIZE == aContainingBlockWidth) {
    if (eStyleUnit_Percent == mStylePosition->mOffset.GetLeftUnit()) {
      leftIsAuto = PR_TRUE;
    }
    if (eStyleUnit_Percent == mStylePosition->mOffset.GetRightUnit()) {
      rightIsAuto = PR_TRUE;
    }
  }

  // If neither 'left' not 'right' are auto, then we're over-constrained and
  // we ignore one of them
  if (!leftIsAuto && !rightIsAuto) {
    const nsStyleDisplay* display;
    frame->GetStyleData(eStyleStruct_Display, (const nsStyleStruct*&)display);
    
    if (NS_STYLE_DIRECTION_LTR == display->mDirection) {
      rightIsAuto = PR_TRUE;
    } else {
      leftIsAuto = PR_TRUE;
    }
  }

  if (leftIsAuto) {
    if (rightIsAuto) {
      // If both are 'auto' (their initial values), the computed values are 0
      mComputedOffsets.left = mComputedOffsets.right = 0;
    } else {
      // 'Right' isn't 'auto' so compute its value
      if (eStyleUnit_Inherit == mStylePosition->mOffset.GetRightUnit()) {
        mComputedOffsets.right = cbrs->mComputedOffsets.right;
      } else {
        ComputeHorizontalValue(aContainingBlockWidth, mStylePosition->mOffset.GetRightUnit(),
                               mStylePosition->mOffset.GetRight(coord),
                               mComputedOffsets.right);
      }
      
      // Computed value for 'left' is minus the value of 'right'
      mComputedOffsets.left = -mComputedOffsets.right;
    }

  } else {
    NS_ASSERTION(rightIsAuto, "unexpected specified constraint");
    
    // 'Left' isn't 'auto' so compute its value
    if (eStyleUnit_Inherit == mStylePosition->mOffset.GetLeftUnit()) {
      mComputedOffsets.left = cbrs->mComputedOffsets.left;
    } else {
      ComputeHorizontalValue(aContainingBlockWidth, mStylePosition->mOffset.GetLeftUnit(),
                             mStylePosition->mOffset.GetLeft(coord),
                             mComputedOffsets.left);
    }

    // Computed value for 'right' is minus the value of 'left'
    mComputedOffsets.right = -mComputedOffsets.left;
  }

  // Compute the 'top' and 'bottom' values. The 'top' and 'bottom' properties
  // move relatively positioned elements up and down. They also must be each 
  // other's negative
  PRBool  topIsAuto = eStyleUnit_Auto == mStylePosition->mOffset.GetTopUnit();
  PRBool  bottomIsAuto = eStyleUnit_Auto == mStylePosition->mOffset.GetBottomUnit();

  // Check for percentage based values and a containing block height that
  // depends on the content height. Treat them like 'auto'
  if (NS_AUTOHEIGHT == aContainingBlockHeight) {
    if (eStyleUnit_Percent == mStylePosition->mOffset.GetTopUnit()) {
      topIsAuto = PR_TRUE;
    }
    if (eStyleUnit_Percent == mStylePosition->mOffset.GetBottomUnit()) {
      bottomIsAuto = PR_TRUE;
    }
  }

  // If neither is 'auto', 'bottom' is ignored
  if (!topIsAuto && !bottomIsAuto) {
    bottomIsAuto = PR_TRUE;
  }

  if (topIsAuto) {
    if (bottomIsAuto) {
      // If both are 'auto' (their initial values), the computed values are 0
      mComputedOffsets.top = mComputedOffsets.bottom = 0;
    } else {
      // 'Bottom' isn't 'auto' so compute its value
      if (eStyleUnit_Inherit == mStylePosition->mOffset.GetBottomUnit()) {
        mComputedOffsets.bottom = cbrs->mComputedOffsets.bottom;
      } else {
        ComputeVerticalValue(aContainingBlockHeight, mStylePosition->mOffset.GetBottomUnit(),
                               mStylePosition->mOffset.GetBottom(coord),
                               mComputedOffsets.bottom);
      }
      
      // Computed value for 'top' is minus the value of 'bottom'
      mComputedOffsets.top = -mComputedOffsets.bottom;
    }

  } else {
    NS_ASSERTION(bottomIsAuto, "unexpected specified constraint");
    
    // 'Top' isn't 'auto' so compute its value
    if (eStyleUnit_Inherit == mStylePosition->mOffset.GetTopUnit()) {
      mComputedOffsets.top = cbrs->mComputedOffsets.top;
    } else {
      ComputeVerticalValue(aContainingBlockHeight, mStylePosition->mOffset.GetTopUnit(),
                             mStylePosition->mOffset.GetTop(coord),
                             mComputedOffsets.top);
    }

    // Computed value for 'bottom' is minus the value of 'top'
    mComputedOffsets.bottom = -mComputedOffsets.top;
  }
}

// Returns the nearest containing block frame for the specified frame.
// Also returns the left, top, right, and bottom edges of the specified
// frame's content area. These are in the coordinate space of the block
// frame itself
static nsIFrame*
GetNearestContainingBlock(nsIFrame* aFrame, nsMargin& aContentArea)
{
  aFrame->GetParent(&aFrame);
  while (aFrame) {
    nsIAtom*  frameType;
    PRBool    isBlock;

    aFrame->GetFrameType(&frameType);
    isBlock = (frameType == nsLayoutAtoms::blockFrame) ||
              (frameType == nsLayoutAtoms::areaFrame);
    NS_IF_RELEASE(frameType);

    if (isBlock) {
      break;
    }
    aFrame->GetParent(&aFrame);
  }

  if (aFrame) {
    nsSize  size;
  
    aFrame->GetSize(size);
    aContentArea.left = 0;
    aContentArea.top = 0;
    aContentArea.right = size.width;
    aContentArea.bottom = size.height;
  
    // Subtract off for border and padding. If it can't be computed because
    // it's percentage based (for example) then just ignore it
    nsStyleSpacing*  spacing;
    nsMargin         borderPadding;
    aFrame->GetStyleData(eStyleStruct_Spacing, (const nsStyleStruct*&)spacing);
    if (spacing->GetBorderPadding(borderPadding)) {
      aContentArea.left += borderPadding.left;
      aContentArea.top += borderPadding.top;
      aContentArea.right -= borderPadding.right;
      aContentArea.bottom -= borderPadding.bottom;
    }
  }

  return aFrame;
}

// When determining the hypothetical box that would have been if the element
// had been in the flow we may not be able to exactly determine both the left
// and right edges. For example, if the element is a non-replaced inline-level
// element we would have to reflow it in order to determine it desired width.
// In that case depending on the progression direction either the left or
// right edge would be marked as not being exact
struct nsHypotheticalBox {
  nscoord       mLeft, mRight;
  nscoord       mTop;
  PRPackedBool  mLeftIsExact, mRightIsExact;

  nsHypotheticalBox() {
    mLeftIsExact = mRightIsExact = PR_FALSE;
  }
};
      
static PRBool
GetIntrinsicSizeFor(nsIFrame* aFrame, nsSize& aIntrinsicSize)
{
  // See if it is an image frame
  nsIAtom*  frameType;
  PRBool    result = PR_FALSE;

  // Currently the only type of replaced frame that we can get the intrinsic
  // size for is an image frame
  // XXX We should add back the GetReflowMetrics() function and one of the
  // things should be the intrinsic size...
  aFrame->GetFrameType(&frameType);
  if (frameType == nsLayoutAtoms::imageFrame) {
    nsImageFrame* imageFrame = (nsImageFrame*)aFrame;

    imageFrame->GetIntrinsicImageSize(aIntrinsicSize);
    result = (aIntrinsicSize != nsSize(0, 0));
  }
  
  NS_IF_RELEASE(frameType);
  return result;
}

nscoord
nsHTMLReflowState::CalculateHorizBorderPaddingMargin(nscoord aContainingBlockWidth)
{
  nsMargin  border, padding, margin;

  // Get the border
  if (!mStyleSpacing->GetBorder(border)) {
    // CSS2 has no percentage borders
    border.SizeTo(0, 0, 0, 0);
  }

  // See if the style system can provide us the padding directly
  if (!mStyleSpacing->GetPadding(padding)) {
    nsStyleCoord left, right;

    // We have to compute the left and right values
    if (eStyleUnit_Inherit == mStyleSpacing->mPadding.GetLeftUnit()) {
      padding.left = 0;  // just ignore
    } else {
      ComputeHorizontalValue(aContainingBlockWidth,
                             mStyleSpacing->mPadding.GetLeftUnit(),
                             mStyleSpacing->mPadding.GetLeft(left),
                             padding.left);
    }
    if (eStyleUnit_Inherit == mStyleSpacing->mPadding.GetRightUnit()) {
      padding.right = 0;  // just ignore
    } else {
      ComputeHorizontalValue(aContainingBlockWidth,
                             mStyleSpacing->mPadding.GetRightUnit(),
                             mStyleSpacing->mPadding.GetRight(right),
                             padding.right);
    }
  }

  // See if the style system can provide us the margin directly
  if (!mStyleSpacing->GetMargin(margin)) {
    nsStyleCoord left, right;

    // We have to compute the left and right values
    if ((eStyleUnit_Auto == mStyleSpacing->mMargin.GetLeftUnit()) ||
        (eStyleUnit_Inherit == mStyleSpacing->mMargin.GetLeftUnit())) {
      margin.left = 0;  // just ignore
    } else {
      ComputeHorizontalValue(aContainingBlockWidth,
                             mStyleSpacing->mMargin.GetLeftUnit(),
                             mStyleSpacing->mMargin.GetLeft(left),
                             margin.left);
    }
    if ((eStyleUnit_Auto == mStyleSpacing->mMargin.GetRightUnit()) ||
        (eStyleUnit_Inherit == mStyleSpacing->mMargin.GetRightUnit())) {
      margin.right = 0;  // just ignore
    } else {
      ComputeHorizontalValue(aContainingBlockWidth,
                             mStyleSpacing->mMargin.GetRightUnit(),
                             mStyleSpacing->mMargin.GetRight(right),
                             margin.right);
    }
  }

  return padding.left + padding.right + border.left + border.right +
         margin.left + margin.right;
}

static void
GetPlaceholderOffset(nsIFrame* aPlaceholderFrame,
                     nsIFrame* aBlockFrame,
                     nsPoint&  aOffset)
{
  aPlaceholderFrame->GetOrigin(aOffset);

  // Convert the placeholder position to the coordinate space of the block
  // frame that contains it
  nsIFrame* parent;
  aPlaceholderFrame->GetParent(&parent);
  while (parent && (parent != aBlockFrame)) {
    nsPoint origin;

    parent->GetOrigin(origin);
    aOffset += origin;
    parent->GetParent(&parent);
  }
}

static nsIFrame*
FindImmediateChildOf(nsIFrame* aParent, nsIFrame* aDescendantFrame)
{
  nsIFrame* result = aDescendantFrame;

  while (result) {
    nsIFrame* parent;
    
    result->GetParent(&parent);
    if (parent == aParent) {
      break;
    }

    // The frame is not an immediate child of aParent so walk up another level
    result = parent;
  }

  return result;
}

// Calculate the hypothetical box that the element would have if it were in
// the flow. The values returned are relative to the padding edge of the
// absolute containing block
void
nsHTMLReflowState::CalculateHypotheticalBox(nsIPresContext*    aPresContext,
                                            nsIFrame*          aPlaceholderFrame,
                                            nsIFrame*          aBlockFrame,
                                            nsMargin&          aBlockContentArea,
                                            nsIFrame*          aAbsoluteContainingBlockFrame,
                                            nsHypotheticalBox& aHypotheticalBox)
{
  // If it's a replaced element and it has a 'auto' value for 'width', see if we
  // can get the intrinsic size. This will allow us to exactly determine both the
  // left and right edges
  nsStyleUnit widthUnit = mStylePosition->mWidth.GetUnit();
  nsSize      intrinsicSize;
  PRBool      knowIntrinsicSize = PR_FALSE;
  if (NS_FRAME_IS_REPLACED(mFrameType) && (eStyleUnit_Auto == widthUnit)) {
    // See if we can get the intrinsic size of the element
    knowIntrinsicSize = GetIntrinsicSizeFor(frame, intrinsicSize);
  }

  // See if we can calculate what the box width would have been if the
  // element had been in the flow
  nscoord boxWidth;
  PRBool  knowBoxWidth = PR_FALSE;
  if ((NS_STYLE_DISPLAY_INLINE == mStyleDisplay->mDisplay) &&
      !NS_FRAME_IS_REPLACED(mFrameType)) {
    // For non-replaced inline-level elements the 'width' property doesn't apply,
    // so we don't know what the width would have been without reflowing it

  } else {
    // It's either a replaced inline-level element or a block-level element
    nscoord horizBorderPaddingMargin;

    // Determine the total amount of horizontal border/padding/margin that
    // the element would have had if it had been in the flow. Note that we
    // ignore any 'auto' and 'inherit' values
    horizBorderPaddingMargin = CalculateHorizBorderPaddingMargin(aBlockContentArea.right -
                                                                 aBlockContentArea.left);

    if (NS_FRAME_IS_REPLACED(mFrameType) && (eStyleUnit_Auto == widthUnit)) {
      // It's a replaced element with an 'auto' width so the box width is
      // its intrinsic size plus any border/padding/margin
      if (knowIntrinsicSize) {
        boxWidth = intrinsicSize.width + horizBorderPaddingMargin;
        knowBoxWidth = PR_TRUE;
      }

    } else if ((eStyleUnit_Inherit == widthUnit) || (eStyleUnit_Auto == widthUnit)) {
      // The box width is the containing block width
      boxWidth = aBlockContentArea.right - aBlockContentArea.left;
      knowBoxWidth = PR_TRUE;
    
    } else {
      // We need to compute it. It's important we do this, because if it's
      // percentage based this computed value may be different from the comnputed
      // value calculated using the absolute containing block width
      ComputeHorizontalValue(aBlockContentArea.right - aBlockContentArea.left,
                             widthUnit, mStylePosition->mWidth, boxWidth);
      boxWidth += horizBorderPaddingMargin;
      knowBoxWidth = PR_TRUE;
    }
  }
  
  // Get the 'direction' of the block
  const nsStyleDisplay* blockDisplay;
  aBlockFrame->GetStyleData(eStyleStruct_Display, (const nsStyleStruct*&)blockDisplay);

  // How we determine the hypothetical box depends on whether the element
  // would have been inline-level or block-level
  if (NS_STYLE_DISPLAY_INLINE == mStyleDisplay->mDisplay) {
    nsPoint placeholderOffset;

    // Get the placeholder x-offset and y-offset in the coordinate
    // space of the block frame that contains it
    GetPlaceholderOffset(aPlaceholderFrame, aBlockFrame, placeholderOffset);

    // The y-offset is the baseline of where the text would be if it were
    // in the flow. We need the top position and not the baseline position
    nsIFontMetrics*     metrics;
    const nsStyleFont*  font;

    frame->GetStyleData(eStyleStruct_Font, (const nsStyleStruct*&)font);
    aPresContext->GetMetricsFor(font->mFont, &metrics);
    if (metrics) {
      nscoord   ascent;

      // Adjust the y-offset up by the font ascent. That will translate from
      // the baseline to the top of where the text would be
      metrics->GetMaxAscent(ascent);
      placeholderOffset.y -= ascent;
      NS_RELEASE(metrics);
    }
    aHypotheticalBox.mTop = placeholderOffset.y;

    // To determine the left and right offsets we need to look at the block's 'direction'
    if (NS_STYLE_DIRECTION_LTR == blockDisplay->mDirection) {
      // The placeholder represents the left edge of the hypothetical box
      aHypotheticalBox.mLeft = placeholderOffset.x;
      aHypotheticalBox.mLeftIsExact = PR_TRUE;

      if (knowBoxWidth) {
        aHypotheticalBox.mRight = aHypotheticalBox.mLeft + boxWidth;
        aHypotheticalBox.mRightIsExact = PR_TRUE;
      } else {
        // We can't compute the right edge because we don't know the desired
        // width. So instead use the right content edge of the block parent,
        // but remember it's not exact
        aHypotheticalBox.mRight = aBlockContentArea.right;
        aHypotheticalBox.mRightIsExact = PR_FALSE;
      }
      
    } else {
      // The placeholder represents the right edge of the hypothetical box
      aHypotheticalBox.mRight = placeholderOffset.x;
      aHypotheticalBox.mRightIsExact = PR_TRUE;
      
      if (knowBoxWidth) {
        aHypotheticalBox.mLeft = aHypotheticalBox.mRight - boxWidth;
        aHypotheticalBox.mLeftIsExact = PR_TRUE;
      } else {
        // We can't compute the left edge because we don't know the desired
        // width. So instead use the left content edge of the block parent,
        // but remember it's not exact
        aHypotheticalBox.mLeft = aBlockContentArea.left;
        aHypotheticalBox.mLeftIsExact = PR_FALSE;
      }
    }

  } else {
    // The element would have been block-level which means it would be below
    // the line containing the placeholder frame
    if (aBlockFrame) {
      nsIFrame*   blockChild;
      nsLineBox*  lineBox;
      nsLineBox*  prevLineBox;
      PRBool      isFloater;

      // We need the immediate child of the block frame, and that may not be
      // the placeholder frame
      blockChild = FindImmediateChildOf(aBlockFrame, aPlaceholderFrame);
      lineBox = ((nsBlockFrame*)aBlockFrame)->FindLineFor(blockChild, &prevLineBox,
                                                          &isFloater);
      if (lineBox) {
        // The top of the hypothetical box is just below the line containing
        // the placeholder
        aHypotheticalBox.mTop = lineBox->mBounds.YMost();
      } else {
        nsPoint placeholderOffset;
        
        // Just use the placeholder's y-offset
        GetPlaceholderOffset(aPlaceholderFrame, aBlockFrame, placeholderOffset);
        aHypotheticalBox.mTop = placeholderOffset.y;
      }
    }

    // To determine the left and right offsets we need to look at the block's 'direction'
    if (NS_STYLE_DIRECTION_LTR == blockDisplay->mDirection) {
      aHypotheticalBox.mLeft = aBlockContentArea.left;
      aHypotheticalBox.mLeftIsExact = PR_TRUE;

      // If we know the box width then we can determine the right edge
      if (knowBoxWidth) {
        aHypotheticalBox.mRight = aHypotheticalBox.mLeft + boxWidth;
        aHypotheticalBox.mRightIsExact = PR_TRUE;
      } else {
        // We can't compute the right edge because we don't know the intrinsic
        // width yet. So instead use the right content edge of the block parent,
        // but remember it's not exact
        aHypotheticalBox.mRight = aBlockContentArea.right;
        aHypotheticalBox.mRightIsExact = PR_FALSE;
      }

    } else {
      aHypotheticalBox.mRight = aBlockContentArea.right;
      aHypotheticalBox.mRightIsExact = PR_TRUE;

      // If we know the box width then we can determine the left edge
      if (knowBoxWidth) {
        aHypotheticalBox.mLeft = aHypotheticalBox.mRight - boxWidth;
        aHypotheticalBox.mLeftIsExact = PR_TRUE;
      } else {
        // We can't compute the left edge because we don't know the intrinsic
        // width yet. So instead use the left content edge of the block parent,
        // but remember it's not exact
        aHypotheticalBox.mLeft = aBlockContentArea.left;
        aHypotheticalBox.mLeftIsExact = PR_FALSE;
      }
    }
  }

  // The current coordinate space is that of the nearest block to the placeholder.
  // Convert to the coordinate space of the absolute containing block
  if (aBlockFrame != aAbsoluteContainingBlockFrame) {
    nsIFrame* parent = aBlockFrame;
    do {
      nsPoint origin;

      parent->GetOrigin(origin);
      aHypotheticalBox.mLeft += origin.x;
      aHypotheticalBox.mRight += origin.x;
      aHypotheticalBox.mTop += origin.y;

      // Move up the tree one level
      parent->GetParent(&parent);
    } while (parent && (parent != aAbsoluteContainingBlockFrame));
  }

  // The specified offsets are relative to the absolute containing block's padding
  // edge, and our current values are relative to the border edge so translate
  nsMargin              border;
  const nsStyleSpacing* spacing;

  aAbsoluteContainingBlockFrame->GetStyleData(eStyleStruct_Spacing, (const nsStyleStruct*&)spacing);
  if (!spacing->GetBorder(border)) {
    NS_NOTYETIMPLEMENTED("percentage border");
  }
  aHypotheticalBox.mLeft -= border.left;
  aHypotheticalBox.mRight -= border.left;
  aHypotheticalBox.mTop -= border.top;
}

void
nsHTMLReflowState::InitAbsoluteConstraints(nsIPresContext* aPresContext,
                                           const nsHTMLReflowState* cbrs,
                                           nscoord containingBlockWidth,
                                           nscoord containingBlockHeight)
{
  NS_PRECONDITION(containingBlockHeight != NS_AUTOHEIGHT,
                  "containing block height must be constrained");

  // Get the placeholder frame
  nsIFrame*     placeholderFrame;
  nsCOMPtr<nsIPresShell> presShell;
  aPresContext->GetShell(getter_AddRefs(presShell));

  presShell->GetPlaceholderFrameFor(frame, &placeholderFrame);
  NS_ASSERTION(nsnull != placeholderFrame, "no placeholder frame");

  // Find the nearest containing block frame to the placeholder frame,
  // and return its content area left, top, right, and bottom edges
  nsMargin  blockContentArea;
  nsIFrame* blockFrame = GetNearestContainingBlock(placeholderFrame,
                                                   blockContentArea);
  
  // If both 'left' and 'right' are 'auto' or both 'top' and 'bottom' are
  // 'auto', then compute the hypothetical box of where the element would
  // have been if it had been in the flow
  nsHypotheticalBox hypotheticalBox;
  if (((eStyleUnit_Auto == mStylePosition->mOffset.GetLeftUnit()) &&
       (eStyleUnit_Auto == mStylePosition->mOffset.GetRightUnit())) ||
      ((eStyleUnit_Auto == mStylePosition->mOffset.GetTopUnit()) &&
       (eStyleUnit_Auto == mStylePosition->mOffset.GetBottomUnit()))) {

    CalculateHypotheticalBox(aPresContext, placeholderFrame, blockFrame,
                             blockContentArea, cbrs->frame, hypotheticalBox);
  }

  // Initialize the 'left' and 'right' computed offsets
  // XXX Handle new 'static-position' value...
  PRBool        leftIsAuto = PR_FALSE, rightIsAuto = PR_FALSE;
  nsStyleCoord  coord;
  if (eStyleUnit_Inherit == mStylePosition->mOffset.GetLeftUnit()) {
    mComputedOffsets.left = cbrs->mComputedOffsets.left;
  } else if (eStyleUnit_Auto == mStylePosition->mOffset.GetLeftUnit()) {
    mComputedOffsets.left = 0;
    leftIsAuto = PR_TRUE;
  } else {
    ComputeHorizontalValue(containingBlockWidth, mStylePosition->mOffset.GetLeftUnit(),
                           mStylePosition->mOffset.GetLeft(coord),
                           mComputedOffsets.left);
  }
  if (eStyleUnit_Inherit == mStylePosition->mOffset.GetRightUnit()) {
    mComputedOffsets.right = cbrs->mComputedOffsets.right;
  } else if (eStyleUnit_Auto == mStylePosition->mOffset.GetRightUnit()) {
    mComputedOffsets.right = 0;
    rightIsAuto = PR_TRUE;
  } else {
    ComputeHorizontalValue(containingBlockWidth, mStylePosition->mOffset.GetRightUnit(),
                           mStylePosition->mOffset.GetRight(coord),
                           mComputedOffsets.right);
  }

  // When the CSS2 spec refers to direction it means the containing block's
  // direction and not the direction of the absolutely positioned element itself
  PRUint8 direction = cbrs->mStyleDisplay->mDirection;

  // Initialize the 'width' computed value
  nsStyleUnit widthUnit = mStylePosition->mWidth.GetUnit();
  PRBool      widthIsAuto = (eStyleUnit_Auto == widthUnit);
  if (!widthIsAuto) {
    if (eStyleUnit_Inherit == widthUnit) {
      // The inherited value comes from the parent and not the absolute
      // containing block
      mComputedWidth = blockContentArea.right - blockContentArea.left;

    } else {
      // Use the specified value for the computed width
      ComputeHorizontalValue(containingBlockWidth, widthUnit,
                             mStylePosition->mWidth, mComputedWidth);
    }
    
    // Factor in any minimum and maximum size information
    if (mComputedWidth > mComputedMaxWidth) {
      mComputedWidth = mComputedMaxWidth;
    } else if (mComputedWidth < mComputedMinWidth) {
      mComputedWidth = mComputedMinWidth;
    }
  }

  // See if none of 'left', 'width', and 'right', is 'auto'
  if (!leftIsAuto && !widthIsAuto && !rightIsAuto) {
    // See whether we're over-constrained
    PRInt32 availBoxSpace = containingBlockWidth - mComputedOffsets.left - mComputedOffsets.right;
    PRInt32 availContentSpace = availBoxSpace - mComputedBorderPadding.left -
                                mComputedBorderPadding.right;

    if (availContentSpace < mComputedWidth) {
      // We're over-constrained so use 'direction' to dictate which value to
      // ignore
      if (NS_STYLE_DIRECTION_LTR == direction) {
        // Ignore the specified value for 'right'
        mComputedOffsets.right = containingBlockWidth - mComputedOffsets.left -
          mComputedBorderPadding.left - mComputedWidth - mComputedBorderPadding.right;
      } else {
        // Ignore the specified value for 'left'
        mComputedOffsets.left = containingBlockWidth - mComputedBorderPadding.left -
          mComputedWidth - mComputedBorderPadding.right - mComputedOffsets.right;
      }

    } else {
      // Calculate any 'auto' margin values
      PRBool  marginLeftIsAuto = (eStyleUnit_Auto == mStyleSpacing->mMargin.GetLeftUnit());
      PRBool  marginRightIsAuto = (eStyleUnit_Auto == mStyleSpacing->mMargin.GetRightUnit());
      PRInt32 availMarginSpace = availContentSpace - mComputedWidth;

      if (marginLeftIsAuto) {
        if (marginRightIsAuto) {
          // Both 'margin-left' and 'margin-right' are 'auto', so they get
          // equal values
          mComputedMargin.left = availMarginSpace / 2;
          mComputedMargin.right = availMarginSpace - mComputedMargin.left;
        } else {
          // Just 'margin-left' is 'auto'
          mComputedMargin.left = availMarginSpace - mComputedMargin.right;
        }
      } else {
        // Just 'margin-right' is 'auto'
        mComputedMargin.right = availMarginSpace - mComputedMargin.left;
      }
    }

  } else {
    // See if all three of 'left', 'width', and 'right', are 'auto'
    if (leftIsAuto && widthIsAuto && rightIsAuto) {
      // Use the 'direction' to dictate whether 'left' or 'right' is
      // treated like 'static-position'
      if (NS_STYLE_DIRECTION_LTR == direction) {
        if (hypotheticalBox.mLeftIsExact) {
          mComputedOffsets.left = hypotheticalBox.mLeft;
          leftIsAuto = PR_FALSE;
        } else {
          // Well, we don't know 'left' so we have to use 'right' and
          // then solve for 'left'
          mComputedOffsets.right = hypotheticalBox.mRight;
          rightIsAuto = PR_FALSE;
        }
      } else {
        if (hypotheticalBox.mRightIsExact) {
          mComputedOffsets.right = containingBlockWidth - hypotheticalBox.mRight;
          rightIsAuto = PR_FALSE;
        } else {
          // Well, we don't know 'right' so we have to use 'left' and
          // then solve for 'right'
          mComputedOffsets.left = hypotheticalBox.mLeft;
          leftIsAuto = PR_FALSE;
        }
      }
    }

    // At this point we know that at least one of 'left', 'width', and 'right'
    // is 'auto', but not all three. Examine the various combinations
    if (widthIsAuto) {
      if (leftIsAuto || rightIsAuto) {
        if (NS_FRAME_IS_REPLACED(mFrameType)) {
          // For a replaced element we use the intrinsic size
          mComputedWidth = NS_INTRINSICSIZE;
        } else {
          // The width is shrink-to-fit but constrained by the containing block width
          mComputedWidth = NS_SHRINKWRAPWIDTH;
          
          PRInt32 maxWidth = containingBlockWidth - mComputedOffsets.left -
                             mComputedMargin.left - mComputedBorderPadding.left -
                             mComputedBorderPadding.right - mComputedMargin.right -
                             mComputedOffsets.right;
          if (maxWidth <= 0) {
            maxWidth = 1;
          }
          if (mComputedMaxWidth > maxWidth) {
            mComputedMaxWidth = maxWidth;
          }
        }

        if (leftIsAuto) {
          mComputedOffsets.left = NS_AUTOOFFSET;   // solve for 'left'
        } else {
          mComputedOffsets.right = NS_AUTOOFFSET;  // solve for 'right'
        }

      } else {
        // Only 'width' is 'auto' so just solve for 'width'
        mComputedWidth = containingBlockWidth - mComputedOffsets.left -
          mComputedMargin.left - mComputedBorderPadding.left -
          mComputedBorderPadding.right -
          mComputedMargin.right - mComputedOffsets.right;

        // Factor in any minimum and maximum size information
        if (mComputedWidth > mComputedMaxWidth) {
          mComputedWidth = mComputedMaxWidth;
        } else if (mComputedWidth < mComputedMinWidth) {
          mComputedWidth = mComputedMinWidth;
        }

        // XXX If the direction is rtl then we need to reevaluate left...
      }

    } else {
      // Either 'left' or 'right' or both is 'auto'
      if (leftIsAuto && rightIsAuto) {
        // Use the 'direction' to dictate whether 'left' or 'right' is treated like
        // 'static-position'
        if (NS_STYLE_DIRECTION_LTR == direction) {
          if (hypotheticalBox.mLeftIsExact) {
            mComputedOffsets.left = hypotheticalBox.mLeft;
            leftIsAuto = PR_FALSE;
          } else {
            // Well, we don't know 'left' so we have to use 'right' and
            // then solve for 'left'
            mComputedOffsets.right = hypotheticalBox.mRight;
            rightIsAuto = PR_FALSE;
          }
        } else {
          if (hypotheticalBox.mRightIsExact) {
            mComputedOffsets.right = containingBlockWidth - hypotheticalBox.mRight;
            rightIsAuto = PR_FALSE;
          } else {
            // Well, we don't know 'right' so we have to use 'left' and
            // then solve for 'right'
            mComputedOffsets.left = hypotheticalBox.mLeft;
            leftIsAuto = PR_FALSE;
          }
        }
      }

      if (leftIsAuto) {
        // Solve for 'left'
        mComputedOffsets.left = containingBlockWidth - mComputedMargin.left -
          mComputedBorderPadding.left - mComputedWidth - mComputedBorderPadding.right - 
          mComputedMargin.right - mComputedOffsets.right;

      } else if (rightIsAuto) {
        // Solve for 'right'
        mComputedOffsets.right = containingBlockWidth - mComputedOffsets.left -
          mComputedMargin.left - mComputedBorderPadding.left - mComputedWidth -
          mComputedBorderPadding.right - mComputedMargin.right;
      }
    }
  }

  // Initialize the 'top' and 'bottom' computed offsets
  nsStyleUnit heightUnit = mStylePosition->mHeight.GetUnit();
  PRBool      topIsAuto = PR_FALSE, bottomIsAuto = PR_FALSE;
  if (eStyleUnit_Inherit == mStylePosition->mOffset.GetTopUnit()) {
    mComputedOffsets.top = cbrs->mComputedOffsets.top;
  } else if (eStyleUnit_Auto == mStylePosition->mOffset.GetTopUnit()) {
    mComputedOffsets.top = 0;
    topIsAuto = PR_TRUE;
  } else {
    nsStyleCoord c;
    ComputeVerticalValue(containingBlockHeight,
                         mStylePosition->mOffset.GetTopUnit(),
                         mStylePosition->mOffset.GetTop(c),
                         mComputedOffsets.top);
  }
  if (eStyleUnit_Inherit == mStylePosition->mOffset.GetBottomUnit()) {
    mComputedOffsets.bottom = cbrs->mComputedOffsets.bottom;
  } else if (eStyleUnit_Auto == mStylePosition->mOffset.GetBottomUnit()) {
    mComputedOffsets.bottom = 0;        
    bottomIsAuto = PR_TRUE;
  } else {
    nsStyleCoord c;
    ComputeVerticalValue(containingBlockHeight,
                         mStylePosition->mOffset.GetBottomUnit(),
                         mStylePosition->mOffset.GetBottom(c),
                         mComputedOffsets.bottom);
  }

  // Initialize the 'height' computed value
  PRBool  heightIsAuto = (eStyleUnit_Auto == heightUnit);
  if (!heightIsAuto) {
    if (eStyleUnit_Inherit == heightUnit) {
      // The inherited value comes from the parent and not the absolute
      // containing block
      mComputedHeight = blockContentArea.bottom - blockContentArea.top;

    } else {
      // Use the specified value for the computed height
      // XXX Handle 'inherit'. The inherited value comes from the parent
      // and not the containing block
      ComputeVerticalValue(containingBlockHeight, heightUnit,
                           mStylePosition->mHeight, mComputedHeight);
    }
    
    // Factor in any minimum and maximum size information
    if (mComputedHeight > mComputedMaxHeight) {
      mComputedHeight = mComputedMaxHeight;
    } else if (mComputedHeight < mComputedMinHeight) {
      mComputedHeight = mComputedMinHeight;
    }
  }

  // See if none of 'top', 'height', and 'bottom', is 'auto'
  if (!topIsAuto && !heightIsAuto && !bottomIsAuto) {
    // See whether we're over-constrained
    PRInt32 availBoxSpace = containingBlockHeight - mComputedOffsets.top - mComputedOffsets.bottom;
    PRInt32 availContentSpace = availBoxSpace - mComputedBorderPadding.top -
                                mComputedBorderPadding.bottom;

    if (availContentSpace < mComputedHeight) {
      // We're over-constrained so ignore the specified value for 'bottom'
      mComputedOffsets.bottom = containingBlockHeight - mComputedOffsets.top -
        mComputedBorderPadding.top - mComputedHeight - mComputedBorderPadding.bottom;

    } else {
      // Calculate any 'auto' margin values
      PRBool  marginTopIsAuto = (eStyleUnit_Auto == mStyleSpacing->mMargin.GetTopUnit());
      PRBool  marginBottomIsAuto = (eStyleUnit_Auto == mStyleSpacing->mMargin.GetBottomUnit());
      PRInt32 availMarginSpace = availContentSpace - mComputedHeight;

      if (marginTopIsAuto) {
        if (marginBottomIsAuto) {
          // Both 'margin-top' and 'margin-bottom' are 'auto', so they get
          // equal values
          mComputedMargin.top = availMarginSpace / 2;
          mComputedMargin.bottom = availMarginSpace - mComputedMargin.top;
        } else {
          // Just 'margin-top' is 'auto'
          mComputedMargin.top = availMarginSpace - mComputedMargin.bottom;
        }
      } else {
        // Just 'margin-bottom' is 'auto'
        mComputedMargin.bottom = availMarginSpace - mComputedMargin.top;
      }
    }

  } else {
    // See if all three of 'top', 'height', and 'bottom', are 'auto'
    if (topIsAuto && heightIsAuto && bottomIsAuto) {
      // Treat 'top' like 'static-position'
      mComputedOffsets.top = hypotheticalBox.mTop;
      topIsAuto = PR_FALSE;
    }

    // At this point we know that at least one of 'top', 'height', and 'bottom'
    // is 'auto', but not all three. Examine the various combinations
    if (heightIsAuto) {
      if (topIsAuto || bottomIsAuto) {
        if (NS_FRAME_IS_REPLACED(mFrameType)) {
          // For a replaced element we use the intrinsic size
          mComputedHeight = NS_INTRINSICSIZE;
        } else {
          // The height is based on the content
          mComputedHeight = NS_AUTOHEIGHT;
        }

        if (topIsAuto) {
          mComputedOffsets.top = NS_AUTOOFFSET;     // solve for 'top'
        } else {
          mComputedOffsets.bottom = NS_AUTOOFFSET;  // solve for 'bottom'
        }

      } else {
        // Only 'height' is 'auto' so just solve for 'height'
        mComputedHeight = containingBlockHeight - mComputedOffsets.top -
          mComputedMargin.top - mComputedBorderPadding.top -
          mComputedBorderPadding.bottom -
          mComputedMargin.bottom - mComputedOffsets.bottom;

        // Factor in any minimum and maximum size information
        if (mComputedHeight > mComputedMaxHeight) {
          mComputedHeight = mComputedMaxHeight;
        } else if (mComputedHeight < mComputedMinHeight) {
          mComputedHeight = mComputedMinHeight;
        }
      }

    } else {
      // Either 'top' or 'bottom' or both is 'auto'
      if (topIsAuto && bottomIsAuto) {
        // Treat 'top' like 'static-position'
        mComputedOffsets.top = hypotheticalBox.mTop;
        topIsAuto = PR_FALSE;
      }

      if (topIsAuto) {
        // Solve for 'top'
        mComputedOffsets.top = containingBlockHeight - mComputedMargin.top -
          mComputedBorderPadding.top - mComputedHeight - mComputedBorderPadding.bottom - 
          mComputedMargin.bottom - mComputedOffsets.bottom;

      } else if (bottomIsAuto) {
        // Solve for 'bottom'
        mComputedOffsets.bottom = containingBlockHeight - mComputedOffsets.top -
          mComputedMargin.top - mComputedBorderPadding.top - mComputedHeight -
          mComputedBorderPadding.bottom - mComputedMargin.bottom;
      }
    }
  }
}

static PRBool
IsInitialContainingBlock(nsIFrame* aFrame)
{
  nsIContent* content;
  PRBool      result = PR_FALSE;

  aFrame->GetContent(&content);
  if (content) {
    nsIContent* parentContent;

    content->GetParent(parentContent);
    if (!parentContent) {
      // The containing block corresponds to the document element so it's
      // the initial containing block
      result = PR_TRUE;
    }
    NS_IF_RELEASE(parentContent);
  }
  NS_IF_RELEASE(content);
  return result;
}

nscoord 
GetVerticalMarginBorderPadding(const nsHTMLReflowState* aReflowState)
{
  nscoord result = 0;
  if (!aReflowState) return result;

  // zero auto margins
  nsMargin margin = aReflowState->mComputedMargin;
  if (NS_AUTOMARGIN == margin.top) 
    margin.top = 0;
  if (NS_AUTOMARGIN == margin.bottom) 
    margin.bottom = 0;

  result += margin.top + margin.bottom;
  result += aReflowState->mComputedBorderPadding.top + 
            aReflowState->mComputedBorderPadding.bottom;

  return result;
}

/* Get the height based on the viewport of the containing block specified 
 * in aReflowState when the containing block has mComputedHeight == NS_AUTOHEIGHT
 * and it is the body.
 */
nscoord
CalcQuirkContainingBlockHeight(const nsHTMLReflowState& aReflowState)
{
  nsHTMLReflowState* firstBlockRS = nsnull; // a candidate for body frame
  nsHTMLReflowState* firstAreaRS  = nsnull; // a candidate for html frame
  nscoord result = 0;

  const nsHTMLReflowState* rs = &aReflowState;
  for (; rs; rs = (nsHTMLReflowState *)(rs->parentReflowState)) { 
    nsCOMPtr<nsIAtom> frameType;
    rs->frame->GetFrameType(getter_AddRefs(frameType));
    // if the ancestor is auto height then skip it and continue up if it 
    // is the first block/area frame and possibly the body/html
    if (nsLayoutAtoms::blockFrame == frameType.get()) {
      if (!firstBlockRS) {
        firstBlockRS = (nsHTMLReflowState*)rs;
        if (NS_AUTOHEIGHT == rs->mComputedHeight) continue;
      }
      else break;
    }
    else if (nsLayoutAtoms::areaFrame == frameType.get()) {
      if (!firstAreaRS) {
        firstAreaRS = (nsHTMLReflowState*)rs;
        if (NS_AUTOHEIGHT == rs->mComputedHeight) continue;
      }
      else break;
    }
    else if (nsLayoutAtoms::canvasFrame == frameType.get()) {
      // Use scroll frames' computed height if we have one, this will
      // allow us to get viewport height for native scrollbars.
      nsHTMLReflowState* scrollState = (nsHTMLReflowState *)rs->parentReflowState;
      nsCOMPtr<nsIAtom> scrollFrameType;
      scrollState->frame->GetFrameType(getter_AddRefs(scrollFrameType));
      if (nsLayoutAtoms::scrollFrame == scrollFrameType.get()) {
        rs = scrollState;
      }
	}
    else {
      break;
    }

    // if the ancestor has a computed height, it is the percent base
    result = rs->mComputedHeight;
    // if unconstrained - don't sutract borders - would result in huge height
    if (NS_AUTOHEIGHT == result) return result;

    // if we got to the canvas frame, then subtract out 
    // margin/border/padding for the BODY and HTML elements
    if (nsLayoutAtoms::canvasFrame == frameType.get()) {
      result -= GetVerticalMarginBorderPadding(firstBlockRS); 
      result -= GetVerticalMarginBorderPadding(firstAreaRS); 
    }
    // if we got to the html frame, then subtract out 
    // margin/border/padding for the BODY element
    else if (nsLayoutAtoms::areaFrame == frameType.get()) {
      // make sure it is the body
      nsCOMPtr<nsIAtom> fType;
      rs->parentReflowState->frame->GetFrameType(getter_AddRefs(fType));
      if (nsLayoutAtoms::canvasFrame == fType.get()) {
        result -= GetVerticalMarginBorderPadding(firstBlockRS);
      }
    }
    break;
  }

  return result;
}
// Called by InitConstraints() to compute the containing block rectangle for
// the element. Handles the special logic for absolutely positioned elements
void
nsHTMLReflowState::ComputeContainingBlockRectangle(nsIPresContext*          aPresContext,
                                                   const nsHTMLReflowState* aContainingBlockRS,
                                                   nscoord&                 aContainingBlockWidth,
                                                   nscoord&                 aContainingBlockHeight)
{
  // Unless the element is absolutely positioned, the containing block is
  // formed by the content edge of the nearest block-level ancestor
  aContainingBlockWidth = aContainingBlockRS->mComputedWidth;
  aContainingBlockHeight = aContainingBlockRS->mComputedHeight;
  
  if (NS_FRAME_GET_TYPE(mFrameType) == NS_CSS_FRAME_TYPE_ABSOLUTE) {
    // See if the ancestor is block-level or inline-level
    if (NS_FRAME_GET_TYPE(aContainingBlockRS->mFrameType) == NS_CSS_FRAME_TYPE_INLINE) {
      // The CSS2 spec says that if the ancestor is inline-level, the containing
      // block depends on the 'direction' property of the ancestor. For direction
      // 'ltr', it's the top and left of the content edges of the first box and
      // the bottom and right content edges of the last box
      //
      // XXX This is a pain because it isn't top-down and it requires that we've
      // completely reflowed the ancestor. It also isn't clear what happens when
      // a relatively positioned ancestor is split across pages. So instead use
      // the computed width and height of the nearest block-level ancestor
      const nsHTMLReflowState*  cbrs = aContainingBlockRS;
      while (cbrs) {
        nsCSSFrameType  type = NS_FRAME_GET_TYPE(cbrs->mFrameType);
        if ((NS_CSS_FRAME_TYPE_BLOCK == type) ||
            (NS_CSS_FRAME_TYPE_FLOATING == type) ||
            (NS_CSS_FRAME_TYPE_ABSOLUTE == type)) {

          aContainingBlockWidth = cbrs->mComputedWidth;
          aContainingBlockHeight = cbrs->mComputedHeight;

          if (NS_CSS_FRAME_TYPE_ABSOLUTE == type) {
            aContainingBlockWidth += cbrs->mComputedPadding.left +
                                     cbrs->mComputedPadding.right;
            aContainingBlockHeight += cbrs->mComputedPadding.top +
                                      cbrs->mComputedPadding.bottom;
          }
          break;
        }

        cbrs = (const nsHTMLReflowState*)cbrs->parentReflowState;   // XXX cast
      }

    } else {
      // If the ancestor is block-level, the containing block is formed by the
      // padding edge of the ancestor
      aContainingBlockWidth += aContainingBlockRS->mComputedPadding.left +
                               aContainingBlockRS->mComputedPadding.right;

      // If the containing block is the initial containing block and it has a
      // height that depends on its content, then use the viewport height instead.
      // This gives us a reasonable value against which to compute percentage
      // based heights and to do bottom relative positioning
      if ((NS_AUTOHEIGHT == aContainingBlockHeight) &&
          IsInitialContainingBlock(aContainingBlockRS->frame)) {

        // Use the viewport height as the containing block height
        const nsHTMLReflowState* rs = aContainingBlockRS->parentReflowState;
        while (rs) {
          aContainingBlockHeight = rs->mComputedHeight;
          rs = rs->parentReflowState;
        }

      } else {
        aContainingBlockHeight += aContainingBlockRS->mComputedPadding.top +
                                  aContainingBlockRS->mComputedPadding.bottom;
      }
    }
  } else {
    // If this is an unconstrained reflow, then reset the containing block
    // width to NS_UNCONSTRAINEDSIZE. This way percentage based values have
    // no effect
    if (NS_UNCONSTRAINEDSIZE == availableWidth) {
      aContainingBlockWidth = NS_UNCONSTRAINEDSIZE;
    }
    // a table in quirks mode gets a containing block based on the viewport (less  
    // body margins, border, padding) if the table is a child of the body.
    if (NS_AUTOHEIGHT == aContainingBlockHeight) {
      nsCOMPtr<nsIAtom> fType;
      frame->GetFrameType(getter_AddRefs(fType));
      if (nsLayoutAtoms::tableFrame == fType.get() ||
          nsLayoutAtoms::htmlFrameOuterFrame == fType.get()) {
        nsCompatibility mode;
        aPresContext->GetCompatibilityMode(&mode);
        if (eCompatibility_NavQuirks == mode) {
          aContainingBlockHeight = CalcQuirkContainingBlockHeight(*aContainingBlockRS);
        }
      }
    }
  }
}

// XXX refactor this code to have methods for each set of properties
// we are computing: width,height,line-height; margin; offsets

void
nsHTMLReflowState::InitConstraints(nsIPresContext* aPresContext,
                                   nscoord         aContainingBlockWidth,
                                   nscoord         aContainingBlockHeight)
{
  // If this is the root frame, then set the computed width and
  // height equal to the available space
  if (nsnull == parentReflowState) {
    mComputedWidth = availableWidth;
    mComputedHeight = availableHeight;
    mComputedMargin.SizeTo(0, 0, 0, 0);
    mComputedPadding.SizeTo(0, 0, 0, 0);
    mComputedBorderPadding.SizeTo(0, 0, 0, 0);
    mComputedOffsets.SizeTo(0, 0, 0, 0);
    mComputedMinWidth = mComputedMinHeight = 0;
    mComputedMaxWidth = mComputedMaxHeight = NS_UNCONSTRAINEDSIZE;
  } else {
    // Get the containing block reflow state
    const nsHTMLReflowState* cbrs =
      GetContainingBlockReflowState(parentReflowState);
    NS_ASSERTION(nsnull != cbrs, "no containing block");

    // If we weren't given a containing block width and height, then
    // compute one
    if (aContainingBlockWidth == -1) {
      ComputeContainingBlockRectangle(aPresContext, cbrs, aContainingBlockWidth, 
                                      aContainingBlockHeight);
    }

    // See if the element is relatively positioned
    if (NS_STYLE_POSITION_RELATIVE == mStylePosition->mPosition) {
      ComputeRelativeOffsets(cbrs, aContainingBlockWidth, aContainingBlockHeight);
    } else {
      // Initialize offsets to 0
      mComputedOffsets.SizeTo(0, 0, 0, 0);
    }

#if 0
    nsFrame::ListTag(stdout, frame); printf(": cb=");
    nsFrame::ListTag(stdout, cbrs->frame); printf(" size=%d,%d\n", aContainingBlockWidth, aContainingBlockHeight);
#endif

    // See if the containing block height is based on the size of its
    // content
    if (NS_AUTOHEIGHT == aContainingBlockHeight) {
      // See if the containing block is a scrolled frame, i.e. its
      // parent is a scroll frame. The presence of the intervening
      // frame (that the scroll frame scrolls) needs to be hidden from
      // the containingBlockHeight calcuation.
      if (cbrs->parentReflowState) {
        nsIFrame* f = cbrs->parentReflowState->frame;
        nsCOMPtr<nsIAtom>  fType;
        f->GetFrameType(getter_AddRefs(fType));
        if (nsLayoutAtoms::scrollFrame == fType.get()) {
          // Use the scroll frame's computed height instead
          aContainingBlockHeight =
            ((nsHTMLReflowState*)cbrs->parentReflowState)->mComputedHeight;
        }
      }
    }

    // Compute margins from the specified margin style information. These
    // become the default computed values, and may be adjusted below
    // XXX fix to provide 0,0 for the top&bottom margins for
    // inline-non-replaced elements
    ComputeMargin(aContainingBlockWidth, cbrs);
    ComputePadding(aContainingBlockWidth, cbrs);
    if (!mStyleSpacing->GetBorder(mComputedBorderPadding)) {
      // CSS2 has no percentage borders
      mComputedBorderPadding.SizeTo(0, 0, 0, 0);
    }
    mComputedBorderPadding += mComputedPadding;

    nsStyleUnit widthUnit = mStylePosition->mWidth.GetUnit();
    nsStyleUnit heightUnit = mStylePosition->mHeight.GetUnit();

    nsCOMPtr<nsIAtom>  frameType;
    frame->GetFrameType(getter_AddRefs(frameType));

    // Check for a percentage based width and an unconstrained containing
    // block width
    if (eStyleUnit_Percent == widthUnit) {
      if ((NS_UNCONSTRAINEDSIZE == aContainingBlockWidth) ||
          ((cbrs->mIsAutoWidthPctBase) && 
           (nsLayoutAtoms::tableOuterFrame != frameType.get()) &&
           (nsLayoutAtoms::tableFrame      != frameType.get()))) {
        // Interpret the width like 'auto'
        widthUnit = eStyleUnit_Auto;
      }
    }
    // if the cbrs is an auto width descendant (non table) from a table cell and
    // our width may be determined using it, then propogate the flag.
    if (cbrs->mIsAutoWidthPctBase) {
      if ((eStyleUnit_Auto    == widthUnit) ||
          (eStyleUnit_Percent == widthUnit) ||
          (eStyleUnit_Inherit == widthUnit)) {
        if ((nsLayoutAtoms::tableOuterFrame != frameType.get()) &&
            (nsLayoutAtoms::tableFrame      != frameType.get())) {
          mIsAutoWidthPctBase = PR_TRUE;
        }
      }
    }

    // Check for a percentage based height and a containing block height
    // that depends on the content height
    if (eStyleUnit_Percent == heightUnit) {
      if (NS_AUTOHEIGHT == aContainingBlockHeight) {
        // Interpret the height like 'auto'
        heightUnit = eStyleUnit_Auto;
      }
    }

    // Calculate the computed values for min and max properties
    ComputeMinMaxValues(aContainingBlockWidth, aContainingBlockHeight, cbrs);

    // Calculate the computed width and height. This varies by frame type
    if ((NS_FRAME_REPLACED(NS_CSS_FRAME_TYPE_INLINE) == mFrameType) ||
        (NS_FRAME_REPLACED(NS_CSS_FRAME_TYPE_FLOATING) == mFrameType)) {
      // Inline replaced element and floating replaced element are basically
      // treated the same. First calculate the computed width
      if (eStyleUnit_Inherit == widthUnit) {
        mComputedWidth = aContainingBlockWidth;
      } else if (eStyleUnit_Auto == widthUnit) {
        // A specified value of 'auto' uses the element's intrinsic width
        mComputedWidth = NS_INTRINSICSIZE;
      } else {
        ComputeHorizontalValue(aContainingBlockWidth, widthUnit,
                               mStylePosition->mWidth,
                               mComputedWidth);
      }

      if (mComputedWidth != NS_INTRINSICSIZE) {
        // Take into account minimum and maximum sizes
        if (mComputedWidth > mComputedMaxWidth) {
          mComputedWidth = mComputedMaxWidth;
        } else if (mComputedWidth < mComputedMinWidth) {
          mComputedWidth = mComputedMinWidth;
        }
  
        // See what edge the width applies to (the default is the content
        // edge)
        if (mStylePosition->mBoxSizing == NS_STYLE_BOX_SIZING_PADDING) {
          mComputedWidth -= mComputedPadding.left + mComputedPadding.right;
        } else if (mStylePosition->mBoxSizing == NS_STYLE_BOX_SIZING_BORDER) {
          mComputedWidth -= mComputedBorderPadding.left + mComputedBorderPadding.right;
        }
      }

      // Now calculate the computed height
      if (eStyleUnit_Inherit == heightUnit) {
        mComputedHeight = aContainingBlockHeight;
      } else if (eStyleUnit_Auto == heightUnit) {
        // A specified value of 'auto' uses the element's intrinsic height
        mComputedHeight = NS_INTRINSICSIZE;
      } else {
        ComputeVerticalValue(aContainingBlockHeight, heightUnit,
                             mStylePosition->mHeight,
                             mComputedHeight);
      }

      if (mComputedHeight != NS_INTRINSICSIZE) {
        // Take into account minimum and maximum sizes
        if (mComputedHeight > mComputedMaxHeight) {
          mComputedHeight = mComputedMaxHeight;
        } else if (mComputedHeight < mComputedMinHeight) {
          mComputedHeight = mComputedMinHeight;
        }
  
        // See what edge the height applies to (the default is the content
        // edge)
        if (mStylePosition->mBoxSizing == NS_STYLE_BOX_SIZING_PADDING) {
          mComputedHeight -= mComputedPadding.top + mComputedPadding.bottom;
        } else if (mStylePosition->mBoxSizing == NS_STYLE_BOX_SIZING_BORDER) {
          mComputedHeight -= mComputedBorderPadding.top + mComputedBorderPadding.bottom;
        }
      }
    
    } else if (NS_CSS_FRAME_TYPE_FLOATING == mFrameType) {
      // Floating non-replaced element. First calculate the computed width
      if (eStyleUnit_Inherit == widthUnit) {
        mComputedWidth = aContainingBlockWidth;
      } else if (eStyleUnit_Auto == widthUnit) {
        if ((NS_UNCONSTRAINEDSIZE == aContainingBlockWidth) &&
            (eStyleUnit_Percent == mStylePosition->mWidth.GetUnit())) {
          // The element has a percentage width, but since the containing
          // block width is unconstrained we set 'widthUnit' to 'auto'
          // above. However, we want the element to be unconstrained, too
          mComputedWidth = NS_UNCONSTRAINEDSIZE;

        } else if (NS_STYLE_DISPLAY_TABLE == mStyleDisplay->mDisplay) {
          // It's an outer table because an inner table is not positioned
          // shrink wrap its width since the outer table is anonymous
          mComputedWidth = NS_SHRINKWRAPWIDTH;

        } else {
          // The CSS2 spec says the computed width should be 0; however, that's
          // not what Nav and IE do and even the spec doesn't really want that
          // to happen.
          //
          // Instead, have the element shrink wrap its width
          mComputedWidth = NS_SHRINKWRAPWIDTH;

          // Limnit the width to the containing block width
          if (mComputedMaxWidth > aContainingBlockWidth) {
            mComputedMaxWidth = aContainingBlockWidth;
          }
        }

      } else {
        ComputeHorizontalValue(aContainingBlockWidth, widthUnit,
                               mStylePosition->mWidth,
                               mComputedWidth);
      }

      // Take into account minimum and maximum sizes
      if (mComputedWidth != NS_SHRINKWRAPWIDTH) {
        if (mComputedWidth > mComputedMaxWidth) {
          mComputedWidth = mComputedMaxWidth;
        } else if (mComputedWidth < mComputedMinWidth) {
          mComputedWidth = mComputedMinWidth;
        }
  
        // See what edge the width applies to (the default is the content
        // edge)
        if (mComputedWidth > 0) {
          if (mStylePosition->mBoxSizing == NS_STYLE_BOX_SIZING_PADDING) {
            mComputedWidth -= mComputedPadding.left + mComputedPadding.right;
          } else if (mStylePosition->mBoxSizing == NS_STYLE_BOX_SIZING_BORDER) {
            mComputedWidth -= mComputedBorderPadding.left + mComputedBorderPadding.right;
          }
        }
      }

      // Now calculate the computed height
      if (eStyleUnit_Inherit == heightUnit) {
        mComputedHeight = aContainingBlockHeight;
      } else if (eStyleUnit_Auto == heightUnit) {
        mComputedHeight = NS_AUTOHEIGHT;  // let it choose its height
      } else {
        ComputeVerticalValue(aContainingBlockHeight, heightUnit,
                             mStylePosition->mHeight,
                             mComputedHeight);
      }

      // Take into account minimum and maximum sizes
      if (mComputedHeight != NS_AUTOHEIGHT) {
        if (mComputedHeight > mComputedMaxHeight) {
          mComputedHeight = mComputedMaxHeight;
        } else if (mComputedHeight < mComputedMinHeight) {
          mComputedHeight = mComputedMinHeight;
        }
  
        // See what edge the height applies to (the default is the content
        // edge)
        if (mStylePosition->mBoxSizing == NS_STYLE_BOX_SIZING_PADDING) {
          mComputedHeight -= mComputedPadding.top + mComputedPadding.bottom;
        } else if (mStylePosition->mBoxSizing == NS_STYLE_BOX_SIZING_BORDER) {
          mComputedHeight -= mComputedBorderPadding.top + mComputedBorderPadding.bottom;
        }
      }
    
    } else if (NS_CSS_FRAME_TYPE_INTERNAL_TABLE == mFrameType) {
      // Internal table elements. The rules vary depending on the type.
      // Calculate the computed width
      if ((NS_STYLE_DISPLAY_TABLE_ROW == mStyleDisplay->mDisplay) ||
          (NS_STYLE_DISPLAY_TABLE_ROW_GROUP == mStyleDisplay->mDisplay)) {
        // 'width' property doesn't apply to table rows and row groups
        widthUnit = eStyleUnit_Auto;
      }

      if (eStyleUnit_Inherit == widthUnit) {
        mComputedWidth = aContainingBlockWidth;
      } else if (eStyleUnit_Auto == widthUnit) {
        if (NS_STYLE_DISPLAY_TABLE_CELL == mStyleDisplay->mDisplay) {
          // set the flag to ignore mComputedWidth for percent calculations 
          // when the cell is a containing block.
          mIsAutoWidthPctBase = PR_TRUE;
        }
        mComputedWidth = availableWidth;

        if (mComputedWidth != NS_UNCONSTRAINEDSIZE) {
          // Internal table elements don't have margins, but they have border
          // and padding
          mComputedWidth -= mComputedBorderPadding.left +
            mComputedBorderPadding.right;
        }
      
      } else {
        ComputeHorizontalValue(aContainingBlockWidth, widthUnit,
                               mStylePosition->mWidth,
                               mComputedWidth);
      }

      // Calculate the computed height
      if ((NS_STYLE_DISPLAY_TABLE_COLUMN == mStyleDisplay->mDisplay) ||
          (NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP == mStyleDisplay->mDisplay)) {
        // 'height' property doesn't apply to table columns and column groups
        heightUnit = eStyleUnit_Auto;
      }
      if (eStyleUnit_Inherit == heightUnit) {
        mComputedHeight = aContainingBlockHeight;
      } else if (eStyleUnit_Auto == heightUnit) {
        mComputedHeight = NS_AUTOHEIGHT;
      } else {
        ComputeVerticalValue(aContainingBlockHeight, heightUnit,
                             mStylePosition->mHeight,
                             mComputedHeight);
      }

      // Doesn't apply to table elements
      mComputedMinWidth = mComputedMinHeight = 0;
      mComputedMaxWidth = mComputedMaxHeight = NS_UNCONSTRAINEDSIZE;

    } else if (NS_FRAME_GET_TYPE(mFrameType) == NS_CSS_FRAME_TYPE_ABSOLUTE) {
      // XXX not sure if this belongs here or somewhere else - cwk
      // an nsHTMLFrameInnerFrame doesn't get a placeholder frame, the nsHTMLFrameOuterFrame does
      nsIAtom* targetFrameType;
      frame->GetFrameType(&targetFrameType);
      if (nsLayoutAtoms::htmlFrameInnerFrame != targetFrameType) {
        InitAbsoluteConstraints(aPresContext, cbrs, aContainingBlockWidth,
                                aContainingBlockHeight);
      }
      NS_IF_RELEASE(targetFrameType);
    } else if (NS_CSS_FRAME_TYPE_INLINE == mFrameType) {
      // Inline non-replaced elements do not have computed widths or heights
      // XXX add this check to HaveFixedContentHeight/Width too
      mComputedWidth = NS_UNCONSTRAINEDSIZE;
      mComputedHeight = NS_UNCONSTRAINEDSIZE;
      mComputedMargin.top = 0;
      mComputedMargin.bottom = 0;
      mComputedMinWidth = mComputedMinHeight = 0;
      mComputedMaxWidth = mComputedMaxHeight = NS_UNCONSTRAINEDSIZE;
    } else {
      ComputeBlockBoxData(aPresContext, cbrs, widthUnit, heightUnit,
                          aContainingBlockWidth,
                          aContainingBlockHeight);
    }
  }
}

// Compute the box data for block and block-replaced elements in the
// normal flow.
void
nsHTMLReflowState::ComputeBlockBoxData(nsIPresContext* aPresContext,
                                       const nsHTMLReflowState* cbrs,
                                       nsStyleUnit aWidthUnit,
                                       nsStyleUnit aHeightUnit,
                                       nscoord aContainingBlockWidth,
                                       nscoord aContainingBlockHeight)
{
  // Compute the content width
  if (eStyleUnit_Auto == aWidthUnit) {
    if (NS_FRAME_IS_REPLACED(mFrameType)) {
      // Block-level replaced element in the flow. A specified value of 
      // 'auto' uses the element's intrinsic width (CSS2 10.3.4)
      mComputedWidth = NS_INTRINSICSIZE;
    } else {
      // Block-level non-replaced element in the flow. 'auto' values
      // for margin-left and margin-right become 0, and the sum of the
      // areas must equal the width of the content-area of the parent
      // element.
      if (NS_UNCONSTRAINEDSIZE == availableWidth) {
        // During pass1 table reflow, auto side margin values are
        // uncomputable (== 0).
        mComputedWidth = NS_UNCONSTRAINEDSIZE;
      } else if (NS_SHRINKWRAPWIDTH == aContainingBlockWidth) {
        // The containing block should shrink wrap its width, so have
        // the child block do the same
        mComputedWidth = NS_UNCONSTRAINEDSIZE;

        // Let its content area be as wide as the containing block's max width
        // minus any margin and border/padding
        nscoord maxWidth = cbrs->mComputedMaxWidth - mComputedMargin.left -
                           mComputedBorderPadding.left - mComputedMargin.right -
                           mComputedBorderPadding.right;

        if (maxWidth < mComputedMaxWidth) {
          mComputedMaxWidth = maxWidth;
        }

      } else {
        // tables act like replaced elements regarding mComputedWidth 
        nsCOMPtr<nsIAtom> fType;
        frame->GetFrameType(getter_AddRefs(fType));
        if (nsLayoutAtoms::tableOuterFrame == fType.get()) {
          mComputedWidth = 0; // XXX temp fix for trees
        } else if (nsLayoutAtoms::tableFrame == fType.get()) {
          mComputedWidth = NS_SHRINKWRAPWIDTH;
          if (eStyleUnit_Auto == mStyleSpacing->mMargin.GetLeftUnit()) {
            mComputedMargin.left = NS_AUTOMARGIN;
          }
          if (eStyleUnit_Auto == mStyleSpacing->mMargin.GetRightUnit()) {
            mComputedMargin.right = NS_AUTOMARGIN;
          }
        } else {
          mComputedWidth = availableWidth - mComputedMargin.left -
            mComputedMargin.right - mComputedBorderPadding.left -
            mComputedBorderPadding.right;
        }

        // Take into account any min and max values
        if (mComputedWidth > mComputedMaxWidth) {
          // Use 'max-width' as the value for 'width'
          mComputedWidth = mComputedMaxWidth;
        } else if (mComputedWidth < mComputedMinWidth) {
          // Use 'min-width' as the value for 'width'
          mComputedWidth = mComputedMinWidth;
        }
      }
    }
  } else {
    if (eStyleUnit_Inherit == aWidthUnit) {
      // Use parent element's width. Note that if its width was
      // 'inherit', then it already did this so we don't need to
      // recurse upwards.
      //
      // We use the containing block's width here for the "parent"
      // elements width, because we want to skip over any intervening
      // inline elements (since width doesn't apply to them).
      if (NS_UNCONSTRAINEDSIZE != aContainingBlockWidth) {
        mComputedWidth = aContainingBlockWidth;
      }
      else {
        mComputedWidth = NS_UNCONSTRAINEDSIZE;
      }
    }
    else {
      ComputeHorizontalValue(aContainingBlockWidth, aWidthUnit,
                             mStylePosition->mWidth, mComputedWidth);
    }

    // Take into account any min and max values
    if (mComputedWidth > mComputedMaxWidth) {
      mComputedWidth = mComputedMaxWidth;
    } else if (mComputedWidth < mComputedMinWidth) {
      mComputedWidth = mComputedMinWidth;
    }

    // See what edge the width applies to (the default is the content
    // edge)
    if (mComputedWidth != NS_UNCONSTRAINEDSIZE) {
      if (mStylePosition->mBoxSizing == NS_STYLE_BOX_SIZING_PADDING) {
        mComputedWidth -= mComputedPadding.left + mComputedPadding.right;
      } else if (mStylePosition->mBoxSizing == NS_STYLE_BOX_SIZING_BORDER) {
        mComputedWidth -= mComputedBorderPadding.left + mComputedBorderPadding.right;
      }
    }

    // Now that we have the computed-width, compute the side margins
    CalculateBlockSideMargins(cbrs->mComputedWidth, mComputedWidth);
  }

  // Compute the content height
  if (eStyleUnit_Inherit == aHeightUnit) {
    // Use parent elements height (note that if its height was inherit
    // then it already did this so we don't need to recurse upwards).
    //
    // We use the containing blocks height here for the "parent"
    // elements height because we want to skip over any interveening
    // inline elements (since height doesn't apply to them).
    if (NS_UNCONSTRAINEDSIZE != aContainingBlockHeight) {
      mComputedHeight = aContainingBlockHeight;
    }
    else {
      mComputedHeight = NS_UNCONSTRAINEDSIZE;
    }
  } else if (eStyleUnit_Auto == aHeightUnit) {
    if (NS_FRAME_IS_REPLACED(mFrameType)) {
      // For replaced elements use the intrinsic size for "auto"
      mComputedHeight = NS_INTRINSICSIZE;
    } else {
      // For non-replaced elements auto means unconstrained
      mComputedHeight = NS_UNCONSTRAINEDSIZE;
    }
  } else {
    ComputeVerticalValue(aContainingBlockHeight, aHeightUnit,
                         mStylePosition->mHeight, mComputedHeight);
  }
  // Take into account any min and max values
  if (mComputedHeight != NS_AUTOHEIGHT) {
    if (mComputedHeight > mComputedMaxHeight) {
      mComputedHeight = mComputedMaxHeight;
    } else if (mComputedHeight < mComputedMinHeight) {
      mComputedHeight = mComputedMinHeight;
    }
  }
  // See what edge the height applies to (the default is the content
  // edge)
  if (mComputedHeight != NS_UNCONSTRAINEDSIZE) {
    if (mStylePosition->mBoxSizing == NS_STYLE_BOX_SIZING_PADDING) {
      mComputedHeight -= mComputedPadding.top + mComputedPadding.bottom;
    } else if (mStylePosition->mBoxSizing == NS_STYLE_BOX_SIZING_BORDER) {
      mComputedHeight -= mComputedBorderPadding.top + mComputedBorderPadding.bottom;
    }
  }
}

// This code enforces section 10.3.3 of the CSS2 spec for this formula:
//
// 'margin-left' + 'border-left-width' + 'padding-left' + 'width' +
//   'padding-right' + 'border-right-width' + 'margin-right'
//   = width of containing block 
//
// Note: the width unit is not auto when this is called
void
nsHTMLReflowState::CalculateBlockSideMargins(nscoord aAvailWidth,
                                             nscoord aComputedWidth)
{
  // We can only provide values for auto side margins in a constrained
  // reflow. For unconstrained reflow there is no effective width to
  // compute against...
  if ((NS_UNCONSTRAINEDSIZE == aComputedWidth) ||
      (NS_UNCONSTRAINEDSIZE == aAvailWidth)) {
    return;
  }

  nscoord sum = mComputedMargin.left + mComputedBorderPadding.left +
    aComputedWidth + mComputedBorderPadding.right + mComputedMargin.right;
  if (sum == aAvailWidth) {
    // The sum is already correct
    return;
  }

  // Determine the left and right margin values. The width value
  // remains constant while we do this.
  PRBool isAutoLeftMargin =
    eStyleUnit_Auto == mStyleSpacing->mMargin.GetLeftUnit();
  PRBool isAutoRightMargin =
    eStyleUnit_Auto == mStyleSpacing->mMargin.GetRightUnit();

  // Calculate how much space is available for margins
  nscoord availMarginSpace = aAvailWidth - aComputedWidth -
    mComputedBorderPadding.left - mComputedBorderPadding.right;

  if (mStyleDisplay->mDisplay == NS_STYLE_DISPLAY_TABLE) {
    // Special rules for tables. In general, tables will stick to the
    // left edge when they are too large otherwise they behave like
    // blocks.
    if (availMarginSpace < 0) {
      // Whoops - the TABLE element is too large for the available
      // space. In this case use the "direction" property to pin the
      // element to the left or right side. Note that we look at the
      // parent's direction since the parent will be placing this
      // element.
      mComputedMargin.left = 0;
      mComputedMargin.right = 0;
      const nsHTMLReflowState* prs = (const nsHTMLReflowState*)
        parentReflowState;
      if (prs && (NS_STYLE_DIRECTION_RTL == prs->mStyleDisplay->mDirection)) {
        mComputedMargin.left = availMarginSpace;
      }
      isAutoLeftMargin = isAutoRightMargin = PR_FALSE;
    }
  }
  else {
    // The css2 spec clearly defines how block elements should be have
    // in section 10.3.3.
    if (!isAutoLeftMargin && !isAutoRightMargin) {
      // Neither margin is 'auto' so we're over constrained. Use the
      // 'direction' property of the parent to tell which margin to
      // ignore
      const nsHTMLReflowState* prs = (const nsHTMLReflowState*)
        parentReflowState;
      if (prs) {
        if (NS_STYLE_DIRECTION_LTR == prs->mStyleDisplay->mDirection) {
          // The specified value of margin-right is ignored (== forced
          // to auto)
          isAutoRightMargin = PR_TRUE;
        }
        else {
          isAutoLeftMargin = PR_TRUE;
        }
      }
      else {
        // No parent reflow state -- assume direction is ltr
        isAutoRightMargin = PR_TRUE;
      }
    }
  }

  // Logic which is common to blocks and tables
  if (isAutoLeftMargin) {
    if (isAutoRightMargin) {
      // Both margins are 'auto' so their computed values are equal
      mComputedMargin.left = availMarginSpace / 2;
      mComputedMargin.right = availMarginSpace - mComputedMargin.left;
    } else {
      mComputedMargin.left = availMarginSpace - mComputedMargin.right;
    }
  } else if (isAutoRightMargin) {
    mComputedMargin.right = availMarginSpace - mComputedMargin.left;
  }
}

PRBool
nsHTMLReflowState::UseComputedHeight()
{
  static PRBool useComputedHeight = PR_FALSE;

#if defined(XP_UNIX) || defined(XP_PC) || defined(XP_BEOS)
  static PRBool firstTime = 1;
  if (firstTime) {
    if (getenv("GECKO_USE_COMPUTED_HEIGHT")) {
      useComputedHeight = PR_TRUE;
    }
    firstTime = 0;
  }
#endif
  return useComputedHeight;
}

static nsIStyleContext*
GetNonInheritedLineHeightStyleContext(nsIStyleContext* aStyleContext)
{
  nsIStyleContext* parentSC;
  parentSC = aStyleContext->GetParent();
  if (parentSC) {
    const nsStyleText* text = (const nsStyleText*)
      parentSC->GetStyleData(eStyleStruct_Text);
    if (eStyleUnit_Inherit == text->mLineHeight.GetUnit()) {
      nsIStyleContext* sc = GetNonInheritedLineHeightStyleContext(parentSC);
      NS_RELEASE(parentSC);
      return sc;
    }
  }
  return parentSC;
}

static nscoord
ComputeLineHeight(nsIRenderingContext* aRenderingContext,
                  nsIStyleContext* aStyleContext)
{
  NS_PRECONDITION(nsnull != aRenderingContext, "no rendering context");

  nscoord lineHeight = -1;

  const nsStyleText* text = (const nsStyleText*)
    aStyleContext->GetStyleData(eStyleStruct_Text);
  const nsStyleFont* font = (const nsStyleFont*)
    aStyleContext->GetStyleData(eStyleStruct_Font);

  nsStyleUnit unit = text->mLineHeight.GetUnit();
  if (eStyleUnit_Inherit == unit) {
    // Inherit parents line-height value
    nsCOMPtr<nsIStyleContext> parentSC =
      getter_AddRefs(GetNonInheritedLineHeightStyleContext(aStyleContext));
    if (parentSC) {
      text = (const nsStyleText*) parentSC->GetStyleData(eStyleStruct_Text);
      unit = text->mLineHeight.GetUnit();
      if (eStyleUnit_Percent == unit) {
        // For percent, we inherit the computed value so update the
        // font to use the parent's font not our font.
        font = (const nsStyleFont*) parentSC->GetStyleData(eStyleStruct_Font);
      }
    }
  }

  if (eStyleUnit_Coord == unit) {
    // For length values just use the pre-computed value
    lineHeight = text->mLineHeight.GetCoordValue();
  }
  else {
    // For "normal", factor or percentage units the computed value of
    // the line-height property is found by multiplying the factor by
    // the font's <b>actual</b> em height. For "normal" we use the
    // font's normal line height (em height + leading).
    nscoord emHeight = -1;
    nscoord normalLineHeight = -1;
    aRenderingContext->SetFont(font->mFont);
    nsCOMPtr<nsIFontMetrics> fm;
    aRenderingContext->GetFontMetrics(*getter_AddRefs(fm));
    if (fm) {
      fm->GetEmHeight(emHeight);
      fm->GetNormalLineHeight(normalLineHeight);
    }
    float factor;
    if (eStyleUnit_Factor == unit) {
      factor = text->mLineHeight.GetFactorValue();
    }
    else if (eStyleUnit_Percent == unit) {
      factor = text->mLineHeight.GetPercentValue();
    }
    else {
      factor = (((float) normalLineHeight) / PR_MAX(1, emHeight));
    }

    // Note: we normally use the actual font height for computing the
    // line-height raw value from the style context. On systems where
    // they disagree the actual font height is more appropriate. This
    // little hack lets us override that behavior to allow for more
    // precise layout in the face of imprecise fonts.
    if (nsHTMLReflowState::UseComputedHeight()) {
      emHeight = font->mFont.size;
    }

    lineHeight = NSToCoordRound(factor * emHeight);
  }

  return lineHeight;
}

nscoord
nsHTMLReflowState::CalcLineHeight(nsIPresContext* aPresContext,
                                  nsIRenderingContext* aRenderingContext,
                                  nsIFrame* aFrame)
{
  nscoord lineHeight = -1;
  nsCOMPtr<nsIStyleContext> sc;
  aFrame->GetStyleContext(getter_AddRefs(sc));
  if (sc) {
    lineHeight = ComputeLineHeight(aRenderingContext, sc);
  }
  if (lineHeight < 0) {
    // Negative line-heights are not allowed by the spec. Translate
    // them into "normal" when found.
    const nsStyleFont* font = (const nsStyleFont*)
      sc->GetStyleData(eStyleStruct_Font);
    if (UseComputedHeight()) {
      lineHeight = font->mFont.size;
    }
    else {
      aRenderingContext->SetFont(font->mFont);
      nsCOMPtr<nsIFontMetrics> fm;
      aRenderingContext->GetFontMetrics(*getter_AddRefs(fm));
      if (fm) {
        fm->GetNormalLineHeight(lineHeight);
      }
    }
  }
  return lineHeight;
}

void
nsHTMLReflowState::ComputeHorizontalValue(nscoord aContainingBlockWidth,
                                          nsStyleUnit aUnit,
                                          const nsStyleCoord& aCoord,
                                          nscoord& aResult)
{
  NS_PRECONDITION(eStyleUnit_Inherit != aUnit, "unexpected unit");
  aResult = 0;
  if (eStyleUnit_Percent == aUnit) {
    if (NS_UNCONSTRAINEDSIZE == aContainingBlockWidth) {
      aResult = 0;
    } else {
      float pct = aCoord.GetPercentValue();
      aResult = NSToCoordFloor(aContainingBlockWidth * pct);
    }
  
  } else if (eStyleUnit_Coord == aUnit) {
    aResult = aCoord.GetCoordValue();
  }
  else if (eStyleUnit_Chars == aUnit) {
    if ((nsnull == rendContext) || (nsnull == frame)) {
      // We can't compute it without a rendering context or frame, so
      // pretend its zero...
    }
    else {
      const nsStyleFont* font;
      frame->GetStyleData(eStyleStruct_Font, (const nsStyleStruct*&) font);
      rendContext->SetFont(font->mFont);
      nscoord fontWidth;
      rendContext->GetWidth('M', fontWidth);
      aResult = aCoord.GetIntValue() * fontWidth;
    }
  }
}

void
nsHTMLReflowState::ComputeVerticalValue(nscoord aContainingBlockHeight,
                                        nsStyleUnit aUnit,
                                        const nsStyleCoord& aCoord,
                                        nscoord& aResult)
{
  NS_PRECONDITION(eStyleUnit_Inherit != aUnit, "unexpected unit");
  aResult = 0;
  if (eStyleUnit_Percent == aUnit) {
    // Verify no one is trying to calculate a percentage based height against
    // a height that's shrink wrapping to its content. In that case they should
    // treat the specified value like 'auto'
    NS_ASSERTION(NS_AUTOHEIGHT != aContainingBlockHeight, "unexpected containing block height");
    float pct = aCoord.GetPercentValue();
    aResult = NSToCoordFloor(aContainingBlockHeight * pct);

  } else if (eStyleUnit_Coord == aUnit) {
    aResult = aCoord.GetCoordValue();
  }
}

void
nsHTMLReflowState::ComputeMargin(nscoord aContainingBlockWidth,
                                 const nsHTMLReflowState* aContainingBlockRS)
{
  // If style style can provide us the margin directly, then use it.
  if (!mStyleSpacing->GetMargin(mComputedMargin)) {
    // We have to compute the value
    if (NS_UNCONSTRAINEDSIZE == aContainingBlockWidth) {
      mComputedMargin.left = 0;
      mComputedMargin.right = 0;

      if (eStyleUnit_Coord == mStyleSpacing->mMargin.GetLeftUnit()) {
        nsStyleCoord left;
        
        mStyleSpacing->mMargin.GetLeft(left),
        mComputedMargin.left = left.GetCoordValue();
      }
      if (eStyleUnit_Coord == mStyleSpacing->mMargin.GetRightUnit()) {
        nsStyleCoord right;
        
        mStyleSpacing->mMargin.GetRight(right),
        mComputedMargin.right = right.GetCoordValue();
      }

    } else {
      nsStyleCoord left, right;

      if (eStyleUnit_Inherit == mStyleSpacing->mMargin.GetLeftUnit()) {
        mComputedMargin.left = aContainingBlockRS->mComputedMargin.left;
      } else {
        ComputeHorizontalValue(aContainingBlockWidth,
                               mStyleSpacing->mMargin.GetLeftUnit(),
                               mStyleSpacing->mMargin.GetLeft(left),
                               mComputedMargin.left);
      }
      if (eStyleUnit_Inherit == mStyleSpacing->mMargin.GetRightUnit()) {
        mComputedMargin.right = aContainingBlockRS->mComputedMargin.right;
      } else {
        ComputeHorizontalValue(aContainingBlockWidth,
                               mStyleSpacing->mMargin.GetRightUnit(),
                               mStyleSpacing->mMargin.GetRight(right),
                               mComputedMargin.right);
      }
    }

    const nsHTMLReflowState* rs2 = GetPageBoxReflowState(parentReflowState);
    nsStyleCoord top, bottom;
    if (nsnull != rs2) {
      // According to the CSS2 spec, margin percentages are
      // calculated with respect to the *height* of the containing
      // block when in a paginated context.
      if (eStyleUnit_Inherit == mStyleSpacing->mMargin.GetTopUnit()) {
        mComputedMargin.top = aContainingBlockRS->mComputedMargin.top;
      } else {
        ComputeVerticalValue(rs2->mComputedHeight,
                             mStyleSpacing->mMargin.GetTopUnit(),
                             mStyleSpacing->mMargin.GetTop(top),
                             mComputedMargin.top);
      }
      if (eStyleUnit_Inherit == mStyleSpacing->mMargin.GetBottomUnit()) {
        mComputedMargin.bottom = aContainingBlockRS->mComputedMargin.bottom;
      } else {
        ComputeVerticalValue(rs2->mComputedHeight,
                             mStyleSpacing->mMargin.GetBottomUnit(),
                             mStyleSpacing->mMargin.GetBottom(bottom),
                             mComputedMargin.bottom);
      }
    }
    else {
      // According to the CSS2 spec, margin percentages are
      // calculated with respect to the *width* of the containing
      // block, even for margin-top and margin-bottom.
      if (NS_UNCONSTRAINEDSIZE == aContainingBlockWidth) {
        mComputedMargin.top = 0;
        mComputedMargin.bottom = 0;

      } else {
        if (eStyleUnit_Inherit == mStyleSpacing->mMargin.GetTopUnit()) {
          mComputedMargin.top = aContainingBlockRS->mComputedMargin.top;
        } else {
          ComputeHorizontalValue(aContainingBlockWidth,
                                 mStyleSpacing->mMargin.GetTopUnit(),
                                 mStyleSpacing->mMargin.GetTop(top),
                                 mComputedMargin.top);
        }
        if (eStyleUnit_Inherit == mStyleSpacing->mMargin.GetBottomUnit()) {
          mComputedMargin.bottom = aContainingBlockRS->mComputedMargin.bottom;
        } else {
          ComputeHorizontalValue(aContainingBlockWidth,
                                 mStyleSpacing->mMargin.GetBottomUnit(),
                                 mStyleSpacing->mMargin.GetBottom(bottom),
                                 mComputedMargin.bottom);
        }
      }
    }
  }
}

void
nsHTMLReflowState::ComputePadding(nscoord aContainingBlockWidth,
                                  const nsHTMLReflowState* aContainingBlockRS)

{
  // If style can provide us the padding directly, then use it.
  if (!mStyleSpacing->GetPadding(mComputedPadding)) {
    // We have to compute the value
    nsStyleCoord left, right, top, bottom;

    if (eStyleUnit_Inherit == mStyleSpacing->mPadding.GetLeftUnit()) {
      mComputedPadding.left = aContainingBlockRS->mComputedPadding.left;
    } else {
      ComputeHorizontalValue(aContainingBlockWidth,
                             mStyleSpacing->mPadding.GetLeftUnit(),
                             mStyleSpacing->mPadding.GetLeft(left),
                             mComputedPadding.left);
    }
    if (eStyleUnit_Inherit == mStyleSpacing->mPadding.GetRightUnit()) {
      mComputedPadding.right = aContainingBlockRS->mComputedPadding.right;
    } else {
      ComputeHorizontalValue(aContainingBlockWidth,
                             mStyleSpacing->mPadding.GetRightUnit(),
                             mStyleSpacing->mPadding.GetRight(right),
                             mComputedPadding.right);
    }

    // According to the CSS2 spec, percentages are calculated with respect to
    // containing block width for padding-top and padding-bottom
    if (eStyleUnit_Inherit == mStyleSpacing->mPadding.GetTopUnit()) {
      mComputedPadding.top = aContainingBlockRS->mComputedPadding.top;
    } else {
      ComputeHorizontalValue(aContainingBlockWidth,
                             mStyleSpacing->mPadding.GetTopUnit(),
                             mStyleSpacing->mPadding.GetTop(top),
                             mComputedPadding.top);
    }
    if (eStyleUnit_Inherit == mStyleSpacing->mPadding.GetBottomUnit()) {
      mComputedPadding.bottom = aContainingBlockRS->mComputedPadding.bottom;
    } else {
      ComputeHorizontalValue(aContainingBlockWidth,
                             mStyleSpacing->mPadding.GetBottomUnit(),
                             mStyleSpacing->mPadding.GetBottom(bottom),
                             mComputedPadding.bottom);
    }
  }
}

void
nsHTMLReflowState::ComputeMinMaxValues(nscoord aContainingBlockWidth,
                                       nscoord aContainingBlockHeight,
                                       const nsHTMLReflowState* aContainingBlockRS)
{
  nsStyleUnit minWidthUnit = mStylePosition->mMinWidth.GetUnit();
  if (eStyleUnit_Inherit == minWidthUnit) {
    mComputedMinWidth = aContainingBlockRS->mComputedMinWidth;
  } else {
    ComputeHorizontalValue(aContainingBlockWidth, minWidthUnit,
                           mStylePosition->mMinWidth, mComputedMinWidth);
  }
  nsStyleUnit maxWidthUnit = mStylePosition->mMaxWidth.GetUnit();
  if (eStyleUnit_Inherit == maxWidthUnit) {
    mComputedMaxWidth = aContainingBlockRS->mComputedMaxWidth;
  } else if (eStyleUnit_Null == maxWidthUnit) {
    // Specified value of 'none'
    mComputedMaxWidth = NS_UNCONSTRAINEDSIZE;  // no limit
  } else {
    ComputeHorizontalValue(aContainingBlockWidth, maxWidthUnit,
                           mStylePosition->mMaxWidth, mComputedMaxWidth);
  }

  // If the computed value of 'min-width' is greater than the value of
  // 'max-width', 'max-width' is set to the value of 'min-width'
  if (mComputedMinWidth > mComputedMaxWidth) {
    mComputedMaxWidth = mComputedMinWidth;
  }

  nsStyleUnit minHeightUnit = mStylePosition->mMinHeight.GetUnit();
  if (eStyleUnit_Inherit == minHeightUnit) {
    mComputedMinHeight = aContainingBlockRS->mComputedMinHeight;
  } else {
    // Check for percentage based values and a containing block height that
    // depends on the content height. Treat them like 'auto'
    if ((NS_AUTOHEIGHT == aContainingBlockHeight) &&
        (eStyleUnit_Percent == minHeightUnit)) {
      mComputedMinHeight = 0;
    } else {
      ComputeVerticalValue(aContainingBlockHeight, minHeightUnit,
                           mStylePosition->mMinHeight, mComputedMinHeight);
    }
  }
  nsStyleUnit maxHeightUnit = mStylePosition->mMaxHeight.GetUnit();
  if (eStyleUnit_Inherit == maxHeightUnit) {
    mComputedMaxHeight = aContainingBlockRS->mComputedMaxHeight;
  } else if (eStyleUnit_Null == maxHeightUnit) {
    // Specified value of 'none'
    mComputedMaxHeight = NS_UNCONSTRAINEDSIZE;  // no limit
  } else {
    // Check for percentage based values and a containing block height that
    // depends on the content height. Treat them like 'auto'
    if ((NS_AUTOHEIGHT == aContainingBlockHeight) && 
        (eStyleUnit_Percent == maxHeightUnit)) {
      mComputedMaxHeight = NS_UNCONSTRAINEDSIZE;
    } else {
      ComputeVerticalValue(aContainingBlockHeight, maxHeightUnit,
                           mStylePosition->mMaxHeight, mComputedMaxHeight);
    }
  }

  // If the computed value of 'min-height' is greater than the value of
  // 'max-height', 'max-height' is set to the value of 'min-height'
  if (mComputedMinHeight > mComputedMaxHeight) {
    mComputedMaxHeight = mComputedMinHeight;
  }
}
