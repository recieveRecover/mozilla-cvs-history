/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are Copyright (C) 1998
 * Netscape Communications Corporation.  All Rights Reserved.
 */
#ifndef nsILinkHandler_h___
#define nsILinkHandler_h___

#include "nsweb.h"
#include "nsISupports.h"

class nsIFrame;
class nsString;
class nsIPostData;
struct nsGUIEvent;

// Interface ID for nsILinkHandler
#define NS_ILINKHANDLER_IID \
 { 0xa6cf905b, 0x15b3, 0x11d2,{0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32}}

enum nsLinkState {
  eLinkState_Unvisited  = 0,
  eLinkState_Visited    = 1, 
  eLinkState_OutOfDate  = 2,  // visited, but the cache is stale
  eLinkState_Active     = 3,  // mouse is down on link
  eLinkState_Hover      = 4   // mouse is hovering over link
};

/**
 * Interface used for handling clicks on links
 */
class nsILinkHandler : public nsISupports {
public:
  /**
   * Process a click on a link. aFrame is the frame that contains the
   * linked content. aURLSpec is an absolute url spec that defines the
   * destination for the link. aTargetSpec indicates where the link is
   * targeted (it may be an empty string).
   */
  NS_IMETHOD OnLinkClick(nsIFrame* aFrame, 
                         const nsString& aURLSpec,
                         const nsString& aTargetSpec,
                         nsIPostData* aPostData = 0) = 0;

  /**
   * Process a mouse-over a link. aFrame is the frame that contains the
   * linked content. aURLSpec is an absolute url spec that defines the
   * destination for the link. aTargetSpec indicates where the link is
   * targeted (it may be an empty string).
   */
  NS_IMETHOD OnOverLink(nsIFrame* aFrame, 
                        const nsString& aURLSpec,
                        const nsString& aTargetSpec) = 0;

  /**
   * Get the state of a link to a given absolute URL
   */
  NS_IMETHOD GetLinkState(const nsString& aURLSpec, nsLinkState& aState) = 0;
};

#endif /* nsILinkHandler_h___ */
