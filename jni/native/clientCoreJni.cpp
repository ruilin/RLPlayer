#include <android/log.h>
#include <sstream>

#include "../libcommon/platFromSel.h"
#include "../libcommon/LetMutex.h"
#include "../libcommon/common.h"
#include "../avcore/clientCore.h"
#include "com_jjcj_media_JJMediaSDK.h"

#ifdef __cplusplus
extern "C" {
#include "player/player.h"
#include <native/decoder/hw_deocder.h>
#include "player/gl_renderer.h"
}
#endif

#define LOG_TO_MONITOR
#ifdef LOG_TO_MONITOR
#define LOG_TAG "SDK"
#define DLog1(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define ILog1(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define FLOG1(format, ...) LogToFile("jjmedia.log", 1, format,  ## __VA_ARGS__)
#else
#define DLog1(...)
#endif

/*
 * 使用native surface渲染，需要设置回调的数据格式为RGB，并且上层view不初始化OpenGL
 */
#define USE_NATIVE_SURFACE		0

#define VIDEO_WIDTH		800
#define VIDEO_HEIGHT		450


static JavaVM *jvm = NULL;
static jobject JniCallbackObj = NULL;
static clientCore *clientCoreP = NULL;
static void *player = NULL;
static void *hw_decoder = NULL;
static bool isHwDecodeSupported;
static bool isHwDecode;
static bool isStopingVideo;
static bool isStopingAudio;
CLetLock mVideoLock;
CLetLock mAudioLock;

jint JNI_OnLoad(JavaVM *vm, void *reserved) {
	DLog1("SDK CLIENT JNI_OnLoad()");
	jvm = vm;
	if (!jvm) {
		return -1;
	}
	JNIEnv* env;
	if (JNI_OK != vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_4)) {
		return -1;
	}
	return JNI_VERSION_1_4;
}

void throwJavaException(JNIEnv *env, const char *msg) {
	// You can put your own exception here
	jclass c = env->FindClass("java/io/RuntimeException");
	if (NULL == c) {
		//B plan: null pointer ...
		c = env->FindClass("java/lang/NullPointerException");
	}
	env->ThrowNew(c, msg);
}

template<typename T>
static std::string toString(const T& t) {
	std::ostringstream ss;
	ss << t;
	return ss.str();
}

void changeDecodeMode(JNIEnv *env, bool _isHwDecode) {
	clientCoreP->setHardDecodec(_isHwDecode);
	mVideoLock.lock();
	isHwDecode = _isHwDecode;
	if (isHwDecode) {
		/* 切硬解 */
#if USE_NATIVE_SURFACE
		player_destroy(player, env);
		player = NULL;
#else
		gl_set_type(env, GL_RENDER_HW);
#endif
	} else {
		/* 切软解 */
#if USE_NATIVE_SURFACE
	if (!player) {
		jobject jsurface = hw_decoder_getSurface(hw_decoder);
		if (jsurface) {
			player = player_create(env, jsurface, VIDEO_WIDTH, VIDEO_HEIGHT);
		}
	}
#else
	gl_set_type(env, GL_RENDER_SW);
#endif
	}
	mVideoLock.unlock();
	return;
}

/*
 * 回调接口
 */
class mediaCallBack: public IAudioPlayCallBack, public IVideoPlayCallBack {
public:
	mediaCallBack(void);
	virtual ~mediaCallBack(void);
	void videoFlip(char *pBuf, int width, int height, bool ifRotate,
			bool ifToBGR);
	virtual void OnAudioData(char* data, int len);
	virtual void OnVideoData(char *data, int len, int width, int height);

private:
	char *tempbuf;
	int tempbuf_len;

	jbyteArray audio_jarray;
	uint32_t audio_jarray_len;

	jbyteArray video_jarray;
	uint32_t video_jarray_len;
};

mediaCallBack::mediaCallBack() {
	tempbuf = NULL;
	tempbuf_len = 0;

	audio_jarray = NULL;
	audio_jarray_len = 0;

	video_jarray = NULL;
	video_jarray_len = 0;

	return;
}

mediaCallBack::~mediaCallBack() {
	if (tempbuf != NULL) {
		free(tempbuf);
		tempbuf = NULL;
		tempbuf_len = 0;
	}
	JNIEnv *env;
	jvm->GetEnv((void**) &env, JNI_VERSION_1_4);
	if (audio_jarray != NULL) {
		env->DeleteGlobalRef(audio_jarray);
		audio_jarray_len = 0;
	}
	if (video_jarray != NULL) {
		env->DeleteGlobalRef(video_jarray);
		video_jarray_len = 0;
	}
	return;
}

