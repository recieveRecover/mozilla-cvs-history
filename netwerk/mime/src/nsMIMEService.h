/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * There is a 1-to-many relationship between MIME-types and file extensions
 * I.e. there can be many file extensions that have the same mime type.
 */

#ifndef ___nsIMIMEService__h___
#define ___nsIMIMEService__h___

#include "nsIMIMEService.h"
#include "nsIURI.h"
#include "nsHashtable.h"


class nsMIMEService : public nsIMIMEService {

    // nsISupports methods
    NS_DECL_ISUPPORTS

    // nsMIMEService methods
    nsMIMEService();
    virtual ~nsMIMEService();

    // nsIMIMEService methods
    NS_DECL_NSIMIMESERVICE

private:
    nsresult AddMapping(const char* mimeType, 
                        const char* extension,
                        const char* description);
    nsresult InitFromURI(nsIURI *aUri);
    nsresult InitFromFile(const char *aFileName);
    nsresult InitFromHack();

    
    nsHashtable             *mInfoHashtable;
};

#endif // ___nsIMIMEService__h___
