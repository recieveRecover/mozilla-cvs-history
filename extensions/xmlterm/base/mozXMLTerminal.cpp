/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "MPL"); you may not use this file
 * except in compliance with the MPL. You may obtain a copy of
 * the MPL at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the MPL is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the MPL for the specific language governing
 * rights and limitations under the MPL.
 * 
 * The Original Code is XMLterm.
 * 
 * The Initial Developer of the Original Code is Ramalingam Saravanan.
 * Portions created by Ramalingam Saravanan <svn@xmlterm.org> are
 * Copyright (C) 1999 Ramalingam Saravanan. All Rights Reserved.
 * 
 * Contributor(s):
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */

// mozXMLTerminal.cpp: implementation of mozIXMLTerminal interface
// to manage all XMLTerm operations.
// Creates a new mozXMLTermSession object to manage session input/output.
// Creates a mozLineTermAux object to access LineTerm operations.
// Creates key/text/mouse/drag listener objects to handle user events.

#include "nscore.h"
#include "nspr.h"

#include "nsCOMPtr.h"
#include "nsString.h"

#include "nsIDocument.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDocumentViewer.h"
#include "nsIDocumentLoaderObserver.h"
#include "nsIObserver.h"

#include "nsIPresContext.h"
#include "nsICaret.h"

#include "nsIDOMEventReceiver.h"
#include "nsIDOMEventListener.h"

#include "nsIServiceManager.h"
#include "nsISupportsPrimitives.h"

#include "nsWidgetsCID.h"
#include "nsIClipboard.h"
#include "nsITransferable.h"

#include "nsFont.h"
#include "nsIFontMetrics.h"

#include "mozXMLT.h"
#include "mozXMLTermUtils.h"
#include "mozXMLTerminal.h"
#include "mozIXMLTermSuspend.h"

////////////////////////////////////////////////////////////////////////

static NS_DEFINE_IID(kISupportsIID,       NS_ISUPPORTS_IID);

static NS_DEFINE_IID(kIXMLTerminalIID,    MOZIXMLTERMINAL_IID);
static NS_DEFINE_IID(kXMLTerminalCID,     MOZXMLTERMINAL_CID);

static NS_DEFINE_CID(kCClipboardCID,      NS_CLIPBOARD_CID);
static NS_DEFINE_CID(kCTransferableCID,   NS_TRANSFERABLE_CID);

/////////////////////////////////////////////////////////////////////////
// mozXMLTerminal factory
/////////////////////////////////////////////////////////////////////////

nsresult
NS_NewXMLTerminal(mozIXMLTerminal** aXMLTerminal)
{
    NS_PRECONDITION(aXMLTerminal != nsnull, "null ptr");
    if (!aXMLTerminal)
        return NS_ERROR_NULL_POINTER;

    *aXMLTerminal = new mozXMLTerminal();
    if (! *aXMLTerminal)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(*aXMLTerminal);
    return NS_OK;
}


/////////////////////////////////////////////////////////////////////////
// mozXMLTerminal implementation
/////////////////////////////////////////////////////////////////////////
mozXMLTerminal::mozXMLTerminal() :
  mInitialized(PR_FALSE),

  mCookie(""),

  mCommand(""),
  mPromptExpr(""),

  mInitInput(""),

  mXMLTermShell(nsnull),
  mDocShell(nsnull),
  mPresShell(nsnull),
  mDOMDocument(nsnull),

  mXMLTermSession(nsnull),

  mLineTermAux(nsnull),
  mNeedsResizing(false),

  mKeyListener(nsnull),
  mTextListener(nsnull),
  mMouseListener(nsnull),
  mDragListener(nsnull)
{
  NS_INIT_REFCNT();
}


mozXMLTerminal::~mozXMLTerminal()
{
  if (mInitialized) {
    Finalize();
  }
}


// Implement AddRef and Release
NS_IMPL_ADDREF(mozXMLTerminal)
NS_IMPL_RELEASE(mozXMLTerminal)


