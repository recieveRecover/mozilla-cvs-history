//* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * The Original Code is Mozilla Annotation Service
 *
 * The Initial Developer of the Original Code is
 * Google Inc.
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Brett Wilson <brettw@gmail.com> (original author)
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

#include "nsIAnnotationService.h"
#include "nsCOMArray.h"
#include "nsCOMPtr.h"
#include "mozIStorageService.h"
#include "mozIStorageConnection.h"

class nsAnnotationService : public nsIAnnotationService
{
public:
  nsAnnotationService();

  nsresult Init();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIANNOTATIONSERVICE

private:
  ~nsAnnotationService();

protected:
  nsCOMPtr<mozIStorageService> mDBService;
  nsCOMPtr<mozIStorageConnection> mDBConn;

  nsCOMPtr<mozIStorageStatement> mDBSetAnnotation;
  nsCOMPtr<mozIStorageStatement> mDBGetAnnotation;
  nsCOMPtr<mozIStorageStatement> mDBGetAnnotationFromURI;
  nsCOMPtr<mozIStorageStatement> mDBAddAnnotation;

  nsCOMArray<nsIAnnotationObserver> mObservers;

  static const int kAnnoIndex_ID;
  static const int kAnnoIndex_Page;
  static const int kAnnoIndex_Name;
  static const int kAnnoIndex_MimeType;
  static const int kAnnoIndex_Content;
  static const int kAnnoIndex_Flags;
  static const int kAnnoIndex_Expiration;

  nsresult HasAnnotationInternal(PRInt64 aURLID, const nsACString& aName,
                                 PRBool* hasAnnotation, PRInt64* annotationID);
  nsresult StartGetAnnotationFromURI(nsIURI* aURI,
                                     const nsACString& aName);
  nsresult StartSetAnnotation(nsIURI* aURI,
                              const nsACString& aName,
                              PRInt32 aFlags, PRInt32 aExpiration,
                              mozIStorageStatement** aStatement);
  void CallSetObservers(nsIURI* aURI, const nsACString& aName);
};
