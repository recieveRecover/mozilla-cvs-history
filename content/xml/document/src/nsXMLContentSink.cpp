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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */
#include "nsCOMPtr.h"
#include "nsXMLContentSink.h"
#include "nsIElementFactory.h"
#include "nsIParser.h"
#include "nsIUnicharInputStream.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMDocumentType.h"
#include "nsIDOMDOMImplementation.h"
#include "nsIXMLDocument.h"
#include "nsIXMLContent.h"
#include "nsIScriptGlobalObject.h"
#include "nsIURL.h"
#include "nsIRefreshURI.h"
#include "nsNetUtil.h"
#include "nsIWebShell.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIContent.h"
#include "nsITextContent.h"
#include "nsIPresContext.h"
#include "nsIPresShell.h"
#include "nsIViewManager.h"
#include "nsIDOMComment.h"
#include "nsIDOMCDATASection.h"
#include "nsDOMDocumentType.h"
#include "nsIHTMLContent.h"
#include "nsHTMLEntities.h"
#include "nsHTMLParts.h"
#include "nsVoidArray.h"
#include "nsCRT.h"
#include "nsICSSLoader.h"
#include "nsICSSStyleSheet.h"
#include "nsIHTMLContentContainer.h"
#include "nsHTMLAtoms.h"
#include "nsContentUtils.h"
#include "nsLayoutAtoms.h"
#include "nsContentCID.h"
#include "nsIScriptContext.h"
#include "nsINameSpace.h"
#include "nsINameSpaceManager.h"
#include "nsIServiceManager.h"
#include "nsIScriptSecurityManager.h"
#include "nsIContentViewer.h"
#include "jsapi.h" // for JSVERSION_* and JS_VersionToString
#include "prtime.h"
#include "prlog.h"
#include "prmem.h"
#include "nsXSLContentSink.h"
#include "nsParserCIID.h"
#include "nsParserUtils.h"
#include "nsIDocumentViewer.h"
#include "nsIScrollable.h"
#include "nsIWebNavigation.h"

// XXX misnamed header file, but oh well
#include "nsHTMLTokens.h"

static char kNameSpaceSeparator = ':';
static char kNameSpaceDef[] = "xmlns";
static char kStyleSheetPI[] = "xml-stylesheet";
static char kXSLType[] = "text/xsl";

static NS_DEFINE_CID(kNameSpaceManagerCID, NS_NAMESPACEMANAGER_CID);
static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);

nsINameSpaceManager* nsXMLContentSink::gNameSpaceManager = nsnull;
PRUint32 nsXMLContentSink::gRefCnt = 0;

// XXX Open Issues:
// 1) what's not allowed - We need to figure out which HTML tags
//    (prefixed with a HTML namespace qualifier) are explicitly not
//    allowed (if any).
// 2) factoring code with nsHTMLContentSink - There's some amount of
//    common code between this and the HTML content sink. This will
//    increase as we support more and more HTML elements. How can code
//    from the code be factored?

nsresult
NS_NewXMLContentSink(nsIXMLContentSink** aResult,
                     nsIDocument* aDoc,
                     nsIURI* aURL,
                     nsIWebShell* aWebShell)
{
  NS_PRECONDITION(nsnull != aResult, "null ptr");
  if (nsnull == aResult) {
    return NS_ERROR_NULL_POINTER;
  }
  nsXMLContentSink* it;
  NS_NEWXPCOM(it, nsXMLContentSink);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  nsresult rv = it->Init(aDoc, aURL, aWebShell);
  if (NS_OK != rv) {
    delete it;
    return rv;
  }
  return it->QueryInterface(NS_GET_IID(nsIXMLContentSink), (void **)aResult);
}

nsXMLContentSink::nsXMLContentSink()
{
  NS_INIT_REFCNT();
  gRefCnt++;
  if (gRefCnt == 1) {
     nsresult rv = nsServiceManager::GetService(kNameSpaceManagerCID,
                                          NS_GET_IID(nsINameSpaceManager),
                                          (nsISupports**) &gNameSpaceManager);
     NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get namespace manager");
  }

  mDocument = nsnull;
  mDocumentURL = nsnull;
  mDocumentBaseURL = nsnull;
  mWebShell = nsnull;
  mParser = nsnull;
  mRootElement = nsnull;
  mDocElement = nsnull;
  mContentStack = nsnull;
  mNameSpaceStack = nsnull;
  mText = nsnull;
  mTextLength = 0;
  mTextSize = 0;
  mConstrainSize = PR_TRUE;
  mInScript = PR_FALSE;
  mInTitle = PR_FALSE;
  mStyleSheetCount = 0;
  mCSSLoader       = nsnull;
  mXSLTransformMediator = nsnull;
}

nsXMLContentSink::~nsXMLContentSink()
{
  gRefCnt--;
  if (gRefCnt == 0) {
    NS_IF_RELEASE(gNameSpaceManager);
  }

  NS_IF_RELEASE(mDocument);
  NS_IF_RELEASE(mDocumentURL);
  NS_IF_RELEASE(mDocumentBaseURL);
  NS_IF_RELEASE(mWebShell);
  NS_IF_RELEASE(mParser);
  NS_IF_RELEASE(mRootElement);
  NS_IF_RELEASE(mDocElement);
  if (nsnull != mNameSpaceStack) {
    // There shouldn't be any here except in an error condition
    PRInt32 index = mNameSpaceStack->Count();
    while (0 < index--) {
      nsINameSpace* nameSpace = (nsINameSpace*)mNameSpaceStack->ElementAt(index);
      NS_RELEASE(nameSpace);
    }
    delete mNameSpaceStack;
  }
  if (nsnull != mText) {
    PR_FREEIF(mText);
  }
  NS_IF_RELEASE(mCSSLoader);
}

nsresult
nsXMLContentSink::Init(nsIDocument* aDoc,
                       nsIURI* aURL,
                       nsIWebShell* aContainer)
{
  NS_PRECONDITION(nsnull != aDoc, "null ptr");
  NS_PRECONDITION(nsnull != aURL, "null ptr");
  if ((nsnull == aDoc) || (nsnull == aURL)) {
    return NS_ERROR_NULL_POINTER;
  }

  mDocument = aDoc;
  NS_ADDREF(aDoc);
  mDocumentURL = aURL;
  NS_ADDREF(aURL);
  mDocumentBaseURL = aURL;
  NS_ADDREF(aURL);
  mWebShell = aContainer;
  NS_IF_ADDREF(aContainer);

  mState = eXMLContentSinkState_InProlog;
  mDocElement = nsnull;
  mRootElement = nsnull;

  // XXX this presumes HTTP header info is alread set in document
  // XXX if it isn't we need to set it here...
  mDocument->GetHeaderData(nsHTMLAtoms::headerDefaultStyle, mPreferredStyle);

  nsIHTMLContentContainer* htmlContainer = nsnull;
  if (NS_SUCCEEDED(aDoc->QueryInterface(NS_GET_IID(nsIHTMLContentContainer), (void**)&htmlContainer))) {
    htmlContainer->GetCSSLoader(mCSSLoader);
    NS_RELEASE(htmlContainer);
  }

  return aDoc->GetNodeInfoManager(*getter_AddRefs(mNodeInfoManager));
}

NS_IMPL_THREADSAFE_ADDREF(nsXMLContentSink)
NS_IMPL_THREADSAFE_RELEASE(nsXMLContentSink)

NS_INTERFACE_MAP_BEGIN(nsXMLContentSink)
	NS_INTERFACE_MAP_ENTRY(nsIXMLContentSink)
	NS_INTERFACE_MAP_ENTRY(nsIContentSink)
	NS_INTERFACE_MAP_ENTRY(nsIObserver)
	NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
	NS_INTERFACE_MAP_ENTRY(nsIStreamLoaderObserver)
	NS_INTERFACE_MAP_ENTRY(nsICSSLoaderObserver)
	NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIXMLContentSink)
NS_INTERFACE_MAP_END

  // nsIContentSink
NS_IMETHODIMP
nsXMLContentSink::WillBuildModel(void)
{
  // Notify document that the load is beginning
  mDocument->BeginLoad();
  return NS_OK;
}

void
nsXMLContentSink::ScrollToRef()
{
  if (mRef.Length() > 0)
  {
    PRInt32 i, ns = mDocument->GetNumberOfShells();
    for (i = 0; i < ns; i++) {
      nsCOMPtr<nsIPresShell> shell(dont_AddRef(mDocument->GetShellAt(i)));
      if (shell) {
        shell->FlushPendingNotifications();
        // Scroll to the anchor
        shell->GoToAnchor(mRef);
      }
    }
  }
}

