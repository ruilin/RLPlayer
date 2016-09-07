/*
 * gl_renderer.h
 *
 *  Created on: 2016年7月7日
 *      Author: Ruilin
 */

#ifndef JNI_NATIVE_PLAYER_GL_RENDERER_H_
#define JNI_NATIVE_PLAYER_GL_RENDERER_H_

#include <pthread.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <native/base.h>
#include <native/decoder/hw_deocder.h>
#include "../com_ruilin_rlplayer_player_RlMediaSDK.h"


typedef enum {
	GL_RENDER_HW = 0,
	GL_RENDER_SW = 1,
} GL_RENDER_TYPE;

void gl_init(JNIEnv *env, GL_RENDER_TYPE defMode, void *deocder);
void gl_uninit(JNIEnv *env);
void gl_render_frame(JNIEnv *env, unsigned char *data, unsigned int len, unsigned short width, unsigned short height);
void gl_set_type(JNIEnv *env, GL_RENDER_TYPE type);
BOOL gl_isHwRending(void);

#endif /* JNI_NATIVE_PLAYER_GL_RENDERER_H_ */
