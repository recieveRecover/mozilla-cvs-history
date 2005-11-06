/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * IBM Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2000
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifdef IBMBIDI

#include "nsBidiFrames.h"
#include "nsLayoutAtoms.h"


nsDirectionalFrame::nsDirectionalFrame(PRUnichar aChar)
  : mChar(aChar)
{
}

nsDirectionalFrame::~nsDirectionalFrame()
{
}

PRUnichar
nsDirectionalFrame::GetChar(void) const
{
  return mChar;
}

/**
 * Get the "type" of the frame
 *
 * @see nsLayoutAtoms::directionalFrame
 */
nsIAtom*
nsDirectionalFrame::GetType() const
{ 
  return nsLayoutAtoms::directionalFrame; 
}
  
const nsIID&
nsDirectionalFrame::GetIID()
{
  static nsIID iid = NS_DIRECTIONAL_FRAME_IID;
  return iid;
}

NS_IMETHODIMP
nsDirectionalFrame::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  nsresult rv = NS_NOINTERFACE;

  if (!aInstancePtr) {
    rv = NS_ERROR_NULL_POINTER;
  }
  else if (aIID.Equals(NS_GET_IID(nsDirectionalFrame) ) ) {
    *aInstancePtr = (void*) this;
    rv = NS_OK;
  }
  return rv;
}

void*
nsDirectionalFrame::operator new(size_t aSize) CPP_THROW_NEW
{
  void* frame = ::operator new(aSize);
  if (frame) {
    memset(frame, 0, aSize);
  }
  return frame;
}

nsIFrame*
NS_NewDirectionalFrame(nsIPresShell* aPresShell, PRUnichar aChar)
{
  return new nsDirectionalFrame(aChar);
}

#endif /* IBMBIDI */
