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
 * The Original Code is mozilla.org code.
 * 
 * The Initial Developer of the Original Code is Christopher Blizzard.
 * Portions created by Christopher Blizzard are Copyright (C)
 * Christopher Blizzard.  All Rights Reserved.
 * 
 * Contributor(s):
 *   Christopher Blizzard <blizzard@mozilla.org>
 */

#ifndef nsAppShell_h__
#define nsAppShell_h__

#include "nsIAppShell.h"
#include "nsIEventQueue.h"
#include "nsCOMPtr.h"

class nsAppShell : public nsIAppShell {
 public:

  nsAppShell();
  virtual ~nsAppShell();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIAPPSHELL

 private:

  nsCOMPtr<nsIEventQueue> mEventQueue;
};

#endif /* nsAppShell_h__ */
