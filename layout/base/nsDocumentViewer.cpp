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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Dan Rosen <dr@netscape.com>
 *   Roland Mainz <roland.mainz@informatik.med.uni-giessen.de>
 *
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

#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsCRT.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsISupports.h"
#include "nsIContent.h"
#include "nsIContentViewerContainer.h"
#include "nsIDocumentViewer.h"
#include "nsIDOMWindowInternal.h"

#include "nsIDocument.h"
#include "nsIPresContext.h"
#include "nsIPresShell.h"
#include "nsIStyleSet.h"
#include "nsIStyleSheet.h"
#include "nsICSSStyleSheet.h"
#include "nsIFrame.h"

#include "nsIScriptGlobalObjectOwner.h"
#include "nsIScriptGlobalObject.h"
#include "nsILinkHandler.h"
#include "nsIDOMDocument.h"
#include "nsISelectionListener.h"
#include "nsISelectionPrivate.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMHTMLElement.h"
#include "nsIDOMRange.h"
#include "nsContentCID.h"
#include "nsLayoutCID.h"
#include "nsGenericHTMLElement.h"

#include "nsViewsCID.h"
#include "nsWidgetsCID.h"
#include "nsGfxCIID.h"
#include "nsIDeviceContext.h"
#include "nsIDeviceContextSpec.h"
#include "nsIDeviceContextSpecFactory.h"
#include "nsIViewManager.h"
#include "nsIView.h"

#include "nsIPref.h"
#include "nsIPageSequenceFrame.h"
#include "nsIURL.h"
#include "nsIWebShell.h"
#include "nsIContentViewerEdit.h"
#include "nsIContentViewerFile.h"
#include "nsIMarkupDocumentViewer.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeNode.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIDocShell.h"
#include "nsIFrameDebug.h"
#include "nsILayoutHistoryState.h"
#include "nsLayoutAtoms.h"
#include "nsIFrameManager.h"
#include "nsIParser.h"
#include "nsIPrintContext.h"
#include "nsGUIEvent.h"
#include "nsHTMLReflowState.h"
#include "nsIDOMHTMLAnchorElement.h"
#include "nsIDOMHTMLAreaElement.h"
#include "nsIDOMHTMLLinkElement.h"
#include "nsIDOMHTMLImageElement.h"

#include "nsIChromeRegistry.h"

#include "nsIEventQueueService.h"
#include "nsIEventQueue.h"

// Timer Includes
#include "nsITimer.h"
#include "nsITimerCallback.h"
#include "nsITimelineService.h"

#include "nsPIDOMWindow.h"
#include "nsIFocusController.h"

// Print Options
#include "nsIPrintSettings.h"
#include "nsIPrintOptions.h"
#include "nsGfxCIID.h"
#include "nsIServiceManager.h"
#include "nsHTMLAtoms.h" // XXX until atoms get factored into nsLayoutAtoms
#include "nsISimpleEnumerator.h"
#include "nsISupportsPrimitives.h"
static NS_DEFINE_CID(kPrintOptionsCID, NS_PRINTOPTIONS_CID);

// Printing Events
#include "nsIEventQueue.h"
#include "nsIEventQueueService.h"
static NS_DEFINE_CID(kEventQueueServiceCID, NS_EVENTQUEUESERVICE_CID);

// Printing
#include "nsIDOMHTMLFrameElement.h"
#include "nsIDOMHTMLFrameSetElement.h"
#include "nsIDOMHTMLIFrameElement.h"

// Print Preview
#include "nsIPrintPreviewContext.h"
#include "nsIEventProcessor.h"
#include "imgIContainer.h" // image animation mode constants

// Print Progress
#include "nsPrintProgress.h"
#include "nsPrintProgressParams.h"


// Print error dialog
#include "nsIPrompt.h"
#include "nsIWindowWatcher.h"
#include "nsIStringBundle.h"

#define NS_ERROR_GFX_PRINTER_BUNDLE_URL "chrome://communicator/locale/printing.properties"

// FrameSet
#include "nsINodeInfo.h"
#include "nsIDocument.h"
#include "nsHTMLAtoms.h"
#include "nsIHTMLContent.h"
#include "nsINameSpaceManager.h"
#include "nsIWebShell.h"

//focus
#include "nsIDOMEventReceiver.h" 
#include "nsIDOMFocusListener.h"
#include "nsISelectionController.h"

#include "nsITransformMediator.h"

#ifdef IBMBIDI
#include "nsIUBidiUtils.h"
#endif

static NS_DEFINE_CID(kEventQueueService, NS_EVENTQUEUESERVICE_CID);
static NS_DEFINE_CID(kPresShellCID, NS_PRESSHELL_CID);
static NS_DEFINE_CID(kGalleyContextCID,  NS_GALLEYCONTEXT_CID);
static NS_DEFINE_CID(kPrintContextCID,  NS_PRINTCONTEXT_CID);
static NS_DEFINE_CID(kStyleSetCID,  NS_STYLESET_CID);

#ifdef NS_DEBUG

#undef NOISY_VIEWER
#else
#undef NOISY_VIEWER
#endif

#if defined(DEBUG_rods) || defined(DEBUG_dcone)
#define DEBUG_PRINTING
#endif

#ifdef DEBUG_PRINTING
// XXX NOTE: I am including a header from the layout directory
// merely to set up a file pointer for debug logging. This is
// fragile and may break in the future, which means it can be 
// removed if necessary
#if defined(XP_PC)
#include "../../../layout/html/base/src/nsSimplePageSequence.h"
#endif
#define PRT_YESNO(_p) ((_p)?"YES":"NO")

static const char * gFrameTypesStr[]       = {"eDoc", "eFrame", "eIFrame", "eFrameSet"};
static const char * gPrintFrameTypeStr[]   = {"kNoFrames", "kFramesAsIs", "kSelectedFrame", "kEachFrameSep"};
static const char * gFrameHowToEnableStr[] = {"kFrameEnableNone", "kFrameEnableAll", "kFrameEnableAsIsAndEach"};
static const char * gPrintRangeStr[]       = {"kRangeAllPages", "kRangeSpecifiedPageRange", "kRangeSelection", "kRangeFocusFrame"};
static PRUint32     gDumpFileNameCnt   = 0;
static PRUint32     gDumpLOFileNameCnt = 0;

#define PRINT_DEBUG_MSG1(_msg1) fprintf(mPrt->mDebugFD, (_msg1)); 
#define PRINT_DEBUG_MSG2(_msg1, _msg2) fprintf(mPrt->mDebugFD, (_msg1), (_msg2)); 
#define PRINT_DEBUG_MSG3(_msg1, _msg2, _msg3) fprintf(mPrt->mDebugFD, (_msg1), (_msg2), (_msg3)); 
#define PRINT_DEBUG_MSG4(_msg1, _msg2, _msg3, _msg4) fprintf(mPrt->mDebugFD, (_msg1), (_msg2), (_msg3), (_msg4)); 
#define PRINT_DEBUG_MSG5(_msg1, _msg2, _msg3, _msg4, _msg5) fprintf(mPrt->mDebugFD, (_msg1), (_msg2), (_msg3), (_msg4), (_msg5)); 
#define PRINT_DEBUG_FLUSH fflush(mPrt->mDebugFD);
#else //--------------
#define PRT_YESNO(_p) 
#define PRINT_DEBUG_MSG1(_msg) 
#define PRINT_DEBUG_MSG2(_msg1, _msg2) 
#define PRINT_DEBUG_MSG3(_msg1, _msg2, _msg3) 
#define PRINT_DEBUG_MSG4(_msg1, _msg2, _msg3, _msg4) 
#define PRINT_DEBUG_MSG5(_msg1, _msg2, _msg3, _msg4, _msg5) 
#define PRINT_DEBUG_FLUSH 

#endif

enum PrintObjectType  {eDoc = 0, eFrame = 1, eIFrame = 2, eFrameSet = 3};

class DocumentViewerImpl;
class nsPagePrintTimer;

// New PrintPreview
static NS_DEFINE_CID(kPrintPreviewContextCID,  NS_PRINT_PREVIEW_CONTEXT_CID);

// a small delegate class used to avoid circular references

#ifdef XP_MAC
#pragma mark ** nsDocViewerSelectionListener **
#endif

class nsDocViewerSelectionListener : public nsISelectionListener
{
public:

  // nsISupports interface...
  NS_DECL_ISUPPORTS

  // nsISelectionListerner interface
  NS_DECL_NSISELECTIONLISTENER  

                       nsDocViewerSelectionListener()
                       : mDocViewer(NULL)
                       , mGotSelectionState(PR_FALSE)
                       , mSelectionWasCollapsed(PR_FALSE)
                       {
                         NS_INIT_REFCNT();
                       }
                       
  virtual              ~nsDocViewerSelectionListener() {}
   
  nsresult             Init(DocumentViewerImpl *aDocViewer);

protected:

  DocumentViewerImpl*  mDocViewer;
  PRPackedBool         mGotSelectionState;
  PRPackedBool         mSelectionWasCollapsed;
  
};


/** editor Implementation of the FocusListener interface
 */
class nsDocViewerFocusListener : public nsIDOMFocusListener 
{
public:
  /** default constructor
   */
  nsDocViewerFocusListener();
  /** default destructor
   */
  virtual ~nsDocViewerFocusListener();


/*interfaces for addref and release and queryinterface*/
  NS_DECL_ISUPPORTS

/*BEGIN implementations of focus event handler interface*/
  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent);
  NS_IMETHOD Focus(nsIDOMEvent* aEvent);
  NS_IMETHOD Blur(nsIDOMEvent* aEvent);
/*END implementations of focus event handler interface*/
  nsresult             Init(DocumentViewerImpl *aDocViewer);

private:
    DocumentViewerImpl*  mDocViewer;
};



#ifdef XP_MAC
#pragma mark ** DocumentViewerImpl **
#endif

//---------------------------------------------------
//-- PrintObject Class
//---------------------------------------------------
struct PrintObject
{

public:
  PrintObject();
  ~PrintObject(); // non-virtual

  // Methods
  PRBool IsPrintable()  { return !mDontPrint; }

  nsCOMPtr<nsIWebShell>    mWebShell;
  PrintObjectType mFrameType;
  nsCOMPtr<nsIPresContext> mPresContext;
  nsCOMPtr<nsIStyleSet>    mStyleSet;
  nsCOMPtr<nsIPresShell>   mPresShell;
  nsCOMPtr<nsIViewManager> mViewManager;
  nsCOMPtr<nsIWidget>      mWindow;
  nsIView         *mView;
  nsIView         *mRootView;

  nsIContent      *mContent;
  nsIFrame        *mSeqFrame;
  nsIFrame        *mPageFrame;
  PRInt32          mPageNum;
  nsRect           mRect;
  nsRect           mReflowRect;

  nsVoidArray      mKids;
  PrintObject*     mParent;
  PRPackedBool     mHasBeenPrinted;
  PRPackedBool     mDontPrint;
  PRPackedBool     mPrintAsIs;
  PRPackedBool     mSkippedPageEject;
  PRPackedBool     mSharedPresShell;

  nsRect           mClipRect;

  PRUint16         mImgAnimationMode;

private:
  PrintObject& operator=(const PrintObject& aOther); // not implemented

};

//---------------------------------------------------
//-- PrintData Class
//---------------------------------------------------
class PrintData {
public:

  PrintData();
  ~PrintData(); // non-virtual

  // Listener Helper Methods
  void OnEndPrinting(nsresult aResult);
  void OnStartPrinting();
  static void DoOnProgressChange(nsVoidArray& aListeners, 
                                 PRInt32      aProgess, 
                                 PRInt32      aMaxProgress,
                                 PRBool       aDoStartStop = PR_FALSE,
                                 PRInt32      aFlag = 0);

  nsCOMPtr<nsIDeviceContext>   mPrintDC;
  nsIView                     *mPrintView;
  FILE                        *mDebugFilePtr;    // a file where information can go to when printing

  PrintObject *                mPrintObject;
  PrintObject *                mSelectedPO;

  nsVoidArray                      mPrintProgressListeners;
  nsCOMPtr<nsIWebProgressListener> mPrintProgressListener;
  nsCOMPtr<nsIPrintProgress>       mPrintProgress;
  nsCOMPtr<nsIPrintProgressParams> mPrintProgressParams;
  PRBool                           mShowProgressDialog;
  
  nsCOMPtr<nsIDOMWindowInternal> mCurrentFocusWin; // cache a pointer to the currently focused window

  nsVoidArray*                mPrintDocList;
  nsCOMPtr<nsIDeviceContext>  mPrintDocDC;
  nsCOMPtr<nsIDOMWindow>      mPrintDocDW;
  PRPackedBool                mIsIFrameSelected;
  PRPackedBool                mIsParentAFrameSet;
  PRPackedBool                mPrintingAsIsSubDoc;
  PRInt16                     mPrintFrameType;
  PRPackedBool                mOnStartSent;
  PRInt32                     mNumPrintableDocs;
  PRInt32                     mNumDocsPrinted;
  PRInt32                     mNumPrintablePages;
  PRInt32                     mNumPagesPrinted;

  nsCOMPtr<nsIPrintSettings>  mPrintSettings;

#ifdef DEBUG_PRINTING
  FILE *           mDebugFD;
#endif

private:
  PrintData& operator=(const PrintData& aOther); // not implemented

};

//-------------------------------------------------------------
class DocumentViewerImpl : public nsIDocumentViewer,
                           public nsIContentViewerEdit,
                           public nsIContentViewerFile,
                           public nsIMarkupDocumentViewer,
                           public nsIEventProcessor
{
  friend class nsDocViewerSelectionListener;
  friend class nsPagePrintTimer;
  friend class PrintData;
  
public:
  DocumentViewerImpl();
  DocumentViewerImpl(nsIPresContext* aPresContext);
  
  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  // nsISupports interface...
  NS_DECL_ISUPPORTS

  // nsIContentViewer interface...
  NS_DECL_NSICONTENTVIEWER

  // nsIDocumentViewer interface...
  NS_IMETHOD SetUAStyleSheet(nsIStyleSheet* aUAStyleSheet);
  NS_IMETHOD GetDocument(nsIDocument*& aResult);
  NS_IMETHOD GetPresShell(nsIPresShell*& aResult);
  NS_IMETHOD GetPresContext(nsIPresContext*& aResult);
  NS_IMETHOD CreateDocumentViewerUsing(nsIPresContext* aPresContext,
                                       nsIDocumentViewer*& aResult);
  NS_IMETHOD SetTransformMediator(nsITransformMediator* aMediator);

  // nsIContentViewerEdit
  NS_DECL_NSICONTENTVIEWEREDIT

  // nsIContentViewerFile
  NS_DECL_NSICONTENTVIEWERFILE

  // nsIMarkupDocumentViewer
  NS_DECL_NSIMARKUPDOCUMENTVIEWER

  // nsIEventProcessor
  NS_DECL_NSIEVENTPROCESSOR

  typedef void (*CallChildFunc)(nsIMarkupDocumentViewer* aViewer,
                                void* aClosure);
  nsresult CallChildren(CallChildFunc aFunc, void* aClosure);

  // Printing Methods
  PRBool   PrintPage(nsIPresContext* aPresContext,nsIPrintSettings* aPrintSettings,PrintObject* aPOect);
  PRBool   DonePrintingPages(PrintObject* aPO);
  
  // helper method
  static void GetWebShellTitleAndURL(nsIWebShell * aWebShell, PRUnichar** aTitle, PRUnichar** aURLStr);

protected:
  virtual ~DocumentViewerImpl();

private:
  void ForceRefresh(void);
  nsresult CreateStyleSet(nsIDocument* aDocument, nsIStyleSet** aStyleSet);
  nsresult MakeWindow(nsIWidget* aParentWidget,
                      const nsRect& aBounds);

  nsresult GetDocumentSelection(nsISelection **aSelection, nsIPresShell * aPresShell = nsnull);
  nsresult FindFrameSetWithIID(nsIContent * aParentContent, const nsIID& aIID);
  PRBool   IsThereARangeSelection(nsIDOMWindowInternal * aDOMWin);
  PRBool   IsParentAFrameSet(nsIWebShell * aParent);
  PRBool   IsWebShellAFrameSet(nsIWebShell * aParent);
  
  PRBool   IsThereAnIFrameSelected(nsIWebShell* aWebShell,
                                   nsIDOMWindowInternal * aDOMWin,
                                   PRPackedBool& aDoesContainFrameset);
  PRBool   IsWindowsInOurSubTree(nsIDOMWindowInternal * aDOMWindow);


  nsresult GetPopupNode(nsIDOMNode** aNode);
  nsresult GetPopupLinkNode(nsIDOMNode** aNode);
  nsresult GetPopupImageNode(nsIDOMNode** aNode);

  //---------------------------------------------------------------------
  void BuildDocTree(nsIDocShellTreeNode * aParentNode, 
                    nsVoidArray *         aDocList, 
                    PrintObject *         aPO);
  nsresult ReflowDocList(PrintObject * aPO);
  void SetClipRect(PrintObject*  aPO, 
                   const nsRect& aClipRect,
                   nscoord       aOffsetX,
                   nscoord       aOffsetY,
                   PRBool        aDoingSetClip);

  nsresult ReflowPrintObject(PrintObject * aPO);
  void CalcPageFrameLocation(nsIPresShell * aPresShell,
                                  PrintObject*   aPO);
  PrintObject * FindPrintObjectByWS(PrintObject* aPO, nsIWebShell * aWebShell);
  void MapContentForPO(PrintObject*   aRootObject,
                       nsIPresShell*  aPresShell,
                       nsIContent*    aContent);
  void MapContentToWebShells(PrintObject* aRootPO, PrintObject* aPO);
  void MapSubDocFrameLocations(PrintObject* aPO);
  PrintObject* FindPrintObjectByDOMWin(PrintObject* aParentObject, nsIDOMWindowInternal * aDOMWin);
  void GetPresShellAndRootContent(nsIWebShell * aWebShell, nsIPresShell** aPresShell, nsIContent** aContent);

  void CalcNumPrintableDocsAndPages(PRInt32& aNumDocs, PRInt32& aNumPages);
  void DoProgressForAsIsFrames();
  void DoProgressForSeparateFrames();
  void DoPrintProgress(PRBool aIsForPrinting);
  void SetDocAndURLIntoProgress(nsIWebShell* aWebShell, nsIPrintProgressParams* aParams);
  nsresult CheckForPrinters(nsIPrintOptions*  aPrintOptions,
                            nsIPrintSettings* aPrintSettings,
                            PRUint32          aErrorCode,
                            PRBool            aIsPrinting);

  // get the currently infocus frame for the document viewer
  nsIDOMWindowInternal * FindFocusedDOMWindowInternal();

  // get the DOMWindow for a given WebShell
  nsIDOMWindowInternal * GetDOMWinForWebShell(nsIWebShell* aWebShell);

  //
  // The following three methods are used for printing...
  //
  nsresult DocumentReadyForPrinting();
  //nsresult PrintSelection(nsIDeviceContextSpec * aDevSpec);
  nsresult GetSelectionDocument(nsIDeviceContextSpec * aDevSpec, nsIDocument ** aNewDoc);

  static void PR_CALLBACK HandlePLEvent(PLEvent* aEvent);
  static void PR_CALLBACK DestroyPLEvent(PLEvent* aEvent);

  nsresult SetupToPrintContent(nsIWebShell*          aParent,
                               nsIDeviceContext*     aDContext,
                               nsIDOMWindowInternal* aCurrentFocusedDOMWin);
  nsresult EnablePOsForPrinting();

  PRBool   PrintDocContent(PrintObject* aPO, nsresult& aStatus);
  nsresult DoPrint(PrintObject * aPO, PRBool aDoSyncPrinting, PRBool& aDonePrinting);
  void SetPrintAsIs(PrintObject* aPO, PRBool aAsIs = PR_TRUE);
  void SetPrintPO(PrintObject* aPO, PRBool aPrint);

#ifdef NS_PRINT_PREVIEW
  nsresult RemoveEventProcessorFromVMs(PrintObject* aPO);
  nsresult ShowDocList(PrintObject* aPO, PRBool aShow);
  void InstallNewPresentation();
  void ReturnToGalleyPresentation();
  void TurnScriptingOn(PRBool aDoTurnOn);
#endif


  // Timer Methods
  nsresult StartPagePrintTimer(nsIPresContext * aPresContext, 
                               nsIPrintSettings* aPrintSettings,
                               PrintObject*     aPO,
                               PRUint32         aDelay);

  void PrepareToStartLoad(void);
  
  // Misc
  void ShowPrintErrorDialog(nsresult printerror, PRBool aIsPrinting = PR_TRUE);

protected:
  // IMPORTANT: The ownership implicit in the following member
  // variables has been explicitly checked and set using nsCOMPtr
  // for owning pointers and raw COM interface pointers for weak
  // (ie, non owning) references. If you add any members to this
  // class, please make the ownership explicit (pinkerton, scc).
  
  nsISupports* mContainer; // [WEAK] it owns me!
  nsCOMPtr<nsIDeviceContext> mDeviceContext;   // ??? can't hurt, but...
  nsIView*                 mView;        // [WEAK] cleaned up by view mgr

  // the following seven items are explicitly in this order
  // so they will be destroyed in the reverse order (pinkerton, scc)
  nsCOMPtr<nsITransformMediator> mTransformMediator;
  nsCOMPtr<nsIDocument>    mDocument;
  nsCOMPtr<nsIWidget>      mWindow;      // ??? should we really own it?
  nsCOMPtr<nsIViewManager> mViewManager;
  nsCOMPtr<nsIPresContext> mPresContext;
  nsCOMPtr<nsIPresShell>   mPresShell;

  nsCOMPtr<nsIStyleSheet>  mUAStyleSheet;

  nsCOMPtr<nsISelectionListener> mSelectionListener;
  nsCOMPtr<nsIDOMFocusListener> mFocusListener;
  
  nsCOMPtr<nsIContentViewer> mPreviousViewer;

  PRBool  mEnableRendering;
  PRBool  mStopped;
  PRBool  mLoaded;
  PRInt16 mNumURLStarts;
  PRInt16 mDestroyRefCount;     // a second "refcount" for the document viewer's "destroy"
  nsIPageSequenceFrame* mPageSeqFrame;


  PRBool            mIsPrinting;
  PrintData*        mPrt;
  nsPagePrintTimer* mPagePrintTimer;

#ifdef NS_PRINT_PREVIEW
  PRBool            mIsDoingPrintPreview; // per DocumentViewer
  nsIWidget*        mParentWidget; // purposely won't be ref counted
  PrintData*        mPrtPreview;
#endif

#ifdef NS_DEBUG
  FILE* mDebugFile;
#endif

  // static memeber variables
  static PRBool mIsCreatingPrintPreview;
  static PRBool mIsDoingPrinting;

  // document management data
  //   these items are specific to markup documents (html and xml)
  //   may consider splitting these out into a subclass
  PRBool   mAllowPlugins;
  /* character set member data */
  nsString mDefaultCharacterSet;
  nsString mHintCharset;
  PRInt32 mHintCharsetSource;
  nsString mForceCharacterSet;
};

//---------------------------------------------------
//-- Page Timer Class
//---------------------------------------------------
class nsPagePrintTimer : public nsITimerCallback
{
public:

  NS_DECL_ISUPPORTS

  nsPagePrintTimer()
      : mDocViewer(nsnull), mPresContext(nsnull), mPrintSettings(nsnull), mDelay(0)
  {
    NS_INIT_ISUPPORTS();
  }

  virtual ~nsPagePrintTimer()
  {
    if (mTimer) {
      mTimer->Cancel();
    }
    DocumentViewerImpl::mIsDoingPrinting = PR_FALSE;
    mDocViewer->Destroy();
    NS_RELEASE(mDocViewer);
  }


  nsresult StartTimer()
  {
    nsresult result;
    mTimer = do_CreateInstance("@mozilla.org/timer;1", &result);
    if (NS_FAILED(result)) {
      NS_WARNING("unable to start the timer");
    } else {
      mTimer->Init(this, mDelay, NS_PRIORITY_NORMAL, NS_TYPE_ONE_SHOT);
    } 
    return result;
  }



  // nsITimerCallback
  NS_IMETHOD_(void) Notify(nsITimer *timer)
  {
    if (mPresContext && mDocViewer) {
      PRPackedBool initNewTimer = PR_TRUE;
      // Check to see if we are done
      // donePrinting will be true if it completed successfully or
      // if the printing was cancelled
      PRBool donePrinting = mDocViewer->PrintPage(mPresContext, mPrintSettings, mPrintObj);
      if (donePrinting) {
        // now clean up print or print the next webshell
        if (mDocViewer->DonePrintingPages(mPrintObj)) {
          initNewTimer = PR_FALSE;
        }
      } 

      Stop();
      if (initNewTimer) {
        nsresult result = StartTimer();
        if (NS_FAILED(result)) {
          donePrinting = PR_TRUE;     // had a failure.. we are finished..
          DocumentViewerImpl::mIsDoingPrinting = PR_FALSE;
        }
      }
    }
  }

  void Init(DocumentViewerImpl* aDocViewerImpl, 
            nsIPresContext*     aPresContext,
            nsIPrintSettings*   aPrintSettings,
            PrintObject*        aPO,
            PRUint32            aDelay) 
  {
    NS_IF_RELEASE(mDocViewer);
    mDocViewer       = aDocViewerImpl;
    NS_ADDREF(mDocViewer);    

    mPresContext     = aPresContext;
    mPrintSettings   = aPrintSettings;
    mPrintObj        = aPO;
    mDelay           = aDelay;
  }

  nsresult Start(DocumentViewerImpl* aDocViewerImpl, 
                 nsIPresContext*     aPresContext,
                 nsIPrintSettings*   aPrintSettings,
                 PrintObject*        aPO,
                 PRUint32            aDelay) 
  {
    Init(aDocViewerImpl, aPresContext, aPrintSettings, aPO, aDelay);
    return StartTimer();
  }


  void Stop()
  {
    if (mTimer) {
      mTimer->Cancel();
      mTimer = nsnull;
    }
  }

private:
  DocumentViewerImpl*        mDocViewer;
  nsIPresContext*            mPresContext;
  nsCOMPtr<nsIPrintSettings> mPrintSettings;
  nsCOMPtr<nsITimer>         mTimer;
  PRUint32                   mDelay;
  PrintObject *              mPrintObj;
};

NS_IMPL_ISUPPORTS1(nsPagePrintTimer, nsITimerCallback)

