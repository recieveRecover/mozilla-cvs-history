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
 * The Original Code is Mozilla SVG Project code.
 *
 * The Initial Developer of the Original Code is
 * Jonathan Watt.
 * Portions created by the Initial Developer are Copyright (C) 2004
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Jonathan Watt <jonathan.watt@strath.ac.uk> (original author)
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

#include "nsSVGElement.h"
#include "nsIDOMSVGMetadataElement.h"

typedef nsSVGElement nsSVGMetadataElementBase;

class nsSVGMetadataElement : public nsSVGMetadataElementBase,
                             public nsIDOMSVGMetadataElement
{
protected:
  friend nsresult NS_NewSVGMetadataElement(nsIContent **aResult,
                                           nsINodeInfo *aNodeInfo);
  nsSVGMetadataElement(nsINodeInfo* aNodeInfo);
  virtual ~nsSVGMetadataElement();
  nsresult Init();

public:
  // interfaces:

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGMETADATAELEMENT

  // xxx I wish we could use virtual inheritance
  NS_FORWARD_NSIDOMNODE_NO_CLONENODE(nsSVGElement::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGElement::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGElement::)
};

NS_IMPL_NS_NEW_SVG_ELEMENT(Metadata)


//----------------------------------------------------------------------
// nsISupports methods

NS_IMPL_ADDREF_INHERITED(nsSVGMetadataElement, nsSVGMetadataElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGMetadataElement, nsSVGMetadataElementBase)

NS_INTERFACE_MAP_BEGIN(nsSVGMetadataElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNode)
  NS_INTERFACE_MAP_ENTRY(nsIDOMElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGMetadataElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGMetadataElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGMetadataElementBase)


//----------------------------------------------------------------------
// Implementation

nsSVGMetadataElement::nsSVGMetadataElement(nsINodeInfo *aNodeInfo)
  : nsSVGMetadataElementBase(aNodeInfo)
{
}

nsSVGMetadataElement::~nsSVGMetadataElement()
{
}

nsresult
nsSVGMetadataElement::Init()
{
  return NS_OK;
}


//----------------------------------------------------------------------
// nsIDOMNode methods

NS_IMPL_DOM_CLONENODE_WITH_INIT(nsSVGMetadataElement)
