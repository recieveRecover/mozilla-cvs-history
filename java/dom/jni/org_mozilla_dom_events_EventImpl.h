/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class org_mozilla_dom_events_EventImpl */

#ifndef _Included_org_mozilla_dom_events_EventImpl
#define _Included_org_mozilla_dom_events_EventImpl
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     org_mozilla_dom_events_EventImpl
 * Method:    getCurrentNode
 * Signature: ()Lorg/w3c/dom/Node;
 */
JNIEXPORT jobject JNICALL Java_org_mozilla_dom_events_EventImpl_getCurrentNode
  (JNIEnv *, jobject);

/*
 * Class:     org_mozilla_dom_events_EventImpl
 * Method:    getEventPhase
 * Signature: ()S
 */
JNIEXPORT jshort JNICALL Java_org_mozilla_dom_events_EventImpl_getEventPhase
  (JNIEnv *, jobject);

/*
 * Class:     org_mozilla_dom_events_EventImpl
 * Method:    getTarget
 * Signature: ()Lorg/w3c/dom/events/EventTarget;
 */
JNIEXPORT jobject JNICALL Java_org_mozilla_dom_events_EventImpl_getTarget
  (JNIEnv *, jobject);

/*
 * Class:     org_mozilla_dom_events_EventImpl
 * Method:    getType
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_mozilla_dom_events_EventImpl_getType
  (JNIEnv *, jobject);

/*
 * Class:     org_mozilla_dom_events_EventImpl
 * Method:    preventBubble
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_mozilla_dom_events_EventImpl_preventBubble
  (JNIEnv *, jobject);

/*
 * Class:     org_mozilla_dom_events_EventImpl
 * Method:    preventCapture
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_mozilla_dom_events_EventImpl_preventCapture
  (JNIEnv *, jobject);

/*
 * Class:     org_mozilla_dom_events_EventImpl
 * Method:    preventDefault
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_mozilla_dom_events_EventImpl_preventDefault
  (JNIEnv *, jobject);

#ifdef __cplusplus
}
#endif
#endif
