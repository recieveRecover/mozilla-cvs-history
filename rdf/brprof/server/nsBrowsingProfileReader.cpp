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

#define FORCE_PR_LOG

#if defined(WIN32)
#include <strstrea.h>
#else
#include <strstream.h>
#endif

#include "ONESrvPI.hpp"
#include "nsCOMPtr.h"
#include "nsIBrowsingProfile.h"
#include "nsIEventQueueService.h"
#include "nsIGenericFactory.h"
#include "nsIRDFDataSource.h"
#include "nsIRDFNode.h"
#include "nsIRDFResource.h"
#include "nsIRDFService.h"
#include "nsIServiceManager.h"
#include "nsRDFCID.h"
#include "nsString.h"
#include "nsXPIDLString.h"
#include "plevent.h"
#include "plstr.h"
#include "prlog.h"
#include "rdf.h"

////////////////////////////////////////////////////////////////////////

// brprof
static NS_DEFINE_CID(kBrowsingProfileCID,   NS_BROWSINGPROFILE_CID);

// rdf
static NS_DEFINE_CID(kRDFServiceCID,        NS_RDFSERVICE_CID);

// xpcom
static NS_DEFINE_CID(kEventQueueServiceCID, NS_EVENTQUEUESERVICE_CID);
static NS_DEFINE_CID(kGenericFactoryCID,    NS_GENERICFACTORY_CID);

DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, Page);

#define OPENDIR_NAMESPACE_URI "http://directory.mozilla.org/rdf#"
DEFINE_RDF_VOCAB(OPENDIR_NAMESPACE_URI, OPENDIR, Topic);
DEFINE_RDF_VOCAB(OPENDIR_NAMESPACE_URI, OPENDIR, narrow);
DEFINE_RDF_VOCAB(OPENDIR_NAMESPACE_URI, OPENDIR, catid);

////////////////////////////////////////////////////////////////////////

#ifdef PR_LOGGING
PRLogModuleInfo* gLog;
#endif

const char kInstanceName[] = "BRPROF";

class nsBrowsingProfileReader : public WAIWebApplicationService
{
protected:
    nsIRDFService* mRDFService;
    nsIRDFDataSource* mDirectory;

    // pseudo-constants
    static nsIRDFResource* kNC_Page;
    static nsIRDFResource* kOPENDIR_Topic;
    static nsIRDFResource* kOPENDIR_narrow;
    static nsIRDFResource* kOPENDIR_catid;

public:
    nsBrowsingProfileReader(const char *object_name = (const char *)NULL,
                            int argc=0,
                            char **argv=0);

    virtual ~nsBrowsingProfileReader();

    virtual long
    Run(WAIServerRequest_ptr session);

    virtual char*
    getServiceInfo();

    nsresult
    Init();

    nsresult
    WriteVector(WAIServerRequest_ptr aSession,
                const nsBrowsingProfileVector& aVector);

    nsresult
    WriteError(WAIServerRequest_ptr aSession,
               const char* aMessage);

    nsresult
    GetCategory(PRInt32 aCategoryID, nsXPIDLCString& aCategory);
};

nsIRDFResource* nsBrowsingProfileReader::kNC_Page        = nsnull;
nsIRDFResource* nsBrowsingProfileReader::kOPENDIR_Topic  = nsnull;
nsIRDFResource* nsBrowsingProfileReader::kOPENDIR_narrow = nsnull;
nsIRDFResource* nsBrowsingProfileReader::kOPENDIR_catid  = nsnull;

////////////////////////////////////////////////////////////////////////

nsBrowsingProfileReader::nsBrowsingProfileReader(const char *object_name, int argc, char **argv)
    : WAIWebApplicationService(object_name, argc, argv),
      mRDFService(nsnull),
      mDirectory(nsnull)
{
#ifdef PR_LOGGING
    if (! gLog)
        gLog = PR_NewLogModule("nsBrowsingProfileReader");
#endif
}


nsBrowsingProfileReader::~nsBrowsingProfileReader()
{
    NS_IF_RELEASE(mDirectory);
    if (mRDFService) {
        nsServiceManager::ReleaseService(kRDFServiceCID, mRDFService);
    }
}


