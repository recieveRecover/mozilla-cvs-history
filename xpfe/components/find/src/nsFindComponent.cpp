/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
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

#include "nsIFindComponent.h"

#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsIAppShellService.h"
#include "nsIDOMXULDocument.h"
#include "nsIDOMElement.h"
#include "nsIDocument.h"
#include "nsITextServicesDocument.h"
#include "nsTextServicesCID.h"
#include "nsIWebShell.h"
#include "nsIWebShellWindow.h"
#include "nsIPresShell.h"
#include "nsIContentViewer.h"
#include "nsIDocumentViewer.h"
#include "nsIContent.h"
#include "nsIURL.h"
#include "nsFileSpec.h"
#include "nsIFactory.h"
#include "pratom.h"
#include "nsIServiceManager.h"

#include "nsFindComponent.h"
#include "nsFindDialog.h"

#define DEBUG_FIND

// ctor
nsFindComponent::nsFindComponent()
    : mLastSearchString(),
      mLastIgnoreCase( PR_FALSE ),
      mLastSearchBackwards( PR_FALSE ),
      mLastWrapSearch( PR_FALSE )
{
    NS_INIT_REFCNT();

    // Initialize "last" stuff from prefs, if we wanted to be really clever...
}

// dtor
nsFindComponent::~nsFindComponent()
{
}

NS_IMETHODIMP
nsFindComponent::CreateContext( nsIWebShell *aWebShell,
                                nsISupports **aResult )
{

    if (!aResult)
			return NS_ERROR_NULL_POINTER;
      
    // Construct a new Context with this document.
    Context		*newContext = new Context();
   	if (!newContext)
   		return NS_ERROR_OUT_OF_MEMORY;
   
     // Do the expected AddRef on behalf of caller.
    newContext->AddRef();

    nsresult	rv = newContext->Init( aWebShell,
                        mLastSearchString,
                        mLastIgnoreCase,
                        mLastSearchBackwards,
                        mLastWrapSearch);
 		if (NS_FAILED(rv))
 		{
 			NS_RELEASE(newContext);
 			return rv;
 		}
 
		*aResult = newContext;
    return NS_OK;
}

#ifdef XP_MAC
#pragma mark -
#endif


nsFindComponent::Context::Context()
{
  NS_INIT_REFCNT();
	// all our other members are self-initiating
}


nsFindComponent::Context::~Context()
{
	// the nsCOMPtr will do its thing to release the document

}

NS_IMETHODIMP
nsFindComponent::Context::Init( nsIWebShell *aWebShell,
                 const nsString &lastSearchString,
                 PRBool lastIgnoreCase,
                 PRBool lastSearchBackward,
                 PRBool lastWrapSearch)
{
	if (!aWebShell)
		return NS_ERROR_INVALID_ARG;
		
    mWebShell        = nsDontQueryInterface<nsIWebShell>( aWebShell );
    mLastBlockIndex  = 0;
    mLastBlockOffset = -1;
	mSearchString    = lastSearchString;
	mIgnoreCase      = lastIgnoreCase;
	mSearchBackwards = lastSearchBackward;
	mWrapSearch      = lastWrapSearch;
	
	return NS_OK;
}


static NS_DEFINE_CID(kCTextServicesDocumentCID, NS_TEXTSERVICESDOCUMENT_CID);
static NS_DEFINE_IID(kITextServicesDocumentIID, NS_ITEXTSERVICESDOCUMENT_IID);


