package com.ruilin.rlplayer.media;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;

public class RlAudioPlayer {
	private AudioTrack audioPlayer;
	
	
	public RlAudioPlayer() {
	}
	
	public void start(byte[] data, int len) {
		if (audioPlayer == null) {
			int minBuffSize = AudioTrack.getMinBufferSize(48000, AudioFormat.CHANNEL_OUT_STEREO,
					AudioFormat.ENCODING_PCM_16BIT);
			audioPlayer = new AudioTrack(AudioManager.STREAM_MUSIC, 48000, AudioFormat.CHANNEL_OUT_STEREO,
					AudioFormat.ENCODING_PCM_16BIT, 1 * minBuffSize, AudioTrack.MODE_STREAM);
			audioPlayer.play();
		}
		if (audioPlayer.getPlayState() == AudioTrack.PLAYSTATE_PLAYING) {
			audioPlayer.write(data, 0, len);
		}
	}
	
	public void stop() {
		audioPlayer.stop();
		audioPlayer.release();
		audioPlayer = null;
	}
}
