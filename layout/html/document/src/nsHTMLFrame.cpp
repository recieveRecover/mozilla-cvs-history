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

#include "nsHTMLContainer.h"
#include "nsLeafFrame.h"
#include "nsHTMLContainerFrame.h"
#include "nsIWebShell.h"
#include "nsIPresContext.h"
#include "nsIPresShell.h"
#include "nsHTMLIIDs.h"
#include "nsRepository.h"
#include "nsIStreamListener.h"
#include "nsIURL.h"
#include "nsIDocument.h"
#include "nsIView.h"
#include "nsIViewManager.h"
#include "nsWidgetsCID.h"
#include "nsViewsCID.h"
#include "nsHTMLAtoms.h"
#include "nsIScrollableView.h"
#include "nsStyleCoord.h"
#include "nsIStyleContext.h"
#include "nsCSSLayout.h"
#include "nsIDocumentLoader.h"
#include "nsIPref.h"
//#include "nsIDocumentWidget.h"
#include "nsHTMLFrameset.h"
class nsHTMLFrame;

static NS_DEFINE_IID(kIWebShellContainerIID, NS_IWEB_SHELL_CONTAINER_IID);
static NS_DEFINE_IID(kIStreamObserverIID, NS_ISTREAMOBSERVER_IID);
static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIWebShellIID, NS_IWEB_SHELL_IID);
static NS_DEFINE_IID(kWebShellCID, NS_WEB_SHELL_CID);
static NS_DEFINE_IID(kIViewIID, NS_IVIEW_IID);
static NS_DEFINE_IID(kCViewCID, NS_VIEW_CID);
static NS_DEFINE_IID(kCChildCID, NS_CHILD_CID);

/*******************************************************************************
 * TempObserver XXX temporary until doc manager/loader is in place
 ******************************************************************************/
class TempObserver : public nsIStreamObserver
{
public:
  TempObserver() { NS_INIT_REFCNT(); }

  ~TempObserver() {}
  // nsISupports
  NS_DECL_ISUPPORTS

  // nsIStreamObserver
  NS_IMETHOD OnStartBinding(nsIURL* aURL, const char *aContentType);
  NS_IMETHOD OnProgress(nsIURL* aURL, PRInt32 aProgress, PRInt32 aProgressMax);
  NS_IMETHOD OnStatus(nsIURL* aURL, const nsString& aMsg);
  NS_IMETHOD OnStopBinding(nsIURL* aURL, PRInt32 status, const nsString& aMsg);

protected:

  nsString mURL;
  nsString mOverURL;
  nsString mOverTarget;
};

/*******************************************************************************
 * FrameLoadingInfo 
 ******************************************************************************/
class FrameLoadingInfo : public nsISupports
{
public:
  FrameLoadingInfo(const nsSize& aSize);

  // nsISupports interface...
  NS_DECL_ISUPPORTS

protected:
  virtual ~FrameLoadingInfo() {}

public:
  nsSize mFrameSize;
};

/*******************************************************************************
 * nsHTMLFrameOuterFrame
 ******************************************************************************/
class nsHTMLFrameOuterFrame : public nsHTMLContainerFrame {

public:
  nsHTMLFrameOuterFrame(nsIContent* aContent, nsIFrame* aParent);

  NS_IMETHOD ListTag(FILE* out = stdout) const;

  NS_IMETHOD Paint(nsIPresContext& aPresContext,
                   nsIRenderingContext& aRenderingContext,
                   const nsRect& aDirtyRect);

  NS_IMETHOD Reflow(nsIPresContext&      aPresContext,
                    nsReflowMetrics&     aDesiredSize,
                    const nsReflowState& aReflowState,
                    nsReflowStatus&      aStatus);
  NS_IMETHOD  VerifyTree() const;
  nscoord GetBorderWidth(nsIPresContext& aPresContext);
  PRBool IsInline();

protected:
  virtual void GetDesiredSize(nsIPresContext* aPresContext,
                              const nsReflowState& aReflowState,
                              nsReflowMetrics& aDesiredSize);
  virtual PRIntn GetSkipSides() const;
};

