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

#pragma once

#include "CTriStateTableHeader.h"

#include <vector.h>

#include <PP_Types.h>

#include "htrdf.h"

class CHyperTreeHeader : public CTriStateTableHeader
{
public:
	enum	{
		class_ID = 'hhdr'
	,	columnTextTraits_ID = 130
	};
	
	struct ColumnInfo {
		void*	token;
		Uint32	tokenType;
	};
	
					CHyperTreeHeader(LStream* inStream);
	virtual			~CHyperTreeHeader();
	
	void			SetUpColumns ( HT_View inView );
	
		// NOTE: inColumnKey should be zero-based.
	ColumnInfo&		GetColumnInfo(Uint32 inColumnKey);
	
protected:

		// overridden to talk to HT
	virtual void	ShowHideRightmostColumn(Boolean inShow);
	virtual void	MoveColumn(ColumnIndexT inColumn, ColumnIndexT inMoveTo);

	HT_View				mHTView;
	vector<ColumnInfo>	mColumnInfo;
};