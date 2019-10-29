#include "Stearing.h"



int Stearing::mapPosition(int direction)
{
	int corectedDirection = (direction * direction) / 100;

	if (direction >= -5 && direction <= 5) return center;
	if (direction > 5)//в право
		return map(corectedDirection, 0, 100, center, max_right);
	if (direction < -5)
		return  map(corectedDirection, 0, 100, center, max_left);
}

Stearing::Stearing(int pin)
{
	this->pin = pin;
	pinMode(pin, OUTPUT);
	servo = new Servo();
	servo->attach(pin);
}


Stearing::~Stearing()
{
}

void Stearing::setPosition(int position)
{
	int newPos = mapPosition(position);
	if (newPos != factPosition) {
		console.printf("Stearing->%i\n", newPos);
	}
	factPosition = newPos;

}

void Stearing::loop()
{
	servo->write(factPosition);
}