NS_IMETHODIMP
nsXMLContentSink::DidBuildModel(PRInt32 aQualityLevel)
{
  // XXX this is silly; who cares?
  PRInt32 i, ns = mDocument->GetNumberOfShells();
  for (i = 0; i < ns; i++) {
    nsCOMPtr<nsIPresShell> shell( dont_AddRef(mDocument->GetShellAt(i)) );
    if (shell) {
      nsCOMPtr<nsIViewManager> vm;
      shell->GetViewManager(getter_AddRefs(vm));
      if(vm) {
        vm->SetQuality(nsContentQuality(aQualityLevel));
      }
    }
  }

  mDocument->SetRootContent(mDocElement);

  nsresult rv = NS_OK;
  if (mXSLTransformMediator) {
    rv = SetupTransformMediator();
  }

  if (!mXSLTransformMediator || NS_FAILED(rv)) {
    StartLayout();
    ScrollToRef();
    mDocument->EndLoad();
  }

  // Drop our reference to the parser to get rid of a circular
  // reference.
  NS_IF_RELEASE(mParser);

  return NS_OK;
}

// The observe method is called on completion of the transform.  The nsISupports argument is an
// nsIDOMElement interface to the root node of the output content model.
NS_IMETHODIMP
nsXMLContentSink::Observe(nsISupports *aSubject, const PRUnichar *aTopic, const PRUnichar *someData)
{
  nsresult rv = NS_OK;
  nsAutoString topic(aTopic);

  if (topic.Equals(NS_LITERAL_STRING("xslt-done"))) {
    nsCOMPtr<nsIContent> content;

    // Set the output content model on the document
    content = do_QueryInterface(aSubject, &rv);
    if (NS_SUCCEEDED(rv)) {
      nsCOMPtr<nsIDOMDocument> resultDOMDoc;
      mXSLTransformMediator->GetResultDocument(getter_AddRefs(resultDOMDoc));
      nsCOMPtr<nsIDocument> resultDoc = do_QueryInterface(resultDOMDoc);

      nsCOMPtr<nsIDocument> sourceDoc = mDocument;
      NS_RELEASE(mDocument);

      mDocument = resultDoc;
      NS_ADDREF(mDocument);
      mDocument->SetRootContent(content);

      // Reset the observer on the transform mediator
      mXSLTransformMediator->SetTransformObserver(nsnull);

      // Start the layout process
      StartLayout();
      sourceDoc->EndLoad();

      nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(mWebShell));
      nsCOMPtr<nsIContentViewer> contentViewer;
      rv = docShell->GetContentViewer(getter_AddRefs(contentViewer));
      if (NS_SUCCEEDED(rv) && contentViewer) {
        contentViewer->LoadComplete(NS_OK);
      }
    }
    else
    {
      // Transform failed
      nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(mWebShell));
      nsCOMPtr<nsIContentViewer> contentViewer;
      rv = docShell->GetContentViewer(getter_AddRefs(contentViewer));
      nsCOMPtr<nsIDocumentViewer> documentViewer(do_QueryInterface(contentViewer));
      if (documentViewer) {
        documentViewer->SetTransformMediator(nsnull);
      }

      mXSLTransformMediator = nsnull;
      mDocument->SetRootContent(mDocElement);

      // Start the layout process
      StartLayout();
      mDocument->EndLoad();
    }
  }
  return rv;
}


// Provide the transform mediator with the source document's content
// model and the output document, and register the XML content sink
// as the transform observer.  The transform mediator will call
// the nsIObserver::Observe() method on the transform observer once
// the transform is completed.  The nsISupports pointer to the Observe
// method will be an nsIDOMElement pointer to the root node of the output
// content model.
nsresult
nsXMLContentSink::SetupTransformMediator()
{
  nsresult rv = NS_OK;

  nsCOMPtr<nsIDOMDocument> currentDOMDoc(do_QueryInterface(mDocument));
  mXSLTransformMediator->SetSourceContentModel(currentDOMDoc);

  // Create the result document
  nsCOMPtr<nsIDOMDocument> resultDOMDoc;

  nsCOMPtr<nsIURI> url;
  mDocument->GetBaseURL(*getter_AddRefs(url));

  nsAutoString emptyStr;
  rv = NS_NewDOMDocument(getter_AddRefs(resultDOMDoc), emptyStr, emptyStr, nsnull, url);
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIXMLDocument> resultXMLDoc(do_QueryInterface(resultDOMDoc));
  resultXMLDoc->SetDefaultStylesheets(url);

  nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(mWebShell));
  nsCOMPtr<nsIContentViewer> contentViewer;
  rv = docShell->GetContentViewer(getter_AddRefs(contentViewer));
  if (NS_SUCCEEDED(rv) && contentViewer) {
    contentViewer->SetDOMDocument(resultDOMDoc);
  }

  mXSLTransformMediator->SetResultDocument(resultDOMDoc);
  mXSLTransformMediator->SetTransformObserver(this);

  return rv;
}

NS_IMETHODIMP
nsXMLContentSink::WillInterrupt(void)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXMLContentSink::WillResume(void)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXMLContentSink::SetParser(nsIParser* aParser)
{
  NS_IF_RELEASE(mParser);
  mParser = aParser;
  NS_IF_ADDREF(mParser);

  return NS_OK;
}

// XXX Code copied from nsHTMLContentSink. It should be shared.
nsresult
nsXMLContentSink::AddAttributes(const nsIParserNode& aNode,
                                nsIContent* aContent,
                                PRBool aIsHTML)
{
  // Add tag attributes to the content attributes
  nsAutoString name;
  PRInt32 ac = aNode.GetAttributeCount();
  for (PRInt32 i = 0; i < ac; i++) {
    // Get upper-cased key
    const nsAReadableString& key = aNode.GetKeyAt(i);
    name.Assign(key);

    nsCOMPtr<nsIAtom> nameSpacePrefix(dont_AddRef(CutNameSpacePrefix(name)));
    nsCOMPtr<nsIAtom> nameAtom(dont_AddRef(NS_NewAtom(name)));
    PRInt32 nameSpaceID;

    if (nameSpacePrefix) {
        nameSpaceID = GetNameSpaceId(nameSpacePrefix);
    } else {
      if (nameAtom.get() == nsLayoutAtoms::xmlnsNameSpace)
        nameSpaceID = kNameSpaceID_XMLNS;
      else
        nameSpaceID = kNameSpaceID_None;
    }

    if (kNameSpaceID_Unknown == nameSpaceID) {
      nameSpaceID = kNameSpaceID_None;
      nameAtom = dont_AddRef(NS_NewAtom(key));
      nameSpacePrefix = nsnull;
    } else if ((kNameSpaceID_XMLNS == nameSpaceID) && aIsHTML) {
      name.InsertWithConversion("xmlns:", 0);
      nameAtom = dont_AddRef(NS_NewAtom(name));
      nameSpaceID = kNameSpaceID_HTML;  // XXX this is wrong, but necessary until HTML can store other namespaces for attrs
    }

    nsCOMPtr<nsINodeInfo> ni;
    mNodeInfoManager->GetNodeInfo(nameAtom, nameSpacePrefix, nameSpaceID,
                                  *getter_AddRefs(ni));
    NS_ENSURE_TRUE(ni, NS_ERROR_FAILURE);

    // Add attribute to content
    aContent->SetAttribute(ni, aNode.GetValueAt(i), PR_FALSE);
  }

  // Give autoloading links a chance to fire
  if (mWebShell) {
    nsCOMPtr<nsIXMLContent> xmlcontent(do_QueryInterface(aContent));
    if (xmlcontent) {
      nsresult rv = xmlcontent->MaybeTriggerAutoLink(mWebShell);
      if (rv == NS_XML_AUTOLINK_REPLACE ||
          rv == NS_XML_AUTOLINK_UNDEFINED) {
        // If we do not terminate the parse, we just keep generating link trigger
        // events. We want to parse only up to the first replace link, and stop.
        mParser->Terminate();
      }
    }
  }

  return NS_OK;
}

