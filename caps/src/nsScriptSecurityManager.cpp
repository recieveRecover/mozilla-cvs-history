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
 * Copyright (C) 1998-2000 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 * Norris Boyd
 * Mitch Stoltz
 * Steve Morse
 */
#include "nsScriptSecurityManager.h"
#include "nsIServiceManager.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsIURL.h"
#include "nsIJARURI.h"
#include "nspr.h"
#include "plstr.h"
#include "nsCOMPtr.h"
#include "nsJSPrincipals.h"
#include "nsSystemPrincipal.h"
#include "nsCodebasePrincipal.h"
#include "nsCertificatePrincipal.h"
#include "nsAggregatePrincipal.h"
#include "nsCRT.h"
#include "nsXPIDLString.h"
#include "nsIJSContextStack.h"
#include "nsDOMError.h"
#include "xpcexception.h"
#include "nsDOMCID.h"
#include "jsdbgapi.h"
#include "nsIXPConnect.h"
#include "nsIXPCSecurityManager.h"
#include "nsTextFormatter.h"
#include "nsIIOService.h"
#include "nsIStringBundle.h"
#include "nsNetUtil.h"
#include "nsDirectoryService.h"
#include "nsDirectoryServiceDefs.h"
#include "nsIFile.h"
#include "nsIZipReader.h"
#include "nsIJAR.h"
#include "nsIPluginInstance.h"
#include "nsIXPConnect.h"
#include "nsIScriptGlobalObject.h"
#include "nsIDOMWindowInternal.h"
#include "nscore.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIPrompt.h"
#include "nsIWindowWatcher.h"
#include "nsIConsoleService.h"
#include "nsISecurityCheckedComponent.h"
#include "nsIPref.h"

static NS_DEFINE_IID(kIIOServiceIID, NS_IIOSERVICE_IID);
static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);
static NS_DEFINE_IID(kIStringBundleServiceIID, NS_ISTRINGBUNDLESERVICE_IID);
static NS_DEFINE_IID(kStringBundleServiceCID, NS_STRINGBUNDLESERVICE_CID);
static NS_DEFINE_CID(kPrefServiceCID, NS_PREFSERVICE_CID);
static NS_DEFINE_CID(kCScriptNameSetRegistryCID, 
                     NS_SCRIPT_NAMESET_REGISTRY_CID);
static NS_DEFINE_CID(kZipReaderCID, NS_ZIPREADER_CID);

//-- All values of this enum except the first map to odd numbers so they can be
//   stored in a void* without being confused for a pointer value (which is always even).
enum {
    SCRIPT_SECURITY_UNDEFINED_ACCESS = 0,
    SCRIPT_SECURITY_CAPABILITY_ONLY = 1,
    SCRIPT_SECURITY_SAME_ORIGIN_ACCESS = 3,
    SCRIPT_SECURITY_ALL_ACCESS = 5,
    SCRIPT_SECURITY_NO_ACCESS = 7
};

enum {
    CLASS_POLICY_DEFAULT = 1,
    CLASS_POLICY_SITE
};

// Result of this function should not be freed.
static const PRUnichar *
JSValIDToString(JSContext *aJSContext, const jsval idval) {
    JSString *str = JS_ValueToString(aJSContext, idval);
    if(!str)
        return nsnull;
    return NS_REINTERPRET_CAST(PRUnichar*, JS_GetStringChars(str));
}

// Convinience method to get the current js context stack.
// Uses cached JSContextStack service instead of calling through
// to the service manager.
JSContext *
nsScriptSecurityManager::GetCurrentContextQuick()
{
    // Get JSContext from stack.
    nsresult rv;
    if (!mThreadJSContextStack) {
        mThreadJSContextStack = do_GetService("@mozilla.org/js/xpc/ContextStack;1", &rv);
    }
    if (!mThreadJSContextStack)
        return nsnull;
    JSContext *cx;
    if (NS_FAILED(mThreadJSContextStack->Peek(&cx)))
        return nsnull;
    return cx;
}

/////////////////////////////
// nsScriptSecurityManager //
/////////////////////////////

////////////////////////////////////
// Methods implementing ISupports //
////////////////////////////////////

NS_IMPL_THREADSAFE_ISUPPORTS3(nsScriptSecurityManager,
                   nsIScriptSecurityManager,
                   nsIXPCSecurityManager,
                   nsIObserver)

///////////////////////////////////////////////////
// Methods implementing nsIScriptSecurityManager //
///////////////////////////////////////////////////

///////////////// Security Checks /////////////////
NS_IMETHODIMP
nsScriptSecurityManager::CheckPropertyAccess(PRUint32 aAction,
                                             JSContext* aJSContext,
                                             JSObject* aJSObject,
                                             nsISupports* aObj,
                                             nsIClassInfo* aClassInfo,
                                             const char* aClassName,
                                             const char* aProperty,
                                             PRBool aSkipFrame)
{
    return CheckPropertyAccessImpl(aAction, nsnull, aJSContext, aJSObject, aObj,
                                   aClassInfo, nsnull, aClassName, aProperty, 
                                   aSkipFrame, nsnull);
}

nsresult
nsScriptSecurityManager::CheckPropertyAccessImpl(PRUint32 aAction,
                                                 nsIXPCNativeCallContext* aCallContext,
                                                 JSContext* aJSContext, JSObject* aJSObject,
                                                 nsISupports* aObj, nsIClassInfo* aClassInfo,
                                                 jsval aName, const char* aClassName, 
                                                 const char* aProperty, 
                                                 PRBool aSkipFrame, void** aPolicy)
{
    nsCOMPtr<nsIPrincipal> subjectPrincipal;
    JSStackFrame *notused;
    if (NS_FAILED(GetPrincipalAndFrame(aJSContext, aSkipFrame, 
                                       getter_AddRefs(subjectPrincipal), &notused)))
        return NS_ERROR_FAILURE;

    PRBool equals;
    if (!subjectPrincipal ||
        NS_SUCCEEDED(subjectPrincipal->Equals(mSystemPrincipal, &equals)) && equals) 
        // We have native code or the system principal: just allow access
        return NS_OK;

#ifdef DEBUG_mstoltz
    if (aProperty)
          printf("### CheckPropertyAccess(%s.%s, %i) ", aClassName, aProperty, aAction);
    else
    {
        nsXPIDLCString classDescription;
        if (aClassInfo)
            aClassInfo->GetClassDescription(getter_Copies(classDescription));
        if(!classDescription)
            classDescription = "UnknownClass";
        nsCAutoString propertyStr(classDescription);
        propertyStr += '.';
        propertyStr.AppendWithConversion((PRUnichar*)JSValIDToString(aJSContext, aName));

        char* property;
        property = propertyStr.ToNewCString();
        printf("### CanAccess(%s, %i) ", property, aAction);
        PR_FREEIF(property);
    }
#endif

    //-- Look up the policy for this property/method
    PRInt32 secLevel;
    nsCAutoString capability;
    if (aPolicy && *aPolicy)
    {
#ifdef DEBUG_mstoltz
        printf("Cached ");
#endif
        secLevel = (PRInt32)*aPolicy;
    }
    else
    {
        nsXPIDLCString className;
        nsCAutoString propertyName(aProperty);
        if (aClassName)
            className = aClassName;
        else
        //-- Get className and propertyName from aClassInfo and aName, repectively
        {
            if(aClassInfo)
                aClassInfo->GetClassDescription(getter_Copies(className));
            if (!className)
                className = "UnknownClass";
            propertyName.AssignWithConversion((PRUnichar*)JSValIDToString(aJSContext, aName));
        }

        secLevel = GetSecurityLevel(aJSContext, subjectPrincipal, aClassInfo, className,
                                    propertyName, aAction, capability, aPolicy);
    }

    nsresult rv;
    switch (secLevel)
    {
    case SCRIPT_SECURITY_ALL_ACCESS:
#ifdef DEBUG_mstoltz
        printf("Level: AllAccess ");
#endif
        rv = NS_OK;
        break;
    case SCRIPT_SECURITY_SAME_ORIGIN_ACCESS:
        {
#ifdef DEBUG_mstoltz
		    printf("Level: SameOrigin ");
#endif
            if(aJSObject)
            {
                nsCOMPtr<nsIPrincipal> objectPrincipal;
                if (NS_FAILED(GetObjectPrincipal(aJSContext, 
                                                 NS_REINTERPRET_CAST(JSObject*, aJSObject),
                                                 getter_AddRefs(objectPrincipal))))
                    return NS_ERROR_FAILURE;
                rv = CheckSameOrigin(aJSContext, subjectPrincipal, objectPrincipal,
                                     aAction == nsIXPCSecurityManager::ACCESS_SET_PROPERTY,
                                     aSkipFrame);
            }
            else
                rv = NS_ERROR_DOM_SECURITY_ERR;
            break;
        }
    case SCRIPT_SECURITY_CAPABILITY_ONLY:
        {
#ifdef DEBUG_mstoltz
		    printf("Level: Capability ");
#endif
            PRBool capabilityEnabled = PR_FALSE;
            rv = IsCapabilityEnabled(capability, &capabilityEnabled);
            if (NS_FAILED(rv) || !capabilityEnabled)
                rv = NS_ERROR_DOM_SECURITY_ERR;
            else
                rv = NS_OK;
            break;
        }
    default:
        // Default is no access
#ifdef DEBUG_mstoltz
        printf("Level: NoAccess (%i)",secLevel);
#endif
        rv = NS_ERROR_DOM_SECURITY_ERR;
    }

    if NS_SUCCEEDED(rv)
    {
#ifdef DEBUG_mstoltz
    printf(" GRANTED.\n");
#endif
        return rv;
    }

    //--See if the object advertises a non-default level of access
    //  using nsISecurityCheckedComponent
    nsCOMPtr<nsISecurityCheckedComponent> checkedComponent =
        do_QueryInterface(aObj);

    nsXPIDLCString objectSecurityLevel;
    if (checkedComponent)
    {
        nsCOMPtr<nsIXPConnectWrappedNative> wrapper;
        nsCOMPtr<nsIInterfaceInfo> interfaceInfo;
        const nsIID* objIID;
        rv = aCallContext->GetCalleeWrapper(getter_AddRefs(wrapper));
        if (NS_SUCCEEDED(rv))
            rv = wrapper->FindInterfaceWithMember(aName, getter_AddRefs(interfaceInfo));
        if (NS_SUCCEEDED(rv))
            rv = interfaceInfo->GetIIDShared(&objIID);
        if (NS_SUCCEEDED(rv))
        {
            switch (aAction)
            {
            case nsIXPCSecurityManager::ACCESS_GET_PROPERTY:
                checkedComponent->CanGetProperty(objIID,
                                                 JSValIDToString(aJSContext, aName),
                                                 getter_Copies(objectSecurityLevel));
                break;
            case nsIXPCSecurityManager::ACCESS_SET_PROPERTY:
                checkedComponent->CanSetProperty(objIID,
                                                 JSValIDToString(aJSContext, aName),
                                                 getter_Copies(objectSecurityLevel));
                break;
            case nsIXPCSecurityManager::ACCESS_CALL_METHOD:
                checkedComponent->CanCallMethod(objIID,
                                                JSValIDToString(aJSContext, aName),
                                                getter_Copies(objectSecurityLevel));
            }
        }
    }
    rv = CheckXPCPermissions(aJSContext, aObj, objectSecurityLevel, aSkipFrame,
                               "Permission denied to access property");
#ifdef DEBUG_mstoltz
    if(NS_SUCCEEDED(rv))
        printf("CheckXPCPerms GRANTED.\n");
    else
        printf("CheckXPCPerms DENIED.\n");
#endif
    return rv;
}

