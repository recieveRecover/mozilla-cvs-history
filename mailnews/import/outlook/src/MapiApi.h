/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
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
#ifndef MapiApi_h___
#define MapiApi_h___

#include "nscore.h"
#include "nsString.h"
#include "nsVoidArray.h"

#include <stdio.h>

#include <windows.h>
#include <mapi.h>
#include <mapix.h>
#include <mapidefs.h>
#include <mapicode.h>
#include <mapitags.h>
#include <mapiutil.h>
// wabutil.h expects mapiutil to define _MAPIUTIL_H but it actually
// defines _MAPIUTIL_H_
#define _MAPIUTIL_H



class CMapiFolder {
public:
	CMapiFolder();
	CMapiFolder( const CMapiFolder *pCopyFrom);
	CMapiFolder( const char *pDisplayName, ULONG cbEid, LPENTRYID lpEid, int depth, LONG oType = MAPI_FOLDER);
	~CMapiFolder();

	void	SetDoImport( BOOL doIt) { m_doImport = doIt;}
	void	SetObjectType( long oType) { m_objectType = oType;}
	void	SetDisplayName( const char *pDisplayName) { m_displayName = pDisplayName;}
	void	SetEntryID( ULONG cbEid, LPENTRYID lpEid);
	void	SetDepth( int depth) { m_depth = depth;}
	void	SetFilePath( const char *pFilePath) { m_mailFilePath = pFilePath;}
	
	BOOL	GetDoImport( void) const { return( m_doImport);}
	LONG	GetObjectType( void) const { return( m_objectType);}
	void	GetDisplayName( nsCString& name) const { name = m_displayName;}
	void	GetFilePath( nsCString& path) const { path = m_mailFilePath;}
	BOOL	IsStore( void) const { return( m_objectType == MAPI_STORE);}
	BOOL	IsFolder( void) const { return( m_objectType == MAPI_FOLDER);}
	int		GetDepth( void) const { return( m_depth);}

	LPENTRYID	GetEntryID( ULONG *pCb = NULL) const { if (pCb) *pCb = m_cbEid; return( (LPENTRYID) m_lpEid);}
	ULONG		GetCBEntryID( void) const { return( m_cbEid);}

private:
	LONG		m_objectType;
	ULONG		m_cbEid;
	BYTE *		m_lpEid;
	nsCString	m_displayName;
	int			m_depth;
	nsCString	m_mailFilePath;
	BOOL		m_doImport;

};

class CMapiFolderList {
public:
	CMapiFolderList();
	~CMapiFolderList();

	void			AddItem( CMapiFolder *pFolder);
	CMapiFolder *	GetItem( int index) { if ((index >= 0) && (index < m_array.Count())) return( (CMapiFolder *)GetAt( index)); else return( NULL);}
	void			ClearAll( void);

	// Debugging and reporting
	void			DumpList( void);

	void *			GetAt( int index) { return( m_array.ElementAt( index));}
	int				GetSize( void) { return( m_array.Count());}

protected:
	void	EnsureUniqueName( CMapiFolder *pFolder);
	void	GenerateFilePath( CMapiFolder *pFolder);
	void	ChangeName( nsCString& name);

private:
	nsVoidArray		m_array;
};


class CMsgStore {
public:
	CMsgStore( ULONG cbEid = 0, LPENTRYID lpEid = NULL);
	~CMsgStore();

	void		SetEntryID( ULONG cbEid, LPENTRYID lpEid);
	BOOL		Open( LPMAPISESSION pSession, LPMDB *ppMdb);
	
	ULONG		GetCBEntryID( void) { return( m_cbEid);}
	LPENTRYID	GetLPEntryID( void) { return( (LPENTRYID) m_lpEid);}

private:
	ULONG		m_cbEid;
	BYTE *		m_lpEid;
	LPMDB		m_lpMdb;
};

class CMapiContentIter {
public:
	virtual BOOL HandleContentItem( ULONG oType, ULONG cb, LPENTRYID pEntry) = 0;
};

class CMapiHierarchyIter {
public:
	virtual BOOL HandleHierarchyItem( ULONG oType, ULONG cb, LPENTRYID pEntry) = 0;
};

class CMapiApi {
public:
	CMapiApi();
	~CMapiApi();

	static BOOL		LoadMapi( void);
	static BOOL		LoadMapiEntryPoints( void);
	static void		UnloadMapi( void);

	static HINSTANCE	m_hMapi32;

