/*
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
 * The Original Code is the mozilla.org code.
 * 
 * The Initial Developer of the Original Code is Christopher Blizzard.
 * Portions created Christopher Blizzard are Copyright (C) Christopher
 * Blizzard.  All Rights Reserved.
 * 
 * Contributor(s):
 */

#ifndef nsBasePointerService_h_
#define nsBasePointerService_h_

#include <nsIPointerService.h>

class nsBasePointerService : public nsIPointerService {
 public:
  nsBasePointerService();
  virtual ~nsBasePointerService();

  // nsISupports
  NS_DECL_ISUPPORTS

  // nsIPointerService
  NS_IMETHOD WidgetUnderPointer(nsIWidget **_retval,
				PRUint32 *aXOffset, PRUint32 *aYOffset);

};

#endif /* nsBasePointerService_h_ */
