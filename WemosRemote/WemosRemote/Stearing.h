#pragma once

#include <Servo.h>
#include "Console.h"
#include "RoboconMotor.h"
#include "SetupController.h"

class Stearing
{
	int pin = 0;
	int factPosition = 0;

	int mapPosition(int pos, PotentiometerLinearity stearing_linearity);

public:
	Stearing(int pin);
	~Stearing();
	Servo * servo = nullptr;
	int max_left = 60;
	int max_right = 120;
	int center = 90;
	bool isEnabled = false;

	//positio -100..100
	void setPosition(int position, PotentiometerLinearity stearing_linearity);

	void loop();
};

