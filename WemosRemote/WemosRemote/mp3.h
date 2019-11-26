#pragma once

#include <AudioFileSourceSPIFFS.h>
#include <AudioFileSourceID3.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2SNoDAC.h>

AudioGeneratorMP3 *mp3;

AudioFileSourceSPIFFS *mp3_file;
AudioFileSourceID3 *mp3_id3;

AudioOutputI2SNoDAC *out;

void initMP3()
{
	out = new AudioOutputI2SNoDAC();
	mp3 = new AudioGeneratorMP3();
}

void stopMP3()
{
	mp3->stop();
	Serial.println("Stopped mp3");
}

void playMP3(const char* path)
{
	Serial.print("begin mp3:");
	Serial.println(path);

	mp3_file = new AudioFileSourceSPIFFS(path);
	mp3_id3 = new AudioFileSourceID3(mp3_file);
	mp3->begin(mp3_id3, out);
	if (!mp3->loop()) { mp3->stop(); }
}