nsresult
nsScriptSecurityManager::CheckSameOrigin(JSContext *aCx, nsIPrincipal* aSubject, 
                                         nsIPrincipal* aObject, PRUint32 aAction,
                                         PRBool aSkipFrame)
{
    /*
    ** Get origin of subject and object and compare.
    */
    if (aSubject == aObject)
        return NS_OK;

    PRBool isSameOrigin = PR_FALSE;
    if (NS_FAILED(aSubject->Equals(aObject, &isSameOrigin)))
        return NS_ERROR_FAILURE;
    
    if (isSameOrigin)
        return NS_OK;

    // Allow access to about:blank
    nsCOMPtr<nsICodebasePrincipal> objectCodebase = do_QueryInterface(aObject);
    if (objectCodebase)
    {
        nsXPIDLCString origin;
        if (NS_FAILED(objectCodebase->GetOrigin(getter_Copies(origin))))
            return NS_ERROR_FAILURE;
        if (nsCRT::strcasecmp(origin, "about:blank") == 0)
            return NS_OK;
    }

    /*
    ** If we failed the origin tests it still might be the case that we
    ** are a signed script and have permissions to do this operation.
    ** Check for that here
    */
    PRBool capabilityEnabled = PR_FALSE;
    const char* cap = aAction == nsIXPCSecurityManager::ACCESS_SET_PROPERTY ?
                      "UniversalBrowserWrite" : "UniversalBrowserRead";
    if (NS_FAILED(IsCapabilityEnabledImpl(cap, aSkipFrame, &capabilityEnabled)))
        return NS_ERROR_FAILURE;
    if (capabilityEnabled)
        return NS_OK;

    /*
    ** Access tests failed, so now report error.
    */
    return NS_ERROR_DOM_PROP_ACCESS_DENIED;
}

PRBool 
nsScriptSecurityManager::IsDOMClass(nsIClassInfo* aClassInfo)
{
    if (!aClassInfo)
        return PR_FALSE;
    PRUint32 classFlags;
    nsresult rv = aClassInfo->GetFlags(&classFlags);
    return NS_SUCCEEDED(rv) && (classFlags | nsIClassInfo::DOM_OBJECT);
}

PRInt32 
nsScriptSecurityManager::GetSecurityLevel(JSContext* aJSContext,
                                          nsIPrincipal *principal,
                                          nsIClassInfo* aClassInfo,
                                          const char* aClassName,
                                          const char* aPropertyName,
                                          PRUint32 aAction,
                                          nsCString &capability,
                                          void** aPolicy)
{
    nsresult rv;
    PRInt32 secLevel = SCRIPT_SECURITY_NO_ACCESS;
    //-- See if we have a security policy for this class, otherwise use the default
    void* classPolicy = nsnull;
    if(mClassPolicies)
    {
        nsCStringKey classKey(aClassName);
        classPolicy = mClassPolicies->Get(&classKey);
    }
    if (classPolicy)
    {
        //-- Look up the security policy for this property
        nsCAutoString prefName;
        if (NS_FAILED(GetPrefName(principal, aClassName, aPropertyName,
                      classPolicy, prefName)))
            return SCRIPT_SECURITY_NO_ACCESS;
        char *secLevelString;
        rv = mSecurityPrefs->SecurityGetCharPref(prefName, &secLevelString);
        if (NS_FAILED(rv))
        {
            prefName += (aAction == nsIXPCSecurityManager::ACCESS_SET_PROPERTY ? ".set" : ".get");
            rv = mSecurityPrefs->SecurityGetCharPref(prefName, &secLevelString);
        }
        if (NS_SUCCEEDED(rv) && secLevelString) 
        {
            if (PL_strcmp(secLevelString, "sameOrigin") == 0)
                secLevel = SCRIPT_SECURITY_SAME_ORIGIN_ACCESS;
            else if (PL_strcmp(secLevelString, "allAccess") == 0)
                secLevel = SCRIPT_SECURITY_ALL_ACCESS;
            else if (PL_strcmp(secLevelString, "noAccess") == 0)
                secLevel = SCRIPT_SECURITY_NO_ACCESS;
            else 
            {
                // string should be the name of a capability
                capability = secLevelString;
                secLevelString = nsnull;
                secLevel = SCRIPT_SECURITY_CAPABILITY_ONLY;
            }
            if (secLevelString)
                PR_Free(secLevelString);
            return secLevel;
        }
    }
    //-- No policy for this property.
    //   Use the default policy: sameOrigin for DOM, noAccess for everything else
    if(IsDOMClass(aClassInfo))
        secLevel = SCRIPT_SECURITY_SAME_ORIGIN_ACCESS;
    if (!classPolicy && aPolicy)
        //-- If there's no stored policy for this property, 
        //   we can annotate the class's aPolicy field and avoid checking
        //   policy prefs next time.
        *aPolicy = (void*)secLevel;
    return secLevel;
    return SCRIPT_SECURITY_UNDEFINED_ACCESS;
}

struct nsDomainEntry
{
    nsDomainEntry(const char *anOrigin, const char *aPolicy,
                  int aPolicyLength) 
        : mNext(nsnull), mOrigin(anOrigin), mPolicy(aPolicy, aPolicyLength)
    { }
    PRBool Matches(const char *anOrigin)
    {
        int len = nsCRT::strlen(anOrigin);
        int thisLen = mOrigin.Length();
        if (len < thisLen)
            return PR_FALSE;
        if (mOrigin.RFindChar(':', PR_FALSE, thisLen-1, 1) != -1)
        //-- Policy applies to all URLs of this scheme, compare scheme only
            return mOrigin.EqualsWithConversion(anOrigin, PR_TRUE, thisLen);

        //-- Policy applies to a particular host; compare scheme://host.domain
        if (!mOrigin.Equals(anOrigin + (len - thisLen)))
            return PR_FALSE;
        if (len == thisLen)
            return PR_TRUE;
        char charBefore = anOrigin[len-thisLen-1];
        return (charBefore == '.' || charBefore == ':' || charBefore == '/');
    }
    nsDomainEntry *mNext;
    nsCString mOrigin;
    nsCString mPolicy;
};

nsresult
nsScriptSecurityManager::GetPrefName(nsIPrincipal* principal,
                                     const char* aClassName, const char* aPropertyName,
                                     void* aClassPolicy, nsCString &result)
{
    static const char *defaultStr = "default";
    result = "capability.policy.";
    if (aClassPolicy != (void*)CLASS_POLICY_SITE)
        //-- No per-site policy; use the policy named "default"
        result += defaultStr;
    else //-- Look up the name of the relevant per-site policy
    {
        PRBool equals = PR_TRUE;
        if (principal && NS_FAILED(principal->Equals(mSystemPrincipal, &equals)))
            return NS_ERROR_FAILURE;
        if (equals)
            result += defaultStr;
        else
        {
            nsCOMPtr<nsICodebasePrincipal> codebase = do_QueryInterface(principal);
            if (!codebase)
                return NS_ERROR_FAILURE;
            nsresult rv;
            nsXPIDLCString origin;
            if (NS_FAILED(rv = codebase->GetOrigin(getter_Copies(origin))))
                return rv;
            nsCString *policy = nsnull;
            if (mOriginToPolicyMap)
            {
                const char *s = origin;
                const char *nextToLastDot = nsnull;
                const char *lastDot = nsnull;
                const char *colon = nsnull;
                const char *p = s;
                while (*p)
                {
                    if (*p == '.')
                    {
                        nextToLastDot = lastDot;
                        lastDot = p;
                    }
                    if (!colon && *p == ':')
                        colon = p;
                    p++;
                }
                nsCStringKey key(nextToLastDot ? nextToLastDot+1 : s);
                nsDomainEntry *de = (nsDomainEntry *) mOriginToPolicyMap->Get(&key);
                if (!de)
                {
                    nsCAutoString scheme(s, colon-s+1);
                    nsCStringKey schemeKey(scheme);
                    de = (nsDomainEntry *) mOriginToPolicyMap->Get(&schemeKey);
                }
                while (de)
                {
                    if (de->Matches(s))
                    {
                        policy = &de->mPolicy;
                        break;
                    }
                    de = de->mNext;
                }
            }
            if (policy)
                result += *policy;
            else
                result += defaultStr;
        }
    }
    result += '.';
    result += aClassName;
    result += '.';
    result += aPropertyName;
    return NS_OK;
}

