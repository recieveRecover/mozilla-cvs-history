/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Communicator client code, 
 * released March 31, 1998. 
 *
 * The Initial Developer of the Original Code is Netscape Communications 
 * Corporation.  Portions created by Netscape are 
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 *
 * Contributors:
 *     Daniel Veditz <dveditz@netscape.com>
 */

#include "xpistub.h"

#include "nsRepository.h"
#include "nsIServiceManager.h"
#include "nsCOMPtr.h"

#include "nsSpecialSystemDirectory.h" 

#include "nscore.h"
#include "nspr.h"

#include "nsStubNotifier.h"

#include "nsISoftwareUpdate.h"
#include "nsSoftwareUpdateIIDs.h"
#include "nsPIXPIStubHook.h"

#include "plstr.h"

#ifdef XP_PC
#include <direct.h>
#define COMPONENT_REG "component.reg"
#endif

#ifdef XP_MAC
#define COMPONENT_REG "\pComponent Registry"
#include "MoreFilesExtras.h"
#endif


//------------------------------------------------------------------------
//          globals
//------------------------------------------------------------------------

static nsIXPINotifier      *gNotifier = 0;
static nsISoftwareUpdate   *gXPI = 0;
static nsIServiceManager   *gServiceMgr = 0;

static NS_DEFINE_IID(kSoftwareUpdateCID, NS_SoftwareUpdate_CID);



//------------------------------------------------------------------------
//          XPI_Init()
//------------------------------------------------------------------------
PR_PUBLIC_API(nsresult) XPI_Init(   
#ifdef XP_MAC
                                    const FSSpec&       aXPIStubDir, 
                                    const FSSpec&       aProgramDir,
#else
                                    const char*         aProgramDir,
#endif
                                    pfnXPIStart         startCB, 
                                    pfnXPIProgress      progressCB,
                                    pfnXPIFinal         finalCB     )
{
    nsresult              rv;
    nsCOMPtr<nsIFileSpec> nsIfsDirectory;
    nsFileSpec            nsfsDirectory;
    nsFileSpec            nsfsRegFile;

    //--------------------------------------------------------------------
    // Initialize XPCOM and AutoRegister() its components
    //--------------------------------------------------------------------
#ifdef XP_MAC
    OSErr      err = noErr;
    long       xpiStubDirID = 0;
    Boolean    isDir = false;
    FSSpec     fsRegFile;
    
    FSpGetDirectoryID(&aXPIStubDir, &xpiStubDirID, &isDir);
    err = FSMakeFSSpec(aXPIStubDir.vRefNum, xpiStubDirID, COMPONENT_REG, &fsRegFile);
    if (err!=noErr)
        return NS_ERROR_UNEXPECTED;
    	
    nsfsRegFile = fsRegFile;
    nsfsDirectory = aXPIStubDir;
    rv = NS_InitXPCOM(&gServiceMgr, &nsfsRegFile, &nsfsDirectory);
#elif defined(XP_PC)
    char componentPath[_MAX_PATH];

    getcwd(componentPath, _MAX_PATH);
    nsfsDirectory   = componentPath;
    nsfsRegFile     = componentPath;
    nsfsDirectory  += "\\components";
    nsfsRegFile    += "\\";
    nsfsRegFile    += COMPONENT_REG;

    rv = NS_InitXPCOM(&gServiceMgr, &nsfsRegFile, &nsfsDirectory);

#else
    rv = NS_InitXPCOM(&gServiceMgr, NULL, NULL);
#endif

    if (!NS_SUCCEEDED(rv))
        return rv;


    //--------------------------------------------------------------------
    // Get the SoftwareUpdate (XPInstall) service.
    //
    // Since AppShell is not started by XPIStub the XPI service is never 
    // registered with the service manager. We keep a local pointer to it 
    // so it stays alive througout.
    //--------------------------------------------------------------------
    rv = nsComponentManager::CreateInstance(kSoftwareUpdateCID, 
                                            nsnull,
                                            nsISoftwareUpdate::GetIID(),
                                            (void**) &gXPI);
    if (!NS_SUCCEEDED(rv))
        return rv;


    //--------------------------------------------------------------------
    // Override XPInstall's natural assumption that the current executable
    // is Mozilla. Use the given directory as the "Program" folder.
    //--------------------------------------------------------------------
    nsCOMPtr<nsPIXPIStubHook>   hook = do_QueryInterface(gXPI);
    nsFileSpec                  dirSpec( aProgramDir );
    nsCOMPtr<nsIFileSpec>       iDirSpec;

    NS_NewFileSpecWithSpec( dirSpec, getter_AddRefs(iDirSpec) );    
    
    if (hook && iDirSpec)
        hook->SetProgramDirectory( iDirSpec );
    else
        return NS_ERROR_NULL_POINTER;


    //--------------------------------------------------------------------
    // Save the install wizard's callbacks as a nsIXPINotifer for later
    //--------------------------------------------------------------------
    nsStubNotifier* stub = new nsStubNotifier( startCB, progressCB, finalCB );
    if (!stub)
    {
        gXPI->Release();
        rv = NS_ERROR_OUT_OF_MEMORY;
    }
    else
    {
        rv = stub->QueryInterface(nsIXPINotifier::GetIID(), (void**)&gNotifier);
    }
    return rv;
}



//------------------------------------------------------------------------
//          XPI_Exit()
//------------------------------------------------------------------------
PR_PUBLIC_API(void) XPI_Exit()
{
    if (gNotifier)
        gNotifier->Release();

    if (gXPI)
        gXPI->Release();

    NS_ShutdownXPCOM(gServiceMgr);

}




//------------------------------------------------------------------------
//          XPI_Install()
//------------------------------------------------------------------------
PR_PUBLIC_API(nsresult) XPI_Install(
#ifdef XP_MAC
                                    const FSSpec& aFile,
#else
                                    const char*   aFile,
#endif
                                    const char*   aArgs, 
                                    long          aFlags )
{
    nsresult                rv = NS_ERROR_NULL_POINTER;
    nsString                args(aArgs);
    nsCOMPtr<nsIFileSpec>   iFile;
    nsFileSpec              file(aFile);
    nsFileURL               URL(file);
    nsString                URLstr(URL.GetURLString());

    NS_NewFileSpecWithSpec( file, getter_AddRefs(iFile) );

    if (iFile && gXPI)
        rv = gXPI->InstallJar( iFile, URLstr.GetUnicode(), args.GetUnicode(), 
                               (aFlags | XPI_NO_NEW_THREAD), gNotifier );

    return rv;
}