nsCOMPtr<nsITextServicesDocument>
nsFindComponent::Context::MakeTSDocument() {
	nsCOMPtr<nsITextServicesDocument> result;

    // Create the text services document.
	nsresult rv = nsComponentManager::CreateInstance( kCTextServicesDocumentCID,
                                                      nsnull,
                                                      kITextServicesDocumentIID,
                                                      (void **)getter_AddRefs( result ) );
    if ( NS_SUCCEEDED(rv) ) {
        nsCOMPtr<nsIDocument>  document;
        nsCOMPtr<nsIPresShell> presShell;
    
        // Get content viewer from the web shell.
        nsCOMPtr<nsIContentViewer> contentViewer;
        rv = mWebShell ? mWebShell->GetContentViewer(getter_AddRefs(contentViewer))
                       : NS_ERROR_NULL_POINTER;
    
        if ( contentViewer ) {
            // Up-cast to a document viewer.
            nsCOMPtr<nsIDocumentViewer> docViewer(do_QueryInterface(contentViewer));
            if ( docViewer ) {
                // Get the document and pres shell from the doc viewer.
                rv = docViewer->GetDocument(*getter_AddRefs(document));
                if ( document ) {
                    rv = docViewer->GetPresShell(*getter_AddRefs(presShell));
                }
            }
        }
    
        if ( document && presShell ) {
            // Compare this document with the one we searched last time.
            if ( document.get() == mLastDocument ) {
                // Same document, use the block index/offset we have remembered.
            } else {
                // New document, remember it and reset saved index/offset.
                mLastDocument = document;
                mLastBlockIndex = 0;
                mLastBlockOffset = -1;
            }

            // Upcast document to a DOM document.
            nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(document);

            if ( domDoc ) {
                // Initialize the text services document.
                rv = result->InitWithDocument( domDoc, presShell );
                
                if ( NS_SUCCEEDED( rv ) ) {
                    // Position to remembered location.
                    if ( mLastBlockIndex ) {
                        // Sign indicates whether we're off first vs. last.
                        if ( mLastBlockIndex > 0 ) {
                            // Relative to first block.
                            rv = result->FirstBlock();
                            for( PRInt32 i = 0; NS_SUCCEEDED(rv) && i < mLastBlockIndex; i++ ) {
                                rv = result->NextBlock();
                            }
                        } else {
                            // Relative to last block.
                            rv = result->LastBlock();
                            for( PRInt32 i = 0; NS_SUCCEEDED(rv) && i > mLastBlockIndex; i-- ) {
                                rv = result->PrevBlock();
                            }
                        }
                    } else {
                        // Initialize, based on whether which direction we're searching in.
                        if (mSearchBackwards)
                            rv = result->LastBlock();
                        else
                            rv = result->FirstBlock();
                    }
                    // If something went wrong, reset result.
                    if ( NS_FAILED( rv ) ) {
                        result = nsDontQueryInterface<nsITextServicesDocument>( 0 );
                    }
                } else {
                    // Give up.
                    result = nsDontQueryInterface<nsITextServicesDocument>( 0 );
                }
            } else {
                // Give up.
                result = nsDontQueryInterface<nsITextServicesDocument>( 0 );
            }
        } else {
            // Error, release the result and return null.
            result = nsDontQueryInterface<nsITextServicesDocument>( 0 );
        }
    }

    // Return the resulting text services document.
    return result;
}



// ----------------------------------------------------------------
//	CharsMatch
//
//	Compare chars. Match if both are whitespace, or both are
//	non whitespace and same char.
// ----------------------------------------------------------------

static PRBool CharsMatch(PRUnichar c1, PRUnichar c2)
{
	return (nsString::IsSpace(c1) && nsString::IsSpace(c2)) ||
						(c1 == c2);
	
}


// ----------------------------------------------------------------
//	FindInString
//
//	Routine to search in an nsString which is smart about extra
//  whitespace, can search backwards, and do case insensitive search.
//
//	This uses a brute-force algorithm, which should be sufficient
//	for our purposes (text searching)
// 
//	searchStr contains the text from a content node, which can contain
//	extra white space between words, which we have to deal with.
//	The offsets passed in and back are offsets into searchStr,
//	and thus include extra white space.
//
//	If we are ignoring case, the strings have already been lowercased
// 	at this point.
//
//	Returns -1 if the string is not found, or if the pattern is an
//	empty string, or if startOffset is off the end of the string.
// ----------------------------------------------------------------