void
nsXMLContentSink::PushNameSpacesFrom(const nsIParserNode& aNode)
{
  nsAutoString k, prefix;
  PRInt32 ac = aNode.GetAttributeCount();
  PRInt32 offset;
  nsINameSpace* nameSpace = nsnull;

  if ((nsnull != mNameSpaceStack) && (0 < mNameSpaceStack->Count())) {
    nameSpace = (nsINameSpace*)mNameSpaceStack->ElementAt(mNameSpaceStack->Count() - 1);
    NS_ADDREF(nameSpace);
  }
  else {
    nsINameSpaceManager* manager = nsnull;
    mDocument->GetNameSpaceManager(manager);
    NS_ASSERTION(nsnull != manager, "no name space manager in document");
    if (nsnull != manager) {
      manager->CreateRootNameSpace(nameSpace);
      NS_RELEASE(manager);
    }
  }

  if (nsnull != nameSpace) {
    for (PRInt32 i = 0; i < ac; i++) {
      const nsAReadableString& key = aNode.GetKeyAt(i);
      k.Assign(key);
      // Look for "xmlns" at the start of the attribute name
      offset = k.Find(kNameSpaceDef);
      if (0 == offset) {
        if (k.Length() == (sizeof(kNameSpaceDef)-1)) {
          // If there's nothing left, this is a default namespace
          prefix.Truncate();
        }
        else {
          PRUnichar next = k.CharAt(sizeof(kNameSpaceDef)-1);
          // If the next character is a :, there is a namespace prefix
          if (':' == next) {
            prefix.Truncate();
            k.Right(prefix, k.Length()-sizeof(kNameSpaceDef));
          }
          else {
            continue;
          }
        }

        // Open a local namespace
        nsIAtom* prefixAtom = ((0 < prefix.Length()) ? NS_NewAtom(prefix) : nsnull);
        nsINameSpace* child = nsnull;
        nameSpace->CreateChildNameSpace(prefixAtom, aNode.GetValueAt(i), child);
        if (nsnull != child) {
          NS_RELEASE(nameSpace);
          nameSpace = child;
        }
        NS_IF_RELEASE(prefixAtom);
      }
    }
    if (nsnull == mNameSpaceStack) {
      mNameSpaceStack = new nsVoidArray();
    }
    mNameSpaceStack->AppendElement(nameSpace);
  }
}

nsIAtom*  nsXMLContentSink::CutNameSpacePrefix(nsString& aString)
{
  nsAutoString  prefix;
  PRInt32 nsoffset = aString.FindChar(kNameSpaceSeparator);
  if (-1 != nsoffset) {
    aString.Left(prefix, nsoffset);
    aString.Cut(0, nsoffset+1);
  }
  if (0 < prefix.Length()) {
    return NS_NewAtom(prefix);
  }
  return nsnull;
}

NS_IMETHODIMP
nsXMLContentSink::OpenContainer(const nsIParserNode& aNode)
{
  nsresult result = NS_OK;
  nsAutoString tag;
  nsCOMPtr<nsIAtom> nameSpacePrefix;
  PRBool isHTML = PR_FALSE;
  PRBool pushContent = PR_TRUE;
  nsCOMPtr<nsIContent> content;

  // XXX Hopefully the parser will flag this before we get
  // here. If we're in the epilog, there should be no
  // new elements
  PR_ASSERT(eXMLContentSinkState_InEpilog != mState);

  FlushText();

  mState = eXMLContentSinkState_InDocumentElement;

  tag.Assign(aNode.GetText());
  nameSpacePrefix = getter_AddRefs(CutNameSpacePrefix(tag));
  nsCOMPtr<nsIAtom> tagAtom(dont_AddRef(NS_NewAtom(tag)));

  // We must register namespace declarations found in the attribute list
  // of an element before creating the element. This is because the
  // namespace prefix for an element might be declared within the attribute
  // list.
  PushNameSpacesFrom(aNode);

  PRInt32 nameSpaceID = GetNameSpaceId(nameSpacePrefix);

  nsCOMPtr<nsINodeInfo> nodeInfo;

  mNodeInfoManager->GetNodeInfo(tagAtom, nameSpacePrefix, nameSpaceID,
                                *getter_AddRefs(nodeInfo));

  isHTML = IsHTMLNameSpace(nameSpaceID);

  if (isHTML) {
    if (tagAtom.get() == nsHTMLAtoms::script) {
      result = ProcessStartSCRIPTTag(aNode);
    } else if (tagAtom.get() == nsHTMLAtoms::title) {
      if (mTitleText.IsEmpty())
        mInTitle = PR_TRUE; // The first title wins
    }

    nsCOMPtr<nsIHTMLContent> htmlContent;
    result = NS_CreateHTMLElement(getter_AddRefs(htmlContent), nodeInfo);
    content = do_QueryInterface(htmlContent);

    if (tagAtom.get() == nsHTMLAtoms::textarea) {
      mTextAreaElement = do_QueryInterface(htmlContent);
    } else if (tagAtom.get() == nsHTMLAtoms::style) {
      mStyleElement = htmlContent;
    } else if (tagAtom.get() == nsHTMLAtoms::base) {
      if (!mBaseElement) {
        mBaseElement = htmlContent; // The first base wins
      }
    } else if (tagAtom.get() == nsHTMLAtoms::meta) {
      if (!mMetaElement) {
        mMetaElement = htmlContent;
      }
    } else if (tagAtom.get() == nsHTMLAtoms::link) {
      if (!mLinkElement) {
        mLinkElement = htmlContent;
      }
    }
  }
  else {
    // The first step here is to see if someone has provided their
    // own content element implementation (e.g., XUL or MathML).  
    // This is done based off a contractid/namespace scheme.
    nsCOMPtr<nsIElementFactory> elementFactory;

    // This should *not* be done for every node, only when we find
    // a new namespace!!! -- jst
    GetElementFactory(nameSpaceID, getter_AddRefs(elementFactory));
    if (elementFactory) {
      // Create the content element using the element factory.
      elementFactory->CreateInstanceByTag(nodeInfo, getter_AddRefs(content));
    }
    else {
      nsCOMPtr<nsIXMLContent> xmlContent;
      result = NS_NewXMLElement(getter_AddRefs(xmlContent), nodeInfo);

      content = do_QueryInterface(xmlContent);
    }
  }
  
  if (NS_OK == result) {
    PRInt32 id;
    mDocument->GetAndIncrementContentID(&id);
    content->SetContentID(id);
    content->SetDocument(mDocument, PR_FALSE, PR_TRUE);

    // Set the attributes on the new content element
    result = AddAttributes(aNode, content, isHTML);
    if (NS_OK == result) {
      // If this is the document element
      if (nsnull == mDocElement) {
        mDocElement = content;
        NS_ADDREF(mDocElement);

        // For XSL, we need to wait till after the transform
        // to set the root content object.  Hence, the following
        // ifndef.
        if (!mXSLTransformMediator)
            mDocument->SetRootContent(mDocElement);
      }
      else {
        nsCOMPtr<nsIContent> parent = getter_AddRefs(GetCurrentContent());

        parent->AppendChildTo(content, PR_FALSE, PR_FALSE);
      }
      if (pushContent) {
        PushContent(content);
      }
    }

    // Set the ID attribute atom on the node info object for this node
    nsCOMPtr<nsIAtom> IDAttr;
    result = aNode.GetIDAttributeAtom(getter_AddRefs(IDAttr));
    if (IDAttr && NS_SUCCEEDED(result))
      result = nodeInfo->SetIDAttributeAtom(IDAttr);
  }

  return result;
}

NS_IMETHODIMP
nsXMLContentSink::CloseContainer(const nsIParserNode& aNode)
{
  nsresult result = NS_OK;
  nsAutoString tag;
  nsCOMPtr<nsIAtom> nameSpacePrefix;
  PRBool isHTML = PR_FALSE;
  PRBool popContent = PR_TRUE;

  // XXX Hopefully the parser will flag this before we get
  // here. If we're in the prolog or epilog, there should be
  // no close tags for elements.
  PR_ASSERT(eXMLContentSinkState_InDocumentElement == mState);

  tag.Assign(aNode.GetText());

  nameSpacePrefix = getter_AddRefs(CutNameSpacePrefix(tag));
  PRInt32 nameSpaceID = GetNameSpaceId(nameSpacePrefix);
  isHTML = IsHTMLNameSpace(nameSpaceID);

  FlushText();

  if (isHTML) {
    nsCOMPtr<nsIAtom> tagAtom(dont_AddRef(NS_NewAtom(tag)));

    if (tagAtom.get() == nsHTMLAtoms::script) {
      result = ProcessEndSCRIPTTag(aNode);
    } else if (tagAtom.get() == nsHTMLAtoms::title) {
      if (mInTitle) { // The first title wins
        nsCOMPtr<nsIXMLDocument> xmlDoc(do_QueryInterface(mDocument));
        if (xmlDoc) {
          mTitleText.CompressWhitespace();
          xmlDoc->SetTitle(mTitleText.GetUnicode());
        }        
        mInTitle = PR_FALSE;
      }
    } else if (tagAtom.get() == nsHTMLAtoms::textarea) {
      if (mTextAreaElement) {
        mTextAreaElement->SetDefaultValue(mTextareaText);
        mTextAreaElement = nsnull;
        mTextareaText.Truncate();
      }
    } else if (tagAtom.get() == nsHTMLAtoms::style) {
      if (mStyleElement) {
        result = ProcessSTYLETag(aNode);
        mStyleElement=nsnull;
        mStyleText.Truncate();
      }
    } else if (tagAtom.get() == nsHTMLAtoms::base) {
      if (mBaseElement) {
        result = ProcessBASETag();
      }
    } else if (tagAtom.get() == nsHTMLAtoms::meta) {
      if (mMetaElement) {
        result = ProcessMETATag();
        mMetaElement = nsnull;  // HTML can have more than one meta so clear this now
      }
    } else if (tagAtom.get() == nsHTMLAtoms::link) {
      if (mLinkElement) {
        result = ProcessLINKTag();
        mLinkElement = nsnull;  // HTML can have more than one link so clear this now
      }
    }
  }

  nsCOMPtr<nsIContent> content;
  if (popContent) {
    content = getter_AddRefs(PopContent());
    if (content) {
      if (mDocElement == content.get()) {
        mState = eXMLContentSinkState_InEpilog;
      }
    }
    else {
      // XXX Again, the parser should catch unmatched tags and
      // we should never get here.
      PR_ASSERT(0);
    }
  }
  nsINameSpace* nameSpace = PopNameSpaces();
  if (content) {
    nsCOMPtr<nsIXMLContent> xmlContent = do_QueryInterface(content);
    if (xmlContent) {
      xmlContent->SetContainingNameSpace(nameSpace);
    }
  }
  NS_IF_RELEASE(nameSpace);

  return result;
}

