/**
 * ��򵥵Ļ���FFmpeg����Ƶ������-��׿ - �����
 * Simplest FFmpeg Android Decoder - One Library
 * 
 * ������ Lei Xiaohua
 * leixiaohua1020@126.com
 * �й�ý��ѧ/���ֵ��Ӽ���
 * Communication University of China / Digital TV Technology
 * http://blog.csdn.net/leixiaohua1020
 * 
 * �������ǰ�׿ƽ̨����򵥵Ļ���FFmpeg����Ƶ��������
 * ����Խ��������Ƶ��ݽ����YUV������ݡ�
 * 
 * This software is the simplest decoder based on FFmpeg in Android. 
 * It can decode video stream to raw YUV data.
 * 
 */
package com.ruilin.rlplayer.demo;


import com.ruilin.rlplayer.R;
import com.ruilin.rlplayer.media.RlMediaSDK;

import android.app.Activity;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.Menu;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;

public class MainActivity extends Activity implements OnClickListener {

	EditText urlEdittext_input;
	EditText urlEdittext_output;
	
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        RlMediaSDK.init(this, null, false, null);
        
        setContentView(R.layout.activity_main);
        
        urlEdittext_input = (EditText) this.findViewById(R.id.input_url);
        urlEdittext_output = (EditText) this.findViewById(R.id.output_url);
        
		findViewById(R.id.button_start).setOnClickListener(this);
		findViewById(R.id.button_pause).setOnClickListener(this);
		findViewById(R.id.button_stop).setOnClickListener(this);
		
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.main, menu);
        return true;
    }
    
    //JNI
    public native int decode(String inputurl, String outputurl);

	@Override
	public void onClick(View v) {
		switch (v.getId()) {
		case R.id.button_start:
			String folderurl=Environment.getExternalStorageDirectory().getPath();
			
			String urltext_input=urlEdittext_input.getText().toString();
	        String inputurl=folderurl+"/"+urltext_input;
	        
	        String urltext_output=urlEdittext_output.getText().toString();
	        String outputurl=folderurl+"/"+urltext_output;
	        
	        Log.i("inputurl",inputurl);
	        Log.i("outputurl",outputurl);
	    
//	        decode(inputurl,outputurl);
	        RlMediaSDK.startPlayMediaFile(inputurl);
			break;
		case R.id.button_pause:
			RlMediaSDK.pause();
			break;
		case R.id.button_stop:
			 RlMediaSDK.stop();
			break;
		}
	}
    
//    static{
//    	System.loadLibrary("ffmpeg");
//    	System.loadLibrary("rlplayer");
//    }
}
