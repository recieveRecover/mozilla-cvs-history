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
/*	sbconvtb.c	*/
/*
	Function that handle single byte Conversion table
*/
#include "intlpriv.h"

/* ------------------------------------------------*
		XP_MAC
 * ------------------------------------------------*/
#ifdef XP_MAC

PUBLIC char ** 
INTL_GetSingleByteTable(int16 from_csid, int16 to_csid, int32 resourceid)
{
	return FE_GetSingleByteTable(from_csid, to_csid,resourceid);
}
PUBLIC char *INTL_LockTable(char **cvthdl)
{
	return *cvthdl;
}
PUBLIC void INTL_FreeSingleByteTable(char **cvthdl) 
{
	FE_FreeSingleByteTable(cvthdl);
}
#endif

/* ------------------------------------------------*
		XP_WIN
 * ------------------------------------------------*/
#if defined(XP_WIN) || defined(XP_OS2)



PUBLIC char ** 
INTL_GetSingleByteTable(int16 from_csid, int16 to_csid, int32 resourceid)
{
	return FE_GetSingleByteTable(from_csid, to_csid,resourceid);
}
PUBLIC char *INTL_LockTable(char **cvthdl)
{
	return FE_LockTable(cvthdl);
}
PUBLIC void INTL_FreeSingleByteTable(char **cvthdl) 
{
	FE_FreeSingleByteTable(cvthdl);
}
#endif

/* ------------------------------------------------*
		XP_UNIX
 * ------------------------------------------------*/
#ifdef XP_UNIX
/* TRANSLATION RESOURCES */
/* Tables for Win CP1252 -> ISO 8859-1          */
PRIVATE unsigned char cp1252_to_iso8859_1[] = {
/*8x*/  '?', '?', ',', 'f', '?', '?', '?', '?', '^', '?', 'S', '<', '?', '?', '?', '?',
/*9x*/  '?', '?', '?', '?', '?', '*', '-', '-', '~', '?', 's', '>', '?', '?', '?', 'Y',
/*Ax*/ 0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,
/*Bx*/ 0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF,
/*Cx*/ 0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,
/*Dx*/ 0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF,
/*Ex*/ 0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,
/*Fx*/ 0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF
};

PRIVATE char *cp1252_to_iso8859_1_p = (char*)cp1252_to_iso8859_1;

/*     Translation 8859-5.txt -> koi8r.txt   */
/* There are total 61 character unmap !! */
PRIVATE unsigned char iso8859_5_to_koi8r[] = {
/*8x*/  '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?',
/*9x*/  '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?',
/*Ax*/ 0x9A,0xB3, '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?',
/*Bx*/ 0xE1,0xE2,0xF7,0xE7,0xE4,0xE5,0xF6,0xFA,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,0xF0,
/*Cx*/ 0xF2,0xF3,0xF4,0xF5,0xE6,0xE8,0xE3,0xFE,0xFB,0xFD,0xFF,0xF9,0xF8,0xFC,0xE0,0xF1,
/*Dx*/ 0xC1,0xC2,0xD7,0xC7,0xC4,0xC5,0xD6,0xDA,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,0xD0,
/*Ex*/ 0xD2,0xD3,0xD4,0xD5,0xC6,0xC8,0xC3,0xDE,0xDB,0xDD,0xDF,0xD9,0xD8,0xDC,0xC0,0xD1,
/*Fx*/  '?',0xA3, '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?',
};
PRIVATE char *iso8859_5_to_koi8r_p = (char*)iso8859_5_to_koi8r;

/*     Translation koi8r.txt -> 8859-5.txt   */
/* There are total 61 character unmap !! */
PRIVATE unsigned char koi8r_to_iso8859_5[] = {
/*8x*/  '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?',
/*9x*/  '?', '?', '?', '?', '?', '?', '?', '?', '?', '?',0xA0, '?', '?', '?', '?', '?',
/*Ax*/  '?', '?', '?',0xF1, '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?',
/*Bx*/  '?', '?', '?',0xA1, '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?',
/*Cx*/ 0xEE,0xD0,0xD1,0xE6,0xD4,0xD5,0xE4,0xD3,0xE5,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,
/*Dx*/ 0xDF,0xEF,0xE0,0xE1,0xE2,0xE3,0xD6,0xD2,0xEC,0xEB,0xD7,0xE8,0xED,0xE9,0xE7,0xEA,
/*Ex*/ 0xCE,0xB0,0xB1,0xC6,0xB4,0xB5,0xC4,0xB3,0xC5,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,
/*Fx*/ 0xBF,0xCF,0xC0,0xC1,0xC2,0xC3,0xB6,0xB2,0xCC,0xCB,0xB7,0xC8,0xCD,0xC9,0xC7,0xCA,
};
PRIVATE char *koi8r_to_iso8859_5_p = (char*)koi8r_to_iso8859_5;

