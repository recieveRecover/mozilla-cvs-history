/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
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
/* AUTO-GENERATED. DO NOT EDIT!!! */

#include "jsapi.h"
#include "nsJSUtils.h"
#include "nscore.h"
#include "nsIScriptContext.h"
#include "nsIJSScriptObject.h"
#include "nsIScriptObjectOwner.h"
#include "nsIScriptGlobalObject.h"
#include "nsIPtr.h"
#include "nsString.h"
#include "nsIDOMEditorAppCore.h"
#include "nsIDOMWindow.h"
#include "nsIScriptNameSpaceManager.h"
#include "nsRepository.h"
#include "nsDOMCID.h"


static NS_DEFINE_IID(kIScriptObjectOwnerIID, NS_ISCRIPTOBJECTOWNER_IID);
static NS_DEFINE_IID(kIJSScriptObjectIID, NS_IJSSCRIPTOBJECT_IID);
static NS_DEFINE_IID(kIScriptGlobalObjectIID, NS_ISCRIPTGLOBALOBJECT_IID);
static NS_DEFINE_IID(kIEditorAppCoreIID, NS_IDOMEDITORAPPCORE_IID);
static NS_DEFINE_IID(kIWindowIID, NS_IDOMWINDOW_IID);

NS_DEF_PTR(nsIDOMEditorAppCore);
NS_DEF_PTR(nsIDOMWindow);


