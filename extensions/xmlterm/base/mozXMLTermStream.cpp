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
 */

// mozXMLTermStream.cpp: implementation of mozIXMLTermStream
// to display HTML/XML streams as documents

#include "nscore.h"
#include "prlog.h"

#include "nsCOMPtr.h"
#include "nsString.h"

#include "nsIAllocator.h"

#include "nsIServiceManager.h"
#include "nsIIOService.h"
#include "nsIDocumentLoader.h"
#include "nsIContentViewer.h"
#include "nsIDocumentViewer.h"
#include "nsIPresContext.h"
#include "nsIPresShell.h"

#include "nsIViewManager.h"
#include "nsIScrollableView.h"
#include "nsIDeviceContext.h"
#include "nsIFrame.h"

#include "nsIScriptContextOwner.h"
#include "nsIScriptGlobalObject.h"

#include "nsIDOMWindow.h"
#include "nsIDOMWindowCollection.h"
#include "nsIDOMElement.h"
#include "nsIDOMNode.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMDocument.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDocument.h"

#include "nsIURL.h"
#include "nsNetUtil.h"

#include "mozXMLT.h"
#include "mozXMLTermUtils.h"
#include "mozXMLTermStream.h"

/////////////////////////////////////////////////////////////////////////
// mozXMLTermStream factory
/////////////////////////////////////////////////////////////////////////

nsresult
NS_NewXMLTermStream(mozIXMLTermStream** aXMLTermStream)
{
    NS_PRECONDITION(aXMLTermStream != nsnull, "null ptr");
    if (!aXMLTermStream)
        return NS_ERROR_NULL_POINTER;

    *aXMLTermStream = new mozXMLTermStream();
    if (! *aXMLTermStream)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(*aXMLTermStream);
    return NS_OK;
}


/////////////////////////////////////////////////////////////////////////
// mozXMLTermStream implementation
/////////////////////////////////////////////////////////////////////////

static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);

mozXMLTermStream::mozXMLTermStream() :
  mUTF8Buffer(""),
  mUTF8Offset(0),
  mMaxResizeHeight(0),
  mDOMWindow( nsnull ),
#ifdef NO_WORKAROUND
  mDOMIFrameElement( nsnull ),
  mContext( nsnull ),
  mLoadGroup( nsnull ),
  mChannel( nsnull ),
  mStreamListener( nsnull )
#else // !NO_WORKAROUND
  mDOMHTMLDocument( nsnull )
#endif // !NO_WORKAROUND
{
  NS_INIT_REFCNT();
}


mozXMLTermStream::~mozXMLTermStream()
{
}


// Implement AddRef and Release
NS_IMPL_ADDREF(mozXMLTermStream)
NS_IMPL_RELEASE(mozXMLTermStream)

NS_IMETHODIMP 
mozXMLTermStream::QueryInterface(REFNSIID aIID,void** aInstancePtr)
{
  if (aInstancePtr == NULL) {
    return NS_ERROR_NULL_POINTER;
  }

  // Always NULL result, in case of failure
  *aInstancePtr = NULL;

  if ( aIID.Equals(NS_GET_IID(nsISupports))) {
    *aInstancePtr = NS_STATIC_CAST(nsISupports*,
                                   NS_STATIC_CAST(mozIXMLTermStream*,this));

  } else if(aIID.Equals(NS_GET_IID(nsIBaseStream))) {
    *aInstancePtr = NS_STATIC_CAST(nsIBaseStream*,this);

  } else if(aIID.Equals(NS_GET_IID(nsIInputStream))) {
    *aInstancePtr = NS_STATIC_CAST(nsIInputStream*,this);

  } else if(aIID.Equals(NS_GET_IID(mozIXMLTermStream))) {
    *aInstancePtr = NS_STATIC_CAST(mozIXMLTermStream*,this);

  } else {
    return NS_ERROR_NO_INTERFACE;
  }

  NS_ADDREF_THIS();

  XMLT_LOG(mozXMLTermStream::QueryInterface,20,("mRefCnt = %d\n", mRefCnt));

  return NS_OK;
}


// mozIXMLTermStream interface

