/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
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

#include "nsIRegistry.h"
#include "nsIEnumerator.h"
#include "nsDirectoryService.h"
#include "NSReg.h"
#include "prmem.h"
#include "prlock.h"
#include "prlog.h"
#include "nsCRT.h"
#include "nsAllocator.h"

#include "nsCOMPtr.h"
#include "nsILocalFile.h"
#include "nsIServiceManager.h"
#include "nsTextFormatter.h"

/* extra locking for the paranoid */
/* #define EXTRA_THREADSAFE */
#ifndef EXTRA_THREADSAFE
#define PR_Lock(x)           (void)0
#define PR_Unlock(x)         (void)0
#endif

/*-------------------------------- nsRegistry ----------------------------------
| This class implements the nsIRegistry interface using the functions          |
| provided by libreg (as declared in mozilla/modules/libreg/include/NSReg.h).  |
|                                                                              |
| Since that interface is designed to match the libreg function, this class    |
| is implemented with each member function being a simple wrapper for the      |
| corresponding libreg function.                                               |
|                                                                              |
| #define EXTRA_THREADSAFE if you are worried about libreg thread safety.      |
| It should not be necessary, but I'll leave in the code for the paranoid.     |
------------------------------------------------------------------------------*/




struct nsRegistry : public nsIRegistry {
    // This class implements the nsISupports interface functions.
    NS_DECL_ISUPPORTS

    // This class implements the nsIRegistry interface functions.
    NS_DECL_NSIREGISTRY

    // ctor/dtor
    nsRegistry();
    virtual ~nsRegistry();

protected:
    HREG   mReg; // Registry handle.
#ifdef EXTRA_THREADSAFE
    PRLock *mregLock;	// libreg isn't threadsafe. Use locks to synchronize.
#endif
    char *mCurRegFile;	// these are to prevent open from opening the registry again
    nsWellKnownRegistry mCurRegID;

    NS_IMETHOD Close();
}; // nsRegistry


#include "nsIFactory.h"
/*----------------------------- nsRegistryFactory ------------------------------
| Class factory for nsRegistry objects.                                        |
------------------------------------------------------------------------------*/
struct nsRegistryFactory : public nsIFactory {
    // This class implements the nsISupports interface functions.
    NS_DECL_ISUPPORTS

    // nsIFactory methods
    NS_IMETHOD CreateInstance(nsISupports *,const nsIID &,void **);
    NS_IMETHOD LockFactory(PRBool aLock);

    // ctor
    nsRegistryFactory();
};


/*--------------------------- nsRegSubtreeEnumerator ---------------------------
| This class implements the nsIEnumerator interface and is used to implement   |
| the nsRegistry EnumerateSubtrees and EnumerateAllSubtrees functions.         |
------------------------------------------------------------------------------*/
struct nsRegSubtreeEnumerator : public nsIEnumerator {
    // This class implements the nsISupports interface functions.
    NS_DECL_ISUPPORTS

    // This class implements the nsIEnumerator interface functions.
    NS_DECL_NSIENUMERATOR

    // ctor/dtor
    nsRegSubtreeEnumerator( HREG hReg, RKEY rKey, PRBool all );
    virtual ~nsRegSubtreeEnumerator();

protected:
    NS_IMETHOD advance(); // Implementation file; does appropriate NR_RegEnum call.
    HREG    mReg;   // Handle to registry we're affiliated with.
    RKEY    mKey;   // Base key being enumerated.
    char    mName[MAXREGPATHLEN]; // The name of the current key which is in mNext
    REGENUM mEnum;  // Corresponding libreg "enumerator".
    REGENUM mNext;  // Lookahead value.
    PRUint32  mStyle; // Style (indicates all or some);
    PRBool  mDone;  // Done flag.
#ifdef EXTRA_THREADSAFE
    PRLock *mregLock;
#endif
}; // nsRegSubtreeEnumerator


/*--------------------------- nsRegValueEnumerator -----------------------------
| This class is a variation on nsRegSubtreeEnumerator that allocates           |
| nsRegistryValue objects rather than nsRegistryNode objects.  It also         |
| overrides certain functions to make sure the "value" oriented libreg         |
| functions used rather than the subtree oriented ones.                        |
------------------------------------------------------------------------------*/
struct nsRegValueEnumerator : public nsRegSubtreeEnumerator {
    // Override CurrentItem to allocate nsRegistryValue objects.
    NS_IMETHOD CurrentItem( nsISupports **result );

    // Override advance() to use proper NR_RegEnumEntries.
    NS_IMETHOD advance();

    // ctor/dtor
    nsRegValueEnumerator( HREG hReg, RKEY rKey );
}; // nsRegValueEnumerator

/*------------------------------ nsRegistryNode --------------------------------
| This class implements the nsIRegistryNode interface.  Instances are         |
| allocated by nsRegSubtreeEnumerator::CurrentItem.                           |
------------------------------------------------------------------------------*/
struct nsRegistryNode : public nsIRegistryNode {
    // This class implements the nsISupports interface functions.
    NS_DECL_ISUPPORTS

    // This class implements the nsIRegistryNode interface functions.
    NS_DECL_NSIREGISTRYNODE

    // ctor
    nsRegistryNode( HREG hReg, char *name, RKEY childKey );
    virtual ~nsRegistryNode();
    
protected:
    HREG    mReg;  // Handle to registry this node is part of.
    char    mName[MAXREGPATHLEN]; // Buffer to hold name.
    RKEY    mChildKey;	// Key corresponding to mName
#ifdef EXTRA_THREADSAFE
    PRLock *mregLock;
#endif
}; // nsRegistryNode


/*------------------------------ nsRegistryValue -------------------------------
| This class implements the nsIRegistryValue interface.  Instances are         |
| allocated by nsRegValueEnumerator::CurrentItem.                              |
------------------------------------------------------------------------------*/
struct nsRegistryValue : public nsIRegistryValue {
    // This class implements the nsISupports interface functions.
    NS_DECL_ISUPPORTS

    // This class implements the nsIRegistryValue interface functions.
    NS_DECL_NSIREGISTRYVALUE

    // ctor
    nsRegistryValue( HREG hReg, RKEY key, REGENUM slot );
    virtual ~nsRegistryValue();

protected:
    nsresult getInfo(); // Get registry info.
    HREG    mReg;  // Handle to registry this node is part of.
    RKEY    mKey;  // Key this node is under.
    REGENUM mEnum; // Copy of corresponding content of parent enumerator.
    REGINFO mInfo; // Value info.
    char    mName[MAXREGNAMELEN]; // Buffer to hold name.
    REGERR  mErr; // XXX This causes this class to be NON THREAD SAFE
#ifdef EXTRA_THREADSAFE
    PRLock *mregLock;
#endif
}; // nsRegistryValue


