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

#include "jsapi.h"
#include "nsJSUtils.h"
#include "nscore.h"
#include "nsIScriptContext.h"

#include "nsString.h"
#include "nsInstall.h"

#include "nsIDOMInstallVersion.h"

#include "stdio.h"

#ifdef _WINDOWS
#include "nsJSWinReg.h"
#include "nsJSWinProfile.h"

extern JSClass WinRegClass;
extern JSClass WinProfileClass;
#endif

//
// Install property ids
//
enum Install_slots 
{
  INSTALL_USERPACKAGENAME = -1,
  INSTALL_REGPACKAGENAME  = -2,
  INSTALL_SILENT          = -3,
  INSTALL_JARFILE         = -4,
  INSTALL_FORCE           = -5,
  INSTALL_ARGUMENTS       = -6,
  INSTALL_URL             = -7,
  INSTALL_STATUSSENT      = -8,
  INSTALL_INSTALL         = -9
};

/***********************************************************************/
//
// Install Properties Getter
//
PR_STATIC_CALLBACK(JSBool)
GetInstallProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsInstall *a = (nsInstall*)JS_GetPrivate(cx, obj);

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == a) {
    return JS_TRUE;
  }

  if (JSVAL_IS_INT(id)) 
  {
    switch(JSVAL_TO_INT(id)) {
      case INSTALL_USERPACKAGENAME:
      {
        nsAutoString prop;
        if (NS_OK == a->GetUserPackageName(prop)) 
        {
            *vp = STRING_TO_JSVAL( JS_NewUCStringCopyN(cx, prop.GetUnicode(), prop.Length()) );
        }
        else 
        {
          return JS_TRUE;
        }
        break;
      }
      case INSTALL_REGPACKAGENAME:
      {
        nsAutoString prop;
        if (NS_OK == a->GetRegPackageName(prop)) 
        {
          *vp = STRING_TO_JSVAL( JS_NewUCStringCopyN(cx, prop.GetUnicode(), prop.Length()) );
        }
        else 
        {
          return JS_TRUE;
        }
        break;
      }
      case INSTALL_JARFILE:
      {
        nsAutoString prop;
        
        a->GetJarFileLocation(prop);
        *vp = STRING_TO_JSVAL( JS_NewUCStringCopyN(cx, prop.GetUnicode(), prop.Length()) );
        
        break;
      }

      case INSTALL_ARGUMENTS:
      {
        nsString prop;
        
        a->GetInstallArguments(prop); 
        *vp = STRING_TO_JSVAL( JS_NewUCStringCopyN(cx, prop.GetUnicode(), prop.Length()) );
        
        break;
      }
        
      case INSTALL_URL:
      {
        nsString prop;
        
        a->GetInstallURL(prop); 
        *vp = STRING_TO_JSVAL( JS_NewUCStringCopyN(cx, prop.GetUnicode(), prop.Length()) );
        
        break;
      }

      case INSTALL_STATUSSENT:
          *vp = BOOLEAN_TO_JSVAL( a->GetStatusSent() );
          break;

      case INSTALL_INSTALL:
          *vp = OBJECT_TO_JSVAL(obj);
          break;
        
      default:
        return JS_TRUE;
    }
  }
  else {
    return JS_TRUE;
  }

  return PR_TRUE;
}

/***********************************************************************/
//
// Install Properties Setter
//
PR_STATIC_CALLBACK(JSBool)
SetInstallProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsInstall *a = (nsInstall*)JS_GetPrivate(cx, obj);

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == a) {
    return JS_TRUE;
  }

  if (JSVAL_IS_INT(id)) {
    switch(JSVAL_TO_INT(id)) {
      case 0:
      default:
          return JS_TRUE;
    }
  }
  else {
    return JS_TRUE;
  }

  return PR_TRUE;
}


static void PR_CALLBACK FinalizeInstall(JSContext *cx, JSObject *obj)
{
    nsInstall *nativeThis = (nsInstall*)JS_GetPrivate(cx, obj);
    delete nativeThis;
}

/***********************************************************************/
/*                        JS Utilities                                 */
/***********************************************************************/
void ConvertJSValToStr(nsString&  aString,
                      JSContext* aContext,
                      jsval      aValue)
{
  JSString *jsstring;
  jschar* chars=nsnull;

  if((jsstring = JS_ValueToString(aContext, aValue)) != nsnull)
  {
    aString.SetString(JS_GetStringChars(jsstring));
    if (aString.EqualsIgnoreCase("null"))
    {
        aString.SetString("");
    }
  }
  else
  {
    aString.SetString("");
  }
}

void ConvertStrToJSVal(const nsString& aProp,
                      JSContext* aContext,
                      jsval* aReturn)
{
  JSString *jsstring = JS_NewUCStringCopyN(aContext, aProp.GetUnicode(), aProp.Length());
  // set the return value
  *aReturn = STRING_TO_JSVAL(jsstring);
}

PRBool ConvertJSValToBool(PRBool* aProp,
                         JSContext* aContext,
                         jsval aValue)
{
  JSBool temp;
  if(JSVAL_IS_BOOLEAN(aValue) && JS_ValueToBoolean(aContext, aValue, &temp))
  {
    *aProp = (PRBool)temp;
  }
  else
  {
    JS_ReportError(aContext, "Parameter must be a boolean");
    return JS_FALSE;
  }

  return JS_TRUE;
}

PRBool ConvertJSValToObj(nsISupports** aSupports,
                        REFNSIID aIID,
                        const nsString& aTypeName,
                        JSContext* aContext,
                        jsval aValue)
{
  if (JSVAL_IS_NULL(aValue)) {
    *aSupports = nsnull;
  }
  else if (JSVAL_IS_OBJECT(aValue)) {
    JSObject* jsobj = JSVAL_TO_OBJECT(aValue); 
    JSClass* jsclass = JS_GetClass(aContext, jsobj);
    if ((nsnull != jsclass) && (jsclass->flags & JSCLASS_HAS_PRIVATE)) {
      nsISupports *supports = (nsISupports *)JS_GetPrivate(aContext, jsobj);
      if (NS_OK != supports->QueryInterface(aIID, (void **)aSupports)) {
        char buf[128];
        char typeName[128];
        aTypeName.ToCString(typeName, sizeof(typeName));
        sprintf(buf, "Parameter must of type %s", typeName);
        JS_ReportError(aContext, buf);
        return JS_FALSE;
      }
    }
    else {
      JS_ReportError(aContext, "Parameter isn't an object");
      return JS_FALSE;
    }
  }
  else {
    JS_ReportError(aContext, "Parameter must be an object");
    return JS_FALSE;
  }

  return JS_TRUE;
}


void ConvertJSvalToVersionString(nsString& versionString, JSContext* cx, jsval argument)
{
    versionString = "";
    
    if( JSVAL_IS_OBJECT(argument) )
    {
        if(!JSVAL_IS_NULL(argument))
        {
            JSObject* jsobj   = JSVAL_TO_OBJECT(argument);
            JSClass*  jsclass = JS_GetClass(cx, jsobj);

            if ((nsnull != jsclass) && (jsclass->flags & JSCLASS_HAS_PRIVATE)) 
            {
                nsIDOMInstallVersion* version = (nsIDOMInstallVersion*)JS_GetPrivate(cx, jsobj);
                version->ToString(versionString);
            }
        }
    }
    else
    {
        ConvertJSValToStr(versionString, cx, argument);
    }
}
/***********************************************************************/
/***********************************************************************/