/*******************************************************************************
 * nsHTMLFrameInnerFrame
 ******************************************************************************/
class nsHTMLFrameInnerFrame : public nsLeafFrame {

public:

  nsHTMLFrameInnerFrame(nsIContent* aContent, nsIFrame* aParentFrame);

  NS_IMETHOD ListTag(FILE* out = stdout) const;

  /**
    * @see nsIFrame::Paint
    */
  NS_IMETHOD Paint(nsIPresContext& aPresContext,
                   nsIRenderingContext& aRenderingContext,
                   const nsRect& aDirtyRect);

  /**
    * @see nsIFrame::Reflow
    */
  NS_IMETHOD Reflow(nsIPresContext&      aCX,
                    nsReflowMetrics&     aDesiredSize,
                    const nsReflowState& aReflowState,
                    nsReflowStatus&      aStatus);

  NS_IMETHOD MoveTo(nscoord aX, nscoord aY);
  NS_IMETHOD SizeTo(nscoord aWidth, nscoord aHeight);

  NS_IMETHOD GetParentContent(nsHTMLFrame*& aContent);

protected:
  nsresult CreateWebShell(nsIPresContext& aPresContext, const nsSize& aSize);

  virtual ~nsHTMLFrameInnerFrame();

  virtual void GetDesiredSize(nsIPresContext* aPresContext,
                              const nsReflowState& aReflowState,
                              nsReflowMetrics& aDesiredSize);

  nsIWebShell* mWebShell;
  PRBool mCreatingViewer;

  // XXX fix these
  TempObserver* mTempObserver;
};

/*******************************************************************************
 * nsHTMLFrame
 ******************************************************************************/
class nsHTMLFrame : public nsHTMLContainer {
public:
 
  NS_IMETHOD CreateFrame(nsIPresContext*  aPresContext,
                         nsIFrame*        aParentFrame,
                         nsIStyleContext* aStyleContext,
                         nsIFrame*&       aResult);
  NS_IMETHOD List(FILE* out = stdout, PRInt32 aIndent = 0) const;
  NS_IMETHOD MapAttributesInto(nsIStyleContext* aContext,
                               nsIPresContext* aPresContext);
  NS_IMETHOD SetAttribute(nsIAtom* aAttribute, const nsString& aValue,
                          PRBool aNotify);

  PRBool GetURL(nsString& aURLSpec);
  PRBool GetName(nsString& aName);
  nsScrollPreference GetScrolling();
  nsFrameborder GetFrameBorder();
  PRBool IsInline() { return mInline; }

  PRInt32 GetMarginWidth(nsIPresContext* aPresContext);
  PRInt32 GetMarginHeight(nsIPresContext* aPresContext);  


protected:
  nsHTMLFrame(nsIAtom* aTag, PRBool aInline, nsIWebShell* aParentWebWidget);
  virtual  ~nsHTMLFrame();
  PRInt32 GetMargin(nsIAtom* aType, nsIPresContext* aPresContext);


  PRBool mInline; // true for <IFRAME>, false for <FRAME>
  // this is held for a short time until the frame uses it, so it is not ref counted
  nsIWebShell* mParentWebWidget;  

  friend nsresult NS_NewHTMLIFrame(nsIHTMLContent** aInstancePtrResult,
                                   nsIAtom* aTag, nsIWebShell* aWebWidget);
  friend nsresult NS_NewHTMLFrame(nsIHTMLContent** aInstancePtrResult,
                                   nsIAtom* aTag, nsIWebShell* aWebWidget);
  friend class nsHTMLFrameInnerFrame;

};

/*******************************************************************************
 * nsHTMLFrameOuterFrame
 ******************************************************************************/
nsHTMLFrameOuterFrame::nsHTMLFrameOuterFrame(nsIContent* aContent, nsIFrame* aParent)
  : nsHTMLContainerFrame(aContent, aParent)
{
}

