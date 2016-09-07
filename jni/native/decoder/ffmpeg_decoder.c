

#include "ffmpeg_decoder.h"

//Output FFmpeg's av_log()
void custom_log(void *ptr, int level, const char* fmt, va_list vl) {
	FILE *fp = fopen("/storage/emulated/0/av_log.txt", "a+");
	if (fp) {
		vfprintf(fp, fmt, vl);
		fflush(fp);
		fclose(fp);
	}
}

JNIEXPORT jint JNICALL Java_com_ruilin_rlplayer_MainActivity_decode(JNIEnv *env, jobject obj, jstring input_jstr, jstring output_jstr) {
	AVFormatContext *pFormatCtx;
	int i, videoindex;
	AVCodecContext *pCodecCtx;
	AVCodec *pCodec;
	AVFrame *pFrame, *pFrameYUV;
	uint8_t *out_buffer;
	AVPacket *packet;
	int y_size;
	int ret, got_picture;
	struct SwsContext *img_convert_ctx;
	FILE *fp_yuv;
	int frame_cnt;
	clock_t time_start, time_finish;
	double time_duration = 0.0;

	char input_str[500] = { 0 };
	char output_str[500] = { 0 };
	char info[1000] = { 0 };
	sprintf(input_str, "%s", (*env)->GetStringUTFChars(env, input_jstr, NULL));
	sprintf(output_str, "%s",
			(*env)->GetStringUTFChars(env, output_jstr, NULL));

	//FFmpeg av_log() callback
	av_log_set_callback(custom_log);

	av_register_all();
	avformat_network_init();
	pFormatCtx = avformat_alloc_context();

	if (avformat_open_input(&pFormatCtx, input_str, NULL, NULL) != 0) {
		LOGE("Couldn't open input stream.\n");
		return -1;
	}
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
		LOGE("Couldn't find stream information.\n");
		return -1;
	}
	videoindex = -1;
	for (i = 0; i < pFormatCtx->nb_streams; i++)
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoindex = i;
			break;
		}
	if (videoindex == -1) {
		LOGE("Couldn't find a video stream.\n");
		return -1;
	}
	pCodecCtx = pFormatCtx->streams[videoindex]->codec;
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL) {
		LOGE("Couldn't find Codec.\n");
		return -1;
	}
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		LOGE("Couldn't open codec.\n");
		return -1;
	}

	pFrame = av_frame_alloc();
	pFrameYUV = av_frame_alloc();
	out_buffer = (unsigned char *) av_malloc(
			av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width,
					pCodecCtx->height, 1));
	av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, out_buffer,
			AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);

	packet = (AVPacket *) av_malloc(sizeof(AVPacket));

	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
			pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height,
			AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

	sprintf(info, "[Input     ]%s\n", input_str);
	sprintf(info, "%s[Output    ]%s\n", info, output_str);
	sprintf(info, "%s[Format    ]%s\n", info, pFormatCtx->iformat->name);
	sprintf(info, "%s[Codec     ]%s\n", info, pCodecCtx->codec->name);
	sprintf(info, "%s[Resolution]%dx%d\n", info, pCodecCtx->width,
			pCodecCtx->height);

	fp_yuv = fopen(output_str, "wb+");
	if (fp_yuv == NULL) {
		printf("Cannot open output file.\n");
		return -1;
	}

	frame_cnt = 0;
	time_start = clock();

	while (av_read_frame(pFormatCtx, packet) >= 0) {
		if (packet->stream_index == videoindex) {
			ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture,
					packet);
			if (ret < 0) {
				LOGE("Decode Error.\n");
				return -1;
			}
			if (got_picture) {
				sws_scale(img_convert_ctx,
						(const uint8_t* const *) pFrame->data, pFrame->linesize,
						0, pCodecCtx->height, pFrameYUV->data,
						pFrameYUV->linesize);

				y_size = pCodecCtx->width * pCodecCtx->height;
				fwrite(pFrameYUV->data[0], 1, y_size, fp_yuv);    //Y
				fwrite(pFrameYUV->data[1], 1, y_size / 4, fp_yuv);  //U
				fwrite(pFrameYUV->data[2], 1, y_size / 4, fp_yuv);  //V
				//Output info
				char pictype_str[10] = { 0 };
				switch (pFrame->pict_type) {
				case AV_PICTURE_TYPE_I:
					sprintf(pictype_str, "I");
					break;
				case AV_PICTURE_TYPE_P:
					sprintf(pictype_str, "P");
					break;
				case AV_PICTURE_TYPE_B:
					sprintf(pictype_str, "B");
					break;
				default:
					sprintf(pictype_str, "Other");
					break;
				}
				LOGI("Frame Index: %5d. Type:%s", frame_cnt, pictype_str);
				frame_cnt++;
			}
		}
		av_free_packet(packet);
	}
	//flush decoder
	//FIX: Flush Frames remained in Codec
	while (1) {
		ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
		if (ret < 0)
			break;
		if (!got_picture)
			break;
		sws_scale(img_convert_ctx, (const uint8_t* const *) pFrame->data,
				pFrame->linesize, 0, pCodecCtx->height, pFrameYUV->data,
				pFrameYUV->linesize);
		int y_size = pCodecCtx->width * pCodecCtx->height;
		fwrite(pFrameYUV->data[0], 1, y_size, fp_yuv);    //Y
		fwrite(pFrameYUV->data[1], 1, y_size / 4, fp_yuv);  //U
		fwrite(pFrameYUV->data[2], 1, y_size / 4, fp_yuv);  //V
		//Output info
		char pictype_str[10] = { 0 };
		switch (pFrame->pict_type) {
		case AV_PICTURE_TYPE_I:
			sprintf(pictype_str, "I");
			break;
		case AV_PICTURE_TYPE_P:
			sprintf(pictype_str, "P");
			break;
		case AV_PICTURE_TYPE_B:
			sprintf(pictype_str, "B");
			break;
		default:
			sprintf(pictype_str, "Other");
			break;
		}
		LOGI("Frame Index: %5d. Type:%s", frame_cnt, pictype_str);
		frame_cnt++;
	}
	time_finish = clock();
	time_duration = (double) (time_finish - time_start);

	sprintf(info, "%s[Time      ]%fms\n", info, time_duration);
	sprintf(info, "%s[Count     ]%d\n", info, frame_cnt);

	sws_freeContext(img_convert_ctx);

	fclose(fp_yuv);

	av_frame_free(&pFrameYUV);
	av_frame_free(&pFrame);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);

	return 0;
}


