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
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Johnny Stenback <jst@mozilla.jstenback.com> (original author)
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

#include "nsLSSerializer.h"
#include "nsDOMSerializer.h"
#include "nsIDOMLSSerializerFilter.h"
#include "nsIDOMDocument.h"
#include "nsIDOMDOMImplementation.h"
#include "nsReadableUtils.h"
#include "nsCRT.h"

#include "nsIDOMClassInfo.h"

// QueryInterface implementation for nsLSSerializer
NS_INTERFACE_MAP_BEGIN(nsLSSerializer)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMLSSerializer)
  NS_INTERFACE_MAP_ENTRY(nsIDOMLSSerializer)
  NS_INTERFACE_MAP_ENTRY_EXTERNAL_DOM_CLASSINFO(LSSerializer)
NS_INTERFACE_MAP_END


NS_IMPL_ADDREF(nsLSSerializer)
NS_IMPL_RELEASE(nsLSSerializer)

nsLSSerializer::nsLSSerializer()
  : mNewLine(NS_LINEBREAK, NS_LINEBREAK_LEN)
{
}

nsLSSerializer::~nsLSSerializer()
{
}

NS_IMETHODIMP
nsLSSerializer::GetDomConfig(nsIDOMDOMConfiguration * *aDomConfig)
{
  *aDomConfig = nsnull;

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsLSSerializer::GetNewLine(nsAString & aNewLine)
{
  CopyUTF8toUTF16(mNewLine, aNewLine);

  return NS_OK;
}

NS_IMETHODIMP
nsLSSerializer::SetNewLine(const nsAString & aNewLine)
{
  CopyUTF16toUTF8(aNewLine, mNewLine);

  return NS_OK;
}

NS_IMETHODIMP
nsLSSerializer::GetFilter(nsIDOMLSSerializerFilter * *aFilter)
{
  *aFilter = mFilter;

  return NS_OK;
}

NS_IMETHODIMP
nsLSSerializer::SetFilter(nsIDOMLSSerializerFilter * aFilter)
{
  mFilter = aFilter;

  return NS_OK;
}

NS_IMETHODIMP
nsLSSerializer::Write(nsIDOMNode *nodeArg, nsIDOMLSOutput *destination,
                      PRBool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsLSSerializer::WriteToURI(nsIDOMNode *nodeArg, const nsAString & uri,
                           PRBool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsLSSerializer::WriteToString(nsIDOMNode *nodeArg, nsAString & _retval)
{
  return mInnerSerializer->SerializeToString(nodeArg, _retval);
}

nsresult
nsLSSerializer::Init()
{
  mInnerSerializer = new nsDOMSerializer();

  return mInnerSerializer ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}