static nsresult NS_NewUpdateTimer(nsPagePrintTimer **aResult)
{

  NS_PRECONDITION(aResult, "null param");

  nsPagePrintTimer* result = new nsPagePrintTimer;

  if (!result) {
    *aResult = nsnull;
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(result);
  *aResult = result;

  return NS_OK;
}

//---------------------------------------------------
//-- PrintData Class Impl
//---------------------------------------------------
PrintData::PrintData() :
  mPrintView(nsnull), mDebugFilePtr(nsnull), mPrintObject(nsnull), mSelectedPO(nsnull),
  mShowProgressDialog(PR_TRUE), mPrintDocList(nsnull), mIsIFrameSelected(PR_FALSE),
  mIsParentAFrameSet(PR_FALSE), mPrintingAsIsSubDoc(PR_FALSE),
  mPrintFrameType(nsIPrintSettings::kFramesAsIs), mOnStartSent(PR_FALSE),
  mNumPrintableDocs(0), mNumDocsPrinted(0), mNumPrintablePages(0), mNumPagesPrinted(0)
{
#ifdef DEBUG_PRINTING
  mDebugFD = fopen("printing.log", "w");
#endif
}

PrintData::~PrintData() 
{

  // printing is complete, clean up now

  OnEndPrinting(NS_OK); // removes listener

  if (mPrintDC) {
#ifdef DEBUG_PRINTING
    fprintf(mDebugFD, "****************** End Document ************************\n");
#endif
    mPrintDC->EndDocument();
  }

  delete mPrintObject;

  if (mPrintDocList != nsnull) {
    mPrintDocList->Clear();
    delete mPrintDocList;
  }

#ifdef DEBUG_PRINTING
  fclose(mDebugFD);
#endif

  DocumentViewerImpl::mIsDoingPrinting = PR_FALSE;

  for (PRInt32 i=0;i<mPrintProgressListeners.Count();i++) {
    nsIWebProgressListener* wpl = NS_STATIC_CAST(nsIWebProgressListener*, mPrintProgressListeners.ElementAt(i));
    NS_ASSERTION(wpl, "nsIWebProgressListener is NULL!");
    NS_RELEASE(wpl);
  }

}

void PrintData::OnStartPrinting()
{
  if (!mOnStartSent) {
    DoOnProgressChange(mPrintProgressListeners, 100, 100, PR_TRUE, nsIWebProgressListener::STATE_START|nsIWebProgressListener::STATE_IS_DOCUMENT);
    mOnStartSent = PR_TRUE;
  }
}

void PrintData::OnEndPrinting(nsresult aResult)
{
  // Make sure OnStartPrinting was called
  OnStartPrinting();

  DoOnProgressChange(mPrintProgressListeners, 100, 100, PR_TRUE, nsIWebProgressListener::STATE_STOP|nsIWebProgressListener::STATE_IS_DOCUMENT);
  if (mPrintProgress && mShowProgressDialog) {
    mPrintProgress->CloseProgressDialog(PR_TRUE);
  }
}

void
PrintData::DoOnProgressChange(nsVoidArray& aListeners, 
                              PRInt32      aProgess, 
                              PRInt32      aMaxProgress,
                              PRBool       aDoStartStop,
                              PRInt32      aFlag)
{
  for (PRInt32 i=0;i<aListeners.Count();i++) {
    nsIWebProgressListener* wpl = NS_STATIC_CAST(nsIWebProgressListener*, aListeners.ElementAt(i));
    NS_ASSERTION(wpl, "nsIWebProgressListener is NULL!");
    wpl->OnProgressChange(nsnull, nsnull, aProgess, aMaxProgress, aProgess, aMaxProgress); 
    if (aDoStartStop) {
      wpl->OnStateChange(nsnull, nsnull, aFlag, 0);
    }
  }
}


//---------------------------------------------------
//-- PrintObject Class Impl
//---------------------------------------------------
PrintObject::PrintObject() : 
  mWebShell(nsnull), mFrameType(eFrame),
  mView(nsnull), mRootView(nsnull), mContent(nsnull),
  mSeqFrame(nsnull), mPageFrame(nsnull), mPageNum(-1), 
  mRect(0,0,0,0), mReflowRect(0,0,0,0),
  mParent(nsnull), mHasBeenPrinted(PR_FALSE), mDontPrint(PR_TRUE),
  mPrintAsIs(PR_FALSE), mSkippedPageEject(PR_FALSE), mSharedPresShell(PR_FALSE),
  mClipRect(-1,-1, -1, -1),
  mImgAnimationMode(imgIContainer::kNormalAnimMode)
{
}

PrintObject::~PrintObject()
{
  if (mPresContext) {
    mPresContext->SetImageAnimationMode(mImgAnimationMode);
  }

  for (PRInt32 i=0;i<mKids.Count();i++) {
    PrintObject* po = (PrintObject*)mKids[i];
    NS_ASSERTION(po, "PrintObject can't be null!");
    delete po;
  } 
  if (mPresShell && !mSharedPresShell)
    mPresShell->Destroy();
}

//------------------------------------------------------------------
// DocumentViewerImpl
//------------------------------------------------------------------
// Class IDs
static NS_DEFINE_CID(kViewManagerCID,       NS_VIEW_MANAGER_CID);
static NS_DEFINE_CID(kScrollingViewCID,     NS_SCROLLING_VIEW_CID);
static NS_DEFINE_CID(kWidgetCID,            NS_CHILD_CID);
static NS_DEFINE_CID(kViewCID,              NS_VIEW_CID);

// Data members
PRBool DocumentViewerImpl::mIsCreatingPrintPreview = PR_FALSE;
PRBool DocumentViewerImpl::mIsDoingPrinting        = PR_FALSE;

nsresult
NS_NewDocumentViewer(nsIDocumentViewer** aResult)
{
  NS_PRECONDITION(aResult, "null OUT ptr");
  if (!aResult) {
    return NS_ERROR_NULL_POINTER;
  }
  DocumentViewerImpl* it = new DocumentViewerImpl();
  if (nsnull == it) {
    *aResult = nsnull;
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return CallQueryInterface(it, aResult);
}

// Note: operator new zeros our memory
DocumentViewerImpl::DocumentViewerImpl()
{
  NS_INIT_ISUPPORTS();
  PrepareToStartLoad();

#ifdef NS_PRINT_PREVIEW
  mParentWidget = nsnull;
#endif
}

void DocumentViewerImpl::PrepareToStartLoad() {
  mEnableRendering  = PR_TRUE;
  mStopped          = PR_FALSE;
  mLoaded           = PR_FALSE;
  mPrt              = nsnull;
  mIsPrinting       = PR_FALSE;

#ifdef NS_PRINT_PREVIEW
  mIsDoingPrintPreview = PR_FALSE;
  mPrtPreview          = nsnull;
#endif

#ifdef NS_DEBUG
  mDebugFile = nsnull;
#endif
}

DocumentViewerImpl::DocumentViewerImpl(nsIPresContext* aPresContext)
  : mPresContext(aPresContext)
{
  NS_INIT_ISUPPORTS();
  mHintCharsetSource = kCharsetUninitialized;
  mAllowPlugins      = PR_TRUE;
  PrepareToStartLoad();
}

NS_IMPL_ISUPPORTS6(DocumentViewerImpl,
                   nsIContentViewer,
                   nsIDocumentViewer,
                   nsIMarkupDocumentViewer,
                   nsIContentViewerFile,
                   nsIContentViewerEdit,
                   nsIEventProcessor)

DocumentViewerImpl::~DocumentViewerImpl()
{
  NS_ASSERTION(!mDocument, "User did not call nsIContentViewer::Close");
  if (mDocument)
    Close();

  NS_ASSERTION(!mPresShell, "User did not call nsIContentViewer::Destroy");
  if (mPresShell)
    Destroy();

  // XXX(?) Revoke pending invalidate events

  // clear weak references before we go away
  if (mPresContext) {
    mPresContext->SetContainer(nsnull);
    mPresContext->SetLinkHandler(nsnull);
  }
}

/*
 * This method is called by the Document Loader once a document has
 * been created for a particular data stream...  The content viewer
 * must cache this document for later use when Init(...) is called.
 *
 * This method is also called when an out of band document.write() happens.
 * In that case, the document passed in is the same as the previous document.
 */
NS_IMETHODIMP
DocumentViewerImpl::LoadStart(nsISupports *aDoc)
{
#ifdef NOISY_VIEWER
  printf("DocumentViewerImpl::LoadStart\n");
#endif

  nsresult rv;
  if (!mDocument) {
    mDocument = do_QueryInterface(aDoc,&rv);
  }
  else if (mDocument == aDoc) {
    // Reset the document viewer's state back to what it was 
    // when the document load started.
    PrepareToStartLoad();
  }

  return rv;
}

NS_IMETHODIMP
DocumentViewerImpl::SetContainer(nsISupports* aContainer)
{
  mContainer = aContainer;
  if (mPresContext) {
    mPresContext->SetContainer(aContainer);
  }
  return NS_OK;
}

NS_IMETHODIMP
DocumentViewerImpl::GetContainer(nsISupports** aResult)
{
   NS_ENSURE_ARG_POINTER(aResult);

   *aResult = mContainer;
   NS_IF_ADDREF(*aResult);

   return NS_OK;
}

NS_IMETHODIMP
DocumentViewerImpl::Init(nsIWidget* aParentWidget,
                         nsIDeviceContext* aDeviceContext,
                         const nsRect& aBounds)
{
#ifdef NS_PRINT_PREVIEW
  mParentWidget = aParentWidget; // not ref counted
#endif

  nsresult rv;
  NS_ENSURE_TRUE(mDocument, NS_ERROR_NULL_POINTER);

  mDeviceContext = dont_QueryInterface(aDeviceContext);
#ifdef NS_PRINT_PREVIEW
  // Clear PrintPreview Alternate Device
  if (mDeviceContext) {
    mDeviceContext->SetAltDevice(nsnull);
    mDeviceContext->SetCanonicalPixelScale(1.0);
  }
#endif

  PRBool makeCX = PR_FALSE;
  if (!mPresContext) {
    // Create presentation context
    if (mIsCreatingPrintPreview) {
      mPresContext = do_CreateInstance(kPrintPreviewContextCID,&rv);
    } else {
      mPresContext = do_CreateInstance(kGalleyContextCID,&rv);
    }
    if (NS_FAILED(rv)) return rv;

    mPresContext->Init(aDeviceContext); 
#ifdef NS_PRINT_PREVIEW
    makeCX = !mIsDoingPrintPreview; // needs to be true except when we are already in PP
#else
    makeCX = PR_TRUE;
#endif
  }

  nsCOMPtr<nsIInterfaceRequestor> requestor(do_QueryInterface(mContainer));
  if (requestor) {
    nsCOMPtr<nsILinkHandler> linkHandler;
    requestor->GetInterface(NS_GET_IID(nsILinkHandler), 
       getter_AddRefs(linkHandler));
    mPresContext->SetContainer(mContainer);
    mPresContext->SetLinkHandler(linkHandler);

    if (!mIsDoingPrintPreview) {
      // Set script-context-owner in the document
      nsCOMPtr<nsIScriptGlobalObjectOwner> owner;
      requestor->GetInterface(NS_GET_IID(nsIScriptGlobalObjectOwner),
                              getter_AddRefs(owner));

      if (owner) {
        nsCOMPtr<nsIScriptGlobalObject> global;
        owner->GetScriptGlobalObject(getter_AddRefs(global));

        if (global) {
          mDocument->SetScriptGlobalObject(global);
          nsCOMPtr<nsIDOMDocument> domdoc(do_QueryInterface(mDocument));

          if (domdoc) {
            global->SetNewDocument(domdoc, PR_TRUE);
          }
        }
      }
    }
  }

  // Create the ViewManager and Root View...
  rv = MakeWindow(aParentWidget, aBounds);
  Hide();

  if (NS_FAILED(rv)) return rv;

  // Create the style set...
  nsIStyleSet* styleSet;
  rv = CreateStyleSet(mDocument, &styleSet);
  if (NS_FAILED(rv)) return rv;

    // Now make the shell for the document
    rv = mDocument->CreateShell(mPresContext, mViewManager, styleSet,
                                getter_AddRefs(mPresShell));
    NS_RELEASE(styleSet);
  if (NS_FAILED(rv)) return rv;
  mPresShell->BeginObservingDocument();

      // Initialize our view manager
      nsRect bounds;
      mWindow->GetBounds(bounds);
      nscoord width = bounds.width;
      nscoord height = bounds.height;
      float p2t;
      mPresContext->GetPixelsToTwips(&p2t);
      width = NSIntPixelsToTwips(width, p2t);
      height = NSIntPixelsToTwips(height, p2t);
      mViewManager->DisableRefresh();
      mViewManager->SetWindowDimensions(width, height);

      /* Setup default view manager background color */
      /* This may be overridden by the docshell with the background color for the
         last document loaded into the docshell */
      nscolor bgcolor = NS_RGB(0, 0, 0);
      mPresContext->GetDefaultBackgroundColor(&bgcolor);
      mViewManager->SetDefaultBackgroundColor(bgcolor);

      if (!makeCX) {
        // Make shell an observer for next time
        // XXX - we observe the docuement always, see above after preshell is created
        // mPresShell->BeginObservingDocument();

//XXX I don't think this should be done *here*; and why paint nothing
//(which turns out to cause black flashes!)???
        // Resize-reflow this time
        mPresShell->InitialReflow(width, height);

        // Now trigger a refresh
        if (mEnableRendering) {
          mViewManager->EnableRefresh(NS_VMREFRESH_IMMEDIATE);
        }
      }

  // now register ourselves as a selection listener, so that we get called
  // when the selection changes in the window
  nsDocViewerSelectionListener *selectionListener;
  NS_NEWXPCOM(selectionListener, nsDocViewerSelectionListener);
  if (!selectionListener) return NS_ERROR_OUT_OF_MEMORY;
  selectionListener->Init(this);
  
  // this is the owning reference. The nsCOMPtr will take care of releasing
  // our ref to the listener on destruction.
  NS_ADDREF(selectionListener);
  mSelectionListener = do_QueryInterface(selectionListener);
  NS_RELEASE(selectionListener);
  if (NS_FAILED(rv)) return rv;
  
  nsCOMPtr<nsISelection> selection;
  rv = GetDocumentSelection(getter_AddRefs(selection));
  if (NS_FAILED(rv)) return rv;
  
  nsCOMPtr<nsISelectionPrivate> selPrivate(do_QueryInterface(selection));
  rv = selPrivate->AddSelectionListener(mSelectionListener);
  if (NS_FAILED(rv)) return rv;
  
  //focus listener
  // now register ourselves as a focus listener, so that we get called
  // when the focus changes in the window
  nsDocViewerFocusListener *focusListener;
  NS_NEWXPCOM(focusListener, nsDocViewerFocusListener);
  if (!focusListener) return NS_ERROR_OUT_OF_MEMORY;
  focusListener->Init(this);
  
  // this is the owning reference. The nsCOMPtr will take care of releasing
  // our ref to the listener on destruction.
  NS_ADDREF(focusListener);
  mFocusListener = do_QueryInterface(focusListener);
  NS_RELEASE(focusListener);
  if (NS_FAILED(rv)) 
    return rv;

  if(mDocument)
  {
    // get the DOM event receiver
    nsCOMPtr<nsIDOMEventReceiver> erP (do_QueryInterface(mDocument, &rv));
    if(NS_FAILED(rv)) 
      return rv;
    if(!erP)
      return NS_ERROR_FAILURE;

    rv = erP->AddEventListenerByIID(mFocusListener, NS_GET_IID(nsIDOMFocusListener));
    NS_ASSERTION(NS_SUCCEEDED(rv), "failed to register focus listener");
  }

  return rv;
}

//
// LoadComplete(aStatus)
//
//   aStatus - The status returned from loading the document.
//
// This method is called by the container when the document has been
// completely loaded.
//
NS_IMETHODIMP
DocumentViewerImpl::LoadComplete(nsresult aStatus)
{
  nsresult rv = NS_OK;
  NS_ENSURE_TRUE(mDocument, NS_ERROR_NOT_AVAILABLE);

  nsCOMPtr<nsIScriptGlobalObject> global;

  // First, get the script global object from the document...
  rv = mDocument->GetScriptGlobalObject(getter_AddRefs(global));

  // Fail if no ScriptGlobalObject is available...
  NS_ASSERTION(global, "nsIScriptGlobalObject not set for document!");
  if (!global) return NS_ERROR_NULL_POINTER;

  mLoaded = PR_TRUE;

  /* We need to protect ourself against auto-destruction in case the window is closed
     while processing the OnLoad event.
     See bug http://bugzilla.mozilla.org/show_bug.cgi?id=78445 for more explanation.
  */
  NS_ADDREF_THIS();

  // Now, fire either an OnLoad or OnError event to the document...
  if(NS_SUCCEEDED(aStatus)) {
    nsEventStatus status = nsEventStatus_eIgnore;
    nsEvent event;

    event.eventStructType = NS_EVENT;
    event.message = NS_PAGE_LOAD;
    rv = global->HandleDOMEvent(mPresContext, &event, nsnull,
                                NS_EVENT_FLAG_INIT, &status);
#ifdef MOZ_TIMELINE
    // if navigator.xul's load is complete, the main nav window is visible
    // mark that point.
    if (mDocument) {
      //printf("DEBUG: getting uri from document (%p)\n", mDocument.get());
      nsCOMPtr<nsIURI> uri;
      mDocument->GetDocumentURL(getter_AddRefs(uri));
      if (uri.get() != nsnull) {
        //printf("DEBUG: getting spec fro uri (%p)\n", uri.get());
        char *spec = nsnull;
        uri->GetSpec(&spec);
        if (spec && !strcmp(spec, "chrome://navigator/content/navigator.xul")) {
          NS_TIMELINE_MARK("Navigator Window visible now");
        }
        CRTFREEIF(spec);
      }
    }
#endif /* MOZ_TIMELINE */
  } else {
    // XXX: Should fire error event to the document...
  }
  
  // Now that the document has loaded, we can tell the presshell
  // to unsuppress painting.
  if (mPresShell && !mStopped)
    mPresShell->UnsuppressPainting();

  NS_RELEASE_THIS();

  return rv;
}

NS_IMETHODIMP
DocumentViewerImpl::Unload()
{
  nsresult rv;

  if (!mDocument) {
    return NS_ERROR_NULL_POINTER;
  }

  // First, get the script global object from the document...
  nsCOMPtr<nsIScriptGlobalObject> global;

  rv = mDocument->GetScriptGlobalObject(getter_AddRefs(global));
  if (!global) {
    // Fail if no ScriptGlobalObject is available...
    NS_ASSERTION(0, "nsIScriptGlobalObject not set for document!");
    return NS_ERROR_NULL_POINTER;
  }

  // Now, fire an Unload event to the document...
  nsEventStatus status = nsEventStatus_eIgnore;
  nsEvent event;

  event.eventStructType = NS_EVENT;
  event.message = NS_PAGE_UNLOAD;
  rv = global->HandleDOMEvent(mPresContext, &event, nsnull,
                              NS_EVENT_FLAG_INIT, &status);

  return rv;
}

NS_IMETHODIMP
DocumentViewerImpl::Close()
{
  // All callers are supposed to call close to break circular
  // references.  If we do this stuff in the destructor, the
  // destructor might never be called (especially if we're being
  // used from JS.

  // Close is also needed to disable scripts during paint suppression,
  // since we transfer the existing global object to the new document
  // that is loaded.  In the future, the global object may become a proxy
  // for an object that can be switched in and out so that we don't need
  // to disable scripts during paint suppression.

  nsresult rv;

  if (mDocument) {
#ifdef NS_PRINT_PREVIEW
    // Turn scripting back on 
    // after PrintPreview had turned it off
    if (mPrtPreview) {
      TurnScriptingOn(PR_TRUE);
    }
#endif

    // Break global object circular reference on the document created
    // in the DocViewer Init
    nsCOMPtr<nsIScriptGlobalObject> globalObject;
    mDocument->GetScriptGlobalObject(getter_AddRefs(globalObject));
    if (globalObject) {
      globalObject->SetNewDocument(nsnull, PR_TRUE);
    }
    // out of band cleanup of webshell
    mDocument->SetScriptGlobalObject(nsnull);
    if (mFocusListener) {
      // get the DOM event receiver
      nsCOMPtr<nsIDOMEventReceiver> erP( do_QueryInterface(mDocument, &rv) );
      if(NS_SUCCEEDED(rv) && erP)
        erP->RemoveEventListenerByIID(mFocusListener, NS_GET_IID(nsIDOMFocusListener));
    }
 }

  mDocument = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
DocumentViewerImpl::Destroy()
{
  if (mDestroyRefCount != 0) {
    --mDestroyRefCount;
    return NS_OK;
  }

  // All callers are supposed to call destroy to break circular
  // references.  If we do this stuff in the destructor, the
  // destructor might never be called (especially if we're being
  // used from JS.

  if (mPrt) {
    mPrt->OnEndPrinting(NS_ERROR_FAILURE);
    delete mPrt;
    mPrt = nsnull;
  }

#ifdef NS_PRINT_PREVIEW
  if (mPrtPreview) {
    RemoveEventProcessorFromVMs(mPrtPreview->mPrintObject);
    delete mPrtPreview;
    mPrtPreview = nsnull;
  }
#endif

  // Avoid leaking the old viewer.
  if (mPreviousViewer) {
    mPreviousViewer->Destroy();
    mPreviousViewer = nsnull;
  }

  if (mDeviceContext)
    mDeviceContext->FlushFontCache();

  if (mPresShell) {
    // Break circular reference (or something)
    mPresShell->EndObservingDocument();
    nsCOMPtr<nsISelection> selection;
    nsresult rv = GetDocumentSelection(getter_AddRefs(selection));
    nsCOMPtr<nsISelectionPrivate> selPrivate(do_QueryInterface(selection));
    if (NS_SUCCEEDED(rv) && selPrivate && mSelectionListener) 
      selPrivate->RemoveSelectionListener(mSelectionListener);
    mPresShell->Destroy();
    mPresShell = nsnull;
  }
  
  return NS_OK;
}

NS_IMETHODIMP
DocumentViewerImpl::Stop(void)
{
  NS_ASSERTION(mDocument, "Stop called too early or too late");
  if (mDocument) {
    mDocument->StopDocumentLoad();
  }

  mStopped = PR_TRUE;

  if (!mLoaded && mPresShell) {
    // Well, we might as well paint what we have so far.
    mPresShell->UnsuppressPainting();
  }

  return NS_OK;
}

NS_IMETHODIMP
DocumentViewerImpl::GetDOMDocument(nsIDOMDocument **aResult)
{
  NS_ENSURE_TRUE(mDocument, NS_ERROR_NOT_AVAILABLE);
  return CallQueryInterface(mDocument.get(), aResult);
}

NS_IMETHODIMP
DocumentViewerImpl::SetDOMDocument(nsIDOMDocument *aDocument)
{
  // Assumptions:
  //
  // 1) this document viewer has been initialized with a call to Init().
  // 2) the stylesheets associated with the document have been added to the document.

  // XXX Right now, this method assumes that the layout of the current document
  // hasn't started yet.  More cleanup will probably be necessary to make this
  // method work for the case when layout *has* occurred for the current document.
  // That work can happen when and if it is needed.
  
  nsresult rv;
  if (nsnull == aDocument)
    return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIDocument> newDoc = do_QueryInterface(aDocument, &rv);
  if (NS_FAILED(rv)) return rv;

  // 0) Replace the old document with the new one
  mDocument = newDoc;

  // 1) Set the script global object on the new document
  nsCOMPtr<nsIInterfaceRequestor> requestor(do_QueryInterface(mContainer));
  if (requestor) {
    nsCOMPtr<nsIScriptGlobalObjectOwner> owner;
    requestor->GetInterface(NS_GET_IID(nsIScriptGlobalObjectOwner),
       getter_AddRefs(owner));
    if (nsnull != owner) {
      nsCOMPtr<nsIScriptGlobalObject> global;
      rv = owner->GetScriptGlobalObject(getter_AddRefs(global));
      if (NS_SUCCEEDED(rv) && (nsnull != global)) {
        mDocument->SetScriptGlobalObject(global);
        global->SetNewDocument(aDocument, PR_TRUE);
      }
    }
  }  

  // 2) Create a new style set for the document
  nsCOMPtr<nsIStyleSet> styleSet;
  rv = CreateStyleSet(mDocument, getter_AddRefs(styleSet));
  if (NS_FAILED(rv)) return rv;

  // 3) Replace the current pres shell with a new shell for the new document 
  mPresShell->EndObservingDocument();
  mPresShell->Destroy();

  mPresShell = nsnull;
  rv = newDoc->CreateShell(mPresContext, mViewManager, styleSet,
                           getter_AddRefs(mPresShell));
  if (NS_FAILED(rv)) return rv;

  mPresShell->BeginObservingDocument();

  // 4) Register the focus listener on the new document
  if(mDocument)
  {    
    nsCOMPtr<nsIDOMEventReceiver> erP = do_QueryInterface(mDocument, &rv);
    if(NS_FAILED(rv) || !erP)
      return rv ? rv : NS_ERROR_FAILURE;

    rv = erP->AddEventListenerByIID(mFocusListener, NS_GET_IID(nsIDOMFocusListener));
    NS_ASSERTION(NS_SUCCEEDED(rv), "failed to register focus listener");
  }

  return rv;
}

NS_IMETHODIMP
DocumentViewerImpl::SetUAStyleSheet(nsIStyleSheet* aUAStyleSheet)
{
  mUAStyleSheet = dont_QueryInterface(aUAStyleSheet);
  return NS_OK;
}
  
NS_IMETHODIMP
DocumentViewerImpl::GetDocument(nsIDocument*& aResult)
{
  aResult = mDocument;
  NS_IF_ADDREF(aResult);
  return NS_OK;
}
  
NS_IMETHODIMP
DocumentViewerImpl::GetPresShell(nsIPresShell*& aResult)
{
  aResult = mPresShell;
  NS_IF_ADDREF(aResult);
  return NS_OK;
}
  
NS_IMETHODIMP
DocumentViewerImpl::GetPresContext(nsIPresContext*& aResult)
{
  aResult = mPresContext;
  NS_IF_ADDREF(aResult);
  return NS_OK;
}

NS_IMETHODIMP
DocumentViewerImpl::GetBounds(nsRect& aResult)
{
  NS_ENSURE_TRUE(mDocument, NS_ERROR_NOT_AVAILABLE);
  NS_PRECONDITION(mWindow, "null window");
  if (mWindow) {
    mWindow->GetBounds(aResult);
  }
  else {
    aResult.SetRect(0, 0, 0, 0);
  }
  return NS_OK;
}

NS_IMETHODIMP
DocumentViewerImpl::GetPreviousViewer(nsIContentViewer** aViewer)
{
  *aViewer = mPreviousViewer;
  NS_IF_ADDREF(*aViewer);
  return NS_OK;
}

NS_IMETHODIMP
DocumentViewerImpl::SetPreviousViewer(nsIContentViewer* aViewer)
{
  // NOTE:  |Show| sets |mPreviousViewer| to null without calling this
  // function.

  if (aViewer) {
    NS_ASSERTION(!mPreviousViewer,
                 "can't set previous viewer when there already is one");

    // In a multiple chaining situation (which occurs when running a thrashing
    // test like i-bench or jrgm's tests with no delay), we can build up a 
    // whole chain of viewers.  In order to avoid this, we always set our previous
    // viewer to the MOST previous viewer in the chain, and then dump the intermediate
    // link from the chain.  This ensures that at most only 2 documents are alive
    // and undestroyed at any given time (the one that is showing and the one that
    // is loading with painting suppressed).
    nsCOMPtr<nsIContentViewer> prevViewer;
    aViewer->GetPreviousViewer(getter_AddRefs(prevViewer));
    if (prevViewer) {
      aViewer->SetPreviousViewer(nsnull);
      aViewer->Destroy();
      return SetPreviousViewer(prevViewer);
    }
  }

  mPreviousViewer = aViewer;
  return NS_OK;
}

NS_IMETHODIMP
DocumentViewerImpl::SetBounds(const nsRect& aBounds)
{
  NS_ENSURE_TRUE(mDocument, NS_ERROR_NOT_AVAILABLE);
  NS_PRECONDITION(mWindow, "null window");
  if (mWindow) {
    // Don't have the widget repaint. Layout will generate repaint requests
    // during reflow
    mWindow->Resize(aBounds.x, aBounds.y, aBounds.width, aBounds.height,
                    PR_FALSE);
  }
  return NS_OK;
}

NS_IMETHODIMP
DocumentViewerImpl::Move(PRInt32 aX, PRInt32 aY)
{
  NS_ENSURE_TRUE(mDocument, NS_ERROR_NOT_AVAILABLE);
  NS_PRECONDITION(mWindow, "null window");
  if (mWindow) {
    mWindow->Move(aX, aY);
  }
  return NS_OK;
}

NS_IMETHODIMP
DocumentViewerImpl::Show(void)
{
  NS_ENSURE_TRUE(mDocument, NS_ERROR_NOT_AVAILABLE);
  NS_PRECONDITION(mWindow, "null window");

  // We don't need the previous viewer anymore since we're not
  // displaying it.
  if (mPreviousViewer) {
    // This little dance *may* only be to keep
    // PresShell::EndObservingDocument happy, but I'm not sure.
    nsCOMPtr<nsIContentViewer> prevViewer(mPreviousViewer);
    mPreviousViewer = nsnull;
    prevViewer->Destroy();
  }

  if (mWindow) {
    mWindow->Show(PR_TRUE);
  }
  return NS_OK;
}

NS_IMETHODIMP
DocumentViewerImpl::Hide(void)
{
  NS_ENSURE_TRUE(mDocument, NS_ERROR_NOT_AVAILABLE);
  NS_PRECONDITION(mWindow, "null window");
  if (mWindow) {
    mWindow->Show(PR_FALSE);
  }
  return NS_OK;
}

nsresult
DocumentViewerImpl::FindFrameSetWithIID(nsIContent * aParentContent, const nsIID& aIID)
{
  PRInt32 numChildren;
  aParentContent->ChildCount(numChildren);

  // do a breadth search across all siblings
  PRInt32 inx;
  for (inx=0;inx<numChildren;inx++) {
    nsCOMPtr<nsIContent> child;
    if (NS_SUCCEEDED(aParentContent->ChildAt(inx, *getter_AddRefs(child))) && child) {
      nsCOMPtr<nsISupports> temp;
      if (NS_SUCCEEDED(child->QueryInterface(aIID, (void**)getter_AddRefs(temp)))) {
        return NS_OK;
      }
    }
  }

  return NS_ERROR_FAILURE;
}

/** ---------------------------------------------------
 *  Helper function 
 */
static nsIPresShell*
GetPresShellFor(nsIDocShell* aDocShell)
{
  nsIPresShell* shell = nsnull;
  if (nsnull != aDocShell) {
    nsIContentViewer* cv = nsnull;
    aDocShell->GetContentViewer(&cv);
    if (nsnull != cv) {
      nsIDocumentViewer* docv = nsnull;
      CallQueryInterface(cv, &docv);
      if (nsnull != docv) {
        nsIPresContext* cx;
        docv->GetPresContext(cx);
        if (nsnull != cx) {
          cx->GetShell(&shell);
          NS_RELEASE(cx);
        }
        NS_RELEASE(docv);
      }
      NS_RELEASE(cv);
    }
  }
  return shell;
}

//---------------------------------------------------------------
//---------------------------------------------------------------
//-- Debug helper routines
//---------------------------------------------------------------
//---------------------------------------------------------------
#ifdef DEBUG_PRINTING

/** ---------------------------------------------------
 *  Dumps Frames for Printing
 */
static void DumpFrames(FILE*                 out, 
                       nsIPresContext*       aPresContext, 
                       nsIRenderingContext * aRendContext, 
                       nsIFrame *            aFrame, 
                       PRInt32               aLevel)
{
  NS_ASSERTION(out, "Pointer is null!");
  NS_ASSERTION(aPresContext, "Pointer is null!");
  NS_ASSERTION(aRendContext, "Pointer is null!");
  NS_ASSERTION(aFrame, "Pointer is null!");

  nsIFrame * child;
  aFrame->FirstChild(aPresContext, nsnull, &child);
  while (child != nsnull) {
    for (PRInt32 i=0;i<aLevel;i++) {
     fprintf(out, "  ");
    }
    nsAutoString tmp;
    nsIFrameDebug*  frameDebug;

    if (NS_SUCCEEDED(CallQueryInterface(child, &frameDebug))) {
      frameDebug->GetFrameName(tmp);
    }
    fputs(NS_LossyConvertUCS2toASCII(tmp).get(), out);
    nsFrameState state;
    child->GetFrameState(&state);
    PRBool isSelected;
    if (NS_SUCCEEDED(child->IsVisibleForPainting(aPresContext, *aRendContext, PR_TRUE, &isSelected))) {
      fprintf(out, " %p %s", child, isSelected?"VIS":"UVS");
      nsRect rect;
      child->GetRect(rect);
      fprintf(out, "[%d,%d,%d,%d] ", rect.x, rect.y, rect.width, rect.height);
      nsIView * view;
      child->GetView(aPresContext, &view);
      fprintf(out, "v: %p ", view);
      fprintf(out, "\n");
      DumpFrames(out, aPresContext, aRendContext, child, aLevel+1);
      child->GetNextSibling(&child);
    }
  }
}


/** ---------------------------------------------------
 *  Dumps the Views from the DocShell
 */
static void
DumpViews(nsIDocShell* aDocShell, FILE* out)
{
  NS_ASSERTION(aDocShell, "Pointer is null!");
  NS_ASSERTION(out, "Pointer is null!");

  if (nsnull != aDocShell) {
    fprintf(out, "docshell=%p \n", aDocShell);
    nsIPresShell* shell = GetPresShellFor(aDocShell);
    if (nsnull != shell) {
      nsCOMPtr<nsIViewManager> vm;
      shell->GetViewManager(getter_AddRefs(vm));
      if (vm) {
        nsIView* root;
        vm->GetRootView(root);
        if (nsnull != root) {
          root->List(out);
        }
      }
      NS_RELEASE(shell);
    }
    else {
      fputs("null pres shell\n", out);
    }

    // dump the views of the sub documents
    PRInt32 i, n;
    nsCOMPtr<nsIDocShellTreeNode> docShellAsNode(do_QueryInterface(aDocShell));
    docShellAsNode->GetChildCount(&n);
    for (i = 0; i < n; i++) {
      nsCOMPtr<nsIDocShellTreeItem> child;
      docShellAsNode->GetChildAt(i, getter_AddRefs(child));
      nsCOMPtr<nsIDocShell> childAsShell(do_QueryInterface(child));
      if (childAsShell) {
        DumpViews(childAsShell, out);
      }
    }
  }
}

/** ---------------------------------------------------
 *  Dumps the Views and Frames
 */
void DumpLayoutData(char*              aTitleStr,
                    char*              aURLStr,
                    nsIPresContext*    aPresContext,
                    nsIDeviceContext * aDC,
                    nsIFrame *         aRootFrame,
                    nsIWebShell *      aWebShell,
                    FILE*              aFD = nsnull)
{
  if (aPresContext == nsnull || aDC == nsnull) {
    return;
  }

#ifdef NS_PRINT_PREVIEW
  nsCOMPtr<nsIPrintPreviewContext> ppContext = do_QueryInterface(aPresContext);
  if (ppContext) {
    return;
  }
#endif


  NS_ASSERTION(aRootFrame, "Pointer is null!");
  NS_ASSERTION(aWebShell, "Pointer is null!");

  // Dump all the frames and view to a a file
  char filename[256];
  sprintf(filename, "dump_layout_%d.txt", gDumpLOFileNameCnt++);
  FILE * fd = aFD?aFD:fopen(filename, "w");
  if (fd) {
    fprintf(fd, "Title: %s\n", aTitleStr?aTitleStr:"");
    fprintf(fd, "URL:   %s\n", aURLStr?aURLStr:"");
    fprintf(fd, "--------------- Frames ----------------\n");
    fprintf(fd, "--------------- Frames ----------------\n");
    nsCOMPtr<nsIRenderingContext> renderingContext;
    aDC->CreateRenderingContext(*getter_AddRefs(renderingContext));
    DumpFrames(fd, aPresContext, renderingContext, aRootFrame, 0);
    fprintf(fd, "---------------------------------------\n\n");
    fprintf(fd, "--------------- Views From Root Frame----------------\n");
    nsIView * v;
    aRootFrame->GetView(aPresContext, &v);
    if (v) {
      v->List(fd);
    } else {
      printf("View is null!\n");
    }
    nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(aWebShell));
    if (docShell) {
      fprintf(fd, "--------------- All Views ----------------\n");
      DumpViews(docShell, fd);
      fprintf(fd, "---------------------------------------\n\n");
    }
    if (aFD == nsnull) {
      fclose(fd);
    }
  }
}


static void DumpPrintObjectsList(nsVoidArray * aDocList, FILE* aFD = nsnull)
{
  NS_ASSERTION(aDocList, "Pointer is null!");

  FILE * fd = aFD?aFD:stdout;

  char * types[] = {"DC", "FR", "IF", "FS"};
  fprintf(fd, "Doc List\n***************************************************\n");
  fprintf(fd, "T  P A H    PO    WebShell   Seq     Page      Root     Page#    Rect\n");
  PRInt32 cnt = aDocList->Count();
  for (PRInt32 i=0;i<cnt;i++) {
    PrintObject* po = (PrintObject*)aDocList->ElementAt(i);
    NS_ASSERTION(po, "PrintObject can't be null!");
    nsIFrame* rootFrame = nsnull;
    if (po->mPresShell) {
      po->mPresShell->GetRootFrame(&rootFrame);
      while (rootFrame != nsnull) {
        nsIPageSequenceFrame * sqf = nsnull;
        if (NS_SUCCEEDED(CallQueryInterface(rootFrame, &sqf)) && sqf) {
          break;
        }
        rootFrame->FirstChild(po->mPresContext, nsnull, &rootFrame);
      }
    }

    fprintf(fd, "%s %d %d %d %p %p %p %p %p   %d   %d,%d,%d,%d\n", types[po->mFrameType], 
            po->IsPrintable(), po->mPrintAsIs, po->mHasBeenPrinted, po, po->mWebShell, po->mSeqFrame,
            po->mPageFrame, rootFrame, po->mPageNum, po->mRect.x, po->mRect.y, po->mRect.width, po->mRect.height); 
    if (fd != nsnull && fd != stdout) {
      fprintf(stdout, "%s %d %d %d %p %p %p %p %p   %d   %d,%d,%d,%d\n", types[po->mFrameType], 
              po->IsPrintable(), po->mPrintAsIs, po->mHasBeenPrinted, po, po->mWebShell, po->mSeqFrame,
              po->mPageFrame, rootFrame, po->mPageNum, po->mRect.x, po->mRect.y, po->mRect.width, po->mRect.height); 
    }
  } 
} 
static void DumpPrintObjectsTree(PrintObject * aPO, int aLevel= 0, FILE* aFD = nsnull)
{
  NS_ASSERTION(aPO, "Pointer is null!");

  FILE * fd = aFD?aFD:stdout;
  char * types[] = {"DC", "FR", "IF", "FS"};
  if (aLevel == 0) {
    fprintf(fd, "DocTree\n***************************************************\n");
    fprintf(fd, "T     PO    WebShell   Seq      Page     Page#    Rect\n");
  }
  PRInt32 cnt = aPO->mKids.Count();
  for (PRInt32 i=0;i<cnt;i++) {
    PrintObject* po = (PrintObject*)aPO->mKids.ElementAt(i);
    NS_ASSERTION(po, "PrintObject can't be null!");
    for (PRInt32 k=0;k<aLevel;k++) fprintf(fd, "  ");
    fprintf(fd, "%s %p %p %p %p %d %d,%d,%d,%d\n", types[po->mFrameType], po, po->mWebShell, po->mSeqFrame,
           po->mPageFrame, po->mPageNum, po->mRect.x, po->mRect.y, po->mRect.width, po->mRect.height); 
  } 
}
 
static void GetDocTitleAndURL(PrintObject* aPO, char *& aDocStr, char *& aURLStr)
{
  PRUnichar * docTitleStr;
  PRUnichar * docURLStr;
  nsAutoString strDocTitle;
  nsAutoString strURL;
  DocumentViewerImpl::GetWebShellTitleAndURL(aPO->mWebShell, &docTitleStr, &docURLStr); 

  if (!docTitleStr) {
    if (docURLStr) {
      docTitleStr = docURLStr;
      docURLStr   = nsnull;
    } else {
      docTitleStr = ToNewUnicode(NS_LITERAL_STRING("Document"));
    }
  }
  aDocStr = nsnull;
  aURLStr = nsnull;

  if (docTitleStr) {
    nsAutoString strDocTitle(docTitleStr);
    aDocStr = ToNewCString(strDocTitle);
    nsMemory::Free(docTitleStr);
  }

  if (docURLStr) {
    nsAutoString strURL(docURLStr);
    aURLStr = ToNewCString(strURL);
    nsMemory::Free(docURLStr);
  }
}

static void DumpPrintObjectsTreeLayout(PrintObject * aPO, 
                                       nsIDeviceContext * aDC,
                                       int aLevel= 0, FILE * aFD = nsnull)
{
  NS_ASSERTION(aPO, "Pointer is null!");
  NS_ASSERTION(aDC, "Pointer is null!");

  char * types[] = {"DC", "FR", "IF", "FS"};
  FILE * fd = nsnull;
  if (aLevel == 0) {
    fd = fopen("tree_layout.txt", "w");
    fprintf(fd, "DocTree\n***************************************************\n");
    fprintf(fd, "***************************************************\n");
    fprintf(fd, "T     PO    WebShell   Seq      Page     Page#    Rect\n");
  } else {
    fd = aFD;
  }
  if (fd) {
    nsIFrame* rootFrame = nsnull;
    if (aPO->mPresShell) {
      aPO->mPresShell->GetRootFrame(&rootFrame);
    }
    for (PRInt32 k=0;k<aLevel;k++) fprintf(fd, "  ");
    fprintf(fd, "%s %p %p %p %p %d %d,%d,%d,%d\n", types[aPO->mFrameType], aPO, aPO->mWebShell, aPO->mSeqFrame,
           aPO->mPageFrame, aPO->mPageNum, aPO->mRect.x, aPO->mRect.y, aPO->mRect.width, aPO->mRect.height); 
    char * docStr;
    char * urlStr;
    GetDocTitleAndURL(aPO, docStr, urlStr);
    DumpLayoutData(docStr, urlStr, aPO->mPresContext, aDC, rootFrame, aPO->mWebShell, fd);
    if (docStr) nsMemory::Free(docStr);
    if (urlStr) nsMemory::Free(urlStr);
    fprintf(fd, "<***************************************************>\n");

    PRInt32 cnt = aPO->mKids.Count();
    for (PRInt32 i=0;i<cnt;i++) {
      PrintObject* po = (PrintObject*)aPO->mKids.ElementAt(i);
      NS_ASSERTION(po, "PrintObject can't be null!");
      DumpPrintObjectsTreeLayout(po, aDC, aLevel+1, fd);
    }
  }
  if (aLevel == 0 && fd) {
    fclose(fd);
  }
} 

static void DumpPrintObjectsListStart(char * aStr, nsVoidArray * aDocList, FILE* aFD = nsnull)
{
  NS_ASSERTION(aStr, "Pointer is null!");
  NS_ASSERTION(aDocList, "Pointer is null!");

  FILE * fd = aFD?aFD:stdout;
  fprintf(fd, "%s\n", aStr);
  DumpPrintObjectsList(aDocList, aFD);
}

#define DUMP_DOC_LIST(_title) DumpPrintObjectsListStart((_title), mPrt->mPrintDocList, mPrt->mDebugFD);
#define DUMP_DOC_TREE DumpPrintObjectsTree(mPrt->mPrintObject);
#define DUMP_DOC_TREELAYOUT DumpPrintObjectsTreeLayout(mPrt->mPrintObject, mPrt->mPrintDC);
#else
#define DUMP_DOC_LIST(_title)
#define DUMP_DOC_TREE
#define DUMP_DOC_TREELAYOUT 
#endif

//---------------------------------------------------------------
//---------------------------------------------------------------
//-- End of debug helper routines
//---------------------------------------------------------------
//---------------------------------------------------------------

/** ---------------------------------------------------
 *  Giving a child frame it searches "up" the tree until it 
 *  finds a "Page" frame.
 */
static nsIFrame * GetPageFrame(nsIFrame * aFrame)
{
  nsIFrame * frame = aFrame;
  while (frame != nsnull) {
    nsCOMPtr<nsIAtom> type;
    frame->GetFrameType(getter_AddRefs(type));
    if (type.get() == nsLayoutAtoms::pageFrame) {
      return frame;
    }
    frame->GetParent(&frame);
  }
  return nsnull;
}


/** ---------------------------------------------------
 *  Find by checking content's tag type
 */
static nsIFrame * FindFrameByType(nsIPresContext* aPresContext,
                                  nsIFrame *      aParentFrame,
                                  nsIAtom *       aType,
                                  nsRect&         aRect,
                                  nsRect&         aChildRect) 
{
  NS_ASSERTION(aPresContext, "Pointer is null!");
  NS_ASSERTION(aParentFrame, "Pointer is null!");
  NS_ASSERTION(aType, "Pointer is null!");

  nsIFrame * child;
  nsRect rect;
  aParentFrame->GetRect(rect);
  aRect.x += rect.x;
  aRect.y += rect.y;
  aParentFrame->FirstChild(aPresContext, nsnull, &child);
  while (child != nsnull) {
    nsCOMPtr<nsIContent> content;
    child->GetContent(getter_AddRefs(content));
    if (content) {
      nsCOMPtr<nsIAtom> type;
      content->GetTag(*getter_AddRefs(type));
      if (type.get() == aType) {
        nsRect r;
        child->GetRect(r);
        aChildRect.SetRect(aRect.x + r.x, aRect.y + r.y, r.width, r.height);
        aRect.x -= rect.x;
        aRect.y -= rect.y;
        return child;
      }
    }
    nsIFrame * fndFrame = FindFrameByType(aPresContext, child, aType, aRect, aChildRect);
    if (fndFrame != nsnull) {
      return fndFrame;
    }
    child->GetNextSibling(&child);
  }
  aRect.x -= rect.x;
  aRect.y -= rect.y;
  return nsnull;
}

/** ---------------------------------------------------
 *  Find by checking frames type
 */
static nsresult FindSelectionBounds(nsIPresContext* aPresContext,
                                    nsIRenderingContext& aRC,
                                    nsIFrame *      aParentFrame,
                                    nsRect&         aRect,
                                    nsIFrame *&     aStartFrame,
                                    nsRect&         aStartRect,
                                    nsIFrame *&     aEndFrame,
                                    nsRect&         aEndRect)
{
  NS_ASSERTION(aPresContext, "Pointer is null!");
  NS_ASSERTION(aParentFrame, "Pointer is null!");

  nsIFrame * child;
  aParentFrame->FirstChild(aPresContext, nsnull, &child);
  nsRect rect;
  aParentFrame->GetRect(rect);
  aRect.x += rect.x;
  aRect.y += rect.y;
  while (child != nsnull) {
    nsFrameState state;
    child->GetFrameState(&state);
    // only leaf frames have this bit flipped
    // then check the hard way
    PRBool isSelected = (state & NS_FRAME_SELECTED_CONTENT) == NS_FRAME_SELECTED_CONTENT;
    if (isSelected) {
      if (NS_FAILED(child->IsVisibleForPainting(aPresContext, aRC, PR_TRUE, &isSelected))) {
        return NS_ERROR_FAILURE;
      }
    }

    if (isSelected) {
      nsRect r;
      child->GetRect(r);
      if (aStartFrame == nsnull) {
        aStartFrame = child;
        aStartRect.SetRect(aRect.x + r.x, aRect.y + r.y, r.width, r.height);
      } else {
        child->GetRect(r);
        aEndFrame = child;
        aEndRect.SetRect(aRect.x + r.x, aRect.y + r.y, r.width, r.height);
      }
    }
    FindSelectionBounds(aPresContext, aRC, child, aRect, aStartFrame, aStartRect, aEndFrame, aEndRect);
    child->GetNextSibling(&child);
  }
  aRect.x -= rect.x;
  aRect.y -= rect.y;
  return NS_OK;
}

/** ---------------------------------------------------
 *  This method finds the starting and ending page numbers
 *  of the selection and also returns rect for each where
 *  the x,y of the rect is relative to the very top of the
 *  frame tree (absolutely positioned)
 */
static nsresult GetPageRangeForSelection(nsIPresShell *        aPresShell,
                                         nsIPresContext*       aPresContext,
                                         nsIRenderingContext&  aRC,
                                         nsISelection*         aSelection,
                                         nsIPageSequenceFrame* aPageSeqFrame,
                                         nsIFrame**            aStartFrame,
                                         PRInt32&              aStartPageNum,
                                         nsRect&               aStartRect,
                                         nsIFrame**            aEndFrame,
                                         PRInt32&              aEndPageNum,
                                         nsRect&               aEndRect)
{
  NS_ASSERTION(aPresShell, "Pointer is null!");
  NS_ASSERTION(aPresContext, "Pointer is null!");
  NS_ASSERTION(aSelection, "Pointer is null!");
  NS_ASSERTION(aPageSeqFrame, "Pointer is null!");
  NS_ASSERTION(aStartFrame, "Pointer is null!");
  NS_ASSERTION(aEndFrame, "Pointer is null!");

  nsIFrame * seqFrame;
  if (NS_FAILED(CallQueryInterface(aPageSeqFrame, &seqFrame))) {
    return NS_ERROR_FAILURE;
  }

  nsIFrame * startFrame = nsnull;
  nsIFrame * endFrame   = nsnull;
  nsRect rect;
  seqFrame->GetRect(rect);

  // start out with the sequence frame and search the entire frame tree
  // capturing the the starting and ending child frames of the selection 
  // and their rects
  FindSelectionBounds(aPresContext, aRC, seqFrame, rect, startFrame, aStartRect, endFrame, aEndRect);

#ifdef DEBUG_rodsX
  printf("Start Frame: %p\n", startFrame);
  printf("End Frame:   %p\n", endFrame);
#endif

  // initial the page numbers here 
  // in case we don't find and frames
  aStartPageNum = -1;
  aEndPageNum   = -1;

  nsIFrame * startPageFrame;
  nsIFrame * endPageFrame;

  // check to make sure we found a starting frame
  if (startFrame != nsnull) {
    // Now search up the tree to find what page the
    // start/ending selections frames are on
    //
    // Check to see if start should be same as end if
    // the end frame comes back null
    if (endFrame == nsnull) {
      // XXX the "GetPageFrame" step could be integrated into 
      // the FindSelectionBounds step, but walking up to find
      // the parent of a child frame isn't expensive and it makes
      // FindSelectionBounds a little easier to understand
      startPageFrame = GetPageFrame(startFrame);
      endPageFrame   = startPageFrame;
      aEndRect       = aStartRect;
    } else {
      startPageFrame = GetPageFrame(startFrame);
      endPageFrame   = GetPageFrame(endFrame);
    }
  } else {
    return NS_ERROR_FAILURE;
  }

#ifdef DEBUG_rodsX
  printf("Start Page: %p\n", startPageFrame);
  printf("End Page:   %p\n", endPageFrame);

  // dump all the pages and their pointers
  {
  PRInt32 pageNum = 1;
  nsIFrame * child;
  seqFrame->FirstChild(aPresContext, nsnull, &child);
  while (child != nsnull) {
    printf("Page: %d - %p\n", pageNum, child);
    pageNum++;
    child->GetNextSibling(&child);
  }
  }
#endif

  // Now that we have the page frames
  // find out what the page numbers are for each frame
  PRInt32 pageNum = 1;
  nsIFrame * page;
  seqFrame->FirstChild(aPresContext, nsnull, &page);
  while (page != nsnull) {
    if (page == startPageFrame) {
      aStartPageNum = pageNum;
    } 
    if (page == endPageFrame) {
      aEndPageNum = pageNum;
    }
    pageNum++;
    page->GetNextSibling(&page);
  }

#ifdef DEBUG_rodsX
  printf("Start Page No: %d\n", aStartPageNum);
  printf("End Page No:   %d\n", aEndPageNum);
#endif

  *aStartFrame = startPageFrame;
  *aEndFrame   = endPageFrame;

  return NS_OK;
}

//-------------------------------------------------------
PRBool 
DocumentViewerImpl::IsParentAFrameSet(nsIWebShell * aParent)
{
  NS_ASSERTION(aParent, "Pointer is null!");

  // See if if the incoming doc is the root document
  nsCOMPtr<nsIDocShellTreeItem> parentAsItem(do_QueryInterface(aParent));
  if (!parentAsItem) return PR_FALSE;

  // When it is the top level document we need to check 
  // to see if it contains a frameset. If it does, then 
  // we only want to print the doc's children and not the document itself
  // For anything else we always print all the children and the document
  // for example, if the doc contains an IFRAME we eant to print the child
  // document (the IFRAME) and then the rest of the document.
  //
  // XXX we really need to search the frame tree, and not the content
  // but there is no way to distinguish between IFRAMEs and FRAMEs
  // with the GetFrameType call.
  // Bug 53459 has been files so we can eventually distinguish 
  // between IFRAME frames and FRAME frames
  PRBool isFrameSet = PR_FALSE;
  // only check to see if there is a frameset if there is 
  // NO parent doc for this doc. meaning this parent is the root doc
  nsCOMPtr<nsIPresShell> shell;
  mPresContext->GetShell(getter_AddRefs(shell));
  if (shell) {
    nsCOMPtr<nsIDocument> doc;
    shell->GetDocument(getter_AddRefs(doc));
    if (doc) {
      nsCOMPtr<nsIContent> rootContent;
      doc->GetRootContent(getter_AddRefs(rootContent));
      if (rootContent) {
        if (NS_SUCCEEDED(FindFrameSetWithIID(rootContent, NS_GET_IID(nsIDOMHTMLFrameSetElement)))) {
          isFrameSet = PR_TRUE;
        }
      }
    }
  }
  return isFrameSet;
}

//-------------------------------------------------------
PRBool 
DocumentViewerImpl::IsWebShellAFrameSet(nsIWebShell * aWebShell)
{
  NS_ASSERTION(aWebShell, "Pointer is null!");

  PRBool doesContainFrameSet = PR_FALSE;
  nsCOMPtr<nsIPresShell> presShell;
  nsCOMPtr<nsIContent>   rootContent;
  GetPresShellAndRootContent(aWebShell, getter_AddRefs(presShell), getter_AddRefs(rootContent));
  if (rootContent) {
    if (NS_SUCCEEDED(FindFrameSetWithIID(rootContent, NS_GET_IID(nsIDOMHTMLFrameSetElement)))) {
      doesContainFrameSet = PR_TRUE;
    }
  }
  return doesContainFrameSet;
}

//-------------------------------------------------------
void
DocumentViewerImpl::GetWebShellTitleAndURL(nsIWebShell * aWebShell, 
                                           PRUnichar**   aTitle, 
                                           PRUnichar**   aURLStr)
{
  NS_ASSERTION(aWebShell, "Pointer is null!");
  NS_ASSERTION(aTitle, "Pointer is null!");
  NS_ASSERTION(aURLStr, "Pointer is null!");

  *aTitle  = nsnull;
  *aURLStr = nsnull;

  nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(aWebShell));
  if (docShell) {
    nsCOMPtr<nsIPresShell> presShell;
    docShell->GetPresShell(getter_AddRefs(presShell));
    if (presShell) {
      nsCOMPtr<nsIDocument> doc;
      presShell->GetDocument(getter_AddRefs(doc));
      if (doc) {
        const nsString* docTitle = doc->GetDocumentTitle();
        if (docTitle && !docTitle->IsEmpty()) {
          *aTitle = ToNewUnicode(*docTitle);
        }

        nsCOMPtr<nsIURI> url;
        doc->GetDocumentURL(getter_AddRefs(url));
        if (url) {
          nsXPIDLCString urlCStr;
          url->GetSpec(getter_Copies(urlCStr));
          if (urlCStr.get()) {
            *aURLStr = ToNewUnicode(urlCStr);
          }
        }

      }
    }
  }
}


//-------------------------------------------------------
PRBool
DocumentViewerImpl::DonePrintingPages(PrintObject* aPO)
{
  //NS_ASSERTION(aPO, "Pointer is null!");
  PRINT_DEBUG_MSG3("****** In DV::DonePrintingPages PO: %p (%s)\n", aPO, aPO?gFrameTypesStr[aPO->mFrameType]:"");

  if (aPO != nsnull) {
    aPO->mHasBeenPrinted = PR_TRUE;
    nsresult rv;
    PRBool didPrint = PrintDocContent(mPrt->mPrintObject, rv);
    if (NS_SUCCEEDED(rv) && didPrint) {
      PRINT_DEBUG_MSG4("****** In DV::DonePrintingPages PO: %p (%s) didPrint:%s (Not Done Printing)\n", aPO, gFrameTypesStr[aPO->mFrameType], PRT_YESNO(didPrint));
      return PR_FALSE;
    }
  }

  DoProgressForAsIsFrames();
  DoProgressForSeparateFrames();

  mIsDoingPrinting = PR_FALSE;
  delete mPrt;
  mPrt = nsnull;

  NS_IF_RELEASE(mPagePrintTimer);

  return PR_TRUE;
}

//-------------------------------------------------------
PRBool
DocumentViewerImpl::PrintPage(nsIPresContext*   aPresContext,
                              nsIPrintSettings* aPrintSettings,
                              PrintObject*      aPO)
{
  NS_ASSERTION(aPresContext, "Pointer is null!");
  NS_ASSERTION(aPrintSettings, "Pointer is null!");
  NS_ASSERTION(aPO, "Pointer is null!");

  PRINT_DEBUG_MSG1("-----------------------------------\n");
  PRINT_DEBUG_MSG3("------ In DV::PrintPage PO: %p (%s)\n", aPO, gFrameTypesStr[aPO->mFrameType]);

  if (aPrintSettings != nsnull) {
    PRBool isCancelled;
    aPrintSettings->GetIsCancelled(&isCancelled);
    // DO NOT allow the print job to be cancelled if it is Print FrameAsIs
    // because it is only printing one page.
    if (isCancelled && mPrt->mPrintFrameType != nsIPrintSettings::kFramesAsIs) {
      aPrintSettings->SetIsCancelled(PR_FALSE);
      return PR_TRUE;
    }
  }
  if (mPrt->mPrintProgress) {
    PRBool isCancelled;
    mPrt->mPrintProgress->GetProcessCanceledByUser(&isCancelled);
    // DO NOT allow the print job to be cancelled if it is Print FrameAsIs
    // because it is only printing one page.
    if (isCancelled && mPrt->mPrintFrameType != nsIPrintSettings::kFramesAsIs) {
      aPrintSettings->SetIsCancelled(PR_FALSE);
      return PR_TRUE;
    }
  }

  PRInt32 pageNum;
  PRInt32 curPage;
  PRInt32 endPage;
  mPageSeqFrame->GetCurrentPageNum(&pageNum);

  PRBool donePrinting = PR_FALSE;
  PRBool isDoingPrintRange;
  mPageSeqFrame->IsDoingPrintRange(&isDoingPrintRange);
  if (isDoingPrintRange) {
    PRInt32 fromPage;
    PRInt32 toPage;
    PRInt32 numPages;
    mPageSeqFrame->GetPrintRange(&fromPage, &toPage);
    mPageSeqFrame->GetNumPages(&numPages);
    if (fromPage > numPages) {
      return PR_TRUE;
    }
    if (toPage > numPages) {
      toPage = numPages;
    }

    PRINT_DEBUG_MSG4("****** Printing Page %d printing from %d to page %d\n", pageNum, fromPage, toPage);

    donePrinting = pageNum >= toPage;
    curPage = pageNum - fromPage;
    endPage = toPage - fromPage;
  } else {
    PRInt32 numPages;
    mPageSeqFrame->GetNumPages(&numPages);

    PRINT_DEBUG_MSG3("****** Printing Page %d of %d page(s)\n", pageNum, numPages);

    donePrinting = pageNum >= numPages;
    curPage = pageNum;
    endPage = numPages;
  }

  // NOTE: mPrt->mPrintFrameType gets set to  "kFramesAsIs" when a
  // plain document contains IFrames, so we need to override that case here
  if (mPrt->mPrintFrameType == nsIPrintSettings::kEachFrameSep) {
    DoProgressForSeparateFrames();

  } else if (mPrt->mPrintFrameType != nsIPrintSettings::kFramesAsIs || 
             mPrt->mPrintObject->mFrameType == eDoc && aPO == mPrt->mPrintObject) {
    PrintData::DoOnProgressChange(mPrt->mPrintProgressListeners, curPage+1, endPage);
  }

  // Set Clip when Printing "AsIs" or 
  // when printing an IFrame for SelectedFrame or EachFrame
  PRBool setClip = PR_FALSE;
  switch (mPrt->mPrintFrameType) {

    case nsIPrintSettings::kFramesAsIs:
      setClip = PR_TRUE;
      break;

    case nsIPrintSettings::kSelectedFrame:
      if (aPO->mPrintAsIs) {
        if (aPO->mFrameType == eIFrame) {
          setClip = aPO != mPrt->mSelectedPO;
        }
      }
      break;

    case nsIPrintSettings::kEachFrameSep:
      if (aPO->mPrintAsIs) {
        if (aPO->mFrameType == eIFrame) {
          setClip = PR_TRUE;
        }
      }
      break;

  } //switch 

  if (setClip) {
    mPageSeqFrame->SetClipRect(aPO->mPresContext, &aPO->mClipRect);
  }

  // Print the Page
  // if a print job was cancelled externally, an EndPage or BeginPage may
  // fail and the failure is passed back here.
  // Returning PR_TRUE means we are done printing.
  if (NS_FAILED(mPageSeqFrame->PrintNextPage(aPresContext))) {
    return PR_TRUE;
  }                

  // Now see if any of the SubDocs are on this page
  if (aPO->mPrintAsIs) {
    nsIPageSequenceFrame * curPageSeq = mPageSeqFrame;
    aPO->mHasBeenPrinted = PR_TRUE;
    PRInt32 cnt = aPO->mKids.Count();
    for (PRInt32 i=0;i<cnt;i++) {
      PrintObject* po = (PrintObject*)aPO->mKids[i];
      NS_ASSERTION(po, "PrintObject can't be null!");

      // Now verify that SubDoc's PageNum matches the
      // page num of it's parent doc
      curPageSeq->GetCurrentPageNum(&pageNum);
      nsIFrame* fr;
      CallQueryInterface(curPageSeq, &fr);

      if (fr == po->mSeqFrame && pageNum == po->mPageNum) {
        PRBool donePrintingSubDoc;
        DoPrint(po, PR_TRUE, donePrintingSubDoc); // synchronous printing, it changes the value mPageSeqFrame
        po->mHasBeenPrinted = PR_TRUE;
      }
    } // while
    mPageSeqFrame = curPageSeq;

    if (aPO->mParent == nsnull ||
        (aPO->mParent != nsnull && !aPO->mParent->mPrintAsIs && aPO->mPrintAsIs)) {
      mPageSeqFrame->DoPageEnd(aPresContext);
    }

    // XXX this is because PrintAsIs for FrameSets reflows to two pages
    // not sure why, but this needs to be fixed.
    if (aPO->mFrameType == eFrameSet && mPrt->mPrintFrameType == nsIPrintSettings::kFramesAsIs) {
      return PR_TRUE;
    }
  }

  return donePrinting;
}

//-------------------------------------------------------
// Recursively finds a PrintObject that has the aWebShell
PrintObject * DocumentViewerImpl::FindPrintObjectByWS(PrintObject* aPO, nsIWebShell * aWebShell)
{
  NS_ASSERTION(aPO, "Pointer is null!");
  NS_ASSERTION(aWebShell, "Pointer is null!");

  if (aPO->mWebShell == aWebShell) {
    return aPO;
  }
  PRInt32 cnt = aPO->mKids.Count();
  for (PRInt32 i=0;i<cnt;i++) {
    PrintObject* kid = (PrintObject*)aPO->mKids.ElementAt(i);
    NS_ASSERTION(kid, "PrintObject can't be null!");
    PrintObject* po = FindPrintObjectByWS(kid, aWebShell);
    if (po != nsnull) {
      return po;
    }
  }
  return nsnull;
}

//-------------------------------------------------------
// Recursively build a list of of sub documents to be printed
// that mirrors the document tree
void 
DocumentViewerImpl::BuildDocTree(nsIDocShellTreeNode * aParentNode, 
                                 nsVoidArray *         aDocList, 
                                 PrintObject *         aPO)
{
  NS_ASSERTION(aParentNode, "Pointer is null!");
  NS_ASSERTION(aDocList, "Pointer is null!");
  NS_ASSERTION(aPO, "Pointer is null!");

  PRInt32 childWebshellCount;
  aParentNode->GetChildCount(&childWebshellCount);
  if (childWebshellCount > 0) {
    for (PRInt32 i=0;i<childWebshellCount;i++) {
      nsCOMPtr<nsIDocShellTreeItem> child;
      aParentNode->GetChildAt(i, getter_AddRefs(child));
      nsCOMPtr<nsIDocShell> childAsShell(do_QueryInterface(child));
      nsCOMPtr<nsIContentViewer>  viewer;
      childAsShell->GetContentViewer(getter_AddRefs(viewer));
      if (viewer) {
        nsCOMPtr<nsIContentViewerFile> viewerFile(do_QueryInterface(viewer));
        if (viewerFile) {
          nsCOMPtr<nsIWebShell> childWebShell(do_QueryInterface(child));
          nsCOMPtr<nsIDocShellTreeNode> childNode(do_QueryInterface(child));
          PrintObject * po = new PrintObject;
          po->mWebShell = childWebShell;
          po->mParent   = aPO;
          aPO->mKids.AppendElement(po);
          aDocList->AppendElement(po);
          BuildDocTree(childNode, aDocList, po);
        }
      }
    }
  }
}

//-------------------------------------------------------
// Recursively sets the PO items to be printed "As Is"
// from the given item down into the tree
void
DocumentViewerImpl::SetPrintAsIs(PrintObject* aPO, PRBool aAsIs)
{
  NS_ASSERTION(aPO, "Pointer is null!");

  aPO->mPrintAsIs = aAsIs;
  for (PRInt32 i=0;i<aPO->mKids.Count();i++) {
    SetPrintAsIs((PrintObject*)aPO->mKids[i], aAsIs);
  } 
}

//-------------------------------------------------------
// Recursively sets all the PO items to be printed
// from the given item down into the tree
void
DocumentViewerImpl::SetPrintPO(PrintObject* aPO, PRBool aPrint)
{
  NS_ASSERTION(aPO, "Pointer is null!");

  aPO->mDontPrint = !aPrint;
  for (PRInt32 i=0;i<aPO->mKids.Count();i++) {
    SetPrintPO((PrintObject*)aPO->mKids[i], aPrint);
  } 
}

//-------------------------------------------------------
// Finds the Page Frame and the absolute location on the page
// for a Sub document.
//
// NOTE: This MUST be done after the sub-doc has been laid out
// This is called by "MapSubDocFrameLocations"
//
void DocumentViewerImpl::CalcPageFrameLocation(nsIPresShell * aPresShell,
                                               PrintObject*   aPO)
{
  NS_ASSERTION(aPresShell, "Pointer is null!");
  NS_ASSERTION(aPO, "Pointer is null!");

  if (aPO != nsnull && aPO->mContent != nsnull) {

    // Find that frame for the sub-doc's content element
    // in the parent document
    nsIFrame * frame;
    aPresShell->GetPrimaryFrameFor(aPO->mContent, &frame);
    NS_ASSERTION(frame, "Frame shouldn't be null!");
    nsMargin borderPadding(0, 0, 0, 0);
    frame->CalcBorderPadding(borderPadding);

    // Calc absolute position of the frame all the way up
    // to the SimpleSeq frame
    nsRect rect;
    frame->GetRect(rect);
    rect.Deflate(borderPadding);

    rect.x = 0;
    rect.y = 0;
    nsIFrame * parent    = frame;
    nsIFrame * pageFrame = nsnull;
    nsIFrame * seqFrame  = nsnull;
    while (parent != nsnull) {
      nsRect rr;
      parent->GetRect(rr);
      rect.x += rr.x;
      rect.y += rr.y;
      nsIFrame * temp = parent;
      temp->GetParent(&parent);
      // Keep a pointer to the Seq and Page frames 
      nsIPageSequenceFrame * sqf = nsnull;
      if (parent != nsnull && 
          NS_SUCCEEDED(CallQueryInterface(parent, &sqf)) && sqf) {
        pageFrame = temp;
        seqFrame  = parent;
      }
    }
    NS_ASSERTION(seqFrame, "The sequencer frame can't be null!");
    NS_ASSERTION(pageFrame, "The page frame can't be null!");

    // Remember the Frame location information for later
    aPO->mRect      = rect;
    aPO->mSeqFrame  = seqFrame;
    aPO->mPageFrame = pageFrame;

    // Calc the Page No it is on
    PRInt32 pageNum = 1;
    nsIFrame * child;
    seqFrame->FirstChild(aPO->mPresContext, nsnull, &child);
    while (child != nsnull) {
      if (pageFrame == child) {
        aPO->mPageNum = pageNum;
        break;
      }
      pageNum++;
      child->GetNextSibling(&child);
    } // while
  }
}

//-------------------------------------------------------
// This recusively walks the PO tree calculating the
// the page location and the absolute frame location for
// a sub-doc.
//
// NOTE: This MUST be done after the sub-doc has been laid out
// This is called by "ReflowDocList"
//
void
DocumentViewerImpl::MapSubDocFrameLocations(PrintObject* aPO)
{
  NS_ASSERTION(aPO, "Pointer is null!");

  if (aPO->mParent != nsnull && aPO->mParent->mPresShell) {
    CalcPageFrameLocation(aPO->mParent->mPresShell, aPO);
  }

  if (aPO->mPresShell) {
    for (PRInt32 i=0;i<aPO->mKids.Count();i++) {
      MapSubDocFrameLocations((PrintObject*)aPO->mKids[i]);
    }
  }
}


//-------------------------------------------------------
// This method is key to the entire print mechanism.
//
// This "maps" or figures out which sub-doc represents a
// given Frame or IFrame in it's parent sub-doc.
// 
// So the Mcontent pointer in the child sub-doc points to the
// content in the it's parent document, that caused it to be printed.
// This is used later to (after reflow) to find the absolute location
// of the sub-doc on it's parent's page frame so it can be 
// printed in the correct location.
//
// This method recursvely "walks" the content for a document finding
// all the Frames and IFrames, then sets the "mFrameType" data member
// which tells us what type of PO we have
void 
DocumentViewerImpl::MapContentForPO(PrintObject*   aRootObject,
                                    nsIPresShell*  aPresShell,
                                    nsIContent*    aContent)
{
  NS_ASSERTION(aRootObject, "Pointer is null!");
  NS_ASSERTION(aPresShell, "Pointer is null!");
  NS_ASSERTION(aContent, "Pointer is null!");

  nsCOMPtr<nsISupports> supps;
  aPresShell->GetSubShellFor(aContent, getter_AddRefs(supps));
  if (supps) {
    nsCOMPtr<nsIWebShell> webShell(do_QueryInterface(supps));
    if (webShell) {
      PrintObject * po = FindPrintObjectByWS(aRootObject, webShell);
      NS_ASSERTION(po, "PO can't be null!");

      if (po != nsnull) {
        po->mContent  = aContent;

        // Now, "type" the PO 
        nsCOMPtr<nsIDOMHTMLFrameSetElement> frameSet(do_QueryInterface(aContent));
        if (frameSet) {
          po->mFrameType = eFrameSet;
        } else {
          nsCOMPtr<nsIDOMHTMLFrameElement> frame(do_QueryInterface(aContent));
          if (frame) {
            po->mFrameType = eFrame;
          } else {
            nsCOMPtr<nsIDOMHTMLIFrameElement> iFrame(do_QueryInterface(aContent));
            if (iFrame) {
              po->mFrameType = eIFrame;
              po->mPrintAsIs = PR_TRUE;
              if (po->mParent) {
                po->mParent->mPrintAsIs = PR_TRUE;
              }
            }
          }
        }
      }
    }
  }

  // walk children content
  PRInt32 count;
  aContent->ChildCount(count);
  for (PRInt32 i=0;i<count;i++) {
    nsIContent* child;
    aContent->ChildAt(i, child);
    MapContentForPO(aRootObject, aPresShell, child);
  }
}

//-------------------------------------------------------
// The walks the PO tree and for each document it walks the content
// tree looking for any content that are sub-shells 
//
// It then sets the mContent pointer in the "found" PO object back to the 
// the document that contained it.
void 
DocumentViewerImpl::MapContentToWebShells(PrintObject* aRootPO,
                                          PrintObject* aPO)
{
  NS_ASSERTION(aRootPO, "Pointer is null!");
  NS_ASSERTION(aPO, "Pointer is null!");

  // Recursively walk the content from the root item
  nsCOMPtr<nsIPresShell> presShell;
  nsCOMPtr<nsIContent> rootContent;
  GetPresShellAndRootContent(aPO->mWebShell, getter_AddRefs(presShell), getter_AddRefs(rootContent));
  if (presShell && rootContent) {
    MapContentForPO(aRootPO, presShell, rootContent);
  }

  // Continue recursively walking the chilren of this PO
  for (PRInt32 i=0;i<aPO->mKids.Count();i++) {
    MapContentToWebShells(aRootPO, (PrintObject*)aPO->mKids[i]);
  } 

}

//-------------------------------------------------------
// This gets ref counted copies of the PresShell and Root Content
// for a given nsIWebShell
void 
DocumentViewerImpl::GetPresShellAndRootContent(nsIWebShell *  aWebShell, 
                                               nsIPresShell** aPresShell, 
                                               nsIContent**   aContent)
{
  NS_ASSERTION(aWebShell, "Pointer is null!");
  NS_ASSERTION(aPresShell, "Pointer is null!");
  NS_ASSERTION(aContent, "Pointer is null!");

  *aContent   = nsnull;
  *aPresShell = nsnull;

  nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(aWebShell));

  nsCOMPtr<nsIPresShell> presShell(getter_AddRefs(GetPresShellFor(docShell)));
  if (!presShell) return;

  nsCOMPtr<nsIPresContext> presContext;
  presShell->GetPresContext(getter_AddRefs(presContext));
  if (!presContext) return;

  nsCOMPtr<nsIDocument> doc;
  presShell->GetDocument(getter_AddRefs(doc));
  if (!doc) return;

  doc->GetRootContent(aContent); // this addrefs
  *aPresShell = presShell.get();
  NS_ADDREF(*aPresShell);
}

