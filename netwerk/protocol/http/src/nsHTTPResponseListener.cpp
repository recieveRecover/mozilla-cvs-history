/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#include "nspr.h"
#include "nsAuthEngine.h"
#include "nsHTTPRequest.h"
#include "nsIStreamListener.h"
#include "nsHTTPResponseListener.h"
#include "nsIChannel.h"
#include "nsIBufferInputStream.h"
#include "nsHTTPChannel.h"
#include "nsHTTPResponse.h"
#include "nsIHttpEventSink.h"
#include "nsCRT.h"

#include "nsHTTPAtoms.h"
#include "nsIHttpNotify.h"
#include "nsINetModRegEntry.h"
#include "nsProxyObjectManager.h"
#include "nsIServiceManager.h"
#include "nsINetModuleMgr.h"
#include "nsIEventQueueService.h"

#include "nsXPIDLString.h" 

#include "nsIIOService.h"
static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);

#if defined(PR_LOGGING)
extern PRLogModuleInfo* gHTTPLog;
#endif /* PR_LOGGING */

//
// This specifies the maximum allowable size for a server Status-Line
// or Response-Header.
//
static const int kMAX_HEADER_SIZE = 60000;


nsHTTPResponseListener::nsHTTPResponseListener(nsHTTPChannel* aConnection):
    mFirstLineParsed(PR_FALSE),
    mHeadersDone(PR_FALSE),
    mFiredOnHeadersAvailable(PR_FALSE),
    mFiredOpenOnStartRequest(PR_FALSE),
    mAsyncReadAfterAsyncOpen(PR_FALSE),
    mReadLength(0),
    mResponse(nsnull),
    mResponseContext(nsnull),
    mBytesReceived(0)
{
    NS_INIT_REFCNT();

    NS_ASSERTION(aConnection, "HTTPChannel is null.");
    mConnection = aConnection;
    NS_IF_ADDREF(mConnection);

    PR_LOG(gHTTPLog, PR_LOG_ALWAYS, 
           ("Creating nsHTTPResponseListener [this=%x].\n", this));

}

nsHTTPResponseListener::~nsHTTPResponseListener()
{
    PR_LOG(gHTTPLog, PR_LOG_ALWAYS, 
           ("Deleting nsHTTPResponseListener [this=%x].\n", this));

    NS_IF_RELEASE(mConnection);
    NS_IF_RELEASE(mResponse);
}

NS_IMPL_ISUPPORTS2(nsHTTPResponseListener, nsIStreamListener, nsIStreamObserver);

static NS_DEFINE_IID(kProxyObjectManagerIID, NS_IPROXYEVENT_MANAGER_IID);
static NS_DEFINE_CID(kEventQueueService, NS_EVENTQUEUESERVICE_CID);
static NS_DEFINE_CID(kNetModuleMgrCID, NS_NETMODULEMGR_CID);

////////////////////////////////////////////////////////////////////////////////
// nsIStreamListener methods:

NS_IMETHODIMP
nsHTTPResponseListener::OnDataAvailable(nsIChannel* channel,
                                        nsISupports* context,
                                        nsIInputStream *i_pStream, 
                                        PRUint32 i_SourceOffset,
                                        PRUint32 i_Length)
{
    nsresult rv = NS_OK;
    PRUint32 actualBytesRead;
    NS_ASSERTION(i_pStream, "No stream supplied by the transport!");
    nsCOMPtr<nsIBufferInputStream> bufferInStream = do_QueryInterface(i_pStream);

    PR_LOG(gHTTPLog, PR_LOG_ALWAYS, 
           ("nsHTTPResponseListener::OnDataAvailable [this=%x].\n"
            "\tstream=%x. \toffset=%d. \tlength=%d.\n",
            this, i_pStream, i_SourceOffset, i_Length));

    if (!mResponse)
    {
        mResponse = new nsHTTPResponse ();
        if (!mResponse) {
            NS_ERROR("Failed to create the response object!");
            return NS_ERROR_OUT_OF_MEMORY;
        }
        NS_ADDREF(mResponse);
        mConnection->SetResponse(mResponse);
    }
    //
    // Parse the status line and the response headers from the server
    //
    if (!mHeadersDone) {
        //
        // Parse the status line from the server.  This is always the 
        // first line of the response...
        //
        if (!mFirstLineParsed) {
            rv = ParseStatusLine(bufferInStream, i_Length, &actualBytesRead);
            i_Length -= actualBytesRead;
        }

        PR_LOG(gHTTPLog, PR_LOG_ALWAYS, 
               ("\tOnDataAvailable [this=%x]. Parsing Headers\n", this));
        //
        // Parse the response headers as long as there is more data and
        // the headers are not done...
        //
        while (NS_SUCCEEDED(rv) && i_Length && !mHeadersDone) {
            rv = ParseHTTPHeader(bufferInStream, i_Length, &actualBytesRead);
            NS_ASSERTION(i_Length - actualBytesRead <= i_Length, "wrap around");
            i_Length -= actualBytesRead;
        }

        if (NS_FAILED(rv)) return rv;
        
        // Don't do anything else until all headers have been parsed
        if (!mHeadersDone)
            return NS_OK;

        //
        // All the headers have been read.
        //
        rv = FinishedResponseHeaders();
        if (NS_FAILED(rv)) return rv;
    }

    // At this point we've digested headers from the server and we're
    // onto the actual data. If this transaction was initiated without
    // an AsyncRead, we just want to pass the OnData() notifications
    // straight through to the consumer.
    //
    // .... otherwise...
    //
    // if we were AsyncOpened, we want to increment our internal byte count
    // so when we finally push the stream to the consumer via AsyncRead,
    // we're sure to pass him all the data that has queued up.

    if (mConnection->mOpenObserver && !mAsyncReadAfterAsyncOpen) {
        mBytesReceived += i_Length;
        mDataStream = i_pStream;
    } else {

        //
        // Abort the connection if the consumer has been released.  This will 
        // happen if a redirect has been processed...
        //
        if (!mConsumer) {
            // XXX: What should the return code be?
            rv = NS_BINDING_ABORTED;
        }

        if (NS_SUCCEEDED(rv)) {
            if (i_Length) {
                PR_LOG(gHTTPLog, PR_LOG_ALWAYS, 
                       ("\tOnDataAvailable [this=%x]. Calling consumer "
                        "OnDataAvailable.\tlength:%d\n", this, i_Length));

                rv = mConsumer->OnDataAvailable(mConnection, mResponseContext, i_pStream, 0, 
                                                  i_Length);
                if (NS_FAILED(rv)) {
                  PR_LOG(gHTTPLog, PR_LOG_ERROR, 
                         ("\tOnDataAvailable [this=%x]. Consumer failed!"
                          "Status: %x\n", this, rv));
                }
            }
        } 
    } // end !mConnection->mOpenObserver

    return rv;
}


NS_IMETHODIMP
nsHTTPResponseListener::OnStartRequest(nsIChannel* channel, nsISupports* i_pContext)
{
    nsresult rv;

    PR_LOG(gHTTPLog, PR_LOG_ALWAYS, 
           ("nsHTTPResponseListener::OnStartRequest [this=%x].\n", this));

    // Initialize header varaibles...  
    mHeadersDone     = PR_FALSE;
    mFirstLineParsed = PR_FALSE;

    // Cache the nsIStreamListener and ISupports context of the consumer...
    rv = mConnection->GetResponseDataListener(getter_AddRefs(mConsumer));
    if (NS_SUCCEEDED(rv))
        mConnection->GetResponseContext(getter_AddRefs(mResponseContext));

    return rv;
}

NS_IMETHODIMP
nsHTTPResponseListener::OnStopRequest(nsIChannel* channel,
                                      nsISupports* i_pContext,
                                      nsresult i_Status,
                                      const PRUnichar* i_pMsg)
{
    nsresult rv = NS_OK;

    PR_LOG(gHTTPLog, PR_LOG_ALWAYS, 
           ("nsHTTPResponseListener::OnStopRequest [this=%x]."
            "\tStatus = %x\n", this, i_Status));

    if (NS_SUCCEEDED(rv) && !mHeadersDone) {
        //
        // Oh great!!  The server has closed the connection without sending 
        // an entity.  Assume that it has sent all the response headers and
        // process them - in case the status indicates that some action should
        // be taken (ie. redirect).
        //
        // Ignore the return code, since the request is being completed...
        //
        mHeadersDone = PR_TRUE;
        if (mResponse) {
            (void)FinishedResponseHeaders();
        }
    }

    // Notify the HTTPChannel that the response has completed...
    NS_ASSERTION(mConnection, "HTTPChannel is null.");
    if (mConnection) {
        mConnection->ResponseCompleted(channel, i_Status, i_pMsg);
    }

    if (mConnection->mOpenObserver) {
        // we're done processing the data
        rv = mConnection->mOpenObserver->OnStopRequest(mConnection,
                                                   mConnection->mOpenContext,
                                                   i_Status, i_pMsg);
        if (NS_FAILED(rv)) return rv;
    }

    // The HTTPChannel is no longer needed...
    NS_IF_RELEASE(mConnection);

    // The Response Context is no longer needed...
    mResponseContext = nsnull;

    return rv;
}