/** Open stream in specified frame, or in current frame if frameName is null
 * @param aDOMWindow parent window
 * @param frameName name of child frame in which to display stream, or null
 *                  to display in parent window
 * @param contentURL URL of stream content
 * @param contentType MIME type of stream content
 * @param maxResizeHeight maximum resize height (0=> do not resize)
 * @return NS_OK on success
 */
NS_IMETHODIMP mozXMLTermStream::Open(nsIDOMWindow* aDOMWindow,
                                     const char* frameName,
                                     const char* contentURL,
                                     const char* contentType,
                                     PRInt32 maxResizeHeight)
{
  nsresult result;

  XMLT_LOG(mozXMLTermStream::Open,20,("contentURL=%s, contentType=%s\n",
                                      contentURL, contentType));

  mMaxResizeHeight = maxResizeHeight;

  if (frameName && (strlen(frameName) > 0)) {
    // Open stream in named subframe of current frame
    XMLT_LOG(mozXMLTermStream::Open,22,("frameName=%s\n", frameName));

    nsAutoString innerFrameName = frameName;

    // Get DOM IFRAME element
    nsCOMPtr<nsIDOMDocument> domDoc;
    result = aDOMWindow->GetDocument(getter_AddRefs(domDoc));
    if (NS_FAILED(result) || !domDoc)
      return NS_ERROR_FAILURE;

    nsCOMPtr<nsIDOMHTMLDocument> domHTMLDoc = do_QueryInterface(domDoc);
    if (!domHTMLDoc)
      return NS_ERROR_FAILURE;

    nsCOMPtr<nsIDOMNodeList> nodeList;
    result = domHTMLDoc->GetElementsByName(innerFrameName,
                                           getter_AddRefs(nodeList));
    if (NS_FAILED(result) || !nodeList)
      return NS_ERROR_FAILURE;

    PRUint32 count;
    nodeList->GetLength(&count);
    PR_ASSERT(count==1);

    nsCOMPtr<nsIDOMNode> domNode;
    result = nodeList->Item(0, getter_AddRefs(domNode));
    if (NS_FAILED(result) || !domNode)
      return NS_ERROR_FAILURE;

    mDOMIFrameElement = do_QueryInterface(domNode);
    if (!mDOMIFrameElement)
      return NS_ERROR_FAILURE;

    // Ensure that it is indeed an IFRAME element
    nsAutoString tagName;
    result = mDOMIFrameElement->GetTagName(tagName);
    if (NS_FAILED(result))
      return NS_ERROR_FAILURE;

    if (!tagName.EqualsIgnoreCase("iframe"))
      return NS_ERROR_FAILURE;

    if (mMaxResizeHeight > 0) {
      // Set initial IFRAME size to be as wide as the window, but very short
      nsAutoString attWidth = "width";
      nsAutoString valWidth = "100%";
      mDOMIFrameElement->SetAttribute(attWidth,valWidth);

      nsAutoString attHeight = "height";
      nsAutoString valHeight = "10";
      mDOMIFrameElement->SetAttribute(attHeight,valHeight);
    }

    // Get inner DOM window by looking up the frames list
    nsCOMPtr<nsIDOMWindow> innerDOMWindow;
    result = mozXMLTermUtils::GetInnerDOMWindow(aDOMWindow, innerFrameName,
                                getter_AddRefs(innerDOMWindow));
    if (NS_FAILED(result) || !innerDOMWindow)
      return NS_ERROR_FAILURE;

    mDOMWindow = innerDOMWindow;

  } else {
    // Open stream in current frame
    mDOMIFrameElement = nsnull;
    mDOMWindow = aDOMWindow;
  }

  // Get webshell for DOM window
  nsCOMPtr<nsIWebShell> webShell;
  result = mozXMLTermUtils::ConvertDOMWindowToWebShell(mDOMWindow,
                                                    getter_AddRefs(webShell));
  if (NS_FAILED(result) || !webShell)
    return NS_ERROR_FAILURE;

#ifdef NO_WORKAROUND
  printf("mozXMLTermStream::Open, NO_WORKAROUND\n");

  nsCOMPtr<nsIInputStream> inputStream = this;

  nsCOMPtr<nsIURI> uri;
  result = NS_NewURI(getter_AddRefs(uri), contentURL, nsnull);
  if (NS_FAILED(result))
    return result;

  result = NS_NewLoadGroup(nsnull, getter_AddRefs(mLoadGroup));
  if (NS_FAILED(result))
    return result;

  PRInt32 contentLength = 1024; // ??? What's this length
  result = NS_NewInputStreamChannel(uri, contentType, contentLength,
                                    inputStream, mLoadGroup,
                                    nsnull,  // notificationCallbacks
                                    nsIChannel::LOAD_NORMAL, 
                                    nsnull, 0, 0,
                                    getter_AddRefs(mChannel));
  if (NS_FAILED(result))
    return result;

  // Determine Class ID for viewing specified mimetype
  nsCID classID;
  static const char command[] = "view";
  nsCAutoString progID = NS_DOCUMENT_LOADER_FACTORY_PROGID_PREFIX;
  progID += command;
  progID += "/";
  progID += contentType;

  result = nsComponentManager::ProgIDToCLSID(progID.GetBuffer(),
                                             &classID);
  if (NS_FAILED(result))
    return result;

  nsCOMPtr<nsIDocumentLoaderFactory> docLoaderFactory;
  result = nsComponentManager::CreateInstance(classID, nsnull,
                                         NS_GET_IID(nsIDocumentLoaderFactory),
                                         getter_AddRefs(docLoaderFactory));
  if (NS_FAILED(result))
    return result;

  nsCOMPtr<nsIContentViewer> contentViewer;
  result = docLoaderFactory->CreateInstance(command,
                                            mChannel,
                                            mLoadGroup,
                                            contentType,
                                            webShell,
                                            nsnull,
                                            getter_AddRefs(mStreamListener),
                                            getter_AddRefs(contentViewer) );
  if (NS_FAILED(result))
    return result;

  nsCOMPtr<nsIContentViewerContainer> contViewContainer =
                                             do_QueryInterface(webShell);
  result = contentViewer->SetContainer(contViewContainer);
  if (NS_FAILED(result))
    return result;

  result = webShell->Embed(contentViewer, command, (nsISupports*) nsnull);
  if (NS_FAILED(result))
    return result;

  result = mStreamListener->OnStartRequest(mChannel, mContext);
  if (NS_FAILED(result))
    return result;
#else // !NO_WORKAROUND
  printf("mozXMLTermStream::Open, WORKAROUND\n");

  nsCOMPtr<nsIDOMDocument> innerDOMDoc;
  result = mDOMWindow->GetDocument(getter_AddRefs(innerDOMDoc));
  printf("mozXMLTermStream::Open,check1, 0x%x\n", result);
  if (NS_FAILED(result) || !innerDOMDoc)
    return NS_ERROR_FAILURE;

  mDOMHTMLDocument = do_QueryInterface(innerDOMDoc);
  printf("mozXMLTermStream::Open,check2, 0x%x\n", result);
  if (!mDOMHTMLDocument)
    return NS_ERROR_FAILURE;

  result = mDOMHTMLDocument->Open();
  printf("mozXMLTermStream::Open,check3, 0x%x\n", result);
  if (NS_FAILED(result))
    return result;
#endif // !NO_WORKAROUND

  XMLT_LOG(mozXMLTermStream::Open,21,("returning\n"));

  return NS_OK;
}


