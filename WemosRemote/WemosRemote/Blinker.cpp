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
	unsigned long offset = millis() - startTime;//������ ���� ������� �� �������
	BlinkerItem * item = current;
	if (item == nullptr) return;//���� ���� �������� �� ���� - ����� �� ������
	if (offset < item->offset) return;//���� ��� �� �� ������ - ��������
	analogWrite(item->pin, item->value);//��� ������
	if (debug) {
		Serial.print(name);
		Serial.printf(": %i->%i\n", item->pin, item->value);
	}
	current = item->next;//���������� �� ���������� ��������
	if (repeat) {
		if (current == nullptr) {//����� ������, �������� � �������
			current = first;
			startTime = millis();
		}
	}
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
		analogWrite(item->pin, 0);
		item = item->next;
	}
	return this;
}

BlinkerItem::BlinkerItem()
{
	next = nullptr;
}
