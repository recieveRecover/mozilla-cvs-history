/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#ifndef nsFont_h___
#define nsFont_h___

#include "nscore.h"
#include "nsCoord.h"
#include "nsString.h"

// XXX we need a method to enumerate all of the possible fonts on the
// system across family, weight, style, size, etc. But not here!

// Enumerator callback function. Return PR_FALSE to stop
typedef PRBool (*nsFontFamilyEnumFunc)(const nsString& aFamily, PRBool aGeneric, void *aData);

// Font structure.
class nsFont {
public:
  // The family name of the font -- MUST be in lowercase!
  nsString name;

  // The style of font (normal, italic, oblique)
  PRUint8 style;

  // The variant of the font (normal, small-caps)
  PRUint8 variant;

  // The weight of the font (0-999)
  PRUint16 weight;

  // The decorations on the font (underline, overline,
  // line-through). The decorations can be binary or'd together.
  PRUint8 decorations;

  // The size of the font, in nscoord units
  nscoord size;

  // Initialize the font struct with an iso-latin1 name
  nsFont(const char* aName, PRUint8 aStyle, PRUint8 aVariant,
         PRUint16 aWeight, PRUint8 aDecoration, nscoord aSize);

  // Initialize the font struct with a (potentially) unicode name
  nsFont(const nsString& aName, PRUint8 aStyle, PRUint8 aVariant,
         PRUint16 aWeight, PRUint8 aDecoration, nscoord aSize);

  // Make a copy of the given font
  nsFont(const nsFont& aFont);

#ifdef DEBUG
// so we can set breakpoints on creation/deletion
  nsFont();

  ~nsFont();
#endif

  PRBool operator==(const nsFont& aOther) const {
    return Equals(aOther);
  }

  PRBool Equals(const nsFont& aOther) const ;

  nsFont& operator=(const nsFont& aOther);

  // Utility method to interpret name string
  // enumerates all families specified by this font only
  // returns PR_TRUE if completed, PF_FALSE if stopped
  // enclosing quotes will be removed, and whitespace compressed (as needed)
  PRBool EnumerateFamilies(nsFontFamilyEnumFunc aFunc, void* aData) const;
  void GetFirstFamily(nsString& aFamily) const;
};

#define NS_FONT_STYLE_NORMAL              0
#define NS_FONT_STYLE_ITALIC              1
#define NS_FONT_STYLE_OBLIQUE             2

#define NS_FONT_VARIANT_NORMAL            0
#define NS_FONT_VARIANT_SMALL_CAPS        1

#define NS_FONT_DECORATION_NONE           0x0
#define NS_FONT_DECORATION_UNDERLINE      0x1
#define NS_FONT_DECORATION_OVERLINE       0x2
#define NS_FONT_DECORATION_LINE_THROUGH   0x4

#define NS_FONT_WEIGHT_NORMAL             400
#define NS_FONT_WEIGHT_BOLD               700

#endif /* nsFont_h___ */
