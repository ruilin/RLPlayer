/*
 * hw_deocder.h
 *
 *  Created on: 2016年6月24日
 *      Author: Ruilin
 */

#ifndef JNI_JNI_PLAYER_HW_DEOCDER_H_
#define JNI_JNI_PLAYER_HW_DEOCDER_H_

#include <native/base.h>
#include <native/com_ruilin_rlplayer_player_JJMediaSDK.h>

void *hw_decoder_create(JNIEnv *env);
void hw_decoder_destroy(JNIEnv *env, void *_decoder);
void hw_decoder_resetSurface(void *_decoder, JNIEnv * env, jobject surface);
jobject hw_decoder_getSurface(void *_decoder);
void hw_decoder_close(void *_decoder, JNIEnv *env);
BOOL hw_decoder_decodeFrame(void *_decoder, JNIEnv * env, char *data, unsigned int len, unsigned short width, unsigned short height);
BOOL hw_decoder_isHwSupported(void *_decoder, JNIEnv * env);

#endif /* JNI_JNI_PLAYER_HW_DEOCDER_H_ */
