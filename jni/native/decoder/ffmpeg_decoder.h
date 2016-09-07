/*
 * ffmpeg_decoder.h
 *
 *  Created on: 2016年9月5日
 *      Author: Ruilin
 */

#ifndef NATIVE_DECODER_FFMPEG_DECODER_H_
#define NATIVE_DECODER_FFMPEG_DECODER_H_

#include <stdio.h>
#include <time.h>
#include <libffmpeg/include/libavcodec/avcodec.h>
#include <libffmpeg/include/libavformat/avformat.h>
#include <libffmpeg/include/libavutil/log.h>
#include <libffmpeg/include/libswscale/swscale.h>
#include "base.h"

typedef void ffmpeg_decoder_callback_video(void *callbackObject, unsigned char *data, unsigned int len, unsigned short width, unsigned short height);


int decodeFile(char *input_str, ffmpeg_decoder_callback_video *onVideo, void *callbackObject);

#endif /* NATIVE_DECODER_FFMPEG_DECODER_H_ */