NS_IMETHODIMP
nsScriptSecurityManager::CheckLoadURIFromScript(JSContext *cx, nsIURI *aURI)
{
    // Get a context if necessary
    if (!cx)
    {
        cx = GetCurrentContextQuick();
        if (!cx)
            return NS_OK; // No JS context, so allow the load
    }

    // Get principal of currently executing script.
    nsCOMPtr<nsIPrincipal> principal;
    if (NS_FAILED(GetCallingPrincipal(cx, getter_AddRefs(principal))))
        return NS_ERROR_FAILURE;

    // Native code can load all URIs.
    if (!principal) 
        return NS_OK;

    // The system principal can load all URIs.
    PRBool equals = PR_FALSE;
    if (NS_FAILED(principal->Equals(mSystemPrincipal, &equals)))
        return NS_ERROR_FAILURE;
    if (equals)
        return NS_OK;

    // Otherwise, principal should have a codebase that we can use to
    // do the remaining tests.
    nsCOMPtr<nsICodebasePrincipal> codebase = do_QueryInterface(principal);
    if (!codebase) 
        return NS_ERROR_FAILURE;
    nsCOMPtr<nsIURI> uri;
    if (NS_FAILED(codebase->GetURI(getter_AddRefs(uri)))) 
        return NS_ERROR_FAILURE;
    if (NS_SUCCEEDED(CheckLoadURI(uri, aURI, nsIScriptSecurityManager::STANDARD )))
        return NS_OK;

    // See if we're attempting to load a file: URI. If so, let a 
    // UniversalFileRead capability trump the above check.
    PRBool isFile = PR_FALSE;
    PRBool isRes = PR_FALSE;
    if (NS_FAILED(aURI->SchemeIs("file", &isFile)) || 
        NS_FAILED(aURI->SchemeIs("resource", &isRes)))
        return NS_ERROR_FAILURE;
    if (isFile || isRes)
    {
        PRBool enabled;
        if (NS_FAILED(IsCapabilityEnabled("UniversalFileRead", &enabled)))
            return NS_ERROR_FAILURE;
        if (enabled)
            return NS_OK;
    }

    // Report error.
    nsXPIDLCString spec;
    if (NS_FAILED(aURI->GetSpec(getter_Copies(spec))))
        return NS_ERROR_FAILURE;
	JS_ReportError(cx, "illegal URL method '%s'", (const char *)spec);
    return NS_ERROR_DOM_BAD_URI;
}

NS_IMETHODIMP
nsScriptSecurityManager::CheckLoadURI(nsIURI *aSourceURI, nsIURI *aTargetURI,
                                      PRUint32 aFlags)
{
    nsCOMPtr<nsIJARURI> jarURI;
    nsCOMPtr<nsIURI> sourceUri(aSourceURI);
    while((jarURI = do_QueryInterface(sourceUri)))
        jarURI->GetJARFile(getter_AddRefs(sourceUri));
    if (!sourceUri) return NS_ERROR_FAILURE;

    nsXPIDLCString sourceScheme;
    if (NS_FAILED(sourceUri->GetScheme(getter_Copies(sourceScheme))))
        return NS_ERROR_FAILURE;

    // Some loads are not allowed from mail/news messages
    if ((aFlags & nsIScriptSecurityManager::DISALLOW_FROM_MAIL) && 
        (nsCRT::strcasecmp(sourceScheme, "mailbox")  == 0 ||
         nsCRT::strcasecmp(sourceScheme, "imap")     == 0 ||
         nsCRT::strcasecmp(sourceScheme, "news")     == 0))
    {
        return NS_ERROR_DOM_BAD_URI;
    }

    nsCOMPtr<nsIURI> targetUri(aTargetURI);
    while((jarURI = do_QueryInterface(targetUri)))
        jarURI->GetJARFile(getter_AddRefs(targetUri));
    if (!targetUri) return NS_ERROR_FAILURE;

    nsXPIDLCString targetScheme;
    if (NS_FAILED(targetUri->GetScheme(getter_Copies(targetScheme))))
        return NS_ERROR_FAILURE;
    
    if (nsCRT::strcasecmp(targetScheme, sourceScheme) == 0)
    {
        // every scheme can access another URI from the same scheme
        return NS_OK;
    }

    enum Action { AllowProtocol, DenyProtocol, PrefControlled, ChromeProtocol, AboutProtocol };
    static const struct { 
        const char *name;
        Action action;
    } protocolList[] = {
        //-- Keep the most commonly used protocols at the top of the list
        //   to increase performance
        { "http",            AllowProtocol  },
        { "file",            PrefControlled },
        { "https",           AllowProtocol  },
        { "chrome",          ChromeProtocol },
        { "mailbox",         DenyProtocol   },
        { "pop",             AllowProtocol  },
        { "imap",            DenyProtocol   },
        { "pop3",            DenyProtocol   },
        { "news",            AllowProtocol  },
        { "javascript",      AllowProtocol  },
        { "ftp",             AllowProtocol  },
        { "about",           AboutProtocol  },
        { "mailto",          AllowProtocol  },
        { "aim",             AllowProtocol  },
        { "data",            AllowProtocol  },
        { "keyword",         DenyProtocol   },
        { "resource",        DenyProtocol   },
        { "gopher",          AllowProtocol  },
        { "datetime",        DenyProtocol   },
        { "finger",          AllowProtocol  },
        { "res",             DenyProtocol   }
    };

    nsXPIDLCString targetSpec;
    for (unsigned i=0; i < sizeof(protocolList)/sizeof(protocolList[0]); i++) {
        if (nsCRT::strcasecmp(targetScheme, protocolList[i].name) == 0) {
            PRBool doCheck = PR_FALSE;
            switch (protocolList[i].action) {
            case AllowProtocol:
                // everyone can access these schemes.
                return NS_OK;
            case PrefControlled:
                // Allow access if pref is false
                mPrefs->GetBoolPref("security.checkloaduri", &doCheck);
                return doCheck ? ReportErrorToConsole(aTargetURI) : NS_OK;
            case ChromeProtocol:
                return (aFlags & nsIScriptSecurityManager::ALLOW_CHROME) ?
                    NS_OK : ReportErrorToConsole(aTargetURI);
            case AboutProtocol:
                // Allow loading about:blank, otherwise deny
                if(NS_FAILED(targetUri->GetSpec(getter_Copies(targetSpec))))
                    return NS_ERROR_FAILURE;
                return (PL_strcmp(targetSpec, "about:blank") == 0) || 
                       (PL_strcmp(targetSpec, "about:") == 0) ||
                       (PL_strcmp(targetSpec, "about:mozilla") == 0) ?
                    NS_OK : ReportErrorToConsole(aTargetURI);
            case DenyProtocol:
                // Deny access
                return ReportErrorToConsole(aTargetURI);
            }
        }
    }

    // If we reach here, we have an unknown protocol. Warn, but allow.
    // This is risky from a security standpoint, but allows flexibility
    // in installing new protocol handlers after initial ship.
    NS_WARN_IF_FALSE(PR_FALSE, "unknown protocol in nsScriptSecurityManager::CheckLoadURI");

    return NS_OK;
}

nsresult 
nsScriptSecurityManager::ReportErrorToConsole(nsIURI* aTarget)
{
    nsXPIDLCString spec;
    nsresult rv = aTarget->GetSpec(getter_Copies(spec));
    if (NS_FAILED(rv)) return rv;

    nsAutoString msg;
    msg.AssignWithConversion("The link to ");
    msg.AppendWithConversion(spec);
    msg.AppendWithConversion(" was blocked by the security manager.\nRemote content may not link to local content.");
    // Report error in JS console
    nsCOMPtr<nsIConsoleService> console(do_GetService("@mozilla.org/consoleservice;1"));
    if (console)
    {
      PRUnichar* messageUni = msg.ToNewUnicode();
      if (!messageUni)
          return NS_ERROR_FAILURE;
      console->LogStringMessage(messageUni);
      nsMemory::Free(messageUni);
    }
#ifndef DEBUG
    else // If JS console reporting failed, print to stderr.
#endif
    {
      char* messageCstr = msg.ToNewCString();
      if (!messageCstr)
          return NS_ERROR_FAILURE;
      fprintf(stderr, "%s\n", messageCstr);
      PR_Free(messageCstr);
    }
    //-- Always returns an error
    return NS_ERROR_DOM_BAD_URI;
}


NS_IMETHODIMP
nsScriptSecurityManager::CheckLoadURIStr(const char* aSourceURIStr, const char* aTargetURIStr,
                                         PRUint32 aFlags)
{
    nsCOMPtr<nsIURI> source;
    nsresult rv = NS_NewURI(getter_AddRefs(source), aSourceURIStr, nsnull);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIURI> target;
    rv = NS_NewURI(getter_AddRefs(target), aTargetURIStr, nsnull);
    NS_ENSURE_SUCCESS(rv, rv);
    return CheckLoadURI(source, target, aFlags);
}

NS_IMETHODIMP
nsScriptSecurityManager::CheckFunctionAccess(JSContext *aCx, void *aFunObj, 
                                             void *aTargetObj)
{
    //-- This check is called for event handlers
    nsCOMPtr<nsIPrincipal> subject;
    nsresult rv = GetFunctionObjectPrincipal(aCx, (JSObject *)aFunObj, 
                                             getter_AddRefs(subject));
    if (NS_FAILED(rv))
        return rv;

    PRBool isSystem;
    if (NS_SUCCEEDED(subject->Equals(mSystemPrincipal, &isSystem)) && isSystem) 
        // This is the system principal: just allow access
        return NS_OK;

    // Check if the principal the function was compiled under is
    // allowed to execute scripts.
    if (!subject)
      return NS_ERROR_DOM_SECURITY_ERR;

    PRBool result;
    rv = CanExecuteScripts(aCx, subject, &result);
    if (NS_FAILED(rv))
      return rv;
    
    if (!result)
      return NS_ERROR_DOM_SECURITY_ERR;

    /*
    ** Get origin of subject and object and compare.
    */
    JSObject* obj = (JSObject*)aTargetObj;
    nsCOMPtr<nsIPrincipal> object;
    if (NS_FAILED(GetObjectPrincipal(aCx, obj, getter_AddRefs(object))))
        return NS_ERROR_FAILURE;
    if (subject == object)
        return NS_OK;

    PRBool isSameOrigin = PR_FALSE;
    if (NS_FAILED(subject->Equals(object, &isSameOrigin)))
        return NS_ERROR_FAILURE;

    if (isSameOrigin)
        return NS_OK;

    // Allow access to about:blank
    nsCOMPtr<nsICodebasePrincipal> objectCodebase = do_QueryInterface(object);
    if (objectCodebase)
    {
        nsXPIDLCString origin;
        if (NS_FAILED(objectCodebase->GetOrigin(getter_Copies(origin))))
            return NS_ERROR_FAILURE;
        if (nsCRT::strcasecmp(origin, "about:blank") == 0)
            return NS_OK;
    }

    /*
    ** Access tests failed.  Fail silently without a JS exception.
    */
    return NS_ERROR_DOM_SECURITY_ERR;
}

