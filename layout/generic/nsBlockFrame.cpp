/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
// vim:cindent:ts=2:et:sw=2:
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Steve Clark <buster@netscape.com>
 *   Robert O'Callahan <roc+moz@cs.cmu.edu>
 *   L. David Baron <dbaron@dbaron.org>
 *   IBM Corporation
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
#include "nsCOMPtr.h"
#include "nsBlockFrame.h"
#include "nsBlockReflowContext.h"
#include "nsBlockReflowState.h"
#include "nsBlockBandData.h"
#include "nsBulletFrame.h"
#include "nsLineBox.h"
#include "nsInlineFrame.h"
#include "nsLineLayout.h"
#include "nsPlaceholderFrame.h"
#include "nsStyleConsts.h"
#include "nsFrameManager.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsReflowPath.h"
#include "nsStyleContext.h"
#include "nsIView.h"
#include "nsIFontMetrics.h"
#include "nsHTMLParts.h"
#include "nsHTMLAtoms.h"
#include "nsHTMLValue.h"
#include "nsIDOMEvent.h"
#include "nsIHTMLContent.h"
#include "prprf.h"
#include "nsLayoutAtoms.h"
#include "nsITextContent.h"
#include "nsStyleChangeList.h"
#include "nsIFrameSelection.h"
#include "nsSpaceManager.h"
#include "nsIntervalSet.h"
#include "prenv.h"
#include "plstr.h"
#include "nsGUIEvent.h"
#include "nsLayoutErrors.h"
#include "nsAutoPtr.h"
#include "nsIServiceManager.h"
#ifdef ACCESSIBILITY
#include "nsIAccessibilityService.h"
#endif
#include "nsLayoutUtils.h"

#ifdef IBMBIDI
#include "nsBidiPresUtils.h"
#endif // IBMBIDI

#include "nsIDOMHTMLBodyElement.h"
#include "nsIDOMHTMLHtmlElement.h"

static const int MIN_LINES_NEEDING_CURSOR = 20;

#ifdef DEBUG
#include "nsPrintfCString.h"
#include "nsBlockDebugFlags.h"

PRBool nsBlockFrame::gLamePaintMetrics;
PRBool nsBlockFrame::gLameReflowMetrics;
PRBool nsBlockFrame::gNoisy;
PRBool nsBlockFrame::gNoisyDamageRepair;
PRBool nsBlockFrame::gNoisyMaxElementWidth;
PRBool nsBlockFrame::gNoisyReflow;
PRBool nsBlockFrame::gReallyNoisyReflow;
PRBool nsBlockFrame::gNoisySpaceManager;
PRBool nsBlockFrame::gVerifyLines;
PRBool nsBlockFrame::gDisableResizeOpt;

PRInt32 nsBlockFrame::gNoiseIndent;

struct BlockDebugFlags {
  const char* name;
  PRBool* on;
};

static const BlockDebugFlags gFlags[] = {
  { "reflow", &nsBlockFrame::gNoisyReflow },
  { "really-noisy-reflow", &nsBlockFrame::gReallyNoisyReflow },
  { "max-element-width", &nsBlockFrame::gNoisyMaxElementWidth },
  { "space-manager", &nsBlockFrame::gNoisySpaceManager },
  { "verify-lines", &nsBlockFrame::gVerifyLines },
  { "damage-repair", &nsBlockFrame::gNoisyDamageRepair },
  { "lame-paint-metrics", &nsBlockFrame::gLamePaintMetrics },
  { "lame-reflow-metrics", &nsBlockFrame::gLameReflowMetrics },
  { "disable-resize-opt", &nsBlockFrame::gDisableResizeOpt },
};
#define NUM_DEBUG_FLAGS (sizeof(gFlags) / sizeof(gFlags[0]))

static void
ShowDebugFlags()
{
  printf("Here are the available GECKO_BLOCK_DEBUG_FLAGS:\n");
  const BlockDebugFlags* bdf = gFlags;
  const BlockDebugFlags* end = gFlags + NUM_DEBUG_FLAGS;
  for (; bdf < end; bdf++) {
    printf("  %s\n", bdf->name);
  }
  printf("Note: GECKO_BLOCK_DEBUG_FLAGS is a comma separated list of flag\n");
  printf("names (no whitespace)\n");
}

void
nsBlockFrame::InitDebugFlags()
{
  static PRBool firstTime = PR_TRUE;
  if (firstTime) {
    firstTime = PR_FALSE;
    char* flags = PR_GetEnv("GECKO_BLOCK_DEBUG_FLAGS");
    if (flags) {
      PRBool error = PR_FALSE;
      for (;;) {
        char* cm = PL_strchr(flags, ',');
        if (cm) *cm = '\0';

        PRBool found = PR_FALSE;
        const BlockDebugFlags* bdf = gFlags;
        const BlockDebugFlags* end = gFlags + NUM_DEBUG_FLAGS;
        for (; bdf < end; bdf++) {
          if (PL_strcasecmp(bdf->name, flags) == 0) {
            *(bdf->on) = PR_TRUE;
            printf("nsBlockFrame: setting %s debug flag on\n", bdf->name);
            gNoisy = PR_TRUE;
            found = PR_TRUE;
            break;
          }
        }
        if (!found) {
          error = PR_TRUE;
        }

        if (!cm) break;
        *cm = ',';
        flags = cm + 1;
      }
      if (error) {
        ShowDebugFlags();
      }
    }
  }
}

#endif

// add in a sanity check for absurdly deep frame trees.  See bug 42138
// can't just use IsFrameTreeTooDeep() because that method has side effects we don't want
#define MAX_DEPTH_FOR_LIST_RENUMBERING 200  // 200 open displayable tags is pretty unrealistic

//----------------------------------------------------------------------

// Debugging support code

#ifdef DEBUG
const char* nsBlockFrame::kReflowCommandType[] = {
  "ContentChanged",
  "StyleChanged",
  "ReflowDirty",
  "Timeout",
  "UserDefined",
};
#endif

#ifdef REALLY_NOISY_FIRST_LINE
static void
DumpStyleGeneaology(nsIFrame* aFrame, const char* gap)
{
  fputs(gap, stdout);
  nsFrame::ListTag(stdout, aFrame);
  printf(": ");
  nsStyleContext* sc = aFrame->GetStyleContext();
  while (nsnull != sc) {
    nsStyleContext* psc;
    printf("%p ", sc);
    psc = sc->GetParent();
    sc = psc;
  }
  printf("\n");
}
#endif

#ifdef REFLOW_STATUS_COVERAGE
static void
RecordReflowStatus(PRBool aChildIsBlock, nsReflowStatus aFrameReflowStatus)
{
  static PRUint32 record[2];

  // 0: child-is-block
  // 1: child-is-inline
  PRIntn index = 0;
  if (!aChildIsBlock) index |= 1;

  // Compute new status
  PRUint32 newS = record[index];
  if (NS_INLINE_IS_BREAK(aFrameReflowStatus)) {
    if (NS_INLINE_IS_BREAK_BEFORE(aFrameReflowStatus)) {
      newS |= 1;
    }
    else if (NS_FRAME_IS_NOT_COMPLETE(aFrameReflowStatus)) {
      newS |= 2;
    }
    else {
      newS |= 4;
    }
  }
  else if (NS_FRAME_IS_NOT_COMPLETE(aFrameReflowStatus)) {
    newS |= 8;
  }
  else {
    newS |= 16;
  }

  // Log updates to the status that yield different values
  if (record[index] != newS) {
    record[index] = newS;
    printf("record(%d): %02x %02x\n", index, record[0], record[1]);
  }
}
#endif

//----------------------------------------------------------------------

const nsIID kBlockFrameCID = NS_BLOCK_FRAME_CID;

nsresult
NS_NewBlockFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame, PRUint32 aFlags)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsBlockFrame* it = new (aPresShell) nsBlockFrame;
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  it->SetFlags(aFlags);
  *aNewFrame = it;
  return NS_OK;
}

nsBlockFrame::nsBlockFrame() 
{
#ifdef DEBUG
  InitDebugFlags();
#endif
}

nsBlockFrame::~nsBlockFrame()
{
}

NS_IMETHODIMP
nsBlockFrame::Destroy(nsPresContext* aPresContext)
{
  mAbsoluteContainer.DestroyFrames(this, aPresContext);
  // Outside bullets are not in our child-list so check for them here
  // and delete them when present.
  if (mBullet && HaveOutsideBullet()) {
    mBullet->Destroy(aPresContext);
    mBullet = nsnull;
  }

  mFloats.DestroyFrames(aPresContext);

  nsLineBox::DeleteLineList(aPresContext, mLines);

  // destroy overflow lines now
  nsLineList* overflowLines = RemoveOverflowLines();
  if (overflowLines) {
    nsLineBox::DeleteLineList(aPresContext, *overflowLines);
  }
  nsFrameList* overflowOutOfFlows = RemoveOverflowOutOfFlows();
  if (overflowOutOfFlows) {
    overflowOutOfFlows->DestroyFrames(aPresContext);
    delete overflowOutOfFlows;
  }

  return nsBlockFrameSuper::Destroy(aPresContext);
}

NS_IMETHODIMP
nsBlockFrame::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  NS_PRECONDITION(aInstancePtr, "null out param");
  if (aIID.Equals(kBlockFrameCID)) {
    *aInstancePtr = NS_STATIC_CAST(void*, NS_STATIC_CAST(nsBlockFrame*, this));
    return NS_OK;
  }
  if (aIID.Equals(NS_GET_IID(nsILineIterator)) ||
      aIID.Equals(NS_GET_IID(nsILineIteratorNavigator)))
  {
    nsLineIterator* it = new nsLineIterator;
    if (!it) {
      *aInstancePtr = nsnull;
      return NS_ERROR_OUT_OF_MEMORY;
    }
    NS_ADDREF(it); // reference passed to caller
    const nsStyleVisibility* visibility = GetStyleVisibility();
    nsresult rv = it->Init(mLines,
                           visibility->mDirection == NS_STYLE_DIRECTION_RTL);
    if (NS_FAILED(rv)) {
      NS_RELEASE(it);
      return rv;
    }
    *aInstancePtr = NS_STATIC_CAST(void*,
            NS_STATIC_CAST(nsILineIteratorNavigator*, it));
    return NS_OK;
  }
  return nsBlockFrameSuper::QueryInterface(aIID, aInstancePtr);
}

NS_IMETHODIMP
nsBlockFrame::IsSplittable(nsSplittableType& aIsSplittable) const
{
  aIsSplittable = NS_FRAME_SPLITTABLE_NON_RECTANGULAR;
  return NS_OK;
}

#ifdef DEBUG
NS_METHOD
nsBlockFrame::List(nsPresContext* aPresContext, FILE* out, PRInt32 aIndent) const
{
  IndentBy(out, aIndent);
  ListTag(out);
#ifdef DEBUG_waterson
  fprintf(out, " [parent=%p]", mParent);
#endif
  if (HasView()) {
    fprintf(out, " [view=%p]", NS_STATIC_CAST(void*, GetView()));
  }
  if (nsnull != mNextSibling) {
    fprintf(out, " next=%p", NS_STATIC_CAST(void*, mNextSibling));
  }

  // Output the flow linkage
  if (nsnull != mPrevInFlow) {
    fprintf(out, " prev-in-flow=%p", NS_STATIC_CAST(void*, mPrevInFlow));
  }
  if (nsnull != mNextInFlow) {
    fprintf(out, " next-in-flow=%p", NS_STATIC_CAST(void*, mNextInFlow));
  }

  // Output the rect and state
  fprintf(out, " {%d,%d,%d,%d}", mRect.x, mRect.y, mRect.width, mRect.height);
  if (0 != mState) {
    fprintf(out, " [state=%08x]", mState);
  }
  nsBlockFrame* f = NS_CONST_CAST(nsBlockFrame*, this);
  nsRect* overflowArea = f->GetOverflowAreaProperty(PR_FALSE);
  if (overflowArea) {
    fprintf(out, " [overflow=%d,%d,%d,%d]", overflowArea->x, overflowArea->y,
            overflowArea->width, overflowArea->height);
  }
  PRInt32 numInlineLines = 0;
  PRInt32 numBlockLines = 0;
  if (! mLines.empty()) {
    for (const_line_iterator line = begin_lines(), line_end = end_lines();
         line != line_end;
         ++line)
    {
      if (line->IsBlock())
        numBlockLines++;
      else
        numInlineLines++;
    }
  }
  fprintf(out, " sc=%p(i=%d,b=%d)",
          NS_STATIC_CAST(void*, mStyleContext), numInlineLines, numBlockLines);
  nsIAtom* pseudoTag = mStyleContext->GetPseudoType();
  if (pseudoTag) {
    nsAutoString atomString;
    pseudoTag->ToString(atomString);
    fprintf(out, " pst=%s",
            NS_LossyConvertUCS2toASCII(atomString).get());
  }
  fputs("<\n", out);

  aIndent++;

  // Output the lines
  if (! mLines.empty()) {
    for (const_line_iterator line = begin_lines(), line_end = end_lines();
         line != line_end;
         ++line)
    {
      line->List(aPresContext, out, aIndent);
    }
  }

  nsIAtom* listName = nsnull;
  PRInt32 listIndex = 0;
  for (;;) {
    listName = GetAdditionalChildListName(listIndex++);
    if (nsnull == listName) {
      break;
    }
    nsIFrame* kid = GetFirstChild(listName);
    if (kid) {
      IndentBy(out, aIndent);
      nsAutoString tmp;
      if (nsnull != listName) {
        listName->ToString(tmp);
        fputs(NS_LossyConvertUCS2toASCII(tmp).get(), out);
      }
      fputs("<\n", out);
      while (kid) {
        nsIFrameDebug*  frameDebug;

        if (NS_SUCCEEDED(CallQueryInterface(kid, &frameDebug))) {
          frameDebug->List(aPresContext, out, aIndent + 1);
        }
        kid = kid->GetNextSibling();
      }
      IndentBy(out, aIndent);
      fputs(">\n", out);
    }
  }

  aIndent--;
  IndentBy(out, aIndent);
  fputs(">\n", out);

  return NS_OK;
}

NS_IMETHODIMP_(nsFrameState)
nsBlockFrame::GetDebugStateBits() const
{
  // We don't want to include our cursor flag in the bits the
  // regression tester looks at
  return nsBlockFrameSuper::GetDebugStateBits() & ~NS_BLOCK_HAS_LINE_CURSOR;
}

NS_IMETHODIMP
nsBlockFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("Block"), aResult);
}
#endif

nsIAtom*
nsBlockFrame::GetType() const
{
  return nsLayoutAtoms::blockFrame;
}

/////////////////////////////////////////////////////////////////////////////
// Child frame enumeration

nsIFrame*
nsBlockFrame::GetFirstChild(nsIAtom* aListName) const
{
  if (mAbsoluteContainer.GetChildListName() == aListName) {
    nsIFrame* result = nsnull;
    mAbsoluteContainer.FirstChild(this, aListName, &result);
    return result;
  }
  else if (nsnull == aListName) {
    return (mLines.empty()) ? nsnull : mLines.front()->mFirstChild;
  }
  else if (aListName == nsLayoutAtoms::overflowList) {
    nsLineList* overflowLines = GetOverflowLines();
    return overflowLines ? overflowLines->front()->mFirstChild : nsnull;
  }
  else if (aListName == nsLayoutAtoms::overflowOutOfFlowList) {
    nsFrameList* oof = GetOverflowOutOfFlows();
    return oof ? oof->FirstChild() : nsnull;
  }
  else if (aListName == nsLayoutAtoms::floatList) {
    return mFloats.FirstChild();
  }
  else if (aListName == nsLayoutAtoms::bulletList) {
    if (HaveOutsideBullet()) {
      return mBullet;
    }
  }
  return nsnull;
}

nsIAtom*
nsBlockFrame::GetAdditionalChildListName(PRInt32 aIndex) const
{
  switch (aIndex) {
  case NS_BLOCK_FRAME_FLOAT_LIST_INDEX:
    return nsLayoutAtoms::floatList;
  case NS_BLOCK_FRAME_BULLET_LIST_INDEX:
    return nsLayoutAtoms::bulletList;
  case NS_BLOCK_FRAME_OVERFLOW_LIST_INDEX:
    return nsLayoutAtoms::overflowList;
  case NS_BLOCK_FRAME_OVERFLOW_OOF_LIST_INDEX:
    return nsLayoutAtoms::overflowOutOfFlowList;
  case NS_BLOCK_FRAME_ABSOLUTE_LIST_INDEX:
    return mAbsoluteContainer.GetChildListName();
  default:
    return nsnull;
  }
}

/* virtual */ PRBool
nsBlockFrame::IsContainingBlock() const
{
  return PR_TRUE;
}

//////////////////////////////////////////////////////////////////////
// Frame structure methods

//////////////////////////////////////////////////////////////////////
// Reflow methods

static void
CalculateContainingBlock(const nsHTMLReflowState& aReflowState,
                         nscoord                  aFrameWidth,
                         nscoord                  aFrameHeight,
                         nscoord&                 aContainingBlockWidth,
                         nscoord&                 aContainingBlockHeight)
{
  aContainingBlockWidth = -1;  // have reflow state calculate
  aContainingBlockHeight = -1; // have reflow state calculate

  // The issue there is that for a 'height' of 'auto' the reflow state code
  // won't know how to calculate the containing block height because it's
  // calculated bottom up. We don't really want to do this for the initial
  // containing block so that's why we have the check for if the element
  // is absolutely or relatively positioned
  if (aReflowState.mStyleDisplay->IsAbsolutelyPositioned() ||
      (NS_STYLE_POSITION_RELATIVE == aReflowState.mStyleDisplay->mPosition)) {
    aContainingBlockWidth = aFrameWidth;
    aContainingBlockHeight = aFrameHeight;

    // Containing block is relative to the padding edge
    nsMargin  border;
    if (!aReflowState.mStyleBorder->GetBorder(border)) {
      NS_NOTYETIMPLEMENTED("percentage border");
    }
    aContainingBlockWidth -= border.left + border.right;
    aContainingBlockHeight -= border.top + border.bottom;
  }
}

NS_IMETHODIMP
nsBlockFrame::Reflow(nsPresContext*          aPresContext,
                     nsHTMLReflowMetrics&     aMetrics,
                     const nsHTMLReflowState& aReflowState,
                     nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsBlockFrame", aReflowState.reason);
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aMetrics, aStatus);
#ifdef DEBUG
  if (gNoisyReflow) {
    nsCAutoString reflow;
    reflow.Append(nsHTMLReflowState::ReasonToString(aReflowState.reason));

    if (aReflowState.reason == eReflowReason_Incremental) {
      nsHTMLReflowCommand *command = aReflowState.path->mReflowCommand;

      if (command) {
        // We're the target.
        reflow += " (";

        nsReflowType type;
        command->GetType(type);
        reflow += kReflowCommandType[type];

        reflow += ")";
      }
    }

    IndentBy(stdout, gNoiseIndent);
    ListTag(stdout);
    printf(": begin %s reflow availSize=%d,%d computedSize=%d,%d\n",
           reflow.get(),
           aReflowState.availableWidth, aReflowState.availableHeight,
           aReflowState.mComputedWidth, aReflowState.mComputedHeight);
  }
  if (gNoisy) {
    gNoiseIndent++;
  }
  PRTime start = LL_ZERO; // Initialize these variablies to silence the compiler.
  PRInt32 ctc = 0;        // We only use these if they are set (gLameReflowMetrics).
  if (gLameReflowMetrics) {
    start = PR_Now();
    ctc = nsLineBox::GetCtorCount();
  }
#endif

  nsSize oldSize = GetSize();

  // Should we create a space manager?
  nsAutoSpaceManager autoSpaceManager(NS_CONST_CAST(nsHTMLReflowState &, aReflowState));

  // XXXldb If we start storing the space manager in the frame rather
  // than keeping it around only during reflow then we should create it
  // only when there are actually floats to manage.  Otherwise things
  // like tables will gain significant bloat.
  if (NS_BLOCK_SPACE_MGR & mState)
    autoSpaceManager.CreateSpaceManagerFor(aPresContext, this);

  // See if it's an incremental reflow command targeted only at
  // absolute frames. If so we can skip a whole lot of work via this
  // fast path.
  if (mAbsoluteContainer.HasAbsoluteFrames() &&
      eReflowReason_Incremental == aReflowState.reason &&
      !aMetrics.mComputeMEW &&
      mAbsoluteContainer.ReflowingAbsolutesOnly(this, aReflowState)) {
    nscoord containingBlockWidth;
    nscoord containingBlockHeight;

    CalculateContainingBlock(aReflowState, mRect.width, mRect.height,
                             containingBlockWidth, containingBlockHeight);
    
    mAbsoluteContainer.IncrementalReflow(this, aPresContext, aReflowState,
                                         containingBlockWidth,
                                         containingBlockHeight);

    // Just return our current size as our desired size.
    aMetrics.width = mRect.width;
    aMetrics.height = mRect.height;
    aMetrics.ascent = mAscent;
    aMetrics.descent = aMetrics.height - aMetrics.ascent;
    
    // Whether or not we're complete hasn't changed
    aStatus = (nsnull != mNextInFlow) ? NS_FRAME_NOT_COMPLETE : NS_FRAME_COMPLETE;
    
    // Factor the absolutely positioned child bounds into the overflow area
    ComputeCombinedArea(aReflowState, aMetrics);
    nsRect childBounds;
    mAbsoluteContainer.CalculateChildBounds(aPresContext, childBounds);
    aMetrics.mOverflowArea.UnionRect(aMetrics.mOverflowArea, childBounds);
    
    FinishAndStoreOverflow(&aMetrics);

#ifdef DEBUG
    if (gNoisy) {
      gNoiseIndent--;
    }
#endif
    
    return NS_OK;
  }

  // OK, some lines may be reflowed. Blow away any saved line cursor because
  // we may invalidate the nondecreasing combinedArea.y/yMost invariant,
  // and we may even delete the line with the line cursor.
  ClearLineCursor();

  if (IsFrameTreeTooDeep(aReflowState, aMetrics)) {
#ifdef DEBUG_kipp
    {
      extern char* nsPresShell_ReflowStackPointerTop;
      char marker;
      char* newsp = (char*) &marker;
      printf("XXX: frame tree is too deep; approx stack size = %d\n",
             nsPresShell_ReflowStackPointerTop - newsp);
    }
#endif
    aStatus = NS_FRAME_COMPLETE;
    return NS_OK;
  }

  nsBlockReflowState state(aReflowState, aPresContext, this, aMetrics,
                           aReflowState.mFlags.mHasClearance || (NS_BLOCK_MARGIN_ROOT & mState),
                           (NS_BLOCK_MARGIN_ROOT & mState));

  // The condition for doing Bidi resolutions includes a test for the
  // dirtiness flags, because blocks sometimes send a resize reflow
  // even though they have dirty children, An example where this can
  // occur is when adding lines to a text control (bugs 95228 and 95400
  // were caused by not doing Bidi resolution in these cases)
  if (eReflowReason_Resize != aReflowState.reason ||
      mState & NS_FRAME_IS_DIRTY || mState & NS_FRAME_HAS_DIRTY_CHILDREN) {
#ifdef IBMBIDI
    if (! mLines.empty()) {
      if (aPresContext->BidiEnabled()) {
        nsBidiPresUtils* bidiUtils = aPresContext->GetBidiUtils();
        if (bidiUtils) {
          PRBool forceReflow;
          nsresult rc = bidiUtils->Resolve(aPresContext, this,
                                           mLines.front()->mFirstChild,
                                           forceReflow,
                                           aReflowState.mFlags.mVisualBidiFormControl);
          if (NS_SUCCEEDED(rc) && forceReflow) {
            // Mark everything dirty
            // XXXldb This should be done right.
            for (line_iterator line = begin_lines(), line_end = end_lines();
                 line != line_end;
                 ++line)
            {
              line->MarkDirty();
            }
          }
        }
      }
    }
#endif // IBMBIDI
    RenumberLists(aPresContext);
  }

  nsresult rv = NS_OK;

  // ALWAYS drain overflow. We never want to leave the previnflow's
  // overflow lines hanging around; block reflow depends on the
  // overflow line lists being cleared out between reflow passes.
  DrainOverflowLines();

  switch (aReflowState.reason) {
  case eReflowReason_Initial:
#ifdef NOISY_REFLOW_REASON
    ListTag(stdout);
    printf(": reflow=initial\n");
#endif
    rv = PrepareInitialReflow(state);
    mState &= ~NS_FRAME_FIRST_REFLOW;
    break;  

  case eReflowReason_Dirty:
    // Do nothing; the dirty lines will already have been marked.
    break;

  case eReflowReason_Incremental: {
#ifdef NOISY_REFLOW_REASON
    ListTag(stdout);
    printf(": reflow=incremental ");
#endif
    nsReflowPath *path = aReflowState.path;
    nsHTMLReflowCommand *command = path->mReflowCommand;
    if (command) {
      nsReflowType type;
      command->GetType(type);
#ifdef NOISY_REFLOW_REASON
      printf("type=%s ", kReflowCommandType[type]);
#endif
      switch (type) {
      case eReflowType_StyleChanged:
        rv = PrepareStyleChangedReflow(state);
        break;
      case eReflowType_ReflowDirty:
        // Do nothing; the dirty lines will already have been marked.
        break;
      case eReflowType_ContentChanged:
        // Perform a full reflow.
        rv = PrepareResizeReflow(state);
        break;
      default:
        // We shouldn't get here. But, if we do, perform a full reflow.
        NS_ERROR("unexpected reflow type");
        rv = PrepareResizeReflow(state);
        break;
      }
    }

    if (path->FirstChild() != path->EndChildren()) {
      // We're along the reflow path, but not necessarily the target
      // of the reflow.
#ifdef NOISY_REFLOW_REASON
      ListTag(stdout);
      printf(" next={ ");

      for (nsReflowPath::iterator iter = path->FirstChild();
           iter != path->EndChildren();
           ++iter) {
        nsFrame::ListTag(stdout, *iter);
        printf(" ");
      }

      printf("}");
#endif

      rv = PrepareChildIncrementalReflow(state);
    }

#ifdef NOISY_REFLOW_REASON
    printf("\n");
#endif

    break;
  }

  case eReflowReason_StyleChange:
    rv = PrepareStyleChangedReflow(state);
    break;

  case eReflowReason_Resize:
  default:
#ifdef NOISY_REFLOW_REASON
    ListTag(stdout);
    printf(": reflow=resize (%d)\n", aReflowState.reason);
#endif
    rv = PrepareResizeReflow(state);
    break;
  }

  NS_ASSERTION(NS_SUCCEEDED(rv), "setting up reflow failed");
  if (NS_FAILED(rv)) return rv;

  // Now reflow...
  rv = ReflowDirtyLines(state);
  NS_ASSERTION(NS_SUCCEEDED(rv), "reflow dirty lines failed");
  if (NS_FAILED(rv)) return rv;

  // If the block is complete, put continuted floats in the closest ancestor 
  // block that uses the same space manager and leave the block complete; this 
  // allows subsequent lines on the page to be impacted by floats. If the 
  // block is incomplete or there is no ancestor using the same space manager, 
  // put continued floats at the beginning of the first overflow line.
  nsFrameList* overflowPlace = nsnull;
  if ((NS_UNCONSTRAINEDSIZE != aReflowState.availableHeight) && 
      (overflowPlace = RemoveOverflowPlaceholders())) {
    PRBool gaveToAncestor = PR_FALSE;
    if (NS_FRAME_IS_COMPLETE(state.mReflowStatus)) {
      // find the nearest block ancestor that uses the same space manager
      for (const nsHTMLReflowState* ancestorRS = aReflowState.parentReflowState; 
           ancestorRS; 
           ancestorRS = ancestorRS->parentReflowState) {
        nsIFrame* ancestor = ancestorRS->frame;
        nsIAtom* fType = ancestor->GetType();
        if ((nsLayoutAtoms::blockFrame == fType) || (nsLayoutAtoms::areaFrame == fType)) {
          if (aReflowState.mSpaceManager == ancestorRS->mSpaceManager) {
            // Put the continued floats in ancestor since it uses the same space manager
            nsFrameList* ancestorPlace =
              ((nsBlockFrame*)ancestor)->GetOverflowPlaceholders();
            if (ancestorPlace) {
              ancestorPlace->AppendFrames(ancestor, overflowPlace->FirstChild());
            }
            else {
              // ancestor doesn't have an overflow place holder list, so
              // create one. Note that we use AppendFrames() to add the
              // frames, instead of passing them into the constructor, so
              // we can levarage the code in AppendFrames() which updates
              // the parent for each frame in the list.

              ancestorPlace = new nsFrameList();
              if (ancestorPlace) {
                ancestorPlace->AppendFrames(ancestor, overflowPlace->FirstChild());
                ((nsBlockFrame*)ancestor)->SetOverflowPlaceholders(ancestorPlace);
              }
              else 
                return NS_ERROR_OUT_OF_MEMORY;
            }
            gaveToAncestor = PR_TRUE;
            break;
          }
        }
      }
    }
    if (!gaveToAncestor) {
      PRInt32 numOverflowPlace = overflowPlace->GetLength();
      nsLineList* overflowLines = GetOverflowLines();
      if (overflowLines) {
        nsPlaceholderFrame* lastPlaceholder = 
          (nsPlaceholderFrame*)overflowPlace->LastChild();
        line_iterator firstLine = overflowLines->begin();
        nsIFrame* firstFrame = firstLine->mFirstChild;

        // hook up the last placeholder with the existing overflow frames
        NS_ASSERTION(firstFrame != lastPlaceholder, "trying to set next sibling to self");
        lastPlaceholder->SetNextSibling(firstFrame);
        if (firstLine->IsBlock()) { 
          // Create a new line as the first line and put the floats there;
          nsLineBox* newLine = state.NewLineBox(overflowPlace->FirstChild(), numOverflowPlace, PR_FALSE);
          overflowLines->push_front(newLine);
        }
        else { // floats go on 1st overflow line
          firstLine->mFirstChild = overflowPlace->FirstChild();
          firstLine->SetChildCount(firstLine->GetChildCount() + numOverflowPlace);
        }
      }
      else {
        // Create a line, put the floats in it, and then push.
        nsLineBox* newLine = state.NewLineBox(overflowPlace->FirstChild(), numOverflowPlace, PR_FALSE);
        if (!newLine) 
          return NS_ERROR_OUT_OF_MEMORY;
        mLines.push_back(newLine);
        nsLineList::iterator nextToLastLine = ----end_lines();
        PushLines(state, nextToLastLine);
      }
      state.mReflowStatus |= NS_FRAME_NOT_COMPLETE | NS_FRAME_REFLOW_NEXTINFLOW;
    }

    delete overflowPlace;
  }

  if (NS_FRAME_IS_NOT_COMPLETE(state.mReflowStatus)) {
    if (GetOverflowLines()) {
      state.mReflowStatus |= NS_FRAME_REFLOW_NEXTINFLOW;
    }

    if (NS_STYLE_OVERFLOW_CLIP == aReflowState.mStyleDisplay->mOverflowX) {
      state.mReflowStatus = NS_FRAME_COMPLETE;
    }
    else {
#ifdef DEBUG_kipp
      ListTag(stdout); printf(": block is not complete\n");
#endif
    }
  }

  // XXX_perf get rid of this!  This is one of the things that makes
  // incremental reflow O(N^2).
  BuildFloatList(state);

  // Compute our final size
  ComputeFinalSize(aReflowState, state, aMetrics);
  nsRect currentOverflow = aMetrics.mOverflowArea;
  FinishAndStoreOverflow(&aMetrics);

  // see if verifyReflow is enabled, and if so store off the space manager pointer
#ifdef DEBUG
  PRInt32 verifyReflowFlags = nsIPresShell::GetVerifyReflowFlags();
  if (VERIFY_REFLOW_INCLUDE_SPACE_MANAGER & verifyReflowFlags)
  {
    // this is a leak of the space manager, but it's only in debug if verify reflow is enabled, so not a big deal
    nsIPresShell *shell = aPresContext->GetPresShell();
    if (shell) {
      nsHTMLReflowState&  reflowState = (nsHTMLReflowState&)aReflowState;
      rv = SetProperty(nsLayoutAtoms::spaceManagerProperty,
                       reflowState.mSpaceManager,
                       nsnull /* should be nsSpaceManagerDestroyer*/);

      autoSpaceManager.DebugOrphanSpaceManager();
    }
  }