NS_INTERFACE_MAP_BEGIN(mozXMLTerminal)
		/*
			I maintained the order from the original, however,
			the original called |XMLT_LOG| and in the interface-map form
			it no longer does.  Is this an issue?
		*/
	NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, mozIXMLTerminal)
	NS_INTERFACE_MAP_ENTRY(mozIXMLTerminal)
	NS_INTERFACE_MAP_ENTRY(nsIDocumentLoaderObserver)
	NS_INTERFACE_MAP_ENTRY(nsIObserver)
	NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
NS_INTERFACE_MAP_END


NS_IMETHODIMP mozXMLTerminal::GetCurrentEntryNumber(PRInt32 *aNumber)
{
  if (!mXMLTermSession)
    return NS_ERROR_FAILURE;

  return mXMLTermSession->GetCurrentEntryNumber(aNumber);
}


NS_IMETHODIMP mozXMLTerminal::GetHistory(PRInt32 *aHistory)
{
  if (!mXMLTermSession)
    return NS_ERROR_FAILURE;

  return mXMLTermSession->GetHistory(aHistory);
}


NS_IMETHODIMP mozXMLTerminal::SetHistory(PRInt32 aHistory)
{
  if (!mXMLTermSession)
    return NS_ERROR_FAILURE;

  return mXMLTermSession->SetHistory(aHistory);
}

NS_IMETHODIMP mozXMLTerminal::GetPrompt(PRUnichar **aPrompt)
{
  if (!mXMLTermSession)
    return NS_ERROR_FAILURE;

  return mXMLTermSession->GetPrompt(aPrompt);
}


NS_IMETHODIMP mozXMLTerminal::SetPrompt(const PRUnichar* aPrompt)
{
  if (!mXMLTermSession)
    return NS_ERROR_FAILURE;

  return mXMLTermSession->SetPrompt(aPrompt);
}


NS_IMETHODIMP mozXMLTerminal::GetKeyIgnore(PRBool* aIgnore)
{
  if (!mKeyListener)
    return NS_ERROR_FAILURE;

  nsCOMPtr<mozIXMLTermSuspend> suspend= do_QueryInterface(mKeyListener);
  if (!suspend)
    return NS_ERROR_FAILURE;

  return suspend->GetSuspend(aIgnore);
}


NS_IMETHODIMP mozXMLTerminal::SetKeyIgnore(const PRBool aIgnore)
{
  if (!mKeyListener)
    return NS_ERROR_FAILURE;

  nsCOMPtr<mozIXMLTermSuspend> suspend= do_QueryInterface(mKeyListener);
  if (!suspend)
    return NS_ERROR_FAILURE;

  return suspend->SetSuspend(aIgnore);
}


// Initialize by starting load of Init page for XMLTerm
NS_IMETHODIMP mozXMLTerminal::Init(nsIDocShell* aDocShell,
                                   mozIXMLTermShell* aXMLTermShell,
                                   const PRUnichar* aURL,
                                   const PRUnichar* args)
{
  XMLT_LOG(mozXMLTerminal::Init,20,("\n"));

  if (!aDocShell)
      return NS_ERROR_NULL_POINTER;

  if (mDocShell)
    return NS_ERROR_ALREADY_INITIALIZED;

  mDocShell = aDocShell;          // containing docshell; no addref

  mXMLTermShell = aXMLTermShell;  // containing xmlterm shell; no addref

  nsresult result = NS_OK;

  printf("mozXMLTerminal::Init, check1\n");
  nsCOMPtr<nsIContentViewer> contViewer;
  result = mDocShell->GetContentViewer(getter_AddRefs(contViewer));
  printf("mozXMLTerminal::Init, check2, result=%x, contViewer=%x\n",
         result, (unsigned int) contViewer.get());

  // NOTE: Need to parse args string!!!
  mCommand = "";
  mPromptExpr = "";
  mInitInput = args;

  // Initialization completed
  mInitialized = PR_TRUE;

  if ((aURL != nsnull) && (*aURL != 0)) {
    // Load URL and activate XMLTerm after loading
    XMLT_LOG(mozXMLTerminal::Init,22,("setting DocLoaderObs\n"));

    // About to create owning reference to this
    result = mDocShell->SetDocLoaderObserver((nsIDocumentLoaderObserver*)this);
    if (NS_FAILED(result))
      return NS_ERROR_FAILURE;

    XMLT_LOG(mozXMLTerminal::Init,22,("done setting DocLoaderObs\n"));

    // Load initial XMLterm background document
    nsCAutoString urlCString(aURL);

    nsCOMPtr<nsIURI> uri;
    result = uri->SetSpec(urlCString.GetBuffer());
    if (NS_FAILED(result))
      return NS_ERROR_FAILURE;

    result = mDocShell->LoadURI(uri, nsnull);
    if (NS_FAILED(result))
      return NS_ERROR_FAILURE;

  } else {
    // Document already loaded; activate XMLTerm
    result = Activate();
    if (NS_FAILED(result))
      return NS_ERROR_FAILURE;
  }

  XMLT_LOG(mozXMLTerminal::Init,21,("exiting\n"));
  return result;
}


