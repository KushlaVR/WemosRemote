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
#define pinWipers D4//Дворники

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

struct {
	int speed;



	int wiperPos;
	int wiperPause;
	int wiperDuration;
	int wiperHalfDuration;
	ulong wiperStartTime;
} state;

BenchMark input_X = BenchMark();
BenchMark input_Y = BenchMark();
BenchMark input_CH3 = BenchMark();
BenchMark input_CH4 = BenchMark();

BenchMark * input_Wipers = &input_CH4;


PCF8574 portExt = PCF8574(0x27);

extBlinker stopLight = extBlinker("Stop light", &portExt);
extBlinker leftLight = extBlinker("Left light", &portExt);
extBlinker rightLight = extBlinker("Right light", &portExt);
extBlinker BackLight = extBlinker("Back light", &portExt);

VirtualButton btnLight = VirtualButton(btnLight_Press, btnLight_Hold, btnLight_Release);

int servoPos = 90;

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

void btnLight_Press() {

}

void btnLight_Hold() {

}

void btnLight_Release() {

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
	//leftLight.debug = true;
	//serialController.leftLight = &leftLight;

	rightLight
		.Add(bitRightLight, 0, HIGH)
		->Add(bitRightLight, 500, 0)
		->Add(bitRightLight, 1000, 0);
	//rightLight.debug = true;
	//serialController.rightLight = &rightLight;


	BackLight
		.Add(bitBackLight, 0, HIGH)
		->Add(bitBackLight, 500, HIGH)
		->repeat = false;
	//BackLight.debug = true;
}



void refreshConfig() {
	stopLight.item(2)->offset = config.stop_light_duration;
	BackLight.item(1)->offset = config.back_light_timeout;
}

void handleStearing() {
	if (!input_X.isValid()) {
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
		return;
	}

	//Проміжки включення правого/лівого поворота
	int center = 90;
	int leftGap = center - input_X.OUT_min;
	int rightGap = center - input_X.OUT_max;

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

void handleSpeed() {
	if (!input_Y.isValid()) {
		state.speed = 0;
		if (BackLight.isRunning()) {
			Serial.println("BackLight end");
			BackLight.end();
		}
		if (stopLight.isRunning()) {
			stopLight.end();
		}
		return;
	}
	int center = 90;
	int forwardGap = center - input_Y.OUT_min;
	int reverceGap = center - input_Y.OUT_max;


	int forward_limit = (forwardGap * config.reverce_limit) / 100;
	int reverce_limit = (reverceGap * config.reverce_limit) / 100;


	int speed = center - input_Y.pos;
	if (input_Y.isChanged) {
		Serial.printf("delta=%i;f=%i;r=%i;c=%i\n", speed, forward_limit, reverce_limit, center);
	}

	if (speed > reverce_limit) {
		if (!BackLight.isRunning()) {
			Serial.println("BackLight begin");
			BackLight.begin();
		}
	}
	else if (-speed > forward_limit) {
		if (BackLight.isRunning()) {
			Serial.println("BackLight end");
			BackLight.end();
		}
	}

	if (state.speed != speed) {
		//швидкість змінилась
		if (abs(speed) < 5) {//Зупинка
			stopLight.begin();
		}
		else
		{
			if (abs(speed) > abs(state.speed)) {//Швидкість зросла
				stopLight.end();
			}
			else {
				if (abs(state.speed - speed) > 5) {//Швидкість впала більше ніж на 10
					stopLight.begin();
				}
			}
		}
		state.speed = speed;
	}

}

void handleWipers() {

	if (input_Wipers->isValid()) {
		if (input_Wipers->pos < 70) {
			state.wiperPos = 0;
		}
		else if (input_Wipers->pos < 110) {
			if (state.wiperStartTime == 0) {
				state.wiperPos = 1;
				state.wiperStartTime = millis();
				state.wiperDuration = 2000;
				state.wiperHalfDuration = 1000;
				state.wiperPause = 2000;
			}
		}
		else {
			if (state.wiperStartTime == 0) {
				state.wiperPos = 2;
				state.wiperStartTime = millis();
				state.wiperDuration = 1400;
				state.wiperHalfDuration = 700;
				state.wiperPause = 200;
			}
		}
	}

	int angle = 0;

	if (state.wiperStartTime != 0) {
		ulong spendTime = millis() - state.wiperStartTime;
		if (spendTime < state.wiperHalfDuration) {
			//Рух в перед
			angle = (spendTime * 180) / state.wiperHalfDuration;
		}
		else if (spendTime < state.wiperDuration) {
			//Рух назад
			spendTime = spendTime - state.wiperHalfDuration;
			angle = 180 - ((spendTime * 180) / state.wiperHalfDuration);
		}
		else if (spendTime < (state.wiperDuration + state.wiperPause)) {
			//Вичікування паузи
			angle = 0;
		}
		else {
			//Кінець циклу
			state.wiperStartTime = 0;
		}
	}

	wipers.write(angle);
}

// the setup function runs once when you press reset or power the board
void setup() {

	Serial.begin(115200);
	console.output = &Serial;
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


	s = config.ssid + "_" + WiFi.macAddress();
	s.replace(":", "");
	strcpy(&SSID[0], s.c_str());

	s = config.password;
	strcpy(&SSID_password[0], s.c_str());

	WiFi.softAP(SSID, SSID_password);

	webServer.setup();
	webServer.on("/api/values", HTTPMethod::HTTP_GET, Values_Get);
	webServer.apName = String(SSID);

	testServo.attach(pinTest);
	wipers.attach(pinWipers);

	setupBlinkers();

	setupController.reloadConfig = &refreshConfig;
	input_X.IN_max = config.ch1_max;
	input_X.IN_min = config.ch1_min;

	input_Y.IN_max = config.ch2_max;
	input_Y.IN_min = config.ch2_min;

	input_CH3.IN_max = config.ch3_max;
	input_CH3.IN_min = config.ch3_min;

	input_CH4.IN_max = config.ch3_max;
	input_CH4.IN_min = config.ch3_min;

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

	input_X.loop();
	input_Y.loop();
	input_CH3.loop();
	input_CH4.loop();

	if (input_X.isChanged) {
		Serial.printf("Servo X = %i (%i)\n", input_X.pos, input_X.ImpulsLength);
	}
	if (input_Y.isChanged) {
		Serial.printf("Servo Y = %i (%i)\n", input_Y.pos, input_Y.ImpulsLength);
	}
	if (input_CH3.isChanged) {
		Serial.printf("Servo CH3 = %i (%i)\n", input_CH3.pos, input_CH3.ImpulsLength);
	}
	if (input_CH4.isChanged) {
		Serial.printf("Servo CH4 = %i (%i)\n", input_CH4.pos, input_CH4.ImpulsLength);
	}

	handleStearing();
	handleSpeed();
	handleWipers();

	stopLight.loop();
	leftLight.loop();
	rightLight.loop();
	BackLight.loop();

	webServer.loop();
}

