/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are Copyright (C) 1998
 * Netscape Communications Corporation.  All Rights Reserved.
 */

#include "nsEUCTWToUnicode.h"

//----------------------------------------------------------------------
// Global functions and data [declaration]

static PRUint16 g_EUCTWMappingTable[] = {
#include "cns_1.ut"
};

static PRInt16 g_EUCTWShiftTable[] =  {
  2, uMultibytesCharset,  
  ShiftCell(u1ByteChar,   1, 0x00, 0x7F, 0x00, 0x00, 0x00, 0x7F),
  ShiftCell(u2BytesChar,  2, 0xA1, 0xFE, 0xA1, 0x40, 0xFE, 0xFE)
};

//----------------------------------------------------------------------
// Class nsEUCTWToUnicode [implementation]

nsEUCTWToUnicode::nsEUCTWToUnicode() 
: nsTableDecoderSupport((uShiftTable*) &g_EUCTWShiftTable, 
                        (uMappingTable*) &g_EUCTWMappingTable)
{
}

nsresult nsEUCTWToUnicode::CreateInstance(nsISupports ** aResult) 
{
  *aResult = new nsEUCTWToUnicode();
  return (*aResult == NULL)? NS_ERROR_OUT_OF_MEMORY : NS_OK;
}

//----------------------------------------------------------------------
// Subclassing of nsTableDecoderSupport class [implementation]

NS_IMETHODIMP nsEUCTWToUnicode::GetMaxLength(const char * aSrc, 
                                              PRInt32 aSrcLength, 
                                              PRInt32 * aDestLength)
{
  // we are a single byte to Unicode converter, so...
  *aDestLength = aSrcLength;
  return NS_OK_UDEC_EXACTLENGTH;
}