int decodeFile(char *input_str, ffmpeg_decoder_callback_video *onVideo, void *callbackObject) {
	AVFormatContext *pFormatCtx;
	int i, videoindex;
	AVCodecContext *pCodecCtx;
	AVCodec *pCodec;
	AVFrame *pFrame, *pFrameYUV;
	uint8_t *out_buffer;
	AVPacket *packet;
	int y_size;
	int yuv_size;
	int ret, got_picture;
	struct SwsContext *img_convert_ctx;
	int frame_cnt;
	clock_t time_start, time_finish;
	double time_duration = 0.0;

	char info[1000] = { 0 };

	//FFmpeg av_log() callback
	av_log_set_callback(custom_log);

	av_register_all();
	avformat_network_init();
	pFormatCtx = avformat_alloc_context();

	if (avformat_open_input(&pFormatCtx, input_str, NULL, NULL) != 0) {
		LOGE("Couldn't open input stream.\n");
		return -1;
	}
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
		LOGE("Couldn't find stream information.\n");
		return -1;
	}
	videoindex = -1;
	for (i = 0; i < pFormatCtx->nb_streams; i++)
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoindex = i;
			break;
		}
	if (videoindex == -1) {
		LOGE("Couldn't find a video stream.\n");
		return -1;
	}
	pCodecCtx = pFormatCtx->streams[videoindex]->codec;
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL) {
		LOGE("Couldn't find Codec.\n");
		return -1;
	}
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		LOGE("Couldn't open codec.\n");
		return -1;
	}

	pFrame = av_frame_alloc();
	pFrameYUV = av_frame_alloc();
	out_buffer = (unsigned char *) av_malloc(
			av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width,
					pCodecCtx->height, 1));
	av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, out_buffer,
			AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);

	packet = (AVPacket *) av_malloc(sizeof(AVPacket));

	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
			pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height,
			AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

	sprintf(info, "[Input     ]%s\n", input_str);
	sprintf(info, "%s[Format    ]%s\n", info, pFormatCtx->iformat->name);
	sprintf(info, "%s[Codec     ]%s\n", info, pCodecCtx->codec->name);
	sprintf(info, "%s[Resolution]%dx%d\n", info, pCodecCtx->width,
			pCodecCtx->height);

