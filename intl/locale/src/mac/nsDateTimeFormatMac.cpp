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
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */

#include "nsIServiceManager.h"
#include "nsICharsetConverterManager.h"
#include "nsDateTimeFormatMac.h"
#include <Resources.h>
#include <IntlResources.h>
#include <DateTimeUtils.h>
#include <Script.h>
#include <TextUtils.h>
#include "nsIComponentManager.h"
#include "nsLocaleCID.h"
#include "nsILocaleService.h"
#include "nsIPlatformCharset.h"
#include "nsIMacLocale.h"
#include "nsCRT.h"
#include "nsCOMPtr.h"
#include "plstr.h"
#include "prmem.h"

static NS_DEFINE_IID(kIDateTimeFormatIID, NS_IDATETIMEFORMAT_IID);
static NS_DEFINE_CID(kMacLocaleFactoryCID, NS_MACLOCALEFACTORY_CID);
static NS_DEFINE_CID(kCharsetConverterManagerCID, NS_ICHARSETCONVERTERMANAGER_CID);
static NS_DEFINE_CID(kLocaleServiceCID, NS_LOCALESERVICE_CID); 
static NS_DEFINE_CID(kPlatformCharsetCID, NS_PLATFORMCHARSET_CID);


////////////////////////////////////////////////////////////////////////////////

static Intl1Hndl GetItl1Resource(short scriptcode, short regioncode)
{
	long itl1num;

	if (smRoman == scriptcode)
	{
		itl1num = regioncode;	// if smRoman, use regioncode to differenciate
	} else {
		// get itlb from currenty system script
		ItlbRecord **ItlbRecordHandle;
		ItlbRecordHandle = (ItlbRecord **)::GetResource('itlb', scriptcode);
		
		// get itl1 number from itlb resource, if possible
		// otherwise, use the one return from script manager, 
		// (Script manager won't update itl1 number when the change on the fly )
		if(ItlbRecordHandle != NULL)
		{
			if(*ItlbRecordHandle == NULL)
				::LoadResource((Handle)ItlbRecordHandle);
				
			if(*ItlbRecordHandle != NULL)
				itl1num = (*ItlbRecordHandle)->itlbDate;
			else
				itl1num = ::GetScriptVariable(scriptcode, smScriptDate);
		} else {	// Use this as fallback
			itl1num = ::GetScriptVariable(scriptcode, smScriptDate);
		}
	}
	
	// get itl1 resource 
	Intl1Hndl Itl1RecordHandle;
	Itl1RecordHandle = (Intl1Hndl)::GetResource('itl1', itl1num);
	return Itl1RecordHandle;
}

static Intl0Hndl GetItl0Resource(short scriptcode, short regioncode)
{
	long itl0num;

	if (smRoman == scriptcode)
	{
		itl0num = regioncode;	// if smRoman, use regioncode to differenciate
	} else {
		// get itlb from currenty system script
		ItlbRecord **ItlbRecordHandle;
		ItlbRecordHandle = (ItlbRecord **)::GetResource('itlb', scriptcode);
		
		// get itl0 number from itlb resource, if possible
		// otherwise, use the one return from script manager, 
		// (Script manager won't update itl1 number when the change on the fly )
		if(ItlbRecordHandle != NULL)
		{
			if(*ItlbRecordHandle == NULL)
				::LoadResource((Handle)ItlbRecordHandle);
				
			if(*ItlbRecordHandle != NULL)
				itl0num = (*ItlbRecordHandle)->itlbNumber;
			else
				itl0num = ::GetScriptVariable(scriptcode, smScriptNumber);
		} else {	// Use this as fallback
			itl0num = ::GetScriptVariable(scriptcode, smScriptNumber);
		}
	}
	
	// get itl1 resource 
	Intl0Hndl Itl0RecordHandle;
	Itl0RecordHandle = (Intl0Hndl)::GetResource('itl0', itl0num);
	return Itl0RecordHandle;
}

