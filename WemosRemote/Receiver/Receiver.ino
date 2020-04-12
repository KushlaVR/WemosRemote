/*
 Name:		Receiver.ino
 Created:	3/16/2020 8:26:34 PM
 Author:	Віталік
*/
#include <ESP8266WiFi.h> 
#include <ESP8266WebServer.h> 
#include <DNSServer.h>
#include <ESP8266mDNS.h>

#include <FS.h>
#include <Wire.h>
#include <Servo.h>

#include "Console.h"
#include "Json.h"
#include "Blinker.h"
#include "Button.h"
#include "Blinker.h"
#include "PCF8574.h"
#include "BenchMark.h"
#include "WebUIController.h"
#include "SetupController.h"

#define pinServo_X D5
#define pinServo_Y D6
#define pinServo_CH3 D7
#define pinServo_CH4 D8

#define pinTest D0//Тестовий пін
#define pinWipers D8//Дворники

#define pinI2C_SCL D1 //pcf8574
#define pinI2C_SDA D2 //pcf8574

//pcf8574 Port usage
#define bitParkingLight 0
#define bitHeadLight 1
#define bitHighLight 2
#define bitFogLight 3
#define bitLeftLight 4
#define bitRightLight 5
#define bitBackLight 6
#define bitStopLight 7

ConfigStruct config;


char SSID[32];
char SSID_password[20];



BenchMark input_X = BenchMark();
BenchMark input_Y = BenchMark();
BenchMark input_CH3 = BenchMark();
BenchMark input_CH4 = BenchMark();

PCF8574 portExt = PCF8574(0x27);

extBlinker stopLight = extBlinker("Stop light", &portExt);
extBlinker leftLight = extBlinker("Left light", &portExt);
extBlinker rightLight = extBlinker("Right light", &portExt);

VirtualButton out_BackLight = VirtualButton(out_BackLight_On, nullptr, out_BackLight_Off);

int servoPos = 90;
int wipersPos = 0;

Servo testServo = Servo();
Servo wipers = Servo();

bool interruptAttached = false;

void ICACHE_RAM_ATTR  pinServo_X_CHANGE() {
	if (digitalRead(pinServo_X))
		input_X.Start();
	else
		input_X.Stop();
}

void ICACHE_RAM_ATTR  pinServo_Y_CHANGE() {
	if (digitalRead(pinServo_Y))
		input_Y.Start();
	else
		input_Y.Stop();
}

void ICACHE_RAM_ATTR  pinServo_CH3_CHANGE() {
	if (digitalRead(pinServo_CH3))
		input_CH3.Start();
	else
		input_CH3.Stop();
}

void ICACHE_RAM_ATTR  pinServo_CH4_CHANGE() {
	if (digitalRead(pinServo_CH4))
		input_CH4.Start();
	else
		input_CH4.Stop();
}



void out_BackLight_On() {
	portExt.write(bitBackLight, HIGH);
	console.println("out_BackLight_On");
}
void out_BackLight_Off() {
	portExt.write(bitBackLight, LOW);
	console.println("out_BackLight_Off");
}


void printValues(JsonString * out)
{
	out->beginObject();
	out->AddValue("ch1_val", String(input_X.ImpulsLength));
	out->AddValue("ch2_val", String(input_Y.ImpulsLength));
	out->AddValue("ch3_val", String(input_CH3.ImpulsLength));
	out->AddValue("ch4_val", String(input_CH4.ImpulsLength));
	out->endObject();
}


void Values_Get() {
	JsonString ret = JsonString();
	printValues(&ret);
	webServer.jsonOk(&ret);
}


void setupBlinkers() {
	stopLight.Add(bitStopLight, 0, 0)
		->Add(bitStopLight, 0, HIGH)
		->Add(bitStopLight, config.stop_light_duration, 0)
		->repeat = false;
	stopLight.debug = true;

	//Налаштування поворотників
	leftLight
		.Add(bitLeftLight, 0, HIGH)
		->Add(bitLeftLight, 500, 0)
		->Add(bitLeftLight, 1000, 0);
	leftLight.debug = true;
	//serialController.leftLight = &leftLight;

	rightLight
		.Add(bitRightLight, 0, HIGH)
		->Add(bitRightLight, 500, 0)
		->Add(bitRightLight, 1000, 0);
	rightLight.debug = true;
	//serialController.rightLight = &rightLight;

}



void refreshConfig() {
	stopLight.item(2)->offset = config.stop_light_duration;
}