//-------------------------------------------------------
// Recursively sets the clip rect on all thchildren
void 
DocumentViewerImpl::SetClipRect(PrintObject*  aPO, 
                                const nsRect& aClipRect,
                                nscoord       aOffsetX,
                                nscoord       aOffsetY,
                                PRBool        aDoingSetClip)
{
  NS_ASSERTION(aPO, "Pointer is null!");

  nsRect clipRect = aClipRect;
  if (aDoingSetClip) {
    nscoord width  = (aPO->mRect.x+aPO->mRect.width) > aClipRect.width?aClipRect.width-aPO->mRect.x:aPO->mRect.width;
    nscoord height = (aPO->mRect.y+aPO->mRect.height) > aClipRect.height?aClipRect.height-aPO->mRect.y:aPO->mRect.height;
    aPO->mClipRect.SetRect(aPO->mRect.x, aPO->mRect.y, width, height);

  } 
  
  PRBool doClip = aDoingSetClip;

  if (aPO->mFrameType == eFrame) {
    if (aDoingSetClip) {
      aPO->mClipRect.SetRect(aOffsetX, aOffsetY, aPO->mClipRect.width, aPO->mClipRect.height);
      clipRect = aPO->mClipRect;
    } else if (mPrt->mPrintFrameType == nsIPrintSettings::kFramesAsIs) {
      aPO->mClipRect.SetRect(aOffsetX, aOffsetY, aPO->mRect.width, aPO->mRect.height);
      clipRect = aPO->mClipRect;
      doClip = PR_TRUE;
    }

  } else if (aPO->mFrameType == eIFrame) {

    if (aDoingSetClip) {
      aPO->mClipRect.SetRect(aOffsetX, aOffsetY, aPO->mClipRect.width, aPO->mClipRect.height);
      clipRect = aPO->mClipRect;
    } else {

      if (mPrt->mPrintFrameType == nsIPrintSettings::kSelectedFrame) {
        if (aPO->mParent && aPO->mParent == mPrt->mSelectedPO) {
          aPO->mClipRect.SetRect(aOffsetX, aOffsetY, aPO->mRect.width, aPO->mRect.height);
          clipRect = aPO->mClipRect;
          doClip = PR_TRUE;
        }
      } else {
        aPO->mClipRect.SetRect(aOffsetX, aOffsetY, aPO->mRect.width, aPO->mRect.height);
        clipRect = aPO->mClipRect;
        doClip = PR_TRUE;
      }
    }

  }


  PRINT_DEBUG_MSG3("In DV::SetClipRect PO: %p (%9s) ", aPO, gFrameTypesStr[aPO->mFrameType]);
  PRINT_DEBUG_MSG5("%5d,%5d,%5d,%5d\n", aPO->mClipRect.x, aPO->mClipRect.y,aPO->mClipRect.width, aPO->mClipRect.height);

  PRInt32 cnt = aPO->mKids.Count();
  for (PRInt32 i=0;i<cnt;i++) {
    SetClipRect((PrintObject *)aPO->mKids[i], clipRect, 
                aOffsetX+aPO->mRect.x, aOffsetY+aPO->mRect.y, doClip);
  }
}

