/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class com_southernstars_sscore_JSSObjectArray */

#ifndef _Included_com_southernstars_sscore_JSSObjectArray
#define _Included_com_southernstars_sscore_JSSObjectArray
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     com_southernstars_sscore_JSSObjectArray
 * Method:    create
 * Signature: ()Lcom/southernstars/sscore/JSSObjectArray;
 */
JNIEXPORT jobject JNICALL Java_com_southernstars_sscore_JSSObjectArray_create
  (JNIEnv *, jclass);

/*
 * Class:     com_southernstars_sscore_JSSObjectArray
 * Method:    destroy
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_southernstars_sscore_JSSObjectArray_destroy
  (JNIEnv *, jobject);

/*
 * Class:     com_southernstars_sscore_JSSObjectArray
 * Method:    importFromCSV
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_southernstars_sscore_JSSObjectArray_importFromCSV
  (JNIEnv *, jobject, jstring);

/*
 * Class:     com_southernstars_sscore_JSSObjectArray
 * Method:    exportToCSV
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_southernstars_sscore_JSSObjectArray_exportToCSV
  (JNIEnv *, jobject, jstring);

#ifdef __cplusplus
}
#endif
#endif