void handleTurnLight() {
	//Проміжки включення правого/лівого поворота
	int center = input_X.convert(config.ch1_center);
	int leftGap = center - 0;
	int rightGap = center - 180;

	//Фактичне відхилення 
	int leftLimit = (leftGap * config.turn_light_limit) / 100;
	int rightLimit = (rightGap * config.turn_light_limit) / 100;

	int delta = center - input_X.pos;

	//Serial.printf("delta=%i;ll=%i;rl=%i;c=%i\n", delta, leftLimit, rightLimit, center);

	if (delta > leftLimit) {
		if (rightLight.isRunning()) {
			Serial.println("right end");
			rightLight.end();
		}
		if (!leftLight.isRunning())
		{
			Serial.println("left begin");
			leftLight.begin();
		}
	}
	else if (delta < rightLimit) {
		if (leftLight.isRunning())
		{
			Serial.println("left end");
			leftLight.end();
		}
		if (!rightLight.isRunning())
		{
			Serial.println("right begin");
			rightLight.begin();
		}
	}
	else {
		if (leftLight.isRunning())
		{
			Serial.println("left end");
			leftLight.end();
		}
		if (rightLight.isRunning())
		{
			Serial.println("right end");
			rightLight.end();
		}
	}
}

// the setup function runs once when you press reset or power the board
void setup() {

	Serial.begin(115200);
	Serial.println("");
	Serial.println("");
	Serial.println("Start...");

	pinMode(pinServo_X, INPUT);
	pinMode(pinServo_Y, INPUT);
	pinMode(pinServo_CH3, INPUT);
	pinMode(pinServo_CH4, INPUT);

	digitalWrite(pinServo_X, LOW);
	digitalWrite(pinServo_Y, LOW);
	digitalWrite(pinServo_CH3, LOW);
	digitalWrite(pinServo_CH4, LOW);

	String s;
	if (!SPIFFS.begin()) {
		console.println(F("No file system!"));
		console.println(F("Fomating..."));
		if (SPIFFS.format())
			console.println(F("OK"));
		else {
			console.println(F("Fail.... rebooting..."));
			while (true);
		}
	}

	if (SPIFFS.exists("/intro.txt")) {
		File f = SPIFFS.open("/intro.txt", "r");
		s = f.readString();
		console.println(s.c_str());
	}
	else {
		console.println(("Starting..."));
	}

	setupController.cfg = &config;
	setupController.loadConfig();

	portExt.begin(pinI2C_SDA, pinI2C_SCL);
	portExt.write8(0x00);

	webServer.on("/api/values", HTTPMethod::HTTP_GET, Values_Get);

	s = config.ssid + "_" + WiFi.macAddress();
	s.replace(":", "");
	strcpy(&SSID[0], s.c_str());

	s = config.password;
	strcpy(&SSID_password[0], s.c_str());

	WiFi.softAP(SSID, SSID_password);

	webServer.setup();
	webServer.apName = String(SSID);

	testServo.attach(pinTest);
	wipers.attach(pinWipers);

	setupBlinkers();

	setupController.reloadConfig = &refreshConfig;

	Serial.println("Starting");
}

// the loop function runs over and over again until power down or reset
void loop() {

	if (!interruptAttached) {
		Serial.println("interrupt attached");
		attachInterrupt(digitalPinToInterrupt(pinServo_X), pinServo_X_CHANGE, CHANGE);
		attachInterrupt(digitalPinToInterrupt(pinServo_Y), pinServo_Y_CHANGE, CHANGE);
		attachInterrupt(digitalPinToInterrupt(pinServo_CH3), pinServo_CH3_CHANGE, CHANGE);
		attachInterrupt(digitalPinToInterrupt(pinServo_CH4), pinServo_CH4_CHANGE, CHANGE);
		interruptAttached = true;
	}

	if (Serial.available()) {
		String s = Serial.readStringUntil('\n');
		servoPos = s.toInt();
		while (Serial.available()) Serial.read();
	}

	testServo.write(servoPos);
	wipers.write(wipersPos);

	input_X.loop();
	input_Y.loop();
	input_CH3.loop();
	input_CH4.loop();

	if (input_X.isChanged) {
		Serial.printf("Servo X = %i (%i)\n", input_X.pos, input_X.ImpulsLength);
	}
	if (input_Y.isChanged) {
		Serial.printf("Servo Y = %i (%i)\n", input_Y.pos, input_X.ImpulsLength);
	}
	if (input_CH3.isChanged) {
		Serial.printf("Servo CH3 = %i (%i)\n", input_CH3.pos, input_X.ImpulsLength);
	}
	if (input_CH4.isChanged) {
		Serial.printf("Servo CH4 = %i (%i)\n", input_CH4.pos, input_X.ImpulsLength);
	}

	handleTurnLight();

	stopLight.loop();
	leftLight.loop();
	rightLight.loop();

}

