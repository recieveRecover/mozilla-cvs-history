/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class com_sleepycat_db_DbMpool */

#ifndef _Included_com_sleepycat_db_DbMpool
#define _Included_com_sleepycat_db_DbMpool
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     com_sleepycat_db_DbMpool
 * Method:    finalize
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_sleepycat_db_DbMpool_finalize
  (JNIEnv *, jobject);

/*
 * Class:     com_sleepycat_db_DbMpool
 * Method:    fstat
 * Signature: ()[Lcom/sleepycat/db/DbMpoolFStat;
 */
JNIEXPORT jobjectArray JNICALL Java_com_sleepycat_db_DbMpool_fstat
  (JNIEnv *, jobject);

/*
 * Class:     com_sleepycat_db_DbMpool
 * Method:    stat
 * Signature: ()Lcom/sleepycat/db/DbMpoolStat;
 */
JNIEXPORT jobject JNICALL Java_com_sleepycat_db_DbMpool_stat
  (JNIEnv *, jobject);

/*
 * Class:     com_sleepycat_db_DbMpool
 * Method:    trickle
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_sleepycat_db_DbMpool_trickle
  (JNIEnv *, jobject, jint);

#ifdef __cplusplus
}
#endif
#endif
