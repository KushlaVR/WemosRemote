#pragma once

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif
#include "Console.h"
#include "PCF8574.h"

class BlinkerItem
{
public:
	int iteration;
	BlinkerItem();
	~BlinkerItem() {};
	BlinkerItem * next;
	int pin;
	unsigned long offset;
	int value;
};

class Blinker
{
	BlinkerItem * first;
	BlinkerItem * last;
	BlinkerItem * current;

	String name;
	unsigned long startTime = 0;

public:
	bool debug = false;
	bool repeat = true;
	int offLevel = LOW;
	Blinker(String name);
	~Blinker();
	void loop();
	Blinker * Add(int pin, unsigned long offset, uint8_t value);
	Blinker * begin() { current = first; startTime = millis(); return this; };
	Blinker * end();
	virtual void setupPin(int pin);
	virtual void write(int pin, int value);
	BlinkerItem * item(int index);
	bool isRunning() { return startTime != 0; };
};


class Beeper :public Blinker {

public:
	Beeper(String name) : Blinker(name) {};
	~Beeper() {};
	virtual void write(int pin, int value);
};


class extBlinker : public Blinker {

public :
	PCF8574 * extPort = nullptr;

	extBlinker(String name, PCF8574 * extPort) : Blinker(name) { this->extPort = extPort; };
	~extBlinker() {};
	virtual void write(int pin, int value);
	virtual void setupPin(int pin);
};
