package com.ruilin.rlplayer.media;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import android.content.Context;
import android.graphics.SurfaceTexture;
import android.opengl.GLES11Ext;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.opengl.GLSurfaceView.Renderer;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;

/**
 * @author Ruilin
 * http://blog.csdn.net/yanzi1225627/article/details/33339965
 */
public class RlVideoView extends GLSurfaceView implements Renderer, SurfaceTexture.OnFrameAvailableListener {
	private static final String TAG = RlVideoView.class.getSimpleName();
	private static final int RENDER_TYPE_HW	= 0;
	private static final int RENDER_TYPE_SW	= 1;
	private SurfaceTexture mSurfaceTexture;
	private int mUpdateSurface = 0;
//	private DirectDrawer mDirectDrawer;
	private boolean mClearColor;

	public RlVideoView(Context context, AttributeSet attrs) {
		super(context, attrs);
		setEGLContextClientVersion(2);
		setRenderer(this);
		setRenderMode(RENDERMODE_WHEN_DIRTY);
		
		int textureID = RlMediaSDK.glGenTexture();
		mSurfaceTexture = new SurfaceTexture(textureID);
		mSurfaceTexture.setOnFrameAvailableListener(this);
		Surface surface = new Surface(mSurfaceTexture);
		RlVideoDecoder.getInstance().open(surface, 800, 450);
		surface.release();
	}

	@Override
	public void surfaceDestroyed(SurfaceHolder holder) {
		super.surfaceDestroyed(holder);
		RlMediaSDK.glUninit();
		RlVideoDecoder.getInstance().close();
	}
	
	@Override
	public void onSurfaceCreated(GL10 gl, EGLConfig config) {
		Log.i(TAG, "onSurfaceCreated...");
//		int textureID = JJMediaSDK.glGenTexture();
//		mSurfaceTexture = new SurfaceTexture(textureID);
//		mSurfaceTexture.setOnFrameAvailableListener(this);
		
//		mDirectDrawer = new DirectDrawer(mTextureID);
	}

	@Override
	public void onSurfaceChanged(GL10 gl, int width, int height) {
		// TODO Auto-generated method stub
		Log.i(TAG, "onSurfaceChanged...");
//		GLES20.glViewport(0, 0, width, height);
		RlMediaSDK.glInit(this, width, height);
		clearColor();
	}

	@Override
	public synchronized void onDrawFrame(GL10 gl) {
		// TODO Auto-generated method stub
		synchronized (this) {
			if (mClearColor) {
				mClearColor = false;
				GLES20.glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
				GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT | GLES20.GL_DEPTH_BUFFER_BIT);
				return;
			}
		}
		
		boolean isHwRending = false;
		synchronized (this) {
			if (mUpdateSurface > 0) {
				isHwRending = true;
				mSurfaceTexture.updateTexImage();
				mUpdateSurface--;	
			}
		}
//		switch (mRenderType) {
//		case RENDER_TYPE_HW: 
//			/* hardware decoder render */
////			openHwDecoder();
////			GLES20.glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
////			GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT | GLES20.GL_DEPTH_BUFFER_BIT);
////			float[] mtx = new float[16];
////			mSurface.getTransformMatrix(mtx);
////			mDirectDrawer.draw(mtx);
//			openHwDecoder();
//			JJMediaSDK.glRender(mTextureID);
//			break;
//			
//		case RENDER_TYPE_SW:
//			/* software deocder render */
//			JJMediaSDK.glRender(-1);
//			break;
//		default:
//			break;
//		}
		RlMediaSDK.glRender(isHwRending);
	}

	@Override
	public void onFrameAvailable(SurfaceTexture surfaceTexture) {
		/**
		 * if this function does not invoked, 
		 * may be due to not invoking @SurfaceTexture.updateTexImage().
		 */
		synchronized (this) {
			mUpdateSurface++;
			this.requestRender();
		}
	}
	
	private void clearColor() {
		synchronized (this) {
			mClearColor = true;
			requestRender();
		}
	}
	
	private void openHwDecoder() {
		if (RlVideoDecoder.getInstance().isOpen() || mSurfaceTexture == null) {
			return;
		}
		Surface surface = new Surface(mSurfaceTexture);
		RlVideoDecoder.getInstance().open(surface, 800, 450);
		surface.release();
	}
	
	private void closeHwDecoder() {
		RlVideoDecoder.getInstance().close();	
	}
	
	private int createTextureID() {
		int[] texture = new int[1];
		GLES20.glGenTextures(1, texture, 0);
		GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, texture[0]);
		GLES20.glTexParameterf(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GL10.GL_TEXTURE_MIN_FILTER, GL10.GL_LINEAR);
		GLES20.glTexParameterf(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GL10.GL_TEXTURE_MAG_FILTER, GL10.GL_LINEAR);
		GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GL10.GL_TEXTURE_WRAP_S, GL10.GL_CLAMP_TO_EDGE);
		GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GL10.GL_TEXTURE_WRAP_T, GL10.GL_CLAMP_TO_EDGE);
		return texture[0];
	}

	public SurfaceTexture getSurfaceTexture() {
		return mSurfaceTexture;
	}

//	private synchronized static void setRenderType(JJVideoView view, int renderType) {
//		mRenderType = renderType;
////		mDirectDrawer.reset();
//		switch (renderType) {
//		case RENDER_TYPE_HW:
//			if (view != null) {
//				view.openHwDecoder();
//			}
//			break;
//		case RENDER_TYPE_SW:
//			if (JJVideoDecoder.getInstance().isOpen()) {
//				JJVideoDecoder.getInstance().close();
//			}
//			break;
//		default:
//			break;
//		}
//	}
	
    public Surface getSurface() {
    	SurfaceHolder holder = getHolder();
    	if (holder != null) {
    		return holder.getSurface();
    	}
    	return null;
    }

}