#pragma once
#include "Print.h"

class Console: public Print
{

public:
	Console();
	~Console();

	Print * output = nullptr;

	virtual size_t write(uint8_t b);
};


extern Console console;
