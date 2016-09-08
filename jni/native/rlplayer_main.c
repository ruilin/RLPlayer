/*
 * rlplayer_main.c
 *
 *  Created on: 2016年9月5日
 *      Author: Ruilin
 */

#include <stdio.h>
#include <native/decoder/hw_deocder.h>
#include "com_ruilin_rlplayer_player_RlMediaSDK.h"
#include "player/player.h"
#include "player/gl_renderer.h"
#include "decoder/ffmpeg_decoder.h"


static JavaVM *jvm = NULL;
static void *player = NULL;
static void *sw_decoder = NULL;
static void *hw_decoder = NULL;
static BOOL isHwDecodeSupported;
static BOOL isHwDecode;


jint JNI_OnLoad(JavaVM *vm, void *reserved) {
	jvm = vm;
	if (!jvm) {
		return -1;
	}
	JNIEnv* env;
	if (JNI_OK != (*vm)->GetEnv(vm, (void**)(&env), JNI_VERSION_1_4)) {
		return -1;
	}
	return JNI_VERSION_1_4;
}


void changeDecodeMode(JNIEnv *env, BOOL _isHwDecode) {
	isHwDecode = _isHwDecode;
	if (isHwDecode) {
		/* 切硬解 */
		gl_set_type(env, GL_RENDER_HW);
	} else {
		/* 切软解 */
		gl_set_type(env, GL_RENDER_SW);
	}
	return;
}

JNI(jboolean, initAvCoreJni)(JNIEnv *env, jobject obj, jobject ctx, jobject cb, jboolean ifOpenHwDecode, jstring logPath) {
	BOOL errFlg = TRUE;
	player = NULL;
	hw_decoder = hw_decoder_create(env);
	isHwDecodeSupported = hw_decoder_isHwSupported(hw_decoder, env);
	if (ifOpenHwDecode) {
		isHwDecode = isHwDecodeSupported;
	} else {
		isHwDecode = B_FALSE;
	}

	gl_init(env, isHwDecode ? GL_RENDER_HW : GL_RENDER_SW, hw_decoder);

	sw_decoder = ffmpeg_decoder_create();
	CONNECT_CMD_ERR:
	return errFlg;
}

JNI(jboolean, freeAvCoreJni)(JNIEnv *env, jobject obj) {
	gl_uninit(env);
	ffmpeg_decoder_destroy(sw_decoder);
	sw_decoder = NULL;
	hw_decoder_destroy(env, hw_decoder);
	hw_decoder = NULL;
	return TRUE;
}

void ffmpeg_decoder_onVideo(void *callbackObject, unsigned char *data, unsigned int len, unsigned short width, unsigned short height) {
	JNIEnv *env = (JNIEnv *)callbackObject;
	gl_render_frame(env, (unsigned char *)data, len, width, height);
	usleep(41000);
	return;
}

void *run_startPlay(void *path) {
	jint iRet = -1;
	JNIEnv *env;
	(*jvm)->GetEnv(jvm, (void**) &env, JNI_VERSION_1_4);
	if (env == NULL) {
		iRet = (*jvm)->AttachCurrentThread(jvm, (JNIEnv **) &env, NULL);
	}
	char filePath[128] = {};
	sprintf(filePath, "%s", (char *)path);
	ffmpeg_decoder_playerFile(sw_decoder, filePath, ffmpeg_decoder_onVideo, env);
	 if (iRet != -1) {
		 (*jvm)->DetachCurrentThread(jvm);
	 }
	 return NULL;
}

JNI(jboolean, startPlayMediaFile)(JNIEnv *env, jobject obj, jstring path) {
	const char *filePath = (*env)->GetStringUTFChars(env, path, NULL);
	(*env)->ReleaseStringUTFChars(env, path, filePath);

	pthread_t threadId;
	if(pthread_create(&threadId, NULL, run_startPlay, (void *)filePath) !=  0)  {
		LOGE("pthread_create() error !!");
	}

	return TRUE;
}

JNI(jboolean, pause)(JNIEnv *env, jobject obj) {
	ffmpeg_decoder_pause(sw_decoder);
	return TRUE;
}

JNI(jboolean, stop)(JNIEnv *env, jobject obj) {
	ffmpeg_decoder_stop(sw_decoder);
	return TRUE;
}

JNI(jboolean, muteAudioStreamJni)(JNIEnv *env, jobject obj, jboolean en) {
	return TRUE;
}

JNI(jboolean, muteVideoStreamJni)(JNIEnv *env, jobject obj, jboolean en) {
	return TRUE;
}

JNI(jstring, getSDKVerion)(JNIEnv *env, jobject obj) {
	return "";
}

JNI(jobject, getMediaInfo)(JNIEnv *env, jobject obj) {
	return NULL;
}

JNI(jboolean, setHardwareDecode)(JNIEnv *env, jobject obj, jboolean en) {
	return TRUE;
}

JNI(jboolean, isHardwareDecode)(JNIEnv *env, jobject obj) {
	return TRUE;
}


