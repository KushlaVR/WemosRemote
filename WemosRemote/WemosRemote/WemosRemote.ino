/*
 Name:		WemosRemote.ino
 Created:	10/23/2019 7:53:20 PM
 Author:	Віталік
*/



////////////////////////////////////////////// 
//        RemoteXY include library          // 
////////////////////////////////////////////// 

// определение режима соединения и подключение библиотеки RemoteXY  
#define REMOTEXY_MODE__ESP8266WIFI_LIB_POINT
#include <ESP8266WiFi.h> 
#include <ESP8266WebServer.h> 
#include <DNSServer.h>
#include <ESP8266mDNS.h>

#include <FS.h>
#include <Servo.h>

#include "Console.h"
#include "RoboconMotor.h"
#include "Stearing.h"
#include "Json.h"
#include "SerialController.h"
#include "Blinker.h"
#include <RemoteXY.h> 
#include "WebUIController.h"
#include "SetupController.h"

#define pinServo D5
#define pinMotorA D7
#define pinMotorB D6
#define pinFrontLight D4
#define pinLeftLight D2
#define pinRightLight D1
#define pinBackLight D3
#define pinStopLight D8
#define pinParkingLight D0
//#define pinBuzzer RX


// конфигурация интерфейса   
// RemoteXY configurate   
#pragma pack(push, 1) 
uint8_t RemoteXY_CONF[] = {
  255,7,0,0,0,57,0,8,25,0,
  5,43,-7,17,34,34,2,26,31,1,
  0,31,0,13,13,132,16,72,76,0,
  2,0,49,2,17,9,2,26,31,31,
  69,109,0,79,70,70,0,3,130,39,
  49,22,12,2,26,5,51,65,16,35,
  35,2,26,31
};

// this structure defines all the variables of your control interface  
struct {

	// input variable
	int8_t left_joy_x; // =-100..100 координата x положения джойстика 
	int8_t left_joy_y; // =-100..100 координата y положения джойстика 
	uint8_t Light_btn; // =1 если кнопка нажата, иначе =0 
	uint8_t emergency_btn; // =1 если переключатель включен и =0 если отключен 
	uint8_t drive_mode; // =0 если переключатель в положении A, =1 если в положении B, =2 если в положении C, ... 
	int8_t right_joy_x; // =-100..100 координата x положения джойстика 
	int8_t right_joy_y; // =-100..100 координата y положения джойстика 

	  // other variable
	uint8_t connect_flag;  // =1 if wire connected, else =0 

} RemoteXY;
#pragma pack(pop) 

enum LightMode {
	OFF = 0,
	Parking,
	ON
};

ConfigStruct config;

///////////////////////////////////////////// 
//           END RemoteXY include          // 
///////////////////////////////////////////// 

#define REMOTEXY_SERVER_PORT 6377 
char SSID[20];
char SSID_password[20];
bool connected = false;


RoboEffects motorEffect = RoboEffects();
RoboMotor motor = RoboMotor("motor", pinMotorA, pinMotorB, &motorEffect);
Stearing stearingServo = Stearing(pinServo);

SerialController serialController = SerialController();

Blinker leftLight = Blinker("Left light");
Blinker rightLight = Blinker("Right light");
Blinker stopLight = Blinker("Stop light");
Blinker backLight = Blinker("Back light");
Blinker alarmOn = Blinker("Alarm on");
Blinker alarmOff = Blinker("Alarm of");


bool turnOffTurnLights = false;
int turnLightsCondition = 10;

void handleTurnLight(int stearing) {
	if (config.emergency) {//Якщо включена аварійка - нічого не робимо
		turnOffTurnLights = false;
		return;
	}
	if (stearing < -turnLightsCondition) { //Включений лівий поворот
		if (!leftLight.isRunning()) leftLight.begin();
		rightLight.end();
	}
	if (stearing > turnLightsCondition) { //Включений правий поворот
		if (!rightLight.isRunning()) rightLight.begin();
		leftLight.end();
	}
	if (leftLight.isRunning()) {
		if (stearing < -turnLightsCondition) turnOffTurnLights = true;//ставимо флажок, щоб вимкнути поворот після того як руль вернеться в прямк положенн
	}
	else if (rightLight.isRunning()) {
		if (stearing > turnLightsCondition) turnOffTurnLights = true;//ставимо флажок, щоб вимкнути поворот після того як руль вернеться в прямк положенн
	}
	if (turnOffTurnLights && stearing > -turnLightsCondition && stearing < turnLightsCondition) {//Повернули руль в стартове положення
		if (leftLight.isRunning() && !rightLight.isRunning()) {//блимає лівий поворот
			leftLight.end();
			turnOffTurnLights = false;
			console.println("Лівий поворот вимкнено.");
		}
		else if (!leftLight.isRunning() && rightLight.isRunning()) {//блимає правий поворот
			rightLight.end();
			turnOffTurnLights = false;
			console.println("Правий поворот вимкнено.");
		}
	}
}

