/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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

/**
 * MODULE NOTES:
 * @update  gess 4/8/98
 * 
 *         
 */


#include "nsIDTDDebug.h"
#include "nsViewSourceHTML.h"
#include "nsCRT.h"
#include "nsParser.h"
#include "nsScanner.h"
#include "nsParserTypes.h"
#include "nsTokenHandler.h"
#include "nsDTDUtils.h"
#include "nsIContentSink.h"
#include "nsIHTMLContentSink.h"

#include "prenv.h"  //this is here for debug reasons...
#include "prtypes.h"  //this is here for debug reasons...
#include "prio.h"
#include "plstr.h"

#ifdef XP_PC
#include <direct.h> //this is here for debug reasons...
#endif
#include "prmem.h"


static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);                 
static NS_DEFINE_IID(kIDTDIID,      NS_IDTD_IID);
static NS_DEFINE_IID(kClassIID,     NS_VIEWSOURCE_HTML_IID); 


//static const char* kNullURL = "Error: Null URL given";
//static const char* kNullFilename= "Error: Null filename given";
//static const char* kNullTokenizer = "Error: Unable to construct tokenizer";
//static const char* kNullToken = "Error: Null token given";
//static const char* kInvalidTagStackPos = "Error: invalid tag stack position";
static const char* kHTMLTextContentType = "text/html";
static const char* kViewSourceCommand= "view-source";

static nsAutoString gEmpty;
static CTokenRecycler gTokenRecycler;


/**
 *  This method gets called as part of our COM-like interfaces.
 *  Its purpose is to create an interface to parser object
 *  of some type.
 *  
 *  @update   gess 4/8/98
 *  @param    nsIID  id of object to discover
 *  @param    aInstancePtr ptr to newly discovered interface
 *  @return   NS_xxx result code
 */
nsresult CViewSourceHTML::QueryInterface(const nsIID& aIID, void** aInstancePtr)  
{                                                                        
  if (NULL == aInstancePtr) {                                            
    return NS_ERROR_NULL_POINTER;                                        
  }                                                                      

  if(aIID.Equals(kISupportsIID))    {  //do IUnknown...
    *aInstancePtr = (nsIDTD*)(this);                                        
  }
  else if(aIID.Equals(kIDTDIID)) {  //do IParser base class...
    *aInstancePtr = (nsIDTD*)(this);                                        
  }
  else if(aIID.Equals(kClassIID)) {  //do this class...
    *aInstancePtr = (CViewSourceHTML*)(this);                                        
  }                 
  else {
    *aInstancePtr=0;
    return NS_NOINTERFACE;
  }
  NS_ADDREF_THIS();
  return NS_OK;                                                        
}

/**
 *  This method is defined in nsIParser. It is used to 
 *  cause the COM-like construction of an nsParser.
 *  
 *  @update  gess 4/8/98
 *  @param   nsIParser** ptr to newly instantiated parser
 *  @return  NS_xxx error result
 */
NS_HTMLPARS nsresult NS_NewViewSourceHTML(nsIDTD** aInstancePtrResult)
{
  CViewSourceHTML* it = new CViewSourceHTML();

  if (it == 0) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return it->QueryInterface(kClassIID, (void **) aInstancePtrResult);
}


NS_IMPL_ADDREF(CViewSourceHTML)
NS_IMPL_RELEASE(CViewSourceHTML)


static CTokenDeallocator gTokenKiller;


void SetFont(const char* aFace,const char* aSize,PRBool aEnable,nsIContentSink& aSink) {
  if(aEnable){
    CStartToken theFont(eHTMLTag_font);  
    nsCParserNode theFontNode(&theFont,0);
    CAttributeToken theFontAttr("face",aFace);
    theFontNode.AddAttribute(&theFontAttr);
    CAttributeToken theSizeAttr("size",aSize);
    theFontNode.AddAttribute(&theSizeAttr);
    aSink.OpenContainer(theFontNode);
  }
  else {
    CEndToken theFont(eHTMLTag_font);  
    nsCParserNode theFontNode(&theFont,0);
    aSink.CloseContainer(theFontNode);
  }
}