static PRInt32 FindInString(const nsString &searchStr, const nsString &patternStr,
						PRInt32 startOffset, PRBool searchBackwards)
{
	PRInt32		foundOffset = -1;
	PRInt32		patternLen = patternStr.Length();
	PRInt32		searchStrLen = searchStr.Length();
		
	if (patternLen == 0)									// pattern is empty
		return -1;
	
	if (startOffset < 0)
		startOffset = 0;
	
	if (startOffset >= searchStrLen)			// bad start offset
		return -1;
	
	if (patternLen > searchStrLen)				// pattern is longer than string to search
		return -1;
	
	const PRUnichar	*searchBuf = searchStr.GetUnicode();
	const PRUnichar	*patternBuf = patternStr.GetUnicode();

	const PRUnichar	*searchEnd = searchBuf + searchStrLen;
	const PRUnichar	*patEnd = patternBuf + patternLen;
	
	if (searchBackwards)
	{
		// searching backwards
		const PRUnichar	*s = searchEnd - patternLen - 1;
	
		while (s >= searchBuf)
		{
			if (CharsMatch(*patternBuf, *s))			// start potential match
			{
				const PRUnichar	*t = s;
				const PRUnichar	*p = patternBuf;
				PRInt32		curMatchOffset = t - searchBuf;
				PRBool		inWhitespace = nsString::IsSpace(*p);
				
				while (p < patEnd && CharsMatch(*p, *t))
				{
					if (inWhitespace && !nsString::IsSpace(*p))
					{
						// leaving p whitespace. Eat up addition whitespace in s
						while (t < searchEnd - 1 && nsString::IsSpace(*(t + 1)))
							t ++;
							
						inWhitespace = PR_FALSE;
					}
					else
						inWhitespace = nsString::IsSpace(*p);

					t ++;
					p ++;
				}
				
				if (p == patEnd)
				{
					foundOffset = curMatchOffset;
					goto done;
				}
				
				// could be smart about decrementing s here
			}
		
			s --;
		}
	
	}
	else
	{
		// searching forwards
		
		const PRUnichar	*s = &searchBuf[startOffset];
	
		while (s < searchEnd)
		{
			if (CharsMatch(*patternBuf, *s))			// start potential match
			{
				const PRUnichar	*t = s;
				const PRUnichar	*p = patternBuf;
				PRInt32		curMatchOffset = t - searchBuf;
				PRBool		inWhitespace = nsString::IsSpace(*p);
				
				while (p < patEnd && CharsMatch(*p, *t))
				{
					if (inWhitespace && !nsString::IsSpace(*p))
					{
						// leaving p whitespace. Eat up addition whitespace in s
						while (t < searchEnd - 1 && nsString::IsSpace(*(t + 1)))
							t ++;
							
						inWhitespace = PR_FALSE;
					}
					else
						inWhitespace = nsString::IsSpace(*p);

					t ++;
					p ++;
				}
				
				if (p == patEnd)
				{
					foundOffset = curMatchOffset;
					goto done;
				}
				
				// could be smart about incrementing s here
			}
			
			s ++;
		}
	
	}

done:
	return foundOffset;
}




NS_IMETHODIMP
nsFindComponent::Context::DoFind()
{
	if (!mWebShell)
		return NS_ERROR_NOT_INITIALIZED;

	nsAutoString		matchString = mSearchString;
	if (mIgnoreCase)
		matchString.ToLowerCase();
	
    nsresult	rv = NS_OK;
    nsString str;

    // Construct a text services document to use.
    nsCOMPtr<nsITextServicesDocument> txtDoc = MakeTSDocument();

	PRBool done = ( txtDoc == 0 );
	
    // Loop till we find a match or fail.
    while ( !done ) {
        // Remember whether we've reset to beginning/end.
        PRBool reset = PR_FALSE;
        // Look for next match.
        PRBool atEnd = PR_FALSE;
        while ( NS_SUCCEEDED( txtDoc->IsDone( &atEnd ) ) && !atEnd ) {
            rv = txtDoc->GetCurrentTextBlock(&str);
    
            if (NS_FAILED(rv))
                return rv;
    
            if (mIgnoreCase)
                str.ToLowerCase();
            
            PRInt32 foundOffset = FindInString(str, matchString, (mLastBlockOffset == -1) ? 0 : mLastBlockOffset + 1, mSearchBackwards);
    
            mLastBlockOffset = -1;
    
            if (foundOffset != -1) {
                // Match found.  Select it, remember where it was, and quit.
                txtDoc->SetSelection(foundOffset, mSearchString.Length());
                mLastBlockOffset = foundOffset;
                done = PR_TRUE;
                break;
            } else {
                // No match found in this block, try the next (or previous) one.
                if (mSearchBackwards) {
                    txtDoc->PrevBlock();
                    mLastBlockIndex--;
                } else {
                    txtDoc->NextBlock();
                    mLastBlockIndex++;
                }
            }
        }
        // At end (or matched).  Decide which it was...
        if ( !done ) {
            // Hit end without a match.  If we haven't passed this way already,
            // then reset to the first/last block (depending on search direction).
            if ( !reset ) {
                // Reset now.
                reset = PR_TRUE;
                // If not wrapping, give up.
                if ( !mWrapSearch ) {
                    done = PR_TRUE;
                } else {
                    if ( mSearchBackwards ) {
                        // Reset to last block.
                        rv = txtDoc->LastBlock();
                    } else {
                        // Reset to first block.
                        rv = txtDoc->FirstBlock();
                    }
                    // Reset last block index.
                    mLastBlockIndex = 0;
                    // Reset offset in block.
                    mLastBlockOffset = -1;
                }
            } else {
                // already reset.  This means no matches were found.
                done = PR_TRUE;
            }
        }
    }

	return NS_OK;
}



