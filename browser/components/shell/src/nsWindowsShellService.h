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
 * The Original Code is Shell Service.
 *
 * The Initial Developer of the Original Code is mozilla.org.
 * Portions created by the Initial Developer are Copyright (C) 2004
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Ben Goodger    <ben@mozilla.org>
 *  Aaron Kaluszka <ask@swva.net>
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

#ifndef nswindowsshellservice_h____
#define nswindowsshellservice_h____

#include "nscore.h"
#include "nsIWindowsShellService.h"
#include "nsIObserver.h"
#include "nsIGenericFactory.h"

#include <windows.h>

class nsWindowsShellService : public nsIWindowsShellService,
                              public nsIObserver
{
public:
  nsWindowsShellService();
  virtual ~nsWindowsShellService() {};

  NS_DECL_ISUPPORTS
  NS_DECL_NSISHELLSERVICE
  NS_DECL_NSIWINDOWSSHELLSERVICE
  NS_DECL_NSIOBSERVER

  static NS_METHOD Register(nsIComponentManager *aCompMgr, nsIFile *aPath, const char *registryLocation,
                            const char *componentType, const nsModuleComponentInfo *info);

protected:
  PRBool    GetMailAccountKey(HKEY* aResult);
  void      SetRegKey(const char* aKeyName, const char* aValueName, 
                      const char* aValue, PRBool aBackup, HKEY aBackupKey,
                      PRBool aReplaceExisting, PRBool aForAllUsers);
  DWORD     DeleteRegKey(HKEY baseKey, const char *keyName);

  nsresult  RegisterDDESupport();
  nsresult  UnregisterDDESupport();

private:
  PRBool    mCheckedThisSession;
};

#endif // nswindowsshellservice_h____
