package com.ruilin.rlplayer.media;

/**
 * 数据回调接口
 * @author Ruilin
 *
 */
public interface RlMediaCallback {
	/** 音频数据回调 */
	public void onAudioData(byte []data, int len);
	/** 视频不再回调数据 data == null */
	public void onVideoData(boolean isSoftwareDecoded, int width, int height);
}