nsresult
nsScriptSecurityManager::GetRootDocShell(JSContext *cx, nsIDocShell **result)
{
    nsresult rv;
    *result = nsnull;
    nsCOMPtr<nsIDocShell> docshell;
    nsCOMPtr<nsIScriptContext> scriptContext = (nsIScriptContext*)JS_GetContextPrivate(cx);
    if (!scriptContext) return NS_ERROR_FAILURE;
    nsCOMPtr<nsIScriptGlobalObject> globalObject(dont_AddRef(scriptContext->GetGlobalObject()));
    if (!globalObject)  return NS_ERROR_FAILURE;
    rv = globalObject->GetDocShell(getter_AddRefs(docshell));
    if (NS_FAILED(rv)) return rv;
    nsCOMPtr<nsIDocShellTreeItem> docshellTreeItem = do_QueryInterface(docshell, &rv);
    if (NS_FAILED(rv)) return rv;
    nsCOMPtr<nsIDocShellTreeItem> rootItem;
    rv = docshellTreeItem->GetRootTreeItem(getter_AddRefs(rootItem));
    if (NS_FAILED(rv)) return rv;

    return rootItem->QueryInterface(NS_GET_IID(nsIDocShell), (void**)result);
}

NS_IMETHODIMP
nsScriptSecurityManager::CanExecuteScripts(JSContext* cx,
                                           nsIPrincipal *principal,
                                           PRBool *result)
{
    if (principal == mSystemPrincipal)
    {
        // Even if JavaScript is disabled, we must still execute system scripts
        *result = PR_TRUE;
        return NS_OK;
    }

    //-- See if the current window allows JS execution
    nsCOMPtr<nsIDocShell> docshell;
    nsresult rv;
    rv = GetRootDocShell(cx, getter_AddRefs(docshell));
    if (NS_SUCCEEDED(rv))
    {
        rv = docshell->GetAllowJavascript(result);
        if (NS_FAILED(rv)) return rv;
        if (!*result)
        return NS_OK;
    }

    /*
         There used to be a per-domain policy here, but that functionality
         will hopefully be taken over by nsIContentPolicy. Meantime, it's gone.
    */

    if ((mIsJavaScriptEnabled != mIsMailJavaScriptEnabled) && docshell)
    {
        // Is this script running from mail?
               PRUint32 appType;
        rv = docshell->GetAppType(&appType);
        if (NS_FAILED(rv)) return rv;
        if (appType == nsIDocShell::APP_TYPE_MAIL)
        {
            *result = mIsMailJavaScriptEnabled;
            return NS_OK;
        }
    }
    *result = mIsJavaScriptEnabled;
    return NS_OK;
}

///////////////// Principals /////////////////////// 
NS_IMETHODIMP
nsScriptSecurityManager::GetSubjectPrincipal(nsIPrincipal **result)
{
    JSContext *cx = GetCurrentContextQuick();
    if (!cx)
    {
        *result = nsnull;
        return NS_OK;
    }
    return GetSubjectPrincipal(cx, result);
}

NS_IMETHODIMP
nsScriptSecurityManager::GetSystemPrincipal(nsIPrincipal **result)
{
    if (!mSystemPrincipal)
    {
        mSystemPrincipal = new nsSystemPrincipal();
        if (!mSystemPrincipal)
            return NS_ERROR_OUT_OF_MEMORY;
        NS_ADDREF(mSystemPrincipal);
    }
    *result = mSystemPrincipal;
    NS_ADDREF(*result);
    return NS_OK;
}

NS_IMETHODIMP
nsScriptSecurityManager::GetCertificatePrincipal(const char* aCertID,
                                                 nsIPrincipal **result)
{
    nsresult rv;
    //-- Create a certificate principal
    nsCertificatePrincipal *certificate = new nsCertificatePrincipal();
    if (!certificate)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(certificate);
    if (NS_FAILED(certificate->Init(aCertID)))
    {
        NS_RELEASE(certificate);
        return NS_ERROR_FAILURE;
    }
    nsCOMPtr<nsIPrincipal> principal = do_QueryInterface((nsBasePrincipal*)certificate, &rv);
    NS_RELEASE(certificate);
    if (NS_FAILED(rv)) return rv;

    if (mPrincipals)
    {
        // Check to see if we already have this principal.
        nsIPrincipalKey key(principal);
        nsCOMPtr<nsIPrincipal> fromTable = (nsIPrincipal *) mPrincipals->Get(&key);
        if (fromTable) 
            principal = fromTable;
    }

    //-- Bundle this certificate principal into an aggregate principal
    nsAggregatePrincipal* agg = new nsAggregatePrincipal();
    if (!agg) return NS_ERROR_OUT_OF_MEMORY;
    rv = agg->SetCertificate(principal);
    if (NS_FAILED(rv)) return rv;
    principal = do_QueryInterface((nsBasePrincipal*)agg, &rv);
    if (NS_FAILED(rv)) return rv;

    *result = principal;
    NS_ADDREF(*result);
    return NS_OK;
}

nsresult
nsScriptSecurityManager::CreateCodebasePrincipal(nsIURI* aURI, nsIPrincipal **result)
{
	nsresult rv = NS_OK;
    nsCodebasePrincipal *codebase = new nsCodebasePrincipal();
    if (!codebase)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(codebase);
    if (NS_FAILED(codebase->Init(aURI)))
    {
        NS_RELEASE(codebase);
        return NS_ERROR_FAILURE;
    }
    rv = CallQueryInterface((nsBasePrincipal*)codebase, result);
    NS_RELEASE(codebase);
    return rv;
}

NS_IMETHODIMP
nsScriptSecurityManager::GetCodebasePrincipal(nsIURI *aURI,
                                              nsIPrincipal **result)
{
	nsresult rv;
	nsCOMPtr<nsIPrincipal> principal;
	rv = CreateCodebasePrincipal(aURI, getter_AddRefs(principal));
	if (NS_FAILED(rv)) return rv;

    if (mPrincipals)
    {
        //-- Check to see if we already have this principal.
        nsIPrincipalKey key(principal);
        nsCOMPtr<nsIPrincipal> fromTable = (nsIPrincipal *) mPrincipals->Get(&key);
        if (fromTable) 
            principal = fromTable;
		else //-- Check to see if we have a more general principal
		{
			nsCOMPtr<nsICodebasePrincipal> codebasePrin = do_QueryInterface(principal);
			nsXPIDLCString originUrl;
			rv = codebasePrin->GetOrigin(getter_Copies(originUrl));
			if (NS_FAILED(rv)) return rv;
			nsCOMPtr<nsIURI> newURI;
			rv = NS_NewURI(getter_AddRefs(newURI), originUrl, nsnull);
			if (NS_FAILED(rv)) return rv;
			nsCOMPtr<nsIPrincipal> principal2;
			rv = CreateCodebasePrincipal(newURI, getter_AddRefs(principal2));
			if (NS_FAILED(rv)) return rv;
			 nsIPrincipalKey key2(principal2);
				fromTable = (nsIPrincipal *) mPrincipals->Get(&key2);
			if (fromTable) 
				principal = fromTable;
		}		
    }

    //-- Bundle this codebase principal into an aggregate principal
    nsAggregatePrincipal* agg = new nsAggregatePrincipal();
    if (!agg) return NS_ERROR_OUT_OF_MEMORY;
    rv = agg->SetCodebase(principal);
    if (NS_FAILED(rv)) return rv;
    principal = do_QueryInterface((nsBasePrincipal*)agg, &rv);
    if (NS_FAILED(rv)) return rv;

    *result = principal;
    NS_ADDREF(*result);
    return NS_OK;
}

nsresult
nsScriptSecurityManager::GetScriptPrincipal(JSContext *cx, 
                                            JSScript *script,
                                            nsIPrincipal **result) 
{
    if (!script)
    {
        *result = nsnull;
        return NS_OK;
    }
    JSPrincipals *jsp = JS_GetScriptPrincipals(cx, script);
    if (!jsp) {
        // Script didn't have principals -- shouldn't happen.
        return NS_ERROR_FAILURE;
    }
    nsJSPrincipals *nsJSPrin = NS_STATIC_CAST(nsJSPrincipals *, jsp);
    *result = nsJSPrin->nsIPrincipalPtr;
    if (!result)
        return NS_ERROR_FAILURE;
    NS_ADDREF(*result);
    return NS_OK;

}

nsresult
nsScriptSecurityManager::GetFunctionObjectPrincipal(JSContext *cx, 
                                                    JSObject *obj,
                                                    nsIPrincipal **result) 
{
    JSFunction *fun = (JSFunction *) JS_GetPrivate(cx, obj);
    if (JS_GetFunctionObject(fun) != obj)
        // Function has been cloned; get principals from scope
        return GetObjectPrincipal(cx, obj, result);
    JSScript *script = JS_GetFunctionScript(cx, fun);
    return GetScriptPrincipal(cx, script, result);
}

nsresult
nsScriptSecurityManager::GetFramePrincipal(JSContext *cx, 
                                           JSStackFrame *fp,
                                           nsIPrincipal **result) 
{
    JSObject *obj = JS_GetFrameFunctionObject(cx, fp);
    if (!obj)
    {
        // Must be in a top-level script. Get principal from the script.
        JSScript *script = JS_GetFrameScript(cx, fp);
        return GetScriptPrincipal(cx, script, result);
    } 
    return GetFunctionObjectPrincipal(cx, obj, result);
}

