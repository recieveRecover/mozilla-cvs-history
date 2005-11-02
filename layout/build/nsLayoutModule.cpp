/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
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
 *   Pierre Phaneuf <pp@ludusdesign.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
#include "nsXBLAtoms.h"                 // to addref/release table
#include "nsCSSPseudoElements.h"        // to addref/release table
#include "nsCSSPseudoClasses.h"         // to addref/release table
#include "nsCSSAnonBoxes.h"             // to addref/release table
#include "nsCSSKeywords.h"              // to addref/release table
#include "nsCSSProps.h"                 // to addref/release table
#include "nsColorNames.h"               // to addref/release table
#include "nsContentCID.h"
#include "nsContentHTTPStartup.h"
#include "nsContentDLF.h"
#include "nsContentPolicyUtils.h"
#include "nsContentUtils.h"
#include "nsDataDocumentContentPolicy.h"
#include "nsLayoutStylesheetCache.h"
#include "nsDOMCID.h"
#include "nsCSSOMFactory.h"
#include "nsInspectorCSSUtils.h"
#include "nsEventListenerManager.h"
#include "nsGenericElement.h"
#include "nsHTMLAtoms.h"
#include "nsHTMLAtoms.h"
#include "nsHTMLContentSerializer.h"
#include "nsHTMLParts.h"
#include "nsGenericHTMLElement.h"
#include "nsICSSLoader.h"
#include "nsICSSParser.h"
#include "nsCSSScanner.h"
#include "nsICSSStyleSheet.h"
#include "nsICategoryManager.h"
#include "nsIComponentManager.h"
#include "nsIComputedDOMStyle.h"
#include "nsIContentIterator.h"
#include "nsIContentSerializer.h"
#include "nsIController.h"
#include "nsIControllers.h"
#include "nsIDOMDOMImplementation.h"
#include "nsIDOMEventGroup.h"
#include "nsIDOMRange.h"
#include "nsIDocument.h"
#include "nsIDocumentEncoder.h"
#include "nsIDocumentViewer.h"
#include "nsIEventListenerManager.h"
#include "nsIFactory.h"
#include "nsIFrameSelection.h"
#include "nsIFrameUtil.h"
#include "nsIGenericFactory.h"
#include "nsIHTMLCSSStyleSheet.h"
#include "nsIFragmentContentSink.h"
#include "nsHTMLStyleSheet.h"
#include "nsIHTMLToTextSink.h"
#include "nsILayoutDebugger.h"
#include "nsINameSpaceManager.h"
#include "nsINodeInfo.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsIPrivateDOMImplementation.h"
#include "nsIRangeUtils.h"
#include "nsIScriptNameSpaceManager.h"
#include "nsISelection.h"
#include "nsIXBLService.h"
#include "nsICaret.h"
#include "nsJSEnvironment.h"
#include "nsLayoutAtoms.h"
#include "nsPlainTextSerializer.h"
#include "mozSanitizingSerializer.h"
#include "nsRange.h"
#include "nsComputedDOMStyle.h"
#include "nsXMLContentSerializer.h"
#include "nsRuleNode.h"
#include "nsWyciwygProtocolHandler.h"
#include "nsContentAreaDragDrop.h"
#include "nsContentList.h"
#include "nsISyncLoadDOMService.h"
#include "nsCSSFrameConstructor.h"
#include "nsRepeatService.h"
#include "nsSprocketLayout.h"
#include "nsStackLayout.h"
#include "nsBox.h"
#include "nsSpaceManager.h"
#include "nsTextTransformer.h"
#include "nsIFrameTraversal.h"
#include "nsISelectionImageService.h"
#include "nsCSSLoader.h"
#include "nsXULAtoms.h"
#include "nsLayoutCID.h"
#include "nsStyleSet.h"
#include "nsImageFrame.h"
#include "nsILanguageAtomService.h"
#include "nsTextControlFrame.h"
#include "nsStyleSheetService.h"

// Transformiix stuff
#include "nsIDOMXPathExpression.h"
#include "nsIDOMNSXPathExpression.h"
#include "nsIDOMXPathNSResolver.h"
#include "nsIDOMXPathResult.h"
#include "nsXPathEvaluator.h"
#include "nsXPathException.h"
#include "txAtoms.h"
#include "txMozillaXSLTProcessor.h"
#include "txLog.h"
#include "txURIUtils.h"
#include "txXSLTProcessor.h"
#include "nsXPath1Scheme.h"
#include "nsXFormsXPathEvaluator.h"

// view stuff
#include "nsViewsCID.h"
#include "nsViewManager.h"
#include "nsContentCreatorFunctions.h"
#include "nsFrame.h"
#include "nsXBLWindowKeyHandler.h"

// DOM includes
#include "nsDOMException.h"
#include "nsGlobalWindowCommands.h"
#include "nsIControllerCommandTable.h"
#include "nsJSProtocolHandler.h"
#include "nsGlobalWindow.h"
#include "nsDOMClassInfo.h"
#include "nsScriptNameSpaceManager.h"
#include "nsIControllerContext.h"
#include "nsDOMScriptObjectFactory.h"
#include "nsAutoCopyListener.h"
#include "nsDOMAttribute.h"

#include "nsHTMLCanvasFrame.h"

#ifdef MOZ_ENABLE_CANVAS
#include "nsIDOMCanvasRenderingContext2D.h"
#endif

class nsIDocumentLoaderFactory;

#define PRODUCT_NAME "Gecko"

#define NS_HTMLIMGELEMENT_CONTRACTID \
  "@mozilla.org/content/element/html;1?name=img"

#define NS_HTMLOPTIONELEMENT_CONTRACTID \
  "@mozilla.org/content/element/html;1?name=option"

/* 0ddf4df8-4dbb-4133-8b79-9afb966514f5 */
#define NS_PLUGINDOCLOADERFACTORY_CID \
{ 0x0ddf4df8, 0x4dbb, 0x4133, { 0x8b, 0x79, 0x9a, 0xfb, 0x96, 0x65, 0x14, 0xf5 } }

#define NS_WINDOWCOMMANDTABLE_CID \
 { /* 0DE2FBFA-6B7F-11D7-BBBA-0003938A9D96 */        \
  0x0DE2FBFA, 0x6B7F, 0x11D7, {0xBB, 0xBA, 0x00, 0x03, 0x93, 0x8A, 0x9D, 0x96} }

static NS_DEFINE_CID(kWindowCommandTableCID, NS_WINDOWCOMMANDTABLE_CID);

#ifdef MOZ_XUL
#include "nsIXULDocument.h"
#include "nsIXULPopupListener.h"
#include "nsIXULPrototypeCache.h"
#include "nsIXULPrototypeDocument.h"
#include "nsIXULPrototypeDocument.h"
#include "nsIXULSortService.h"
#include "nsXULContentUtils.h"
#include "nsXULElement.h"

NS_IMETHODIMP
NS_NewXULContentBuilder(nsISupports* aOuter, REFNSIID aIID, void** aResult);

NS_IMETHODIMP
NS_NewXULTreeBuilder(nsISupports* aOuter, REFNSIID aIID, void** aResult);
#endif

PR_STATIC_CALLBACK(nsresult) Initialize(nsIModule* aSelf);
static void Shutdown();

#ifdef MOZ_MATHML
#include "nsMathMLAtoms.h"
#include "nsMathMLOperators.h"
#endif

#ifdef MOZ_XTF
#include "nsIXTFService.h"
#include "nsIXMLContentBuilder.h"
#endif

#ifdef MOZ_SVG
#include "nsSVGAtoms.h"
#include "nsSVGTypeCIDs.h"
#include "nsISVGRenderer.h"
#include "nsSVGRect.h"
#include "nsSVGUtils.h"

#ifdef MOZ_SVG_RENDERER_LIBART
void NS_InitSVGRendererLibartGlobals();
void NS_FreeSVGRendererLibartGlobals();
#endif
#ifdef MOZ_SVG_RENDERER_GDIPLUS
void NS_InitSVGRendererGDIPlusGlobals();
void NS_FreeSVGRendererGDIPlusGlobals();
#endif
#endif

