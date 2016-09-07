/*
 * player.c
 * native surface
 *  Created on: 2016年6月22日
 *      Author: Ruilin
 */

#include <pthread.h>
#include <malloc.h>
#include "player.h"

#define RGB_SIZE		4

typedef struct {
	jobject jsurface;
	ANativeWindow *mANativeWindow;
	int videoWidth;
	int videoHeight;
} Player;

static ANativeWindow *player_createWindow(Player *player, JNIEnv *env, uint32_t videoWidth, uint32_t videoHeight) {
	LOGI("player_createWindow() ANativeWindow_fromSurface: %d x %d", videoWidth, videoHeight);
	player->videoWidth = videoWidth;
	player->videoHeight = videoHeight;
	ANativeWindow *mANativeWindow = (ANativeWindow *)ANativeWindow_fromSurface(env, player->jsurface);
	if (mANativeWindow == NULL) {
		LOGE("ANativeWindow_fromSurface error");
		return NULL;
	}
	LOGI("player_createWindow() setBuffersGeometry");
	ANativeWindow_setBuffersGeometry(mANativeWindow, videoWidth, videoHeight, WINDOW_FORMAT_RGBA_8888);
	return mANativeWindow;
}

void *player_create(JNIEnv *env, jobject surface, uint32_t videoWidth, uint32_t videoHeight) {
	if (surface == NULL) {
		return NULL;
	}
	Player *player = (Player *) malloc(sizeof(Player));
	if (player == NULL) {
		return NULL;
	}
	player->videoWidth = 0;
	player->videoHeight = 0;
	player->jsurface = NULL;
	player->mANativeWindow = NULL;

	if (surface != NULL) {
		player->jsurface = (*env)->NewGlobalRef(env, surface);
	}
	if (player->mANativeWindow != NULL) {
		ANativeWindow_release(player->mANativeWindow);
		player->mANativeWindow = NULL;
	}
	player->mANativeWindow = player_createWindow(player, env, videoWidth, videoHeight);
	if (player->mANativeWindow == NULL) {
		// free player
		LOGE("player_create() mANativeWindow == NULL !!");
		if (player->jsurface != NULL) {
			(*env)->DeleteGlobalRef(env, player->jsurface);
			player->jsurface = NULL;
		}
		free(player);
		player = NULL;
		return NULL;
	}
	return player;
}

void player_destroy(void *_player, JNIEnv *env) {
	Player *player = (Player *) _player;
	if (player != NULL) {
		if (player->mANativeWindow != NULL) {
			ANativeWindow_release(player->mANativeWindow);
			player->mANativeWindow = NULL;
			LOGI("player_destroy() ANativeWindow_release ");
		}
		if (player->jsurface != NULL) {
			(*env)->DeleteGlobalRef(env, player->jsurface);
			player->jsurface = NULL;
		}
		free(player);
		player = NULL;
	}
	return;
}

void player_displayFrame(void *_player, JNIEnv *env, char *data, uint32_t len,
		uint32_t width, uint32_t height) {
	Player *player = (Player *) _player;
	if (player == NULL || data == NULL) {
		if (data == NULL) {
			LOGE("player_displayFrame() data == NULL !!");
		} else {
			LOGE("player_displayFrame() player == NULL !!");
		}
		goto RETURN;
	}
	if (player->mANativeWindow == NULL) {
		goto RETURN;
	}
	if (player->videoWidth != width || player->videoHeight != height) {
		player->videoWidth = width;
		player->videoHeight = height;
		if (player->mANativeWindow != NULL) {
			ANativeWindow_release(player->mANativeWindow);
			player->mANativeWindow = NULL;
		}
		player->mANativeWindow = player_createWindow(player, env, width, height);
		if (player->mANativeWindow == NULL) {
			LOGE("player_displayFrame() mANativeWindow == NULL !!");
			goto RETURN;
		}
	}
	ANativeWindow_Buffer nwBuffer;
	if (0 != ANativeWindow_lock(player->mANativeWindow, &nwBuffer, NULL)) {
		LOGE("player_displayFrame() ANativeWindow_lock error");
		goto RETURN;
	}
//	int32_t w = ANativeWindow_getWidth(player->mANativeWindow);
//	int32_t h = ANativeWindow_getHeight(player->mANativeWindow);

	/**
	 * nwBuffer.stride >= nwBuffer.width: nwBuffer.stride 为 POT 即2的n次方
	 */
	if (nwBuffer.width >= nwBuffer.stride) {
		memcpy(nwBuffer.bits, data, len);
	} else {
		/*fixed花屏问题:
		 * 输出stride和width的日志发现,如果正常显示则stride==width, 通过注释可以看出应该是内存对齐问题导致的, 调整代码:
		 * http://blog.csdn.net/lsingle/article/details/38174049?utm_source=tuicool&utm_medium=referral
		 */
		int i;
		for (i = 0; i < height; ++i) {
			memcpy(nwBuffer.bits + nwBuffer.stride * i * RGB_SIZE, data + width * i * RGB_SIZE, width * RGB_SIZE);
		}
	}
	if (0 != ANativeWindow_unlockAndPost(player->mANativeWindow)) {
		LOGE("player_displayFrame() ANativeWindow_unlockAndPost error");
		goto RETURN;
	}

RETURN:
	return;
}
