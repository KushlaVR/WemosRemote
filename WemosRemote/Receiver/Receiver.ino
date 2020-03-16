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
#include <Servo.h>

#include "Console.h"
#include "Json.h"
#include "WebUIController.h"


#define pinServo_X D5
#define pinServo_Y D6
#define pinServo_CH3 D7
#define pinServo_CH4 D8


struct Config {

	int ch1_min;
	int ch1_max;

	int ch2_min;
	int ch2_max;
	
	int ch3_min;
	int ch3_max;
	
	int ch4_min;
	int ch4_max;
} config;
Config * _config = &config;


struct State {
	int ch1;
	int ch2;
	int ch3;
	int ch4;
} state;
State * _state = &state;



char SSID[32];
char SSID_password[20];


bool interruptAttached = false;
ulong servoXStart = 0;
ulong servoXLen = 0;
ulong last_servoXLen = 0;
int lastPos = 0;
int pos = 0;


void ICACHE_RAM_ATTR  pinServo_X_CHANGE() {
	ulong m = micros();
	if (digitalRead(pinServo_CH4)) {
		servoXStart = m;
	}
	else
	{
		servoXLen = m - servoXStart;
	}
}


void printConfig(JsonString * out)
{
	out->beginObject();
	out->AddValue("ch1_min", String(_config->ch1_min));
	out->AddValue("ch1_max", String(_config->ch1_max));

	out->AddValue("ch2_min", String(_config->ch2_min));
	out->AddValue("ch2_max", String(_config->ch2_max));

	out->AddValue("ch3_min", String(_config->ch3_min));
	out->AddValue("ch3_max", String(_config->ch3_max));

	out->AddValue("ch4_min", String(_config->ch4_min));
	out->AddValue("ch4_max", String(_config->ch4_max));

	out->endObject();
}


void printValues(JsonString * out)
{
	out->beginObject();
	out->AddValue("ch1_val", String(_state->ch1));
	out->AddValue("ch2_val", String(_state->ch2));
	out->AddValue("ch3_val", String(_state->ch3));
	out->AddValue("ch4_val", String(_state->ch4));
	out->endObject();
}

void Setup_Get()
{
	JsonString ret = JsonString();
	printConfig(&ret);
	webServer.jsonOk(&ret);
}

void Setup_Post()
{
	if (webServer.hasArg("ch1_min")) { _config->ch1_min = webServer.arg("ch1_min").toInt(); }
	if (webServer.hasArg("ch1_max")) { _config->ch1_max = webServer.arg("ch1_min").toInt(); }

	if (webServer.hasArg("ch2_min")) { _config->ch2_min = webServer.arg("ch2_min").toInt(); }
	if (webServer.hasArg("ch2_max")) { _config->ch2_max = webServer.arg("ch2_min").toInt(); }

	if (webServer.hasArg("ch3_min")) { _config->ch3_min = webServer.arg("ch3_min").toInt(); }
	if (webServer.hasArg("ch3_max")) { _config->ch3_max = webServer.arg("ch3_min").toInt(); }

	if (webServer.hasArg("ch4_min")) { _config->ch4_min = webServer.arg("ch4_min").toInt(); }
	if (webServer.hasArg("ch4_max")) { _config->ch4_max = webServer.arg("ch4_min").toInt(); }

	webServer.Ok();
}

void Values_Get() {
	JsonString ret = JsonString();
	printValues(&ret);
	webServer.jsonOk(&ret);
}

// the setup function runs once when you press reset or power the board
void setup() {
	Serial.println("");
	Serial.println("");

	Serial.begin(115200);
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

	webServer.on("/api/setup", HTTPMethod::HTTP_GET, Setup_Get);
	webServer.on("/api/setup", HTTPMethod::HTTP_POST, Setup_Post);

	webServer.on("/api/values", HTTPMethod::HTTP_GET, Values_Get);


	s = "LIGHT_" + WiFi.macAddress();
	s.replace(":", "");
	strcpy(&SSID[0], s.c_str());

	s = "12345678";
	strcpy(&SSID_password[0], s.c_str());

	WiFi.softAP(SSID, SSID_password);

	webServer.setup();
	webServer.apName = String(SSID);

	Serial.println("Starting");
}

// the loop function runs over and over again until power down or reset
void loop() {
	if (!interruptAttached) {
		Serial.println("attachInterrupt");
		attachInterrupt(digitalPinToInterrupt(pinServo_CH4), pinServo_X_CHANGE, CHANGE);
		interruptAttached = true;
	}

	if (last_servoXLen != servoXLen) {
		Serial.printf("Servo servoXLen = %i\n", servoXLen);
		last_servoXLen = servoXLen;
		if (last_servoXLen < 1000UL) {
			pos = 0;
		}
		else if (last_servoXLen > 2000) {
			pos = 180;
		}
		else
		{
			pos = map(last_servoXLen, 1000, 2000, 0, 180);
		}
	};

	if (lastPos != pos) {
		Serial.printf("Servo X = %i\n", pos);
		lastPos = pos;
	}
	delay(1000);
}

