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

#ifndef nsIListControlFrame_h___
#define nsIListControlFrame_h___

#include "nsISupports.h"
#include "nsFont.h"
class nsFormFrame;
class nsIPresContext;
class nsString;
class nsIContent;


// IID for the nsIListControlFrame class
#define NS_ILISTCONTROLFRAME_IID    \
{ 0xf44db101, 0xa73c, 0x11d2,  \
  { 0x8d, 0xcf, 0x0, 0x60, 0x97, 0x3, 0xc1, 0x4e } }

/** 
  * nsIListControlFrame is the interface for frame-based listboxes.
  */
class nsIListControlFrame : public nsISupports {

public:
  NS_DEFINE_STATIC_IID_ACCESSOR(NS_ILISTCONTROLFRAME_IID)

  /**
   * Sets the ComboBoxFrame
   *
   */
  NS_IMETHOD SetComboboxFrame(nsIFrame* aComboboxFrame) = 0;

  /**
   * Get the Selected Item's String
   *
   */
  NS_IMETHOD GetSelectedItem(nsString & aStr) = 0;

  /**
   * Get the Selected Item's index
   *
   */
  NS_IMETHOD GetSelectedIndex(PRInt32* aIndex) = 0;

  /**
   * Initiates mouse capture for the listbox
   *
   */
  NS_IMETHOD CaptureMouseEvents(nsIPresContext* aPresContext, PRBool aGrabMouseEvents) = 0;

  /**
   * Returns the maximum width and height of an item in the listbox
   */

  NS_IMETHOD GetMaximumSize(nsSize &aSize) = 0;

  /**
   * Returns the number of options in the listbox
   */

  NS_IMETHOD GetNumberOfOptions(PRInt32* aNumOptions) = 0; 

  /**
   * 
   */
  NS_IMETHOD SyncViewWithFrame(nsIPresContext* aPresContext) = 0;

  /**
   * 
   */
  NS_IMETHOD AboutToDropDown() = 0;

  /**
   * 
   */
  NS_IMETHOD AboutToRollup() = 0;

  /**
   *
   */
  NS_IMETHOD UpdateSelection(PRBool aDoDispatchEvent, PRBool aForceUpdate, nsIContent* aContent) = 0;

};

#endif

