/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
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

/**
 * MODULE NOTES:
 * @update  gess 4/8/98
 * 
 *         
 */

#ifndef __NS_WELLFORMED_DTD
#define __NS_WELLFORMED_DTD

#include "nsIDTD.h"
#include "nsISupports.h"
#include "nsHTMLTokens.h"
#include "nsIContentSink.h"

#define NS_WELLFORMED_DTD_IID      \
  {0xa39c6bfd, 0x15f0,  0x11d2, \
  {0x80, 0x41, 0x0, 0x10, 0x4b, 0x98, 0x3f, 0xd4}}


class nsIDTDDebug;
class nsIParserNode;
class nsParser;
class nsHTMLTokenizer;


class CWellFormedDTD : public nsIDTD
{
public:

    NS_DECL_ISUPPORTS


    /**
     *  
     *  
     *  @update  gess 4/9/98
     *  @param   
     *  @return  
     */
    CWellFormedDTD();

    /**
     *  
     *  
     *  @update  gess 4/9/98
     *  @param   
     *  @return  
     */
    virtual ~CWellFormedDTD();

    NS_IMETHOD_(const nsIID&) GetMostDerivedIID(void) const;

    /**
     * Call this method if you want the DTD to construct a clone of itself.
     * @update	gess7/23/98
     * @param 
     * @return
     */
    NS_IMETHOD CreateNewInstance(nsIDTD** aInstancePtrResult);

    /**
     * This method is called to determine if the given DTD can parse
     * a document in a given source-type. 
     * NOTE: Parsing always assumes that the end result will involve
     *       storing the result in the main content model.
     * @update	gess6/24/98
     * @param   
     * @return  TRUE if this DTD can satisfy the request; FALSE otherwise.
     */
    NS_IMETHOD_(eAutoDetectResult) CanParse(CParserContext& aParserContext,
                                            const nsString& aBuffer,
                                            PRInt32 aVersion);

    /**
      * The parser uses a code sandwich to wrap the parsing process. Before
      * the process begins, WillBuildModel() is called. Afterwards the parser
      * calls DidBuildModel(). 
      * @update	rickg 03.20.2000
      * @param	aParserContext
      * @param	aSink
      * @return	error code (almost always 0)
      */
    NS_IMETHOD WillBuildModel(const CParserContext& aParserContext,
                              nsIContentSink* aSink);

    /**
      * The parser uses a code sandwich to wrap the parsing process. Before
      * the process begins, WillBuildModel() is called. Afterwards the parser
      * calls DidBuildModel(). 
      * @update	gess5/18/98
      * @param	aFilename is the name of the file being parsed.
      * @return	error code (almost always 0)
      */
    NS_IMETHOD BuildModel(nsIParser* aParser, nsITokenizer* aTokenizer,
                          nsITokenObserver* anObserver = nsnull,
                          nsIContentSink* aSink = nsnull);

   /**
     * The parser uses a code sandwich to wrap the parsing process. Before
     * the process begins, WillBuildModel() is called. Afterwards the parser
     * calls DidBuildModel(). 
     * @update	gess5/18/98
     * @param	anErrorCode contans the last error that occured
     * @return	error code
     */
    NS_IMETHOD DidBuildModel(nsresult anErrorCode, PRBool aNotifySink,
                             nsIParser* aParser,
                             nsIContentSink* aSink = nsnull);

    /**
     *  
     *  @update  gess 3/25/98
     *  @param   aToken -- token object to be put into content model
     *  @return  0 if all is well; non-zero is an error
     */
    NS_IMETHOD HandleToken(CToken* aToken, nsIParser* aParser);

    /**
     * 
     * @update	gess 12/20/99
     * @param   ptr-ref to (out) tokenizer
     * @return  nsresult
     */
    NS_IMETHOD  GetTokenizer(nsITokenizer*& aTokenizer);

    /**
     * 
     * @update	gess5/18/98
     * @param 
     * @return
     */
    NS_IMETHOD WillResumeParse(void);

    /**
     * 
     * @update	gess5/18/98
     * @param 
     * @return
     */
    NS_IMETHOD WillInterruptParse(void);

    /**
     * Set this to TRUE if you want the DTD to verify its
     * context stack.
     * @update	gess 7/23/98
     * @param 
     * @return
     */
    virtual void SetVerification(PRBool aEnable);

