/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

/* nsDll
 *
 * Abstraction of a Dll. Stores modifiedTime and size for easy detection of
 * change in dll.
 *
 * dp Suresh <dp@netscape.com>
 */

#include "xcDll.h"
#include "nsDebug.h"
#include "nsIComponentManager.h"
#include "nsIModule.h"
#include "nsIFileSpec.h"
#include "nsCOMPtr.h"

nsDll::nsDll(const char *codeDllName, int type)
  : m_dllName(NULL), m_dllSpec(NULL), m_modDate(0), m_size(0),
    m_instance(NULL), m_status(DLL_OK), m_moduleObject(NULL),
    m_persistentDescriptor(NULL), m_nativePath(NULL), m_markForUnload(PR_FALSE)
{
    if (!codeDllName || !*codeDllName)
    {
        m_status = DLL_INVALID_PARAM;
        return;
    }
    m_dllName = nsCRT::strdup(codeDllName);
    if (!m_dllName)
    {
        m_status = DLL_NO_MEM;
        return;
    }
}

nsDll::nsDll(nsIFileSpec *dllSpec, const char *registryLocation)
  : m_dllName(NULL), m_dllSpec(dllSpec), m_modDate(0), m_size(0),
    m_instance(NULL), m_status(DLL_OK), m_moduleObject(NULL),
    m_persistentDescriptor(NULL), m_nativePath(NULL), m_markForUnload(PR_FALSE)

{
    m_registryLocation = nsCRT::strdup(registryLocation);
    Init(dllSpec);
}

nsDll::nsDll(const char *libPersistentDescriptor)
  : m_dllName(NULL), m_dllSpec(NULL), m_modDate(0), m_size(0),
    m_instance(NULL), m_status(DLL_OK), m_moduleObject(NULL),
    m_persistentDescriptor(NULL), m_nativePath(NULL), m_markForUnload(PR_FALSE)

{
    Init(libPersistentDescriptor);
}

nsDll::nsDll(const char *libPersistentDescriptor, PRUint32 modDate, PRUint32 fileSize)
  : m_dllName(NULL), m_dllSpec(NULL), m_modDate(0), m_size(0),
    m_instance(NULL), m_status(DLL_OK), m_moduleObject(NULL),
    m_persistentDescriptor(NULL), m_nativePath(NULL), m_markForUnload(PR_FALSE)

{
    Init(libPersistentDescriptor);

    // and overwrite the modData and fileSize
	m_modDate = modDate;
	m_size = fileSize;
}
 
void
nsDll::Init(nsIFileSpec *dllSpec)
{
    // Addref the m_dllSpec
    m_dllSpec = dllSpec;
    NS_ADDREF(m_dllSpec);

    // Make sure we are dealing with a file
    PRBool isFile = PR_FALSE;
    nsresult rv = m_dllSpec->IsFile(&isFile);
    if (NS_FAILED(rv))
    {
        m_status = DLL_INVALID_PARAM;
        return;
    }
#ifdef XP_UNIX
    /* on Unix, symlinks are fair game too; XXX move to nsFileSpec? */
    if (!isFile) {
#ifdef DEBUG_shaver
        char *pathName;
        m_dllSpec->GetNativePath(&pathName);
        fprintf(stderr, "%s is not a file ", pathName);
        nsAllocator::Free(pathName);
#endif
        if (NS_FAILED(m_dllSpec->IsSymlink(&isFile))) {
            m_status = DLL_INVALID_PARAM;
            return;
        }
#ifdef DEBUG_shaver
        fputs(isFile ? "but it's a symlink\n" : "and it's not a symlink\n",
              stderr);
#endif
    }
#endif /* XP_UNIX */

    if (isFile == PR_FALSE)
    {
      // Not a file. Cant work with it.
      m_status = DLL_NOT_FILE;
      return;
    }
    
    // Populate m_modDate and m_size
    if (NS_FAILED(m_dllSpec->GetModDate(&m_modDate)) ||
        NS_FAILED(m_dllSpec->GetFileSize(&m_size)))
    {
        m_status = DLL_INVALID_PARAM;
        return;
    }

	m_status = DLL_OK;			
}

void
nsDll::Init(const char *libPersistentDescriptor)
{
    nsresult rv;
    m_modDate = 0;
	m_size = 0;
	
	if (libPersistentDescriptor == NULL)
	{
		m_status = DLL_INVALID_PARAM;
		return;
	}

    // Create a FileSpec from the persistentDescriptor
    nsIFileSpec *dllSpec = NULL;
    rv = nsComponentManager::CreateInstance(NS_FILESPEC_PROGID, NULL, NS_GET_IID(nsIFileSpec), (void **) &dllSpec);
    if (NS_FAILED(rv))
    {
        m_status = DLL_INVALID_PARAM;
        return;
    }

    rv = dllSpec->SetPersistentDescriptorString((char *)libPersistentDescriptor);
    if (NS_FAILED(rv))
    {
        m_status = DLL_INVALID_PARAM;
        return;
    }

    Init(dllSpec);
    NS_RELEASE(dllSpec);
}


nsDll::~nsDll(void)
{
    Unload();
    if (m_dllSpec)
        NS_RELEASE(m_dllSpec);
    if (m_dllName)
        nsCRT::free(m_dllName);
    if (m_persistentDescriptor)
        nsCRT::free(m_persistentDescriptor);
    if (m_nativePath)
        nsCRT::free(m_nativePath);
    if (m_registryLocation)
        nsCRT::free(m_registryLocation);

}

