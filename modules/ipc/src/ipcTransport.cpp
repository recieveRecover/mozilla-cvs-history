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
 * The Original Code is Mozilla IPC.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Darin Fisher <darin@netscape.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
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

#include "nsIServiceManager.h"
#include "nsIObserverService.h"
#include "nsISocketTransportService.h"
#include "nsISocketTransport.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsIFile.h"
#include "nsIProcess.h"
#include "nsDirectoryServiceDefs.h"
#include "nsDirectoryServiceUtils.h"
#include "nsNetCID.h"
#include "netCore.h"
#include "prerror.h"
#include "plstr.h"

#include "ipcConfig.h"
#include "ipcLog.h"
#include "ipcMessageUtils.h"
#include "ipcTransport.h"
#include "ipcm.h"

static NS_DEFINE_CID(kSocketTransportServiceCID, NS_SOCKETTRANSPORTSERVICE_CID);

//-----------------------------------------------------------------------------
// ipcTransport
//-----------------------------------------------------------------------------

ipcTransport::~ipcTransport()
{
    if (mFD)
        PR_Close(mFD);
}

nsresult
ipcTransport::Init(const nsACString &appName,
                   const nsACString &socketPath,
                   ipcTransportObserver *obs)
{
    LOG(("ipcTransport::Init\n"));

    mAppName = appName;
    mSocketPath = socketPath;
    mObserver = obs;

    // XXX service should be the observer
    nsCOMPtr<nsIObserverService> observ(do_GetService("@mozilla.org/observer-service;1"));
    if (observ) {
        observ->AddObserver(this, "xpcom-shutdown", PR_FALSE);
        observ->AddObserver(this, "profile-change-net-teardown", PR_FALSE);
    }

    return Connect();
}

nsresult
ipcTransport::Shutdown()
{
    LOG(("ipcTransport::Shutdown\n"));

    mHaveConnection = PR_FALSE;

    if (mReadRequest) {
        mReadRequest->Cancel(NS_BINDING_ABORTED);
        mReadRequest = nsnull;
    }
    if (mWriteRequest) {
        mWriteRequest->Cancel(NS_BINDING_ABORTED);
        mWriteRequest = nsnull;
        mWriteSuspended = PR_FALSE;
    }
    mTransport = nsnull;
    return NS_OK;
}

nsresult
ipcTransport::SendMsg(ipcMessage *msg)
{
    NS_ENSURE_ARG_POINTER(msg);

    LOG(("ipcTransport::SendMsg [dataLen=%u]\n", msg->DataLen()));

    if (!mHaveConnection) {
        LOG(("  delaying message until connected\n"));
        mDelayedQ.Append(msg);
        return NS_OK;
    }

    return SendMsg_Internal(msg);
}

nsresult
ipcTransport::SendMsg_Internal(ipcMessage *msg)
{
    LOG(("ipcTransport::SendMsg_Internal [dataLen=%u]\n", msg->DataLen()));

    mSendQ.EnqueueMsg(msg);

    if (!mWriteRequest) {
        if (!mTransport)
            return NS_ERROR_FAILURE;

        nsresult rv = mTransport->AsyncWrite(&mSendQ, nsnull, 0, PRUint32(-1), 0,
                                             getter_AddRefs(mWriteRequest));
        if (NS_FAILED(rv)) return rv;
    }
    if (mWriteSuspended) {
        mWriteRequest->Resume();
        mWriteSuspended = PR_FALSE;
    }
    return NS_OK;
}

nsresult
ipcTransport::Connect()
{
    nsresult rv;

    LOG(("ipcTransport::Connect\n"));

    if (++mConnectionAttemptCount > 20) {
        LOG(("  giving up after 20 unsuccessful connection attempts\n"));
        return NS_ERROR_ABORT;
    }

    rv = CreateTransport();
    if (NS_FAILED(rv)) return rv; 

    rv = mTransport->AsyncRead(&mReceiver, nsnull, 0, PRUint32(-1), 0,
                               getter_AddRefs(mReadRequest));
    return rv;
}

void
ipcTransport::OnMessageAvailable(const ipcMessage *rawMsg)
{
    LOG(("ipcTransport::OnMsgAvailable [dataLen=%u]\n", rawMsg->DataLen()));

    if (!mHaveConnection) {
        if (rawMsg->Target().Equals(IPCM_TARGET)) {
            if (IPCM_GetMsgType(rawMsg) == IPCM_MSG_TYPE_CLIENT_ID) {
                LOG(("  connection established!\n"));
                mHaveConnection = PR_TRUE;

                // remember our client ID
                ipcMessageCast<ipcmMessageClientID> msg(rawMsg);
                if (mObserver)
                    mObserver->OnConnectionEstablished(msg->ClientID());

                // move messages off the delayed message queue
                while (!mDelayedQ.IsEmpty()) {
                    ipcMessage *msg = mDelayedQ.First();
                    mDelayedQ.RemoveFirst();
                    SendMsg_Internal(msg);
                }
                return;
            }
        }
        LOG(("  received unexpected first message!\n"));
    }
    else if (mObserver)
        mObserver->OnMessageAvailable(rawMsg);
}