// Transformiix
/* 1c1a3c01-14f6-11d6-a7f2-ea502af815dc */
#define TRANSFORMIIX_DOMCI_EXTENSION_CID   \
{ 0x1c1a3c01, 0x14f6, 0x11d6, {0xa7, 0xf2, 0xea, 0x50, 0x2a, 0xf8, 0x15, 0xdc} }

/* {0C351177-0159-4500-86B0-A219DFDE4258} */
#define TRANSFORMIIX_XPATH1_SCHEME_CID \
{ 0xc351177, 0x159, 0x4500, { 0x86, 0xb0, 0xa2, 0x19, 0xdf, 0xde, 0x42, 0x58 } }

#define TRANSFORMIIX_DOMCI_EXTENSION_CONTRACTID \
"@mozilla.org/transformiix-domci-extender;1"

NS_GENERIC_FACTORY_CONSTRUCTOR(nsXPath1SchemeProcessor)

NS_DOMCI_EXTENSION(Transformiix)
    static NS_DEFINE_CID(kXSLTProcessorCID, TRANSFORMIIX_XSLT_PROCESSOR_CID);
    NS_DOMCI_EXTENSION_ENTRY_BEGIN(XSLTProcessor)
        NS_DOMCI_EXTENSION_ENTRY_INTERFACE(nsIXSLTProcessor)
        NS_DOMCI_EXTENSION_ENTRY_INTERFACE(nsIXSLTProcessorObsolete) // XXX DEPRECATED
    NS_DOMCI_EXTENSION_ENTRY_END(XSLTProcessor, nsIXSLTProcessor, PR_TRUE,
                                 &kXSLTProcessorCID)

    static NS_DEFINE_CID(kXPathEvaluatorCID, TRANSFORMIIX_XPATH_EVALUATOR_CID);
    NS_DOMCI_EXTENSION_ENTRY_BEGIN(XPathEvaluator)
        NS_DOMCI_EXTENSION_ENTRY_INTERFACE(nsIDOMXPathEvaluator)
    NS_DOMCI_EXTENSION_ENTRY_END(XPathEvaluator, nsIDOMXPathEvaluator, PR_TRUE,
                                 &kXPathEvaluatorCID)

    NS_DOMCI_EXTENSION_ENTRY_BEGIN(XPathException)
        NS_DOMCI_EXTENSION_ENTRY_INTERFACE(nsIDOMXPathException)
        NS_DOMCI_EXTENSION_ENTRY_INTERFACE(nsIException)
    NS_DOMCI_EXTENSION_ENTRY_END(XPathException, nsIDOMXPathException, PR_TRUE,
                                 nsnull)

    NS_DOMCI_EXTENSION_ENTRY_BEGIN(XPathExpression)
        NS_DOMCI_EXTENSION_ENTRY_INTERFACE(nsIDOMXPathExpression)
        NS_DOMCI_EXTENSION_ENTRY_INTERFACE(nsIDOMNSXPathExpression)
    NS_DOMCI_EXTENSION_ENTRY_END(XPathExpression, nsIDOMXPathExpression,
                                 PR_TRUE, nsnull)

    NS_DOMCI_EXTENSION_ENTRY_BEGIN(XPathNSResolver)
        NS_DOMCI_EXTENSION_ENTRY_INTERFACE(nsIDOMXPathNSResolver)
    NS_DOMCI_EXTENSION_ENTRY_END(XPathNSResolver, nsIDOMXPathNSResolver,
                                 PR_TRUE, nsnull)

    NS_DOMCI_EXTENSION_ENTRY_BEGIN(XPathResult)
        NS_DOMCI_EXTENSION_ENTRY_INTERFACE(nsIDOMXPathResult)
    NS_DOMCI_EXTENSION_ENTRY_END(XPathResult, nsIDOMXPathResult, PR_TRUE,
                                 nsnull)
NS_DOMCI_EXTENSION_END

// Factory Constructor
NS_GENERIC_FACTORY_CONSTRUCTOR(txMozillaXSLTProcessor)
      NS_GENERIC_AGGREGATED_CONSTRUCTOR_INIT(nsXPathEvaluator, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsXFormsXPathEvaluator)

NS_DECL_DOM_CLASSINFO(XSLTProcessor)
NS_DECL_DOM_CLASSINFO(XPathEvaluator)
NS_DECL_DOM_CLASSINFO(XPathException)
NS_DECL_DOM_CLASSINFO(XPathExpression)
NS_DECL_DOM_CLASSINFO(XPathNSResolver)
NS_DECL_DOM_CLASSINFO(XPathResult)

static NS_METHOD
RegisterTransformiix(nsIComponentManager *aCompMgr,
                     nsIFile *aPath,
                     const char *registryLocation,
                     const char *componentType,
                     const nsModuleComponentInfo *info)
{
    nsresult rv;

    nsCOMPtr<nsICategoryManager> catman =
        do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);

    if (NS_FAILED(rv))
        return rv;

    nsXPIDLCString previous;
    rv = catman->AddCategoryEntry(JAVASCRIPT_DOM_CLASS,
                                  "XSLTProcessor",
                                  TRANSFORMIIX_DOMCI_EXTENSION_CONTRACTID,
                                  PR_TRUE, PR_TRUE, getter_Copies(previous));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = catman->AddCategoryEntry(JAVASCRIPT_DOM_CLASS,
                                  "XPathEvaluator",
                                  TRANSFORMIIX_DOMCI_EXTENSION_CONTRACTID,
                                  PR_TRUE, PR_TRUE, getter_Copies(previous));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = catman->AddCategoryEntry(JAVASCRIPT_DOM_CLASS,
                                  "XPathException",
                                  TRANSFORMIIX_DOMCI_EXTENSION_CONTRACTID,
                                  PR_TRUE, PR_TRUE, getter_Copies(previous));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = catman->AddCategoryEntry(JAVASCRIPT_DOM_CLASS,
                                  "XPathExpression",
                                  TRANSFORMIIX_DOMCI_EXTENSION_CONTRACTID,
                                  PR_TRUE, PR_TRUE, getter_Copies(previous));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = catman->AddCategoryEntry(JAVASCRIPT_DOM_CLASS,
                                  "XPathNSResolver",
                                  TRANSFORMIIX_DOMCI_EXTENSION_CONTRACTID,
                                  PR_TRUE, PR_TRUE, getter_Copies(previous));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = catman->AddCategoryEntry(JAVASCRIPT_DOM_CLASS,
                                  "XPathResult",
                                  TRANSFORMIIX_DOMCI_EXTENSION_CONTRACTID,
                                  PR_TRUE, PR_TRUE, getter_Copies(previous));
    NS_ENSURE_SUCCESS(rv, rv);

    char* iidString = NS_GET_IID(nsIXSLTProcessorObsolete).ToString();
    if (!iidString)
      return NS_ERROR_OUT_OF_MEMORY;
    rv = catman->AddCategoryEntry(JAVASCRIPT_DOM_INTERFACE,
                                  "nsIXSLTProcessorObsolete",
                                  iidString,
                                  PR_TRUE, PR_TRUE, getter_Copies(previous));
    NS_Free(iidString);
    NS_ENSURE_SUCCESS(rv, rv);

    iidString = NS_GET_IID(nsIXSLTProcessor).ToString();
    if (!iidString)
      return NS_ERROR_OUT_OF_MEMORY;
    rv = catman->AddCategoryEntry(JAVASCRIPT_DOM_INTERFACE,
                                  "nsIXSLTProcessor",
                                  iidString,
                                  PR_TRUE, PR_TRUE, getter_Copies(previous));
    NS_Free(iidString);

    return rv;
}

//-----------------------------------------------------------------------------

// Per bug 209804, it is necessary to observe the "xpcom-shutdown" event and
// perform shutdown of the layout modules at that time instead of waiting for
// our module destructor to run.  If we do not do this, then we risk holding
// references to objects in other component libraries that have already been
// shutdown (and possibly unloaded if 60709 is ever fixed).

class LayoutShutdownObserver : public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
};

NS_IMPL_ISUPPORTS1(LayoutShutdownObserver, nsIObserver)