long
nsBrowsingProfileReader::Run(WAIServerRequest_ptr aSession)
{
    static const char kBrowsingProfileCookiePrefix[] = "BP=";

    PR_LOG(gLog, PR_LOG_DEBUG,
           ("(%p) start processing request", aSession));

    // Process an HTTP request
    char *cookie = nsnull;
    if ((WAISPISuccess == aSession->getCookie(cookie)) && (nsnull != cookie)) {
		char* browsingProfileCookie =
			PL_strstr(cookie, kBrowsingProfileCookiePrefix);

		if (browsingProfileCookie) {
            // skip over the "BP=" so we can point directly to the hex
            // digits.
            browsingProfileCookie += sizeof(kBrowsingProfileCookiePrefix) - 1;

            PR_LOG(gLog, PR_LOG_DEBUG,
                   ("(%p) browsing profile cookie is set", aSession));

            PR_LOG(gLog, PR_LOG_DEBUG,
                   ("(%p) %s", aSession, browsingProfileCookie));

            // create a cookie object into which we'll try to stuff the
            // hex digits (i.e., let the cookie object do the parsing).
            nsresult rv;
            nsIBrowsingProfile* profile;
            rv = nsComponentManager::CreateInstance(kBrowsingProfileCID,
                                                    nsnull,
                                                    NS_GET_IID(nsIBrowsingProfile),
                                                    (void**) &profile);

            NS_ASSERTION(NS_SUCCEEDED(rv), "unable to create profile");
            if (NS_SUCCEEDED(rv)) {
                PR_LOG(gLog, PR_LOG_DEBUG,
                       ("(%p) constructed profile", aSession));

                rv = profile->SetCookieString(browsingProfileCookie);
                NS_ASSERTION(NS_SUCCEEDED(rv), "unable to set cookie");

                if (NS_SUCCEEDED(rv)) {
                    PR_LOG(gLog, PR_LOG_DEBUG,
                           ("(%p) set cookie string", aSession));

                    // now pull out the browsing profile vector and write
                    // it back as fancy HTML.
                    nsBrowsingProfileVector vector;
                    rv = profile->GetVector(vector);
                    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get vector");

                    if (NS_SUCCEEDED(rv)) {
                        PR_LOG(gLog, PR_LOG_DEBUG,
                               ("(%p) got profile vector", aSession));

                        rv = WriteVector(aSession, vector);
                    }
                }
                NS_RELEASE(profile);
            }
        }
        else {
            WriteError(aSession, "No profile cookie");
        }

        StringDelete(cookie);
    }
    else {
        WriteError(aSession, "No profile cookie.");
    }

    PR_LOG(gLog, PR_LOG_DEBUG,
           ("(%p) done processing request", aSession));

    return 0;
}


char*
nsBrowsingProfileReader::getServiceInfo()
{
    return StringDup("test ver 1.0");
}



nsresult
nsBrowsingProfileReader::Init()
{
    // Bring up RDF, read in the directory data source
    nsresult rv;

    rv = nsServiceManager::GetService(kRDFServiceCID,
                                      NS_GET_IID(nsIRDFService),
                                      (nsISupports**) &mRDFService);

    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get RDF service");
    if (NS_FAILED(rv)) return rv;

    rv = mRDFService->GetDataSource("resource:/res/samples/directory.rdf", &mDirectory);
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get directory data source");
    if (NS_FAILED(rv)) return rv;

    rv = mRDFService->GetResource(kURINC_Page, &kNC_Page);
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get resource");
    if (NS_FAILED(rv)) return rv;

    rv = mRDFService->GetResource(kURIOPENDIR_Topic, &kOPENDIR_Topic);
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get resource");
    if (NS_FAILED(rv)) return rv;

    rv = mRDFService->GetResource(kURIOPENDIR_narrow, &kOPENDIR_narrow);
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get resource");
    if (NS_FAILED(rv)) return rv;

    rv = mRDFService->GetResource(kURIOPENDIR_catid, &kOPENDIR_catid);
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get resource");
    if (NS_FAILED(rv)) return rv;
    
    return NS_OK;
}



