// Button.h

#ifndef _BUTTON_h
#define _BUTTON_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

class ButtonBase {
protected:
	unsigned long pressedTime;
	unsigned long releasedTime;
	void(*press)();
	void(*hold)();
	void(*release)();

	virtual void doPress();
	virtual void doHold();
	virtual void doRelease();
public:
	//Тривалість дребіжання контактів
	int bounce = 200;
	bool isEnabled = true;
	bool isToggleMode = false;
	bool isToggled = false;
	//Час до фіксації кнопки
	unsigned long holdInterval = 2000;
	virtual bool isPressed();
	virtual bool isReleased();
	virtual void handle();
};


class Button :public ButtonBase {
private:
	int pin;

public:
	int condition = 1;
	Button(int pin, void(*press)());
	Button(int pin, void(*press)(), void(*hold)());
	Button(int pin, void(*press)(), void(*hold)(), void(*release)());
	bool isPressed() override;
	bool isReleased() override;
};


class VirtualButton :public ButtonBase {
private:
	int value;

public:
	int condition = HIGH;
	VirtualButton(void(*press)());
	VirtualButton(void(*press)(), void(*hold)());
	VirtualButton(void(*press)(), void(*hold)(), void(*release)());
	bool isPressed() override;
	bool isReleased() override;
	void setValue(int value);
	void reset();

};

#endif