nsresult
nsScriptSecurityManager::GetPrincipalAndFrame(JSContext *cx,
                                              PRBool skipInnerFrame,
                                              nsIPrincipal **result, 
                                              JSStackFrame **frameResult) 
{
    // Get principals from innermost frame of JavaScript or Java.
    JSStackFrame *fp = nsnull; // tell JS_FrameIterator to start at innermost
    if (skipInnerFrame) // Skip the innermost frame
    {
        fp = JS_FrameIterator(cx, &fp);
#ifdef DEBUG_mstoltz
        if(!fp)
            printf("####### JS stack weirdness in GetPrincipalAndFrame.\n");
#endif
    }
    for (fp = JS_FrameIterator(cx, &fp); fp; fp = JS_FrameIterator(cx, &fp))
    {
        if (NS_FAILED(GetFramePrincipal(cx, fp, result)))
            return NS_ERROR_FAILURE;
        if (*result)
        {
            *frameResult = fp;
            return NS_OK;
        }
    }

    //-- If there's no principal on the stack, look at the global object
    //   and return the innermost frame for annotations.
    if (cx) 
    {
        nsCOMPtr<nsIScriptContext> scriptContext = 
            NS_REINTERPRET_CAST(nsIScriptContext*,JS_GetContextPrivate(cx));
        if (scriptContext)
        {
            nsCOMPtr<nsIScriptGlobalObject> global = scriptContext->GetGlobalObject();
            NS_ENSURE_TRUE(global, NS_ERROR_FAILURE);
            nsCOMPtr<nsIScriptObjectPrincipal> globalData = do_QueryInterface(global);
            NS_ENSURE_TRUE(globalData, NS_ERROR_FAILURE);
            globalData->GetPrincipal(result);
            if (*result)
            {
                JSStackFrame *inner = nsnull;
                *frameResult = JS_FrameIterator(cx, &inner);
                return NS_OK;
            }
        }
    }   

    *result = nsnull;
    return NS_OK;
}

nsresult
nsScriptSecurityManager::GetSubjectPrincipal(JSContext *cx, 
                                             nsIPrincipal **result)
{
    JSStackFrame *fp;
    return GetPrincipalAndFrame(cx, PR_FALSE, result, &fp);
}

nsresult
nsScriptSecurityManager::GetCallingPrincipal(JSContext *cx, 
                                             nsIPrincipal **result)
{
    JSStackFrame *fp;
    return GetPrincipalAndFrame(cx, PR_TRUE, result, &fp);
}

nsresult
nsScriptSecurityManager::GetObjectPrincipal(JSContext *aCx, JSObject *aObj,
                                            nsIPrincipal **result)
{
    JSObject *parent = aObj;
    do
    {
        JSClass *jsClass = JS_GetClass(aCx, parent);
        const uint32 privateNsISupports = JSCLASS_HAS_PRIVATE | 
                                          JSCLASS_PRIVATE_IS_NSISUPPORTS;
        if (jsClass && (jsClass->flags & (privateNsISupports)) == 
                            privateNsISupports)
        {
            nsCOMPtr<nsISupports> supports = (nsISupports *) JS_GetPrivate(aCx, parent);
            nsCOMPtr<nsIScriptObjectPrincipal> objPrin = 
                do_QueryInterface(supports);
            if (!objPrin)
            {
                /*
                 * If it's a wrapped native, check the underlying native
                 * instead.
                 */
                nsCOMPtr<nsIXPConnectWrappedNative> xpcNative = 
                    do_QueryInterface(supports);
                if (xpcNative)
                    xpcNative->GetNative(getter_AddRefs(supports));
                objPrin = do_QueryInterface(supports);
            }

            if (objPrin && NS_SUCCEEDED(objPrin->GetPrincipal(result)))
                return NS_OK;
        }
        parent = JS_GetParent(aCx, parent);
    } while (parent);

    // Couldn't find a principal for this object.
    return NS_ERROR_FAILURE;
}

nsresult
nsScriptSecurityManager::SavePrincipal(nsIPrincipal* aToSave)
{
    NS_ASSERTION(mSecurityPrefs, "nsScriptSecurityManager::mSecurityPrefs not initialized");
    nsresult rv;
    nsCOMPtr<nsIPrincipal> persistent = aToSave;
    nsCOMPtr<nsIAggregatePrincipal> aggregate = do_QueryInterface(aToSave, &rv);
    if (NS_SUCCEEDED(rv))
        if (NS_FAILED(aggregate->GetPrimaryChild(getter_AddRefs(persistent))))
            return NS_ERROR_FAILURE;

    //-- Save to mPrincipals
    if (!mPrincipals) 
    {
        mPrincipals = new nsSupportsHashtable(31);
        if (!mPrincipals)
            return NS_ERROR_OUT_OF_MEMORY;
    }
    nsIPrincipalKey key(persistent);
    mPrincipals->Put(&key, persistent);

    //-- Save to prefs
    nsXPIDLCString idPrefName;
    nsXPIDLCString id;
    nsXPIDLCString grantedList;
    nsXPIDLCString deniedList;
    rv = persistent->GetPreferences(getter_Copies(idPrefName),
                                    getter_Copies(id),
                                    getter_Copies(grantedList),
                                    getter_Copies(deniedList));
    if (NS_FAILED(rv)) return NS_ERROR_FAILURE;

    nsXPIDLCString grantedPrefName;
    nsXPIDLCString deniedPrefName;
    rv = PrincipalPrefNames( idPrefName, 
                             getter_Copies(grantedPrefName), 
                             getter_Copies(deniedPrefName)  );
    if (NS_FAILED(rv)) return NS_ERROR_FAILURE;

    mIsWritingPrefs = PR_TRUE;
    if (grantedList)
        mSecurityPrefs->SecuritySetCharPref(grantedPrefName, grantedList);
    else
        mSecurityPrefs->SecurityClearUserPref(grantedPrefName);

    if (deniedList)
        mSecurityPrefs->SecuritySetCharPref(deniedPrefName, deniedList);
    else
        mSecurityPrefs->SecurityClearUserPref(deniedPrefName);

    if (grantedList || deniedList)
        mSecurityPrefs->SecuritySetCharPref(idPrefName, id);
    else
        mSecurityPrefs->SecurityClearUserPref(idPrefName);

    mIsWritingPrefs = PR_FALSE;
    return mPrefService->SavePrefFile(nsnull);
}

///////////////// Capabilities API /////////////////////
NS_IMETHODIMP
nsScriptSecurityManager::IsCapabilityEnabled(const char *capability,
                                                 PRBool *result)
{
    return IsCapabilityEnabledImpl(capability, PR_FALSE, result);
}

nsresult
nsScriptSecurityManager::IsCapabilityEnabledImpl(const char *capability,
                                                 PRBool aSkipFrame,
                                                 PRBool *result)
{
    nsresult rv;
    JSStackFrame *fp = nsnull;
    JSContext *cx = GetCurrentContextQuick();
    fp = cx ? JS_FrameIterator(cx, &fp) : nsnull;
    if (!fp) {
        // No script code on stack. Allow execution.
        *result = PR_TRUE;
        return NS_OK;
    }
    if (aSkipFrame)
       fp = JS_FrameIterator(cx, &fp);

    do
    {
        nsCOMPtr<nsIPrincipal> principal;
        if (NS_FAILED(GetFramePrincipal(cx, fp, getter_AddRefs(principal))))
            return NS_ERROR_FAILURE;
        if (!principal)
            continue;

        // First check if the principal is even able to enable the 
        // given capability. If not, don't look any further.
        PRInt16 canEnable;
        rv = principal->CanEnableCapability(capability, &canEnable);
        if (NS_FAILED(rv))
            return rv;
        if (canEnable != nsIPrincipal::ENABLE_GRANTED &&
            canEnable != nsIPrincipal::ENABLE_WITH_USER_PERMISSION) 
        {
            *result = PR_FALSE;
            return NS_OK;
        }

        // Now see if the capability is enabled.
        void *annotation = JS_GetFrameAnnotation(cx, fp);
        rv = principal->IsCapabilityEnabled(capability, annotation, 
                                            result);
        if (NS_FAILED(rv))
            return rv;
        if (*result)
            return NS_OK;
    } while ((fp = JS_FrameIterator(cx, &fp)) != nsnull);
    *result = PR_FALSE;
    return NS_OK;
}

#define PROPERTIES_URL "chrome://communicator/locale/security/security.properties"

nsresult
Localize(char *genericString, nsString &result) 
{
    nsresult ret;
    
    /* create a URL for the string resource file */
    nsIIOService *pNetService = nsnull;
    ret = nsServiceManager::GetService(kIOServiceCID, kIIOServiceIID,
                                       (nsISupports**) &pNetService);
    if (NS_FAILED(ret))
    {
        NS_WARNING("cannot get net service\n");
        return ret;
    }
    nsIURI *uri = nsnull;
    ret = pNetService->NewURI(PROPERTIES_URL, nsnull, &uri);
    if (NS_FAILED(ret))
    {
        NS_WARNING("cannot create URI\n");
        nsServiceManager::ReleaseService(kIOServiceCID, pNetService);
        return ret;
    }
    
    nsIURI *url = nsnull;
    ret = uri->QueryInterface(NS_GET_IID(nsIURI), (void**)&url);
    nsServiceManager::ReleaseService(kIOServiceCID, pNetService);
    
    if (NS_FAILED(ret))
    {
        NS_WARNING("cannot create URL\n");
        return ret;
    }
    
    /* create a bundle for the localization */
    nsIStringBundleService *pStringService = nsnull;
    ret = nsServiceManager::GetService(kStringBundleServiceCID,
        kIStringBundleServiceIID, (nsISupports**) &pStringService);
    if (NS_FAILED(ret))
    {
        NS_WARNING("cannot get string service\n");
        return ret;
    }
    char *spec = nsnull;
    ret = url->GetSpec(&spec);
    if (NS_FAILED(ret))
    {
        NS_WARNING("cannot get url spec\n");
        nsServiceManager::ReleaseService(kStringBundleServiceCID, pStringService);
        nsCRT::free(spec);
        return ret;
    }
    nsIStringBundle *bundle = nsnull;
    ret = pStringService->CreateBundle(spec, &bundle);
    nsCRT::free(spec);
    nsServiceManager::ReleaseService(kStringBundleServiceCID, pStringService);
    if (NS_FAILED(ret))
    {
        NS_WARNING("cannot create instance\n");
        return ret;
    }
    
    /* localize the given string */
    nsAutoString strtmp;
    strtmp.AssignWithConversion(genericString);

    PRUnichar *ptrv = nsnull;
    ret = bundle->GetStringFromName(strtmp.GetUnicode(), &ptrv);
    NS_RELEASE(bundle);
    if (NS_FAILED(ret))
        NS_WARNING("cannot get string from name\n");
    result = ptrv;
    nsCRT::free(ptrv);
    return ret;
}