void
ipcTransport::OnStartRequest(nsIRequest *req)
{
    nsresult status;
    req->GetStatus(&status);

    if (NS_SUCCEEDED(status) && !mHaveConnection && !mSentHello) {
        //
        // send CLIENT_HELLO; expect CLIENT_ID in response.
        //
        SendMsg_Internal(new ipcmMessageClientHello(mAppName.get()));
        mSentHello = PR_TRUE;
    }
}

void
ipcTransport::OnStopRequest(nsIRequest *req, nsresult status)
{
    LOG(("ipcTransport::OnStopRequest [status=%x]\n", status));

    if (mHaveConnection) {
        mHaveConnection = PR_FALSE;
        if (mObserver)
            mObserver->OnConnectionLost();
    }

    if (status == NS_BINDING_ABORTED)
        return;

    if (NS_FAILED(status) && !mTimer) {
        nsresult rv;
        //
        // connection failure
        //
        rv = SpawnDaemon();
        if (NS_FAILED(rv)) {
            LOG(("  failed to spawn daemon [rv=%x]\n", rv));
            return;
        }

        //
        // re-initialize connection after timeout
        //
        mTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
        if (NS_FAILED(rv)) {
            LOG(("  failed to create timer [rv=%x]\n", rv));
            return;
        }

        // use a simple exponential growth algorithm n*2^(n-1)
        PRUint32 ms = 1000 * (1 << (mConnectionAttemptCount - 1));

        LOG(("  waiting %u milliseconds\n", ms));

        rv = mTimer->Init(this, ms, nsITimer::TYPE_ONE_SHOT);
        if (NS_FAILED(rv)) {
            LOG(("  failed to initialize timer [rv=%x]\n", rv));
            return;
        }
    }

    if (req == mReadRequest)
        mReadRequest = nsnull;
    else if (req == mWriteRequest) {
        mWriteRequest = nsnull;
        mWriteSuspended = PR_FALSE;
    }
}

nsresult
ipcTransport::CreateTransport()
{
    nsresult rv;

    nsCOMPtr<nsISocketTransportService> sts(
            do_GetService(kSocketTransportServiceCID, &rv));
    if (NS_FAILED(rv)) return rv;

    rv = sts->CreateTransportOfType(IPC_SOCKET_TYPE,
                                    "127.0.0.1",
                                    IPC_PORT,
                                    nsnull,
                                    1024,
                                    1024*16,
                                    getter_AddRefs(mTransport));
    if (NS_FAILED(rv)) return rv;

#ifndef IPC_USE_INET
    nsCOMPtr<nsISocketTransport> st(do_QueryInterface(mTransport, &rv));
    if (NS_FAILED(rv)) return rv;

    PRNetAddr addr;
    addr.local.family = PR_AF_LOCAL;
    PL_strncpyz(addr.local.path, mSocketPath.get(), sizeof(addr.local.path));

    rv = st->SetAddress(&addr);
#endif
    return rv;
}

nsresult
ipcTransport::SpawnDaemon()
{
    LOG(("ipcTransport::SpawnDaemon\n"));

    nsresult rv;
    nsCOMPtr<nsIFile> file;

    rv = NS_GetSpecialDirectory(NS_XPCOM_CURRENT_PROCESS_DIR, getter_AddRefs(file));
    if (NS_FAILED(rv)) return rv;

    rv = file->AppendNative(NS_LITERAL_CSTRING(IPC_DAEMON_APP_NAME));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIProcess> proc(do_CreateInstance(NS_PROCESS_CONTRACTID,&rv));
    if (NS_FAILED(rv)) return rv;

    rv = proc->Init(file);
    if (NS_FAILED(rv)) return rv;

    PRUint32 pid;
    return proc->Run(PR_FALSE, nsnull, 0, &pid);
}

NS_IMPL_THREADSAFE_ISUPPORTS0(ipcTransport)

NS_IMETHODIMP
ipcTransport::Observe(nsISupports *subject, const char *topic, const PRUnichar *data)
{
    LOG(("ipcTransport::Observe [topic=%s]\n", topic));

    if (strcmp(topic, "timer-callback") == 0) {
        // 
        // try reconnecting to the daemon
        //
        Shutdown();
        Connect();

        mTimer = nsnull;
    }
    else if (strcmp(topic, "xpcom-shutdown") == 0 ||
             strcmp(topic, "profile-change-net-teardown") == 0)
        Shutdown(); 

    return NS_OK;
}

//-----------------------------------------------------------------------------
// ipcSendQueue
//-----------------------------------------------------------------------------

NS_IMETHODIMP_(nsrefcnt)
ipcSendQueue::AddRef()
{
    return mTransport->AddRef();
}