//-------------------------------------------------------
// Recursively reflow each sub-doc and then calc
// all the frame locations of the sub-docs
nsresult 
DocumentViewerImpl::ReflowDocList(PrintObject* aPO)
{
  NS_ASSERTION(aPO, "Pointer is null!");

  // Reflow the PO 
  if (NS_FAILED(ReflowPrintObject(aPO))) {
    return NS_ERROR_FAILURE;
  }

  // Calc the absolute poistion of the frames
  MapSubDocFrameLocations(aPO);

  PRInt32 cnt = aPO->mKids.Count();
  for (PRInt32 i=0;i<cnt;i++) {
    if (NS_FAILED(ReflowDocList((PrintObject *)aPO->mKids[i]))) {
      return NS_ERROR_FAILURE;
    }
  }
  return NS_OK;
}

//-------------------------------------------------------
// Reflow a PrintObject
nsresult
DocumentViewerImpl::ReflowPrintObject(PrintObject * aPO)
{
  NS_ASSERTION(aPO, "Pointer is null!");

  // Now locate the nsIDocument for the WebShell
  nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(aPO->mWebShell));
  NS_ASSERTION(docShell, "The DocShell can't be NULL!");

  nsCOMPtr<nsIPresShell>  wsPresShell;
  docShell->GetPresShell(getter_AddRefs(wsPresShell));
  NS_ASSERTION(wsPresShell, "The PresShell can't be NULL!");

  nsCOMPtr<nsIDocument> document;
  wsPresShell->GetDocument(getter_AddRefs(document));

  // create the PresContext
  PRBool containerIsSet = PR_FALSE;
  nsresult rv;
  if (mIsCreatingPrintPreview) {
    nsCOMPtr<nsIPrintPreviewContext> printPreviewCon(do_CreateInstance(kPrintPreviewContextCID, &rv));
    if (NS_FAILED(rv)) {
      return rv;
    }
    aPO->mPresContext = do_QueryInterface(printPreviewCon);
    printPreviewCon->SetPrintSettings(mPrt->mPrintSettings);
  } else {
    nsCOMPtr<nsIPrintContext> printcon(do_CreateInstance(kPrintContextCID, &rv));
    if (NS_FAILED(rv)) {
      return rv;
    }
    aPO->mPresContext = do_QueryInterface(printcon);
    printcon->SetPrintSettings(mPrt->mPrintSettings);
  }

  // init it with the DC
  (aPO->mPresContext)->Init(mPrt->mPrintDocDC);

  CreateStyleSet(document, getter_AddRefs(aPO->mStyleSet));

  aPO->mPresShell = do_CreateInstance(kPresShellCID, &rv);
  if (NS_FAILED(rv)) {
    return rv;
  }

  aPO->mViewManager = do_CreateInstance(kViewManagerCID, &rv);
  if (NS_FAILED(rv)) {
    return rv;
  }

  rv = aPO->mViewManager->Init(mPrt->mPrintDocDC);
  if (NS_FAILED(rv)) {
    return rv;
  }

  PRInt32 pageWidth, pageHeight;
  mPrt->mPrintDocDC->GetDeviceSurfaceDimensions(pageWidth, pageHeight);

  PRInt32 width, height;
  if (aPO->mContent == nsnull || !aPO->mPrintAsIs ||
      (aPO->mPrintAsIs && aPO->mParent != nsnull && !aPO->mParent->mPrintAsIs) ||
      (aPO->mFrameType == eIFrame && aPO == mPrt->mSelectedPO)) {
    width  = pageWidth;
    height = pageHeight;
  } else {
    width  = aPO->mRect.width;
    height = aPO->mRect.height;
  }

  PRINT_DEBUG_MSG5("In DV::ReflowPrintObject PO: %p (%9s) Setting w,h to %d,%d\n", aPO, gFrameTypesStr[aPO->mFrameType], width, height);

  nsCOMPtr<nsIDOMWindowInternal> domWinIntl(do_QueryInterface(mPrt->mPrintDocDW));
  // XXX - Hack Alert
  // OK,  so ther eis a selection, we will print the entire selection 
  // on one page and then crop the page.
  // This means you can never print any selection that is longer than one page
  // put it keeps it from page breaking in the middle of your print of the selection
  // (see also nsSimplePageSequence.cpp)
  PRInt16 printRangeType = nsIPrintSettings::kRangeAllPages;
  if (mPrt->mPrintSettings != nsnull) {
    mPrt->mPrintSettings->GetPrintRange(&printRangeType);
  }


  if (printRangeType == nsIPrintSettings::kRangeSelection && IsThereARangeSelection(domWinIntl)) {
    height = 0x0FFFFFFF;
  }
  nsRect tbounds = nsRect(0, 0, width, height);

  // Create a child window of the parent that is our "root view/window"
  rv = CallCreateInstance(kViewCID, &aPO->mRootView);
  if (NS_FAILED(rv)) {
    return rv;
  }
  rv = (aPO->mRootView)->Init(aPO->mViewManager, tbounds, nsnull);
  if (NS_FAILED(rv)) {
    return rv;
  }

#ifdef NS_PRINT_PREVIEW
  // Here we decide whether we need scrollbars and 
  // what the parent will be of the widget
  if (mIsCreatingPrintPreview) {
    PRBool canCreateScrollbars = PR_FALSE;
    nsCOMPtr<nsIWidget> widget = mParentWidget;
    // the top PrintObject's widget will always have scrollbars
    if (aPO->mParent != nsnull && aPO->mContent) {
      nsCOMPtr<nsIFrameManager> frameMan;
      aPO->mParent->mPresShell->GetFrameManager(getter_AddRefs(frameMan));
      NS_ASSERTION(frameMan, "No Frame manager!");
      nsIFrame* frame;
      frameMan->GetPrimaryFrameFor(aPO->mContent, &frame);
      if (frame != nsnull && (aPO->mFrameType == eIFrame || aPO->mFrameType == eFrame)) {
        frame->FirstChild(aPO->mParent->mPresContext, nsnull, &frame);
      }
      if (frame != nsnull) {
        nsIView* view = nsnull;
        frame->GetView(aPO->mParent->mPresContext, &view);
        if (view != nsnull) {
          view->GetWidget(*getter_AddRefs(widget));
          canCreateScrollbars = PR_FALSE;
        }
      }
    } else {
      canCreateScrollbars = PR_TRUE;
    }
    rv = aPO->mRootView->CreateWidget(kWidgetCID, nsnull, widget->GetNativeData(NS_NATIVE_WIDGET));
    aPO->mRootView->GetWidget(*getter_AddRefs(aPO->mWindow));
    aPO->mPresContext->SetPaginatedScrolling(canCreateScrollbars);
  }
#endif // NS_PRINT_PREVIEW

  // Setup hierarchical relationship in view manager
  aPO->mViewManager->SetRootView(aPO->mRootView);
  aPO->mPresShell->Init(document, aPO->mPresContext,
                        aPO->mViewManager, aPO->mStyleSet);

  nsCompatibility mode;
  mPresContext->GetCompatibilityMode(&mode);
  aPO->mPresContext->SetCompatibilityMode(mode);
  if (!containerIsSet) {
    nsCOMPtr<nsISupports> supps(do_QueryInterface(aPO->mWebShell));
    aPO->mPresContext->SetContainer(supps);
  }

  // get the old history
  nsCOMPtr<nsIPresShell> presShell;
  nsCOMPtr<nsILayoutHistoryState>  layoutState;
  NS_ENSURE_SUCCESS(GetPresShell(*(getter_AddRefs(presShell))), NS_ERROR_FAILURE);
  presShell->CaptureHistoryState(getter_AddRefs(layoutState),PR_TRUE);

  // set it on the new pres shell
  aPO->mPresShell->SetHistoryState(layoutState);

  // turn off animated GIFs
  if (aPO->mPresContext) {
    aPO->mPresContext->GetImageAnimationMode(&aPO->mImgAnimationMode);
    aPO->mPresContext->SetImageAnimationMode(imgIContainer::kDontAnimMode);
  }

  aPO->mPresShell->BeginObservingDocument();

  nsMargin margin(0,0,0,0);
  mPrt->mPrintSettings->GetMarginInTwips(margin);

  // initialize it with the default/generic case
  nsRect adjRect(aPO->mRect.x != 0?margin.left:0, aPO->mRect.y != 0?margin.top:0, width, height);

  // XXX This is an arbitray height, 
  // but reflow somethimes gets upset when using max int
  // basically, we want to reflow a single page that is large 
  // enough to fit any atomic object like an IFrame
  const PRInt32 kFivePagesHigh = 5;

  // now, change the value for special cases
  if (mPrt->mPrintFrameType == nsIPrintSettings::kEachFrameSep) {
    if (aPO->mFrameType == eFrame) {
      adjRect.SetRect(0, 0, width, height);
    } else if (aPO->mFrameType == eIFrame) {
      height = pageHeight*kFivePagesHigh;
      adjRect.SetRect(aPO->mRect.x != 0?margin.left:0, aPO->mRect.y != 0?margin.top:0, width, height);
    }

  } else if (mPrt->mPrintFrameType == nsIPrintSettings::kSelectedFrame) {
    if (aPO->mFrameType == eFrame) {
      adjRect.SetRect(0, 0, width, height);
    } else if (aPO->mFrameType == eIFrame) {
      if (aPO == mPrt->mSelectedPO) {
        adjRect.x = 0;
        adjRect.y = 0;
      } else {
        height = pageHeight*kFivePagesHigh;
        adjRect.SetRect(aPO->mRect.x != 0?margin.left:0, aPO->mRect.y != 0?margin.top:0, width, height);
      }
    }
  }

  aPO->mPresContext->SetPageDim(&adjRect);
  rv = aPO->mPresShell->InitialReflow(width, height);
  if (NS_SUCCEEDED(rv)) {
    // Transfer Selection Ranges to the new Print PresShell
    nsCOMPtr<nsISelection> selection;
    nsCOMPtr<nsISelection> selectionPS;
    nsresult rvv = GetDocumentSelection(getter_AddRefs(selection), wsPresShell);
    if (NS_SUCCEEDED(rvv) && selection) {
      rvv = GetDocumentSelection(getter_AddRefs(selectionPS), aPO->mPresShell);
      if (NS_SUCCEEDED(rvv) && selectionPS) {
        PRInt32 cnt;
        selection->GetRangeCount(&cnt);
        PRInt32 inx;
        for (inx=0;inx<cnt;inx++) {
          nsCOMPtr<nsIDOMRange> range;
          if (NS_SUCCEEDED(selection->GetRangeAt(inx, getter_AddRefs(range)))) {
            selectionPS->AddRange(range);
          }
        }
      }
    }

#ifdef DEBUG_rods
    {
      char * docStr;
      char * urlStr;
      GetDocTitleAndURL(aPO, docStr, urlStr);
      char filename[256];
      sprintf(filename, "dump_%d.txt", gDumpFileNameCnt++);
      // Dump all the frames and view to a a file
      FILE * fd = fopen(filename, "w");
      if (fd) {
        nsIFrame  *theRootFrame;
        aPO->mPresShell->GetRootFrame(&theRootFrame);
        fprintf(fd, "Title: %s\n", docStr?docStr:"");
        fprintf(fd, "URL:   %s\n", urlStr?urlStr:"");
        fprintf(fd, "--------------- Frames ----------------\n");
        nsCOMPtr<nsIRenderingContext> renderingContext;
        mPrt->mPrintDocDC->CreateRenderingContext(*getter_AddRefs(renderingContext));
        DumpFrames(fd, aPO->mPresContext, renderingContext, theRootFrame, 0);
        fprintf(fd, "---------------------------------------\n\n");
        fprintf(fd, "--------------- Views From Root Frame----------------\n");
        nsIView * v;
        theRootFrame->GetView(aPO->mPresContext, &v);
        if (v) {
          v->List(fd);
        } else {
          printf("View is null!\n");
        }
        nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(aPO->mWebShell));
        if (docShell) {
          fprintf(fd, "--------------- All Views ----------------\n");
          DumpViews(docShell, fd);
          fprintf(fd, "---------------------------------------\n\n");
        }
        fclose(fd);
      }
      if (docStr) nsMemory::Free(docStr);
      if (urlStr) nsMemory::Free(urlStr);
    }
#endif
  }

  aPO->mPresShell->EndObservingDocument();

  return rv;
}


//-------------------------------------------------------
// Given a DOMWindow it recursively finds the PO object that matches
PrintObject* 
DocumentViewerImpl::FindPrintObjectByDOMWin(PrintObject* aPO, nsIDOMWindowInternal * aDOMWin)
{
  NS_ASSERTION(aPO, "Pointer is null!");

  // Often the CurFocused DOMWindow is passed in
  // andit is valid for it to be null, so short circut
  if (aDOMWin == nsnull) {
    return nsnull;
  }

  nsCOMPtr<nsIDOMWindowInternal> domWin(GetDOMWinForWebShell(aPO->mWebShell));
  if (domWin != nsnull && domWin.get() == aDOMWin) {
    return aPO;
  }

  PRInt32 cnt = aPO->mKids.Count();
  for (PRInt32 i=0;i<cnt;i++) {
    PrintObject* po = FindPrintObjectByDOMWin((PrintObject*)aPO->mKids[i], aDOMWin);
    if (po != nsnull) {
      return po;
    }
  }
  return nsnull;
}


//-------------------------------------------------------
nsresult
DocumentViewerImpl::PrintContent(nsIWebShell *      aParent,
                                 nsIDeviceContext * aDContext,
                                 nsIDOMWindow     * aDOMWin,
                                 PRBool             aIsSubDoc)
{
  // XXX Once we get printing for plugins going we will
  // have to revist this method.
  NS_ASSERTION(0, "Still may be needed for plugins");

  return NS_ERROR_FAILURE;
}

//-------------------------------------------------------
// return the DOMWindowInternal for a WebShell
nsIDOMWindowInternal *
DocumentViewerImpl::GetDOMWinForWebShell(nsIWebShell* aWebShell)
{
  NS_ASSERTION(aWebShell, "Pointer is null!");

  nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(aWebShell));
  NS_ASSERTION(docShell, "The DocShell can't be NULL!");
  if (!docShell) return nsnull;

  nsCOMPtr<nsIPresShell>  presShell;
  docShell->GetPresShell(getter_AddRefs(presShell));
  NS_ASSERTION(presShell, "The PresShell can't be NULL!");
  if (!presShell) return nsnull;

  nsCOMPtr<nsIDocument> document;
  presShell->GetDocument(getter_AddRefs(document));
  if (!document) return nsnull;
  
  nsCOMPtr<nsIScriptGlobalObject> scriptGlobalObject;
  document->GetScriptGlobalObject(getter_AddRefs(scriptGlobalObject));
  if (!scriptGlobalObject) return nsnull;

  nsCOMPtr<nsIDOMWindowInternal> domWin = do_QueryInterface(scriptGlobalObject); 
  if (!domWin) return nsnull;

  nsIDOMWindowInternal * dw = domWin.get();
  NS_ADDREF(dw);
  return dw;
}