/*----------------------------- regerr2nsresult --------------------------------
| This utility function maps a REGERR value to a corresponding nsresult        |
| error code.                                                                  |
------------------------------------------------------------------------------*/
static nsresult regerr2nsresult( REGERR err ) {
    nsresult rv = NS_ERROR_UNEXPECTED;
    switch( err ) {
        case REGERR_OK:
            rv = NS_OK;
            break;

        case REGERR_FAIL:
            rv = NS_ERROR_FAILURE;
            break;

        case REGERR_NOMORE:
            rv = NS_ERROR_REG_NO_MORE;
            break;
    
        case REGERR_NOFIND:
            rv = NS_ERROR_REG_NOT_FOUND;
            break;
    
        case REGERR_PARAM:
        case REGERR_BADTYPE:
        case REGERR_BADNAME:
            rv = NS_ERROR_INVALID_ARG;
            break;
    
        case REGERR_NOFILE:
            rv = NS_ERROR_REG_NOFILE;
            break;
    
        case REGERR_MEMORY:
            rv = NS_ERROR_OUT_OF_MEMORY;
            break;
    
        case REGERR_BUFTOOSMALL:
            rv = NS_ERROR_REG_BUFFER_TOO_SMALL;
            break;
    
        case REGERR_NAMETOOLONG:
            rv = NS_ERROR_REG_NAME_TOO_LONG;
            break;
    
        case REGERR_NOPATH:
            rv = NS_ERROR_REG_NO_PATH;
            break;
    
        case REGERR_READONLY:
            rv = NS_ERROR_REG_READ_ONLY;
            break;
    
        case REGERR_BADUTF8:
            rv = NS_ERROR_REG_BAD_UTF8;
            break;
    
    }
    return rv;
}

/*----------------------------- reginfo2DataType -------------------------------
| This utility function converts the type field in the REGINFO structure to    |
| the corresponding nsIRegistry::DataType value.                              |
------------------------------------------------------------------------------*/
static void reginfo2DataType( const REGINFO &in, PRUint32 &out ) {
    // Transfer information, based on entry type.
    switch( in.entryType ) {
        case REGTYPE_ENTRY_STRING_UTF:
            out = nsIRegistry::String;
            //out.length = in.entryLength;
            break;

        case REGTYPE_ENTRY_INT32_ARRAY:
            out = nsIRegistry::Int32;
            // Convert length in bytes to array dimension.
            //out.length = in.entryLength / sizeof(PRInt32);
            break;

        case REGTYPE_ENTRY_BYTES:
            out = nsIRegistry::Bytes;
            //out.length = in.entryLength;
            break;

        case REGTYPE_ENTRY_FILE:
            out = nsIRegistry::File;
            //out.length = in.entryLength;
            break;
    }
}

/*----------------------------- reginfo2DataType -------------------------------
| This utility function converts the length field in the REGINFO structure to  |
| the proper units (if type==Int32 array, we divide by sizeof(PRInt32)).         |
------------------------------------------------------------------------------*/
static void reginfo2Length( const REGINFO &in, PRUint32 &out ) {
    // Transfer information, based on entry type.
    switch( in.entryType ) {
        case REGTYPE_ENTRY_STRING_UTF:
            out = in.entryLength;
            break;

        case REGTYPE_ENTRY_INT32_ARRAY:
            // Convert length in bytes to array dimension.
            out = in.entryLength / sizeof(PRInt32);
            break;

        case REGTYPE_ENTRY_BYTES:
            out = in.entryLength;
            break;

        case REGTYPE_ENTRY_FILE:
            out = in.entryLength;
            break;
    }
}

/*------------------------ nsISupports Implementation --------------------------
| This code generates the implementation of the nsISupports member functions   |
| for each class implemented in this file.                                     |
------------------------------------------------------------------------------*/
NS_IMPL_ISUPPORTS1( nsRegistry,             nsIRegistry      )
NS_IMPL_ISUPPORTS1( nsRegSubtreeEnumerator, nsIEnumerator    )
NS_IMPL_ISUPPORTS1( nsRegistryNode,         nsIRegistryNode  )
NS_IMPL_ISUPPORTS1( nsRegistryValue,        nsIRegistryValue )

/*-------------------------- nsRegistry::nsRegistry ----------------------------
| Vanilla nsRegistry constructor.                                              |
------------------------------------------------------------------------------*/
nsRegistry::nsRegistry() 
    : mReg(0), mCurRegFile(nsnull), mCurRegID(0) {
    NS_INIT_REFCNT();
#ifdef EXTRA_THREADSAFE
    mregLock = PR_NewLock();
#endif
    return;
}

/*------------------------- nsRegistry::~nsRegistry ----------------------------
| The dtor closes the registry file(if open).                                  |
------------------------------------------------------------------------------*/
nsRegistry::~nsRegistry() {
    if( mReg ) {
        Close();
    }
    if (mCurRegFile)
      nsCRT::free(mCurRegFile);
#ifdef EXTRA_THREADSAFE
    if (mregLock) {
        PR_DestroyLock(mregLock);
    }
#endif
    return;
}

/*----------------------------- nsRegistry::Open -------------------------------
| If the argument is null, delegate to OpenDefault, else open the registry     |
| file.  We first check to see if a registry file is already open and close    |
| it if so.                                                                    |
------------------------------------------------------------------------------*/
NS_IMETHODIMP nsRegistry::Open( const char *regFile ) {
    REGERR err = REGERR_OK;

    // Check for default.
    if( !regFile ) {
        return OpenWellKnownRegistry(nsIRegistry::ApplicationRegistry);
    }

#ifdef DEBUG_dp
    printf("nsRegistry: Opening registry %s\n", regFile);
#endif /* DEBUG_dp */
   
    if (mCurRegID != nsIRegistry::None && mCurRegID != nsIRegistry::ApplicationCustomRegistry)
    {
        // Cant open another registry without closing explictly.
        return NS_ERROR_INVALID_ARG;
    }

    // Do we have an open registry ?
    if (mCurRegID != nsIRegistry::None)
    {
        if (mCurRegFile && !nsCRT::strcmp(regFile, mCurRegFile))
        {
            // The right one is already open
            return NS_OK;
        }
        else
        {
            // Opening a new registry without closing an already open one.
            // This is an error.
            return NS_ERROR_FAILURE;
        }
    }

    // Open specified registry.
    PR_Lock(mregLock);
    err = NR_RegOpen((char*)regFile, &mReg );
    PR_Unlock(mregLock);

    mCurRegID = nsIRegistry::ApplicationCustomRegistry;
    // No error checking for no mem. Trust me.
    mCurRegFile = nsCRT::strdup(regFile);

    // Convert the result.
    return regerr2nsresult( err );
}

