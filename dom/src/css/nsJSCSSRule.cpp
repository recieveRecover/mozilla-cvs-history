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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */
/* AUTO-GENERATED. DO NOT EDIT!!! */

#include "jsapi.h"
#include "nsJSUtils.h"
#include "nsDOMError.h"
#include "nscore.h"
#include "nsIScriptContext.h"
#include "nsIScriptSecurityManager.h"
#include "nsIJSScriptObject.h"
#include "nsIScriptObjectOwner.h"
#include "nsIScriptGlobalObject.h"
#include "nsCOMPtr.h"
#include "nsDOMPropEnums.h"
#include "nsIPtr.h"
#include "nsString.h"
#include "nsIDOMCSSRule.h"
#include "nsIDOMCSSStyleSheet.h"


static NS_DEFINE_IID(kIScriptObjectOwnerIID, NS_ISCRIPTOBJECTOWNER_IID);
static NS_DEFINE_IID(kIJSScriptObjectIID, NS_IJSSCRIPTOBJECT_IID);
static NS_DEFINE_IID(kIScriptGlobalObjectIID, NS_ISCRIPTGLOBALOBJECT_IID);
static NS_DEFINE_IID(kICSSRuleIID, NS_IDOMCSSRULE_IID);
static NS_DEFINE_IID(kICSSStyleSheetIID, NS_IDOMCSSSTYLESHEET_IID);

NS_DEF_PTR(nsIDOMCSSRule);
NS_DEF_PTR(nsIDOMCSSStyleSheet);

//
// CSSRule property ids
//
enum CSSRule_slots {
  CSSRULE_TYPE = -1,
  CSSRULE_CSSTEXT = -2,
  CSSRULE_SHEET = -3
};

/***********************************************************************/
//
// CSSRule Properties Getter
//
PR_STATIC_CALLBACK(JSBool)
GetCSSRuleProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsIDOMCSSRule *a = (nsIDOMCSSRule*)nsJSUtils::nsGetNativeThis(cx, obj);

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == a) {
    return JS_TRUE;
  }

  if (JSVAL_IS_INT(id)) {
    nsIScriptContext *scriptCX = (nsIScriptContext *)JS_GetContextPrivate(cx);
    nsCOMPtr<nsIScriptSecurityManager> secMan;
    if (NS_OK != scriptCX->GetSecurityManager(getter_AddRefs(secMan))) {
      return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_SECMAN_ERR);
    }
    switch(JSVAL_TO_INT(id)) {
      case CSSRULE_TYPE:
      {
        PRBool ok = PR_FALSE;
        secMan->CheckScriptAccess(scriptCX, obj, NS_DOM_PROP_CSSRULE_TYPE, PR_FALSE, &ok);
        if (!ok) {
          return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_SECURITY_ERR);
        }
        PRUint16 prop;
        nsresult result = NS_OK;
        result = a->GetType(&prop);
        if (NS_SUCCEEDED(result)) {
          *vp = INT_TO_JSVAL(prop);
        }
        else {
          return nsJSUtils::nsReportError(cx, result);
        }
        break;
      }
      case CSSRULE_CSSTEXT:
      {
        PRBool ok = PR_FALSE;
        secMan->CheckScriptAccess(scriptCX, obj, NS_DOM_PROP_CSSRULE_CSSTEXT, PR_FALSE, &ok);
        if (!ok) {
          return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_SECURITY_ERR);
        }
        nsAutoString prop;
        nsresult result = NS_OK;
        result = a->GetCssText(prop);
        if (NS_SUCCEEDED(result)) {
          nsJSUtils::nsConvertStringToJSVal(prop, cx, vp);
        }
        else {
          return nsJSUtils::nsReportError(cx, result);
        }
        break;
      }
      case CSSRULE_SHEET:
      {
        PRBool ok = PR_FALSE;
        secMan->CheckScriptAccess(scriptCX, obj, NS_DOM_PROP_CSSRULE_SHEET, PR_FALSE, &ok);
        if (!ok) {
          return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_SECURITY_ERR);
        }
        nsIDOMCSSStyleSheet* prop;
        nsresult result = NS_OK;
        result = a->GetSheet(&prop);
        if (NS_SUCCEEDED(result)) {
          // get the js object
          nsJSUtils::nsConvertObjectToJSVal((nsISupports *)prop, cx, vp);
        }
        else {
          return nsJSUtils::nsReportError(cx, result);
        }
        break;
      }
      default:
        return nsJSUtils::nsCallJSScriptObjectGetProperty(a, cx, id, vp);
    }
  }
  else {
    return nsJSUtils::nsCallJSScriptObjectGetProperty(a, cx, id, vp);
  }

  return PR_TRUE;
}