// nsIBaseStream interface
NS_IMETHODIMP mozXMLTermStream::Close(void)
{
  nsresult result;

  XMLT_LOG(mozXMLTermStream::Close,20,("\n"));

  mUTF8Buffer = "";
  mUTF8Offset = 0;

#ifdef NO_WORKAROUND
  PRUint32 sourceOffset = 0;
  PRUint32 count = 0;
  result = mStreamListener->OnDataAvailable(mChannel, mContext,
                                            this, sourceOffset, count);
  if (NS_FAILED(result))
    return result;

  nsresult status = NS_OK;
  nsAutoString errorMsg = "";
  result = mStreamListener->OnStopRequest(mChannel, mContext,
                                          status, errorMsg.GetUnicode());
  if (NS_FAILED(result))
    return result;

  mContext = nsnull;
  mLoadGroup = nsnull;
  mChannel = nsnull;
  mStreamListener = nsnull;

#else // !NO_WORKAROUND
  result = mDOMHTMLDocument->Close();
  if (NS_FAILED(result))
    return result;
#endif // !NO_WORKAROUND

  if (mMaxResizeHeight && mDOMIFrameElement) {
    // Size frame to content
    result = SizeToContentHeight(mMaxResizeHeight);
  }
  mMaxResizeHeight = 0;

  // Release interfaces etc
  mDOMWindow = nsnull;
  mDOMIFrameElement = nsnull;

  return NS_OK;
}


