/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * Copyright (C) 1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#include "msgCore.h"
#include "nsMsgSearchCore.h"
#include "nsMsgSearchAdapter.h"
#include "nsMsgSearchScopeTerm.h"
#include "nsMsgI18N.h"
#include "nsIPref.h"
#include "nsXPIDLString.h"
#include "nsMsgSearchTerm.h"
#include "nsMsgSearchBoolExpression.h"

// This stuff lives in the base class because the IMAP search syntax 
// is used by the Dredd SEARCH command as well as IMAP itself

// km - the NOT and HEADER strings are not encoded with a trailing
//      <space> because they always precede a mnemonic that has a
//      preceding <space> and double <space> characters cause some
//		imap servers to return an error.
const char *nsMsgSearchAdapter::m_kImapBefore   = " SENTBEFORE ";
const char *nsMsgSearchAdapter::m_kImapBody     = " BODY ";
const char *nsMsgSearchAdapter::m_kImapCC       = " CC ";
const char *nsMsgSearchAdapter::m_kImapFrom     = " FROM ";
const char *nsMsgSearchAdapter::m_kImapNot      = " NOT";
const char *nsMsgSearchAdapter::m_kImapUnDeleted= " UNDELETED";
const char *nsMsgSearchAdapter::m_kImapOr       = " OR";
const char *nsMsgSearchAdapter::m_kImapSince    = " SENTSINCE ";
const char *nsMsgSearchAdapter::m_kImapSubject  = " SUBJECT ";
const char *nsMsgSearchAdapter::m_kImapTo       = " TO ";
const char *nsMsgSearchAdapter::m_kImapHeader   = " HEADER";
const char *nsMsgSearchAdapter::m_kImapAnyText  = " TEXT ";
const char *nsMsgSearchAdapter::m_kImapKeyword  = " KEYWORD ";
const char *nsMsgSearchAdapter::m_kNntpKeywords = " KEYWORDS "; //ggrrrr...
const char *nsMsgSearchAdapter::m_kImapSentOn = " SENTON ";
const char *nsMsgSearchAdapter::m_kImapSeen = " SEEN ";
const char *nsMsgSearchAdapter::m_kImapAnswered = " ANSWERED ";
const char *nsMsgSearchAdapter::m_kImapNotSeen = " UNSEEN ";
const char *nsMsgSearchAdapter::m_kImapNotAnswered = " UNANSWERED ";
const char *nsMsgSearchAdapter::m_kImapCharset = " CHARSET ";

static NS_DEFINE_CID(kPrefServiceCID, NS_PREF_CID);

NS_IMETHODIMP nsMsgSearchAdapter::FindTargetFolder(const nsMsgResultElement *,nsIMsgFolder * *)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsMsgSearchAdapter::ModifyResultElement(nsMsgResultElement *, nsMsgSearchValue *)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsMsgSearchAdapter::OpenResultElement(nsMsgResultElement *)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMPL_ISUPPORTS1(nsMsgSearchAdapter, nsIMsgSearchAdapter)

nsMsgSearchAdapter::nsMsgSearchAdapter(nsIMsgSearchScopeTerm *scope, nsMsgSearchTermArray &searchTerms) 
	: m_searchTerms(searchTerms)
{
  NS_INIT_ISUPPORTS();
  m_scope = scope;
}

nsMsgSearchAdapter::~nsMsgSearchAdapter()
{
}

NS_IMETHODIMP nsMsgSearchAdapter::ValidateTerms ()
{
  // all this used to do is check if the object had been deleted - we can skip that.
	return NS_OK;
}

NS_IMETHODIMP nsMsgSearchAdapter::Abort ()
{
	return NS_ERROR_NOT_IMPLEMENTED;

}
NS_IMETHODIMP nsMsgSearchAdapter::Search () 
{
  return NS_OK; 
}

NS_IMETHODIMP nsMsgSearchAdapter::SendUrl () 
{
  return NS_OK; 
}

NS_IMETHODIMP nsMsgSearchAdapter::GetEncoding (char **encoding) 
{
  return NS_OK; 
}

NS_IMETHODIMP nsMsgSearchAdapter::AddHit(nsMsgKey key)
{
  NS_ASSERTION(PR_FALSE, "shouldn't call this base class impl");
  return NS_ERROR_FAILURE;
}