NS_IMETHODIMP
nsXMLContentSink::AddLeaf(const nsIParserNode& aNode)
{
  // XXX For now, all leaf content is character data
  // XXX make sure to push/pop name spaces here too (for attributes)
  switch (aNode.GetTokenType()) {
    case eToken_text:
    case eToken_whitespace:
    case eToken_newline:
      AddText(aNode.GetText());
      break;

    case eToken_cdatasection:
      AddCDATASection(aNode);    
      break;

    case eToken_entity:
    {
      nsAutoString tmp;
      PRInt32 unicode = aNode.TranslateToUnicodeStr(tmp);
      if (unicode < 0) {
        return AddText(aNode.GetText());
      }
      return AddText(tmp);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsXMLContentSink::NotifyError(const nsParserError* aError)
{
  return NS_OK;
}

// nsIXMLContentSink
NS_IMETHODIMP
nsXMLContentSink::AddXMLDecl(const nsIParserNode& aNode)
{
  // XXX We'll ignore it for now
  printf("nsXMLContentSink::AddXMLDecl\n");
  return NS_OK;
}

nsresult
nsXMLContentSink::AddContentAsLeaf(nsIContent *aContent)
{
  nsresult result = NS_OK;

  if ((eXMLContentSinkState_InProlog == mState) ||
      (eXMLContentSinkState_InEpilog == mState)) {
    nsCOMPtr<nsIDOMDocument> domDoc( do_QueryInterface(mDocument) );
    nsCOMPtr<nsIDOMNode> trash;
    nsCOMPtr<nsIDOMNode> child( do_QueryInterface(aContent) );
    NS_ASSERTION(child, "not a dom node");
    domDoc->AppendChild(child, getter_AddRefs(trash));
  }
  else {
    nsCOMPtr<nsIContent> parent = getter_AddRefs(GetCurrentContent());

    if (parent) {
      result = parent->AppendChildTo(aContent, PR_FALSE, PR_FALSE);
    }
  }

  return result;
}

NS_IMETHODIMP
nsXMLContentSink::AddComment(const nsIParserNode& aNode)
{
  FlushText();

  nsIContent *comment;
  nsIDOMComment *domComment;
  nsresult result = NS_OK;

  result = NS_NewCommentNode(&comment);
  if (NS_OK == result) {
    result = comment->QueryInterface(NS_GET_IID(nsIDOMComment), (void **)&domComment);
    if (NS_OK == result) {
      domComment->AppendData(aNode.GetText());
      NS_RELEASE(domComment);

      comment->SetDocument(mDocument, PR_FALSE, PR_TRUE);
      result = AddContentAsLeaf(comment);
    }
    NS_RELEASE(comment);
  }

  return result;
}

NS_IMETHODIMP
nsXMLContentSink::AddCDATASection(const nsIParserNode& aNode)
{
  FlushText();

  nsIContent *cdata;
  nsIDOMCDATASection *domCDATA;
  nsresult result = NS_OK;

  const nsAReadableString& text = aNode.GetText();
  if (mInScript) {
    mScriptText.Append(text);
  } else if (mInTitle) {
    mTitleText.Append(text);
  } else if (mTextAreaElement) {
    mTextareaText.Append(text);
  } else if (mStyleElement) {
    mStyleText.Append(text);
  }
  result = NS_NewXMLCDATASection(&cdata);
  if (NS_OK == result) {
    result = cdata->QueryInterface(NS_GET_IID(nsIDOMCDATASection), (void **)&domCDATA);
    if (NS_OK == result) {
      domCDATA->AppendData(text);
      NS_RELEASE(domCDATA);

      cdata->SetDocument(mDocument, PR_FALSE, PR_TRUE);
      result = AddContentAsLeaf(cdata);
    }
    NS_RELEASE(cdata);
  }

  return result;
}

static void
ParseProcessingInstruction(const nsString& aText,
                           nsString& aTarget,
                           nsString& aData)
{
  PRInt32 offset;

  aTarget.Truncate();
  aData.Truncate();

  offset = aText.FindCharInSet(" \n\r\t");
  if (-1 != offset) {
    aText.Mid(aTarget, 2, offset-2);
    aText.Mid(aData, offset+1, aText.Length()-offset-3);
  }
}

static void SplitMimeType(const nsString& aValue, nsString& aType, nsString& aParams)
{
  aType.Truncate();
  aParams.Truncate();
  PRInt32 semiIndex = aValue.FindChar(PRUnichar(';'));
  if (-1 != semiIndex) {
    aValue.Left(aType, semiIndex);
    aValue.Right(aParams, (aValue.Length() - semiIndex) - 1);
    aParams.StripWhitespace();
  }
  else {
    aType = aValue;
  }
  aType.StripWhitespace();
}

nsresult
nsXMLContentSink::CreateStyleSheetURL(nsIURI** aUrl,
                                      const nsAReadableString& aHref)
{
  nsresult result = NS_OK;
  result = NS_NewURI(aUrl, aHref, mDocumentBaseURL);
  return result;

}

// Create an XML parser and an XSL content sink and start parsing
// the XSL stylesheet located at the given URL.
nsresult
nsXMLContentSink::LoadXSLStyleSheet(nsIURI* aUrl, const nsString& aType)
{
  nsresult rv = NS_OK;

  // Create a transform mediator
  rv = NS_NewTransformMediator(getter_AddRefs(mXSLTransformMediator), aType);
  if (NS_FAILED(rv)) {
    // No XSLT processor available, continue normal document loading
    return NS_OK;
  }

  static NS_DEFINE_CID(kCParserCID, NS_PARSER_CID);

  // Create the XML parser
  nsCOMPtr<nsIParser> parser(do_CreateInstance(kCParserCID, &rv));
  if (NS_FAILED(rv)) return rv;

  // Enable the transform mediator. It will start the transform
  // as soon as it has enough state to do so.  The state needed is
  // the source content model, the style content model, the current
  // document, and an observer.  The XML and XSL content sinks provide
  // this state by calling the various setters on nsITransformMediator.
  mXSLTransformMediator->SetEnabled(PR_TRUE);

  // The document viewer owns the transform mediator.
  nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(mWebShell));
  nsCOMPtr<nsIContentViewer> contentViewer;
  rv = docShell->GetContentViewer(getter_AddRefs(contentViewer));
  nsCOMPtr<nsIDocumentViewer> documentViewer(do_QueryInterface(contentViewer));
  if (documentViewer) {
    documentViewer->SetTransformMediator(mXSLTransformMediator);
  }

  // Create the XSL stylesheet document
  nsCOMPtr<nsIDOMDocument> styleDOMDoc;
  nsAutoString emptyStr;
  emptyStr.Truncate();
  rv = NS_NewDOMDocument(getter_AddRefs(styleDOMDoc), emptyStr, emptyStr, nsnull, aUrl);
  if (NS_FAILED(rv)) return rv;
  nsCOMPtr<nsIDocument> styleDoc(do_QueryInterface(styleDOMDoc));

  // Create the XSL content sink
  nsCOMPtr<nsIXMLContentSink> sink;
  rv = NS_NewXSLContentSink(getter_AddRefs(sink), mXSLTransformMediator, styleDoc, aUrl, mWebShell);
  if (NS_FAILED(rv)) return rv;

  // Hook up the content sink to the parser's output and ask the parser
  // to start parsing the URL specified by aURL.
  parser->SetContentSink(sink);
  nsAutoString utf8(NS_LITERAL_STRING("UTF-8"));
  styleDoc->SetDocumentCharacterSet(utf8);
  parser->SetDocumentCharset(utf8, kCharsetFromDocTypeDefault);
  parser->Parse(aUrl);

  // Set the parser as the stream listener and start the URL load
  nsCOMPtr<nsIStreamListener> sl;
  rv = parser->QueryInterface(NS_GET_IID(nsIStreamListener), (void**)getter_AddRefs(sl));
  if (NS_FAILED(rv)) return rv;

  rv = NS_OpenURI(sl, nsnull, aUrl);

  return rv;
}

nsresult
nsXMLContentSink::ProcessStyleLink(nsIContent* aElement,
                                   const nsString& aHref, PRBool aAlternate,
                                   const nsString& aTitle, const nsString& aType,
                                   const nsString& aMedia)
{
  nsresult rv = NS_OK;

  if (aType.EqualsIgnoreCase(kXSLType))
    rv = ProcessXSLStyleLink(aElement, aHref, aAlternate, aTitle, aType, aMedia);
  else
    rv = ProcessCSSStyleLink(aElement, aHref, aAlternate, aTitle, aType, aMedia);

  return rv;
}

nsresult
nsXMLContentSink::ProcessXSLStyleLink(nsIContent* aElement,
                                   const nsString& aHref, PRBool aAlternate,
                                   const nsString& aTitle, const nsString& aType,
                                   const nsString& aMedia)
{
  nsresult rv = NS_OK;
  nsIURI* url;

  // LoadXSLStyleSheet needs a mWebShell.
  if (!mWebShell) return rv;

  rv = CreateStyleSheetURL(&url, aHref);
  if (NS_SUCCEEDED(rv)) {
    rv = LoadXSLStyleSheet(url, aType);
    NS_RELEASE(url);
  }

  return rv;
}

nsresult
nsXMLContentSink::ProcessCSSStyleLink(nsIContent* aElement,
                                   const nsString& aHref, PRBool aAlternate,
                                   const nsString& aTitle, const nsString& aType,
                                   const nsString& aMedia)
{
  nsresult result = NS_OK;

  if (aAlternate) { // if alternate, does it have title?
    if (0 == aTitle.Length()) { // alternates must have title
      return NS_OK; //return without error, for now
    }
  }

  nsAutoString  mimeType;
  nsAutoString  params;
  SplitMimeType(aType, mimeType, params);

  if ((0 == mimeType.Length()) || mimeType.EqualsIgnoreCase("text/css")) {
    nsIURI* url = nsnull;
// XXX we need to get passed in the nsILoadGroup here!
//    nsILoadGroup* group = mDocument->GetDocumentLoadGroup();
    result = NS_NewURI(&url, aHref, mDocumentBaseURL/*, group*/);
    if (NS_OK != result) {
      return NS_OK; // The URL is bad, move along, don't propogate the error (for now)
    }

    PRBool blockParser = PR_FALSE;
    if (! aAlternate) {
      if (0 < aTitle.Length()) {  // possibly preferred sheet
        if (0 == mPreferredStyle.Length()) {
          mPreferredStyle = aTitle;
          mCSSLoader->SetPreferredSheet(aTitle);
          mDocument->SetHeaderData(nsHTMLAtoms::headerDefaultStyle, aTitle);
        }
      }
      else {  // persistent sheet, block
        blockParser = PR_TRUE;
      }
    }

    PRBool doneLoading;
    result = mCSSLoader->LoadStyleLink(aElement, url, aTitle, aMedia, kNameSpaceID_Unknown,
                                       mStyleSheetCount++,
                                       ((blockParser) ? mParser : nsnull),
                                       doneLoading, nsnull);
    NS_RELEASE(url);
    if (NS_SUCCEEDED(result) && blockParser && (! doneLoading)) {
      result = NS_ERROR_HTMLPARSER_BLOCK;
    }
  }
  return result;
}

NS_IMETHODIMP
nsXMLContentSink::StyleSheetLoaded(nsICSSStyleSheet* aSheet, 
                                  PRBool aDidNotify)
{
  return NS_OK;
}

nsresult
nsXMLContentSink::ProcessSTYLETag(const nsIParserNode& aNode)
{
  nsresult rv = NS_OK;
  
  PRInt32 i, count;
  mStyleElement->GetAttributeCount(count);

  nsAutoString src;
  nsAutoString title; 
  nsAutoString type; 
  nsAutoString media; 

  for (i = 0; i < count; i++) {
    PRInt32 namespaceID;
    nsCOMPtr<nsIAtom> name, prefix;
    mStyleElement->GetAttributeNameAt(i,namespaceID,*getter_AddRefs(name),*getter_AddRefs(prefix));
    if (name.get() == nsHTMLAtoms::src) {
      mStyleElement->GetAttribute(namespaceID,name,src);
      src.StripWhitespace();
    }
    else if (name.get() == nsHTMLAtoms::title) {
      mStyleElement->GetAttribute(namespaceID,name,title);
      title.CompressWhitespace();
    }
    else if (name.get() == nsHTMLAtoms::type) {
      mStyleElement->GetAttribute(namespaceID,name,type);
      type.StripWhitespace();
    }
    else if (name.get() == nsHTMLAtoms::media) {
      mStyleElement->GetAttribute(namespaceID,name,media);
      media.ToLowerCase();
    }
  }

  nsAutoString  mimeType;
  nsAutoString  params;
  SplitMimeType(type, mimeType, params);

  PRBool blockParser = PR_FALSE;//kBlockByDefault;

  if ((0 == mimeType.Length()) || mimeType.EqualsIgnoreCase("text/css")) { 

    if (0 < title.Length()) {  // possibly preferred sheet
      if (0 == mPreferredStyle.Length()) {
        mPreferredStyle = title;
        mCSSLoader->SetPreferredSheet(title);
        mDocument->SetHeaderData(nsHTMLAtoms::headerDefaultStyle, title);
      }
    }

    PRBool doneLoading = PR_FALSE;

    nsIUnicharInputStream* uin = nsnull;
    if (0 == src.Length()) {

      // Create a text node holding the content
      nsCOMPtr<nsIContent> text;
      rv = NS_NewTextNode(getter_AddRefs(text));
      if (text) {
        nsCOMPtr<nsIDOMText> tc(do_QueryInterface(text));
        if (tc) {
          tc->SetData(mStyleText);
        }
        mStyleElement->AppendChildTo(text, PR_FALSE, PR_FALSE);
        text->SetDocument(mDocument, PR_FALSE, PR_TRUE);
      }

      // Create a string to hold the data and wrap it up in a unicode
      // input stream.
      rv = NS_NewStringUnicharInputStream(&uin, new nsString(mStyleText));
      if (NS_OK != rv) {
        return rv;
      }

      // Now that we have a url and a unicode input stream, parse the
      // style sheet.
      rv = mCSSLoader->LoadInlineStyle(mStyleElement, uin, title, media, kNameSpaceID_Unknown,
                                       mStyleSheetCount++, 
                                       ((blockParser) ? mParser : nsnull),
                                       doneLoading, this);
      NS_RELEASE(uin);
    } 
    else {
#if 0
      // This is non-standard. We support it for HTML for backwards compatibility,
      // but maybe we should withdraw the support for XHTML.

      // src with immediate style data doesn't add up
      // XXX what does nav do?
      // Use the SRC attribute value to load the URL
      nsIURI* url = nsnull;
      {
        rv = NS_NewURI(&url, src, mDocumentBaseURL);
      }
      if (NS_OK != rv) {
        return rv;
      }

      rv = mCSSLoader->LoadStyleLink(mStyleElement, url, title, media, kNameSpaceID_Unknown,
                                     mStyleSheetCount++, 
                                     ((blockParser) ? mParser : nsnull), 
                                     doneLoading, this);
      NS_RELEASE(url);
#endif
    }
    if (NS_SUCCEEDED(rv) && blockParser && (! doneLoading)) {
      rv = NS_ERROR_HTMLPARSER_BLOCK;
    }
  }//if ((0 == mimeType.Length()) || mimeType.EqualsIgnoreCase("text/css"))

  return rv;
}

nsresult
nsXMLContentSink::ProcessBASETag()
{
  nsresult rv = NS_OK;

  if (mDocument) {
    nsAutoString value;
  
    if (NS_CONTENT_ATTR_HAS_VALUE == mBaseElement->GetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::target, value)) {
      mDocument->SetBaseTarget(value);
    }

    if (NS_CONTENT_ATTR_HAS_VALUE == mBaseElement->GetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::href, value)) {
      nsCOMPtr<nsIURI> baseURI;
      rv = NS_NewURI(getter_AddRefs(baseURI), value, nsnull);
      if (NS_SUCCEEDED(rv)) {
        rv = mDocument->SetBaseURL(baseURI); // The document checks if it is legal to set this base
        if (NS_SUCCEEDED(rv)) {
          NS_IF_RELEASE(mDocumentBaseURL);
          mDocument->GetBaseURL(mDocumentBaseURL);
        }
      }
    }
  }

  return rv;
}

