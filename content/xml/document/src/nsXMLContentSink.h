/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef nsXMLContentSink_h__
#define nsXMLContentSink_h__

#include "nsIXMLContentSink.h"
#include "nsIViewManager.h"
#include "nsIScrollableView.h"
#include "nsWeakReference.h"
#include "nsIUnicharInputStream.h"
#include "nsIStreamLoader.h"
#include "nsCOMArray.h"
#include "nsINodeInfo.h"
#include "nsIDOMHTMLTextAreaElement.h"
#include "nsICSSLoaderObserver.h"
#include "nsIHTMLContent.h"
#include "nsIDOMHTMLScriptElement.h"
#include "nsIScriptLoader.h"
#include "nsIScriptLoaderObserver.h"
#include "nsSupportsArray.h"
#include "nsIExpatSink.h"
#include "nsIDocumentTransformer.h"
#include "nsCOMPtr.h"
#include "nsIDocShell.h"

class nsICSSStyleSheet;
class nsIDocument;
class nsIURI;
class nsIContent;
class nsAutoVoidArray;
class nsIUnicharInputStream;
class nsIParser;
class nsINameSpace;
class nsICSSLoader;
class nsIElementFactory;

typedef enum {
  eXMLContentSinkState_InProlog,
  eXMLContentSinkState_InDocumentElement,
  eXMLContentSinkState_InEpilog
} XMLContentSinkState;

// XXX Till the parser knows a little bit more about XML, 
// this is a HTMLContentSink.
class nsXMLContentSink : public nsIXMLContentSink,
                         public nsITransformObserver,
                         public nsSupportsWeakReference,
                         public nsIScriptLoaderObserver,
                         public nsICSSLoaderObserver,
                         public nsIExpatSink
{
public:
  nsXMLContentSink();
  virtual ~nsXMLContentSink();

  nsresult Init(nsIDocument* aDoc,
                nsIURI* aURL,
                nsISupports* aContainer,
                nsIChannel* aChannel);

  // nsISupports
  NS_DECL_ISUPPORTS
  NS_DECL_NSISCRIPTLOADEROBSERVER
  NS_DECL_NSIEXPATSINK

  // nsIContentSink
  NS_IMETHOD WillBuildModel(void);
  NS_IMETHOD DidBuildModel(PRInt32 aQualityLevel);
  NS_IMETHOD WillInterrupt(void);
  NS_IMETHOD WillResume(void);
  NS_IMETHOD SetParser(nsIParser* aParser);  
  NS_IMETHOD FlushPendingNotifications() { return NS_OK; }
  NS_IMETHOD SetDocumentCharset(nsACString& aCharset);

  // nsICSSLoaderObserver
  NS_IMETHOD StyleSheetLoaded(nsICSSStyleSheet*aSheet, PRBool aNotify);

  // nsITransformObserver
  NS_IMETHOD OnDocumentCreated(nsIDOMDocument *aResultDocument);
  NS_IMETHOD OnTransformDone(nsresult aResult, nsIDOMDocument *aResultDocument);

protected:
  void StartLayout();

  nsresult PushNameSpacesFrom(const PRUnichar** aAttributes);
  nsresult AddAttributes(const PRUnichar** aNode, nsIContent* aContent);
  nsresult AddText(const PRUnichar* aString, PRInt32 aLength);
  nsresult ProcessEndSCRIPTTag(nsIContent* aContent);

  virtual PRBool OnOpenContainer(const PRUnichar **aAtts, 
                                 PRUint32 aAttsCount, 
                                 PRInt32 aNameSpaceID, 
                                 nsIAtom* aTagName,
                                 PRUint32 aLineNumber) { return PR_TRUE; }
  virtual nsresult CreateElement(const PRUnichar** aAtts, PRUint32 aAttsCount,
                                 nsINodeInfo* aNodeInfo, PRUint32 aLineNumber,
                                 nsIContent** aResult, PRBool* aAppendContent);

  virtual nsresult CloseElement(nsIContent* aContent, PRBool* aAppendContent);

  virtual nsresult FlushText(PRBool aCreateTextNode=PR_TRUE,
                             PRBool* aDidFlush=nsnull);

  nsresult AddContentAsLeaf(nsIContent *aContent);

  static void SplitXMLName(const nsAString& aString, nsIAtom **aPrefix,
                           nsIAtom **aTagName);
  PRInt32 GetNameSpaceId(nsIAtom* aPrefix);
  already_AddRefed<nsINameSpace> PopNameSpaces();

  nsIContent* GetCurrentContent();
  PRInt32 PushContent(nsIContent *aContent);
  already_AddRefed<nsIContent> PopContent();


  nsresult ProcessBASETag(nsIContent* aContent);
  nsresult ProcessMETATag(nsIContent* aContent);
  nsresult ProcessHeaderData(nsIAtom* aHeader, const nsAString& aValue,
                             nsIContent* aContent);
  nsresult ProcessHTTPHeaders(nsIChannel* aChannel);
  nsresult ProcessLink(nsIContent* aElement, const nsAString& aLinkData);

  nsresult RefreshIfEnabled(nsIViewManager* vm);
  
  NS_IMETHOD ProcessStyleLink(nsIContent* aElement,
                              const nsString& aHref,
                              PRBool aAlternate,
                              const nsString& aTitle,
                              const nsString& aType,
                              const nsString& aMedia);

  nsresult LoadXSLStyleSheet(nsIURI* aUrl);

  static void
  GetElementFactory(PRInt32 aNameSpaceID, nsIElementFactory** aResult);

  void ScrollToRef(PRBool aReallyScroll);
  
  nsresult MaybePrettyPrint();
  
  nsIDocument*     mDocument;
  nsIURI*          mDocumentURL;
  nsIURI*          mDocumentBaseURL; // can be set via HTTP headers
  nsCOMPtr<nsIDocShell> mDocShell;
  nsIParser*       mParser;
  nsIContent*      mDocElement;
  PRUnichar*       mText;
  nsICSSLoader*    mCSSLoader;  

  nsCOMArray<nsIDOMHTMLScriptElement> mScriptElements;
  XMLContentSinkState mState;

  nsCString mRef; // ScrollTo #ref
  nsString mTitleText; 
  
  PRInt32 mTextLength;
  PRInt32 mTextSize;
  PRUint32 mScriptLineNo;
  
  PRPackedBool mConstrainSize;
  PRPackedBool mInTitle;
  PRPackedBool mNeedToBlockParser;
  PRPackedBool mPrettyPrintXML;
  PRPackedBool mPrettyPrintHasSpecialRoot;
  PRPackedBool mPrettyPrintHasFactoredElements;
  PRPackedBool mHasProcessedBase;

  nsCOMArray<nsIContent>           mContentStack;
  nsCOMArray<nsINameSpace>         mNameSpaceStack;
  nsCOMPtr<nsINodeInfoManager>     mNodeInfoManager;
  nsCOMPtr<nsIDocumentTransformer> mXSLTProcessor;
};

#endif // nsXMLContentSink_h__
