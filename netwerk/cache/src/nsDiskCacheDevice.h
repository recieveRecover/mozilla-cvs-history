/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is nsMemoryCacheDevice.h, released February 20, 2001.
 * 
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 2001 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s): 
 *    Gordon Sheridan, 20-February-2001
 */

#ifndef _nsDiskCacheDevice_h_
#define _nsDiskCacheDevice_h_

#include "nsCacheDevice.h"


class nsDiskCacheDevice : public nsCacheDevice
{
public:
    enum { kDiskCacheID = 0x4469736B };  // 'Disk'

    nsDiskCacheDevice();
    virtual ~nsDiskCacheDevice();

    nsresult  Init();

    static nsresult  Create(nsCacheDevice **result);

    virtual const char *   GetDeviceID(void);
    virtual nsCacheEntry * FindEntry( nsCString * key );
    virtual nsresult       DeactivateEntry( nsCacheEntry * entry );
    virtual nsresult       BindEntry( nsCacheEntry * entry );

    virtual nsresult GetTransportForEntry( nsCacheEntry * entry,
                                           nsITransport **transport );

    virtual nsresult OnDataSizeChanged( nsCacheEntry * entry );

private:

};


#endif // _nsDiskCacheDevice_h_
