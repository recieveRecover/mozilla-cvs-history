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
#ifndef nsTableOuterFrame_h__
#define nsTableOuterFrame_h__

#include "nscore.h"
#include "nsContainerFrame.h"

class nsTableFrame;
class nsVoidArray;
struct nsStyleMolecule;
class nsTableCaptionFrame;
struct OuterTableReflowState;

/**
 * main frame for an nsTable content object, 
 * the nsTableOuterFrame contains 0 or more nsTableCaptionFrames, 
 * and a single nsTableFrame psuedo-frame, often referred to as the "inner frame'.
 * <P> Unlike other frames that handle continuing across breaks, nsTableOuterFrame
 * has no notion of "unmapped" children.  All children (captions and inner table)
 * have frames created in Pass 1, so from the layout process' point of view, they
 * are always mapped
 *
 * @author  sclark
 */
class nsTableOuterFrame : public nsContainerFrame
{
public:

  /** instantiate a new instance of nsTableFrame.
    * @param aInstancePtrResult  the new object is returned in this out-param
    * @param aContent            the table object to map
    * @param aIndexInParent      which child is the new frame?
    * @param aParent             the parent of the new frame
    *
    * @return  NS_OK if the frame was properly allocated, otherwise an error code
    */
  static nsresult NewFrame(nsIFrame** aInstancePtrResult,
                           nsIContent* aContent,
                           PRInt32     aIndexInParent,
                           nsIFrame*   aParent);

  /** @see nsIFrame::Paint */
  virtual void  Paint(nsIPresContext& aPresContext,
                      nsIRenderingContext& aRenderingContext,
                      const nsRect& aDirtyRect);

  /** outer tables are reflowed in two steps.
    * Step 1:, we lay out all of the captions and the inner table with
    * height and width set to NS_UNCONSTRAINEDSIZE.
    * This gives us absolute minimum and maximum widths for each component.
    * In the second step, we set all the captions and the inner table to 
    * the width of the widest component, given the table's style, width constraints
    * and compatibility mode.<br>
    * Step 2: With the widths now known, we reflow the captions and table.<br>
    * NOTE: for breaking across pages, this method has to account for table content 
    *       that is not laid out linearly vis a vis the frames.  
    *       That is, content hierarchy and the frame hierarchy do not match.
    *
    * @see NeedsReflow
    * @see ResizeReflowCaptionsPass1
    * @see ResizeReflowTopCaptionsPass2
    * @see ResizeReflowBottomCaptionsPass2
    * @see PlaceChild
    * @see ReflowMappedChildren
    * @see PullUpChildren
    * @see ReflowChild
    * @see nsTableFrame::ResizeReflowPass1
    * @see nsTableFrame::ResizeReflowPass2
    * @see nsTableFrame::BalanceColumnWidths
    * @see nsIFrame::ResizeReflow 
    */
  ReflowStatus  ResizeReflow(nsIPresContext*  aPresContext,
                             nsReflowMetrics& aDesiredSize,
                             const nsSize&    aMaxSize,
                             nsSize*          aMaxElementSize);

  /** @see nsIFrame::IncrementalReflow */
  ReflowStatus  IncrementalReflow(nsIPresContext*  aPresContext,
                                  nsReflowMetrics& aDesiredSize,
                                  const nsSize&    aMaxSize,
                                  nsReflowCommand& aReflowCommand);

  /** @see nsContainerFrame */
  virtual nsIFrame* CreateContinuingFrame(nsIPresContext* aPresContext,
                                          nsIFrame*       aParent);
  /** destructor */
  virtual ~nsTableOuterFrame();

protected:

  /** protected constructor 
    * @see NewFrame
    */
  nsTableOuterFrame(nsIContent* aContent,
                   PRInt32 aIndexInParent,
					         nsIFrame* aParentFrame);

  /** return PR_TRUE if the table needs to be reflowed.  
    * the outer table needs to be reflowed if the table content has changed,
    * or if the table style attributes or parent max height/width have
    * changed.
    */
  virtual PRBool NeedsReflow(const nsSize& aMaxSize);

  /** returns PR_TRUE if the data obtained from the first reflow pass
    * is cached and still valid (ie, no content or style change notifications.)
    */
  virtual PRBool IsFirstPassValid();

  /** setter for mFirstPassValid. 
    * should be called with PR_FALSE when:
    *   content changes, style changes, or context changes
    */
  virtual void SetFirstPassValid(PRBool aValidState);