int handledLightMode = 0;
int handledBackLightMode = 0;
void handleLight() {
	if (!RemoteXY.connect_flag) {
		config.LightMode = LightMode::OFF;
	};
	if (handledLightMode != config.LightMode) {
		handledLightMode = config.LightMode;
		int val;
		switch (config.LightMode)
		{
		case LightMode::OFF:
			digitalWrite(pinFrontLight, LOW);
			digitalWrite(pinParkingLight, LOW);
			break;
		case LightMode::Parking:
			val = map(config.parking_light_on, 0, 100, 0, 255);
			analogWrite(pinFrontLight, val);
			analogWrite(pinParkingLight, val);
			break;
		case LightMode::ON:
			val = map(config.front_light_on, 0, 100, 0, 255);
			analogWrite(pinFrontLight, val);
			val = map(config.parking_light_on, 0, 100, 0, 255);
			analogWrite(pinParkingLight, val);
			break;
		default:
			break;
		}
	}
	if (config.backLightMode != handledBackLightMode) {
		handledBackLightMode = config.backLightMode;
		switch (config.backLightMode)
		{
		case LightMode::OFF:
			backLight.end();
			break;
		case LightMode::ON:
			backLight.begin();
			break;
		default:
			break;
		}
	}
}


void setupBlinkers() {
	//Налаштування фар
	pinMode(pinFrontLight, OUTPUT);
	pinMode(pinLeftLight, OUTPUT);
	pinMode(pinRightLight, OUTPUT);
	pinMode(pinBackLight, OUTPUT);
	pinMode(pinStopLight, OUTPUT);

	digitalWrite(pinFrontLight, LOW);
	digitalWrite(pinLeftLight, LOW);
	digitalWrite(pinRightLight, LOW);
	digitalWrite(pinBackLight, LOW);
	digitalWrite(pinStopLight, LOW);

	pinMode(pinParkingLight, OUTPUT);
	digitalWrite(pinParkingLight, LOW);


	//Налаштування поворотників
	leftLight
		.Add(pinLeftLight, 0, config.parking_light_on)
		->Add(pinLeftLight, 500, 0)
		->Add(pinLeftLight, 1000, 0);
	serialController.leftLight = &leftLight;
	rightLight
		.Add(pinRightLight, 0, config.parking_light_on)
		->Add(pinRightLight, 500, 0)
		->Add(pinRightLight, 1000, 0);
	serialController.rightLight = &rightLight;

	alarmOff
		.Add(pinLeftLight, 0, config.parking_light_on)
		->Add(pinRightLight, 0, config.parking_light_on)
		->Add(pinLeftLight, 200, 0)
		->Add(pinRightLight, 200, 0)
		->Add(pinLeftLight, 400, config.parking_light_on)
		->Add(pinRightLight, 400, config.parking_light_on)
		->Add(pinLeftLight, 600, 0)
		->Add(pinRightLight, 600, 0);
	alarmOff.repeat = false;
	//alarmOff.debug = true;

	alarmOn
		.Add(pinLeftLight, 0, config.parking_light_on)
		->Add(pinRightLight, 0, config.parking_light_on)
		->Add(pinLeftLight, 200, 0)
		->Add(pinRightLight, 200, 0);
	alarmOn.repeat = false;
	//alarmOn.debug = true;

	stopLight.Add(pinStopLight, 0, 0)
		->Add(pinStopLight, 2, 255)
		->Add(pinStopLight, config.stop_light_duration, 0)
		->repeat = false;
	stopLight.debug = true;

	backLight.Add(pinBackLight, 0, 0)
		->Add(pinBackLight, 16, 255)
		->Add(pinBackLight, 20, 0)
		->repeat = true;


}


void loadConfig()
{
	String s;
	JsonString cfg = "";
	File cfgFile;
	if (!SPIFFS.exists("/config.json")) {
		console.println(("Default setting loaded..."));
		cfg.beginObject();
		cfg.AddValue("ssid", "WEMOS");
		cfg.AddValue("password", "12345678");
		cfg.AddValue("mode", "debug");
		cfg.AddValue("center", "90");
		cfg.AddValue("max_left", "150");
		cfg.AddValue("max_right", "60");
		cfg.AddValue("min_speed", "50");
		cfg.AddValue("front_light_on", "80");
		cfg.AddValue("parking_light_on", "10");
		cfg.AddValue("stop_light_duration", "2000");
		cfg.endObject();

		cfgFile = SPIFFS.open("/config.json", "w");
		cfgFile.print(cfg.c_str());
		cfgFile.flush();
		cfgFile.close();
	}
	else {
		console.println(("Reading config..."));
		cfgFile = SPIFFS.open("/config.json", "r");
		s = cfgFile.readString();
		cfg = JsonString(s.c_str());
		cfgFile.close();
	}

	config.ssid = String(cfg.getValue("ssid"));
	config.password = String(cfg.getValue("password"));

	s = cfg.getValue("mode");
	if (s == "debug")
		config.debug = true;
	else
		config.debug = false;

	//Servo config reading
	config.center = cfg.getInt("center");
	config.max_left = cfg.getInt("max_left");
	config.max_right = cfg.getInt("max_right");

	//motor config reading
	config.min_speed = cfg.getInt("min_speed");

	config.front_light_on = cfg.getInt("front_light_on");
	config.parking_light_on = cfg.getInt("parking_light_on");
	config.stop_light_duration = cfg.getInt("stop_light_duration");

	s = config.ssid + "_" + WiFi.macAddress();
	s.replace(":", "");
	strcpy(&SSID[0], s.c_str());

	s = config.password;
	strcpy(&SSID_password[0], s.c_str());

}


