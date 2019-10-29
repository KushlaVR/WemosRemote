#pragma once

#include <Servo.h>
#include "Console.h"
#include "RoboconMotor.h"

class Stearing
{
	int pin = 0;
	int factPosition = 0;

	int mapPosition(int pos);

public:
	Stearing(int pin);
	~Stearing();
	Servo * servo = nullptr;
	int max_left = 60;
	int max_right = 120;
	int center = 90;

	//positio -100..100
	void setPosition(int position);

	void loop();
};

