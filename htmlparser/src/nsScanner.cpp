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

//#define __INCREMENTAL 1

#include "nsScanner.h"
#include "nsDebug.h"

const char* kBadHTMLText="<H3>Oops...</H3>You just tried to read a non-existent document: <BR>";

#ifdef __INCREMENTAL
const int   kBufsize=1;
#else
const int   kBufsize=64;
#endif


/**
 *  Use this constructor if you want i/o to be based on an
 *  incremental netstream. If you pass a null filename, you
 *  can still provide data to the scanner via append.
 *
 *  @update  gess 5/12/98
 *  @param   aFilename --
 *  @return  
 */
CScanner::CScanner(nsString& aFilename,PRBool aCreateStream) : 
    mBuffer(""), mFilename(aFilename) 
{
  mOffset=0;
  mMarkPos=-1;
  mTotalRead=0;
  mOwnsStream=aCreateStream;
  mFileStream=0;
  if(aCreateStream) {
    char buffer[513];
    aFilename.ToCString(buffer,sizeof(buffer)-1);
    #if defined(XP_UNIX) && (defined(IRIX) || defined(MKLINUX))
      /* XXX: IRIX does not support ios::binary */
      mFileStream=new fstream(buffer,ios::in);
    #else
      mFileStream=new fstream(buffer,ios::in|ios::binary);
    #endif
  } //if
}

/**
 *  Use this constructor if you want i/o to be stream based.
 *
 *  @update  gess 5/12/98
 *  @param   aStream --
 *  @param   assumeOwnership --
 *  @param   aFilename --
 *  @return  
 */
CScanner::CScanner(nsString& aFilename,fstream& aStream,PRBool assumeOwnership) :
    mBuffer(""), mFilename(aFilename)
{    
  mOffset=0;
  mMarkPos=-1;
  mTotalRead=0;
  mOwnsStream=assumeOwnership;
  mFileStream=&aStream;
}


/**
 *  default destructor
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  
 */
CScanner::~CScanner() {
  if(mFileStream) {
    mFileStream->close();
    if(mOwnsStream)
      delete mFileStream;
  }
  mFileStream=0;
}

/**
 *  Resets current offset position of input stream to marked position. 
 *  This allows us to back up to this point if the need should arise, 
 *  such as when tokenization gets interrupted.
 *  NOTE: IT IS REALLY BAD FORM TO CALL RELEASE WITHOUT CALLING MARK FIRST!
 *
 *  @update  gess 5/12/98
 *  @param   
 *  @return  
 */
PRInt32 CScanner::RewindToMark(void){
  mOffset=mMarkPos;
  return mOffset;
}

/**
 *  Records current offset position in input stream. This allows us
 *  to back up to this point if the need should arise, such as when
 *  tokenization gets interrupted.
 *
 *  @update  gess 5/12/98
 *  @param   
 *  @return  
 */
PRInt32 CScanner::Mark(void){
  mMarkPos=mOffset;
  return mMarkPos;
}


/**
 *  
 *  @update  gess 5/12/98
 */
void _PreCompressBuffer(nsString& aBuffer,PRInt32& anOffset,PRInt32& aMarkPos){
  //To determine how much of our internal buffer to truncate, 
  //we should check mMarkPos. That represents the point at which
  //we've guaranteed the client we can back up to, so make sure
  //you don't lose any of the data beyond that point.

/*
  if((anOffset!=aMarkPos) && (0<=aMarkPos)) {
    if(aMarkPos>0) {
      aBuffer.Cut(0,aMarkPos);
      if(anOffset>aMarkPos)
        anOffset-=aMarkPos;
    }
  }
  else aBuffer.Truncate();
*/
  PRInt32 len=aBuffer.Length();
  if((aMarkPos<len) && (aMarkPos>=0)) {
    aBuffer.Cut(0,aMarkPos);
    anOffset-=aMarkPos;
  }
  else {
    aBuffer.Truncate();
    anOffset=0;
  }
  aMarkPos=0;
}


/** 
 * Append data to our underlying input buffer as
 * if it were read from an input stream.
 *
 * @update  gess4/3/98
 * @return  error code
 */
PRBool CScanner::Append(nsString& aBuffer) {
  _PreCompressBuffer(mBuffer,mOffset,mMarkPos);
  mBuffer.Append(aBuffer);
  mTotalRead+=aBuffer.Length();
  return PR_TRUE;
}

/**
 *  
 *  
 *  @update  gess 5/21/98
 *  @param   
 *  @return  
 */
PRBool CScanner::Append(const char* aBuffer, PRInt32 aLen){
  _PreCompressBuffer(mBuffer,mOffset,mMarkPos);
  mBuffer.Append(aBuffer,aLen);
  mTotalRead+=aLen;
  return PR_TRUE;
}

/** 
 * Grab data from underlying stream.
 *
 * @update  gess4/3/98
 * @return  error code
 */
PRInt32 CScanner::FillBuffer(void) {
  PRInt32 anError=0;

  _PreCompressBuffer(mBuffer,mOffset,mMarkPos);

  if(!mFileStream) {
    //This is DEBUG code!!!!!!  XXX DEBUG XXX
    //If you're here, it means someone tried to load a
    //non-existent document. So as a favor, we emit a
    //little bit of HTML explaining the error.
    if(0==mTotalRead) {
      mBuffer.Append((const char*)kBadHTMLText);
      mBuffer.Append(mFilename);
    }
    else return kInterrupted;
  }
  else {
    PRInt32 numread=0;
    char buf[kBufsize+1];
    buf[kBufsize]=0;

    if(mFileStream) {
      mFileStream->read(buf,kBufsize);
      numread=mFileStream->gcount();
    }
    mOffset=mBuffer.Length();
    if((0<numread) && (0==anError))
      mBuffer.Append((const char*)buf,numread);
    mTotalRead+=mBuffer.Length();
  }

  return anError;
}