char *
nsMsgSearchAdapter::TryToConvertCharset(char *sourceStr, const PRUnichar *srcCharset, const PRUnichar * destCharset, PRBool useMime2)
{
	char *result = nsnull;

	if (sourceStr == nsnull) 
		return nsnull;
#ifdef DO_I18N  // I have no idea what we should do here.
	if ((src_csid != dest_csid) || (useMime2))
	{
		// Need to convert. See if we can.

		// ### mwelch Much of this code is lifted from 
		//     lib/libi18n/mime2fun.c (in particular,
		//     intl_EncodeMimePartIIStr).
		CCCDataObject obj = nsnull;
		CCCFunc cvtfunc = nsnull;
		int srcLen = XP_STRLEN(sourceStr);

		obj = INTL_CreateCharCodeConverter();
		if (obj == nsnull)
			return 0;

		/* setup converter from src_csid --> dest_csid */
		INTL_GetCharCodeConverter(src_csid, dest_csid, obj) ;
		cvtfunc = INTL_GetCCCCvtfunc(obj);

		if (cvtfunc)
		{
			// We can convert, so do it.
			if (useMime2)
				// Force MIME-2 encoding so that the charset (if necessary) gets
				// passed to the Dredd server inline
				result = INTL_EncodeMimePartIIStr(sourceStr, src_csid, TRUE);
			else
			{
				// Copy the source string before using it for conversion.
				// You just don't know where those bits have been.
				char *temp = XP_STRDUP(sourceStr);
				if (temp)
				{
					// (result) will differ from (temp) iff a larger string 
					// were needed to contain the converted chars.
					// (or so I understand)
					result = (char *) cvtfunc(obj, 
											  (unsigned char*)temp, 
											  srcLen);
					if (result != temp)
						XP_FREE(temp);
				}
			}
		}
		XP_FREEIF(obj);
	}
#endif
	return result;
}

char *
nsMsgSearchAdapter::GetImapCharsetParam(const PRUnichar *destCharset)
{
	char *result = nsnull;

	// Specify a character set unless we happen to be US-ASCII.
  if (nsCRT::strcmp(destCharset, "us-ascii"))
	    result = PR_smprintf("%s%s", nsMsgSearchAdapter::m_kImapCharset, destCharset);

	return result;
}

char *nsMsgSearchAdapter::EscapeSearchUrl (const char *nntpCommand)
{
	char *result = nsnull;
	// max escaped length is two extra characters for every character in the cmd.
  char *scratchBuf = (char*) PR_Malloc(3*nsCRT::strlen(nntpCommand) + 1);
	if (scratchBuf)
	{
		char *scratchPtr = scratchBuf;
		while (PR_TRUE)
		{
			char ch = *nntpCommand++;
			if (!ch)
				break;
			if (ch == '#' || ch == '?' || ch == '@' || ch == '\\')
			{
				*scratchPtr++ = '\\';
				sprintf (scratchPtr, "%X", ch);
                                   /* Reviewed 4.51 safe use of sprintf */
				scratchPtr += 2;
			}
			else
				*scratchPtr++ = ch;
		}
		*scratchPtr = '\0';
		result = PL_strdup (scratchBuf); // realloc down to smaller size
    nsCRT::free (scratchBuf);
	}
	return result;
}

char *nsMsgSearchAdapter::EscapeImapSearchProtocol(const char *imapCommand)
{
	char *result = nsnull;
	// max escaped length is one extra character for every character in the cmd.
  char *scratchBuf = (char*) PR_Malloc (2*nsCRT::strlen(imapCommand) + 1);
	if (scratchBuf)
	{
		char *scratchPtr = scratchBuf;
		while (1)
		{
			char ch = *imapCommand++;
			if (!ch)
				break;
			if (ch == '\\')
			{
				*scratchPtr++ = '\\';
				*scratchPtr++ = '\\';
			}
			else
				*scratchPtr++ = ch;
		}
		*scratchPtr = '\0';
    result = nsCRT::strdup (scratchBuf); // realloc down to smaller size
    nsCRT::free (scratchBuf);
	}
	return result;
}