/*----------------------------- nsRegistry::OpenWellKnownRegistry --------------
| Takes a registry id and maps that to a file name for opening. We first check |
| to see if a registry file is already open and close  it if so.               |
------------------------------------------------------------------------------*/
NS_IMETHODIMP nsRegistry::OpenWellKnownRegistry( nsWellKnownRegistry regid ) {
    REGERR err = REGERR_OK;

    if (mCurRegID != nsIRegistry::None && mCurRegID != regid)
    {
        // Cant open another registry without closing explictly.
        return NS_ERROR_INVALID_ARG;
    }

    if (mCurRegID == regid)
    {
        // Already opened.
        return NS_OK;
    }

    nsresult rv;
    nsCOMPtr<nsIFile> registryLocation;

    PRBool foundReg = PR_FALSE;
    char *regFile = nsnull;
    
    switch ( (nsWellKnownRegistry) regid ) {
      case ApplicationComponentRegistry:
        {
            nsCOMPtr<nsIProperties> directoryService;
            rv = nsDirectoryService::Create(nsnull, 
                                            NS_GET_IID(nsIProperties), 
                                            getter_AddRefs(directoryService));
            if (NS_FAILED(rv)) return rv;
            directoryService->Get("xpcom.currentProcess.componentRegistry", NS_GET_IID(nsIFile), 
                                          getter_AddRefs(registryLocation));

            if (registryLocation != nsnull)
            {
                foundReg = PR_TRUE;
                registryLocation->GetPath(&regFile);  // dougt fix...
                // dveditz needs to fix his registry so that I can pass an
                // nsIFile interface and not hack 
                if (!regFile)
                  return NS_ERROR_OUT_OF_MEMORY;
            }
            
        }
        break;
      case ApplicationRegistry:
        {
            foundReg = PR_TRUE;
            // NULL regFile will open the right one for this case.
        }
        break;

      default:
        break;
    }

    if (foundReg == PR_FALSE) {
        return NS_ERROR_REG_BADTYPE;
    }
   
#ifdef DEBUG_dp
    printf("nsRegistry: Opening std registry %s\n", regFile);
#endif /* DEBUG_dp */

    PR_Lock(mregLock);
    err = NR_RegOpen((char*)regFile, &mReg );
    PR_Unlock(mregLock);

    if (regFile)
      nsAllocator::Free(regFile);

    // Store the registry that was opened for optimizing future opens.
    mCurRegID = regid;

    // Convert the result.
    return regerr2nsresult( err );
}

#if 0
/*-------------------------- nsRegistry::OpenDefault ---------------------------
| Open the "default" registry; in the case of this libreg-based implementation |
| that is done by passing a null file name pointer to NR_RegOpen.              |
------------------------------------------------------------------------------*/
NS_IMETHODIMP nsRegistry::OpenDefault() {
    return OpenWellKnownRegistry(nsIRegistry::ApplicationRegistry);
}
#endif

/*----------------------------- nsRegistry::Close ------------------------------
| Tests the mReg handle and if non-null, closes the registry via NR_RegClose.  |
------------------------------------------------------------------------------*/
NS_IMETHODIMP nsRegistry::Close() {
    REGERR err = REGERR_OK;
    if( mReg ) {
        PR_Lock(mregLock);
        err = NR_RegClose( mReg );
        PR_Unlock(mregLock);
        mReg = 0;
        if (mCurRegFile)
          nsCRT::free(mCurRegFile);
        mCurRegFile = nsnull;
        mCurRegID = 0;
    }
    return regerr2nsresult( err );
}

/*----------------------------- nsRegistry::Flush ------------------------------
| Flushes the registry via NR_RegFlush.                                        |
------------------------------------------------------------------------------*/
NS_IMETHODIMP nsRegistry::Flush() {
    REGERR err = REGERR_FAIL;
    if( mReg ) {
        PR_Lock(mregLock);
        err = NR_RegFlush( mReg );
        PR_Unlock(mregLock);
    }
    return regerr2nsresult( err );
}

/*----------------------------- nsRegistry::IsOpen -----------------------------
| Tests the mReg handle and returns whether the registry is open or not.       |
------------------------------------------------------------------------------*/
NS_IMETHODIMP nsRegistry::IsOpen( PRBool *result ) {
    *result = ( mReg != 0 );
    return NS_OK;
}


/*--------------------------- nsRegistry::AddKey -------------------------------
| Add a key into the registry or find an existing one.  This is generally used |
| instead of GetKey unless it's an error for the key not to exist already      i
------------------------------------------------------------------------------*/
NS_IMETHODIMP nsRegistry::AddKey( nsRegistryKey baseKey, const PRUnichar *keyname, nsRegistryKey *_retval)
{
    if ( !keyname ) 
        return NS_ERROR_NULL_POINTER;

    nsString name( nsSubsumeStr( NS_CONST_CAST(PRUnichar*,keyname), PR_FALSE ) );
    char* utf8name = name.ToNewUTF8String();
    if ( !utf8name )
        return NS_ERROR_OUT_OF_MEMORY;

    nsresult rv = AddSubtree( baseKey, utf8name, _retval );

    Recycle( utf8name );
    return rv;
}

/*--------------------------- nsRegistry::GetKey -------------------------------
| returns the nsRegistryKey associated with a given node in the registry       |
------------------------------------------------------------------------------*/
NS_IMETHODIMP nsRegistry::GetKey(nsRegistryKey baseKey, const PRUnichar *keyname, nsRegistryKey *_retval)
{
    if ( !keyname || !_retval ) 
        return NS_ERROR_NULL_POINTER;

    nsString name( nsSubsumeStr( NS_CONST_CAST(PRUnichar*,keyname), PR_FALSE ) );
    char* utf8name = name.ToNewUTF8String();
    if ( !utf8name )
        return NS_ERROR_OUT_OF_MEMORY;

    nsresult rv = GetSubtree( baseKey, utf8name, _retval );

    Recycle( utf8name );
    return rv;
}

/*--------------------------- nsRegistry::RemoveKey ----------------------------
| Delete a key from the registry                                               |
------------------------------------------------------------------------------*/
NS_IMETHODIMP nsRegistry::RemoveKey(nsRegistryKey baseKey, const PRUnichar *keyname)
{
    if ( !keyname ) 
        return NS_ERROR_NULL_POINTER;

    nsString name( nsSubsumeStr( NS_CONST_CAST(PRUnichar*,keyname), PR_FALSE ) );
    char* utf8name = name.ToNewUTF8String();
    if ( !utf8name )
        return NS_ERROR_OUT_OF_MEMORY;

    nsresult rv = RemoveSubtree( baseKey, utf8name );

    Recycle( utf8name );
    return rv;
}

NS_IMETHODIMP nsRegistry::GetString(nsRegistryKey baseKey, const PRUnichar *valname, PRUnichar **_retval)
{
    nsresult rv = NS_OK;

    // Make sure caller gave us place for result.
    if ( !valname || !_retval )
        return NS_ERROR_NULL_POINTER;

    // initialize the return value
    *_retval = nsnull;
    char *tmpstr = nsnull;

    nsString name( nsSubsumeStr( NS_CONST_CAST(PRUnichar*,valname), PR_FALSE ) );
    char* utf8name = name.ToNewUTF8String();
    if ( !utf8name )
        return NS_ERROR_OUT_OF_MEMORY;

    rv = GetStringUTF8( baseKey, utf8name, &tmpstr );
    if (NS_SUCCEEDED(rv))
    {
        nsString tmpfmt("%s");
        *_retval = nsTextFormatter::smprintf( tmpfmt.GetUnicode(), tmpstr );
        nsCRT::free(tmpstr);
        if ( *_retval == nsnull )
            rv = NS_ERROR_OUT_OF_MEMORY;
    }

    Recycle( utf8name );
    return rv;
}