NS_IMETHODIMP
LayoutShutdownObserver::Observe(nsISupports *aSubject,
                                const char *aTopic,
                                const PRUnichar *someData)
{
  if (!strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID))
    Shutdown();
  return NS_OK;
}

//-----------------------------------------------------------------------------

static PRBool gInitialized = PR_FALSE;
static nsIExceptionProvider *gXPathExceptionProvider;

// Perform our one-time intialization for this module

// static
nsresult PR_CALLBACK
Initialize(nsIModule* aSelf)
{
  NS_PRECONDITION(!gInitialized, "module already initialized");
  if (gInitialized) {
    return NS_OK;
  }

  gInitialized = PR_TRUE;

  nsJSEnvironment::Startup();
  nsresult rv = nsContentUtils::Init();
  if (NS_FAILED(rv)) {
    NS_ERROR("Could not initialize nsContentUtils");

    Shutdown();

    return rv;
  }
  rv = nsAttrValue::Init();
  if (NS_FAILED(rv)) {
    NS_ERROR("Could not initialize nsAttrValue");

    Shutdown();

    return rv;
  }

  // Register all of our atoms once
  nsCSSAnonBoxes::AddRefAtoms();
  nsCSSPseudoClasses::AddRefAtoms();
  nsCSSPseudoElements::AddRefAtoms();
  nsCSSKeywords::AddRefTable();
  nsCSSProps::AddRefTable();
  nsColorNames::AddRefTable();
  nsHTMLAtoms::AddRefAtoms();
  nsXBLAtoms::AddRefAtoms();
  nsLayoutAtoms::AddRefAtoms();
  nsXULAtoms::AddRefAtoms();

#ifdef MOZ_XUL
  rv = nsXULContentUtils::Init();
  if (NS_FAILED(rv)) {
    NS_ERROR("Could not initialize nsXULContentUtils");

    Shutdown();

    return rv;
  }
#endif

#ifdef MOZ_MATHML
  nsMathMLOperators::AddRefTable();
  nsMathMLAtoms::AddRefAtoms();
#endif

#ifdef MOZ_SVG
  if (nsSVGUtils::SVGEnabled())
    nsContentDLF::RegisterSVG();
  nsSVGAtoms::AddRefAtoms();
#ifdef MOZ_SVG_RENDERER_LIBART
  NS_InitSVGRendererLibartGlobals();
#endif
#ifdef MOZ_SVG_RENDERER_GDIPLUS
  NS_InitSVGRendererGDIPlusGlobals();
#endif
#endif

#ifdef DEBUG
  nsFrame::DisplayReflowStartup();
#endif
  rv = nsTextTransformer::Initialize();
  if (NS_FAILED(rv)) {
    NS_ERROR("Could not initialize nsTextTransformer");

    Shutdown();

    return rv;
  }
  nsDOMAttribute::Initialize();

  gXPathExceptionProvider = new nsXPathExceptionProvider();
  NS_IF_ADDREF(gXPathExceptionProvider);
  if (!gXPathExceptionProvider || !txXSLTProcessor::init()) {
    Shutdown();
    return NS_ERROR_OUT_OF_MEMORY;
  }
  nsCOMPtr<nsIExceptionService> xs =
      do_GetService(NS_EXCEPTIONSERVICE_CONTRACTID);
  if (xs)
    xs->RegisterExceptionProvider(gXPathExceptionProvider,
                                  NS_ERROR_MODULE_DOM_XPATH);

  // Add our shutdown observer.
  nsCOMPtr<nsIObserverService> observerService =
    do_GetService("@mozilla.org/observer-service;1");

  if (observerService) {
    LayoutShutdownObserver* observer =
      new LayoutShutdownObserver();

    if (!observer) {
      Shutdown();

      return NS_ERROR_OUT_OF_MEMORY;
    }

    observerService->AddObserver(observer, NS_XPCOM_SHUTDOWN_OBSERVER_ID, PR_FALSE);
  } else {
    NS_WARNING("Could not get an observer service.  We will leak on shutdown.");
  }

  return NS_OK;
}

// Shutdown this module, releasing all of the module resources

// static
void
Shutdown()
{
  NS_PRECONDITION(gInitialized, "module not initialized");
  if (!gInitialized)
    return;

  gInitialized = PR_FALSE;

  nsCOMPtr<nsIExceptionService> xs =
    do_GetService(NS_EXCEPTIONSERVICE_CONTRACTID);
  if (xs && gXPathExceptionProvider) {
    xs->UnregisterExceptionProvider(gXPathExceptionProvider,
                                    NS_ERROR_MODULE_DOM_XPATH);
  }
  NS_IF_RELEASE(gXPathExceptionProvider);

  NS_IF_RELEASE(NS_CLASSINFO_NAME(XSLTProcessor));
  NS_IF_RELEASE(NS_CLASSINFO_NAME(XPathEvaluator));
  NS_IF_RELEASE(NS_CLASSINFO_NAME(XPathException));
  NS_IF_RELEASE(NS_CLASSINFO_NAME(XPathExpression));
  NS_IF_RELEASE(NS_CLASSINFO_NAME(XPathNSResolver));
  NS_IF_RELEASE(NS_CLASSINFO_NAME(XPathResult));

  txXSLTProcessor::shutdown();
  nsDOMAttribute::Shutdown();
  nsRange::Shutdown();
  nsGenericElement::Shutdown();
  nsEventListenerManager::Shutdown();
  nsContentList::Shutdown();
  nsComputedDOMStyle::Shutdown();
  CSSLoaderImpl::Shutdown();
#ifdef DEBUG
  nsFrame::DisplayReflowShutdown();
#endif

  // Release all of our atoms
  nsColorNames::ReleaseTable();
  nsCSSProps::ReleaseTable();
  nsCSSKeywords::ReleaseTable();
  nsRepeatService::Shutdown();
  nsStackLayout::Shutdown();
  nsBox::Shutdown();

#ifdef MOZ_XUL
  nsXULContentUtils::Finish();
  nsXULElement::ReleaseGlobals();
  nsXULPrototypeElement::ReleaseGlobals();
  nsXULPrototypeScript::ReleaseGlobals();
  nsSprocketLayout::Shutdown();
#endif

#ifdef MOZ_MATHML
  nsMathMLOperators::ReleaseTable();
#endif

#ifdef MOZ_SVG
#ifdef MOZ_SVG_RENDERER_LIBART
  NS_FreeSVGRendererLibartGlobals();
#endif
#ifdef MOZ_SVG_RENDERER_GDIPLUS
  NS_FreeSVGRendererGDIPlusGlobals();
#endif
#endif

  nsCSSFrameConstructor::ReleaseGlobals();
  nsTextTransformer::Shutdown();
  nsSpaceManager::Shutdown();
  nsImageFrame::ReleaseGlobals();

  nsCSSScanner::ReleaseGlobals();

  NS_IF_RELEASE(nsContentDLF::gUAStyleSheet);
  NS_IF_RELEASE(nsRuleNode::gLangService);
  nsGenericHTMLElement::Shutdown();

  nsAttrValue::Shutdown();
  nsContentUtils::Shutdown();
  nsLayoutStylesheetCache::Shutdown();
  NS_NameSpaceManagerShutdown();
  nsStyleSet::FreeGlobals();

  nsGlobalWindow::ShutDown();
  nsDOMClassInfo::ShutDown();
  nsTextControlFrame::ShutDown();
  nsXBLWindowKeyHandler::ShutDown();
  nsAutoCopyListener::Shutdown();
}

#ifdef NS_DEBUG
nsresult NS_NewFrameUtil(nsIFrameUtil** aResult);
nsresult NS_NewLayoutDebugger(nsILayoutDebugger** aResult);
#endif