/***********************************************************************/
//
// CSSRule Properties Setter
//
PR_STATIC_CALLBACK(JSBool)
SetCSSRuleProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsIDOMCSSRule *a = (nsIDOMCSSRule*)nsJSUtils::nsGetNativeThis(cx, obj);

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == a) {
    return JS_TRUE;
  }

  if (JSVAL_IS_INT(id)) {
    nsIScriptContext *scriptCX = (nsIScriptContext *)JS_GetContextPrivate(cx);
    nsCOMPtr<nsIScriptSecurityManager> secMan;
    if (NS_OK != scriptCX->GetSecurityManager(getter_AddRefs(secMan))) {
      return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_SECMAN_ERR);
    }
    switch(JSVAL_TO_INT(id)) {
      case CSSRULE_CSSTEXT:
      {
        PRBool ok = PR_FALSE;
        secMan->CheckScriptAccess(scriptCX, obj, NS_DOM_PROP_CSSRULE_CSSTEXT, PR_TRUE, &ok);
        if (!ok) {
          return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_SECURITY_ERR);
        }
        nsAutoString prop;
        nsJSUtils::nsConvertJSValToString(prop, cx, *vp);
      
        a->SetCssText(prop);
        
        break;
      }
      default:
        return nsJSUtils::nsCallJSScriptObjectSetProperty(a, cx, id, vp);
    }
  }
  else {
    return nsJSUtils::nsCallJSScriptObjectSetProperty(a, cx, id, vp);
  }

  return PR_TRUE;
}


//
// CSSRule finalizer
//
PR_STATIC_CALLBACK(void)
FinalizeCSSRule(JSContext *cx, JSObject *obj)
{
  nsJSUtils::nsGenericFinalize(cx, obj);
}


//
// CSSRule enumerate
//
PR_STATIC_CALLBACK(JSBool)
EnumerateCSSRule(JSContext *cx, JSObject *obj)
{
  return nsJSUtils::nsGenericEnumerate(cx, obj);
}


//
// CSSRule resolve
//
PR_STATIC_CALLBACK(JSBool)
ResolveCSSRule(JSContext *cx, JSObject *obj, jsval id)
{
  return nsJSUtils::nsGenericResolve(cx, obj, id);
}


/***********************************************************************/
//
// class for CSSRule
//
JSClass CSSRuleClass = {
  "CSSRule", 
  JSCLASS_HAS_PRIVATE | JSCLASS_PRIVATE_IS_NSISUPPORTS,
  JS_PropertyStub,
  JS_PropertyStub,
  GetCSSRuleProperty,
  SetCSSRuleProperty,
  EnumerateCSSRule,
  ResolveCSSRule,
  JS_ConvertStub,
  FinalizeCSSRule
};