NS_IMETHODIMP
nsFindComponent::Context::Reset( nsIWebShell *aNewWebShell )
{

	if (!aNewWebShell)
		return NS_ERROR_INVALID_ARG;

    mWebShell = nsDontQueryInterface<nsIWebShell>( aNewWebShell );
    return NS_OK;
}


NS_IMETHODIMP
nsFindComponent::Find( nsISupports *aContext )
{
    nsresult rv = NS_OK;

    if ( aContext && mAppShell ) {
        Context *context = (Context*)aContext;

        // Open Find dialog and prompt for search parameters.
        nsIWebShellWindow *newWindow;

        // Make url for dialog xul.
        nsIURL *url;
        
        // this should be a chrome URI
        // chrome://navigator/dialogs/content/default/finddialog.xul or something.
        rv = NS_NewURL( &url, "resource:/res/samples/finddialog.xul" );

        // Create callbacks object for the find dialog.
        nsFindDialog *dialog = new nsFindDialog( this, context );

        rv = mAppShell->CreateTopLevelWindow( nsnull,
                                              url,
                                              PR_TRUE,
                                              newWindow,
                                              nsnull,
                                              dialog,
                                              425,
                                              200 );

        if ( NS_SUCCEEDED( rv ) ) {
            // Tell the dialog its nsIWebShellWindow.
            dialog->SetWindow( newWindow );
        }

        // Release the url for the xul file.
        NS_RELEASE( url );
    } else {
        rv = NS_ERROR_NULL_POINTER;
    }

    return rv;
}

NS_IMETHODIMP
nsFindComponent::FindNext(nsISupports *aContext)
{
    nsresult rv = NS_OK;

		if (!aContext)
			return NS_ERROR_NULL_POINTER;
			
		// For now, just record request to console.
		Context *context = (Context*)aContext;
#ifdef DEBUG_FIND
		printf( "nsFindComponent::FindNext\n\tkey=%s\n\tignoreCase=%ld\tsearchBackward=%ld\n",
			      (const char *)nsAutoCString( context->mSearchString ),
			      context->mIgnoreCase, context->mSearchBackwards);
#endif
		context->DoFind();


		// Record this for out-of-the-blue FindNext calls.
		mLastSearchString    = context->mSearchString;
		mLastIgnoreCase      = context->mIgnoreCase;
		mLastSearchBackwards = context->mSearchBackwards;
		mLastWrapSearch      = context->mWrapSearch;

    return rv;
}

NS_IMETHODIMP
nsFindComponent::ResetContext( nsISupports *aContext,
                               nsIWebShell *aNewWebShell ) {
    nsresult rv = NS_OK;
    if ( aContext && aNewWebShell ) {
        // Pass on the new document to the context.
        Context *context = (Context*)aContext;
        context->Reset( aNewWebShell );
    } else {
        rv = NS_ERROR_NULL_POINTER;
    }
    return rv;
}

#ifdef XP_MAC
#pragma mark -
#endif

// nsFindComponent::Context implementation...
NS_IMPL_ISUPPORTS( nsFindComponent::Context, nsISupports::GetIID() )

NS_IMPL_IAPPSHELLCOMPONENT( nsFindComponent, nsIFindComponent, NS_IFINDCOMPONENT_PROGID )