#ifdef MOZ_XUL
nsresult NS_NewBoxObject(nsIBoxObject** aResult);
nsresult NS_NewListBoxObject(nsIBoxObject** aResult);
nsresult NS_NewScrollBoxObject(nsIBoxObject** aResult);
nsresult NS_NewMenuBoxObject(nsIBoxObject** aResult);
nsresult NS_NewEditorBoxObject(nsIBoxObject** aResult);
nsresult NS_NewPopupBoxObject(nsIBoxObject** aResult);
nsresult NS_NewBrowserBoxObject(nsIBoxObject** aResult);
nsresult NS_NewIFrameBoxObject(nsIBoxObject** aResult);
nsresult NS_NewTreeBoxObject(nsIBoxObject** aResult);
#endif

#ifdef MOZ_ENABLE_CANVAS
nsresult NS_NewCanvasRenderingContext2D(nsIDOMCanvasRenderingContext2D** aResult);
#endif

nsresult NS_CreateFrameTraversal(nsIFrameTraversal** aResult);
nsresult NS_NewSelectionImageService(nsISelectionImageService** aResult);

nsresult NS_NewSelection(nsIFrameSelection** aResult);
nsresult NS_NewDomSelection(nsISelection** aResult);
nsresult NS_NewDocumentViewer(nsIDocumentViewer** aResult);
nsresult NS_NewRange(nsIDOMRange** aResult);
nsresult NS_NewRangeUtils(nsIRangeUtils** aResult);
nsresult NS_NewContentIterator(nsIContentIterator** aResult);
nsresult NS_NewPreContentIterator(nsIContentIterator** aResult);
nsresult NS_NewGenRegularIterator(nsIContentIterator** aResult);
nsresult NS_NewContentSubtreeIterator(nsIContentIterator** aResult);
nsresult NS_NewGenSubtreeIterator(nsIContentIterator** aInstancePtrResult);
nsresult NS_NewContentDocumentLoaderFactory(nsIDocumentLoaderFactory** aResult);
nsresult NS_NewHTMLCopyTextEncoder(nsIDocumentEncoder** aResult);
nsresult NS_NewTextEncoder(nsIDocumentEncoder** aResult);
nsresult NS_NewXBLService(nsIXBLService** aResult);
nsresult NS_NewContentPolicy(nsIContentPolicy** aResult);
nsresult NS_NewSyncLoadDOMService(nsISyncLoadDOMService** aResult);
nsresult NS_NewDOMEventGroup(nsIDOMEventGroup** aResult);

NS_IMETHODIMP NS_NewXULControllers(nsISupports* aOuter, REFNSIID aIID, void** aResult);

#ifdef MOZ_SVG
#ifdef MOZ_SVG_RENDERER_GDIPLUS
nsresult NS_NewSVGRendererGDIPlus(nsISVGRenderer** aResult);
#endif // MOZ_SVG_RENDERER_GDIPLUS
#ifdef MOZ_SVG_RENDERER_LIBART
nsresult NS_NewSVGRendererLibart(nsISVGRenderer** aResult);
#endif // MOZ_SVG_RENDERER_LIBART
#ifdef MOZ_SVG_RENDERER_CAIRO
nsresult NS_NewSVGRendererCairo(nsISVGRenderer** aResult);
#endif // MOZ_SVG_RENDERER_CAIRO
#endif

#define MAKE_CTOR(ctor_, iface_, func_)                   \
static NS_IMETHODIMP                                      \
ctor_(nsISupports* aOuter, REFNSIID aIID, void** aResult) \
{                                                         \
  *aResult = nsnull;                                      \
  if (aOuter)                                             \
    return NS_ERROR_NO_AGGREGATION;                       \
  iface_* inst;                                           \
  nsresult rv = func_(&inst);                             \
  if (NS_SUCCEEDED(rv)) {                                 \
    rv = inst->QueryInterface(aIID, aResult);             \
    NS_RELEASE(inst);                                     \
  }                                                       \
  return rv;                                              \
}

#ifdef DEBUG
MAKE_CTOR(CreateNewFrameUtil,             nsIFrameUtil,                NS_NewFrameUtil)
MAKE_CTOR(CreateNewLayoutDebugger,        nsILayoutDebugger,           NS_NewLayoutDebugger)
#endif

MAKE_CTOR(CreateNewFrameTraversal,      nsIFrameTraversal,      NS_CreateFrameTraversal)
MAKE_CTOR(CreateNewPresShell,           nsIPresShell,           NS_NewPresShell)
#ifdef MOZ_XUL
MAKE_CTOR(CreateNewBoxObject,           nsIBoxObject,           NS_NewBoxObject)
MAKE_CTOR(CreateNewListBoxObject,       nsIBoxObject,           NS_NewListBoxObject)
MAKE_CTOR(CreateNewMenuBoxObject,       nsIBoxObject,           NS_NewMenuBoxObject)
MAKE_CTOR(CreateNewPopupBoxObject,      nsIBoxObject,           NS_NewPopupBoxObject)
MAKE_CTOR(CreateNewBrowserBoxObject,    nsIBoxObject,           NS_NewBrowserBoxObject)
MAKE_CTOR(CreateNewEditorBoxObject,     nsIBoxObject,           NS_NewEditorBoxObject)
MAKE_CTOR(CreateNewIFrameBoxObject,     nsIBoxObject,           NS_NewIFrameBoxObject)
MAKE_CTOR(CreateNewScrollBoxObject,     nsIBoxObject,           NS_NewScrollBoxObject)
MAKE_CTOR(CreateNewTreeBoxObject,       nsIBoxObject,           NS_NewTreeBoxObject)
#endif
MAKE_CTOR(CreateSelectionImageService,  nsISelectionImageService,NS_NewSelectionImageService)
#ifdef MOZ_SVG
#ifdef MOZ_SVG_RENDERER_GDIPLUS
MAKE_CTOR(CreateNewSVGRendererGDIPlus,  nsISVGRenderer,         NS_NewSVGRendererGDIPlus)
#endif // MOZ_SVG_RENDERER_GDIPLUS
#ifdef MOZ_SVG_RENDERER_LIBART
MAKE_CTOR(CreateNewSVGRendererLibart,   nsISVGRenderer,         NS_NewSVGRendererLibart)
#endif // MOZ_SVG_RENDERER_LIBART
#ifdef MOZ_SVG_RENDERER_CAIRO
MAKE_CTOR(CreateNewSVGRendererCairo,   nsISVGRenderer,         NS_NewSVGRendererCairo)
#endif // MOZ_SVG_RENDERER_CAIRO
#endif
MAKE_CTOR(CreateCaret,                  nsICaret,               NS_NewCaret)