#endif

  // Let the absolutely positioned container reflow any absolutely positioned
  // child frames that need to be reflowed, e.g., elements with a percentage
  // based width/height
  // We want to do this under either of two conditions:
  //  1. If we didn't do the incremental reflow above.
  //  2. If our size changed.
  // Even though it's the padding edge that's the containing block, we
  // can use our rect (the border edge) since if the border style
  // changed, the reflow would have been targeted at us so we'd satisfy
  // condition 1.
  if (mAbsoluteContainer.HasAbsoluteFrames()) {
    nsRect childBounds;
    nscoord containingBlockWidth;
    nscoord containingBlockHeight;

    CalculateContainingBlock(aReflowState, aMetrics.width, aMetrics.height,
                             containingBlockWidth, containingBlockHeight);

    PRBool needAbsoluteReflow = PR_TRUE;
    if (eReflowReason_Incremental == aReflowState.reason) {
      // Do the incremental reflows ... would be nice to merge with
      // the reflows below but that would be more work, and more risky
      mAbsoluteContainer.IncrementalReflow(this, aPresContext, aReflowState,
                                           containingBlockWidth,
                                           containingBlockHeight);
      
      // If a reflow was targeted at this block then we'd better
      // reflow the absolutes. For example the borders and padding
      // might have changed in a way that leaves the frame size the
      // same but the padding edge has moved.
      if (!aReflowState.path->mReflowCommand) {
        // Now we can assume that the padding edge hasn't moved.
        // We need to reflow the absolutes if one of them depends on
        // its placeholder position, or the containing block size in a
        // direction in which the containing block size might have
        // changed.
        PRBool cbWidthChanged = aMetrics.width != oldSize.width;
        PRBool isRoot = !GetContent()->GetParent();
        // If isRoot and we have auto height, then we are the initial
        // containing block and the containing block height is the
        // viewport height, which can't change during incremental
        // reflow.
        PRBool cbHeightChanged = isRoot && NS_UNCONSTRAINEDSIZE == aReflowState.mComputedHeight
          ? PR_FALSE : aMetrics.height != oldSize.height;
        if (!mAbsoluteContainer.FramesDependOnContainer(cbWidthChanged, cbHeightChanged)) {
          needAbsoluteReflow = PR_FALSE;
        }
      }
    }

    if (needAbsoluteReflow) {
      rv = mAbsoluteContainer.Reflow(this, aPresContext, aReflowState,
                                     containingBlockWidth,
                                     containingBlockHeight,
                                     &childBounds);
    } else {
      mAbsoluteContainer.CalculateChildBounds(aPresContext, childBounds);
    }

    // Factor the absolutely positioned child bounds into the overflow area
    aMetrics.mOverflowArea.UnionRect(currentOverflow, childBounds);
    FinishAndStoreOverflow(&aMetrics);
  }

  // Determine if we need to repaint our border, background or outline
  CheckInvalidateSizeChange(aPresContext, aMetrics, aReflowState);

  // Clear the space manager pointer in the block reflow state so we
  // don't waste time translating the coordinate system back on a dead
  // space manager.
  if (NS_BLOCK_SPACE_MGR & mState)
    state.mSpaceManager = nsnull;

  aStatus = state.mReflowStatus;

#ifdef DEBUG
  if (gNoisy) {
    gNoiseIndent--;
  }
  if (gNoisyReflow) {
    IndentBy(stdout, gNoiseIndent);
    ListTag(stdout);
    printf(": status=%x (%scomplete) metrics=%d,%d carriedMargin=%d",
           aStatus, NS_FRAME_IS_COMPLETE(aStatus) ? "" : "not ",
           aMetrics.width, aMetrics.height,
           aMetrics.mCarriedOutBottomMargin.get());
    if (mState & NS_FRAME_OUTSIDE_CHILDREN) {
      printf(" combinedArea={%d,%d,%d,%d}",
             aMetrics.mOverflowArea.x,
             aMetrics.mOverflowArea.y,
             aMetrics.mOverflowArea.width,
             aMetrics.mOverflowArea.height);
    }
    if (aMetrics.mComputeMEW) {
      printf(" maxElementWidth=%d", aMetrics.mMaxElementWidth);
    }
    printf("\n");
  }

  if (gLameReflowMetrics) {
    PRTime end = PR_Now();

    PRInt32 ectc = nsLineBox::GetCtorCount();
    PRInt32 numLines = mLines.size();
    if (!numLines) numLines = 1;
    PRTime delta, perLineDelta, lines;
    LL_I2L(lines, numLines);
    LL_SUB(delta, end, start);
    LL_DIV(perLineDelta, delta, lines);

    ListTag(stdout);
    char buf[400];
    PR_snprintf(buf, sizeof(buf),
                ": %lld elapsed (%lld per line) (%d lines; %d new lines)",
                delta, perLineDelta, numLines, ectc - ctc);
    printf("%s\n", buf);
  }
  if (gNoisyMaxElementWidth) {
    if (aMetrics.mComputeMEW) {
      IndentBy(stdout, gNoiseIndent);
      printf("block %p returning with maxElementWidth=%d\n",
             NS_STATIC_CAST(void*, this),
             aMetrics.mMaxElementWidth);
    }
  }
#endif

  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aMetrics);
  return rv;
}

static PRBool
HaveAutoWidth(const nsHTMLReflowState& aReflowState)
{
  return NS_UNCONSTRAINEDSIZE == aReflowState.mComputedWidth ||
         eStyleUnit_Auto == aReflowState.mStylePosition->mWidth.GetUnit();
}


// XXXldb why do we check vertical and horizontal at the same time?  Don't
// we usually care about one or the other?
static PRBool
IsPercentageAwareChild(const nsIFrame* aFrame)
{
  NS_ASSERTION(aFrame, "null frame is not allowed");

  const nsStyleMargin* margin = aFrame->GetStyleMargin();
  if (nsLineLayout::IsPercentageUnitSides(&margin->mMargin)) {
    return PR_TRUE;
  }

  const nsStylePadding* padding = aFrame->GetStylePadding();
  if (nsLineLayout::IsPercentageUnitSides(&padding->mPadding)) {
    return PR_TRUE;
  }

  const nsStyleBorder* border = aFrame->GetStyleBorder();
  if (nsLineLayout::IsPercentageUnitSides(&border->mBorder)) {
    return PR_TRUE;
  }

  const nsStylePosition* pos = aFrame->GetStylePosition();

  if (eStyleUnit_Percent == pos->mWidth.GetUnit()
    || eStyleUnit_Percent == pos->mMaxWidth.GetUnit()
    || eStyleUnit_Percent == pos->mMinWidth.GetUnit()
    || eStyleUnit_Percent == pos->mHeight.GetUnit()
    || eStyleUnit_Percent == pos->mMinHeight.GetUnit()
    || eStyleUnit_Percent == pos->mMaxHeight.GetUnit()
    || nsLineLayout::IsPercentageUnitSides(&pos->mOffset)) { // XXX need more here!!!
    return PR_TRUE;
  }

  return PR_FALSE;
}

PRBool
nsBlockFrame::CheckForCollapsedBottomMarginFromClearanceLine()
{
  line_iterator begin = begin_lines();
  line_iterator line = end_lines();

  while (PR_TRUE) {
    if (begin == line) {
      return PR_FALSE;
    }
    --line;
    if (line->mBounds.height != 0 || !line->CachedIsEmpty()) {
      return PR_FALSE;
    }
    if (line->HasClearance()) {
      return PR_TRUE;
    }
  }
  // not reached
}

void
nsBlockFrame::ComputeFinalSize(const nsHTMLReflowState& aReflowState,
                               nsBlockReflowState&      aState,
                               nsHTMLReflowMetrics&     aMetrics)
{
  const nsMargin& borderPadding = aState.BorderPadding();
#ifdef NOISY_FINAL_SIZE
  ListTag(stdout);
  printf(": mY=%d mIsBottomMarginRoot=%s mPrevBottomMargin=%d bp=%d,%d\n",
         aState.mY, aState.GetFlag(BRS_ISBOTTOMMARGINROOT) ? "yes" : "no",
         aState.mPrevBottomMargin,
         borderPadding.top, borderPadding.bottom);
#endif

  // XXXldb Handling min-width/max-width stuff after reflowing children
  // seems wrong.  But IIRC this function only does more than a little
  // bit in rare cases (or something like that, I'm not really sure).
  // What are those cases, and do we get the wrong behavior?

  // Compute final width
  nscoord maxElementWidth = 0;
#ifdef NOISY_KIDXMOST
  printf("%p aState.mKidXMost=%d\n", this, aState.mKidXMost); 
#endif
  if (!HaveAutoWidth(aReflowState)) {
    // Use style defined width
    aMetrics.width = borderPadding.left + aReflowState.mComputedWidth +
      borderPadding.right;

    if (aState.GetFlag(BRS_COMPUTEMAXELEMENTWIDTH)) {
      if (GetStylePosition()->mWidth.GetUnit() == eStyleUnit_Percent) {
        // for percentage widths, |HaveAutoWidth| is sometimes true and
        // sometimes false (XXXldb check that this is really true), and
        // we want the max-element-width to be the same either way
        // (i.e., whether it's an uncontsrained reflow or a fixed-width
        // reflow).  Thus, do the same thing we do below.
        maxElementWidth = aState.mMaxElementWidth +
          borderPadding.left + borderPadding.right;
      } else {
        // When style defines the width use it for the max-element-width
        // because we can't shrink any smaller.
        maxElementWidth = aMetrics.width;
      }
    }
  }
  else {
    nscoord computedWidth;

    // XXX Misleading comment:
    // There are two options here. We either shrink wrap around our
    // contents or we fluff out to the maximum block width. Note:
    // We always shrink wrap when given an unconstrained width.
    if ((0 == (NS_BLOCK_SHRINK_WRAP & mState)) &&
        !aState.GetFlag(BRS_UNCONSTRAINEDWIDTH) &&
        !aState.GetFlag(BRS_SHRINKWRAPWIDTH)) {
      // XXX Misleading comment:
      // Set our width to the max width if we aren't already that
      // wide. Note that the max-width has nothing to do with our
      // contents (CSS2 section XXX)
      // XXXldb In what cases do we reach this code?
      computedWidth = borderPadding.left + aState.mContentArea.width +
        borderPadding.right;
    } else {
      computedWidth = aState.mKidXMost;
      if (NS_BLOCK_SPACE_MGR & mState) {
        // Include the space manager's state to properly account for the
        // extent of floated elements.
        nscoord xmost;
        if (aReflowState.mSpaceManager->XMost(xmost) &&
            computedWidth < xmost)
          computedWidth = xmost;
      }
      computedWidth += borderPadding.right;
    }

    if (aState.GetFlag(BRS_COMPUTEMAXELEMENTWIDTH)) {
      // Add in border and padding dimensions to already computed
      // max-element-width values.
      maxElementWidth = aState.mMaxElementWidth +
        borderPadding.left + borderPadding.right;
      if (computedWidth < maxElementWidth) {
        // XXXldb It's *compute* max-element-width, not *change size
        // based on* max-element-width...
        computedWidth = maxElementWidth;
      }
    }

    // Apply min/max values
    if (NS_UNCONSTRAINEDSIZE != aReflowState.mComputedMaxWidth) {
      nscoord computedMaxWidth = aReflowState.mComputedMaxWidth +
        borderPadding.left + borderPadding.right;
      if (computedWidth > computedMaxWidth) {
        computedWidth = computedMaxWidth;
      }
    }
    if (NS_UNCONSTRAINEDSIZE != aReflowState.mComputedMinWidth) {
      nscoord computedMinWidth = aReflowState.mComputedMinWidth +
        borderPadding.left + borderPadding.right;
      if (computedWidth < computedMinWidth) {
        computedWidth = computedMinWidth;
      }
    }
    aMetrics.width = computedWidth;

    // If we're shrink wrapping, then now that we know our final width we
    // need to do horizontal alignment of the inline lines and make sure
    // blocks are correctly sized and positioned. Any lines that need
    // final adjustment will have been marked as dirty
    if (aState.GetFlag(BRS_SHRINKWRAPWIDTH) && aState.GetFlag(BRS_NEEDRESIZEREFLOW)) {
      // If the parent reflow state is also shrink wrap width, then
      // we don't need to do this, because it will reflow us after it
      // calculates the final width
      const nsHTMLReflowState *prs = aReflowState.parentReflowState;
      if (!prs || NS_SHRINKWRAPWIDTH != prs->mComputedWidth) {
        // XXX Is this only used on things that are already NS_BLOCK_SPACE_MGR
        // and NS_BLOCK_MARGIN_ROOT?
        nsHTMLReflowState reflowState(aReflowState);

        reflowState.mComputedWidth = aMetrics.width - borderPadding.left -
                                     borderPadding.right;
        reflowState.reason = eReflowReason_Resize;
        reflowState.mSpaceManager->ClearRegions();

#ifdef DEBUG
        nscoord oldDesiredWidth = aMetrics.width;
#endif
        nsBlockReflowState state(reflowState, aState.mPresContext, this,
                                 aMetrics,
                                 aReflowState.mFlags.mHasClearance || (NS_BLOCK_MARGIN_ROOT & mState),
                                 (NS_BLOCK_MARGIN_ROOT & mState));
        ReflowDirtyLines(state);
        aState.mY = state.mY;
        NS_ASSERTION(oldDesiredWidth == aMetrics.width, "bad desired width");
      }
    }
  }

  // Return bottom margin information
  // rbs says he hit this assertion occasionally (see bug 86947), so
  // just set the margin to zero and we'll figure out why later
  //NS_ASSERTION(aMetrics.mCarriedOutBottomMargin.IsZero(),
  //             "someone else set the margin");
  nscoord nonCarriedOutVerticalMargin = 0;
  if (!aState.GetFlag(BRS_ISBOTTOMMARGINROOT)) {
    // Apply rule from CSS 2.1 section 8.3.1. If we have some empty
    // line with clearance and a non-zero top margin and all
    // subsequent lines are empty, then we do not allow our childrens'
    // carried out bottom margin to be carried out of us and collapse
    // with our own bottom margin.
    if (CheckForCollapsedBottomMarginFromClearanceLine()) {
      // Convert the children's carried out margin to something that
      // we will include in our height
      nonCarriedOutVerticalMargin = aState.mPrevBottomMargin.get();
      aState.mPrevBottomMargin.Zero();
    }
    aMetrics.mCarriedOutBottomMargin = aState.mPrevBottomMargin;
  } else {
    aMetrics.mCarriedOutBottomMargin.Zero();
  }

  // Compute final height
  if (NS_UNCONSTRAINEDSIZE != aReflowState.mComputedHeight) {
    if (NS_FRAME_IS_COMPLETE(aState.mReflowStatus)) {
      // Calculate the total unconstrained height including borders and padding. A continuation 
      // will have the same value as the first-in-flow, since the reflow state logic is based on
      // style and doesn't alter mComputedHeight based on prev-in-flows.
      aMetrics.height = borderPadding.top + aReflowState.mComputedHeight + borderPadding.bottom;
      if (mPrevInFlow) {
        // Reduce the height by the height of prev-in-flows. The last-in-flow will automatically
        // pick up the bottom border/padding, since it was part of the original aMetrics.height.
        for (nsIFrame* prev = mPrevInFlow; prev; prev = prev->GetPrevInFlow()) {
          aMetrics.height -= prev->GetRect().height;
          // XXX: All block level next-in-flows have borderPadding.top applied to them (bug 174688). 
          // The following should be removed when this gets fixed. bug 174688 prevents us from honoring 
          // a style height (exactly) and this hack at least compensates by increasing the height by the
          // excessive borderPadding.top.
          aMetrics.height += borderPadding.top;
        }
        aMetrics.height = PR_MAX(0, aMetrics.height);
      }
      if (aMetrics.height > aReflowState.availableHeight) {
        // Take up the available height; continuations will take up the rest.
        aMetrics.height = aReflowState.availableHeight;
        aState.mReflowStatus = NS_FRAME_NOT_COMPLETE;
      }
    }
    else {
      // Use the current height; continuations will take up the rest.
      aMetrics.height = aState.mY + nonCarriedOutVerticalMargin;
    }

    // Don't carry out a bottom margin when our height is fixed.
    aMetrics.mCarriedOutBottomMargin.Zero();
  }
  else {
    nscoord autoHeight = aState.mY + nonCarriedOutVerticalMargin;

    // Shrink wrap our height around our contents.
    if (aState.GetFlag(BRS_ISBOTTOMMARGINROOT)) {
      // When we are a bottom-margin root make sure that our last
      // childs bottom margin is fully applied.
      // XXX check for a fit
      autoHeight += aState.mPrevBottomMargin.get();
    }

    if (NS_BLOCK_SPACE_MGR & mState) {
      // Include the space manager's state to properly account for the
      // bottom margin of any floated elements; e.g., inside a table cell.
      nscoord ymost;
      if (aReflowState.mSpaceManager->YMost(ymost) &&
          autoHeight < ymost)
        autoHeight = ymost;
    }
    autoHeight += borderPadding.bottom;

    // Apply min/max values
    if (NS_UNCONSTRAINEDSIZE != aReflowState.mComputedMaxHeight) {
      nscoord computedMaxHeight = aReflowState.mComputedMaxHeight +
        borderPadding.top + borderPadding.bottom;
      if (autoHeight > computedMaxHeight) {
        autoHeight = computedMaxHeight;
      }
    }
    if (NS_UNCONSTRAINEDSIZE != aReflowState.mComputedMinHeight) {
      nscoord computedMinHeight = aReflowState.mComputedMinHeight +
        borderPadding.top + borderPadding.bottom;
      if (autoHeight < computedMinHeight) {
        autoHeight = computedMinHeight;
      }
    }
    aMetrics.height = autoHeight;

  }

  aMetrics.ascent = mAscent;
  aMetrics.descent = aMetrics.height - aMetrics.ascent;

  if (aState.GetFlag(BRS_COMPUTEMAXELEMENTWIDTH)) {
    // Store away the final value
    aMetrics.mMaxElementWidth = maxElementWidth;
#ifdef DEBUG
    if (gNoisyMaxElementWidth) {
      IndentBy(stdout, gNoiseIndent);
      printf ("nsBlockFrame::CFS: %p returning MEW %d\n", 
              NS_STATIC_CAST(void*, this), aMetrics.mMaxElementWidth);
    }
#endif
  }

#ifdef DEBUG_blocks
  if (CRAZY_WIDTH(aMetrics.width) || CRAZY_HEIGHT(aMetrics.height)) {
    ListTag(stdout);
    printf(": WARNING: desired:%d,%d\n", aMetrics.width, aMetrics.height);
  }
  if (aState.GetFlag(BRS_COMPUTEMAXELEMENTWIDTH) &&
      (maxElementWidth > aMetrics.width))) {
    ListTag(stdout);
    printf(": WARNING: max-element-width:%d desired:%d,%d maxSize:%d,%d\n",
           maxElementWidth, aMetrics.width, aMetrics.height,
           aState.mReflowState.availableWidth,
           aState.mReflowState.availableHeight);
  }
#endif
#ifdef DEBUG
  if (gNoisyMaxElementWidth) {
    if (aState.GetFlag(BRS_COMPUTEMAXELEMENTWIDTH)) {
      IndentBy(stdout, gNoiseIndent);
      if (NS_UNCONSTRAINEDSIZE == aState.mReflowState.availableWidth) {
        printf("PASS1 ");
      }
      ListTag(stdout);
      printf(": max-element-width:%d desired:%d,%d maxSize:%d,%d\n",
             maxElementWidth, aMetrics.width, aMetrics.height,
             aState.mReflowState.availableWidth,
             aState.mReflowState.availableHeight);
    }
  }
#endif

  // If we're requested to update our maximum width, then compute it
  if (aState.GetFlag(BRS_COMPUTEMAXWIDTH)) {
    if (!HaveAutoWidth(aReflowState) &&
        aReflowState.mStylePosition->mWidth.GetUnit() != eStyleUnit_Percent) {
      aMetrics.mMaximumWidth = aMetrics.width;
    } else {
      // We need to add in for the right border/padding
      // XXXldb Why right and not left?
      aMetrics.mMaximumWidth = aState.mMaximumWidth + borderPadding.right;
    }
#ifdef NOISY_MAXIMUM_WIDTH
    printf("nsBlockFrame::ComputeFinalSize block %p setting aMetrics.mMaximumWidth to %d\n", this, aMetrics.mMaximumWidth);
#endif
  }

  ComputeCombinedArea(aReflowState, aMetrics);
}

void
nsBlockFrame::ComputeCombinedArea(const nsHTMLReflowState& aReflowState,
                                  nsHTMLReflowMetrics& aMetrics)
{
  // Compute the combined area of our children
  // XXX_perf: This can be done incrementally.  It is currently one of
  // the things that makes incremental reflow O(N^2).
  nsRect area(0, 0, aMetrics.width, aMetrics.height);
  if (NS_STYLE_OVERFLOW_CLIP != aReflowState.mStyleDisplay->mOverflowX) {
    for (line_iterator line = begin_lines(), line_end = end_lines();
         line != line_end;
         ++line) {
      area.UnionRect(area, line->GetCombinedArea());
    }

    // Factor the bullet in; normally the bullet will be factored into
    // the line-box's combined area. However, if the line is a block
    // line then it won't; if there are no lines, it won't. So just
    // factor it in anyway (it can't hurt if it was already done).
    // XXXldb Can we just fix GetCombinedArea instead?
    if (mBullet) {
      area.UnionRect(area, mBullet->GetRect());
    }
  }
#ifdef NOISY_COMBINED_AREA
  ListTag(stdout);
  printf(": ca=%d,%d,%d,%d\n", area.x, area.y, area.width, area.height);
#endif

  aMetrics.mOverflowArea = area;
}

nsresult
nsBlockFrame::PrepareInitialReflow(nsBlockReflowState& aState)
{
  PrepareResizeReflow(aState);
  return NS_OK;
}

nsresult
nsBlockFrame::PrepareChildIncrementalReflow(nsBlockReflowState& aState)
{
  // XXXwaterson this is non-optimal. We'd rather do this in
  // ReflowDirtyLines; however, I'm not quite ready to figure out how
  // to deal with reflow path retargeting yet.
  nsReflowPath *path = aState.mReflowState.path;

  nsReflowPath::iterator iter = path->FirstChild();
  nsReflowPath::iterator end = path->EndChildren();

  for ( ; iter != end; ++iter) {
    // Determine the line being impacted
    line_iterator line = FindLineFor(*iter);
    if (line == end_lines()) {
      // This assertion actually fires on lots of pages
      // (e.g., bugzilla, bugzilla query page), so limit it
      // to a few people until we fix the problem causing it.
      //
      // I think waterson explained once why it was happening -- I think
      // it has something to do with the interaction of the unconstrained
      // reflow in multi-pass reflow with the reflow command's chain, but
      // I don't remember the details.
#if defined(DEBUG_dbaron) || defined(DEBUG_waterson)
      NS_NOTREACHED("We don't have a line for the target of the reflow.  "
                    "Being inefficient");
#endif
      // This can't happen, but just in case it does...
      PrepareResizeReflow(aState);
      continue;
    }

    if (line->IsInline()) {
      if (aState.GetFlag(BRS_COMPUTEMAXWIDTH)) {
        // We've been asked to compute the maximum width of the block
        // frame, which ReflowLine() will handle by performing an
        // unconstrained reflow on the line. If this incremental
        // reflow is targeted at a continuing frame, we may have to
        // retarget it, as the unconstrained reflow can destroy some
        // of the continuations. This will allow the incremental
        // reflow to arrive at the target frame during the first-pass
        // unconstrained reflow.
        nsIFrame *prevInFlow = (*iter)->GetPrevInFlow();
        if (prevInFlow)
          RetargetInlineIncrementalReflow(iter, line, prevInFlow);
      }
    }

    // Just mark this line dirty.  We never need to mark the
    // previous line dirty since either:
    //  * the line is a block, and there would never be a chance to pull
    //    something up
    //  * It's an incremental reflow to something within an inline, which
    //    we know must be very limited.
    line->MarkDirty();
  }
  return NS_OK;
}

void
nsBlockFrame::RetargetInlineIncrementalReflow(nsReflowPath::iterator &aTarget,
                                              line_iterator &aLine,
                                              nsIFrame *aPrevInFlow)
{
  // To retarget the reflow, we'll walk back through the continuations
  // until we reach the primary frame, or we reach a continuation that
  // is preceded by a ``hard'' line break.
  NS_ASSERTION(aLine->Contains(*aTarget),
               "line doesn't contain the target of the incremental reflow");

  // Now fix the iterator, keeping track of how many lines we walk
  // back through.
  PRInt32 lineCount = 0;
  do {
    // XXX this might happen if the block is split; e.g.,
    // printing or print preview. For now, panic.
    NS_ASSERTION(aLine != begin_lines(),
                 "ran out of lines before we ran out of prev-in-flows");

    // Is the previous line a ``hard'' break? If so, stop: these
    // continuations will be preserved during an unconstrained reflow.
    // XXXwaterson should this be `!= NS_STYLE_CLEAR_NONE'?
    --aLine;
    if (aLine->GetBreakTypeAfter() == NS_STYLE_CLEAR_LINE)
      break;

    *aTarget = aPrevInFlow;
    aPrevInFlow = aPrevInFlow->GetPrevInFlow();

#ifdef DEBUG
    // Paranoia. Ensure that the prev-in-flow is really in the
    // previous line.
    line_iterator check = FindLineFor(*aTarget);
    NS_ASSERTION(check == aLine, "prev-in-flow not in previous linebox");
#endif

    ++lineCount;
  } while (aPrevInFlow);

  if (lineCount > 0) {
    // Fix any frames deeper in the reflow path.
#if 0
    // XXXwaterson fix me! we've got to recurse through the iterator's
    // kids.  This really shouldn't matter unless we want to implement
    // `display: inline-block' or do XBL form controls. Why, you ask?
    // Because what will happen is that inline frames will get flowed
    // with a resize reflow, which will be sufficient to mask any
    // glitches that would otherwise occur. However, as soon as boxes
    // or blocks end up in the flow here (and aren't explicit reflow
    // roots), they may optimize away the resize reflow.

    // Get the reflow path, which is stored as a stack (i.e., the next
    // frame in the reflow is at the _end_ of the array).
    nsVoidArray *path = aState.mReflowState.reflowCommand->GetPath();

    for (PRInt32 i = path->Count() - 1; i >= 0; --i) {
      nsIFrame *frame = NS_STATIC_CAST(nsIFrame *, path->ElementAt(i));

      // Stop if we encounter a non-inline frame in the reflow path.
      const nsStyleDisplay* display = frame->GetStyleDisplay();

      if (NS_STYLE_DISPLAY_INLINE != display->mDisplay)
        break;

      // Walk back to the primary frame.
      PRInt32 count = lineCount;
      nsIFrame *prevInFlow;
      do {
        prevInFlow = frame->GetPrevInFlow();
      } while (--count >= 0 && prevInFlow && (frame = prevInFlow));

      path->ReplaceElementAt(frame, i);
    }
#else
    NS_WARNING("blowing an incremental reflow targeted at a nested inline");
#endif
  }
}

nsresult
nsBlockFrame::MarkLineDirty(line_iterator aLine)
{
  // Mark aLine dirty
  aLine->MarkDirty();
#ifdef DEBUG
  if (gNoisyReflow) {
    IndentBy(stdout, gNoiseIndent);
    ListTag(stdout);
    printf(": mark line %p dirty\n", NS_STATIC_CAST(void*, aLine.get()));
  }
#endif

  // Mark previous line dirty if its an inline line so that it can
  // maybe pullup something from the line just affected.
  // XXX We don't need to do this if aPrevLine ends in a break-after...
  if (aLine != mLines.front() &&
      aLine->IsInline() &&
      aLine.prev()->IsInline()) {
    aLine.prev()->MarkDirty();
#ifdef DEBUG
    if (gNoisyReflow) {
      IndentBy(stdout, gNoiseIndent);
      ListTag(stdout);
      printf(": mark prev-line %p dirty\n",
             NS_STATIC_CAST(void*, aLine.prev().get()));
    }
#endif
  }

  return NS_OK;
}

nsresult
nsBlockFrame::UpdateBulletPosition(nsBlockReflowState& aState)
{
  if (nsnull == mBullet) {
    // Don't bother if there is no bullet
    return NS_OK;
  }
  const nsStyleList* styleList = GetStyleList();
  if (NS_STYLE_LIST_STYLE_POSITION_INSIDE == styleList->mListStylePosition) {
    if (mBullet && HaveOutsideBullet()) {
      // We now have an inside bullet, but used to have an outside
      // bullet.  Adjust the frame line list
      if (! mLines.empty()) {
        // if we have a line already, then move the bullet to the front of the
        // first line
        nsIFrame* child = nsnull;
        nsLineBox* firstLine = mLines.front();
        
        // bullet should not have any siblings if it was an outside bullet
        NS_ASSERTION(!mBullet->GetNextSibling(), "outside bullet should not have siblings");

        // move bullet to front and chain the previous frames, and update the line count
        child = firstLine->mFirstChild;
        firstLine->mFirstChild = mBullet;
        mBullet->SetNextSibling(child);
        PRInt32 count = firstLine->GetChildCount();
        firstLine->SetChildCount(count+1);
        // dirty it here in case the caller does not
        firstLine->MarkDirty();
      } else {
        // no prior lines, just create a new line for the bullet
        nsLineBox* line = aState.NewLineBox(mBullet, 1, PR_FALSE);
        if (!line) {
          return NS_ERROR_OUT_OF_MEMORY;
        }
        mLines.push_back(line);
      }
    }
    mState &= ~NS_BLOCK_FRAME_HAS_OUTSIDE_BULLET;
  }
  else {
    if (!HaveOutsideBullet()) {
      // We now have an outside bullet, but used to have an inside
      // bullet. Take the bullet frame out of the first lines frame
      // list.
      if ((! mLines.empty()) && (mBullet == mLines.front()->mFirstChild)) {
        nsIFrame* next = mBullet->GetNextSibling();
        mBullet->SetNextSibling(nsnull);
        PRInt32 count = mLines.front()->GetChildCount() - 1;
        NS_ASSERTION(count >= 0, "empty line w/o bullet");
        mLines.front()->SetChildCount(count);
        if (0 == count) {
          nsLineBox* oldFront = mLines.front();
          mLines.pop_front();
          aState.FreeLineBox(oldFront);
          if (! mLines.empty()) {
            mLines.front()->MarkDirty();
          }
        }
        else {
          mLines.front()->mFirstChild = next;
          mLines.front()->MarkDirty();
        }
      }
    }
    mState |= NS_BLOCK_FRAME_HAS_OUTSIDE_BULLET;
  }
#ifdef DEBUG
  VerifyLines(PR_TRUE);
#endif
  return NS_OK;
}

nsresult
nsBlockFrame::PrepareStyleChangedReflow(nsBlockReflowState& aState)
{
  nsresult rv = UpdateBulletPosition(aState);

  // Mark everything dirty
  for (line_iterator line = begin_lines(), line_end = end_lines();
       line != line_end;
       ++line)
  {
    line->MarkDirty();
  }
  return rv;
}

nsresult
nsBlockFrame::PrepareResizeReflow(nsBlockReflowState& aState)
{
  // See if we can try and avoid marking all the lines as dirty
  PRBool  tryAndSkipLines = PR_FALSE;

  // we need to calculate if any part of then block itself 
  // is impacted by a float (bug 19579)
  aState.GetAvailableSpace();

  // See if this is a constrained resize reflow that is not impacted by floats
  if ((! aState.IsImpactedByFloat()) &&
      (aState.mReflowState.reason == eReflowReason_Resize) &&
      (NS_UNCONSTRAINEDSIZE != aState.mReflowState.availableWidth)) {

    // If the text is left-aligned, then we try and avoid reflowing the lines
    const nsStyleText* styleText = GetStyleText();

    if ((NS_STYLE_TEXT_ALIGN_LEFT == styleText->mTextAlign) ||
        ((NS_STYLE_TEXT_ALIGN_DEFAULT == styleText->mTextAlign) &&
         (NS_STYLE_DIRECTION_LTR == aState.mReflowState.mStyleVisibility->mDirection))) {
      tryAndSkipLines = PR_TRUE;
    }
  }

#ifdef DEBUG
  if (gDisableResizeOpt) {
    tryAndSkipLines = PR_FALSE;
  }
  if (gNoisyReflow) {
    if (!tryAndSkipLines) {
      const nsStyleText* styleText = GetStyleText();
      IndentBy(stdout, gNoiseIndent);
      ListTag(stdout);
      printf(": marking all lines dirty: reason=%d availWidth=%d textAlign=%d\n",
             aState.mReflowState.reason,
             aState.mReflowState.availableWidth,
             styleText->mTextAlign);
    }
  }
#endif

  if (tryAndSkipLines) {
    nscoord newAvailWidth = aState.mReflowState.mComputedBorderPadding.left;
     
    if (NS_SHRINKWRAPWIDTH == aState.mReflowState.mComputedWidth) {
      if (NS_UNCONSTRAINEDSIZE != aState.mReflowState.mComputedMaxWidth) {
        newAvailWidth += aState.mReflowState.mComputedMaxWidth;
      }
      else {
        newAvailWidth += aState.mReflowState.availableWidth;
      }
    } else {
      if (NS_UNCONSTRAINEDSIZE != aState.mReflowState.mComputedWidth) {
        newAvailWidth += aState.mReflowState.mComputedWidth;
      }
      else {
        newAvailWidth += aState.mReflowState.availableWidth;
      }
    }
    NS_ASSERTION(NS_UNCONSTRAINEDSIZE != newAvailWidth, "bad math, newAvailWidth is infinite");

#ifdef DEBUG
    if (gNoisyReflow) {
      IndentBy(stdout, gNoiseIndent);
      ListTag(stdout);
      printf(": trying to avoid marking all lines dirty\n");
    }
#endif

    for (line_iterator line = begin_lines(), line_end = end_lines();
         line != line_end;
         ++line)
    {
      // We let child blocks make their own decisions the same
      // way we are here.
      if (line->IsBlock() ||
          // XXXldb We need HasPercentageDescendant, not HasPercentageChild!!!
          line->HasPercentageChild() || 
          line->HasFloats() ||
          (line != mLines.back() && !line->HasBreakAfter()) ||
          line->ResizeReflowOptimizationDisabled() ||
          line->IsImpactedByFloat() ||
          (line->mBounds.XMost() > newAvailWidth)) {
        line->MarkDirty();
      }

#ifdef REALLY_NOISY_REFLOW
      if (!line->IsBlock()) {
        printf("PrepareResizeReflow thinks line %p is %simpacted by floats\n", 
        line, line->IsImpactedByFloat() ? "" : "not ");
      }
#endif
#ifdef DEBUG
      if (gNoisyReflow && !line->IsDirty()) {
        IndentBy(stdout, gNoiseIndent + 1);
        printf("skipped: line=%p next=%p %s %s%s%s breakTypeBefore/After=%d/%d xmost=%d\n",
           NS_STATIC_CAST(void*, line.get()),
           NS_STATIC_CAST(void*, (line.next() != end_lines() ? line.next().get() : nsnull)),
           line->IsBlock() ? "block" : "inline",
           line->HasBreakAfter() ? "has-break-after " : "",
           line->HasFloats() ? "has-floats " : "",
           line->IsImpactedByFloat() ? "impacted " : "",
           line->GetBreakTypeBefore(), line->GetBreakTypeAfter(),
           line->mBounds.XMost());
      }
#endif
    }
  }
  else {
    // Mark everything dirty
    for (line_iterator line = begin_lines(), line_end = end_lines();
         line != line_end;
         ++line)
    {
      line->MarkDirty();
    }
  }
  return NS_OK;
}