/*  Translation cp1250 -> 8859-2 
 * There are 32 characters unmapped:
 * 80 - 89, 8B, 90 - 99, 9B, A6, A9, AB, AC, AE, B1, B5, B6, B7, BB
 */
PRIVATE unsigned char cp1250_to_iso8859_2[] = {
/*8x*/  '?', '?', '?', '?', '?', '?', '?', '?', '?', '?',0xA9, '?',0xA6,0xAB,0xAE,0xAC,
/*9x*/  '?', '?', '?', '?', '?', '?', '?', '?', '?', '?',0xB9, '?',0xB6,0xBB,0xBE,0xBC,
/*Ax*/ 0xA0,0xB7,0xA2,0xA3,0xA4,0xA1, '?',0xA7,0xA8, '?',0xAA, '?', '?',0xAD, '?',0xAF,
/*Bx*/ 0xB0, '?',0xB2,0xB3,0xB4, '?', '?', '?',0xB8,0xB1,0xBA, '?',0xA5,0xBD,0xB5,0xBF,
/*Cx*/ 0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,
/*Dx*/ 0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF,
/*Ex*/ 0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,
/*Fx*/ 0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF,
};
PRIVATE char *cp1250_to_iso8859_2_p = (char*)cp1250_to_iso8859_2;


/* Translation 8859-2 -> cp1250
 * There are 32 characters unmapped: 80 - 8F, 90 - 9F 
 */
PRIVATE unsigned char iso8859_2_to_cp1250[] = {
/*8x*/  '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?',
/*9x*/  '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?',
/*Ax*/ 0xA0,0xA5,0xA2,0xA3,0xA4,0xBC,0x8C,0xA7,0xA8,0x8A,0xAA,0x8D,0x8F,0xAD,0x8E,0xAF,
/*Bx*/ 0xB0,0xB9,0xB2,0xB3,0xB4,0xBE,0x9C,0xA1,0xB8,0x9A,0xBA,0x9D,0x9F,0xBD,0x9E,0xBF,
/*Cx*/ 0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,
/*Dx*/ 0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF,
/*Ex*/ 0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,
/*Fx*/ 0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF,
};
PRIVATE char *iso8859_2_to_cp1250_p = (char*)iso8859_2_to_cp1250;

/* Translation cp1251 -> 8859-5
 * There are 32 characters unmapped:
 * 82, 84 - 89, 8B, 91 - 99, 9B, A4 - A6, A9, AB, AC, AE, B0, B1, B4 - B7, BB,
 */
PRIVATE unsigned char cp1251_to_iso8859_5[] = {
/*8x*/ 0xA2,0xA3, '?',0xF3, '?', '?', '?', '?', '?', '?',0xA9, '?',0xAA,0xAC,0xAB,0xAF,
/*9x*/ 0xF2, '?', '?', '?', '?', '?', '?', '?', '?', '?',0xF9, '?',0xFA,0xFC,0xFB,0xFF,
/*Ax*/ 0xA0,0xAE,0xFE,0xA8, '?', '?', '?',0xFD,0xA1, '?',0xA4, '?', '?',0xAD, '?',0xA7,
/*Bx*/  '?', '?',0xA6,0xF6, '?', '?', '?', '?',0xF1,0xF0,0xF4, '?',0xF8,0xA5,0xF5,0xF7,
/*Cx*/ 0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF,
/*Dx*/ 0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,
/*Ex*/ 0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF,
/*Fx*/ 0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,
};
PRIVATE char *cp1251_to_iso8859_5_p = (char*)cp1251_to_iso8859_5;

