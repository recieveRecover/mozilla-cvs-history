/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#ifndef nsTextAddress_h__
#define nsTextAddress_h__

#include "nscore.h"
#include "nsString.h"
#include "nsVoidArray.h"
#include "nsIFileSpec.h"
#include "nsISupportsArray.h"
#include "nsIImportFieldMap.h"


class nsIAddrDatabase;
class nsIMdbRow;

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

class nsTextAddress {
public:
	nsTextAddress();
	virtual ~nsTextAddress();

	nsresult	ImportAddresses( PRBool *pAbort, const PRUnichar *pName, nsIFileSpec *pSrc, nsIAddrDatabase *pDb, nsIImportFieldMap *fieldMap, nsString& errors);
	nsresult	ImportLDIF( PRBool *pAbort, const PRUnichar *pName, nsIFileSpec *pSrc, nsIAddrDatabase *pDb, nsString& errors);

	nsresult	DetermineDelim( nsIFileSpec *pSrc);
	char		GetDelim( void) { return( m_delim);}

	static nsresult		IsLDIFFile( nsIFileSpec *pSrc, PRBool *pIsLDIF);
	
	static nsresult		ReadRecordNumber( nsIFileSpec *pSrc, char *pLine, PRInt32 bufferSz, char delim, PRInt32 *pLineLen, PRInt32 rNum);
	static nsresult		ReadRecord( nsIFileSpec *pSrc, char *pLine, PRInt32 bufferSz, char delim, PRInt32 *pLineLen);
	static PRBool		GetField( const char *pLine, PRInt32 maxLen, PRInt32 index, nsCString& field, char delim);

private:
	nsresult		ProcessLine( const char *pLine, PRInt32 len, nsString& errors);
	
	static PRBool		IsLineComplete( const char *pLine, PRInt32 len, char delim);
	static PRInt32		CountFields( const char *pLine, PRInt32 maxLen, char delim);
	static void			SanitizeSingleLine( nsCString& val);

private:
	// LDIF stuff, this wasn't originally written by me!
	nsresult	str_parse_line( char *line, char **type, char **value, int *vlen);
	char *		str_getline( char **next);
	nsresult	GetLdifStringRecord(char* buf, PRInt32 len, PRInt32& stopPos);
	nsresult	ParseLdifFile( nsIFileSpec *pSrc);
	void		AddLdifRowToDatabase( void);
	void		AddLdifColToDatabase(nsIMdbRow* newRow, char* typeSlot, char* valueSlot);

private:
	nsCString			m_ldifLine;
	char				m_delim;
	nsIAddrDatabase *	m_database;
	nsIImportFieldMap *	m_fieldMap;
};



#endif /* nsTextAddress_h__ */