void SetColor(const char* aColor,PRBool aEnable,nsIContentSink& aSink) {
  if(aEnable){
    CStartToken theFont(eHTMLTag_font);  
    nsCParserNode theFontNode(&theFont,0);
    CAttributeToken theFontAttr("color",aColor);
    theFontNode.AddAttribute(&theFontAttr);
    aSink.OpenContainer(theFontNode);
  }
  else {
    CEndToken theFont(eHTMLTag_font);  
    nsCParserNode theFontNode(&theFont,0);
    aSink.CloseContainer(theFontNode);
  }
}

void SetStyle(eHTMLTags theTag,PRBool aEnable,nsIContentSink& aSink) {
  if(aEnable){
    CStartToken theToken(theTag);  
    nsCParserNode theNode(&theToken,0);
    aSink.OpenContainer(theNode);
  }
  else {
    CEndToken theToken(theTag);  
    nsCParserNode theNode(&theToken,0);
    aSink.CloseContainer(theNode);
  }
}


/**
 *  Default constructor
 *  
 *  @update  gess 4/9/98
 *  @param   
 *  @return  
 */
CViewSourceHTML::CViewSourceHTML() : nsIDTD(), mTokenDeque(gTokenKiller) {
  NS_INIT_REFCNT();
  mParser=0;
  mSink=0;
  mFilename;
  mLineNumber=0;
}

/**
 *  Default destructor
 *  
 *  @update  gess 4/9/98
 *  @param   
 *  @return  
 */
CViewSourceHTML::~CViewSourceHTML(){
  mParser=0; //just to prove we destructed...
}

/**
 * Call this method if you want the DTD to construct a fresh 
 * instance of itself. 
 * @update	gess7/23/98
 * @param 
 * @return
 */
nsresult CViewSourceHTML::CreateNewInstance(nsIDTD** aInstancePtrResult){
  return NS_NewViewSourceHTML(aInstancePtrResult);
}

/**
 * This method is called to determine if the given DTD can parse
 * a document in a given source-type. 
 * NOTE: Parsing always assumes that the end result will involve
 *       storing the result in the main content model.
 * @update	gess6/24/98
 * @param   
 * @return  TRUE if this DTD can satisfy the request; FALSE otherwise.
 */
PRBool CViewSourceHTML::CanParse(nsString& aContentType, nsString& aCommand, PRInt32 aVersion){
  PRBool result=(aContentType.Equals(kHTMLTextContentType) && (aCommand.Equals(kViewSourceCommand)));
  return result;
}


/**
 * 
 * @update	gess7/7/98
 * @param 
 * @return
 */
eAutoDetectResult CViewSourceHTML::AutoDetectContentType(nsString& aBuffer,nsString& aType){
  eAutoDetectResult result=eUnknownDetect;
  if(PR_TRUE==aType.Equals(kHTMLTextContentType)) 
    result=eValidDetect;
  return result;
}

/**
 * 
 * @update	gess5/18/98
 * @param 
 * @return
 */
NS_IMETHODIMP CViewSourceHTML::WillBuildModel(nsString& aFilename,PRBool aNotifySink){
  nsresult result=NS_OK;
  mFilename=aFilename;

  if((aNotifySink) && (mSink)) {
    mLineNumber=0;
    result = mSink->WillBuildModel();

    /* COMMENT OUT THIS BLOCK IF: you aren't using an nsHTMLContentSink...*/
    {
      nsIHTMLContentSink* theSink=(nsIHTMLContentSink*)mSink;

        //now let's automatically open the html...
      CStartToken theHTMLToken(eHTMLTag_html);
      nsCParserNode theHTMLNode(&theHTMLToken,0);
      theSink->OpenHTML(theHTMLNode);

        //now let's automatically open the body...
      CStartToken theBodyToken(eHTMLTag_body);
      nsCParserNode theBodyNode(&theBodyToken,0);
      theSink->OpenBody(theBodyNode);

      SetFont("courier","-1",PR_TRUE,*mSink);


    }
    /* COMMENT OUT THIS BLOCK IF: you aren't using an nsHTMLContentSink...*/
  }

  return result;
}

/**
 * 
 * @update	gess5/18/98
 * @param 
 * @return
 */