/* Translation 8859-5 -> cp1251
 * There are 32 characters unmapped:
 * 80 - 8F, 90 - 9F
 */
PRIVATE unsigned char iso8859_5_to_cp1251[] = {
/*8x*/  '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?',
/*9x*/  '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?',
/*Ax*/ 0xA0,0xA8,0x80,0x81,0xAA,0xBD,0xB2,0xAF,0xA3,0x8A,0x8C,0x8E,0x8D,0xAD,0xA1,0x8F,
/*Bx*/ 0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,
/*Cx*/ 0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF,
/*Dx*/ 0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,
/*Ex*/ 0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF,
/*Fx*/ 0xB9,0xB8,0x90,0x83,0xBA,0xBE,0xB3,0xBF,0xBC,0x9A,0x9C,0x9E,0x9D,0xA7,0xA2,0x9F,
};
PRIVATE char *iso8859_5_to_cp1251_p = (char*)iso8859_5_to_cp1251;

/* Translation cp1253 -> 8859-7
 * There are 40 characters unmapped:
 * 80 - 8F, 90 - 9F, A4 - A5, AA, AE, B5 - B6, D2, FF
 */
PRIVATE unsigned char cp1253_to_iso8859_7[] = {
/*8x*/  '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?',
/*9x*/  '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?',
/*Ax*/ 0xA0,0xB5,0xB6,0xA3, '?', '?',0xA6,0xA7,0xA8,0xA9, '?',0xAB,0xAC,0xAD, '?',0xAF,
/*Bx*/ 0xB0,0xB1,0xB2,0xB3,0xB4, '?', '?',0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF,
/*Cx*/ 0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,
/*Dx*/ 0xD0,0xD1, '?',0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF,
/*Ex*/ 0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,
/*Fx*/ 0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE, '?',
};
PRIVATE char *cp1253_to_iso8859_7_p = (char*)cp1253_to_iso8859_7;

/* Translation 8859-7 -> cp1253
 * There are 40 characters unmapped:
 * 80 - 8F, 90 - 9F, A1 - A2, A4 - A5, AA, AE, D2, FF
 */
PRIVATE unsigned char iso8859_7_to_cp1253[] = {
/*8x*/  '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?',
/*9x*/  '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?',
/*Ax*/ 0xA0, '?', '?',0xA3, '?', '?',0xA6,0xA7,0xA8,0xA9, '?',0xAB,0xAC,0xAD, '?',0xAF,
/*Bx*/ 0xB0,0xB1,0xB2,0xB3,0xB4,0xA1,0xA2,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF,
/*Cx*/ 0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,
/*Dx*/ 0xD0,0xD1, '?',0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF,
/*Ex*/ 0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,
/*Fx*/ 0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE, '?',
};
PRIVATE char *iso8859_7_to_cp1253_p = (char*)iso8859_7_to_cp1253;

/*     Translation cp1251.txt -> koi8r.txt   */
PRIVATE unsigned char cp1251_to_koi8r[] = {
/*8x*/  '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?',
/*9x*/  '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?',
/*Ax*/  '?', '?', '?', '?', '?', '?', '?', '?',0xB3, '?', '?', '?', '?', '?', '?', '?',
/*Bx*/  '?', '?', '?', '?', '?', '?', '?', '?',0xA3, '?', '?', '?', '?', '?', '?', '?',
/*Cx*/ 0xE1,0xE2,0xF7,0xE7,0xE4,0xE5,0xF6,0xFA,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,0xF0,
/*Dx*/ 0xF2,0xF3,0xF4,0xF5,0xE6,0xE8,0xE3,0xFE,0xFB,0xFD,0xFF,0xF9,0xF8,0xFC,0xE0,0xF1,
/*Ex*/ 0xC1,0xC2,0xD7,0xC7,0xC4,0xC5,0xD6,0xDA,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,0xD0,
/*Fx*/ 0xD2,0xD3,0xD4,0xD5,0xC6,0xC8,0xC3,0xDE,0xDB,0xDD,0xDF,0xD9,0xD8,0xDC,0xC0,0xD1,
};
PRIVATE char *cp1251_to_koi8r_p = (char*)cp1251_to_koi8r;