NS_IMETHODIMP nsRegistry::SetString(nsRegistryKey baseKey, const PRUnichar *valname, const PRUnichar *value)
{
    if ( !valname || ! value )
        return NS_ERROR_NULL_POINTER;

    nsresult rv = NS_OK;

    nsString name( nsSubsumeStr( NS_CONST_CAST(PRUnichar*,valname), PR_FALSE ) );
    nsString val( nsSubsumeStr( NS_CONST_CAST(PRUnichar*,value), PR_FALSE ) );
    char* utf8name = name.ToNewUTF8String();
    char* utf8val  = val.ToNewUTF8String();

    if ( utf8name && utf8val )
        rv = SetStringUTF8( baseKey, utf8name, utf8val );
    else
        rv = NS_ERROR_OUT_OF_MEMORY;

    if ( utf8name ) Recycle(utf8name);
    if ( utf8val ) Recycle(utf8val);

    return rv;
}

/*--------------------------- nsRegistry::GetString ----------------------------
| First, look for the entry using GetValueInfo.  If found, and it's a string,  |
| allocate space for it and fetch the value.                                   |
------------------------------------------------------------------------------*/
NS_IMETHODIMP nsRegistry::GetStringUTF8( nsRegistryKey baseKey, const char *path, char **result ) {
    nsresult rv = NS_OK;
    REGERR   err = REGERR_OK;

    // Make sure caller gave us place for result.
    if ( !result )
        return NS_ERROR_NULL_POINTER;

    char   regStr[MAXREGPATHLEN];

    // initialize the return value
    *result = 0;

    // Attempt to get string into our fixed buffer
    PR_Lock(mregLock);
    err = NR_RegGetEntryString( mReg,(RKEY)baseKey,(char*)path, regStr, sizeof regStr );
    PR_Unlock(mregLock);

    if ( err == REGERR_OK )
    {
        *result = nsCRT::strdup(regStr);
        if (!*result)
            rv = NS_ERROR_OUT_OF_MEMORY;
    }
    else if ( err == REGERR_BUFTOOSMALL ) 
    {
        // find the real size and malloc it
        PRUint32 length;
        rv = GetValueLength( baseKey, path, &length );
        // See if that worked.
        if( rv == NS_OK ) 
        {
            *result =(char*)nsAllocator::Alloc( length + 1 );
            if( *result ) 
            {
                // Get string from registry into result buffer.
                PR_Lock(mregLock);
                err = NR_RegGetEntryString( mReg,(RKEY)baseKey,(char*)path, *result, length+1 );
                PR_Unlock(mregLock);

                // Convert status.
                rv = regerr2nsresult( err );
                if ( rv != NS_OK )
                {
                    // Didn't get result, free buffer
                    nsCRT::free( *result );
                    *result = 0;
                }
            }
            else
            {
                rv = NS_ERROR_OUT_OF_MEMORY;
            }
        }
    }
    else
    {
        // Convert status.
        rv = regerr2nsresult( err );
    }

   return rv;
}

/*--------------------------- nsRegistry::SetString ----------------------------
| Simply sets the registry contents using NR_RegSetEntryString.                |
------------------------------------------------------------------------------*/
NS_IMETHODIMP nsRegistry::SetStringUTF8( nsRegistryKey baseKey, const char *path, const char *value ) {
    REGERR err = REGERR_OK;
    // Set the contents.
    PR_Lock(mregLock);
    err = NR_RegSetEntryString( mReg,(RKEY)baseKey,(char*)path,(char*)value );
    PR_Unlock(mregLock);
    // Convert result.
    return regerr2nsresult( err );
}

/*---------------------------- nsRegistry::GetInt ------------------------------
| This function is just shorthand for fetching a 1-element PRInt32 array.  We  |
| implement it "manually" using NR_RegGetEntry                                 |
------------------------------------------------------------------------------*/
NS_IMETHODIMP nsRegistry::GetInt( nsRegistryKey baseKey, const char *path, PRInt32 *result ) {
    nsresult rv = NS_OK;
    REGERR err = REGERR_OK;

    // Make sure caller gave us place for result.
    if( result ) {
        // Get info about the requested entry.
        PRUint32 type;
        rv = GetValueType( baseKey, path, &type );
        // See if that worked.
        if( rv == NS_OK ) {
            // Make sure the entry is an PRInt32 array.
            if( type == Int32 ) {
                uint32 len = sizeof *result;
                // Get int from registry into result field.
                PR_Lock(mregLock);
                err = NR_RegGetEntry( mReg,(RKEY)baseKey,(char*)path, result, &len );
                PR_Unlock(mregLock);
                // Convert status.
                rv = regerr2nsresult( err );
            } else {
                // They asked for the wrong type of value.
                rv = NS_ERROR_REG_BADTYPE;
            }
        }
    } else {
        rv = NS_ERROR_NULL_POINTER;
    }
    return rv;
}


/*---------------------------- nsRegistry::GetLongLong--------------------------
| This function is just shorthand for fetching a 1-element PRInt64 array.  We  |
| implement it "manually" using NR_RegGetEntry                                 |
------------------------------------------------------------------------------*/
NS_IMETHODIMP nsRegistry::GetLongLong( nsRegistryKey baseKey, const char *path, PRInt64 *result ) {
    REGERR err = REGERR_OK;
    
    PR_Lock(mregLock);
    
    uint32 length = sizeof(PRInt64);
    err = NR_RegGetEntry( mReg,(RKEY)baseKey,(char*)path,(void*)result,&length);
    
    PR_Unlock(mregLock);
    
    // Convert status.
    return regerr2nsresult( err );
}
/*---------------------------- nsRegistry::SetInt ------------------------------
| Write out the value as a one-element PRInt32 array, using NR_RegSetEntry.      |
------------------------------------------------------------------------------*/
NS_IMETHODIMP nsRegistry::SetInt( nsRegistryKey baseKey, const char *path, PRInt32 value ) {
    REGERR err = REGERR_OK;
    // Set the contents.
    PR_Lock(mregLock);
    err = NR_RegSetEntry( mReg,
                (RKEY)baseKey,
                (char*)path,
                           REGTYPE_ENTRY_INT32_ARRAY,
                           &value,
                           sizeof value );
    PR_Unlock(mregLock);
    // Convert result.
    return regerr2nsresult( err );
}



