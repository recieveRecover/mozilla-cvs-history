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
 * The Original Code is Golden Hills Computer Services code.
 *
 * The Initial Developer of the Original Code is
 * Brian Stell <bstell@ix.netcom.com>.
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Brian Stell <bstell@ix.netcom.com>.
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
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

#ifndef TYPE8_H
#define TYPE8_H

#include <stdio.h>
#include "nspr.h"
#ifdef MOZ_ENABLE_XFT
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_CACHE_H
#include FT_CACHE_IMAGE_H
#include FT_OUTLINE_H
// macros to handle FreeType2 26.6 numbers (26 bit number with 6 bit fraction)
#define FT_REG_TO_16_16(x) ((x)<<16)
#ifndef FT_MulFix
#define FT_MulFix(v, s) (((v)*(s))>>16)
#endif
#define FT_ROUND(x) (((x) + 32) & ~63) // 63 = 2^6 - 1
#define FT_TRUNC(x) ((x) >> 6)
#define FT_DESIGN_UNITS_TO_PIXELS(v, s) FT_TRUNC(FT_ROUND(FT_MulFix((v) , (s))))
#else /* MOZ_ENABLE_FREETYPE2 */
#include "nsIFreeType2.h"
#endif /* MOZ_ENABLE_XFT */

void AddCIDCheckCode(FILE *aFile);
PRBool FT2SubsetToType8(FT_Face aFace, const PRUnichar *aCharIDs,
                        PRUint32 aNumChars, int aWmode, FILE *aFile);
char*  FT2ToType8CidFontName(FT_Face aFace, int aWmode);


#endif /* TYPE8_H */
