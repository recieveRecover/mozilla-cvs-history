/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */

#include "msgCore.h"    // precompiled header...

#include "nsPop3Service.h"
#include "nsIMsgIncomingServer.h"
#include "nsIPop3IncomingServer.h"
#include "nsIMsgMailSession.h"

#include "nsIPref.h"

#include "nsPop3URL.h"
#include "nsPop3Sink.h"
#include "nsPop3Protocol.h"
#include "nsMsgLocalCID.h"
#include "nsMsgBaseCID.h"
#include "nsXPIDLString.h"
#include "nsCOMPtr.h"
#include "nsIMsgWindow.h"

#include "nsIFileLocator.h"
#include "nsFileLocations.h"
#include "nsIRDFService.h"
#include "nsIRDFDataSource.h"
#include "nsRDFCID.h"

#define POP3_PORT 110 // The IANA port for Pop3

#define PREF_MAIL_ROOT_POP3 "mail.root.pop3"

static NS_DEFINE_CID(kPrefCID, NS_PREF_CID);
static NS_DEFINE_CID(kPop3UrlCID, NS_POP3URL_CID);
static NS_DEFINE_CID(kMsgMailSessionCID, NS_MSGMAILSESSION_CID);
static NS_DEFINE_IID(kIFileLocatorIID,      NS_IFILELOCATOR_IID);
static NS_DEFINE_CID(kFileLocatorCID,       NS_FILELOCATOR_CID);
static NS_DEFINE_CID(kRDFServiceCID, NS_RDFSERVICE_CID);

nsPop3Service::nsPop3Service()
{
    NS_INIT_REFCNT();
}

nsPop3Service::~nsPop3Service()
{}

NS_IMPL_ISUPPORTS3(nsPop3Service,
                         nsIPop3Service,
                         nsIProtocolHandler,
                         nsIMsgProtocolInfo)

NS_IMETHODIMP nsPop3Service::CheckForNewMail(nsIMsgWindow* aMsgWindow, 
							   nsIUrlListener * aUrlListener,
							   nsIMsgFolder *inbox, 
                               nsIPop3IncomingServer *popServer,
                               nsIURI ** aURL)
{
	nsresult rv = NS_OK;
	char * hostname = nsnull;

    nsCOMPtr<nsIMsgIncomingServer> server;
	nsCOMPtr<nsIURI> url;

	server = do_QueryInterface(popServer);
    if (server) 
		server->GetHostName(&hostname);
    
	if (NS_SUCCEEDED(rv) && popServer && hostname)
	{
        // now construct a pop3 url...
        char * urlSpec = PR_smprintf("pop3://%s:%d?check", hostname, POP3_PORT);
        rv = BuildPop3Url(urlSpec, inbox, popServer, aUrlListener, getter_AddRefs(url), aMsgWindow);
        PR_FREEIF(urlSpec);
		if (hostname) PL_strfree(hostname);
    }

    
	if (NS_SUCCEEDED(rv) && url) 
		rv = RunPopUrl(server, url);

	if (aURL && url) // we already have a ref count on pop3url...
	{
		*aURL = url; // transfer ref count to the caller...
		NS_IF_ADDREF(*aURL);
	}
	
	return rv;
}


