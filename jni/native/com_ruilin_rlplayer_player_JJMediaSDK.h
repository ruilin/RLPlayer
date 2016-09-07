#include <jni.h>
#include <native/base.h>

#ifndef _Included_com_jjcj_media_JJMediaSDK
#define _Included_com_jjcj_media_JJMediaSDK

#ifdef __cplusplus
extern "C" {
#endif

#define JOWW(rettype, name) JNIEXPORT rettype JNICALL Java_com_ruilin_rlplayer_player_JJMediaSDK_##name
#define JAVA_CLASS(name) "com/ruilin/rlplayer/player/" name

JOWW(jboolean, initAvCoreJni)(JNIEnv *env, jobject obj, jobject ctx, jobject cb, jboolean ifOpenHwDecode, jstring logPath);
JOWW(jboolean, freeAvCoreJni)(JNIEnv *env, jobject obj);
JOWW(jboolean, rcvStreamStartJni)(JNIEnv *env, jobject obj, jstring path);
JOWW(jboolean, rcvStreamStopJni)(JNIEnv *env, jobject obj);
JOWW(jboolean, muteAudioStreamJni)(JNIEnv *env, jobject obj, jboolean en);
JOWW(jboolean, muteVideoStreamJni)(JNIEnv *env, jobject obj, jboolean en);
JOWW(jstring, getSDKVerion)(JNIEnv *env, jobject obj);
JOWW(jobject, getMediaInfo)(JNIEnv *env, jobject obj);
JOWW(jboolean, setHardwareDecode)(JNIEnv *env, jobject obj, jboolean en);
JOWW(jboolean, isHardwareDecode)(JNIEnv *env, jobject obj);

JOWW(jboolean, glInit)(JNIEnv *env, jobject obj, jobject glView, jint width, jint height);
JOWW(void, glUninit)(JNIEnv *env, jobject obj);
JOWW(void, glRender)(JNIEnv *env, jobject obj, jboolean isHwRending);
JOWW(jint, glGenTexture)(JNIEnv *env, jobject obj);

#ifdef __cplusplus
}
#endif
#endif