/*---------------------------- nsRegistry::SetLongLong---------------------------
| Write out the value as a one-element PRInt64 array, using NR_RegSetEntry.      |
------------------------------------------------------------------------------*/
NS_IMETHODIMP nsRegistry::SetLongLong( nsRegistryKey baseKey, const char *path, PRInt64* value ) {
    REGERR err = REGERR_OK;
    // Set the contents.
    PR_Lock(mregLock);

    err = NR_RegSetEntry( mReg,
                        (RKEY)baseKey,
                        (char*)path,
                        REGTYPE_ENTRY_BYTES,
                        (void*)value,
                        sizeof(PRInt64) );

    PR_Unlock(mregLock);
    // Convert result.
    return regerr2nsresult( err );
}

/*-------------------------- nsRegistry::AddSubtree ----------------------------
| Add a new registry subkey with the specified name, using NR_RegAddKey.       |
------------------------------------------------------------------------------*/
NS_IMETHODIMP nsRegistry::AddSubtree( nsRegistryKey baseKey, const char *path, nsRegistryKey *result ) {
    REGERR err = REGERR_OK;
    // Add the subkey.
    PR_Lock(mregLock);
    err = NR_RegAddKey( mReg,(RKEY)baseKey,(char*)path,(RKEY*)result );
    PR_Unlock(mregLock);
    // Convert result.
    return regerr2nsresult( err );
}

/*-------------------------- nsRegistry::AddSubtreeRaw--------------------------
| Add a new registry subkey with the specified name, using NR_RegAddKeyRaw     |
------------------------------------------------------------------------------*/
NS_IMETHODIMP nsRegistry::AddSubtreeRaw( nsRegistryKey baseKey, const char *path, nsRegistryKey *result ) {
    REGERR err = REGERR_OK;
    // Add the subkey.
    PR_Lock(mregLock);
    err = NR_RegAddKeyRaw( mReg,(RKEY)baseKey,(char*)path,(RKEY*)result );
    PR_Unlock(mregLock);
    // Convert result.
    return regerr2nsresult( err );
}


/*------------------------- nsRegistry::RemoveSubtree --------------------------
| Deletes the subtree at a given location using NR_RegDeleteKey.               |
------------------------------------------------------------------------------*/
NS_IMETHODIMP nsRegistry::RemoveSubtree( nsRegistryKey baseKey, const char *path ) {
    REGERR err = REGERR_OK;
    // Delete the subkey.
    PR_Lock(mregLock);
    err = NR_RegDeleteKey( mReg,(RKEY)baseKey,(char*)path );
    PR_Unlock(mregLock);
    // Convert result.
    return regerr2nsresult( err );
}


/*------------------------- nsRegistry::RemoveSubtreeRaw -----------------------
| Deletes the subtree at a given location using NR_RegDeleteKeyRaw             |
------------------------------------------------------------------------------*/
NS_IMETHODIMP nsRegistry::RemoveSubtreeRaw( nsRegistryKey baseKey, const char *keyname ) {
    nsresult rv = NS_OK;
    REGERR err = REGERR_OK;

	// libreg doesn't delete keys if there are subkeys under the key
	// Hence we have to recurse through to delete the subtree

    RKEY key;
    char subkeyname[MAXREGPATHLEN+1];
    int n = sizeof(subkeyname);
    REGENUM state = 0;

    PR_Lock(mregLock);
    err = NR_RegGetKeyRaw(mReg, baseKey, (char *)keyname, &key);
    PR_Unlock(mregLock);
    if (err != REGERR_OK)
	{
		rv = regerr2nsresult( err );
        return rv;
	}

    // Now recurse through and delete all keys under hierarchy
	
    subkeyname[0] = '\0';
    while (NR_RegEnumSubkeys(mReg, key, &state, subkeyname, n, REGENUM_NORMAL) == REGERR_OK)
    {
#ifdef DEBUG_dp
		printf("...recursing into %s\n", subkeyname);
#endif /* DEBUG_dp */
        err = RemoveSubtreeRaw(key, subkeyname);
        if (err != REGERR_OK) break;
    }

    // If success in deleting all subkeys, delete this key too
    if (err == REGERR_OK)
    {
#ifdef DEBUG_dp
		printf("...deleting %s\n", keyname);
#endif /* DEBUG_dp */
        PR_Lock(mregLock);
        err = NR_RegDeleteKeyRaw(mReg, baseKey, (char *)keyname);
        PR_Unlock(mregLock);
    }

	// Convert result.
  	rv = regerr2nsresult( err );
    return rv;
}
/*-------------------------- nsRegistry::GetSubtree ----------------------------
| Returns a nsRegistryKey(RKEY) for a given key/path.  The key is           |
| obtained using NR_RegGetKey.                                                 |
------------------------------------------------------------------------------*/
NS_IMETHODIMP nsRegistry::GetSubtree( nsRegistryKey baseKey, const char *path, nsRegistryKey *result ) {
    nsresult rv = NS_OK;
    REGERR err = REGERR_OK;
    // Make sure we have a place for the result.
    if( result ) {
        // Get key.
        PR_Lock(mregLock);
        err = NR_RegGetKey( mReg,(RKEY)baseKey,(char*)path,(RKEY*)result );
        PR_Unlock(mregLock);
        // Convert result.
        rv = regerr2nsresult( err );
    } else {
        rv = NS_ERROR_NULL_POINTER;
    }
    return rv;
}

/*-------------------------- nsRegistry::GetSubtreeRaw--------------------------
| Returns a nsRegistryKey(RKEY) for a given key/path.  The key is           |
| obtained using NR_RegGetKeyRaw.                                              |
------------------------------------------------------------------------------*/
NS_IMETHODIMP nsRegistry::GetSubtreeRaw( nsRegistryKey baseKey, const char *path, nsRegistryKey *result ) {
    nsresult rv = NS_OK;
    REGERR err = REGERR_OK;
    // Make sure we have a place for the result.
    if( result ) {
        // Get key.
        PR_Lock(mregLock);
        err = NR_RegGetKeyRaw( mReg,(RKEY)baseKey,(char*)path,(RKEY*)result );
        PR_Unlock(mregLock);
        // Convert result.
        rv = regerr2nsresult( err );
    } else {
        rv = NS_ERROR_NULL_POINTER;
    }
    return rv;
}


/*----------------------- nsRegistry::EnumerateSubtrees ------------------------
| Allocate a nsRegSubtreeEnumerator object and return it to the caller.        |
| We construct the enumerator using the registry handle from this registry     |
| object, the user-specified registry key, and indicate that we don't want     |
| to recurse down subtrees.  No libreg functions are invoked at this point     |
|(that will happen when the enumerator member functions are called).          |
------------------------------------------------------------------------------*/
NS_IMETHODIMP nsRegistry::EnumerateSubtrees( nsRegistryKey baseKey, nsIEnumerator **result ) {
    nsresult rv = NS_OK;
    // Make sure we have a place to put the result.
    if( result ) {
        *result = new nsRegSubtreeEnumerator( mReg,(RKEY)baseKey, PR_FALSE );
        // Check for success.
        if( *result ) {
            // Bump refcnt on behalf of caller.
          NS_ADDREF(*result);
        } else {
            // Unable to allocate space for the enumerator object.
            rv = NS_ERROR_OUT_OF_MEMORY;
        }
    } else {
        rv = NS_ERROR_NULL_POINTER;
    }
    return rv;
}