MAKE_CTOR(CreateNameSpaceManager,         nsINameSpaceManager,         NS_GetNameSpaceManager)
MAKE_CTOR(CreateEventListenerManager,     nsIEventListenerManager,     NS_NewEventListenerManager)
MAKE_CTOR(CreateDOMEventGroup,            nsIDOMEventGroup,            NS_NewDOMEventGroup)
MAKE_CTOR(CreateDocumentViewer,           nsIDocumentViewer,           NS_NewDocumentViewer)
MAKE_CTOR(CreateCSSStyleSheet,            nsICSSStyleSheet,            NS_NewCSSStyleSheet)
MAKE_CTOR(CreateHTMLDocument,             nsIDocument,                 NS_NewHTMLDocument)
MAKE_CTOR(CreateHTMLCSSStyleSheet,        nsIHTMLCSSStyleSheet,        NS_NewHTMLCSSStyleSheet)
MAKE_CTOR(CreateDOMImplementation,        nsIDOMDOMImplementation,     NS_NewDOMImplementation)
MAKE_CTOR(CreateXMLDocument,              nsIDocument,                 NS_NewXMLDocument)
#ifdef MOZ_SVG
MAKE_CTOR(CreateSVGDocument,              nsIDocument,                 NS_NewSVGDocument)
#endif
MAKE_CTOR(CreateImageDocument,            nsIDocument,                 NS_NewImageDocument)
MAKE_CTOR(CreateCSSParser,                nsICSSParser,                NS_NewCSSParser)
MAKE_CTOR(CreateCSSLoader,                nsICSSLoader,                NS_NewCSSLoader)
MAKE_CTOR(CreateDOMSelection,             nsISelection,                NS_NewDomSelection)
MAKE_CTOR(CreateSelection,                nsIFrameSelection,           NS_NewSelection)
MAKE_CTOR(CreateRange,                    nsIDOMRange,                 NS_NewRange)
MAKE_CTOR(CreateRangeUtils,               nsIRangeUtils,               NS_NewRangeUtils)
MAKE_CTOR(CreateContentIterator,          nsIContentIterator,          NS_NewContentIterator)
MAKE_CTOR(CreatePreContentIterator,       nsIContentIterator,          NS_NewPreContentIterator)
MAKE_CTOR(CreateGeneratedContentIterator, nsIContentIterator,          NS_NewGenRegularIterator)
MAKE_CTOR(CreateGeneratedSubtreeIterator, nsIContentIterator,          NS_NewGenSubtreeIterator)
MAKE_CTOR(CreateSubtreeIterator,          nsIContentIterator,          NS_NewContentSubtreeIterator)
// CreateHTMLImgElement, see below
// CreateHTMLOptionElement, see below
MAKE_CTOR(CreateTextEncoder,              nsIDocumentEncoder,          NS_NewTextEncoder)
MAKE_CTOR(CreateHTMLCopyTextEncoder,      nsIDocumentEncoder,          NS_NewHTMLCopyTextEncoder)
MAKE_CTOR(CreateXMLContentSerializer,     nsIContentSerializer,        NS_NewXMLContentSerializer)
MAKE_CTOR(CreateHTMLContentSerializer,    nsIContentSerializer,        NS_NewHTMLContentSerializer)
MAKE_CTOR(CreatePlainTextSerializer,      nsIContentSerializer,        NS_NewPlainTextSerializer)
MAKE_CTOR(CreateHTMLFragmentSink,         nsIFragmentContentSink,      NS_NewHTMLFragmentContentSink)
MAKE_CTOR(CreateHTMLFragmentSink2,        nsIFragmentContentSink,      NS_NewHTMLFragmentContentSink2)
MAKE_CTOR(CreateXMLFragmentSink,          nsIFragmentContentSink,      NS_NewXMLFragmentContentSink)
MAKE_CTOR(CreateXMLFragmentSink2,         nsIFragmentContentSink,      NS_NewXMLFragmentContentSink2)
MAKE_CTOR(CreateSanitizingHTMLSerializer, nsIContentSerializer,        NS_NewSanitizingHTMLSerializer)
MAKE_CTOR(CreateXBLService,               nsIXBLService,               NS_NewXBLService)
MAKE_CTOR(CreateContentPolicy,            nsIContentPolicy,            NS_NewContentPolicy)
MAKE_CTOR(CreateComputedDOMStyle,         nsIComputedDOMStyle,         NS_NewComputedDOMStyle)
#ifdef MOZ_XUL
MAKE_CTOR(CreateXULSortService,           nsIXULSortService,           NS_NewXULSortService)
// NS_NewXULContentBuilder
// NS_NewXULTreeBuilder
MAKE_CTOR(CreateXULDocument,              nsIXULDocument,              NS_NewXULDocument)
MAKE_CTOR(CreateXULPopupListener,         nsIXULPopupListener,         NS_NewXULPopupListener)
// NS_NewXULControllers
// NS_NewXULPrototypeCache
#endif
#ifdef MOZ_XTF
MAKE_CTOR(CreateXTFService,               nsIXTFService,               NS_NewXTFService)
MAKE_CTOR(CreateXMLContentBuilder,        nsIXMLContentBuilder,        NS_NewXMLContentBuilder)
#endif
#ifdef MOZ_SVG
MAKE_CTOR(CreateSVGRect,                  nsIDOMSVGRect,               NS_NewSVGRect)
#endif
NS_GENERIC_FACTORY_CONSTRUCTOR(nsContentHTTPStartup)
MAKE_CTOR(CreateContentDLF,               nsIDocumentLoaderFactory,    NS_NewContentDocumentLoaderFactory)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsCSSOMFactory)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsInspectorCSSUtils)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsWyciwygProtocolHandler)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsContentAreaDragDrop)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDataDocumentContentPolicy)
MAKE_CTOR(CreateSyncLoadDOMService,       nsISyncLoadDOMService,       NS_NewSyncLoadDOMService)
MAKE_CTOR(CreatePluginDocument,           nsIDocument,                 NS_NewPluginDocument)

#ifdef MOZ_ENABLE_CANVAS
MAKE_CTOR(CreateCanvasRenderingContext2D, nsIDOMCanvasRenderingContext2D, NS_NewCanvasRenderingContext2D)
#endif

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsStyleSheetService, Init)

// views are not refcounted, so this is the same as
// NS_GENERIC_FACTORY_CONSTRUCTOR without the NS_ADDREF/NS_RELEASE
#define NS_GENERIC_FACTORY_CONSTRUCTOR_NOREFS(_InstanceClass)                 \
static NS_IMETHODIMP                                                          \
_InstanceClass##Constructor(nsISupports *aOuter, REFNSIID aIID,               \
                            void **aResult)                                   \
{                                                                             \
    nsresult rv;                                                              \
                                                                              \
    _InstanceClass * inst;                                                    \
                                                                              \
    *aResult = NULL;                                                          \
    if (NULL != aOuter) {                                                     \
        rv = NS_ERROR_NO_AGGREGATION;                                         \
        return rv;                                                            \
    }                                                                         \
                                                                              \
    NS_NEWXPCOM(inst, _InstanceClass);                                        \
    if (NULL == inst) {                                                       \
        rv = NS_ERROR_OUT_OF_MEMORY;                                          \
        return rv;                                                            \
    }                                                                         \
    rv = inst->QueryInterface(aIID, aResult);                                 \
                                                                              \
    return rv;                                                                \
}                                                                             \

NS_GENERIC_FACTORY_CONSTRUCTOR(nsViewManager)

static NS_IMETHODIMP
CreateHTMLImgElement(nsISupports* aOuter, REFNSIID aIID, void** aResult)
{
  *aResult = nsnull;
  if (aOuter)
    return NS_ERROR_NO_AGGREGATION;
  // Note! NS_NewHTMLImageElement is special cased to handle a null nodeinfo
  nsIContent* inst = NS_NewHTMLImageElement(nsnull);
  nsresult rv = NS_ERROR_OUT_OF_MEMORY;
  if (inst) {
    NS_ADDREF(inst);
    rv = inst->QueryInterface(aIID, aResult);
    NS_RELEASE(inst);
  }
  return rv;
}

static NS_IMETHODIMP
RegisterHTMLImgElement(nsIComponentManager *aCompMgr,
                       nsIFile* aPath,
                       const char* aRegistryLocation,
                       const char* aComponentType,
                       const nsModuleComponentInfo* aInfo)
{
  nsCOMPtr<nsICategoryManager> catman =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID);

  if (!catman)
    return NS_ERROR_FAILURE;

  nsXPIDLCString previous;
  nsresult rv = catman->AddCategoryEntry(JAVASCRIPT_GLOBAL_CONSTRUCTOR_CATEGORY,
                                         "Image", NS_HTMLIMGELEMENT_CONTRACTID,
                                         PR_TRUE, PR_TRUE, getter_Copies(previous));
  NS_ENSURE_SUCCESS(rv, rv);

  return catman->AddCategoryEntry(JAVASCRIPT_GLOBAL_CONSTRUCTOR_PROTO_ALIAS_CATEGORY,
                                  "Image", "HTMLImageElement",
                                  PR_TRUE, PR_TRUE, getter_Copies(previous));
}

static NS_IMETHODIMP
UnregisterHTMLImgElement(nsIComponentManager* aCompMgr,
                         nsIFile* aPath,
                         const char* aRegistryLocation,
                         const nsModuleComponentInfo* aInfo)
{
  // XXX remove category entry
  return NS_OK;
}

static NS_IMETHODIMP
CreateHTMLOptionElement(nsISupports* aOuter, REFNSIID aIID, void** aResult)
{
  *aResult = nsnull;
  if (aOuter)
    return NS_ERROR_NO_AGGREGATION;
  // Note! NS_NewHTMLOptionElement is special cased to handle a null nodeinfo
  nsIContent* inst = NS_NewHTMLOptionElement(nsnull);
  nsresult rv = NS_ERROR_OUT_OF_MEMORY;
  if (inst) {
    NS_ADDREF(inst);
    rv = inst->QueryInterface(aIID, aResult);
    NS_RELEASE(inst);
  }
  return rv;
}