nsresult nsPop3Service::GetNewMail(nsIMsgWindow *aMsgWindow, nsIUrlListener * aUrlListener,
                                   nsIPop3IncomingServer *popServer,
                                   nsIURI ** aURL)
{
	nsresult rv = NS_OK;
	char * popHost = nsnull;
	nsCOMPtr<nsIURI> url;

	nsCOMPtr<nsIMsgIncomingServer> server;
	server = do_QueryInterface(popServer);    

    if (server) 
		server->GetHostName(&popHost);

    if (!popHost) return NS_ERROR_FAILURE;
    
	if (NS_SUCCEEDED(rv) && popServer)
	{
        // now construct a pop3 url...
        char * urlSpec = PR_smprintf("pop3://%s:%d", popHost, POP3_PORT);
        rv = BuildPop3Url(urlSpec, nsnull, popServer, aUrlListener, getter_AddRefs(url), aMsgWindow);
        PR_FREEIF(urlSpec);
	}
    
	if (NS_SUCCEEDED(rv) && url) 
	{
		nsCOMPtr <nsIMsgMailNewsUrl> mailNewsUrl = do_QueryInterface(url);
		if (mailNewsUrl)
			mailNewsUrl->SetMsgWindow(aMsgWindow);
		rv = RunPopUrl(server, url);
	}

    if (popHost) PL_strfree(popHost);

	if (aURL && url) // we already have a ref count on pop3url...
	{
		*aURL = url; // transfer ref count to the caller...
		NS_IF_ADDREF(*aURL);
	}
	return rv;
}

nsresult nsPop3Service::BuildPop3Url(char * urlSpec,
									 nsIMsgFolder *inbox,
                                     nsIPop3IncomingServer *server,
									 nsIUrlListener * aUrlListener,
                                     nsIURI ** aUrl,
									 nsIMsgWindow *aMsgWindow)
{
	nsPop3Sink * pop3Sink = new nsPop3Sink();
	if (pop3Sink)
	{
		pop3Sink->SetPopServer(server);
		pop3Sink->SetFolder(inbox);
	}

	// now create a pop3 url and a protocol instance to run the url....
	nsCOMPtr<nsIPop3URL> pop3Url;
	nsresult rv = nsComponentManager::CreateInstance(kPop3UrlCID,
                                            nsnull,
                                            NS_GET_IID(nsIPop3URL),
                                            getter_AddRefs(pop3Url));
	if (pop3Url)
	{
		nsXPIDLCString userName;
		nsCOMPtr<nsIMsgIncomingServer> msgServer = do_QueryInterface(server);
		msgServer->GetUsername(getter_Copies(userName));

		pop3Url->SetPop3Sink(pop3Sink);
		pop3Url->SetUsername(userName);

		if (aUrlListener)
		{
			nsCOMPtr<nsIMsgMailNewsUrl> mailnewsurl = do_QueryInterface(pop3Url);
			if (mailnewsurl)
			{
				mailnewsurl->RegisterListener(aUrlListener);

				mailnewsurl->SetMsgWindow(aMsgWindow);
			}
		}


		if (aUrl)
		{
			rv = pop3Url->QueryInterface(NS_GET_IID(nsIURI), (void **) aUrl);
			if (*aUrl)
			{
				(*aUrl)->SetSpec(urlSpec);
				// the following is only a temporary work around hack because necko
				// is loosing our port when the url is just scheme://host:port.
				// when they fix this bug I can remove the following code where we
				// manually set the port.
				(*aUrl)->SetPort(POP3_PORT);
			}
		}
	}

	return rv;
}

nsresult nsPop3Service::RunPopUrl(nsIMsgIncomingServer * aServer, nsIURI * aUrlToRun)
{
	nsresult rv = NS_OK;
	if (aServer && aUrlToRun)
	{
		nsXPIDLCString userName;

		// load up required server information
		rv = aServer->GetUsername(getter_Copies(userName));

		// find out if the server is busy or not...if the server is busy, we are 
		// *NOT* going to run the url
		PRBool serverBusy = PR_FALSE;
		rv = aServer->GetServerBusy(&serverBusy);

		if (!serverBusy)
		{
			nsPop3Protocol * protocol = new nsPop3Protocol(aUrlToRun);
			if (protocol)
			{
				rv = protocol->Initialize(aUrlToRun);
				if(NS_FAILED(rv))
				{
					delete protocol;
					return rv;
				}
				protocol->SetUsername(userName);
				rv = protocol->LoadUrl(aUrlToRun);
			}
		} 
	} // if server

	return rv;
}


