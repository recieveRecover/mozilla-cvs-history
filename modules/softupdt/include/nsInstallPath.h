/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#ifndef nsInstallPath_h__
#define nsInstallPath_h__

#include "prtypes.h"
#include "nsSoftwareUpdate.h"
#include "nsVersionInfo.h"
#include "nsFolderSpec.h"


PR_BEGIN_EXTERN_C

struct nsInstallPath {

public:

  /* Public Fields */

  /* Public Methods */

  /*  Constructor
   *   inSoftUpdate    - softUpdate object we belong to
   *   inVRName        - full path of the registry component
   *   inVInfo         - full version info
   *   inJarLocation   - location inside the JAR file
   *   folderspec      - FolderSpec of target file
   *   inPartialPath   - target file on disk relative to folderspec
   */
  nsInstallPatch(nsSoftwareUpdate* inSoftUpdate,
                 char* inVRName,
                 nsVersionInfo* inVInfo,
                 char* inJarLocation);
  
  nsInstallPatch(nsSoftwareUpdate* inSoftUpdate,
                 char* inVRName,
                 nsVersionInfo* inVInfo,
                 char* inJarLocation,
                 nsFolderSpec* folderSpec,
                 char* inPartialPath);
  
  ~nsInstallPath();
  
  void Prepare();
  
  /* Complete
   * Completes the install:
   * - move the patched file to the final location
   * - updates the registry
   */
  void Complete();
  
  void Abort();
  
  char* toString();
  
private:
  
  /* Private Fields */
  char* vrName;              // Registry name of the component
  nsVersionInfo* versionInfo;    // Version
  char* jarLocation;         // Location in the JAR
  char* patchURL;            // extracted location of diff (xpURL)
  char* targetfile;          // source and final file (native)
  char* patchedfile;         // temp name of patched file
  
  /* Private Methods */
  void checkPrivileges();
  char* NativePatch( char* srcfile, char* diffURL );
  PRInt32 NativeReplace( char* target, char* tmpfile );
  void   NativeDeleteFile( char* file );
  
};

PR_END_EXTERN_C

#endif /* nsInstallPath_h__ */