char *nsMsgSearchAdapter::EscapeQuoteImapSearchProtocol(const char *imapCommand)
{
	char *result = nsnull;
	// max escaped length is one extra character for every character in the cmd.
  char *scratchBuf = (char*) PR_Malloc (2*nsCRT::strlen(imapCommand) + 1);
	if (scratchBuf)
	{
		char *scratchPtr = scratchBuf;
		while (1)
		{
			char ch = *imapCommand++;
			if (!ch)
				break;
			if (ch == '"')
			{
				*scratchPtr++ = '\\';
				*scratchPtr++ = '"';
			}
			else
				*scratchPtr++ = ch;
		}
		*scratchPtr = '\0';
    result = nsCRT::strdup (scratchBuf); // realloc down to smaller size
    nsCRT::free (scratchBuf);
	}
	return result;
}


char *nsMsgSearchAdapter::UnEscapeSearchUrl (const char *commandSpecificData)
{
  char *result = (char*) PR_Malloc (nsCRT::strlen(commandSpecificData) + 1);
	if (result)
	{
		char *resultPtr = result;
		while (1)
		{
			char ch = *commandSpecificData++;
			if (!ch)
				break;
			if (ch == '\\')
			{
				char scratchBuf[3];
				scratchBuf[0] = (char) *commandSpecificData++;
				scratchBuf[1] = (char) *commandSpecificData++;
				scratchBuf[2] = '\0';
				int accum = 0;
				sscanf (scratchBuf, "%X", &accum);
				*resultPtr++ = (char) accum;
			}
			else
				*resultPtr++ = ch;
		}
		*resultPtr = '\0';
	}
	return result;
}


void
nsMsgSearchAdapter::GetSearchCharsets(nsString &srcCharset, nsString& dstCharset)
{
    nsresult rv;
//  char *defaultCharset =   nsMsgI18NGetDefaultMailCharset();
  nsAutoString defaultCharset; defaultCharset.AssignWithConversion(nsMsgI18NGetDefaultMailCharset());
	srcCharset = defaultCharset;
	dstCharset = defaultCharset;

	if (m_scope)
	{
    // ### DMB is there a way to get the charset for the "window"?

        nsCOMPtr<nsIMsgFolder> folder;
        rv = m_scope->GetFolder(getter_AddRefs(folder));
        
		// Ask the newsgroup/folder for its csid.
		if (NS_SUCCEEDED(rv) && folder)
        {
            nsXPIDLString folderCharset;
            folder->GetCharset(getter_Copies(folderCharset));
            dstCharset.Assign(folderCharset);
        }
	}


	// If 
	// the destination is still CS_DEFAULT, make the destination match
	// the source. (CS_DEFAULT is an indication that the charset
	// was undefined or unavailable.)
  // ### well, it's not really anymore. Is there an equivalent?
  if (!nsCRT::strcmp(dstCharset.GetUnicode(), defaultCharset.GetUnicode()))
		dstCharset = srcCharset;

	PRBool forceAscii = PR_FALSE;

	NS_WITH_SERVICE(nsIPref, prefs, kPrefServiceCID, &rv);

	if (NS_SUCCEEDED(rv))
	{
		rv = prefs->GetBoolPref("mailnews.force_ascii_search", &forceAscii);
	}

	if (forceAscii)
	{
		// Special cases to use in order to force US-ASCII searching with Latin1
		// or MacRoman text. Eurgh. This only has to happen because IMAP
		// and Dredd servers currently (4/23/97) only support US-ASCII.
		// 
		// If the dest csid is ISO Latin 1 or MacRoman, attempt to convert the 
		// source text to US-ASCII. (Not for now.)
		// if ((dst_csid == CS_LATIN1) || (dst_csid == CS_MAC_ROMAN))
			dstCharset.AssignWithConversion("us-ascii");
	}
}

