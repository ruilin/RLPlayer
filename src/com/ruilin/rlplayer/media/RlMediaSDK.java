package com.ruilin.rlplayer.media;

import java.io.File;
import java.text.SimpleDateFormat;
import java.util.Date;

import android.content.Context;
import android.os.Environment;
import android.util.Log;
import android.view.Surface;

/**
 * 媒体SDK
 * @author Ruilin
 *
 */
public class RlMediaSDK {
	public final static int LBS_TYPE_TEST = 1; // 测试环境
	public final static int LBS_TYPE_RELEASE = 0; // 正式环境


	public static void init(Context context, RlMediaCallback callback, boolean ifOpenHwDecode,
			String logPath) {
		init(context, callback, ifOpenHwDecode, null, logPath);
	}

	public static void init(Object context, RlMediaCallback callback, boolean ifOpenHwDecode,
			String libPath, String logPath) {
		try {
			if (libPath == null) {
		    	System.loadLibrary("ffmpeg");
		    	System.loadLibrary("rlplayer");
			} else {
				System.load(libPath);
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
		/* set dir for log files */
		SimpleDateFormat formatter = new SimpleDateFormat("yyyyMMddHHmmss");
		Date curDate = new Date(System.currentTimeMillis());// 获取当前时间
		String fileName = formatter.format(curDate) + ".log";
		if (logPath == null) {
			/* create default dir */
			logPath = Environment.getExternalStorageDirectory().getAbsolutePath() + "/jjmedia/log/";
		}
		File dir = new File(logPath);
		if (dir.exists() || dir.mkdirs()) {
			if (dir.isDirectory()) {
				logPath += ("/" + fileName);
			}
		} else {
			logPath = null;
		}
		initAvCoreJni(context, callback, ifOpenHwDecode, logPath);
	}

	/**
	 * 初始化SDK context：context callback：数据回调接口 lbsType：测试/正式环境
	 * ifSupportHardDecord: 是否支持硬解码
	 */
	private static native boolean initAvCoreJni(Object context, RlMediaCallback callback,
			boolean ifOpenHwDecode, String logPath);
	/**
	 * 销毁SDK
	 */
	public static native boolean freeAvCoreJni();

	/**
	 * 开始接收流。但是在调用之前，使用onVideoSet和onAudioSet先设置好回调
	 * 参数：userId是用户的ID，userId是房间的ID 返回值：true为成功，false为失败.
	 */
	public static native boolean startPlayMediaFile(String path);

	/**
	 * 停止收流。 参数： 返回值：true为成功，false为失败.
	 */
	public static native boolean stop();
	
	public static native boolean pause();

	/**
	 * 关闭音频，发和收流时都会起作用 参数：en为true时，音频会被关闭。false时会打开音频
	 * 返回值：true为成功，false为失败.
	 */
	public static native boolean muteAudioStreamJni(boolean en);

	/**
	 * 关闭视频，发和收流时都会起作用 参数：en为true时，视频会被关闭。false时会打开视频
	 * 返回值：true为成功，false为失败.
	 */
	public static native boolean muteVideoStreamJni(boolean en);
	
	/**
	 * 获取SDK版本号
	 */
	public static native String getSDKVerion();

	/**
	 * 获取媒体库相关信息，目前仅有IP地址和端口
	 */
	public static native RlMediaInfo getMediaInfo();
	
	/**
	 * 设置为硬解码模式，回调将直接返回原数据，由调用者进行硬解码
	 */
	public static native boolean setHardwareDecode(boolean en);
	
	/**
	 * 是否为硬解码 
	 */
	public static native boolean isHardwareDecode();
	
	/**
	 * 如果是在子房间中，则需要设置主房间ID
	 */
	public static native boolean setMainRoomId(int mainRoomId);

	/**
	 * 设置是否在麦
	 */
	public static native boolean setMicinfo(boolean en);

	/**
	 * 通知SDK网络状态发生变化
	 */
	public static native boolean networkChange();
	
	public static native boolean glInit(RlVideoView glView, int width, int height);  
    public static native void glRender(boolean isHwRending);
    public static native void glUninit();
    public static native int glGenTexture();
}
