#pragma once
#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

#include <Servo.h>
#include "RoboconMotor.h"
#include "Stearing.h"
#include "Blinker.h"
#include "Console.h"

class SerialController
{
	int cmdPos = 0;
	char cmd[256];

public:
	SerialController();
	~SerialController();

	bool isRunning = false;
	MotorBase * motor = nullptr;
	Stearing * stearing = nullptr;
	Blinker * leftLight = nullptr;
	Blinker * rightLight = nullptr;
	Blinker  * siren1 = nullptr;

	void loop();

	void cmdMotor(String cmd);
	void cmdStearing(String cmd);
	void cmdFlash(String cmd);
};