nsresult nsMsgSearchAdapter::EncodeImapTerm (nsMsgSearchTerm *term, PRBool reallyDredd, const PRUnichar *srcCharset, const PRUnichar *destCharset, char **ppOutTerm)
{
	nsresult err = NS_OK;
	PRBool useNot = PR_FALSE;
	PRBool useQuotes = PR_FALSE;
	PRBool excludeHeader = PR_FALSE;
	PRBool ignoreValue = PR_FALSE;
	char *arbitraryHeader = nsnull;
	const char *whichMnemonic = nsnull;
	const char *orHeaderMnemonic = nsnull;

	*ppOutTerm = nsnull;

	if (term->m_operator == nsMsgSearchOp::DoesntContain || term->m_operator == nsMsgSearchOp::Isnt)
		useNot = PR_TRUE;

	switch (term->m_attribute)
	{
	case nsMsgSearchAttrib::ToOrCC:
		orHeaderMnemonic = m_kImapCC;
		// fall through to case nsMsgSearchAttrib::To:
	case nsMsgSearchAttrib::To:
		whichMnemonic = m_kImapTo;
		break;
	case nsMsgSearchAttrib::CC:
		whichMnemonic = m_kImapCC;
		break;
	case nsMsgSearchAttrib::Sender:
		whichMnemonic = m_kImapFrom;
		break;
	case nsMsgSearchAttrib::Subject:
		whichMnemonic = m_kImapSubject;
		break;
	case nsMsgSearchAttrib::Body:
		whichMnemonic = m_kImapBody;
		excludeHeader = PR_TRUE;
		break;
	case nsMsgSearchAttrib::OtherHeader:  // arbitrary header? if so create arbitrary header string
		if (term->m_arbitraryHeader.Length() > 0)
		{
			arbitraryHeader = new char [nsCRT::strlen(term->m_arbitraryHeader) + 6];  // 6 bytes for SPACE \" .... \" SPACE
			if (!arbitraryHeader)
				return NS_ERROR_OUT_OF_MEMORY;
			arbitraryHeader[0] = '\0';
			PL_strcat(arbitraryHeader, " \"");
			PL_strcat(arbitraryHeader, term->m_arbitraryHeader);
			PL_strcat(arbitraryHeader, "\" ");
			whichMnemonic = arbitraryHeader;
		}
		else
			return NS_ERROR_FAILURE;
		break;
	case nsMsgSearchAttrib::AgeInDays:  // added for searching online for age in days...
		// for AgeInDays, we are actually going to perform a search by date, so convert the operations for age
		// to the IMAP mnemonics that we would use for date!
		switch (term->m_operator)
		{
		case nsMsgSearchOp::IsGreaterThan:
			whichMnemonic = m_kImapBefore;
			break;
		case nsMsgSearchOp::IsLessThan:
			whichMnemonic = m_kImapSince;
			break;
		case nsMsgSearchOp::Is:
			whichMnemonic = m_kImapSentOn;
			break;
		default:
			NS_ASSERTION(PR_FALSE, "invalid search operator");
			return NS_ERROR_INVALID_ARG;
		}
		excludeHeader = PR_TRUE;
		break;
	case nsMsgSearchAttrib::Date:
		switch (term->m_operator)
		{
		case nsMsgSearchOp::IsBefore:
			whichMnemonic = m_kImapBefore;
			break;
		case nsMsgSearchOp::IsAfter:
			whichMnemonic = m_kImapSince;
			break;
		case nsMsgSearchOp::Isnt:  /* we've already added the "Not" so just process it like it was a date is search */
		case nsMsgSearchOp::Is:
			whichMnemonic = m_kImapSentOn;
			break;
		default:
			NS_ASSERTION(PR_FALSE, "invalid search operator");
			return NS_ERROR_INVALID_ARG;
		}
		excludeHeader = PR_TRUE;
		break;
	case nsMsgSearchAttrib::AnyText:
		whichMnemonic = m_kImapAnyText;
		excludeHeader = PR_TRUE;
		break;
	case nsMsgSearchAttrib::Keywords:
		whichMnemonic = m_kNntpKeywords;
		break;
	case nsMsgSearchAttrib::MsgStatus:
		useNot = PR_FALSE; // bizarrely, NOT SEEN is wrong, but UNSEEN is right.
		ignoreValue = PR_TRUE; // the mnemonic is all we need
		excludeHeader = PR_TRUE;
		switch (term->m_value.u.msgStatus)
		{
		case MSG_FLAG_READ:
			whichMnemonic = term->m_operator == nsMsgSearchOp::Is ? m_kImapSeen : m_kImapNotSeen;
			break;
		case MSG_FLAG_REPLIED:
			whichMnemonic = term->m_operator == nsMsgSearchOp::Is ? m_kImapAnswered : m_kImapNotAnswered;
			break;
		default:
			NS_ASSERTION(PR_FALSE, "invalid search operator");
			return NS_ERROR_INVALID_ARG;
		}
		break;
	default:
			NS_ASSERTION(PR_FALSE, "invalid search operator");
		return NS_ERROR_INVALID_ARG;
	}

	char *value = "";
	char dateBuf[100];
	dateBuf[0] = '\0';
	
	PRBool valueWasAllocated = PR_FALSE;
	if (term->m_attribute == nsMsgSearchAttrib::Date)
	{
		// note that there used to be code here that encoded an RFC822 date for imap searches.
		// The IMAP RFC 2060 is misleading to the point that it looks like it requires an RFC822
		// date but really it expects dd-mmm-yyyy, like dredd, and refers to the RFC822 date only in that the
		// dd-mmm-yyyy date will match the RFC822 date within the message.

		PRTime adjustedDate = term->m_value.u.date;
		if (whichMnemonic == m_kImapSince)
		{
			// it looks like the IMAP server searches on Since includes the date in question...
			// our UI presents Is, IsGreater and IsLessThan. For the IsGreater case (m_kImapSince)
			// we need to adjust the date so we get greater than and not greater than or equal to which
			// is what the IMAP server wants to search on
      // won't work on Mac.
//			adjustedDate += 60 * 60 * 24; // bump up to the day after this one...
		}

   	PRExplodedTime exploded;
    PR_ExplodeTime(adjustedDate, PR_LocalTimeParameters, &exploded);
    PR_FormatTimeUSEnglish(dateBuf, sizeof(dateBuf), "%d-%b-%Y", &exploded);
//		strftime (dateBuf, sizeof(dateBuf), "%d-%b-%Y", localtime (/* &term->m_value.u.date */ &adjustedDate));
		value = dateBuf;
	}
	else
	{
		if (term->m_attribute == nsMsgSearchAttrib::AgeInDays)
		{
			// okay, take the current date, subtract off the age in days, then do an appropriate Date search on 
			// the resulting day.
			PRTime now = PR_Now();
			PRTime matchDay;
			LL_I2L(matchDay, 0); // = now ;  - term->m_value.u.age * 60 * 60 * 24; won't work on the mac
   	PRExplodedTime exploded;
    PR_ExplodeTime(matchDay, PR_LocalTimeParameters, &exploded);
    PR_FormatTimeUSEnglish(dateBuf, sizeof(dateBuf), "%d-%b-%Y", &exploded);
//			strftime (dateBuf, sizeof(dateBuf), "%d-%b-%Y", localtime (&matchDay));
			value = dateBuf;
		}
		else

		if (IsStringAttribute(term->m_attribute))
		{
			char *unconvertedValue; // = reallyDredd ? MSG_EscapeSearchUrl (term->m_value.u.string) : msg_EscapeImapSearchProtocol(term->m_value.u.string);

			// Ugly switch for Korean mail/news charsets.
			// We want to do this here because here is where
			// we know what charset we want to use.
#ifdef DOING_CHARSET
			if (reallyDredd)
				dest_csid = INTL_DefaultNewsCharSetID(dest_csid);
			else
				dest_csid = INTL_DefaultMailCharSetID(dest_csid);
#endif

			unconvertedValue = TryToConvertCharset(term->m_value.u.string,
										srcCharset,
										destCharset,
										reallyDredd);
			if (!unconvertedValue)
				unconvertedValue = nsCRT::strdup(term->m_value.u.string); // couldn't convert, send as is

			value = reallyDredd ? EscapeSearchUrl (unconvertedValue) : EscapeImapSearchProtocol(unconvertedValue);
			PR_Free(unconvertedValue);

			valueWasAllocated = PR_TRUE;
			useQuotes = !reallyDredd || (PL_strchr(value, ' ') != nsnull);
			if (useQuotes)
			{
				unconvertedValue = value;
				value = EscapeQuoteImapSearchProtocol(unconvertedValue);
				PR_Free(unconvertedValue);
			}

		}
	}

	int len = nsCRT::strlen(whichMnemonic) + nsCRT::strlen(value) + (useNot ? nsCRT::strlen(m_kImapNot) : 0) + 
		(useQuotes ? 2 : 0) + nsCRT::strlen(m_kImapHeader) + 
		(orHeaderMnemonic ? (nsCRT::strlen(m_kImapHeader) + nsCRT::strlen(m_kImapOr) + (useNot ? nsCRT::strlen(m_kImapNot) : 0) + 
		nsCRT::strlen(orHeaderMnemonic) + nsCRT::strlen(value) + 2 /*""*/) : 0) + 1;
	char *encoding = new char[len];
	if (encoding)
	{
		encoding[0] = '\0';
		// Remember: if ToOrCC and useNot then the expression becomes NOT To AND Not CC as opposed to (NOT TO) || (NOT CC)
		if (orHeaderMnemonic && !useNot)
			PL_strcat(encoding, m_kImapOr);
		if (useNot)
			PL_strcat (encoding, m_kImapNot);
		if (!excludeHeader)
			PL_strcat (encoding, m_kImapHeader);
		PL_strcat (encoding, whichMnemonic);
		if (!ignoreValue)
			err = EncodeImapValue(encoding, value, useQuotes, reallyDredd);
		
		if (orHeaderMnemonic)
		{
			if (useNot)
				PL_strcat(encoding, m_kImapNot);
			
			PL_strcat (encoding, m_kImapHeader);

			PL_strcat (encoding, orHeaderMnemonic);
			if (!ignoreValue)
				err = EncodeImapValue(encoding, value, useQuotes, reallyDredd);
		}
		
		// kmcentee, don't let the encoding end with whitespace, 
		// this throws off later url STRCMP
		if (*encoding && *(encoding + nsCRT::strlen(encoding) - 1) == ' ')
			*(encoding + nsCRT::strlen(encoding) - 1) = '\0';
	}

	if (value && valueWasAllocated)
		PR_Free (value);
	if (arbitraryHeader)
		delete arbitraryHeader;

	*ppOutTerm = encoding;

	return err;
}

