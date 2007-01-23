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
 * The Original Code is Mozilla MathML Project.
 *
 * The Initial Developer of the Original Code is
 * The University Of Queensland.
 * Portions created by the Initial Developer are Copyright (C) 1999
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Roger B. Sidje <rbs@maths.uq.edu.au>
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
#include "nsFrame.h"
#include "nsPresContext.h"
#include "nsUnitConversion.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIRenderingContext.h"
#include "nsIFontMetrics.h"

#include "nsMathMLmspaceFrame.h"


//
// <mspace> -- space - implementation
//

nsIFrame*
NS_NewMathMLmspaceFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMathMLmspaceFrame(aContext);
}

nsMathMLmspaceFrame::~nsMathMLmspaceFrame()
{
}

PRBool
nsMathMLmspaceFrame::IsLeaf() const
{
  return PR_TRUE;
}

void
nsMathMLmspaceFrame::ProcessAttributes(nsPresContext* aPresContext)
{
  /*
  parse the attributes

  width  = number h-unit 
  height = number v-unit 
  depth  = number v-unit 
  */

  nsAutoString value;
  nsCSSValue cssValue;

  // width 
  mWidth = 0;
  GetAttribute(mContent, mPresentationData.mstyle, nsGkAtoms::width,
               value);
  if (!value.IsEmpty()) {
    if ((ParseNumericValue(value, cssValue) ||
         ParseNamedSpaceValue(mPresentationData.mstyle, value, cssValue)) &&
         cssValue.IsLengthUnit()) {
      mWidth = CalcLength(aPresContext, mStyleContext, cssValue);
    }
  }

  // height
  mHeight = 0;
  GetAttribute(mContent, mPresentationData.mstyle, nsGkAtoms::height,
               value);
  if (!value.IsEmpty()) {
    if ((ParseNumericValue(value, cssValue) ||
         ParseNamedSpaceValue(mPresentationData.mstyle, value, cssValue)) &&
         cssValue.IsLengthUnit()) {
      mHeight = CalcLength(aPresContext, mStyleContext, cssValue);
    }
  }

  // depth
  mDepth = 0;
  GetAttribute(mContent, mPresentationData.mstyle, nsGkAtoms::depth_,
               value);
  if (!value.IsEmpty()) {
    if ((ParseNumericValue(value, cssValue) ||
         ParseNamedSpaceValue(mPresentationData.mstyle, value, cssValue)) &&
         cssValue.IsLengthUnit()) {
      mDepth = CalcLength(aPresContext, mStyleContext, cssValue);
    }
  }
}

NS_IMETHODIMP
nsMathMLmspaceFrame::Reflow(nsPresContext*          aPresContext,
                            nsHTMLReflowMetrics&     aDesiredSize,
                            const nsHTMLReflowState& aReflowState,
                            nsReflowStatus&          aStatus)
{
  ProcessAttributes(aPresContext);

  mBoundingMetrics.Clear();
  mBoundingMetrics.width = mWidth;
  mBoundingMetrics.ascent = mHeight;
  mBoundingMetrics.descent = mDepth;
  mBoundingMetrics.leftBearing = 0;
  mBoundingMetrics.rightBearing = mWidth;

  aDesiredSize.ascent = mHeight;
  aDesiredSize.width = mWidth;
  aDesiredSize.height = aDesiredSize.ascent + mDepth;
  // Also return our bounding metrics
  aDesiredSize.mBoundingMetrics = mBoundingMetrics;

  aStatus = NS_FRAME_COMPLETE;
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
  return NS_OK;
}