NS_IMETHODIMP CViewSourceHTML::DidBuildModel(PRInt32 anErrorCode,PRBool aNotifySink){
  nsresult result= NS_OK;

  //ADD CODE HERE TO CLOSE OPEN CONTAINERS...

  if((aNotifySink) && (mSink)) {

    /* COMMENT OUT THIS BLOCK IF: you aren't using an nsHTMLContentSink...*/
    {
      nsIHTMLContentSink* theSink=(nsIHTMLContentSink*)mSink;

        //now let's automatically close the pre...

      SetStyle(eHTMLTag_font,PR_FALSE,*mSink);
      
      //now let's automatically close the body...
      CEndToken theBodyToken(eHTMLTag_body);
      nsCParserNode theBodyNode(&theBodyToken,0);
      theSink->CloseBody(theBodyNode);

       //now let's automatically close the html...
      CEndToken theHTMLToken(eHTMLTag_html);
      nsCParserNode theHTMLNode(&theBodyToken,0);
      theSink->CloseHTML(theBodyNode);

      result = mSink->DidBuildModel(1);
    }
    /* COMMENT OUT THIS BLOCK IF: you aren't using an nsHTMLContentSink...*/

  }
  return result;
}

/**
 * 
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return 
 */
void CViewSourceHTML::SetParser(nsIParser* aParser) {
  mParser=(nsParser*)aParser;
}

/**
 *  This method gets called in order to set the content
 *  sink for this parser to dump nodes to.
 *  
 *  @update  gess 3/25/98
 *  @param   nsIContentSink interface for node receiver
 *  @return  
 */
nsIContentSink* CViewSourceHTML::SetContentSink(nsIContentSink* aSink) {
  nsIContentSink* old=mSink;
  mSink=aSink;
  return old;
}

static eHTMLTags gSkippedContentTags[]={ eHTMLTag_script, eHTMLTag_style,  eHTMLTag_title,  eHTMLTag_textarea};


/**
 * 
 * @update	gess11/9/98
 * @param 
 * @return
 */
NS_IMETHODIMP CViewSourceHTML::ConsumeStartTag(PRUnichar aChar,CScanner& aScanner,CToken*& aToken) {
  PRInt32 theDequeSize=mTokenDeque.GetSize();
  nsresult result=NS_OK;

  aToken=gTokenRecycler.CreateTokenOfType(eToken_start,eHTMLTag_unknown,gEmpty);
  
  if(aToken) {
    result= aToken->Consume(aChar,aScanner);  //tell new token to finish consuming text...    
    if(NS_OK==result) {
      if(((CStartToken*)aToken)->IsAttributed()) {
        result=ConsumeAttributes(aChar,aScanner,(CStartToken*)aToken);
      }

      //EEEEECCCCKKKK!!! 
      //This code is confusing, so pay attention.
      //If you're here, it's because we were in the midst of consuming a start
      //tag but ran out of data (not in the stream, but in this *part* of the stream.
      //For simplicity, we have to unwind our input. Therefore, we pop and discard
      //any new tokens we've cued this round. Later we can get smarter about this.
      if(NS_OK!=result) {
        while(mTokenDeque.GetSize()>theDequeSize) {
          delete (CToken*)mTokenDeque.PopBack();
        }
      }


    } //if
  } //if
  return result;
}

/**
 *  This method is called just after a known text char has
 *  been consumed and we should read a text run.
 *  
 *  @update gess 3/25/98
 *  @param  aChar: last char read
 *  @param  aScanner: see nsScanner.h
 *  @param  anErrorCode: arg that will hold error condition
 *  @return new token or null 
 */
NS_IMETHODIMP CViewSourceHTML::ConsumeText(const nsString& aString,CScanner& aScanner,CToken*& aToken){
  nsresult result=NS_OK;
  aToken=gTokenRecycler.CreateTokenOfType(eToken_text,eHTMLTag_text,aString);
  if(aToken) {
    PRUnichar ch=0;
    result=aToken->Consume(ch,aScanner);
    if(result) {
      nsString& temp=aToken->GetStringValueXXX();
      if(0==temp.Length()){
        delete aToken;
        aToken = nsnull;
      }
      else result=kNoError;
    }
  }
  return result;
}

/**
 *  This method is called just after we've consumed a start
 *  tag, and we now have to consume its attributes.
 *  
 *  @update  gess 3/25/98
 *  @param   aChar: last char read
 *  @param   aScanner: see nsScanner.h
 *  @return  
 */
