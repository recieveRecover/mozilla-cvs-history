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

//
// CShelfMixin.cp
//
// Implementation of the CShelf class. See header files for details
//

#include "CShelfMixin.h"
#include "prefapi.h"
#include "CTargetFramer.h"

#include "LCommander.h"


//
// Constructor
//
CShelf :: CShelf ( LDividedView* inDivView, const char* inPrefString, 
					LDividedView::FeatureFlags inFlags ) 
	: mPrefString(inPrefString), mShelf(inDivView)
{
	// configure the divided view appropriately
	mShelf->OnlySetFeatureFlags ( inFlags );
}


//
// Destructor
//
// Don't dispose of anything explicitly.
//
CShelf :: ~CShelf ( )
{
}


//
// ToggleShelf
//
// Expand or collapse the shelf based on the previous state. Will also update the 
// preference if the parameter flag is true.
//
void 
CShelf :: ToggleShelf ( bool inUpdatePref )
{
	if ( !mShelf )
		return;			// this is the case for composer
	
	mShelf->DoZapAction();
	
	// Update the visible pref. The pref should be true if the pane is showing (!collapsed)
	if ( inUpdatePref && mPrefString.c_str() )
		PREF_SetBoolPref ( mPrefString.c_str(), IsShelfOpen() );
	
	// force menu items to update show "Show" and "Hide" string changes are reflected
	LCommander::SetUpdateCommandStatus(true);
	
} // ToggleShelf


//
// SetShelfState
//
// Explicitly set the state of the shelf (useful for initialization based on a preference).
// This is so confusing because all we can do is toggle the panes in LDividedView.
//
void
CShelf :: SetShelfState ( bool inShelfOpen, bool inUpdatePref )
{
	if ( !mShelf )
		return;			// this is the case for composer
	
	bool isShelfOpenNow = IsShelfOpen();
	if ( inShelfOpen ) {
		if ( !isShelfOpenNow )
			ToggleShelf(inUpdatePref);
	}
	else
		if ( isShelfOpenNow )
			ToggleShelf(inUpdatePref);
	
} // SetShelfState


//
// IsShelfOpen
//
// return the state of the shelf.
//
bool
CShelf :: IsShelfOpen ( )
{
	if ( !mShelf )
		return false;			// this is the case for composer
	
	// this class handles both the case where the first pane is the one being collapsed
	// and also the case where the 2nd pane is the one being collapsed. Either way, if
	// one of them is collapsed, then the shelf is not open (because the concept of the
	// shelf is tied to the one that is collapsed)
	return ( ! (mShelf->IsFirstPaneCollapsed() || mShelf->IsSecondPaneCollapsed()) ) ;

} // IsShelfOpen
