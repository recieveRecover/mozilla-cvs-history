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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#include "nscore.h"
#include "nsIAllocator.h"
#include "plstr.h"
#include "stdio.h"
#include "nsIWalletService.h"
#include "nsIServiceManager.h"
#include "nsIDOMWindow.h"
#include "nsCOMPtr.h"
#include "nsIWebShell.h"
#include "nsIDocShell.h"
#include "nsIWebShellWindow.h"
#include "nsIScriptGlobalObject.h"
#include "nsWalletEditor.h"

static NS_DEFINE_IID(kWalletServiceCID, NS_WALLETSERVICE_CID);

////////////////////////////////////////////////////////////////////////

WalletEditorImpl::WalletEditorImpl()
{
  NS_INIT_REFCNT();
}

WalletEditorImpl::~WalletEditorImpl()
{
}

NS_IMPL_ISUPPORTS1(WalletEditorImpl, nsIWalletEditor);

NS_IMETHODIMP
WalletEditorImpl::GetValue(PRUnichar** aValue)
{
  NS_PRECONDITION(aValue != nsnull, "null ptr");
  if (!aValue) {
    return NS_ERROR_NULL_POINTER;
  }
  nsresult res;
  NS_WITH_SERVICE(nsIWalletService, walletservice, kWalletServiceCID, &res);
  if (NS_FAILED(res)) return res;
  nsAutoString walletList;
  res = walletservice->WALLET_PreEdit(walletList);
  if (NS_SUCCEEDED(res)) {
    *aValue = walletList.ToNewUnicode();
  }
  return res;
}

#if 0
static void DOMWindowToWebShellWindow(
              nsIDOMWindow *DOMWindow,
              nsCOMPtr<nsIWebShellWindow> *webWindow)
{
  if (!DOMWindow) {
    return; // with webWindow unchanged -- its constructor gives it a null ptr
  }
  nsCOMPtr<nsIScriptGlobalObject> globalScript(do_QueryInterface(DOMWindow));
  nsCOMPtr<nsIDocShell> docShell;
  if (globalScript) {
    globalScript->GetDocShell(getter_AddRefs(docShell));
  }
  nsCOMPtr<nsIWebShell> webshell(do_QueryInterface(docShell));
  nsCOMPtr<nsIWebShell> rootWebshell;
  if(!webshell)
   return;
  nsCOMPtr<nsIWebShellContainer> topLevelWindow;
  webshell->GetTopLevelWindow(getter_AddRefs(topLevelWindow));
  *webWindow = do_QueryInterface(topLevelWindow);
}
#endif /* 0 */

NS_IMETHODIMP
WalletEditorImpl::SetValue(const PRUnichar* aValue, nsIDOMWindow* win)
{
  /* process the value */
  NS_PRECONDITION(aValue != nsnull, "null ptr");
  if (! aValue) {
    return NS_ERROR_NULL_POINTER;
  }
  nsresult res;
  NS_WITH_SERVICE(nsIWalletService, walletservice, kWalletServiceCID, &res);
  if (NS_FAILED(res)) return res;
  nsAutoString walletList = aValue;
  res = walletservice->WALLET_PostEdit(walletList);
  return res;
}