NS_IMETHODIMP CViewSourceHTML::ConsumeAttributes(PRUnichar aChar,CScanner& aScanner,CStartToken* aToken) {
  PRBool done=PR_FALSE;
  nsresult result=NS_OK;
  PRInt16 theAttrCount=0;

  while((!done) && (result==NS_OK)) {
    CAttributeToken* theToken= (CAttributeToken*)gTokenRecycler.CreateTokenOfType(eToken_attribute,eHTMLTag_unknown,gEmpty);
    if(theToken){
      result=theToken->Consume(aChar,aScanner);  //tell new token to finish consuming text...    
 
      //Much as I hate to do this, here's some special case code.
      //This handles the case of empty-tags in XML. Our last
      //attribute token will come through with a text value of ""
      //and a textkey of "/". We should destroy it, and tell the 
      //start token it was empty.
      nsString& key=theToken->GetKey();
      nsString& text=theToken->GetStringValueXXX();
      if((key[0]==kForwardSlash) && (0==text.Length())){
        //tada! our special case! Treat it like an empty start tag...
        aToken->SetEmpty(PR_TRUE);
        delete theToken;
      }
      else if(NS_OK==result){
        theAttrCount++;
        mTokenDeque.Push(theToken);
      }//if
      else delete theToken; //we can't keep it...
    }//if
    
    if(NS_OK==result){
      result=aScanner.Peek(aChar);
      if(aChar==kGreaterThan) { //you just ate the '>'
        aScanner.GetChar(aChar); //skip the '>'
        done=PR_TRUE;
      }//if
    }//if
  }//while

  aToken->SetAttributeCount(theAttrCount);
  return result;
}

/**
 *  This method is called just after a "&" has been consumed 
 *  and we know we're at the start of an entity.  
 *  
 *  @update gess 3/25/98
 *  @param  aChar: last char read
 *  @param  aScanner: see nsScanner.h
 *  @param  anErrorCode: arg that will hold error condition
 *  @return new token or null 
 */
NS_IMETHODIMP CViewSourceHTML::ConsumeEntity(PRUnichar aChar,CScanner& aScanner,CToken*& aToken) {
   PRUnichar  ch;
   nsresult result=aScanner.GetChar(ch);

   if(NS_OK==result) {
     if(nsString::IsAlpha(ch)) { //handle common enity references &xxx; or &#000.
       aToken = gTokenRecycler.CreateTokenOfType(eToken_entity,eHTMLTag_entity,gEmpty);
       result = aToken->Consume(ch,aScanner);  //tell new token to finish consuming text...    
     }
     else if(kHashsign==ch) {
       aToken = gTokenRecycler.CreateTokenOfType(eToken_entity,eHTMLTag_entity,gEmpty);
       result=aToken->Consume(0,aScanner);
     }
     else {
       //oops, we're actually looking at plain text...
       nsAutoString temp("&");
       temp.Append(ch);
       result=ConsumeText(temp,aScanner,aToken);
     }
   }//if
   return result;
}

/**
 *  This method is called just after whitespace has been 
 *  consumed and we know we're at the start a whitespace run.  
 *  
 *  @update gess 3/25/98
 *  @param  aChar: last char read
 *  @param  aScanner: see nsScanner.h
 *  @param  anErrorCode: arg that will hold error condition
 *  @return new token or null 
 */
NS_IMETHODIMP CViewSourceHTML::ConsumeWhitespace(PRUnichar aChar,CScanner& aScanner,CToken*& aToken) {
  aToken = gTokenRecycler.CreateTokenOfType(eToken_whitespace,eHTMLTag_whitespace,gEmpty);
  nsresult result=kNoError;
  if(aToken) {
     result=aToken->Consume(aChar,aScanner);
  }
  return kNoError;
}

/**
 *  This method is called just after a "<!" has been consumed 
 *  and we know we're at the start of a comment.  
 *  
 *  @update gess 3/25/98
 *  @param  aChar: last char read
 *  @param  aScanner: see nsScanner.h
 *  @param  anErrorCode: arg that will hold error condition
 *  @return new token or null 
 */