nsresult
nsXMLContentSink::ProcessHeaderData(nsIAtom* aHeader,const nsAReadableString& aValue,nsIHTMLContent* aContent)
{
  nsresult rv=NS_OK;
  // XXX necko isn't going to process headers coming in from the parser          
  //NS_WARNING("need to fix how necko adds mime headers (in HTMLContentSink::ProcessMETATag)");
  
  // see if we have a refresh "header".
  if (aHeader == nsHTMLAtoms::refresh) {
    // first get our baseURI
    nsCOMPtr<nsIDocShell> docShell = do_QueryInterface(mWebShell, &rv);
    if (NS_FAILED(rv)) return rv;
    
    nsCOMPtr<nsIURI> baseURI;
    nsCOMPtr<nsIWebNavigation> webNav = do_QueryInterface(docShell);
    rv = webNav->GetCurrentURI(getter_AddRefs(baseURI));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIRefreshURI> reefer = do_QueryInterface(mWebShell);
    if (reefer) {
      rv = reefer->RefreshURIFromHeader(baseURI, aValue);
      if (NS_FAILED(rv)) return rv;
    }
  } // END refresh
  return rv;
}

nsresult
nsXMLContentSink::ProcessMETATag()
{
  nsresult rv = NS_OK;

  // set any HTTP-EQUIV data into document's header data as well as url
  nsAutoString header;
  mMetaElement->GetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::httpEquiv, header);
  if (header.Length() > 0) {
    nsAutoString result;
    mMetaElement->GetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::content, result);
    if (result.Length() > 0) {
      header.ToLowerCase();
      nsCOMPtr<nsIAtom> fieldAtom(dont_AddRef(NS_NewAtom(header)));
      rv=ProcessHeaderData(fieldAtom,result,mMetaElement); 
    }//if (result.Length() > 0) 
  }//if (header.Length() > 0) 

  return rv;
}