NS_IMETHODIMP_(nsrefcnt)
ipcSendQueue::Release()
{
    return mTransport->Release();
}

NS_IMPL_QUERY_INTERFACE2(ipcSendQueue, nsIStreamProvider, nsIRequestObserver)

NS_IMETHODIMP
ipcSendQueue::OnStartRequest(nsIRequest *request,
                             nsISupports *context)
{
    LOG(("ipcSendQueue::OnStartRequest\n"));

    if (mTransport)
        mTransport->OnStartRequest(request);

    return NS_OK;
}

NS_IMETHODIMP
ipcSendQueue::OnStopRequest(nsIRequest *request,
                            nsISupports *context,
                            nsresult status)
{
    LOG(("ipcSendQueue::OnStopRequest [status=%x]\n", status));

    if (mTransport)
        mTransport->OnStopRequest(request, status);

    return NS_OK;
}

struct ipcWriteState
{
    ipcMessage *msg;
    PRBool      complete;
};

static NS_METHOD ipcWriteMessage(nsIOutputStream *stream,
                                 void            *closure,
                                 char            *segment,
                                 PRUint32         offset,
                                 PRUint32         count,
                                 PRUint32        *countWritten)
{
    ipcWriteState *state = (ipcWriteState *) closure;

    if (state->msg->WriteTo(segment, count,
                            countWritten, &state->complete) != PR_SUCCESS)
        return NS_ERROR_UNEXPECTED;

    return NS_OK;
}

NS_IMETHODIMP
ipcSendQueue::OnDataWritable(nsIRequest *request,
                             nsISupports *context,
                             nsIOutputStream *stream,
                             PRUint32 offset,
                             PRUint32 count)
{
    PRUint32 n;
    nsresult rv;
    ipcWriteState state;
    PRBool wroteSomething = PR_FALSE;

    LOG(("ipcSendQueue::OnDataWritable\n"));

    while (!mQueue.IsEmpty()) {
        state.msg = mQueue.First();
        state.complete = PR_FALSE;

        rv = stream->WriteSegments(ipcWriteMessage, &state, count, &n);
        if (NS_FAILED(rv))
            break;

        if (state.complete) {
            LOG(("  wrote message %u bytes\n", mQueue.First()->MsgLen()));
            mQueue.DeleteFirst();
        }

        wroteSomething = PR_TRUE;
    }

    if (wroteSomething)
        return NS_OK;

    LOG(("  suspending write request\n"));

    mTransport->SetWriteSuspended(PR_TRUE);
    return NS_BASE_STREAM_WOULD_BLOCK;
}

//----------------------------------------------------------------------------
// ipcReceiver
//----------------------------------------------------------------------------

NS_IMETHODIMP_(nsrefcnt)
ipcReceiver::AddRef()
{
    return mTransport->AddRef();
}

NS_IMETHODIMP_(nsrefcnt)
ipcReceiver::Release()
{
    return mTransport->Release();
}

NS_IMPL_QUERY_INTERFACE2(ipcReceiver, nsIStreamListener, nsIRequestObserver)

NS_IMETHODIMP
ipcReceiver::OnStartRequest(nsIRequest *request,
                            nsISupports *context)
{
    LOG(("ipcReceiver::OnStartRequest\n"));

    if (mTransport)
        mTransport->OnStartRequest(request);

    return NS_OK;
}

NS_IMETHODIMP
ipcReceiver::OnStopRequest(nsIRequest *request,
                           nsISupports *context,
                           nsresult status)
{
    LOG(("ipcReceiver::OnStopRequest [status=%x]\n", status));

    if (mTransport)
        mTransport->OnStopRequest(request, status);

    return NS_OK;
}

static NS_METHOD ipcReadMessage(nsIInputStream *stream,
                                void           *closure,
                                const char     *segment,
                                PRUint32        offset,
                                PRUint32        count,
                                PRUint32       *countRead)
{
    ipcReceiver *receiver = (ipcReceiver *) closure;
    return receiver->ReadSegment(segment, count, countRead);
}

NS_IMETHODIMP
ipcReceiver::OnDataAvailable(nsIRequest *request,
                             nsISupports *context,
                             nsIInputStream *stream,
                             PRUint32 offset,
                             PRUint32 count)
{
    LOG(("ipcReceiver::OnDataAvailable [count=%u]\n", count));

    PRUint32 countRead;
    return stream->ReadSegments(ipcReadMessage, this, count, &countRead);
}

nsresult
ipcReceiver::ReadSegment(const char *ptr, PRUint32 count, PRUint32 *countRead)
{
    *countRead = 0;
    while (count) {
        PRUint32 nread;
        PRBool complete;

        mMsg.ReadFrom(ptr, count, &nread, &complete);

        if (complete) {
            mTransport->OnMessageAvailable(&mMsg);
            mMsg.Reset();
        }

        count -= nread;
        ptr += nread;
        *countRead += nread;
    }

    return NS_OK;
}
