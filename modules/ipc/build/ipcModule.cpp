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
 * The Original Code is Mozilla IPC.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Darin Fisher <darin@netscape.com>
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

#include "nsIGenericFactory.h"
#include "ipcService.h"
#include "ipcCID.h"
#include "ipcConfig.h"

//-----------------------------------------------------------------------------
// Define the contructor function for the objects
//
// NOTE: This creates an instance of objects by using the default constructor
//-----------------------------------------------------------------------------
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(ipcService, Init)

#ifndef IPC_USE_INET
#include "ipcSocketProvider.h"
NS_GENERIC_FACTORY_CONSTRUCTOR(ipcSocketProvider)
#endif

//-----------------------------------------------------------------------------
// Define a table of CIDs implemented by this module along with other
// information like the function to create an instance, contractid, and
// class name.
//-----------------------------------------------------------------------------
static const nsModuleComponentInfo components[] = {
  { IPC_SERVICE_CLASSNAME,
    IPC_SERVICE_CID,
    IPC_SERVICE_CONTRACTID,
    ipcServiceConstructor, },
#ifndef IPC_USE_INET
  { IPC_SOCKETPROVIDER_CLASSNAME,
    IPC_SOCKETPROVIDER_CID,
    NS_NETWORK_SOCKET_CONTRACTID_PREFIX "ipc",
    ipcSocketProviderConstructor, },
#endif
};

//-----------------------------------------------------------------------------
// Implement the NSGetModule() exported function for your module
// and the entire implementation of the module object.
//-----------------------------------------------------------------------------
NS_IMPL_NSGETMODULE(ipcModule, components)