nscoord
nsHTMLFrameOuterFrame::GetBorderWidth(nsIPresContext& aPresContext)
{
  if (IsInline()) {
    const nsStyleSpacing* spacing =
      (const nsStyleSpacing*)mStyleContext->GetStyleData(eStyleStruct_Spacing);
    nsStyleCoord leftBorder;
    spacing->mBorder.GetLeft(leftBorder);
    nsStyleUnit unit = leftBorder.GetUnit(); 
    if (eStyleUnit_Coord == unit) {
      return leftBorder.GetCoordValue();
    }
  } 
  return 0;
}

PRIntn
nsHTMLFrameOuterFrame::GetSkipSides() const
{
  return 0;
}

void 
nsHTMLFrameOuterFrame::GetDesiredSize(nsIPresContext* aPresContext,
                                      const nsReflowState& aReflowState,
                                      nsReflowMetrics& aDesiredSize)
{
  // <frame> processing does not use this routine, only <iframe>
  float p2t = aPresContext->GetPixelsToTwips();

  nsSize size;
  PRIntn ss = nsCSSLayout::GetStyleSize(aPresContext, aReflowState, size);

  // XXX this needs to be changed from (200,200) to a better default for inline frames
  if (0 == (ss & NS_SIZE_HAS_WIDTH)) {
    size.width = NSIntPixelsToTwips(200, p2t);
  }
  if (0 == (ss & NS_SIZE_HAS_HEIGHT)) {
    size.height = NSIntPixelsToTwips(200, p2t);
  }

  aDesiredSize.width  = size.width;
  aDesiredSize.height = size.height;
  aDesiredSize.ascent = aDesiredSize.height;
  aDesiredSize.descent = 0;
}

PRBool nsHTMLFrameOuterFrame::IsInline()
{ 
  return ((nsHTMLFrame*)mContent)->IsInline(); 
}

NS_IMETHODIMP
nsHTMLFrameOuterFrame::Paint(nsIPresContext& aPresContext,
                         nsIRenderingContext& aRenderingContext,
                         const nsRect& aDirtyRect)
{
  //printf("outer paint %d (%d,%d,%d,%d) ", this, aDirtyRect.x, aDirtyRect.y, aDirtyRect.width, aDirtyRect.height);
  if (nsnull != mFirstChild) {
    mFirstChild->Paint(aPresContext, aRenderingContext, aDirtyRect);
  }
  if (IsInline()) {
    return nsHTMLContainerFrame::Paint(aPresContext, aRenderingContext, aDirtyRect);
  } else {
    return NS_OK;
  }
}

