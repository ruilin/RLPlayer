/*
 * player.h
 *
 *  Created on: 2016年6月22日
 *      Author: Ruilin
 */

#ifndef JNI_JNI_PLAYER_PLAYER_H_
#define JNI_JNI_PLAYER_PLAYER_H_

#include <jni.h>
#include <android/native_window.h>
#include <native/base.h>
#include <string.h>


void *player_create(JNIEnv *env, jobject surface, uint32_t videoWidth, uint32_t videoHeight);
void player_destroy(void *player, JNIEnv *env);
void player_displayFrame(void *_player, JNIEnv *env, char *data, uint32_t len, uint32_t width, uint32_t height);

#endif /* JNI_JNI_PLAYER_PLAYER_H_ */
