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
 * Contributor(s): 
 */
#include "nsTextContentChangeData.h"

// Create a new instance of nsTextContentChangeData with a refcnt of 1
nsresult
NS_NewTextContentChangeData(nsTextContentChangeData** aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  nsTextContentChangeData* it = new nsTextContentChangeData();
  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  it->mRefCnt++;
  *aResult = it;
  return NS_OK;
}

nsTextContentChangeData::nsTextContentChangeData()
  : mType(Insert),
    mOffset(0),
    mLength(0),
    mReplaceLength(0)
{
  NS_INIT_REFCNT();
}

nsTextContentChangeData::~nsTextContentChangeData()
{
}

NS_IMPL_ISUPPORTS1(nsTextContentChangeData, nsITextContentChangeData);


NS_IMETHODIMP
nsTextContentChangeData::GetChangeType(ChangeType* aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = mType;
  return NS_OK;
}

NS_IMETHODIMP
nsTextContentChangeData::GetReplaceData(PRInt32* aOffset,
                                        PRInt32* aSourceLength,
                                        PRInt32* aReplaceLength)
{
  NS_ENSURE_ARG_POINTER(aOffset);
  NS_ENSURE_ARG_POINTER(aSourceLength);
  NS_ENSURE_ARG_POINTER(aReplaceLength);
  *aOffset = mOffset;
  *aSourceLength = mLength;
  *aReplaceLength = mReplaceLength;
  return NS_OK;
}

NS_IMETHODIMP
nsTextContentChangeData::GetInsertData(PRInt32* aOffset,
                                       PRInt32* aInsertLength)
{
  NS_ENSURE_ARG_POINTER(aOffset);
  NS_ENSURE_ARG_POINTER(aInsertLength);
  *aOffset = mOffset;
  *aInsertLength = mLength;
  return NS_OK;
}

NS_IMETHODIMP
nsTextContentChangeData::GetAppendData(PRInt32* aOffset,
                                       PRInt32* aAppendLength)
{
  NS_ENSURE_ARG_POINTER(aOffset);
  NS_ENSURE_ARG_POINTER(aAppendLength);
  *aOffset = mOffset;
  *aAppendLength = mLength;
  return NS_OK;
}