NS_IMETHODIMP CViewSourceHTML::ConsumeComment(PRUnichar aChar,CScanner& aScanner,CToken*& aToken){
  aToken = gTokenRecycler.CreateTokenOfType(eToken_comment,eHTMLTag_comment,gEmpty);
  nsresult result=NS_OK;
  if(aToken) {
     result=aToken->Consume(aChar,aScanner);
  }
  return result;
}

/**
 *  This method is called just after a newline has been consumed. 
 *  
 *  @update gess 3/25/98
 *  @param  aChar: last char read
 *  @param  aScanner: see nsScanner.h
 *  @param  aToken is the newly created newline token that is parsing
 *  @return error code
 */
NS_IMETHODIMP CViewSourceHTML::ConsumeNewline(PRUnichar aChar,CScanner& aScanner,CToken*& aToken){
  aToken=gTokenRecycler.CreateTokenOfType(eToken_newline,eHTMLTag_newline,gEmpty);
  nsresult result=NS_OK;
  if(aToken) {
    result=aToken->Consume(aChar,aScanner);
  }
  return kNoError;
}

/**
 *  This method is called just after a "<" has been consumed 
 *  and we know we're at the start of some kind of tagged 
 *  element. We don't know yet if it's a tag or a comment.
 *  
 *  @update  gess 5/12/98
 *  @param   aChar is the last char read
 *  @param   aScanner is represents our input source
 *  @param   aToken is the out arg holding our new token
 *  @return  error code (may return kInterrupted).
 */
NS_IMETHODIMP CViewSourceHTML::ConsumeTag(PRUnichar aChar,CScanner& aScanner,CToken*& aToken) {

  nsresult result=aScanner.GetChar(aChar);

  if(NS_OK==result) {

    switch(aChar) {
      case kForwardSlash:
        PRUnichar ch; 
        result=aScanner.Peek(ch);
        if(NS_OK==result) {
          if(nsString::IsAlpha(ch))
            aToken=gTokenRecycler.CreateTokenOfType(eToken_end,eHTMLTag_unknown,gEmpty);
          else aToken=gTokenRecycler.CreateTokenOfType(eToken_comment,eHTMLTag_unknown,gEmpty);
        }//if
        break;

      case kExclamation:
        aToken=gTokenRecycler.CreateTokenOfType(eToken_comment,eHTMLTag_comment,gEmpty);
        break;

      case kQuestionMark: //it must be an XML processing instruction...
        aToken=gTokenRecycler.CreateTokenOfType(eToken_instruction,eHTMLTag_unknown,gEmpty);
        break;

      default:
        if(nsString::IsAlpha(aChar))
          return ConsumeStartTag(aChar,aScanner,aToken);
        else if(kEOF!=aChar) {
          nsAutoString temp("<");
          return ConsumeText(temp,aScanner,aToken);
        }
    } //switch

    if((0!=aToken) && (NS_OK==result)) {
      result= aToken->Consume(aChar,aScanner);  //tell new token to finish consuming text...    
      if(result) {
        delete aToken;
        aToken=0;
      }
    } //if
  } //if
  return result;
}


/**
 *  This method repeatedly called by the tokenizer. 
 *  Each time, we determine the kind of token were about to 
 *  read, and then we call the appropriate method to handle
 *  that token type.
 *  
 *  @update gess 3/25/98
 *  @param  aChar: last char read
 *  @param  aScanner: see nsScanner.h
 *  @param  anErrorCode: arg that will hold error condition
 *  @return new token or null 
 */
NS_IMETHODIMP CViewSourceHTML::ConsumeToken(CToken*& aToken){
  aToken=0;
  if(mTokenDeque.GetSize()>0) {
    aToken=(CToken*)mTokenDeque.Pop();
    return NS_OK;
  }

  nsresult   result=NS_OK;
  CScanner* theScanner=mParser->GetScanner();
  if(NS_OK==result){
    PRUnichar theChar;
    result=theScanner->GetChar(theChar);
    switch(result) {
      case kEOF:
          //We convert from eof to complete here, because we never really tried to get data.
          //All we did was try to see if data was available, which it wasn't.
          //It's important to return process complete, so that controlling logic can know that
          //everything went well, but we're done with token processing.
        result=kProcessComplete;
        break;

      case kInterrupted:
        theScanner->RewindToMark();
        break; 

      case NS_OK:
      default:
        switch(theChar) {
          case kLessThan:
            result=ConsumeTag(theChar,*theScanner,aToken);
            break;

          case kAmpersand:
            result=ConsumeEntity(theChar,*theScanner,aToken);
            break;
          
          case kCR: case kLF:
            result=ConsumeNewline(theChar,*theScanner,aToken);
            break;
          
          case kNotFound:
            break;
          
          default:
            if(!nsString::IsSpace(theChar)) {
              nsAutoString temp(theChar);
              result=ConsumeText(temp,*theScanner,aToken);
              break;
            }
            result=ConsumeWhitespace(theChar,*theScanner,aToken);
            break;
        } //switch
        break; 
    } //switch
//    if(NS_OK==result)
//      result=theScanner->Eof(); 
  } //if
  return result;
}