NS_IMETHODIMP nsPop3Service::GetScheme(char * *aScheme)
{
	nsresult rv = NS_OK;
	if (aScheme)
		*aScheme = nsCRT::strdup("pop3");
	else
		rv = NS_ERROR_NULL_POINTER;
	return rv; 
}

NS_IMETHODIMP nsPop3Service::GetDefaultPort(PRInt32 *aDefaultPort)
{
	nsresult rv = NS_OK;
	if (aDefaultPort)
		*aDefaultPort = POP3_PORT;
	else
		rv = NS_ERROR_NULL_POINTER;
	return rv; 	
}

NS_IMETHODIMP nsPop3Service::NewURI(const char *aSpec, nsIURI *aBaseURI, nsIURI **_retval)
{
    nsresult rv = NS_ERROR_FAILURE;
    if (!aSpec || !_retval) return rv;
    nsCAutoString folderUri(aSpec);
    nsCOMPtr<nsIRDFResource> resource;
    PRInt32 offset = folderUri.Find("?");
    if (offset)
        folderUri.Truncate(offset);

	NS_WITH_SERVICE(nsIRDFService, rdfService, kRDFServiceCID, &rv); 
    if (NS_FAILED(rv)) return rv;
    rv = rdfService->GetResource(folderUri.GetBuffer(),
                                 getter_AddRefs(resource));
    if (NS_FAILED(rv)) return rv;
    nsCOMPtr<nsIMsgFolder> folder = do_QueryInterface(resource, &rv);
    if (NS_FAILED(rv)) return rv;
    nsCOMPtr<nsIMsgIncomingServer> server;
    rv = folder->GetServer(getter_AddRefs(server));
    if (NS_FAILED(rv)) return rv;
    nsCOMPtr<nsIPop3IncomingServer> popServer = do_QueryInterface(server,&rv);
    if (NS_FAILED(rv)) return rv;
    nsXPIDLCString hostname;
    nsXPIDLCString username;
    server->GetHostName(getter_Copies(hostname));
    server->GetUsername(getter_Copies(username));
    nsCAutoString popSpec = "pop://";
    popSpec += hostname;
    popSpec += ":";
    popSpec.AppendInt(POP3_PORT);
    popSpec += "?";
    const char *uidl = PL_strstr(aSpec, "uidl=");
    if (!uidl) return NS_ERROR_FAILURE;
    popSpec += uidl;
    nsCOMPtr<nsIUrlListener> urlListener = do_QueryInterface(folder, &rv);
    if (NS_FAILED(rv)) return rv;
    rv = BuildPop3Url((char *)popSpec.GetBuffer(), folder, popServer,
                      urlListener, _retval, nsnull); 
    if (NS_SUCCEEDED(rv))
    {
        nsCOMPtr<nsIMsgMailNewsUrl> mailnewsurl = 
            do_QueryInterface(*_retval, &rv);
        if (NS_SUCCEEDED(rv))
        {
            mailnewsurl->SetUsername((const char*) username);
        }
        nsCOMPtr<nsIPop3URL> popurl = do_QueryInterface(mailnewsurl, &rv);
        if (NS_SUCCEEDED(rv))
        {
            nsCAutoString messageUri (aSpec);
            messageUri.ReplaceSubstring("mailbox:", "mailbox_message:");
            messageUri.ReplaceSubstring("?number=", "#");
            offset = messageUri.Find("&");
            if (offset)
                messageUri.Truncate(offset);
            popurl->SetMessageUri(messageUri.GetBuffer());
            nsCOMPtr<nsIPop3Sink> pop3Sink;
            rv = popurl->GetPop3Sink(getter_AddRefs(pop3Sink));
            if (NS_SUCCEEDED(rv))
                pop3Sink->SetBuildMessageUri(PR_TRUE);
        }
    }
    return rv;
}

