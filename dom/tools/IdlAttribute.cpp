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

#include "IdlAttribute.h"
#include <ostream.h>

ostream& operator<<(ostream &s, IdlAttribute &aAttribute)
{
  if (aAttribute.GetReadOnly()) {
    s << "readonly ";
  }
  s << "attribute ";

  char type[128];
  aAttribute.GetTypeAsString(type, 128);
  return s << type << " " << aAttribute.GetName() << ";";
}

IdlAttribute::IdlAttribute()
{
  mReadOnly = 0;
}

IdlAttribute::~IdlAttribute()
{
}

void IdlAttribute::SetReadOnly(int aReadOnlyFlag)
{
  mReadOnly = aReadOnlyFlag;
}

int IdlAttribute::GetReadOnly()
{
  return mReadOnly;
}