static PRBool
CheckConfirmDialog(JSContext* cx, const PRUnichar *szMessage, const PRUnichar *szCheckMessage,
                   PRBool *checkValue) 
{
    nsresult res;
    //-- Get a prompter for the current window.
    nsCOMPtr<nsIPrompt> prompter;
    nsCOMPtr<nsIScriptContext> scriptContext = (nsIScriptContext*)JS_GetContextPrivate(cx);
    if (scriptContext)
    {
        nsCOMPtr<nsIScriptGlobalObject> globalObject(dont_AddRef(scriptContext->GetGlobalObject()));
        NS_ASSERTION(globalObject, "script context has no global object");
        nsCOMPtr<nsIDOMWindowInternal> domWin = do_QueryInterface(globalObject);
        if (domWin)
            domWin->GetPrompter(getter_AddRefs(prompter));
    }

    if (!prompter)
    {
        //-- Couldn't get prompter from the current window, so get the prompt service.
        nsCOMPtr<nsIWindowWatcher> wwatch(do_GetService("@mozilla.org/embedcomp/window-watcher;1"));
        if (wwatch)
          wwatch->GetNewPrompter(0, getter_AddRefs(prompter));
    }
    if (!prompter)
    {
        *checkValue = 0;
        return PR_FALSE;
    }
    
    PRInt32 buttonPressed = 1; /* in case user exits dialog by clicking X */
    nsAutoString dialogTitle;
    if (NS_FAILED(res = Localize("Titleline", dialogTitle)))
        return PR_FALSE;
    
    res = prompter->ConfirmEx(dialogTitle.GetUnicode(), szMessage,
                              (nsIPrompt::BUTTON_TITLE_YES * nsIPrompt::BUTTON_POS_0) +
                              (nsIPrompt::BUTTON_TITLE_NO * nsIPrompt::BUTTON_POS_1),
                              nsnull, nsnull, nsnull, szCheckMessage, checkValue, &buttonPressed);
    
    if (NS_FAILED(res))
        *checkValue = 0;
    if (*checkValue != 0 && *checkValue != 1)
        *checkValue = 0; /* this should never happen but it is happening!!! */
    return (buttonPressed == 0);
}

NS_IMETHODIMP
nsScriptSecurityManager::RequestCapability(nsIPrincipal* aPrincipal, 
                                           const char *capability, PRInt16* canEnable)
{
    if (NS_FAILED(aPrincipal->CanEnableCapability(capability, canEnable)))
        return NS_ERROR_FAILURE;
    if (*canEnable == nsIPrincipal::ENABLE_WITH_USER_PERMISSION)
    {
        // Prompt user for permission to enable capability.
        static PRBool remember = PR_TRUE;
        nsAutoString query, check;
        if (NS_FAILED(Localize("EnableCapabilityQuery", query)))
            return NS_ERROR_FAILURE;
        if (NS_FAILED(Localize("CheckMessage", check)))
            return NS_ERROR_FAILURE;
        char *source;
        if (NS_FAILED(aPrincipal->ToUserVisibleString(&source)))
            return NS_ERROR_FAILURE;
        PRUnichar *message = nsTextFormatter::smprintf(query.GetUnicode(), source);
        Recycle(source);
        JSContext *cx = GetCurrentContextQuick();
        if (CheckConfirmDialog(cx, message, check.GetUnicode(), &remember))
            *canEnable = nsIPrincipal::ENABLE_GRANTED;
        else
            *canEnable = nsIPrincipal::ENABLE_DENIED;
        PR_FREEIF(message);
        if (remember)
        {
            //-- Save principal to prefs and to mPrincipals
            if (NS_FAILED(aPrincipal->SetCanEnableCapability(capability, *canEnable)))
                return NS_ERROR_FAILURE;
            if (NS_FAILED(SavePrincipal(aPrincipal)))
                return NS_ERROR_FAILURE;
        }
    }
    return NS_OK;
}

NS_IMETHODIMP
nsScriptSecurityManager::EnableCapability(const char *capability)
{
    JSContext *cx = GetCurrentContextQuick();
    JSStackFrame *fp;

    //Error checks for capability string length (200)
    if(PL_strlen(capability)>200)
    {
		static const char msg[] = "Capability name too long";
		JS_SetPendingException(cx, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, msg)));
        return NS_ERROR_FAILURE;
    }
    
    nsCOMPtr<nsIPrincipal> principal;
    if (NS_FAILED(GetPrincipalAndFrame(cx, PR_FALSE, getter_AddRefs(principal), 
                                       &fp)))
        return NS_ERROR_FAILURE;
    void *annotation = JS_GetFrameAnnotation(cx, fp);
    PRBool enabled;
    if (NS_FAILED(principal->IsCapabilityEnabled(capability, annotation, 
                                                 &enabled)))
        return NS_ERROR_FAILURE;
    if (enabled)
        return NS_OK;

    PRInt16 canEnable;
    if (NS_FAILED(RequestCapability(principal, capability, &canEnable)))
        return NS_ERROR_FAILURE;
   
    if (canEnable != nsIPrincipal::ENABLE_GRANTED)
    {
		static const char msg[] = "enablePrivilege not granted";
		JS_SetPendingException(cx, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, msg)));
        return NS_ERROR_FAILURE; // XXX better error code?
    }
    if (NS_FAILED(principal->EnableCapability(capability, &annotation))) 
        return NS_ERROR_FAILURE;
    JS_SetFrameAnnotation(cx, fp, annotation);
    return NS_OK;
}

NS_IMETHODIMP
nsScriptSecurityManager::RevertCapability(const char *capability)
{
    JSContext *cx = GetCurrentContextQuick();
    JSStackFrame *fp;
    nsCOMPtr<nsIPrincipal> principal;
    if (NS_FAILED(GetPrincipalAndFrame(cx, PR_FALSE, getter_AddRefs(principal), 
                                       &fp)))
        return NS_ERROR_FAILURE;
    void *annotation = JS_GetFrameAnnotation(cx, fp);
    principal->RevertCapability(capability, &annotation);
    JS_SetFrameAnnotation(cx, fp, annotation);
    return NS_OK;
}

NS_IMETHODIMP
nsScriptSecurityManager::DisableCapability(const char *capability)
{
    JSContext *cx = GetCurrentContextQuick();
    JSStackFrame *fp;
    nsCOMPtr<nsIPrincipal> principal;
    if (NS_FAILED(GetPrincipalAndFrame(cx, PR_FALSE, getter_AddRefs(principal), 
                                       &fp)))
        return NS_ERROR_FAILURE;
    void *annotation = JS_GetFrameAnnotation(cx, fp);
    principal->DisableCapability(capability, &annotation);
    JS_SetFrameAnnotation(cx, fp, annotation);
    return NS_OK;
}

//////////////// Master Certificate Functions ///////////////////////////////////////
NS_IMETHODIMP
nsScriptSecurityManager::SetCanEnableCapability(const char* certificateID, 
                                                const char* capability,
                                                PRInt16 canEnable)
{
    nsresult rv;
    nsCOMPtr<nsIPrincipal> subjectPrincipal;
    rv = GetSubjectPrincipal(getter_AddRefs(subjectPrincipal));
    if (NS_FAILED(rv)) return NS_ERROR_FAILURE;

    //-- Get the system certificate
    if (!mSystemCertificate)
    {
        nsCOMPtr<nsIFile> systemCertFile;
        NS_WITH_SERVICE(nsIProperties, directoryService, NS_DIRECTORY_SERVICE_CONTRACTID, &rv);
        if (!directoryService) return NS_ERROR_FAILURE;
        rv = directoryService->Get(NS_XPCOM_CURRENT_PROCESS_DIR, NS_GET_IID(nsIFile), 
                              getter_AddRefs(systemCertFile));
        if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
#ifdef XP_MAC
        // On Mac, this file will be located in the Essential Files folder
        systemCertFile->Append("Essential Files");
        if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
#endif
        systemCertFile->Append("systemSignature.jar");
        if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
        nsCOMPtr<nsIZipReader> systemCertZip;
        rv = nsComponentManager::CreateInstance(kZipReaderCID, nsnull, 
                                                NS_GET_IID(nsIZipReader),
                                                getter_AddRefs(systemCertZip));
        if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
        systemCertZip->Init(systemCertFile);
        rv = systemCertZip->Open();
        if (NS_SUCCEEDED(rv))
        {
            nsCOMPtr<nsIJAR> systemCertJar = do_QueryInterface(systemCertZip, &rv);
            if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
            rv = systemCertJar->GetCertificatePrincipal(nsnull, 
                                                        getter_AddRefs(mSystemCertificate));
            if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
        }
    }

    //-- Make sure the caller's principal is the system certificate
    PRBool isEqual = PR_FALSE;
    if (mSystemCertificate)
    {
        rv = mSystemCertificate->Equals(subjectPrincipal, &isEqual);
        if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
    }
    if (!isEqual)
    {
        JSContext* cx = GetCurrentContextQuick();
        if (!cx) return NS_ERROR_FAILURE;
		static const char msg1[] = "Only code signed by the system certificate may call SetCanEnableCapability or Invalidate";
        static const char msg2[] = "Attempt to call SetCanEnableCapability or Invalidate when no system certificate has been established";
            JS_SetPendingException(cx, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, 
                                   mSystemCertificate ? msg1 : msg2)));
        return NS_ERROR_FAILURE;
    }

    //-- Get the target principal
    nsCOMPtr<nsIPrincipal> objectPrincipal;
    rv =  GetCertificatePrincipal(certificateID, getter_AddRefs(objectPrincipal));
    if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
    rv = objectPrincipal->SetCanEnableCapability(capability, canEnable);
    if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
    return SavePrincipal(objectPrincipal);
}