NS_IMETHODIMP nsHTMLFrameOuterFrame::ListTag(FILE* out) const
{
  nsHTMLContainerFrame::ListTag(out);
  fputs(" (OUTER)", out);
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLFrameOuterFrame::Reflow(nsIPresContext&      aPresContext,
                              nsReflowMetrics&     aDesiredSize,
                              const nsReflowState& aReflowState,
                              nsReflowStatus&      aStatus)
{
  NS_FRAME_TRACE(NS_FRAME_TRACE_CALLS,
     ("enter nsHTMLFrameOuterFrame::Reflow: maxSize=%d,%d reason=%d",
      aReflowState.maxSize.width,
      aReflowState.maxSize.height,
      aReflowState.reason));

  if (IsInline()) {
    GetDesiredSize(&aPresContext, aReflowState, aDesiredSize);
  } else {
    aDesiredSize.width  = aReflowState.maxSize.width;
    aDesiredSize.height = aReflowState.maxSize.height;
  }

  if (nsnull == mFirstChild) {
    mFirstChild = new nsHTMLFrameInnerFrame(mContent, this);
    // XXX temporary! use style system to get correct style!
    mFirstChild->SetStyleContext(&aPresContext, mStyleContext);
    mChildCount = 1;
  }
 
  // nsContainerFrame::PaintBorder has some problems, kludge it here
  nscoord borderWidth  = GetBorderWidth(aPresContext);
  nscoord kludge = borderWidth/2;
  nsSize innerSize(aDesiredSize.width - borderWidth - kludge, aDesiredSize.height - borderWidth - kludge);

  // Reflow the child and get its desired size
  nsReflowMetrics kidMetrics(aDesiredSize.maxElementSize);
  nsReflowState kidReflowState(mFirstChild, aReflowState, innerSize);
  mFirstChild->WillReflow(aPresContext);
  aStatus = ReflowChild(mFirstChild, &aPresContext, kidMetrics, kidReflowState);
  NS_ASSERTION(NS_FRAME_IS_COMPLETE(aStatus), "bad status");
  
  // Place and size the child
  nsRect rect(borderWidth, borderWidth, innerSize.width, innerSize.height);
  mFirstChild->SetRect(rect);
  mFirstChild->DidReflow(aPresContext, NS_FRAME_REFLOW_FINISHED);

  // XXX what should the max-element-size of an iframe be? Shouldn't
  // iframe's normally shrink wrap around their content when they
  // don't have a specified width/height?
  if (nsnull != aDesiredSize.maxElementSize) {
    aDesiredSize.maxElementSize->width = aDesiredSize.width;
    aDesiredSize.maxElementSize->height = aDesiredSize.height;
  }

  NS_FRAME_TRACE(NS_FRAME_TRACE_CALLS,
     ("exit nsHTMLFrameOuterFrame::Reflow: size=%d,%d status=%x",
      aDesiredSize.width, aDesiredSize.height, aStatus));

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLFrameOuterFrame::VerifyTree() const
{
  // XXX Completely disabled for now; once pseud-frames are reworked
  // then we can turn it back on.
  return NS_OK;
}

/*******************************************************************************
 * nsHTMLFrameInnerFrame
 ******************************************************************************/
nsHTMLFrameInnerFrame::nsHTMLFrameInnerFrame(nsIContent* aContent,
                                             nsIFrame* aParentFrame)
  : nsLeafFrame(aContent, aParentFrame)
{
  mWebShell = nsnull;
  mCreatingViewer = PR_FALSE;
  mTempObserver = new TempObserver();
  NS_ADDREF(mTempObserver);
}

nsHTMLFrameInnerFrame::~nsHTMLFrameInnerFrame()
{
  if (nsnull != mWebShell) {
    // XXX: Is the needed (or wanted?)
    mWebShell->SetContainer(nsnull);
    NS_RELEASE(mWebShell);
  }
  NS_RELEASE(mTempObserver);
}

#if 0
float nsHTMLFrameInnerFrame::GetTwipsToPixels()
{
  nsISupports* parentSup;
  if (mWebShell) {
    mWebShell->GetContainer(&parentSup);
    if (parentSup) {
      nsIWebShell* parentWidget;
      nsresult res = parentSup->QueryInterface(kIWebWidgetIID, (void**)&parentWidget);
      if (NS_OK == res) {
        nsIPresContext* presContext = parentWidget->GetPresContext();
        NS_RELEASE(parentWidget);
        if (presContext) {
          float ret = presContext->GetTwipsToPixels();
          NS_RELEASE(presContext);
          return ret;
        } 
      } else {
        NS_ASSERTION(0, "invalid web widget container");
      }
    } else {
      NS_ASSERTION(0, "invalid web widget container");
    }
  }
  return (float)0.05;  // this should not be reached
}
#endif


NS_IMETHODIMP nsHTMLFrameInnerFrame::ListTag(FILE* out) const
{
  nsLeafFrame::ListTag(out);
  fputs(" (INNER)", out);
  return NS_OK;
}

NS_METHOD
nsHTMLFrameInnerFrame::MoveTo(nscoord aX, nscoord aY)
{
  return nsLeafFrame::MoveTo(aX, aY);
}

NS_METHOD
nsHTMLFrameInnerFrame::SizeTo(nscoord aWidth, nscoord aHeight)
{
  return nsLeafFrame::SizeTo(aWidth, aHeight);
}

NS_IMETHODIMP
nsHTMLFrameInnerFrame::Paint(nsIPresContext& aPresContext,
                         nsIRenderingContext& aRenderingContext,
                         const nsRect& aDirtyRect)
{
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLFrameInnerFrame::GetParentContent(nsHTMLFrame*& aContent)
{
  nsHTMLFrameOuterFrame* parent;
  GetGeometricParent((nsIFrame*&)parent);
  return parent->GetContent((nsIContent*&)aContent);
}


void TempMakeAbsURL(nsIContent* aContent, nsString& aRelURL, nsString& aAbsURL)
{
  nsIURL* docURL = nsnull;
  nsIDocument* doc = nsnull;
  aContent->GetDocument(doc);
  if (nsnull != doc) {
    docURL = doc->GetDocumentURL();
    NS_RELEASE(doc);
  }

  nsAutoString base;
  nsresult rv = NS_MakeAbsoluteURL(docURL, base, aRelURL, aAbsURL);
  NS_IF_RELEASE(docURL);
}


nsresult
nsHTMLFrameInnerFrame::CreateWebShell(nsIPresContext& aPresContext,
                                      const nsSize& aSize)
{
  nsresult rv;
  nsHTMLFrame* content;
  GetParentContent(content);

  rv = NSRepository::CreateInstance(kWebShellCID, nsnull, kIWebShellIID,
                                    (void**)&mWebShell);
  if (NS_OK != rv) {
    NS_ASSERTION(0, "could not create web widget");
    return rv;
  }

  // pass along marginwidth, marginheight so sub document can use it
  mWebShell->SetMarginWidth(content->GetMarginWidth(&aPresContext));
  mWebShell->SetMarginHeight(content->GetMarginHeight(&aPresContext));

  nsString frameName;
  if (content->GetName(frameName)) {
    mWebShell->SetName(frameName);
  }

  // If our container is a web-shell, inform it that it has a new
  // child. If it's not a web-shell then some things will not operate
  // properly.
  nsISupports* container;
  aPresContext.GetContainer(&container);
  if (nsnull != container) {
    nsIWebShell* outerShell = nsnull;
    container->QueryInterface(kIWebShellIID, (void**) &outerShell);
    if (nsnull != outerShell) {
      outerShell->AddChild(mWebShell);

      // connect the container...
      nsIWebShellContainer* outerContainer = nsnull;
      container->QueryInterface(kIWebShellContainerIID, (void**) &outerContainer);
      if (nsnull != outerContainer) {
        mWebShell->SetContainer(outerContainer);
        NS_RELEASE(outerContainer);
      }

      nsIPref*  outerPrefs = nsnull;  // connect the prefs
      outerShell->GetPrefs(outerPrefs);
      if (nsnull != outerPrefs) {
        mWebShell->SetPrefs(outerPrefs);
        NS_RELEASE(outerPrefs);
      }
      NS_RELEASE(outerShell);
    }
    NS_RELEASE(container);
  }

  float t2p = aPresContext.GetTwipsToPixels();
  nsIPresShell *presShell = aPresContext.GetShell();     

  // create, init, set the parent of the view
  nsIView* view;
  rv = NSRepository::CreateInstance(kCViewCID, nsnull, kIViewIID,
                                        (void **)&view);
  if (NS_OK != rv) {
    NS_ASSERTION(0, "Could not create view for nsHTMLFrame");
    return rv;
  }

  nsIView* parView;
  nsPoint origin;
  GetOffsetFromView(origin, parView);  
  nsRect viewBounds(origin.x, origin.y, aSize.width, aSize.height);

  nsIViewManager* viewMan = presShell->GetViewManager();  
  NS_RELEASE(presShell);
  rv = view->Init(viewMan, viewBounds,
                  parView, &kCChildCID);
  viewMan->InsertChild(parView, view, 0);
  NS_RELEASE(viewMan);
  SetView(view);

  nsIWidget* widget;
  view->GetWidget(widget);
  nsRect webBounds(0, 0, NSToCoordRound(aSize.width * t2p), 
                   NSToCoordRound(aSize.height * t2p));

  mWebShell->Init(widget->GetNativeData(NS_NATIVE_WIDGET), 
                  webBounds.x, webBounds.y,
                  webBounds.width, webBounds.height,
                  content->GetScrolling());
  NS_RELEASE(content);
  NS_RELEASE(widget);

  mWebShell->SetObserver(mTempObserver);
  mWebShell->Show();

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLFrameInnerFrame::Reflow(nsIPresContext&      aPresContext,
                              nsReflowMetrics&     aDesiredSize,
                              const nsReflowState& aReflowState,
                              nsReflowStatus&      aStatus)
{
  NS_FRAME_TRACE(NS_FRAME_TRACE_CALLS,
     ("enter nsHTMLFrameInnerFrame::Reflow: maxSize=%d,%d reason=%d",
      aReflowState.maxSize.width,
      aReflowState.maxSize.height,
      aReflowState.reason));

  nsresult rv = NS_OK;

  // use the max size set in aReflowState by the nsHTMLFrameOuterFrame as our size
  if (!mCreatingViewer) {
    nsHTMLFrame* content;
    GetParentContent(content);

    nsAutoString url;
    content->GetURL(url);
    nsSize size;
 
    if (nsnull == mWebShell) {
      rv = CreateWebShell(aPresContext, aReflowState.maxSize);
    }

    if (nsnull != mWebShell) {
      mCreatingViewer=PR_TRUE;

      // load the document
      nsString absURL;
      TempMakeAbsURL(content, url, absURL);

      rv = mWebShell->LoadURL(absURL,          // URL string
                              nsnull);         // Post Data
    }
    NS_RELEASE(content);
  }

  aDesiredSize.width  = aReflowState.maxSize.width;
  aDesiredSize.height = aReflowState.maxSize.height;
  aDesiredSize.ascent = aDesiredSize.height;
  aDesiredSize.descent = 0;

  // resize the sub document
  float t2p = aPresContext.GetTwipsToPixels();
  nsRect subBounds;

  mWebShell->GetBounds(subBounds.x, subBounds.y,
                       subBounds.width, subBounds.height);
  subBounds.width  = NSToCoordRound(aDesiredSize.width * t2p);
  subBounds.height = NSToCoordRound(aDesiredSize.height * t2p);
  mWebShell->SetBounds(subBounds.x, subBounds.y,
                       subBounds.width, subBounds.height);

  aStatus = NS_FRAME_COMPLETE;

  NS_FRAME_TRACE(NS_FRAME_TRACE_CALLS,
     ("exit nsHTMLFrameInnerFrame::Reflow: size=%d,%d rv=%x",
      aDesiredSize.width, aDesiredSize.height, aStatus));
  return rv;
}

void 
nsHTMLFrameInnerFrame::GetDesiredSize(nsIPresContext* aPresContext,
                                      const nsReflowState& aReflowState,
                                      nsReflowMetrics& aDesiredSize)
{
  // it must be defined, but not called
  NS_ASSERTION(0, "this should never be called");
  aDesiredSize.width   = 0;
  aDesiredSize.height  = 0;
  aDesiredSize.ascent  = 0;
  aDesiredSize.descent = 0;
}

/*******************************************************************************
 * nsHTMLFrame
 ******************************************************************************/
nsHTMLFrame::nsHTMLFrame(nsIAtom* aTag, PRBool aInline, nsIWebShell* aParentWebWidget)
  : nsHTMLContainer(aTag), mInline(aInline), mParentWebWidget(aParentWebWidget)
{
}

nsHTMLFrame::~nsHTMLFrame()
{
  mParentWebWidget = nsnull;
}

NS_IMETHODIMP
nsHTMLFrame::List(FILE* out, PRInt32 aIndent) const
{
  for (PRInt32 i = aIndent; --i >= 0; ) fputs("  ", out);   // Indent
  fprintf(out, "%X ", this);
  if (mInline) {
    fprintf(out, "INLINE \n", this);
  } else {
    fprintf(out, "\n");
  }
  return nsHTMLContent::List(out, aIndent);
}

NS_IMETHODIMP
nsHTMLFrame::SetAttribute(nsIAtom* aAttribute, const nsString& aString,
                          PRBool aNotify)
{
  nsHTMLValue val;
  if (ParseImageProperty(aAttribute, aString, val)) { // convert width or height to pixels
    return nsHTMLTagContent::SetAttribute(aAttribute, val, aNotify);
  }
  return nsHTMLContainer::SetAttribute(aAttribute, aString, aNotify);
}


NS_IMETHODIMP
nsHTMLFrame::MapAttributesInto(nsIStyleContext* aContext, 
                               nsIPresContext* aPresContext)
{
  MapImagePropertiesInto(aContext, aPresContext);
  return NS_OK;
}

PRInt32 nsHTMLFrame::GetMargin(nsIAtom* aType, nsIPresContext* aPresContext) 
{
  float p2t = aPresContext->GetPixelsToTwips();
  nsAutoString strVal;
  PRInt32 intVal;

  if (NS_CONTENT_ATTR_HAS_VALUE == (GetAttribute(aType, strVal))) {
    PRInt32 status;
    intVal = strVal.ToInteger(&status);
    if (intVal < 0) {
      intVal = 0;
    }
    return NSIntPixelsToTwips(intVal, p2t);
  }

  return -1;
}

PRInt32 nsHTMLFrame::GetMarginWidth(nsIPresContext* aPresContext)
{
  return GetMargin(nsHTMLAtoms::marginwidth, aPresContext);
}

PRInt32 nsHTMLFrame::GetMarginHeight(nsIPresContext* aPresContext)
{
  return GetMargin(nsHTMLAtoms::marginheight, aPresContext);
}

nsresult
nsHTMLFrame::CreateFrame(nsIPresContext*  aPresContext,
                          nsIFrame*        aParentFrame,
                          nsIStyleContext* aStyleContext,
                          nsIFrame*&       aResult)
{
  nsIFrame* frame = new nsHTMLFrameOuterFrame(this, aParentFrame);
  if (nsnull == frame) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  aResult = frame;
  frame->SetStyleContext(aPresContext, aStyleContext);
  return NS_OK;
}

PRBool nsHTMLFrame::GetURL(nsString& aURLSpec)
{
  nsHTMLValue value;
  if (NS_CONTENT_ATTR_HAS_VALUE == (GetAttribute(nsHTMLAtoms::src, value))) {
    if (eHTMLUnit_String == value.GetUnit()) {
      value.GetStringValue(aURLSpec);
      return PR_TRUE;
    }
  }
  aURLSpec.SetLength(0);
  return PR_FALSE;
}

PRBool nsHTMLFrame::GetName(nsString& aName)
{
  nsHTMLValue value;
  if (NS_CONTENT_ATTR_HAS_VALUE == (GetAttribute(nsHTMLAtoms::name, value))) {
    if (eHTMLUnit_String == value.GetUnit()) {
      value.GetStringValue(aName);
      return PR_TRUE;
    }
  }
  aName.SetLength(0);
  return PR_FALSE;
}

nsScrollPreference nsHTMLFrame::GetScrolling()
{
  nsHTMLValue value;
  if (NS_CONTENT_ATTR_HAS_VALUE == (GetAttribute(nsHTMLAtoms::scrolling, value))) {
    if (eHTMLUnit_String == value.GetUnit()) {
      nsAutoString scrolling;
      value.GetStringValue(scrolling);
      if (scrolling.EqualsIgnoreCase("yes")) {
        return nsScrollPreference_kAlwaysScroll;
      } 
      else if (scrolling.EqualsIgnoreCase("no")) {
        return nsScrollPreference_kNeverScroll;
      }
    }
  }
  return nsScrollPreference_kAuto;
}

nsFrameborder nsHTMLFrame::GetFrameBorder()
{
  nsHTMLValue value;
  if (NS_CONTENT_ATTR_HAS_VALUE == (GetAttribute(nsHTMLAtoms::frameborder, value))) {
    if (eHTMLUnit_String == value.GetUnit()) {
      nsAutoString frameborder;
      value.GetStringValue(frameborder);
      if (frameborder.EqualsIgnoreCase("no") || frameborder.EqualsIgnoreCase("0")) {
        return eFrameborder_No;
      } else {
        return eFrameborder_Yes;
      }
    }
  }
  return eFrameborder_Notset;
}

nsresult
NS_NewHTMLFrame(nsIHTMLContent** aInstancePtrResult,
                 nsIAtom* aTag, nsIWebShell* aWebWidget)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null ptr");
  if (nsnull == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }

  nsIHTMLContent* it = new nsHTMLFrame(aTag, PR_FALSE, aWebWidget);

  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return it->QueryInterface(kIHTMLContentIID, (void**) aInstancePtrResult);
}

nsresult
NS_NewHTMLIFrame(nsIHTMLContent** aInstancePtrResult,
                 nsIAtom* aTag, nsIWebShell* aWebWidget)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null ptr");
  if (nsnull == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }

  nsIHTMLContent* it = new nsHTMLFrame(aTag, PR_TRUE, aWebWidget);

  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return it->QueryInterface(kIHTMLContentIID, (void**) aInstancePtrResult);
}

/*******************************************************************************
 * FrameLoadingInfo
 ******************************************************************************/
FrameLoadingInfo::FrameLoadingInfo(const nsSize& aSize)
{
  NS_INIT_REFCNT();

  mFrameSize = aSize;
}

/*
 * Implementation of ISupports methods...
 */
NS_IMPL_ISUPPORTS(FrameLoadingInfo,kISupportsIID);

// XXX temp implementation

NS_IMPL_ADDREF(TempObserver);
NS_IMPL_RELEASE(TempObserver);

/*******************************************************************************
 * TempObserver
 ******************************************************************************/
nsresult
TempObserver::QueryInterface(const nsIID& aIID,
                            void** aInstancePtrResult)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null pointer");
  if (nsnull == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(kIStreamObserverIID)) {
    *aInstancePtrResult = (void*) ((nsIStreamObserver*)this);
    AddRef();
    return NS_OK;
  }
  if (aIID.Equals(kISupportsIID)) {
    *aInstancePtrResult = (void*) ((nsISupports*)((nsIDocumentObserver*)this));
    AddRef();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}


NS_IMETHODIMP
TempObserver::OnProgress(nsIURL* aURL, PRInt32 aProgress, PRInt32 aProgressMax)
{
#if 0
  fputs("[progress ", stdout);
  fputs(mURL, stdout);
  printf(" %d %d ", aProgress, aProgressMax);
  fputs("]\n", stdout);
#endif
  return NS_OK;
}

NS_IMETHODIMP
TempObserver::OnStatus(nsIURL* aURL, const nsString& aMsg)
{
#if 0
  fputs("[status ", stdout);
  fputs(mURL, stdout);
  fputs(aMsg, stdout);
  fputs("]\n", stdout);
#endif
  return NS_OK;
}

NS_IMETHODIMP
TempObserver::OnStartBinding(nsIURL* aURL, const char *aContentType)
{
#if 0
  fputs("Loading ", stdout);
  fputs(mURL, stdout);
  fputs("\n", stdout);
#endif
  return NS_OK;
}

NS_IMETHODIMP
TempObserver::OnStopBinding(nsIURL* aURL, PRInt32 status, const nsString& aMsg)
{
#if 0
  fputs("Done loading ", stdout);
  fputs(mURL, stdout);
  fputs("\n", stdout);
#endif
  return NS_OK;
}


