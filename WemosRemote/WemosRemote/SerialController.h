#pragma once
#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

#include <Servo.h>
#include "RoboconMotor.h"
#include "Blinker.h"

class SerialController
{
	int cmdPos = 0;
	char cmd[256];

public:
	SerialController();
	~SerialController();

	bool isRunning = false;
	RoboMotor * motor = nullptr;
	Servo * stearing = nullptr;
	Blinker * leftLight = nullptr;
	Blinker * rightLight = nullptr;
	Blinker  * siren1 = nullptr;

	void loop();

	void cmdMotor(String cmd);
	void cmdStearing(String cmd);
	void cmdFlash(String cmd);
};