/**
 * 
 * @update	gess5/18/98
 * @param 
 * @return
 */
NS_IMETHODIMP CViewSourceHTML::WillResumeParse(void){
  nsresult result = NS_OK;
  if(mSink) {
    result = mSink->WillResume();
  }
  return result;
}

/**
 * 
 * @update	gess5/18/98
 * @param 
 * @return
 */
NS_IMETHODIMP CViewSourceHTML::WillInterruptParse(void){
  nsresult result = NS_OK;
  if(mSink) {
    result = mSink->WillInterrupt();
  }
  return result;
}

/**
 * Called by the parser to initiate dtd verification of the
 * internal context stack.
 * @update	gess 7/23/98
 * @param 
 * @return
 */
PRBool CViewSourceHTML::Verify(nsString& aURLRef){
  PRBool result=PR_TRUE;
  return result;
}

/**
 * Called by the parser to enable/disable dtd verification of the
 * internal context stack.
 * @update	gess 7/23/98
 * @param 
 * @return
 */
void CViewSourceHTML::SetVerification(PRBool aEnabled){
}

/**
 *  This method is called to determine whether or not a tag
 *  of one type can contain a tag of another type.
 *  
 *  @update  gess 3/25/98
 *  @param   aParent -- int tag of parent container
 *  @param   aChild -- int tag of child container
 *  @return  PR_TRUE if parent can contain child
 */
PRBool CViewSourceHTML::CanContain(PRInt32 aParent,PRInt32 aChild) const{
  PRBool result=PR_TRUE;
  return result;
}

/**
 *  This method gets called to determine whether a given 
 *  tag is itself a container
 *  
 *  @update  gess 3/25/98
 *  @param   aTag -- tag to test for containership
 *  @return  PR_TRUE if given tag can contain other tags
 */
PRBool CViewSourceHTML::IsContainer(PRInt32 aTag) const{
  PRBool result=PR_TRUE;
  return result;
}


/**
 *  This method gets called to determine whether a given 
 *  tag is itself a container
 *  
 *  @update  gess 3/25/98
 *  @param   aTag -- tag to test for containership
 *  @return  PR_TRUE if given tag can contain other tags
 */
PRBool EmitTag(nsCParserNode& aNode,nsIContentSink& aSink,PRBool anEndToken) {
  static nsString     theString;
  static nsAutoString theLTEntity("lt");
  static nsAutoString theGTEntity("gt");
  static const char*  theColors[]={"purple","red"};

  PRBool result=PR_TRUE;

  CEntityToken theStartEntityToken(theLTEntity);  
  nsCParserNode theStartNode(&theStartEntityToken,aNode.GetSourceLineNumber());
  aSink.AddLeaf(theStartNode);

  SetStyle(eHTMLTag_b,PR_TRUE,aSink);
  SetColor(theColors[eHTMLTag_userdefined==aNode.GetNodeType()],PR_TRUE,aSink);

  if(anEndToken)
    theString="/";
  else theString="";
  theString.Append(aNode.GetText());
  {
    CTextToken theToken(theString);  
    nsCParserNode theNode(&theToken,aNode.GetSourceLineNumber());
    aSink.AddLeaf(theNode);
  }
  SetStyle(eHTMLTag_font,PR_FALSE,aSink);
  SetStyle(eHTMLTag_b,PR_FALSE,aSink);

  PRInt32 theCount=aNode.GetAttributeCount();
  if(0<theCount){
    PRInt32 theIndex=0;
    for(theIndex=0;theIndex<theCount;theIndex++){
      
       //begin by writing the key...
      {
        SetStyle(eHTMLTag_b,PR_TRUE,aSink);
        theString=" ";
        theString.Append(aNode.GetKeyAt(theIndex));
        CTextToken theToken(theString);  
        nsCParserNode theNode(&theToken,aNode.GetSourceLineNumber());
        aSink.AddLeaf(theNode);
        SetStyle(eHTMLTag_b,PR_FALSE,aSink);
      }

      { //next write the equal sign...
        theString="=";
        CTextToken theToken(theString);  
        nsCParserNode theNode(&theToken,aNode.GetSourceLineNumber());
        aSink.AddLeaf(theNode);
      }

       //begin by writing the value...
      {
        SetColor("blue",PR_TRUE,aSink);
        theString=aNode.GetValueAt(theIndex);
        CTextToken theToken(theString);  
        nsCParserNode theNode(&theToken,aNode.GetSourceLineNumber());
        aSink.AddLeaf(theNode);
        SetStyle(eHTMLTag_font,PR_FALSE,aSink);
      }
    }
  }

  CEntityToken theEndEntityToken(theGTEntity);  
  nsCParserNode theEndNode(&theEndEntityToken,aNode.GetSourceLineNumber());
  aSink.AddLeaf(theEndNode);

  return result;
}