void mediaCallBack::videoFlip(char *pBuf, int width, int height, bool ifRotate,
		bool ifToBGR) {
	int m_pixels = 4;
	int len = width * height * m_pixels;
	if (tempbuf == NULL || tempbuf_len != len) {
		if (tempbuf != NULL) {
			free(tempbuf);
		}
		tempbuf_len = len;
		tempbuf = (char *) malloc(tempbuf_len);
	}
	memset(tempbuf, 0, len);
	if (ifRotate && ifToBGR) {
		/* 翻转并转颜色 */
		for (int y = 0; y < height; y++) {
			char*ybuf = pBuf + (height - y - 1) * width * m_pixels; //这里翻转了
			for (int x = 0; x < width; x++) {
				char *xbuf = ybuf + x * m_pixels;
				char *dbuf = tempbuf + y * width * m_pixels + x * m_pixels;
				dbuf[2] = xbuf[0];
				dbuf[1] = xbuf[1];
				dbuf[0] = xbuf[2];
				dbuf[3] = xbuf[3];
			}
		}
	} else if (ifRotate) {
		/* 翻转 */
		for (int y = 0; y < height; y++) {
			char*ybuf = pBuf + (height - y - 1) * width * m_pixels; //这里翻转了
			for (int x = 0; x < width; x++) {
				char *xbuf = ybuf + x * m_pixels;
				char *dbuf = tempbuf + y * width * m_pixels + x * m_pixels;
				dbuf[0] = xbuf[0];
				dbuf[1] = xbuf[1];
				dbuf[2] = xbuf[2];
				dbuf[3] = xbuf[3];
			}
		}
	} else {
		/* 转颜色 */
		for (int y = 0; y < height; y++) {
			char *ybuf = pBuf + y * width * m_pixels;
			for (int x = 0; x < width; x++) {
				char* xbuf = ybuf + x * m_pixels;
				char *dbuf = tempbuf + y * width * m_pixels + x * m_pixels;
				dbuf[2] = xbuf[0];
				dbuf[1] = xbuf[1];
				dbuf[0] = xbuf[2];
				dbuf[3] = xbuf[3];
			}
		}
	}

	memcpy(pBuf, tempbuf, len);
//  free(tempbuf);
	return;
}

void mediaCallBack::OnAudioData(char* data, int len) {
	mAudioLock.lock();
	if (isStopingAudio) {
		LOGD("OnAudioData() SDK is finishing, can not play again !!");
		mAudioLock.unlock();
		return;
	}

	JNIEnv *env;
	jint iRet = -1;
	jvm->GetEnv((void**) &env, JNI_VERSION_1_4);
	if (env == NULL) {
		iRet = jvm->AttachCurrentThread((JNIEnv **) &env, NULL);
	}
	if (JniCallbackObj != NULL) {
		if (audio_jarray == NULL || audio_jarray_len != len) {
			if (audio_jarray != NULL) {
				env->DeleteGlobalRef(audio_jarray);
			}
			audio_jarray_len = len;

			jbyteArray jarray = env->NewByteArray(len);
			audio_jarray = (jbyteArray) env->NewGlobalRef(jarray);
			env->DeleteLocalRef(jarray);
		}
		env->SetByteArrayRegion(audio_jarray, 0, len, (jbyte *) data);

		jclass jniCB = (env)->GetObjectClass(JniCallbackObj);
		jmethodID onAudioData = (env)->GetMethodID(jniCB, "onAudioData",
				"([BI)V");
		if (0 != onAudioData) {
			(env)->CallVoidMethod(JniCallbackObj, onAudioData, audio_jarray,
					len);
			if (env->ExceptionCheck()) {
				DLog("mediaCallBack::OnAudioData(): CallVoidMethod Exception: jmethodID:%d  JniCallbackObj:%p, audio_jarray:%p", onAudioData, JniCallbackObj, audio_jarray);
			}
		}
	}
	if (iRet != -1) {
		jvm->DetachCurrentThread();
	}
	mAudioLock.unlock();
	return;
}