static NS_IMETHODIMP
RegisterHTMLOptionElement(nsIComponentManager *aCompMgr,
                          nsIFile* aPath,
                          const char* aRegistryLocation,
                          const char* aComponentType,
                          const nsModuleComponentInfo* aInfo)
{
  nsCOMPtr<nsICategoryManager> catman =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID);

  if (!catman)
    return NS_ERROR_FAILURE;

  nsXPIDLCString previous;
  nsresult rv = catman->AddCategoryEntry(JAVASCRIPT_GLOBAL_CONSTRUCTOR_CATEGORY,
                                         "Option", NS_HTMLOPTIONELEMENT_CONTRACTID,
                                         PR_TRUE, PR_TRUE, getter_Copies(previous));
  NS_ENSURE_SUCCESS(rv, rv);

  return catman->AddCategoryEntry(JAVASCRIPT_GLOBAL_CONSTRUCTOR_PROTO_ALIAS_CATEGORY,
                                  "Option", "HTMLOptionElement",
                                  PR_TRUE, PR_TRUE, getter_Copies(previous));
}

static NS_IMETHODIMP
UnregisterHTMLOptionElement(nsIComponentManager* aCompMgr,
                            nsIFile* aPath,
                            const char* aRegistryLocation,
                            const nsModuleComponentInfo* aInfo)
{
  // XXX remove category entry
  return NS_OK;
}

static NS_METHOD
RegisterDataDocumentContentPolicy(nsIComponentManager *aCompMgr,
                                  nsIFile* aPath,
                                  const char* aRegistryLocation,
                                  const char* aComponentType,
                                  const nsModuleComponentInfo* aInfo)
{
  nsresult rv;
  nsCOMPtr<nsICategoryManager> catman =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  if (NS_FAILED(rv)) {
    return rv;
  }
  nsXPIDLCString previous;
  return catman->AddCategoryEntry("content-policy",
                                  NS_DATADOCUMENTCONTENTPOLICY_CONTRACTID,
                                  NS_DATADOCUMENTCONTENTPOLICY_CONTRACTID,
                                  PR_TRUE, PR_TRUE, getter_Copies(previous));
}

static NS_METHOD
UnregisterDataDocumentContentPolicy(nsIComponentManager *aCompMgr,
                                    nsIFile *aPath,
                                    const char *registryLocation,
                                    const nsModuleComponentInfo *info)
{
  nsresult rv;
  nsCOMPtr<nsICategoryManager> catman =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  if (NS_FAILED(rv)) {
    return rv;
  }
  
  return catman->DeleteCategoryEntry("content-policy",
                                     NS_DATADOCUMENTCONTENTPOLICY_CONTRACTID,
                                     PR_TRUE);
}

static NS_METHOD
CreateWindowCommandTableConstructor(nsISupports *aOuter,
                                    REFNSIID aIID, void **aResult)
{
  nsresult rv;
  nsCOMPtr<nsIControllerCommandTable> commandTable =
      do_CreateInstance(NS_CONTROLLERCOMMANDTABLE_CONTRACTID, &rv);
  if (NS_FAILED(rv)) return rv;

  rv = nsWindowCommandRegistration::RegisterWindowCommands(commandTable);
  if (NS_FAILED(rv)) return rv;

  return commandTable->QueryInterface(aIID, aResult);
}

static NS_METHOD
CreateWindowControllerWithSingletonCommandTable(nsISupports *aOuter,
                                      REFNSIID aIID, void **aResult)
{
  nsresult rv;
  nsCOMPtr<nsIController> controller =
       do_CreateInstance("@mozilla.org/embedcomp/base-command-controller;1", &rv);

 if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIControllerCommandTable> windowCommandTable = do_GetService(kWindowCommandTableCID, &rv);
  if (NS_FAILED(rv)) return rv;

  // this is a singleton; make it immutable
  windowCommandTable->MakeImmutable();

  nsCOMPtr<nsIControllerContext> controllerContext = do_QueryInterface(controller, &rv);
  if (NS_FAILED(rv)) return rv;

  controllerContext->Init(windowCommandTable);
  if (NS_FAILED(rv)) return rv;

  return controller->QueryInterface(aIID, aResult);
}

NS_GENERIC_FACTORY_CONSTRUCTOR(nsDOMScriptObjectFactory)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsBaseDOMException)

