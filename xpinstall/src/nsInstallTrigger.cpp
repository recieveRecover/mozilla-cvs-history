/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are Copyright (C) 1998
 * Netscape Communications Corporation.  All Rights Reserved.
 */

#include "nsSoftwareUpdate.h"
#include "nsSoftwareUpdateStream.h"
#include "nsInstallTrigger.h"
#include "nsIDOMInstallTriggerGlobal.h"

#include "nscore.h"
#include "nsIFactory.h"
#include "nsISupports.h"
#include "nsIScriptGlobalObject.h"

#include "pratom.h"
#include "prefapi.h"

#include "nsSpecialSystemDirectory.h"

static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIFactoryIID, NS_IFACTORY_IID);
static NS_DEFINE_IID(kIScriptObjectOwnerIID, NS_ISCRIPTOBJECTOWNER_IID);

static NS_DEFINE_IID(kIInstallTrigger_IID, NS_IDOMINSTALLTRIGGERGLOBAL_IID);






nsInstallTrigger::nsInstallTrigger()
{
    mScriptObject   = nsnull;
    NS_INIT_REFCNT();
}

nsInstallTrigger::~nsInstallTrigger()
{
}

NS_IMETHODIMP 
nsInstallTrigger::QueryInterface(REFNSIID aIID,void** aInstancePtr)
{
    if (aInstancePtr == NULL)
    {
        return NS_ERROR_NULL_POINTER;
    }

    // Always NULL result, in case of failure
    *aInstancePtr = NULL;

    if ( aIID.Equals(kIScriptObjectOwnerIID))
    {
        *aInstancePtr = (void*) ((nsIScriptObjectOwner*)this);
        AddRef();
        return NS_OK;
    }
    else if ( aIID.Equals(kIInstallTrigger_IID) )
    {
        *aInstancePtr = (void*) ((nsIDOMInstallTriggerGlobal*)this);
        AddRef();
        return NS_OK;
    }
    else if ( aIID.Equals(kISupportsIID) )
    {
        *aInstancePtr = (void*)(nsISupports*)(nsIScriptObjectOwner*)this;
        AddRef();
        return NS_OK;
    }

     return NS_NOINTERFACE;
}

NS_IMPL_ADDREF(nsInstallTrigger)
NS_IMPL_RELEASE(nsInstallTrigger)



NS_IMETHODIMP 
nsInstallTrigger::GetScriptObject(nsIScriptContext *aContext, void** aScriptObject)
{
    NS_PRECONDITION(nsnull != aScriptObject, "null arg");
    nsresult res = NS_OK;
    
    if (nsnull == mScriptObject) 
    {
         nsIScriptGlobalObject *global = aContext->GetGlobalObject();

        res = NS_NewScriptInstallTriggerGlobal(  aContext, 
                                                (nsISupports *)(nsIDOMInstallTriggerGlobal*)this, 
                                                (nsISupports *)global, 
                                                &mScriptObject);
        NS_IF_RELEASE(global);

    }
  

    *aScriptObject = mScriptObject;
    return res;
}

NS_IMETHODIMP 
nsInstallTrigger::SetScriptObject(void *aScriptObject)
{
  mScriptObject = aScriptObject;
  return NS_OK;
}


NS_IMETHODIMP    
nsInstallTrigger::UpdateEnabled(PRBool* aReturn)
{
    PREF_GetBoolPref( (const char*) AUTOUPDATE_ENABLE_PREF, aReturn);
    return NS_OK;
}


NS_IMETHODIMP    
nsInstallTrigger::StartSoftwareUpdate(const nsString& aURL, PRInt32 aFlags, PRInt32* aReturn)
{
    nsString localFile;
    CreateTempFileFromURL(aURL, localFile);

    nsInstallInfo *nextInstall = new nsInstallInfo( aURL, localFile, aFlags);

    // start the download (this will clean itself up)
    nsSoftwareUpdateListener *downloader = new nsSoftwareUpdateListener(nextInstall);

    *aReturn = NS_OK;  // maybe we should do something more.
    return NS_OK;
}

NS_IMETHODIMP    
nsInstallTrigger::StartSoftwareUpdate(const nsString& aURL, PRInt32* aReturn)
{
    nsString localFile;
    CreateTempFileFromURL(aURL, localFile);

    nsInstallInfo *nextInstall = new nsInstallInfo( aURL, localFile, 0);

    // start the download (this will clean itself up)
    nsSoftwareUpdateListener *downloader = new nsSoftwareUpdateListener(nextInstall);

    *aReturn = NS_OK;  // maybe we should do something more.
    return NS_OK;
}

NS_IMETHODIMP    
nsInstallTrigger::ConditionalSoftwareUpdate(const nsString& aURL, const nsString& aRegName, PRInt32 aDiffLevel, const nsString& aVersion, PRInt32 aMode, PRInt32* aReturn)
{
    return NS_OK;
}

NS_IMETHODIMP    
nsInstallTrigger::ConditionalSoftwareUpdate(const nsString& aURL, const nsString& aRegName, PRInt32 aDiffLevel, nsIDOMInstallVersion* aVersion, PRInt32 aMode, PRInt32* aReturn)
{
    return NS_OK;
}

