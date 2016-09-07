/*
 * base.h
 *
 *  Created on: 2016年6月22日
 *      Author: Ruilin
 */

#ifndef JNI_JNI_PLAYER_BASE_H_
#define JNI_JNI_PLAYER_BASE_H_

#include <jni.h>
#include <android/log.h>

#define  LOG_TAG    __FILE__
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

typedef enum {
	TRUE = 1,
	FALSE = 0,
} BOOL;

#define B_TRUE		TRUE
#define B_FALSE	FALSE
#define NULL		0

#endif /* JNI_JNI_PLAYER_BASE_H_ */