//
// CSSRule class properties
//
static JSPropertySpec CSSRuleProperties[] =
{
  {"type",    CSSRULE_TYPE,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"cssText",    CSSRULE_CSSTEXT,    JSPROP_ENUMERATE},
  {"sheet",    CSSRULE_SHEET,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {0}
};


//
// CSSRule class methods
//
static JSFunctionSpec CSSRuleMethods[] = 
{
  {0}
};


//
// CSSRule constructor
//
PR_STATIC_CALLBACK(JSBool)
CSSRule(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  return JS_FALSE;
}


//
// CSSRule class initialization
//
extern "C" NS_DOM nsresult NS_InitCSSRuleClass(nsIScriptContext *aContext, void **aPrototype)
{
  JSContext *jscontext = (JSContext *)aContext->GetNativeContext();
  JSObject *proto = nsnull;
  JSObject *constructor = nsnull;
  JSObject *parent_proto = nsnull;
  JSObject *global = JS_GetGlobalObject(jscontext);
  jsval vp;

  if ((PR_TRUE != JS_LookupProperty(jscontext, global, "CSSRule", &vp)) ||
      !JSVAL_IS_OBJECT(vp) ||
      ((constructor = JSVAL_TO_OBJECT(vp)) == nsnull) ||
      (PR_TRUE != JS_LookupProperty(jscontext, JSVAL_TO_OBJECT(vp), "prototype", &vp)) || 
      !JSVAL_IS_OBJECT(vp)) {

    proto = JS_InitClass(jscontext,     // context
                         global,        // global object
                         parent_proto,  // parent proto 
                         &CSSRuleClass,      // JSClass
                         CSSRule,            // JSNative ctor
                         0,             // ctor args
                         CSSRuleProperties,  // proto props
                         CSSRuleMethods,     // proto funcs
                         nsnull,        // ctor props (static)
                         nsnull);       // ctor funcs (static)
    if (nsnull == proto) {
      return NS_ERROR_FAILURE;
    }

    if ((PR_TRUE == JS_LookupProperty(jscontext, global, "CSSRule", &vp)) &&
        JSVAL_IS_OBJECT(vp) &&
        ((constructor = JSVAL_TO_OBJECT(vp)) != nsnull)) {
      vp = INT_TO_JSVAL(nsIDOMCSSRule::UNKNOWN_RULE);
      JS_SetProperty(jscontext, constructor, "UNKNOWN_RULE", &vp);

      vp = INT_TO_JSVAL(nsIDOMCSSRule::STYLE_RULE);
      JS_SetProperty(jscontext, constructor, "STYLE_RULE", &vp);

      vp = INT_TO_JSVAL(nsIDOMCSSRule::IMPORT_RULE);
      JS_SetProperty(jscontext, constructor, "IMPORT_RULE", &vp);

      vp = INT_TO_JSVAL(nsIDOMCSSRule::MEDIA_RULE);
      JS_SetProperty(jscontext, constructor, "MEDIA_RULE", &vp);

      vp = INT_TO_JSVAL(nsIDOMCSSRule::FONT_FACE_RULE);
      JS_SetProperty(jscontext, constructor, "FONT_FACE_RULE", &vp);

      vp = INT_TO_JSVAL(nsIDOMCSSRule::PAGE_RULE);
      JS_SetProperty(jscontext, constructor, "PAGE_RULE", &vp);

    }

  }
  else if ((nsnull != constructor) && JSVAL_IS_OBJECT(vp)) {
    proto = JSVAL_TO_OBJECT(vp);
  }
  else {
    return NS_ERROR_FAILURE;
  }

  if (aPrototype) {
    *aPrototype = proto;
  }
  return NS_OK;
}


//
// Method for creating a new CSSRule JavaScript object
//
extern "C" NS_DOM nsresult NS_NewScriptCSSRule(nsIScriptContext *aContext, nsISupports *aSupports, nsISupports *aParent, void **aReturn)
{
  NS_PRECONDITION(nsnull != aContext && nsnull != aSupports && nsnull != aReturn, "null argument to NS_NewScriptCSSRule");
  JSObject *proto;
  JSObject *parent;
  nsIScriptObjectOwner *owner;
  JSContext *jscontext = (JSContext *)aContext->GetNativeContext();
  nsresult result = NS_OK;
  nsIDOMCSSRule *aCSSRule;

  if (nsnull == aParent) {
    parent = nsnull;
  }
  else if (NS_OK == aParent->QueryInterface(kIScriptObjectOwnerIID, (void**)&owner)) {
    if (NS_OK != owner->GetScriptObject(aContext, (void **)&parent)) {
      NS_RELEASE(owner);
      return NS_ERROR_FAILURE;
    }
    NS_RELEASE(owner);
  }
  else {
    return NS_ERROR_FAILURE;
  }

  if (NS_OK != NS_InitCSSRuleClass(aContext, (void **)&proto)) {
    return NS_ERROR_FAILURE;
  }

  result = aSupports->QueryInterface(kICSSRuleIID, (void **)&aCSSRule);
  if (NS_OK != result) {
    return result;
  }

  // create a js object for this class
  *aReturn = JS_NewObject(jscontext, &CSSRuleClass, proto, parent);
  if (nsnull != *aReturn) {
    // connect the native object to the js object
    JS_SetPrivate(jscontext, (JSObject *)*aReturn, aCSSRule);
  }
  else {
    NS_RELEASE(aCSSRule);
    return NS_ERROR_FAILURE; 
  }

  return NS_OK;
}
