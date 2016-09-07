package com.ruilin.rlplayer.media;

import java.io.IOException;
import java.nio.ByteBuffer;

import android.annotation.TargetApi;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaCodecInfo.CodecCapabilities;
import android.media.MediaCodecList;
import android.media.MediaFormat;
import android.os.Build;
import android.util.Log;
import android.view.Surface;

/**
 * 硬解码器
 * @author Ruilin
 */
public class RlVideoDecoder {
	private static final String TAG = RlVideoDecoder.class.getSimpleName();
	private static final String H264_MIME_TYPE = "video/avc";
	private static final int ERROR_NOT_SAME_THREAD = -5;
	private static final int ERROR_IllEGAL_STATE = -6;
	private static final int DEQUEUE_INPUT_TIMEOUT = 100000; // timeout (us)
	private static final int DEQUEUE_OUT_TIMEOUT = 0;
	private static final String[] supportedH264HwCodecPrefixes = { "OMX.qcom.", "OMX.Intel.", "OMX.Exynos." };
	private static final int COLOR_QCOM_FORMATYUV420PackedSemiPlanar32m = 0x7FA30C04;
	private static final int[] supportedColorList = { 
			CodecCapabilities.COLOR_FormatYUV420Planar,
			CodecCapabilities.COLOR_FormatYUV420SemiPlanar, 
			CodecCapabilities.COLOR_QCOM_FormatYUV420SemiPlanar,
			COLOR_QCOM_FORMATYUV420PackedSemiPlanar32m 
			};
	
	private static final int DECODE_SUCCESS = 0;
	private static final int DECODE_FAIL = 1;
	private static final int DECODE_ERROR = 2;
	
	private static RlVideoDecoder sDecoder;
	
	private MediaCodec mMediaCodec;
	private MediaCodec.BufferInfo mInfo;
	private Thread mediaCodecThread;


	private RlVideoDecoder() {
		mInfo = new MediaCodec.BufferInfo();
	}
	
	public static RlVideoDecoder getInstance() {
		if (sDecoder == null) {
			sDecoder = new RlVideoDecoder();
		}
		return sDecoder;
	}

	public synchronized boolean open(final Surface surface, final int width, final int height) {
		if (surface == null || !surface.isValid()) {
			Log.e(TAG, "open() surface == null || !surface.isValid()");
			return false;
		}
		if (mMediaCodec != null) {
			Log.e(TAG, "open() mediaCodec already exists");
			return false;
		}
		mediaCodecThread = Thread.currentThread();
		try {
			mMediaCodec = MediaCodec.createDecoderByType(H264_MIME_TYPE);
			if (mMediaCodec == null) {
				Log.e(TAG, "MediaCodec create failed!!");
				return false;
			}
			MediaFormat mediaFormat = MediaFormat.createVideoFormat(H264_MIME_TYPE, width, height);
			mMediaCodec.configure(mediaFormat, surface, null, 0);
			mMediaCodec.start();
		} catch (IllegalStateException e) {
			e.printStackTrace();
			return false;
		} catch (IOException e) {
			e.printStackTrace();
			return false;
		}
		return true;
	}

	public synchronized void close() {
		if (mMediaCodec == null) {
			return;
		}
		try {
			Log.i(TAG, "MediaCodec.close()");
			mMediaCodec.stop();
			mMediaCodec.release();
			mMediaCodec = null;
		} catch (IllegalStateException e) {
			e.printStackTrace();
		}
	}
	
	public boolean isOpen() {
		return (mMediaCodec != null);
	}