/***********************************************************************/
//
// EditorAppCore Properties Getter
//
PR_STATIC_CALLBACK(JSBool)
GetEditorAppCoreProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsIDOMEditorAppCore *a = (nsIDOMEditorAppCore*)JS_GetPrivate(cx, obj);

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == a) {
    return JS_TRUE;
  }

  if (JSVAL_IS_INT(id)) {
    switch(JSVAL_TO_INT(id)) {
      case 0:
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
// EditorAppCore Properties Setter
//
PR_STATIC_CALLBACK(JSBool)
SetEditorAppCoreProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsIDOMEditorAppCore *a = (nsIDOMEditorAppCore*)JS_GetPrivate(cx, obj);

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == a) {
    return JS_TRUE;
  }

  if (JSVAL_IS_INT(id)) {
    switch(JSVAL_TO_INT(id)) {
      case 0:
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
// EditorAppCore finalizer
//
PR_STATIC_CALLBACK(void)
FinalizeEditorAppCore(JSContext *cx, JSObject *obj)
{
  nsJSUtils::nsGenericFinalize(cx, obj);
}


//
// EditorAppCore enumerate
//
PR_STATIC_CALLBACK(JSBool)
EnumerateEditorAppCore(JSContext *cx, JSObject *obj)
{
  return nsJSUtils::nsGenericEnumerate(cx, obj);
}


//
// EditorAppCore resolve
//
PR_STATIC_CALLBACK(JSBool)
ResolveEditorAppCore(JSContext *cx, JSObject *obj, jsval id)
{
  return nsJSUtils::nsGenericResolve(cx, obj, id);
}


//
// Native method SetAttribute
//
PR_STATIC_CALLBACK(JSBool)
EditorAppCoreSetAttribute(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMEditorAppCore *nativeThis = (nsIDOMEditorAppCore*)JS_GetPrivate(cx, obj);
  nsAutoString b0;
  nsAutoString b1;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if (argc >= 2) {

    nsJSUtils::nsConvertJSValToString(b0, cx, argv[0]);

    nsJSUtils::nsConvertJSValToString(b1, cx, argv[1]);

    if (NS_OK != nativeThis->SetAttribute(b0, b1)) {
      return JS_FALSE;
    }

    *rval = JSVAL_VOID;
  }
  else {
    JS_ReportError(cx, "Function setAttribute requires 2 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method Undo
//
PR_STATIC_CALLBACK(JSBool)
EditorAppCoreUndo(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMEditorAppCore *nativeThis = (nsIDOMEditorAppCore*)JS_GetPrivate(cx, obj);

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if (argc >= 0) {

    if (NS_OK != nativeThis->Undo()) {
      return JS_FALSE;
    }

    *rval = JSVAL_VOID;
  }
  else {
    JS_ReportError(cx, "Function undo requires 0 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}

//
// Native method Redo
//
PR_STATIC_CALLBACK(JSBool)
EditorAppCoreRedo(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMEditorAppCore *nativeThis = (nsIDOMEditorAppCore*)JS_GetPrivate(cx, obj);

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if (argc >= 0) {

    if (NS_OK != nativeThis->Redo()) {
      return JS_FALSE;
    }

    *rval = JSVAL_VOID;
  }
  else {
    JS_ReportError(cx, "Function redo requires 0 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method Exit
//
PR_STATIC_CALLBACK(JSBool)
EditorAppCoreExit(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMEditorAppCore *nativeThis = (nsIDOMEditorAppCore*)JS_GetPrivate(cx, obj);

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if (argc >= 0) {

    if (NS_OK != nativeThis->Exit()) {
      return JS_FALSE;
    }

    *rval = JSVAL_VOID;
  }
  else {
    JS_ReportError(cx, "Function exit requires 0 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method SetToolbarWindow
//
PR_STATIC_CALLBACK(JSBool)
EditorAppCoreSetToolbarWindow(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMEditorAppCore *nativeThis = (nsIDOMEditorAppCore*)JS_GetPrivate(cx, obj);
  nsIDOMWindowPtr b0;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if (argc >= 1) {

    if (JS_FALSE == nsJSUtils::nsConvertJSValToObject((nsISupports **)&b0,
                                           kIWindowIID,
                                           "Window",
                                           cx,
                                           argv[0])) {
      return JS_FALSE;
    }

    if (NS_OK != nativeThis->SetToolbarWindow(b0)) {
      return JS_FALSE;
    }

    *rval = JSVAL_VOID;
  }
  else {
    JS_ReportError(cx, "Function setToolbarWindow requires 1 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method SetContentWindow
//
PR_STATIC_CALLBACK(JSBool)
EditorAppCoreSetContentWindow(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMEditorAppCore *nativeThis = (nsIDOMEditorAppCore*)JS_GetPrivate(cx, obj);
  nsIDOMWindowPtr b0;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if (argc >= 1) {

    if (JS_FALSE == nsJSUtils::nsConvertJSValToObject((nsISupports **)&b0,
                                           kIWindowIID,
                                           "Window",
                                           cx,
                                           argv[0])) {
      return JS_FALSE;
    }

    if (NS_OK != nativeThis->SetContentWindow(b0)) {
      return JS_FALSE;
    }

    *rval = JSVAL_VOID;
  }
  else {
    JS_ReportError(cx, "Function setContentWindow requires 1 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method SetWebShellWindow
//
PR_STATIC_CALLBACK(JSBool)
EditorAppCoreSetWebShellWindow(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMEditorAppCore *nativeThis = (nsIDOMEditorAppCore*)JS_GetPrivate(cx, obj);
  nsIDOMWindowPtr b0;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if (argc >= 1) {

    if (JS_FALSE == nsJSUtils::nsConvertJSValToObject((nsISupports **)&b0,
                                           kIWindowIID,
                                           "Window",
                                           cx,
                                           argv[0])) {
      return JS_FALSE;
    }

    if (NS_OK != nativeThis->SetWebShellWindow(b0)) {
      return JS_FALSE;
    }

    *rval = JSVAL_VOID;
  }
  else {
    JS_ReportError(cx, "Function setWebShellWindow requires 1 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


/***********************************************************************/
//
// class for EditorAppCore
//
JSClass EditorAppCoreClass = {
  "EditorAppCore", 
  JSCLASS_HAS_PRIVATE,
  JS_PropertyStub,
  JS_PropertyStub,
  GetEditorAppCoreProperty,
  SetEditorAppCoreProperty,
  EnumerateEditorAppCore,
  ResolveEditorAppCore,
  JS_ConvertStub,
  FinalizeEditorAppCore
};


//
// EditorAppCore class properties
//
static JSPropertySpec EditorAppCoreProperties[] =
{
  {0}
};


//
// EditorAppCore class methods
//
static JSFunctionSpec EditorAppCoreMethods[] = 
{
  {"setAttribute",          EditorAppCoreSetAttribute,     2},
  {"undo",          EditorAppCoreUndo,     0},
  {"redo",          EditorAppCoreRedo,     0},
  {"exit",          EditorAppCoreExit,     0},
  {"setToolbarWindow",          EditorAppCoreSetToolbarWindow,     1},
  {"setContentWindow",          EditorAppCoreSetContentWindow,     1},
  {"setWebShellWindow",          EditorAppCoreSetWebShellWindow,     1},
  {0}
};


//
// EditorAppCore constructor
//
PR_STATIC_CALLBACK(JSBool)
EditorAppCore(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsresult result;
  nsIID classID;
  nsIScriptContext* context = (nsIScriptContext*)JS_GetContextPrivate(cx);
  nsIScriptNameSpaceManager* manager;
  nsIDOMEditorAppCore *nativeThis;
  nsIScriptObjectOwner *owner = nsnull;

  static NS_DEFINE_IID(kIDOMEditorAppCoreIID, NS_IDOMEDITORAPPCORE_IID);

  result = context->GetNameSpaceManager(&manager);
  if (NS_OK != result) {
    return JS_FALSE;
  }

  result = manager->LookupName("EditorAppCore", PR_TRUE, classID);
  NS_RELEASE(manager);
  if (NS_OK != result) {
    return JS_FALSE;
  }

  result = nsRepository::CreateInstance(classID,
                                        nsnull,
                                        kIDOMEditorAppCoreIID,
                                        (void **)&nativeThis);
  if (NS_OK != result) {
    return JS_FALSE;
  }

  // XXX We should be calling Init() on the instance

  result = nativeThis->QueryInterface(kIScriptObjectOwnerIID, (void **)&owner);
  if (NS_OK != result) {
    NS_RELEASE(nativeThis);
    return JS_FALSE;
  }

  owner->SetScriptObject((void *)obj);
  JS_SetPrivate(cx, obj, nativeThis);

  NS_RELEASE(owner);
  return JS_TRUE;
}

//
// EditorAppCore class initialization
//
extern "C" NS_DOM nsresult NS_InitEditorAppCoreClass(nsIScriptContext *aContext, void **aPrototype)
{
  JSContext *jscontext = (JSContext *)aContext->GetNativeContext();
  JSObject *proto = nsnull;
  JSObject *constructor = nsnull;
  JSObject *parent_proto = nsnull;
  JSObject *global = JS_GetGlobalObject(jscontext);
  jsval vp;

  if ((PR_TRUE != JS_LookupProperty(jscontext, global, "EditorAppCore", &vp)) ||
      !JSVAL_IS_OBJECT(vp) ||
      ((constructor = JSVAL_TO_OBJECT(vp)) == nsnull) ||
      (PR_TRUE != JS_LookupProperty(jscontext, JSVAL_TO_OBJECT(vp), "prototype", &vp)) || 
      !JSVAL_IS_OBJECT(vp)) {

    if (NS_OK != NS_InitBaseAppCoreClass(aContext, (void **)&parent_proto)) {
      return NS_ERROR_FAILURE;
    }
    proto = JS_InitClass(jscontext,     // context
                         global,        // global object
                         parent_proto,  // parent proto 
                         &EditorAppCoreClass,      // JSClass
                         EditorAppCore,            // JSNative ctor
                         0,             // ctor args
                         EditorAppCoreProperties,  // proto props
                         EditorAppCoreMethods,     // proto funcs
                         nsnull,        // ctor props (static)
                         nsnull);       // ctor funcs (static)
    if (nsnull == proto) {
      return NS_ERROR_FAILURE;
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
// Method for creating a new EditorAppCore JavaScript object
//
extern "C" NS_DOM nsresult NS_NewScriptEditorAppCore(nsIScriptContext *aContext, nsISupports *aSupports, nsISupports *aParent, void **aReturn)
{
  NS_PRECONDITION(nsnull != aContext && nsnull != aSupports && nsnull != aReturn, "null argument to NS_NewScriptEditorAppCore");
  JSObject *proto;
  JSObject *parent;
  nsIScriptObjectOwner *owner;
  JSContext *jscontext = (JSContext *)aContext->GetNativeContext();
  nsresult result = NS_OK;
  nsIDOMEditorAppCore *aEditorAppCore;

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

  if (NS_OK != NS_InitEditorAppCoreClass(aContext, (void **)&proto)) {
    return NS_ERROR_FAILURE;
  }

  result = aSupports->QueryInterface(kIEditorAppCoreIID, (void **)&aEditorAppCore);
  if (NS_OK != result) {
    return result;
  }

  // create a js object for this class
  *aReturn = JS_NewObject(jscontext, &EditorAppCoreClass, proto, parent);
  if (nsnull != *aReturn) {
    // connect the native object to the js object
    JS_SetPrivate(jscontext, (JSObject *)*aReturn, aEditorAppCore);
  }
  else {
    NS_RELEASE(aEditorAppCore);
    return NS_ERROR_FAILURE; 
  }

  return NS_OK;
}