// The list of components we register
static const nsModuleComponentInfo gComponents[] = {
#ifdef MOZ_SVG
#ifdef MOZ_SVG_RENDERER_GDIPLUS
  { "SVG GdiPlus Renderer",
    NS_SVG_RENDERER_GDIPLUS_CID,
    NS_SVG_RENDERER_GDIPLUS_CONTRACTID,
    CreateNewSVGRendererGDIPlus },
#endif // MOZ_SVG_RENDERER_GDIPLUS
#ifdef MOZ_SVG_RENDERER_LIBART
  { "SVG Libart Renderer",
    NS_SVG_RENDERER_LIBART_CID,
    NS_SVG_RENDERER_LIBART_CONTRACTID,
    CreateNewSVGRendererLibart },
#endif // MOZ_SVG_RENDERER_LIBART
#ifdef MOZ_SVG_RENDERER_CAIRO
  { "SVG Cairo Renderer",
    NS_SVG_RENDERER_CAIRO_CID,
    NS_SVG_RENDERER_CAIRO_CONTRACTID,
    CreateNewSVGRendererCairo },
#endif // MOZ_SVG_RENDERER_CAIRO
#endif // MOZ_SVG

#ifdef DEBUG
  { "Frame utility",
    NS_FRAME_UTIL_CID,
    nsnull,
    CreateNewFrameUtil },
  { "Layout debugger",
    NS_LAYOUT_DEBUGGER_CID,
    nsnull,
    CreateNewLayoutDebugger },
#endif

  { "Frame Traversal",
    NS_FRAMETRAVERSAL_CID,
    nsnull,
    CreateNewFrameTraversal },

  { "selection image storage",
    NS_SELECTIONIMAGESERVICE_CID,
    nsnull,
    CreateSelectionImageService },

  { "caret",
    NS_CARET_CID,
    "@mozilla.org/layout/caret;1",
    CreateCaret },

  // XXX ick
  { "Presentation shell",
    NS_PRESSHELL_CID,
    nsnull,
    CreateNewPresShell },

  // XXX end ick

#ifdef MOZ_XUL
  { "XUL Box Object",
    NS_BOXOBJECT_CID,
    "@mozilla.org/layout/xul-boxobject;1",
    CreateNewBoxObject },

  { "XUL Listbox Box Object",
    NS_LISTBOXOBJECT_CID,
    "@mozilla.org/layout/xul-boxobject-listbox;1",
    CreateNewListBoxObject },

  { "XUL Menu Box Object",
    NS_MENUBOXOBJECT_CID,
    "@mozilla.org/layout/xul-boxobject-menu;1",
    CreateNewMenuBoxObject },

  { "XUL Popup Box Object",
    NS_POPUPBOXOBJECT_CID,
    "@mozilla.org/layout/xul-boxobject-popup;1",
    CreateNewPopupBoxObject },

  { "XUL Browser Box Object",
    NS_BROWSERBOXOBJECT_CID,
    "@mozilla.org/layout/xul-boxobject-browser;1",
    CreateNewBrowserBoxObject },

  { "XUL Editor Box Object",
    NS_EDITORBOXOBJECT_CID,
    "@mozilla.org/layout/xul-boxobject-editor;1",
    CreateNewEditorBoxObject },

  { "XUL Iframe Object",
    NS_IFRAMEBOXOBJECT_CID,
    "@mozilla.org/layout/xul-boxobject-iframe;1",
    CreateNewIFrameBoxObject },

  { "XUL ScrollBox Object",
    NS_SCROLLBOXOBJECT_CID,
    "@mozilla.org/layout/xul-boxobject-scrollbox;1",
    CreateNewScrollBoxObject },

  { "XUL Tree Box Object",
    NS_TREEBOXOBJECT_CID,
    "@mozilla.org/layout/xul-boxobject-tree;1",
    CreateNewTreeBoxObject },

#endif

  { "Namespace manager",
    NS_NAMESPACEMANAGER_CID,
    NS_NAMESPACEMANAGER_CONTRACTID,
    CreateNameSpaceManager },

  { "Event listener manager",
    NS_EVENTLISTENERMANAGER_CID,
    nsnull,
    CreateEventListenerManager },

  { "DOM Event group",
    NS_DOMEVENTGROUP_CID,
    nsnull,
    CreateDOMEventGroup },

  { "Document Viewer",
    NS_DOCUMENT_VIEWER_CID,
    nsnull,
    CreateDocumentViewer },

  { "CSS Style Sheet",
    NS_CSS_STYLESHEET_CID,
    nsnull,
    CreateCSSStyleSheet },

  { "HTML document",
    NS_HTMLDOCUMENT_CID,
    nsnull,
    CreateHTMLDocument },

  { "HTML-CSS style sheet",
    NS_HTML_CSS_STYLESHEET_CID,
    nsnull,
    CreateHTMLCSSStyleSheet },

  { "DOM implementation",
    NS_DOM_IMPLEMENTATION_CID,
    nsnull,
    CreateDOMImplementation },


  { "XML document",
    NS_XMLDOCUMENT_CID,
    "@mozilla.org/xml/xml-document;1",
    CreateXMLDocument },

#ifdef MOZ_SVG
  { "SVG document",
    NS_SVGDOCUMENT_CID,
    "@mozilla.org/svg/svg-document;1",
    CreateSVGDocument },
#endif

  { "Image document",
    NS_IMAGEDOCUMENT_CID,
    nsnull,
    CreateImageDocument },

  { "CSS parser",
    NS_CSSPARSER_CID,
    "@mozilla.org/content/css-parser;1",
    CreateCSSParser },

  { "CSS loader",
    NS_CSS_LOADER_CID,
    nsnull,
    CreateCSSLoader },

  { "Dom selection",
    NS_DOMSELECTION_CID,
    "@mozilla.org/content/dom-selection;1",
    CreateDOMSelection },

  { "Frame selection",
    NS_FRAMESELECTION_CID,
    nsnull,
    CreateSelection },

  { "Range",
    NS_RANGE_CID,
    "@mozilla.org/content/range;1",
    CreateRange },

  { "Range Utils",
    NS_RANGEUTILS_CID,
    "@mozilla.org/content/range-utils;1",
    CreateRangeUtils },

  { "Content iterator",
    NS_CONTENTITERATOR_CID,
    "@mozilla.org/content/post-content-iterator;1",
    CreateContentIterator },

  { "Pre Content iterator",
    NS_PRECONTENTITERATOR_CID,
    "@mozilla.org/content/pre-content-iterator;1",
    CreatePreContentIterator },

  { "Generated Content iterator",
    NS_GENERATEDCONTENTITERATOR_CID,
    "@mozilla.org/content/generated-content-iterator;1",
    CreateGeneratedContentIterator },

  { "Generated Subtree iterator",
    NS_GENERATEDSUBTREEITERATOR_CID,
    "@mozilla.org/content/generated-subtree-content-iterator;1",
    CreateGeneratedSubtreeIterator },

  { "Subtree iterator",
    NS_SUBTREEITERATOR_CID,
    "@mozilla.org/content/subtree-content-iterator;1",
    CreateSubtreeIterator },

  { "CSS Object Model Factory",
    NS_CSSOMFACTORY_CID,
    nsnull,
    nsCSSOMFactoryConstructor },

  { "Inspector CSS Utils",
    NS_INSPECTORCSSUTILS_CID,
    nsnull,
    nsInspectorCSSUtilsConstructor },

  // Needed to support "new Option;" and "new Image;" in JavaScript
  { "HTML img element",
    NS_HTMLIMAGEELEMENT_CID,
    NS_HTMLIMGELEMENT_CONTRACTID,
    CreateHTMLImgElement,
    RegisterHTMLImgElement, 
    UnregisterHTMLImgElement },

  { "HTML option element",
    NS_HTMLOPTIONELEMENT_CID,
    NS_HTMLOPTIONELEMENT_CONTRACTID,
    CreateHTMLOptionElement,
    RegisterHTMLOptionElement,
    UnregisterHTMLOptionElement },

#ifdef MOZ_ENABLE_CANVAS
  { "Canvas 2D Rendering Context",
    NS_CANVASRENDERINGCONTEXT2D_CID,
    "@mozilla.org/content/canvas-rendering-context;1?id=2d",
    CreateCanvasRenderingContext2D },
#endif

  { "XML document encoder",
    NS_TEXT_ENCODER_CID,
    NS_DOC_ENCODER_CONTRACTID_BASE "text/xml",
    CreateTextEncoder },

  { "XML document encoder",
    NS_TEXT_ENCODER_CID,
    NS_DOC_ENCODER_CONTRACTID_BASE "application/xml",
    CreateTextEncoder },

  { "XML document encoder",
    NS_TEXT_ENCODER_CID,
    NS_DOC_ENCODER_CONTRACTID_BASE "application/xhtml+xml",
    CreateTextEncoder },

#ifdef MOZ_SVG
  { "SVG document encoder",
    NS_TEXT_ENCODER_CID,
    NS_DOC_ENCODER_CONTRACTID_BASE "image/svg+xml",
    CreateTextEncoder },
#endif

  { "HTML document encoder",
    NS_TEXT_ENCODER_CID,
    NS_DOC_ENCODER_CONTRACTID_BASE "text/html",
    CreateTextEncoder },

  { "Plaintext document encoder",
    NS_TEXT_ENCODER_CID,
    NS_DOC_ENCODER_CONTRACTID_BASE "text/plain",
    CreateTextEncoder },

  { "HTML copy encoder",
    NS_HTMLCOPY_TEXT_ENCODER_CID,
    NS_HTMLCOPY_ENCODER_CONTRACTID,
    CreateHTMLCopyTextEncoder },

  { "XML content serializer",
    NS_XMLCONTENTSERIALIZER_CID,
    NS_CONTENTSERIALIZER_CONTRACTID_PREFIX "text/xml",
    CreateXMLContentSerializer },

  { "XML content serializer",
    NS_XMLCONTENTSERIALIZER_CID,
    NS_CONTENTSERIALIZER_CONTRACTID_PREFIX "application/xml",
    CreateXMLContentSerializer },

  { "XML content serializer",
    NS_XMLCONTENTSERIALIZER_CID,
    NS_CONTENTSERIALIZER_CONTRACTID_PREFIX "application/xhtml+xml",
    CreateXMLContentSerializer },

#ifdef MOZ_SVG
  { "SVG content serializer",
    NS_XMLCONTENTSERIALIZER_CID,
    NS_CONTENTSERIALIZER_CONTRACTID_PREFIX "image/svg+xml",
    CreateXMLContentSerializer },
#endif

  { "HTML content serializer",
    NS_HTMLCONTENTSERIALIZER_CID,
    NS_CONTENTSERIALIZER_CONTRACTID_PREFIX "text/html",
    CreateHTMLContentSerializer },

  { "XUL content serializer",
    NS_XMLCONTENTSERIALIZER_CID,
    NS_CONTENTSERIALIZER_CONTRACTID_PREFIX "application/vnd.mozilla.xul+xml",
    CreateXMLContentSerializer },

  { "plaintext content serializer",
    NS_PLAINTEXTSERIALIZER_CID,
    NS_CONTENTSERIALIZER_CONTRACTID_PREFIX "text/plain",
    CreatePlainTextSerializer },

  { "plaintext sink",
    NS_PLAINTEXTSERIALIZER_CID,
    NS_PLAINTEXTSINK_CONTRACTID,
    CreatePlainTextSerializer },

  { "html fragment sink",
    NS_HTMLFRAGMENTSINK_CID,
    NS_HTMLFRAGMENTSINK_CONTRACTID,
    CreateHTMLFragmentSink },

  { "html fragment sink 2",
    NS_HTMLFRAGMENTSINK2_CID,
    NS_HTMLFRAGMENTSINK2_CONTRACTID,
    CreateHTMLFragmentSink2 },

  { "HTML sanitizing content serializer",
    MOZ_SANITIZINGHTMLSERIALIZER_CID,
    MOZ_SANITIZINGHTMLSERIALIZER_CONTRACTID,
    CreateSanitizingHTMLSerializer },

  { "xml fragment sink",
    NS_XMLFRAGMENTSINK_CID,
    NS_XMLFRAGMENTSINK_CONTRACTID,
    CreateXMLFragmentSink },

  { "xml fragment sink 2",
    NS_XMLFRAGMENTSINK2_CID,
    NS_XMLFRAGMENTSINK2_CONTRACTID,
    CreateXMLFragmentSink2 },

  { "XBL Service",
    NS_XBLSERVICE_CID,
    "@mozilla.org/xbl;1",
    CreateXBLService },

  { "Content policy service",
    NS_CONTENTPOLICY_CID,
    NS_CONTENTPOLICY_CONTRACTID,
    CreateContentPolicy },

  { "Data document content policy",
    NS_DATADOCUMENTCONTENTPOLICY_CID,
    NS_DATADOCUMENTCONTENTPOLICY_CONTRACTID,
    nsDataDocumentContentPolicyConstructor,
    RegisterDataDocumentContentPolicy,
    UnregisterDataDocumentContentPolicy },

  { "DOM CSS Computed Style Declaration",
    NS_COMPUTEDDOMSTYLE_CID,
    "@mozilla.org/DOM/Level2/CSS/computedStyleDeclaration;1",
    CreateComputedDOMStyle },

  { "XUL Controllers",
    NS_XULCONTROLLERS_CID,
    "@mozilla.org/xul/xul-controllers;1",
    NS_NewXULControllers },

#ifdef MOZ_XUL
  { "XUL Sort Service",
    NS_XULSORTSERVICE_CID,
    "@mozilla.org/xul/xul-sort-service;1",
    CreateXULSortService },

  { "XUL Template Builder",
    NS_XULTEMPLATEBUILDER_CID,
    "@mozilla.org/xul/xul-template-builder;1",
    NS_NewXULContentBuilder },

  { "XUL Tree Builder",
    NS_XULTREEBUILDER_CID,
    "@mozilla.org/xul/xul-tree-builder;1",
    NS_NewXULTreeBuilder },

  { "XUL Document",
    NS_XULDOCUMENT_CID,
    "@mozilla.org/xul/xul-document;1",
    CreateXULDocument },

  { "XUL PopupListener",
    NS_XULPOPUPLISTENER_CID,
    "@mozilla.org/xul/xul-popup-listener;1",
    CreateXULPopupListener },

  { "XUL Prototype Cache",
    NS_XULPROTOTYPECACHE_CID,
    "@mozilla.org/xul/xul-prototype-cache;1",
    NS_NewXULPrototypeCache },

  { NS_XULPROTOTYPEDOCUMENT_CLASSNAME,
    NS_XULPROTOTYPEDOCUMENT_CID,
    nsnull,
    NS_NewXULPrototypeDocument },
#endif

#ifdef MOZ_XTF
  { "XTF Service",
    NS_XTFSERVICE_CID,
    NS_XTFSERVICE_CONTRACTID,
    CreateXTFService },

  { "XML Content Builder",
    NS_XMLCONTENTBUILDER_CID,
    NS_XMLCONTENTBUILDER_CONTRACTID,
    CreateXMLContentBuilder },
#endif

#ifdef MOZ_SVG
  { "SVG Rect",
    NS_SVGRECT_CID,
    NS_SVGRECT_CONTRACTID,
    CreateSVGRect },
#endif

  { "Content HTTP Startup Listener",
    NS_CONTENTHTTPSTARTUP_CID,
    NS_CONTENTHTTPSTARTUP_CONTRACTID,
    nsContentHTTPStartupConstructor,
    nsContentHTTPStartup::RegisterHTTPStartup,
    nsContentHTTPStartup::UnregisterHTTPStartup },

  { "Document Loader Factory",
    NS_CONTENT_DOCUMENT_LOADER_FACTORY_CID,
    "@mozilla.org/content/document-loader-factory;1",
    CreateContentDLF,
    nsContentDLF::RegisterDocumentFactories,
    nsContentDLF::UnregisterDocumentFactories },

  { "Wyciwyg Handler",
    NS_WYCIWYGPROTOCOLHANDLER_CID,
    NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "wyciwyg",
    nsWyciwygProtocolHandlerConstructor },

  { "Content Area DragDrop",
    NS_CONTENTAREADRAGDROP_CID,
    NS_CONTENTAREADRAGDROP_CONTRACTID,
    nsContentAreaDragDropConstructor },

  { "SyncLoad DOM Service",
    NS_SYNCLOADDOMSERVICE_CID,
    NS_SYNCLOADDOMSERVICE_CONTRACTID,
    CreateSyncLoadDOMService },

  // DOM objects
  { "Script Object Factory",
    NS_DOM_SCRIPT_OBJECT_FACTORY_CID,
    nsnull,
    nsDOMScriptObjectFactoryConstructor
  },
  { "Base DOM Exception",
    NS_BASE_DOM_EXCEPTION_CID,
    nsnull,
    nsBaseDOMExceptionConstructor
  },
  { "JavaScript Protocol Handler",
    NS_JSPROTOCOLHANDLER_CID,
    NS_JSPROTOCOLHANDLER_CONTRACTID,
    nsJSProtocolHandler::Create },
  { "Window Command Table",
    NS_WINDOWCOMMANDTABLE_CID,
    "",
    CreateWindowCommandTableConstructor
  },
  { "Window Command Controller",
    NS_WINDOWCONTROLLER_CID,
    NS_WINDOWCONTROLLER_CONTRACTID,
    CreateWindowControllerWithSingletonCommandTable
  },

  // view stuff
  { "View Manager", NS_VIEW_MANAGER_CID, "@mozilla.org/view-manager;1",
    nsViewManagerConstructor },

  { "Plugin Document Loader Factory",
    NS_PLUGINDOCLOADERFACTORY_CID,
    "@mozilla.org/content/plugin/document-loader-factory;1",
    CreateContentDLF },

  { "Plugin Document",
    NS_PLUGINDOCUMENT_CID,
    nsnull,
    CreatePluginDocument },

  { "Style sheet service",
    NS_STYLESHEETSERVICE_CID,
    NS_STYLESHEETSERVICE_CONTRACTID,
    nsStyleSheetServiceConstructor },

  // transformiix

  { "XSLTProcessor",
    TRANSFORMIIX_XSLT_PROCESSOR_CID,
    TRANSFORMIIX_XSLT_PROCESSOR_CONTRACTID,
    txMozillaXSLTProcessorConstructor,
    RegisterTransformiix },

  { "XPathEvaluator",
    TRANSFORMIIX_XPATH_EVALUATOR_CID,
    NS_XPATH_EVALUATOR_CONTRACTID,
    nsXPathEvaluatorConstructor },

  { "XFormsXPathEvaluator",
    TRANSFORMIIX_XFORMS_XPATH_EVALUATOR_CID,
    NS_XFORMS_XPATH_EVALUATOR_CONTRACTID,
    nsXFormsXPathEvaluatorConstructor },

  { "Transformiix DOMCI Extender",
    TRANSFORMIIX_DOMCI_EXTENSION_CID,
    TRANSFORMIIX_DOMCI_EXTENSION_CONTRACTID,
    NS_DOMCI_EXTENSION_CONSTRUCTOR(Transformiix) },

  { "XPath1 XPointer Scheme Processor",
    TRANSFORMIIX_XPATH1_SCHEME_CID,
    NS_XPOINTER_SCHEME_PROCESSOR_BASE "xpath1",
    nsXPath1SchemeProcessorConstructor }
};

NS_IMPL_NSGETMODULE_WITH_CTOR(nsLayoutModule, gComponents, Initialize)