static void AbbrevWeekdayString(DateTimeRec &dateTime, Str255 weekdayString, Intl1Hndl Itl1RecordHandle )
{
	Boolean gotit = false;
	
	// If we can get itl1Resource, exam it.
	if(Itl1RecordHandle != NULL )
	{
		if(*Itl1RecordHandle == NULL)
			::LoadResource((Handle)Itl1RecordHandle);

		if(*Itl1RecordHandle == NULL)
		{
			weekdayString[0] = 0;
			return;
		}
		// if itl1 resource is in the itl1ExtRec format 
		// look at the additional table
		// See IM-Text Appendix B for details
		if((unsigned short)((*Itl1RecordHandle)->localRtn[0]) == 0xA89F)
		{	// use itl1ExtRect
			Itl1ExtRec **Itl1ExtRecHandle;
			Itl1ExtRecHandle = (Itl1ExtRec **) Itl1RecordHandle;
			
			// check abbrevDaysTableLength and abbrevDaysTableOffset
			if(((*Itl1ExtRecHandle)->abbrevDaysTableLength != 0) &&
			   ((*Itl1ExtRecHandle)->abbrevDaysTableOffset != 0))
			{	
				// use the additional table for abbreviation weekday name
				// Japanese use it.
				// Start Pointer access to Handle, no HLock since we don't
				// call any API here.
				// Be careful when you debug- don't move memory :)
				Ptr abTablePt;
				short itemlen;
				
				// Ok, change it back to range [0-6]
				short weekday = dateTime.dayOfWeek - 1;	
				
				abTablePt = (Ptr)(*Itl1ExtRecHandle);
				abTablePt +=  (*Itl1ExtRecHandle)->abbrevDaysTableOffset;
				
				// first 2 byte in the table should be the count.
				itemlen = (short) *((short*)abTablePt);	
				abTablePt += 2;	
				
				if(weekday < itemlen)
				{
					unsigned char len;
					short i;
					// iterate till we hit the weekday name we want
					for(i = 0 ; i < weekday ; i++)	
					{
						len = *abTablePt;
						// shift to the next one. don't forget the len byte itself.
						abTablePt += len + 1;	
					}
					// Ok, we got it, let's copy it.
					len = *abTablePt;
					::BlockMoveData(abTablePt,&weekdayString[0] ,len+1);
					gotit = true;
				}
			}
		}
		
		// didn't get it. Either it is not in itl1ExtRect format or it don't have
		// additional abbreviation table. 
		// use itl1Rect instead.
		if(! gotit)
		{	
			// get abbreviation length
			// not the length is not always 3. Some country use longer (say 4)
			// abbreviation.
			short abbrLen = (*Itl1RecordHandle)->abbrLen;
			// Fix  Traditional Chinese problem
			if(((((*Itl1RecordHandle)->intl1Vers) >> 8) == verTaiwan ) &&
			   (abbrLen == 4) &&
			   ((*Itl1RecordHandle)->days[0][0] == 6) && 
			   ((*Itl1RecordHandle)->days[1][0] == 6) && 
			   ((*Itl1RecordHandle)->days[2][0] == 6) && 
			   ((*Itl1RecordHandle)->days[3][0] == 6) && 
			   ((*Itl1RecordHandle)->days[4][0] == 6) && 
			   ((*Itl1RecordHandle)->days[5][0] == 6) && 
			   ((*Itl1RecordHandle)->days[6][0] == 6))
			{
				abbrLen = 6;
			}
			weekdayString[0] = abbrLen;
			// copy the weekday name with that abbreviation length
			::BlockMoveData(&((*Itl1RecordHandle)->days[dateTime.dayOfWeek-1][1]),
				 &weekdayString[1] , abbrLen);
			gotit = true;
		}
	}
	else 
	{	// cannot get itl1 resource, return with null string.
		weekdayString[0] = 0;
	}
}


////////////////////////////////////////////////////////////////////////////////


NS_IMPL_ISUPPORTS(nsDateTimeFormatMac, kIDateTimeFormatIID);

nsresult nsDateTimeFormatMac::Initialize(nsILocale* locale)
{
  PRUnichar *aLocaleUnichar = NULL;
  nsString aCategory("NSILOCALE_TIME");
  nsresult res;

  // use cached info if match with stored locale
  if (NULL == locale) {
    if (mLocale.Length() && mLocale.EqualsIgnoreCase(mAppLocale)) {
      return NS_OK;
    }
  }
  else {
    res = locale->GetCategory(aCategory.GetUnicode(), &aLocaleUnichar);
    if (NS_SUCCEEDED(res) && NULL != aLocaleUnichar) {
      if (mLocale.Length() && mLocale.EqualsIgnoreCase(aLocaleUnichar)) {
        nsAllocator::Free(aLocaleUnichar);
        return NS_OK;
      }
      nsAllocator::Free(aLocaleUnichar);
    }
  }

  mScriptcode = smSystemScript;
  mLangcode = langEnglish;
  mRegioncode = verUS;
  mCharset.SetString("ISO-8859-1");
  

  // get locale string, use app default if no locale specified
  if (NULL == locale) {
    NS_WITH_SERVICE(nsILocaleService, localeService, kLocaleServiceCID, &res);
    if (NS_SUCCEEDED(res)) {
      nsILocale *appLocale;
      res = localeService->GetApplicationLocale(&appLocale);
      if (NS_SUCCEEDED(res)) {
        res = appLocale->GetCategory(aCategory.GetUnicode(), &aLocaleUnichar);
        if (NS_SUCCEEDED(res) && NULL != aLocaleUnichar) {
          mAppLocale.SetString(aLocaleUnichar); // cache app locale name
        }
        appLocale->Release();
      }
    }
  }
  else {
    res = locale->GetCategory(aCategory.GetUnicode(), &aLocaleUnichar);
  }

  // Get a script code and charset name from locale, if available
  if (NS_SUCCEEDED(res) && NULL != aLocaleUnichar) {
    mLocale.SetString(aLocaleUnichar); // cache locale name
    nsAllocator::Free(aLocaleUnichar);

    nsCOMPtr <nsIMacLocale> macLocale = do_GetService(kMacLocaleFactoryCID, &res);
    if (NS_SUCCEEDED(res)) {
      res = macLocale->GetPlatformLocale(&mLocale, &mScriptcode, &mLangcode, &mRegioncode);
    }

    nsCOMPtr <nsIPlatformCharset> platformCharset = do_GetService(NS_PLATFORMCHARSET_PROGID, &res);
    if (NS_SUCCEEDED(res)) {
      PRUnichar* mappedCharset = NULL;
      res = platformCharset->GetDefaultCharsetForLocale(mLocale.GetUnicode(), &mappedCharset);
      if (NS_SUCCEEDED(res) && mappedCharset) {
        mCharset.SetString(mappedCharset);
        nsAllocator::Free(mappedCharset);
      }
    }
  }
  
  return res;
}

