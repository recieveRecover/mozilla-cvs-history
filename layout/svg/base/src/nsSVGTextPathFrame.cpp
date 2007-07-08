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
 * The Initial Developer of the Original Code is IBM Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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

#include "nsSVGTextPathFrame.h"
#include "nsSVGTextFrame.h"
#include "nsIDOMSVGTextPathElement.h"
#include "nsIDOMSVGAnimatedLength.h"
#include "nsSVGLength.h"
#include "nsIDOMSVGURIReference.h"
#include "nsSVGUtils.h"
#include "nsContentUtils.h"
#include "nsIDOMSVGAnimatedPathData.h"
#include "nsSVGPathElement.h"


NS_IMPL_ISUPPORTS1(nsSVGPathListener, nsIMutationObserver)

nsSVGPathListener::nsSVGPathListener(nsIContent *aPathElement,
                                     nsSVGTextPathFrame *aTextPathFrame) :
  mTextPathFrame(aTextPathFrame)
{
  mObservedPath = do_GetWeakReference(aPathElement);
  aPathElement->AddMutationObserver(this);
}

nsSVGPathListener::~nsSVGPathListener()
{
  nsCOMPtr<nsIContent> path = do_QueryReferent(mObservedPath);
  if (path)
    path->RemoveMutationObserver(this);
}

void
nsSVGPathListener::AttributeChanged(nsIDocument *aDocument,
                                    nsIContent *aContent,
                                    PRInt32 aNameSpaceID,
                                    nsIAtom *aAttribute,
                                    PRInt32 aModType)
{
  mTextPathFrame->UpdateGraphic();
}

//----------------------------------------------------------------------
// Implementation

nsIFrame*
NS_NewSVGTextPathFrame(nsIPresShell* aPresShell, nsIContent* aContent,
                       nsIFrame* parentFrame, nsStyleContext* aContext)
{
  NS_ASSERTION(parentFrame, "null parent");
  if (parentFrame->GetType() != nsGkAtoms::svgTextFrame) {
    NS_ERROR("trying to construct an SVGTextPathFrame for an invalid container");
    return nsnull;
  }
  
  nsCOMPtr<nsIDOMSVGTextPathElement> tpath_elem = do_QueryInterface(aContent);
  if (!tpath_elem) {
    NS_ERROR("Trying to construct an SVGTextPathFrame for a "
             "content element that doesn't support the right interfaces");
    return nsnull;
  }

  return new (aPresShell) nsSVGTextPathFrame(aContext);
}

NS_IMETHODIMP
nsSVGTextPathFrame::Init(nsIContent*      aContent,
                         nsIFrame*        aParent,
                         nsIFrame*        aPrevInFlow)
{
  nsSVGTextPathFrameBase::Init(aContent, aParent, aPrevInFlow);

  nsCOMPtr<nsIDOMSVGTextPathElement> tpath = do_QueryInterface(mContent);

  {
    nsCOMPtr<nsIDOMSVGAnimatedLength> length;
    tpath->GetStartOffset(getter_AddRefs(length));

    // XXX: Gross hack as stand-in until length lists converted
#ifdef DEBUG_tor
    fprintf(stderr,
            "### Using nsSVGTextPathFrame mStartOffset hack - fix me\n");
#endif
    nsCOMPtr<nsIDOMSVGLength> offset;
    length->GetAnimVal(getter_AddRefs(offset));
    PRUint16 type;
    float value;
    offset->GetUnitType(&type);
    offset->GetValueInSpecifiedUnits(&value);
    nsCOMPtr<nsISVGLength> l;
    NS_NewSVGLength(getter_AddRefs(l), value, type);
    mStartOffset = l;

    NS_ASSERTION(mStartOffset, "no startOffset");
    if (!mStartOffset)
      return NS_ERROR_FAILURE;

    NS_NewSVGLengthList(getter_AddRefs(mX),
                        static_cast<nsSVGElement*>(mContent),
                        nsSVGUtils::X);
    if (mX) {
      nsCOMPtr<nsIDOMSVGLength> length;
      mX->AppendItem(mStartOffset, getter_AddRefs(length));
    }
  }

  {
    nsCOMPtr<nsIDOMSVGURIReference> aRef = do_QueryInterface(mContent);
    if (aRef)
      aRef->GetHref(getter_AddRefs(mHref));
    if (!mHref)
      return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

nsIAtom *
nsSVGTextPathFrame::GetType() const
{
  return nsGkAtoms::svgTextPathFrame;
}


NS_IMETHODIMP_(already_AddRefed<nsIDOMSVGLengthList>)
nsSVGTextPathFrame::GetX()
{
  nsIDOMSVGLengthList *retval = mX;
  NS_IF_ADDREF(retval);
  return retval;
}

NS_IMETHODIMP_(already_AddRefed<nsIDOMSVGLengthList>)
nsSVGTextPathFrame::GetY()
{
  return nsnull;
}

NS_IMETHODIMP_(already_AddRefed<nsIDOMSVGLengthList>)
nsSVGTextPathFrame::GetDx()
{
  return nsnull;
}

NS_IMETHODIMP_(already_AddRefed<nsIDOMSVGLengthList>)
nsSVGTextPathFrame::GetDy()
{
  return nsnull;
}

//----------------------------------------------------------------------
// nsSVGTextPathFrame methods:

nsIFrame *
nsSVGTextPathFrame::GetPathFrame() {
  nsIFrame *path = nsnull;

  nsAutoString str;
  mHref->GetAnimVal(str);

  nsCOMPtr<nsIURI> targetURI;
  nsCOMPtr<nsIURI> base = mContent->GetBaseURI();
  nsContentUtils::NewURIWithDocumentCharset(getter_AddRefs(targetURI), str,
                                            mContent->GetCurrentDoc(), base);

  nsSVGUtils::GetReferencedFrame(&path, targetURI, mContent,
                                 PresContext()->PresShell());
  if (!path || (path->GetType() != nsGkAtoms::svgPathGeometryFrame))
    return nsnull;
  return path;
}

already_AddRefed<gfxFlattenedPath>
nsSVGTextPathFrame::GetFlattenedPath() {
  nsIFrame *path = GetPathFrame();
  if (!path)
    return nsnull;

  nsSVGPathGeometryElement *element = static_cast<nsSVGPathGeometryElement*>
                                                 (path->GetContent());

  if (!mPathListener) {
    mPathListener = new nsSVGPathListener(path->GetContent(), this);
  }

  nsCOMPtr<nsIDOMSVGMatrix> localTM = element->GetLocalTransformMatrix();

  return element->GetFlattenedPath(localTM);
}

//----------------------------------------------------------------------
// nsIFrame methods

NS_IMETHODIMP
nsSVGTextPathFrame::AttributeChanged(PRInt32         aNameSpaceID,
                                     nsIAtom*        aAttribute,
                                     PRInt32         aModType)
{
  if (aNameSpaceID == kNameSpaceID_None &&
      aAttribute == nsGkAtoms::startOffset) {
    UpdateGraphic();
  } else if (aNameSpaceID == kNameSpaceID_XLink &&
             aAttribute == nsGkAtoms::href) {
    mPathListener = nsnull;
    UpdateGraphic();
  }

  return NS_OK;
}