nsresult
nsXMLContentSink::ProcessLINKTag()
{
  nsresult  result = NS_OK;

  nsAutoString href;
  nsAutoString rel; 
  nsAutoString title; 
  nsAutoString type; 
  nsAutoString media; 

  mLinkElement->GetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::href, href);
  mLinkElement->GetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::rel, rel);
  mLinkElement->GetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::title, title);
  mLinkElement->GetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::type, type);
  mLinkElement->GetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::media, media);

  href.CompressWhitespace();
  rel.CompressWhitespace();
  title.CompressWhitespace();
  type.CompressWhitespace();
  media.CompressWhitespace();
  media.ToLowerCase(); // HTML4.0 spec is inconsistent, make it case INSENSITIVE

  if (!href.IsEmpty()) {
    result = ProcessStyleLink(mLinkElement, href, rel.Find("alternate") == 0, title, type, media);
  }

  return result;
}


NS_IMETHODIMP
nsXMLContentSink::AddProcessingInstruction(const nsIParserNode& aNode)
{
  nsresult result = NS_OK;
  nsAutoString text, target, data;
  nsCOMPtr<nsIContent> node;

  FlushText();

  text.Assign(aNode.GetText());
  ParseProcessingInstruction(text, target, data);
  result = NS_NewXMLProcessingInstruction(getter_AddRefs(node), target, data);
  if (NS_OK == result) {
    node->SetDocument(mDocument, PR_FALSE, PR_TRUE);
    result = AddContentAsLeaf(node);
  }

  if (NS_OK == result) {
    nsAutoString type, href, title, media, alternate;

    // If it's a stylesheet PI...
    if (target.EqualsWithConversion(kStyleSheetPI)) {
      result = nsParserUtils::GetQuotedAttributeValue(text, NS_ConvertASCIItoUCS2("href"), href);
      // If there was an error or there's no href, we can't do
      // anything with this PI
      if ((NS_OK != result) || href.IsEmpty()) {
        return NS_OK;
      }
      result = nsParserUtils::GetQuotedAttributeValue(text, NS_ConvertASCIItoUCS2("type"), type);
      if (NS_FAILED(result)) {
        type.AssignWithConversion("text/css");  // Default the type attribute to the mime type for CSS
      }
      result = nsParserUtils::GetQuotedAttributeValue(text, NS_ConvertASCIItoUCS2("title"), title);
      if (NS_SUCCEEDED(result)) {
        title.CompressWhitespace();
      }
      result = nsParserUtils::GetQuotedAttributeValue(text, NS_ConvertASCIItoUCS2("media"), media);
      if (NS_SUCCEEDED(result)) {
        media.ToLowerCase();
      }
      result = nsParserUtils::GetQuotedAttributeValue(text, NS_ConvertASCIItoUCS2("alternate"), alternate);
      result = ProcessStyleLink(node, href, alternate.Equals(NS_LITERAL_STRING("yes")),
                                title, type, media);
    }
  }

  return result;
}


/**
 * DOCTYPE declaration is covered with very strict rules, which
 * makes our life here simpler because the XML parser has already
 * detected errors. The only slightly problematic case is whitespace
 * between the tokens. There MUST be whitespace between the tokens
 * EXCEPT right before > and [.
 */
static const char* kWhitespace = " \r\n\t\b"; // Optimized for typical cases

static void
GetDocTypeToken(nsString& aStr,
                nsString& aToken,
                PRBool aQuotedString)
{
  aStr.Trim(kWhitespace,PR_TRUE,PR_FALSE); // If we don't do this we must look ahead
                                           // before Cut() and adjust the cut amount.
  
  if (aQuotedString) {    
    PRInt32 endQuote = aStr.FindChar(aStr[0],PR_FALSE,1);
    aStr.Mid(aToken,1,endQuote-1);
    aStr.Cut(0,endQuote+1);
  } else {    
    static const char* kDelimiter = " >[\r\n\t\b"; // Optimized for typical cases
    PRInt32 tokenEnd = aStr.FindCharInSet(kDelimiter);
    if (tokenEnd > 0) {
      aStr.Left(aToken, tokenEnd);
      aStr.Cut(0, tokenEnd);
    }
  }
}

NS_IMETHODIMP
nsXMLContentSink::AddDocTypeDecl(const nsIParserNode& aNode, PRInt32 aMode)
{
  nsresult rv = NS_OK;

  nsCOMPtr<nsIDOMDocument> doc(do_QueryInterface(mDocument));
  if (!doc)
    return NS_OK;

  nsAutoString docTypeStr(aNode.GetText()); 
  nsAutoString str, name, publicId, systemId;

  if (docTypeStr.EqualsWithConversion("<!DOCTYPE", PR_FALSE, 9)) {
    docTypeStr.Right(str, docTypeStr.Length()-9);
  }

  GetDocTypeToken(str, name, PR_FALSE);

  nsAutoString token;

  GetDocTypeToken(str, token, PR_FALSE);
  if (token.Equals(NS_LITERAL_STRING("PUBLIC"))) {
    GetDocTypeToken(str, publicId, PR_TRUE);
    GetDocTypeToken(str, systemId, PR_TRUE);
  }
  else if (token.Equals(NS_LITERAL_STRING("SYSTEM"))) {
    GetDocTypeToken(str, systemId, PR_TRUE);
  }

  // The rest is the internal subset (minus whitespace and the trailing >)
  str.Truncate(str.Length()-1); // Delete the trailing >
  str.Trim(kWhitespace);

  nsCOMPtr<nsIDOMDocumentType> oldDocType;
  nsCOMPtr<nsIDOMDocumentType> docType;
  
  // Create a new doctype node
  rv = NS_NewDOMDocumentType(getter_AddRefs(docType),
                             name, nsnull, nsnull,
                             publicId, systemId, str);
  if (NS_FAILED(rv) || !docType) {
    return rv;
  }

  nsCOMPtr<nsIDOMNode> tmpNode;
  
  return doc->AppendChild(docType, getter_AddRefs(tmpNode));
}

nsresult
nsXMLContentSink::FlushText(PRBool aCreateTextNode, PRBool* aDidFlush)
{
  nsresult rv = NS_OK;
  PRBool didFlush = PR_FALSE;
  if (0 != mTextLength) {
    if (aCreateTextNode) {
      nsIContent* content;
      rv = NS_NewTextNode(&content);
      if (NS_OK == rv) {
        // Set the content's document
        content->SetDocument(mDocument, PR_FALSE, PR_TRUE);

        // Set the text in the text node
        nsITextContent* text = nsnull;
        content->QueryInterface(NS_GET_IID(nsITextContent), (void**) &text);
        text->SetText(mText, mTextLength, PR_FALSE);
        NS_RELEASE(text);

        // Add text to its parent
        AddContentAsLeaf(content);
        NS_RELEASE(content);
      }
    }
    mTextLength = 0;
    didFlush = PR_TRUE;
  }
  if (nsnull != aDidFlush) {
    *aDidFlush = didFlush;
  }
  return rv;
}

