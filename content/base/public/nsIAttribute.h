/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
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
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Peter Van der Beken <peterv@netscape.com>
 *   Allan Beaufour <allan@beaufour.dk>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
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

#ifndef nsIAttribute_h___
#define nsIAttribute_h___

#include "nsISupports.h"
#include "nsINodeInfo.h"
#include "nsIContent.h"
#include "nsINode.h"
#include "nsIDOMGCParticipant.h"

class nsIAtom;
class nsDOMAttributeMap;

#define NS_IATTRIBUTE_IID  \
{ 0xba2a0ee1, 0x2484, 0x49d7, \
 { 0xb5, 0x3c, 0xc8, 0xe4, 0x82, 0x9d, 0xa4, 0xf2 } }

class nsIAttribute : public nsINode
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IATTRIBUTE_IID)

  virtual void SetMap(nsDOMAttributeMap *aMap) = 0;
  
  nsDOMAttributeMap *GetMap()
  {
    return mAttrMap;
  }

  nsINodeInfo *NodeInfo()
  {
    return mNodeInfo;
  }

  virtual nsIContent* GetContent() const = 0;

  /**
   * Called when our ownerElement is moved into a new document.
   * Updates the nodeinfo of this node.
   */
  virtual nsresult SetOwnerDocument(nsIDocument* aDocument) = 0;

protected:
  nsIAttribute(nsDOMAttributeMap *aAttrMap, nsINodeInfo *aNodeInfo)
    : nsINode(aNodeInfo), mAttrMap(aAttrMap)
  {
  }

  nsDOMAttributeMap *mAttrMap; // WEAK
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIAttribute, NS_IATTRIBUTE_IID)

#endif /* nsIAttribute_h___ */
