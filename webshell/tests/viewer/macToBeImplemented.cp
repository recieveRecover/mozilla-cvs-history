/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

// Unimplemented code temporarily lives here

#include "fe_proto.h"
#include "xp_file.h"
#include "prlog.h"


/* Netlib utility routine, should be ripped out */
void	FE_FileType(char * path, Bool * useDefault, char ** fileType, char ** encoding)
{
	if ((path == NULL) || (fileType == NULL) || (encoding == NULL))
		return;

	*useDefault = TRUE;
	*fileType = NULL;
	*encoding = NULL;
}

#include "mcom_db.h"
DB * dbopen(const char *fname, int flags,int mode, DBTYPE type, const void *openinfo)
{
	PR_ASSERT(FALSE);
	return NULL;
}

char * XP_FileReadLine(char * dest, int32 bufferSize, XP_File file)
{
	PR_ASSERT(FALSE);
	return NULL;
}