/** Adjusts height of frame displaying stream to fit content
 * @param maxHeight maximum height of resized frame (pixels)
 *                  (zero value implies no maximum)
 */
NS_IMETHODIMP mozXMLTermStream::SizeToContentHeight(PRInt32 maxHeight)
{
  nsresult result;

  // Get webshell
  nsCOMPtr<nsIWebShell> webShell;
  result = mozXMLTermUtils::ConvertDOMWindowToWebShell(mDOMWindow,
                                                   getter_AddRefs(webShell));
  if (NS_FAILED(result) || !webShell)
    return NS_ERROR_FAILURE;

  // Get pres context
  nsCOMPtr<nsIPresContext> presContext;
  result = mozXMLTermUtils::GetWebShellPresContext(webShell,
                                                getter_AddRefs(presContext));
  if (NS_FAILED(result) || !presContext)
    return NS_ERROR_FAILURE;

  // Get scrollable view
  nsCOMPtr<nsIScrollableView> scrollableView;
  result = mozXMLTermUtils::GetPresContextScrollableView(presContext,
                                              getter_AddRefs(scrollableView));
  if (NS_FAILED(result) || !scrollableView)
    return NS_ERROR_FAILURE;

  // Get device context
  nsCOMPtr<nsIDeviceContext> deviceContext;
  result = mozXMLTermUtils::GetPresContextDeviceContext(presContext,
                                              getter_AddRefs(deviceContext));
  if (NS_FAILED(result) || !deviceContext)
    return NS_ERROR_FAILURE;

  // Determine twips to pixels conversion factor
  float pixelScale;
  presContext->GetTwipsToPixels(&pixelScale);

  // Get scrollbar dimensions in pixels
  float sbWidth, sbHeight;
  deviceContext->GetScrollBarDimensions(sbWidth, sbHeight);
  PRInt32 scrollBarWidth = PRInt32(sbWidth*pixelScale);
  PRInt32 scrollBarHeight = PRInt32(sbHeight*pixelScale);

  // Determine webshell size in pixels
  PRInt32 shellX, shellY, shellWidth, shellHeight;
  result = webShell->GetBounds(shellX, shellY, shellWidth, shellHeight);

  // Determine page size in pixels
  nscoord contX, contY;
  scrollableView->GetContainerSize(&contX, &contY);

  PRInt32 pageWidth, pageHeight;
  pageWidth = PRInt32((float)contX*pixelScale);
  pageHeight = PRInt32((float)contY*pixelScale);

  printf("mozXMLTermStream::SizeToContentHeight: scrollbar %d, %d\n",
         scrollBarWidth, scrollBarHeight);

  printf("mozXMLTermStream::SizeToContentHeight: shell %d, %d\n",
         shellWidth, shellHeight);

  printf("mozXMLTermStream::SizeToContentHeight: page %d, %d, %e\n",
         pageWidth, pageHeight, pixelScale);

  if ((pageHeight > shellHeight) || (pageWidth > shellWidth)) {
    // Page larger than webshell
    nsAutoString attHeight = "height";
    nsAutoString attWidth = "width";
    nsAutoString attValue = "";

    PRInt32 newPageHeight = pageHeight;
    PRInt32 excessWidth = (pageWidth+scrollBarWidth - shellWidth);

    printf("mozXMLTermStream::SizeToContentHeight: excessWidth %d\n",
           excessWidth);

    if (excessWidth > 0) {
      // Widen IFRAME beyond page width by scrollbar width
      attValue = "";
      attValue.Append(shellWidth+scrollBarWidth);
      mDOMIFrameElement->SetAttribute(attWidth,attValue);

      // Recompute page dimensions
      scrollableView->GetContainerSize(&contX, &contY);
      pageWidth = PRInt32((float)contX*pixelScale);
      pageHeight = PRInt32((float)contY*pixelScale);

      newPageHeight = pageHeight;
      if (excessWidth > scrollBarWidth)
        newPageHeight += scrollBarHeight;

      printf("mozXMLTermStream::SizeToContentHeight: page2 %d, %d, %d\n",
             pageWidth, pageHeight, newPageHeight);

      // Reset IFRAME width
      attValue = "";
      attValue.Append(shellWidth);
      mDOMIFrameElement->SetAttribute(attWidth,attValue);
    }

    // Resize IFRAME height to match page height (subject to a maximum)
    if (newPageHeight > maxHeight) newPageHeight = maxHeight;
    attValue = "";
    attValue.Append(newPageHeight);
    mDOMIFrameElement->SetAttribute(attHeight,attValue);
  }

  return NS_OK;
}


