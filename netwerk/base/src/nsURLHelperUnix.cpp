/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
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
 *   Alec Flett <alecf@netscape.com>
 *   Darin Fisher <darin@netscape.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

/* Unix-specific local file uri parsing */
#include "nsURLHelper.h"
#include "nsEscape.h"
#include "nsILocalFile.h"

nsresult
net_GetURLSpecFromFile(nsIFile *aFile, nsACString &result)
{
    // NOTE: This is identical to the implementation in nsURLHelperOSX.cpp

    nsresult rv;
    nsCAutoString ePath;

    // construct URL spec from native file path
    rv = aFile->GetNativePath(ePath);
    if (NS_FAILED(rv)) return rv;

    nsCAutoString escPath;
    NS_NAMED_LITERAL_CSTRING(prefix, "file://");
        
    // Escape the path with the directory mask
    if (NS_EscapeURL(ePath.get(), ePath.Length(), esc_Directory+esc_Forced, escPath))
        escPath.Insert(prefix, 0);
    else
        escPath.Assign(prefix + ePath);

    // esc_Directory does not escape the semicolons, so if a filename 
    // contains semicolons we need to manually escape them.
    escPath.ReplaceSubstring(";", "%3b");

    // XXX this should be unnecessary
    if (escPath.Last() != '/') {
        PRBool dir;
        rv = aFile->IsDirectory(&dir);
        if (NS_FAILED(rv))
            NS_WARNING(PromiseFlatCString(
                NS_LITERAL_CSTRING("Cannot tell if ") + escPath +
                NS_LITERAL_CSTRING(" is a directory or file")).get());
        else if (dir) {
            // make sure we have a trailing slash
            escPath += "/";
        }
    }
    
    result = escPath;
    return NS_OK;
}

nsresult
net_GetFileFromURLSpec(const nsACString &aURL, nsIFile **result)
{
    // NOTE: See also the implementation in nsURLHelperOSX.cpp,
    // which is based on this.

    nsresult rv;

    nsCOMPtr<nsILocalFile> localFile;
    rv = NS_NewNativeLocalFile(nsCString(), PR_TRUE, getter_AddRefs(localFile));
    if (NS_FAILED(rv))
      return rv;
    
    nsCAutoString directory, fileBaseName, fileExtension, path;

    rv = net_ParseFileURL(aURL, directory, fileBaseName, fileExtension);
    if (NS_FAILED(rv)) return rv;

    if (!directory.IsEmpty())
        NS_EscapeURL(directory, esc_Directory|esc_AlwaysCopy, path);
    if (!fileBaseName.IsEmpty())
        NS_EscapeURL(fileBaseName, esc_FileBaseName|esc_AlwaysCopy, path);
    if (!fileExtension.IsEmpty()) {
        path += '.';
        NS_EscapeURL(fileExtension, esc_FileExtension|esc_AlwaysCopy, path);
    }
    
    NS_UnescapeURL(path);

    // assuming path is encoded in the native charset
    rv = localFile->InitWithNativePath(path);
    if (NS_FAILED(rv)) return rv;

    NS_ADDREF(*result = localFile);
    return NS_OK;
}