//	fp_yuv = fopen(output_str, "wb+");
//	if (fp_yuv == NULL) {
//		printf("Cannot open output file.\n");
//		return -1;
//	}

	frame_cnt = 0;
	time_start = clock();

	y_size = pCodecCtx->width * pCodecCtx->height;
	yuv_size = y_size * 3 / 2;
	unsigned char *mOutputBuffer = malloc(yuv_size);

	while (av_read_frame(pFormatCtx, packet) >= 0) {
		if (packet->stream_index == videoindex) {
			ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture,
					packet);
			if (ret < 0) {
				LOGE("Decode Error.\n");
				return -1;
			}
			if (got_picture) {
				sws_scale(img_convert_ctx,
						(const uint8_t* const *) pFrame->data, pFrame->linesize,
						0, pCodecCtx->height, pFrameYUV->data,
						pFrameYUV->linesize);

//				fwrite(pFrameYUV->data[0], 1, y_size, fp_yuv);    //Y
//				fwrite(pFrameYUV->data[1], 1, y_size / 4, fp_yuv);  //U
//				fwrite(pFrameYUV->data[2], 1, y_size / 4, fp_yuv);  //V
		        int i, j, k;
		        for (i = 0; i < pCodecCtx->height; i++) {
		        	memcpy(mOutputBuffer + pCodecCtx->width * i, pFrameYUV->data[0] + pFrameYUV->linesize[0] * i, pCodecCtx->width);
		        }
		        for (j = 0; j < pCodecCtx->height / 2; j++) {
		        	memcpy(mOutputBuffer + pCodecCtx->width * i + (pCodecCtx->width >> 1) * j, pFrameYUV->data[1] + pFrameYUV->linesize[1] * j, (pCodecCtx->width >> 1));
		        }
		        for (k = 0; k < pCodecCtx->height / 2; k++) {
		            memcpy(mOutputBuffer + pCodecCtx->width * i + (pCodecCtx->width >> 1) * j + (pCodecCtx->width >> 1) * k, pFrameYUV->data[2] + pFrameYUV->linesize[2] * k, (pCodecCtx->width >> 1));
		        }
				onVideo(callbackObject, mOutputBuffer, yuv_size, pCodecCtx->width, pCodecCtx->height);
				//Output info
				char pictype_str[10] = { 0 };
				switch (pFrame->pict_type) {
				case AV_PICTURE_TYPE_I:
					sprintf(pictype_str, "I");
					break;
				case AV_PICTURE_TYPE_P:
					sprintf(pictype_str, "P");
					break;
				case AV_PICTURE_TYPE_B:
					sprintf(pictype_str, "B");
					break;
				default:
					sprintf(pictype_str, "Other");
					break;
				}
				LOGI("Frame Index: %5d. Type:%s", frame_cnt, pictype_str);
				frame_cnt++;
			}
		}
		av_free_packet(packet);
	}
	//flush decoder
	//FIX: Flush Frames remained in Codec
	while (1) {
		ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
		if (ret < 0)
			break;
		if (!got_picture)
			break;
		sws_scale(img_convert_ctx, (const uint8_t* const *) pFrame->data,
				pFrame->linesize, 0, pCodecCtx->height, pFrameYUV->data,
				pFrameYUV->linesize);
		int y_size = pCodecCtx->width * pCodecCtx->height;
//		fwrite(pFrameYUV->data[0], 1, y_size, fp_yuv);    //Y
//		fwrite(pFrameYUV->data[1], 1, y_size / 4, fp_yuv);  //U
//		fwrite(pFrameYUV->data[2], 1, y_size / 4, fp_yuv);  //V


        int i, j, k;
        for (i = 0; i < pCodecCtx->height; i++) {
        	memcpy(mOutputBuffer + pCodecCtx->width * i, pFrameYUV->data[0] + pFrameYUV->linesize[0] * i, pCodecCtx->width);
        }
        for (j = 0; j < pCodecCtx->height / 2; j++) {
        	memcpy(mOutputBuffer + pCodecCtx->width * i + (pCodecCtx->width >> 1) * j, pFrameYUV->data[1] + pFrameYUV->linesize[1] * j, (pCodecCtx->width >> 1));
        }
        for (k = 0; k < pCodecCtx->height / 2; k++) {
            memcpy(mOutputBuffer + pCodecCtx->width * i + (pCodecCtx->width >> 1) * j + (pCodecCtx->width >> 1) * k, pFrameYUV->data[2] + pFrameYUV->linesize[2] * k, (pCodecCtx->width >> 1));
        }
		onVideo(callbackObject, mOutputBuffer, yuv_size, pCodecCtx->width, pCodecCtx->height);


		//Output info
		char pictype_str[10] = { 0 };
		switch (pFrame->pict_type) {
		case AV_PICTURE_TYPE_I:
			sprintf(pictype_str, "I");
			break;
		case AV_PICTURE_TYPE_P:
			sprintf(pictype_str, "P");
			break;
		case AV_PICTURE_TYPE_B:
			sprintf(pictype_str, "B");
			break;
		default:
			sprintf(pictype_str, "Other");
			break;
		}
		LOGI("Frame Index: %5d. Type:%s", frame_cnt, pictype_str);
		frame_cnt++;
	}
	time_finish = clock();
	time_duration = (double) (time_finish - time_start);

	sprintf(info, "%s[Time      ]%fms\n", info, time_duration);
	sprintf(info, "%s[Count     ]%d\n", info, frame_cnt);

	sws_freeContext(img_convert_ctx);

//	fclose(fp_yuv);

	av_frame_free(&pFrameYUV);
	av_frame_free(&pFrame);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);

	free(mOutputBuffer);
	mOutputBuffer = NULL;
	return 0;
}