nsresult nsMsgSearchAdapter::EncodeImapValue(char *encoding, const char *value, PRBool useQuotes, PRBool reallyDredd)
{
	// By NNTP RFC, SEARCH HEADER SUBJECT "" is legal and means 'find messages without a subject header'
	if (!reallyDredd)
	{
		// By IMAP RFC, SEARCH HEADER SUBJECT "" is illegal and will generate an error from the server
		if (!value || !value[0])
			return NS_ERROR_NULL_POINTER;
	}
	
	if (useQuotes)
		PL_strcat(encoding, "\"");
	PL_strcat (encoding, value);
	if (useQuotes)
		PL_strcat(encoding, "\"");

	return NS_OK;
}


nsresult nsMsgSearchAdapter::EncodeImap (char **ppOutEncoding, nsMsgSearchTermArray &searchTerms, const PRUnichar *srcCharset, const PRUnichar *destCharset, PRBool reallyDredd)
{
	// i've left the old code (before using CBoolExpression for debugging purposes to make sure that
	// the new code generates the same encoding string as the old code.....

	nsresult err = NS_OK;
	*ppOutEncoding = nsnull;

	int termCount = searchTerms.Count();
	int i = 0;
	int encodingLength = 0;

	// Build up an array of encodings, one per query term
	char **termEncodings = new char *[termCount];
	if (!termEncodings)
		return NS_ERROR_OUT_OF_MEMORY;
	// create our expression
	nsMsgSearchBoolExpression * expression = new nsMsgSearchBoolExpression();
	if (!expression)
		return NS_ERROR_OUT_OF_MEMORY;

	for (i = 0; i < termCount && NS_SUCCEEDED(err); i++)
	{
		err = EncodeImapTerm (searchTerms.ElementAt(i), reallyDredd, srcCharset, destCharset, &termEncodings[i]);
		if (NS_SUCCEEDED(err) && nsnull != termEncodings[i])
		{
			encodingLength += nsCRT::strlen(termEncodings[i]) + 1;
			expression = expression->AddSearchTerm(searchTerms.ElementAt(i),termEncodings[i]);
		}
	}

	if (NS_SUCCEEDED(err)) 
	{
		// Catenate the intermediate encodings together into a big string
		char *totalEncoding = new char [encodingLength + (!reallyDredd ? nsCRT::strlen(m_kImapUnDeleted) : 0) + 1];
		nsCString encodingBuff;

		if (totalEncoding)
		{
			totalEncoding[0] = '\0';

			int offset = 0;       // offset into starting place for the buffer
			if (!reallyDredd)
				PL_strcat(totalEncoding, m_kImapUnDeleted);

			if (!reallyDredd)
			{
				encodingBuff.Append(m_kImapUnDeleted);
				offset = nsCRT::strlen(m_kImapUnDeleted);
			}

			expression->GenerateEncodeStr(&encodingBuff);

			for (i = 0; i < termCount; i++)
			{
				if (termEncodings[i])
				{
					PL_strcat (totalEncoding, termEncodings[i]);
					delete [] termEncodings[i];
				}
			}
		}
		else
			err = NS_ERROR_OUT_OF_MEMORY;

		delete totalEncoding;
		delete expression;

		// Set output parameter if we encoded the query successfully
		if (NS_SUCCEEDED(err))
			*ppOutEncoding = encodingBuff.ToNewCString();
	}

	delete [] termEncodings;

	return err;
}


