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
// Mike Pinkerton, Netscape Communications
//
// Class that draws the header area for the nav center which shows the title
// of the currently selected view as well as harboring the closebox for
// an easy way to close up the navCenter shelf.
//

#include "CNavCenterTitle.h"
#include "CNavCenterSelectorPane.h"		// for message id's
#include "URDFUtilities.h"

extern RDF_NCVocab gNavCenter;			// RDF vocab struct for NavCenter


CNavCenterTitle :: CNavCenterTitle ( LStream *inStream )
	: CGrayBevelView (inStream),
		mTitle(NULL), mView(NULL)
{

}


CNavCenterTitle :: ~CNavCenterTitle()
{
	// nothing to do 
}


//
// FinishCreateSelf
//
// Last minute setup stuff....
//
void
CNavCenterTitle :: FinishCreateSelf ( )
{
	mTitle = dynamic_cast<LCaption*>(FindPaneByID(kTitlePaneID));
	Assert_(mTitle != NULL);
	
} // FinishCreateSelf


//
// ListenToMessage
//
// We want to know when the selected workspace changes so that we can update the
// title string. The RDFCoordinator sets us up as a listener to the selector pane
// which will broadcast when things change.
//
void
CNavCenterTitle :: ListenToMessage ( MessageT inMessage, void* ioParam ) 
{
	switch ( inMessage ) {
	
		case CNavCenterSelectorPane::msg_ActiveSelectorChanged:
		{
			mView = reinterpret_cast<HT_View>(ioParam);
			if ( mView ) {
				// do not delete |buffer|
				const char* buffer = HT_GetViewName ( mView );
				TitleCaption().SetDescriptor(LStr255(buffer));
				
				// if we're in the middle of a drag and drop, draw NOW, not
				// when we get a refresh event.
				if ( ::StillDown() ) {
					FocusDraw();
					Draw(nil);
				}
			}
		}
	
	} // case of which message

} // ListenToMessage


void
CNavCenterTitle :: DrawBeveledFill ( )
{
	StColorState saved;
	
	if ( mView ) {
		HT_Resource topNode = HT_TopNode(mView);
		if ( topNode ) {
			char* url = NULL;
			PRBool success = HT_GetNodeData ( topNode, gNavCenter->titleBarBGURL, HT_COLUMN_STRING, &url );
			if ( success && url ) {
				// draw the background image tiled to fill the whole pane
				Point topLeft = { 0, 0 };
				SetImageURL ( url );
				DrawImage ( topLeft, kTransformNone, mFrameSize.width, mFrameSize.height );
				FocusDraw();
			}
			else 
				EraseBackground(topNode);
		}
		else
			EraseBackground(NULL);
	}
	else
		EraseBackground(NULL);

} // DrawBeveledFill


//
// DrawStandby
//
// Draw correctly when the image has yet to load.
//
void
CNavCenterTitle :: DrawStandby ( const Point & /*inTopLeft*/, 
									const IconTransformType /*inTransform*/ ) const
{
	// we're just waiting for the image to come in, who cares if we don't use
	// HT's color?
	EraseBackground(NULL);

} // DrawStandby


//
// EraseBackground
//
// Fill in the bg with either the correct HT color (from a property on |inTopNode|, the
// correct AM color, or the default GA implementation (if we are before AM 1.1).
//
void
CNavCenterTitle :: EraseBackground ( HT_Resource inTopNode ) const
{
	// when we can get the right AM bg color (in AM 1.1), use that but for now just ignore it
	if ( !inTopNode || ! URDFUtilities::SetupBackgroundColor(inTopNode, gNavCenter->titleBarBGColor,
											kThemeListViewSortColumnBackgroundBrush) ) {
		CNavCenterTitle* self = const_cast<CNavCenterTitle*>(this);		// hack
		self->CGrayBevelView::DrawBeveledFill();
	}
	else {
		// use HT's color
		Rect bounds;
		CalcLocalFrameRect(bounds);
		::EraseRect(&bounds);
	}
} // EraseBackground