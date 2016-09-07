#LOCAL_PATH := $(call my-dir)
#include $(CLEAR_VARS)
#
#LOCAL_MODULE    := native
#
#LOCAL_LDLIBS += -llog
#
##LOCAL_SRC_FILES :=    \
##clientCoreJni.cpp		\
##player/player.c			\
##player/hw_decoder.c			\
#
#
#LOCAL_CFLAGS := -DGOOGLE_PROTOBUF_NO_RTTI=1 -DHAVE_SYS_POLL_H -DHAVE_ALLOCA_H -DHAVE_UNISTD_H
#LOCAL_CFLAGS += -Wall -O2 -DSYS=posix -DNO_CRYPTO #librtmp
#LOCAL_CPPFLAGS := -std=c++11
#LOCAL_CPPFLAGS += -fexceptions #json
## curl
#LOCAL_CFLAGS += -D_GNU_SOURCE
#LOCAL_CPPFLAGS += -frtti 
#
## -landroid: supports player.c to use <android/native_window.h>
#LOCAL_EXPORT_LDLIBS := -lz -landroid
#LOCAL_EXPORT_CFLAGS := $(LOCAL_CFLAGS)
#LOCAL_EXPORT_CPPFLAGS := $(LOCAL_CPPFLAGS)
#LOCAL_EXPORT_C_INCLUDES := $(LOCAL_C_INCLUDES)
#
#LOCAL_C_INCLUDES = $(LOCAL_PATH)
#
#
#include $(BUILD_STATIC_LIBRARY)
