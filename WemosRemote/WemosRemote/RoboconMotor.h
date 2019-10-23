// RoboconMotor.h

#ifndef _ROBOCONMOTOR_h
#define _ROBOCONMOTOR_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

class RoboEffects {

private:
	long start = 0;
public :
	//Duration millisecond (1000 miliseconds = 1 second)
	long span = 0;
	long duration = 1000;
	long fullProgress = 1000;
	long halfProgress = 500;
	void begin();
	int softStart();
	int softEnd();
	int softStartSoftEnd();
};


class RoboMotor {
private:
	RoboEffects * effect;
	int etalonDuration = 3000;
	int targetSpeed = 0;
	int delta;
	long weight = 10000;
	int pwmPin;
	int motorPinA;
	int motorPinB;

public:
	int factSpeed = 0;
	Print * responder;
	String name;
	RoboMotor(String name, int pwmPin, int reley1Pin, int reley2Pin, RoboEffects *effect);
	RoboMotor(String name, int pinA, int pinB, RoboEffects *effect);
	//Задати вагу механхму в грамах
	void setWeight(long weight);
	//Обробка сенсорів
	void loop();
	//задати цільову швидкість
	void setSpeed(int speed);
    void reset();

};

#endif

