/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Original Author: David W. Hyatt (hyatt@netscape.com)
 *
 * Contributor(s): 
 */

#ifndef nsIScrollbarFrame_h___
#define nsIScrollbarFrame_h___

// {9A6B0416-4A5D-4550-BEB5-C94D18A69A94}
#define NS_ISCROLLBARFRAME_IID \
{ 0x9a6b0416, 0x4a5d, 0x4550, { 0xbe, 0xb5, 0xc9, 0x4d, 0x18, 0xa6, 0x9a, 0x94 } }

static NS_DEFINE_IID(kIScrollbarFrameIID,     NS_ISCROLLBARFRAME_IID);

class nsIScrollbarMediator;

class nsIScrollbarFrame : public nsISupports {

public:
  static const nsIID& GetIID() { static nsIID iid = NS_ISCROLLBARFRAME_IID; return iid; }
  
  NS_IMETHOD GetScrollbarMediator(nsIScrollbarMediator** aResult) = 0;
  NS_IMETHOD SetScrollbarMediator(nsIScrollbarMediator* aMediator) = 0;
};

#endif