////////////////////////////////////////////////////////////////////////////////
// nsHTTPResponseListener methods:

nsresult nsHTTPResponseListener::FireSingleOnData(nsIStreamListener *aListener, nsISupports *aContext)
{
    nsresult rv;

    mConsumer = aListener;
    mResponseContext = aContext;

    if (mHeadersDone) {
        rv = FinishedResponseHeaders();
        if (NS_FAILED(rv)) return rv;
        
        if (mBytesReceived) {
            rv = mConsumer->OnDataAvailable(mConnection, mResponseContext,
                                            mDataStream, 0, mBytesReceived);
        }
        mDataStream = 0;
    }
    
    mAsyncReadAfterAsyncOpen = PR_TRUE;
    return rv;
}

nsresult nsHTTPResponseListener::FireOnHeadersAvailable()
{
    nsresult rv;
    NS_ASSERTION(mHeadersDone, "Headers have not been received!");

    if (mHeadersDone) {
        rv = mConnection->OnHeadersAvailable();
    } else {
        rv = NS_ERROR_FAILURE;
    }

    return rv;
}

NS_METHOD
nsWriteToString(void* closure,
                const char* fromRawSegment,
                PRUint32 offset,
                PRUint32 count,
                PRUint32 *writeCount)
{
  nsString *str = (nsString*)closure;

  str->Append(fromRawSegment, count);
  *writeCount = count;
  
  return NS_OK;
}


nsresult nsHTTPResponseListener::ParseStatusLine(nsIBufferInputStream* in, 
                                                 PRUint32 aLength,
                                                 PRUint32 *aBytesRead)
{
  nsresult rv = NS_OK;

  PRBool bFoundString = PR_FALSE;
  PRUint32 offsetOfEnd, totalBytesToRead, actualBytesRead;

  PR_LOG(gHTTPLog, PR_LOG_ALWAYS, 
         ("nsHTTPResponseListener::ParseStatusLine [this=%x].\taLength=%d\n", 
          this, aLength));

  *aBytesRead = 0;

  if (kMAX_HEADER_SIZE < mHeaderBuffer.Length()) {
    // This server is yanking our chain...
    return NS_ERROR_FAILURE;
  }

  // Look for the LF which ends the Status-Line.
  rv = in->Search("\n", PR_FALSE, &bFoundString, &offsetOfEnd);
  if (NS_FAILED(rv)) return rv;

  if (!bFoundString) {
    //
    // This is a partial header...  Read the entire buffer and wait for
    // more data...
    //
    totalBytesToRead = aLength;
  } else {
    // Do not forget to include the LF character in the read...
    totalBytesToRead = offsetOfEnd+1;
  }

  rv = in->ReadSegments(nsWriteToString, 
                        (void*)&mHeaderBuffer, 
                        totalBytesToRead, 
                        &actualBytesRead);
  if (NS_FAILED(rv)) return rv;

  *aBytesRead += actualBytesRead;

  // Wait for more data to arrive before processing the header...
  if (!bFoundString) return NS_OK;

  PR_LOG(gHTTPLog, PR_LOG_ALWAYS, 
         ("\tParseStatusLine [this=%x].\tGot Status-Line:%s\n"
         , this, mHeaderBuffer.GetBuffer()));

  //
  // Replace all LWS with single SP characters.  Also remove the CRLF
  // characters...
  //
  mHeaderBuffer.CompressSet(" \t", ' ');
  mHeaderBuffer.StripChars("\r\n");

  rv = mResponse->ParseStatusLine(mHeaderBuffer);
  if (NS_FAILED(rv)) return rv;

  mFirstLineParsed = PR_TRUE;

  return NS_OK;
}