//-------------------------------------------------------
nsresult
DocumentViewerImpl::EnablePOsForPrinting()
{
  // NOTE: All POs have been "turned off" for printing
  // this is where we decided which POs get printed.
  mPrt->mSelectedPO = nsnull;

  if (mPrt->mPrintSettings == nsnull) {
    return NS_ERROR_FAILURE;
  }

  mPrt->mPrintFrameType = nsIPrintSettings::kNoFrames; 
  mPrt->mPrintSettings->GetPrintFrameType(&mPrt->mPrintFrameType);

  PRInt16 printHowEnable = nsIPrintSettings::kFrameEnableNone; 
  mPrt->mPrintSettings->GetHowToEnableFrameUI(&printHowEnable);

  PRInt16 printRangeType = nsIPrintSettings::kRangeAllPages;
  mPrt->mPrintSettings->GetPrintRange(&printRangeType);

  PRINT_DEBUG_MSG1("\n********* DocumentViewerImpl::EnablePOsForPrinting *********\n");
  PRINT_DEBUG_MSG2("PrintFrameType:     %s \n", gPrintFrameTypeStr[mPrt->mPrintFrameType]);
  PRINT_DEBUG_MSG2("HowToEnableFrameUI: %s \n", gFrameHowToEnableStr[printHowEnable]);
  PRINT_DEBUG_MSG2("PrintRange:         %s \n", gPrintRangeStr[printRangeType]);
  PRINT_DEBUG_MSG1("----\n");
  PRINT_DEBUG_FLUSH

  // ***** This is the ultimate override *****
  // if we are printing the selection (either an IFrame or selection range)
  // then set the mPrintFrameType as if it were the selected frame
  if (printRangeType == nsIPrintSettings::kRangeSelection) {
    mPrt->mPrintFrameType = nsIPrintSettings::kSelectedFrame;
    printHowEnable        = nsIPrintSettings::kFrameEnableNone; 
  }

  // This tells us that the "Frame" UI has turned off,
  // so therefore there are no FrameSets/Frames/IFrames to be printed
  //
  // This means there are not FrameSets, 
  // but the document could contain an IFrame
  if (printHowEnable == nsIPrintSettings::kFrameEnableNone) {

    // Print all the pages or a sub range of pages
    if (printRangeType == nsIPrintSettings::kRangeAllPages ||
        printRangeType == nsIPrintSettings::kRangeSpecifiedPageRange) {
      SetPrintPO(mPrt->mPrintObject, PR_TRUE);

      // Set the children so they are PrinAsIs
      // In this case, the children are probably IFrames
      if (mPrt->mPrintObject->mKids.Count() > 0) {
        for (PRInt32 i=0;i<mPrt->mPrintObject->mKids.Count();i++) {
          PrintObject* po = (PrintObject*)mPrt->mPrintObject->mKids[i];
          NS_ASSERTION(po, "PrintObject can't be null!");
          SetPrintAsIs(po);
        }

        // ***** Another override *****
        mPrt->mPrintFrameType = nsIPrintSettings::kFramesAsIs;
      }
      PRINT_DEBUG_MSG2("PrintFrameType:     %s \n", gPrintFrameTypeStr[mPrt->mPrintFrameType]);
      PRINT_DEBUG_MSG2("HowToEnableFrameUI: %s \n", gFrameHowToEnableStr[printHowEnable]);
      PRINT_DEBUG_MSG2("PrintRange:         %s \n", gPrintRangeStr[printRangeType]);
      return NS_OK;
    }
    
    // This means we are either printed a selected IFrame or
    // we are printing the current selection
    if (printRangeType == nsIPrintSettings::kRangeSelection) {

      // If the currentFocusDOMWin can'r be null if something is selected
      if (mPrt->mCurrentFocusWin) {
        // Find the selected IFrame
        PrintObject * po = FindPrintObjectByDOMWin(mPrt->mPrintObject, mPrt->mCurrentFocusWin);
        if (po != nsnull) {
          mPrt->mSelectedPO = po;
          // Makes sure all of its children are be printed "AsIs"
          SetPrintAsIs(po);

          // Now, only enable this POs (the selected PO) and all of its children
          SetPrintPO(po, PR_TRUE);

          // check to see if we have a range selection, 
          // as oppose to a insert selection
          // this means if the user just clicked on the IFrame then 
          // there will not be a selection so we want the entire page to print
          //
          // XXX this is sort of a hack right here to make the page 
          // not try to reposition itself when printing selection
          nsCOMPtr<nsIDOMWindowInternal> domWin = getter_AddRefs(GetDOMWinForWebShell(po->mWebShell));
          if (!IsThereARangeSelection(domWin)) {
            printRangeType = nsIPrintSettings::kRangeAllPages;
            mPrt->mPrintSettings->SetPrintRange(printRangeType);
          }
          PRINT_DEBUG_MSG2("PrintFrameType:     %s \n", gPrintFrameTypeStr[mPrt->mPrintFrameType]);
          PRINT_DEBUG_MSG2("HowToEnableFrameUI: %s \n", gFrameHowToEnableStr[printHowEnable]);
          PRINT_DEBUG_MSG2("PrintRange:         %s \n", gPrintRangeStr[printRangeType]);
          return NS_OK;
        }
      } else {
        for (PRInt32 i=0;i<mPrt->mPrintDocList->Count();i++) {
          PrintObject* po = (PrintObject*)mPrt->mPrintDocList->ElementAt(i);
          NS_ASSERTION(po, "PrintObject can't be null!");
          nsCOMPtr<nsIDOMWindowInternal> domWin = getter_AddRefs(GetDOMWinForWebShell(po->mWebShell));
          if (IsThereARangeSelection(domWin)) {
            mPrt->mCurrentFocusWin = domWin;
            SetPrintPO(po, PR_TRUE);
            break;
          }
        }
        return NS_OK;
      }
    }
  }

  // check to see if there is a selection when a FrameSet is present
  if (printRangeType == nsIPrintSettings::kRangeSelection) {
    // If the currentFocusDOMWin can'r be null if something is selected
    if (mPrt->mCurrentFocusWin) {
      // Find the selected IFrame
      PrintObject * po = FindPrintObjectByDOMWin(mPrt->mPrintObject, mPrt->mCurrentFocusWin);
      if (po != nsnull) {
        mPrt->mSelectedPO = po;
        // Makes sure all of its children are be printed "AsIs"
        SetPrintAsIs(po);

        // Now, only enable this POs (the selected PO) and all of its children
        SetPrintPO(po, PR_TRUE);

        // check to see if we have a range selection, 
        // as oppose to a insert selection
        // this means if the user just clicked on the IFrame then 
        // there will not be a selection so we want the entire page to print
        //
        // XXX this is sort of a hack right here to make the page 
        // not try to reposition itself when printing selection
        nsCOMPtr<nsIDOMWindowInternal> domWin = getter_AddRefs(GetDOMWinForWebShell(po->mWebShell));
        if (!IsThereARangeSelection(domWin)) {
          printRangeType = nsIPrintSettings::kRangeAllPages;
          mPrt->mPrintSettings->SetPrintRange(printRangeType);
        }
        PRINT_DEBUG_MSG2("PrintFrameType:     %s \n", gPrintFrameTypeStr[mPrt->mPrintFrameType]);
        PRINT_DEBUG_MSG2("HowToEnableFrameUI: %s \n", gFrameHowToEnableStr[printHowEnable]);
        PRINT_DEBUG_MSG2("PrintRange:         %s \n", gPrintRangeStr[printRangeType]);
        return NS_OK;
      }
    }
  }

  // If we are printing "AsIs" then sets all the POs to be printed as is
  if (mPrt->mPrintFrameType == nsIPrintSettings::kFramesAsIs) {
    SetPrintAsIs(mPrt->mPrintObject);
    SetPrintPO(mPrt->mPrintObject, PR_TRUE);
    return NS_OK;
  }

  // If we are printing the selected Frame then
  // find that PO for that selected DOMWin and set it all of its
  // children to be printed
  if (mPrt->mPrintFrameType == nsIPrintSettings::kSelectedFrame) {

    if ((mPrt->mIsParentAFrameSet && mPrt->mCurrentFocusWin) || mPrt->mIsIFrameSelected) {
      PrintObject * po = FindPrintObjectByDOMWin(mPrt->mPrintObject, mPrt->mCurrentFocusWin);
      if (po != nsnull) {
        mPrt->mSelectedPO = po;
        // NOTE: Calling this sets the "po" and 
        // we don't want to do this for documents that have no children,
        // because then the "DoEndPage" gets called and it shouldn't 
        if (po->mKids.Count() > 0) {
          // Makes sure that itself, and all of its children are printed "AsIs"
          SetPrintAsIs(po);
        }

        // Now, only enable this POs (the selected PO) and all of its children
        SetPrintPO(po, PR_TRUE);
      }
    }
    return NS_OK;
  } 
  
  // If we are print each subdoc separately, 
  // then don't print any of the FraneSet Docs 
  if (mPrt->mPrintFrameType == nsIPrintSettings::kEachFrameSep) {
    SetPrintPO(mPrt->mPrintObject, PR_TRUE);
    PRInt32 cnt = mPrt->mPrintDocList->Count();
    for (PRInt32 i=0;i<cnt;i++) {
      PrintObject* po = (PrintObject*)mPrt->mPrintDocList->ElementAt(i);
      NS_ASSERTION(po, "PrintObject can't be null!");
      if (po->mFrameType == eFrameSet) {
        po->mDontPrint = PR_TRUE;
      }
    }
  }

  return NS_OK;
}

//-------------------------------------------------------
nsresult
DocumentViewerImpl::SetupToPrintContent(nsIWebShell*          aParent,
                                        nsIDeviceContext*     aDContext,
                                        nsIDOMWindowInternal* aCurrentFocusedDOMWin)
{

  NS_ENSURE_ARG_POINTER(aParent);
  NS_ENSURE_ARG_POINTER(aDContext);
  // NOTE: aCurrentFocusedDOMWin may be null (which is OK)

  mPrt->mPrintDocDC = aDContext;

  // In this step we figure out which documents should be printed
  // i.e. if we are printing the selection then only enable that PrintObject
  // for printing
  if (NS_FAILED(EnablePOsForPrinting())) {
    return NS_ERROR_FAILURE;
  }
  DUMP_DOC_LIST("\nAfter Enable------------------------------------------");

  // Here we reflow all the PrintObjects
  if (NS_FAILED(ReflowDocList(mPrt->mPrintObject))) {
    return NS_ERROR_FAILURE;
  }

  DUMP_DOC_LIST("\nAfter Reflow------------------------------------------");
  PRINT_DEBUG_MSG1("\n-------------------------------------------------------\n\n");

  // Set up the clipping rectangle for all documents
  // When frames are being printed as part of a frame set and also IFrames, 
  // they are reflowed with a very large page height. We need to setup the
  // clipping so they do not rpint over top of anything else
  PRINT_DEBUG_MSG1("SetClipRect-------------------------------------------------------\n");
  nsRect clipRect(-1,-1,-1, -1);
  SetClipRect(mPrt->mPrintObject, clipRect, 0, 0, PR_FALSE);

  CalcNumPrintableDocsAndPages(mPrt->mNumPrintableDocs, mPrt->mNumPrintablePages);

  PRINT_DEBUG_MSG3("--- Printing %d docs and %d pages\n", mPrt->mNumPrintableDocs, mPrt->mNumPrintablePages);
  DUMP_DOC_TREELAYOUT;
  PRINT_DEBUG_FLUSH;

#ifdef DEBUG_PRINTING_X
#if defined(XP_PC)
  for (PRInt32 i=0;i<mPrt->mPrintDocList->Count();i++) {
    PrintObject* po = (PrintObject*)mPrt->mPrintDocList->ElementAt(i);
    NS_ASSERTION(po, "PrintObject can't be null!");
    if (po->mPresShell) {
      nsIPageSequenceFrame* pageSequence;
      po->mPresShell->GetPageSequenceFrame(&pageSequence);
      if (pageSequence != nsnull) {
        // install the debugging file pointer
        nsSimplePageSequenceFrame * sf = NS_STATIC_CAST(nsSimplePageSequenceFrame*, pageSequence);
        sf->SetDebugFD(mPrt->mDebugFD);
      }
    }
  }
#endif
#endif

  mPrt->mPrintDocDC = aDContext;
  mPrt->mPrintDocDW = aCurrentFocusedDOMWin;

  // This will print the webshell document
  // when it completes asynchronously in the DonePrintingPages method
  // it will check to see if there are more webshells to be printed and 
  // then PrintDocContent will be called again.

  nsCOMPtr<nsIWebShell> webContainer(do_QueryInterface(mContainer));
  PRUnichar * docTitleStr;
  PRUnichar * docURLStr;
  GetWebShellTitleAndURL(webContainer, &docTitleStr, &docURLStr); 

  if (!docTitleStr) {
    if (docURLStr) {
      docTitleStr = docURLStr;
      docURLStr   = nsnull;
    } else {
      docTitleStr = ToNewUnicode(NS_LITERAL_STRING("Document"));
    }
  }

  nsresult rv = NS_OK;
  if (mIsDoingPrinting) {
    // BeginDocument may pass back a FAILURE code
    // i.e. On Windows, if you are printing to a file and hit "Cancel" 
    //      to the "File Name" dialog, this comes back as an error
    // Don't start printing when regression test are executed  
    rv = mPrt->mDebugFilePtr ? NS_OK: mPrt->mPrintDC->BeginDocument(docTitleStr);
    PRINT_DEBUG_MSG1("****************** Begin Document ************************\n");

    if (docTitleStr != nsnull) {
      nsMemory::Free(docTitleStr);
    }
    if (docURLStr != nsnull) {
      nsMemory::Free(docURLStr);
    }

    if (NS_FAILED(rv)) {
      return rv;
    }

    PrintDocContent(mPrt->mPrintObject, rv); // ignore return value
  }

  return rv;
}

//-------------------------------------------------------
nsresult
DocumentViewerImpl::DoPrint(PrintObject * aPO, PRBool aDoSyncPrinting, PRBool& aDonePrinting)
{
  NS_ASSERTION(mPrt->mPrintDocList, "Pointer is null!");

  PRINT_DEBUG_MSG2("\n**************************** %s ****************************\n", gFrameTypesStr[aPO->mFrameType]);
  PRINT_DEBUG_MSG3("****** In DV::DoPrint   PO: %p aDoSyncPrinting: %s \n", aPO, PRT_YESNO(aDoSyncPrinting));

  nsIWebShell*    webShell      = aPO->mWebShell.get();
  nsIPresShell*   poPresShell   = aPO->mPresShell;
  nsIPresContext* poPresContext = aPO->mPresContext;
  nsIView*        poRootView    = aPO->mRootView;

  nsCOMPtr<nsIWebShell> webContainer(do_QueryInterface(mContainer));
  NS_ASSERTION(webShell, "The WebShell can't be NULL!");

  if (mPrt->mPrintProgressParams) {
    SetDocAndURLIntoProgress(aPO->mWebShell, mPrt->mPrintProgressParams);
  }

  if (webShell != nsnull) {

    PRInt16 printRangeType = nsIPrintSettings::kRangeAllPages;
    nsresult rv;
    if (mPrt->mPrintSettings != nsnull) {
      mPrt->mPrintSettings->GetPrintRange(&printRangeType);
    }

    // Ask the page sequence frame to print all the pages
    nsIPageSequenceFrame* pageSequence;
    poPresShell->GetPageSequenceFrame(&pageSequence);
    NS_ASSERTION(nsnull != pageSequence, "no page sequence frame");

    // Now, depending how we are printing and what type of doc we are printing
    // we must configure the sequencer correctly.
    // so we are about to be very explicit about the whole process

    PRBool skipPageEjectOnly      = PR_FALSE;
    PRBool skipAllPageAdjustments = PR_FALSE;
    PRBool doOffsetting           = PR_FALSE;
    PRBool doAddInParentsOffset   = PR_TRUE;
    PRBool skipSetTitle           = PR_FALSE;

    if (aPO->mFrameType == eFrame) {
      switch (mPrt->mPrintFrameType) {
      case nsIPrintSettings::kFramesAsIs: 
          skipAllPageAdjustments = PR_TRUE;
          doOffsetting           = PR_TRUE;
          break;

        case nsIPrintSettings::kSelectedFrame:
          if (aPO->mKids.Count() > 0) {
            skipPageEjectOnly = PR_TRUE;
          }
          break;

        case nsIPrintSettings::kEachFrameSep:
          if (aPO->mKids.Count() > 0) {
            skipPageEjectOnly = PR_TRUE;
          }
          break;
      } // switch

    } else if (aPO->mFrameType == eIFrame) {
      switch (mPrt->mPrintFrameType) {
        case nsIPrintSettings::kFramesAsIs:
          skipAllPageAdjustments = PR_TRUE;
          doOffsetting           = PR_TRUE;
          skipSetTitle           = PR_TRUE;
          break;

        case nsIPrintSettings::kSelectedFrame:
          if (aPO != mPrt->mSelectedPO) {
            skipAllPageAdjustments = PR_TRUE;
            doOffsetting           = PR_TRUE;
            doAddInParentsOffset   = aPO->mParent != nsnull && aPO->mParent->mFrameType == eIFrame && aPO->mParent != mPrt->mSelectedPO;
            skipSetTitle           = PR_TRUE;
          } else {
            skipPageEjectOnly = aPO->mKids.Count() > 0;
          }
          break;

        case nsIPrintSettings::kEachFrameSep:
          skipAllPageAdjustments = PR_TRUE;
          doOffsetting           = PR_TRUE;
          doAddInParentsOffset   = aPO->mParent != nsnull && aPO->mParent->mFrameType == eIFrame;
          skipSetTitle           = PR_TRUE;
          break;
      } // switch
    } else {
      // FrameSets skip page eject only if printing AsIs
      skipPageEjectOnly = aPO->mPrintAsIs;
    }

    // That we are all configured, 
    // let's set everything up to print
    if (skipPageEjectOnly) {
      pageSequence->SkipPageEnd();
      aPO->mSkippedPageEject = PR_TRUE;

    } else {

      if (skipAllPageAdjustments) {
        pageSequence->SuppressHeadersAndFooters(PR_TRUE);
        pageSequence->SkipPageBegin();
        pageSequence->SkipPageEnd();
        aPO->mSkippedPageEject = PR_TRUE;
      } else {
        aPO->mSkippedPageEject = PR_FALSE;
      }

      if (doOffsetting) {
        nscoord x = 0;
        nscoord y = 0;
        PrintObject * po = aPO;
        while (po != nsnull) {
          //if (mPrt->mPrintFrameType != nsIPrintSettings::kSelectedFrame || po != aPO->mParent) {
          PRBool isParent = po == aPO->mParent;
          if (!isParent || (isParent && doAddInParentsOffset)) {
            x += po->mRect.x;
            y += po->mRect.y;
          }
          po = po->mParent;
        }
        pageSequence->SetOffset(x, y);
      }
    }

    PRINT_DEBUG_MSG5("*** skipPageEjectOnly: %s  skipAllPageAdjustments: %s  doOffsetting: %s  doAddInParentsOffset: %s\n", 
                      PRT_YESNO(skipPageEjectOnly), PRT_YESNO(skipAllPageAdjustments), 
                      PRT_YESNO(doOffsetting), PRT_YESNO(doAddInParentsOffset));

    // mPrt->mDebugFilePtr this is onlu non-null when compiled for debugging
    if (nsnull != mPrt->mDebugFilePtr) {
#ifdef NS_DEBUG
      // output the regression test
      nsIFrameDebug* fdbg;
      nsIFrame* root;
      poPresShell->GetRootFrame(&root);

      if (NS_SUCCEEDED(CallQueryInterface(root, &fdbg))) {
        fdbg->DumpRegressionData(poPresContext, mPrt->mDebugFilePtr, 0, PR_TRUE);
      }
      fclose(mPrt->mDebugFilePtr);      
#endif
    } else {
      nsIFrame* rootFrame;
      poPresShell->GetRootFrame(&rootFrame);

#if defined(DEBUG_rods) || defined(DEBUG_dconeX)
      {
        char * docStr;
        char * urlStr;
        GetDocTitleAndURL(aPO, docStr, urlStr);
        DumpLayoutData(docStr, urlStr, poPresContext, mPrt->mPrintDocDC, rootFrame, webShell);
        if (docStr) nsMemory::Free(docStr);
        if (urlStr) nsMemory::Free(urlStr);
      }
#endif

      if (mPrt->mPrintSettings) {
        if (!skipSetTitle) {
          PRUnichar * docTitleStr;
          PRUnichar * docURLStr;
          GetWebShellTitleAndURL(webShell, &docTitleStr, &docURLStr); 

          if (!docTitleStr) {
            docTitleStr = ToNewUnicode(NS_LITERAL_STRING(""));
          }

          if (docTitleStr) {
            mPrt->mPrintSettings->SetTitle(docTitleStr);
            nsMemory::Free(docTitleStr);
          }

          if (docURLStr) {
            mPrt->mPrintSettings->SetDocURL(docURLStr);
            nsMemory::Free(docURLStr);
          }
        }

        if (nsIPrintSettings::kRangeSelection == printRangeType) {
          poPresContext->SetIsRenderingOnlySelection(PR_TRUE);
          // temporarily creating rendering context
          // which is needed to dinf the selection frames
          nsCOMPtr<nsIRenderingContext> rc;
          mPrt->mPrintDocDC->CreateRenderingContext(*getter_AddRefs(rc));

          // find the starting and ending page numbers
          // via the selection
          nsIFrame* startFrame;
          nsIFrame* endFrame;
          PRInt32   startPageNum;
          PRInt32   endPageNum;
          nsRect    startRect;
          nsRect    endRect;

          nsCOMPtr<nsISelection> selectionPS;
          nsresult rvv = GetDocumentSelection(getter_AddRefs(selectionPS), poPresShell);
          if (NS_SUCCEEDED(rvv) && selectionPS) {
          }

          rv = GetPageRangeForSelection(poPresShell, poPresContext, *rc, selectionPS, pageSequence, 
                                        &startFrame, startPageNum, startRect, 
                                        &endFrame, endPageNum, endRect);
          if (NS_SUCCEEDED(rv)) {
            mPrt->mPrintSettings->SetStartPageRange(startPageNum);
            mPrt->mPrintSettings->SetEndPageRange(endPageNum);
            nsMargin margin(0,0,0,0);
            mPrt->mPrintSettings->GetMarginInTwips(margin);

            if (startPageNum == endPageNum) {
              nsIFrame * seqFrame;
              if (NS_FAILED(CallQueryInterface(pageSequence, &seqFrame))) {
                mIsDoingPrinting = PR_FALSE;
                return NS_ERROR_FAILURE;
              }
              nsRect rect(0,0,0,0);
              nsRect areaRect;
              nsIFrame * areaFrame = FindFrameByType(poPresContext, startFrame, nsHTMLAtoms::body, rect, areaRect);
              if (areaFrame) {
                startRect.y -= margin.top;
                endRect.y   -= margin.top;
                areaRect.y -= startRect.y;
                areaRect.x -= margin.left;
                // XXX This is temporary fix for printing more than one page of a selection
                pageSequence->SetSelectionHeight(startRect.y, endRect.y+endRect.height-startRect.y);
              }
            }
          }
        }

        nsCOMPtr<nsIPrintPreviewContext> ppContext = do_QueryInterface(poPresContext);
        if (!ppContext) {
          nsIFrame * seqFrame;
          if (NS_FAILED(CallQueryInterface(pageSequence, &seqFrame))) {
            mIsDoingPrinting = PR_FALSE;
            return NS_ERROR_FAILURE;
          }

          nsRect srect;
          seqFrame->GetRect(srect);

          nsRect r;
          poRootView->GetBounds(r);
          r.x = r.y = 0;
          r.height = srect.height;
          aPO->mViewManager->ResizeView(poRootView, r, PR_FALSE);

          rootFrame->GetRect(r);

          r.height = srect.height;
          rootFrame->SetRect(poPresContext, r);

          mPageSeqFrame = pageSequence;
          mPageSeqFrame->StartPrint(poPresContext, mPrt->mPrintSettings);

          if (!aDoSyncPrinting) {
            // Get the delay time in between the printing of each page
            // this gives the user more time to press cancel
            PRInt32 printPageDelay = 500;
            mPrt->mPrintSettings->GetPrintPageDelay(&printPageDelay);

            // Schedule Page to Print
            PRINT_DEBUG_MSG3("Scheduling Print of PO: %p (%s) \n", aPO, gFrameTypesStr[aPO->mFrameType]);
            StartPagePrintTimer(poPresContext, mPrt->mPrintSettings, aPO, printPageDelay);
          } else {
            DoProgressForAsIsFrames();
            // Print the page synchronously
            PRINT_DEBUG_MSG3("Async Print of PO: %p (%s) \n", aPO, gFrameTypesStr[aPO->mFrameType]);
            aDonePrinting = PrintPage(poPresContext, mPrt->mPrintSettings, aPO);
          }
        }
      } else {
        // not sure what to do here!
        mIsDoingPrinting = PR_FALSE;
        return NS_ERROR_FAILURE;
      }
    }
  } else {
    aPO->mDontPrint = PR_TRUE;
    aDonePrinting = PR_FALSE;
  }

  return NS_OK;
}

//-------------------------------------------------------
// Figure out how many documents and how many total pages we are printing
void
DocumentViewerImpl::CalcNumPrintableDocsAndPages(PRInt32& aNumDocs, PRInt32& aNumPages)
{
  aNumPages = 0;
  // Count the number of printable documents
  // and printable pages
  PRInt32 numOfPrintableDocs = 0;
  PRInt32 i;
  for (i=0;i<mPrt->mPrintDocList->Count();i++) {
    PrintObject* po = (PrintObject*)mPrt->mPrintDocList->ElementAt(i);
    NS_ASSERTION(po, "PrintObject can't be null!");
    if (po->IsPrintable()) {
      if (po->mPresShell &&
          po->mFrameType != eIFrame &&
          po->mFrameType != eFrameSet) {
        nsIPageSequenceFrame* pageSequence;
        po->mPresShell->GetPageSequenceFrame(&pageSequence);
        nsIFrame * seqFrame;
        if (NS_SUCCEEDED(CallQueryInterface(pageSequence, &seqFrame))) {
          nsIFrame* frame;
          seqFrame->FirstChild(po->mPresContext, nsnull, &frame);
          while (frame) {
            aNumPages++;
            frame->GetNextSibling(&frame);
          }
        }
      }

      numOfPrintableDocs++;
    }
  }
}

//-------------------------------------------------------
void
DocumentViewerImpl::DoProgressForAsIsFrames()
{
  // mPrintFrameType is set to kFramesAsIs event though the Doc Type maybe eDoc
  // this is done to make the printing of embedded IFrames easier
  // NOTE: we don't want to advance the progress in that case, it is down elsewhere
  if (mPrt->mPrintFrameType == nsIPrintSettings::kFramesAsIs && mPrt->mPrintObject->mFrameType != eDoc) {
    mPrt->mNumDocsPrinted++;
    PrintData::DoOnProgressChange(mPrt->mPrintProgressListeners, mPrt->mNumDocsPrinted, mPrt->mNumPrintableDocs);
  }
}

//-------------------------------------------------------
void
DocumentViewerImpl::DoProgressForSeparateFrames()
{
  if (mPrt->mPrintFrameType == nsIPrintSettings::kEachFrameSep) {
    mPrt->mNumPagesPrinted++;
    // notify the listener of printed docs
    PrintData::DoOnProgressChange(mPrt->mPrintProgressListeners, mPrt->mNumPagesPrinted+1, mPrt->mNumPrintablePages);
  }
}

//-------------------------------------------------------
// Called for each WebShell that needs to be printed
PRBool
DocumentViewerImpl::PrintDocContent(PrintObject* aPO, nsresult& aStatus)
{
  NS_ASSERTION(aPO, "Pointer is null!");


  if (!aPO->mHasBeenPrinted && aPO->IsPrintable()) {
    PRBool donePrinting;
    // donePrinting is only valid when when doing synchronous printing
    aStatus = DoPrint(aPO, PR_FALSE, donePrinting); 
    if (donePrinting) {
      return PR_TRUE;
    }
  }

  for (PRInt32 i=0;i<aPO->mKids.Count();i++) {
    PrintObject* po = (PrintObject*)aPO->mKids[i];
    PRBool printed = PrintDocContent(po, aStatus);
    if (printed || NS_FAILED(aStatus)) {
      return PR_TRUE;
    }
  } 
  return PR_FALSE; 
}