/*--------------------- nsRegistry::EnumerateAllSubtrees -----------------------
| Same as EnumerateSubtrees but we pass PR_TRUE to request that the            |
| enumerator object descend subtrees when it is used.                          |
------------------------------------------------------------------------------*/
NS_IMETHODIMP nsRegistry::EnumerateAllSubtrees( nsRegistryKey baseKey, nsIEnumerator **result ) {
    nsresult rv = NS_OK;
    // Make sure we have a place to put the result.
    if( result ) {
        *result = new nsRegSubtreeEnumerator( mReg,(RKEY)baseKey, PR_TRUE );
        // Check for success.
        if( *result ) {
            // Bump refcnt on behalf of caller.
          NS_ADDREF(*result);
        } else {
            // Unable to allocate space for the enumerator object.
            rv = NS_ERROR_OUT_OF_MEMORY;
        }
    } else {
        rv = NS_ERROR_NULL_POINTER;
    }
    return rv;
}

/*------------------------- nsRegistry::GetValueType ---------------------------
| Gets the type from the registry using the NR_GetEntryInfo libreg API.        |
| The result is transferred to the PRUint32 value passed in (with conversion     |
| to the appropriate nsIRegistry::DataType value).                             |
------------------------------------------------------------------------------*/
NS_IMETHODIMP nsRegistry::GetValueType( nsRegistryKey baseKey, const char *path, PRUint32 *result ) {
    nsresult rv = NS_OK;
    REGERR err = REGERR_OK;
    // Make sure we have a place to put the result.
    if( result ) {
        // Get registry info into local structure.
        REGINFO info = { sizeof info, 0, 0 };
        PR_Lock(mregLock);
        err = NR_RegGetEntryInfo( mReg,(RKEY)baseKey,(char*)path, &info );
        PR_Unlock(mregLock);
        if( err == REGERR_OK ) {
            // Copy info to user's result value.
            reginfo2DataType( info, *result );
        } else {
            rv = regerr2nsresult( err );
        }
    } else {
        rv = NS_ERROR_NULL_POINTER;
    }
    return rv;
}

/*------------------------ nsRegistry::GetValueLength --------------------------
| Gets the registry value info via NR_RegGetEntryInfo.  The length is          |
| converted to the proper "units" via reginfo2Length.                          |
------------------------------------------------------------------------------*/
NS_IMETHODIMP nsRegistry::GetValueLength( nsRegistryKey baseKey, const char *path, PRUint32 *result ) {
    nsresult rv = NS_OK;
    REGERR err = REGERR_OK;
    // Make sure we have a place to put the result.
    if( result ) {
        // Get registry info into local structure.
        REGINFO info = { sizeof info, 0, 0 };
        PR_Lock(mregLock);
        err = NR_RegGetEntryInfo( mReg,(RKEY)baseKey,(char*)path, &info );
        PR_Unlock(mregLock);
        if( err == REGERR_OK ) {
            // Copy info to user's result value.
            reginfo2Length( info, *result );
        } else {
            rv = regerr2nsresult( err );
        }
    } else {
        rv = NS_ERROR_NULL_POINTER;
    }
    return rv;
}

/*-------------------------- nsRegistry::DeleteValue ---------------------------
| Remove the registry value with the specified name                            |
------------------------------------------------------------------------------*/
NS_IMETHODIMP nsRegistry::DeleteValue( nsRegistryKey baseKey, const char *path)
{
    REGERR err = REGERR_OK;
    // Delete the value
    PR_Lock(mregLock);
    err = NR_RegDeleteEntry( mReg,(RKEY)baseKey,(char*)path );
    PR_Unlock(mregLock);
    // Convert result.
    return regerr2nsresult( err );
}

/*------------------------ nsRegistry::EnumerateValues -------------------------
| Allocates and returns an instance of nsRegValueEnumerator constructed in     |
| a similar fashion as the nsRegSubtreeEnumerator is allocated/returned by     |
| EnumerateSubtrees.                                                           |
------------------------------------------------------------------------------*/
NS_IMETHODIMP nsRegistry::EnumerateValues( nsRegistryKey baseKey, nsIEnumerator **result ) {
    nsresult rv = NS_OK;
    // Make sure we have a place to put the result.
    if( result ) {
        *result = new nsRegValueEnumerator( mReg,(RKEY)baseKey );
        // Check for success.
        if( *result ) {
            // Bump refcnt on behalf of caller.
            NS_ADDREF(*result);
        } else {
            // Unable to allocate space for the enumerator object.
            rv = NS_ERROR_OUT_OF_MEMORY;
        }
    } else {
        rv = NS_ERROR_NULL_POINTER;
    }
    return rv;
}

/*---------------------- nsRegistry::GetCurrentUserName ------------------------
| Simple wrapper for NR_RegGetUsername.                                        |
------------------------------------------------------------------------------*/
NS_IMETHODIMP nsRegistry::GetCurrentUserName( char **result ) {
    nsresult rv = NS_OK;
    REGERR err = REGERR_OK;
    // Make sure we have a place to put the result.
    if( result ) {
        // Get the user name.
        PR_Lock(mregLock);
        err = NR_RegGetUsername( result );
        PR_Unlock(mregLock);
        // Convert the result.
        rv = regerr2nsresult( err );
    } else {
        rv = NS_ERROR_NULL_POINTER;
    }
    return rv;
}

/*---------------------- nsRegistry::SetCurrentUserName ------------------------
| Simple wrapper for NR_RegSetUsername.                                        |
------------------------------------------------------------------------------*/
NS_IMETHODIMP nsRegistry::SetCurrentUserName( const char *name ) {
    nsresult rv = NS_OK;
    REGERR err = REGERR_OK;
    // Set the user name.
    PR_Lock(mregLock);
    err = NR_RegSetUsername( name );
    PR_Unlock(mregLock);
    // Convert result.
    rv = regerr2nsresult( err );
    return rv;
}

/*----------------------------- nsRegistry::Pack -------------------------------
| Simple wrapper for NR_RegPack.  We don't set up any callback.                |
------------------------------------------------------------------------------*/
NS_IMETHODIMP nsRegistry::Pack() {
    nsresult rv = NS_OK;
    REGERR err = REGERR_OK;
    // Pack the registry.
    PR_Lock(mregLock);
    err = NR_RegPack( mReg, 0, 0 );
    PR_Unlock(mregLock);
    // Convert result.
    rv = regerr2nsresult( err );
    return rv;
}

