#include "Console.h"

Console console = Console();

Console::Console()
{
}


Console::~Console()
{
}

size_t Console::write(uint8_t b)
{
	if (output != nullptr) return output->write(b);
	return 1;
}