NS_IMETHODIMP
DocumentViewerImpl::SetEnableRendering(PRBool aOn)
{
  NS_ENSURE_TRUE(mDocument, NS_ERROR_NOT_AVAILABLE);
  mEnableRendering = aOn;
  if (mViewManager) {
    if (aOn) {
      mViewManager->EnableRefresh(NS_VMREFRESH_IMMEDIATE);
      nsIView* view; 
      mViewManager->GetRootView(view);   // views are not refCounted 
      if (view) { 
        mViewManager->UpdateView(view, NS_VMREFRESH_IMMEDIATE);
      } 
    }
    else {
      mViewManager->DisableRefresh();
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
DocumentViewerImpl::GetEnableRendering(PRBool* aResult)
{
  NS_ENSURE_TRUE(mDocument, NS_ERROR_NOT_AVAILABLE);
  NS_PRECONDITION(nsnull != aResult, "null OUT ptr");
  if (aResult) {
    *aResult = mEnableRendering;
  }
  return NS_OK;
}

void
DocumentViewerImpl::ForceRefresh()
{
  mWindow->Invalidate(PR_TRUE);
}

nsresult
DocumentViewerImpl::CreateStyleSet(nsIDocument* aDocument,
                                   nsIStyleSet** aStyleSet)
{
  // this should eventually get expanded to allow for creating
  // different sets for different media
  nsresult rv;

  if (!mUAStyleSheet) {
    NS_WARNING("unable to load UA style sheet");
  }

  rv = CallCreateInstance(kStyleSetCID, aStyleSet);
  if (NS_OK == rv) {
    PRInt32 index = 0;
    aDocument->GetNumberOfStyleSheets(&index);

    while (0 < index--) {
      nsCOMPtr<nsIStyleSheet> sheet;
      aDocument->GetStyleSheetAt(index, getter_AddRefs(sheet));

      /*
       * GetStyleSheetAt will return all style sheets in the document but
       * we're only interested in the ones that are enabled.
       */

      PRBool styleEnabled;
      sheet->GetEnabled(styleEnabled);

      if (styleEnabled) {
        (*aStyleSet)->AddDocStyleSheet(sheet, aDocument);
      }
    }

    nsCOMPtr<nsIChromeRegistry> chromeRegistry = 
             do_GetService("@mozilla.org/chrome/chrome-registry;1", &rv);
    if (NS_SUCCEEDED(rv) && chromeRegistry) {
      nsCOMPtr<nsISupportsArray> sheets;

      // Now handle the user sheets.
      nsCOMPtr<nsIDocShellTreeItem> docShell(do_QueryInterface(mContainer));
      PRInt32 shellType;
      docShell->GetItemType(&shellType);
      PRBool isChrome = (shellType == nsIDocShellTreeItem::typeChrome);
      sheets = nsnull;
      chromeRegistry->GetUserSheets(isChrome, getter_AddRefs(sheets));
      if(sheets){
        nsCOMPtr<nsICSSStyleSheet> sheet;
        PRUint32 count;
        sheets->Count(&count);
        for(PRUint32 i=0; i<count; i++) {
          sheets->GetElementAt(i, getter_AddRefs(sheet));
          (*aStyleSet)->AppendUserStyleSheet(sheet);
        }
      }

      // Append chrome sheets (scrollbars + forms).
      nsCOMPtr<nsIDocShell> ds(do_QueryInterface(mContainer));
      chromeRegistry->GetAgentSheets(ds, getter_AddRefs(sheets));
      if(sheets){
        nsCOMPtr<nsICSSStyleSheet> sheet;
        PRUint32 count;
        sheets->Count(&count);
        for(PRUint32 i=0; i<count; i++) {
          sheets->GetElementAt(i, getter_AddRefs(sheet));
          (*aStyleSet)->AppendAgentStyleSheet(sheet);
        }
      }
    }

    if (mUAStyleSheet) {
      (*aStyleSet)->AppendAgentStyleSheet(mUAStyleSheet);
    }
  }
  return NS_OK;
}

nsresult
DocumentViewerImpl::MakeWindow(nsIWidget* aParentWidget,
                               const nsRect& aBounds)
{
  nsresult rv;

  mViewManager = do_CreateInstance(kViewManagerCID, &rv);
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIDeviceContext> dx;
  mPresContext->GetDeviceContext(getter_AddRefs(dx));

 
  nsRect tbounds = aBounds;
  float p2t;
  mPresContext->GetPixelsToTwips(&p2t);
  tbounds *= p2t;

   // Initialize the view manager with an offset. This allows the viewmanager
   // to manage a coordinate space offset from (0,0)
  rv = mViewManager->Init(dx);
  if (NS_FAILED(rv))
    return rv;
  rv = mViewManager->SetWindowOffset(tbounds.x, tbounds.y);
  if (NS_FAILED(rv))
    return rv;

   // Reset the bounds offset so the root view is set to 0,0. The offset is
   // specified in nsIViewManager::Init above. 
   // Besides, layout will reset the root view to (0,0) during reflow, 
   // so changing it to 0,0 eliminates placing
   // the root view in the wrong place initially.
  tbounds.x = 0;
  tbounds.y = 0;

  // Create a child window of the parent that is our "root view/window"
  // Create a view
  rv = CallCreateInstance(kViewCID, &mView);
  if (NS_FAILED(rv))
    return rv;
  rv = mView->Init(mViewManager, tbounds, nsnull);
  if (NS_FAILED(rv))
    return rv;

  rv = mView->CreateWidget(kWidgetCID, nsnull,
                           aParentWidget->GetNativeData(NS_NATIVE_WIDGET),
                           PR_TRUE, PR_FALSE);

  if (rv != NS_OK)
    return rv;

  // Setup hierarchical relationship in view manager
  mViewManager->SetRootView(mView);

  mView->GetWidget(*getter_AddRefs(mWindow));

  // This SetFocus is necessary so the Arrow Key and Page Key events
  // go to the scrolled view as soon as the Window is created instead of going to
  // the browser window (this enables keyboard scrolling of the document)
  // mWindow->SetFocus();

  return rv;
}

nsresult DocumentViewerImpl::GetDocumentSelection(nsISelection **aSelection, nsIPresShell * aPresShell)
{
  if (aPresShell == nsnull) {
    if (!mPresShell) {
      return NS_ERROR_NOT_INITIALIZED;
    }
    aPresShell = mPresShell;
  }
  if (!aSelection) return NS_ERROR_NULL_POINTER;
  if (!aPresShell) return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsISelectionController> selcon;
  selcon = do_QueryInterface(aPresShell);
  if (selcon) 
    return selcon->GetSelection(nsISelectionController::SELECTION_NORMAL, aSelection);  
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
DocumentViewerImpl::CreateDocumentViewerUsing(nsIPresContext* aPresContext,
                                              nsIDocumentViewer*& aResult)
{
  if (!mDocument) {
    // XXX better error
    return NS_ERROR_NULL_POINTER;
  }
  if (nsnull == aPresContext) {
    return NS_ERROR_NULL_POINTER;
  }

  // Create new viewer
  DocumentViewerImpl* viewer = new DocumentViewerImpl(aPresContext);
  if (nsnull == viewer) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  NS_ADDREF(viewer);

  // XXX make sure the ua style sheet is used (for now; need to be
  // able to specify an alternate)
  viewer->SetUAStyleSheet(mUAStyleSheet);

  // Bind the new viewer to the old document
  nsresult rv = viewer->LoadStart(mDocument);
  
  aResult = viewer;

  return rv;
}

void PR_CALLBACK DocumentViewerImpl::HandlePLEvent(PLEvent* aEvent)
{
  DocumentViewerImpl *viewer;

  viewer = (DocumentViewerImpl*)PL_GetEventOwner(aEvent);

  NS_ASSERTION(viewer, "The event owner is null.");
  if (viewer) {
    viewer->DocumentReadyForPrinting();
  }
}

void PR_CALLBACK DocumentViewerImpl::DestroyPLEvent(PLEvent* aEvent)
{
  DocumentViewerImpl *viewer;

  viewer = (DocumentViewerImpl*)PL_GetEventOwner(aEvent);
  NS_IF_RELEASE(viewer);

  delete aEvent;
}


nsresult DocumentViewerImpl::DocumentReadyForPrinting()
{
  nsresult rv = NS_ERROR_FAILURE;
  nsCOMPtr<nsIWebShell> webContainer;

  webContainer = do_QueryInterface(mContainer);
  if(webContainer) {
    //
    // Send the document to the printer...
    //
    rv = SetupToPrintContent(webContainer, mPrt->mPrintDC, mPrt->mCurrentFocusWin);
    if (NS_FAILED(rv)) {
      // The print job was canceled or there was a problem
      // So remove all other documents from the print list
      DonePrintingPages(nsnull);
    }
  }
  return rv;
}

NS_IMETHODIMP 
DocumentViewerImpl::SetTransformMediator(nsITransformMediator* aMediator)
{
  NS_ASSERTION(nsnull == mTransformMediator, "nsXMLDocument::SetTransformMediator(): \
    Cannot set a second transform mediator\n");
  mTransformMediator = aMediator;
  return NS_OK;
}

#ifdef XP_MAC
#pragma mark -
#endif

/* ========================================================================================
 * nsIContentViewerEdit
 * ======================================================================================== */

NS_IMETHODIMP DocumentViewerImpl::Search()
{
  NS_ASSERTION(0, "NOT IMPLEMENTED");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP DocumentViewerImpl::GetSearchable(PRBool *aSearchable)
{
  NS_ASSERTION(0, "NOT IMPLEMENTED");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP DocumentViewerImpl::ClearSelection()
{
  nsresult rv;
  nsCOMPtr<nsISelection> selection;

  rv = GetDocumentSelection(getter_AddRefs(selection));
  if (NS_FAILED(rv)) return rv;

  return selection->CollapseToStart();
}

NS_IMETHODIMP DocumentViewerImpl::SelectAll()
{
  // XXX this is a temporary implementation copied from nsWebShell
  // for now. I think nsDocument and friends should have some helper
  // functions to make this easier.
  nsCOMPtr<nsISelection> selection;
  nsresult rv;
  rv = GetDocumentSelection(getter_AddRefs(selection));
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIDOMHTMLDocument> htmldoc = do_QueryInterface(mDocument);
  nsCOMPtr<nsIDOMNode> bodyNode;
  
  if (htmldoc)
  {
    nsCOMPtr<nsIDOMHTMLElement>bodyElement;
    rv = htmldoc->GetBody(getter_AddRefs(bodyElement));
    if (NS_FAILED(rv) || !bodyElement) return rv;

    bodyNode = do_QueryInterface(bodyElement);
  }
  else if (mDocument)
  {
    nsCOMPtr<nsIContent> rootContent;
    mDocument->GetRootContent(getter_AddRefs(rootContent));
    bodyNode = do_QueryInterface(rootContent);
  }
  if (!bodyNode) return NS_ERROR_FAILURE; 
  
  rv = selection->RemoveAllRanges();
  if (NS_FAILED(rv)) return rv;

  rv = selection->SelectAllChildren(bodyNode);
  return rv;
}

NS_IMETHODIMP DocumentViewerImpl::CopySelection()
{
  NS_ENSURE_TRUE(mPresShell, NS_ERROR_NOT_INITIALIZED);
  return mPresShell->DoCopy();
}

NS_IMETHODIMP DocumentViewerImpl::CopyLinkLocation()
{
  NS_ENSURE_TRUE(mPresShell, NS_ERROR_NOT_INITIALIZED);
  nsCOMPtr<nsIDOMNode> node;
  nsresult rv = GetPopupLinkNode(getter_AddRefs(node));
  // make noise if we're not in a link
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(node, NS_ERROR_FAILURE);
  return mPresShell->DoCopyLinkLocation(node);
}

NS_IMETHODIMP DocumentViewerImpl::CopyImageLocation()
{
  NS_ENSURE_TRUE(mPresShell, NS_ERROR_NOT_INITIALIZED);
  nsCOMPtr<nsIDOMNode> node;
  nsresult rv = GetPopupImageNode(getter_AddRefs(node));
  // make noise if we're not in an image
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(node, NS_ERROR_FAILURE);
  return mPresShell->DoCopyImageLocation(node);
}

NS_IMETHODIMP DocumentViewerImpl::CopyImageContents()
{
  NS_ENSURE_TRUE(mPresShell, NS_ERROR_NOT_INITIALIZED);
  nsCOMPtr<nsIDOMNode> node;
  nsresult rv = GetPopupImageNode(getter_AddRefs(node));
  // make noise if we're not in an image
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(node, NS_ERROR_FAILURE);
  return mPresShell->DoCopyImageContents(node);
}

NS_IMETHODIMP DocumentViewerImpl::GetCopyable(PRBool *aCopyable)
{
  nsCOMPtr<nsISelection> selection;
  nsresult rv;
  rv = GetDocumentSelection(getter_AddRefs(selection));
  if (NS_FAILED(rv)) return rv;
  
  PRBool isCollapsed;
  selection->GetIsCollapsed(&isCollapsed);
  
  *aCopyable = !isCollapsed;
  return NS_OK;
}

NS_IMETHODIMP DocumentViewerImpl::CutSelection()
{
  NS_ASSERTION(0, "NOT IMPLEMENTED");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP DocumentViewerImpl::GetCutable(PRBool *aCutable)
{
  *aCutable = PR_FALSE;  // mm, will this ever be called for an editable document?
  return NS_OK;
}

NS_IMETHODIMP DocumentViewerImpl::Paste()
{
  NS_ASSERTION(0, "NOT IMPLEMENTED");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP DocumentViewerImpl::GetPasteable(PRBool *aPasteable)
{
  *aPasteable = PR_FALSE;
  return NS_OK;
}

#ifdef XP_MAC
#pragma mark -
#endif

/* ========================================================================================
 * nsIContentViewerFile
 * ======================================================================================== */
NS_IMETHODIMP
DocumentViewerImpl::Save()
{
  NS_ASSERTION(0, "NOT IMPLEMENTED");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
DocumentViewerImpl::GetSaveable(PRBool *aSaveable)
{
  NS_ASSERTION(0, "NOT IMPLEMENTED");
  return NS_ERROR_NOT_IMPLEMENTED;
}

static NS_DEFINE_IID(kDeviceContextSpecFactoryCID, NS_DEVICE_CONTEXT_SPEC_FACTORY_CID);

nsresult DocumentViewerImpl::GetSelectionDocument(nsIDeviceContextSpec * aDevSpec, nsIDocument ** aNewDoc)
{
  //NS_ENSURE_ARG_POINTER(*aDevSpec);
  NS_ENSURE_ARG_POINTER(aNewDoc);

    // create document
  nsCOMPtr<nsIDocument> doc;
  nsresult rv = NS_NewHTMLDocument(getter_AddRefs(doc));
  if (NS_FAILED(rv)) { return rv; }
  if (!doc) { return NS_ERROR_NULL_POINTER; }

  nsCOMPtr<nsINodeInfoManager> nimgr;
  rv = doc->GetNodeInfoManager(*getter_AddRefs(nimgr));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsINodeInfo> nodeInfo;
  nimgr->GetNodeInfo(nsHTMLAtoms::html, nsnull, kNameSpaceID_None,
                     *getter_AddRefs(nodeInfo));

  // create document content
  nsCOMPtr<nsIHTMLContent> htmlElement;
  nsCOMPtr<nsIHTMLContent> headElement;
  nsCOMPtr<nsIHTMLContent> bodyElement;
    // create the root
  rv = NS_NewHTMLHtmlElement(getter_AddRefs(htmlElement), nodeInfo);
  if (NS_FAILED(rv)) { return rv; }
  if (!htmlElement) { return NS_ERROR_NULL_POINTER; }
    // create the head

  nimgr->GetNodeInfo(NS_LITERAL_STRING("head"), nsnull,
                     kNameSpaceID_None, *getter_AddRefs(nodeInfo));

  rv = NS_NewHTMLHeadElement(getter_AddRefs(headElement), nodeInfo);
  if (NS_FAILED(rv)) { return rv; }
  if (!headElement) { return NS_ERROR_NULL_POINTER; }
  headElement->SetDocument(doc, PR_FALSE, PR_TRUE);
    // create the body

  nimgr->GetNodeInfo(nsHTMLAtoms::body, nsnull, kNameSpaceID_None,
                    *getter_AddRefs(nodeInfo));

  rv = NS_NewHTMLBodyElement(getter_AddRefs(bodyElement), nodeInfo);
  if (NS_FAILED(rv)) { return rv; }
  if (!bodyElement) { return NS_ERROR_NULL_POINTER; }
  bodyElement->SetDocument(doc, PR_FALSE, PR_TRUE);
    // put the head and body into the root
  rv = htmlElement->AppendChildTo(headElement, PR_FALSE, PR_FALSE);
  if (NS_FAILED(rv)) { return rv; }
  rv = htmlElement->AppendChildTo(bodyElement, PR_FALSE, PR_FALSE);
  if (NS_FAILED(rv)) { return rv; }
  
  // load the document into the docshell
  nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(doc);
  if (!domDoc) { return NS_ERROR_NULL_POINTER; }
  nsCOMPtr<nsIDOMElement> htmlDOMElement = do_QueryInterface(htmlElement);
  if (!htmlDOMElement) { return NS_ERROR_NULL_POINTER; }

  //nsCOMPtr<nsIContent> rootContent(do_QueryInterface(htmlElement));
  //doc->SetRootContent(rootContent);

  *aNewDoc = doc.get();
  NS_ADDREF(*aNewDoc);

  return rv;

}

//------------------------------------------------------
PRBool 
DocumentViewerImpl::IsThereAnIFrameSelected(nsIWebShell*           aWebShell, 
                                            nsIDOMWindowInternal * aDOMWin,
                                            PRPackedBool&          aIsParentFrameSet)
{
  aIsParentFrameSet = IsParentAFrameSet(aWebShell);
  PRBool iFrameIsSelected = PR_FALSE;
#if 1
  PrintObject* po = FindPrintObjectByDOMWin(mPrt->mPrintObject, aDOMWin);
  iFrameIsSelected = po && po->mFrameType == eIFrame;
#else
  // First, check to see if we are a frameset
  if (!aIsParentFrameSet) {
    // Check to see if there is a currenlt focused frame
    // if so, it means the selected frame is either the main webshell
    // or an IFRAME
    if (aDOMWin != nsnull) {
      // Get the main webshell's DOMWin to see if it matches 
      // the frame that is selected
      nsCOMPtr<nsIDOMWindowInternal> domWin = getter_AddRefs(GetDOMWinForWebShell(aWebShell));
      if (aDOMWin != nsnull && domWin != aDOMWin) {
        iFrameIsSelected = PR_TRUE; // we have a selected IFRAME
      }
    }
  }
#endif
  return iFrameIsSelected;
}


//------------------------------------------------------
PRBool 
DocumentViewerImpl::IsThereARangeSelection(nsIDOMWindowInternal * aDOMWin)
{
  nsCOMPtr<nsIPresShell> presShell;
  if (aDOMWin != nsnull) {
    nsCOMPtr<nsIScriptGlobalObject> scriptObj(do_QueryInterface(aDOMWin));
    nsCOMPtr<nsIDocShell> docShell;
    scriptObj->GetDocShell(getter_AddRefs(docShell));
    docShell->GetPresShell(getter_AddRefs(presShell));
  }

  // check here to see if there is a range selection
  // so we know whether to turn on the "Selection" radio button
  nsCOMPtr<nsISelection> selection;
  GetDocumentSelection(getter_AddRefs(selection), presShell);
  if (selection) {
    PRInt32 count;
    selection->GetRangeCount(&count);
    if (count == 1) {
      nsCOMPtr<nsIDOMRange> range;
      if (NS_SUCCEEDED(selection->GetRangeAt(0, getter_AddRefs(range)))) {
        // check to make sure it isn't an insertion selection
        PRBool isCollapsed;
        selection->GetIsCollapsed(&isCollapsed);
        return !isCollapsed;
      }
    }
  }
  return PR_FALSE;
}

#ifdef NS_PRINT_PREVIEW
//-------------------------------------------------------
// Recursively walks the PrintObject tree and removed the DocViewer
// as an event processor
nsresult 
DocumentViewerImpl::RemoveEventProcessorFromVMs(PrintObject* aPO)
{
  NS_ASSERTION(aPO, "Pointer is null!");

  if (aPO->mViewManager != nsnull) {
    aPO->mViewManager->SetEventProcessor(nsnull);
  }
  PRInt32 cnt = aPO->mKids.Count();
  for (PRInt32 i=0;i<cnt;i++) {
    if (NS_FAILED(RemoveEventProcessorFromVMs((PrintObject *)aPO->mKids[i]))) {
      return NS_ERROR_FAILURE;
    }
  }
  return NS_OK;
}

//-------------------------------------------------------
// Recursively walks the PrintObject tree and installs the DocViewer
// as an event processor and it shows the window
nsresult 
DocumentViewerImpl::ShowDocList(PrintObject* aPO, PRBool aShow)
{
  NS_ASSERTION(aPO, "Pointer is null!");

  if (aPO->mViewManager != nsnull) {
    aPO->mViewManager->SetEventProcessor(this);
  }

  PRBool donePrinting;
  DoPrint(aPO, PR_FALSE, donePrinting);

  if (aPO->mWindow != nsnull) {
    aPO->mWindow->Show(aShow);
  }

  PRInt32 cnt = aPO->mKids.Count();
  for (PRInt32 i=0;i<cnt;i++) {
    if (NS_FAILED(ShowDocList((PrintObject *)aPO->mKids[i], aShow))) {
      return NS_ERROR_FAILURE;
    }
  }
  return NS_OK;
}


void
DocumentViewerImpl::TurnScriptingOn(PRBool aDoTurnOn)
{
  NS_ASSERTION(mDocument, "We MUST have a document.");

  // get the script global object
  nsCOMPtr<nsIScriptGlobalObject> scriptGlobalObj;
  nsresult rv = mDocument->GetScriptGlobalObject(getter_AddRefs(scriptGlobalObj));
  NS_ASSERTION(NS_SUCCEEDED(rv) && scriptGlobalObj, "Can't get nsIScriptGlobalObject");
  nsCOMPtr<nsIScriptContext> scx;
  rv = scriptGlobalObj->GetContext(getter_AddRefs(scx));
  NS_ASSERTION(NS_SUCCEEDED(rv) && scx, "Can't get nsIScriptContext");
  scx->SetScriptsEnabled(aDoTurnOn);
}

void
DocumentViewerImpl::InstallNewPresentation()
{
  // Get the current size of what is being viewed
  nsRect area;
  mPresContext->GetVisibleArea(area);

  nsRect bounds;
  mWindow->GetBounds(bounds);

  // In case we have focus focus the parent DocShell
  // which in this case should always be chrome
  nsCOMPtr<nsIDocShellTreeItem>  dstParentItem;
  nsCOMPtr<nsIDocShellTreeItem>  dstItem(do_QueryInterface(mContainer));
  if (dstItem) {
    dstItem->GetParent(getter_AddRefs(dstParentItem));
    nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(dstParentItem));
    if (docShell) {
      docShell->SetHasFocus(PR_TRUE);
    }
  }

  // turn off selection painting
  nsCOMPtr<nsISelectionController> selectionController = do_QueryInterface(mPresShell);
  if (selectionController) {
    selectionController->SetDisplaySelection(nsISelectionController::SELECTION_OFF);
  }

  // Start to kill off the old Presentation
  // by cleaning up the PresShell
  if (mPresShell) {
    // Break circular reference (or something)
    mPresShell->EndObservingDocument();
    nsCOMPtr<nsISelection> selection;
    nsresult rv = GetDocumentSelection(getter_AddRefs(selection));
    nsCOMPtr<nsISelectionPrivate> selPrivate(do_QueryInterface(selection));
    if (NS_SUCCEEDED(rv) && selPrivate && mSelectionListener) 
      selPrivate->RemoveSelectionListener(mSelectionListener);
    mPresShell->Destroy();
  }

  // clear weak references before we go away
  if (mPresContext) {
    mPresContext->SetContainer(nsnull);
    mPresContext->SetLinkHandler(nsnull);
  }

  // Destroy the old Presentation
  mPresShell    = nsnull;
  mPresContext  = nsnull;
  mViewManager  = nsnull;
  mWindow       = nsnull;

  // Install the new Presentation
  PrintObject * po = mPrt->mPrintObject;
  mPresShell    = po->mPresShell;
  mPresContext  = po->mPresContext;
  mViewManager  = po->mViewManager;
  mWindow       = po->mWindow;

  po->mSharedPresShell = PR_TRUE;
  mPresShell->BeginObservingDocument();

  nscoord width  = bounds.width;
  nscoord height = bounds.height;
  float p2t;
  mPresContext->GetPixelsToTwips(&p2t);
  width = NSIntPixelsToTwips(width, p2t);
  height = NSIntPixelsToTwips(height, p2t);
  mViewManager->DisableRefresh();
  mViewManager->SetWindowDimensions(width, height);

  mDeviceContext->SetUseAltDC(kUseAltDCFor_FONTMETRICS, PR_FALSE);
  mDeviceContext->SetUseAltDC(kUseAltDCFor_CREATE_RC, PR_FALSE);

  mViewManager->EnableRefresh(NS_VMREFRESH_NO_SYNC);
  Show();

  ShowDocList(mPrt->mPrintObject, PR_TRUE);

}

void
DocumentViewerImpl::ReturnToGalleyPresentation()
{
  if (!mIsDoingPrintPreview) {
    NS_ASSERTION(0, "Wow, we should never get here!");
    return;
  }
  delete mPrtPreview;
  mPrtPreview = nsnull;

  // Get the current size of what is being viewed
  nsRect area;
  mPresContext->GetVisibleArea(area);

  nsRect bounds;
  mWindow->GetBounds(bounds);

  // In case we have focus focus the parent DocShell
  // which in this case should always be chrome
  nsCOMPtr<nsIDocShellTreeItem>  dstParentItem;
  nsCOMPtr<nsIDocShellTreeItem>  dstItem(do_QueryInterface(mContainer));
  if (dstItem) {
    dstItem->GetParent(getter_AddRefs(dstParentItem));
    nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(dstParentItem));
    if (docShell) {
      docShell->SetHasFocus(PR_TRUE);
    }
  }

  // Start to kill off the old Presentation
  // by cleaning up the PresShell
  if (mPresShell) {
    // Break circular reference (or something)
    mPresShell->EndObservingDocument();
    nsCOMPtr<nsISelection> selection;
    nsresult rv = GetDocumentSelection(getter_AddRefs(selection));
    nsCOMPtr<nsISelectionPrivate> selPrivate(do_QueryInterface(selection));
    if (NS_SUCCEEDED(rv) && selPrivate && mSelectionListener) 
      selPrivate->RemoveSelectionListener(mSelectionListener);
    mPresShell->Destroy();
  }

  // clear weak references before we go away
  if (mPresContext) {
    mPresContext->SetContainer(nsnull);
    mPresContext->SetLinkHandler(nsnull);
  }

  // Destroy the old Presentation
  mPresShell    = nsnull;
  mPresContext  = nsnull;
  mViewManager  = nsnull;
  mWindow       = nsnull;

  // Very important! Turn On scripting
  TurnScriptingOn(PR_TRUE);

  Init(mParentWidget, mDeviceContext, bounds);

  // this needs to be set here not earlier,
  // because it is needing when re-constructing the Galley Mode)
  mIsDoingPrintPreview = PR_FALSE;

  mViewManager->EnableRefresh(NS_VMREFRESH_NO_SYNC);
  Show();

}

#endif // NS_PRINT_PREVIEW

//-----------------------------------------------------------------
// This method checks to see if there is at least one printer defined
// and if so, it sets the first printer in the list as the default name
// in the PrintSettings which is then used for Printer Preview
nsresult 
DocumentViewerImpl::CheckForPrinters(nsIPrintOptions*  aPrintOptions,
                                     nsIPrintSettings* aPrintSettings,
                                     PRUint32          aErrorCode,
                                     PRBool            aIsPrinting)
{
  NS_ENSURE_ARG_POINTER(aPrintOptions);
  NS_ENSURE_ARG_POINTER(aPrintSettings);

  nsresult rv = NS_ERROR_FAILURE;
  PRUint32 numPrinters = 0;

  nsCOMPtr<nsISimpleEnumerator> simpEnum;
  aPrintOptions->AvailablePrinters(getter_AddRefs(simpEnum));
  if (simpEnum) {
    numPrinters = 0;
    do {
      PRBool hasMore;
      simpEnum->HasMoreElements(&hasMore);
      if (!hasMore) break;

      nsCOMPtr<nsISupports> supps;
      simpEnum->GetNext(getter_AddRefs(supps));
      if (numPrinters == 0) {
        nsCOMPtr<nsISupportsWString> wStr = do_QueryInterface(supps);
        if (wStr) {
          PRUnichar* defPrinterName;
          wStr->ToString(&defPrinterName);
          aPrintSettings->SetPrinterName(defPrinterName);
          nsMemory::Free(defPrinterName);
        }
      }
      numPrinters++;
    } while (1);

    if (numPrinters == 0) {
      // this means there were no printers
      ShowPrintErrorDialog(aErrorCode, aIsPrinting);
    } else {
      rv = NS_OK;
    }
  } else {
    // this means there were no printers
    // XXX the ifdefs are temporary until they correctly implement Available Printers
#if defined(XP_MAC)
    rv = NS_OK;
#else
    ShowPrintErrorDialog(aErrorCode, aIsPrinting);
#endif
  }
  return rv;
}

/** ---------------------------------------------------
 *  See documentation above in the nsIContentViewerfile class definition
 *	@update 11/01/01 rods
 *
 *  For a full and detailed understanding of the issues with 
 *  PrintPreview: See the design spec that is attached to Bug 107562 
 */
NS_IMETHODIMP
DocumentViewerImpl::PrintPreview(nsIPrintSettings* aPrintSettings)
{
  nsresult rv = NS_OK;

#ifdef NS_PRINT_PREVIEW

  // if we are printing another URL, then exit
  // the reason we check here is because this method can be called while 
  // another is still in here (the printing dialog is a good example).
  // the only time we can print more than one job at a time is the regression tests
  if (mIsDoingPrintPreview) {
    ReturnToGalleyPresentation();
    return NS_OK;
  }

  mPrt = new PrintData();
  if (mPrt == nsnull) {
    mIsCreatingPrintPreview = PR_FALSE;
    return NS_ERROR_OUT_OF_MEMORY;
  }

  mPrt->mPrintSettings = aPrintSettings;
  nsCOMPtr<nsIPrintOptions> printService = do_GetService(kPrintOptionsCID, &rv);
  if (NS_SUCCEEDED(rv) && printService) {
    // if they don't pass in a PrintSettings, then make one
    if (mPrt->mPrintSettings == nsnull) {
      printService->CreatePrintSettings(getter_AddRefs(mPrt->mPrintSettings));
    }
    NS_ASSERTION(mPrt->mPrintSettings, "You can't PrintPreview without a PrintSettings!");

    // Get the default printer name and set it into the PrintSettings
    if (NS_FAILED(CheckForPrinters(printService, mPrt->mPrintSettings, NS_ERROR_GFX_PRINTER_PRINTPREVIEW, PR_FALSE))) {
      delete mPrt;
      mPrt = nsnull;
      return NS_ERROR_FAILURE;
    }
  }
  
  // Let's print ...
  mIsCreatingPrintPreview = PR_TRUE;
  mIsDoingPrintPreview    = PR_TRUE;

  mIsDoingPrintPreview = PR_TRUE;

  // Very important! Turn Off scripting
  TurnScriptingOn(PR_FALSE);

  // Get the currently focused window and cache it
  // because the Print Dialog will "steal" focus and later when you try
  // to get the currently focused windows it will be NULL
  mPrt->mCurrentFocusWin = getter_AddRefs(FindFocusedDOMWindowInternal());

  // Check to see if there is a "regular" selection
  PRBool isSelection = IsThereARangeSelection(mPrt->mCurrentFocusWin);

  // Create a list for storing the WebShells that need to be printed
  if (mPrt->mPrintDocList == nsnull) {
    mPrt->mPrintDocList = new nsVoidArray();
    if (mPrt->mPrintDocList == nsnull) {
      mIsCreatingPrintPreview = PR_FALSE;
      mIsDoingPrintPreview    = PR_FALSE;
      TurnScriptingOn(PR_TRUE);
      return NS_ERROR_FAILURE;
    }
  } else {
    mPrt->mPrintDocList->Clear();
  }

  // Get the webshell for this documentviewer
  nsCOMPtr<nsIWebShell> webContainer(do_QueryInterface(mContainer));

  // Add Root Doc to Tree and List
  mPrt->mPrintObject             = new PrintObject;
  mPrt->mPrintObject->mWebShell  = webContainer;
  mPrt->mPrintDocList->AppendElement(mPrt->mPrintObject);

  mPrt->mIsParentAFrameSet = IsParentAFrameSet(webContainer);
  mPrt->mPrintObject->mFrameType = mPrt->mIsParentAFrameSet?eFrameSet:eDoc;

  // Build the "tree" of PrintObjects
  nsCOMPtr<nsIDocShellTreeNode>  parentAsNode(do_QueryInterface(webContainer));
  BuildDocTree(parentAsNode, mPrt->mPrintDocList, mPrt->mPrintObject);

  // Create the linkage from the suv-docs back to the content element
  // in the parent document
  MapContentToWebShells(mPrt->mPrintObject, mPrt->mPrintObject);

  // Get whether the doc contains a frameset 
  // Also, check to see if the currently focus webshell 
  // is a child of this webshell
  mPrt->mIsIFrameSelected = IsThereAnIFrameSelected(webContainer, mPrt->mCurrentFocusWin, mPrt->mIsParentAFrameSet);


  DUMP_DOC_LIST("\nAfter Mapping------------------------------------------");

  // Setup print options for UI
  rv = NS_ERROR_FAILURE;
  if (mPrt->mPrintSettings != nsnull) {
    if (mPrt->mIsParentAFrameSet) {
      if (mPrt->mCurrentFocusWin) {
        mPrt->mPrintSettings->SetHowToEnableFrameUI(nsIPrintSettings::kFrameEnableAll);
      } else {
        mPrt->mPrintSettings->SetHowToEnableFrameUI(nsIPrintSettings::kFrameEnableAsIsAndEach);
      }
    } else {
      mPrt->mPrintSettings->SetHowToEnableFrameUI(nsIPrintSettings::kFrameEnableNone);
    }
    // Now determine how to set up the Frame print UI
    mPrt->mPrintSettings->SetPrintOptions(nsIPrintSettings::kEnableSelectionRB, isSelection || mPrt->mIsIFrameSelected);
  }

#ifdef DEBUG_PRINTING
  if (mPrt->mPrintSettings) {
    PRInt16 printHowEnable = nsIPrintSettings::kFrameEnableNone; 
    mPrt->mPrintSettings->GetHowToEnableFrameUI(&printHowEnable);
    PRBool val;
    mPrt->mPrintSettings->GetPrintOptions(nsIPrintSettings::kEnableSelectionRB, &val);

    PRINT_DEBUG_MSG1("********* DocumentViewerImpl::Print *********\n");
    PRINT_DEBUG_MSG2("IsParentAFrameSet:   %s \n", PRT_YESNO(mPrt->mIsParentAFrameSet));
    PRINT_DEBUG_MSG2("IsIFrameSelected:    %s \n", PRT_YESNO(mPrt->mIsIFrameSelected));
    PRINT_DEBUG_MSG2("Main Doc Frame Type: %s \n", gFrameTypesStr[mPrt->mPrintObject->mFrameType]);
    PRINT_DEBUG_MSG2("HowToEnableFrameUI:  %s \n", gFrameHowToEnableStr[printHowEnable]);
    PRINT_DEBUG_MSG2("EnableSelectionRB:   %s \n", PRT_YESNO(val));
    PRINT_DEBUG_MSG1("*********************************************\n");
    PRINT_DEBUG_FLUSH
  }
#endif

  PRBool doSilent = PR_TRUE;

  nscoord width  = NS_INCHES_TO_TWIPS(8.5);
  nscoord height = NS_INCHES_TO_TWIPS(11.0);

  nsCOMPtr<nsIDeviceContext> ppDC;
  nsCOMPtr<nsIDeviceContextSpecFactory> factory = do_CreateInstance(kDeviceContextSpecFactoryCID);
  if (factory) {
    nsIDeviceContextSpec *devspec = nsnull;
    nsCOMPtr<nsIDeviceContext> dx;
    factory->CreateDeviceContextSpec(mWindow, aPrintSettings, devspec, doSilent);
    if (nsnull != devspec) {
      nsresult rv = mDeviceContext->GetDeviceContextFor(devspec, *getter_AddRefs(ppDC)); 
      if (NS_SUCCEEDED(rv)) {
        mDeviceContext->SetAltDevice(ppDC);
        if (mPrt->mPrintSettings != nsnull) {
          double scaling;
          mPrt->mPrintSettings->GetScaling(&scaling);
          if (scaling < 1.0) {
            mDeviceContext->SetCanonicalPixelScale(float(scaling));
          }
        }
        ppDC->GetDeviceSurfaceDimensions(width, height);
        NS_RELEASE(devspec);
      }
    }
  }

  if (doSilent) {
    mPrt->mPrintSettings->SetPrintFrameType(nsIPrintSettings::kFramesAsIs);
  }

  mPrt->mPrintDC = mDeviceContext; // XXX why?

  if (mDeviceContext) {
    mDeviceContext->SetUseAltDC(kUseAltDCFor_FONTMETRICS, PR_TRUE);
    mDeviceContext->SetUseAltDC(kUseAltDCFor_CREATE_RC, PR_TRUE);
    mDeviceContext->SetUseAltDC(kUseAltDCFor_SURFACE_DIM, PR_TRUE);
  }
  rv = DocumentReadyForPrinting();

  mIsCreatingPrintPreview = PR_FALSE;

  /* cleaup on failure + notify user */
  if (NS_FAILED(rv)) {
    if (mPrt) {
      mPrt->OnEndPrinting(rv);
      delete mPrt;
      mPrt = nsnull;
    }

    /* cleanup done, let's fire-up an error dialog to notify the user
     * what went wrong... 
     */   
    ShowPrintErrorDialog(rv);
    TurnScriptingOn(PR_TRUE);
    mIsCreatingPrintPreview = PR_FALSE;
    mIsDoingPrintPreview    = PR_FALSE;
    return rv;
  }


  // At this point we are done preparing everything 
  // before it is to be created

  // Noew create the new Presentation and display it
  InstallNewPresentation();

  // PrintPreview was built using the mPrt (code reuse)
  // then we assign it over
  mPrtPreview = mPrt;
  mPrt        = nsnull;

#endif // NS_PRINT_PREVIEW

  return NS_OK;
}


void
DocumentViewerImpl::SetDocAndURLIntoProgress(nsIWebShell* aWebShell,
                                             nsIPrintProgressParams* aParams)
{
  NS_ASSERTION(aWebShell, "Must have vaild nsIWebShell");
  NS_ASSERTION(aParams, "Must have vaild nsIPrintProgressParams");
  if (aWebShell == nsnull || aParams == nsnull) {
    return;
  }
  PRUnichar * docTitleStr;
  PRUnichar * docURLStr;
  GetWebShellTitleAndURL(aWebShell, &docTitleStr, &docURLStr); 
  mPrt->mPrintProgressParams->SetDocTitle((const PRUnichar*) docTitleStr);
  mPrt->mPrintProgressParams->SetDocURL((const PRUnichar*) docURLStr);
  if (docTitleStr != nsnull) nsMemory::Free(docTitleStr);
  if (docURLStr != nsnull) nsMemory::Free(docURLStr);
}

void
DocumentViewerImpl::DoPrintProgress(PRBool aIsForPrinting)
{
  nsPrintProgress* prtProgress = new nsPrintProgress();
  nsresult rv = prtProgress->QueryInterface(NS_GET_IID(nsIPrintProgress), (void**)getter_AddRefs(mPrt->mPrintProgress));
  if (NS_FAILED(rv)) return;


  rv = prtProgress->QueryInterface(NS_GET_IID(nsIWebProgressListener), (void**)getter_AddRefs(mPrt->mPrintProgressListener));
  if (NS_FAILED(rv)) return;
  // add to listener list
  mPrt->mPrintProgressListeners.AppendElement((void*)mPrt->mPrintProgressListener);
  nsIWebProgressListener* wpl = NS_STATIC_CAST(nsIWebProgressListener*, mPrt->mPrintProgressListener.get());
  NS_ASSERTION(wpl, "nsIWebProgressListener is NULL!");
  NS_ADDREF(wpl);

  nsCOMPtr<nsIPref> prefs (do_GetService(NS_PREF_CONTRACTID));
  if (prefs) {
    prefs->GetBoolPref("print.show_print_progress", &mPrt->mShowProgressDialog);
    if (mPrt->mShowProgressDialog) {
      nsPrintProgressParams* prtProgressParams = new nsPrintProgressParams();
      nsCOMPtr<nsIPrintProgressParams> params;
      rv = prtProgressParams->QueryInterface(NS_GET_IID(nsIPrintProgressParams), (void**)getter_AddRefs(mPrt->mPrintProgressParams));
      if (NS_SUCCEEDED(rv) && mPrt->mPrintProgressParams) {
        SetDocAndURLIntoProgress(mPrt->mPrintObject->mWebShell, mPrt->mPrintProgressParams);

        nsCOMPtr<nsIWindowWatcher> wwatch(do_GetService("@mozilla.org/embedcomp/window-watcher;1"));
        if (wwatch) {
          nsCOMPtr<nsIDOMWindow> active;
          wwatch->GetActiveWindow(getter_AddRefs(active));

          nsCOMPtr<nsIDOMWindowInternal> parent(do_QueryInterface(active));
          mPrt->mPrintProgress->OpenProgressDialog(parent, "chrome://global/content/printProgress.xul", mPrt->mPrintProgressParams);
        }
      }
    }
  }
}

#ifdef NS_DEBUG
/** ---------------------------------------------------
 *  See documentation above in the nsIContentViewerfile class definition
 *	@update 01/24/00 dwc
 */
NS_IMETHODIMP
DocumentViewerImpl::Print(PRBool            aSilent,
                          FILE *            aDebugFile, 
                          nsIPrintSettings* aPrintSettings)
{
  mDebugFile = aDebugFile;
  return Print(aSilent, aPrintSettings, nsnull);
}
#endif

/** ---------------------------------------------------
 *  See documentation above in the nsIContentViewerfile class definition
 *	@update 01/24/00 dwc
 */
NS_IMETHODIMP
DocumentViewerImpl::Print(PRBool            aSilent,
                          nsIPrintSettings* aPrintSettings,
                          nsIWebProgressListener* aWebProgressListener)
{
#ifdef DEBUG_PRINTING
  // need for capturing result on each doc and sub-doc that is printed
  gDumpFileNameCnt   = 0;
  gDumpLOFileNameCnt = 0;
#endif

  nsresult rv;

  // if we are printing another URL, then exit
  // the reason we check here is because this method can be called while 
  // another is still in here (the printing dialog is a good example).
  // the only time we can print more than one job at a time is the regression tests
  if (mIsDoingPrinting) {
    // Let the user know we are not ready to print.
    rv = NS_ERROR_NOT_AVAILABLE;
    ShowPrintErrorDialog(rv);
    return rv;
  }
  
  mPrt = new PrintData();
  if (mPrt == nsnull) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  // if they don't pass in a PrintSettings, then make one
  // it will have all the default values
  mPrt->mPrintSettings = aPrintSettings;
  nsCOMPtr<nsIPrintOptions> printService = do_GetService(kPrintOptionsCID, &rv);
  if (NS_SUCCEEDED(rv) && printService) {
    // if they don't pass in a PrintSettings, then make one
    if (mPrt->mPrintSettings == nsnull) {
      printService->CreatePrintSettings(getter_AddRefs(mPrt->mPrintSettings));
    }
    NS_ASSERTION(mPrt->mPrintSettings, "You can't PrintPreview without a PrintSettings!");

    // Get the default printer name and set it into the PrintSettings
    if (NS_FAILED(CheckForPrinters(printService, mPrt->mPrintSettings, NS_ERROR_GFX_PRINTER_NO_PRINTER_AVAILABLE, PR_TRUE))) {
      delete mPrt;
      mPrt = nsnull;
      return NS_ERROR_FAILURE;
    }
  }
  mPrt->mPrintSettings->SetPrintSilent(aSilent);

  // Let's print ...
  mIsDoingPrinting = PR_TRUE;

  if (aWebProgressListener != nsnull) {
    mPrt->mPrintProgressListeners.AppendElement((void*)aWebProgressListener);
    NS_ADDREF(aWebProgressListener);
  }
  
  // Get the currently focused window and cache it
  // because the Print Dialog will "steal" focus and later when you try
  // to get the currently focused windows it will be NULL
  mPrt->mCurrentFocusWin = getter_AddRefs(FindFocusedDOMWindowInternal());

  // Check to see if there is a "regular" selection
  PRBool isSelection = IsThereARangeSelection(mPrt->mCurrentFocusWin);

  // Create a list for storing the WebShells that need to be printed
  if (mPrt->mPrintDocList == nsnull) {
    mPrt->mPrintDocList = new nsVoidArray();
    if (mPrt->mPrintDocList == nsnull) {
      mIsDoingPrinting = PR_FALSE;
      delete mPrt;
      mPrt = nsnull;
      return NS_ERROR_FAILURE;
    }
  } else {
    mPrt->mPrintDocList->Clear();
  }

  // Get the webshell for this documentviewer
  nsCOMPtr<nsIWebShell> webContainer(do_QueryInterface(mContainer));

  // Add Root Doc to Tree and List
  mPrt->mPrintObject             = new PrintObject;
  mPrt->mPrintObject->mWebShell  = webContainer;
  mPrt->mPrintDocList->AppendElement(mPrt->mPrintObject);

  mPrt->mIsParentAFrameSet = IsParentAFrameSet(webContainer);
  mPrt->mPrintObject->mFrameType = mPrt->mIsParentAFrameSet?eFrameSet:eDoc;

  // Build the "tree" of PrintObjects
  nsCOMPtr<nsIDocShellTreeNode>  parentAsNode(do_QueryInterface(webContainer));
  BuildDocTree(parentAsNode, mPrt->mPrintDocList, mPrt->mPrintObject);

  // Create the linkage from the suv-docs back to the content element
  // in the parent document
  MapContentToWebShells(mPrt->mPrintObject, mPrt->mPrintObject);


  // Get whether the doc contains a frameset 
  // Also, check to see if the currently focus webshell 
  // is a child of this webshell
  mPrt->mIsIFrameSelected = IsThereAnIFrameSelected(webContainer, mPrt->mCurrentFocusWin, mPrt->mIsParentAFrameSet);

  DUMP_DOC_LIST("\nAfter Mapping------------------------------------------");

  rv = NS_ERROR_FAILURE;
  // Setup print options for UI
  if (mPrt->mIsParentAFrameSet) {
    if (mPrt->mCurrentFocusWin) {
      mPrt->mPrintSettings->SetHowToEnableFrameUI(nsIPrintSettings::kFrameEnableAll);
    } else {
      mPrt->mPrintSettings->SetHowToEnableFrameUI(nsIPrintSettings::kFrameEnableAsIsAndEach);
    }
  } else {
    mPrt->mPrintSettings->SetHowToEnableFrameUI(nsIPrintSettings::kFrameEnableNone);
  }
  // Now determine how to set up the Frame print UI
  mPrt->mPrintSettings->SetPrintOptions(nsIPrintSettings::kEnableSelectionRB, isSelection || mPrt->mIsIFrameSelected);

#ifdef DEBUG_PRINTING
  if (mPrt->mPrintSettings) {
    PRInt16 printHowEnable = nsIPrintSettings::kFrameEnableNone; 
    mPrt->mPrintSettings->GetHowToEnableFrameUI(&printHowEnable);
    PRBool val;
    mPrt->mPrintSettings->GetPrintOptions(nsIPrintSettings::kEnableSelectionRB, &val);

    PRINT_DEBUG_MSG1("********* DocumentViewerImpl::Print *********\n");
    PRINT_DEBUG_MSG2("IsParentAFrameSet:   %s \n", PRT_YESNO(mPrt->mIsParentAFrameSet));
    PRINT_DEBUG_MSG2("IsIFrameSelected:    %s \n", PRT_YESNO(mPrt->mIsIFrameSelected));
    PRINT_DEBUG_MSG2("Main Doc Frame Type: %s \n", gFrameTypesStr[mPrt->mPrintObject->mFrameType]);
    PRINT_DEBUG_MSG2("HowToEnableFrameUI:  %s \n", gFrameHowToEnableStr[printHowEnable]);
    PRINT_DEBUG_MSG2("EnableSelectionRB:   %s \n", PRT_YESNO(val));
    PRINT_DEBUG_MSG1("*********************************************\n");
    PRINT_DEBUG_FLUSH
  }
#endif

  /* create factory (incl. create print dialog) */
  nsCOMPtr<nsIDeviceContextSpecFactory> factory =
          do_CreateInstance(kDeviceContextSpecFactoryCID, &rv);
          
  if (NS_SUCCEEDED(rv)) {
#ifdef DEBUG_dcone
    printf("PRINT JOB STARTING\n");
#endif

    nsIDeviceContextSpec *devspec = nsnull;
    nsCOMPtr<nsIDeviceContext> dx;
    mPrt->mPrintDC = nsnull; // XXX why?

#ifdef NS_DEBUG
    mPrt->mDebugFilePtr = mDebugFile;
#endif

    PRBool printSilently;
    mPrt->mPrintSettings->GetPrintSilent(&printSilently);
    rv = factory->CreateDeviceContextSpec(mWindow, mPrt->mPrintSettings, devspec, printSilently);
    if (NS_SUCCEEDED(rv)) {
      rv = mPresContext->GetDeviceContext(getter_AddRefs(dx));
      if (NS_SUCCEEDED(rv))
        rv = dx->GetDeviceContextFor(devspec, *getter_AddRefs(mPrt->mPrintDC));
        double scaling;
        mPrt->mPrintSettings->GetScaling(&scaling);
        if (scaling <= 1.0) {
          dx->SetCanonicalPixelScale(float(scaling));
        }
        
      if (NS_SUCCEEDED(rv)) {
        NS_RELEASE(devspec);

        if(webContainer) {
#ifdef DEBUG_dcone
          float   a1,a2;
          PRInt32 i1,i2;

          printf("CRITICAL PRINTING INFORMATION\n");

          // DEVICE CONTEXT INFORMATION from PresContext
          printf("DeviceContext of Presentation Context(%x)\n",dx);
          dx->GetDevUnitsToTwips(a1);
          dx->GetTwipsToDevUnits(a2);
          printf("    DevToTwips = %f TwipToDev = %f\n",a1,a2);
          dx->GetAppUnitsToDevUnits(a1);
          dx->GetDevUnitsToAppUnits(a2);
          printf("    AppUnitsToDev = %f DevUnitsToApp = %f\n",a1,a2);
          dx->GetCanonicalPixelScale(a1);
          printf("    GetCanonicalPixelScale = %f\n",a1);
          dx->GetScrollBarDimensions(a1, a2);
          printf("    ScrollBar x = %f y = %f\n",a1,a2);
          dx->GetZoom(a1);
          printf("    Zoom = %f\n",a1);
          dx->GetDepth((PRUint32&)i1);
          printf("    Depth = %d\n",i1);
          dx->GetDeviceSurfaceDimensions(i1,i2);
          printf("    DeviceDimension w = %d h = %d\n",i1,i2);


          // DEVICE CONTEXT INFORMATION
          printf("DeviceContext created for print(%x)\n",mPrt->mPrintDC);
          mPrt->mPrintDC->GetDevUnitsToTwips(a1);
          mPrt->mPrintDC->GetTwipsToDevUnits(a2);
          printf("    DevToTwips = %f TwipToDev = %f\n",a1,a2);
          mPrt->mPrintDC->GetAppUnitsToDevUnits(a1);
          mPrt->mPrintDC->GetDevUnitsToAppUnits(a2);
          printf("    AppUnitsToDev = %f DevUnitsToApp = %f\n",a1,a2);
          mPrt->mPrintDC->GetCanonicalPixelScale(a1);
          printf("    GetCanonicalPixelScale = %f\n",a1);
          mPrt->mPrintDC->GetScrollBarDimensions(a1, a2);
          printf("    ScrollBar x = %f y = %f\n",a1,a2);
          mPrt->mPrintDC->GetZoom(a1);
          printf("    Zoom = %f\n",a1);
          mPrt->mPrintDC->GetDepth((PRUint32&)i1);
          printf("    Depth = %d\n",i1);
          mPrt->mPrintDC->GetDeviceSurfaceDimensions(i1,i2);
          printf("    DeviceDimension w = %d h = %d\n",i1,i2);

#endif /* DEBUG_dcone */

          // Always check and set the print settings first and then fall back
          // onto the PrintService if there isn't a PrintSettings
          //
          // Posiible Usage values:
          //   nsIPrintSettings::kUseInternalDefault
          //   nsIPrintSettings::kUseSettingWhenPossible
          //
          // NOTE: The consts are the same for PrintSettings and PrintSettings
          PRInt16 printFrameTypeUsage = nsIPrintSettings::kUseSettingWhenPossible;
          mPrt->mPrintSettings->GetPrintFrameTypeUsage(&printFrameTypeUsage);

          // Ok, see if we are going to use our value and override the default
          if (printFrameTypeUsage == nsIPrintSettings::kUseSettingWhenPossible) {
            // Get the Print Options/Settings PrintFrameType to see what is preferred
            PRInt16 printFrameType = nsIPrintSettings::kEachFrameSep;
            mPrt->mPrintSettings->GetPrintFrameType(&printFrameType);

            // Don't let anybody do something stupid like try to set it to
            // kNoFrames when we are printing a FrameSet
            if (printFrameType == nsIPrintSettings::kNoFrames) {
              mPrt->mPrintFrameType = nsIPrintSettings::kEachFrameSep;
              mPrt->mPrintSettings->SetPrintFrameType(mPrt->mPrintFrameType);
            } else {
              // First find out from the PrinService what options are available 
              // to us for Printing FrameSets
              PRInt16 howToEnableFrameUI;
              mPrt->mPrintSettings->GetHowToEnableFrameUI(&howToEnableFrameUI);
              if (howToEnableFrameUI != nsIPrintSettings::kFrameEnableNone) {
                switch (howToEnableFrameUI) {
                  case nsIPrintSettings::kFrameEnableAll:
                    mPrt->mPrintFrameType = printFrameType;
                    break;

                  case nsIPrintSettings::kFrameEnableAsIsAndEach:
                    if (printFrameType != nsIPrintSettings::kSelectedFrame) {
                      mPrt->mPrintFrameType = printFrameType;
                    } else { // revert back to a good value
                      mPrt->mPrintFrameType = nsIPrintSettings::kEachFrameSep;
                    }
                    break;
                } // switch
                mPrt->mPrintSettings->SetPrintFrameType(mPrt->mPrintFrameType);
              }
            }
          }

          if (printService) {
            // check to see if we are printing to a file
            PRBool isPrintToFile = PR_FALSE;
            mPrt->mPrintSettings->GetPrintToFile(&isPrintToFile);
            if (isPrintToFile) {
            // On some platforms The BeginDocument needs to know the name of the file
            // and it uses the PrintService to get it, so we need to set it into the PrintService here
              PRUnichar* fileName;
              mPrt->mPrintSettings->GetToFileName(&fileName);
              if (fileName != nsnull) {
                printService->SetPrintToFile(PR_TRUE);
                printService->SetToFileName(fileName);
                nsMemory::Free(fileName);
              }
            } else {
              printService->SetPrintToFile(PR_FALSE);
              printService->SetToFileName(nsnull);
            }
          }

          DoPrintProgress(PR_TRUE);

          // Print listener setup...
          if (mPrt != nsnull) {
            mPrt->OnStartPrinting();    
          }

          //
          // The mIsPrinting flag is set when the ImageGroup observer is
          // notified that images must be loaded as a result of the 
          // InitialReflow...
          //
          if(!mIsPrinting  || (mPrt->mDebugFilePtr != nsnull)) {
            rv = DocumentReadyForPrinting();
#ifdef DEBUG_dcone
            printf("PRINT JOB ENDING, OBSERVER WAS NOT CALLED\n");
#endif
          } else {
            // use the observer mechanism to finish the printing
#ifdef DEBUG_dcone
            printf("PRINTING OBSERVER STARTED\n");
#endif
          }
        }
      }      
    } 
  }
  
  /* cleaup on failure + notify user */
  if (NS_FAILED(rv)) {
    /* cleanup... */
    if (mPagePrintTimer) {
      mPagePrintTimer->Stop();
      NS_RELEASE(mPagePrintTimer);
    }
    
    if (mPrt) {
      mPrt->OnEndPrinting(rv);
      delete mPrt;
      mPrt = nsnull;
    }
    mIsDoingPrinting = PR_FALSE;

    /* cleanup done, let's fire-up an error dialog to notify the user
     * what went wrong... 
     */
    if (rv != NS_ERROR_ABORT)
      ShowPrintErrorDialog(rv);
  }
      
  return rv;
}


void
DocumentViewerImpl::ShowPrintErrorDialog(nsresult aPrintError, PRBool aIsPrinting)
{
  nsresult rv;
  
  static NS_DEFINE_CID(kCStringBundleServiceCID,  NS_STRINGBUNDLESERVICE_CID);
  nsCOMPtr<nsIStringBundleService> stringBundleService = do_GetService(kCStringBundleServiceCID);
  
  if (!stringBundleService) { 
    NS_WARNING("ERROR: Failed to get StringBundle Service instance.\n");
    return;
  }
  nsCOMPtr<nsIStringBundle> myStringBundle;
  rv = stringBundleService->CreateBundle(NS_ERROR_GFX_PRINTER_BUNDLE_URL, getter_AddRefs(myStringBundle));    
  if (NS_FAILED(rv)) 
    return;
    
  PRUnichar   *msg        = nsnull,
              *title      = nsnull;
  nsAutoString stringName;

  switch(aPrintError)
  {
#define NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(nserr) case nserr: stringName = NS_LITERAL_STRING(#nserr); break;
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_GFX_PRINTER_CMD_NOT_FOUND)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_GFX_PRINTER_CMD_FAILURE)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_GFX_PRINTER_NO_PRINTER_AVAILABLE)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_GFX_PRINTER_NAME_NOT_FOUND)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_GFX_PRINTER_ACCESS_DENIED)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_GFX_PRINTER_INVALID_ATTRIBUTE)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_GFX_PRINTER_PRINTER_NOT_READY)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_GFX_PRINTER_OUT_OF_PAPER)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_GFX_PRINTER_PRINTER_IO_ERROR)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_GFX_PRINTER_COULD_NOT_OPEN_FILE)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_GFX_PRINTER_FILE_IO_ERROR)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_GFX_PRINTER_PRINTPREVIEW)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_UNEXPECTED)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_OUT_OF_MEMORY)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_NOT_IMPLEMENTED)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_NOT_AVAILABLE)
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_ABORT)
    default:
      NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG(NS_ERROR_FAILURE)
