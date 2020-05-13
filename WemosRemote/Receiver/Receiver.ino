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

#define lightOFF HIGH
#define lightON LOW

ConfigStruct config;


char SSID[32];
char SSID_password[20];

struct {
	int speed;

	int wiperAngle;
	int wiperPos;
	int wiperPause;
	int wiperDuration;
	int wiperHalfDuration;
	ulong wiperStartTime;

	bool parkingLight;
	bool headLight;
	bool highLight;
	bool fogLight;

} state;

BenchMark input_X = BenchMark();
BenchMark input_Y = BenchMark();
BenchMark input_CH3 = BenchMark();
BenchMark input_CH4 = BenchMark();

BenchMark * input_Wipers = &input_CH4;
BenchMark * input_Light = &input_CH3;


//PCF8574 portExt = PCF8574(0x27);
PCF8574 * portExt;// = PCF8574(0x3F);

extBlinker * stopLight;// = extBlinker("Stop light", portExt);
extBlinker * leftLight;// = extBlinker("Left light", portExt);
extBlinker * rightLight;// = extBlinker("Right light", portExt);
extBlinker * BackLight;// = extBlinker("Back light", portExt);

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
	Serial.println("btnLight_Press");
	if (state.parkingLight == false) {
		state.parkingLight = true;
		state.headLight = false;
		state.highLight = false;
		Serial.println("parking");
	}
	else if (state.headLight == false) {
		state.parkingLight = true;
		state.headLight = true;
		state.highLight = false;
		Serial.println("head");
	}
	else if (state.highLight == false) {
		state.parkingLight = true;
		state.headLight = true;
		state.highLight = true;
		Serial.println("high");
	}
	else {
		state.parkingLight = false;
		state.headLight = false;
		state.highLight = false;
		Serial.println("off");
	}
}

void btnLight_Hold() {
	Serial.println("btnLight_Hold");
	if (state.fogLight == true)
		state.fogLight = false;
	else
		state.fogLight = true;
}

void btnLight_Release() {
	Serial.println("btnLight_Release");
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


	stopLight = new extBlinker("Stop light", portExt);
	leftLight = new extBlinker("Left light", portExt);
	rightLight = new extBlinker("Right light", portExt);
	BackLight = new extBlinker("Back light", portExt);

	stopLight
		->Add(bitStopLight, 0, lightOFF)
		->Add(bitStopLight, 0, lightON)
		->Add(bitStopLight, config.stop_light_duration, lightOFF)
		->repeat = false;
	stopLight->debug = true;
	stopLight->offLevel = lightOFF;

	//Налаштування поворотників
	leftLight
		->Add(bitLeftLight, 0, lightON)
		->Add(bitLeftLight, 500, lightOFF)
		->Add(bitLeftLight, 1000, lightOFF);
	leftLight->offLevel = lightOFF;
	//leftLight.debug = true;
	//serialController.leftLight = &leftLight;

	rightLight
		->Add(bitRightLight, 0, lightON)
		->Add(bitRightLight, 500, lightOFF)
		->Add(bitRightLight, 1000, lightOFF);
	rightLight->offLevel = lightOFF;
	//rightLight.debug = true;
	//serialController.rightLight = &rightLight;


	BackLight
		->Add(bitBackLight, 0, lightON)
		->Add(bitBackLight, 500, lightON)
		->repeat = false;
	BackLight->offLevel = lightOFF;
	//BackLight.debug = true;
}



void refreshConfig() {
	stopLight->item(2)->offset = config.stop_light_duration;
	BackLight->item(1)->offset = config.back_light_timeout;
}