// performs a locale sensitive date formatting operation on the time_t parameter
nsresult nsDateTimeFormatMac::FormatTime(nsILocale* locale, 
                                      const nsDateFormatSelector  dateFormatSelector, 
                                      const nsTimeFormatSelector timeFormatSelector, 
                                      const time_t  timetTime, 
                                      nsString& stringOut)
{
  return FormatTMTime(locale, dateFormatSelector, timeFormatSelector, localtime(&timetTime), stringOut);
}

// performs a locale sensitive date formatting operation on the struct tm parameter
nsresult nsDateTimeFormatMac::FormatTMTime(nsILocale* locale, 
                                           const nsDateFormatSelector  dateFormatSelector, 
                                           const nsTimeFormatSelector timeFormatSelector, 
                                           const struct tm*  tmTime, 
                                           nsString& stringOut)
{
  DateTimeRec macDateTime;
  Str255 timeString, dateString;
  int32 dateTime;	
  nsresult res;

  // set up locale data
  (void) Initialize(locale);
  
  // return, nothing to format
  if (dateFormatSelector == kDateFormatNone && timeFormatSelector == kTimeFormatNone) {
    stringOut.SetString("");
    return NS_OK;
  }

  stringOut.SetString(asctime(tmTime));	// set the default string, in case for API/conversion errors
  
  // convert struct tm to input format of mac toolbox call
  NS_ASSERTION(tmTime->tm_mon >= 0, "tm is not set correctly");
  NS_ASSERTION(tmTime->tm_mday >= 1, "tm is not set correctly");
  NS_ASSERTION(tmTime->tm_hour >= 0, "tm is not set correctly");
  NS_ASSERTION(tmTime->tm_min >= 0, "tm is not set correctly");
  NS_ASSERTION(tmTime->tm_sec >= 0, "tm is not set correctly");
  NS_ASSERTION(tmTime->tm_wday >= 0, "tm is not set correctly");

  macDateTime.year = tmTime->tm_year + 1900;	

  // Mac use 1 for Jan and 12 for Dec
  // tm use 0 for Jan and 11 for Dec 
  macDateTime.month = tmTime->tm_mon + 1;	
  macDateTime.day = tmTime->tm_mday;
  macDateTime.hour = tmTime->tm_hour;
  macDateTime.minute = tmTime->tm_min;
  macDateTime.second = tmTime->tm_sec;

  // Mac use 1 for sunday 7 for saturday
  // tm use 0 for sunday 6 for saturday 
  macDateTime.dayOfWeek = tmTime->tm_wday +1 ; 

  ::DateToSeconds( &macDateTime, (unsigned long *) &dateTime);
  
  Handle itl1Handle = (Handle) GetItl1Resource(mScriptcode, mRegioncode);
  Handle itl0Handle = (Handle) GetItl0Resource(mScriptcode, mRegioncode);
  NS_ASSERTION(itl1Handle && itl0Handle, "failed to get itl handle");

  // get time string
  if (timeFormatSelector != kTimeFormatNone) {
    // modify itl0 to force 24 hour time cycle !
    if (timeFormatSelector == kTimeFormatSecondsForce24Hour || 
        timeFormatSelector == kTimeFormatNoSecondsForce24Hour) {
      Intl0Hndl itl0HandleToModify = (Intl0Hndl) itl0Handle;
      UInt8 timeCycle = (**itl0HandleToModify).timeCycle;
      (**itl0HandleToModify).timeCycle = timeCycle24;
      ::TimeString(dateTime, (timeFormatSelector == kTimeFormatSeconds), timeString, itl0Handle);
      (**itl0HandleToModify).timeCycle = timeCycle;
    }
    else {
      ::TimeString(dateTime, (timeFormatSelector == kTimeFormatSeconds), timeString, itl0Handle);
    }
  }
  
  // get date string
  switch (dateFormatSelector) {
    case kDateFormatLong:
      ::DateString(dateTime, abbrevDate, dateString, itl1Handle);
      break;
    case kDateFormatShort:
      ::DateString(dateTime, shortDate, dateString, itl0Handle);
      break;
    case kDateFormatYearMonth:
      dateString[0] =  strftime((char*)&dateString[1],254,"%y/%m",tmTime);
      break;
    case kDateFormatWeekday:
      AbbrevWeekdayString(macDateTime, dateString, (Intl1Hndl)itl1Handle);
      // try fallback if it return with null string.
      if(dateString[0] == 0) {	//	cannot get weekdayString from itl1 , try fallback
        dateString[0] =  strftime((char*)&dateString[1],254,"%a",tmTime);
      }
      break;
  }
  
  // construct a C string
  char *aBuffer;
  if (dateFormatSelector != kDateFormatNone && timeFormatSelector != kTimeFormatNone) {
    aBuffer = p2cstr(dateString);
    strcat(aBuffer, " ");
    strcat(aBuffer, p2cstr(timeString));
  }
  else if (dateFormatSelector != kDateFormatNone) {
    aBuffer = p2cstr(dateString);
  }
  else if (timeFormatSelector != kTimeFormatNone) {
    aBuffer = p2cstr(timeString);
  }


  // convert result to unicode
  NS_WITH_SERVICE(nsICharsetConverterManager, ccm, kCharsetConverterManagerCID, &res);
  if(NS_SUCCEEDED(res) && ccm) {
    nsCOMPtr <nsIUnicodeDecoder> decoder;
    res = ccm->GetUnicodeDecoder(&mCharset, getter_AddRefs(decoder));
    if(NS_SUCCEEDED(res) && decoder) {
      PRInt32 unicharLength = 0;
      PRInt32 srcLength = (PRInt32) PL_strlen(aBuffer);
      res = decoder->GetMaxLength(aBuffer, srcLength, &unicharLength);
      PRUnichar *unichars = new PRUnichar [ unicharLength ];
  
      if (nsnull != unichars) {
        res = decoder->Convert(aBuffer, &srcLength,
                               unichars, &unicharLength);
        if (NS_SUCCEEDED(res)) {
          stringOut.SetString(unichars, unicharLength);
        }
      }
      delete [] unichars;
    }    
  }
  
  return res;
}