nsresult nsHTTPResponseListener::ParseHTTPHeader(nsIBufferInputStream* in,
                                                 PRUint32 aLength,
                                                 PRUint32 *aBytesRead)
{
  nsresult rv = NS_OK;

  PRBool bFoundString;
  PRUint32 offsetOfEnd, totalBytesToRead, actualBytesRead;

  *aBytesRead = 0;

  if (kMAX_HEADER_SIZE < mHeaderBuffer.Length()) {
    // This server is yanking our chain...
    return NS_ERROR_FAILURE;
  }

  //
  // Read the header from the input buffer...  A header is terminated by 
  // a CRLF.  Header values may be extended over multiple lines by preceeding
  // each extra line with linear white space...
  //
  do {
    //
    // If last character in the header string is a LF, then the header 
    // may be complete...
    //
    if (mHeaderBuffer.Last() == '\n' ) {
      // This line is either LF or CRLF so the header is complete...
      if (mHeaderBuffer.Length() <= 2) {
          break;
      }

      rv = in->Search(" ", PR_FALSE, &bFoundString, &offsetOfEnd);
      if (NS_FAILED(rv)) return rv;
      if (!bFoundString && offsetOfEnd == 0) 
          return NS_OK;     // Need to wait for more data to see if the header is complete

      if (!bFoundString || offsetOfEnd != 0) {
          // then check for tab too
          rv = in->Search("\t", PR_FALSE, &bFoundString, &offsetOfEnd);
          if (NS_FAILED(rv)) return rv;
          NS_ASSERTION(!(!bFoundString && offsetOfEnd == 0), "should have been checked above");
          if (!bFoundString || offsetOfEnd != 0) {
              break; // neither space nor tab, so jump out of the loop
          }
      }
      // else, go around the loop again and accumulate the rest of the header...
    }

    // Look for the next LF in the buffer...
    rv = in->Search("\n", PR_FALSE, &bFoundString, &offsetOfEnd);
    if (NS_FAILED(rv)) return rv;

    if (!bFoundString) {
      //
      // The buffer contains a partial header.  Read the entire buffer 
      // and wait for more data...
      //
      totalBytesToRead = aLength;
    } else {
    // Do not forget to include the LF character in the read...
      totalBytesToRead = offsetOfEnd+1;
    }

    // Append the buffer into the header string...
    rv = in->ReadSegments(nsWriteToString, 
                          (void*)&mHeaderBuffer, 
                          totalBytesToRead, 
                          &actualBytesRead);
    if (NS_FAILED(rv)) return rv;

    *aBytesRead += actualBytesRead;

    // Partial header - wait for more data to arrive...
    if (!bFoundString) return NS_OK;

  } while (PR_TRUE);

  PR_LOG(gHTTPLog, PR_LOG_ALWAYS, 
         ("\tParseHTTPHeader [this=%x].\tGot header string:%s\n",
          this, mHeaderBuffer.GetBuffer()));

  //
  // Replace all LWS with single SP characters.  And remove all of the CRLF
  // characters...
  //
  mHeaderBuffer.CompressSet(" \t", ' ');
  mHeaderBuffer.StripChars("\r\n");

  if (mHeaderBuffer.Length() == 0) {
    mHeadersDone = PR_TRUE;
    return NS_OK;
  }

  return mResponse->ParseHeader(mHeaderBuffer);
}

nsresult nsHTTPResponseListener::FinishedResponseHeaders(void)
{
  nsresult rv;

  if (mFiredOnHeadersAvailable)
      return NS_OK;

  rv = NS_OK;

  PR_LOG(gHTTPLog, PR_LOG_ALWAYS, 
         ("nsHTTPResponseListener::FinishedResponseHeaders [this=%x].\n",
          this));

  if (mConnection->mOpenObserver && !mFiredOpenOnStartRequest) {
      mConnection->mRawResponseListener = this;
      rv = mConnection->mOpenObserver->OnStartRequest(mConnection, 
                                                      mConnection->mOpenContext);
      mFiredOpenOnStartRequest = PR_TRUE;

      // We want to defer header completion notification until the 
      // caller actually does an AsyncRead();
      if (!mAsyncReadAfterAsyncOpen)
          return rv;
  }

  // Notify the consumer that headers are available...
  FireOnHeadersAvailable();
  mFiredOnHeadersAvailable = PR_TRUE;

  //
  // Check the status code to see if any special processing is necessary.
  //
  // If a redirect (ie. 30x) occurs, the mConsumer is released and a new
  // request is issued...
  //
  rv = ProcessStatusCode();

  //
  // Fire the OnStartRequest notification - now that user data is available
  //
  if (NS_SUCCEEDED(rv) && mConsumer) {
    rv = mConsumer->OnStartRequest(mConnection, mResponseContext);
    if (NS_FAILED(rv)) {
      PR_LOG(gHTTPLog, PR_LOG_ERROR, 
             ("\tOnStartRequest [this=%x]. Consumer failed!"
              "Status: %x\n", this, rv));
    }
  } 

  return rv;
}

