/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Sun Microsystems, Inc.
 * Portions created by the Initial Developer are Copyright (C) 2001
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * Created by: Paul Sandoz   <paul.sandoz@sun.com> 
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

#include "nsAbLDAPDirectory.h"
#include "nsAbLDAPProperties.h"
#include "nsAbUtils.h"

#include "nsAbQueryStringToExpression.h"

#include "nsAbBaseCID.h"
#include "nsIAddrBookSession.h"

#include "nsIRDFService.h"

#include "nsString.h"
#include "nsXPIDLString.h"
#include "prprf.h"
#include "nsAutoLock.h"


nsAbLDAPDirectory::nsAbLDAPDirectory() :
    nsAbDirectoryRDFResource(),
    nsAbLDAPDirectoryQuery (),
    mInitialized(PR_FALSE),
    mInitializedConnection(PR_FALSE),
    mPerformingQuery(PR_FALSE),
    mContext(0),
    mLock(0)
{
}

nsAbLDAPDirectory::~nsAbLDAPDirectory()
{
    if (mLock)
        PR_DestroyLock (mLock);
}

NS_IMPL_ISUPPORTS_INHERITED3(nsAbLDAPDirectory, nsAbDirectoryRDFResource, nsIAbDirectory, nsIAbDirectoryQuery, nsIAbDirectorySearch)


