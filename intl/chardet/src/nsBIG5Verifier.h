/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
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
/* 
 * DO NOT EDIT THIS DOCUMENT MANUALLY !!!
 * THIS FILE IS AUTOMATICALLY GENERATED BY THE TOOLS UNDER
 *    mozilla/intl/chardet/tools/
 * Please contact ftang@netscape.com or mozilla-i18n@mozilla.org
 * if you have any question. Thanks
 */
#include "nsVerifier.h"
static PRUint32 BIG5_cls [ 256 / 8 ] = {
PCK4BITS(0,1,1,1,1,1,1,1),  // 00 - 07 
PCK4BITS(1,1,1,1,1,1,0,0),  // 08 - 0f 
PCK4BITS(1,1,1,1,1,1,1,1),  // 10 - 17 
PCK4BITS(1,1,1,0,1,1,1,1),  // 18 - 1f 
PCK4BITS(1,1,1,1,1,1,1,1),  // 20 - 27 
PCK4BITS(1,1,1,1,1,1,1,1),  // 28 - 2f 
PCK4BITS(1,1,1,1,1,1,1,1),  // 30 - 37 
PCK4BITS(1,1,1,1,1,1,1,1),  // 38 - 3f 
PCK4BITS(2,2,2,2,2,2,2,2),  // 40 - 47 
PCK4BITS(2,2,2,2,2,2,2,2),  // 48 - 4f 
PCK4BITS(2,2,2,2,2,2,2,2),  // 50 - 57 
PCK4BITS(2,2,2,2,2,2,2,2),  // 58 - 5f 
PCK4BITS(2,2,2,2,2,2,2,2),  // 60 - 67 
PCK4BITS(2,2,2,2,2,2,2,2),  // 68 - 6f 
PCK4BITS(2,2,2,2,2,2,2,2),  // 70 - 77 
PCK4BITS(2,2,2,2,2,2,2,1),  // 78 - 7f 
PCK4BITS(4,4,4,4,4,4,4,4),  // 80 - 87 
PCK4BITS(4,4,4,4,4,4,4,4),  // 88 - 8f 
PCK4BITS(4,4,4,4,4,4,4,4),  // 90 - 97 
PCK4BITS(4,4,4,4,4,4,4,4),  // 98 - 9f 
PCK4BITS(4,3,3,3,3,3,3,3),  // a0 - a7 
PCK4BITS(3,3,3,3,3,3,3,3),  // a8 - af 
PCK4BITS(3,3,3,3,3,3,3,3),  // b0 - b7 
PCK4BITS(3,3,3,3,3,3,3,3),  // b8 - bf 
PCK4BITS(3,3,3,3,3,3,3,3),  // c0 - c7 
PCK4BITS(3,3,3,3,3,3,3,3),  // c8 - cf 
PCK4BITS(3,3,3,3,3,3,3,3),  // d0 - d7 
PCK4BITS(3,3,3,3,3,3,3,3),  // d8 - df 
PCK4BITS(3,3,3,3,3,3,3,3),  // e0 - e7 
PCK4BITS(3,3,3,3,3,3,3,3),  // e8 - ef 
PCK4BITS(3,3,3,3,3,3,3,3),  // f0 - f7 
PCK4BITS(3,3,3,3,3,3,3,0)   // f8 - ff 
};


static PRUint32 BIG5_st [ 3] = {
PCK4BITS(eError,eStart,eStart,     3,eError,eError,eError,eError),//00-07 
PCK4BITS(eError,eError,eItsMe,eItsMe,eItsMe,eItsMe,eItsMe,eError),//08-0f 
PCK4BITS(eError,eStart,eStart,eStart,eStart,eStart,eStart,eStart) //10-17 
};


static nsVerifier nsBIG5Verifier = {
     "BIG5",
    {
       eIdxSft4bits, 
       eSftMsk4bits, 
       eBitSft4bits, 
       eUnitMsk4bits, 
       BIG5_cls 
    },
    5,
    {
       eIdxSft4bits, 
       eSftMsk4bits, 
       eBitSft4bits, 
       eUnitMsk4bits, 
       BIG5_st 
    }
};
