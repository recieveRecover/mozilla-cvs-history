/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * 
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *  
 * The Original Code is The Waterfall Java Plugin Module
 *  
 * The Initial Developer of the Original Code is Sun Microsystems Inc
 * Portions created by Sun Microsystems Inc are Copyright (C) 2001
 * All Rights Reserved.
 * 
 * $Id: sun_jvmp_mozilla_JSObject.h,v 1.2 2001/07/12 19:57:46 edburns%acm.org Exp $
 * 
 * Contributor(s):
 * 
 *     Nikolay N. Igotti <nikolay.igotti@Sun.Com>
 */

/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class sun_jvmp_mozilla_JSObject */

#ifndef _Included_sun_jvmp_mozilla_JSObject
#define _Included_sun_jvmp_mozilla_JSObject
#ifdef __cplusplus
extern "C" {
#endif
/* Inaccessible static: m_evaluator */
/*
 * Class:     sun_jvmp_mozilla_JSObject
 * Method:    JSFinalize
 * Signature: (IIJ)V
 */
JNIEXPORT void JNICALL Java_sun_jvmp_mozilla_JSObject_JSFinalize
  (JNIEnv *, jclass, jint, jint, jlong);

/*
 * Class:     sun_jvmp_mozilla_JSObject
 * Method:    JSGetNativeJSObject
 * Signature: (JLjava/lang/String;[[B[IILjava/security/AccessControlContext;)I
 */
JNIEXPORT jint JNICALL Java_sun_jvmp_mozilla_JSObject_JSGetNativeJSObject
  (JNIEnv *, jclass, jlong, jstring, jobjectArray, jintArray, jint, jobject);

/*
 * Class:     sun_jvmp_mozilla_JSObject
 * Method:    JSGetThreadID
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_sun_jvmp_mozilla_JSObject_JSGetThreadID
  (JNIEnv *, jclass, jlong);

/*
 * Class:     sun_jvmp_mozilla_JSObject
 * Method:    JSObjectCall
 * Signature: (IIJLjava/lang/String;[[B[IILjava/lang/String;[Ljava/lang/Object;Ljava/security/AccessControlContext;)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_sun_jvmp_mozilla_JSObject_JSObjectCall
  (JNIEnv *, jclass, jint, jint, jlong, jstring, jobjectArray, jintArray, jint, jstring, jobjectArray, jobject);

/*
 * Class:     sun_jvmp_mozilla_JSObject
 * Method:    JSObjectEval
 * Signature: (IIJLjava/lang/String;[[B[IILjava/lang/String;Ljava/security/AccessControlContext;)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_sun_jvmp_mozilla_JSObject_JSObjectEval
  (JNIEnv *, jclass, jint, jint, jlong, jstring, jobjectArray, jintArray, jint, jstring, jobject);

/*
 * Class:     sun_jvmp_mozilla_JSObject
 * Method:    JSObjectGetMember
 * Signature: (IIJLjava/lang/String;[[B[IILjava/lang/String;Ljava/security/AccessControlContext;)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_sun_jvmp_mozilla_JSObject_JSObjectGetMember
  (JNIEnv *, jclass, jint, jint, jlong, jstring, jobjectArray, jintArray, jint, jstring, jobject);

/*
 * Class:     sun_jvmp_mozilla_JSObject
 * Method:    JSObjectGetSlot
 * Signature: (IIJLjava/lang/String;[[B[IIILjava/security/AccessControlContext;)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_sun_jvmp_mozilla_JSObject_JSObjectGetSlot
  (JNIEnv *, jclass, jint, jint, jlong, jstring, jobjectArray, jintArray, jint, jint, jobject);

/*
 * Class:     sun_jvmp_mozilla_JSObject
 * Method:    JSObjectRemoveMember
 * Signature: (IIJLjava/lang/String;[[B[IILjava/lang/String;Ljava/security/AccessControlContext;)V
 */
JNIEXPORT void JNICALL Java_sun_jvmp_mozilla_JSObject_JSObjectRemoveMember
  (JNIEnv *, jclass, jint, jint, jlong, jstring, jobjectArray, jintArray, jint, jstring, jobject);

/*
 * Class:     sun_jvmp_mozilla_JSObject
 * Method:    JSObjectSetMember
 * Signature: (IIJLjava/lang/String;[[B[IILjava/lang/String;Ljava/lang/Object;Ljava/security/AccessControlContext;)V
 */
JNIEXPORT void JNICALL Java_sun_jvmp_mozilla_JSObject_JSObjectSetMember
  (JNIEnv *, jclass, jint, jint, jlong, jstring, jobjectArray, jintArray, jint, jstring, jobject, jobject);

/*
 * Class:     sun_jvmp_mozilla_JSObject
 * Method:    JSObjectSetSlot
 * Signature: (IIJLjava/lang/String;[[B[IIILjava/lang/Object;Ljava/security/AccessControlContext;)V
 */
JNIEXPORT void JNICALL Java_sun_jvmp_mozilla_JSObject_JSObjectSetSlot
  (JNIEnv *, jclass, jint, jint, jlong, jstring, jobjectArray, jintArray, jint, jint, jobject, jobject);

/*
 * Class:     sun_jvmp_mozilla_JSObject
 * Method:    JSObjectToString
 * Signature: (IIJ)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_sun_jvmp_mozilla_JSObject_JSObjectToString
  (JNIEnv *, jclass, jint, jint, jlong);

#ifdef __cplusplus
}
#endif
#endif
