#include "Blinker.h"



Blinker::Blinker(String name)
{
	first = nullptr;
	current = nullptr;
	this->name = name;
}


Blinker::~Blinker()
{
}

void Blinker::loop()
{
	if (startTime == 0) return;
	unsigned long offset = millis() - startTime;//—к≥льки часу пройшло в≥д початку
	BlinkerItem * item = current;
	if (item == nullptr) return;//якщо немаЇ елемента на черз≥ - н≥чого не робимо
	if (offset < item->offset) return;//якщо час ще не настав - виходимо
	write(item->pin, item->value);//„ас настав
	if (debug) {
		console.print(name);
		console.printf(": %i->%i\n", item->pin, item->value);
	}
	current = item->next;//ѕереходимо до наступного елемента
	if (repeat) {
		if (current == nullptr) {//к≥нець списку, починаЇмо з початку
			current = first;
			startTime = millis();
		}
	}
	if (current == nullptr) end();
}

Blinker * Blinker::Add(int pin, unsigned long offset, uint8_t value)
{
	pinMode(pin, OUTPUT);
	BlinkerItem * item = new BlinkerItem();
	item->pin = pin;
	item->offset = offset;
	item->value = value;
	if (first == nullptr) {
		first = item;
		last = item;
	}
	else {
		last->next = item;
		last = item;
	}
	return this;
}

Blinker * Blinker::end()
{
	current = nullptr;
	startTime = 0;
	BlinkerItem * item = first;
	while (item != nullptr)
	{
		write(item->pin, 0);
		item = item->next;
	}
	return this;
}

void Blinker::write(int pin, int value)
{
	if (value == 0)
		digitalWrite(pin, LOW);
	else if (value == 255)
		digitalWrite(pin, HIGH);
	else
		analogWrite(pin, value);
}

BlinkerItem * Blinker::item(int index)
{
	int i = 0;
	BlinkerItem * item = first;
	while (item != nullptr)
	{
		if (i == index) return item;
		item = item->next;
		i++;
	}
	return nullptr;
}

BlinkerItem::BlinkerItem()
{
	next = nullptr;
}

void Beeper::write(int pin, int value)
{
	if (value == 0)
		noTone(pin);
	else {
		analogWriteFreq(value);
		tone(pin, value);
	}
}