void setup()
{
	Serial.begin(115200);
	Serial.println();
	Serial.println();
	console.output = &Serial;

	analogWriteRange(255);
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


	loadConfig();
	


	if (!config.debug) {
		Serial.end();
		console.output = nullptr;
	}

	setupBlinkers();

	stearingServo.setPosition(0);

	motor.responder = &console;
	motor.setWeight(800);
	motor.reset();

	serialController.stearing = &stearingServo;
	serialController.motor = &motor;


	remotexy = new CRemoteXY(RemoteXY_CONF_PROGMEM, &RemoteXY, REMOTEXY_ACCESS_PASSWORD, SSID, SSID_password, REMOTEXY_SERVER_PORT);//RemoteXY_Init();
	RemoteXY.drive_mode = 1;

	console.println("Start");
	webServer.setup();
	webServer.apName = String(SSID);
	setupController.cfg = &config;
}


int mapSpeed(int speed) {
	int corectedSpeed = (speed * speed) / 100;

	if (speed > 0)
		return map(corectedSpeed, 0, 100, config.min_speed, 255);
	if (speed < 0)
		return -map(corectedSpeed, 0, 100, config.min_speed, 255);
	return 0;
}

void loop()
{
	RemoteXY_Handler();

	stearingServo.max_left = config.max_left;
	stearingServo.max_right = config.max_right;
	stearingServo.center = config.center;

	if (RemoteXY.connect_flag) {
		if (!connected) {
			console.println("Connected!");
			leftLight.end();
			rightLight.end();
			alarmOff.begin();
			motor.reset();
			stearingServo.setPosition(0);
			connected = true;
		}
		int pos;
		int speed;
		switch (RemoteXY.drive_mode)
		{
		case 1: //лівий джойстик повороти, Повзунок - швидкість
			speed = mapSpeed(RemoteXY.right_joy_y);
			motor.setSpeed(speed);
			if (speed < 0)
				config.backLightMode = LightMode::ON;
			else
				config.backLightMode = LightMode::OFF;

			if (RemoteXY.right_joy_y > -10 && RemoteXY.right_joy_y < 10) {
				if (!config.stopped) {
					stopLight.begin();
					config.stopped = true;
				}
			}
			else {
				stopLight.end();
				config.stopped = false;
			}
			stearingServo.setPosition(RemoteXY.left_joy_x);
			handleTurnLight(RemoteXY.left_joy_x);
			break;

		default: //Все керування лівим джойстиком
			speed = mapSpeed(RemoteXY.left_joy_y);
			motor.setSpeed(speed);
			if (speed < 0)
				config.backLightMode = LightMode::ON;
			else
				config.backLightMode = LightMode::OFF;

			if (RemoteXY.left_joy_y > -5 && RemoteXY.left_joy_y < 5) {
				if (!config.stopped) {
					stopLight.begin();
					config.stopped = true;
				}
			}
			else {
				stopLight.end();
				config.stopped = false;
			}
			stearingServo.setPosition(RemoteXY.left_joy_x);
			handleTurnLight(RemoteXY.left_joy_x);
			break;

		}
		if (RemoteXY.emergency_btn) {
			if (!config.emergency) {
				leftLight.begin();
				rightLight.begin();
				config.emergency = true;
			}
		}
		else {
			if (config.emergency) {
				leftLight.end();
				rightLight.end();
				config.emergency = false;
			}
		}
		if (RemoteXY.Light_btn != config.light_btn) {
			if (RemoteXY.Light_btn) {
				config.LightMode++;
				if (config.LightMode > LightMode::ON) config.LightMode = 0;
			}
			config.light_btn = RemoteXY.Light_btn;
		}
		handleLight();
	}
	else {
		if (connected) {
			console.println("Disconnected!");
			connected = false;
			alarmOn.begin();
			motor.reset();
			stearingServo.setPosition(0);
			config.LightMode = 0;
			handleLight();
		}
	}
	if (config.debug) {
		serialController.loop();
	}
	motor.loop();
	stearingServo.loop();
	leftLight.loop();
	rightLight.loop();
	alarmOff.loop();
	alarmOn.loop();
	stopLight.loop();
	backLight.loop();
	webServer.loop();
}