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
#ifndef nsIAreaFrame_h___
#define nsIAreaFrame_h___

#include "nsIFrame.h"

// 7327a0a0-bc8d-11d1-8539-00a02468fab6
#define NS_IAREAFRAME_IID \
 { 0xa6cf90de, 0x15b3, 0x11d2, {0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32}}

/**
 * An interface for managing anchored items. Note that this interface is not
 * an nsISupports interface, and therefore you cannot QueryInterface() back
 */
class nsIAreaFrame
{
public:
  /**
   * Returns the x-most and y-most for the child absoolutely positioned
   * elements
   */
  NS_IMETHOD GetPositionedInfo(nscoord& aXMost, nscoord& aYMost) const = 0;
};

#endif /* nsIAreaFrame_h___ */
