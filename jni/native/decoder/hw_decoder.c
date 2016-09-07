/*
 * hw_decoder.c
 *
 *  Created on: 2016年6月24日
 *      Author: Ruilin
 */

#include <native/decoder/hw_deocder.h>

typedef struct {
	jclass jcls;
	jobject jobj;
	jobject jsurface;
	jbyteArray jarray;
	unsigned int jarray_len;
	int videoWidth;
	int videoHeight;
	BOOL isOpened;
} Decoder;

#define FUN_PATH	"()L"JAVA_CLASS("JJVideoDecoder")";"
/* 获取单例 */
#define getJavaDecoder(env, clazz, jDecoder) { 																			\
		clazz = (*env)->FindClass(env, JAVA_CLASS("JJVideoDecoder")); 											\
		jmethodID method = (*env)->GetStaticMethodID(env, clazz, "getInstance", FUN_PATH); 	\
		jDecoder = (*env)->CallStaticObjectMethod(env, clazz, method); 										\
}

BOOL hw_decoder_isHwSupported(void *_decoder, JNIEnv * env) {
	Decoder *decoder = (Decoder *) _decoder;
	if (decoder == NULL) {
		return B_FALSE;
	}
	jmethodID method = (*env)->GetStaticMethodID(env, decoder->jcls, "isH264HwSupported", "()Z");
	jboolean result = (*env)->CallStaticBooleanMethod(env, decoder->jcls, method);
	return result;
}

static BOOL hw_decoder_open(void *_decoder, JNIEnv * env, unsigned short width, unsigned short height) {
//	return 1; //TODO
	Decoder *decoder = (Decoder *)_decoder;
	if (decoder == NULL || decoder->jsurface == NULL) {
		LOGE("hw_decoder_open() decoder == NULL || decoder->jsurface == NULL");
		return B_FALSE;
	}
	if (decoder->isOpened) {
		hw_decoder_close(decoder, env);
	}
	jmethodID method = (*env)->GetMethodID(env, decoder->jcls, "open", "(Landroid/view/Surface;II)Z");
	jboolean result = (*env)->CallBooleanMethod(env, decoder->jobj, method, decoder->jsurface, width, height);
	decoder->isOpened = result;
	return result;
}

void hw_decoder_close(void *_decoder, JNIEnv *env) {
	Decoder *decoder = (Decoder *)_decoder;
	if (decoder == NULL || !decoder->isOpened) {
		LOGD("hw_decoder_close() decoder NULL || !decoder->isOpened");
		return;
	}
	jmethodID method = (*env)->GetMethodID(env, decoder->jcls, "close", "()V");
	(*env)->CallVoidMethod(env, decoder->jobj, method);
	decoder->isOpened = B_FALSE;
	return;
}

void hw_decoder_resetSurface(void *_decoder, JNIEnv * env, jobject surface) {
	return; // TODO
	Decoder *decoder = (Decoder *)_decoder;
	if (decoder == NULL) {
		return;
	}
	hw_decoder_close(decoder, env);
	if (decoder->jsurface != NULL) {
		(*env)->DeleteGlobalRef(env, decoder->jsurface);
		decoder->jsurface = NULL;
	}
	if (surface != NULL) {
		decoder->jsurface = (*env)->NewGlobalRef(env, surface);
	}
	return;
}

jobject hw_decoder_getSurface(void *_decoder) {
	Decoder *decoder = (Decoder *)_decoder;
	if (decoder == NULL) {
		return NULL;
	}
	return decoder->jsurface;
}

void *hw_decoder_create(JNIEnv *env) {
	Decoder *decoder = malloc(sizeof(Decoder));
	if (decoder == NULL) {
		LOGE("hw_decoder_create() malloc failed !!!");
		return NULL;
	}
	decoder->isOpened = B_FALSE;
	jclass clazz;
	jobject object;
	getJavaDecoder(env, clazz, object)

	decoder->jcls = (*env)->NewGlobalRef(env, clazz);
	decoder->jobj = (*env)->NewGlobalRef(env, object);
	decoder->jsurface = NULL;
	decoder->jarray = NULL;
	decoder->jarray_len = 0;
	return decoder;
}

void hw_decoder_destroy(JNIEnv *env, void *_decoder) {
	Decoder *decoder = (Decoder *) _decoder;
//	hw_decoder_close(decoder, env);
	if (decoder != NULL) {
		if (decoder->jcls != NULL) {
			(*env)->DeleteGlobalRef(env, decoder->jcls);
			decoder->jcls = NULL;
		}
		if (decoder->jobj != NULL) {
			(*env)->DeleteGlobalRef(env, decoder->jobj);
			decoder->jobj = NULL;
		}
		if (decoder->jarray != NULL) {
			(*env)->DeleteGlobalRef(env, decoder->jarray);
			decoder->jarray = NULL;
			decoder->jarray_len = 0;
		}
		if (decoder->jsurface != NULL) {
			(*env)->DeleteGlobalRef(env, decoder->jsurface);
			decoder->jsurface = NULL;
		}
		free(decoder);
		decoder = NULL;
	}
	return;
}

BOOL hw_decoder_decodeFrame(void *_decoder, JNIEnv * env, char *data, unsigned int len, unsigned short width, unsigned short height) {
	Decoder *decoder = (Decoder *)_decoder;
	if (decoder == NULL) {
		LOGE("hw_decoder_decodeFrame() decoder == NULL");
		return B_TRUE;	// 避免被切换为软解
	}
//	if (!decoder->isOpened && !hw_decoder_open(decoder, env, width, height)) {
//		LOGE("hw_decoder_decodeFrame - hw_decoder_open() failed");
//		return B_FALSE;
//	}
	if (decoder->jarray_len < len) {
		decoder->jarray_len = len;
		(*env)->DeleteGlobalRef(env, decoder->jarray);
		decoder->jarray = NULL;
	}
	if (decoder->jarray == NULL) {
		jbyteArray jarray = (*env)->NewByteArray(env, len);
		decoder->jarray = (jbyteArray) (*env)->NewGlobalRef(env, jarray);
		(*env)->DeleteLocalRef(env, jarray);
	}
	(*env)->SetByteArrayRegion(env, decoder->jarray, 0, len, (jbyte *)data);

	jmethodID method = (*env)->GetMethodID(env, decoder->jcls, "decodeFrame", "([BIII)I");
	jint result = (*env)->CallIntMethod(env, decoder->jobj, method, decoder->jarray, len, width, height);
//	jmethodID method = (*env)->GetStaticMethodID(env, decoder->jcls, "test", "()Ljava/lang/String;");
//	jstring result = (*env)->CallStaticObjectMethod(env, decoder->jcls, method);
//		const char* str = (*env)->GetStringUTFChars(env, result, 0);
//		LOGE("xxxxx  %s", str);
	return (result != 1);
}