char *nsMsgSearchAdapter::TransformSpacesToStars (const char *spaceString, msg_TransformType transformType)
{
	char *starString;
	
	if (transformType == kOverwrite)
	{
		if ((starString = nsCRT::strdup(spaceString)) != nsnull)
		{
			char *star = starString;
			while ((star = PL_strchr(star, ' ')) != nsnull)
				*star = '*';
		}
	}
	else
	{
		int i, count;

		for (i = 0, count = 0; spaceString[i]; )
		{
			if (spaceString[i++] == ' ')
			{
				count++;
				while (spaceString[i] && spaceString[i] == ' ') i++;
			}
		}

		if (transformType == kSurround)
			count *= 2;

		if (count > 0)
		{
			if ((starString = (char *)PR_Malloc(i + count + 1)) != nsnull)
			{
				int j;

				for (i = 0, j = 0; spaceString[i]; )
				{
					if (spaceString[i] == ' ')
					{
						starString[j++] = '*';
						starString[j++] = ' ';
						if (transformType == kSurround)
							starString[j++] = '*';

						i++;
						while (spaceString[i] && spaceString[i] == ' ')
							i++;
					}
					else
						starString[j++] = spaceString[i++];
				}
				starString[j] = 0;
			}
		}
		else
			starString = nsCRT::strdup(spaceString);
	}

	return starString;
}