// De-initialize XMLTerminal
NS_IMETHODIMP mozXMLTerminal::Finalize(void)
{
  if (mXMLTermSession) {
    // Finalize XMLTermSession object and delete it (it is not ref. counted)
    mXMLTermSession->Finalize();
    delete mXMLTermSession;
    mXMLTermSession = nsnull;
  }

  if (mDOMDocument) {
    // Release any event listeners for the document
    nsCOMPtr<nsIDOMEventReceiver> eventReceiver;
    nsresult result = mDOMDocument->QueryInterface(NS_GET_IID(nsIDOMEventReceiver), getter_AddRefs(eventReceiver));

    if (NS_SUCCEEDED(result) && eventReceiver) {
      if (mKeyListener) {
        eventReceiver->RemoveEventListenerByIID(mKeyListener,
                                                NS_GET_IID(nsIDOMKeyListener));
        mKeyListener = nsnull;
      }

      if (mTextListener) {
        eventReceiver->RemoveEventListenerByIID(mTextListener,
                                                NS_GET_IID(nsIDOMTextListener));
        mTextListener = nsnull;
      }

      if (mMouseListener) {
        eventReceiver->RemoveEventListenerByIID(mMouseListener,
                                                NS_GET_IID(nsIDOMMouseListener));
        mMouseListener = nsnull;
      }

      if (mDragListener) {
        eventReceiver->RemoveEventListenerByIID(mDragListener,
                                                NS_GET_IID(nsIDOMDragListener));
        mDragListener = nsnull;
      }
    }
    mDOMDocument = nsnull;
  }

  if (mLineTermAux) {
    // Finalize and release reference to LineTerm object owned by us
    mLineTermAux->CloseAux();
    mLineTermAux = nsnull;
  }

  if (mDocShell) {
    // Stop observing document loading
    mDocShell->SetDocLoaderObserver((nsIDocumentLoaderObserver *)nsnull);
    mDocShell = nsnull;
  }

  mPresShell = nsnull;
  mXMLTermShell = nsnull;

  mInitialized = PR_FALSE;

  return NS_OK;
}


/** Activates XMLterm and instantiates LineTerm;
 * called at the the end of Init page loading.
 */
