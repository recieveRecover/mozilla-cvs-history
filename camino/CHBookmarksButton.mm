/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
*
* The contents of this file are subject to the Mozilla Public
* License Version 1.1 (the "License"); you may not use this file
* except in compliance with the License. You may obtain a copy of
* the License at http://www.mozilla.org/MPL/
*
* Software distributed under the License is distributed on an "AS
* IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
* implied. See the License for the specific language governing
* rights and limitations under the License.
*
* The Original Code is the Mozilla browser.
*
* The Initial Developer of the Original Code is Netscape
* Communications Corporation. Portions created by Netscape are
* Copyright (C) 2002 Netscape Communications Corporation. All
* Rights Reserved.
*
* Contributor(s):
*   David Hyatt <hyatt@netscape.com> (Original Author)
*/

#import "CHBookmarksButton.h"
#include "nsIDOMElement.h"
#include "nsIContent.h"
#include "nsString.h"
#import "BookmarksService.h"

@implementation CHBookmarksButton

- (id)initWithFrame:(NSRect)frame {
    if ( (self = [super initWithFrame:frame]) ) {
        mElement = nsnull;
        [self setBezelStyle: NSRegularSquareBezelStyle];
        [self setBordered: NO];
        [self setImagePosition: NSImageLeft];
        [self setRefusesFirstResponder: YES];
        [self setFont: [NSFont labelFontOfSize: 11.0]];
    }
  return self;
}

-(IBAction)openBookmark:(id)aSender
{
  // See if we're a group.
  nsAutoString group;
  mElement->GetAttribute(NS_LITERAL_STRING("group"), group);
  if (!group.IsEmpty()) {
    BookmarksService::OpenBookmarkGroup([[[self window] windowController] getTabBrowser], mElement);
    return;
  }
  
  // Get the href attribute.  This is the URL we want to load.
  nsAutoString href;
  mElement->GetAttribute(NS_LITERAL_STRING("href"), href);
  nsCAutoString cref; cref.AssignWithConversion(href);
  if (cref.IsEmpty())
    return;

  NSString* url = [NSString stringWithCString: cref.get()];

  // Now load the URL in the window.
  [[[self window] windowController] loadURL:[NSURL URLWithString: url]];

  // Focus and activate our content area.
  [[[[[self window] windowController] getBrowserWrapper] getBrowserView] setActive: YES];
}

-(void)drawRect:(NSRect)aRect
{
  [super drawRect: aRect];
}

-(NSMenu*)menuForEvent:(NSEvent*)aEvent
{
  return [[self superview] menu];
}

-(void)mouseDown:(NSEvent*)aEvent
{
  if (mIsFolder) {
    nsCOMPtr<nsIContent> content(do_QueryInterface(mElement));
    NSMenu* menu = BookmarksService::LocateMenu(content);
    [NSMenu popUpContextMenu: menu withEvent: aEvent forView: self];
  }
  else
    [super mouseDown: aEvent];
}

-(void)setElement: (nsIDOMElement*)aElt
{
  mElement = aElt;
  nsAutoString tag;
  mElement->GetLocalName(tag);

  nsAutoString group;
  mElement->GetAttribute(NS_LITERAL_STRING("group"), group);

  if (!group.IsEmpty()) {
    mIsFolder = NO;
    [self setImage: [NSImage imageNamed: @"groupbookmark"]];
    [self setAction: @selector(openBookmark:)];
    [self setTarget: self];
  }
  else if (tag.Equals(NS_LITERAL_STRING("folder"))) {
    [self setImage: [NSImage imageNamed: @"folder"]];
    mIsFolder = YES;
  }
  else {
    mIsFolder = NO;
    [self setImage: [NSImage imageNamed: @"smallbookmark"]];
    [self setAction: @selector(openBookmark:)];
    [self setTarget: self];
  }
  
  nsAutoString name;
  mElement->GetAttribute(NS_LITERAL_STRING("name"), name);
  [self setTitle: [NSString stringWithCharacters: name.get() length: nsCRT::strlen(name.get())]];
}

-(nsIDOMElement*)element
{
  return mElement;
}

@end