	@TargetApi(Build.VERSION_CODES.JELLY_BEAN)
	public int decodeFrame(final byte[] data, final int len, final int width, final int height) {
		if (mMediaCodec == null) {
			Log.e(TAG, "decodeFrame() mediaCodec == null");
			return DECODE_ERROR;
		}
		int inputIndex = dequeueInputBuffer();
		switch (inputIndex) {
			case ERROR_IllEGAL_STATE:
				Log.e(TAG, "dequeueInputBuffer(): ERROR_IllEGAL_STATE!!");
				return DECODE_ERROR;
			case ERROR_NOT_SAME_THREAD:
				Log.e(TAG, "dequeueInputBuffer(): ERROR_NOT_SAME_THREAD!!");
				return DECODE_ERROR;
			case -1:
				Log.e(TAG, "dequeueInputBuffer(): no such buffer is currently available !!!");
				return DECODE_FAIL;
			default:
				break;
		}
		if (inputIndex >= 0) {
			ByteBuffer[] buffers = mMediaCodec.getInputBuffers();
			ByteBuffer inputBuffer = buffers[inputIndex];
			inputBuffer.clear();
			inputBuffer.put(data, 0, len);
			queueInputBuffer(inputIndex, len, 0); //mCount * 1000000 / FRAME_RATE
			dequeueAndRenderOutputBuffer(0);
		} else {
			if (!dequeueAndRenderOutputBuffer(0) && !dequeueAndRenderOutputBuffer(30 * 1000)) {
			}
		}
		if ((mInfo.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0) {
			return DECODE_FAIL;
		}

//		MediaCodec.BufferInfo bufferInfo = new MediaCodec.BufferInfo();
//		int outIndex = dequeueOutputBuffer(bufferInfo);
//			/*
//			switch (outIndex) {
//			case MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED:
//				Log.d(TAG, "INFO_OUTPUT_BUFFERS_CHANGED");
//				break;
//			case MediaCodec.INFO_OUTPUT_FORMAT_CHANGED:
//				Log.d(TAG, "New format " + mediaCodec.getOutputFormat());
//				break;
//			case MediaCodec.INFO_TRY_AGAIN_LATER:
//				Log.d(TAG, "dequeueOutputBuffer timed out!");
//				break;
//			default:
//				break;
//			}
//			*/
//		// release
//		while (outIndex >= 0) {
//			if (!releaseOutputBuffer(outIndex, true)) {
//				break;
//			}
//			outIndex = dequeueOutputBuffer(bufferInfo);
//		}
		return DECODE_SUCCESS;
	}

	public boolean dequeueAndRenderOutputBuffer(int outtime) {
		int outIndex = mMediaCodec.dequeueOutputBuffer(mInfo, outtime);
		switch (outIndex) {
			case MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED:
				return false;
			case MediaCodec.INFO_OUTPUT_FORMAT_CHANGED:
				return false;
			case MediaCodec.INFO_TRY_AGAIN_LATER:
				return false;
			case MediaCodec.BUFFER_FLAG_SYNC_FRAME:
				return false;
			default:
				mMediaCodec.releaseOutputBuffer(outIndex, true);//show image right now
				return true;
		}
	}

	@TargetApi(Build.VERSION_CODES.JELLY_BEAN)
	private boolean queueInputBuffer(int inputBufferIndex, int size, long timestampUs) {
		try {
			checkOnMediaCodecThread();
			mMediaCodec.queueInputBuffer(inputBufferIndex, 0, size, timestampUs, 0);
			return true;
		} catch (IllegalStateException e) {
			return false;
		} catch (RuntimeException e) {
			return false;
		}
	}

	@TargetApi(Build.VERSION_CODES.JELLY_BEAN)
	private int dequeueInputBuffer() {
		try {
			checkOnMediaCodecThread();
			return mMediaCodec.dequeueInputBuffer(DEQUEUE_INPUT_TIMEOUT);
		} catch (IllegalStateException e) {
			return ERROR_IllEGAL_STATE;
		} catch (RuntimeException e) {
			return ERROR_NOT_SAME_THREAD;
		}
	}

	@TargetApi(Build.VERSION_CODES.JELLY_BEAN)
	private int dequeueOutputBuffer(MediaCodec.BufferInfo bufferInfo) {
		try {
			checkOnMediaCodecThread();
			return mMediaCodec.dequeueOutputBuffer(bufferInfo, DEQUEUE_OUT_TIMEOUT);
		} catch (IllegalStateException e) {
			return ERROR_IllEGAL_STATE;
		} catch (RuntimeException e) {
			return ERROR_NOT_SAME_THREAD;
		}
	}

	@TargetApi(Build.VERSION_CODES.JELLY_BEAN)
	private boolean releaseOutputBuffer(int index, boolean render) {
		try {
			checkOnMediaCodecThread();
			mMediaCodec.releaseOutputBuffer(index, render);
			return true;
		} catch (IllegalStateException e) {
			Log.e(TAG, "releaseOutputBuffer failed", e);
			return false;
		} catch (RuntimeException e) {
			return false;
		}
	}

	// Helper struct for findVp8Decoder() below.
	private static class DecoderProperties {
		public DecoderProperties(String codecName, int colorFormat) {
			this.codecName = codecName;
			this.colorFormat = colorFormat;
		}

		public final String codecName; // OpenMax component name for VP8 codec.
		public final int colorFormat; // Color format supported by codec.
	}

	@TargetApi(Build.VERSION_CODES.JELLY_BEAN)
	private static DecoderProperties findDecoder(String mime, String[] supportedCodecPrefixes) {
		if (Build.VERSION.SDK_INT < Build.VERSION_CODES.KITKAT) {
			return null; // MediaCodec.setParameters is missing.
		}
		for (int i = 0; i < MediaCodecList.getCodecCount(); ++i) {
			MediaCodecInfo info = MediaCodecList.getCodecInfoAt(i);
			if (info.isEncoder()) {
				continue;
			}
			String name = null;
			for (String mimeType : info.getSupportedTypes()) {
				if (mimeType.equals(mime)) {
					name = info.getName();
					break;
				}
			}
			if (name == null) {
				continue; // No HW support in this codec; try the next one.
			}
			Log.v(TAG, "Found candidate decoder " + name);

			// Check if this is supported decoder.
			boolean supportedCodec = false;
			for (String codecPrefix : supportedCodecPrefixes) {
				if (name.startsWith(codecPrefix)) {
					supportedCodec = true;
					break;
				}
			}
			if (!supportedCodec) {
				continue;
			}

			// Check if codec supports either yuv420 or nv12.
			CodecCapabilities capabilities = info.getCapabilitiesForType(mime);
			for (int colorFormat : capabilities.colorFormats) {
				Log.v(TAG, "   Color: 0x" + Integer.toHexString(colorFormat));
			}
			for (int supportedColorFormat : supportedColorList) {
				for (int codecColorFormat : capabilities.colorFormats) {
					if (codecColorFormat == supportedColorFormat) {
						// Found supported HW decoder.
						Log.d(TAG,
								"Found target decoder " + name + ". Color: 0x" + Integer.toHexString(codecColorFormat));
						return new DecoderProperties(name, codecColorFormat);
					}
				}
			}
		}
		return null; // No HW decoder.
	}


	private void checkOnMediaCodecThread() {
//		if (mediaCodecThread.getId() != Thread.currentThread().getId()) {
//			throw new RuntimeException("MediaCodecVideoDecoder previously operated on " + mediaCodecThread
//					+ " but is now called on " + Thread.currentThread());
//		}
	}

	public static boolean isH264HwSupported() {
		return findDecoder(H264_MIME_TYPE, supportedH264HwCodecPrefixes) != null;
	}

}