NS_IMETHODIMP mozXMLTerminal::Activate(void)
{
  nsresult result = NS_OK;

#if 0
  // TEMPORARY: Testing mozIXMLTermStream
  nsAutoString streamData = "<HTML><HEAD><TITLE>Stream Title</TITLE>"
                            "<SCRIPT language='JavaScript'>"
                            "function clik(){ dump('click\\n');return(false);}"
                            "</SCRIPT></HEAD>"
                            "<BODY><B>Stream Body "
                            "<SPAN STYLE='color: blue' onClick='return clik();'>Clik</SPAN></B><BR>"
"<TABLE WIDTH=720><TR><TD WIDTH=700 BGCOLOR=maroon>&nbsp;</TABLE>"
                            "<BR>ABCD<BR>EFGH<BR>JKLM<BR>"
                            "</BODY></HTML>";

  nsCOMPtr<mozIXMLTermStream> stream;
  result = NS_NewXMLTermStream(getter_AddRefs(stream));
  if (NS_FAILED(result)) {
    fprintf(stderr, "mozXMLTerminal::Activate: Failed to create stream\n");
    return result;
  }

  nsCOMPtr<nsIDOMWindow> outerDOMWindow;
  result = mozXMLTermUtils::ConvertDocShellToDOMWindow(mDocShell,
                                              getter_AddRefs(outerDOMWindow));

  if (NS_FAILED(result) || !outerDOMWindow) {
    fprintf(stderr, "mozXMLTerminal::Activate: Failed to convert docshell\n");
    return NS_ERROR_FAILURE;
  }

  result = stream->Open(outerDOMWindow, "iframet", "chrome://dummy",
                        "text/html", 800);
  if (NS_FAILED(result)) {
    fprintf(stderr, "mozXMLTerminal::Activate: Failed to open stream\n");
    return result;
  }

  result = stream->Write(streamData.GetUnicode());
  if (NS_FAILED(result)) {
    fprintf(stderr, "mozXMLTerminal::Activate: Failed to write to stream\n");
    return result;
  }

  result = stream->Close();
  if (NS_FAILED(result)) {
    fprintf(stderr, "mozXMLTerminal::Activate: Failed to close stream\n");
    return result;
  }

#endif

  XMLT_LOG(mozXMLTerminal::Activate,20,("\n"));

  if (!mInitialized)
    return NS_ERROR_NOT_INITIALIZED;

  PR_ASSERT(mDocShell != nsnull);

  if ((mDOMDocument != nsnull) || (mPresShell != nsnull))
    return NS_ERROR_FAILURE;

  // Get reference to presentation shell
  nsCOMPtr<nsIPresContext> presContext;
  result = mDocShell->GetPresContext(getter_AddRefs(presContext));
  if (NS_FAILED(result) || !presContext)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIPresShell> presShell;
  result = mDocShell->GetPresShell(getter_AddRefs(presShell));
  if (NS_FAILED(result) || !presShell)
    return NS_ERROR_FAILURE;

  // Get reference to DOMDocument
  nsCOMPtr<nsIDocument> document;
  result = presShell->GetDocument(getter_AddRefs(document));

  if (NS_FAILED(result) || !document)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMDocument> domDocument = do_QueryInterface(document);
  if (!domDocument)
    return NS_ERROR_FAILURE;

  // Save references to presentation shell and DOMDocument
  // (SVN: Should these be addref'ed and released in the destructor?)
  mPresShell = presShell;      // no addref
  mDOMDocument = domDocument;  // no addref

  // Show caret
  ShowCaret();

  // Determine current screen dimensions
  PRInt32 nRows, nCols, xPixels, yPixels;
  result = ScreenSize(nRows, nCols, xPixels, yPixels);
  if (NS_FAILED(result))
    return result;

  // The above call does not return the correct screen dimensions.
  // (because rendering is still in progress?)
  // As a workaround, explicitly set screen dimensions
  nRows = 24;
  nCols = 80;
  xPixels = 0;
  yPixels = 0;

  // Instantiate and initialize XMLTermSession object
  mXMLTermSession = new mozXMLTermSession();

  if (!mXMLTermSession) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  result = mXMLTermSession->Init(this, mPresShell, mDOMDocument, nRows, nCols);
  if (NS_FAILED(result)) {
    XMLT_WARNING("mozXMLTerminal::Activate: Warning - Failed to initialize XMLTermSession\n");
    return NS_ERROR_FAILURE;
  }

  // Instantiate LineTerm  
  XMLT_LOG(mozXMLTerminal::Activate,22,
           ("instantiating lineterm, nRows=%d, nCols=%d\n", nRows, nCols));
  result = NS_NewLineTermAux(getter_AddRefs(mLineTermAux));
  if (NS_FAILED(result)) {
    XMLT_WARNING("mozXMLTerminal::Activate: Warning - Failed to instantiate LineTermAux\n");
    return result;
  }

  // Open LineTerm to execute command
  // Non-owning reference to this; delete LineTerm before deleting self
  PRInt32 options = 0;

  XMLT_LOG(mozXMLTerminal::Activate,22,("Opening LineTerm\n"));
  nsCOMPtr<nsIObserver> anObserver = this;
#ifdef NO_CALLBACK
  anObserver = nsnull;
#endif
  nsAutoString cookie;
  result = mLineTermAux->OpenAux(mCommand.GetUnicode(),
                                 mInitInput.GetUnicode(),
                                 mPromptExpr.GetUnicode(),
                                 options, LTERM_DETERMINE_PROCESS,
                                 nRows, nCols, xPixels, yPixels,
                                 mDOMDocument, anObserver, cookie);

  if (NS_FAILED(result)) {
    XMLT_WARNING("mozXMLTerminal::Activate: Warning - Failed to open LineTermAux\n");
    return result;
  }
  XMLT_LOG(mozXMLTerminal::Activate,22,("Opened LineTerm\n"));

  // Save cookie
  mCookie = cookie;

  // Get the DOM event receiver for document
  nsCOMPtr<nsIDOMEventReceiver> eventReceiver;
  result = mDOMDocument->QueryInterface(NS_GET_IID(nsIDOMEventReceiver),
                                        getter_AddRefs(eventReceiver));
  if (NS_FAILED(result)) {
    XMLT_WARNING("mozXMLTerminal::Activate: Warning - Failed to get DOM receiver\n");
    return result;
  }

  // Create a key listener
  result = NS_NewXMLTermKeyListener(getter_AddRefs(mKeyListener), this);
  if (NS_OK != result) {
    XMLT_WARNING("mozXMLTerminal::Activate: Warning - Failed to get key listener\n");
    return result;
  }

  // Register the key listener with the DOM event receiver
  result = eventReceiver->AddEventListenerByIID(mKeyListener,
                                                NS_GET_IID(nsIDOMKeyListener));

  if (NS_FAILED(result)) {
    XMLT_WARNING("mozXMLTerminal::Activate: Warning - Failed to register key listener\n");
    return result;
  }

  // Create a text listener
  result = NS_NewXMLTermTextListener(getter_AddRefs(mTextListener), this);
  if (NS_OK != result) {
    XMLT_WARNING("mozXMLTerminal::Activate: Warning - Failed to get text listener\n");
    return result;
  }

  // Register the text listener with the DOM event receiver
  result = eventReceiver->AddEventListenerByIID(mTextListener,
                                                NS_GET_IID(nsIDOMTextListener));
  if (NS_FAILED(result)) {
    XMLT_WARNING("mozXMLTerminal::Activate: Warning - Failed to register text listener\n");
    return result;
  }

  // Create a mouse listener
  result = NS_NewXMLTermMouseListener(getter_AddRefs(mMouseListener), this);
  if (NS_OK != result) {
    XMLT_WARNING("mozXMLTerminal::Activate: Warning - Failed to get mouse listener\n");
    return result;
  }

  // Register the mouse listener with the DOM event receiver
  result = eventReceiver->AddEventListenerByIID(mMouseListener,
                                                NS_GET_IID(nsIDOMMouseListener));
  if (NS_FAILED(result)) {
    XMLT_WARNING("mozXMLTerminal::Activate: Warning - Failed to register mouse listener\n");
    return result;
  }

  // Create a drag listener
  result = NS_NewXMLTermDragListener(getter_AddRefs(mDragListener), this);
  if (NS_OK != result) {
    XMLT_WARNING("mozXMLTerminal::Activate: Warning - Failed to get drag listener\n");
    return result;
  }

  // Register the drag listener with the DOM event receiver
  result = eventReceiver->AddEventListenerByIID(mDragListener,
                                                NS_GET_IID(nsIDOMDragListener));
  if (NS_FAILED(result)) {
    XMLT_WARNING("mozXMLTerminal::Activate: Warning - Failed to register drag listener\n");
    return result;
  }

  return NS_OK;
}