void handleStearing() {
	if (!input_X.isValid()) {
		if (leftLight->isRunning())
		{
			Serial.println("left end");
			leftLight->end();
		}
		if (rightLight->isRunning())
		{
			Serial.println("right end");
			rightLight->end();
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
		if (rightLight->isRunning()) {
			Serial.println("right end");
			rightLight->end();
		}
		if (!leftLight->isRunning())
		{
			Serial.println("left begin");
			leftLight->begin();
		}
	}
	else if (delta < rightLimit) {
		if (leftLight->isRunning())
		{
			Serial.println("left end");
			leftLight->end();
		}
		if (!rightLight->isRunning())
		{
			Serial.println("right begin");
			rightLight->begin();
		}
	}
	else {
		if (leftLight->isRunning())
		{
			Serial.println("left end");
			leftLight->end();
		}
		if (rightLight->isRunning())
		{
			Serial.println("right end");
			rightLight->end();
		}
	}
}

void handleSpeed() {
	if (!input_Y.isValid()) {
		state.speed = 0;
		if (BackLight->isRunning()) {
			Serial.println("BackLight end");
			BackLight->end();
		}
		if (stopLight->isRunning()) {
			stopLight->end();
		}
		return;
	}
	int center = 90;
	int forwardGap = center - input_Y.OUT_min;
	int reverceGap = center - input_Y.OUT_max;


	int forward_limit = (forwardGap * config.reverce_limit) / 100;
	int reverce_limit = (reverceGap * config.reverce_limit) / 100;


	int speed = input_Y.pos - center;
	if (input_Y.isChanged) {
		Serial.printf("delta=%i;f=%i;r=%i;c=%i\n", speed, forward_limit, reverce_limit, center);
	}

	if (speed > forward_limit) {
		if (BackLight->isRunning()) {
			Serial.println("BackLight end");
			BackLight->end();
		}
	}
	else if (speed < reverce_limit) {
		if (!BackLight->isRunning()) {
			Serial.println("BackLight begin");
			BackLight->begin();
		}
	}


	if (state.speed != speed) {
		//швидкість змінилась
		if (abs(state.speed) > 5)//передуваємо в русі
		{
			if (abs(speed) < 5) {//Зупинка
				stopLight->begin();
			}
			else
			{
				if (abs(speed) > abs(state.speed)) {//Швидкість зросла
					stopLight->end();
				}
				else {
					if (abs(state.speed - speed) > 5) {//Швидкість впала більше ніж на 10
						stopLight->begin();
					}
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
				state.wiperDuration = config.wiper1Duration;
				state.wiperHalfDuration = config.wiper1Duration / 2;
				state.wiperPause = config.wiper1Pause;
			}
		}
		else {
			if (state.wiperStartTime == 0) {
				state.wiperPos = 2;
				state.wiperStartTime = millis();
				state.wiperDuration = config.wiper2Duration;
				state.wiperHalfDuration = config.wiper2Duration / 2;
				state.wiperPause = config.wiper2Pause;
			}
		}
	}

	int angle = config.wiper0;
	if (state.wiperStartTime != 0) {
		if (!wipers.attached()) {
			wipers.attach(pinWipers, 1000, 2000);
		}
		int gap = config.wiper180 - config.wiper0;
		ulong spendTime = millis() - state.wiperStartTime;
		if (spendTime < state.wiperHalfDuration) {
			//Рух в перед
			angle = config.wiper0 + ((spendTime * gap) / state.wiperHalfDuration);
		}
		else if (spendTime < state.wiperDuration) {
			//Рух назад
			spendTime = spendTime - state.wiperHalfDuration;
			angle = config.wiper180 - ((spendTime * gap) / state.wiperHalfDuration);
		}
		else if (spendTime < (state.wiperDuration + state.wiperPause)) {
			//Вичікування паузи
			angle = config.wiper0;
		}
		else {
			//Кінець циклу
			state.wiperStartTime = 0;
		}
		if (state.wiperAngle != angle) {
			wipers.write(angle);
			state.wiperAngle = angle;
		}
	}
	else {
		if (wipers.attached()) {
			wipers.detach();
		}
	}
}

void handleHeadLight() {
	if (input_Light->pos > 90)
		btnLight.setValue(HIGH);
	else
		btnLight.setValue(LOW);

	if (state.parkingLight) {
		portExt->write(bitParkingLight, lightON);
		if (state.fogLight)
			portExt->write(bitFogLight, lightON);
		else
			portExt->write(bitFogLight, lightOFF);
	}
	else {
		portExt->write(bitParkingLight, lightOFF);
		portExt->write(bitFogLight, lightOFF);
	}

	if (state.headLight)
		portExt->write(bitHeadLight, lightON);
	else
		portExt->write(bitHeadLight, lightOFF);

	if (state.highLight)
		portExt->write(bitHighLight, lightON);
	else
		portExt->write(bitHighLight, lightOFF);

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

	portExt = new PCF8574(config.port_addr);
	portExt->begin(pinI2C_SDA, pinI2C_SCL);
	portExt->write8(0xFF);


	s = config.ssid + "_" + WiFi.macAddress();
	s.replace(":", "");
	strcpy(&SSID[0], s.c_str());

	s = config.password;
	strcpy(&SSID_password[0], s.c_str());

	WiFi.softAP(SSID, SSID_password);

	webServer.setup();
	webServer.on("/api/values", HTTPMethod::HTTP_GET, Values_Get);
	webServer.apName = String(SSID);

	testServo.attach(pinTest, 1000, 2000);

	setupBlinkers();

	setupController.reloadConfig = &refreshConfig;
	input_X.IN_max = config.ch1_max;
	input_X.IN_center = config.ch1_center;
	input_X.IN_min = config.ch1_min;

	input_Y.IN_max = config.ch2_max;
	input_Y.IN_center = config.ch2_center;
	input_Y.IN_min = config.ch2_min;

	input_CH3.IN_max = config.ch3_max;
	input_CH3.IN_center = config.ch3_min + ((config.ch3_max - config.ch3_min) / 2);
	input_CH3.IN_min = config.ch3_min;

	input_CH4.IN_max = config.ch4_max;
	input_CH4.IN_center = config.ch4_min + ((config.ch4_max - config.ch4_min) / 2);
	input_CH4.IN_min = config.ch4_min;

	btnLight.condition = HIGH;

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
	handleHeadLight();

	stopLight->loop();
	leftLight->loop();
	rightLight->loop();
	BackLight->loop();

	webServer.loop();
}