  /** create all child frames for this table */
  virtual void CreateChildFrames(nsIPresContext*  aPresContext);

  /** reflow the captions in an infinite space, caching the min/max sizes for each
    */
  virtual ReflowStatus ResizeReflowCaptionsPass1(nsIPresContext*  aPresContext, 
                                                 nsStyleMolecule* aTableStyle);

  /** reflow the top captions in a space constrained by the computed table width
    * and the heigth given to us by our parent.  Top captions are laid down
    * before the inner table.
    */
  virtual ReflowStatus ResizeReflowTopCaptionsPass2(nsIPresContext*  aPresContext,
                                                    nsReflowMetrics& aDesiredSize,
                                                    const nsSize&    aMaxSize,
                                                    nsSize*          aMaxElementSize,
                                                    nsStyleMolecule* aTableStyle);

  /** reflow the bottom captions in a space constrained by the computed table width
    * and the heigth given to us by our parent.  Bottom captions are laid down
    * after the inner table.
    */
  virtual ReflowStatus ResizeReflowBottomCaptionsPass2(nsIPresContext*  aPresContext,
                                                       nsReflowMetrics& aDesiredSize,
                                                       const nsSize&    aMaxSize,
                                                       nsSize*          aMaxElementSize,
                                                       nsStyleMolecule* aTableStyle,
                                                       nscoord          aYOffset);

  nscoord       GetTopMarginFor(nsIPresContext* aCX,
                                OuterTableReflowState& aState,
                                nsStyleMolecule* aKidMol);

  void          PlaceChild( nsIPresContext*    aPresContext,
                            OuterTableReflowState& aState,
                            nsIFrame*          aKidFrame,
                            const nsRect&      aKidRect,
                            nsStyleMolecule*   aKidMol,
                            nsSize*            aMaxElementSize,
                            nsSize&            aKidMaxElementSize);

  /**
   * Reflow the frames we've already created
   *
   * @param   aPresContext presentation context to use
   * @param   aState current inline state
   * @return  true if we successfully reflowed all the mapped children and false
   *            otherwise, e.g. we pushed children to the next in flow
   */
  PRBool        ReflowMappedChildren(nsIPresContext*        aPresContext,
                                     OuterTableReflowState& aState,
                                     nsSize*                aMaxElementSize);

  /**
   * Try and pull-up frames from our next-in-flow
   *
   * @param   aPresContext presentation context to use
   * @param   aState current inline state
   * @return  true if we successfully pulled-up all the children and false
   *            otherwise, e.g. child didn't fit
   */
  PRBool        PullUpChildren(nsIPresContext*        aPresContext,
                               OuterTableReflowState& aState,
                               nsSize*                aMaxElementSize);

  virtual       void SetReflowState(OuterTableReflowState& aState, 
                                    nsIFrame*              aKidFrame);

  virtual nsIFrame::ReflowStatus ReflowChild( nsIFrame*        aKidFrame,
                                              nsIPresContext*  aPresContext,
                                              nsReflowMetrics& aDesiredSize,
                                              const nsSize&    aMaxSize,
                                              nsSize*          aMaxElementSize,
                                              OuterTableReflowState& aState);

  /** overridden here to handle special caption-table relationship
    * @see nsContainerFrame::VerifyTree
    */
  virtual void VerifyTree() const;

  /** overridden here to handle special caption-table relationship
    * @see nsContainerFrame::PrepareContinuingFrame
    */
  virtual void PrepareContinuingFrame(nsIPresContext*    aPresContext,
                                      nsIFrame*          aParent,
                                      nsTableOuterFrame* aContFrame);

  /** create the inner table frame (nsTableFrame)
    * handles initial creation as well as creation of continuing frames
    */
  virtual void CreateInnerTableFrame(nsIPresContext* aPresContext);


private:
  /** used to keep track of this frame's children */
  nsTableFrame *mInnerTableFrame;
  nsVoidArray * mCaptionFrames;
  nsVoidArray * mBottomCaptions;

  /** used to keep track of min/max caption requirements */
  PRInt32 mMinCaptionWidth;
  PRInt32 mMaxCaptionWidth;

  /** used to cache reflow results so we can optimize out reflow in some circumstances */
  nsReflowMetrics mDesiredSize;
  nsSize mMaxElementSize;

  /** we can skip the first pass on captions if mFirstPassValid is true */
  PRBool mFirstPassValid;

};



#endif