/** Returns current screen size in rows/cols and in pixels
 * @param (output) rows
 * @param (output) cols
 * @param (output) xPixels
 * @param (output) yPixels
 */
NS_IMETHODIMP mozXMLTerminal::ScreenSize(PRInt32& rows, PRInt32& cols,
                                         PRInt32& xPixels, PRInt32& yPixels)
{
  nsresult result;

  XMLT_LOG(mozXMLTerminal::ScreenSize,70,("\n"));

  // Get presentation context
  nsCOMPtr<nsIPresContext> presContext;
  result = mPresShell->GetPresContext( getter_AddRefs(presContext) );
  if (NS_FAILED(result))
    return result;

  // Get the default fixed pitch font
  nsFont defaultFixedFont("dummyfont", NS_FONT_STYLE_NORMAL,
                          NS_FONT_VARIANT_NORMAL,
                          NS_FONT_WEIGHT_NORMAL,
                          NS_FONT_DECORATION_NONE, 16);

  result = presContext->GetDefaultFixedFont(defaultFixedFont);
  if (NS_FAILED(result))
    return result;

  // Get metrics for fixed font
  nsCOMPtr<nsIFontMetrics> fontMetrics;
  result = presContext->GetMetricsFor(defaultFixedFont,
                                      getter_AddRefs(fontMetrics));
  if (NS_FAILED(result) || !fontMetrics)
    return result;

  // Get font height (includes leading?)
  nscoord fontHeight, fontWidth;
  result = fontMetrics->GetHeight(fontHeight);
  result = fontMetrics->GetMaxAdvance(fontWidth);

  // Determine docshell size in twips
  nsRect shellArea;
  result = presContext->GetVisibleArea(shellArea);
  if (NS_FAILED(result))
    return result;

  // Determine twips to pixels conversion factor
  float pixelScale;
  presContext->GetTwipsToPixels(&pixelScale);

  // Convert dimensions to pixels
  float xdel, ydel;
  xdel = pixelScale * fontWidth;
  ydel = pixelScale * fontHeight + 2;

  xPixels = (int) (pixelScale * shellArea.width);
  yPixels = (int) (pixelScale * shellArea.height);

  // Determine number of rows/columns
  rows = (int) ((yPixels-16) / ydel);
  cols = (int) ((xPixels-20) / xdel);

  if (rows < 1) rows = 1;
  if (cols < 1) cols = 1;

  XMLT_LOG(mozXMLTerminal::ScreenSize,72,
           ("rows=%d, cols=%d, xPixels=%d, yPixels=%d\n",
            rows, cols, xPixels, yPixels));

  return NS_OK;
}