/**
 *  determine if the scanner has reached EOF
 *  
 *  @update  gess 5/12/98
 *  @param   
 *  @return  0=!eof 1=eof kInterrupted=interrupted
 */
PRInt32 CScanner::Eof() {
  PRInt32 theError=0;

  if(mOffset>=mBuffer.Length()) {
    theError=FillBuffer();  
  }
  
  if(0==theError) 
    return (0==mBuffer.Length());

  return theError;
}

/**
 *  retrieve next char from scanners internal input stream
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  error code reflecting read status
 */
PRInt32 CScanner::GetChar(PRUnichar& aChar) {
  PRInt32 result=Eof();
  if(!result) {
    aChar=mBuffer[mOffset++];
    result=kNoError;
  }
  return result;
}


/**
 *  peek ahead to consume next char from scanner's internal
 *  input buffer
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  
 */
PRInt32 CScanner::Peek(PRUnichar& aChar) {
  PRInt32 result=Eof();
  if(!result) {
    aChar=mBuffer[mOffset];        
    result=kNoError;
  }
  return result;
}


/**
 *  Push the given char back onto the scanner
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  error code
 */
PRInt32 CScanner::PutBack(PRUnichar aChar) {
  if(mOffset>0)
    mOffset--;
  else mBuffer.Insert(aChar,0);
  return kNoError;
}


/**
 *  Skip whitespace on scanner input stream
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  error status
 */
PRInt32 CScanner::SkipWhitespace(void) {
  static nsAutoString chars(" \n\r\t");
  return SkipOver(chars);
}

/**
 *  Skip over chars as long as they equal given char
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  error code
 */
PRInt32 CScanner::SkipOver(PRUnichar aSkipChar){
  PRUnichar ch=0;
  PRInt32   result=kNoError;

  while(kNoError==result) {
    result=GetChar(ch);
    if(!result) {
      if(ch!=aSkipChar) {
        PutBack(ch);
        break;
      }
    } 
    else break;
  } //while
  return result;
}

/**
 *  Skip over chars as long as they're in aSkipSet
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  error code
 */
PRInt32 CScanner::SkipOver(nsString& aSkipSet){
  PRUnichar ch=0;
  PRInt32   result=kNoError;

  while(kNoError==result) {
    result=GetChar(ch);
    if(!result) {
      PRInt32 pos=aSkipSet.Find(ch);
      if(kNotFound==pos) {
        PutBack(ch);
        break;
      }
    } 
    else break;
  } //while
  return result;
}

/**
 *  Skip over chars as long as they're in aValidSet
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  error code
 */
PRInt32 CScanner::SkipPast(nsString& aValidSet){
  NS_NOTYETIMPLEMENTED("Error: SkipPast not yet implemented.");
  return kNoError;
}

/**
 *  Consume chars as long as they are <i>in</i> the 
 *  given validSet of input chars.
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  error code
 */
PRInt32 CScanner::ReadWhile(nsString& aString,nsString& aValidSet,PRBool addTerminal){
  PRUnichar theChar=0;
  PRInt32   result=kNoError;
  PRInt32   wrPos=0;

  while(kNoError==result) {
    result=GetChar(theChar);
    if(kNoError==result) {
      PRInt32 pos=aValidSet.Find(theChar);
      if(kNotFound==pos) {
        if(addTerminal)
          aString+=theChar;
        else PutBack(theChar);
        break;
      }
      else aString+=theChar;
    }
  }
  return result;
}


/**
 *  Consume characters until you find one contained in given
 *  input set.
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  error code
 */
PRInt32 CScanner::ReadUntil(nsString& aString,nsString& aTerminalSet,PRBool addTerminal){
  PRUnichar ch=0;
  PRInt32   result=kNoError;

  while(!result) {
    result=GetChar(ch);
    if(kNoError==result) {
      PRInt32 pos=aTerminalSet.Find(ch);
      if(kNotFound!=pos) {
        if(addTerminal)
          aString+=ch;
        else PutBack(ch);
        break;
      }
      else aString+=ch;
    }
  }
  return result;
}


/**
 *  Consumes chars until you see the given terminalChar
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  error code
 */
PRInt32 CScanner::ReadUntil(nsString& aString,PRUnichar aTerminalChar,PRBool addTerminal){
  PRUnichar ch=0;
  PRInt32   result=kNoError;

  while(kNoError==result) {
    result=GetChar(ch);
    if(ch==aTerminalChar) {
      if(addTerminal)
        aString+=ch;
      else PutBack(ch);
      break;
    }
    else aString+=ch;
  }
  return result;
}

/**
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  
 */
nsString& CScanner::GetBuffer(void) {
  return mBuffer;
}

/**
 *  Retrieve the name of the file that the scanner is reading from.
 *  In some cases, it's just a given name, because the scanner isn't
 *  really reading from a file.
 *  
 *  @update  gess 5/12/98
 *  @return  
 */
nsString& CScanner::GetFilename(void) {
  return mFilename;
}

/**
 *  Conduct self test. Actually, selftesting for this class
 *  occurs in the parser selftest.
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  
 */

void CScanner::SelfTest(void) {
#ifdef _DEBUG
#endif
}



