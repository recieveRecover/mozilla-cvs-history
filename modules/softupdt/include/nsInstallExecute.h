/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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

#ifndef nsInstallExecute_h__
#define nsInstallExecute_h__

#include "prtypes.h"
#include "nsSoftwareUpdate.h"

PR_BEGIN_EXTERN_C

struct nsInstallExecute  : public nsInstallObject {

public:

  /* Public Fields */

  /* Public Methods */

  /*	Constructor
   *	inJarLocation	- location inside the JAR file
   *	inZigPtr        - pointer to the ZIG *
   */
  nsInstallExecute(nsSoftwareUpdate* inSoftUpdate, char* inJarLocation, char* *errorMsg, char* inArgs);
  
  virtual ~nsInstallExecute();
  
  /* Prepare
   * Extracts	file out of	the	JAR	archive	into the temp directory
   */
  char* Prepare(void);
  
  /* Complete
   * Completes the install by executing the file
   * Security hazard: make sure we request the right permissions
   */
  char* Complete(void);
  
  void Abort(void);
  
  char* toString();
  
  /* should these be protected? */
  PRBool CanUninstall();
  PRBool RegisterPackageNode();
  
private:
  
  /* Private Fields */
  char* jarLocation; // Location in the JAR
  char* tempFile;    // temporary file location
  char* args;        // command line arguments
  char* cmdline;     // constructed command-line
  
  /* Private Methods */
  char* NativeComplete(void);
  void NativeAbort(void);
  
};

PR_END_EXTERN_C

#endif /* nsInstallExecute_h__ */