////////////////////////////////////////////////
// Methods implementing nsIXPCSecurityManager //
////////////////////////////////////////////////

NS_IMETHODIMP
nsScriptSecurityManager::CanCreateWrapper(JSContext *aJSContext, 
                                          const nsIID &aIID, 
                                          nsISupports *aObj,
                                          nsIClassInfo *aClassInfo,
                                          void **aPolicy)
{
#if 0
    char* iidStr = aIID.ToString();
    printf("### CanCreateWrapper(%s) ", iidStr);
    PR_FREEIF(iidStr);
#endif
// XXX Special case for nsIXPCException ?
    if (IsDOMClass(aClassInfo))
    {
#if 0
        printf("DOM class - GRANTED.\n");
#endif
        return NS_OK;
    }

    //--See if the object advertises a non-default level of access
    //  using nsISecurityCheckedComponent
    nsCOMPtr<nsISecurityCheckedComponent> checkedComponent =
        do_QueryInterface(aObj);

    nsXPIDLCString objectSecurityLevel;
    if (checkedComponent)
        checkedComponent->CanCreateWrapper((nsIID *)&aIID, getter_Copies(objectSecurityLevel));

    return CheckXPCPermissions(aJSContext, aObj, objectSecurityLevel, PR_FALSE,
                               "Permission denied to create wrapper for object");
}

NS_IMETHODIMP
nsScriptSecurityManager::CanCreateInstance(JSContext *aJSContext,
                                           const nsCID &aCID)
{
    //XXX Special cases needed: exceptions?
#if 0
    char* cidStr = aCID.ToString();
    printf("### CanCreateInstance(%s) ", cidStr);
    PR_FREEIF(cidStr);
#endif

    return CheckXPCPermissions(aJSContext, nsnull, nsnull, PR_FALSE, 
        "Permission denied to create instance of class");
}

NS_IMETHODIMP
nsScriptSecurityManager::CanGetService(JSContext *aJSContext, 
                                       const nsCID &aCID)
{
#if 0
    char* cidStr = aCID.ToString();
    printf("### CanGetService(%s) ", cidStr);
    PR_FREEIF(cidStr);
#endif

    return CheckXPCPermissions(aJSContext, nsnull, nsnull, PR_FALSE, 
                              "Permission denied to get service");
}

/* void CanAccess (in PRUint32 aAction, in nsIXPCNativeCallContext aCallContext, in JSContextPtr aJSContext, in JSObjectPtr aJSObject, in nsISupports aObj, in nsIClassInfo aClassInfo, in JSVal aName, inout voidPtr aPolicy); */
NS_IMETHODIMP 
nsScriptSecurityManager::CanAccess(PRUint32 aAction,
                                   nsIXPCNativeCallContext* aCallContext,
                                   JSContext* aJSContext,
                                   JSObject* aJSObject,
                                   nsISupports* aObj,
                                   nsIClassInfo* aClassInfo,
                                   jsval aName,
                                   void** aPolicy)
{
    return CheckPropertyAccessImpl(aAction, aCallContext, aJSContext, aJSObject,
                                   aObj, aClassInfo, aName, nsnull, nsnull, PR_TRUE, aPolicy);
}

nsresult
nsScriptSecurityManager::CheckXPCPermissions(JSContext *aJSContext,
                                             nsISupports* aObj,
                                             const char* aObjectSecurityLevel,
                                             PRBool aSkipFrame,
                                             const char* aErrorMsg)
{
    //-- Check for the all-powerful UniversalXPConnect privilege
    PRBool ok = PR_FALSE;
    if (NS_SUCCEEDED(IsCapabilityEnabledImpl("UniversalXPConnect", aSkipFrame, &ok)) && ok)
        return NS_OK;

    //-- If the object implements nsISecurityCheckedComponent, it has a non-default policy.
    if (aObjectSecurityLevel)
    {
        if (PL_strcasecmp(aObjectSecurityLevel, "AllAccess") == 0)
            return NS_OK;
        else if (PL_strcasecmp(aObjectSecurityLevel, "NoAccess") != 0)
        {
            PRBool canAccess = PR_FALSE;
            if (NS_SUCCEEDED(IsCapabilityEnabled(aObjectSecurityLevel, &canAccess)) &&
                canAccess)
                return NS_OK;
        }
    }

    //-- If user allows scripting of plugins by untrusted scripts, 
    //   and the target object is a plugin, allow the access.
    if(aObj)
    {
        nsresult rv;
        nsCOMPtr<nsIPluginInstance> plugin = do_QueryInterface(aObj, &rv);
        if (NS_SUCCEEDED(rv))
        {
            static PRBool prefSet = PR_FALSE;
            static PRBool allowPluginAccess = PR_FALSE;
            if (!prefSet)
            {
                rv = mPrefs->GetBoolPref("security.xpconnect.plugin.unrestricted",
                                         &allowPluginAccess);
                prefSet = PR_TRUE;
            }
            if (allowPluginAccess)
                return NS_OK;
        }
    }

    //-- Access tests failed, so report error
    JS_SetPendingException(aJSContext, 
                           STRING_TO_JSVAL(JS_NewStringCopyZ(aJSContext, aErrorMsg)));
    return NS_ERROR_DOM_XPCONNECT_ACCESS_DENIED;
}

/////////////////////////////////////
// Method implementing nsIObserver //
/////////////////////////////////////
NS_IMETHODIMP
nsScriptSecurityManager::Observe(nsISupports* aObject, const PRUnichar* aAction,
                                 const PRUnichar* aPrefName)
{
    nsresult rv = NS_OK;
    nsCAutoString prefNameStr;
    prefNameStr.AssignWithConversion(aPrefName);
    char* prefName = prefNameStr.ToNewCString();
    if (!prefName)
        return NS_ERROR_OUT_OF_MEMORY;

    static const char jsPrefix[] = "javascript.";
    if(PL_strncmp(prefName, jsPrefix, sizeof(jsPrefix)-1) == 0)
        JSEnabledPrefChanged();
    else if((PL_strncmp(prefName, sPrincipalPrefix, sizeof(sPrincipalPrefix)-1) == 0) &&
            !mIsWritingPrefs)
    {
        static const char id[] = "id";
        char* lastDot = PL_strrchr(prefName, '.');
        //-- This check makes sure the string copy below doesn't overwrite its bounds
        if(PL_strlen(lastDot) >= sizeof(id))
        {
            PL_strcpy(lastDot + 1, id);
            const char** idPrefArray = (const char**)&prefName;
            rv = InitPrincipals(1, idPrefArray);
        }
    }
    PR_Free(prefName);
    return rv;
}

/////////////////////////////////////////////
// Constructor, Destructor, Initialization //
/////////////////////////////////////////////
nsScriptSecurityManager::nsScriptSecurityManager(void)
    : mOriginToPolicyMap(nsnull),
      mClassPolicies(nsnull),
      mSystemPrincipal(nsnull), mPrincipals(nsnull), 
      mIsJavaScriptEnabled(PR_FALSE),
      mIsMailJavaScriptEnabled(PR_FALSE),
      mIsWritingPrefs(PR_FALSE),
      mNameSetRegistered(PR_FALSE)

{
    NS_INIT_REFCNT();
    InitPrefs();
    mThreadJSContextStack = do_GetService("@mozilla.org/js/xpc/ContextStack;1");
}

nsScriptSecurityManager::~nsScriptSecurityManager(void)
{
    delete mOriginToPolicyMap;
    delete mClassPolicies;
    NS_IF_RELEASE(mSystemPrincipal);
    delete mPrincipals;
} 

nsScriptSecurityManager *
nsScriptSecurityManager::GetScriptSecurityManager()
{
    static nsScriptSecurityManager *ssecMan = NULL;
    if (!ssecMan)
    {
        ssecMan = new nsScriptSecurityManager();
        if (!ssecMan)
            return NULL;
        nsresult rv;

        NS_WITH_SERVICE(nsIXPConnect, xpc, nsIXPConnect::GetCID(), &rv);
        if (NS_SUCCEEDED(rv) && xpc)
        {
            rv = xpc->SetDefaultSecurityManager(
                            NS_STATIC_CAST(nsIXPCSecurityManager*, ssecMan), 
                            nsIXPCSecurityManager::HOOK_ALL);
            if (NS_FAILED(rv))
                NS_WARNING("failed to install xpconnect security manager!");    
#ifdef DEBUG_jband
            else
                printf("!!!!! xpc security manager registered\n");
#endif
        }
        else
            NS_WARNING("can't get xpconnect to install security manager!");    
    }
    return ssecMan;
}

const char* nsScriptSecurityManager::sJSEnabledPrefName = "javascript.enabled";
const char* nsScriptSecurityManager::sJSMailEnabledPrefName = "javascript.allow.mailnews";
const char* nsScriptSecurityManager::sPrincipalPrefix = "capability.principal";

PR_STATIC_CALLBACK(PRBool)
DeleteEntry(nsHashKey *aKey, void *aData, void* closure)
{
    nsDomainEntry *entry = (nsDomainEntry *) aData;
    do
    {
        nsDomainEntry *next = entry->mNext;
        delete entry;
        entry = next;
    } while (entry);
    return PR_TRUE;
}

