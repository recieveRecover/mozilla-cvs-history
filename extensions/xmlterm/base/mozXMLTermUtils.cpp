/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "MPL"); you may not use this file
 * except in compliance with the MPL. You may obtain a copy of
 * the MPL at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the MPL is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the MPL for the specific language governing
 * rights and limitations under the MPL.
 * 
 * The Original Code is XMLterm.
 * 
 * The Initial Developer of the Original Code is Ramalingam Saravanan.
 * Portions created by Ramalingam Saravanan <svn@xmlterm.org> are
 * Copyright (C) 1999 Ramalingam Saravanan. All Rights Reserved.
 * 
 * Contributor(s):
 */

// mozXMLTermUtils.cpp: XMLTerm utilities implementation

#include "nscore.h"
#include "nspr.h"
#include "prlog.h"

#include "nsCOMPtr.h"
#include "nsString.h"

#include "nsIContentViewer.h"
#include "nsIDocumentViewer.h"
#include "nsIPresContext.h"
#include "nsIPresShell.h"
#include "nsIDeviceContext.h"

#include "nsIPrincipal.h"
#include "nsIScriptContext.h"
#include "nsIScriptContextOwner.h"
#include "nsIScriptGlobalObject.h"
#include "nsIDOMWindowCollection.h"
#include "nsIDocument.h"

#include "mozXMLT.h"
#include "mozXMLTermUtils.h"

/////////////////////////////////////////////////////////////////////////

/** Gets presentation context for web shell
 * @param aWebShell web shell
 * @param aPresContext returned presentation context
 * @return NS_OK on success
 */
NS_EXPORT nsresult
mozXMLTermUtils::GetWebShellPresContext(nsIWebShell* aWebShell,
                                        nsIPresContext** aPresContext)
{
  nsresult result;

  XMLT_LOG(mozXMLTermUtils::GetWebShellPresContext,30,("\n"));

  if (!aPresContext)
    return NS_ERROR_FAILURE;

  *aPresContext = nsnull;

  nsCOMPtr<nsIContentViewer> contViewer;
  result = aWebShell->GetContentViewer(getter_AddRefs(contViewer));
  if (NS_FAILED(result) || !contViewer)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDocumentViewer> docViewer;
  result = contViewer->QueryInterface(NS_GET_IID(nsIDocumentViewer),
                                      (void**)getter_AddRefs(docViewer));
  if (NS_FAILED(result) || !docViewer)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIPresContext> presContext;
  result = docViewer->GetPresContext(*getter_AddRefs(presContext));
  if (NS_FAILED(result) || !presContext)
    return NS_ERROR_FAILURE;

  *aPresContext = presContext.get();
  NS_ADDREF(*aPresContext);

  return NS_OK;
}


/** Gets DOM document for web shell
 * @param aWebShell web shell
 * @param aDOMDocument returned DOM document
 * @return NS_OK on success
 */
NS_EXPORT nsresult
mozXMLTermUtils::GetWebShellDOMDocument(nsIWebShell* aWebShell,
                                        nsIDOMDocument** aDOMDocument)
{
  nsresult result;

  XMLT_LOG(mozXMLTermUtils::GetWebShellDOMDocument,30,("\n"));

  if (!aDOMDocument)
    return NS_ERROR_FAILURE;

  *aDOMDocument = nsnull;

  nsCOMPtr<nsIContentViewer> contViewer;
  result = aWebShell->GetContentViewer(getter_AddRefs(contViewer));
  if (NS_FAILED(result) || !contViewer)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDocumentViewer> docViewer;
  result = contViewer->QueryInterface(NS_GET_IID(nsIDocumentViewer),
                                      (void**)getter_AddRefs(docViewer));
  if (NS_FAILED(result) || !docViewer)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDocument> document;
  result = docViewer->GetDocument(*getter_AddRefs(document));

  if (NS_FAILED(result) || !document)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMDocument> domDocument = do_QueryInterface(document);
  if (!domDocument)
    return NS_ERROR_FAILURE;

  *aDOMDocument = domDocument.get();
  NS_ADDREF(*aDOMDocument);

  return NS_OK;
}


/** Gets DOM window for web shell
 * @param aWebShell web shell
 * @param aDOMWindow returned DOM window (frame)
 * @return NS_OK on success
 */
NS_EXPORT nsresult
mozXMLTermUtils::ConvertWebShellToDOMWindow(nsIWebShell* aWebShell,
                                    nsIDOMWindow** aDOMWindow)
{
  nsresult result;

  XMLT_LOG(mozXMLTermUtils::ConvertWebShellToDOMWindow,30,("\n"));

  if (!aDOMWindow)
    return NS_ERROR_FAILURE;

  *aDOMWindow = nsnull;

  nsCOMPtr<nsIScriptContextOwner> scriptContextOwner =
                                              do_QueryInterface(aWebShell);
  if (!scriptContextOwner)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIScriptGlobalObject> scriptGlobalObject;
  result = scriptContextOwner->GetScriptGlobalObject(getter_AddRefs(scriptGlobalObject));

  if (NS_FAILED(result) || !scriptGlobalObject)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMWindow> domWindow = do_QueryInterface(scriptGlobalObject);
  if (!domWindow)
    return NS_ERROR_FAILURE;

  *aDOMWindow = domWindow.get();
  NS_ADDREF(*aDOMWindow);

  return NS_OK;
}


