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

#include "IdlParameter.h"
#include <ostream.h>

ostream& operator<<(ostream &s, IdlParameter &aParameter)
{
  switch (aParameter.GetAttribute()) {
    case IdlParameter::INPUT:
      s << "in ";
      break;
    case IdlParameter::OUTPUT:
      s << "out ";
      break;
    case IdlParameter::INOUT:
      s << "inout ";
      break;
  }
  char type[128];
  aParameter.GetTypeAsString(type, 128);
  return s << type << " " << aParameter.GetName();
}

IdlParameter::IdlParameter()
{
  mAttribute = 0;
  mIsOptional = 0;
}

IdlParameter::~IdlParameter()
{
}

void IdlParameter::SetAttribute(CallAttribute aAttribute)
{
  mAttribute = aAttribute;
}

int IdlParameter::GetAttribute()
{
  return mAttribute;
}

void            
IdlParameter::SetIsOptional(int aIsOptional)
{
  mIsOptional = aIsOptional;
}

int             
IdlParameter::GetIsOptional()
{
  return mIsOptional;
}

