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
public:
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


class MotorBase {
private:
	RoboEffects * effect;
	int etalonDuration = 3000;
	int targetSpeed = 0;
	int delta;
	long weight = 10000;
public:
	int controllerType = 0;
	int factSpeed = 0;
	Print * responder;
	String name;
	bool isEnabled = false;

	MotorBase(String name, RoboEffects *effect);
	~MotorBase() {};

	//Задати вагу механхму в грамах
	void setWeight(long weight);
	//задати цільову швидкість
	void setSpeed(int speed);
	void reset();

	void loop();
	virtual void write(int newSpeed);
};

class HBridge : public MotorBase {
private:

	int pwmPin;
	int motorPinA;
	int motorPinB;

public:

	HBridge(String name, int pwmPin, int reley1Pin, int reley2Pin, RoboEffects *effect);
	HBridge(String name, int pinA, int pinB, RoboEffects *effect);

	~HBridge() {};
	
	
	virtual void write(int newSpeed);

};

class SpeedController : public MotorBase {

private:
	int pin;
	Servo * servo = nullptr;

public: 
	SpeedController(String name, int pin, RoboEffects *effect);
	~SpeedController();

	virtual void write(int newSpeed);

};

#endif