nsresult
nsBrowsingProfileReader::WriteVector(WAIServerRequest_ptr aSession,
                                     const nsBrowsingProfileVector& aVector)
{
    ostrstream out;

    // header
#if 0
    out << "<html>" << endl;
    out << "<head>" << endl;
    out << "<title>Browsing Profile</title" << endl;
    out << "</head>" << endl;
    out << "<body>" << endl;
#endif

    out << "<h1>Browsing Profile</h1>" << endl;

    // magic
    out << "<b>Magic:</b> ";
    out << aVector.mHeader.mInfo.mCheck << " ";
    if (aVector.mHeader.mInfo.mCheck == nsBrowsingProfile_Check)
        out << "(good)";
    else
        out << "<font color='red'>(bad)</font>";
    out << "<br></br>" << endl;

    // version
    PR_LOG(gLog, PR_LOG_DEBUG,
           ("(%p) cookie version %d.%d",
            aSession,
            aVector.mHeader.mInfo.mMajorVersion,
            aVector.mHeader.mInfo.mMinorVersion));

    out << "<b>Version:</b> ";
    out << aVector.mHeader.mInfo.mMajorVersion << ".";
    out << aVector.mHeader.mInfo.mMinorVersion << "<br></br>" << endl;

    // categories
    out << "<table border='1'>" << endl;
    out << "<tr>"               << endl;
    out << "<td>Category</td>"  << endl;
    out << "<td>Visits</td>"    << endl;
    out << "<td>Flags</td>"     << endl;
    out << "</tr>"              << endl;

    nsXPIDLCString category;
    for (PRInt32 i = 0; i < nsBrowsingProfile_CategoryCount; ++i) {
        // zero means no more categories -- profile is small
        if (aVector.mCategory[i].mID == 0)
            break;

        nsresult rv = GetCategory(aVector.mCategory[i].mID, category);
        if (NS_SUCCEEDED(rv)) {
            out << "  <tr>" << endl;
            out << "    <td>" << (const char*) category                     << "</td>" << endl;
            out << "    <td>" << (PRInt32) aVector.mCategory[i].mVisitCount << "</td>" << endl;
            out << "    <td>" << (PRInt32) aVector.mCategory[i].mFlags      << "</td>" << endl;
            out << "  </tr>" << endl;
        }
    }
    out << "</table>" << endl;

    // footer
#if 0
    out << "</body>" << endl;
    out << "</html>" << endl;
#endif

    aSession->setResponseContentLength(out.pcount());
    aSession->StartResponse();
    aSession->WriteClient((const unsigned char *)out.str(), out.pcount()); 
    out.rdbuf()->freeze(0);

    return NS_OK;
}


nsresult
nsBrowsingProfileReader::WriteError(WAIServerRequest_ptr aSession,
                                    const char* aMessage)
{
    ostrstream out;

    out << "<h1>Browsing Profile</h1>" << endl;
    out << "<b>" << aMessage << "</b>" << endl;

    aSession->setResponseContentLength(out.pcount());
    aSession->StartResponse();
    aSession->WriteClient((const unsigned char *)out.str(), out.pcount()); 
    out.rdbuf()->freeze(0);

    return NS_OK;
}

nsresult
nsBrowsingProfileReader::GetCategory(PRInt32 aCategoryID, nsXPIDLCString& aCategory)
{
    nsresult rv;
    nsAutoString categoryIDStr;
    categoryIDStr.Append(aCategoryID, 10);

    nsCOMPtr<nsIRDFLiteral> categoryIDLiteral;
    rv = mRDFService->GetLiteral(categoryIDStr, getter_AddRefs(categoryIDLiteral));
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get literal for category ID");
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIRDFResource> topic;
    rv = mDirectory->GetSource(kOPENDIR_catid, categoryIDLiteral, PR_TRUE, getter_AddRefs(topic));
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to find topic for category");
    if (NS_FAILED(rv)) return rv;

    // if we didn't actually find a value, then GetCategory() failed.
    if (rv != NS_OK)
        return NS_ERROR_FAILURE;

    rv = topic->GetValue(getter_Copies(aCategory));
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get category URI");
    if (NS_FAILED(rv)) return rv;

    PR_LOG(gLog, PR_LOG_DEBUG,
           ("  %d ==> %s", aCategoryID, (const char*) aCategory));

    return NS_OK;
}


////////////////////////////////////////////////////////////////////////

//#include "nsDOMCID.h"    // for NS_SCRIPT_NAMESET_REGISTRY_CID

#include "nsIIOService.h"

#include "nsLayoutCID.h"   // for NS_NAMESPACEMANAGER_CID
#include "nsParserCIID.h"