	static void		MAPIUninitialize( void);
	static HRESULT	MAPIInitialize( LPVOID lpInit);
	static SCODE	MAPIAllocateBuffer( ULONG cbSize, LPVOID FAR * lppBuffer);
	static ULONG	MAPIFreeBuffer( LPVOID lpBuff);
	static HRESULT	MAPILogonEx( ULONG ulUIParam, LPTSTR lpszProfileName, LPTSTR lpszPassword, FLAGS flFlags, LPMAPISESSION FAR * lppSession);
	static HRESULT	OpenStreamOnFile( LPALLOCATEBUFFER lpAllocateBuffer, LPFREEBUFFER lpFreeBuffer, ULONG ulFlags, LPTSTR lpszFileName, LPTSTR lpszPrefix, LPSTREAM FAR * lppStream);
	static void		FreeProws( LPSRowSet prows);


	BOOL	Initialize( void);
	BOOL	LogOn( void);

	void	AddMessageStore( CMsgStore *pStore);
	void	SetCurrentMsgStore( LPMDB lpMdb) { m_lpMdb = lpMdb;}

	// Open any given entry from the current Message Store	
	BOOL	OpenEntry( ULONG cbEntry, LPENTRYID pEntryId, LPUNKNOWN *ppOpen);
	static BOOL OpenMdbEntry( LPMDB lpMdb, ULONG cbEntry, LPENTRYID pEntryId, LPUNKNOWN *ppOpen);

	// Fill in the folders list with the heirarchy from the given
	// message store.
	BOOL	GetStoreFolders( ULONG cbEid, LPENTRYID lpEid, CMapiFolderList& folders, int startDepth);
	BOOL	OpenStore( ULONG cbEid, LPENTRYID lpEid, LPMDB *ppMdb);

	// Iteration
	BOOL	IterateStores( CMapiFolderList& list);
	BOOL	IterateContents( CMapiContentIter *pIter, LPMAPIFOLDER pFolder, ULONG flags = 0);
	BOOL	IterateHierarchy( CMapiHierarchyIter *pIter, LPMAPIFOLDER pFolder, ULONG flags = 0);
	
	// Properties
	static LPSPropValue	GetMapiProperty( LPMAPIPROP pProp, ULONG tag);
	static BOOL			GetEntryIdFromProp( LPSPropValue pVal, ULONG& cbEntryId, LPENTRYID& lpEntryId, BOOL delVal = TRUE);
	static BOOL			GetStringFromProp( LPSPropValue pVal, nsCString& val, BOOL delVal = TRUE);
	static LONG			GetLongFromProp( LPSPropValue pVal, BOOL delVal = TRUE);
	static BOOL			GetLargeStringProperty( LPMAPIPROP pProp, ULONG tag, nsCString& val);
	static BOOL			IsLargeProperty( LPSPropValue pVal);

	// Debugging & reporting stuff
	static void			ListProperties( LPMAPIPROP lpProp, BOOL getValues = TRUE);
	static void			ListPropertyValue( LPSPropValue pVal, nsCString& s);

protected:
	BOOL			HandleHierarchyItem( ULONG oType, ULONG cb, LPENTRYID pEntry);
	BOOL			HandleContentsItem( ULONG oType, ULONG cb, LPENTRYID pEntry);
	void			GetStoreInfo( CMapiFolder *pFolder, long *pSzContents);

	// array of available message stores, cached so that
	// message stores are only opened once, preventing multiple
	// logon's by the user if the store requires a logon.
	CMsgStore *		FindMessageStore( ULONG cbEid, LPENTRYID lpEid);
	void			ClearMessageStores( void);

	// Debugging & reporting stuff
	static void			GetPropTagName( ULONG tag, nsCString& s);
	static void			ReportStringProp( const char *pTag, LPSPropValue pVal);
	static void			ReportUIDProp( const char *pTag, LPSPropValue pVal);
	static void			ReportLongProp( const char *pTag, LPSPropValue pVal);


private:
	nsVoidArray		m_stores;
	BOOL			m_initialized;
	LPMAPISESSION	m_lpSession;
	LPMDB			m_lpMdb;
	HRESULT			m_lastError;
};


class CMapiFolderContents {
public:
	CMapiFolderContents( LPMDB lpMdb, ULONG cbEID, LPENTRYID lpEid);
	~CMapiFolderContents();

	BOOL	GetNext( ULONG *pcbEid, LPENTRYID *ppEid, ULONG *poType, BOOL *pDone);

	ULONG	GetCount( void) { return( m_count);}

protected:
	BOOL SetUpIter( void);

private:
	HRESULT			m_lastError;
	BOOL			m_failure;
	LPMDB			m_lpMdb;
	LPMAPIFOLDER	m_lpFolder;
	LPMAPITABLE		m_lpTable;
	ULONG			m_fCbEid;
	BYTE *			m_fLpEid;
	ULONG			m_count;
	ULONG			m_iterCount;
	BYTE *			m_lastLpEid;
	ULONG			m_lastCbEid;
};


#endif /* MapiApi_h__ */