// Transmit string to LineTerm (use saved cookie)
NS_IMETHODIMP mozXMLTerminal::SendTextAux(const nsString& aString)
{
  return SendText(aString, mCookie.GetUnicode());
}


// Transmit string to LineTerm
NS_IMETHODIMP mozXMLTerminal::SendText(const nsString& aString,
                                       const PRUnichar* aCookie)
{
  nsresult result;

  if (!mLineTermAux)
    return NS_ERROR_FAILURE;

  nsAutoString sendStr = aString;

  // Preprocess string and check if it is to be consumed
  PRBool consumed, checkSize;
  result = mXMLTermSession->Preprocess(sendStr, consumed, checkSize);

  PRBool screenMode;
  GetScreenMode(&screenMode);

  if (!screenMode && (checkSize || mNeedsResizing)) {
    // Resize terminal, if need be
    mXMLTermSession->Resize(mLineTermAux);
    mNeedsResizing = false;
  }

  if (!consumed) {
    result = mLineTermAux->Write(sendStr.GetUnicode(), aCookie);
    if (NS_FAILED(result)) {
      // Abort XMLterm session
      nsAutoString abortCode = "SendText";
      mXMLTermSession->Abort(mLineTermAux, abortCode);
      return NS_ERROR_FAILURE;
    }
  }

  return NS_OK;
}

/** Shows the caret and make it editable.
 */
NS_IMETHODIMP mozXMLTerminal::ShowCaret(void)
{
  // In principle, this method needs to be called only once;
  // in practice, certain operations seem to hide the caret
  // especially when one starts mucking around with the display:
  // style property.
  // Under those circumstances, call this method to re-display the caret.

  if (!mPresShell)
    return NS_ERROR_FAILURE;

  mPresShell->SetCaretEnabled(PR_TRUE);

  nsCOMPtr<nsICaret> caret;
  if (NS_SUCCEEDED(mPresShell->GetCaret(getter_AddRefs(caret)))) {
    caret->SetCaretVisible(PR_TRUE);
    caret->SetCaretReadOnly(PR_FALSE);
  }

  return NS_OK;
}