//-----------------------------------------------------------------------------
//------------------- Validity checking for menu items etc. -------------------
//-----------------------------------------------------------------------------

nsMsgSearchValidityTable::nsMsgSearchValidityTable ()
{
	// Set everything to be unavailable and disabled
	for (int i = 0; i < nsMsgSearchAttrib::kNumMsgSearchAttributes; i++)
    for (int j = 0; j < nsMsgSearchOp::kNumMsgSearchOperators; j++)
		{
			SetAvailable (i, j, FALSE);
			SetEnabled (i, j, FALSE);
			SetValidButNotShown (i,j, FALSE);
		}
	m_numAvailAttribs = 0;   // # of attributes marked with at least one available operator
}

int nsMsgSearchValidityTable::GetNumAvailAttribs()
{
	m_numAvailAttribs = 0;
	for (int i = 0; i < nsMsgSearchAttrib::kNumMsgSearchAttributes; i++)
		for (int j = 0; j < nsMsgSearchOp::kNumMsgSearchOperators; j++)
			if (GetAvailable(i, j))
			{
				m_numAvailAttribs++;
				break;
			}
	return m_numAvailAttribs;
}

nsresult nsMsgSearchValidityTable::ValidateTerms (nsMsgSearchTermArray &termList)
{
	nsresult err = NS_OK;

	for (int i = 0; i < termList.Count(); i++)
	{
		nsMsgSearchTerm *term = termList.ElementAt(i);
//		XP_ASSERT(term->IsValid());
		if (!GetEnabled(term->m_attribute, term->m_operator) || 
			!GetAvailable(term->m_attribute, term->m_operator))
		{
			if (!GetValidButNotShown(term->m_attribute, term->m_operator))
				err = NS_MSG_ERROR_INVALID_SEARCH_SCOPE;
		}
	}

	return err;
}


