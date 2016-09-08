#include <jni.h>
#include <native/base.h>

#ifndef _Included_com_ruilin_rlplayer_RlMediaSDK
#define _Included_com_ruilin_rlplayer_RlMediaSDK

#ifdef __cplusplus
extern "C" {
#endif

#define JNI(rettype, name) JNIEXPORT rettype JNICALL Java_com_ruilin_rlplayer_media_RlMediaSDK_##name
#define JAVA_CLASS(name) "com/ruilin/rlplayer/media/" name
#define JAVA_CLASS_NAME_DECODER		JAVA_CLASS("RlVideoDecoder")

JNI(jboolean, initAvCoreJni)(JNIEnv *env, jobject obj, jobject ctx, jobject cb, jboolean ifOpenHwDecode, jstring logPath);
JNI(jboolean, freeAvCoreJni)(JNIEnv *env, jobject obj);
JNI(jboolean, startPlayMediaFile)(JNIEnv *env, jobject obj, jstring path);
JNI(jboolean, stop)(JNIEnv *env, jobject obj);
JNI(jboolean, pause)(JNIEnv *env, jobject obj);
JNI(jboolean, muteAudioStreamJni)(JNIEnv *env, jobject obj, jboolean en);
JNI(jboolean, muteVideoStreamJni)(JNIEnv *env, jobject obj, jboolean en);
JNI(jstring, getSDKVerion)(JNIEnv *env, jobject obj);
JNI(jobject, getMediaInfo)(JNIEnv *env, jobject obj);
JNI(jboolean, setHardwareDecode)(JNIEnv *env, jobject obj, jboolean en);
JNI(jboolean, isHardwareDecode)(JNIEnv *env, jobject obj);

JNI(jboolean, glInit)(JNIEnv *env, jobject obj, jobject glView, jint width, jint height);
JNI(void, glUninit)(JNIEnv *env, jobject obj);
JNI(void, glRender)(JNIEnv *env, jobject obj, jboolean isHwRending);
JNI(jint, glGenTexture)(JNIEnv *env, jobject obj);

#ifdef __cplusplus
}
#endif
#endif