#define NS_ACCUMULATION_BUFFER_SIZE 4096

NS_IMETHODIMP
nsXMLContentSink::AddCharacterData(const nsIParserNode& aNode)
{
  return AddText(aNode.GetText());
}

nsresult
nsXMLContentSink::AddText(const nsAReadableString& aString)
{
  PRInt32 addLen = aString.Length();
  if (0 == addLen) {
    return NS_OK;
  }

  if (mInScript) {
    mScriptText.Append(aString);
  } else if (mInTitle) {
    mTitleText.Append(aString);
  } else if (mTextAreaElement) {
    mTextareaText.Append(aString);
  } else if (mStyleElement) {
    mStyleText.Append(aString);
  }

  // Create buffer when we first need it
  if (0 == mTextSize) {
    mText = (PRUnichar *) PR_MALLOC(sizeof(PRUnichar) * NS_ACCUMULATION_BUFFER_SIZE);
    if (nsnull == mText) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    mTextSize = NS_ACCUMULATION_BUFFER_SIZE;
  }

  // Copy data from string into our buffer; flush buffer when it fills up
  PRInt32 offset = 0;
  while (0 != addLen) {
    PRInt32 amount = mTextSize - mTextLength;
    if (amount > addLen) {
      amount = addLen;
    }
    if (0 == amount) {
      if (mConstrainSize) {
        nsresult rv = FlushText();
        if (NS_OK != rv) {
          return rv;
        }
      }
      else {
        mTextSize += addLen;
        mText = (PRUnichar *) PR_REALLOC(mText, sizeof(PRUnichar) * mTextSize);
        if (nsnull == mText) {
          return NS_ERROR_OUT_OF_MEMORY;
        }
      }
    }
    mTextLength +=
      nsContentUtils::CopyNewlineNormalizedUnicodeTo(aString, offset, 
                                                     &mText[mTextLength], 
                                                     amount);
    offset += amount;
    addLen -= amount;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXMLContentSink::AddUnparsedEntity(const nsIParserNode& aNode)
{
  printf("nsXMLContentSink::AddUnparsedEntity\n");
  return NS_OK;
}

NS_IMETHODIMP
nsXMLContentSink::AddNotation(const nsIParserNode& aNode)
{
  printf("nsXMLContentSink::AddNotation\n");
  return NS_OK;
}

NS_IMETHODIMP
nsXMLContentSink::AddEntityReference(const nsIParserNode& aNode)
{
  printf("nsXMLContentSink::AddEntityReference\n");
  return NS_OK;
}

PRInt32
nsXMLContentSink::GetNameSpaceId(nsIAtom* aPrefix)
{
  PRInt32 id = (nsnull == aPrefix) ? kNameSpaceID_None : kNameSpaceID_Unknown;
  if ((nsnull != mNameSpaceStack) && (0 < mNameSpaceStack->Count())) {
    PRInt32 index = mNameSpaceStack->Count() - 1;
    nsINameSpace* nameSpace = (nsINameSpace*)mNameSpaceStack->ElementAt(index);
    nameSpace->FindNameSpaceID(aPrefix, id);
  }

  return id;
}

nsINameSpace*
nsXMLContentSink::PopNameSpaces()
{
  if ((nsnull != mNameSpaceStack) && (0 < mNameSpaceStack->Count())) {
    PRInt32 index = mNameSpaceStack->Count() - 1;
    nsINameSpace* nameSpace = (nsINameSpace*)mNameSpaceStack->ElementAt(index);
    mNameSpaceStack->RemoveElementAt(index);
    return nameSpace;
  }

  return nsnull;
}

PRBool
nsXMLContentSink::IsHTMLNameSpace(PRInt32 aID)
{
  return PRBool(kNameSpaceID_HTML == aID);
}

nsIContent*
nsXMLContentSink::GetCurrentContent()
{
  if (nsnull != mContentStack) {
    PRUint32 count;
    mContentStack->Count(&count);
    PR_ASSERT(count);
    if (count) {
      return (nsIContent *)mContentStack->ElementAt(count-1);
    }
  }
  return nsnull;
}

PRInt32
nsXMLContentSink::PushContent(nsIContent *aContent)
{
  PRUint32 count;
  if (nsnull == mContentStack) {
    NS_NewISupportsArray(getter_AddRefs(mContentStack));
  }

  mContentStack->AppendElement(aContent);
  mContentStack->Count(&count);

  return count;
}

nsIContent*
nsXMLContentSink::PopContent()
{  
  nsIContent* content = nsnull;
  if (nsnull != mContentStack) {
    PRUint32 index, count;
    mContentStack->Count(&count);
    index =  count - 1;
    content = (nsIContent *)mContentStack->ElementAt(index);
    mContentStack->RemoveElementAt(index);
  }

  // The caller should NS_RELEASE the returned content object.
  return content;
}

void
nsXMLContentSink::StartLayout()
{

  // Reset scrolling to default settings for this shell.
  // This must happen before the initial reflow, when we create the root frame
  nsCOMPtr<nsIScrollable> scrollableContainer(do_QueryInterface(mWebShell));
  if (scrollableContainer) {
    scrollableContainer->ResetScrollbarPreferences();
  }

  PRInt32 i, ns = mDocument->GetNumberOfShells();
  for (i = 0; i < ns; i++) {
    nsIPresShell* shell = mDocument->GetShellAt(i);
    if (nsnull != shell) {
      // Make shell an observer for next time
      shell->BeginObservingDocument();

      // Resize-reflow this time
      nsCOMPtr<nsIPresContext> cx;
      shell->GetPresContext(getter_AddRefs(cx));
      nsRect r;
      cx->GetVisibleArea(r);
      shell->InitialReflow(r.width, r.height);

      // Now trigger a refresh
      nsCOMPtr<nsIViewManager> vm;
      shell->GetViewManager(getter_AddRefs(vm));
      if (vm) {
        RefreshIfEnabled(vm);
      }

      NS_RELEASE(shell);
    }
  }

  // If the document we are loading has a reference or it is a top level
  // frameset document, disable the scroll bars on the views.
  nsXPIDLCString ref;
  nsIURL* url;
  nsresult rv = mDocumentURL->QueryInterface(NS_GET_IID(nsIURL), (void**)&url);
  if (NS_SUCCEEDED(rv)) {
    rv = url->GetRef(getter_Copies(ref));
    NS_RELEASE(url);
  }
  if (rv == NS_OK) {
    mRef.AssignWithConversion(ref);
  }

  PRBool topLevelFrameset = PR_FALSE;
  nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(mWebShell));
  if (docShellAsItem) {
    nsCOMPtr<nsIDocShellTreeItem> root;
    docShellAsItem->GetSameTypeRootTreeItem(getter_AddRefs(root));
    if(docShellAsItem.get() == root.get()) {
      topLevelFrameset = PR_TRUE;
    }
  }

  if (ref || topLevelFrameset) {
    // XXX support more than one presentation-shell here

    // Get initial scroll preference and save it away; disable the
    // scroll bars.
    ns = mDocument->GetNumberOfShells();
    for (i = 0; i < ns; i++) {
      nsCOMPtr<nsIPresShell> shell(dont_AddRef(mDocument->GetShellAt(i)));
      if (shell) {
        nsCOMPtr<nsIViewManager> vm;
        shell->GetViewManager(getter_AddRefs(vm));
        if (vm) {
          nsIView* rootView = nsnull;
          vm->GetRootView(rootView);
          if (nsnull != rootView) {
            nsIScrollableView* sview = nsnull;
            rootView->QueryInterface(NS_GET_IID(nsIScrollableView), (void**) &sview);
            if (nsnull != sview) {
              sview->SetScrollPreference(nsScrollPreference_kNeverScroll);
            }
          }
        }
      }
    }
  }
}

NS_IMETHODIMP
nsXMLContentSink::ResumeParsing()
{
  if (mParser) {
    mParser->ContinueParsing();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXMLContentSink::EvaluateScript(nsString& aScript, nsIURI *aScriptURI, PRUint32 aLineNo, const char* aVersion)
{
  nsresult rv = NS_OK;

  if (0 < aScript.Length()) {
    nsCOMPtr<nsIScriptGlobalObject> scriptGlobal;
    mDocument->GetScriptGlobalObject(getter_AddRefs(scriptGlobal));
    if (scriptGlobal) {
      nsCOMPtr<nsIScriptContext> context;
      NS_ENSURE_SUCCESS(scriptGlobal->GetContext(getter_AddRefs(context)),
         NS_ERROR_FAILURE);

      char* url = nsnull;
      if (aScriptURI) {
        rv = aScriptURI->GetSpec(&url);
      }

      nsCOMPtr<nsIPrincipal> principal;
      if (NS_SUCCEEDED(rv)) {
        rv = mDocument->GetPrincipal(getter_AddRefs(principal));
        NS_ASSERTION(principal, "principal required for document");
      }

      if (NS_SUCCEEDED(rv)) {
        nsAutoString val;
        PRBool isUndefined;

        (void) context->EvaluateString(aScript, nsnull, principal, url, aLineNo, aVersion,
                                       val, &isUndefined);
      }
      if (url) {
        nsCRT::free(url);
      }
    }
  }

  return rv;
}

nsresult
nsXMLContentSink::ProcessEndSCRIPTTag(const nsIParserNode& aNode)
{
  nsresult result = NS_OK;
  if (mInScript) {
    nsCOMPtr<nsIURI> docURI( dont_AddRef( mDocument->GetDocumentURL() ) );
    result = EvaluateScript(mScriptText, docURI, mScriptLineNo, mScriptLanguageVersion);  
    mScriptText.Truncate();
    mInScript = PR_FALSE;
  }

  return result;
}

NS_IMETHODIMP
nsXMLContentSink::OnStreamComplete(nsIStreamLoader* aLoader,
                                   nsISupports* context,
                                   nsresult aStatus,
                                   PRUint32 stringLen,
                                   const char* string)
{
  nsresult rv = NS_OK;
  nsString aData; aData.AssignWithConversion(string, stringLen);

  if (NS_OK == aStatus) {
    { // scope in block so nsCOMPtr released at one point
      nsCOMPtr<nsIChannel> channel;
      nsCOMPtr<nsIRequest> request;
      aLoader->GetRequest(getter_AddRefs(request));
      if (request)
        channel = do_QueryInterface(request);

      nsCOMPtr<nsIURI> url;
      if (channel) {
        channel->GetURI(getter_AddRefs(url));
      }
      
      if(mParser) {
        mParser->UnblockParser(); // make sure to unblock the parser before evaluating the script
      }

      rv = EvaluateScript(aData, url, 1, mScriptLanguageVersion);
    }
    if (NS_FAILED(rv)) return rv;
  }

  if(mParser && mParser->IsParserEnabled()){
    rv=mParser->ContinueParsing();
  }

  if (NS_FAILED(rv)) return rv;

  return rv;
}

nsresult
nsXMLContentSink::ProcessStartSCRIPTTag(const nsIParserNode& aNode)
{
  nsresult rv = NS_OK;
  PRBool isJavaScript = PR_TRUE;
  const char* jsVersionString = nsnull;
  PRInt32 i, ac = aNode.GetAttributeCount();

  // Look for SRC attribute and look for a LANGUAGE attribute
  nsAutoString src;
  for (i = 0; i < ac; i++) {
    nsAutoString key(aNode.GetKeyAt(i));
    if (key.EqualsIgnoreCase("src")) {
      src = aNode.GetValueAt(i);
    }
    else if (key.EqualsIgnoreCase("type")) {
      const nsString& type = aNode.GetValueAt(i);

      nsAutoString  mimeType;
      nsAutoString  params;
      SplitMimeType(type, mimeType, params);

      isJavaScript = mimeType.EqualsIgnoreCase("text/javascript");
      if (isJavaScript) {
        JSVersion jsVersion = JSVERSION_DEFAULT;
        if (params.Find("version=", PR_TRUE) == 0) {
          if (params.Length() != 11 || params[8] != '1' || params[9] != '.')
            jsVersion = JSVERSION_UNKNOWN;
          else switch (params[10]) {
            case '0': jsVersion = JSVERSION_1_0; break;
            case '1': jsVersion = JSVERSION_1_1; break;
            case '2': jsVersion = JSVERSION_1_2; break;
            case '3': jsVersion = JSVERSION_1_3; break;
            case '4': jsVersion = JSVERSION_1_4; break;
            case '5': jsVersion = JSVERSION_1_5; break;
            default:  jsVersion = JSVERSION_UNKNOWN;
          }
        }
        jsVersionString = JS_VersionToString(jsVersion);
      }
    }
    else if (key.EqualsIgnoreCase("language")) {
      const nsString& lang = aNode.GetValueAt(i);
      isJavaScript = nsParserUtils::IsJavaScriptLanguage(lang, &jsVersionString);
    }
  }

  // Don't process scripts that aren't JavaScript
  if (isJavaScript) {
    mScriptLanguageVersion = jsVersionString;

    // If there is a SRC attribute...
    if (src.Length() > 0) {
      // Use the SRC attribute value to load the URL
      nsIURI* url = nsnull;
      nsAutoString absURL;
    // XXX we need to get passed in the nsILoadGroup here!
//      nsILoadGroup* group = mDocument->GetDocumentLoadGroup();
      rv = NS_NewURI(&url, src, mDocumentBaseURL);
      if (NS_OK != rv) {
        return rv;
      }

      // Check that this page is allowed to load this URI.
      NS_WITH_SERVICE(nsIScriptSecurityManager, securityManager, 
                      NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
      if (NS_FAILED(rv)) 
          return rv;
      rv = securityManager->CheckLoadURI(mDocumentBaseURL, url, nsIScriptSecurityManager::ALLOW_CHROME);
      if (NS_FAILED(rv)) 
          return rv;

      nsCOMPtr<nsIStreamLoader> loader;
      nsCOMPtr<nsILoadGroup> loadGroup;

      mDocument->GetDocumentLoadGroup(getter_AddRefs(loadGroup));
      rv = NS_NewStreamLoader(getter_AddRefs(loader), url, this, nsnull,
                              loadGroup);
      NS_RELEASE(url);
      if (NS_OK == rv) {
        rv = NS_ERROR_HTMLPARSER_BLOCK;
      }
    }
    else {
      // Wait until we get the script content
      mInScript = PR_TRUE;
      mConstrainSize = PR_FALSE;
      mScriptLineNo = (PRUint32)aNode.GetSourceLineNumber();
    }
  }

  return rv;
}

nsresult
nsXMLContentSink::RefreshIfEnabled(nsIViewManager* vm)
{
  if (vm) {
    nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(mWebShell));
    nsCOMPtr<nsIContentViewer> contentViewer;
    nsresult rv = docShell->GetContentViewer(getter_AddRefs(contentViewer));
    if (NS_SUCCEEDED(rv) && contentViewer) {
      PRBool enabled;
      contentViewer->GetEnableRendering(&enabled);
      if (enabled) {
        vm->EnableRefresh(NS_VMREFRESH_IMMEDIATE);
      }
    }
  }
  return NS_OK;
}


////////////////////////////////////////////////////////////////////////
//
//   XML Element Factory
//

class XMLElementFactoryImpl : public nsIElementFactory
{
protected:
  XMLElementFactoryImpl();
  virtual ~XMLElementFactoryImpl();

public:
  friend
  nsresult
  NS_NewXMLElementFactory(nsIElementFactory** aResult);

  // nsISupports interface
  NS_DECL_ISUPPORTS

  // nsIElementFactory interface
  NS_IMETHOD CreateInstanceByTag(nsINodeInfo *aNodeInfo, nsIContent** aResult);

};


XMLElementFactoryImpl::XMLElementFactoryImpl()
{
  NS_INIT_REFCNT();
}

XMLElementFactoryImpl::~XMLElementFactoryImpl()
{
}


NS_IMPL_ISUPPORTS(XMLElementFactoryImpl, NS_GET_IID(nsIElementFactory));


nsresult
NS_NewXMLElementFactory(nsIElementFactory** aResult)
{
  NS_PRECONDITION(aResult != nsnull, "null ptr");
  if (! aResult)
    return NS_ERROR_NULL_POINTER;

  XMLElementFactoryImpl* result = new XMLElementFactoryImpl();
  if (! result)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(result);
  *aResult = result;
  return NS_OK;
}



NS_IMETHODIMP
XMLElementFactoryImpl::CreateInstanceByTag(nsINodeInfo *aNodeInfo,
                                           nsIContent** aResult)
{
  nsCOMPtr<nsIXMLContent> xmlContent;
  nsresult rv = NS_NewXMLElement(getter_AddRefs(xmlContent), aNodeInfo);
  nsCOMPtr<nsIContent> result = do_QueryInterface(xmlContent);
  *aResult = result;
  NS_IF_ADDREF(*aResult);
  return rv; 
}

void 
nsXMLContentSink::GetElementFactory(PRInt32 aNameSpaceID, nsIElementFactory** aResult)
{
  nsresult rv;
  nsAutoString nameSpace;
  gNameSpaceManager->GetNameSpaceURI(aNameSpaceID, nameSpace);

  nsCAutoString contractID( NS_ELEMENT_FACTORY_CONTRACTID_PREFIX );
  contractID.AppendWithConversion(nameSpace);

  // Retrieve the appropriate factory.
  NS_WITH_SERVICE(nsIElementFactory, elementFactory, contractID, &rv);

  *aResult = elementFactory;
  NS_IF_ADDREF(*aResult);
}