// global variable with destructor allows automatic cleanup
nsMsgSearchValidityManager gValidityMgr; 


nsMsgSearchValidityManager::nsMsgSearchValidityManager ()
{
	m_offlineMailTable = nsnull;
	m_onlineMailTable = nsnull;
	m_onlineMailFilterTable = nsnull;
	m_newsTable = nsnull;
	m_localNewsTable = nsnull;
#ifdef DOING_EXNEWSSEARCH
	m_newsExTable = nsnull;
#endif
#ifdef DOING_LDAP
	m_ldapTable = nsnull;
#endif
}


nsMsgSearchValidityManager::~nsMsgSearchValidityManager ()
{
	if (nsnull != m_offlineMailTable)
		delete m_offlineMailTable;
	if (nsnull != m_onlineMailTable)
		delete m_onlineMailTable;
	if (nsnull != m_onlineMailFilterTable)
		delete m_onlineMailFilterTable;
	if (nsnull != m_newsTable)
		delete m_newsTable;
	if (nsnull != m_localNewsTable)
		delete m_localNewsTable;
#ifdef DOING_EXNEWSSEARCH
	if (nsnull != m_newsExTable)
		delete m_newsExTable;
#endif
#ifdef DOING_LDAP
	if (nsnull != m_ldapTable)
		delete m_ldapTable;
#endif
}


//-----------------------------------------------------------------------------
// Bottleneck accesses to the objects so we can allocate and initialize them
// lazily. This way, there's no heap overhead for the validity tables until the
// user actually searches that scope.
//-----------------------------------------------------------------------------

nsresult nsMsgSearchValidityManager::GetTable (int whichTable, nsMsgSearchValidityTable **ppOutTable)
{
	NS_ENSURE_ARG(ppOutTable);

	nsresult err = NS_OK;

	// hack alert...currently FEs are setting scope to News even if it should be set to OfflineNewsgroups...
	// i'm fixing this by checking if we are in offline mode...
#ifdef DOING_OFFLINE
	if (NET_IsOffline() && (whichTable == news || whichTable == newsEx))
		whichTable = localNews;
#endif

	switch (whichTable)
	{
	case offlineMail:
		if (!m_offlineMailTable)
			err = InitOfflineMailTable ();
		*ppOutTable = m_offlineMailTable;
		break;
	case onlineMail:
		if (!m_onlineMailTable)
			err = InitOnlineMailTable ();
		*ppOutTable = m_onlineMailTable;
		break;
	case onlineMailFilter:
		if (!m_onlineMailFilterTable)
			err = InitOnlineMailFilterTable ();
		*ppOutTable = m_onlineMailFilterTable;
		break;
	case news:
		if (!m_newsTable)
			err = InitNewsTable ();
		*ppOutTable = m_newsTable;
		break;
#ifdef DOING_OFFLINE
	case localNews:
		if (!m_localNewsTable)
			err = InitLocalNewsTable();
		*ppOutTable = m_localNewsTable;
		break;
#endif
#ifdef DOING_EXNEWSSEARCH
	case newsEx:
		if (!m_newsExTable)
			err = InitNewsExTable ();
		*ppOutTable = m_newsExTable;
		break;
#endif
#ifdef DOING_LDAP
	case Ldap:
		if (!m_ldapTable)
			err = InitLdapTable ();
		*ppOutTable = m_ldapTable;
		break;
#endif
	default:                 
		NS_ASSERTION(PR_FALSE, "invalid table type");
		err = NS_MSG_ERROR_INVALID_SEARCH_TERM;
	}

	return err;
}



nsresult nsMsgSearchValidityManager::NewTable(nsMsgSearchValidityTable **aTable)
{
	NS_ENSURE_ARG (aTable);
	*aTable = new nsMsgSearchValidityTable;
	if (nsnull == *aTable)
		return NS_ERROR_OUT_OF_MEMORY;
  return NS_OK;
}