#if defined(XP_PC)
#define DOM_DLL    "jsdom.dll"
#define LAYOUT_DLL "gkhtml.dll"
#define NETLIB_DLL "netlib.dll"
#define PARSER_DLL "gkparser.dll"
#define RDF_DLL    "rdf.dll"
#define XPCOM_DLL  "xpcom32.dll"
#elif defined(XP_UNIX) || defined(XP_BEOS)
#define DOM_DLL    "libjsdom"MOZ_DLL_SUFFIX
#define LAYOUT_DLL "libgklayout"MOZ_DLL_SUFFIX
#define NETLIB_DLL "libnecko"MOZ_DLL_SUFFIX
#define PARSER_DLL "libhtmlpars"MOZ_DLL_SUFFIX
#define RDF_DLL    "librdf"MOZ_DLL_SUFFIX
#define XPCOM_DLL  "libxpcom"MOZ_DLL_SUFFIX
#elif defined(XP_MAC)
#define DOM_DLL    "DOM_DLL"
#define LAYOUT_DLL "LAYOUT_DLL"
#define NETLIB_DLL "NETLIB_DLL"
#define PARSER_DLL "PARSER_DLL"
#define RDF_DLL    "RDF_DLL"
#define XPCOM_DLL  "XPCOM_DLL"
#endif

static nsresult
SetupRegistry()
{
    // netlib
    static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);
    nsComponentManager::RegisterComponent(kIOServiceCID, NULL, NULL, NETLIB_DLL, PR_FALSE, PR_FALSE);

    // parser
    static NS_DEFINE_CID(kParserCID,                NS_PARSER_IID);
    static NS_DEFINE_CID(kWellFormedDTDCID,         NS_WELLFORMEDDTD_CID);

    nsComponentManager::RegisterComponent(kParserCID,                NULL, NULL, PARSER_DLL, PR_FALSE, PR_FALSE);
    nsComponentManager::RegisterComponent(kWellFormedDTDCID,         NULL, NULL, PARSER_DLL, PR_FALSE, PR_FALSE);

    // layout
    static NS_DEFINE_CID(kNameSpaceManagerCID,      NS_NAMESPACEMANAGER_CID);

    nsComponentManager::RegisterComponent(kNameSpaceManagerCID,      NULL, NULL, LAYOUT_DLL, PR_FALSE, PR_FALSE);

    // xpcom
    nsComponentManager::RegisterComponent(kEventQueueServiceCID,     NULL, NULL, XPCOM_DLL,  PR_FALSE, PR_FALSE);
    nsComponentManager::RegisterComponent(kGenericFactoryCID,        NULL, NULL, XPCOM_DLL,  PR_FALSE, PR_FALSE);

    return NS_OK;
}

////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv)
{
    // Get the hostname we're supposed to talk to
    char localhost[256];
    char *host;

    if (argc > 1) {
        if (argc == 1)
            host = argv[1];
        else if (*argv[1] != '-')
            host = argv[1];
    }
    else {
        gethostname(localhost, sizeof(localhost));
        host = localhost;
    }

    nsresult rv;

    // Setup the registry
    SetupRegistry();

    // Get netlib off the floor...
    nsIEventQueueService* theEventQueueService = nsnull;
    rv = nsServiceManager::GetService(kEventQueueServiceCID,
                                      NS_GET_IID(nsIEventQueueService),
                                      (nsISupports**) &theEventQueueService);

    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get event queue service");
    if (NS_FAILED(rv)) return rv;

    rv = theEventQueueService->CreateThreadEventQueue();
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to create thread event queue");
    if (NS_FAILED(rv)) return rv;

    nsIEventQueue* mainQueue;
    rv = theEventQueueService->GetThreadEventQueue(NS_CURRENT_THREAD,
                                                   &mainQueue);

    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get event queue for current thread");
    if (NS_FAILED(rv)) return rv;

		NS_IF_RELEASE(mainQueue);

    // Create a WAI instance, register, and run.
    nsBrowsingProfileReader service(kInstanceName, argc, argv);
    rv = service.Init();
    if (NS_FAILED(rv)) return rv;

    if (! service.RegisterService(host)) {
        printf("Failed to Register with %s\n", host);
        return 1;
    }

    /* Wait forever */
    while(1)
        Sleep(1000);

    return 0;
}