/*-------------- nsRegSubtreeEnumerator::nsRegSubtreeEnumerator ----------------
| The ctor simply stashes all the information that will be needed to enumerate |
| the subkeys.                                                                 |
------------------------------------------------------------------------------*/
nsRegSubtreeEnumerator::nsRegSubtreeEnumerator( HREG hReg, RKEY rKey, PRBool all )
    : mReg( hReg ), mKey( rKey ), mEnum( 0 ), mNext( 0 ),
      mStyle( all ? REGENUM_DESCEND : REGENUM_CHILDREN ), mDone( PR_FALSE ) {
    NS_INIT_REFCNT();

    mName[0] = '\0';

#ifdef EXTRA_THREADSAFE
    // Create a registry lock
    mregLock = PR_NewLock();
#endif
    return;
}

nsRegSubtreeEnumerator::~nsRegSubtreeEnumerator()
{
#ifdef EXTRA_THREADSAFE
    if (mregLock) {
        PR_DestroyLock(mregLock);
    }
#endif
}

/*----------------------- nsRegSubtreeEnumerator::First ------------------------
| Set mEnum to 0; this will cause the next NR_RegEnum call to go to            |
| the beginning.  We then do a Next() call in order to do a "lookahead" to     |
| properly detect an empty list (i.e., set the mDone flag).                    |
------------------------------------------------------------------------------*/
NS_IMETHODIMP
nsRegSubtreeEnumerator::First() {
    nsresult rv = NS_OK;
    // Reset "done" flag.
    mDone = PR_FALSE;
    // Clear Name
    mName[0] = '\0';
    // Go to beginning.
    mEnum = mNext = 0;
    // Lookahead so mDone flag gets set for empty list.
    rv = Next();
    return rv;
}

/*----------------------- nsRegSubtreeEnumerator::Next -------------------------
| First, we check if we've already advanced to the end by checking the  mDone  |
| flag.                                                                        |
|                                                                              |
| We advance mEnum to the next enumeration value which is in the mNext         |
| lookahead buffer.  We must then call advance to lookahead and properly set   |
| the isDone flag.                                                             |
------------------------------------------------------------------------------*/
NS_IMETHODIMP
nsRegSubtreeEnumerator::Next() {
    nsresult rv = NS_OK;
    // Check for at end.
    if ( !mDone ) {
        // Advance to next spot.
        mEnum = mNext;
        // Lookahead so mDone is properly set (and to update mNext).
        rv = advance();
    } else {
        // Set result accordingly.
        rv = regerr2nsresult( REGERR_NOMORE );
    }
    return rv;
}

/*---------------------- nsRegSubtreeEnumerator::advance -----------------------
| Advance mNext to next subkey using NR_RegEnumSubkeys.  We set mDone if       |
| there are no more subkeys.                                                   |
------------------------------------------------------------------------------*/
NS_IMETHODIMP nsRegSubtreeEnumerator::advance() {
    REGERR err = REGERR_OK;
    PR_Lock(mregLock);
    err = NR_RegEnumSubkeys( mReg, mKey, &mNext, mName, sizeof mName, mStyle );
    // See if we ran off end.
    if( err == REGERR_NOMORE ) {
        // Remember we've run off end.
        mDone = PR_TRUE;
    }
    PR_Unlock(mregLock);
    // Convert result.
    nsresult rv = regerr2nsresult( err );
    return rv;
};

/*-------------------- nsRegSubtreeEnumerator::CurrentItem ---------------------
| Allocates and returns a new instance of class nsRegistryNode.  The node      |
| object will hold the curent mEnum value so it can obtain its name from       |
| the registry when asked.                                                     |
------------------------------------------------------------------------------*/
NS_IMETHODIMP
nsRegSubtreeEnumerator::CurrentItem( nsISupports **result) {
    nsresult rv = NS_OK;
    // Make sure there is a place to put the result.
    if( result ) {
        *result = new nsRegistryNode( mReg, mName, (RKEY) mNext );
        if( *result ) {
            NS_ADDREF(*result);
        } else {
            rv = NS_ERROR_OUT_OF_MEMORY;
        }
    } else {
        rv = NS_ERROR_NULL_POINTER;
    }
    return rv;
}

/*---------------------- nsRegSubtreeEnumerator::IsDone ------------------------
| Simply return mDone.                                                         |
------------------------------------------------------------------------------*/
NS_IMETHODIMP
nsRegSubtreeEnumerator::IsDone() {
    nsresult rv = mDone ? NS_OK : NS_ENUMERATOR_FALSE;
    return rv;
}


/*---------------- nsRegValueEnumerator::nsRegValueEnumerator ------------------
| Delegates everything to the base class constructor.                          |
------------------------------------------------------------------------------*/
nsRegValueEnumerator::nsRegValueEnumerator( HREG hReg, RKEY rKey )
    : nsRegSubtreeEnumerator( hReg, rKey, PR_FALSE ) {
    return;
}


/*--------------------- nsRegValueEnumerator::CurrentItem ----------------------
| As the nsRegSubtreeEnumerator counterpart, but allocates an object of        |
| class nsRegistryValue.                                                       |
------------------------------------------------------------------------------*/
NS_IMETHODIMP
nsRegValueEnumerator::CurrentItem( nsISupports **result ) {
    nsresult rv = NS_OK;
    // Make sure there is a place to put the result.
    if( result ) {
        *result = new nsRegistryValue( mReg, mKey, mEnum );
        if( *result ) {
            NS_ADDREF(*result);
        } else {
            rv = NS_ERROR_OUT_OF_MEMORY;
        }
    } else {
        rv = NS_ERROR_NULL_POINTER;
    }
    return rv;
}

/*----------------------- nsRegValueEnumerator::advance ------------------------
| Advance mNext to next subkey using NR_RegEnumEntries.  We set mDone if       |
| there are no more entries.                                                   |
------------------------------------------------------------------------------*/
NS_IMETHODIMP nsRegValueEnumerator::advance() {
    REGERR err = REGERR_OK;
    char name[MAXREGNAMELEN];
    PRUint32 len = sizeof name;
    REGINFO info = { sizeof info, 0, 0 };
    PR_Lock(mregLock);
    err = NR_RegEnumEntries( mReg, mKey, &mNext, name, len, &info );
    // See if we ran off end.
    if( err == REGERR_NOMORE ) {
        // Remember we've run off end.
        mDone = PR_TRUE;
    }
    PR_Unlock(mregLock);
    // Convert result.
    nsresult rv = regerr2nsresult( err );
    return rv;
};


/*---------------------- nsRegistryNode::nsRegistryNode ------------------------
| Store the arguments in the corresponding data members and initialize         |
| the other data members.  We defer the libreg calls till we're asked for      |
| our name.  We use mErr==-1 to indicate we haven't fetched the name yet.      |
------------------------------------------------------------------------------*/
nsRegistryNode::nsRegistryNode( HREG hReg, char *name, RKEY childKey )
    : mReg( hReg ), mChildKey( childKey ) {
    NS_INIT_REFCNT();

    PR_ASSERT(name != nsnull);
    strcpy(mName, name);

#ifdef EXTRA_THREADSAFE
    mregLock = PR_NewLock();
#endif
    
    return;
}

