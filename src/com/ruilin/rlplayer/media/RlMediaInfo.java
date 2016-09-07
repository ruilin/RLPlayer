package com.ruilin.rlplayer.media;

/**
 * 媒体SDK 信息
 * @author Ruilin
 *
 */
public class RlMediaInfo implements Cloneable {
	/**
	 * 注：成员变量名与JNI层代码对应，不可随意修改
	 */
	String addr;	// ip
	int port;		// 端口
	int width;
	int height;
	
	public String getAddr() {
		return addr;
	}
	public int getPort() {
		return port;
	}
	public int getWidth() {
		return width;
	}
	public int getHeight() {
		return height;
	}
	
	@Override
	public Object clone() {
		try {
			return super.clone();
		} catch (CloneNotSupportedException e) {
			e.printStackTrace();
		}
		return null;
	}
}
