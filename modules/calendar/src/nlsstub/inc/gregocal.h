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

#ifndef gregocal_h__
#define gregocal_h__

#include "ptypes.h"
#include "calendar.h"

class TimeZone;
class Locale;

class NS_NLS GregorianCalendar : public Calendar
{

public:
  GregorianCalendar();
  ~GregorianCalendar();

  GregorianCalendar(ErrorCode& aSuccess);
  GregorianCalendar(TimeZone* aZoneToAdopt, ErrorCode& aSuccess);
  GregorianCalendar(const TimeZone& aZone, ErrorCode& aSuccess);
  GregorianCalendar(const Locale& aLocale, ErrorCode& aSuccess);
  GregorianCalendar(TimeZone* aZoneToAdopt, const Locale& aLocale, ErrorCode& aSuccess);
  GregorianCalendar(const TimeZone& aZone, const Locale& aLocale, ErrorCode& aSuccess);
  GregorianCalendar(PRInt32 aYear, PRInt32 aMonth, PRInt32 aDate, ErrorCode& aSuccess);
  GregorianCalendar(PRInt32 aYear, PRInt32 aMonth, PRInt32 aDate, PRInt32 aHour, PRInt32 aMinute, ErrorCode& aSuccess);
  GregorianCalendar(PRInt32 aYear, PRInt32 aMonth, PRInt32 aDate, PRInt32 aHour, PRInt32 aMinute, PRInt32 aSecond, ErrorCode& aSuccess);
  
  virtual void add(EDateFields aField, PRInt32 aAmount, ErrorCode& aStatus);

};




#endif
