#include "Json.h"

JsonString::JsonString(const char * cstr) :String(cstr) {}

void JsonString::appendComa() {
	if (endsWith("\"") || endsWith("}") || endsWith("]")) *this += ",";
}

void JsonString::AddValue(String name, String value)
{
	appendComa();
	*this += "\"" + name + "\":" + "\"" + value + "\"";
}

void JsonString::beginObject()
{
	appendComa();
	*this += "{";
}

void JsonString::endObject()
{
	String s = *static_cast<String*> (this);
	*this += "}";
}


void JsonString::beginArray(String arrayName)
{
	appendComa();
	*this += "\"" + arrayName + "\":[";
}

void JsonString::endArray()
{
	*this += "]";
}

String JsonString::getValue(char * key)
{
	int p = indexOf(key);
	if (p > 0) {
		p = indexOf(":", p + 1);
		if (p > 0) {
			int startIndex = indexOf("\"", p + 1);
			if (startIndex > 0) {
				startIndex++;
				int endIndex = indexOf("\"", startIndex);
				if (endIndex > 0) {
					return substring(startIndex, endIndex);
				}
			}
		}
	}
	return "";
}

int JsonString::getInt(char * key)
{
	String s = getValue(key);
	return s.toInt();
}