#undef NS_ERROR_TO_LOCALIZED_PRINT_ERROR_MSG      
  }

  myStringBundle->GetStringFromName(stringName.get(), &msg);
  if (aIsPrinting) {
    myStringBundle->GetStringFromName(NS_LITERAL_STRING("print_error_dialog_title").get(), &title);
  } else {
    myStringBundle->GetStringFromName(NS_LITERAL_STRING("printpreview_error_dialog_title").get(), &title);
  }

  if (!msg)
    return;
    
  nsCOMPtr<nsIWindowWatcher> wwatch(do_GetService("@mozilla.org/embedcomp/window-watcher;1"));
  if (wwatch) {
    nsCOMPtr<nsIDOMWindow> active;
    wwatch->GetActiveWindow(getter_AddRefs(active));

    nsCOMPtr<nsIDOMWindowInternal> parent(do_QueryInterface(active));
    
    if (parent) {
      nsCOMPtr<nsIPrompt> dialog; 
      parent->GetPrompter(getter_AddRefs(dialog)); 
      if (dialog) {
        dialog->Alert(title, msg);
      }
    }
  }
}


NS_IMETHODIMP
DocumentViewerImpl::GetPrintable(PRBool *aPrintable) 
{
  NS_ENSURE_ARG_POINTER(aPrintable);

  
  if(mIsDoingPrinting==PR_TRUE){
    *aPrintable = PR_FALSE;
  } else {
    *aPrintable = PR_TRUE;
  }

  return NS_OK;
}

#ifdef XP_MAC
#pragma mark -
#endif

//*****************************************************************************
// nsIMarkupDocumentViewer
//*****************************************************************************   

NS_IMETHODIMP DocumentViewerImpl::ScrollToNode(nsIDOMNode* aNode)
{
   NS_ENSURE_ARG(aNode);
   NS_ENSURE_TRUE(mDocument, NS_ERROR_NOT_AVAILABLE);
   nsCOMPtr<nsIPresShell> presShell;
   NS_ENSURE_SUCCESS(GetPresShell(*(getter_AddRefs(presShell))), NS_ERROR_FAILURE);

   // Get the nsIContent interface, because that's what we need to 
   // get the primary frame
   
   nsCOMPtr<nsIContent> content(do_QueryInterface(aNode));
   NS_ENSURE_TRUE(content, NS_ERROR_FAILURE);

   // Get the primary frame
   nsIFrame* frame;  // Remember Frames aren't ref-counted.  They are in their 
                     // own special little world.

   NS_ENSURE_SUCCESS(presShell->GetPrimaryFrameFor(content, &frame),
      NS_ERROR_FAILURE);

   // tell the pres shell to scroll to the frame
   NS_ENSURE_SUCCESS(presShell->ScrollFrameIntoView(frame, 
                                                    NS_PRESSHELL_SCROLL_TOP, 
                                                    NS_PRESSHELL_SCROLL_ANYWHERE), 
                     NS_ERROR_FAILURE); 
   return NS_OK; 
}

NS_IMETHODIMP DocumentViewerImpl::GetAllowPlugins(PRBool* aAllowPlugins)
{
   NS_ENSURE_ARG_POINTER(aAllowPlugins);

   *aAllowPlugins = mAllowPlugins;
   return NS_OK;
}

NS_IMETHODIMP DocumentViewerImpl::SetAllowPlugins(PRBool aAllowPlugins)
{
   mAllowPlugins = aAllowPlugins;
   return NS_OK;
}

nsresult
DocumentViewerImpl::CallChildren(CallChildFunc aFunc, void* aClosure)
{
  nsCOMPtr<nsIDocShellTreeNode> docShellNode(do_QueryInterface(mContainer));
  if (docShellNode)
  {
    PRInt32 i;
    PRInt32 n;
    docShellNode->GetChildCount(&n);
    for (i=0; i < n; i++) 
    {
      nsCOMPtr<nsIDocShellTreeItem> child;
      docShellNode->GetChildAt(i, getter_AddRefs(child));
      nsCOMPtr<nsIDocShell> childAsShell(do_QueryInterface(child));
      NS_ASSERTION(childAsShell, "null child in docshell");
      if (childAsShell) 
      {
        nsCOMPtr<nsIContentViewer> childCV;
        childAsShell->GetContentViewer(getter_AddRefs(childCV));
        if (childCV) 
        {
          nsCOMPtr<nsIMarkupDocumentViewer> markupCV = do_QueryInterface(childCV);
          if (markupCV) {
            (*aFunc)(markupCV, aClosure);
          }
        }
      }
    }
  }
  return NS_OK;
}

struct TextZoomInfo
{
  float mTextZoom;
};

static void
SetChildTextZoom(nsIMarkupDocumentViewer* aChild, void* aClosure)
{
  struct TextZoomInfo* textZoomInfo = (struct TextZoomInfo*) aClosure;
  aChild->SetTextZoom(textZoomInfo->mTextZoom);
}

NS_IMETHODIMP DocumentViewerImpl::SetTextZoom(float aTextZoom)
{
  if (mDeviceContext) {
    mDeviceContext->SetTextZoom(aTextZoom);
    if (mPresContext) {
      mPresContext->ClearStyleDataAndReflow();
    }
  }

  // now set the text zoom on all children of mContainer
  struct TextZoomInfo textZoomInfo = { aTextZoom };
  return CallChildren(SetChildTextZoom, &textZoomInfo);
}

NS_IMETHODIMP DocumentViewerImpl::GetTextZoom(float* aTextZoom)
{
  NS_ENSURE_ARG_POINTER(aTextZoom);

  if (mDeviceContext) {
    return mDeviceContext->GetTextZoom(*aTextZoom);
  }

  *aTextZoom = 1.0;
  return NS_OK;
}