// nsIInputStream interface
NS_IMETHODIMP mozXMLTermStream::Available(PRUint32 *_retval)
{
  if (!_retval)
    return NS_ERROR_NULL_POINTER;

  *_retval = mUTF8Buffer.Length() - mUTF8Offset;

  XMLT_LOG(mozXMLTermStream::Available,60,("retval=%d\n", *_retval));

  return NS_OK;
}


NS_IMETHODIMP mozXMLTermStream::Read(char* buf, PRUint32 count,
                                     PRUint32* _retval)
{
  XMLT_LOG(mozXMLTermStream::Read,60,("count=%d\n", count));

  if (!_retval)
    return NS_ERROR_NULL_POINTER;

  PR_ASSERT(mUTF8Buffer.Length() >= mUTF8Offset);

  PRUint32 remCount = mUTF8Buffer.Length() - mUTF8Offset;

  if (remCount == 0) {
    // Empty buffer
    *_retval = 0;
    return NS_OK;
  }

  if (count >= remCount) {
    // Return entire remaining buffer
    *_retval = remCount;

  } else {
    // Return only portion of buffer
    *_retval = count;
  }

  // Copy portion of string
  mUTF8Buffer.ToCString(buf, *_retval, mUTF8Offset);

  mUTF8Offset += *_retval;

  XMLT_LOG(mozXMLTermStream::Read,61,("*retval=%d\n", *_retval));

  return NS_OK;
}


/** Write Unicode string to stream (blocks until write is completed)
 * @param buf string to write
 * @return NS_OK on success
 */
NS_IMETHODIMP mozXMLTermStream::Write(const PRUnichar* buf)
{
  nsresult result;

  XMLT_LOG(mozXMLTermStream::Write,50,("\n"));

  if (!buf)
    return NS_ERROR_FAILURE;

  nsAutoString strBuf ( buf );

  // Convert Unicode string to UTF8 and store in buffer
  char* utf8Str = strBuf.ToNewUTF8String();
  mUTF8Buffer = utf8Str;
  nsAllocator::Free(utf8Str);

  mUTF8Offset = 0;

#ifdef NO_WORKAROUND
  PRUint32 sourceOffset = 0;

  while (mUTF8Offset < mUTF8Buffer.Length()) {
    PRUint32 remCount = mUTF8Buffer.Length() - mUTF8Offset;
    result = mStreamListener->OnDataAvailable(mChannel, mContext,
                                              this, sourceOffset, remCount);
    if (NS_FAILED(result))
      return result;
  }

#else // !NO_WORKAROUND
  result = mDOMHTMLDocument->Write(strBuf);
  if (NS_FAILED(result))
    return result;
#endif // !NO_WORKAROUND

  printf("mozXMLTermStream::Write: str=%s\n", mUTF8Buffer.GetBuffer());

  XMLT_LOG(mozXMLTermStream::Write,51,("returning mUTF8Offset=%d\n",
                                       mUTF8Offset));

  return NS_OK;
}