nsresult nsHTTPResponseListener::ProcessStatusCode(void)
{
  nsresult rv = NS_OK;
  PRUint32 statusCode, statusClass;

  statusCode = 0;
  rv = mResponse->GetStatus(&statusCode);

  PRBool authAttempt = PR_FALSE;
  mConnection->GetAuthTriedWithPrehost(&authAttempt);
  
  if ( statusCode != 401 && authAttempt) {
    // we know this auth challenge response was successful. cache any authentication 
    // now so URLs within this body can use it.
    nsAuthEngine* pEngine;
    NS_ASSERTION(mConnection->mHandler, "HTTP handler went away");
    if (NS_SUCCEEDED(mConnection->mHandler->GetAuthEngine(&pEngine)) )
      {
        nsXPIDLCString authString;
        NS_ASSERTION(mConnection->mRequest, "HTTP request went away");
        rv = mConnection->mRequest->GetHeader(nsHTTPAtoms::Authorization, 
                                              getter_Copies(authString));
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsIURI> luri;
        rv = mConnection->GetURI(getter_AddRefs(luri));
        if (NS_FAILED(rv)) return rv;
              
        pEngine->SetAuthString(luri, authString);
      }
  }
  
  statusClass = statusCode / 100;

  switch (statusClass) {
    //
    // Informational: 1xx
    //
    case 1:
      PR_LOG(gHTTPLog, PR_LOG_ALWAYS, 
             ("ProcessStatusCode [this=%x].\tStatus - Informational: %d.\n",
              this, statusCode));
      break;

    //
    // Successful: 2xx
    //
    case 2:
      PR_LOG(gHTTPLog, PR_LOG_ALWAYS, 
             ("ProcessStatusCode [this=%x].\tStatus - Successful: %d.\n",
              this, statusCode));
      // If channel's AuthTriedWithPrehost then enter this user/pass and 
      // authstring into the auth list. 
      break;

    //
    // Redirection: 3xx
    //
    case 3:
      PR_LOG(gHTTPLog, PR_LOG_ALWAYS, 
             ("ProcessStatusCode [this=%x].\tStatus - Redirection: %d.\n",
              this, statusCode));
      rv = ProcessRedirection(statusCode);
      break;

    //
    // Client Error: 4xx
    //
    case 4:
      PR_LOG(gHTTPLog, PR_LOG_ALWAYS, 
            ("ProcessStatusCode [this=%x].\tStatus - Client Error: %d.\n",
            this, statusCode));
      if (401 == statusCode) {
        rv = ProcessAuthentication(statusCode);
      }
      break;

    //
    // Server Error: 5xo
    //
    case 5:
      PR_LOG(gHTTPLog, PR_LOG_ALWAYS, 
             ("ProcessStatusCode [this=%x].\tStatus - Server Error: %d.\n",
              this, statusCode));
      break;

    //
    // Unknown Status Code catagory...
    //
    default:
      PR_LOG(gHTTPLog, PR_LOG_ALWAYS, 
             ("ProcessStatusCode [this=%x].\tStatus - Unknown Status Code catagory: %d.\n",
              this, statusCode));
      break;
  }

  return rv;
}



nsresult
nsHTTPResponseListener::ProcessRedirection(PRInt32 aStatusCode)
{
  nsresult rv = NS_OK;
  nsXPIDLCString location;

  mResponse->GetHeader(nsHTTPAtoms::Location, getter_Copies(location));

  if (((301 == aStatusCode) || (302 == aStatusCode)) && (location))
  {
      nsCOMPtr<nsIChannel> channel;

      rv = mConnection->Redirect(location, getter_AddRefs(channel));
      if (NS_SUCCEEDED(rv)) {
        /*
           Disconnect the consumer from this response listener...  
           This allows the entity that follows to be discarded 
           without notifying the consumer...
        */
        mConsumer = 0;
        mResponseContext = 0;
      }
  }
  return rv;
}

nsresult
nsHTTPResponseListener::ProcessAuthentication(PRInt32 aStatusCode)
{
    NS_ASSERTION(aStatusCode == 401, // thats all we handle for now... 
            "We don't handle other types of errors!"); 

    if (aStatusCode != 401)
        return NS_OK; // Let life go on...

    nsresult rv = NS_OK;
    nsXPIDLCString challenge; // identifies the auth type and realm.

    if (NS_FAILED(rv = mResponse->GetHeader(
                nsHTTPAtoms::WWW_Authenticate, 
                getter_Copies(challenge))))
     // We can't send user-password without this challenge.
        return rv;
    // TODO Add proxy-authenticate header check too...
     // can we do * on an XPIDLCString? check... todo
    if (!challenge || !*challenge)
        return rv;

    nsCOMPtr<nsIChannel> channel;
    if (NS_FAILED(rv = mConnection->Authenticate(challenge, 
                    getter_AddRefs(channel))))
        return rv;


    /*
       Disconnect the consumer from this response listener...  
       This allows the entity that follows to be discarded 
       without notifying the consumer...
    */
    mConsumer = 0;
    mResponseContext = 0;
    return rv;
}