nsRegistryNode::~nsRegistryNode()
{
#ifdef EXTRA_THREADSAFE
    if (mregLock) {
        PR_DestroyLock(mregLock);
    }
#endif
}

/*-------------------------- nsRegistryNode::GetName ---------------------------
| If we haven't fetched it yet, get the name of the corresponding subkey now,  |
| using NR_RegEnumSubkeys.                                                     |
------------------------------------------------------------------------------*/
NS_IMETHODIMP nsRegistryNode::GetName( char **result ) {
    if (result == nsnull) return NS_ERROR_NULL_POINTER;
    // Make sure there is a place to put the result.
    *result = nsCRT::strdup( mName );
    if ( !*result ) return NS_ERROR_OUT_OF_MEMORY;
    return NS_OK;
}

/*-------------------------- nsRegistryNode::GetKey ----------------------------
| Get the subkey corresponding to this node                                    |						
| using NR_RegEnumSubkeys.                                                     |
------------------------------------------------------------------------------*/
NS_IMETHODIMP nsRegistryNode::GetKey( nsRegistryKey *r_key ) {
    nsresult rv = NS_OK;
    if (r_key == nsnull) return NS_ERROR_NULL_POINTER;
    *r_key = mChildKey;
    return rv;
}
    


/*--------------------- nsRegistryValue::nsRegistryValue -----------------------
| Implemented the same way as the nsRegistryNode ctor.                         |
------------------------------------------------------------------------------*/
nsRegistryValue::nsRegistryValue( HREG hReg, RKEY key, REGENUM slot )
    : mReg( hReg ), mKey( key ), mEnum( slot ), mErr( -1 ) {
    NS_INIT_REFCNT();
#ifdef EXTRA_THREADSAFE
    mregLock = PR_NewLock();
#endif
    mInfo.size = sizeof(REGINFO);
}

nsRegistryValue::~nsRegistryValue()
{
#ifdef EXTRA_THREADSAFE
    if (mregLock) {
        PR_DestroyLock(mregLock);
    }
#endif
}

/*------------------------- nsRegistryValue::GetName ---------------------------
| See nsRegistryNode::GetName; we use NR_RegEnumEntries in this case.         |
------------------------------------------------------------------------------*/
NS_IMETHODIMP nsRegistryValue::GetName( char **result ) {
    nsresult rv = NS_OK;
    // Make sure we have a place to put the result.
    if( result ) {
        // Ensure we've got the info we need.
        rv = getInfo();            
        if( rv == NS_OK || rv == NS_ERROR_REG_NO_MORE ) {
            // worked, return actual result.
            *result = nsCRT::strdup( mName );
            if ( *result ) {
                rv = NS_OK;
            } else {
                rv = NS_ERROR_OUT_OF_MEMORY;
            }
        }
    } else {
        rv = NS_ERROR_NULL_POINTER;
    }
    return rv;
}

/*----------------------- nsRegistryValue::GetType ------------------------
| We test if we've got the info already.  If not, we git it by calling         |
| getInfo.  We calculate the result by converting the REGINFO type field to    |
| a nsIRegistry::DataType value (using reginfo2DataType).                      |
------------------------------------------------------------------------------*/
NS_IMETHODIMP nsRegistryValue::GetType( PRUint32 *result ) {
    nsresult rv = NS_OK;
    // Make sure we have room for th result.
    if( result ) {
        // Make sure we've got the info we need.
        rv = getInfo();
        // Check if it worked.
        if( rv == NS_OK ) {
            // Convert result from REGINFO to nsIRegistry::ValueInfo.
            reginfo2DataType( mInfo, *result );
        }
    } else {
        rv = NS_ERROR_NULL_POINTER;
    }
    return rv;
}

/*---------------------- nsRegistryValue::GetLength -----------------------
| We test if we've got the info already.  If not, we git it by calling         |
| getInfo.  We calculate the result by converting the REGINFO type field to    |
| a nsIRegistry::DataType value (using reginfo2Length).                        |
------------------------------------------------------------------------------*/
NS_IMETHODIMP nsRegistryValue::GetLength( PRUint32 *result ) {
    nsresult rv = NS_OK;
    // Make sure we have room for th result.
    if( result ) {
        // Make sure we've got the info we need.
        rv = getInfo();
        // Check if it worked.
        if( rv == NS_OK ) {
            // Convert result from REGINFO to length.
            reginfo2Length( mInfo, *result );
        }
    } else {
        rv = NS_ERROR_NULL_POINTER;
    }
    return rv;
}

/*------------------------- nsRegistryValue::getInfo ---------------------------
| Call NR_RegEnumEntries to set the mInfo/mName data members.                  |
------------------------------------------------------------------------------*/
nsresult nsRegistryValue::getInfo() {
    nsresult rv = NS_OK;
    // Test whether we haven't tried to get it yet.
    if( mErr == -1 ) {
        REGENUM temp = mEnum;
        // Get name and info.
        PR_Lock(mregLock);
        mErr = NR_RegEnumEntries( mReg, mKey, &temp, mName, sizeof mName, &mInfo );
        // Convert result.
        rv = regerr2nsresult( mErr );            
        PR_Unlock(mregLock);
    }
    return rv;
}


nsRegistryFactory::nsRegistryFactory() {
    NS_INIT_REFCNT();
}

NS_IMPL_ISUPPORTS1(nsRegistryFactory, nsIFactory)

NS_IMETHODIMP
nsRegistryFactory::CreateInstance(nsISupports *aOuter,
                                   const nsIID &aIID,
                                   void **aResult) {
    nsresult rv = NS_OK;
    nsRegistry* newRegistry;

    if(aResult == nsnull) {
        return NS_ERROR_NULL_POINTER;
    } else {
        *aResult = nsnull;
    }

    if(0 != aOuter) {
        return NS_ERROR_NO_AGGREGATION;
    }

    NS_NEWXPCOM(newRegistry, nsRegistry);

    if(newRegistry == nsnull) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    NS_ADDREF(newRegistry);
    rv = newRegistry->QueryInterface(aIID, aResult);
    NS_RELEASE(newRegistry);

    return rv;
}

nsresult
nsRegistryFactory::LockFactory(PRBool aLock)
{
  // Not implemented in simplest case.
  return NS_OK;
}

// This is a temporary hack; needs work to support dynamic binding
// via nsComponentManager and support for multiple factories per DLL.
extern "C" NS_EXPORT nsresult
NS_RegistryGetFactory(nsIFactory** aFactory ) {
    nsresult rv = NS_OK;

    if( aFactory == 0 ) {
        return NS_ERROR_NULL_POINTER;
    } else {
        *aFactory = 0;
    }

    nsIFactory* inst = new nsRegistryFactory();
    if(0 == inst) {
        rv = NS_ERROR_OUT_OF_MEMORY;
    } else {
        NS_ADDREF(inst);
        *aFactory = inst;
    }

    return rv;
}