//----------------------------------------

nsBlockFrame::line_iterator
nsBlockFrame::FindLineFor(nsIFrame* aFrame)
{
  // This assertion actually fires on lots of pages (e.g., bugzilla,
  // bugzilla query page), so limit it to a few people until we fix the
  // problem causing it.  It's related to the similarly |#ifdef|ed
  // assertion in |PrepareChildIncrementalReflow|.
#if defined(DEBUG_dbaron) || defined(DEBUG_waterson)
  NS_PRECONDITION(aFrame, "why pass a null frame?");
#endif

  line_iterator line = begin_lines(),
                line_end = end_lines();
  for ( ; line != line_end; ++line) {
    // If the target frame is in-flow, and this line contains the it,
    // then we've found our line.
    if (line->Contains(aFrame))
      return line;

    // If the target frame is floated, and this line contains the
    // float's placeholder, then we've found our line.
    if (line->HasFloats()) {
      for (nsFloatCache *fc = line->GetFirstFloat();
           fc != nsnull;
           fc = fc->Next()) {
        if (aFrame == fc->mPlaceholder->GetOutOfFlowFrame())
          return line;
      }
    }
  }

  return line_end;
}

/**
 * Propagate reflow "damage" from from earlier lines to the current
 * line.  The reflow damage comes from the following sources:
 *  1. The regions of float damage remembered during reflow.
 *  2. The combination of nonzero |aDeltaY| and any impact by a float,
 *     either the previous reflow or now.
 *
 * When entering this function, |aLine| is still at its old position and
 * |aDeltaY| indicates how much it will later be slid (assuming it
 * doesn't get marked dirty and reflowed entirely).
 */
void
nsBlockFrame::PropagateFloatDamage(nsBlockReflowState& aState,
                                   nsLineBox* aLine,
                                   nscoord aDeltaY)
{
  NS_PRECONDITION(!aLine->IsDirty(), "should never be called on dirty lines");

  // Check the damage region recorded in the float damage.
  nsSpaceManager *spaceManager = aState.mReflowState.mSpaceManager;
  if (spaceManager->HasFloatDamage()) {
    nscoord lineYA = aLine->mBounds.y + aDeltaY;
    nscoord lineYB = lineYA + aLine->mBounds.height;
    if (spaceManager->IntersectsDamage(lineYA, lineYB)) {
      aLine->MarkDirty();
      return;
    }
  }

  if (aDeltaY) {
    // Cases we need to find:
    //
    // 1. the line was impacted by a float and now isn't
    // 2. the line wasn't impacted by a float and now is
    // 3. the line is impacted by a float both before and after and 
    //    the float has changed position relative to the line (or it's
    //    a different float).  (XXXPerf we don't currently
    //    check whether the float changed size.  We currently just
    //    mark blocks dirty and ignore any possibility of damage to
    //    inlines by it being a different float with a different
    //    size.)
    //
    //    XXXPerf: An optimization: if the line was and is completely
    //    impacted by a float and the float hasn't changed size,
    //    then we don't need to mark the line dirty.
    aState.GetAvailableSpace(aLine->mBounds.y + aDeltaY);
    PRBool wasImpactedByFloat = aLine->IsImpactedByFloat();
    PRBool isImpactedByFloat = aState.IsImpactedByFloat();
#ifdef REALLY_NOISY_REFLOW
    printf("nsBlockFrame::PropagateFloatDamage %p was = %d, is=%d\n", 
       this, wasImpactedByFloat, isImpactedByFloat);
#endif
    // Mark the line dirty if:
    //  1. It used to be impacted by a float and now isn't, or vice
    //     versa.
    //  2. It is impacted by a float and it is a block, which means
    //     that more or less of the line could be impacted than was in
    //     the past.  (XXXPerf This could be optimized further, since
    //     we're marking the whole line dirty.)
    if ((wasImpactedByFloat != isImpactedByFloat) ||
        (isImpactedByFloat && aLine->IsBlock())) {
      aLine->MarkDirty();
    }
  }
}

static void
DirtyLinesWithDirtyContinuations(const nsLineList::iterator& aLineStart,
                                 const nsLineList::iterator& aLineEnd)
{
  // The line we're looking at right now
  nsLineList::iterator line(aLineEnd);

  // Whether the line following the current one is dirty
  PRBool nextLineDirty = PR_FALSE;
  
  while (line != aLineStart) {
    --line;

    if (nextLineDirty && line->IsInline() && line->IsLineWrapped()) {
      line->MarkDirty();
      // Note that nextLineDirty is already true and |line| will be the "next
      // line" next time through this loop, and we just marked it dirty, so
      // just leave nextLineDirty as true.
    } else {
      nextLineDirty = line->IsDirty();
    }
  }
}

static void PlaceFrameView(nsPresContext* aPresContext, nsIFrame* aFrame);
static void CollectFloats(nsIFrame* aFrame, nsIFrame* aBlockParent,
                          nsIFrame** aHead, nsIFrame** aTail);

static PRBool LineHasClear(nsLineBox* aLine) {
  return aLine->GetBreakTypeBefore() || aLine->HasFloatBreakAfter()
    || (aLine->IsBlock() && (aLine->mFirstChild->GetStateBits() & NS_BLOCK_HAS_CLEAR_CHILDREN));
}


static void ReparentFrame(nsIFrame* aFrame, nsIFrame* aOldParent,
                          nsIFrame* aNewParent) {
  aFrame->SetParent(aNewParent);

  // When pushing and pulling frames we need to check for whether any
  // views need to be reparented
  nsHTMLContainerFrame::ReparentFrameView(aFrame->GetPresContext(), aFrame,
                                          aOldParent, aNewParent);
}

/**
 * Reparent a whole list of floats from aOldParent to this block.  The
 * floats might be taken from aOldParent's overflow list. Whichever
 * list they're on, they must be the first floats in the list. They
 * will be removed from the list.
 */
void
nsBlockFrame::ReparentFloats(nsIFrame* aFirstFrame,
                             nsBlockFrame* aOldParent, PRBool aFromOverflow) {
  nsIFrame* head = nsnull;
  nsIFrame* tail = nsnull;
  CollectFloats(aFirstFrame, aOldParent, &head, &tail);
  if (head) {
    if (aFromOverflow) {
      nsFrameList* oofs = aOldParent->GetOverflowOutOfFlows();
      NS_ASSERTION(oofs && head == oofs->FirstChild(), "Floats out of order");
      if (tail->GetNextSibling()) {
        oofs->SetFrames(tail->GetNextSibling());
      } else {
        delete aOldParent->RemoveOverflowOutOfFlows();
      }
    } else {
      NS_ASSERTION(head == aOldParent->mFloats.FirstChild(),
                   "Floats out of order");
      aOldParent->mFloats.SetFrames(tail->GetNextSibling());
    }
    for (nsIFrame* f = head; f != tail->GetNextSibling();
         f = f->GetNextSibling()) {
      ReparentFrame(f, aOldParent, this);
    }
  }
}

/**
 * Reflow the dirty lines
 */
nsresult
nsBlockFrame::ReflowDirtyLines(nsBlockReflowState& aState)
{
  nsresult rv = NS_OK;
  PRBool keepGoing = PR_TRUE;
  PRBool repositionViews = PR_FALSE; // should we really need this?
  PRBool foundAnyClears = PR_FALSE;

#ifdef DEBUG
  if (gNoisyReflow) {
    if (aState.mReflowState.reason == eReflowReason_Incremental) {
      IndentBy(stdout, gNoiseIndent);
      ListTag(stdout);
      printf(": incrementally reflowing dirty lines");

      nsHTMLReflowCommand *command = aState.mReflowState.path->mReflowCommand;
      if (command) {
        nsReflowType type;
        command->GetType(type);
        printf(": type=%s(%d)", kReflowCommandType[type], type);
      }
    }
    else {
      IndentBy(stdout, gNoiseIndent);
      ListTag(stdout);
      printf(": reflowing dirty lines");
    }
    printf(" computedWidth=%d\n", aState.mReflowState.mComputedWidth);
    gNoiseIndent++;
  }
#endif

  // Check whether we need to do invalidation for the child block
  PRBool doInvalidate =
    aState.mReflowState.reason == eReflowReason_Incremental ||
    aState.mReflowState.reason == eReflowReason_Dirty ||
    aState.mReflowState.reason == eReflowReason_Resize;
  
    // the amount by which we will slide the current line if it is not
    // dirty
  nscoord deltaY = 0;

    // whether we did NOT reflow the previous line and thus we need to
    // recompute the carried out margin before the line if we want to
    // reflow it or if its previous margin is dirty
  PRBool needToRecoverState = PR_FALSE;
  PRBool lastLineMovedUp = PR_FALSE;
  // We save up information about BR-clearance here
  PRUint8 inlineFloatBreakType = NS_STYLE_CLEAR_NONE;

  line_iterator line = begin_lines(), line_end = end_lines();

  // If we're supposed to update our maximum width, then we'll also need to
  // reflow any line if it's line wrapped and has a dirty continuing line.
  if (aState.GetFlag(BRS_COMPUTEMAXWIDTH)) {
    ::DirtyLinesWithDirtyContinuations(line, line_end);
  }

  // Reflow the lines that are already ours
  for ( ; line != line_end; ++line, aState.AdvanceToNextLine()) {
#ifdef DEBUG
    if (gNoisyReflow) {
      nsRect lca(line->GetCombinedArea());
      IndentBy(stdout, gNoiseIndent);
      printf("line=%p mY=%d dirty=%s oldBounds={%d,%d,%d,%d} oldCombinedArea={%d,%d,%d,%d} deltaY=%d mPrevBottomMargin=%d\n",
             NS_STATIC_CAST(void*, line.get()), aState.mY,
             line->IsDirty() ? "yes" : "no",
             line->mBounds.x, line->mBounds.y,
             line->mBounds.width, line->mBounds.height,
             lca.x, lca.y, lca.width, lca.height,
             deltaY, aState.mPrevBottomMargin.get());
      gNoiseIndent++;
    }
#endif

    // This really sucks, but we have to look inside any blocks that have clear
    // elements inside them.
    // XXX what can we do smarter here?
    if (line->IsBlock() &&
        (line->mFirstChild->GetStateBits() & NS_BLOCK_HAS_CLEAR_CHILDREN)) {
      line->MarkDirty();
    }

    // We have to reflow the line if it's a block whose clearance
    // might have changed, so detect that.
    if (!line->IsDirty() && line->GetBreakTypeBefore() != NS_STYLE_CLEAR_NONE) {
      nscoord curY = aState.mY;
      // See where we would be after applying any clearance due to
      // BRs.
      if (inlineFloatBreakType != NS_STYLE_CLEAR_NONE) {
        curY = aState.ClearFloats(curY, inlineFloatBreakType);
      }

      nscoord newY = aState.ClearFloats(curY, line->GetBreakTypeBefore());
      
      if (line->HasClearance()) {
        // Reflow the line if it might not have clearance anymore.
        if (newY == curY
            // aState.mY is the clearance point which should be the
            // top border-edge of the block frame. If sliding the
            // block by deltaY isn't going to put it in the predicted
            // position, then we'd better reflow the line.
            || newY != line->mBounds.y + deltaY) {
          line->MarkDirty();
        }
      } else {
        // Reflow the line if the line might have clearance now.
        if (curY != newY) {
          line->MarkDirty();
        }
      }
    }

    // We might have to reflow a line that is after a clearing BR.
    if (inlineFloatBreakType != NS_STYLE_CLEAR_NONE) {
      aState.mY = aState.ClearFloats(aState.mY, inlineFloatBreakType);
      if (aState.mY != line->mBounds.y + deltaY) {
        // SlideLine is not going to put the line where the clearance
        // put it. Reflow the line to be sure.
        line->MarkDirty();
      }
      inlineFloatBreakType = NS_STYLE_CLEAR_NONE;
    }

    // Make sure |aState.mPrevBottomMargin| is at the correct position
    // before calling PropagateFloatDamage.
    if (needToRecoverState &&
        (line->IsDirty() || line->IsPreviousMarginDirty())) {
      // We need to reconstruct the bottom margin only if we didn't
      // reflow the previous line and we do need to reflow (or repair
      // the top position of) the next line.
      aState.ReconstructMarginAbove(line);
    }

    PRBool previousMarginWasDirty = line->IsPreviousMarginDirty();
    if (previousMarginWasDirty) {
      // If the previous margin is dirty, reflow the current line
      line->MarkDirty();
      line->ClearPreviousMarginDirty();
    } else if (line->mBounds.YMost() + deltaY > aState.mBottomEdge) {
      // Lines that aren't dirty but get slid past our height constraint must
      // be reflowed.
      line->MarkDirty();
    } else if (!line->IsDirty()) {
      // See if there's any reflow damage that requires that we mark the
      // line dirty.
      PropagateFloatDamage(aState, line, deltaY);
    }

    if (needToRecoverState) {
      needToRecoverState = PR_FALSE;

      // Update aState.mPrevChild as if we had reflowed all of the frames in
      // this line.  This is expensive in some cases, since it requires
      // walking |GetNextSibling|.
      if (line->IsDirty())
        aState.mPrevChild = line.prev()->LastChild();
    }

    // Now repair the line and update |aState.mY| by calling
    // |ReflowLine| or |SlideLine|.
    if (line->IsDirty()) {
      lastLineMovedUp = PR_TRUE;

      PRBool maybeReflowingForFirstTime =
        line->mBounds.x == 0 && line->mBounds.y == 0 &&
        line->mBounds.width == 0 && line->mBounds.height == 0;

      // Compute the dirty lines "before" YMost, after factoring in
      // the running deltaY value - the running value is implicit in
      // aState.mY.
      nscoord oldY = line->mBounds.y;
      nscoord oldYMost = line->mBounds.YMost();

      // Reflow the dirty line. If it's an incremental reflow, then force
      // it to invalidate the dirty area if necessary
      rv = ReflowLine(aState, line, &keepGoing, doInvalidate);
      if (NS_FAILED(rv)) {
        return rv;
      }
      if (!keepGoing) {
#ifdef DEBUG
        if (gNoisyReflow) {
          gNoiseIndent--;
          nsRect lca(line->GetCombinedArea());
          IndentBy(stdout, gNoiseIndent);
          printf("line=%p mY=%d newBounds={%d,%d,%d,%d} newCombinedArea={%d,%d,%d,%d} deltaY=%d mPrevBottomMargin=%d childCount=%d\n",
                 NS_STATIC_CAST(void*, line.get()), aState.mY,
                 line->mBounds.x, line->mBounds.y,
                 line->mBounds.width, line->mBounds.height,
                 lca.x, lca.y, lca.width, lca.height,
                 deltaY, aState.mPrevBottomMargin.get(),
                 line->GetChildCount());
        }
#endif
        if (0 == line->GetChildCount()) {
          DeleteLine(aState, line, line_end);
        }
        break;
      }

      if (line.next() != end_lines() &&
          // Check if the line might be empty, so the previous dirty
          // margin might carry through to the next line.
          ((line->mBounds.height == 0 && previousMarginWasDirty) ||
           // Check if the current line might have just been reflowed
           // for the first time.
           maybeReflowingForFirstTime ||
           // Check if the current line might have been tested in a
           // subsequent line's ShouldApplyTopMargin
           (oldY == 0 && line->mBounds.y == 0 &&
            // EXCEPT if it wasn't empty before and it wasn't empty
            // now, nothing has changed that could affect
            // ShouldApplyTopMargin
            !(oldYMost != 0 && line->mBounds.height != 0)))) {
        // In all these cases we must mark the the previous margin of
        // the next line dirty.
        line.next()->MarkPreviousMarginDirty();
        // since it's marked dirty, nobody will care about |deltaY|
      }

      // If the line was just reflowed for the first time, then its
      // old mBounds cannot be trusted so this deltaY computation is
      // bogus. But that's OK because we just did
      // MarkPreviousMarginDirty on the next line which will force it
      // to be reflowed, so this computation of deltaY will not be
      // used.
      deltaY = line->mBounds.YMost() - oldYMost;
    } else {
      lastLineMovedUp = deltaY < 0;

      if (deltaY != 0)
        SlideLine(aState, line, deltaY);
      else
        repositionViews = PR_TRUE;

      // XXX EVIL O(N^2) EVIL
      aState.RecoverStateFrom(line, deltaY);

      // Keep mY up to date in case we're propagating reflow damage
      // and also because our final height may depend on it. If the
      // line is inlines, then only update mY if the line is not
      // empty, because that's what PlaceLine does. (Empty blocks may
      // want to update mY, e.g. if they have clearance.)
      if (line->IsBlock() || !line->CachedIsEmpty()) {
        aState.mY = line->mBounds.YMost();

        if (aState.GetFlag(BRS_SHRINKWRAPWIDTH)) {
          // Mark the line as dirty so once we known the final shrink
          // wrap width we can reflow the line to the correct size.
          // It's OK to skip doing this for empty lines of inlines.
          // XXX We don't always need to do this...
          // XXX For inlines, we could record in the line box
          // that HorzontalAlignFrames does not depend on the line width,
          // and thus we don't have to mark it dirty here
          line->MarkDirty();
          aState.SetFlag(BRS_NEEDRESIZEREFLOW, PR_TRUE);
        }
      }

      // Record if we need to clear floats before reflowing the next
      // line. Note that inlineFloatBreakType will be handled and
      // cleared before the next line is processed, so there is no
      // need to combine break types here.
      if (line->HasFloatBreakAfter()) {
        inlineFloatBreakType = line->GetBreakTypeAfter();
      }

      needToRecoverState = PR_TRUE;
    }

    if (LineHasClear(line.get())) {
      foundAnyClears = PR_TRUE;
    }

#ifdef DEBUG
    if (gNoisyReflow) {
      gNoiseIndent--;
      nsRect lca(line->GetCombinedArea());
      IndentBy(stdout, gNoiseIndent);
      printf("line=%p mY=%d newBounds={%d,%d,%d,%d} newCombinedArea={%d,%d,%d,%d} deltaY=%d mPrevBottomMargin=%d\n",
             NS_STATIC_CAST(void*, line.get()), aState.mY,
             line->mBounds.x, line->mBounds.y,
             line->mBounds.width, line->mBounds.height,
             lca.x, lca.y, lca.width, lca.height,
             deltaY, aState.mPrevBottomMargin.get());
    }
#endif
  }

  if (needToRecoverState) {
    // Is this expensive?
    aState.ReconstructMarginAbove(line);

    // Update aState.mPrevChild as if we had reflowed all of the frames in
    // the last line.  This is expensive in some cases, since it requires
    // walking |GetNextSibling|.
    aState.mPrevChild = line.prev()->LastChild();
  }

  // Should we really have to do this?
  if (repositionViews)
    ::PlaceFrameView(aState.mPresContext, this);

  // We can skip trying to pull up the next line if there is no next
  // in flow or if the next in flow is not changing and we cannot have
  // added more space for its first line to be pulled up into.
  if (!aState.mNextInFlow ||
      (aState.mReflowState.mFlags.mNextInFlowUntouched && !lastLineMovedUp)) {
    if (aState.mNextInFlow) {
      aState.mReflowStatus |= NS_FRAME_NOT_COMPLETE;
    }
  } else {
    // Pull data from a next-in-flow if there's still room for more
    // content here.
    while (keepGoing && (nsnull != aState.mNextInFlow)) {
      // Grab first line from our next-in-flow
      nsBlockFrame* nextInFlow = aState.mNextInFlow;
      line_iterator nifLine = nextInFlow->begin_lines();
      nsLineBox *toMove;
      PRBool collectOverflowFloats;
      if (nifLine != nextInFlow->end_lines()) {
        toMove = nifLine;
        nextInFlow->mLines.erase(nifLine);
        collectOverflowFloats = PR_FALSE;
      } else {
        // Grab an overflow line if there are any
        nsLineList* overflowLines = nextInFlow->RemoveOverflowLines();
        if (!overflowLines) {
          aState.mNextInFlow =
            NS_STATIC_CAST(nsBlockFrame*, nextInFlow->mNextInFlow);
          continue;
        }
        nifLine = overflowLines->begin();
        NS_ASSERTION(nifLine != overflowLines->end(),
                     "Stored overflow line list should not be empty");
        toMove = nifLine;
        nifLine = overflowLines->erase(nifLine);
        if (nifLine != overflowLines->end()) {
          // We need to this remove-and-put-back dance because we want
          // to avoid making the overflow line list empty while it's
          // stored in the property (because the property has the
          // invariant that the list is never empty).
          nextInFlow->SetOverflowLines(overflowLines);
        }
        collectOverflowFloats = PR_TRUE;
      }

      if (0 == toMove->GetChildCount()) {
        // The line is empty. Try the next one.
        NS_ASSERTION(nsnull == toMove->mFirstChild, "bad empty line");
        aState.FreeLineBox(toMove);
        continue;
      }

      // XXX move to a subroutine: run-in, overflow, pullframe and this do this
      // Make the children in the line ours.
      nsIFrame* frame = toMove->mFirstChild;
      nsIFrame* lastFrame = nsnull;
      PRInt32 n = toMove->GetChildCount();
      while (--n >= 0) {
        ReparentFrame(frame, nextInFlow, this);
        lastFrame = frame;
        frame = frame->GetNextSibling();
      }
      lastFrame->SetNextSibling(nsnull);

      // Add line to our line list
      if (aState.mPrevChild) {
        aState.mPrevChild->SetNextSibling(toMove->mFirstChild);
      }

      line = mLines.before_insert(end_lines(), toMove);

      // Reparent floats whose placeholders are in the line. We need
      // to do this differently depending on whether the line is an
      // overflow line or not.
      if (collectOverflowFloats) {
        ReparentFloats(line->mFirstChild, nextInFlow, PR_TRUE);
      } else {
        // If line contains floats, remove them from aState.mNextInFlow's
        // float list. They will be pushed onto this blockframe's float
        // list, via BuildFloatList(), when we are done reflowing dirty lines.
        //
        // XXX: If the call to BuildFloatList() is removed from
        //      nsBlockFrame::Reflow(), we'll probably need to manually
        //      append the floats to |this|'s float list.
        //
        // We don't need to fix the line's float cache because we're
        // next going to reflow the line, which will wipe and
        // rebuild the float cache.
        if (line->HasFloats()) {
          nsFloatCache* fc = line->GetFirstFloat();
          while (fc) {
            if (fc->mPlaceholder) {
              nsIFrame* floatFrame = fc->mPlaceholder->GetOutOfFlowFrame();
              if (floatFrame) {
                nextInFlow->mFloats.RemoveFrame(floatFrame);
                ReparentFrame(floatFrame, nextInFlow, this);
              }
            }
            fc = fc->Next();
          }
        }
      }

      // Now reflow it and any lines that it makes during it's reflow
      // (we have to loop here because reflowing the line may case a new
      // line to be created; see SplitLine's callers for examples of
      // when this happens).
      while (line != end_lines()) {
        rv = ReflowLine(aState, line, &keepGoing, doInvalidate);
        if (NS_FAILED(rv)) {
          NS_WARNING("Line reflow failed");
          return rv;
        }
        if (!keepGoing) {
#ifdef DEBUG
          if (gNoisyReflow) {
            gNoiseIndent--;
            nsRect lca(line->GetCombinedArea());
            IndentBy(stdout, gNoiseIndent);
            printf("line=%p mY=%d newBounds={%d,%d,%d,%d} newCombinedArea={%d,%d,%d,%d} deltaY=%d mPrevBottomMargin=%d childCount=%d\n",
                   NS_STATIC_CAST(void*, line.get()), aState.mY,
                   line->mBounds.x, line->mBounds.y,
                   line->mBounds.width, line->mBounds.height,
                   lca.x, lca.y, lca.width, lca.height,
                   deltaY, aState.mPrevBottomMargin.get(),
                   line->GetChildCount());
          }
#endif
          if (0 == line->GetChildCount()) {
            DeleteLine(aState, line, line_end);
          }
          break;
        }

        if (LineHasClear(line.get())) {
          foundAnyClears = PR_TRUE;
        }

        // If this is an inline frame then its time to stop
        ++line;
        aState.AdvanceToNextLine();
      }
    }

    if (NS_FRAME_IS_NOT_COMPLETE(aState.mReflowStatus)) {
      aState.mReflowStatus |= NS_FRAME_REFLOW_NEXTINFLOW;
    }
  }

  // Handle an odd-ball case: a list-item with no lines
  if (mBullet && HaveOutsideBullet() && mLines.empty()) {
    nsHTMLReflowMetrics metrics(nsnull);
    ReflowBullet(aState, metrics);

    // There are no lines so we have to fake up some y motion so that
    // we end up with *some* height.
    aState.mY += metrics.height;
  }

  if (foundAnyClears) {
    AddStateBits(NS_BLOCK_HAS_CLEAR_CHILDREN);
  } else {
    RemoveStateBits(NS_BLOCK_HAS_CLEAR_CHILDREN);
  }

#ifdef DEBUG
  if (gNoisyReflow) {
    gNoiseIndent--;
    IndentBy(stdout, gNoiseIndent);
    ListTag(stdout);
    printf(": done reflowing dirty lines (status=%x)\n",
           aState.mReflowStatus);
  }
#endif

  return rv;
}

void
nsBlockFrame::DeleteLine(nsBlockReflowState& aState,
                         nsLineList::iterator aLine,
                         nsLineList::iterator aLineEnd)
{
  NS_PRECONDITION(0 == aLine->GetChildCount(), "can't delete !empty line");
  if (0 == aLine->GetChildCount()) {
    NS_ASSERTION(aState.mCurrentLine == aLine,
                 "using function more generally than designed, "
                 "but perhaps OK now");
    nsLineBox *line = aLine;
    aLine = mLines.erase(aLine);
    aState.FreeLineBox(line);
    // Mark the previous margin of the next line dirty since we need to
    // recompute its top position.
    if (aLine != aLineEnd)
      aLine->MarkPreviousMarginDirty();
  }
}

/**
 * Takes two rectangles whose origins must be the same, and computes
 * the difference between their union and their intersection as two
 * rectangles. (This difference is a superset of the difference
 * between the two rectangles.)
 */
static void GetRectDifferenceStrips(const nsRect& aR1, const nsRect& aR2,
                                    nsRect* aHStrip, nsRect* aVStrip) {
  NS_ASSERTION(aR1.TopLeft() == aR2.TopLeft(),
               "expected rects at the same position");
  nsRect unionRect(aR1.x, aR1.y, PR_MAX(aR1.width, aR2.width),
                   PR_MAX(aR1.height, aR2.height));
  nscoord VStripStart = PR_MIN(aR1.width, aR2.width);
  nscoord HStripStart = PR_MIN(aR1.height, aR2.height);
  *aVStrip = unionRect;
  aVStrip->x += VStripStart;
  aVStrip->width -= VStripStart;
  *aHStrip = unionRect;
  aHStrip->y += HStripStart;
  aHStrip->height -= HStripStart;
}

/**
 * Reflow a line. The line will either contain a single block frame
 * or contain 1 or more inline frames. aKeepReflowGoing indicates
 * whether or not the caller should continue to reflow more lines.
 */