nsresult nsAbLDAPDirectory::Initiate ()
{
    if (mIsQueryURI == PR_FALSE)
        return NS_ERROR_FAILURE;

    if (mInitialized == PR_TRUE)
        return NS_OK;

    nsresult rv;

    mLock = PR_NewLock ();
    if(!mLock)
    {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    rv = nsAbQueryStringToExpression::Convert (mQueryString.get (),
        getter_AddRefs(mExpression));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = InitiateConnection ();

    mInitialized = PR_TRUE;
    return rv;
}

nsresult nsAbLDAPDirectory::InitiateConnection ()
{
    if (mInitializedConnection == PR_TRUE)
        return NS_OK;

    nsresult rv;

    mURL = do_CreateInstance(NS_LDAPURL_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCAutoString uriString (mURINoQuery);
    uriString.ReplaceSubstring ("moz-abldapdirectory:", "ldap:");

    rv = mURL->SetSpec(uriString.get ());
    NS_ENSURE_SUCCESS(rv, rv);

    mConnection = do_CreateInstance(NS_LDAPCONNECTION_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    mInitializedConnection = PR_TRUE;
    return rv;
}


/* 
 *
 * nsIAbDirectory methods
 *
 */

NS_IMETHODIMP nsAbLDAPDirectory::GetOperations(PRInt32 *aOperations)
{
    *aOperations = nsIAbDirectory::opSearch;
    return NS_OK;
}

NS_IMETHODIMP nsAbLDAPDirectory::GetChildNodes(nsIEnumerator* *result)
{
    nsCOMPtr<nsISupportsArray> array;
    NS_NewISupportsArray(getter_AddRefs(array));
    if (!array)
            return NS_ERROR_OUT_OF_MEMORY;

    return array->Enumerate(result);
}

NS_IMETHODIMP nsAbLDAPDirectory::GetChildCards(nsIEnumerator** result)
{
    nsresult rv;
    
    // Start the search
    rv = StartSearch ();
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsISupportsArray> array;
    NS_NewISupportsArray(getter_AddRefs(array));
    if (!array)
            return NS_ERROR_OUT_OF_MEMORY;

    return array->Enumerate(result);
}

NS_IMETHODIMP nsAbLDAPDirectory::HasCard(nsIAbCard* card, PRBool* hasCard)
{
    nsresult rv;

    rv = Initiate ();
    NS_ENSURE_SUCCESS(rv, rv);

    nsVoidKey key (NS_STATIC_CAST(void* ,card));

    // Enter lock
    nsAutoLock lock (mLock);

    *hasCard = mCache.Exists (&key);
    if (*hasCard == PR_FALSE && mPerformingQuery == PR_TRUE)
            return NS_ERROR_NOT_AVAILABLE;

    return NS_OK;
}

/* 
 *
 * nsAbLDAPDirectoryQuery methods
 *
 */

nsresult nsAbLDAPDirectory::GetLDAPConnection (nsILDAPConnection** connection)
{
    nsresult rv;

    rv = InitiateConnection ();
    NS_ENSURE_SUCCESS(rv, rv);

    NS_IF_ADDREF(*connection = mConnection);
    return rv;
}

nsresult nsAbLDAPDirectory::GetLDAPURL (nsILDAPURL** url)
{
    nsresult rv;

    rv = InitiateConnection ();
    NS_ENSURE_SUCCESS(rv, rv);

    NS_IF_ADDREF(*url = mURL);
    return rv;
}

nsresult nsAbLDAPDirectory::CreateCard (nsILDAPURL* uri, const char* dn, nsIAbCard** result)
{
    nsresult rv;

    nsCOMPtr <nsIAbCard> card = do_CreateInstance(NS_ABLDAPCARD_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    NS_IF_ADDREF(*result = card);
    return NS_OK;
}

/* 
 *
 * nsIAbDirectorySearch methods
 *
 */

NS_IMETHODIMP nsAbLDAPDirectory::StartSearch ()
{
    nsresult rv;
    
    if (mIsQueryURI == PR_FALSE ||
        mQueryString.Length () == 0)
        return NS_OK;


    rv = Initiate ();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = StopSearch ();
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIAbDirectoryQueryArguments> arguments = do_CreateInstance(NS_ABDIRECTORYQUERYARGUMENTS_CONTRACTID,&rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = arguments->SetExpression (mExpression);
    NS_ENSURE_SUCCESS(rv, rv);

    // Set the return properties to
    // return nsIAbCard interfaces
    nsCStringArray properties;
    properties.AppendCString (nsCAutoString ("card:nsIAbCard"));
    CharPtrArrayGuard returnProperties (PR_FALSE);
    rv = CStringArrayToCharPtrArray::Convert (properties,returnProperties.GetSizeAddr(),
                    returnProperties.GetArrayAddr(), PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = arguments->SetReturnProperties (returnProperties.GetSize(), returnProperties.GetArray());
    NS_ENSURE_SUCCESS(rv, rv);

    rv = arguments->SetQuerySubDirectories (PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);

    // Set the the query listener
    nsCOMPtr<nsIAbDirectoryQueryResultListener> queryListener;
    nsAbDirSearchListener* _queryListener =
        new nsAbDirSearchListener (this);
    queryListener = _queryListener;

    // Perform the query
    //
    // XXX todo, instead of 100, use the ldap_2.servers.xxx.maxHits pref
    // the problem is how to get that value here.
    //
    // I'm thinking that nsAbDirectories should know their key so that
    // they can do a lookup of server values from the key, when they need it
    // (as those values can change)
    rv = DoQuery(arguments, queryListener, 100, 0, &mContext);
    NS_ENSURE_SUCCESS(rv, rv);
    
    // Enter lock
    nsAutoLock lock (mLock);
    mPerformingQuery = PR_TRUE;
    mCache.Reset ();

    return rv;
}

NS_IMETHODIMP nsAbLDAPDirectory::StopSearch ()
{
    nsresult rv;

    rv = Initiate ();
    NS_ENSURE_SUCCESS(rv, rv);

    // Enter lock
    {
        nsAutoLock lockGuard (mLock);
        if (mPerformingQuery == PR_FALSE)
            return NS_OK;
        mPerformingQuery = PR_FALSE;
    }
    // Exit lock

    rv = StopQuery (mContext);

    return rv;
}


/* 
 *
 * nsAbDirSearchListenerContext methods
 *
 */
nsresult nsAbLDAPDirectory::OnSearchFinished (PRInt32 result)
{
    nsresult rv;

    rv = Initiate ();
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoLock lock (mLock);
    mPerformingQuery = PR_FALSE;

    return NS_OK;
}

nsresult nsAbLDAPDirectory::OnSearchFoundCard (nsIAbCard* card)
{
    nsresult rv;

    rv = Initiate ();
    NS_ENSURE_SUCCESS(rv, rv);

    nsVoidKey key (NS_STATIC_CAST(void* ,card));
    // Enter lock
    {
        nsAutoLock lock (mLock);
        mCache.Put (&key, card);
    }
    // Exit lock

    nsCOMPtr<nsIAddrBookSession> abSession = do_GetService(NS_ADDRBOOKSESSION_CONTRACTID, &rv);
    if(NS_SUCCEEDED(rv))
        abSession->NotifyDirectoryItemAdded(this, card);

    return NS_OK;
}

NS_IMETHODIMP nsAbLDAPDirectory::GetSupportsMailingLists(PRBool *aSupportsMailingsLists)
{
  NS_ENSURE_ARG_POINTER(aSupportsMailingsLists);
  *aSupportsMailingsLists = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP nsAbLDAPDirectory::GetIsRemote(PRBool *aIsRemote)
{
  NS_ENSURE_ARG_POINTER(aIsRemote);
  *aIsRemote = PR_TRUE;
  return NS_OK;
}