NS_IMETHODIMP nsPop3Service::NewChannel(nsIURI *aURI, nsIChannel **_retval)
{
	nsresult rv = NS_OK;
	nsPop3Protocol * protocol = new nsPop3Protocol(aURI);
	if (protocol)
	{
        rv = protocol->Initialize(aURI);
        if (NS_FAILED(rv)) 
        {
            delete protocol;
            return rv;
        }
        nsXPIDLCString username;
        nsCOMPtr<nsIMsgMailNewsUrl> url = do_QueryInterface(aURI, &rv);
        if (NS_SUCCEEDED(rv) && url)
        {
            url->GetUsername(getter_Copies(username));
            protocol->SetUsername((const char *)username);
        }
		rv = protocol->QueryInterface(NS_GET_IID(nsIChannel), (void **) _retval);
	}
	else
		rv = NS_ERROR_NULL_POINTER;

	return rv;
}


NS_IMETHODIMP
nsPop3Service::SetDefaultLocalPath(nsIFileSpec *aPath)
{
    nsresult rv;
    NS_WITH_SERVICE(nsIPref, prefs, kPrefCID, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = prefs->SetFilePref(PREF_MAIL_ROOT_POP3, aPath, PR_FALSE /* set default */);
    return rv;
}     

NS_IMETHODIMP
nsPop3Service::GetDefaultLocalPath(nsIFileSpec ** aResult)
{
    nsresult rv;
    NS_WITH_SERVICE(nsIPref, prefs, kPrefCID, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = prefs->GetFilePref(PREF_MAIL_ROOT_POP3, aResult);
    if (NS_SUCCEEDED(rv)) return rv;

    NS_WITH_SERVICE(nsIFileLocator, locator, kFileLocatorCID, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = locator->GetFileLocation(nsSpecialFileSpec::App_MailDirectory50, aResult);
    if (NS_FAILED(rv)) return rv;    

    rv = SetDefaultLocalPath(*aResult);
    return rv;
}
    

NS_IMETHODIMP
nsPop3Service::GetServerIID(nsIID* *aServerIID)
{
    *aServerIID = new nsIID(NS_GET_IID(nsIPop3IncomingServer));
    return NS_OK;
}

NS_IMETHODIMP
nsPop3Service::GetRequiresUsername(PRBool *aRequiresUsername)
{
        NS_ENSURE_ARG_POINTER(aRequiresUsername);
        *aRequiresUsername = PR_TRUE;
        return NS_OK;
}

NS_IMETHODIMP
nsPop3Service::GetPreflightPrettyNameWithEmailAddress(PRBool *aPreflightPrettyNameWithEmailAddress)
{
        NS_ENSURE_ARG_POINTER(aPreflightPrettyNameWithEmailAddress);
        *aPreflightPrettyNameWithEmailAddress = PR_TRUE;
        return NS_OK;
}

NS_IMETHODIMP
nsPop3Service::GetCanDelete(PRBool *aCanDelete)
{
        NS_ENSURE_ARG_POINTER(aCanDelete);
        *aCanDelete = PR_TRUE;
        return NS_OK;
}

NS_IMETHODIMP
nsPop3Service::GetCanDuplicate(PRBool *aCanDuplicate)
{
        NS_ENSURE_ARG_POINTER(aCanDuplicate);
        *aCanDuplicate = PR_TRUE;
        return NS_OK;
}        

NS_IMETHODIMP
nsPop3Service::GetDefaultCopiesAndFoldersPrefsToServer(PRBool *aDefaultCopiesAndFoldersPrefsToServer)
{
    NS_ENSURE_ARG_POINTER(aDefaultCopiesAndFoldersPrefsToServer);
    // when a pop3 server is created, the copies and folder prefs for the associated identity
    // point to folders on this server.
	// when we create a pop server, we give it its own Drafts, Sent, Templates, etc folders.
    *aDefaultCopiesAndFoldersPrefsToServer = PR_TRUE;
    return NS_OK;
} 


