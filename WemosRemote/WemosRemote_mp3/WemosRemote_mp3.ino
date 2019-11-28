#include <Arduino.h>
#include <vector>
#include "AudioFileSourceSPIFFS.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2SNoDAC.h"
#include <AudioFileSourceID3.h>


class Track {
public:
	const char* name;
	AudioFileSourceSPIFFS* file;
	AudioFileSourceID3* id3;
	bool repeat = false;


	Track() {};
	~Track() {};

	void open() {
		file = new AudioFileSourceSPIFFS(name);
		id3 = new AudioFileSourceID3(file);
	}
};

class mp3Player {

private:

	AudioGeneratorMP3* mp3;
	AudioOutputI2SNoDAC* out;

	std::vector<Track*> playlist;
	Track* current = nullptr;
	bool paused = false;
public:
	mp3Player() {
		out = new AudioOutputI2SNoDAC();
		mp3 = new AudioGeneratorMP3();
	}

	~mp3Player() {
	}

	Track* addTrack(const char* name) {
		Track* ret = new Track();
		ret->name = name;
		ret->open();
		playlist.push_back(ret);
		return ret;
	}

	void play(Track* track) {
		paused = false;
		Serial.println(track->name);
		Serial.flush();
		current = track;
		mp3->begin(track->id3, out);
	}

	void play(int index) {
		play(playlist[index]);
	}

	void pause() {
		Serial.println("Paused");
		paused = true;
	}

	void renew() {
		Serial.println("Renew");
		paused = false;
	}

	void loop() {
		if (paused) return;
		if (current == nullptr) return;
		if (!mp3->loop()) {
			if (current->repeat) {
				Serial.println("Repeat");
				Serial.println(current->name);
				Serial.flush();
				mp3->begin(current->id3, out);
			}
		}
	}

	void stop() {
		if (current != nullptr) {
			Serial.println("Stopped");
			Serial.println(current->name);
			Serial.flush();
			current = nullptr;
		}
	}


	bool isRunnin() {
		return (current != nullptr);
	}
};


mp3Player player = mp3Player();

void setup()
{
	Serial.begin(115200);
	delay(1000);
	SPIFFS.begin();

	player.addTrack("/mp3/0.mp3")->repeat = true;



}


ulong start;
int stage = 0;

void loop()
{

	if (stage == 0) {
		if (!player.isRunnin()) {
			player.play(0);
			start = millis();
			stage = 1;
		}
	}

	//ulong ellapsed = (millis() - start);
	//if (stage == 1) {
	//	if (ellapsed > 1000) {
	//		player.pause();
	//		stage = 2;
	//	}
	//}

	//if (stage == 2) {
	//	if (ellapsed > 4000) {
	//		player.renew();
	//		stage = 3;
	//	}
	//}

	//if (stage == 3) {
	//	if (ellapsed > 8000) {
	//		player.stop();
	//		stage = 4;
	//	}
	//}

	//if (stage == 4) {
	//	if (ellapsed > 10000) {
	//		stage = 0;
	//		Serial.println("loop");
	//		Serial.flush();
	//	}
	//}

	player.loop();

}