// performs a locale sensitive date formatting operation on the PRTime parameter
nsresult nsDateTimeFormatMac::FormatPRTime(nsILocale* locale, 
                                           const nsDateFormatSelector  dateFormatSelector, 
                                           const nsTimeFormatSelector timeFormatSelector, 
                                           const PRTime  prTime, 
                                           nsString& stringOut)
{
  PRExplodedTime explodedTime;
  PR_ExplodeTime(prTime, PR_LocalTimeParameters, &explodedTime);

  return FormatPRExplodedTime(locale, dateFormatSelector, timeFormatSelector, &explodedTime, stringOut);
}

// performs a locale sensitive date formatting operation on the PRExplodedTime parameter
nsresult nsDateTimeFormatMac::FormatPRExplodedTime(nsILocale* locale, 
                                                   const nsDateFormatSelector  dateFormatSelector, 
                                                   const nsTimeFormatSelector timeFormatSelector, 
                                                   const PRExplodedTime*  explodedTime, 
                                                   nsString& stringOut)
{
  struct tm  tmTime;
  nsCRT::memset( &tmTime, 0, sizeof(tmTime) );

  tmTime.tm_yday = explodedTime->tm_yday;
  tmTime.tm_wday = explodedTime->tm_wday;
  tmTime.tm_year = explodedTime->tm_year;
  tmTime.tm_year -= 1900;
  tmTime.tm_mon = explodedTime->tm_month;
  tmTime.tm_mday = explodedTime->tm_mday;
  tmTime.tm_hour = explodedTime->tm_hour;
  tmTime.tm_min = explodedTime->tm_min;
  tmTime.tm_sec = explodedTime->tm_sec;

  return FormatTMTime(locale, dateFormatSelector, timeFormatSelector, &tmTime, stringOut);
}

