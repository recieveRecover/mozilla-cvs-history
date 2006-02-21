/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * The Original Code is the Mozilla SVG project.
 *
 * The Initial Developer of the Original Code is
 * Crocodile Clips Ltd..
 * Portions created by the Initial Developer are Copyright (C) 2001
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Alex Fritze <alex.fritze@crocodile-clips.com> (original author)
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

#ifndef NSSVGFOREIGNOBJECTFRAME_H__
#define NSSVGFOREIGNOBJECTFRAME_H__

#include "nsBlockFrame.h"
#include "nsISVGChildFrame.h"
#include "nsISVGContainerFrame.h"
#include "nsISVGValueObserver.h"
#include "nsWeakReference.h"
#include "nsISVGRendererRegion.h"
#include "nsIDOMSVGMatrix.h"
#include "nsIDOMSVGLength.h"

typedef nsBlockFrame nsSVGForeignObjectFrameBase;

class nsISVGFilterFrame;

class nsSVGForeignObjectFrame : public nsSVGForeignObjectFrameBase,
                                public nsISVGContainerFrame,
                                public nsISVGChildFrame,
                                public nsISVGValueObserver,
                                public nsSupportsWeakReference
{
  friend nsIFrame*
  NS_NewSVGForeignObjectFrame(nsIPresShell* aPresShell, nsIContent* aContent);
protected:
  nsSVGForeignObjectFrame();
  virtual ~nsSVGForeignObjectFrame();
  nsresult Init();
  
  // nsISupports interface:
  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);
private:
  NS_IMETHOD_(nsrefcnt) AddRef() { return NS_OK; }
  NS_IMETHOD_(nsrefcnt) Release() { return NS_OK; }  
public:
  // nsIFrame:  
  NS_IMETHOD Init(nsPresContext*  aPresContext,
                  nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsStyleContext*  aContext,
                  nsIFrame*        aPrevInFlow);

  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  NS_IMETHOD  AppendFrames(nsIAtom*        aListName,
                           nsIFrame*       aFrameList);
  
  NS_IMETHOD  InsertFrames(nsIAtom*        aListName,
                           nsIFrame*       aPrevFrame,
                           nsIFrame*       aFrameList);
  
  NS_IMETHOD  RemoveFrame(nsIAtom*        aListName,
                          nsIFrame*       aOldFrame);
  
  /**
   * Get the "type" of the frame
   *
   * @see nsLayoutAtoms::svgForeignObjectFrame
   */
  // XXX Need to make sure that any of the code examining
  // frametypes, particularly code looking at block and area
  // also handles foreignObject before we return our own frametype
  // virtual nsIAtom* GetType() const;
  virtual PRBool IsFrameOfType(PRUint32 aFlags) const;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGForeignObject"), aResult);
  }
#endif

  // nsISVGValueObserver
  NS_IMETHOD WillModifySVGObservable(nsISVGValue* observable,
                                     nsISVGValue::modificationType aModType);
  NS_IMETHOD DidModifySVGObservable (nsISVGValue* observable,
                                     nsISVGValue::modificationType aModType);

  // nsISupportsWeakReference
  // implementation inherited from nsSupportsWeakReference
  
  // nsISVGChildFrame interface:
  NS_IMETHOD PaintSVG(nsISVGRendererCanvas* canvas,
                      const nsRect& dirtyRectTwips,
                      PRBool ignoreFilter);
  NS_IMETHOD GetFrameForPointSVG(float x, float y, nsIFrame** hit);  
  NS_IMETHOD_(already_AddRefed<nsISVGRendererRegion>) GetCoveredRegion();
  NS_IMETHOD InitialUpdate();
  NS_IMETHOD NotifyCanvasTMChanged(PRBool suppressInvalidation);
  NS_IMETHOD NotifyRedrawSuspended();
  NS_IMETHOD NotifyRedrawUnsuspended();
  NS_IMETHOD SetMatrixPropagation(PRBool aPropagate);
  NS_IMETHOD SetOverrideCTM(nsIDOMSVGMatrix *aCTM);
  NS_IMETHOD GetBBox(nsIDOMSVGRect **_retval);
  NS_IMETHOD GetFilterRegion(nsISVGRendererRegion **_retval) {
    *_retval = mFilterRegion;
    NS_IF_ADDREF(*_retval);
    return NS_OK;
  }  

  // nsISVGContainerFrame interface:
  virtual already_AddRefed<nsIDOMSVGMatrix> GetCanvasTM();
  virtual already_AddRefed<nsSVGCoordCtxProvider> GetCoordContextProvider();

  // foreignobject public methods
  /**
   * @param aPt a point in the twips coordinate system of the SVG outer frame
   * Transforms the point to a point in this frame's twips coordinate system
   */
  nsPoint TransformPointFromOuter(nsPoint aPt);
  
protected:
  // implementation helpers:
  void Update();
  already_AddRefed<nsISVGRendererRegion> DoReflow();
  float GetPxPerTwips();
  float GetTwipsPerPx();
  // Get the bounding box relative to the outer SVG element, in user units
  void GetBBoxInternal(float* aX, float *aY, float* aWidth, float *aHeight);
  already_AddRefed<nsIDOMSVGMatrix> GetTMIncludingOffset();
  nsresult TransformPointFromOuterPx(float aX, float aY, nsPoint* aOut);

  PRBool mIsDirty;
  nsCOMPtr<nsIDOMSVGLength> mX;
  nsCOMPtr<nsIDOMSVGLength> mY;
  nsCOMPtr<nsIDOMSVGLength> mWidth;
  nsCOMPtr<nsIDOMSVGLength> mHeight;
  nsCOMPtr<nsIDOMSVGMatrix> mCanvasTM;
  PRBool mPropagateTransform;
  nsCOMPtr<nsIDOMSVGMatrix>    mOverrideCTM;

  nsCOMPtr<nsISVGRendererRegion> mFilterRegion;
  nsISVGFilterFrame *mFilter;
};

#endif