//
// Native method AbortInstall
//
PR_STATIC_CALLBACK(JSBool)
InstallAbortInstall(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsInstall *nativeThis = (nsInstall*)JS_GetPrivate(cx, obj);

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if(argc >= 0)
  {
    //  public int AbortInstall(void);

    if(NS_OK != nativeThis->AbortInstall())
    {
      return JS_FALSE;
    }

    *rval = JSVAL_VOID;
  }
  else
  {
    JS_ReportError(cx, "Function AbortInstall requires 0 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method AddDirectory
//
PR_STATIC_CALLBACK(JSBool)
InstallAddDirectory(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsInstall *nativeThis = (nsInstall*)JS_GetPrivate(cx, obj);
  PRInt32 nativeRet;
  nsAutoString b0;
  nsAutoString b1;
  nsAutoString b2;
  nsAutoString b3;
  nsAutoString b4;
  PRBool b5;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if (argc == 1)                             
  {
    // public int AddDirectory (String jarSourcePath)

    ConvertJSValToStr(b0, cx, argv[0]);
       
    if(NS_OK != nativeThis->AddDirectory(b0, &nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else if (argc == 4)                             
  {
    //  public int AddDirectory ( String registryName,
    //                            String jarSourcePath,
    //                            String localDirSpec,
    //                            String relativeLocalPath); 

    ConvertJSValToStr(b0, cx, argv[0]);
    ConvertJSValToStr(b1, cx, argv[1]);
    ConvertJSValToStr(b2, cx, argv[2]);
    ConvertJSValToStr(b3, cx, argv[3]);

    if(NS_OK != nativeThis->AddDirectory(b0, b1, b2, b3, &nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else if (argc == 5)                             
  {
    //  public int AddDirectory ( String registryName,
    //                            String version,  --OR-- VersionInfo version
    //                            String jarSourcePath,
    //                            Object localDirSpec,
    //                            String relativeLocalPath); 

    ConvertJSValToStr(b0, cx, argv[0]);
    ConvertJSvalToVersionString(b1, cx, argv[1]);
    ConvertJSValToStr(b2, cx, argv[2]);
    ConvertJSValToStr(b3, cx, argv[3]);
    ConvertJSValToStr(b4, cx, argv[4]);
    
    if(NS_OK != nativeThis->AddDirectory(b0, b1, b2, b3, b4, &nativeRet))
    {
        return JS_FALSE;
    }
    
    *rval = INT_TO_JSVAL(nativeRet);

  }
  else if (argc == 6)
  {
     //   public int AddDirectory (  String registryName,
     //                              String version,        --OR--     VersionInfo version, 
     //                              String jarSourcePath,
     //                              Object localDirSpec,
     //                              String relativeLocalPath,
     //                              Boolean forceUpdate);  

    ConvertJSValToStr(b0, cx, argv[0]);
    ConvertJSvalToVersionString(b1, cx, argv[1]);
    ConvertJSValToStr(b2, cx, argv[2]);
    ConvertJSValToStr(b3, cx, argv[3]);
    ConvertJSValToStr(b4, cx, argv[4]);

    if(!ConvertJSValToBool(&b5, cx, argv[5]))
    {
      return JS_FALSE;
    }

    if(NS_OK != nativeThis->AddDirectory(b0, b1, b2, b3, b4, b5, &nativeRet))
    {
        return JS_FALSE;
    }
    
    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "Install.AddDirectory() parameters error");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method AddSubcomponent
//
PR_STATIC_CALLBACK(JSBool)
InstallAddSubcomponent(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsInstall *nativeThis = (nsInstall*)JS_GetPrivate(cx, obj);
  PRInt32 nativeRet;
  nsAutoString b0;
  nsAutoString b1;
  nsAutoString b2;
  nsAutoString b3;
  nsAutoString b4;
  PRBool b5;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if(argc >= 6)
  {
    //  public int AddSubcomponent ( String registryName,
    //                               String version,        --OR--     VersionInfo version, 
    //                               String jarSourcePath,
    //                               Object localDirSpec,
    //                               String relativeLocalPath,
    //                               Boolean forceUpdate); 

    ConvertJSValToStr(b0, cx, argv[0]);
    ConvertJSvalToVersionString(b1, cx, argv[1]);
    ConvertJSValToStr(b2, cx, argv[2]);
    ConvertJSValToStr(b3, cx, argv[3]);
    ConvertJSValToStr(b4, cx, argv[4]);

    if(!ConvertJSValToBool(&b5, cx, argv[5]))
    {
      return JS_FALSE;
    }

    if(NS_OK != nativeThis->AddSubcomponent(b0, b1, b2, b3, b4, b5, &nativeRet))
    {
    return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else if(argc >= 5)
  {
    //  public int AddSubcomponent ( String registryName,
    //                               String version,  --OR-- VersionInfo version
    //                               String jarSourcePath,
    //                               Object localDirSpec,
    //                               String relativeLocalPath);

    ConvertJSValToStr(b0, cx, argv[0]);
    ConvertJSvalToVersionString(b1, cx, argv[1]);
    ConvertJSValToStr(b2, cx, argv[2]);
    ConvertJSValToStr(b3, cx, argv[3]);
    ConvertJSValToStr(b4, cx, argv[4]);

    if(NS_OK != nativeThis->AddSubcomponent(b0, b1, b2, b3, b4, &nativeRet))
    {
        return JS_FALSE;
    }
    
    *rval = INT_TO_JSVAL(nativeRet);
  }
  else if(argc >= 4)
  {
    //  public int AddSubcomponent ( String registryName,
    //                               String jarSourcePath,
    //                               Object localDirSpec,
    //                               String relativeLocalPath);

    ConvertJSValToStr(b0, cx, argv[0]);
    ConvertJSValToStr(b1, cx, argv[1]);
    ConvertJSValToStr(b2, cx, argv[2]);
    ConvertJSValToStr(b3, cx, argv[3]);

    if(NS_OK != nativeThis->AddSubcomponent(b0, b1, b2, b3, &nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else if(argc >= 1)
  {
    //  public int AddSubcomponent ( String jarSourcePath);

    ConvertJSValToStr(b0, cx, argv[0]);

    if(NS_OK != nativeThis->AddSubcomponent(b0, &nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "Function AddSubcomponent requires 6 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method DeleteComponent
//
PR_STATIC_CALLBACK(JSBool)
InstallDeleteComponent(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsInstall *nativeThis = (nsInstall*)JS_GetPrivate(cx, obj);
  PRInt32 nativeRet;
  nsAutoString b0;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if(argc >= 1)
  {
    //  public int DeleteComponent ( String registryName);

    ConvertJSValToStr(b0, cx, argv[0]);

    if(NS_OK != nativeThis->DeleteComponent(b0, &nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "Function DeleteComponent requires 1 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method DeleteFile
//
PR_STATIC_CALLBACK(JSBool)
InstallDeleteFile(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsInstall *nativeThis = (nsInstall*)JS_GetPrivate(cx, obj);
  PRInt32 nativeRet;
  nsAutoString b0;
  nsAutoString b1;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if(argc >= 2)
  {
    //  public int DeleteFile ( Object folder,
    //                          String relativeFileName);

    ConvertJSValToStr(b0, cx, argv[0]);
    ConvertJSValToStr(b1, cx, argv[1]);

    if(NS_OK != nativeThis->DeleteFile(b0, b1, &nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "Function DeleteFile requires 2 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method DiskSpaceAvailable
//
PR_STATIC_CALLBACK(JSBool)
InstallDiskSpaceAvailable(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsInstall *nativeThis = (nsInstall*)JS_GetPrivate(cx, obj);
  PRInt64 nativeRet;
  nsAutoString b0;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if(argc >= 1)
  {
    //  public int DiskSpaceAvailable ( Object localDirSpec);

    ConvertJSValToStr(b0, cx, argv[0]);

    if(NS_OK != nativeThis->DiskSpaceAvailable(b0, &nativeRet))
    {
      return JS_FALSE;
    }

    double d;
    
    LL_L2D(d, nativeRet);
    JS_NewDoubleValue( cx, d, rval );
    
  }
  else
  {
    JS_ReportError(cx, "Function DiskSpaceAvailable requires 1 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method Execute
//
PR_STATIC_CALLBACK(JSBool)
InstallExecute(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsInstall *nativeThis = (nsInstall*)JS_GetPrivate(cx, obj);
  PRInt32 nativeRet;
  nsAutoString b0;
  nsAutoString b1;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if(argc >= 2)
  {
    //  public int Execute ( String jarSourcePath,
    //                       String args);

    ConvertJSValToStr(b0, cx, argv[0]);
    ConvertJSValToStr(b1, cx, argv[1]);

    if(NS_OK != nativeThis->Execute(b0, b1, &nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else if(argc >= 1)
  {
    //  public int Execute ( String jarSourcePath);

    ConvertJSValToStr(b0, cx, argv[0]);

    if(NS_OK != nativeThis->Execute(b0, &nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "Function Execute requires 2 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method FinalizeInstall
//
PR_STATIC_CALLBACK(JSBool)
InstallFinalizeInstall(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsInstall *nativeThis = (nsInstall*)JS_GetPrivate(cx, obj);
  PRInt32 nativeRet;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if(argc >= 0)
  {
    //  public int FinalizeInstall (void);

    if(NS_OK != nativeThis->FinalizeInstall(&nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "Function FinalizeInstall requires 0 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method Gestalt
//
PR_STATIC_CALLBACK(JSBool)
InstallGestalt(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsInstall *nativeThis = (nsInstall*)JS_GetPrivate(cx, obj);
  PRInt32 nativeRet;
  nsAutoString b0;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if(argc >= 1)
  {
    //  public int Gestalt ( String selector,
    //                       long   *response);

    ConvertJSValToStr(b0, cx, argv[0]);

    if(NS_OK != nativeThis->Gestalt(b0, &nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "Function Gestalt requires 1 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method GetComponentFolder
//
PR_STATIC_CALLBACK(JSBool)
InstallGetComponentFolder(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsInstall *nativeThis = (nsInstall*)JS_GetPrivate(cx, obj);
  nsString* nativeRet;
  nsAutoString b0;
  nsAutoString b1;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if(argc >= 2)
  {
    //  public int GetComponentFolder ( String registryName,
    //                                  String subDirectory);

    ConvertJSValToStr(b0, cx, argv[0]);
    ConvertJSValToStr(b1, cx, argv[1]);

    if(NS_OK != nativeThis->GetComponentFolder(b0, b1, &nativeRet))
    {
      return JS_FALSE;
    }

    if(nsnull == nativeRet)
      *rval = JSVAL_NULL;
    else
      ConvertStrToJSVal(*nativeRet, cx, rval);
  }
  else if(argc >= 1)
  {
    //  public int GetComponentFolder ( String registryName);

    ConvertJSValToStr(b0, cx, argv[0]);

    if(NS_OK != nativeThis->GetComponentFolder(b0, &nativeRet))
    {
      return JS_FALSE;
    }

    if(nsnull == nativeRet)
      *rval = JSVAL_NULL;
    else
      ConvertStrToJSVal(*nativeRet, cx, rval);
  }
  else
  {
    JS_ReportError(cx, "Function GetComponentFolder requires 2 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method GetFolder
//
PR_STATIC_CALLBACK(JSBool)
InstallGetFolder(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsInstall *nativeThis = (nsInstall*)JS_GetPrivate(cx, obj);
  nsString* nativeRet;
  nsAutoString b0;
  nsAutoString b1;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if(argc >= 2)
  {
    //  public int GetFolder ( String folderName, --OR-- Object localDirSpec,
    //                         String subDirectory);

    ConvertJSValToStr(b0, cx, argv[0]);
    ConvertJSValToStr(b1, cx, argv[1]);

    if(NS_OK != nativeThis->GetFolder(b0, b1, &nativeRet))
    {
      return JS_FALSE;
    }

    if(nsnull == nativeRet)
      *rval = JSVAL_NULL;
    else
      ConvertStrToJSVal(*nativeRet, cx, rval);
  }
  else if(argc >= 1)
  {
    //  public int GetFolder ( String folderName);

    ConvertJSValToStr(b0, cx, argv[0]);

    if(NS_OK != nativeThis->GetFolder(b0, &nativeRet))
    {
      return JS_FALSE;
    }

    if(nsnull == nativeRet)
      *rval = JSVAL_NULL;
    else
      ConvertStrToJSVal(*nativeRet, cx, rval);
  }
  else
  {
    JS_ReportError(cx, "Function GetFolder requires 2 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method GetLastError
//
PR_STATIC_CALLBACK(JSBool)
InstallGetLastError(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsInstall *nativeThis = (nsInstall*)JS_GetPrivate(cx, obj);
  PRInt32 nativeRet;


  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if(argc >= 0)
  {
    //  public int GetLastError (void);

    if(NS_OK != nativeThis->GetLastError(&nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "Function GetLastError requires 0 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method GetWinProfile
//
PR_STATIC_CALLBACK(JSBool)
InstallGetWinProfile(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  *rval = JSVAL_NULL;

#ifdef _WINDOWS
  nsInstall *nativeThis = (nsInstall*)JS_GetPrivate(cx, obj);
  nsAutoString b0;
  nsAutoString b1;
  
  // If there's no private data, this must be the prototype, so ignore
  if(nsnull == nativeThis)
  {
    return JS_TRUE;
  }

  if(argc >= 2)
  {
    //  public int GetWinProfile (Object folder,
    //                            String file);

    ConvertJSValToStr(b0, cx, argv[0]);
    ConvertJSValToStr(b1, cx, argv[1]);

    if(NS_OK != nativeThis->GetWinProfile(b0, b1, cx, &WinProfileClass, rval))
    {
      return JS_FALSE;
    }
  }
  else
  {
    JS_ReportError(cx, "Function GetWinProfile requires 0 parameters");
    return JS_FALSE;
  }
#endif

  return JS_TRUE;
}


//
// Native method GetWinRegistry
//
PR_STATIC_CALLBACK(JSBool)
InstallGetWinRegistry(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  *rval = JSVAL_NULL;

#ifdef _WINDOWS
  
  nsInstall *nativeThis = (nsInstall*)JS_GetPrivate(cx, obj);
  JSBool rBool = JS_FALSE;

  // If there's no private data, this must be the prototype, so ignore
  if(nsnull == nativeThis)
  {
    return JS_TRUE;
  }

  if(argc >= 0)
  {
    //  public int GetWinRegistry (void);
    if(NS_OK != nativeThis->GetWinRegistry(cx, &WinRegClass, rval))
    {
      return JS_FALSE;
    }
  }
  else
  {
    JS_ReportError(cx, "Function GetWinRegistry requires 0 parameters");
    return JS_FALSE;
  }
#endif

  return JS_TRUE;
}


//
// Native method LoadResources
//
PR_STATIC_CALLBACK(JSBool)
InstallLoadResources(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    nsInstall *nativeThis = (nsInstall*)JS_GetPrivate(cx, obj);
    nsAutoString b0;

    *rval = JSVAL_NULL;

    // If there's no private data, this must be the prototype, so ignore
    if (nsnull == nativeThis) {
        return JS_TRUE;
    }

    if (argc >= 1)
    {
	    ConvertJSValToStr(b0, cx, argv[0]);
	    if (NS_OK != nativeThis->LoadResources(cx, b0, rval))
	    {
		    return JS_FALSE;
	    }
    }
    else
    {
        JS_ReportError(cx, "Function LoadResources requires 1 parameter");
	    return JS_FALSE;
    }

    return JS_TRUE;
}


//
// Native method Patch
//
PR_STATIC_CALLBACK(JSBool)
InstallPatch(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsInstall *nativeThis = (nsInstall*)JS_GetPrivate(cx, obj);
  PRInt32 nativeRet;
  nsAutoString b0;
  nsAutoString b1;
  nsAutoString b2;
  nsAutoString b3;
  nsAutoString b4;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if(argc >= 5)
  {
    //  public int Patch (String registryName,
    //                    String version,        --OR-- VersionInfo version,
    //                    String jarSourcePath,
    //                    Object localDirSpec,
    //                    String relativeLocalPath);

    ConvertJSValToStr(b0, cx, argv[0]);
    ConvertJSvalToVersionString(b1, cx, argv[1]);
    ConvertJSValToStr(b2, cx, argv[2]);
    ConvertJSValToStr(b3, cx, argv[3]);
    ConvertJSValToStr(b4, cx, argv[4]);

   if(NS_OK != nativeThis->Patch(b0, b1, b2, b3, b4, &nativeRet))
    {
        return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else if(argc >= 4)
  {
    //  public int Patch (String registryName,
    //                    String jarSourcePath,
    //                    Object localDirSpec,
    //                    String relativeLocalPath);

    ConvertJSValToStr(b0, cx, argv[0]);
    ConvertJSValToStr(b1, cx, argv[1]);
    ConvertJSValToStr(b2, cx, argv[2]);
    ConvertJSValToStr(b3, cx, argv[3]);

    if(NS_OK != nativeThis->Patch(b0, b1, b2, b3, &nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "Function Patch requires 5 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method ResetError
//
PR_STATIC_CALLBACK(JSBool)
InstallResetError(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsInstall *nativeThis = (nsInstall*)JS_GetPrivate(cx, obj);

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if(argc >= 0)
  {
    //  public int ResetError (void);

    if(NS_OK != nativeThis->ResetError())
    {
      return JS_FALSE;
    }

    *rval = JSVAL_VOID;
  }
  else
  {
    JS_ReportError(cx, "Function ResetError requires 0 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method SetPackageFolder
//
PR_STATIC_CALLBACK(JSBool)
InstallSetPackageFolder(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsInstall *nativeThis = (nsInstall*)JS_GetPrivate(cx, obj);
  nsAutoString b0;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if(argc >= 1)
  {
    //  public int SetPackageFolder (Object folder);

    ConvertJSValToStr(b0, cx, argv[0]);

    if(NS_OK != nativeThis->SetPackageFolder(b0))
    {
      return JS_FALSE;
    }

    *rval = JSVAL_VOID;
  }
  else
  {
    JS_ReportError(cx, "Function SetPackageFolder requires 1 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}

//
// Native method StartInstall
//
PR_STATIC_CALLBACK(JSBool)
InstallStartInstall(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsInstall *nativeThis = (nsInstall*)JS_GetPrivate(cx, obj);
  PRInt32 nativeRet;
  nsAutoString b0;
  nsAutoString b1;
  nsAutoString b2;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if(argc == 3 || argc == 4)
  {
    //  public int StartInstall (String userPackageName,
    //                           String package,
    //                           String version); --OR-- VersionInfo version
    //                           flags?
    ConvertJSValToStr(b0, cx, argv[0]);
    ConvertJSValToStr(b1, cx, argv[1]);
    ConvertJSvalToVersionString(b2, cx, argv[2]);

    if(NS_OK != nativeThis->StartInstall(b0, b1, b2, &nativeRet))
    {
        return JS_FALSE;
    }


    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "Function StartInstall requires 3 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method Uninstall
//
PR_STATIC_CALLBACK(JSBool)
InstallUninstall(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsInstall *nativeThis = (nsInstall*)JS_GetPrivate(cx, obj);
  PRInt32 nativeRet;
  nsAutoString b0;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if(argc >= 1)
  {
    //  public int Uninstall (String packageName);

    ConvertJSValToStr(b0, cx, argv[0]);

    if(NS_OK != nativeThis->Uninstall(b0, &nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "Function Uninstall requires 1 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


/*START HACK FOR DEBUGGING UNTIL ALERTS WORK*/

PR_STATIC_CALLBACK(JSBool)
InstallTRACE(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsAutoString b0;
  
  ConvertJSValToStr(b0, cx, argv[0]);
  
  char *tempStr;
  tempStr = b0.ToNewCString();
  printf("Install:\t%s\n", tempStr);

  Recycle(tempStr);

  return JS_TRUE;
}

/*END HACK FOR DEBUGGING UNTIL ALERTS WORK*/

//
// Native method DirCreate
//
PR_STATIC_CALLBACK(JSBool)
InstallFileOpDirCreate(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsInstall *nativeThis = (nsInstall*)JS_GetPrivate(cx, obj);
  PRInt32 nativeRet;
  nsAutoString b0;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if(nsnull == nativeThis)
  {
    return JS_TRUE;
  }

  if(argc >= 1)
  {
    //  public int DirCreate (String aNativeFolderPath);

    ConvertJSValToStr(b0, cx, argv[0]);
    nsFileSpec fsB0(b0);

    if(NS_OK != nativeThis->FileOpDirCreate(fsB0, &nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "Function DirCreate requires 1 parameter");
    return JS_FALSE;
  }

  return JS_TRUE;
}

//
// Native method DirGetParent
//
PR_STATIC_CALLBACK(JSBool)
InstallFileOpDirGetParent(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsInstall*   nativeThis = (nsInstall*)JS_GetPrivate(cx, obj);
  nsFileSpec   nativeRet;
  nsAutoString b0;
  nsString     nativeRetNSStr;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if(nsnull == nativeThis)
  {
    return JS_TRUE;
  }

  if(argc >= 1)
  {
    //  public int DirGetParent (String NativeFolderPath);

    ConvertJSValToStr(b0, cx, argv[0]);
    nsFileSpec fsB0(b0);

    if(NS_OK != nativeThis->FileOpDirGetParent(fsB0, &nativeRet))
    {
      return JS_FALSE;
    }

    nativeRetNSStr = nativeRet.GetNativePathCString();
    *rval = STRING_TO_JSVAL(JS_NewUCStringCopyN(cx, nativeRetNSStr.GetUnicode(), nativeRetNSStr.Length()));
  }
  else
  {
    JS_ReportError(cx, "Function DirGetParent requires 1 parameter");
    return JS_FALSE;
  }

  return JS_TRUE;
}

//
// Native method DirRemove
//
PR_STATIC_CALLBACK(JSBool)
InstallFileOpDirRemove(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsInstall *nativeThis = (nsInstall*)JS_GetPrivate(cx, obj);
  PRInt32 nativeRet;
  nsAutoString b0;
  PRBool       b1;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if(nsnull == nativeThis)
  {
    return JS_TRUE;
  }

  if(argc >= 2)
  {
    //  public int DirRemove (String aNativeFolderPath,
    //                        Bool   aRecursive);

    ConvertJSValToStr(b0, cx, argv[0]);
    nsFileSpec fsB0(b0);
    if(!ConvertJSValToBool(&b1, cx, argv[1]))
    {
      JS_ReportError(cx, "2nd parameter needs to be a Boolean value");
      return JS_FALSE;
    }

    if(NS_OK != nativeThis->FileOpDirRemove(fsB0, b1, &nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "Function DirRemove requires 2 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}

//
// Native method DirRename
//
PR_STATIC_CALLBACK(JSBool)
InstallFileOpDirRename(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsInstall *nativeThis = (nsInstall*)JS_GetPrivate(cx, obj);
  PRInt32 nativeRet;
  nsAutoString b0;
  nsAutoString b1;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if(nsnull == nativeThis)
  {
    return JS_TRUE;
  }

  if(argc >= 2)
  {
    //  public int DirRename (String aSourceFolder,
    //                        String aTargetFolder);

    ConvertJSValToStr(b0, cx, argv[0]);
    ConvertJSValToStr(b1, cx, argv[1]);
    nsFileSpec fsB0(b0);

// fix: nsFileSpec::Rename() does not accept new name as a
//      nsFileSpec type.  It only accepts a char* type for the new name
//      This is a bug with nsFileSpec.  A char* will be used until
//      nsFileSpec if fixed.
//    nsFileSpec fsB1(b1);

    if(NS_OK != nativeThis->FileOpDirRename(fsB0, b1, &nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "Function DirRename requires 2 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}

//
// Native method FileCopy
//
PR_STATIC_CALLBACK(JSBool)
InstallFileOpFileCopy(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsInstall *nativeThis = (nsInstall*)JS_GetPrivate(cx, obj);
  PRInt32 nativeRet;
  nsAutoString b0;
  nsAutoString b1;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if(nsnull == nativeThis)
  {
    return JS_TRUE;
  }

  if(argc >= 2)
  {
    //  public int FileCopy (String aSourceFolder,
    //                       String aTargetFolder);

    ConvertJSValToStr(b0, cx, argv[0]);
    ConvertJSValToStr(b1, cx, argv[1]);
    nsFileSpec fsB0(b0);
    nsFileSpec fsB1(b1);

    if(NS_OK != nativeThis->FileOpFileCopy(fsB0, fsB1, &nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "Function FileCopy requires 2 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}

//
// Native method FileDelete
//
PR_STATIC_CALLBACK(JSBool)
InstallFileOpFileDelete(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsInstall *nativeThis = (nsInstall*)JS_GetPrivate(cx, obj);
  PRInt32 nativeRet;
  nsAutoString b0;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if(nsnull == nativeThis)
  {
    return JS_TRUE;
  }

  if(argc >= 1)
  {
    //  public int FileDelete (String aSourceFolder);

    ConvertJSValToStr(b0, cx, argv[0]);
    nsFileSpec fsB0(b0);

    if(NS_OK != nativeThis->FileOpFileDelete(fsB0, PR_FALSE, &nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "Function FileDelete requires 1 parameter");
    return JS_FALSE;
  }

  return JS_TRUE;
}

//
// Native method FileExists
//
PR_STATIC_CALLBACK(JSBool)
InstallFileOpFileExists(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsInstall *nativeThis = (nsInstall*)JS_GetPrivate(cx, obj);
  PRInt32 nativeRet;
  nsAutoString b0;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if(nsnull == nativeThis)
  {
    return JS_TRUE;
  }

  if(argc >= 1)
  {
    //  public int FileExists (String NativeFolderPath);

    ConvertJSValToStr(b0, cx, argv[0]);
    nsFileSpec fsB0(b0);

    if(NS_OK != nativeThis->FileOpFileExists(fsB0, &nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "Function FileExists requires 1 parameter");
    return JS_FALSE;
  }

  return JS_TRUE;
}

//
// Native method FileExecute
//
PR_STATIC_CALLBACK(JSBool)
InstallFileOpFileExecute(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsInstall *nativeThis = (nsInstall*)JS_GetPrivate(cx, obj);
  PRInt32 nativeRet;
  nsAutoString b0;
  nsAutoString b1;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if(nsnull == nativeThis)
  {
    return JS_TRUE;
  }

  if(argc >= 2)
  {
    //  public int FileExecute (String aSourceFolder,
    //                          String aParameters);

    ConvertJSValToStr(b0, cx, argv[0]);
    ConvertJSValToStr(b1, cx, argv[1]);
    nsFileSpec fsB0(b0);

    if(NS_OK != nativeThis->FileOpFileExecute(fsB0, b1, &nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else if(argc == 1)
  {
    //  public int FileExecute (String aSourceFolder,
    //                          String aParameters);

    ConvertJSValToStr(b0, cx, argv[0]);
    b1 = "";
    nsFileSpec fsB0(b0);

    if(NS_OK != nativeThis->FileOpFileExecute(fsB0, b1, &nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "Function FileExecute requires 1 or 2 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}

//
// Native method FileGetNativeVersion
//
PR_STATIC_CALLBACK(JSBool)
InstallFileOpFileGetNativeVersion(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsInstall*   nativeThis = (nsInstall*)JS_GetPrivate(cx, obj);
  nsAutoString nativeRet;
  nsAutoString b0;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if(nsnull == nativeThis)
  {
    return JS_TRUE;
  }

  if(argc >= 1)
  {
    //  public int FileGetNativeVersion (String NativeFolderPath);

    ConvertJSValToStr(b0, cx, argv[0]);
    nsFileSpec fsB0(b0);

    if(NS_OK != nativeThis->FileOpFileGetNativeVersion(fsB0, &nativeRet))
    {
      return JS_FALSE;
    }

    *rval = STRING_TO_JSVAL(JS_NewUCStringCopyN(cx, nativeRet.GetUnicode(), nativeRet.Length()));
  }
  else
  {
    JS_ReportError(cx, "Function FileGetNativeVersion requires 1 parameter");
    return JS_FALSE;
  }

  return JS_TRUE;
}

//
// Native method FileGetDiskSpaceAvailable
//
PR_STATIC_CALLBACK(JSBool)
InstallFileOpFileGetDiskSpaceAvailable(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{

  nsInstall*   nativeThis = (nsInstall*)JS_GetPrivate(cx, obj);
  PRInt64     nativeRet;
  nsAutoString b0;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if(nsnull == nativeThis)
  {
    return JS_TRUE;
  }

  if(argc >= 1)
  {
    //  public int FileGetDiskSpaceAvailable (String NativeFolderPath);

    ConvertJSValToStr(b0, cx, argv[0]);
    nsFileSpec fsB0(b0);

    if(NS_OK != nativeThis->FileOpFileGetDiskSpaceAvailable(fsB0, &nativeRet))
    {
      return JS_FALSE;
    }
    
    double d;
    LL_L2D(d, nativeRet);
    JS_NewDoubleValue( cx, d, rval );
  }
  else
  {
    JS_ReportError(cx, "Function FileGetDiskSpaceAvailable requires 1 parameter");
    return JS_FALSE;
  }

  return JS_TRUE;
}

//
// Native method FileGetModDate
//
PR_STATIC_CALLBACK(JSBool)
InstallFileOpFileGetModDate(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsInstall*   nativeThis = (nsInstall*)JS_GetPrivate(cx, obj);
  PRUint32     nativeRet;
  nsAutoString b0;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if(nsnull == nativeThis)
  {
    return JS_TRUE;
  }

  if(argc >= 1)
  {
    //  public int FileGetModDate (String NativeFolderPath);

    ConvertJSValToStr(b0, cx, argv[0]);
    nsFileSpec fsB0(b0);

    if(NS_OK != nativeThis->FileOpFileGetModDate(fsB0, &nativeRet))
    {
      return JS_FALSE;
    }

    if ( nativeRet <= JSVAL_INT_MAX )
      *rval = INT_TO_JSVAL(nativeRet);
    else
    {
      JSInt64 l;
      jsdouble d;

      JSLL_UI2L( l, nativeRet );
      JSLL_L2D( d, l );

      JS_NewDoubleValue( cx, d, rval );
    }
  }
  else
  {
    JS_ReportError(cx, "Function FileGetModDate requires 1 parameter");
    return JS_FALSE;
  }

  return JS_TRUE;
}

//
// Native method FileGetSize
//
PR_STATIC_CALLBACK(JSBool)
InstallFileOpFileGetSize(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsInstall*   nativeThis = (nsInstall*)JS_GetPrivate(cx, obj);
  PRUint32     nativeRet;
  nsAutoString b0;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if(nsnull == nativeThis)
  {
    return JS_TRUE;
  }

  if(argc >= 1)
  {
    //  public int FileGetSize (String NativeFolderPath);

    ConvertJSValToStr(b0, cx, argv[0]);
    nsFileSpec fsB0(b0);

    if(NS_OK != nativeThis->FileOpFileGetSize(fsB0, &nativeRet))
    {
      return JS_FALSE;
    }

    if ( nativeRet <= JSVAL_INT_MAX )
      *rval = INT_TO_JSVAL(nativeRet);
    else
    {
      JSInt64 l;
      jsdouble d;

      JSLL_UI2L( l, nativeRet );
      JSLL_L2D( d, l );

      JS_NewDoubleValue( cx, d, rval );
    }
  }
  else
  {
    JS_ReportError(cx, "Function FileGetSize requires 1 parameter");
    return JS_FALSE;
  }

  return JS_TRUE;
}

//
// Native method FileIsDirectory
//
PR_STATIC_CALLBACK(JSBool)
InstallFileOpFileIsDirectory(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsInstall *nativeThis = (nsInstall*)JS_GetPrivate(cx, obj);
  PRInt32 nativeRet;
  nsAutoString b0;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if(nsnull == nativeThis)
  {
    return JS_TRUE;
  }

  if(argc >= 1)
  {
    //  public int FileIsDirectory (String NativeFolderPath);

    ConvertJSValToStr(b0, cx, argv[0]);
    nsFileSpec fsB0(b0);

    if(NS_OK != nativeThis->FileOpFileIsDirectory(fsB0, &nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "Function FileIsDirectory requires 1 parameter");
    return JS_FALSE;
  }

  return JS_TRUE;
}

//
// Native method FileIsFile
//
PR_STATIC_CALLBACK(JSBool)
InstallFileOpFileIsFile(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsInstall *nativeThis = (nsInstall*)JS_GetPrivate(cx, obj);
  PRInt32 nativeRet;
  nsAutoString b0;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if(nsnull == nativeThis)
  {
    return JS_TRUE;
  }

  if(argc >= 1)
  {
    //  public int FileIsFile (String NativeFolderPath);

    ConvertJSValToStr(b0, cx, argv[0]);
    nsFileSpec fsB0(b0);

    if(NS_OK != nativeThis->FileOpFileIsFile(fsB0, &nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "Function FileIsFile requires 1 parameter");
    return JS_FALSE;
  }

  return JS_TRUE;
}

//
// Native method FileModDateChanged
//
PR_STATIC_CALLBACK(JSBool)
InstallFileOpFileModDateChanged(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsInstall*   nativeThis = (nsInstall*)JS_GetPrivate(cx, obj);
  PRInt32      nativeRet;
  nsAutoString b0;
  PRUint32     b1;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if(nsnull == nativeThis)
  {
    return JS_TRUE;
  }

  if(argc >= 2)
  {
    //  public int FileModDateChanged (String aSourceFolder,
    //                                 Number aOldDate);

    ConvertJSValToStr(b0, cx, argv[0]);
    b1 = JSVAL_TO_INT(argv[1]);
    nsFileSpec fsB0(b0);

    if(NS_OK != nativeThis->FileOpFileModDateChanged(fsB0, b1, &nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "Function FileModDateChanged requires 2 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}

//
// Native method FileMove
//
PR_STATIC_CALLBACK(JSBool)
InstallFileOpFileMove(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsInstall *nativeThis = (nsInstall*)JS_GetPrivate(cx, obj);
  PRInt32 nativeRet;
  nsAutoString b0;
  nsAutoString b1;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if(nsnull == nativeThis)
  {
    return JS_TRUE;
  }

  if(argc >= 2)
  {
    //  public int FileMove (String aSourceFolder,
    //                       String aTargetFolder);

    ConvertJSValToStr(b0, cx, argv[0]);
    ConvertJSValToStr(b1, cx, argv[1]);
    nsFileSpec fsB0(b0);
    nsFileSpec fsB1(b1);

    if(NS_OK != nativeThis->FileOpFileMove(fsB0, fsB1, &nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "Function FileMove requires 2 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}

//
// Native method FileRename
//
PR_STATIC_CALLBACK(JSBool)
InstallFileOpFileRename(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsInstall *nativeThis = (nsInstall*)JS_GetPrivate(cx, obj);
  PRInt32 nativeRet;
  nsAutoString b0;
  nsAutoString b1;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if(nsnull == nativeThis)
  {
    return JS_TRUE;
  }

  if(argc >= 2)
  {
    //  public int FileRename (String aSourceFolder,
    //                         String aTargetFolder);

    ConvertJSValToStr(b0, cx, argv[0]);
    ConvertJSValToStr(b1, cx, argv[1]);
    nsFileSpec fsB0(b0);

// fix: nsFileSpec::Rename() does not accept new name as a
//      nsFileSpec type.  It only accepts a char* type for the new name
//      This is a bug with nsFileSpec.  A char* will be used until
//      nsFileSpec if fixed.
//    nsFileSpec fsB1(b1);

    if(NS_OK != nativeThis->FileOpFileRename(fsB0, b1, &nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "Function FileRename requires 2 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}

//
// Native method FileWindowsShortcut
//
PR_STATIC_CALLBACK(JSBool)
InstallFileOpFileWindowsShortcut(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsInstall *nativeThis = (nsInstall*)JS_GetPrivate(cx, obj);
  PRInt32 nativeRet;
  nsAutoString b0;
  nsAutoString b1;
  nsAutoString b2;
  nsAutoString b3;
  nsAutoString b4;
  nsAutoString b5;
  nsFileSpec   nsfsB0;
  nsFileSpec   nsfsB1;
  nsFileSpec   nsfsB3;
  nsFileSpec   nsfsB5;
  PRInt32      b6;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if(nsnull == nativeThis)
  {
    return JS_TRUE;
  }

  if(argc >= 7)
  {
    //  public int FileWindowsShortcut(String aTarget,
    //                                 String aShortcutPath,
    //                                 String aDescription,
    //                                 String aWorkingPath,
    //                                 String aParams,
    //                                 String aIcon,
    //                                 Number aIconId);

    ConvertJSValToStr(b0, cx, argv[0]);
    nsfsB0 = b0;
    ConvertJSValToStr(b1, cx, argv[1]);
    nsfsB1 = b1;
    ConvertJSValToStr(b2, cx, argv[2]);
    ConvertJSValToStr(b3, cx, argv[3]);
    nsfsB3 = b3;
    ConvertJSValToStr(b4, cx, argv[4]);
    ConvertJSValToStr(b5, cx, argv[5]);
    nsfsB5 = b5;

    if(JSVAL_IS_NULL(argv[6]))
    {
      b6 = 0;
    }
    else
    {
      b6 = JSVAL_TO_INT(argv[6]);
    }

    if(NS_OK != nativeThis->FileOpFileWindowsShortcut(nsfsB0, nsfsB1, b2, nsfsB3, b4, nsfsB5, b6, &nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "Function FileWindowsShortcut requires 7 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}

//
// Native method FileMacAlias
//
PR_STATIC_CALLBACK(JSBool)
InstallFileOpFileMacAlias(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsInstall *nativeThis = (nsInstall*)JS_GetPrivate(cx, obj);
  PRInt32 nativeRet;
  nsAutoString b0;
  nsAutoString b1;
  nsAutoString b2;
  nsAutoString b3;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if(nsnull == nativeThis)
  {
    return JS_TRUE;
  }

  if(argc >= 4)
  {
    // public int FileMacAlias( String aSourceFolder,
    //                          String aSourceFileName,
    //                          String aAliasFolder,
    //                          String aAliasName );
    // where
    //      aSourceFolder -- the folder of the installed file from Get*Folder() APIs
    //      aSourceName   -- the installed file's name
    //      aAliasFolder  -- the folder of the new alias from Get*Folder() APIs
    //      aAliasName    -- the actual name of the new alias
    //      
    //      returns SUCCESS for successful scheduling of operation
    //              UNEXPECTED_ERROR for failure
    
    ConvertJSValToStr(b0, cx, argv[0]);
    ConvertJSValToStr(b1, cx, argv[1]);
    ConvertJSValToStr(b2, cx, argv[2]);
    ConvertJSValToStr(b3, cx, argv[3]);

    b0 += b1;
    b2 += b3;
	
    if(NS_OK != nativeThis->FileOpFileMacAlias(b0, b2, &nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else if (argc >= 3)
  {
    // public int FileMacAlias( String aSourceFolder,
    //                          String aSourceName,
    //                          String aAliasFolder );
    // where
    //      aSourceFolder -- the folder of the installed file from Get*Folder() APIs
    //      aSourceName   -- the installed file's name
    //      aAliasFolder  -- the folder of the new alias from Get*Folder() APIs
    //
    // NOTE: 
    //      The destination alias name is aSourceName + " alias" (Mac-like behavior). 
    //      
    //      returns SUCCESS for successful scheduling of operation
    //              UNEXPECTED_ERROR for failure
    
    ConvertJSValToStr(b0, cx, argv[0]);
    ConvertJSValToStr(b1, cx, argv[1]);
    ConvertJSValToStr(b2, cx, argv[2]);
    
    b0 += b1;
    b2 += b1;
    b2 += " alias";   // XXX use GetResourcedString(id)
    
    if(NS_OK != nativeThis->FileOpFileMacAlias(b0, b2, &nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else if (argc >= 2)
  {
    // public int FileMacAlias( String aSourcePath,
    //                          String aAliasPath );
    // where
    //      aSourcePath -- the full path to the installed file 
    //      aAliasPath  -- the full path to the new alias
    //      
    //      returns SUCCESS for successful scheduling of operation
    //              UNEXPECTED_ERROR for failure
    
    ConvertJSValToStr(b0, cx, argv[0]);
    ConvertJSValToStr(b1, cx, argv[1]);
    
    if(NS_OK != nativeThis->FileOpFileMacAlias(b0, b1, &nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "Function FileMacAlias requires 2 to 4 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}

//
// Native method FileUnixLinkCreate
//
PR_STATIC_CALLBACK(JSBool)
InstallFileOpFileUnixLink(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsInstall *nativeThis = (nsInstall*)JS_GetPrivate(cx, obj);
  PRInt32 nativeRet;
  nsAutoString b0;
  PRInt32      b1;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if(nsnull == nativeThis)
  {
    return JS_TRUE;
  }

  if(argc >= 2)
  {
    //  public int FileUnixLinkCreate (String aSourceFolder,
    //                                 Number aFlags);

    ConvertJSValToStr(b0, cx, argv[0]);
    b1 = JSVAL_TO_INT(argv[1]);
    nsFileSpec fsB0(b0);

    if(NS_OK != nativeThis->FileOpFileUnixLink(fsB0, b1, &nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "Function FileUnixLink requires 2 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}

//
// Native method LogComment
//
PR_STATIC_CALLBACK(JSBool)
InstallLogComment(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsInstall *nativeThis = (nsInstall*)JS_GetPrivate(cx, obj);
  nsAutoString b0;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if(nsnull == nativeThis)
  {
    return JS_TRUE;
  }

  if(argc >= 1)
  {
    //  public int LogComment (String aComment);

    ConvertJSValToStr(b0, cx, argv[0]);

    nativeThis->LogComment(b0);
  }
  else
  {
    JS_ReportError(cx, "Function LogComment requires 1 parameter");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method InstallAlert
//
PR_STATIC_CALLBACK(JSBool)
InstallAlert(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsInstall *nativeThis = (nsInstall*)JS_GetPrivate(cx, obj);
  nsAutoString b0;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if(nsnull == nativeThis)
  {
    return JS_TRUE;
  }

  if(argc == 1)
  {
    //  public int InstallAlert (String aComment);

    ConvertJSValToStr(b0, cx, argv[0]);
    nativeThis->Alert(b0);
  }
  else
  {
    JS_ReportError(cx, "Function LogComment requires 1 parameter");
    return JS_FALSE;
  }

  return JS_TRUE;
}

//
// Native method InstallConfirm
//
PR_STATIC_CALLBACK(JSBool)
InstallConfirm(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsInstall *nativeThis = (nsInstall*)JS_GetPrivate(cx, obj);
  nsAutoString b0;
  PRInt32 nativeRet;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if(nsnull == nativeThis)
  {
    return JS_TRUE;
  }

  if(argc == 1)
  {
    //  public int InstallConfirm (String aComment);

    ConvertJSValToStr(b0, cx, argv[0]);
    nativeThis->Confirm(b0, &nativeRet);
  
    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "Function LogComment requires 1 parameter");
    return JS_FALSE;
  }

  return JS_TRUE;
}

/***********************************************************************/
//
// class for Install
//
JSClass InstallClass = {
  "Install", 
  JSCLASS_HAS_PRIVATE,
  JS_PropertyStub,
  JS_PropertyStub,
  GetInstallProperty,
  SetInstallProperty,
  JS_EnumerateStub,
  JS_ResolveStub,
  JS_ConvertStub,
  FinalizeInstall
};

//
// Install class properties
//
static JSPropertySpec InstallProperties[] =
{
  {"userPackageName",   INSTALL_USERPACKAGENAME,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"regPackageName",    INSTALL_REGPACKAGENAME,     JSPROP_ENUMERATE | JSPROP_READONLY},
  {"silent",            INSTALL_SILENT,             JSPROP_ENUMERATE | JSPROP_READONLY},
  {"force",             INSTALL_FORCE,              JSPROP_ENUMERATE | JSPROP_READONLY},
  {"jarfile",           INSTALL_JARFILE,            JSPROP_ENUMERATE | JSPROP_READONLY},
  {"arguments",         INSTALL_ARGUMENTS,          JSPROP_ENUMERATE | JSPROP_READONLY},
  {"url",               INSTALL_URL,                JSPROP_ENUMERATE | JSPROP_READONLY},
  {"_statusSent",       INSTALL_STATUSSENT,         JSPROP_READONLY},
  {"Install",           INSTALL_INSTALL,            JSPROP_READONLY},
  {0}
};


static JSConstDoubleSpec install_constants[] = 
{
    { nsInstall::BAD_PACKAGE_NAME,           "BAD_PACKAGE_NAME"             },
    { nsInstall::UNEXPECTED_ERROR,           "UNEXPECTED_ERROR"             },
    { nsInstall::ACCESS_DENIED,              "ACCESS_DENIED"                },
    { nsInstall::NO_INSTALL_SCRIPT,          "NO_INSTALL_SCRIPT"            },
    { nsInstall::NO_CERTIFICATE,             "NO_CERTIFICATE"               },
    { nsInstall::NO_MATCHING_CERTIFICATE,    "NO_MATCHING_CERTIFICATE"      },
    { nsInstall::CANT_READ_ARCHIVE,          "CANT_READ_ARCHIVE"            },
    { nsInstall::INVALID_ARGUMENTS,          "INVALID_ARGUMENTS"            },
    { nsInstall::ILLEGAL_RELATIVE_PATH,      "ILLEGAL_RELATIVE_PATH"        },
    { nsInstall::USER_CANCELLED,             "USER_CANCELLED"               },
    { nsInstall::INSTALL_NOT_STARTED,        "INSTALL_NOT_STARTED"          },
    { nsInstall::SILENT_MODE_DENIED,         "SILENT_MODE_DENIED"           },
    { nsInstall::NO_SUCH_COMPONENT,          "NO_SUCH_COMPONENT"            },
    { nsInstall::DESTINATION_DOES_NOT_EXIST, "DESTINATION_DOES_NOT_EXIST"   },
    { nsInstall::DESTINATION_READ_ONLY,      "DESTINATION_READ_ONLY"        },
    { nsInstall::DESTINATION_IS_DIRECTORY,   "DESTINATION_IS_DIRECTORY"     },
    { nsInstall::NETWORK_FILE_IS_IN_USE,     "NETWORK_FILE_IS_IN_USE"       },
    { nsInstall::APPLE_SINGLE_ERR,           "APPLE_SINGLE_ERR"             },
    { nsInstall::INVALID_PATH_ERR,           "INVALID_PATH_ERR"             },
    { nsInstall::PATCH_BAD_DIFF,             "PATCH_BAD_DIFF"               },
    { nsInstall::PATCH_BAD_CHECKSUM_TARGET,  "PATCH_BAD_CHECKSUM_TARGET"    },
    { nsInstall::PATCH_BAD_CHECKSUM_RESULT,  "PATCH_BAD_CHECKSUM_RESULT"    },
    { nsInstall::UNINSTALL_FAILED,           "UNINSTALL_FAILED"             },
    { nsInstall::PACKAGE_FOLDER_NOT_SET,     "PACKAGE_FOLDER_NOT_SET"       },
    { nsInstall::EXTRACTION_FAILED,          "EXTRACTION_FAILED"            },
    { nsInstall::FILENAME_ALREADY_USED,      "FILENAME_ALREADY_USED"        },
    { nsInstall::ABORT_INSTALL,              "ABORT_INSTALL"                },
    { nsInstall::DOWNLOAD_ERROR,             "DOWNLOAD_ERROR"               },
    { nsInstall::SCRIPT_ERROR,               "SCRIPT_ERROR"                 },

    { nsInstall::DESTINATION_ALREADY_EXISTS, "DESTINATION_ALREADY_EXISTS"   },
    { nsInstall::DESTINATION_IS_FILE,        "DESTINATION_IS_FILE"          },
    { nsInstall::SOURCE_DOES_NOT_EXIST,      "SOURCE_DOES_NOT_EXIST"        },
    { nsInstall::SOURCE_IS_DIRECTORY,        "SOURCE_IS_DIRECTORY"          },
    { nsInstall::SOURCE_IS_FILE,             "SOURCE_IS_FILE"               },

    { nsInstall::GESTALT_UNKNOWN_ERR,        "GESTALT_UNKNOWN_ERR"          },
    { nsInstall::GESTALT_INVALID_ARGUMENT,   "GESTALT_INVALID_ARGUMENT"     },

    { nsInstall::SUCCESS,                    "SUCCESS"                      },
    { nsInstall::REBOOT_NEEDED,              "REBOOT_NEEDED"                },

    { nsInstall::LIMITED_INSTALL,            "LIMITED_INSTALL"              },
    { nsInstall::FULL_INSTALL,               "FULL_INSTALL"                 },
    { nsInstall::NO_STATUS_DLG ,             "NO_STATUS_DLG"                },
    { nsInstall::NO_FINALIZE_DLG,            "NO_FINALIZE_DLG"              },
    {0}
};

//
// Install class methods
//
static JSFunctionSpec InstallMethods[] = 
{
  {"AbortInstall",              InstallAbortInstall,            0},
  {"AddDirectory",              InstallAddDirectory,            6},
  {"AddSubcomponent",           InstallAddSubcomponent,         6},
  {"DeleteComponent",           InstallDeleteComponent,         1},
  {"DeleteFile",                InstallDeleteFile,              2},
  {"DiskSpaceAvailable",        InstallDiskSpaceAvailable,      1},
  {"Execute",                   InstallExecute,                 2},
  {"FinalizeInstall",           InstallFinalizeInstall,         0},
  {"Gestalt",                   InstallGestalt,                 1},
  {"GetComponentFolder",        InstallGetComponentFolder,      2},
  {"GetFolder",                 InstallGetFolder,               2},
  {"GetLastError",              InstallGetLastError,            0},
  {"GetWinProfile",             InstallGetWinProfile,           2},
  {"GetWinRegistry",            InstallGetWinRegistry,          0},
  {"LoadResources",             InstallLoadResources,           1},
  {"Patch",                     InstallPatch,                   5},
  {"ResetError",                InstallResetError,              0},
  {"SetPackageFolder",          InstallSetPackageFolder,        1},
  {"StartInstall",              InstallStartInstall,            4},
  {"Uninstall",                 InstallUninstall,               1},
/*START HACK FOR DEBUGGING UNTIL ALERTS WORK*/
  {"TRACE",                     InstallTRACE,                   1},
/*END HACK FOR DEBUGGING UNTIL ALERTS WORK*/
  {"DirCreate",                 InstallFileOpDirCreate,                1},
  {"DirGetParent",              InstallFileOpDirGetParent,             1},
  {"DirRemove",                 InstallFileOpDirRemove,                2},
  {"DirRename",                 InstallFileOpDirRename,                2},
  {"FileCopy",                  InstallFileOpFileCopy,                 2},
  {"FileDelete",                InstallFileOpFileDelete,               2},
  {"FileExists",                InstallFileOpFileExists,               1},
  {"FileExecute",               InstallFileOpFileExecute,              2},
  {"FileGetNativeVersion",      InstallFileOpFileGetNativeVersion,     1},
  {"FileGetDiskSpaceAvailable", InstallFileOpFileGetDiskSpaceAvailable,1},
  {"FileGetModDate",            InstallFileOpFileGetModDate,           1},
  {"FileGetSize",               InstallFileOpFileGetSize,              1},
  {"FileIsDirectory",           InstallFileOpFileIsDirectory,          1},
  {"FileIsFile",                InstallFileOpFileIsFile,               1},
  {"FileModDateChanged",        InstallFileOpFileModDateChanged,       2},
  {"FileMove",                  InstallFileOpFileMove,                 2},
  {"FileRename",                InstallFileOpFileRename,               2},
  {"FileWindowsShortcut",       InstallFileOpFileWindowsShortcut,      7},
  {"FileMacAlias",              InstallFileOpFileMacAlias,             2},
  {"FileUnixLink",              InstallFileOpFileUnixLink,             2},
  {"LogComment",                InstallLogComment,                     1},
  {"Alert",                     InstallAlert,                          1},
  {"Confirm",                   InstallConfirm,                        2},
  // -- new forms that match prevailing javascript style --
  {"abortInstall",              InstallAbortInstall,            0},
  {"addDirectory",              InstallAddDirectory,            6},
  {"addFile",                   InstallAddSubcomponent,         6},
  {"alert",                     InstallAlert,                   1},
  {"confirm",                   InstallConfirm,                 2},
  {"deleteRegisteredFile",      InstallDeleteComponent,         1},
  {"execute",                   InstallExecute,                 2},
  {"finalizeInstall",           InstallFinalizeInstall,         0},
  {"gestalt",                   InstallGestalt,                 1},
  {"getComponentFolder",        InstallGetComponentFolder,      2},
  {"getFolder",                 InstallGetFolder,               2},
  {"getLastError",              InstallGetLastError,            0},
  {"getWinProfile",             InstallGetWinProfile,           2},
  {"getWinRegistry",            InstallGetWinRegistry,          0},
  {"loadResources",             InstallLoadResources,           1},
  {"logComment",                InstallLogComment,              1},
  {"patch",                     InstallPatch,                   5},
  {"resetError",                InstallResetError,              0},
  {"setPackageFolder",          InstallSetPackageFolder,        1},
  {"startInstall",              InstallStartInstall,            4},
  {"uninstall",                 InstallUninstall,               1},
  // -- new forms for the file/dir methods --
  {"dirCreate",                 InstallFileOpDirCreate,                1},
  {"dirGetParent",              InstallFileOpDirGetParent,             1},
  {"dirRemove",                 InstallFileOpDirRemove,                2},
  {"dirRename",                 InstallFileOpDirRename,                2},
  {"fileCopy",                  InstallFileOpFileCopy,                 2},
  {"fileDelete",                InstallFileOpFileDelete,               2},
  {"fileExists",                InstallFileOpFileExists,               1},
  {"fileExecute",               InstallFileOpFileExecute,              2},
  {"fileGetNativeVersion",      InstallFileOpFileGetNativeVersion,     1},
  {"fileGetDiskSpaceAvailable", InstallFileOpFileGetDiskSpaceAvailable,1},
  {"fileGetModDate",            InstallFileOpFileGetModDate,           1},
  {"fileGetSize",               InstallFileOpFileGetSize,              1},
  {"fileIsDirectory",           InstallFileOpFileIsDirectory,          1},
  {"fileIsFile",                InstallFileOpFileIsFile,               1},
  {"fileModDateChanged",        InstallFileOpFileModDateChanged,       2},
  {"fileMove",                  InstallFileOpFileMove,                 2},
  {"fileRename",                InstallFileOpFileRename,               2},
  {"fileWindowsShortcut",       InstallFileOpFileWindowsShortcut,      7},
  {"fileMacAlias",              InstallFileOpFileMacAlias,             2},
  {"fileUnixLink",              InstallFileOpFileUnixLink,             2},
  {0}
};





JSObject * InitXPInstallObjects(JSContext *jscontext, 
                             JSObject *global, 
                             const char* jarfile, 
                             const PRUnichar* url,
                             const PRUnichar* args)
{
  JSObject *installObject       = nsnull;
  nsInstall *nativeInstallObject;


    if (global == nsnull)
    {
        //we are the global
        // new global object
        global = JS_NewObject(jscontext, &InstallClass, nsnull, nsnull);
    }

  installObject  = JS_InitClass( jscontext,         // context
                                 global,            // global object
                                 nsnull,            // parent proto 
                                 &InstallClass,     // JSClass
                                 nsnull,            // JSNative ctor
                                 0,                 // ctor args
                                 nsnull,            // proto props
                                 nsnull,            // proto funcs
                                 InstallProperties, // ctor props (static)
                                 InstallMethods);   // ctor funcs (static)

  if (nsnull == installObject) 
  {
      return nsnull;
  }

  if ( PR_FALSE == JS_DefineConstDoubles(jscontext, installObject, install_constants) )
            return nsnull;
  
  nativeInstallObject = new nsInstall();

  nativeInstallObject->SetJarFileLocation(jarfile);
  nativeInstallObject->SetInstallArguments(args);
  nativeInstallObject->SetInstallURL(url);

  JS_SetPrivate(jscontext, installObject, nativeInstallObject);
  nativeInstallObject->SetScriptObject(installObject);
 
#ifdef _WINDOWS
  JSObject *winRegPrototype     = nsnull;
  JSObject *winProfilePrototype = nsnull;

  if(NS_OK != InitWinRegPrototype(jscontext, global, &winRegPrototype))
  {
      return nsnull;
  }
  nativeInstallObject->SaveWinRegPrototype(winRegPrototype);

  if(NS_OK != InitWinProfilePrototype(jscontext, global, &winRegPrototype))
  {
      return nsnull;
  }
  nativeInstallObject->SaveWinProfilePrototype(winProfilePrototype);
#endif
 
  return installObject;
}
