/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#include "nsString2.h"
#include "nsIServiceManager.h"

#include "nsSocketProviderService.h"
#include "nsISocketProvider.h"

static NS_DEFINE_CID(kSocketProviderService, NS_SOCKETPROVIDERSERVICE_CID);

////////////////////////////////////////////////////////////////////////////////

nsSocketProviderService::nsSocketProviderService()
{
  NS_INIT_REFCNT();
}

nsresult
nsSocketProviderService::Init()
{
  return NS_OK;
}

nsSocketProviderService::~nsSocketProviderService()
{
}

NS_METHOD
nsSocketProviderService::Create(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
  nsresult rv;

  if (aOuter)
    return NS_ERROR_NO_AGGREGATION;

  nsSocketProviderService* pSockProvServ = new nsSocketProviderService();
  if (pSockProvServ == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(pSockProvServ);
  rv = pSockProvServ->Init();
  if (NS_FAILED(rv))
    {
    delete pSockProvServ;
    return rv;
    }
  rv = pSockProvServ->QueryInterface(aIID, aResult);
  NS_RELEASE(pSockProvServ);

  return rv;
}

NS_IMPL_ISUPPORTS(nsSocketProviderService, NS_GET_IID(nsISocketProviderService));

////////////////////////////////////////////////////////////////////////////////

#define MAX_TYPE_LENGTH 128

#define MAX_SOCKET_TYPE_PROGID_LENGTH   (MAX_TYPE_LENGTH + NS_NETWORK_SOCKET_PROGID_PREFIX_LENGTH + 1)

NS_IMETHODIMP
nsSocketProviderService::GetSocketProvider(const char *aSocketType, nsISocketProvider **_result)
{
  nsresult rv;

  char buf[MAX_SOCKET_TYPE_PROGID_LENGTH];

  nsAutoString2 progID(NS_NETWORK_SOCKET_PROGID_PREFIX);

  progID += aSocketType;
  progID.ToCString(buf, MAX_SOCKET_TYPE_PROGID_LENGTH);

  rv = nsServiceManager::GetService(buf, NS_GET_IID(nsISocketProvider), (nsISupports **)_result);
  if (NS_FAILED(rv)) 
      return NS_ERROR_UNKNOWN_SOCKET_TYPE;

  return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////
