// Json.h

#ifndef _JSON_h
#define _JSON_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

class JsonString :public String {
public:
	JsonString(const char *cstr = "");
	void appendComa();
	void AddValue(String name, String value);
	void beginObject();
	void endObject();
	void beginArray(String arrayName);
	void endArray();
	String getValue(char* key);
	int getInt(char* key);
};

#endif

