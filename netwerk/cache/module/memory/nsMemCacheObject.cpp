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

/* 
 * nsMemCacheObject
 *
 * Gagan Saksena 04/22/98
 * 
 */
#include "prtypes.h"
#include "nsMemCacheObject.h"
#include "nsMemStream.h"

nsMemCacheObject::~nsMemCacheObject()
{
    if (m_pNextObject)
    {
        delete m_pNextObject;
        m_pNextObject = 0;
    }

    if (m_pObject)
    {
	nsStream* pStream;
        m_pObject->GetStream(&pStream);
	if (pStream)
        	delete (nsMemStream*)pStream;
        delete m_pObject;
        m_pObject = 0;
    }
}
