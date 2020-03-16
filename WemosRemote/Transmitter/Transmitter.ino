/*
 Name:		Transmitter.ino
 Created:	3/15/2020 9:04:54 PM
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

struct State {

	int x;  //0..180
	int y;  //0..180
	int ch3;//0..180
	int ch4;//0..180

} state;

State * cfg = &state;

char SSID[32];
char SSID_password[20];




Servo servo_X;
Servo servo_Y;
Servo servo_CH3;
Servo servo_CH4;

void Setup_Get();
void Setup_Post();

// the setup function runs once when you press reset or power the board
void setup() {
	Serial.begin(115200);
	pinMode(pinServo_X, OUTPUT);
	pinMode(pinServo_Y, OUTPUT);
	pinMode(pinServo_CH3, OUTPUT);
	pinMode(pinServo_CH4, OUTPUT);
	
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

	s = "TR_" + WiFi.macAddress();
	s.replace(":", "");
	strcpy(&SSID[0], s.c_str());

	s = "12345678";
	strcpy(&SSID_password[0], s.c_str());

	WiFi.softAP(SSID, SSID_password);

	webServer.setup();
	webServer.apName = String(SSID);

	servo_X.attach(pinServo_X);
	servo_Y.attach(pinServo_Y);
	servo_CH3.attach(pinServo_CH3);
	servo_CH4.attach(pinServo_CH4);

}


// the loop function runs over and over again until power down or reset
void loop() {

	servo_X.write(cfg->x);
	servo_Y.write(cfg->y);
	servo_CH3.write(cfg->ch3);
	servo_CH4.write(cfg->ch4);
	
	

	webServer.loop();
}

void printConfig(JsonString * out)
{
	out->beginObject();
	out->AddValue("x", String(cfg->x));
	out->AddValue("y", String(cfg->y));
	out->AddValue("ch3", String(cfg->ch3));
	out->AddValue("ch4", String(cfg->ch4));
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
	if (webServer.hasArg("x")) { cfg->x = webServer.arg("x").toInt(); }
	if (webServer.hasArg("y")) { cfg->y = webServer.arg("y").toInt(); }
	if (webServer.hasArg("ch3")) { cfg->ch3 = webServer.arg("ch3").toInt(); }
	if (webServer.hasArg("ch4")) { cfg->ch4 = webServer.arg("ch4").toInt(); }

	webServer.Ok();
}



