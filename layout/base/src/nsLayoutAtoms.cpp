/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
#include "nsLayoutAtoms.h"

// XXX make this be autogenerated. doh!

// media atoms
nsIAtom* nsLayoutAtoms::all;
nsIAtom* nsLayoutAtoms::aural;
nsIAtom* nsLayoutAtoms::braille;
nsIAtom* nsLayoutAtoms::embossed;
nsIAtom* nsLayoutAtoms::handheld;
nsIAtom* nsLayoutAtoms::print;
nsIAtom* nsLayoutAtoms::projection;
nsIAtom* nsLayoutAtoms::screen;
nsIAtom* nsLayoutAtoms::tty;
nsIAtom* nsLayoutAtoms::tv;

// name space atoms
nsIAtom* nsLayoutAtoms::htmlNameSpace;
nsIAtom* nsLayoutAtoms::xmlNameSpace;

// frame additional child lists
nsIAtom* nsLayoutAtoms::absoluteList;
nsIAtom* nsLayoutAtoms::bulletList;
nsIAtom* nsLayoutAtoms::colGroupList;
nsIAtom* nsLayoutAtoms::floaterList;


static nsrefcnt gRefCnt;

void nsLayoutAtoms::AddrefAtoms()
{
  if (0 == gRefCnt) {
    all = NS_NewAtom("ALL");
    aural = NS_NewAtom("AURAL");
    braille = NS_NewAtom("BRAILLE");
    embossed = NS_NewAtom("EMBOSSED");
    handheld = NS_NewAtom("HANDHELD");
    print = NS_NewAtom("PRINT");
    projection = NS_NewAtom("PROJECTION");
    screen = NS_NewAtom("SCREEN");
    tty = NS_NewAtom("TTY");
    tv = NS_NewAtom("TV");

    htmlNameSpace = NS_NewAtom("html");
    xmlNameSpace = NS_NewAtom("xml");

    absoluteList = NS_NewAtom("Absolute-list");
    bulletList = NS_NewAtom("Bullet-list");
    colGroupList = NS_NewAtom("ColGroup-list");
    floaterList = NS_NewAtom("Floater-list");
  }
  ++gRefCnt;
}

void nsLayoutAtoms::ReleaseAtoms()
{
  NS_PRECONDITION(gRefCnt != 0, "bad release atoms");
  if (--gRefCnt == 0) {
    NS_RELEASE(all);
    NS_RELEASE(aural);
    NS_RELEASE(braille);
    NS_RELEASE(embossed);
    NS_RELEASE(handheld);
    NS_RELEASE(print);
    NS_RELEASE(projection);
    NS_RELEASE(screen);
    NS_RELEASE(tty);
    NS_RELEASE(tv);
    
    NS_RELEASE(htmlNameSpace);
    NS_RELEASE(xmlNameSpace);

    NS_RELEASE(absoluteList);
    NS_RELEASE(bulletList);
    NS_RELEASE(colGroupList);
    NS_RELEASE(floaterList);
  }
}

