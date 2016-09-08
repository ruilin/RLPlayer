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
#include <pthread.h>
#include <libffmpeg/include/libavcodec/avcodec.h>
#include <libffmpeg/include/libavformat/avformat.h>
#include <libffmpeg/include/libavutil/log.h>
#include <libffmpeg/include/libswscale/swscale.h>
#include "base.h"

typedef void ffmpeg_decoder_on_video_data(void *callbackObject, unsigned char *data, unsigned int len, unsigned short width, unsigned short height);
typedef void ffmpeg_decoder_on_video_end(void *callbackObject);

typedef enum {
	STATUS_PLAYING			= 0,
	STATUS_PAUSE				= 1,
	STATUS_STOP					= 2,
} PLAYER_STATUS;

void *ffmpeg_decoder_create();
void ffmpeg_decoder_destroy(void *ffmpegDecoder);
int ffmpeg_decoder_playerFile(void *ffmpegDecoder, const char *input_str,
													ffmpeg_decoder_on_video_data *onVideoData,
													ffmpeg_decoder_on_video_end *onVideoEnd, void *callbackObject);
void ffmpeg_decoder_resume(void *ffmpegDecoder);
void ffmpeg_decoder_pause(void *ffmpegDecoder);
void ffmpeg_decoder_stop(void *ffmpegDecoder);
PLAYER_STATUS ffmpeg_decoder_getStauts(void *ffmpegDecoder);

#endif /* NATIVE_DECODER_FFMPEG_DECODER_H_ */