/*     Translation koi8r.txt -> cp1251.txt   */
PRIVATE unsigned char koi8r_to_cp1251[] = {
/*8x*/  '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?',
/*9x*/  '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?',
/*Ax*/  '?', '?', '?',0xB8, '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?',
/*Bx*/  '?', '?', '?',0xA8, '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?',
/*Cx*/ 0xFE,0xE0,0xE1,0xF6,0xE4,0xE5,0xF4,0xE3,0xF5,0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,
/*Dx*/ 0xEF,0xFF,0xF0,0xF1,0xF2,0xF3,0xE6,0xE2,0xFC,0xFB,0xE7,0xF8,0xFD,0xF9,0xF7,0xFA,
/*Ex*/ 0xDE,0xC0,0xC1,0xD6,0xC4,0xC5,0xD4,0xC3,0xD5,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,
/*Fx*/ 0xCF,0xDF,0xD0,0xD1,0xD2,0xD3,0xC6,0xC2,0xDC,0xDB,0xC7,0xD8,0xDD,0xD9,0xD7,0xDA,
};
PRIVATE char *koi8r_to_cp1251_p = (char*)koi8r_to_cp1251;

PUBLIC char ** 
INTL_GetSingleByteTable(int16 from_csid, int16 to_csid, int32 resourceid)
{
	/* LATIN1 */
	if ((from_csid == CS_LATIN1) && (to_csid == CS_LATIN1)) {
		return &cp1252_to_iso8859_1_p;
	}

	/* LATIN2 */
	else if ((from_csid == CS_LATIN2) && (to_csid == CS_CP_1250)) {
		return &iso8859_2_to_cp1250_p;
	}
	else if ((from_csid == CS_CP_1250) && (to_csid == CS_LATIN2)) {
		return &cp1250_to_iso8859_2_p;
	}

	/* CYRILLIC */
	else if ((from_csid == CS_8859_5) && (to_csid == CS_KOI8_R)) {
		return &iso8859_5_to_koi8r_p;
	}
	else if ((from_csid == CS_KOI8_R) && (to_csid == CS_8859_5)) {
		return &koi8r_to_iso8859_5_p;
	}
	else if ((from_csid == CS_8859_5) && (to_csid == CS_CP_1251)) {
		return &iso8859_5_to_cp1251_p;
	}
	else if ((from_csid == CS_CP_1251) && (to_csid == CS_8859_5)) {
		return &cp1251_to_iso8859_5_p;
	}
	else if ((from_csid == CS_CP_1251) && (to_csid == CS_KOI8_R)) {
		return &cp1251_to_koi8r_p;
	}
	else if ((from_csid == CS_KOI8_R) && (to_csid == CS_CP_1251)) {
		return &koi8r_to_cp1251_p;
	}

	/* GREEK */
	else if ((from_csid == CS_8859_7) && (to_csid == CS_CP_1253)) {
		return &iso8859_7_to_cp1253_p;
	}
	else if ((from_csid == CS_CP_1253) && (to_csid == CS_8859_7)) {
		return &cp1253_to_iso8859_7_p;
	}

	/* others To Be Implement */
	else
		return(NULL);
}	
PUBLIC char *INTL_LockTable(char **cvthdl)
{
	/* no need to lock as the tables are static */
	return *cvthdl;
}
PUBLIC void INTL_FreeSingleByteTable(char **cvthdl) {
	/* no need to free as the tables are static */
}

#endif


unsigned char *
One2OneCCC (CCCDataObject obj, const unsigned char *s, int32 l)
{
	char **cvthdl;
	register unsigned char	*cp;
	char *pTable;

	cvthdl = (char **)INTL_GetSingleByteTable(INTL_GetCCCFromCSID(obj), INTL_GetCCCToCSID(obj), INTL_GetCCCCvtflag(obj));
	
	if (cvthdl != NULL)
	{
		pTable = INTL_LockTable(cvthdl);
		for (cp = (unsigned char *)s; cp < (unsigned char *)s + l; cp++)
		{
			if(*cp  & 0x80)
				*cp = pTable[(*cp & 0x7F)];
		}		
	}
	INTL_SetCCCLen(obj, l);
	INTL_FreeSingleByteTable(cvthdl);

	return((unsigned char *)s);
}