/** Gets web shell for DOM window
 * @param aDOMWindow DOM window (frame)
 * @param aWebShell returned web shell
 * @return NS_OK on success
 */
NS_EXPORT nsresult
mozXMLTermUtils::ConvertDOMWindowToWebShell(nsIDOMWindow* aDOMWindow,
                                             nsIWebShell** aWebShell)
{
  XMLT_LOG(mozXMLTermUtils::ConvertDOMWindowToWebShell,30,("\n"));

  nsCOMPtr<nsIScriptGlobalObject> globalObject = do_QueryInterface(aDOMWindow);
  if (!globalObject)
    return NS_ERROR_FAILURE;

  globalObject->GetWebShell(aWebShell);

  if (!*aWebShell)
    return NS_ERROR_FAILURE;

  return NS_OK;
}


/** Locates named inner DOM window (frame) inside outer DOM window
 * @param outerDOMWindow outer DOM window (frame)
 * @param innerFrameName name of inner frame to be returned
 * @param innerDOMWindow returned inner DOM window (frame)
 * @return NS_OK on success
 */
NS_EXPORT nsresult
mozXMLTermUtils::GetInnerDOMWindow(nsIDOMWindow* outerDOMWindow,
                                    const nsString& innerFrameName,
                                    nsIDOMWindow** innerDOMWindow)
{
  nsresult result;

  XMLT_LOG(mozXMLTermUtils::GetInnerDOMWindow,30,("\n"));

  nsCOMPtr<nsIDOMWindowCollection> innerDOMWindowList;
  result = outerDOMWindow->GetFrames(getter_AddRefs(innerDOMWindowList));
  if (NS_FAILED(result) || !innerDOMWindowList)
    return NS_ERROR_FAILURE;

  PRUint32 frameCount = 0;
  result = innerDOMWindowList->GetLength(&frameCount);
  XMLT_LOG(mozXMLTermUtils::GetInnerDOMWindow,31,("frameCount=%d\n",
                                                   frameCount));

  result = innerDOMWindowList->NamedItem(innerFrameName, innerDOMWindow);
  if (NS_FAILED(result) || !*innerDOMWindow)
    return NS_ERROR_FAILURE;

  return NS_OK;
}


/** Gets the scrollable view for presentation context
 * @param aPresContext presentation context
 * @param aScrollableView returned scrollable view
 * @return NS_OK on success
 */
NS_EXPORT nsresult
mozXMLTermUtils::GetPresContextScrollableView(nsIPresContext* aPresContext,
                                         nsIScrollableView** aScrollableView)
{
  nsresult result;

  XMLT_LOG(mozXMLTermUtils::GetPresContextScrollableView,30,("\n"));

  if (!aScrollableView)
    return NS_ERROR_FAILURE;

  *aScrollableView = nsnull;

  nsCOMPtr<nsIPresShell> presShell;
  result = aPresContext->GetShell(getter_AddRefs(presShell));
  if (NS_FAILED(result) || !presShell)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIViewManager> viewManager;
  result = presShell->GetViewManager(getter_AddRefs(viewManager));
  if (NS_FAILED(result) || !viewManager)
    return NS_ERROR_FAILURE;

  return viewManager->GetRootScrollableView(aScrollableView);
}


/** Gets the device context for presentation context
 * @param aPresContext presentation context
 * @param aDeviceContext returned device context
 * @return NS_OK on success
 */
NS_EXPORT nsresult
mozXMLTermUtils::GetPresContextDeviceContext(nsIPresContext* aPresContext,
                                         nsIDeviceContext** aDeviceContext)
{
  nsresult result;

  XMLT_LOG(mozXMLTermUtils::GetPresContextScrollableView,30,("\n"));

  if (!aDeviceContext)
    return NS_ERROR_FAILURE;

  *aDeviceContext = nsnull;

  nsCOMPtr<nsIPresShell> presShell;
  result = aPresContext->GetShell(getter_AddRefs(presShell));
  if (NS_FAILED(result) || !presShell)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIViewManager> viewManager;
  result = presShell->GetViewManager(getter_AddRefs(viewManager));
  if (NS_FAILED(result) || !viewManager)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDeviceContext> deviceContext;
  result = viewManager->GetDeviceContext(*getter_AddRefs(deviceContext));
  if (NS_FAILED(result) || !deviceContext)
    return NS_ERROR_FAILURE;

  *aDeviceContext = deviceContext.get();
  NS_ADDREF(*aDeviceContext);

  return NS_OK;
}


/** Gets the script context for document
 * @param aDOMDocument document providing context
 * @param aScriptContext returned script context
 * @return NS_OK on success
 */