// Paste data from clipboard to terminal
NS_IMETHODIMP mozXMLTerminal::Paste()
{
  nsresult result;
  nsAutoString pasteString;

  XMLT_LOG(mozXMLTerminal::Paste,20,("\n"));

  // Get Clipboard service
  NS_WITH_SERVICE (nsIClipboard, clipboard, kCClipboardCID, &result);
  if ( NS_FAILED(result) )
    return result;
    
  // Generic transferable for getting clipboard data
  nsCOMPtr<nsITransferable> trans;
  result = nsComponentManager::CreateInstance(kCTransferableCID, nsnull, 
                                              NS_GET_IID(nsITransferable), 
                                              (void**) getter_AddRefs(trans));

  if (NS_FAILED(result) || !trans)
    return NS_ERROR_FAILURE;

  // DataFlavors to get out of transferable
  trans->AddDataFlavor(kHTMLMime);
  trans->AddDataFlavor(kUnicodeMime);

  // Get data from clipboard
  result = clipboard->GetData(trans, nsIClipboard::kGlobalClipboard);
  if (NS_FAILED(result))
    return result;

  char* bestFlavor = nsnull;
  nsCOMPtr<nsISupports> genericDataObj;
  PRUint32 objLen = 0;
  result = trans->GetAnyTransferData(&bestFlavor,
                                     getter_AddRefs(genericDataObj), &objLen);
  if (NS_FAILED(result))
    return result;

  nsAutoString flavor ( bestFlavor );

  char* temCStr = flavor.ToNewCString();
  XMLT_LOG(mozXMLTerminal::Paste,20,("flavour=%s\n", temCStr));
  nsAllocator::Free(temCStr);

  if (flavor.Equals(kHTMLMime) || flavor.Equals(kUnicodeMime)) {
    nsCOMPtr<nsISupportsWString> textDataObj ( do_QueryInterface(genericDataObj) );
    if (textDataObj && objLen > 0) {
      PRUnichar* text = nsnull;
      textDataObj->ToString ( &text );
      pasteString.SetString ( text, objLen / 2 );
      result = SendTextAux(pasteString);
    }
  }
  nsAllocator::Free(bestFlavor);

  return NS_OK;
}


// Poll for readable data from LineTerm
NS_IMETHODIMP mozXMLTerminal::Poll(void)
{
  if (!mLineTermAux)
    return NS_ERROR_NOT_INITIALIZED;

  XMLT_LOG(mozXMLTerminal::Poll,20,("\n"));

  nsresult result;
  PRBool processedData;

  result = mXMLTermSession->ReadAll(mLineTermAux, processedData);
  if (NS_FAILED(result)) {
    XMLT_WARNING("mozXMLTerminal::Poll: Warning - Error return 0x%x from ReadAll\n", result);
    return result;
  }

  return NS_OK;
}


// Handle callback from LineTerm when new input/output needs to be displayed
NS_IMETHODIMP mozXMLTerminal::Observe(nsISupports *aSubject,
                                  const PRUnichar *aTopic,
                                  const PRUnichar *someData)
{
  nsCOMPtr<mozILineTermAux> lineTermAux = do_QueryInterface(aSubject);
  PR_ASSERT(lineTermAux != nsnull);

  PR_ASSERT(lineTermAux.get() == mLineTermAux.get());

  return Poll();
}


// Returns document associated with XMLTerminal
NS_IMETHODIMP mozXMLTerminal::GetDocument(nsIDOMDocument** aDoc)
{
  if (!aDoc)
    return NS_ERROR_NULL_POINTER;

  *aDoc = nsnull;

  NS_PRECONDITION(mDOMDocument, "bad state, null mDOMDocument");
  if (!mDOMDocument)
    return NS_ERROR_NOT_INITIALIZED;
  return mDOMDocument->QueryInterface(NS_GET_IID(nsIDOMDocument),
                                      (void **)aDoc);
}


