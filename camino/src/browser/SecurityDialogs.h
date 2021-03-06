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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Brian Ryner <bryner@brianryner.com>
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

#ifndef __SecurityDialogs_h__
#define __SecurityDialogs_h__

#include "nsIClientAuthDialogs.h"
#include "nsITokenPasswordDialogs.h"
#include "nsICertificateDialogs.h"
#include "nsISecurityWarningDialogs.h"
#include "nsITokenDialogs.h"
#include "nsIDOMCryptoDialogs.h"
#include "nsIGenKeypairInfoDlg.h"

#include "nsIStringBundle.h"
#include "nsCOMPtr.h"

class SecurityDialogs : public nsICertificateDialogs,
                        public nsITokenPasswordDialogs,
                        public nsIClientAuthDialogs,
                        public nsISecurityWarningDialogs,
                        public nsITokenDialogs,
                        public nsIDOMCryptoDialogs,
                        public nsIGeneratingKeypairInfoDialogs
{
public:
  SecurityDialogs();
  virtual ~SecurityDialogs();

  NS_DECL_ISUPPORTS;
  NS_DECL_NSICERTIFICATEDIALOGS;
  NS_DECL_NSITOKENPASSWORDDIALOGS;
  NS_DECL_NSICLIENTAUTHDIALOGS;
  NS_DECL_NSISECURITYWARNINGDIALOGS;
  NS_DECL_NSITOKENDIALOGS;
  NS_DECL_NSIDOMCRYPTODIALOGS;
  NS_DECL_NSIGENERATINGKEYPAIRINFODIALOGS;

private:
  nsresult EnsureSecurityStringBundle();

  nsresult AlertDialog(nsIInterfaceRequestor* ctx, const char* prefName,
                       const PRUnichar* messageName,
                       const PRUnichar* showAgainName);

  nsCOMPtr<nsIStringBundle> mSecurityStringBundle;
};

#endif
