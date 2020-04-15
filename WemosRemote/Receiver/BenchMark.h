#pragma once
#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif


class BenchMark
{
	ulong m = 0;
	ulong length = 0;

public:

	ulong pos;
	ulong ImpulsLength = 0;
	bool isChanged;

	ulong IN_min = 550;
	ulong IN_max = 2400;

	int OUT_min = 0;
	int OUT_max = 180;

	BenchMark();
	~BenchMark();

	void ICACHE_RAM_ATTR Start();
	void ICACHE_RAM_ATTR Stop();

	void loop();
	int convert(ulong value);
	bool isValid();

};