nsresult
nsBlockFrame::ReflowLine(nsBlockReflowState& aState,
                         line_iterator aLine,
                         PRBool* aKeepReflowGoing,
                         PRBool aDamageDirtyArea)
{
  nsresult rv = NS_OK;

  NS_ABORT_IF_FALSE(aLine->GetChildCount(), "reflowing empty line");

  // Setup the line-layout for the new line
  aState.mCurrentLine = aLine;
  aLine->ClearDirty();
  aLine->InvalidateCachedIsEmpty();
  
  // Now that we know what kind of line we have, reflow it
  if (aLine->IsBlock()) {
    nsRect oldBounds = aLine->mFirstChild->GetRect();
    nsRect oldCombinedArea(aLine->GetCombinedArea());
    rv = ReflowBlockFrame(aState, aLine, aKeepReflowGoing);
    nsRect newBounds = aLine->mFirstChild->GetRect();

    // We expect blocks to damage any area inside their bounds that is
    // dirty; however, if the frame changes size or position then we
    // need to do some repainting.
    // XXX roc --- the above statement is ambiguous about whether 'bounds'
    // means the frame's bounds or overflowArea, and in fact this is a source
    // of much confusion and bugs. Thus the following hack considers *both*
    // overflowArea and bounds. This should be considered a temporary hack
    // until we decide how it's really supposed to work.
    if (aDamageDirtyArea) {
      nsRect lineCombinedArea(aLine->GetCombinedArea());
      if (oldCombinedArea.TopLeft() != lineCombinedArea.TopLeft() ||
          oldBounds.TopLeft() != newBounds.TopLeft()) {
        // The block has moved, and so to be safe we need to repaint
        // XXX We need to improve on this...
        nsRect  dirtyRect;
        dirtyRect.UnionRect(oldCombinedArea, lineCombinedArea);
#ifdef NOISY_BLOCK_INVALIDATE
        printf("%p invalidate 6 (%d, %d, %d, %d)\n",
               this, dirtyRect.x, dirtyRect.y, dirtyRect.width, dirtyRect.height);
#endif
        Invalidate(dirtyRect);
      } else {
        nsRect combinedAreaHStrip, combinedAreaVStrip;
        nsRect boundsHStrip, boundsVStrip;
        GetRectDifferenceStrips(oldBounds, newBounds,
                                &boundsHStrip, &boundsVStrip);
        GetRectDifferenceStrips(oldCombinedArea, lineCombinedArea,
                                &combinedAreaHStrip, &combinedAreaVStrip);

#ifdef NOISY_BLOCK_INVALIDATE
        printf("%p invalidate boundsVStrip (%d, %d, %d, %d)\n",
               this, boundsVStrip.x, boundsVStrip.y, boundsVStrip.width, boundsVStrip.height);
        printf("%p invalidate boundsHStrip (%d, %d, %d, %d)\n",
               this, boundsHStrip.x, boundsHStrip.y, boundsHStrip.width, boundsHStrip.height);
        printf("%p invalidate combinedAreaVStrip (%d, %d, %d, %d)\n",
               this, combinedAreaVStrip.x, combinedAreaVStrip.y, combinedAreaVStrip.width, combinedAreaVStrip.height);
        printf("%p invalidate combinedAreaHStrip (%d, %d, %d, %d)\n",
               this, combinedAreaHStrip.x, combinedAreaHStrip.y, combinedAreaHStrip.width, combinedAreaHStrip.height);
#endif
        // The first thing Invalidate does is check if the rect is empty, so
        // don't bother doing that here.
        Invalidate(boundsVStrip);
        Invalidate(boundsHStrip);
        Invalidate(combinedAreaVStrip);
        Invalidate(combinedAreaHStrip);
      }
    }
  }
  else {
    nsRect oldCombinedArea(aLine->GetCombinedArea());
    aLine->SetLineWrapped(PR_FALSE);

    // If we're supposed to update the maximum width, then we'll need to reflow
    // the line with an unconstrained width (which will give us the new maximum
    // width), then we'll reflow it again with the constrained width.
    // We only do this if this is a beginning line, i.e., don't do this for
    // lines associated with content that line wrapped (see ReflowDirtyLines()
    // for details).
    // XXX This approach doesn't work when floats are involved in which case
    // we'll either need to recover the float state that applies to the
    // unconstrained reflow or keep it around in a separate space manager...
    PRBool isBeginningLine = aState.mCurrentLine == begin_lines() ||
                             !aState.mCurrentLine.prev()->IsLineWrapped();
    // XXXldb Add &&!aState.GetFlag(BRS_UNCONSTRAINEDWIDTH)
    if (aState.GetFlag(BRS_COMPUTEMAXWIDTH) && isBeginningLine) {
      // First reflow the line with an unconstrained width. 
      nscoord oldY = aState.mY;
      nsCollapsingMargin oldPrevBottomMargin(aState.mPrevBottomMargin);
      PRBool  oldUnconstrainedWidth = aState.GetFlag(BRS_UNCONSTRAINEDWIDTH);

#if defined(DEBUG_waterson) || defined(DEBUG_dbaron)
      // XXXwaterson if oldUnconstrainedWidth was set, why do we need
      // to do the second reflow, below?

      if (oldUnconstrainedWidth)
        printf("*** oldUnconstrainedWidth was already set.\n"
               "*** This code (%s:%d) could be optimized a lot!\n"
               "+++ possibly doing an unnecessary second-pass unconstrained "
               "reflow\n",
               __FILE__, __LINE__);
#endif

      // When doing this we need to set the block reflow state's
      // "mUnconstrainedWidth" variable to PR_TRUE so if we encounter
      // a placeholder and then reflow its associated float we don't
      // end up resetting the line's right edge and have it think the
      // width is unconstrained...
      aState.mSpaceManager->PushState();
      aState.SetFlag(BRS_UNCONSTRAINEDWIDTH, PR_TRUE);
      ReflowInlineFrames(aState, aLine, aKeepReflowGoing, aDamageDirtyArea, PR_TRUE);
      aState.mY = oldY;
      aState.mPrevBottomMargin = oldPrevBottomMargin;
      aState.SetFlag(BRS_UNCONSTRAINEDWIDTH, oldUnconstrainedWidth);
      aState.mSpaceManager->PopState();

      // Update the line's maximum width
      aLine->mMaximumWidth = aLine->mBounds.XMost();
#ifdef NOISY_MAXIMUM_WIDTH
      printf("nsBlockFrame::ReflowLine block %p line %p setting aLine.mMaximumWidth to %d\n", 
             this, NS_STATIC_CAST(void*, aLine.get()), aLine->mMaximumWidth);
#endif
      aState.UpdateMaximumWidth(aLine->mMaximumWidth);

      // Now reflow the line again this time without having it compute
      // the maximum width.
      // We leave whether to compute max-element-width as-is, because
      // making this call throws out the previous max-element-width
      // information stored in the float cache.
      nscoord oldComputeMaximumWidth = aState.GetFlag(BRS_COMPUTEMAXWIDTH);

      aState.SetFlag(BRS_COMPUTEMAXWIDTH, PR_FALSE);
      rv = ReflowInlineFrames(aState, aLine, aKeepReflowGoing, aDamageDirtyArea);
      aState.SetFlag(BRS_COMPUTEMAXWIDTH, oldComputeMaximumWidth);

    } else {
      rv = ReflowInlineFrames(aState, aLine, aKeepReflowGoing, aDamageDirtyArea);
      if (NS_SUCCEEDED(rv))
      {
        if (aState.GetFlag(BRS_COMPUTEMAXWIDTH))
        {
#ifdef NOISY_MAXIMUM_WIDTH
          printf("nsBlockFrame::ReflowLine block %p line %p setting aLine.mMaximumWidth to %d\n", 
                 this, NS_STATIC_CAST(void*, aLine.get()), aLine->mMaximumWidth);
#endif
          aState.UpdateMaximumWidth(aLine->mMaximumWidth);
        }
        if (aState.GetFlag(BRS_COMPUTEMAXELEMENTWIDTH))
        {
#ifdef DEBUG
          if (gNoisyMaxElementWidth) {
            IndentBy(stdout, gNoiseIndent);
            printf("nsBlockFrame::ReflowLine block %p line %p setting aLine.mMaxElementWidth to %d\n", 
                   NS_STATIC_CAST(void*, this), NS_STATIC_CAST(void*, aLine.get()),
                   aLine->mMaxElementWidth);
          }
#endif
          aState.UpdateMaxElementWidth(aLine->mMaxElementWidth);
        }
      }
    }

    // We don't really know what changed in the line, so use the union
    // of the old and new combined areas
    if (aDamageDirtyArea) {
      nsRect dirtyRect;
      dirtyRect.UnionRect(oldCombinedArea, aLine->GetCombinedArea());
#ifdef NOISY_BLOCK_INVALIDATE
      printf("%p invalidate because %s is true (%d, %d, %d, %d)\n",
             this, aDamageDirtyArea ? "aDamageDirtyArea" : "aLine->IsForceInvalidate",
             dirtyRect.x, dirtyRect.y, dirtyRect.width, dirtyRect.height);
      if (aLine->IsForceInvalidate())
        printf("  dirty line is %p\n", NS_STATIC_CAST(void*, aLine.get());
#endif
      Invalidate(dirtyRect);
    }
  }

  return rv;
}

/**
 * Pull frame from the next available location (one of our lines or
 * one of our next-in-flows lines).
 */
nsresult
nsBlockFrame::PullFrame(nsBlockReflowState& aState,
                        line_iterator aLine,
                        PRBool aDamageDeletedLines,
                        nsIFrame*& aFrameResult)
{
  aFrameResult = nsnull;

  // First check our remaining lines
  if (end_lines() != aLine.next()) {
    return PullFrameFrom(aState, aLine, this, PR_FALSE, aLine.next(),
                         aDamageDeletedLines, aFrameResult);
  }

  NS_ASSERTION(!GetOverflowLines(),
    "Our overflow lines should have been removed at the start of reflow");

  // Try each next in flows
  nsBlockFrame* nextInFlow = aState.mNextInFlow;
  while (nextInFlow) {
    // first normal lines, then overflow lines
    if (!nextInFlow->mLines.empty()) {
      return PullFrameFrom(aState, aLine, nextInFlow, PR_FALSE,
                           nextInFlow->mLines.begin(),
                           aDamageDeletedLines, aFrameResult);
    }

    nsLineList* overflowLines = nextInFlow->GetOverflowLines();
    if (overflowLines) {
      return PullFrameFrom(aState, aLine, nextInFlow, PR_TRUE,
                           overflowLines->begin(),
                           aDamageDeletedLines, aFrameResult);
    }

    nextInFlow = (nsBlockFrame*) nextInFlow->mNextInFlow;
    aState.mNextInFlow = nextInFlow;
  }

  return NS_OK;
}

/**
 * Try to pull a frame out of a line pointed at by aFromLine. If a frame
 * is pulled then aPulled will be set to PR_TRUE.  In addition, if
 * aUpdateGeometricParent is set then the pulled frames geometric parent
 * will be updated (e.g. when pulling from a next-in-flows line list).
 *
 * Note: pulling a frame from a line that is a place-holder frame
 * doesn't automatically remove the corresponding float from the
 * line's float array. This happens indirectly: either the line gets
 * emptied (and destroyed) or the line gets reflowed (because we mark
 * it dirty) and the code at the top of ReflowLine empties the
 * array. So eventually, it will be removed, just not right away.
 */
nsresult
nsBlockFrame::PullFrameFrom(nsBlockReflowState& aState,
                            nsLineBox* aLine,
                            nsBlockFrame* aFromContainer,
                            PRBool aFromOverflowLine,
                            nsLineList::iterator aFromLine,
                            PRBool aDamageDeletedLines,
                            nsIFrame*& aFrameResult)
{
  nsLineBox* fromLine = aFromLine;
  NS_ABORT_IF_FALSE(fromLine, "bad line to pull from");
  NS_ABORT_IF_FALSE(fromLine->GetChildCount(), "empty line");
  NS_ABORT_IF_FALSE(aLine->GetChildCount(), "empty line");

  if (fromLine->IsBlock()) {
    // If our line is not empty and the child in aFromLine is a block
    // then we cannot pull up the frame into this line. In this case
    // we stop pulling.
    aFrameResult = nsnull;
  }
  else {
    // Take frame from fromLine
    nsIFrame* frame = fromLine->mFirstChild;
    aLine->SetChildCount(aLine->GetChildCount() + 1);

    PRInt32 fromLineChildCount = fromLine->GetChildCount();
    if (0 != --fromLineChildCount) {
      // Mark line dirty now that we pulled a child
      fromLine->SetChildCount(fromLineChildCount);
      fromLine->MarkDirty();
      fromLine->mFirstChild = frame->GetNextSibling();
    }
    else {
      // Free up the fromLine now that it's empty
      // Its bounds might need to be redrawn, though.
      // XXX WHY do we invalidate the bounds AND the combined area? doesn't
      // the combined area always enclose the bounds?
      if (aDamageDeletedLines) {
        Invalidate(fromLine->mBounds);
      }
      nsLineList* fromLineList = aFromOverflowLine
        ? aFromContainer->RemoveOverflowLines()
        : &aFromContainer->mLines;
      if (aFromLine.next() != fromLineList->end())
        aFromLine.next()->MarkPreviousMarginDirty();

      Invalidate(fromLine->GetCombinedArea());
      fromLineList->erase(aFromLine);
      // Note that aFromLine just got incremented, so don't use it again here!
      aState.FreeLineBox(fromLine);

      // Put any remaining overflow lines back.
      if (aFromOverflowLine && !fromLineList->empty()) {
        aFromContainer->SetOverflowLines(fromLineList);
      }
    }

    // Change geometric parents
    if (aFromContainer != this) {
      // When pushing and pulling frames we need to check for whether any
      // views need to be reparented
      NS_ASSERTION(frame->GetParent() == aFromContainer, "unexpected parent frame");

      ReparentFrame(frame, aFromContainer, this);

      // The frame is being pulled from a next-in-flow; therefore we
      // need to add it to our sibling list.
      frame->SetNextSibling(nsnull);
      if (nsnull != aState.mPrevChild) {
        aState.mPrevChild->SetNextSibling(frame);
      }

      // The frame might have (or contain) floats that need to be
      // brought over too.
      ReparentFloats(frame, aFromContainer, aFromOverflowLine);
    }

    // Stop pulling because we found a frame to pull
    aFrameResult = frame;
#ifdef DEBUG
    VerifyLines(PR_TRUE);
#endif
  }
  return NS_OK;
}

static void
PlaceFrameView(nsPresContext* aPresContext,
               nsIFrame*       aFrame)
{
  if (aFrame->HasView())
    nsContainerFrame::PositionFrameView(aPresContext, aFrame);
  else
    nsContainerFrame::PositionChildViews(aPresContext, aFrame);
}

void
nsBlockFrame::SlideLine(nsBlockReflowState& aState,
                        nsLineBox* aLine, nscoord aDY)
{
  NS_PRECONDITION(aDY != 0, "why slide a line nowhere?");

  Invalidate(aLine->GetCombinedArea());
  // Adjust line state
  aLine->SlideBy(aDY);
  Invalidate(aLine->GetCombinedArea());

  // Adjust the frames in the line
  nsIFrame* kid = aLine->mFirstChild;
  if (!kid) {
    return;
  }

  if (aLine->IsBlock()) {
    if (aDY) {
      nsPoint p = kid->GetPosition();
      p.y += aDY;
      kid->SetPosition(p);
    }

    // Make sure the frame's view and any child views are updated
    ::PlaceFrameView(aState.mPresContext, kid);
  }
  else {
    // Adjust the Y coordinate of the frames in the line.
    // Note: we need to re-position views even if aDY is 0, because
    // one of our parent frames may have moved and so the view's position
    // relative to its parent may have changed
    PRInt32 n = aLine->GetChildCount();
    while (--n >= 0) {
      if (aDY) {
        nsPoint p = kid->GetPosition();
        p.y += aDY;
        kid->SetPosition(p);
      }
      // Make sure the frame's view and any child views are updated
      ::PlaceFrameView(aState.mPresContext, kid);
      kid = kid->GetNextSibling();
    }
  }
}

NS_IMETHODIMP 
nsBlockFrame::AttributeChanged(nsPresContext* aPresContext,
                               nsIContent*     aChild,
                               PRInt32         aNameSpaceID,
                               nsIAtom*        aAttribute,
                               PRInt32         aModType)
{
  nsresult rv = nsBlockFrameSuper::AttributeChanged(aPresContext, aChild,
                                                    aNameSpaceID, aAttribute,
                                                    aModType);

  if (NS_FAILED(rv)) {
    return rv;
  }
  if (nsHTMLAtoms::start == aAttribute) {
    // XXX Not sure if this is necessary anymore
    RenumberLists(aPresContext);

    nsHTMLReflowCommand* reflowCmd;
    rv = NS_NewHTMLReflowCommand(&reflowCmd, this,
                                 eReflowType_ContentChanged,
                                 nsnull,
                                 aAttribute);
    if (NS_SUCCEEDED(rv))
      aPresContext->PresShell()->AppendReflowCommand(reflowCmd);
  }
  else if (nsHTMLAtoms::value == aAttribute) {
    const nsStyleDisplay* styleDisplay = GetStyleDisplay();
    if (NS_STYLE_DISPLAY_LIST_ITEM == styleDisplay->mDisplay) {
      nsIFrame* nextAncestor = mParent;
      nsBlockFrame* blockParent = nsnull;
      
      // Search for the closest ancestor that's a block frame. We
      // make the assumption that all related list items share a
      // common block parent.
      // XXXldb I think that's a bad assumption.
      while (nextAncestor) {
        if (NS_OK == nextAncestor->QueryInterface(kBlockFrameCID, 
                                                  (void**)&blockParent)) {
          break;
        }
        nextAncestor = nextAncestor->GetParent();
      }

      // Tell the enclosing block frame to renumber list items within
      // itself
      if (nsnull != blockParent) {
        // XXX Not sure if this is necessary anymore
        blockParent->RenumberLists(aPresContext);

        nsHTMLReflowCommand* reflowCmd;
        rv = NS_NewHTMLReflowCommand(&reflowCmd, blockParent,
                                     eReflowType_ContentChanged,
                                     nsnull,
                                     aAttribute);
        if (NS_SUCCEEDED(rv))
          aPresContext->PresShell()->AppendReflowCommand(reflowCmd);
      }
    }
  }

  return rv;
}

inline PRBool
IsBorderZero(nsStyleUnit aUnit, nsStyleCoord &aCoord)
{
    return ((aUnit == eStyleUnit_Coord && aCoord.GetCoordValue() == 0));
}

inline PRBool
IsPaddingZero(nsStyleUnit aUnit, nsStyleCoord &aCoord)
{
    return (aUnit == eStyleUnit_Null ||
            (aUnit == eStyleUnit_Coord && aCoord.GetCoordValue() == 0) ||
            (aUnit == eStyleUnit_Percent && aCoord.GetPercentValue() == 0.0));
}

inline PRBool
IsMarginZero(nsStyleUnit aUnit, nsStyleCoord &aCoord)
{
    return (aUnit == eStyleUnit_Null ||
            aUnit == eStyleUnit_Auto ||
            (aUnit == eStyleUnit_Coord && aCoord.GetCoordValue() == 0) ||
            (aUnit == eStyleUnit_Percent && aCoord.GetPercentValue() == 0.0));
}

/* virtual */ PRBool
nsBlockFrame::IsSelfEmpty()
{
  const nsStylePosition* position = GetStylePosition();

  switch (position->mMinHeight.GetUnit()) {
    case eStyleUnit_Coord:
      if (position->mMinHeight.GetCoordValue() != 0)
        return PR_FALSE;
      break;
    case eStyleUnit_Percent:
      if (position->mMinHeight.GetPercentValue() != 0.0f)
        return PR_FALSE;
      break;
    default:
      return PR_FALSE;
  }

  switch (position->mHeight.GetUnit()) {
    case eStyleUnit_Auto:
      break;
    case eStyleUnit_Coord:
      if (position->mHeight.GetCoordValue() != 0)
        return PR_FALSE;
      break;
    case eStyleUnit_Percent:
      if (position->mHeight.GetPercentValue() != 0.0f)
        return PR_FALSE;
      break;
    default:
      return PR_FALSE;
  }

  const nsStyleBorder* border = GetStyleBorder();
  const nsStylePadding* padding = GetStylePadding();
  nsStyleCoord coord;
  if ((border->IsBorderSideVisible(NS_SIDE_TOP) &&
       !IsBorderZero(border->mBorder.GetTopUnit(),
                     border->mBorder.GetTop(coord))) ||
      (border->IsBorderSideVisible(NS_SIDE_BOTTOM) &&
       !IsBorderZero(border->mBorder.GetBottomUnit(),
                     border->mBorder.GetBottom(coord))) ||
      !IsPaddingZero(padding->mPadding.GetTopUnit(),
                    padding->mPadding.GetTop(coord)) ||
      !IsPaddingZero(padding->mPadding.GetBottomUnit(),
                    padding->mPadding.GetBottom(coord))) {
    return PR_FALSE;
  }

  return PR_TRUE;
}

PRBool
nsBlockFrame::IsEmpty()
{
  if (!IsSelfEmpty()) {
    return PR_FALSE;
  }

  for (line_iterator line = begin_lines(), line_end = end_lines();
       line != line_end;
       ++line)
  {
    if (!line->IsEmpty())
      return PR_FALSE;
  }

  return PR_TRUE;
}

PRBool
nsBlockFrame::ShouldApplyTopMargin(nsBlockReflowState& aState,
                                   nsLineBox* aLine)
{
  if (aState.GetFlag(BRS_APPLYTOPMARGIN)) {
    // Apply short-circuit check to avoid searching the line list
    return PR_TRUE;
  }

  if (!aState.IsAdjacentWithTop()) {
    // If we aren't at the top Y coordinate then something of non-zero
    // height must have been placed. Therefore the childs top-margin
    // applies.
    aState.SetFlag(BRS_APPLYTOPMARGIN, PR_TRUE);
    return PR_TRUE;
  }

  // Determine if this line is "essentially" the first line
  for (line_iterator line = begin_lines(); line != aLine; ++line) {
    if (!line->CachedIsEmpty()) {
      // A line which preceeds aLine is non-empty, so therefore the
      // top margin applies.
      aState.SetFlag(BRS_APPLYTOPMARGIN, PR_TRUE);
      return PR_TRUE;
    }
    // No need to apply the top margin if the line has floats.  We
    // should collapse anyway (bug 44419)
  }

  // The line being reflowed is "essentially" the first line in the
  // block. Therefore its top-margin will be collapsed by the
  // generational collapsing logic with its parent (us).
  return PR_FALSE;
}

nsIFrame*
nsBlockFrame::GetTopBlockChild(nsPresContext* aPresContext)
{
  if (mLines.empty())
    return nsnull;

  nsLineBox *firstLine = mLines.front();
  if (firstLine->IsBlock())
    return firstLine->mFirstChild;

  if (!firstLine->CachedIsEmpty())
    return nsnull;

  line_iterator secondLine = begin_lines();
  ++secondLine;
  if (secondLine == end_lines() || !secondLine->IsBlock())
    return nsnull;

  return secondLine->mFirstChild;
}

// If placeholders/floats split during reflowing a line, but that line will 
// be put on the next page, then put the placeholders/floats back the way
// they were before the line was reflowed. 
void
nsBlockFrame::UndoSplitPlaceholders(nsBlockReflowState& aState,
                                    nsIFrame*           aLastPlaceholder)
{
  nsIFrame* undoPlaceholder = nsnull;
  if (aLastPlaceholder) {
    undoPlaceholder = aLastPlaceholder->GetNextSibling();
    aLastPlaceholder->SetNextSibling(nsnull);
  }
  else {
    // just remove the property
    nsFrameList* overflowPlace = RemoveOverflowPlaceholders();
    delete overflowPlace;
  }
  // remove the next in flows of the placeholders that need to be removed
  for (nsIFrame* placeholder = undoPlaceholder; placeholder; ) {
    nsSplittableFrame::RemoveFromFlow(placeholder);
    nsIFrame* savePlaceholder = placeholder; 
    placeholder = placeholder->GetNextSibling();
    savePlaceholder->Destroy(aState.mPresContext);
  }
}

nsresult
nsBlockFrame::ReflowBlockFrame(nsBlockReflowState& aState,
                               line_iterator aLine,
                               PRBool* aKeepReflowGoing)
{
  NS_PRECONDITION(*aKeepReflowGoing, "bad caller");

  nsresult rv = NS_OK;

  nsIFrame* frame = aLine->mFirstChild;
  if (!frame) {
    NS_ASSERTION(PR_FALSE, "program error - unexpected empty line"); 
    return NS_ERROR_NULL_POINTER; 
  }

  // Prepare the block reflow engine
  const nsStyleDisplay* display = frame->GetStyleDisplay();
  nsBlockReflowContext brc(aState.mPresContext, aState.mReflowState,
                           aState.GetFlag(BRS_COMPUTEMAXELEMENTWIDTH),
                           aState.GetFlag(BRS_COMPUTEMAXWIDTH));

  PRUint8 breakType = display->mBreakType;
  // If a float split and its prev-in-flow was followed by a <BR>, then combine 
  // the <BR>'s break type with the block's break type (the block will be the very 
  // next frame after the split float).
  if (NS_STYLE_CLEAR_NONE != aState.mFloatBreakType) {
    breakType = nsLayoutUtils::CombineBreakType(breakType,
                                                aState.mFloatBreakType);
    aState.mFloatBreakType = NS_STYLE_CLEAR_NONE;
  }

  // Clear past floats before the block if the clear style is not none
  aLine->SetBreakTypeBefore(breakType);

  // See if we should apply the top margin. If the block frame being
  // reflowed is a continuation (non-null prev-in-flow) then we don't
  // apply its top margin because its not significant. Otherwise, dig
  // deeper.
  PRBool applyTopMargin =
    !frame->GetPrevInFlow() && ShouldApplyTopMargin(aState, aLine);

  if (applyTopMargin) {
    // The HasClearance setting is only valid if ShouldApplyTopMargin
    // returned PR_FALSE (in which case the top-margin-root set our
    // clearance flag). Otherwise clear it now. We'll set it later on
    // ourselves if necessary.
    aLine->ClearHasClearance();
  }
  PRBool treatWithClearance = aLine->HasClearance();
  // If our top margin was counted as part of some parents top-margin
  // collapse and we are being speculatively reflowed assuming this
  // frame DID NOT need clearance, then we need to check that
  // assumption.
  if (!treatWithClearance && !applyTopMargin && breakType != NS_STYLE_CLEAR_NONE &&
      aState.mReflowState.mDiscoveredClearance) {
    nscoord curY = aState.mY + aState.mPrevBottomMargin.get();
    nscoord clearY = aState.ClearFloats(curY, breakType);
    if (clearY != curY) {
      // Looks like that assumption was invalid, we do need
      // clearance. Tell our ancestor so it can reflow again. It is
      // responsible for actually setting our clearance flag before
      // the next reflow.
      treatWithClearance = PR_TRUE;
      // Only record the first frame that requires clearance
      if (!*aState.mReflowState.mDiscoveredClearance) {
        *aState.mReflowState.mDiscoveredClearance = frame;
    }
      // Exactly what we do now is flexible since we'll definitely be
      // reflowed.
    }
  }
  if (treatWithClearance) {
    applyTopMargin = PR_TRUE;
  }

  nsIFrame* clearanceFrame = nsnull;
  nscoord startingY = aState.mY;
  nsCollapsingMargin incomingMargin = aState.mPrevBottomMargin;
  while (PR_TRUE) {
    nscoord clearance = 0;
    nscoord topMargin = 0;
    PRBool mayNeedRetry = PR_FALSE;
    if (applyTopMargin) {
      // Precompute the blocks top margin value so that we can get the
      // correct available space (there might be a float that's
      // already been placed below the aState.mPrevBottomMargin

      // Setup a reflowState to get the style computed margin-top
      // value. We'll use a reason of `resize' so that we don't fudge
      // any incremental reflow state.
      
      // The availSpace here is irrelevant to our needs - all we want
      // out if this setup is the margin-top value which doesn't depend
      // on the childs available space.
      // XXX building a complete nsHTMLReflowState just to get the margin-top
      // seems like a waste. And we do this for almost every block!
      nsSize availSpace(aState.mContentArea.width, NS_UNCONSTRAINEDSIZE);
      nsHTMLReflowState reflowState(aState.mPresContext, aState.mReflowState,
                                    frame, availSpace, eReflowReason_Resize);
      
      if (treatWithClearance) {
        aState.mY += aState.mPrevBottomMargin.get();
        aState.mPrevBottomMargin.Zero();
      }
      
      // Now compute the collapsed margin-top value into aState.mPrevBottomMargin, assuming
      // that all child margins collapse down to clearanceFrame.
      nsBlockReflowContext::ComputeCollapsedTopMargin(reflowState,
                                                      &aState.mPrevBottomMargin, clearanceFrame, &mayNeedRetry);
      
      // XXX optimization; we could check the collapsing children to see if they are sure
      // to require clearance, and so avoid retrying them
      
      if (clearanceFrame) {
        // Don't allow retries on the second pass. The clearance decisions for the
        // blocks whose top-margins collapse with ours are now fixed.
        mayNeedRetry = PR_FALSE;
      }
      
      if (!treatWithClearance && !clearanceFrame && breakType != NS_STYLE_CLEAR_NONE) {
        // We don't know if we need clearance and this is the first,
        // optimistic pass.  So determine whether *this block* needs
        // clearance. Note that we do not allow the decision for whether
        // this block has clearance to change on the second pass; that
        // decision is only allowed to be made under the optimistic
        // first pass.
        nscoord curY = aState.mY + aState.mPrevBottomMargin.get();
        nscoord clearY = aState.ClearFloats(curY, breakType);
        if (clearY != curY) {
          // Looks like we need clearance and we didn't know about it already. So
          // recompute collapsed margin
          treatWithClearance = PR_TRUE;
          // Remember this decision, needed for incremental reflow
          aLine->SetHasClearance();
          
          // Apply incoming margins
          aState.mY += aState.mPrevBottomMargin.get();
          aState.mPrevBottomMargin.Zero();
          
          // Compute the collapsed margin again, ignoring the incoming margin this time
          mayNeedRetry = PR_FALSE;
          nsBlockReflowContext::ComputeCollapsedTopMargin(reflowState,
                                                          &aState.mPrevBottomMargin, clearanceFrame, &mayNeedRetry);
        }
      }
      
      // Temporarily advance the running Y value so that the
      // GetAvailableSpace method will return the right available
      // space. This undone as soon as the horizontal margins are
      // computed.
      topMargin = aState.mPrevBottomMargin.get();
      
      if (treatWithClearance) {
        nscoord currentY = aState.mY;
        // advance mY to the clear position.
        aState.mY = aState.ClearFloats(aState.mY, breakType);
        
        // Compute clearance. It's the amount we need to add to the top
        // border-edge of the frame, after applying collapsed margins
        // from the frame and its children, to get it to line up with
        // the bottom of the floats. The former is currentY + topMargin,
        // the latter is the current aState.mY.
        // Note that negative clearance is possible
        clearance = aState.mY - (currentY + topMargin);
        
        // Add clearance to our top margin while we compute available
        // space for the frame
        topMargin += clearance;
        
        // Note that aState.mY should stay where it is: at the top
        // border-edge of the frame
      } else {
        // Advance aState.mY to the top border-edge of the frame.
        aState.mY += topMargin;
      }
    }
    
    // Here aState.mY is the top border-edge of the block.
    // Compute the available space for the block
    aState.GetAvailableSpace();
#ifdef REALLY_NOISY_REFLOW
    printf("setting line %p isImpacted to %s\n", aLine, aState.IsImpactedByFloat()?"true":"false");
#endif
    PRBool isImpacted = aState.IsImpactedByFloat() ? PR_TRUE : PR_FALSE;
    aLine->SetLineIsImpactedByFloat(isImpacted);
    nsSplittableType splitType = NS_FRAME_NOT_SPLITTABLE;
    frame->IsSplittable(splitType);
    nsRect availSpace;
    aState.ComputeBlockAvailSpace(frame, splitType, display, availSpace);
    
    // Now put the Y coordinate back to the top of the top-margin +
    // clearance, and flow the block.
    aState.mY -= topMargin;
    availSpace.y -= topMargin;
    if (NS_UNCONSTRAINEDSIZE != availSpace.height) {
      availSpace.height += topMargin;
    }
    
    // keep track of the last overflow float in case we need to undo any new additions
    nsFrameList* overflowPlace = GetOverflowPlaceholders();
    nsIFrame* lastPlaceholder = (overflowPlace) ? overflowPlace->LastChild() : nsnull;
    
    // Reflow the block into the available space
    nsMargin computedOffsets;
    // construct the html reflow state for the block. ReflowBlock 
    // will initialize it
    nsHTMLReflowState blockHtmlRS(aState.mPresContext, aState.mReflowState, frame, 
                                  nsSize(availSpace.width, availSpace.height), 
                                  aState.mReflowState.reason, PR_TRUE);
    blockHtmlRS.mFlags.mHasClearance = aLine->HasClearance();
    
    if (mayNeedRetry) {
      blockHtmlRS.mDiscoveredClearance = &clearanceFrame;
      aState.mSpaceManager->PushState();
    } else if (!applyTopMargin) {
      blockHtmlRS.mDiscoveredClearance = aState.mReflowState.mDiscoveredClearance;
    }
    
    nsReflowStatus frameReflowStatus = NS_FRAME_COMPLETE;
    rv = brc.ReflowBlock(availSpace, applyTopMargin, aState.mPrevBottomMargin,
                         clearance, aState.IsAdjacentWithTop(), computedOffsets,
                         blockHtmlRS, frameReflowStatus);
    
  // Remove the frame from the reflow tree.
    if (aState.mReflowState.path)
      aState.mReflowState.path->RemoveChild(frame);
    if (NS_FAILED(rv)) {
      return rv;
    }
    
    if (mayNeedRetry) {
      if (clearanceFrame) {
        aState.mSpaceManager->PopState();
        aState.mY = startingY;
        aState.mPrevBottomMargin = incomingMargin;
        continue;
      } else {
        // pop the saved state off the stack and discard it, because we
        // want to keep the current state, since our speculation
        // succeeded
        aState.mSpaceManager->DiscardState();
      }
    }
    
    aState.mPrevChild = frame;
    
#if defined(REFLOW_STATUS_COVERAGE)
    RecordReflowStatus(PR_TRUE, frameReflowStatus);
#endif
    
    if (NS_INLINE_IS_BREAK_BEFORE(frameReflowStatus)) {
      // None of the child block fits.
      UndoSplitPlaceholders(aState, lastPlaceholder);
      PushLines(aState, aLine.prev());
      *aKeepReflowGoing = PR_FALSE;
      aState.mReflowStatus = NS_FRAME_NOT_COMPLETE;
    }
    else {
      // Note: line-break-after a block is a nop
      
      // Try to place the child block
      PRBool isAdjacentWithTop = aState.IsAdjacentWithTop();
      nsCollapsingMargin collapsedBottomMargin;
      nsRect combinedArea(0,0,0,0);
      *aKeepReflowGoing = brc.PlaceBlock(blockHtmlRS, isAdjacentWithTop,
                                         aLine.get(),
                                         computedOffsets, collapsedBottomMargin,
                                         aLine->mBounds, combinedArea);
      if (aLine->SetCarriedOutBottomMargin(collapsedBottomMargin)) {
        line_iterator nextLine = aLine;
        ++nextLine;
        if (nextLine != end_lines()) {
          nextLine->MarkPreviousMarginDirty();
        }
      }
      
      if (aState.GetFlag(BRS_SHRINKWRAPWIDTH)) {
        // Mark the line as dirty so once we known the final shrink wrap width
        // we can reflow the block to the correct size
        // XXX We don't always need to do this...
        aLine->MarkDirty();
        aState.SetFlag(BRS_NEEDRESIZEREFLOW, PR_TRUE);
      }
      if (aState.GetFlag(BRS_UNCONSTRAINEDWIDTH) || aState.GetFlag(BRS_SHRINKWRAPWIDTH)) {
        // Add the right margin to the line's bounds.  That way it will be
        // taken into account when we compute our shrink wrap size.
        nscoord marginRight = brc.GetMargin().right;
        if (marginRight != NS_UNCONSTRAINEDSIZE) {
          aLine->mBounds.width += marginRight;
        }
      }
      aLine->SetCombinedArea(combinedArea);
      if (*aKeepReflowGoing) {
        // Some of the child block fit
        
        // Advance to new Y position
        nscoord newY = aLine->mBounds.YMost();
        aState.mY = newY;
        
        // Continue the block frame now if it didn't completely fit in
        // the available space.
        if (NS_FRAME_IS_NOT_COMPLETE(frameReflowStatus)) {
          PRBool madeContinuation;
          rv = CreateContinuationFor(aState, nsnull, frame, madeContinuation);
          if (NS_FAILED(rv)) 
            return rv;
          
          nsIFrame* nextFrame = frame->GetNextInFlow();
          
          // Push continuation to a new line, but only if we actually made one.
          if (madeContinuation) {
            nsLineBox* line = aState.NewLineBox(nextFrame, 1, PR_TRUE);
            if (nsnull == line) {
              return NS_ERROR_OUT_OF_MEMORY;
            }
            mLines.after_insert(aLine, line);
          }
          
          // Advance to next line since some of the block fit. That way
          // only the following lines will be pushed.
          PushLines(aState, aLine);
          aState.mReflowStatus = NS_FRAME_NOT_COMPLETE;
          // If we need to reflow the continuation of the block child,
          // then we'd better reflow our continuation
          if (frameReflowStatus & NS_FRAME_REFLOW_NEXTINFLOW) {
            aState.mReflowStatus |= NS_FRAME_REFLOW_NEXTINFLOW;
            // We also need to make that continuation's line dirty so it
            // gets reflowed when we reflow our next in flow. The
            // nif's line must always be either the first line
            // of the nif's parent block or else one of our own overflow
            // lines. In the latter case the line is already marked dirty,
            // so just detect and handle the first case.
            nsBlockFrame* nifBlock = NS_STATIC_CAST(nsBlockFrame*, nextFrame->GetParent());
            NS_ASSERTION(nifBlock->GetType() == nsLayoutAtoms::blockFrame
                         || nifBlock->GetType() == nsLayoutAtoms::areaFrame,
                         "A block's child's next in flow's parent must be a block!");
            line_iterator firstLine = nifBlock->begin_lines();
            if (firstLine != nifBlock->end_lines() && firstLine->Contains(nextFrame)) {
              firstLine->MarkDirty();
            }
          }
          *aKeepReflowGoing = PR_FALSE;
          
          // The bottom margin for a block is only applied on the last
          // flow block. Since we just continued the child block frame,
          // we know that line->mFirstChild is not the last flow block
          // therefore zero out the running margin value.
#ifdef NOISY_VERTICAL_MARGINS
          ListTag(stdout);
          printf(": reflow incomplete, frame=");
          nsFrame::ListTag(stdout, frame);
          printf(" prevBottomMargin=%d, setting to zero\n",
                 aState.mPrevBottomMargin);
#endif
          aState.mPrevBottomMargin.Zero();
        }
        else {
#ifdef NOISY_VERTICAL_MARGINS
          ListTag(stdout);
          printf(": reflow complete for ");
          nsFrame::ListTag(stdout, frame);
          printf(" prevBottomMargin=%d collapsedBottomMargin=%d\n",
                 aState.mPrevBottomMargin, collapsedBottomMargin.get());
#endif
          aState.mPrevBottomMargin = collapsedBottomMargin;
        }
#ifdef NOISY_VERTICAL_MARGINS
        ListTag(stdout);
        printf(": frame=");
        nsFrame::ListTag(stdout, frame);
        printf(" carriedOutBottomMargin=%d collapsedBottomMargin=%d => %d\n",
               brc.GetCarriedOutBottomMargin(), collapsedBottomMargin.get(),
               aState.mPrevBottomMargin);
#endif
        
        // Post-process the "line"
        nscoord maxElementWidth = 0;
        if (aState.GetFlag(BRS_COMPUTEMAXELEMENTWIDTH)) {
          maxElementWidth = brc.GetMaxElementWidth();
        }
        // If we asked the block to update its maximum width, then record the
        // updated value in the line, and update the current maximum width
        if (aState.GetFlag(BRS_COMPUTEMAXWIDTH)) {
          aLine->mMaximumWidth = brc.GetMaximumWidth();
          aState.UpdateMaximumWidth(aLine->mMaximumWidth);
        }
        PostPlaceLine(aState, aLine, maxElementWidth);
        
        // If the block frame that we just reflowed happens to be our
        // first block, then its computed ascent is ours
        if (frame == GetTopBlockChild(aState.mPresContext)) {
          const nsHTMLReflowMetrics& metrics = brc.GetMetrics();
          mAscent = metrics.ascent;
        }
        
        // Place the "marker" (bullet) frame.
        //
        // According to the CSS2 spec, section 12.6.1, the "marker" box
        // participates in the height calculation of the list-item box's
        // first line box.
        //
        // There are exactly two places a bullet can be placed: near the
        // first or second line. Its only placed on the second line in a
        // rare case: an empty first line followed by a second line that
        // contains a block (example: <LI>\n<P>... ). This is where
        // the second case can happen.
        if (mBullet && HaveOutsideBullet() &&
            ((aLine == mLines.front()) ||
             ((0 == mLines.front()->mBounds.height) &&
              (aLine == begin_lines().next())))) {
          // Reflow the bullet
          nsHTMLReflowMetrics metrics(nsnull);
          ReflowBullet(aState, metrics);
          
          // Doing the alignment using |mAscent| will also cater for bullets
          // that are placed next to a child block (bug 92896)
          // (Note that mAscent should be set by now, otherwise why would
          // we be placing the bullet yet?)
          
          // Tall bullets won't look particularly nice here...
          nsRect bbox = mBullet->GetRect();
          nscoord bulletTopMargin = applyTopMargin
            ? collapsedBottomMargin.get()
            : 0;
          bbox.y = aState.BorderPadding().top + mAscent -
            metrics.ascent + bulletTopMargin;
          mBullet->SetRect(bbox);
        }
      }
      else {
        // None of the block fits. Determine the correct reflow status.
        if (aLine == mLines.front()) {
          // If it's our very first line then we need to be pushed to
          // our parents next-in-flow. Therefore, return break-before
          // status for our reflow status.
          aState.mReflowStatus = NS_INLINE_LINE_BREAK_BEFORE();
        }
        else {
          // Push the line that didn't fit and any lines that follow it
          // to our next-in-flow.
          UndoSplitPlaceholders(aState, lastPlaceholder);
          PushLines(aState, aLine.prev());
          aState.mReflowStatus = NS_FRAME_NOT_COMPLETE;
        }
      }
    }
    break; // out of the reflow retry loop
  }
  
#ifdef DEBUG
  VerifyLines(PR_TRUE);
#endif
  return rv;
}

#define LINE_REFLOW_OK        0
#define LINE_REFLOW_STOP      1
#define LINE_REFLOW_REDO      2
// a frame was complete, but truncated and not at the top of a page
#define LINE_REFLOW_TRUNCATED 3

nsresult
nsBlockFrame::ReflowInlineFrames(nsBlockReflowState& aState,
                                 line_iterator aLine,
                                 PRBool* aKeepReflowGoing,
                                 PRBool aDamageDirtyArea,
                                 PRBool aUpdateMaximumWidth)
{
  nsresult rv = NS_OK;
  *aKeepReflowGoing = PR_TRUE;

#ifdef DEBUG
  PRInt32 spins = 0;
#endif
  PRUint8 lineReflowStatus = LINE_REFLOW_REDO;
  do {
    // Once upon a time we allocated the first 30 nsLineLayout objects
    // on the stack, and then we switched to the heap.  At that time
    // these objects were large (1100 bytes on a 32 bit system).
    // Then the nsLineLayout object was shrunk to 156 bytes by
    // removing some internal buffers.  Given that it is so much
    // smaller, the complexity of 2 different ways of allocating
    // no longer makes sense.  Now we always allocate on the stack

    nsLineLayout lineLayout(aState.mPresContext,
                            aState.mReflowState.mSpaceManager,
                            &aState.mReflowState,
                            aState.GetFlag(BRS_COMPUTEMAXELEMENTWIDTH));
    lineLayout.Init(&aState, aState.mMinLineHeight, aState.mLineNumber);
    rv = DoReflowInlineFrames(aState, lineLayout, aLine,
                              aKeepReflowGoing, &lineReflowStatus,
                              aUpdateMaximumWidth, aDamageDirtyArea);
    lineLayout.EndLineReflow();

#ifdef DEBUG
    spins++;
    if (1000 == spins) {
      ListTag(stdout);
      printf(": yikes! spinning on a line over 1000 times!\n");
      NS_ABORT();
    }
#endif

  } while (NS_SUCCEEDED(rv) && LINE_REFLOW_REDO == lineReflowStatus);

  return rv;
}

// If at least one float on the line was complete, not at the top of
// page, but was truncated, then restore the overflow floats to what
// they were before and push the line.  The floats that will be removed
// from the list aren't yet known by the block's next in flow.  
void
nsBlockFrame::PushTruncatedPlaceholderLine(nsBlockReflowState& aState,
                                           line_iterator       aLine,
                                           nsIFrame*           aLastPlaceholder,
                                           PRBool&             aKeepReflowGoing)
{
  UndoSplitPlaceholders(aState, aLastPlaceholder);

  line_iterator prevLine = aLine;
  --prevLine;
  PushLines(aState, prevLine);
  aKeepReflowGoing = PR_FALSE;
  aState.mReflowStatus = NS_FRAME_NOT_COMPLETE;
}

nsresult
nsBlockFrame::DoReflowInlineFrames(nsBlockReflowState& aState,
                                   nsLineLayout& aLineLayout,
                                   line_iterator aLine,
                                   PRBool* aKeepReflowGoing,
                                   PRUint8* aLineReflowStatus,
                                   PRBool aUpdateMaximumWidth,
                                   PRBool aDamageDirtyArea)
{
  // Forget all of the floats on the line
  aLine->FreeFloats(aState.mFloatCacheFreeList);
  aState.mFloatCombinedArea.SetRect(0, 0, 0, 0);

  // Setup initial coordinate system for reflowing the inline frames
  // into. Apply a previous block frame's bottom margin first.
  if (ShouldApplyTopMargin(aState, aLine)) {
    aState.mY += aState.mPrevBottomMargin.get();
  }
  aState.GetAvailableSpace();
  PRBool impactedByFloats = aState.IsImpactedByFloat() ? PR_TRUE : PR_FALSE;
  aLine->SetLineIsImpactedByFloat(impactedByFloats);
#ifdef REALLY_NOISY_REFLOW
  printf("nsBlockFrame::DoReflowInlineFrames %p impacted = %d\n",
         this, impactedByFloats);
#endif

  const nsMargin& borderPadding = aState.BorderPadding();
  nscoord x = aState.mAvailSpaceRect.x + borderPadding.left;
  nscoord availWidth = aState.mAvailSpaceRect.width;
  nscoord availHeight;
  if (aState.GetFlag(BRS_UNCONSTRAINEDHEIGHT)) {
    availHeight = NS_UNCONSTRAINEDSIZE;
  }
  else {
    /* XXX get the height right! */
    availHeight = aState.mAvailSpaceRect.height;
  }
  if (aUpdateMaximumWidth) {
    availWidth = NS_UNCONSTRAINEDSIZE;
  }
#ifdef IBMBIDI
  else {
    nscoord rightEdge = aState.mReflowState.mRightEdge;
    if ( (rightEdge != NS_UNCONSTRAINEDSIZE)
         && (availWidth < rightEdge) ) {
      availWidth = rightEdge;
    }
  }
#endif // IBMBIDI
  aLineLayout.BeginLineReflow(x, aState.mY,
                              availWidth, availHeight,
                              impactedByFloats,
                              PR_FALSE /*XXX isTopOfPage*/);

  // XXX Unfortunately we need to know this before reflowing the first
  // inline frame in the line. FIX ME.
  if ((0 == aLineLayout.GetLineNumber()) &&
      (NS_BLOCK_HAS_FIRST_LETTER_STYLE & mState)) {
    aLineLayout.SetFirstLetterStyleOK(PR_TRUE);
  }

  // keep track of the last overflow float in case we need to undo any new additions
  nsFrameList* overflowPlace = GetOverflowPlaceholders();
  nsIFrame* lastPlaceholder = (overflowPlace) ? overflowPlace->LastChild() : nsnull;

  // Reflow the frames that are already on the line first
  nsresult rv = NS_OK;
  PRUint8 lineReflowStatus = LINE_REFLOW_OK;
  PRInt32 i;
  nsIFrame* frame = aLine->mFirstChild;
  aLine->SetHasPercentageChild(PR_FALSE); // To be set by ReflowInlineFrame below
  // need to repeatedly call GetChildCount here, because the child
  // count can change during the loop!
  for (i = 0; i < aLine->GetChildCount(); i++) { 
    rv = ReflowInlineFrame(aState, aLineLayout, aLine, frame,
                           &lineReflowStatus);
    if (NS_FAILED(rv)) {
      return rv;
    }
    if (LINE_REFLOW_OK != lineReflowStatus) {
      // It is possible that one or more of next lines are empty
      // (because of DeleteNextInFlowChild). If so, delete them now
      // in case we are finished.
      ++aLine;
      while ((aLine != end_lines()) && (0 == aLine->GetChildCount())) {
        // XXX Is this still necessary now that DeleteNextInFlowChild
        // uses DoRemoveFrame?
        nsLineBox *toremove = aLine;
        aLine = mLines.erase(aLine);
        NS_ASSERTION(nsnull == toremove->mFirstChild, "bad empty line");
        aState.FreeLineBox(toremove);
      }
      --aLine;

      if (LINE_REFLOW_TRUNCATED == lineReflowStatus) {
        // Don't push any lines if we just want to calculate the maximum width
        if (!aUpdateMaximumWidth) {
          // Push the line with the truncated float 
          PushTruncatedPlaceholderLine(aState, aLine, lastPlaceholder, *aKeepReflowGoing);
        }
      }
      break;
    }
    frame = frame->GetNextSibling();
  }

  // Pull frames and reflow them until we can't
  while (LINE_REFLOW_OK == lineReflowStatus) {
    rv = PullFrame(aState, aLine, aDamageDirtyArea, frame);
    if (NS_FAILED(rv)) {
      return rv;
    }
    if (nsnull == frame) {
      break;
    }
    while (LINE_REFLOW_OK == lineReflowStatus) {
      PRInt32 oldCount = aLine->GetChildCount();
      rv = ReflowInlineFrame(aState, aLineLayout, aLine, frame,
                             &lineReflowStatus);
      if (NS_FAILED(rv)) {
        return rv;
      }
      if (aLine->GetChildCount() != oldCount) {
        // We just created a continuation for aFrame AND its going
        // to end up on this line (e.g. :first-letter
        // situation). Therefore we have to loop here before trying
        // to pull another frame.
        frame = frame->GetNextSibling();
      }
      else {
        break;
      }
    }
  }
  if (LINE_REFLOW_REDO == lineReflowStatus) {
    // This happens only when we have a line that is impacted by
    // floats and the first element in the line doesn't fit with
    // the floats.
    //
    // What we do is to advance past the first float we find and
    // then reflow the line all over again.
    NS_ASSERTION(aState.IsImpactedByFloat(),
                 "redo line on totally empty line");
    NS_ASSERTION(NS_UNCONSTRAINEDSIZE != aState.mAvailSpaceRect.height,
                 "unconstrained height on totally empty line");

    aState.mY += aState.mAvailSpaceRect.height;

    // We don't want to advance by the bottom margin anymore (we did it
    // once at the beginning of this function, which will just be called
    // again), and we certainly don't want to go back if it's negative
    // (infinite loop, bug 153429).
    aState.mPrevBottomMargin.Zero();

    // XXX: a small optimization can be done here when paginating:
    // if the new Y coordinate is past the end of the block then
    // push the line and return now instead of later on after we are
    // past the float.
  }
  else if (LINE_REFLOW_TRUNCATED != lineReflowStatus) {
    // If we are propagating out a break-before status then there is
    // no point in placing the line.
    if (!NS_INLINE_IS_BREAK_BEFORE(aState.mReflowStatus)) {
      if (PlaceLine(aState, aLineLayout, aLine, aKeepReflowGoing, aUpdateMaximumWidth)) {
        UndoSplitPlaceholders(aState, lastPlaceholder); // undo since we pushed the current line
      }
    }
  }
  *aLineReflowStatus = lineReflowStatus;

  return rv;
}

/**
 * Reflow an inline frame. The reflow status is mapped from the frames
 * reflow status to the lines reflow status (not to our reflow status).
 * The line reflow status is simple: PR_TRUE means keep placing frames
 * on the line; PR_FALSE means don't (the line is done). If the line
 * has some sort of breaking affect then aLine's break-type will be set
 * to something other than NS_STYLE_CLEAR_NONE.
 */
nsresult
nsBlockFrame::ReflowInlineFrame(nsBlockReflowState& aState,
                                nsLineLayout& aLineLayout,
                                line_iterator aLine,
                                nsIFrame* aFrame,
                                PRUint8* aLineReflowStatus)
{
 NS_ENSURE_ARG_POINTER(aFrame);
  
  *aLineReflowStatus = LINE_REFLOW_OK;

  // If it's currently ok to be reflowing in first-letter style then
  // we must be about to reflow a frame that has first-letter style.
  PRBool reflowingFirstLetter = aLineLayout.GetFirstLetterStyleOK();
#ifdef NOISY_FIRST_LETTER
  ListTag(stdout);
  printf(": reflowing ");
  nsFrame::ListTag(stdout, aFrame);
  printf(" reflowingFirstLetter=%s\n", reflowingFirstLetter ? "on" : "off");
#endif

  // Remember if we have a percentage aware child on this line
  if (IsPercentageAwareChild(aFrame)) {
    aLine->SetHasPercentageChild(PR_TRUE);
  }

  // Reflow the inline frame
  nsReflowStatus frameReflowStatus;
  PRBool         pushedFrame;
  nsresult rv = aLineLayout.ReflowFrame(aFrame, frameReflowStatus,
                                        nsnull, pushedFrame);

  // If this is an incremental reflow, prune the child from the path
  // so we don't incrementally reflow it again.
  // XXX note that we don't currently have any incremental reflows
  // that trace a path through an inline frame (thanks to reflow roots
  // dealing with text inputs), but we may need to deal with this
  // again, someday.
  if (aState.mReflowState.path)
    aState.mReflowState.path->RemoveChild(aFrame);

  if (NS_FAILED(rv)) {
    return rv;
  }
#ifdef REALLY_NOISY_REFLOW_CHILD
  nsFrame::ListTag(stdout, aFrame);
  printf(": status=%x\n", frameReflowStatus);
#endif

#if defined(REFLOW_STATUS_COVERAGE)
  RecordReflowStatus(PR_FALSE, frameReflowStatus);
#endif

  // Send post-reflow notification
  aState.mPrevChild = aFrame;

   /* XXX
      This is where we need to add logic to handle some odd behavior.
      For one thing, we should usually place at least one thing next
      to a left float, even when that float takes up all the width on a line.
      see bug 22496
   */

  // Process the child frames reflow status. There are 5 cases:
  // complete, not-complete, break-before, break-after-complete,
  // break-after-not-complete. There are two situations: we are a
  // block or we are an inline. This makes a total of 10 cases
  // (fortunately, there is some overlap).
  aLine->SetBreakTypeAfter(NS_STYLE_CLEAR_NONE);
  if (NS_INLINE_IS_BREAK(frameReflowStatus) || 
      (NS_STYLE_CLEAR_NONE != aState.mFloatBreakType)) {
    // Always abort the line reflow (because a line break is the
    // minimal amount of break we do).
    *aLineReflowStatus = LINE_REFLOW_STOP;

    // XXX what should aLine's break-type be set to in all these cases?
    PRUint8 breakType = NS_INLINE_GET_BREAK_TYPE(frameReflowStatus);
    NS_ASSERTION((NS_STYLE_CLEAR_NONE != breakType) || 
                 (NS_STYLE_CLEAR_NONE != aState.mFloatBreakType), "bad break type");
    NS_ASSERTION(NS_STYLE_CLEAR_PAGE != breakType, "no page breaks yet");

    if (NS_INLINE_IS_BREAK_BEFORE(frameReflowStatus)) {
      // Break-before cases.
      if (aFrame == aLine->mFirstChild) {
        // If we break before the first frame on the line then we must
        // be trying to place content where theres no room (e.g. on a
        // line with wide floats). Inform the caller to reflow the
        // line after skipping past a float.
        *aLineReflowStatus = LINE_REFLOW_REDO;
      }
      else {
        // It's not the first child on this line so go ahead and split
        // the line. We will see the frame again on the next-line.
        rv = SplitLine(aState, aLineLayout, aLine, aFrame);
        if (NS_FAILED(rv)) {
          return rv;
        }

        // If we're splitting the line because the frame didn't fit and it
        // was pushed, then mark the line as having word wrapped. We need to
        // know that if we're shrink wrapping our width
        if (pushedFrame) {
          aLine->SetLineWrapped(PR_TRUE);
        }
      }
    }
    else {
      // If a float split and its prev-in-flow was followed by a <BR>, then combine 
      // the <BR>'s break type with the inline's break type (the inline will be the very 
      // next frame after the split float).
      if (NS_STYLE_CLEAR_NONE != aState.mFloatBreakType) {
        breakType = nsLayoutUtils::CombineBreakType(breakType,
                                                    aState.mFloatBreakType);
        aState.mFloatBreakType = NS_STYLE_CLEAR_NONE;
      }
      // Break-after cases
      if (breakType == NS_STYLE_CLEAR_LINE) {
        if (!aLineLayout.GetLineEndsInBR()) {
          breakType = NS_STYLE_CLEAR_NONE;
        }
      }
      aLine->SetBreakTypeAfter(breakType);
      if (NS_FRAME_IS_NOT_COMPLETE(frameReflowStatus)) {
        // Create a continuation for the incomplete frame. Note that the
        // frame may already have a continuation.
        PRBool madeContinuation;
        rv = CreateContinuationFor(aState, aLine, aFrame, madeContinuation);
        if (NS_FAILED(rv)) 
          return rv;
        if (!aLineLayout.GetLineEndsInBR()) {
          // Remember that the line has wrapped
          aLine->SetLineWrapped(PR_TRUE);
        }
      }

      // Split line, but after the frame just reflowed
      rv = SplitLine(aState, aLineLayout, aLine, aFrame->GetNextSibling());
      if (NS_FAILED(rv)) {
        return rv;
      }

      if (NS_FRAME_IS_NOT_COMPLETE(frameReflowStatus)) {
        // Mark next line dirty in case SplitLine didn't end up
        // pushing any frames.
        nsLineList_iterator next = aLine.next();
        if (next != end_lines() && !next->IsBlock()) {
          next->MarkDirty();
        }
      }
    }
  }
  else if (NS_FRAME_IS_NOT_COMPLETE(frameReflowStatus)) {
    // Frame is not-complete, no special breaking status

    nsIAtom* frameType = aFrame->GetType();

    // Create a continuation for the incomplete frame. Note that the
    // frame may already have a continuation.
    PRBool madeContinuation;
    rv = (nsLayoutAtoms::placeholderFrame == frameType)
         ? SplitPlaceholder(*aState.mPresContext, *aFrame)
         : CreateContinuationFor(aState, aLine, aFrame, madeContinuation);
    if (NS_FAILED(rv)) 
      return rv;

    // Remember that the line has wrapped
    if (!aLineLayout.GetLineEndsInBR()) {
      aLine->SetLineWrapped(PR_TRUE);
    }
    
    // If we are reflowing the first letter frame or a placeholder then 
    // don't split the line and don't stop the line reflow...
    PRBool splitLine = !reflowingFirstLetter && 
                       (nsLayoutAtoms::placeholderFrame != frameType);
    if (reflowingFirstLetter) {
      if ((nsLayoutAtoms::inlineFrame == frameType) ||
          (nsLayoutAtoms::lineFrame == frameType)) {
        splitLine = PR_TRUE;
      }
    }

    if (splitLine) {
      // Split line after the current frame
      *aLineReflowStatus = LINE_REFLOW_STOP;
      rv = SplitLine(aState, aLineLayout, aLine, aFrame->GetNextSibling());
      if (NS_FAILED(rv)) {
        return rv;
      }

      // Mark next line dirty in case SplitLine didn't end up
      // pushing any frames.
      nsLineList_iterator next = aLine.next();
      if (next != end_lines() && !next->IsBlock()) {
        next->MarkDirty();
      }
    }
  }
  else if (NS_FRAME_IS_TRUNCATED(frameReflowStatus)) {
    // if the frame is a placeholder and was complete but truncated (and not at the top
    // of page), the entire line will be pushed to give it another chance to not truncate.
    if (nsLayoutAtoms::placeholderFrame == aFrame->GetType()) {
      *aLineReflowStatus = LINE_REFLOW_TRUNCATED;
    }
  }  

  return NS_OK;
}

/**
 * Create a continuation, if necessary, for aFrame. Place it in aLine
 * if aLine is not null. Set aMadeNewFrame to PR_TRUE if a new frame is created.
 */
nsresult
nsBlockFrame::CreateContinuationFor(nsBlockReflowState& aState,
                                    nsLineBox*          aLine,
                                    nsIFrame*           aFrame,
                                    PRBool&             aMadeNewFrame)
{
  aMadeNewFrame = PR_FALSE;
  nsresult rv;
  nsIFrame* nextInFlow;
  rv = CreateNextInFlow(aState.mPresContext, this, aFrame, nextInFlow);
  if (NS_FAILED(rv)) {
    return rv;
  }
  if (nsnull != nextInFlow) {
    aMadeNewFrame = PR_TRUE;
    if (aLine) { 
      aLine->SetChildCount(aLine->GetChildCount() + 1);
    }
  }
#ifdef DEBUG
  VerifyLines(PR_FALSE);
#endif
  return rv;
}

nsresult
nsBlockFrame::SplitPlaceholder(nsPresContext& aPresContext,
                               nsIFrame&       aPlaceholder)
{
  nsIFrame* nextInFlow;
  nsresult rv = CreateNextInFlow(&aPresContext, this, &aPlaceholder, nextInFlow);
  if (NS_FAILED(rv)) 
    return rv;
  // put the sibling list back to what it was before the continuation was created
  nsIFrame *contFrame = aPlaceholder.GetNextSibling();
  nsIFrame *next = contFrame->GetNextSibling();
  aPlaceholder.SetNextSibling(next);
  contFrame->SetNextSibling(nsnull);
  // add the placehoder to the overflow floats
  nsFrameList* overflowPlace = GetOverflowPlaceholders();
  if (overflowPlace) {
    overflowPlace->AppendFrames(this, contFrame);
  }
  else {
    overflowPlace = new nsFrameList(contFrame);
    if (overflowPlace) {
      SetOverflowPlaceholders(overflowPlace);
    }
    else return NS_ERROR_NULL_POINTER;
  }
  return NS_OK;
}

nsresult
nsBlockFrame::SplitLine(nsBlockReflowState& aState,
                        nsLineLayout& aLineLayout,
                        line_iterator aLine,
                        nsIFrame* aFrame)
{
  NS_ABORT_IF_FALSE(aLine->IsInline(), "illegal SplitLine on block line");

  PRInt32 pushCount = aLine->GetChildCount() - aLineLayout.GetCurrentSpanCount();
  NS_ABORT_IF_FALSE(pushCount >= 0, "bad push count"); 

#ifdef DEBUG
  if (gNoisyReflow) {
    nsFrame::IndentBy(stdout, gNoiseIndent);
    printf("split line: from line=%p pushCount=%d aFrame=",
           NS_STATIC_CAST(void*, aLine.get()), pushCount);
    if (aFrame) {
      nsFrame::ListTag(stdout, aFrame);
    }
    else {
      printf("(null)");
    }
    printf("\n");
    if (gReallyNoisyReflow) {
      aLine->List(aState.mPresContext, stdout, gNoiseIndent+1);
    }
  }
#endif

  if (0 != pushCount) {
    NS_ABORT_IF_FALSE(aLine->GetChildCount() > pushCount, "bad push");
    NS_ABORT_IF_FALSE(nsnull != aFrame, "whoops");

    // Put frames being split out into their own line
    nsLineBox* newLine = aState.NewLineBox(aFrame, pushCount, PR_FALSE);
    if (!newLine) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    mLines.after_insert(aLine, newLine);
    aLine->SetChildCount(aLine->GetChildCount() - pushCount);
#ifdef DEBUG
    if (gReallyNoisyReflow) {
      newLine->List(aState.mPresContext, stdout, gNoiseIndent+1);
    }
#endif

    // Let line layout know that some frames are no longer part of its
    // state.
    aLineLayout.SplitLineTo(aLine->GetChildCount());
#ifdef DEBUG
    VerifyLines(PR_TRUE);
#endif
  }
  return NS_OK;
}

PRBool
nsBlockFrame::ShouldJustifyLine(nsBlockReflowState& aState,
                                line_iterator aLine)
{
  while (++aLine != end_lines()) {
    // There is another line
    if (0 != aLine->GetChildCount()) {
      // If the next line is a block line then we must not justify
      // this line because it means that this line is the last in a
      // group of inline lines.
      return !aLine->IsBlock();
    }
    // The next line is empty, try the next one
  }

  // XXX Not sure about this part
  // Try our next-in-flows lines to answer the question
  nsBlockFrame* nextInFlow = (nsBlockFrame*) mNextInFlow;
  while (nsnull != nextInFlow) {
    for (line_iterator line = nextInFlow->begin_lines(),
                   line_end = nextInFlow->end_lines();
         line != line_end;
         ++line)
    {
      if (0 != line->GetChildCount())
        return !line->IsBlock();
    }
    nextInFlow = (nsBlockFrame*) nextInFlow->mNextInFlow;
  }

  // This is the last line - so don't allow justification
  return PR_FALSE;
}

PRBool
nsBlockFrame::PlaceLine(nsBlockReflowState& aState,
                        nsLineLayout&       aLineLayout,
                        line_iterator       aLine,
                        PRBool*             aKeepReflowGoing,
                        PRBool              aUpdateMaximumWidth)
{
  // Trim extra white-space from the line before placing the frames
  aLineLayout.TrimTrailingWhiteSpace();

  // Vertically align the frames on this line.
  //
  // According to the CSS2 spec, section 12.6.1, the "marker" box
  // participates in the height calculation of the list-item box's
  // first line box.
  //
  // There are exactly two places a bullet can be placed: near the
  // first or second line. Its only placed on the second line in a
  // rare case: an empty first line followed by a second line that
  // contains a block (example: <LI>\n<P>... ).
  //
  // For this code, only the first case is possible because this
  // method is used for placing a line of inline frames. If the rare
  // case is happening then the worst that will happen is that the
  // bullet frame will be reflowed twice.
  PRBool addedBullet = PR_FALSE;
  if (mBullet && HaveOutsideBullet() && (aLine == mLines.front()) &&
      (!aLineLayout.IsZeroHeight() || (aLine == mLines.back()))) {
    nsHTMLReflowMetrics metrics(nsnull);
    ReflowBullet(aState, metrics);
    aLineLayout.AddBulletFrame(mBullet, metrics);
    addedBullet = PR_TRUE;
  }
  nscoord maxElementWidth;
  aLineLayout.VerticalAlignLine(aLine, &maxElementWidth);
  // Our ascent is the ascent of our first line (but if this line is all
  // whitespace we'll correct things in |ReflowBlockFrame|).
  if (aLine == mLines.front()) {
    mAscent = aLine->mBounds.y + aLine->GetAscent();
  }

  // See if we're shrink wrapping the width
  if (aState.GetFlag(BRS_SHRINKWRAPWIDTH)) {
    // XXXldb Do we really still want to do this?
    // When determining the line's width we also need to include any
    // right floats that impact us. This represents the shrink wrap
    // width of the line
    if (aState.IsImpactedByFloat() && !aLine->IsLineWrapped()) {
      NS_ASSERTION(aState.mContentArea.width >= aState.mAvailSpaceRect.XMost(), "bad state");
      aLine->mBounds.width += aState.mContentArea.width - aState.mAvailSpaceRect.XMost();
    }
  }
#ifdef DEBUG
  {
    static nscoord lastHeight = 0;
    if (CRAZY_HEIGHT(aLine->mBounds.y)) {
      lastHeight = aLine->mBounds.y;
      if (abs(aLine->mBounds.y - lastHeight) > CRAZY_H/10) {
        nsFrame::ListTag(stdout);
        printf(": line=%p y=%d line.bounds.height=%d\n",
               NS_STATIC_CAST(void*, aLine.get()),
               aLine->mBounds.y, aLine->mBounds.height);
      }
    }
    else {
      lastHeight = 0;
    }
  }
#endif

  // Only block frames horizontally align their children because
  // inline frames "shrink-wrap" around their children (therefore
  // there is no extra horizontal space).
  const nsStyleText* styleText = GetStyleText();
  PRBool allowJustify = NS_STYLE_TEXT_ALIGN_JUSTIFY == styleText->mTextAlign &&
                        !aLineLayout.GetLineEndsInBR() &&
                        ShouldJustifyLine(aState, aLine);
  PRBool successful = aLineLayout.HorizontalAlignFrames(aLine->mBounds,
                            allowJustify, aState.GetFlag(BRS_SHRINKWRAPWIDTH));
  // XXX: not only bidi: right alignment can be broken after
  // RelativePositionFrames!!!
  // XXXldb Is something here considering relatively positioned frames at
  // other than their original positions?
  if (!successful) {
    // Mark the line dirty and then later once we've determined the width
    // we can do the horizontal alignment
    aLine->MarkDirty();
    aState.SetFlag(BRS_NEEDRESIZEREFLOW, PR_TRUE);
  }
#ifdef IBMBIDI
  // XXXldb Why don't we do this earlier?
  else {
    if (aState.mPresContext->BidiEnabled()) {
      if (!aState.mPresContext->IsVisualMode()) {
        nsBidiPresUtils* bidiUtils = aState.mPresContext->GetBidiUtils();

        if (bidiUtils && bidiUtils->IsSuccessful() ) {
          nsIFrame* nextInFlow = (aLine.next() != end_lines())
                                 ? aLine.next()->mFirstChild : nsnull;

          bidiUtils->ReorderFrames(aState.mPresContext,
                                   aState.mReflowState.rendContext,
                                   aLine->mFirstChild, nextInFlow,
                                   aLine->GetChildCount() );
        } // bidiUtils
      } // not visual mode
    } // bidi enabled
  } // successful
#endif // IBMBIDI

  nsRect combinedArea;
  aLineLayout.RelativePositionFrames(combinedArea);  // XXXldb This returned width as -15, 2001-06-12, Bugzilla
  aLine->SetCombinedArea(combinedArea);
  if (addedBullet) {
    aLineLayout.RemoveBulletFrame(mBullet);
  }

  // Inline lines do not have margins themselves; however they are
  // impacted by prior block margins. If this line ends up having some
  // height then we zero out the previous bottom margin value that was
  // already applied to the line's starting Y coordinate. Otherwise we
  // leave it be so that the previous blocks bottom margin can be
  // collapsed with a block that follows.
  nscoord newY;

  if (!aLine->CachedIsEmpty()) {
    // This line has some height. Therefore the application of the
    // previous-bottom-margin should stick.
    aState.mPrevBottomMargin.Zero();
    newY = aLine->mBounds.YMost();
  }
  else {
    // Don't let the previous-bottom-margin value affect the newY
    // coordinate (it was applied in ReflowInlineFrames speculatively)
    // since the line is empty.
    // We already called |ShouldApplyTopMargin|, and if we applied it
    // then BRS_APPLYTOPMARGIN is set.
    nscoord dy = aState.GetFlag(BRS_APPLYTOPMARGIN)
                   ? -aState.mPrevBottomMargin.get() : 0;
    newY = aState.mY + dy;
    aLine->SlideBy(dy); // XXXldb Do we really want to do this?
    // keep our ascent in sync
    // XXXldb If it's empty, shouldn't the next line control the ascent?
    if (mLines.front() == aLine) {
      mAscent += dy;
    }
  }

  // See if the line fit. If it doesn't we need to push it. Our first
  // line will always fit.

  // If we're just updating the maximum width then we don't care if it
  // fits; we'll assume it does, so that the maximum width will get
  // updated below. The line will be reflowed again and pushed then
  // if necessary.
  if (mLines.front() != aLine &&
      newY > aState.mBottomEdge &&
      aState.mBottomEdge != NS_UNCONSTRAINEDSIZE &&
      !aUpdateMaximumWidth) {
    // Push this line and all of it's children and anything else that
    // follows to our next-in-flow
    NS_ASSERTION((aState.mCurrentLine == aLine), "oops");
    PushLines(aState, aLine.prev());

    // Stop reflow and whack the reflow status if reflow hasn't
    // already been stopped.
    if (*aKeepReflowGoing) {
      NS_ASSERTION(NS_FRAME_COMPLETE == aState.mReflowStatus, 
                   "lost reflow status");
      aState.mReflowStatus = NS_FRAME_NOT_COMPLETE;
      *aKeepReflowGoing = PR_FALSE;
    }
    return PR_TRUE;
  }

  aState.mY = newY;
  
  // If we're reflowing the line just to incrementally update the
  // maximum width, then don't post-place the line. It's doing work we
  // don't need, and it will update things like aState.mKidXMost that
  // we don't want updated...
  if (aUpdateMaximumWidth) {
    // However, we do need to update the max-element-width if requested
    if (aState.GetFlag(BRS_COMPUTEMAXELEMENTWIDTH)) {
      aState.UpdateMaxElementWidth(maxElementWidth);
      // We also cache the max element width in the line. This is needed for
      // incremental reflow
      aLine->mMaxElementWidth = maxElementWidth;
#ifdef DEBUG
      if (gNoisyMaxElementWidth) {
        IndentBy(stdout, gNoiseIndent);
        printf ("nsBlockFrame::PlaceLine: %p setting MEW for line %p to %d\n", 
                NS_STATIC_CAST(void*, this), NS_STATIC_CAST(void*, aLine.get()),
                maxElementWidth);
      }
#endif
    }

  } else {
    PostPlaceLine(aState, aLine, maxElementWidth);
  }

  // Add the already placed current-line floats to the line
  aLine->AppendFloats(aState.mCurrentLineFloats);

  // Any below current line floats to place?
  if (aState.mBelowCurrentLineFloats.NotEmpty()) {
    // Reflow the below-current-line floats, then add them to the
    // lines float list if there aren't any truncated floats.
    if (aState.PlaceBelowCurrentLineFloats(aState.mBelowCurrentLineFloats)) {
      aLine->AppendFloats(aState.mBelowCurrentLineFloats);
    }
    else { 
      // At least one float is truncated, so fix up any placeholders that got split and 
      // push the line. XXX It may be better to put the float on the next line, but this 
      // is not common enough to justify the complexity.

      // If we just want to calculate the maximum width, then don't push the line.
      // We'll reflow it again and then it will get pushed if necessary.
      if (!aUpdateMaximumWidth) {
        nsFrameList* overflowPlace = GetOverflowPlaceholders();
        nsIFrame* lastPlaceholder = (overflowPlace) ? overflowPlace->LastChild() : nsnull;
        PushTruncatedPlaceholderLine(aState, aLine, lastPlaceholder, *aKeepReflowGoing);
      }
    }
  }

  // When a line has floats, factor them into the combined-area
  // computations.
  if (aLine->HasFloats()) {
    // Combine the float combined area (stored in aState) and the
    // value computed by the line layout code.
    nsRect lineCombinedArea(aLine->GetCombinedArea());
#ifdef NOISY_COMBINED_AREA
    ListTag(stdout);
    printf(": lineCA=%d,%d,%d,%d floatCA=%d,%d,%d,%d\n",
           lineCombinedArea.x, lineCombinedArea.y,
           lineCombinedArea.width, lineCombinedArea.height,
           aState.mFloatCombinedArea.x, aState.mFloatCombinedArea.y,
           aState.mFloatCombinedArea.width,
           aState.mFloatCombinedArea.height);
#endif
    lineCombinedArea.UnionRect(aState.mFloatCombinedArea, lineCombinedArea);

    aLine->SetCombinedArea(lineCombinedArea);
#ifdef NOISY_COMBINED_AREA
    printf("  ==> final lineCA=%d,%d,%d,%d\n",
           lineCombinedArea.x, lineCombinedArea.y,
           lineCombinedArea.width, lineCombinedArea.height);
#endif
  }

  // Apply break-after clearing if necessary
  // This must stay in sync with |ReflowDirtyLines|.
  if (aLine->HasFloatBreakAfter()) {
    aState.mY = aState.ClearFloats(aState.mY, aLine->GetBreakTypeAfter());
  }

  return PR_FALSE;
}

void
nsBlockFrame::PostPlaceLine(nsBlockReflowState& aState,
                            nsLineBox* aLine,
                            nscoord aMaxElementWidth)
{
  // Update max-element-width
  if (aState.GetFlag(BRS_COMPUTEMAXELEMENTWIDTH)) {
    aState.UpdateMaxElementWidth(aMaxElementWidth);
    // We also cache the max element width in the line. This is needed for
    // incremental reflow
    aLine->mMaxElementWidth = aMaxElementWidth;
#ifdef DEBUG
    if (gNoisyMaxElementWidth) {
      IndentBy(stdout, gNoiseIndent);
      printf ("nsBlockFrame::PostPlaceLine: %p setting line %p MEW %d\n", 
              NS_STATIC_CAST(void*, this), NS_STATIC_CAST(void*, aLine),
              aMaxElementWidth);
    }
#endif
  }

  // If this is an unconstrained reflow, then cache the line width in the
  // line. We'll need this during incremental reflow if we're asked to
  // calculate the maximum width
  if (aState.GetFlag(BRS_UNCONSTRAINEDWIDTH)) {
#ifdef NOISY_MAXIMUM_WIDTH
    printf("nsBlockFrame::PostPlaceLine during UC Reflow of block %p line %p caching max width %d\n", 
           NS_STATIC_CAST(void*, this), NS_STATIC_CAST(void*, aLine),
           aLine->mBounds.XMost());
#endif
    aLine->mMaximumWidth = aLine->mBounds.XMost();
  }

  // Update xmost
  nscoord xmost = aLine->mBounds.XMost();

#ifdef DEBUG
  if (CRAZY_WIDTH(xmost)) {
    ListTag(stdout);
    printf(": line=%p xmost=%d\n", NS_STATIC_CAST(void*, aLine), xmost);
  }
#endif
  if (xmost > aState.mKidXMost) {
    aState.mKidXMost = xmost;
#ifdef NOISY_KIDXMOST
    printf("%p PostPlaceLine aState.mKidXMost=%d\n", this, aState.mKidXMost); 
#endif
  }
}

void
nsBlockFrame::PushLines(nsBlockReflowState&  aState,
                        nsLineList::iterator aLineBefore)
{
  nsLineList::iterator overBegin(aLineBefore.next());

  // PushTruncatedPlaceholderLine sometimes pushes the first line.  Ugh.
  PRBool firstLine = overBegin == begin_lines();

  if (overBegin != end_lines()) {
    // XXXldb use presshell arena!
    nsLineList* overflowLines = new nsLineList();
    overflowLines->splice(overflowLines->end(), mLines, overBegin,
                          end_lines());
    NS_ASSERTION(!overflowLines->empty(), "should not be empty");
      // this takes ownership but it won't delete it immediately so we
      // can keep using it.
    SetOverflowLines(overflowLines);
  
    // Mark all the overflow lines dirty so that they get reflowed when
    // they are pulled up by our next-in-flow.

    // XXXldb Can this get called O(N) times making the whole thing O(N^2)?
    for (line_iterator line = overflowLines->begin(),
                   line_end = overflowLines->end();
         line != line_end;
         ++line)
    {
      line->MarkDirty();
      line->MarkPreviousMarginDirty();
      line->mBounds.SetRect(0, 0, 0, 0);
      if (line->HasFloats()) {
        line->FreeFloats(aState.mFloatCacheFreeList);
      }
    }
  }

  // Break frame sibling list
  if (!firstLine)
    aLineBefore->LastChild()->SetNextSibling(nsnull);

#ifdef DEBUG
  VerifyOverflowSituation();
#endif
}

// The overflowLines property is stored as a pointer to a line list,
// which must be deleted.  However, the following functions all maintain
// the invariant that the property is never set if the list is empty.

PRBool
nsBlockFrame::DrainOverflowLines()
{
#ifdef DEBUG
  VerifyOverflowSituation();
#endif
  PRBool drained = PR_FALSE;
  nsLineList* overflowLines;

  // First grab the prev-in-flows overflow lines
  nsBlockFrame* prevBlock = (nsBlockFrame*) mPrevInFlow;
  if (nsnull != prevBlock) {
    overflowLines = prevBlock->RemoveOverflowLines();
    if (nsnull != overflowLines) {
      NS_ASSERTION(! overflowLines->empty(),
                   "overflow lines should never be set and empty");
      drained = PR_TRUE;

      // Make all the frames on the overflow line list mine
      nsIFrame* lastFrame = nsnull;
      nsIFrame* frame = overflowLines->front()->mFirstChild;
      while (nsnull != frame) {
        ReparentFrame(frame, prevBlock, this);

        // Get the next frame
        lastFrame = frame;
        frame = frame->GetNextSibling();
      }

      // The lines on the overflow list have already been marked dirty and their
      // previous margins marked dirty also.

      // Join the line lists
      NS_ASSERTION(lastFrame, "overflow list was created with no frames");
      if (! mLines.empty()) 
      {
        // Remember to recompute the margins on the first line. This will
        // also recompute the correct deltaY if necessary.
       mLines.front()->MarkPreviousMarginDirty();
        // Join the sibling lists together
        lastFrame->SetNextSibling(mLines.front()->mFirstChild);
      }
      // Place overflow lines at the front of our line list
      mLines.splice(mLines.begin(), *overflowLines);
      NS_ASSERTION(overflowLines->empty(), "splice should empty list");
      delete overflowLines;

      nsFrameList* overflowOutOfFlows = prevBlock->RemoveOverflowOutOfFlows();
      if (overflowOutOfFlows) {
        for (nsIFrame* f = overflowOutOfFlows->FirstChild(); f;
             f = f->GetNextSibling()) {
          ReparentFrame(f, prevBlock, this);
        }
        delete overflowOutOfFlows;
      }
    }
  }

  // Now grab our own overflow lines
  overflowLines = RemoveOverflowLines();
  if (overflowLines) {
    NS_ASSERTION(! overflowLines->empty(),
                 "overflow lines should never be set and empty");
    // This can happen when we reflow and not everything fits and then
    // we are told to reflow again before a next-in-flow is created
    // and reflows.

    if (! mLines.empty()) {
      mLines.back()->LastChild()->SetNextSibling(
          overflowLines->front()->mFirstChild );
    }
    // append the overflow to mLines
    mLines.splice(mLines.end(), *overflowLines);
    drained = PR_TRUE;
    delete overflowLines;

    // Likewise, drain our own overflow out-of-flows. We don't need to
    // reparent them since they're already our children. We don't need
    // to put them on any child list since BuildFloatList will put
    // them on some child list. All we need to do is remove the
    // property.
    nsFrameList* overflowOutOfFlows = RemoveOverflowOutOfFlows();
    delete overflowOutOfFlows;
  }
  return drained;
}

nsLineList*
nsBlockFrame::GetOverflowLines() const
{
  if (!(GetStateBits() & NS_BLOCK_HAS_OVERFLOW_LINES)) {
    return nsnull;
  }
  nsLineList* lines = NS_STATIC_CAST(nsLineList*,
    GetProperty(nsLayoutAtoms::overflowLinesProperty));
  NS_ASSERTION(lines && !lines->empty(),
               "value should always be stored and non-empty when state set");
  return lines;
}

nsLineList*
nsBlockFrame::RemoveOverflowLines()
{
  if (!(GetStateBits() & NS_BLOCK_HAS_OVERFLOW_LINES)) {
    return nsnull;
  }
  nsLineList* lines = NS_STATIC_CAST(nsLineList*,
    UnsetProperty(nsLayoutAtoms::overflowLinesProperty));
  NS_ASSERTION(lines && !lines->empty(),
               "value should always be stored and non-empty when state set");
  RemoveStateBits(NS_BLOCK_HAS_OVERFLOW_LINES);
  return lines;
}

// Destructor function for the overflowLines frame property
static void
DestroyOverflowLines(void*           aFrame,
                     nsIAtom*        aPropertyName,
                     void*           aPropertyValue,
                     void*           aDtorData)
{
  if (aPropertyValue) {
    nsLineList* lines = NS_STATIC_CAST(nsLineList*, aPropertyValue);
    nsPresContext *context = NS_STATIC_CAST(nsPresContext*, aDtorData);
    nsLineBox::DeleteLineList(context, *lines);
    delete lines;
  }
}

// This takes ownership of aOverflowLines.
// XXX We should allocate overflowLines from presShell arena!
nsresult
nsBlockFrame::SetOverflowLines(nsLineList* aOverflowLines)
{
  NS_ASSERTION(aOverflowLines, "null lines");
  NS_ASSERTION(!aOverflowLines->empty(), "empty lines");
  NS_ASSERTION(!(GetStateBits() & NS_BLOCK_HAS_OVERFLOW_LINES),
               "Overwriting existing overflow lines");

  nsPresContext *presContext = GetPresContext();
  nsresult rv = presContext->PropertyTable()->
    SetProperty(this, nsLayoutAtoms::overflowLinesProperty, aOverflowLines,
                DestroyOverflowLines, presContext);
  // Verify that we didn't overwrite an existing overflow list
  NS_ASSERTION(rv != NS_PROPTABLE_PROP_OVERWRITTEN, "existing overflow list");
  AddStateBits(NS_BLOCK_HAS_OVERFLOW_LINES);
  return rv;
}

nsFrameList*
nsBlockFrame::GetOverflowOutOfFlows() const
{
  if (!(GetStateBits() & NS_BLOCK_HAS_OVERFLOW_OUT_OF_FLOWS)) {
    return nsnull;
  }
  nsFrameList* result = NS_STATIC_CAST(nsFrameList*,
    GetProperty(nsLayoutAtoms::overflowOutOfFlowsProperty));
  NS_ASSERTION(result, "value should always be non-empty when state set");
  return result;
}

nsFrameList*
nsBlockFrame::RemoveOverflowOutOfFlows()
{
  if (!(GetStateBits() & NS_BLOCK_HAS_OVERFLOW_OUT_OF_FLOWS)) {
    return nsnull;
  }
  nsFrameList* result = NS_STATIC_CAST(nsFrameList*,
    UnsetProperty(nsLayoutAtoms::overflowOutOfFlowsProperty));
  NS_ASSERTION(result, "value should always be non-empty when state set");
  RemoveStateBits(NS_BLOCK_HAS_OVERFLOW_OUT_OF_FLOWS);
  return result;
}

// Destructor function for the overflowPlaceholders frame property
static void
DestroyOverflowOOFs(void*           aFrame,
                    nsIAtom*        aPropertyName,
                    void*           aPropertyValue,
                    void*           aDtorData)
{
  NS_NOTREACHED("This helper method should never be called!");
  delete NS_STATIC_CAST(nsFrameList*, aPropertyValue);
}

// This takes ownership of aFloaters.
nsresult
nsBlockFrame::SetOverflowOutOfFlows(nsFrameList* aOOFs)
{
  NS_ASSERTION(!(GetStateBits() & NS_BLOCK_HAS_OVERFLOW_OUT_OF_FLOWS),
               "Overwriting existing overflow out-of-flows list");
  nsresult rv = SetProperty(nsLayoutAtoms::overflowOutOfFlowsProperty,
                            aOOFs, DestroyOverflowOOFs);
  // Verify that we didn't overwrite an existing overflow list
  NS_ASSERTION(rv != NS_PROPTABLE_PROP_OVERWRITTEN, "existing overflow float list");
  AddStateBits(NS_BLOCK_HAS_OVERFLOW_OUT_OF_FLOWS);
  return rv;
}

nsFrameList*
nsBlockFrame::GetOverflowPlaceholders() const
{
  if (!(GetStateBits() & NS_BLOCK_HAS_OVERFLOW_PLACEHOLDERS)) {
    return nsnull;
  }
  nsFrameList* result = NS_STATIC_CAST(nsFrameList*,
    GetProperty(nsLayoutAtoms::overflowPlaceholdersProperty));
  NS_ASSERTION(result, "value should always be non-empty when state set");
  return result;
}

nsFrameList*
nsBlockFrame::RemoveOverflowPlaceholders()
{
  if (!(GetStateBits() & NS_BLOCK_HAS_OVERFLOW_PLACEHOLDERS)) {
    return nsnull;
  }
  nsFrameList* result = NS_STATIC_CAST(nsFrameList*, 
    UnsetProperty(nsLayoutAtoms::overflowPlaceholdersProperty));
  NS_ASSERTION(result, "value should always be non-empty when state set");
  RemoveStateBits(NS_BLOCK_HAS_OVERFLOW_PLACEHOLDERS);
  return result;
}

// Destructor function for the overflowPlaceholders frame property
static void
DestroyOverflowPlaceholders(void*           aFrame,
                            nsIAtom*        aPropertyName,
                            void*           aPropertyValue,
                            void*           aDtorData)
{
  nsFrameList* overflowPlace = NS_STATIC_CAST(nsFrameList*, aPropertyValue);
  delete overflowPlace;
}

// This takes ownership of aOverflowLines.
// XXX We should allocate overflowLines from presShell arena!
nsresult
nsBlockFrame::SetOverflowPlaceholders(nsFrameList* aOverflowPlaceholders)
{
  NS_ASSERTION(!(GetStateBits() & NS_BLOCK_HAS_OVERFLOW_PLACEHOLDERS),
               "Overwriting existing overflow placeholder list");
  nsresult rv = SetProperty(nsLayoutAtoms::overflowPlaceholdersProperty,
                            aOverflowPlaceholders, DestroyOverflowPlaceholders);
  // Verify that we didn't overwrite an existing overflow list
  NS_ASSERTION(rv != NS_PROPTABLE_PROP_OVERWRITTEN, "existing overflow placeholder list");
  AddStateBits(NS_BLOCK_HAS_OVERFLOW_PLACEHOLDERS);
  return rv;
}

//////////////////////////////////////////////////////////////////////
// Frame list manipulation routines

nsIFrame*
nsBlockFrame::LastChild()
{
  if (! mLines.empty()) {
    return mLines.back()->LastChild();
  }
  return nsnull;
}

NS_IMETHODIMP
nsBlockFrame::AppendFrames(nsPresContext* aPresContext,
                           nsIPresShell&   aPresShell,
                           nsIAtom*        aListName,
                           nsIFrame*       aFrameList)
{
  if (nsnull == aFrameList) {
    return NS_OK;
  }
  if (mAbsoluteContainer.GetChildListName() == aListName) {
    return mAbsoluteContainer.AppendFrames(this, aPresContext, aPresShell, aListName,
                                           aFrameList);
  }
  else if (nsLayoutAtoms::floatList == aListName) {
    // XXX we don't *really* care about this right now because we are
    // BuildFloatList ing still
    mFloats.AppendFrames(nsnull, aFrameList);
    return NS_OK;
  }
  else if (nsnull != aListName) {
    return NS_ERROR_INVALID_ARG;
  }

  // Find the proper last-child for where the append should go
  nsIFrame* lastKid = nsnull;
  nsLineBox* lastLine = mLines.empty() ? nsnull : mLines.back();
  if (lastLine) {
    lastKid = lastLine->LastChild();
  }

  // Add frames after the last child
#ifdef NOISY_REFLOW_REASON
  ListTag(stdout);
  printf(": append ");
  nsFrame::ListTag(stdout, aFrameList);
  if (lastKid) {
    printf(" after ");
    nsFrame::ListTag(stdout, lastKid);
  }
  printf("\n");
#endif
  nsresult rv = AddFrames(aPresContext, aFrameList, lastKid);
  if (NS_SUCCEEDED(rv)) {
    // Ask the parent frame to reflow me.
    ReflowDirtyChild(&aPresShell, nsnull);
  }
  return rv;
}

NS_IMETHODIMP
nsBlockFrame::InsertFrames(nsPresContext* aPresContext,
                           nsIPresShell&   aPresShell,
                           nsIAtom*        aListName,
                           nsIFrame*       aPrevFrame,
                           nsIFrame*       aFrameList)
{
  if (mAbsoluteContainer.GetChildListName() == aListName) {
    return mAbsoluteContainer.InsertFrames(this, aPresContext, aPresShell, aListName,
                                           aPrevFrame, aFrameList);
  }
  else if (nsLayoutAtoms::floatList == aListName) {
    // XXX we don't *really* care about this right now because we are
    // BuildFloatList'ing still
    mFloats.AppendFrames(nsnull, aFrameList);
    return NS_OK;
  }
#ifdef IBMBIDI
  else if (nsLayoutAtoms::nextBidi == aListName) {}
#endif // IBMBIDI
  else if (nsnull != aListName) {
    return NS_ERROR_INVALID_ARG;
  }

#ifdef NOISY_REFLOW_REASON
  ListTag(stdout);
  printf(": insert ");
  nsFrame::ListTag(stdout, aFrameList);
  if (aPrevFrame) {
    printf(" after ");
    nsFrame::ListTag(stdout, aPrevFrame);
  }
  printf("\n");
#endif
  nsresult rv = AddFrames(aPresContext, aFrameList, aPrevFrame);
#ifdef IBMBIDI
  if (aListName != nsLayoutAtoms::nextBidi)
#endif // IBMBIDI
  if (NS_SUCCEEDED(rv)) {
    // Ask the parent frame to reflow me.
    ReflowDirtyChild(&aPresShell, nsnull);
  }
  return rv;
}

nsresult
nsBlockFrame::AddFrames(nsPresContext* aPresContext,
                        nsIFrame* aFrameList,
                        nsIFrame* aPrevSibling)
{
  // Clear our line cursor, since our lines may change.
  ClearLineCursor();

  if (nsnull == aFrameList) {
    return NS_OK;
  }

  nsIPresShell *presShell = aPresContext->PresShell();

  // Attempt to find the line that contains the previous sibling
  nsLineList::iterator prevSibLine = end_lines();
  PRInt32 prevSiblingIndex = -1;
  if (aPrevSibling) {
    // XXX_perf This is technically O(N^2) in some cases, but by using
    // RFind instead of Find, we make it O(N) in the most common case,
    // which is appending cotent.

    // Find the line that contains the previous sibling
    if (! nsLineBox::RFindLineContaining(aPrevSibling,
                                         begin_lines(), prevSibLine,
                                         &prevSiblingIndex)) {
      // Note: defensive code! RFindLineContaining must not return
      // false in this case, so if it does...
      NS_NOTREACHED("prev sibling not in line list");
      aPrevSibling = nsnull;
      prevSibLine = end_lines();
    }
  }

  // Find the frame following aPrevSibling so that we can join up the
  // two lists of frames.
  nsIFrame* prevSiblingNextFrame = nsnull;
  if (aPrevSibling) {
    prevSiblingNextFrame = aPrevSibling->GetNextSibling();

    // Split line containing aPrevSibling in two if the insertion
    // point is somewhere in the middle of the line.
    PRInt32 rem = prevSibLine->GetChildCount() - prevSiblingIndex - 1;
    if (rem) {
      // Split the line in two where the frame(s) are being inserted.
      nsLineBox* line = NS_NewLineBox(presShell, prevSiblingNextFrame, rem, PR_FALSE);
      if (!line) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
      mLines.after_insert(prevSibLine, line);
      prevSibLine->SetChildCount(prevSibLine->GetChildCount() - rem);
      prevSibLine->MarkDirty();
    }

    // Now (partially) join the sibling lists together
    aPrevSibling->SetNextSibling(aFrameList);
  }
  else if (! mLines.empty()) {
    prevSiblingNextFrame = mLines.front()->mFirstChild;
  }

  // Walk through the new frames being added and update the line data
  // structures to fit.
  nsIFrame* newFrame = aFrameList;
  while (newFrame) {
    PRBool isBlock = nsLineLayout::TreatFrameAsBlock(newFrame);

    // If the frame is a block frame, or if there is no previous line or if the
    // previous line is a block line or ended with a <br> then make a new line.
    if (isBlock || prevSibLine == end_lines() || prevSibLine->IsBlock() ||
        (aPrevSibling && aPrevSibling->GetType() == nsLayoutAtoms::brFrame)) {
      // Create a new line for the frame and add its line to the line
      // list.
      nsLineBox* line = NS_NewLineBox(presShell, newFrame, 1, isBlock);
      if (!line) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
      if (prevSibLine != end_lines()) {
        // Append new line after prevSibLine
        mLines.after_insert(prevSibLine, line);
        ++prevSibLine;
      }
      else {
        // New line is going before the other lines
        mLines.push_front(line);
        prevSibLine = begin_lines();
      }
    }
    else {
      prevSibLine->SetChildCount(prevSibLine->GetChildCount() + 1);
      prevSibLine->MarkDirty();
    }

    aPrevSibling = newFrame;
    newFrame = newFrame->GetNextSibling();
  }
  if (prevSiblingNextFrame) {
    // Connect the last new frame to the remainder of the sibling list
    aPrevSibling->SetNextSibling(prevSiblingNextFrame);
  }

#ifdef DEBUG
  VerifyLines(PR_TRUE);
#endif
  return NS_OK;
}

nsBlockFrame::line_iterator
nsBlockFrame::RemoveFloat(nsIFrame* aFloat) {
  // Find which line contains the float
  line_iterator line = begin_lines(), line_end = end_lines();
  for ( ; line != line_end; ++line) {
    if (line->IsInline() && line->RemoveFloat(aFloat)) {
      break;
    }
  }
  
  mFloats.DestroyFrame(GetPresContext(), aFloat);

  return line;
}

NS_IMETHODIMP
nsBlockFrame::RemoveFrame(nsPresContext* aPresContext,
                          nsIPresShell&   aPresShell,
                          nsIAtom*        aListName,
                          nsIFrame*       aOldFrame)
{
  nsresult rv = NS_OK;

#ifdef NOISY_REFLOW_REASON
    ListTag(stdout);
    printf(": remove ");
    nsFrame::ListTag(stdout, aOldFrame);
    printf("\n");
#endif

  if (nsnull == aListName) {
    rv = DoRemoveFrame(aPresContext, aOldFrame);
  }
  else if (mAbsoluteContainer.GetChildListName() == aListName) {
    return mAbsoluteContainer.RemoveFrame(this, aPresContext, aPresShell,
                                          aListName, aOldFrame);
  }
  else if (nsLayoutAtoms::floatList == aListName) {
    line_iterator line = RemoveFloat(aOldFrame);
    line_iterator line_end = end_lines();

    // Mark every line at and below the line where the float was dirty
    // XXXldb This could be done more efficiently.
    for ( ; line != line_end; ++line) {
      line->MarkDirty();
    }
  }
#ifdef IBMBIDI
  else if (nsLayoutAtoms::nextBidi == aListName) {
    // Skip the call to |ReflowDirtyChild| below by returning now.
    return DoRemoveFrame(aPresContext, aOldFrame);
  }
#endif // IBMBIDI
  else {
    rv = NS_ERROR_INVALID_ARG;
  }

  if (NS_SUCCEEDED(rv)) {
    // Ask the parent frame to reflow me.
    ReflowDirtyChild(&aPresShell, nsnull);  
  }
  return rv;
}

void
nsBlockFrame::DoRemoveOutOfFlowFrame(nsPresContext* aPresContext,
                                     nsIFrame*       aFrame)
{
  // First remove aFrame's next in flow
  nsIFrame* nextInFlow = aFrame->GetNextInFlow();
  if (nextInFlow) {
    nsBlockFrame::DoRemoveOutOfFlowFrame(aPresContext, nextInFlow);
  }
  // Now remove aFrame
  const nsStyleDisplay* display = aFrame->GetStyleDisplay();
  // find the containing block, this is either the parent or the grandparent
  // if the parent is an inline frame
  nsIFrame* parent = aFrame->GetParent();
  nsIAtom* parentType = parent->GetType();
  while ((nsLayoutAtoms::blockFrame != parentType) &&
         (nsLayoutAtoms::areaFrame  != parentType)) {
    parent = parent->GetParent();
    parentType = parent->GetType();
  }
  
  nsBlockFrame* block = (nsBlockFrame*)parent;
  // Remove aFrame from the appropriate list. 
  if (display->IsAbsolutelyPositioned()) {
    block->mAbsoluteContainer.RemoveFrame(block, aPresContext,
                                          *(aPresContext->PresShell()),
                                          block->mAbsoluteContainer.GetChildListName(), aFrame);
    aFrame->Destroy(aPresContext);
  }
  else {
    // This also destroys the frame.
    block->RemoveFloat(aFrame);
  }
}

// This helps us iterate over the list of all normal + overflow lines
void
nsBlockFrame::TryAllLines(nsLineList::iterator* aIterator,
                          nsLineList::iterator* aEndIterator,
                          PRBool* aInOverflowLines) {
  if (*aIterator == *aEndIterator) {
    if (!*aInOverflowLines) {
      *aInOverflowLines = PR_TRUE;
      // Try the overflow lines
      nsLineList* overflowLines = GetOverflowLines();
      if (overflowLines) {
        *aIterator = overflowLines->begin();
        *aEndIterator = overflowLines->end();
      }
    }
  }
}

// This function removes aDeletedFrame and all its continuations.  It
// is optimized for deleting a whole series of frames. The easy
// implementation would invoke itself recursively on
// aDeletedFrame->GetNextInFlow, then locate the line containing
// aDeletedFrame and remove aDeletedFrame from that line. But here we
// start by locating aDeletedFrame and then scanning from that point
// on looking for continuations.
nsresult
nsBlockFrame::DoRemoveFrame(nsPresContext* aPresContext,
                            nsIFrame* aDeletedFrame)
{
  // Clear our line cursor, since our lines may change.
  ClearLineCursor();
        
  if (aDeletedFrame->GetStateBits() & NS_FRAME_OUT_OF_FLOW) {
    DoRemoveOutOfFlowFrame(aPresContext, aDeletedFrame);
    return NS_OK;
  }

  nsIPresShell *presShell = aPresContext->PresShell();
  
  // Find the line and the previous sibling that contains
  // deletedFrame; we also find the pointer to the line.
  nsLineList::iterator line = mLines.begin(),
                       line_end = mLines.end();
  PRBool searchingOverflowList = PR_FALSE;
  nsIFrame* prevSibling = nsnull;
  // Make sure we look in the overflow lines even if the normal line
  // list is empty
  TryAllLines(&line, &line_end, &searchingOverflowList);
  while (line != line_end) {
    nsIFrame* frame = line->mFirstChild;
    PRInt32 n = line->GetChildCount();
    while (--n >= 0) {
      if (frame == aDeletedFrame) {
        goto found_frame;
      }
      prevSibling = frame;
      frame = frame->GetNextSibling();
    }
    ++line;
    TryAllLines(&line, &line_end, &searchingOverflowList);
  }
found_frame:;
  if (line == line_end) {
    NS_ERROR("can't find deleted frame in lines");
    return NS_ERROR_FAILURE;
  }

  if (prevSibling && !prevSibling->GetNextSibling()) {
    // We must have found the first frame in the overflow line list. So
    // there is no prevSibling
    prevSibling = nsnull;
  }
  NS_ASSERTION(!prevSibling || prevSibling->GetNextSibling() == aDeletedFrame, "bad prevSibling");

  while ((line != line_end) && (nsnull != aDeletedFrame)) {
    NS_ASSERTION(this == aDeletedFrame->GetParent(), "messed up delete code");
    NS_ASSERTION(line->Contains(aDeletedFrame), "frame not in line");

    // See if the frame being deleted is the last one on the line
    PRBool isLastFrameOnLine = PR_FALSE;
    if (1 == line->GetChildCount()) {
      isLastFrameOnLine = PR_TRUE;
    }
    else if (line->LastChild() == aDeletedFrame) {
      isLastFrameOnLine = PR_TRUE;
    }

    // Remove aDeletedFrame from the line
    nsIFrame* nextFrame = aDeletedFrame->GetNextSibling();
    if (line->mFirstChild == aDeletedFrame) {
      // We should be setting this to null if aDeletedFrame
      // is the only frame on the line. HOWEVER in that case
      // we will be removing the line anyway, see below.
      line->mFirstChild = nextFrame;
    }

    // Hmm, this won't do anything if we're removing a frame in the first
    // overflow line... Hopefully doesn't matter
    --line;
    if (line != line_end && !line->IsBlock()) {
      // Since we just removed a frame that follows some inline
      // frames, we need to reflow the previous line.
      line->MarkDirty();
    }
    ++line;

    // Take aDeletedFrame out of the sibling list. Note that
    // prevSibling will only be nsnull when we are deleting the very
    // first frame in the main or overflow list.
    if (prevSibling) {
      prevSibling->SetNextSibling(nextFrame);
    }

    // Update the child count of the line to be accurate
    PRInt32 lineChildCount = line->GetChildCount();
    lineChildCount--;
    line->SetChildCount(lineChildCount);

    // Destroy frame; capture its next-in-flow first in case we need
    // to destroy that too.
    nsIFrame* deletedNextInFlow = aDeletedFrame->GetNextInFlow();
#ifdef NOISY_REMOVE_FRAME
    printf("DoRemoveFrame: line=%p frame=", line.get());
    nsFrame::ListTag(stdout, aDeletedFrame);
    printf(" prevSibling=%p deletedNextInFlow=%p\n", prevSibling, deletedNextInFlow);
#endif
    aDeletedFrame->Destroy(aPresContext);
    aDeletedFrame = deletedNextInFlow;

    // If line is empty, remove it now.
    if (0 == lineChildCount) {
      nsLineBox *cur = line;
      if (!searchingOverflowList) {
        line = mLines.erase(line);
        // Invalidate the space taken up by the line.
        // XXX We need to do this if we're removing a frame as a result of
        // a call to RemoveFrame(), but we may not need to do this in all
        // cases...
        nsRect lineCombinedArea(cur->GetCombinedArea());
#ifdef NOISY_BLOCK_INVALIDATE
        printf("%p invalidate 10 (%d, %d, %d, %d)\n",
               this, lineCombinedArea.x, lineCombinedArea.y, lineCombinedArea.width, lineCombinedArea.height);
#endif
        Invalidate(lineCombinedArea);
      } else {
        nsLineList* lineList = RemoveOverflowLines();
        line = lineList->erase(line);
        if (!lineList->empty()) {
          SetOverflowLines(lineList);
        }
      }
      cur->Destroy(presShell);

      // If we're removing a line, ReflowDirtyLines isn't going to
      // know that it needs to slide lines unless something is marked
      // dirty.  So mark the previous margin of the next line dirty if
      // there is one.
      if (line != line_end) {
        line->MarkPreviousMarginDirty();
      }
    } else {
      // Make the line that just lost a frame dirty, and advance to
      // the next line.
      if (!deletedNextInFlow || !line->Contains(deletedNextInFlow)) {
        line->MarkDirty();
        ++line;
      }
    }

    if (nsnull != deletedNextInFlow) {
      // See if we should keep looking in the current flow's line list.
      if (deletedNextInFlow->GetParent() != this) {
        // The deceased frames continuation is not a child of the
        // current block. So break out of the loop so that we advance
        // to the next parent.
        break;
      }

      // If we just removed the last frame on the line then we need
      // to advance to the next line.
      if (isLastFrameOnLine) {
        TryAllLines(&line, &line_end, &searchingOverflowList);
        // Detect the case when we've run off the end of the normal line
        // list and we're starting the overflow line list
        if (prevSibling && !prevSibling->GetNextSibling()) {
          prevSibling = nsnull;
        }
      }
    }
  }

#ifdef DEBUG
  VerifyLines(PR_TRUE);
#endif

  // Advance to next flow block if the frame has more continuations
  if (aDeletedFrame) {
    nsBlockFrame* nextBlock = NS_STATIC_CAST(nsBlockFrame*, aDeletedFrame->GetParent());
    NS_ASSERTION(nextBlock->GetType() == nsLayoutAtoms::blockFrame,
                 "Our child's continuation's parent is not a block?");
    return nextBlock->DoRemoveFrame(aPresContext, aDeletedFrame);
  }

  return NS_OK;
}

void
nsBlockFrame::DeleteNextInFlowChild(nsPresContext* aPresContext,
                                    nsIFrame*       aNextInFlow)
{
  nsIFrame* prevInFlow = aNextInFlow->GetPrevInFlow();
  NS_PRECONDITION(prevInFlow, "bad next-in-flow");
  NS_PRECONDITION(IsChild(aNextInFlow), "bad geometric parent");

#ifdef IBMBIDI
  if (!(prevInFlow->GetStateBits() & NS_FRAME_IS_BIDI) ||
      (NS_STATIC_CAST(nsIFrame*,
                      aPresContext->PropertyTable()->GetProperty(prevInFlow, nsLayoutAtoms::nextBidi)) !=
       aNextInFlow))
#endif // IBMBIDI
    DoRemoveFrame(aPresContext, aNextInFlow);
}

////////////////////////////////////////////////////////////////////////
// Float support

static void InitReflowStateForFloat(nsHTMLReflowState* aState, nsPresContext* aPresContext)
{
  /* We build a different reflow context based on the width attribute of the block
   * when it's a float.
   * Auto-width floats need to have their containing-block size set explicitly.
   * factoring in other floats that impact it.  
   * It's possible this should be quirks-only.
   */
  // XXXldb We should really fix this in nsHTMLReflowState::InitConstraints instead.
  const nsStylePosition* position = aState->frame->GetStylePosition();
  nsStyleUnit widthUnit = position->mWidth.GetUnit();

  if (eStyleUnit_Auto == widthUnit) {
    // Initialize the reflow state and constrain the containing block's 
    // width and height to the available width and height.
    aState->Init(aPresContext, aState->availableWidth, aState->availableHeight);
  } else {
    // Initialize the reflow state and use the containing block's
    // computed width and height (or derive appropriate values for an
    // absolutely positioned frame).
    aState->Init(aPresContext);
  }
}

nsresult
nsBlockFrame::ReflowFloat(nsBlockReflowState& aState,
                          nsPlaceholderFrame* aPlaceholder,
                          nsFloatCache*     aFloatCache,
                          nsReflowStatus&     aReflowStatus)
{
  // Reflow the float.
  nsIFrame* floatFrame = aPlaceholder->GetOutOfFlowFrame();
  aReflowStatus = NS_FRAME_COMPLETE;

#ifdef NOISY_FLOAT
  printf("Reflow Float %p in parent %p, availSpace(%d,%d,%d,%d)\n",
          aPlaceholder->GetOutOfFlowFrame(), this, 
          aState.mAvailSpaceRect.x, aState.mAvailSpaceRect.y, 
          aState.mAvailSpaceRect.width, aState.mAvailSpaceRect.height
  );
#endif

  // Compute the available width. By default, assume the width of the
  // containing block.
  nscoord availWidth;
  if (aState.GetFlag(BRS_UNCONSTRAINEDWIDTH)) {
    availWidth = NS_UNCONSTRAINEDSIZE;
  }
  else {
    const nsStyleDisplay* floatDisplay = floatFrame->GetStyleDisplay();

    nsIFrame* prevInFlow = floatFrame->GetPrevInFlow();
    // if the float is continued, constrain its width to the prev-in-flow's width
    if (prevInFlow) {
      availWidth = prevInFlow->GetRect().width;
    }
    else if (NS_STYLE_DISPLAY_TABLE != floatDisplay->mDisplay ||
             eCompatibility_NavQuirks != aState.mPresContext->CompatibilityMode() ) {
      availWidth = aState.mContentArea.width;
    }
    else {
      // This quirk matches the one in nsBlockReflowState::FlowAndPlaceFloat
      // give tables only the available space
      // if they can shrink we may not be constrained to place
      // them in the next line
      availWidth = aState.mAvailSpaceRect.width;
      // round down to twips per pixel so that we fit
      // needed when prev. float has procentage width
      // (maybe is a table flaw that makes table chose to round up
      // but i don't want to change that, too risky)
      nscoord twp = aState.mPresContext->IntScaledPixelsToTwips(1);
      availWidth -=  availWidth % twp;
    }
  }
  nscoord availHeight = ((NS_UNCONSTRAINEDSIZE == aState.mAvailSpaceRect.height) ||
                         (NS_UNCONSTRAINEDSIZE == aState.mContentArea.height))
                        ? NS_UNCONSTRAINEDSIZE 
                        : PR_MAX(0, aState.mContentArea.height - aState.mY);

  // If the float's width is automatic, we can't let the float's
  // width shrink below its maxElementWidth.
  const nsStylePosition* position = floatFrame->GetStylePosition();
  PRBool isAutoWidth = (eStyleUnit_Auto == position->mWidth.GetUnit());

  // We'll need to compute the max element size if either 1) we're
  // auto-width or 2) the state wanted us to compute it anyway.
  PRBool computeMaxElementWidth =
    isAutoWidth || aState.GetFlag(BRS_COMPUTEMAXELEMENTWIDTH);

  nsRect availSpace(aState.BorderPadding().left,
                    aState.BorderPadding().top,
                    availWidth, availHeight);

  // construct the html reflow state for the float. ReflowBlock will 
  // initialize it.
  nsHTMLReflowState floatRS(aState.mPresContext, aState.mReflowState,
                            floatFrame, 
                            nsSize(availSpace.width, availSpace.height), 
                            aState.mReflowState.reason, PR_FALSE);

  InitReflowStateForFloat(&floatRS, aState.mPresContext);

  // Setup a block reflow state to reflow the float.
  nsBlockReflowContext brc(aState.mPresContext, aState.mReflowState,
                           computeMaxElementWidth,
                           aState.GetFlag(BRS_COMPUTEMAXWIDTH));

  // Reflow the float
  PRBool isAdjacentWithTop = aState.IsAdjacentWithTop();

  nsIFrame* clearanceFrame = nsnull;
  nsresult rv;
  do {
    nsCollapsingMargin margin;
    PRBool mayNeedRetry = PR_FALSE;
    nsBlockReflowContext::ComputeCollapsedTopMargin(floatRS, &margin,
                                                    clearanceFrame, &mayNeedRetry);

    if (mayNeedRetry && !clearanceFrame) {
      floatRS.mDiscoveredClearance = &clearanceFrame;
      // We don't need to push the space manager state because the the block has its own
      // space manager that will be destroyed and recreated
    } else {
      floatRS.mDiscoveredClearance = nsnull;
    }

    rv = brc.ReflowBlock(availSpace, PR_TRUE, margin,
                         0, isAdjacentWithTop,
                         aFloatCache->mOffsets, floatRS,
                         aReflowStatus);
  } while (NS_SUCCEEDED(rv) && clearanceFrame);

  // An incomplete reflow status means we should split the float 
  // if the height is constrained (bug 145305). 
  if (NS_FRAME_IS_NOT_COMPLETE(aReflowStatus) && (NS_UNCONSTRAINEDSIZE == availHeight)) 
    aReflowStatus = NS_FRAME_COMPLETE;

  if (NS_FRAME_IS_COMPLETE(aReflowStatus)) {
    // Float is now complete, so delete the placeholder's next in
    // flows, if any; their floats (which are this float's continuations)
    // have already been deleted.
    nsIFrame* nextInFlow = aPlaceholder->GetNextInFlow();
    if (nextInFlow) {
      // If aPlaceholder's parent is an inline, nextInFlow's will be a block.
      NS_STATIC_CAST(nsHTMLContainerFrame*, nextInFlow->GetParent())
        ->DeleteNextInFlowChild(aState.mPresContext, nextInFlow);
    }
  }

  if (NS_SUCCEEDED(rv) && isAutoWidth) {
    nscoord maxElementWidth = brc.GetMaxElementWidth();
    if (maxElementWidth > availSpace.width) {
      // The float's maxElementWidth is larger than the available
      // width. Reflow it again, this time pinning the width to the
      // maxElementWidth.
      availSpace.width = maxElementWidth;
      nsCollapsingMargin marginMEW;
      // construct the html reflow state for the float. 
      // ReflowBlock will initialize it.
      nsHTMLReflowState redoFloatRS(aState.mPresContext, aState.mReflowState,
                                    floatFrame, 
                                    nsSize(availSpace.width, availSpace.height), 
                                    aState.mReflowState.reason, PR_FALSE);

      InitReflowStateForFloat(&redoFloatRS, aState.mPresContext);

      clearanceFrame = nsnull;
      do {
        nsCollapsingMargin marginMEW;
        PRBool mayNeedRetry = PR_FALSE;
        nsBlockReflowContext::ComputeCollapsedTopMargin(redoFloatRS, &marginMEW, clearanceFrame, &mayNeedRetry);

        if (mayNeedRetry && !clearanceFrame) {
          redoFloatRS.mDiscoveredClearance = &clearanceFrame;
          // We don't need to push the space manager state because the
          // the block has its own space manager that will be
          // destroyed and recreated
        } else {
          redoFloatRS.mDiscoveredClearance = nsnull;
        }

        rv = brc.ReflowBlock(availSpace, PR_TRUE, marginMEW,
                             0, isAdjacentWithTop,
                             aFloatCache->mOffsets, redoFloatRS,
                             aReflowStatus);
      } while (NS_SUCCEEDED(rv) && clearanceFrame);
    }
  }

  if (floatFrame->GetType() == nsLayoutAtoms::letterFrame) {
    // We never split floating first letters; an incomplete state for
    // such frames simply means that there is more content to be
    // reflowed on the line.
    if (NS_FRAME_IS_NOT_COMPLETE(aReflowStatus)) 
      aReflowStatus = NS_FRAME_COMPLETE;
  }

  // Remove the float from the reflow tree.
  if (aState.mReflowState.path)
    aState.mReflowState.path->RemoveChild(floatFrame);

  if (NS_FAILED(rv)) {
    return rv;
  }

  // Capture the margin information for the caller
  const nsMargin& m = brc.GetMargin();
  aFloatCache->mMargins.top = brc.GetTopMargin();
  aFloatCache->mMargins.right = m.right;
  brc.GetCarriedOutBottomMargin().Include(m.bottom);
  aFloatCache->mMargins.bottom = brc.GetCarriedOutBottomMargin().get();
  aFloatCache->mMargins.left = m.left;

  const nsHTMLReflowMetrics& metrics = brc.GetMetrics();
  aFloatCache->mCombinedArea = metrics.mOverflowArea;

  // Set the rect, make sure the view is properly sized and positioned,
  // and tell the frame we're done reflowing it
  // XXXldb This seems like the wrong place to be doing this -- shouldn't
  // we be doing this in nsBlockReflowState::FlowAndPlaceFloat after
  // we've positioned the float, and shouldn't we be doing the equivalent
  // of |::PlaceFrameView| here?
  floatFrame->SetSize(nsSize(metrics.width, metrics.height));
  if (floatFrame->HasView()) {
    nsContainerFrame::SyncFrameViewAfterReflow(aState.mPresContext, floatFrame,
                                               floatFrame->GetView(),
                                               &metrics.mOverflowArea,
                                               NS_FRAME_NO_MOVE_VIEW);
  }
  // Pass floatRS so the frame hierarchy can be used (redoFloatRS has the same hierarchy)  
  floatFrame->DidReflow(aState.mPresContext, &floatRS,
                        NS_FRAME_REFLOW_FINISHED);

  // If we computed it, then stash away the max-element-width for later
  if (aState.GetFlag(BRS_COMPUTEMAXELEMENTWIDTH)) {
    nscoord mew = brc.GetMaxElementWidth() +
                  aFloatCache->mMargins.left + aFloatCache->mMargins.right;

    // This is all we need to do to include the float
    // max-element-width since we don't require that we end up with
    // content next to floats.
    aState.UpdateMaxElementWidth(mew); // fix for bug 13553

    // Allow the float width to be restored in state recovery.
    aFloatCache->mMaxElementWidth = mew;
  }
#ifdef NOISY_FLOAT
  printf("end ReflowFloat %p, sized to %d,%d\n",
         floatFrame, metrics.width, metrics.height);
#endif

  // If the placeholder was continued and its first-in-flow was followed by a 
  // <BR>, then cache the <BR>'s break type in aState.mFloatBreakType so that
  // the next frame after the placeholder can combine that break type with its own
  nsIFrame* prevPlaceholder = aPlaceholder->GetPrevInFlow();
  if (prevPlaceholder) {
    // the break occurs only after the last continued placeholder
    PRBool lastPlaceholder = PR_TRUE;
    nsIFrame* next = aPlaceholder->GetNextSibling();
    if (next) {
      if (nsLayoutAtoms::placeholderFrame == next->GetType()) {
        lastPlaceholder = PR_FALSE;
      }
    }
    if (lastPlaceholder) {
      // get the containing block of prevPlaceholder which is our prev-in-flow
      if (mPrevInFlow) {
        // get the break type of the last line in mPrevInFlow
        line_iterator endLine = --((nsBlockFrame*)mPrevInFlow)->end_lines();
        if (endLine->HasFloatBreakAfter()) {
          aState.mFloatBreakType = endLine->GetBreakTypeAfter();
        }
      }
      else NS_ASSERTION(PR_FALSE, "no prev in flow");
    }
  }
  return NS_OK;
}

//////////////////////////////////////////////////////////////////////
// Painting, event handling

PRIntn
nsBlockFrame::GetSkipSides() const
{
  PRIntn skip = 0;
  if (nsnull != mPrevInFlow) {
    skip |= 1 << NS_SIDE_TOP;
  }
  if (nsnull != mNextInFlow) {
    skip |= 1 << NS_SIDE_BOTTOM;
  }
  return skip;
}

#ifdef DEBUG
static void ComputeCombinedArea(nsLineList& aLines,
                                nscoord aWidth, nscoord aHeight,
                                nsRect& aResult)
{
  nscoord xa = 0, ya = 0, xb = aWidth, yb = aHeight;
  for (nsLineList::iterator line = aLines.begin(), line_end = aLines.end();
       line != line_end;
       ++line) {
    // Compute min and max x/y values for the reflowed frame's
    // combined areas
    nsRect lineCombinedArea(line->GetCombinedArea());
    nscoord x = lineCombinedArea.x;
    nscoord y = lineCombinedArea.y;
    nscoord xmost = x + lineCombinedArea.width;
    nscoord ymost = y + lineCombinedArea.height;
    if (x < xa) {
      xa = x;
    }
    if (xmost > xb) {
      xb = xmost;
    }
    if (y < ya) {
      ya = y;
    }
    if (ymost > yb) {
      yb = ymost;
    }
  }

  aResult.x = xa;
  aResult.y = ya;
  aResult.width = xb - xa;
  aResult.height = yb - ya;
}
#endif

NS_IMETHODIMP
nsBlockFrame::IsVisibleForPainting(nsPresContext *     aPresContext, 
                                   nsIRenderingContext& aRenderingContext,
                                   PRBool               aCheckVis,
                                   PRBool*              aIsVisible)
{
  // first check to see if we are visible
  if (aCheckVis) {
    if (!GetStyleVisibility()->IsVisible()) {
      *aIsVisible = PR_FALSE;
      return NS_OK;
    }
  }

  // Start by assuming we are visible and need to be painted
  *aIsVisible = PR_TRUE;

  // NOTE: GetSelectionforVisCheck checks the pagination to make sure we are printing
  // In otherwords, the selection will ALWAYS be null if we are not printing, meaning
  // the visibility will be TRUE in that case
  nsCOMPtr<nsISelection> selection;
  nsresult rv = GetSelectionForVisCheck(aPresContext, getter_AddRefs(selection));
  if (NS_SUCCEEDED(rv) && selection) {
    nsCOMPtr<nsIDOMNode> node(do_QueryInterface(mContent));

    nsCOMPtr<nsIDOMHTMLHtmlElement> html(do_QueryInterface(mContent));
    nsCOMPtr<nsIDOMHTMLBodyElement> body(do_QueryInterface(mContent));

    if (!html && !body) {
      rv = selection->ContainsNode(node, PR_TRUE, aIsVisible);
    }
  }

  return rv;
}

/* virtual */ void
nsBlockFrame::PaintTextDecorationLines(nsIRenderingContext& aRenderingContext, 
                                       nscolor aColor, 
                                       nscoord aOffset, 
                                       nscoord aAscent, 
                                       nscoord aSize) 
{
  aRenderingContext.SetColor(aColor);
  for (nsLineList::iterator line = begin_lines(), line_start = line,
         line_end = end_lines(); 
       line != line_end; ++line) {
    if (!line->IsBlock()) {
      nscoord start = line->mBounds.x;
      nscoord width = line->mBounds.width;

      if (line == line_start) {
        // Adjust for the text-indent.  See similar code in
        // nsLineLayout::BeginLineReflow.
        nscoord indent = 0;
        const nsStyleText* styleText = GetStyleText();
        nsStyleUnit unit = styleText->mTextIndent.GetUnit();
        if (eStyleUnit_Coord == unit) {
          indent = styleText->mTextIndent.GetCoordValue();
        } else if (eStyleUnit_Percent == unit) {
          // It's a percentage of the containing block width.
          nsIFrame* containingBlock =
            nsHTMLReflowState::GetContainingBlockFor(this);
          NS_ASSERTION(containingBlock, "Must have containing block!");
          indent = nscoord(styleText->mTextIndent.GetPercentValue() *
                           containingBlock->GetRect().width);
        }

        // Adjust the start position and the width of the decoration by the
        // value of the indent.  Note that indent can be negative; that's OK.
        // It'll just increase the width (which can also happen to be
        // negative!).
        start += indent;
        width -= indent;
      }
      
      // Only paint if we have a positive width
      if (width > 0) {
        aRenderingContext.FillRect(start,
                                   line->mBounds.y + line->GetAscent() - aOffset, 
                                   width, aSize);
      }
    }
  }
}

NS_IMETHODIMP
nsBlockFrame::Paint(nsPresContext*      aPresContext,
                    nsIRenderingContext& aRenderingContext,
                    const nsRect&        aDirtyRect,
                    nsFramePaintLayer    aWhichLayer,
                    PRUint32             aFlags)
{
  if (NS_FRAME_IS_UNFLOWABLE & mState) {
    return NS_OK;
  }

#ifdef DEBUG
  if (gNoisyDamageRepair) {
    if (NS_FRAME_PAINT_LAYER_BACKGROUND == aWhichLayer) {
      PRInt32 depth = GetDepth();
      nsRect ca;
      ::ComputeCombinedArea(mLines, mRect.width, mRect.height, ca);
      nsFrame::IndentBy(stdout, depth);
      ListTag(stdout);
      printf(": bounds=%d,%d,%d,%d dirty=%d,%d,%d,%d ca=%d,%d,%d,%d\n",
             mRect.x, mRect.y, mRect.width, mRect.height,
             aDirtyRect.x, aDirtyRect.y, aDirtyRect.width, aDirtyRect.height,
             ca.x, ca.y, ca.width, ca.height);
    }
  }
#endif  

  if (NS_FRAME_PAINT_LAYER_BACKGROUND == aWhichLayer) {
    PaintSelf(aPresContext, aRenderingContext, aDirtyRect);
  }

  PRBool paintingSuppressed = PR_FALSE;  
  aPresContext->PresShell()->IsPaintingSuppressed(&paintingSuppressed);
  if (paintingSuppressed)
    return NS_OK;

  const nsStyleDisplay* disp = GetStyleDisplay();

  // If overflow is hidden then set the clip rect so that children don't
  // leak out of us. Note that because overflow'-clip' only applies to
  // the content area we do this after painting the border and background
  if (NS_STYLE_OVERFLOW_CLIP == disp->mOverflowX) {
    aRenderingContext.PushState();
    SetOverflowClipRect(aRenderingContext);
  }

  // Child elements have the opportunity to override the visibility
  // property and display even if the parent is hidden
  if (NS_FRAME_PAINT_LAYER_FLOATS == aWhichLayer) {
    PaintFloats(aPresContext, aRenderingContext, aDirtyRect);
  }

  PaintDecorationsAndChildren(aPresContext, aRenderingContext,
                              aDirtyRect, aWhichLayer, PR_TRUE);

  if (NS_STYLE_OVERFLOW_CLIP == disp->mOverflowX)
    aRenderingContext.PopState();

#if 0
  if ((NS_FRAME_PAINT_LAYER_DEBUG == aWhichLayer) && GetShowFrameBorders()) {
    // Render the bands in the spacemanager
    nsSpaceManager* sm = mSpaceManager;

    if (nsnull != sm) {
      nsBlockBandData band;
      band.Init(sm, nsSize(mRect.width, mRect.height));
      nscoord y = 0;
      while (y < mRect.height) {
        nsRect availArea;
        band.GetAvailableSpace(y, availArea);
  
        // Render a box and a diagonal line through the band
        aRenderingContext.SetColor(NS_RGB(0,255,0));
        aRenderingContext.DrawRect(0, availArea.y,
                                   mRect.width, availArea.height);
        aRenderingContext.DrawLine(0, availArea.y,
                                   mRect.width, availArea.YMost());
  
        // Render boxes and opposite diagonal lines around the
        // unavailable parts of the band.
        PRInt32 i;
        for (i = 0; i < band.GetTrapezoidCount(); i++) {
          const nsBandTrapezoid* trapezoid = band.GetTrapezoid(i);
          if (nsBandTrapezoid::Available != trapezoid->mState) {
            nsRect r = trapezoid->GetRect();
            if (nsBandTrapezoid::OccupiedMultiple == trapezoid->mState) {
              aRenderingContext.SetColor(NS_RGB(0,255,128));
            }
            else {
              aRenderingContext.SetColor(NS_RGB(128,255,0));
            }
            aRenderingContext.DrawRect(r);
            aRenderingContext.DrawLine(r.x, r.YMost(), r.XMost(), r.y);
          }
        }
        y = availArea.YMost();
      }
    }
  }
#endif

  return NS_OK;
}

void
nsBlockFrame::PaintFloats(nsPresContext* aPresContext,
                          nsIRenderingContext& aRenderingContext,
                          const nsRect& aDirtyRect)
{
  for (nsIFrame* floatFrame = mFloats.FirstChild();
       floatFrame;
       floatFrame = floatFrame->GetNextSibling()) {
    PaintChild(aPresContext, aRenderingContext, aDirtyRect,
               floatFrame, NS_FRAME_PAINT_LAYER_BACKGROUND);
    PaintChild(aPresContext, aRenderingContext, aDirtyRect,
               floatFrame, NS_FRAME_PAINT_LAYER_FLOATS);
    PaintChild(aPresContext, aRenderingContext, aDirtyRect,
               floatFrame, NS_FRAME_PAINT_LAYER_FOREGROUND);
  }
}

#ifdef DEBUG
static void DebugOutputDrawLine(nsFramePaintLayer aWhichLayer, PRInt32 aDepth,
                                nsLineBox* aLine, PRBool aDrawn) {
  if (nsBlockFrame::gNoisyDamageRepair &&
      (NS_FRAME_PAINT_LAYER_BACKGROUND == aWhichLayer)) {
    nsFrame::IndentBy(stdout, aDepth+1);
    nsRect lineArea = aLine->GetCombinedArea();
    printf("%s line=%p bounds=%d,%d,%d,%d ca=%d,%d,%d,%d\n",
           aDrawn ? "draw" : "skip",
           NS_STATIC_CAST(void*, aLine),
           aLine->mBounds.x, aLine->mBounds.y,
           aLine->mBounds.width, aLine->mBounds.height,
           lineArea.x, lineArea.y,
           lineArea.width, lineArea.height);
  }
}
#endif

static inline void
PaintLine(const nsRect& aLineArea, const nsRect& aDirtyRect,
          nsBlockFrame::line_iterator& aLine, PRInt32 aDepth,
          PRInt32& aDrawnLines, nsPresContext* aPresContext, 
          nsIRenderingContext& aRenderingContext,
          nsFramePaintLayer aWhichLayer, nsBlockFrame* aFrame) {
  // If the line's combined area (which includes child frames that
  // stick outside of the line's bounding box or our bounding box)
  // intersects the dirty rect then paint the line.
  if (aLineArea.Intersects(aDirtyRect)) {
#ifdef DEBUG
    DebugOutputDrawLine(aWhichLayer, aDepth, aLine.get(), PR_TRUE);
    if (nsBlockFrame::gLamePaintMetrics) {
      aDrawnLines++;
    }
#endif
    nsIFrame* kid = aLine->mFirstChild;
    PRInt32 n = aLine->GetChildCount();
    while (--n >= 0) {
      aFrame->PaintChild(aPresContext, aRenderingContext, aDirtyRect, kid,
                         aWhichLayer);
      kid = kid->GetNextSibling();
    }
  }
#ifdef DEBUG
  else {
    DebugOutputDrawLine(aWhichLayer, aDepth, aLine.get(), PR_FALSE);
  }
#endif  
}

void
nsBlockFrame::PaintChildren(nsPresContext*      aPresContext,
                            nsIRenderingContext& aRenderingContext,
                            const nsRect&        aDirtyRect,
                            nsFramePaintLayer    aWhichLayer,
                            PRUint32             aFlags)
{
  PRInt32 drawnLines; // Will only be used if set (gLamePaintMetrics).
  PRInt32 depth = 0;
#ifdef DEBUG
  if (gNoisyDamageRepair) {
    if (NS_FRAME_PAINT_LAYER_BACKGROUND == aWhichLayer) {
      depth = GetDepth();
    }
  }
  PRTime start = LL_ZERO; // Initialize these variables to silence the compiler.
  if (gLamePaintMetrics) {
    start = PR_Now();
    drawnLines = 0;
  }
#endif

  nsLineBox* cursor = GetFirstLineContaining(aDirtyRect.y);
  line_iterator line_end = end_lines();

  if (cursor) {
    for (line_iterator line = mLines.begin(cursor);
         line != line_end;
         ++line) {
      nsRect lineArea = line->GetCombinedArea();
      if (!lineArea.IsEmpty()) {
        // Because we have a cursor, the combinedArea.ys are non-decreasing.
        // Once we've passed aDirtyRect.YMost(), we can never see it again.
        if (lineArea.y >= aDirtyRect.YMost()) {
          break;
        }
        PaintLine(lineArea, aDirtyRect, line, depth, drawnLines, aPresContext,
                  aRenderingContext, aWhichLayer, this);
      }
    }
  } else {
    PRBool nonDecreasingYs = PR_TRUE;
    PRInt32 lineCount = 0;
    nscoord lastY = PR_INT32_MIN;
    nscoord lastYMost = PR_INT32_MIN;
    for (line_iterator line = begin_lines();
         line != line_end;
         ++line) {
      nsRect lineArea = line->GetCombinedArea();
      if (!lineArea.IsEmpty()) {
        if (lineArea.y < lastY
            || lineArea.YMost() < lastYMost) {
          nonDecreasingYs = PR_FALSE;
        }
        lastY = lineArea.y;
        lastYMost = lineArea.YMost();

        PaintLine(lineArea, aDirtyRect, line, depth, drawnLines, aPresContext,
                  aRenderingContext, aWhichLayer, this);
      }
      lineCount++;
    }

    if (nonDecreasingYs && lineCount >= MIN_LINES_NEEDING_CURSOR) {
      SetupLineCursor();
    }
  }

  if (NS_FRAME_PAINT_LAYER_FOREGROUND == aWhichLayer) {
    if ((nsnull != mBullet) && HaveOutsideBullet()) {
      // Paint outside bullets manually
      PaintChild(aPresContext, aRenderingContext, aDirtyRect, mBullet,
                 aWhichLayer);
    }
  }

#ifdef DEBUG
  if (gLamePaintMetrics) {
    PRTime end = PR_Now();

    PRInt32 numLines = mLines.size();
    if (!numLines) numLines = 1;
    PRTime lines, deltaPerLine, delta;
    LL_I2L(lines, numLines);
    LL_SUB(delta, end, start);
    LL_DIV(deltaPerLine, delta, lines);

    ListTag(stdout);
    char buf[400];
    PR_snprintf(buf, sizeof(buf),
                ": %lld elapsed (%lld per line) lines=%d drawn=%d skip=%d",
                delta, deltaPerLine,
                numLines, drawnLines, numLines - drawnLines);
    printf("%s\n", buf);
  }
#endif
}

// XXXldb Does this handle all overlap cases correctly?  (How?)
nsresult
nsBlockFrame::GetClosestLine(nsILineIterator *aLI, 
                             const nsPoint &aOrigin, 
                             const nsPoint &aPoint, 
                             PRInt32 &aClosestLine)
{
  if (!aLI)
    return NS_ERROR_NULL_POINTER;

  nsRect rect;
  PRInt32 numLines;
  PRInt32 lineFrameCount;
  nsIFrame *firstFrame;
  PRUint32 flags;

  nsresult result = aLI->GetNumLines(&numLines);

  if (NS_FAILED(result) || numLines < 0)
    return NS_OK;//do not handle

  PRInt32 shifted = numLines;
  PRInt32 start = 0, midpoint = 0;
  PRInt32 y = 0;

  while(shifted > 0)
  {
    // Cut the number of lines to look at in half and
    // calculate the midpoint of the region we are looking at.

    shifted >>= 1; //divide by 2
    midpoint  = start + shifted;

    // Get the dimensions of the line that is at the half
    // point of the region we are looking at.

    result = aLI->GetLine(midpoint, &firstFrame, &lineFrameCount,rect,&flags);
    if (NS_FAILED(result))
      break;//do not handle

    // Check to see if our point lies with the line's Y bounds.

    rect+=aOrigin; //offset origin to get comparative coordinates

    y = aPoint.y - rect.y;
    if (y >=0 && (aPoint.y < (rect.y+rect.height)))
    {
      aClosestLine = midpoint; //spot on!
      return NS_OK;
    }

    if (y > 0)
    {
      // If we get here, no match was found above, so aPoint.y must
      // be greater than the Y bounds of the current line rect. Move
      // our starting point just beyond the midpoint of the current region.

      start = midpoint;

      if (numLines > 1 && start < (numLines - 1))
        ++start;
      else
        shifted = 0;
    }
  }

  // Make sure we don't go off the edge in either direction!

  NS_ASSERTION(start >=0 && start <= numLines, "Invalid start calculated.");

  if (start < 0)
    start = 0;
  else if (start >= numLines)
    start = numLines - 1; 

  aClosestLine = start; //close as we could come

  return NS_OK;
}

NS_IMETHODIMP
nsBlockFrame::HandleEvent(nsPresContext* aPresContext, 
                          nsGUIEvent*     aEvent,
                          nsEventStatus*  aEventStatus)
{

  nsresult result;
  nsIPresShell *shell = nsnull;
  if (aEvent->message == NS_MOUSE_MOVE) {
    shell = aPresContext->GetPresShell();
    if (!shell)
      return NS_OK;
    nsCOMPtr<nsIFrameSelection> frameSelection;
    PRBool mouseDown = PR_FALSE;
//check to see if we need to ask the selection controller..
    if (mState & NS_FRAME_INDEPENDENT_SELECTION)
    {
      nsCOMPtr<nsISelectionController> selCon;
      result = GetSelectionController(aPresContext, getter_AddRefs(selCon));
      if (NS_FAILED(result) || !selCon)
        return result?result:NS_ERROR_FAILURE;
      frameSelection = do_QueryInterface(selCon);
    }
    else
      frameSelection = shell->FrameSelection();
    if (!frameSelection || NS_FAILED(frameSelection->GetMouseDownState(&mouseDown)) || !mouseDown) 
      return NS_OK;//do not handle
  }

  if (aEvent->message == NS_MOUSE_LEFT_BUTTON_DOWN || aEvent->message == NS_MOUSE_MOVE ||
    aEvent->message == NS_MOUSE_LEFT_DOUBLECLICK ) {

    nsMouseEvent *me = (nsMouseEvent *)aEvent;

    nsIFrame *resultFrame = nsnull;//this will be passed the handle event when we 
                                   //can tell who to pass it to
    nsIFrame *mainframe = this;
    shell = aPresContext->GetPresShell();
    if (!shell)
      return NS_OK;
    nsCOMPtr<nsILineIterator> it( do_QueryInterface(mainframe, &result) );
    nsIView* parentWithView;
    nsPoint origin;
    nsPeekOffsetStruct pos;

    while(NS_OK == result)
    { //we are starting aloop to allow us to "drill down to the one we want" 
      mainframe->GetOffsetFromView(aPresContext, origin, &parentWithView);

      if (NS_FAILED(result))
        return NS_OK;//do not handle
      PRInt32 closestLine;

      if (NS_FAILED(result = GetClosestLine(it,origin,aEvent->point,closestLine)))
        return result;
      
      
      //we will now ask where to go. if we cant find what we want"aka another block frame" 
      //we drill down again
      pos.mShell = shell;
      pos.mDirection = eDirNext;
      pos.mDesiredX = aEvent->point.x;
      pos.mScrollViewStop = PR_FALSE;
      pos.mIsKeyboardSelect = PR_FALSE;
      result = nsFrame::GetNextPrevLineFromeBlockFrame(aPresContext,
                                          &pos,
                                          mainframe, 
                                          closestLine-1, 
                                          0
                                          );
      
      if (NS_SUCCEEDED(result) && pos.mResultFrame){
        if (result == NS_OK)
          it = do_QueryInterface(pos.mResultFrame, &result);//if this fails thats ok
        resultFrame = pos.mResultFrame;
        mainframe = resultFrame;
      }
      else
        break;//time to go nothing was found
    }
    //end while loop. if nssucceeded resutl then keep going that means
    //we have successfully hit another block frame and we should keep going.


    if (resultFrame)
    {
      if (NS_POSITION_BEFORE_TABLE == result)
      {
        nsCOMPtr<nsISelectionController> selCon;
        result = GetSelectionController(aPresContext, getter_AddRefs(selCon));
        //get the selection controller
        if (NS_SUCCEEDED(result) && selCon) 
        {
          PRInt16 displayresult;
          selCon->GetDisplaySelection(&displayresult);
          if (displayresult == nsISelectionController::SELECTION_OFF)
            return NS_OK;//nothing to do we cannot affect selection from here
        }
        PRBool mouseDown = aEvent->message == NS_MOUSE_MOVE;
        result = shell->FrameSelection()->HandleClick(pos.mResultContent,
                                                      pos.mContentOffset, 
                                                      pos.mContentOffsetEnd,
                                                      mouseDown || me->isShift,
                                                      PR_FALSE,
                                                      pos.mPreferLeft);
      }
      else
        result = resultFrame->HandleEvent(aPresContext, aEvent, aEventStatus);//else let the frame/container do what it needs
      if (aEvent->message == NS_MOUSE_LEFT_BUTTON_DOWN && !IsMouseCaptured(aPresContext))
          CaptureMouse(aPresContext, PR_TRUE);
      return result;
    }
    else
    {
      /*we have to add this because any frame that overrides nsFrame::HandleEvent for mouse down MUST capture the mouse events!!
      if (aEvent->message == NS_MOUSE_LEFT_BUTTON_DOWN && !IsMouseCaptured(aPresContext))
          CaptureMouse(aPresContext, PR_TRUE);*/
      return NS_OK; //just stop it
    }
  }
  return nsFrame::HandleEvent(aPresContext, aEvent, aEventStatus);
}

#ifdef ACCESSIBILITY
NS_IMETHODIMP nsBlockFrame::GetAccessible(nsIAccessible** aAccessible)
{
  *aAccessible = nsnull;
  nsCOMPtr<nsIAccessibilityService> accService = 
    do_GetService("@mozilla.org/accessibilityService;1");
  NS_ENSURE_TRUE(accService, NS_ERROR_FAILURE);

  nsPresContext *aPresContext = GetPresContext();
  if (!mBullet || !aPresContext) {
    return NS_ERROR_FAILURE;
  }

  const nsStyleList* myList = GetStyleList();
  nsAutoString bulletText;
  if (myList->mListStyleImage || myList->mListStyleType == NS_STYLE_LIST_STYLE_DISC ||
      myList->mListStyleType == NS_STYLE_LIST_STYLE_CIRCLE ||
      myList->mListStyleType == NS_STYLE_LIST_STYLE_SQUARE) {
    bulletText.Assign(PRUnichar(0x2022));; // Unicode bullet character
  }
  else if (myList->mListStyleType != NS_STYLE_LIST_STYLE_NONE) {
    mBullet->GetListItemText(aPresContext, *myList, bulletText);
  }

  return accService->CreateHTMLLIAccessible(NS_STATIC_CAST(nsIFrame*, this), 
                                            NS_STATIC_CAST(nsIFrame*, mBullet), 
                                            bulletText,
                                            aAccessible);
}
#endif

void nsBlockFrame::ClearLineCursor() {
  if (!(GetStateBits() & NS_BLOCK_HAS_LINE_CURSOR)) {
    return;
  }

  UnsetProperty(nsLayoutAtoms::lineCursorProperty);
  RemoveStateBits(NS_BLOCK_HAS_LINE_CURSOR);
}

void nsBlockFrame::SetupLineCursor() {
  if (GetStateBits() & NS_BLOCK_HAS_LINE_CURSOR
      || mLines.empty()) {
    return;
  }
   
  SetProperty(nsLayoutAtoms::lineCursorProperty,
              mLines.front(), nsnull);
  AddStateBits(NS_BLOCK_HAS_LINE_CURSOR);
}

nsLineBox* nsBlockFrame::GetFirstLineContaining(nscoord y) {
  if (!(GetStateBits() & NS_BLOCK_HAS_LINE_CURSOR)) {
    return nsnull;
  }

  nsLineBox* property = NS_STATIC_CAST(nsLineBox*,
    GetProperty(nsLayoutAtoms::lineCursorProperty));
  line_iterator cursor = mLines.begin(property);
  nsRect cursorArea = cursor->GetCombinedArea();

  while ((cursorArea.IsEmpty() || cursorArea.YMost() > y)
         && cursor != mLines.front()) {
    cursor = cursor.prev();
    cursorArea = cursor->GetCombinedArea();
  }
  while ((cursorArea.IsEmpty() || cursorArea.YMost() <= y)
         && cursor != mLines.back()) {
    cursor = cursor.next();
    cursorArea = cursor->GetCombinedArea();
  }

  if (cursor.get() != property) {
    SetProperty(nsLayoutAtoms::lineCursorProperty,
                cursor.get(), nsnull);
  }

  return cursor.get();
}

static inline void
GetFrameFromLine(const nsRect& aLineArea, const nsPoint& aTmp,
                 nsBlockFrame::line_iterator& aLine, nsPresContext* aPresContext,
                 nsFramePaintLayer aWhichLayer, nsIFrame** aFrame) {
  if (aLineArea.Contains(aTmp)) {
    nsIFrame* kid = aLine->mFirstChild;
    PRInt32 n = aLine->GetChildCount();
    while (--n >= 0) {
      nsIFrame *hit;
      nsresult rv = kid->GetFrameForPoint(aPresContext, aTmp, aWhichLayer, &hit);
      
      if (NS_SUCCEEDED(rv) && hit) {
        *aFrame = hit;
      }
      kid = kid->GetNextSibling();
    }
  }
}

// Optimized function that uses line combined areas to skip lines
// we know can't contain the point
nsresult
nsBlockFrame::GetFrameForPointUsing(nsPresContext* aPresContext,
                                    const nsPoint& aPoint,
                                    nsIAtom*       aList,
                                    nsFramePaintLayer aWhichLayer,
                                    PRBool         aConsiderSelf,
                                    nsIFrame**     aFrame)
{
  if (aList) {
    return nsContainerFrame::GetFrameForPointUsing(aPresContext,
      aPoint, aList, aWhichLayer, aConsiderSelf, aFrame);
  }

  PRBool inThisFrame = mRect.Contains(aPoint);

  if (! ((mState & NS_FRAME_OUTSIDE_CHILDREN) || inThisFrame ) ) {
    return NS_ERROR_FAILURE;
  }

  *aFrame = nsnull;
  nsPoint tmp(aPoint.x - mRect.x, aPoint.y - mRect.y);

  nsPoint originOffset;
  nsIView *view = nsnull;
  nsresult rv = GetOriginToViewOffset(aPresContext, originOffset, &view);

  if (NS_SUCCEEDED(rv) && view)
    tmp += originOffset;

  nsLineBox* cursor = GetFirstLineContaining(tmp.y);
  line_iterator line_end = end_lines();

  if (cursor) {
    // This is the fast path for large blocks
    for (line_iterator line = mLines.begin(cursor);
         line != line_end;
         ++line) {
      nsRect lineArea = line->GetCombinedArea();
      // Because we have a cursor, the combinedArea.ys are non-decreasing.
      // Once we've passed tmp.y, we can never see it again.
      if (!lineArea.IsEmpty()) {
        if (lineArea.y > tmp.y) {
          break;
        }
        GetFrameFromLine(lineArea, tmp, line, aPresContext, aWhichLayer, aFrame);
      }
    }
  } else {
    PRBool nonDecreasingYs = PR_TRUE;
    PRInt32 lineCount = 0;
    nscoord lastY = PR_INT32_MIN;
    nscoord lastYMost = PR_INT32_MIN;
    for (line_iterator line = mLines.begin();
         line != line_end;
         ++line) {
      nsRect lineArea = line->GetCombinedArea();
      if (!lineArea.IsEmpty()) {
        if (lineArea.y < lastY
            || lineArea.YMost() < lastYMost) {
          nonDecreasingYs = PR_FALSE;
        }
        lastY = lineArea.y;
        lastYMost = lineArea.YMost();

        GetFrameFromLine(lineArea, tmp, line, aPresContext, aWhichLayer, aFrame);
      }
      lineCount++;
    }

    if (nonDecreasingYs && lineCount >= MIN_LINES_NEEDING_CURSOR) {
      SetupLineCursor();
    }
  }
  
  if (*aFrame) {
    return NS_OK;
  }

  if ( inThisFrame && aConsiderSelf ) {
    if (GetStyleVisibility()->IsVisible()) {
      *aFrame = this;
      return NS_OK;
    }
  }

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsBlockFrame::GetFrameForPoint(nsPresContext* aPresContext,
                               const nsPoint& aPoint,
                               nsFramePaintLayer aWhichLayer,
                               nsIFrame** aFrame)
{
  nsresult rv;

  switch (aWhichLayer) {
    case NS_FRAME_PAINT_LAYER_FOREGROUND:
      rv = GetFrameForPointUsing(aPresContext, aPoint, nsnull, NS_FRAME_PAINT_LAYER_FOREGROUND, PR_FALSE, aFrame);
      if (NS_OK == rv) {
        return NS_OK;
      }
      if (nsnull != mBullet) {
        rv = GetFrameForPointUsing(aPresContext, aPoint, nsLayoutAtoms::bulletList, NS_FRAME_PAINT_LAYER_FOREGROUND, PR_FALSE, aFrame);
      }
      return rv;
      break;

    case NS_FRAME_PAINT_LAYER_FLOATS:
      // we painted our floats before our children, and thus
      // we should check floats within children first
      rv = GetFrameForPointUsing(aPresContext, aPoint, nsnull, NS_FRAME_PAINT_LAYER_FLOATS, PR_FALSE, aFrame);
      if (NS_OK == rv) {
        return NS_OK;
      }
      if (mFloats.NotEmpty()) {

        return GetFrameForPointUsing(aPresContext, aPoint,
                                     nsLayoutAtoms::floatList,
                                     NS_FRAME_PAINT_LAYER_ALL,
                                     PR_FALSE, aFrame);
      } else {
        return NS_ERROR_FAILURE;
      }
      break;

    case NS_FRAME_PAINT_LAYER_BACKGROUND:
      // we're a block, so PR_TRUE for consider self
      return GetFrameForPointUsing(aPresContext, aPoint, nsnull,
                                   NS_FRAME_PAINT_LAYER_BACKGROUND,
                                   PR_TRUE, aFrame);
      break;
  }
  // we shouldn't get here
  NS_ASSERTION(PR_FALSE, "aWhichLayer was not understood");
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsBlockFrame::ReflowDirtyChild(nsIPresShell* aPresShell, nsIFrame* aChild)
{
#ifdef DEBUG
  if (gNoisyReflow) {
    IndentBy(stdout, gNoiseIndent);
    ListTag(stdout);
    printf(": ReflowDirtyChild (");
    if (aChild)
      nsFrame::ListTag(stdout, aChild);
    else
      printf("null");
    printf(")\n");

    gNoiseIndent++;
  }
#endif

  if (aChild) {
    // See if the child is absolutely positioned
    if (aChild->GetStateBits() & NS_FRAME_OUT_OF_FLOW) {
      const nsStyleDisplay* disp = aChild->GetStyleDisplay();

      if (disp->IsAbsolutelyPositioned()) {
        // Generate a reflow command to reflow our dirty absolutely
        // positioned child frames.
        // XXX Note that we don't currently try and coalesce the reflow commands,
        // although we should. We can't use the NS_FRAME_HAS_DIRTY_CHILDREN
        // flag, because that's used to indicate whether in-flow children are
        // dirty...
        nsHTMLReflowCommand* reflowCmd;
        nsresult          rv = NS_NewHTMLReflowCommand(&reflowCmd, this,
                                                       eReflowType_ReflowDirty);
        if (NS_SUCCEEDED(rv)) {
          reflowCmd->SetChildListName(mAbsoluteContainer.GetChildListName());
          aPresShell->AppendReflowCommand(reflowCmd);
        }

#ifdef DEBUG
        if (gNoisyReflow) {
          IndentBy(stdout, gNoiseIndent);
          printf("scheduled reflow command for absolutely positioned frame\n");
          --gNoiseIndent;
        }
#endif

        return rv;
      }
    }

    if (aChild == mBullet && HaveOutsideBullet()) {
      // The bullet lives in the first line, unless the first line has
      // height 0 and there is a second line, in which case it lives
      // in the second line.
      line_iterator bulletLine = begin_lines();
      if (bulletLine != end_lines() && bulletLine->mBounds.height == 0 &&
          bulletLine != mLines.back()) {
        bulletLine = bulletLine.next();
      }
      
      if (bulletLine != end_lines()) {
        MarkLineDirty(bulletLine);
      }
      // otherwise we have an empty line list, and ReflowDirtyLines
      // will handle reflowing the bullet.
    } else {
      // Mark the line containing the child frame dirty.
      line_iterator fline = FindLineFor(aChild);
      if (fline != end_lines())
        MarkLineDirty(fline);
    }
  }

  // Either generate a reflow command to reflow the dirty child or 
  // coalesce this reflow request with an existing reflow command    
  if (!(mState & NS_FRAME_HAS_DIRTY_CHILDREN)) {
    // If this is the first dirty child, 
    // post a dirty children reflow command targeted at yourself
    mState |= NS_FRAME_HAS_DIRTY_CHILDREN;

    nsFrame::CreateAndPostReflowCommand(aPresShell, this, 
      eReflowType_ReflowDirty, nsnull, nsnull, nsnull);

#ifdef DEBUG
    if (gNoisyReflow) {
      IndentBy(stdout, gNoiseIndent);
      printf("scheduled reflow command targeted at self\n");
    }
#endif
  }

#ifdef DEBUG
  if (gNoisyReflow) {
    --gNoiseIndent;
  }
#endif
  
  return NS_OK;
}

//////////////////////////////////////////////////////////////////////
// Start Debugging

#ifdef NS_DEBUG
static PRBool
InLineList(nsLineList& aLines, nsIFrame* aFrame)
{
  for (nsLineList::iterator line = aLines.begin(), line_end = aLines.end();
       line != line_end;
       ++line) {
    nsIFrame* frame = line->mFirstChild;
    PRInt32 n = line->GetChildCount();
    while (--n >= 0) {
      if (frame == aFrame) {
        return PR_TRUE;
      }
      frame = frame->GetNextSibling();
    }
  }
  return PR_FALSE;
}

static PRBool
InSiblingList(nsLineList& aLines, nsIFrame* aFrame)
{
  if (! aLines.empty()) {
    nsIFrame* frame = aLines.front()->mFirstChild;
    while (frame) {
      if (frame == aFrame) {
        return PR_TRUE;
      }
      frame = frame->GetNextSibling();
    }
  }
  return PR_FALSE;
}

PRBool
nsBlockFrame::IsChild(nsIFrame* aFrame)
{
  // Continued out-of-flows don't satisfy InLineList(), continued out-of-flows
  // and placeholders don't satisfy InSiblingList().  
  PRBool skipLineList    = PR_FALSE;
  PRBool skipSiblingList = PR_FALSE;
  nsIFrame* prevInFlow = aFrame->GetPrevInFlow();
  if (prevInFlow) {
    nsFrameState state = aFrame->GetStateBits();
    skipLineList    = (state & NS_FRAME_OUT_OF_FLOW); 
    skipSiblingList = (nsLayoutAtoms::placeholderFrame == aFrame->GetType()) ||
                      (state & NS_FRAME_OUT_OF_FLOW);
  }

  if (aFrame->GetParent() != (nsIFrame*)this) {
    return PR_FALSE;
  }
  if ((skipLineList || InLineList(mLines, aFrame)) && 
      (skipSiblingList || InSiblingList(mLines, aFrame))) {
    return PR_TRUE;
  }
  nsLineList* overflowLines = GetOverflowLines();
  if (overflowLines && (skipLineList || InLineList(*overflowLines, aFrame)) && 
      (skipSiblingList || InSiblingList(*overflowLines, aFrame))) {
    return PR_TRUE;
  }
  return PR_FALSE;
}

NS_IMETHODIMP
nsBlockFrame::VerifyTree() const
{
  // XXX rewrite this
  return NS_OK;
}
#endif

// End Debugging
//////////////////////////////////////////////////////////////////////

NS_IMETHODIMP
nsBlockFrame::Init(nsPresContext*  aPresContext,
                   nsIContent*      aContent,
                   nsIFrame*        aParent,
                   nsStyleContext*  aContext,
                   nsIFrame*        aPrevInFlow)
{
  if (aPrevInFlow) {
    // Copy over the block/area frame type flags
    nsBlockFrame*  blockFrame = (nsBlockFrame*)aPrevInFlow;

    SetFlags(blockFrame->mState & NS_BLOCK_FLAGS_MASK);
  }

  nsresult rv = nsBlockFrameSuper::Init(aPresContext, aContent, aParent,
                                        aContext, aPrevInFlow);

  if (IsBoxWrapped())
    mState |= NS_BLOCK_SPACE_MGR;

  return rv;
}

NS_IMETHODIMP
nsBlockFrame::SetInitialChildList(nsPresContext* aPresContext,
                                  nsIAtom*        aListName,
                                  nsIFrame*       aChildList)
{
  nsresult rv = NS_OK;

  if (mAbsoluteContainer.GetChildListName() == aListName) {
    mAbsoluteContainer.SetInitialChildList(this, aPresContext, aListName, aChildList);
  }
  else if (nsLayoutAtoms::floatList == aListName) {
    mFloats.SetFrames(aChildList);
  }
  else {

    // Lookup up the two pseudo style contexts
    if (nsnull == mPrevInFlow) {
      nsRefPtr<nsStyleContext> firstLetterStyle = GetFirstLetterStyle(aPresContext);
      if (nsnull != firstLetterStyle) {
        mState |= NS_BLOCK_HAS_FIRST_LETTER_STYLE;
#ifdef NOISY_FIRST_LETTER
        ListTag(stdout);
        printf(": first-letter style found\n");
#endif
      }
    }

    rv = AddFrames(aPresContext, aChildList, nsnull);
    if (NS_FAILED(rv)) {
      return rv;
    }

    // Create list bullet if this is a list-item. Note that this is done
    // here so that RenumberLists will work (it needs the bullets to
    // store the bullet numbers).
    const nsStyleDisplay* styleDisplay = GetStyleDisplay();
    if ((nsnull == mPrevInFlow) &&
        (NS_STYLE_DISPLAY_LIST_ITEM == styleDisplay->mDisplay) &&
        (nsnull == mBullet)) {
      // Resolve style for the bullet frame
      const nsStyleList* styleList = GetStyleList();
      nsIAtom *pseudoElement;
      switch (styleList->mListStyleType) {
        case NS_STYLE_LIST_STYLE_DISC:
        case NS_STYLE_LIST_STYLE_CIRCLE:
        case NS_STYLE_LIST_STYLE_SQUARE:
          pseudoElement = nsCSSPseudoElements::mozListBullet;
          break;
        default:
          pseudoElement = nsCSSPseudoElements::mozListNumber;
          break;
      }

      nsIPresShell *shell = aPresContext->PresShell();

      nsRefPtr<nsStyleContext> kidSC = shell->StyleSet()->
        ResolvePseudoStyleFor(mContent, pseudoElement, mStyleContext);

      // Create bullet frame
      mBullet = new (shell) nsBulletFrame;

      if (nsnull == mBullet) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
      mBullet->Init(aPresContext, mContent, this, kidSC, nsnull);

      // If the list bullet frame should be positioned inside then add
      // it to the flow now.
      if (NS_STYLE_LIST_STYLE_POSITION_INSIDE ==
          styleList->mListStylePosition) {
        AddFrames(aPresContext, mBullet, nsnull);
        mState &= ~NS_BLOCK_FRAME_HAS_OUTSIDE_BULLET;
      }
      else {
        mState |= NS_BLOCK_FRAME_HAS_OUTSIDE_BULLET;
      }
    }
  }

  return NS_OK;
}

PRBool
nsBlockFrame::FrameStartsCounterScope(nsIFrame* aFrame)
{
  const nsStyleContent* styleContent = aFrame->GetStyleContent();
  if (0 != styleContent->CounterResetCount()) {
    // Winner
    return PR_TRUE;
  }
  return PR_FALSE;
}

void
nsBlockFrame::RenumberLists(nsPresContext* aPresContext)
{
  if (!FrameStartsCounterScope(this)) {
    // If this frame doesn't start a counter scope then we don't need
    // to renumber child list items.
    return;
  }

  // Setup initial list ordinal value
  // XXX Map html's start property to counter-reset style
  PRInt32 ordinal = 1;

  nsCOMPtr<nsIHTMLContent> hc(do_QueryInterface(mContent));

  if (hc) {
    nsHTMLValue value;
    if (NS_CONTENT_ATTR_HAS_VALUE ==
        hc->GetHTMLAttribute(nsHTMLAtoms::start, value)) {
      if (eHTMLUnit_Integer == value.GetUnit()) {
        ordinal = value.GetIntValue();
      }
    }
  }

  // Get to first-in-flow
  nsBlockFrame* block = (nsBlockFrame*) GetFirstInFlow();
  RenumberListsInBlock(aPresContext, block, &ordinal, 0);
}

PRBool
nsBlockFrame::RenumberListsInBlock(nsPresContext* aPresContext,
                                   nsBlockFrame* aBlockFrame,
                                   PRInt32* aOrdinal,
                                   PRInt32 aDepth)
{
  PRBool renumberedABullet = PR_FALSE;

  while (nsnull != aBlockFrame) {
    // Examine each line in the block
    for (line_iterator line = aBlockFrame->begin_lines(),
                       line_end = aBlockFrame->end_lines();
         line != line_end;
         ++line) {
      nsIFrame* kid = line->mFirstChild;
      PRInt32 n = line->GetChildCount();
      while (--n >= 0) {
        PRBool kidRenumberedABullet = RenumberListsFor(aPresContext, kid, aOrdinal, aDepth);
        if (kidRenumberedABullet) {
          line->MarkDirty();
          renumberedABullet = PR_TRUE;
        }
        kid = kid->GetNextSibling();
      }
    }

    // Advance to the next continuation
    aBlockFrame = NS_STATIC_CAST(nsBlockFrame*, aBlockFrame->GetNextInFlow());
  }

  return renumberedABullet;
}

PRBool
nsBlockFrame::RenumberListsFor(nsPresContext* aPresContext,
                               nsIFrame* aKid,
                               PRInt32* aOrdinal,
                               PRInt32 aDepth)
{
  NS_PRECONDITION(aPresContext && aKid && aOrdinal, "null params are immoral!");

  // add in a sanity check for absurdly deep frame trees.  See bug 42138
  if (MAX_DEPTH_FOR_LIST_RENUMBERING < aDepth)
    return PR_FALSE;

  PRBool kidRenumberedABullet = PR_FALSE;
  nsIFrame* kid = aKid;

  // if the frame is a placeholder, then get the out of flow frame
  if (nsLayoutAtoms::placeholderFrame == aKid->GetType()) {
    kid = NS_STATIC_CAST(nsPlaceholderFrame*, aKid)->GetOutOfFlowFrame();
    NS_ASSERTION(kid, "no out-of-flow frame");
  }

  // If the frame is a list-item and the frame implements our
  // block frame API then get its bullet and set the list item
  // ordinal.
  const nsStyleDisplay* display = kid->GetStyleDisplay();
  if (NS_STYLE_DISPLAY_LIST_ITEM == display->mDisplay) {
    // Make certain that the frame is a block frame in case
    // something foreign has crept in.
    nsBlockFrame* listItem;
    nsresult rv = kid->QueryInterface(kBlockFrameCID, (void**)&listItem);
    if (NS_SUCCEEDED(rv)) {
      if (nsnull != listItem->mBullet) {
        PRBool changed;
        *aOrdinal = listItem->mBullet->SetListItemOrdinal(*aOrdinal,
                                                          &changed);
        if (changed) {
          kidRenumberedABullet = PR_TRUE;

          // Invalidate the bullet content area since it may look different now
          nsRect damageRect(nsPoint(0, 0), listItem->mBullet->GetSize());
          listItem->mBullet->Invalidate(damageRect);
        }
      }

      // XXX temporary? if the list-item has child list-items they
      // should be numbered too; especially since the list-item is
      // itself (ASSUMED!) not to be a counter-resetter.
      PRBool meToo = RenumberListsInBlock(aPresContext, listItem, aOrdinal, aDepth + 1);
      if (meToo) {
        kidRenumberedABullet = PR_TRUE;
      }
    }
  }
  else if (NS_STYLE_DISPLAY_BLOCK == display->mDisplay) {
    if (FrameStartsCounterScope(kid)) {
      // Don't bother recursing into a block frame that is a new
      // counter scope. Any list-items in there will be handled by
      // it.
    }
    else {
      // If the display=block element is a block frame then go ahead
      // and recurse into it, as it might have child list-items.
      nsBlockFrame* kidBlock;
      nsresult rv = kid->QueryInterface(kBlockFrameCID, (void**) &kidBlock);
      if (NS_SUCCEEDED(rv)) {
        kidRenumberedABullet = RenumberListsInBlock(aPresContext, kidBlock, aOrdinal, aDepth + 1);
      }
    }
  }
  return kidRenumberedABullet;
}

void
nsBlockFrame::ReflowBullet(nsBlockReflowState& aState,
                           nsHTMLReflowMetrics& aMetrics)
{
  // Reflow the bullet now
  nsSize availSize;
  availSize.width = NS_UNCONSTRAINEDSIZE;
  availSize.height = NS_UNCONSTRAINEDSIZE;

  // Get the reason right.
  // XXXwaterson Should this look just like the logic in
  // nsBlockReflowContext::ReflowBlock and nsLineLayout::ReflowFrame?
  const nsHTMLReflowState &rs = aState.mReflowState;
  nsReflowReason reason = rs.reason;
  if (reason == eReflowReason_Incremental) {
    if (! rs.path->HasChild(mBullet)) {
      // An incremental reflow not explicitly destined to (or through)
      // the child should be treated as a resize...
      reason = eReflowReason_Resize;
    }

    // ...unless its an incremental `style changed' reflow targeted at
    // the block, in which case, we propagate that to its children.
    nsHTMLReflowCommand *command = rs.path->mReflowCommand;
    if (command) {
      nsReflowType type;
      command->GetType(type);
      if (type == eReflowType_StyleChanged)
        reason = eReflowReason_StyleChange;
    }
  }

  nsHTMLReflowState reflowState(aState.mPresContext, rs,
                                mBullet, availSize, reason);
  nsReflowStatus  status;
  mBullet->WillReflow(aState.mPresContext);
  mBullet->Reflow(aState.mPresContext, aMetrics, reflowState, status);

  // Place the bullet now; use its right margin to distance it
  // from the rest of the frames in the line
  nscoord x = 
#ifdef IBMBIDI
           (rs.availableWidth != NS_UNCONSTRAINEDSIZE &&
            NS_STYLE_DIRECTION_RTL == GetStyleVisibility()->mDirection)
             // According to the CSS2 spec, section 12.6.1, outside marker box
             // is distanced from the associated principal box's border edge.
             // |rs.availableWidth| reflects exactly a border edge: it includes
             // border, padding, and content area, without margins.
             ? rs.availableWidth + reflowState.mComputedMargin.left :
#endif
             - reflowState.mComputedMargin.right - aMetrics.width;

  // Approximate the bullets position; vertical alignment will provide
  // the final vertical location.
  const nsMargin& bp = aState.BorderPadding();
  nscoord y = bp.top;
  mBullet->SetRect(nsRect(x, y, aMetrics.width, aMetrics.height));
  mBullet->DidReflow(aState.mPresContext, &aState.mReflowState, NS_FRAME_REFLOW_FINISHED);
}

// This is used to scan overflow frames for any float placeholders,
// and add their floats to the list represented by aHead and aTail. We
// only search the inline descendants.
static void CollectFloats(nsIFrame* aFrame, nsIFrame* aBlockParent,
                          nsIFrame** aHead, nsIFrame** aTail) {
  while (aFrame) {
    // Don't descend into block children
    if (!aFrame->GetStyleDisplay()->IsBlockLevel()) {
      nsIFrame *outOfFlowFrame = nsLayoutUtils::GetFloatFromPlaceholder(aFrame);
      if (outOfFlowFrame) {
        // Make sure that its parent is the block we care
        // about. Otherwise we don't want to mess around with it because
        // it belongs to someone else. I think this could happen if the
        // overflow lines contain a block descendant which owns its own
        // floats.
        NS_ASSERTION(outOfFlowFrame->GetParent() == aBlockParent,
                     "Out of flow frame doesn't have the expected parent");
        if (!*aHead) {
          *aHead = *aTail = outOfFlowFrame;
        } else {
          (*aTail)->SetNextSibling(outOfFlowFrame);
          *aTail = outOfFlowFrame;
        }
      }

      CollectFloats(aFrame->GetFirstChild(nsnull), aBlockParent,
                    aHead, aTail);
    }
    
    aFrame = aFrame->GetNextSibling();
  }
}

//XXX get rid of this -- its slow
void
nsBlockFrame::BuildFloatList(nsBlockReflowState& aState)
{
  // Accumulate float list into mFloats.
  // Use the float cache to speed up searching the lines for floats.
  nsIFrame* head = nsnull;
  nsIFrame* current = nsnull;
  for (line_iterator line = begin_lines(), line_end = end_lines();
       line != line_end;
       ++line) {
    if (line->HasFloats()) {
      nsFloatCache* fc = line->GetFirstFloat();
      while (fc) {
        nsIFrame* floatFrame = fc->mPlaceholder->GetOutOfFlowFrame();
        if (!head) {
          current = head = floatFrame;
        } else {
          current->SetNextSibling(floatFrame);
          current = floatFrame;
        }
        fc = fc->Next();
      }
    }
  }

  // Terminate end of float list just in case a float was removed
  if (current) {
    current->SetNextSibling(nsnull);
  }
  mFloats.SetFrames(head);

  // ensure that the floats in the overflow lines are put on a child list
  // and not dropped from the frame tree!
  // Note that overflow lines do not have any float cache set up for them,
  // because the float cache contains only laid-out floats
  nsLineList* overflowLines = GetOverflowLines();
  if (overflowLines) {
    head = nsnull;
    current = nsnull;

    CollectFloats(overflowLines->front()->mFirstChild,
                  this, &head, &current);

    if (current) {
      current->SetNextSibling(nsnull);

      // Floats that were pushed should be removed from our space
      // manager.  Otherwise the space manager's YMost or XMost might
      // be larger than necessary, causing this block to get an
      // incorrect desired height (or width).  Some of these floats
      // may not actually have been added to the space manager because
      // they weren't reflowed before being pushed; that's OK,
      // RemoveRegions will ignore them. It is safe to do this here
      // because we know from here on the space manager will only be
      // used for its XMost and YMost, not to place new floats and
      // lines.
      aState.mSpaceManager->RemoveTrailingRegions(head);

      nsFrameList* frameList = new nsFrameList(head);
      if (frameList) {
        SetOverflowOutOfFlows(frameList);
      }
    }
  }
}

NS_IMETHODIMP
nsBlockFrame::SetParent(const nsIFrame* aParent)
{
  nsresult rv = nsBlockFrameSuper::SetParent(aParent);
  if (IsBoxWrapped())
    mState |= NS_BLOCK_SPACE_MGR;

  // XXX should we clear NS_BLOCK_SPACE_MGR if we were the child of a box
  // but no longer are?

  return rv;
}

// XXX keep the text-run data in the first-in-flow of the block

#ifdef DEBUG
void
nsBlockFrame::VerifyLines(PRBool aFinalCheckOK)
{
  if (!gVerifyLines) {
    return;
  }
  if (mLines.empty()) {
    return;
  }

  // Add up the counts on each line. Also validate that IsFirstLine is
  // set properly.
  PRInt32 count = 0;
  PRBool seenBlock = PR_FALSE;
  line_iterator line, line_end;
  for (line = begin_lines(), line_end = end_lines();
       line != line_end;
       ++line) {
    if (aFinalCheckOK) {
      NS_ABORT_IF_FALSE(line->GetChildCount(), "empty line");
      if (line->IsBlock()) {
        seenBlock = PR_TRUE;
      }
      if (line->IsBlock()) {
        NS_ASSERTION(1 == line->GetChildCount(), "bad first line");
      }
    }
    count += line->GetChildCount();
  }

  // Then count the frames
  PRInt32 frameCount = 0;
  nsIFrame* frame = mLines.front()->mFirstChild;
  while (frame) {
    frameCount++;
    frame = frame->GetNextSibling();
  }
  NS_ASSERTION(count == frameCount, "bad line list");

  // Next: test that each line has right number of frames on it
  for (line = begin_lines(), line_end = end_lines();
       line != line_end;
        ) {
    count = line->GetChildCount();
    frame = line->mFirstChild;
    while (--count >= 0) {
      frame = frame->GetNextSibling();
    }
    ++line;
    if ((line != line_end) && (0 != line->GetChildCount())) {
      NS_ASSERTION(frame == line->mFirstChild, "bad line list");
    }
  }
}

// Its possible that a frame can have some frames on an overflow
// list. But its never possible for multiple frames to have overflow
// lists. Check that this fact is actually true.
void
nsBlockFrame::VerifyOverflowSituation()
{
  nsBlockFrame* flow = (nsBlockFrame*) GetFirstInFlow();
  while (nsnull != flow) {
    nsLineList* overflowLines = GetOverflowLines();
    if (nsnull != overflowLines) {
      NS_ASSERTION(! overflowLines->empty(), "should not be empty if present");
      NS_ASSERTION(overflowLines->front()->mFirstChild, "bad overflow list");
    }
    flow = (nsBlockFrame*) flow->mNextInFlow;
  }
}

PRInt32
nsBlockFrame::GetDepth() const
{
  PRInt32 depth = 0;
  nsIFrame* parent = mParent;
  while (parent) {
    parent = parent->GetParent();
    depth++;
  }
  return depth;
}
#endif
