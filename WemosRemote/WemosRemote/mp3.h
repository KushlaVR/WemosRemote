#pragma once

#include <AudioFileSourceSPIFFS.h>
#include <AudioFileSourceID3.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2SNoDAC.h>

AudioGeneratorMP3 *mp3;

AudioFileSourceSPIFFS *mp3_file;
AudioFileSourceID3 *mp3_id3;

AudioOutputI2SNoDAC *out;


ulong mp3Start;
ulong mp3Timeout;

void initMP3()
{
	out = new AudioOutputI2SNoDAC();
	mp3 = new AudioGeneratorMP3();
}

void stopMP3()
{
	mp3->stop();
	Serial.println("Stopped mp3");
	Serial.flush();
	mp3Timeout = 0;
}

void playMP3(const char* path, ulong timeout = 0)
{
	Serial.print("begin mp3:");
	Serial.println(path);
	Serial.flush();

	mp3_file = new AudioFileSourceSPIFFS(path);
	mp3_id3 = new AudioFileSourceID3(mp3_file);
	mp3->begin(mp3_id3, out);
	if (!mp3->loop()) { mp3->stop(); }
	mp3Timeout = timeout;
	mp3Start = millis();
}


void loopMP3() {
	if (mp3->isRunning()) {
		if (!mp3->loop()) {
			stopMP3();
		}
		else {
			if (mp3Timeout != 0) {
				if ((millis() - mp3Start) > mp3Timeout) {
					stopMP3();
				}
			}
		}
	}
}