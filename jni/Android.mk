LOCAL_PATH := $(call my-dir)

# Program
include $(CLEAR_VARS)
LOCAL_MODULE := rlplayer

LOCAL_SRC_FILES :=	native/rlplayer_main.c							\
									native/decoder/ffmpeg_decoder.c		\
									native/decoder/hw_decoder.c				\
									native/player/gl_renderer.c					\
									native/player/player.c							\

LOCAL_C_INCLUDES += $(LOCAL_PATH)/libffmpeg/include	\
									+= $(LOCAL_PATH)/native
									
LOCAL_LDLIBS := -landroid -llog -lz -lGLESv2 -lEGL
LOCAL_SHARED_LIBRARIES := ffmpeg


include $(BUILD_SHARED_LIBRARY)


include $(LOCAL_PATH)/libffmpeg/Android.mk
