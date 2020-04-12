#include "BenchMark.h"



BenchMark::BenchMark()
{
}


BenchMark::~BenchMark()
{
}

void BenchMark::Start()
{
	m = micros();
}

void BenchMark::Stop()
{
	length = micros() - m;
}

void BenchMark::loop()
{
	ulong newPos = 0;
	if (ImpulsLength != length) {
		ImpulsLength = length;
		newPos = convert(ImpulsLength);

		if (pos != newPos) {
			isChanged = true;
			pos = newPos;
		}
		else
		{
			isChanged = false;
		}
	}
	else
		isChanged = false;
}

int BenchMark::convert(ulong value)
{
	if (value < IN_min) {
		return OUT_min;
	}
	else if (value > IN_max) {
		return OUT_max;
	}
	else {
		return map(value, IN_min, IN_max, OUT_min, OUT_max);
	}
}