NS_IMETHODIMP    
nsInstallTrigger::ConditionalSoftwareUpdate(const nsString& aURL, const nsString& aRegName, nsIDOMInstallVersion* aVersion, PRInt32 aMode, PRInt32* aReturn)
{
    return NS_OK;
}

NS_IMETHODIMP    
nsInstallTrigger::ConditionalSoftwareUpdate(const nsString& aURL, const nsString& aRegName, const nsString& aVersion, PRInt32 aMode, PRInt32* aReturn)
{
    return NS_OK;
}

NS_IMETHODIMP    
nsInstallTrigger::ConditionalSoftwareUpdate(const nsString& aURL, const nsString& aRegName, const nsString& aVersion, PRInt32* aReturn)
{
    return NS_OK;
}

NS_IMETHODIMP    
nsInstallTrigger::ConditionalSoftwareUpdate(const nsString& aURL, const nsString& aRegName, nsIDOMInstallVersion* aVersion, PRInt32* aReturn)
{
    return NS_OK;
}

NS_IMETHODIMP    
nsInstallTrigger::CompareVersion(const nsString& aRegName, PRInt32 aMajor, PRInt32 aMinor, PRInt32 aRelease, PRInt32 aBuild, PRInt32* aReturn)
{
    return NS_OK;
}

NS_IMETHODIMP    
nsInstallTrigger::CompareVersion(const nsString& aRegName, const nsString& aVersion, PRInt32* aReturn)
{
    return NS_OK;
}

NS_IMETHODIMP    
nsInstallTrigger::CompareVersion(const nsString& aRegName, nsIDOMInstallVersion* aVersion, PRInt32* aReturn)
{
    return NS_OK;
}



// this will take a nsIUrl, and create a temporary file.  If it is local, we just us it.
 
void
nsInstallTrigger::CreateTempFileFromURL(const nsString& aURL, nsString& tempFileString)
{
    // Checking to see if the url is local

    if ( aURL.EqualsIgnoreCase("file://", 7) )
    {       
        tempFileString.SetString( nsNSPRPath(nsFileURL(aURL)) );
    }
    else
    {
        nsSpecialSystemDirectory tempFile(nsSpecialSystemDirectory::OS_TemporaryDirectory);
    
        PRInt32 result = aURL.RFind('/');
        if (result != -1)
        {    
            nsString jarName;
                       
            aURL.Right(jarName, (aURL.Length() - result) );
            
            PRInt32 argOffset = jarName.RFind('?');

            if (argOffset != -1)
            {
                // we need to remove ? and everything after it
                jarName.Truncate(argOffset);
            }
            
            
            tempFile += jarName;
        }
        else
        {   
            tempFile += "xpinstall.jar";
        }

        tempFile.MakeUnique();

        tempFileString.SetString( nsNSPRPath( nsFilePath(tempFile) ) );
    }
}


/////////////////////////////////////////////////////////////////////////
// 
/////////////////////////////////////////////////////////////////////////
static PRInt32 gInstallTriggerInstanceCnt = 0;
static PRInt32 gInstallTriggerLock        = 0;

nsInstallTriggerFactory::nsInstallTriggerFactory(void)
{
    mRefCnt=0;
    PR_AtomicIncrement(&gInstallTriggerInstanceCnt);
}

nsInstallTriggerFactory::~nsInstallTriggerFactory(void)
{
    PR_AtomicDecrement(&gInstallTriggerInstanceCnt);
}

NS_IMETHODIMP 
nsInstallTriggerFactory::QueryInterface(REFNSIID aIID,void** aInstancePtr)
{
    if (aInstancePtr == NULL)
    {
        return NS_ERROR_NULL_POINTER;
    }

    // Always NULL result, in case of failure
    *aInstancePtr = NULL;

    if ( aIID.Equals(kISupportsIID) )
    {
        *aInstancePtr = (void*) this;
    }
    else if ( aIID.Equals(kIFactoryIID) )
    {
        *aInstancePtr = (void*) this;
    }

    if (aInstancePtr == NULL)
    {
        return NS_ERROR_NO_INTERFACE;
    }

    AddRef();
    return NS_OK;
}



NS_IMETHODIMP
nsInstallTriggerFactory::AddRef(void)
{
    return ++mRefCnt;
}


NS_IMETHODIMP
nsInstallTriggerFactory::Release(void)
{
    if (--mRefCnt ==0)
    {
        delete this;
        return 0; // Don't access mRefCnt after deleting!
    }

    return mRefCnt;
}

NS_IMETHODIMP
nsInstallTriggerFactory::CreateInstance(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    if (aResult == NULL)
    {
        return NS_ERROR_NULL_POINTER;
    }

    *aResult = NULL;

    /* do I have to use iSupports? */
    nsInstallTrigger *inst = new nsInstallTrigger();

    if (inst == NULL)
        return NS_ERROR_OUT_OF_MEMORY;

    nsresult result =  inst->QueryInterface(aIID, aResult);

    if (NS_FAILED(result))
        delete inst;

    return result;
}

NS_IMETHODIMP
nsInstallTriggerFactory::LockFactory(PRBool aLock)
{
    if (aLock)
        PR_AtomicIncrement(&gInstallTriggerLock);
    else
        PR_AtomicDecrement(&gInstallTriggerLock);

    return NS_OK;
}
