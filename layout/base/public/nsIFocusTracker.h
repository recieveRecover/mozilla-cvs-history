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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */
#ifndef nsIFocusTracker_h___
#define nsIFocusTracker_h___

#include "nsISupports.h"
#include "nsIFrame.h"


class nsIPresContext;
   

// IID for the nsIFocusTracker interface
#define NS_IFOCUSTRACKER_IID  \
{ 0x81ac51d1, 0x923b, 0x11d2, \
  { 0x91, 0x8f, 0x0, 0x80, 0xc8, 0xe4, 0x4d, 0xb5 } }

class nsIFocusTracker : public nsISupports
{
public:
  static const nsIID& GetIID() { static nsIID iid = NS_IFOCUSTRACKER_IID; return iid; }

  /** ScrollFrameIntoView
   *  limited version of nsPresShell::ScrollFrameIntoView
   *  @param aFrame will be the frame to scroll into view.
   */
  NS_IMETHOD ScrollFrameIntoView(nsIFrame *aFrame) = 0;

  /**
   * Returns the primary frame associated with the content object.
   *
   * The primary frame is the frame that is most closely associated with the
   * content. A frame is more closely associated with the content that another
   * frame if the one frame contains directly or indirectly the other frame (e.g.,
   * when a frame is scrolled there is a scroll frame that contains the frame
   * being scrolled). The primary frame is always the first-in-flow.
   *
   * In the case of absolutely positioned elements and floated elements,
   * the primary frame is the frame that is out of the flow and not the
   * placeholder frame.
   */
  NS_IMETHOD GetPrimaryFrameFor(nsIContent* aContent,
                                nsIFrame**  aPrimaryFrame) const = 0;

  /**
   * GetPresContent will return the nsIPresContext Interface from this 
   * FocusTracker
   * usefull for getting screen coordinates of current selection
   */
  NS_IMETHOD GetPresContext(nsIPresContext **aContext) = 0;
};


#endif //nsIFocusTracker_h___