const char *
nsDll::GetNativePath()
{
    if (m_dllName)
        return m_dllName;
    if (m_nativePath)
        return m_nativePath;
    m_dllSpec->GetNativePath(&m_nativePath);
    return m_nativePath;
}

const char *
nsDll::GetPersistentDescriptorString()
{
    if (m_dllName)
        return m_dllName;
    if (m_persistentDescriptor)
        return m_persistentDescriptor;
    m_dllSpec->GetPersistentDescriptorString(&m_persistentDescriptor);
    return m_persistentDescriptor;
}

PRBool
nsDll::HasChanged()
{
    if (m_dllName)
        return PR_FALSE;

    // If mod date has changed, then dll has changed
    PRBool modDateChanged = PR_FALSE;
    nsresult rv = m_dllSpec->ModDateChanged(m_modDate, &modDateChanged);
    if (NS_FAILED(rv) || modDateChanged == PR_TRUE)
      return PR_TRUE;

    // If size has changed, then dll has changed
    PRUint32 aSize = 0;
    rv = m_dllSpec->GetFileSize(&aSize);
    if (NS_FAILED(rv) || aSize != m_size)
      return PR_TRUE;

#ifdef DEBUG_shaver
    fprintf(stderr, "%s not changed (%d/%d\n)",
            GetNativePath(), m_modDate, m_size);
#endif
    return PR_FALSE;
}




PRBool nsDll::Load(void)
{
	if (m_status != DLL_OK)
	{
		return (PR_FALSE);
	}
	if (m_instance != NULL)
	{
		// Already loaded
		return (PR_TRUE);
	}

    if (m_dllName)
    {
        m_instance = PR_LoadLibrary(m_dllName);
    }
    else
    {
        char *nsprPath = NULL;
        nsresult rv = m_dllSpec->GetNSPRPath(&nsprPath);
        if (NS_FAILED(rv)) return PR_FALSE;

#ifdef	XP_MAC
        // NSPR path is / separated. This works for all NSPR functions
        // except Load Library. Translate to something NSPR can accepts.
        if (nsprPath[0] == '/')
        {
            // convert '/' to ':'
            int c;
            char* str = nsprPath;
            while ((c = *str++) != 0)
            {
                if (c == '/')
                  str[-1] = ':';
            }
            m_instance = PR_LoadLibrary(&nsprPath[1]);		// skip over initial slash
        }
        else
        {
            m_instance = PR_LoadLibrary(nsprPath);
        }
#else
        m_instance = PR_LoadLibrary(nsprPath);
#endif /* XP_MAC */
        nsCRT::free(nsprPath);
    }
    return ((m_instance == NULL) ? PR_FALSE : PR_TRUE);
}

PRBool nsDll::Unload(void)
{
	if (m_status != DLL_OK || m_instance == NULL)
		return (PR_FALSE);

    nsrefcnt refcnt;
    if (m_moduleObject)
    {
        NS_RELEASE2(m_moduleObject, refcnt);
        NS_ASSERTION(refcnt == 0, "Dll moduleObject refcount non zero");
    }

	PRStatus ret = PR_UnloadLibrary(m_instance);
	if (ret == PR_SUCCESS)
	{
		m_instance = NULL;
		return (PR_TRUE);
	}
	else
		return (PR_FALSE);
}

void * nsDll::FindSymbol(const char *symbol)
{
	if (symbol == NULL)
		return (NULL);
	
	// If not already loaded, load it now.
	if (Load() != PR_TRUE)
		return (NULL);

    return(PR_FindSymbol(m_instance, symbol));
}


// Component dll specific functions
nsresult nsDll::GetDllSpec(nsIFileSpec **fsobj)
{
    NS_ASSERTION(m_dllSpec, "m_dllSpec NULL");
    NS_ASSERTION(fsobj, "xcDll::GetModule : Null argument" );

    NS_ADDREF(m_dllSpec);
    *fsobj = m_dllSpec;
    return NS_OK;
}

nsresult nsDll::GetModule(nsISupports *servMgr, nsIModule **cobj)
{
    nsIComponentManager *compMgr;
    nsresult rv = NS_GetGlobalComponentManager(&compMgr);
    NS_ASSERTION(compMgr, "Global Component Manager is null" );
    if (NS_FAILED(rv)) return rv;

    NS_ASSERTION(cobj, "xcDll::GetModule : Null argument" );

    if (m_moduleObject)
    {
        NS_ADDREF(m_moduleObject);
        *cobj = m_moduleObject;
        return NS_OK;
    }

	// If not already loaded, load it now.
	if (Load() != PR_TRUE) return NS_ERROR_FAILURE;

    // We need a nsIFileSpec for location. If we dont
    // have one, create one.
    if (m_dllSpec == NULL && m_dllName)
    {
        // Create m_dllSpec from m_dllName
    }

    nsGetModuleProc proc =
      (nsGetModuleProc) FindSymbol(NS_GET_MODULE_SYMBOL);

    if (proc == NULL)
        return NS_ERROR_FACTORY_NOT_LOADED;

    rv = (*proc) (compMgr, m_dllSpec, &m_moduleObject);
    if (NS_SUCCEEDED(rv))
    {
        NS_ADDREF(m_moduleObject);
        *cobj = m_moduleObject;
    }
    return rv;
}