nsresult
nsScriptSecurityManager::InitPolicies(PRUint32 aPrefCount, const char** aPrefNames)
{
    for (PRUint32 c = 0; c < aPrefCount; c++)
    {
        unsigned count = 0;
        const char *dots[5];
        const char *p;
        for (p=aPrefNames[c]; *p; p++)
        {
            if (*p == '.')
            {
                dots[count++] = p;
                if (count == sizeof(dots)/sizeof(dots[0]))
                    break;
            }
        }
        if (count < sizeof(dots)/sizeof(dots[0]))
            dots[count] = p;
        if (count < 3)
            continue;
        const char *policyName = dots[1] + 1;
        int policyLength = dots[2] - policyName;
        PRBool isDefault = PL_strncmp("default", policyName, policyLength) == 0;
        if (!isDefault && count == 3)
        {
            // capability.policy.<policyname>.sites
            const char *sitesName = dots[2] + 1;
            int sitesLength = dots[3] - sitesName;
            if (PL_strncmp("sites", sitesName, sitesLength) == 0)
            {
                if (!mOriginToPolicyMap)
                {
                    mOriginToPolicyMap = 
                        new nsObjectHashtable(nsnull, nsnull, DeleteEntry, nsnull);
                    if (!mOriginToPolicyMap)
                        return NS_ERROR_OUT_OF_MEMORY;
                }
                char *s;
                if (NS_FAILED(mSecurityPrefs->SecurityGetCharPref(aPrefNames[c], &s)))
                    return NS_ERROR_FAILURE;
                char *q=s;
                char *r=s;
                char *lastDot = nsnull;
                char *nextToLastDot = nsnull;
                PRBool working = PR_TRUE;
                while (working)
                {
                    if (*r == ' ' || *r == '\0')
                    {
                        working = (*r != '\0');
                        *r = '\0';
                        nsCStringKey key(nextToLastDot ? nextToLastDot+1 : q);
                        nsDomainEntry *value = new nsDomainEntry(q, policyName, 
                                                                 policyLength);
                        if (!value)
                            break;
                        nsDomainEntry *de = (nsDomainEntry *) 
                            mOriginToPolicyMap->Get(&key);
                        if (!de)
                            mOriginToPolicyMap->Put(&key, value);
                        else
                        {
                            if (de->Matches(q))
                            {
                                value->mNext = de;
                                mOriginToPolicyMap->Put(&key, value);
                            }
                            else
                            {
                                while (de->mNext)
                                {
                                    if (de->mNext->Matches(q))
                                    {
                                        value->mNext = de->mNext;
                                        de->mNext = value;
                                        break;
                                    }
                                    de = de->mNext;
                                }
                                if (!de->mNext)
                                    de->mNext = value;
                            }
                        }
                        q = r + 1;
                        lastDot = nextToLastDot = nsnull;
                    }
                    else if (*r == '.')
                    {
                        nextToLastDot = lastDot;
                        lastDot = r;
                    }
                    r++;
                }
                PR_Free(s);
            }
        }
        else if (count > 3)
        { // capability.policy.<policyname>.<class>.<property>[.(get|set)]
          // Store the class name so we know this class has a policy set on it
            const char* className = dots[2] + 1;
            PRInt32 classNameLen = dots[3] - className;
            char* classNameNullTerm = PL_strndup(className, classNameLen);
            if (!classNameNullTerm)
                return NS_ERROR_OUT_OF_MEMORY;
            nsCStringKey classNameKey(classNameNullTerm);
            if (!(mClassPolicies))
                mClassPolicies = new nsHashtable(31);
            // We don't actually have to store the class name as data in the hashtable, 
            // since all we check for is whether the key exists.
            void* classPolicy = mClassPolicies->Get(&classNameKey);
            if (isDefault && !classPolicy)
                mClassPolicies->Put(&classNameKey, (void*)CLASS_POLICY_DEFAULT);
            else if (!isDefault && classPolicy != (void*)CLASS_POLICY_SITE)
                mClassPolicies->Put(&classNameKey, (void*)CLASS_POLICY_SITE);
            PR_Free(classNameNullTerm);
        }
    }
    return NS_OK;
}

nsresult
nsScriptSecurityManager::PrincipalPrefNames(const char* pref, 
                                            char** grantedPref, char** deniedPref)
{
    char* lastDot = PL_strrchr(pref, '.');
    if (!lastDot) return NS_ERROR_FAILURE;
    PRInt32 prefLen = lastDot - pref + 1;

    *grantedPref = nsnull;
    *deniedPref = nsnull;

    static const char granted[] = "granted";
    *grantedPref = (char*)PR_MALLOC(prefLen + sizeof(granted));
    if (!grantedPref) return NS_ERROR_OUT_OF_MEMORY;
    PL_strncpy(*grantedPref, pref, prefLen);
    PL_strcpy(*grantedPref + prefLen, granted);

    static const char denied[] = "denied";
    *deniedPref = (char*)PR_MALLOC(prefLen + sizeof(denied));
    if (!deniedPref)
    {
        PR_FREEIF(*grantedPref);
        return NS_ERROR_OUT_OF_MEMORY;
    }
    PL_strncpy(*deniedPref, pref, prefLen);
    PL_strcpy(*deniedPref + prefLen, denied);

    return NS_OK;
}

nsresult
nsScriptSecurityManager::InitPrincipals(PRUint32 aPrefCount, const char** aPrefNames)
{
    /* This is the principal preference syntax:
     * capability.principal.[codebase|certificate].<name>.[id|granted|denied]
     * For example:
     * user_pref("capability.principal.certificate.p1.id","12:34:AB:CD");
     * user_pref("capability.principal.certificate.p1.granted","Capability1 Capability2"); 
     * user_pref("capability.principal.certificate.p1.denied","Capability3");
     */

    static const char idSuffix[] = ".id";
    for (PRUint32 c = 0; c < aPrefCount; c++)
    {
        PRInt32 prefNameLen = PL_strlen(aPrefNames[c]) - (sizeof(idSuffix)-1);
        if (PL_strcasecmp(aPrefNames[c] + prefNameLen, idSuffix) != 0)
            continue;

        char* id;
        if (NS_FAILED(mSecurityPrefs->SecurityGetCharPref(aPrefNames[c], &id))) 
            return NS_ERROR_FAILURE;

        nsXPIDLCString grantedPrefName;
        nsXPIDLCString deniedPrefName;
        nsresult rv = PrincipalPrefNames(aPrefNames[c], 
                                         getter_Copies(grantedPrefName), 
                                         getter_Copies(deniedPrefName));
        if (rv == NS_ERROR_OUT_OF_MEMORY)
            return rv;
        else if (NS_FAILED(rv))
            continue;

        char* grantedList = nsnull;
        mSecurityPrefs->SecurityGetCharPref(grantedPrefName, &grantedList);
        char* deniedList = nsnull;
        mSecurityPrefs->SecurityGetCharPref(deniedPrefName, &deniedList);

        //-- Delete prefs if their value is the empty string
        if ((!id || id[0] == '\0') || 
            ((!grantedList || grantedList[0] == '\0') && (!deniedList || deniedList[0] == '\0')))
        {
            mSecurityPrefs->SecurityClearUserPref(aPrefNames[c]);
            mSecurityPrefs->SecurityClearUserPref(grantedPrefName);
            mSecurityPrefs->SecurityClearUserPref(deniedPrefName);
            PR_FREEIF(grantedList);
            PR_FREEIF(deniedList);
            continue;
        }

        //-- Create a principal based on the prefs
        static const char certificateName[] = "capability.principal.certificate";
        static const char codebaseName[] = "capability.principal.codebase";
        nsCOMPtr<nsIPrincipal> principal;
        if (PL_strncmp(aPrefNames[c], certificateName, 
                       sizeof(certificateName)-1) == 0) 
        {
            nsCertificatePrincipal *certificate = new nsCertificatePrincipal();
            if (certificate) {
                NS_ADDREF(certificate);
                if (NS_SUCCEEDED(certificate->InitFromPersistent(aPrefNames[c], id, 
                                                                 grantedList, deniedList))) 
                    principal = do_QueryInterface((nsBasePrincipal*)certificate);
                NS_RELEASE(certificate);
            }
        } else if(PL_strncmp(aPrefNames[c], codebaseName, 
                       sizeof(codebaseName)-1) == 0) 
        {
            nsCodebasePrincipal *codebase = new nsCodebasePrincipal();
            if (codebase) {
                NS_ADDREF(codebase);
                if (NS_SUCCEEDED(codebase->InitFromPersistent(aPrefNames[c], id, 
                                                              grantedList, deniedList))) 
                    principal = do_QueryInterface((nsBasePrincipal*)codebase);
                NS_RELEASE(codebase);
            }
        }
        PR_FREEIF(grantedList);
        PR_FREEIF(deniedList);
   
        if (principal)
        {
            if (!mPrincipals)
            {
                mPrincipals = new nsSupportsHashtable(31);
                if (!mPrincipals)
                    return NS_ERROR_OUT_OF_MEMORY;
            }
            nsIPrincipalKey key(principal);
            mPrincipals->Put(&key, principal);
        }
    }
    return NS_OK;
}


inline void
nsScriptSecurityManager::JSEnabledPrefChanged()
{
    if (NS_FAILED(mPrefs->GetBoolPref(sJSEnabledPrefName, 
                                      &mIsJavaScriptEnabled)))
        // Default to enabled.
        mIsJavaScriptEnabled = PR_TRUE;

    if (NS_FAILED(mPrefs->GetBoolPref(sJSMailEnabledPrefName, 
                                      &mIsMailJavaScriptEnabled))) 
        // Default to enabled.
        mIsMailJavaScriptEnabled = PR_TRUE;
}

nsresult
nsScriptSecurityManager::InitPrefs()
{
    nsresult rv;
    mPrefService = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mPrefService->GetBranch(nsnull, getter_AddRefs(mPrefs));
    NS_ENSURE_SUCCESS(rv, rv);
    mSecurityPrefs = do_QueryInterface(mPrefs, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIPref> oldPrefService = do_GetService(NS_PREF_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    // Set the initial value of the "javascript.enabled" prefs
    JSEnabledPrefChanged();
    // set observer callbacks in case the value of the pref changes
    oldPrefService->AddObserver(sJSEnabledPrefName, this);
    oldPrefService->AddObserver(sJSMailEnabledPrefName, this);

    PRUint32 prefCount;
    char** prefNames;

    //-- Initialize the policy database from prefs
    rv = mPrefs->GetChildList("capability.policy", &prefCount, &prefNames);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = InitPolicies(prefCount, (const char**)prefNames);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(prefCount, prefNames);

    //-- Initialize the principals database from prefs
    rv = mPrefs->GetChildList(sPrincipalPrefix, &prefCount, &prefNames);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = InitPrincipals(prefCount, (const char**)prefNames);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(prefCount, prefNames);

    //-- Set a callback for principal changes
    oldPrefService->AddObserver(sPrincipalPrefix, this);

    return NS_OK;
}