    /**
     *  This method is called to determine whether or not a tag
     *  of one type can contain a tag of another type.
     *  
     *  @update  gess 3/25/98
     *  @param   aParent -- int tag of parent container
     *  @param   aChild -- int tag of child container
     *  @return  PR_TRUE if parent can contain child
     */
    NS_IMETHOD_(PRBool) CanContain(PRInt32 aParent, PRInt32 aChild) const;

    /**
     *  This method gets called to determine whether a given 
     *  tag is itself a container
     *  
     *  @update  gess 3/25/98
     *  @param   aTag -- tag to test for containership
     *  @return  PR_TRUE if given tag can contain other tags
     */
    NS_IMETHOD_(PRBool) IsContainer(PRInt32 aTag) const;

    /**
     * Retrieve a ptr to the global token recycler...
     * @update	gess8/4/98
     * @return  ptr to recycler (or null)
     */
    NS_IMETHOD_(nsTokenAllocator *) GetTokenAllocator(void);

    /**
     * Use this id you want to stop the building content model
     * --------------[ Sets DTD to STOP mode ]----------------
     * It's recommended to use this method in accordance with
     * the parser's terminate() method.
     *
     * @update	harishd 07/22/99
     * @param 
     * @return
     */
    NS_IMETHOD Terminate(nsIParser* aParser = nsnull);

    /**
     * Give rest of world access to our tag enums, so that CanContain(), etc,
     * become useful.
     */
    NS_IMETHOD StringTagToIntTag(const nsAReadableString &aTag,
                                 PRInt32* aIntTag) const;

    NS_IMETHOD_(const PRUnichar *) IntTagToStringTag(PRInt32 aIntTag) const;
  
    NS_IMETHOD ConvertEntityToUnicode(const nsAReadableString& aEntity,
                                      PRInt32* aUnicode) const;

    NS_IMETHOD_(PRBool) IsBlockElement(PRInt32 aTagID,
                                       PRInt32 aParentID) const
    {
        return PR_FALSE;
    }

    NS_IMETHOD_(PRBool) IsInlineElement(PRInt32 aTagID,
                                        PRInt32 aParentID) const
    {
        return PR_FALSE;
    }

protected:
/*
    NS_IMETHODIMP ConsumeTag(PRUnichar aChar,nsScanner& aScanner,CToken*& aToken);
    NS_IMETHODIMP ConsumeStartTag(PRUnichar aChar,nsScanner& aScanner,CToken*& aToken);
    NS_IMETHODIMP ConsumeText(const nsString& aString,nsScanner& aScanner,CToken*& aToken);
    NS_IMETHODIMP ConsumeNewline(PRUnichar aChar,nsScanner& aScanner,CToken*& aToken);
    NS_IMETHODIMP ConsumeWhitespace(PRUnichar aChar,nsScanner& aScanner,CToken*& aToken);
    NS_IMETHODIMP ConsumeEscapedContent(PRUnichar aChar,nsScanner& aScanner,CToken*& aToken);
    NS_IMETHODIMP ConsumeComment(PRUnichar aChar,nsScanner& aScanner,CToken*& aToken);
    NS_IMETHODIMP ConsumeEntity(PRUnichar aChar,nsScanner& aScanner,CToken*& aToken);
    NS_IMETHODIMP ConsumeAttributes(PRUnichar aChar,nsScanner& aScanner,CStartToken* aToken);
*/
    nsresult    HandleStartToken(CToken* aToken);
    nsresult    HandleEndToken(CToken* aToken);
    nsresult    HandleCommentToken(CToken* aToken);
    nsresult    HandleErrorToken(CToken* aToken);
    nsresult    HandleDocTypeDeclToken(CToken* aToken);
    nsresult    HandleLeafToken(CToken* aToken);
    nsresult    HandleProcessingInstructionToken(CToken* aToken);

    nsParser*           mParser;
    nsIContentSink*     mSink;
    nsString            mFilename;
    PRInt32             mLineNumber;
    nsHTMLTokenizer*    mTokenizer;
    nsresult            mDTDState;
};

extern nsresult NS_NewWellFormed_DTD(nsIDTD** aInstancePtrResult);

#endif 