void mediaCallBack::OnVideoData(char *data, int len, int width, int height) {
	mVideoLock.lock();
	if (isStopingVideo) {
		LOGI("OnVideoData() SDK is finishing, can not render again !!");
		mVideoLock.unlock();
		return;
	}
	mVideoLock.unlock();

	JNIEnv *env;
	jint iRet = -1;
	unsigned int pixelSize = width * height;
	bool isSoftDecoded = (len == (unsigned int)(pixelSize * 1.5f) || (len == (pixelSize << 2))); // check if data has been decoded, (4 bytes pre pixel or 1.5 of YUV)

	if (isHwDecode == isSoftDecoded) {
		LOGI("OnVideoData() invaild frame!!");
		return;
	}

	mVideoLock.lock();
	jvm->GetEnv((void**) &env, JNI_VERSION_1_4);
	if (env == NULL) {
		iRet = jvm->AttachCurrentThread((JNIEnv **) &env, NULL);
	}
	mVideoLock.unlock();


	if (isSoftDecoded) {
		/* rendering for software decoding */
		mVideoLock.lock();
#if USE_NATIVE_SURFACE
		player_displayFrame(player, env, data, len, width, height);
#else
		gl_render_frame(env, (unsigned char *)data, len, width, height);
#endif
//		gl_set_framebuffer(data, len, width, height);
//		gl_render_frame(env);
		mVideoLock.unlock();
	} else {
		/* rendering for hardware decoding */
		mVideoLock.lock();
		BOOL result = hw_decoder_decodeFrame(hw_decoder, env, data, len, 1, 1);
		if (!result) {
			mVideoLock.unlock();
			/* 切换为软解码 */
			changeDecodeMode(env, false);
			goto RETURN;
		} else {
			mVideoLock.unlock();
		}
	}

	/* 回调 */
	if (JniCallbackObj != NULL/* && (isSoftDecoded || gl_isHwRending())*/) {
//		if (video_jarray == NULL || video_jarray_len != len) {
//			if (video_jarray != NULL) {
//				env->DeleteGlobalRef(video_jarray);
//			}
//			video_jarray_len = len;
//
//			jbyteArray jarray = env->NewByteArray(len);
//			video_jarray = (jbyteArray)env->NewGlobalRef(jarray);
//			env->DeleteLocalRef(jarray);
//		}
//		env->SetByteArrayRegion(video_jarray, 0, len, (jbyte *)data);

		jclass jniCB = (env)->GetObjectClass(JniCallbackObj);
		jmethodID onVideoData = (env)->GetMethodID(jniCB, "onVideoData",
				"(ZII)V");
		if (0 != onVideoData) {
			(env)->CallVoidMethod(JniCallbackObj, onVideoData, isSoftDecoded, width, height);
//			(env)->CallVoidMethod(JniCallbackObj, onVideoData, isSoftDecoded,
//			NULL, 0, width, height);
		}
	}
	RETURN:
	mVideoLock.lock();
	if (iRet != -1) {
		jvm->DetachCurrentThread();
	}
	mVideoLock.unlock();

	return;
}


static mediaCallBack *mediaCallBackP = NULL;

/*
 * 初始
 */
JNI(jboolean, initAvCoreJni) (JNIEnv *env, jobject obj, jobject ctx,
		jobject cb, jint lbsType, jboolean ifOpenHwDecode, jstring logPath) {
	DLog1("SDK CLIENT clientCoreInitJni");
	isStopingVideo = false;
	isStopingAudio = false;
	LOGI("set stoping audio   init  0");
	bool errFlg = true;
	player = NULL;
	hw_decoder = hw_decoder_create(env);
	isHwDecodeSupported = hw_decoder_isHwSupported(hw_decoder, env);
	if (ifOpenHwDecode) {
		isHwDecode = isHwDecodeSupported;
	} else {
		isHwDecode = false;
	}
	// hook call back
	if (!env) {
		DLog1("SDK CLIENT java env error");
		errFlg = false;
		goto CONNECT_CMD_ERR;
	}
	if (cb) {
		DLog1("SDK CLIENT hook callback");
		JniCallbackObj = (env)->NewGlobalRef(cb);
	}

	if (mediaCallBackP == NULL) {
		mediaCallBackP = new mediaCallBack();
	}
	gl_init(env, isHwDecode ? GL_RENDER_HW : GL_RENDER_SW, hw_decoder);
	if (clientCoreP == NULL) {
		clientCoreP = new clientCore(lbsType); // 0: 正式环境  1: 测试环境
		clientCoreP->setNetParam(10, 3);
		clientCoreP->OnAudioSet(mediaCallBackP);
		clientCoreP->OnVideoSet(mediaCallBackP);
		clientCoreP->setHardDecodec(isHwDecode);
		
		if (logPath != NULL) {
			 char *_logPath;
			  _logPath = (char *)env->GetStringUTFChars(logPath, 0);
			clientCoreP->setAVLogger(_logPath);
		}
	}
	CONNECT_CMD_ERR:
//	(env)->ReleaseStringUTFChars(serviceIP, serviceIP_);
	DLog1("SDK CLIENT clientCoreInitJni  ok");
	return errFlg;
}