/**
 *  
 *  @update  gess 3/25/98
 *  @param   aToken -- token object to be put into content model
 *  @return  0 if all is well; non-zero is an error
 */
NS_IMETHODIMP CViewSourceHTML::HandleToken(CToken* aToken) {
  nsresult        result=NS_OK;
  CHTMLToken*     theToken= (CHTMLToken*)(aToken);
  eHTMLTokenTypes theType= (eHTMLTokenTypes)theToken->GetTokenType();
  PRBool          theEndTag=PR_TRUE;

  nsCParserNode theNode(theToken,mLineNumber);
  switch(theType) {

    case eToken_newline:
      mLineNumber++; //now fall through
      {
        CStartToken theBRToken(eHTMLTag_br);
        nsCParserNode theBRNode(&theBRToken,mLineNumber);
        result=mSink->AddLeaf(theBRNode); 
      }
      break;

    case eToken_whitespace:
      {
        PRInt32 theLength=aToken->GetStringValueXXX().Length();
        CEntityToken theEntity("nbsp");
        nsCParserNode theEntityNode(&theEntity,mLineNumber);
        int theIndex;
        for(theIndex=0;theIndex<theLength;theIndex++)
          result=mSink->AddLeaf(theEntityNode); 
      }
      break;

    case eToken_entity:
    case eToken_text:
      result=mSink->AddLeaf(theNode); 
      break;

    case eToken_comment:
      {
        SetColor("green",PR_TRUE,*mSink);
        CTextToken theTextToken(aToken->GetStringValueXXX());
        nsCParserNode theTextNode(&theTextToken,mLineNumber);
        result=mSink->AddLeaf(theTextNode); 
        SetStyle(eHTMLTag_font,PR_FALSE,*mSink);
      }
      break;
    
    case eToken_instruction:
      result=mSink->AddProcessingInstruction(theNode); 
      break;

    case eToken_start:
      {
        PRInt16 attrCount=aToken->GetAttributeCount();
        theEndTag=PR_FALSE;
        if(0<attrCount){ //go collect the attributes...
          int attr=0;
          for(attr=0;attr<attrCount;attr++){
            CToken* theToken=mParser->PeekToken();
            if(theToken)  {
              eHTMLTokenTypes theType=eHTMLTokenTypes(theToken->GetTokenType());
              if(eToken_attribute==theType){
                mParser->PopToken(); //pop it for real...
                theNode.AddAttribute(theToken);
              } 
            }
            else return kInterrupted;
          }
        }
      }
      //intentionally fall through...

    case eToken_end:
      EmitTag(theNode,*mSink,theEndTag);
      break;

    case eToken_style:
    case eToken_skippedcontent:
    default:
      result=NS_OK;
  }//switch
  return result;
}


/**
 * 
 * @update	gess8/4/98
 * @param 
 * @return
 */
nsITokenRecycler* CViewSourceHTML::GetTokenRecycler(void){
  return &gTokenRecycler;
}
