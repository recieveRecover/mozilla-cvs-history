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


/* BEGIN NEW_STRING_LIB */
/* See the long commend in xpstring.xps for how this whole thing works */

/*#include <stdio.h>*/

/* Manually include the prefix file, so that MWCPPC picks it up */
#include "MacConfigInclude.h"

/*END NEW NEW_STRING_LIB*/

/* the prefix file should have already defined OTUNIXERRORS at this stage */
#include "OpenTransport.h"


/* BEGIN NEW_STRING_LIB */
#define RESOURCE_STR 1
/*END NEW NEW_STRING_LIB*/

#pragma export on

#include "allxpstr.h"

/*
 * We need this function as certain mac roms (9500/150) seem to have a bug in CFM where
 * they crash when loading fragments that have no code...
 */
void MacintoshForever( void );
void MacintoshForever( void )
{

}