/*
 * 注销
 */
JNI(jboolean, freeAvCoreJni) (JNIEnv *env, jobject obj) {
	DLog1("SDK CLIENT clientCoreDeInitJni()");
	isStopingVideo = true;
	isStopingAudio = true;
	LOGI("set stoping audio   free  1");
	if (env) {
		if (JniCallbackObj != NULL) {
			(env)->DeleteGlobalRef(JniCallbackObj);
			JniCallbackObj = NULL;
		}
	}
	if (clientCoreP) {
		delete clientCoreP;
		clientCoreP = NULL;
	}
	if (mediaCallBackP) {
		delete mediaCallBackP;
		mediaCallBackP = NULL;
	}
	player_destroy(player, env);
	player = NULL;
	gl_uninit(env);
	hw_decoder_destroy(env, hw_decoder);
	hw_decoder = NULL;
	DLog("SDK CLIENT freeAvCoreJni() ======== end");
	return true;
}

/*
 * 开始收流
 */
JNI(jboolean, rcvStreamStartJni) (JNIEnv *env, jobject obj, jobject surface,
		jint userId, jint roomId) {
	DLog1("SDK CLIENT clientRcvStreamStartJni()");
	bool errFlg = true;
//	const char *_tcpIp = (env)->GetStringUTFChars(tcpIp, 0);
//	const char *_rtmpAddr = (env)->GetStringUTFChars(rtmpAddr, 0);
	if (clientCoreP == NULL) {
		errFlg = false;
		goto RETURN;
	}
	clientCoreP->clientRcvStreamStart(userId, roomId);
//    (env)->ReleaseStringUTFChars(tcpIp, _tcpIp);
//    (env)->ReleaseStringUTFChars(rtmpAddr, _rtmpAddr);
	isStopingAudio = false;
	LOGI("set stoping audio   start  0");

	mVideoLock.lock();
	isStopingVideo = false;
	if (player != NULL) {
		player_destroy(player, env);
		player = NULL;
	}
	if (!isHwDecode) {
#if USE_NATIVE_SURFACE
		player = player_create(env, surface, VIDEO_WIDTH, VIDEO_HEIGHT);
#endif
	} else {
//		hw_decoder_resetSurface(hw_decoder, env, surface);
	}
	mVideoLock.unlock();
	DLog("SDK CLIENT clientRcvStreamStartJni() ok");

	RETURN:
	return errFlg;
}

/*
 * 停止收流
 */
JNI(jboolean, rcvStreamStopJni) (JNIEnv *env, jobject obj) {
	DLog1("SDK CLIENT clientRcvStreamStopJni()");

	bool errFlg = true;
	if (clientCoreP == NULL) {
		errFlg = false;
		goto RETURN;
	}
	clientCoreP->clientRcvStreamStop();
	mAudioLock.lock();
	isStopingAudio = true;
	LOGI("set stoping audio   stop  1");
	mAudioLock.unlock();

	mVideoLock.lock();
	isStopingVideo = true;
	player_destroy(player, env);
	player = NULL;
//	hw_decoder_close(hw_decoder, env);
	mVideoLock.unlock();

	RETURN:
	return errFlg;
}

JNI(jboolean, muteAudioStreamJni) (JNIEnv *env, jobject obj, jboolean en) {
	DLog1("SDK CLIENT clientMuteAudioStreamJni()");
//	mAudioLock.lock();
//	if (clientCoreP == NULL) {
//		DLog1("SDK CLIENT clientMuteAudioStreamJni() clientCoreP == NULL");
//		mAudioLock.unlock();
//		return false;
//	}
//	isStopingAudio = en;
//	LOGI("set stoping audio   mute  %d", en);
//	mAudioLock.unlock();
	clientCoreP->clientMuteAudioStream(en);
	return true;
}