// Returns doc shell associated with XMLTerm
NS_IMETHODIMP mozXMLTerminal::GetDocShell(nsIDocShell** aDocShell)
{
  if (!aDocShell)
    return NS_ERROR_NULL_POINTER;

  *aDocShell = nsnull;

  NS_PRECONDITION(mDocShell, "bad state, null mDocShell");
  if (!mDocShell)
    return NS_ERROR_NOT_INITIALIZED;
  return mDocShell->QueryInterface(NS_GET_IID(nsIDocShell),
                                    (void **)aDocShell);
}


// Returns presentation shell associated with XMLTerm
NS_IMETHODIMP mozXMLTerminal::GetPresShell(nsIPresShell** aPresShell)
{
  if (!aPresShell)
    return NS_ERROR_NULL_POINTER;

  *aPresShell = nsnull;

  NS_PRECONDITION(mPresShell, "bad state, null mPresShell");
  if (!mPresShell)
    return NS_ERROR_NOT_INITIALIZED;
  return mPresShell->QueryInterface(NS_GET_IID(nsIPresShell),
                                    (void **)aPresShell);
}


/** Gets flag denoting whether terminal is in full screen mode
 * @param aFlag (output) screen mode flag
 */
NS_IMETHODIMP mozXMLTerminal::GetScreenMode(PRBool* aFlag)
{
  return mXMLTermSession->GetScreenMode(aFlag);
}


/** Checks if supplied cookie is valid for XMLTerm
 * @param aCookie supplied cookie string
 * @param _retval true if supplied cookie matches XMLTerm cookie
 */
NS_IMETHODIMP mozXMLTerminal::MatchesCookie(const PRUnichar* aCookie,
                                            PRBool *_retval)
{
  XMLT_LOG(mozXMLTerminal::MatchesCookie,20,("\n"));

  if (!_retval)
    return NS_ERROR_NULL_POINTER;

  // Check if supplied cookie matches XMLTerm cookie
  *_retval = mCookie.Equals(aCookie);

  if (!(*_retval)) {
    XMLT_ERROR("mozXMLTerminal::MatchesCookie: Error - Cookie mismatch\n");
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}


/** Resizes XMLterm to match a resized window.
 */
NS_IMETHODIMP mozXMLTerminal::Resize(void)
{
  nsresult result;

  XMLT_LOG(mozXMLTerminal::Resize,20,("\n"));

  if (!mXMLTermSession)
    return NS_ERROR_FAILURE;

  PRBool screenMode;
  GetScreenMode(&screenMode);

  if (screenMode) {
    // Delay resizing until next input processing
    mNeedsResizing = true;
  } else {
    // Resize session
    result = mXMLTermSession->Resize(mLineTermAux);
    if (NS_FAILED(result))
      return result;
  }

  return NS_OK;
}


// nsIDocumentLoaderObserver methods
NS_IMETHODIMP
mozXMLTerminal::OnStartDocumentLoad(nsIDocumentLoader* loader, nsIURI* aURL,
                                const char* aCommand)
{
   return NS_OK;
}

NS_IMETHODIMP
mozXMLTerminal::OnEndDocumentLoad(nsIDocumentLoader* loader, nsIChannel* channel,
                              nsresult aStatus)
{

   return NS_OK;
}

NS_IMETHODIMP
mozXMLTerminal::OnStartURLLoad(nsIDocumentLoader* loader,
                                 nsIChannel* channel)
{

   return NS_OK;
}

NS_IMETHODIMP
mozXMLTerminal::OnProgressURLLoad(nsIDocumentLoader* loader,
                                    nsIChannel* channel, PRUint32 aProgress, 
                                    PRUint32 aProgressMax)
{
  return NS_OK;
}

NS_IMETHODIMP
mozXMLTerminal::OnStatusURLLoad(nsIDocumentLoader* loader,
                                  nsIChannel* channel, nsString& aMsg)
{
  return NS_OK;
}

NS_IMETHODIMP
mozXMLTerminal::OnEndURLLoad(nsIDocumentLoader* loader,
                               nsIChannel* channel, nsresult aStatus)
{
  XMLT_LOG(mozXMLTerminal::OnEndURLLoad,20,("\n"));

  // Activate XMLTerm
  Activate();
  return NS_OK;
}