// XXX: SEMANTIC CHANGE! 
//      returns a copy of the string.  Caller is responsible for freeing result
//      using Recycle(aDefaultCharacterSet)
NS_IMETHODIMP DocumentViewerImpl::GetDefaultCharacterSet(PRUnichar** aDefaultCharacterSet)
{
  NS_ENSURE_ARG_POINTER(aDefaultCharacterSet);
  NS_ENSURE_STATE(mContainer);

  if (0 == mDefaultCharacterSet.Length()) 
  {
    nsXPIDLString defCharset;

    nsCOMPtr<nsIWebShell> webShell;
    webShell = do_QueryInterface(mContainer);
    if (webShell)
    { 
      nsCOMPtr<nsIPref> prefs(do_GetService(NS_PREF_CONTRACTID));
      if (prefs)
        prefs->GetLocalizedUnicharPref("intl.charset.default", getter_Copies(defCharset));
    } 

    if (defCharset && defCharset.Length())
      mDefaultCharacterSet.Assign(defCharset.get());
    else
      mDefaultCharacterSet.Assign(NS_LITERAL_STRING("ISO-8859-1"));
  }
  *aDefaultCharacterSet = ToNewUnicode(mDefaultCharacterSet);
  return NS_OK;
}

static void
SetChildDefaultCharacterSet(nsIMarkupDocumentViewer* aChild, void* aClosure)
{
  aChild->SetDefaultCharacterSet((PRUnichar*) aClosure);
}

NS_IMETHODIMP DocumentViewerImpl::SetDefaultCharacterSet(const PRUnichar* aDefaultCharacterSet)
{
  mDefaultCharacterSet = aDefaultCharacterSet;  // this does a copy of aDefaultCharacterSet
  // now set the default char set on all children of mContainer
  return CallChildren(SetChildDefaultCharacterSet,
                      (void*) aDefaultCharacterSet);
}

// XXX: SEMANTIC CHANGE! 
//      returns a copy of the string.  Caller is responsible for freeing result
//      using Recycle(aForceCharacterSet)
NS_IMETHODIMP DocumentViewerImpl::GetForceCharacterSet(PRUnichar** aForceCharacterSet)
{
  NS_ENSURE_ARG_POINTER(aForceCharacterSet);

  nsAutoString emptyStr;
  if (mForceCharacterSet.Equals(emptyStr)) {
    *aForceCharacterSet = nsnull;
  }
  else {
    *aForceCharacterSet = ToNewUnicode(mForceCharacterSet);
  }
  return NS_OK;
}

static void
SetChildForceCharacterSet(nsIMarkupDocumentViewer* aChild, void* aClosure)
{
  aChild->SetForceCharacterSet((PRUnichar*) aClosure);
}

NS_IMETHODIMP DocumentViewerImpl::SetForceCharacterSet(const PRUnichar* aForceCharacterSet)
{
  mForceCharacterSet = aForceCharacterSet;
  // now set the force char set on all children of mContainer
  return CallChildren(SetChildForceCharacterSet, (void*) aForceCharacterSet);
}

// XXX: SEMANTIC CHANGE! 
//      returns a copy of the string.  Caller is responsible for freeing result
//      using Recycle(aHintCharacterSet)
NS_IMETHODIMP DocumentViewerImpl::GetHintCharacterSet(PRUnichar * *aHintCharacterSet)
{
  NS_ENSURE_ARG_POINTER(aHintCharacterSet);

  if(kCharsetUninitialized == mHintCharsetSource) {
    *aHintCharacterSet = nsnull;
  } else {
    *aHintCharacterSet = ToNewUnicode(mHintCharset);
    // this can't possibly be right.  we can't set a value just because somebody got a related value!
    //mHintCharsetSource = kCharsetUninitialized;
  }
  return NS_OK;
}

NS_IMETHODIMP DocumentViewerImpl::GetHintCharacterSetSource(PRInt32 *aHintCharacterSetSource)
{
  NS_ENSURE_ARG_POINTER(aHintCharacterSetSource);

  *aHintCharacterSetSource = mHintCharsetSource;
  return NS_OK;
}

static void
SetChildHintCharacterSetSource(nsIMarkupDocumentViewer* aChild, void* aClosure)
{
  aChild->SetHintCharacterSetSource(NS_PTR_TO_INT32(aClosure));
}

NS_IMETHODIMP DocumentViewerImpl::SetHintCharacterSetSource(PRInt32 aHintCharacterSetSource)
{
  mHintCharsetSource = aHintCharacterSetSource;
  // now set the hint char set source on all children of mContainer
  return CallChildren(SetChildHintCharacterSetSource,
                      (void*) aHintCharacterSetSource);
}

static void
SetChildHintCharacterSet(nsIMarkupDocumentViewer* aChild, void* aClosure)
{
  aChild->SetHintCharacterSet((PRUnichar*) aClosure);
}

NS_IMETHODIMP DocumentViewerImpl::SetHintCharacterSet(const PRUnichar* aHintCharacterSet)
{
  mHintCharset = aHintCharacterSet;
  // now set the hint char set on all children of mContainer
  return CallChildren(SetChildHintCharacterSet, (void*) aHintCharacterSet);
}

#ifdef IBMBIDI
static void
SetChildBidiOptions(nsIMarkupDocumentViewer* aChild, void* aClosure)
{
  aChild->SetBidiOptions(NS_PTR_TO_INT32(aClosure));
}

#endif // IBMBIDI

NS_IMETHODIMP DocumentViewerImpl::SetBidiTextDirection(PRUint8 aTextDirection)
{
#ifdef IBMBIDI
  PRUint32 bidiOptions;

  GetBidiOptions(&bidiOptions);
  SET_BIDI_OPTION_DIRECTION(bidiOptions, aTextDirection);
  SetBidiOptions(bidiOptions);
#endif
  return NS_OK;
}

NS_IMETHODIMP DocumentViewerImpl::GetBidiTextDirection(PRUint8* aTextDirection)
{
#ifdef IBMBIDI
  PRUint32 bidiOptions;

  if (aTextDirection) {
    GetBidiOptions(&bidiOptions);
    *aTextDirection = GET_BIDI_OPTION_DIRECTION(bidiOptions);
  }
#endif
  return NS_OK;
}
  
NS_IMETHODIMP DocumentViewerImpl::SetBidiTextType(PRUint8 aTextType)
{
#ifdef IBMBIDI
  PRUint32 bidiOptions;

  GetBidiOptions(&bidiOptions);
  SET_BIDI_OPTION_TEXTTYPE(bidiOptions, aTextType);
  SetBidiOptions(bidiOptions);
#endif
  return NS_OK;
}

NS_IMETHODIMP DocumentViewerImpl::GetBidiTextType(PRUint8* aTextType)
{
#ifdef IBMBIDI
  PRUint32 bidiOptions;

  if (aTextType) {
    GetBidiOptions(&bidiOptions);
    *aTextType = GET_BIDI_OPTION_TEXTTYPE(bidiOptions);
  }
#endif
  return NS_OK;
}

NS_IMETHODIMP DocumentViewerImpl::SetBidiControlsTextMode(PRUint8 aControlsTextMode)
{
#ifdef IBMBIDI
  PRUint32 bidiOptions;

  GetBidiOptions(&bidiOptions);
  SET_BIDI_OPTION_CONTROLSTEXTMODE(bidiOptions, aControlsTextMode);
  SetBidiOptions(bidiOptions);
#endif
  return NS_OK;
}

NS_IMETHODIMP DocumentViewerImpl::GetBidiControlsTextMode(PRUint8* aControlsTextMode)
{
#ifdef IBMBIDI
  PRUint32 bidiOptions;

  if (aControlsTextMode) {
    GetBidiOptions(&bidiOptions);
    *aControlsTextMode = GET_BIDI_OPTION_CONTROLSTEXTMODE(bidiOptions);
  }
#endif
  return NS_OK;
}

NS_IMETHODIMP DocumentViewerImpl::SetBidiClipboardTextMode(PRUint8 aClipboardTextMode)
{
#ifdef IBMBIDI
  PRUint32 bidiOptions;

  GetBidiOptions(&bidiOptions);
  SET_BIDI_OPTION_CLIPBOARDTEXTMODE(bidiOptions, aClipboardTextMode);
  SetBidiOptions(bidiOptions);
#endif
  return NS_OK;
}

NS_IMETHODIMP DocumentViewerImpl::GetBidiClipboardTextMode(PRUint8* aClipboardTextMode)
{
#ifdef IBMBIDI
  PRUint32 bidiOptions;

  if (aClipboardTextMode) {
    GetBidiOptions(&bidiOptions);
    *aClipboardTextMode = GET_BIDI_OPTION_CLIPBOARDTEXTMODE(bidiOptions);
  }
#endif
  return NS_OK;
}

NS_IMETHODIMP DocumentViewerImpl::SetBidiNumeral(PRUint8 aNumeral)
{
#ifdef IBMBIDI
  PRUint32 bidiOptions;

  GetBidiOptions(&bidiOptions);
  SET_BIDI_OPTION_NUMERAL(bidiOptions, aNumeral);
  SetBidiOptions(bidiOptions);
#endif
  return NS_OK;
}

NS_IMETHODIMP DocumentViewerImpl::GetBidiNumeral(PRUint8* aNumeral)
{
#ifdef IBMBIDI
  PRUint32 bidiOptions;

  if (aNumeral) {
    GetBidiOptions(&bidiOptions);
    *aNumeral = GET_BIDI_OPTION_NUMERAL(bidiOptions);
  }
#endif
  return NS_OK;
}

NS_IMETHODIMP DocumentViewerImpl::SetBidiSupport(PRUint8 aSupport)
{
#ifdef IBMBIDI
  PRUint32 bidiOptions;

  GetBidiOptions(&bidiOptions);
  SET_BIDI_OPTION_SUPPORT(bidiOptions, aSupport);
  SetBidiOptions(bidiOptions);
#endif
  return NS_OK;
}

NS_IMETHODIMP DocumentViewerImpl::GetBidiSupport(PRUint8* aSupport)
{
#ifdef IBMBIDI
  PRUint32 bidiOptions;

  if (aSupport) {
    GetBidiOptions(&bidiOptions);
    *aSupport = GET_BIDI_OPTION_SUPPORT(bidiOptions);
  }
#endif
  return NS_OK;
}

NS_IMETHODIMP DocumentViewerImpl::SetBidiCharacterSet(PRUint8 aCharacterSet)
{
#ifdef IBMBIDI
  PRUint32 bidiOptions;

  GetBidiOptions(&bidiOptions);
  SET_BIDI_OPTION_CHARACTERSET(bidiOptions, aCharacterSet);
  SetBidiOptions(bidiOptions);
#endif
  return NS_OK;
}

NS_IMETHODIMP DocumentViewerImpl::GetBidiCharacterSet(PRUint8* aCharacterSet)
{
#ifdef IBMBIDI
  PRUint32 bidiOptions;

  if (aCharacterSet) {
    GetBidiOptions(&bidiOptions);
    *aCharacterSet = GET_BIDI_OPTION_CHARACTERSET(bidiOptions);
  }
#endif
  return NS_OK;
}

NS_IMETHODIMP DocumentViewerImpl::SetBidiOptions(PRUint32 aBidiOptions)
{
#ifdef IBMBIDI
  if (mPresContext) {
#if 1
    // forcing reflow will cause bug 80352. Temp turn off force reflow and 
    // wait for simon@softel.co.il to find the real solution
    mPresContext->SetBidi(aBidiOptions, PR_FALSE); 
#else
    mPresContext->SetBidi(aBidiOptions, PR_TRUE); // force reflow
#endif
  }
  // now set bidi on all children of mContainer
  CallChildren(SetChildBidiOptions, (void*) aBidiOptions);
#endif // IBMBIDI
  return NS_OK;
}

NS_IMETHODIMP DocumentViewerImpl::GetBidiOptions(PRUint32* aBidiOptions)
{
#ifdef IBMBIDI
  if (aBidiOptions) {
    if (mPresContext) {
      mPresContext->GetBidi(aBidiOptions);
    }
    else
      *aBidiOptions = IBMBIDI_DEFAULT_BIDI_OPTIONS;
  }
#endif // IBMBIDI
  return NS_OK;
}

NS_IMETHODIMP DocumentViewerImpl::SizeToContent()
{
   NS_ENSURE_TRUE(mDocument, NS_ERROR_NOT_AVAILABLE);

   nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(mContainer));
   NS_ENSURE_TRUE(docShellAsItem, NS_ERROR_FAILURE);

   nsCOMPtr<nsIDocShellTreeItem> docShellParent;
   docShellAsItem->GetSameTypeParent(getter_AddRefs(docShellParent));

   // It's only valid to access this from a top frame.  Doesn't work from 
   // sub-frames.
   NS_ENSURE_TRUE(!docShellParent, NS_ERROR_FAILURE);

   nsCOMPtr<nsIPresShell> presShell;
   GetPresShell(*getter_AddRefs(presShell));
   NS_ENSURE_TRUE(presShell, NS_ERROR_FAILURE);

   NS_ENSURE_SUCCESS(presShell->ResizeReflow(NS_UNCONSTRAINEDSIZE,
      NS_UNCONSTRAINEDSIZE), NS_ERROR_FAILURE);

   nsCOMPtr<nsIPresContext> presContext;
   GetPresContext(*getter_AddRefs(presContext));
   NS_ENSURE_TRUE(presContext, NS_ERROR_FAILURE);

   nsRect  shellArea;
   PRInt32 width, height;
   float   pixelScale;

   // so how big is it?
   presContext->GetVisibleArea(shellArea);
   presContext->GetTwipsToPixels(&pixelScale);
   width = PRInt32((float)shellArea.width*pixelScale);
   height = PRInt32((float)shellArea.height*pixelScale);

   nsCOMPtr<nsIDocShellTreeOwner> treeOwner;
   docShellAsItem->GetTreeOwner(getter_AddRefs(treeOwner));
   NS_ENSURE_TRUE(treeOwner, NS_ERROR_FAILURE);

   /* presContext's size was calculated in twips and has already been
      rounded to the equivalent pixels (so the width/height calculation
      we just performed was probably exact, though it was based on
      values already rounded during ResizeReflow). In a surprising
      number of instances, this rounding makes a window which for want
      of one extra pixel's width ends up wrapping the longest line of
      text during actual window layout. This makes the window too short,
      generally clipping the OK/Cancel buttons. Here we add one pixel
      to the calculated width, to circumvent this problem. */
   NS_ENSURE_SUCCESS(treeOwner->SizeShellTo(docShellAsItem, width+1, height),
      NS_ERROR_FAILURE);

   return NS_OK;
}



#ifdef XP_MAC
#pragma mark -
#endif

NS_IMPL_ISUPPORTS1(nsDocViewerSelectionListener, nsISelectionListener);

nsresult nsDocViewerSelectionListener::Init(DocumentViewerImpl *aDocViewer)
{
  mDocViewer = aDocViewer;
  return NS_OK;
}

/*
 * GetPopupNode, GetPopupLinkNode and GetPopupImageNode are helpers
 * for the cmd_copyLink / cmd_copyImageLocation / cmd_copyImageContents family
 * of commands. The focus controller stores the popup node, these retrieve
 * them and munge appropriately. Note that we have to store the popup node
 * rather than retrieving it from EventStateManager::GetFocusedContent because
 * not all content (images included) can receive focus.
 */

nsresult
DocumentViewerImpl::GetPopupNode(nsIDOMNode** aNode)
{
  NS_ENSURE_ARG_POINTER(aNode);

  nsresult rv;

  // get the document
  nsCOMPtr<nsIDocument> document;
  rv = GetDocument(*getter_AddRefs(document));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(document, NS_ERROR_FAILURE);

  // get the script global object
  nsCOMPtr<nsIScriptGlobalObject> global;
  rv = document->GetScriptGlobalObject(getter_AddRefs(global));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(global, NS_ERROR_FAILURE);

  // get the internal dom window
  nsCOMPtr<nsIDOMWindowInternal> internalWin(do_QueryInterface(global, &rv));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(internalWin, NS_ERROR_FAILURE);  

  // get the private dom window
  nsCOMPtr<nsPIDOMWindow> privateWin(do_QueryInterface(internalWin, &rv));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(privateWin, NS_ERROR_FAILURE);

  // get the focus controller
  nsCOMPtr<nsIFocusController> focusController;
  rv = privateWin->GetRootFocusController(getter_AddRefs(focusController));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(focusController, NS_ERROR_FAILURE);

  // get the popup node
  rv = focusController->GetPopupNode(aNode); // addref happens here
  NS_ENSURE_SUCCESS(rv, rv);

  return rv;
}

// GetPopupLinkNode: return popup link node or fail
nsresult
DocumentViewerImpl::GetPopupLinkNode(nsIDOMNode** aNode)
{
  NS_ENSURE_ARG_POINTER(aNode);

  // you get null unless i say so
  *aNode = nsnull;

  // find popup node
  nsCOMPtr<nsIDOMNode> node;
  nsresult rv = GetPopupNode(getter_AddRefs(node));
  NS_ENSURE_SUCCESS(rv, rv);

  // find out if we have a link in our ancestry
  while (node) {

    // are we an anchor?
    nsCOMPtr<nsIDOMHTMLAnchorElement> anchor(do_QueryInterface(node));
    nsCOMPtr<nsIDOMHTMLAreaElement> area;
    nsCOMPtr<nsIDOMHTMLLinkElement> link;
    nsAutoString xlinkType;
    if (!anchor) {
      // area?
      area = do_QueryInterface(node);
      if (!area) {
        // link?
        link = do_QueryInterface(node);
        if (!link) {
          // XLink?
          nsCOMPtr<nsIDOMElement> element(do_QueryInterface(node));
          if (element) {
            element->GetAttributeNS(NS_LITERAL_STRING("http://www.w3.org/1999/xlink"),NS_LITERAL_STRING("type"),xlinkType);
          }
        }
      }
    }
    if (anchor || area || link || xlinkType.Equals(NS_LITERAL_STRING("simple"))) {
      *aNode = node;
      NS_IF_ADDREF(*aNode); // addref
      return NS_OK;
    }
    else {
      // if not, get our parent and keep trying...
      nsCOMPtr<nsIDOMNode> parentNode;
      node->GetParentNode(getter_AddRefs(parentNode));
      node = parentNode;
    }
  }

  // if we have no node, fail
  return NS_ERROR_FAILURE;
}

// GetPopupLinkNode: return popup image node or fail
nsresult
DocumentViewerImpl::GetPopupImageNode(nsIDOMNode** aNode)
{
  NS_ENSURE_ARG_POINTER(aNode);

  // you get null unless i say so
  *aNode = nsnull;

  // find popup node
  nsCOMPtr<nsIDOMNode> node;
  nsresult rv = GetPopupNode(getter_AddRefs(node));
  NS_ENSURE_SUCCESS(rv, rv);

  // XXX find out if we're an image. this really ought to look for objects
  // XXX with type "image/...", but this is good enough for now.
  nsCOMPtr<nsIDOMHTMLImageElement> img(do_QueryInterface(node, &rv));
  if (NS_FAILED(rv)) return rv;
  NS_ENSURE_TRUE(img, NS_ERROR_FAILURE);

  // if we made it here, we're an image.
  *aNode = node;
  NS_IF_ADDREF(*aNode); // addref
  return NS_OK;
}

/*
 * XXX dr
 * ------
 * These two functions -- GetInLink and GetInImage -- are kind of annoying
 * in that they only get called from the controller (in
 * nsDOMWindowController::IsCommandEnabled). The actual construction of the
 * context menus in communicator (nsContextMenu.js) has its own, redundant
 * tests. No big deal, but good to keep in mind if we ever clean context
 * menus.
 */

NS_IMETHODIMP DocumentViewerImpl::GetInLink(PRBool* aInLink)
{
#ifdef DEBUG_dr
  printf("dr :: DocumentViewerImpl::GetInLink\n");
#endif

  NS_ENSURE_ARG_POINTER(aInLink);

  // we're not in a link unless i say so
  *aInLink = PR_FALSE;

  // get the popup link
  nsCOMPtr<nsIDOMNode> node;
  nsresult rv = GetPopupLinkNode(getter_AddRefs(node));
  if (NS_FAILED(rv)) return rv;
  NS_ENSURE_TRUE(node, NS_ERROR_FAILURE);

  // if we made it here, we're in a link
  *aInLink = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP DocumentViewerImpl::GetInImage(PRBool* aInImage)
{
#ifdef DEBUG_dr
  printf("dr :: DocumentViewerImpl::GetInImage\n");
#endif

  NS_ENSURE_ARG_POINTER(aInImage);

  // we're not in an image unless i say so
  *aInImage = PR_FALSE;

  // get the popup image
  nsCOMPtr<nsIDOMNode> node;
  nsresult rv = GetPopupImageNode(getter_AddRefs(node));
  if (NS_FAILED(rv)) return rv;
  NS_ENSURE_TRUE(node, NS_ERROR_FAILURE);

  // if we made it here, we're in an image
  *aInImage = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP nsDocViewerSelectionListener::NotifySelectionChanged(nsIDOMDocument *, nsISelection *, short)
{
  NS_ASSERTION(mDocViewer, "Should have doc viewer!");

  // get the selection state
  nsCOMPtr<nsISelection> selection;
  nsresult rv = mDocViewer->GetDocumentSelection(getter_AddRefs(selection));
  if (NS_FAILED(rv)) return rv;

  PRBool selectionCollapsed;
  selection->GetIsCollapsed(&selectionCollapsed);
  // we only call UpdateCommands when the selection changes from collapsed
  // to non-collapsed or vice versa. We might need another update string
  // for simple selection changes, but that would be expenseive.
  if (!mGotSelectionState || mSelectionWasCollapsed != selectionCollapsed)
  {
    nsCOMPtr<nsIDocument> theDoc;
    mDocViewer->GetDocument(*getter_AddRefs(theDoc));  
    if (!theDoc) return NS_ERROR_FAILURE;
    
    nsCOMPtr<nsIScriptGlobalObject> scriptGlobalObject;
    theDoc->GetScriptGlobalObject(getter_AddRefs(scriptGlobalObject));

    nsCOMPtr<nsIDOMWindowInternal> domWindow = do_QueryInterface(scriptGlobalObject);
    if (!domWindow) return NS_ERROR_FAILURE;
    
    domWindow->UpdateCommands(NS_LITERAL_STRING("select"));
    mGotSelectionState = PR_TRUE;
    mSelectionWasCollapsed = selectionCollapsed;
  }  
  
  return NS_OK;
}

//nsDocViewerFocusListener
NS_IMPL_ISUPPORTS1(nsDocViewerFocusListener, nsIDOMFocusListener);

nsDocViewerFocusListener::nsDocViewerFocusListener()
:mDocViewer(nsnull)
{
  NS_INIT_REFCNT();
}

nsDocViewerFocusListener::~nsDocViewerFocusListener(){}

nsresult
nsDocViewerFocusListener::HandleEvent(nsIDOMEvent* aEvent)
{
  return NS_OK;
}

NS_IMETHODIMP
nsDocViewerFocusListener::Focus(nsIDOMEvent* aEvent)
{
  nsCOMPtr<nsIPresShell> shell;
  if(!mDocViewer)
    return NS_ERROR_FAILURE;

  nsresult result = mDocViewer->GetPresShell(*getter_AddRefs(shell));//deref once cause it take a ptr ref
  if(NS_FAILED(result) || !shell)
    return result?result:NS_ERROR_FAILURE;
  nsCOMPtr<nsISelectionController> selCon;
  selCon = do_QueryInterface(shell);
  PRInt16 selectionStatus;
  selCon->GetDisplaySelection( &selectionStatus);

  //if selection was nsISelectionController::SELECTION_OFF, do nothing
  //otherwise re-enable it. 
  if(selectionStatus == nsISelectionController::SELECTION_DISABLED ||
     selectionStatus == nsISelectionController::SELECTION_HIDDEN)
  {
    selCon->SetDisplaySelection(nsISelectionController::SELECTION_ON);
    selCon->RepaintSelection(nsISelectionController::SELECTION_NORMAL);
  }
  return result;
}

NS_IMETHODIMP
nsDocViewerFocusListener::Blur(nsIDOMEvent* aEvent)
{
  nsCOMPtr<nsIPresShell> shell;
  if(!mDocViewer) 
    return NS_ERROR_FAILURE;

  nsresult result = mDocViewer->GetPresShell(*getter_AddRefs(shell));//deref once cause it take a ptr ref
  if(NS_FAILED(result) || !shell)
    return result?result:NS_ERROR_FAILURE;
  nsCOMPtr<nsISelectionController> selCon;
  selCon = do_QueryInterface(shell);
  PRInt16 selectionStatus;
  selCon->GetDisplaySelection(&selectionStatus);
 
  //if selection was nsISelectionController::SELECTION_OFF, do nothing
  //otherwise re-enable it.
  if(selectionStatus == nsISelectionController::SELECTION_ON)
  {
    selCon->SetDisplaySelection(nsISelectionController::SELECTION_DISABLED);
    selCon->RepaintSelection(nsISelectionController::SELECTION_NORMAL);
  }
  return result;
}


nsresult
nsDocViewerFocusListener::Init(DocumentViewerImpl *aDocViewer)
{
  mDocViewer = aDocViewer;
  return NS_OK;
}


PRBool 
DocumentViewerImpl::IsWindowsInOurSubTree(nsIDOMWindowInternal * aDOMWindow)
{
  PRBool found = PR_FALSE;
  if(aDOMWindow) {
    // now check to make sure it is in "our" tree of webshells
    nsCOMPtr<nsIScriptGlobalObject> scriptObj(do_QueryInterface(aDOMWindow));
    if (scriptObj) {
      nsCOMPtr<nsIDocShell> docShell;
      scriptObj->GetDocShell(getter_AddRefs(docShell));
      if (docShell) {
        nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(docShell));
        if (docShellAsItem) {
          // get this DocViewer webshell
          nsCOMPtr<nsIWebShell> thisDVWebShell(do_QueryInterface(mContainer));
          while (!found) {
            nsCOMPtr<nsIDocShellTreeItem> docShellParent;
            docShellAsItem->GetSameTypeParent(getter_AddRefs(docShellParent));
            if (docShellParent) {
              nsCOMPtr<nsIWebShell> parentWebshell(do_QueryInterface(docShellParent));
              if (parentWebshell) {
                if (parentWebshell.get() == thisDVWebShell.get()) {
                  found = PR_TRUE;
                  break;
                }
              }
            } else {
              break; // at top of tree
            }
            docShellAsItem = docShellParent;
          } // while
        }
      } // docshell
    } // scriptobj
  } // domWindow

  return found;
}

/** ---------------------------------------------------
 *  Get the Focused Frame for a documentviewer
 *  
 */
nsIDOMWindowInternal*
DocumentViewerImpl::FindFocusedDOMWindowInternal()
{
  nsCOMPtr<nsIDOMWindowInternal>  theDOMWin;
  nsCOMPtr<nsIDocument>           theDoc;
  nsCOMPtr<nsIScriptGlobalObject> theSGO;
  nsCOMPtr<nsIFocusController>    focusController;
  nsIDOMWindowInternal *          domWin = nsnull;

  this->GetDocument(*getter_AddRefs(theDoc));  
  if(theDoc){
    theDoc->GetScriptGlobalObject(getter_AddRefs(theSGO));
    if(theSGO){
      nsCOMPtr<nsPIDOMWindow> theDOMWindow = do_QueryInterface(theSGO);
      if(theDOMWindow){
        theDOMWindow->GetRootFocusController(getter_AddRefs(focusController));
        if(focusController){
          focusController->GetFocusedWindow(getter_AddRefs(theDOMWin));
          domWin = theDOMWin.get();
          if(domWin != nsnull) {
            if (IsWindowsInOurSubTree(domWin)){
              NS_ADDREF(domWin);
            } else {
              domWin = nsnull;
            }
          }
        }
      }
    }
  }
  return domWin;
}

/*=============== nsIEventProcessor for PrintPreview ======================*/
// This is the method that gets plugged into the ViewManager 
// so we can decide what events get discard.
NS_IMETHODIMP 
DocumentViewerImpl::ProcessEvent(nsGUIEvent *aEvent, PRBool aIsInContentArea, nsEventStatus *aStatus) const
{

  // short circut for mouse moves
  if (aEvent->message == NS_MOUSE_MOVE) {
    return NS_OK;
  }

  // check to see if it is a mouse event in the content area
  // and discard it if it is
  if (NS_IS_MOUSE_EVENT(aEvent)) {
    if (aIsInContentArea) {
      *aStatus = nsEventStatus_eConsumeNoDefault;
      return NS_ERROR_FAILURE;
    }
  } else if (aEvent->message != NS_PAINT && // these are the events we let thru
             aEvent->message != NS_DESTROY && 
             aEvent->message != NS_SIZE && 
             aEvent->message != NS_SCROLL_EVENT && 
             aEvent->message != NS_SCROLLBAR_POS && 
             aEvent->message != NS_SCROLLBAR_PAGE_NEXT && 
             aEvent->message != NS_SCROLLBAR_PAGE_PREV && 
             aEvent->message != NS_SCROLLBAR_LINE_NEXT && 
             aEvent->message != NS_SCROLLBAR_LINE_PREV && 
             aEvent->message != NS_MOUSE_SCROLL && 
             aEvent->message != NS_SCROLLPORT_UNDERFLOW && 
             aEvent->message != NS_SCROLLPORT_OVERFLOW && 
             aEvent->message != NS_SCROLLPORT_OVERFLOWCHANGED &&
             aEvent->message != NS_ACTIVATE &&
             aEvent->message != NS_SETZLEVEL
             ) {
#ifdef DEBUG_rods
    //printf("Discarding %d\n", aEvent->message);
#endif
    *aStatus = nsEventStatus_eConsumeNoDefault;
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

/*=============== Timer Related Code ======================*/
nsresult
DocumentViewerImpl::StartPagePrintTimer(nsIPresContext * aPresContext, 
                                        nsIPrintSettings* aPrintSettings,
                                        PrintObject*     aPOect,
                                        PRUint32         aDelay)
{
  nsresult result;

  if (!mPagePrintTimer) {
    result = NS_NewUpdateTimer(&mPagePrintTimer);

    if (NS_FAILED(result))
      return result;

    ++mDestroyRefCount;
  }

  return mPagePrintTimer->Start(this, aPresContext, aPrintSettings, aPOect, aDelay);
}