JNI(jboolean, muteVideoStreamJni) (JNIEnv *env, jobject obj, jboolean en) {
	DLog1("SDK CLIENT clientMuteVideoStreamJni()");
//	scoped_lock locker(&mVideoLock);
	mVideoLock.lock();
	if (clientCoreP == NULL) {
		DLog1("SDK CLIENT clientMuteVideoStreamJni() clientCoreP == NULL");
		mVideoLock.unlock();
		return false;
	}
	isStopingVideo = en;
	mVideoLock.unlock();
	clientCoreP->clientMuteVideoStream(en);
	return true;
}

jstring stoJstring(JNIEnv* env, const char* pat) {
	jclass strClass = env->FindClass("Ljava/lang/String;");
	jmethodID ctorID = env->GetMethodID(strClass, "<init>",
			"([BLjava/lang/String;)V");
	jbyteArray bytes = env->NewByteArray(strlen(pat));
	env->SetByteArrayRegion(bytes, 0, strlen(pat), (jbyte*) pat);
	jstring encoding = env->NewStringUTF("utf-8");
	return (jstring) env->NewObject(strClass, ctorID, bytes, encoding);
}

JNI(jstring, getSDKVerion) (JNIEnv *env, jobject obj) {
	DLog1("SDK CLIENT clientGetSDKVerion()");
//	scoped_lock locker(&mLock);
	return stoJstring(env, MEDIA_SDK_VER);
}

JNI(jobject, getMediaInfo) (JNIEnv *env, jobject obj) {
	string ip;
	string& _ip = ip;
	int port;
	int& _port = port;
//	mLock.lock();
	if (clientCoreP != NULL) {
		clientCoreP->getMediaAddr(_ip, _port);
	}
//	mLock.unlock();
	jclass objClass = env->FindClass(JAVA_CLASS("JJMediaInfo"));
	jmethodID mid = env->GetMethodID(objClass, "<init>", "()V");
	jobject newObj = env->NewObject(objClass, mid);
	jfieldID fid_addr = env->GetFieldID(objClass, "addr", "Ljava/lang/String;");
	jfieldID fid_port = env->GetFieldID(objClass, "port", "I");
	jfieldID fid_width = env->GetFieldID(objClass, "width", "I");
	jfieldID fid_height = env->GetFieldID(objClass, "height", "I");
	env->SetObjectField(newObj, fid_addr, env->NewStringUTF(ip.c_str()));
	env->SetIntField(newObj, fid_port, port);
	env->SetIntField(newObj, fid_width, VIDEO_WIDTH);
	env->SetIntField(newObj, fid_height, VIDEO_HEIGHT);
	return newObj;
}

JNI(jboolean, setHardwareDecode) (JNIEnv *env, jobject obj, jboolean en) {
	if (clientCoreP != NULL) {
		changeDecodeMode(env, en && isHwDecodeSupported);
		return true;
	}
	return false;
}

JNI(jboolean, isHardwareDecode)(JNIEnv *env, jobject obj) {
	return isHwDecode;
}

JNI(jboolean, setMainRoomId) (JNIEnv *env, jobject obj, jint mainRoomId) {
	if (clientCoreP != NULL) {
		clientCoreP->setMainRoom(mainRoomId);
		return true;
	}
	return false;
}

JNI(jboolean, setMicinfo) (JNIEnv *env, jobject obj, jboolean en) {
	if (clientCoreP != NULL) {
		clientCoreP->setMicinfo(en);
		return true;
	}
	return false;
}

JNI(jboolean, networkChange) (JNIEnv *env, jobject obj) {
	if (clientCoreP != NULL) {
		clientCoreP->networkChange();
		return true;
	}
	return false;
}

#if 0
void signInStat() {
int32_t errCode;
string errMsg;

// call back
JNIEnv *env;
jint iRet = -1;
jvm->GetEnv((void**)&env, JNI_VERSION_1_4);
if(env == NULL) {
	iRet = jvm->AttachCurrentThread((JNIEnv **)&env, NULL);
}
if(JniCallbackObj != NULL) {
	jstring errMsg_ = (env)->NewStringUTF(errMsg.c_str());
	jclass jniCB = (env)->GetObjectClass(JniCallbackObj);
	jmethodID signInCB = (env)->GetMethodID(jniCB,
			"signInCB","(ILjava/lang/String;)V");
	if(0 != signInCB) {
		(env)->CallVoidMethod(JniCallbackObj, signInCB, errCode, errMsg_);
	}
}
if(iRet != -1) {
	jvm->DetachCurrentThread();
}
}
#endif

