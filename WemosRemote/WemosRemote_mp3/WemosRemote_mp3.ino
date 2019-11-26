﻿#include <Arduino.h>
#include "AudioFileSourceSPIFFS.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2SNoDAC.h"

AudioGeneratorMP3 *mp3;
AudioFileSourceSPIFFS *file;
AudioOutputI2SNoDAC *out;
void setup()
{
	Serial.begin(115200);
	delay(1000);
	SPIFFS.begin();
	file = new AudioFileSourceSPIFFS("/mp3/pno-cs.mp3");
	out = new AudioOutputI2SNoDAC();
	mp3 = new AudioGeneratorMP3();
	mp3->begin(file, out);
}

void loop()
{
	if (mp3->isRunning()) {
		if (!mp3->loop()) mp3->stop();
	}
	else {
		Serial.printf("MP3 done\n");
		delay(1000);
	}
}