/*
 Name:		Test.ino
 Created:	4/15/2020 9:03:21 PM
 Author:	Віталік
*/

#include "PCF8574.h"


#define pinI2C_SCL D1 //pcf8574
#define pinI2C_SDA D2 //pcf8574

PCF8574 portExt = PCF8574(0x27);

// the setup function runs once when you press reset or power the board
void setup() {
	Serial.begin(115200);
	Serial.println("");
	Serial.println("");
	Serial.println("Start...");

	portExt.begin(pinI2C_SDA, pinI2C_SCL);
	portExt.write8(0x00);


	pinMode(BUILTIN_LED, OUTPUT);
}

// the loop function runs over and over again until power down or reset
void loop() {
	delay(1000);
	portExt.write8(0x00);
	digitalWrite(BUILTIN_LED, HIGH);
	delay(1000);
	portExt.write8(0xFF);
	digitalWrite(BUILTIN_LED, LOW);
	Serial.println("step...");

}