NS_EXPORT nsresult
mozXMLTermUtils::GetScriptContext(nsIDOMDocument* aDOMDocument,
                                  nsIScriptContext** aScriptContext)
{
  nsresult result;

  XMLT_LOG(mozXMLTermUtils::GetScriptContext,20,("\n"));

  nsCOMPtr<nsIDocument> doc ( do_QueryInterface(aDOMDocument) );
  if (!doc)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIScriptGlobalObject> scriptGlobalObject;
  result = doc->GetScriptGlobalObject(getter_AddRefs(scriptGlobalObject));

  if (NS_FAILED(result) || !scriptGlobalObject)
    return NS_ERROR_FAILURE;

  return scriptGlobalObject->GetContext(aScriptContext);
}


/** Executes script in specified document's context.
 * @param aDOMDocument document providing context for script execution
 * @param aScript string to be executed
 * @param aOutput output string produced by script execution
 * @return NS_OK if script was valid and got executed properly
 */
NS_EXPORT nsresult
mozXMLTermUtils::ExecuteScript(nsIDOMDocument* aDOMDocument,
                               const nsString& aScript,
                               nsString& aOutput)
{
  nsresult result;

  XMLT_LOG(mozXMLTermUtils::ExecuteScript,20,("\n"));

  // Get document principal
  nsCOMPtr<nsIDocument> doc = do_QueryInterface(aDOMDocument);
  if (!doc)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIPrincipal> docPrincipal=dont_AddRef(doc->GetDocumentPrincipal());

  // Get document script context
  nsCOMPtr<nsIScriptContext> scriptContext;
  result = GetScriptContext(aDOMDocument, getter_AddRefs(scriptContext));
  if (NS_FAILED(result) || !scriptContext)
    return NS_ERROR_FAILURE;

  // Execute script
  PRBool isUndefined = PR_FALSE;
  nsString outputString = "";
  const char* URL = "";
  const char* version = "";
  result = scriptContext-> EvaluateString(aScript, (void *) nsnull,
                                          docPrincipal, URL, 0, version,
                                          aOutput, &isUndefined);
  return result;
}


/** Returns a timestamp string containing the local time, if at least
 * deltaSec seconds have elapsed since the last timestamp. Otherwise,
 * a null string is returned.
 * @param deltaSec minimum elapsed seconds since last timestamp (>=0)
 * @param lastTime in/out parameter containing time of last timestamp
 * @param aTimeStamp  returned timestamp string
 * @return NS_OK on success
 */
NS_IMETHODIMP mozXMLTermUtils::TimeStamp(PRInt32 deltaSec, PRTime& lastTime,
                                     nsString& aTimeStamp)
{
  static const PRInt32 DATE_LEN = 19;
  PRTime deltaTime ;
  char dateStr[DATE_LEN+1];
  PRTime curTime, difTime;

  curTime = PR_Now();
  LL_SUB(difTime, curTime, lastTime);

  LL_I2L(deltaTime, deltaSec*1000000);
  if (LL_CMP(difTime, <, deltaTime)) {
    // Not enough time has elapsed for a new time stamp
    aTimeStamp = "";
    return NS_OK;
  }

  lastTime = curTime;

  // Current local time
  PRExplodedTime localTime;
  PR_ExplodeTime(curTime, PR_LocalTimeParameters, &localTime);

  PRInt32 nWritten = PR_snprintf(dateStr, DATE_LEN+1,
                     "%02d:%02d:%02d %02d/%02d/%04d",
                   localTime.tm_hour, localTime.tm_min, localTime.tm_sec,
                   localTime.tm_mday, localTime.tm_month+1, localTime.tm_year);

  if (nWritten != DATE_LEN)
    return NS_ERROR_FAILURE;

  XMLT_LOG(mozXMLTermUtils::LocalTime,99,("localTime=%s\n", dateStr));

  aTimeStamp = dateStr;
  return NS_OK;
}


/** Returns a string containing a 11-digit random cookie based upon the
 *  current local time and the elapsed execution of the program.
 * @param aCookie  returned cookie string
 * @return NS_OK on success
 */
NS_IMETHODIMP mozXMLTermUtils::RandomCookie(nsString& aCookie)
{
  // Current local time
  PRExplodedTime localTime;
  PR_ExplodeTime(PR_Now(), PR_LocalTimeParameters, &localTime);

  PRInt32        randomNumberA = localTime.tm_sec*1000000+localTime.tm_usec;
  PRIntervalTime randomNumberB = PR_IntervalNow();

  XMLT_LOG(mozXMLTermUtils::RandomCookie,30,("ranA=0x%x, ranB=0x%x\n",
                                        randomNumberA, randomNumberB));
  PR_ASSERT(randomNumberA >= 0);
  PR_ASSERT(randomNumberB >= 0);

  static const char cookieDigits[17] = "0123456789abcdef";
  char cookie[12];
  int j;

  for (j=0; j<6; j++) {
    cookie[j] = cookieDigits[randomNumberA%16];
    randomNumberA = randomNumberA/16;
  }
  for (j=6; j<11; j++) {
    cookie[j] = cookieDigits[randomNumberB%16];
    randomNumberB = randomNumberB/16;
  }
  cookie[11] = '\0';

  aCookie = cookie;

  return NS_OK;
}
