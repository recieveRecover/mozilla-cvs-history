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

#ifndef nsScrollbar_h__
#define nsScrollbar_h__

#include "nsWidget.h"
#include "nsIScrollbar.h"

/**
 * Native Photon scrollbar wrapper.
 */

class nsScrollbar : public nsWidget,
                    public nsIScrollbar
{

public:
                            nsScrollbar(PRBool aIsVertical);
    virtual                 ~nsScrollbar();

    // nsISupports
    NS_DECL_ISUPPORTS_INHERITED

    // nsIScrollBar implementation
    NS_IMETHOD SetMaxRange(PRUint32 aEndRange);
    NS_IMETHOD GetMaxRange(PRUint32& aMaxRange);
    NS_IMETHOD SetPosition(PRUint32 aPos);
    NS_IMETHOD GetPosition(PRUint32& aPos);
    NS_IMETHOD SetThumbSize(PRUint32 aSize);
    NS_IMETHOD GetThumbSize(PRUint32& aSize);
    NS_IMETHOD SetLineIncrement(PRUint32 aSize);
    NS_IMETHOD GetLineIncrement(PRUint32& aSize);
    NS_IMETHOD SetParameters(PRUint32 aMaxRange, PRUint32 aThumbSize,
                               PRUint32 aPosition, PRUint32 aLineIncrement);


    virtual PRBool    OnScroll(nsScrollbarEvent & aEvent, PRUint32 cPos);

protected:
  NS_IMETHOD CreateNative(PtWidget_t *parentWindow);
  static int handle_scroll_move_event (PtWidget_t *aWidget, void *aData, PtCallbackInfo_t *aCbinfo );

private:
  int             mOrientation;

};

#endif // nsScrollbar_
