// 
// 
// 

#include "Button.h"



void ButtonBase::doPress()
{
	if (press != nullptr) press();
}

void ButtonBase::doHold()
{
	if (hold != nullptr) hold();
}

void ButtonBase::doRelease()
{
	if (release != nullptr) release();
}

bool ButtonBase::isPressed()
{
	return false;
}

bool ButtonBase::isReleased()
{
	return false;
}

void ButtonBase::handle()
{
	if (!isEnabled) return;

	if (pressedTime == 0 && (millis() - releasedTime) > bounce) {// нопка не нажата, ≥ з моменту в≥дпусканн€ пройшло б≥льше €к 200 м≥л≥секунд
		if (isPressed()) {
			pressedTime = millis();
			releasedTime = 0;
			if (isToggleMode) {
				if (isToggled) {
					isToggled = false;
					doRelease();
				}
				else {
					isToggled = true;
					doPress();
				}
			}
			else
				doPress();
		}
	}
	else if (releasedTime == 0 && (millis() - pressedTime) > bounce) {// нопка не в≥дпущена, ≥ з моменту нажа
		if ((millis() - pressedTime) > holdInterval) {
			pressedTime = millis();
			if (!isToggleMode) doHold();
		}
		if (isReleased()) {
			pressedTime = 0;
			releasedTime = millis();
			if (!isToggleMode) doRelease();
		}
	}

}


Button::Button(int pin, void(*press)())
{
	this->pin = pin;
	pinMode(pin, INPUT);
	this->press = press;
}
Button::Button(int pin, void(*press)(), void(*hold)())
{
	this->pin = pin;
	pinMode(pin, INPUT);
	this->press = press;
	this->hold = hold;
}
Button::Button(int pin, void(*press)(), void(*hold)(), void(*release)())
{
	this->pin = pin;
	pinMode(pin, INPUT_PULLUP);
	this->press = press;
	this->hold = hold;
	this->release = release;
}

bool Button::isPressed()
{
	return (digitalRead(pin) == condition);
}

bool Button::isReleased()
{
	return !(digitalRead(pin) == condition);
}


VirtualButton::VirtualButton(void(*press)())
{
	this->bounce = 10;
	this->press = press;
}

VirtualButton::VirtualButton(void(*press)(), void(*hold)())
{
	this->bounce = 10;
	this->press = press;
	this->hold = hold;
}

VirtualButton::VirtualButton(void(*press)(), void(*hold)(), void(*release)())
{
	this->bounce = 10;
	this->press = press;
	this->hold = hold;
	this->release = release;
}

bool VirtualButton::isPressed()
{
	return value == condition;
}

bool VirtualButton::isReleased()
{
	return value != condition;
}

void VirtualButton::setValue(int value)
{
	this->value = value;
	handle();
}

void VirtualButton::reset()
{
	value = LOW;
	isToggled = false;
	doRelease();
}
