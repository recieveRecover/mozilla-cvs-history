/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * Original Author: Michael F. Judge (mjudge@netscape.com)
 *
 * Contributor(s): 
 */


#include "nsISupports.h"


#ifndef nsIAutoCopyService_h__
#define nsIAutoCopyService_h__

// {558B93CD-95C1-417d-A66E-F9CA66DC98A8}
#define NS_IAUTOCOPYSERVICE_IID \
{ 0x558b93cd, 0x95c1, 0x417d, { 0xa6, 0x6e, 0xf9, 0xca, 0x66, 0xdc, 0x98, 0xa8 } }

// {8775CA39-4072-4cc0-92D3-A7C2B820089C}
#define NS_AUTOCOPYSERVICE_CID
{ 0x8775ca39, 0x4072, 0x4cc0, { 0x92, 0xd3, 0xa7, 0xc2, 0xb8, 0x20, 0x8, 0x9c } }


class nsIDOMSelection;

class nsIAutoCopyService : public nsISupports
{
public:
  static const nsIID& GetIID() { static nsIID iid = NS_IAUTOCOPYSERVICE; return iid; }
  
  //This will add this service as a selection listener.
  NS_IMETHOD Listen(nsIDOMSelection *aDomSelection)=0;

};


#endif //nsIAutoCopyService_h__

