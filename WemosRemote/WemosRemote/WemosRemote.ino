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
#include "Json.h"
#include "SerialController.h"
#include "Blinker.h"
#include <RemoteXY.h> 
#include "WebUIController.h"

#define pinServo D5
#define pinMotorA D7
#define pinMotorB D8
#define pinFrontLight D4
#define pinLeftLight D2
#define pinRightLight D1
#define pinBackLight D3
#define pinStopLight D6
#define pinParkingLight TX
#define pinBuzzer RX


// конфигурация интерфейса   
// RemoteXY configurate   
#pragma pack(push, 1) 
uint8_t RemoteXY_CONF[] = {
  255,6,0,0,0,56,0,8,25,0,
  5,43,-9,9,43,43,2,26,31,1,
  0,57,4,18,18,132,16,72,76,0,
  2,0,57,39,18,9,2,26,31,31,
  69,109,0,79,70,70,0,3,130,55,
  50,22,12,2,26,4,0,88,2,17,
  60,2,26
};

// this structure defines all the variables of your control interface  
struct {

	// input variable
	int8_t left_joy_x; // =-100..100 координата x положения джойстика 
	int8_t left_joy_y; // =-100..100 координата y положения джойстика 
	uint8_t Light_btn; // =1 если кнопка нажата, иначе =0 
	uint8_t emergency_btn; // =1 если переключатель включен и =0 если отключен 
	uint8_t drive_mode; // =0 если переключатель в положении A, =1 если в положении B, =2 если в положении C, ... 
	int8_t speed; // =0..100 положение слайдера 

	  // other variable
	uint8_t connect_flag;  // =1 if wire connected, else =0 

} RemoteXY;
#pragma pack(pop) 

enum LightMode {
	OFF = 0,
	Parking,
	ON
};

struct {
	int center;//градусів
	int max_left;//градусів
	int max_right;//градусів
	int position;//градусів
	int min_speed;//від 0 до 256
	int front_light_on;//в процентах
	int parking_light_on;//в процентах
	int stop_light_duration;//в мілісекундах

	int LightMode;

	bool stopped;
	bool emergency;
	bool light_btn;

	bool changed;
	bool debug;
} config;


///////////////////////////////////////////// 
//           END RemoteXY include          // 
///////////////////////////////////////////// 

#define REMOTEXY_SERVER_PORT 6377 
char SSID[20];
char SSID_password[20];
bool connected = false;


RoboEffects motorEffect = RoboEffects();
RoboMotor motor = RoboMotor("motor", pinMotorA, pinMotorB, &motorEffect);
Servo stearingServo = Servo();

SerialController serialController = SerialController();

Blinker leftLight = Blinker("Left light");
Blinker rightLight = Blinker("Right light");
Blinker buildinLed = Blinker("Build in led");
Blinker stopLight = Blinker("Stop light");
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
void handleLight() {
	if (handledLightMode != config.LightMode) {
		int val;
		switch (config.LightMode)
		{
		case LightMode::OFF:
			digitalWrite(pinFrontLight, LOW);
			if (!config.debug)	digitalWrite(pinParkingLight, LOW);
			break;
		case LightMode::Parking:
			val = map(config.parking_light_on, 0, 100, 0, 255);
			analogWrite(pinFrontLight, val);
			if (!config.debug) analogWrite(pinParkingLight, val);
			break;
		case LightMode::ON:
			val = map(config.front_light_on, 0, 100, 0, 255);
			analogWrite(pinFrontLight, val);
			val = map(config.parking_light_on, 0, 100, 0, 255);
			if (!config.debug) analogWrite(pinParkingLight, val);
			break;
		default:
			break;
		}
	}
}

void loadConfog() {
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
		cfg.AddValue("min_speed", "10");
		cfg.AddValue("front_light_on", "100");
		cfg.AddValue("parking_light_on", "50");
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
		//console.print(cfg.c_str());
		//console.println();
	}

	s = cfg.getValue("ssid") + "_" + WiFi.macAddress();
	s.replace(":", "");
	strcpy(&SSID[0], s.c_str());
	//Serial.println(SSID);

	s = cfg.getValue("password");
	strcpy(&SSID_password[0], s.c_str());
	//Serial.println(SSID_password);

	s = cfg.getValue("mode");
	if (s == "debug")
		config.debug = true;
	else
		config.debug = false;

	//Servo config reading
	config.center = cfg.getInt("center");
	config.max_left = cfg.getInt("max_left");
	config.max_right = cfg.getInt("max_right");
	config.position = config.center;

	//motor config reading
	config.min_speed = cfg.getInt("min_speed");

	config.front_light_on = cfg.getInt("front_light_on");
	config.parking_light_on = cfg.getInt("parking_light_on");
	config.stop_light_duration = cfg.getInt("stop_light_duration");

	config.changed = true;

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

	if (!config.debug) {
		pinMode(pinParkingLight, OUTPUT);
		pinMode(pinBuzzer, OUTPUT);
		digitalWrite(pinParkingLight, LOW);
		digitalWrite(pinBuzzer, LOW);
	}


	//Налаштування поворотників
	leftLight
		.Add(pinLeftLight, 0, 255)
		->Add(pinLeftLight, 500, 0)
		->Add(pinLeftLight, 1000, 0);
	serialController.leftLight = &leftLight;
	rightLight
		.Add(pinRightLight, 0, 255)
		->Add(pinRightLight, 500, 0)
		->Add(pinRightLight, 1000, 0);
	serialController.rightLight = &rightLight;

	if (config.debug) {
		alarmOff
			.Add(pinLeftLight, 0, 255)
			->Add(pinRightLight, 0, 255)
			->Add(pinLeftLight, 200, 0)
			->Add(pinRightLight, 200, 0)
			->Add(pinLeftLight, 400, 255)
			->Add(pinRightLight, 400, 255)
			->Add(pinLeftLight, 600, 0)
			->Add(pinRightLight, 600, 0);
	}
	else {
		alarmOff
			.Add(pinBuzzer, 0, 255)
			->Add(pinBuzzer, 200, 0)
			->Add(pinBuzzer, 400, 255)
			->Add(pinBuzzer, 600, 0);
	}
	alarmOff.repeat = false;
	alarmOff.debug = true;

	if (config.debug) {
		alarmOn
			.Add(pinLeftLight, 0, 255)
			->Add(pinRightLight, 0, 255)
			->Add(pinLeftLight, 200, 0)
			->Add(pinRightLight, 200, 0);
	}
	else {
		alarmOn
			.Add(pinBuzzer, 0, 255)
			->Add(pinBuzzer, 200, 0);
	}
	alarmOn.repeat = false;
	alarmOn.debug = true;

	stopLight.Add(pinStopLight, 0, 0)
		->Add(pinStopLight, 2, 255)
		->Add(pinStopLight, config.stop_light_duration, 0)
		->repeat = false;
	stopLight.debug = true;
	//Блимак встроїного світлодіода
	buildinLed.Add(BUILTIN_LED, 0, 0)
		->Add(BUILTIN_LED, 500, 255)
		->Add(BUILTIN_LED, 1000, 0);

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


	loadConfog();

	if (!config.debug) {
		Serial.end();
		console.output = nullptr;
	}

	setupBlinkers();

	stearingServo.attach(pinServo);
	stearingServo.write(config.position);

	motor.responder = &console;
	motor.setWeight(800);
	motor.reset();

	serialController.stearing = &stearingServo;
	serialController.motor = &motor;

	buildinLed.begin();

	remotexy = new CRemoteXY(RemoteXY_CONF_PROGMEM, &RemoteXY, REMOTEXY_ACCESS_PASSWORD, SSID, SSID_password, REMOTEXY_SERVER_PORT);//RemoteXY_Init();
	RemoteXY.drive_mode = 1;

	console.println("Start");
	webServer.setup();
	webServer.apName = String(SSID);
}


int mapSpeed(int speed) {
	int corectedSpeed = (speed * speed) / 100;

	if (speed > 0)
		return map(corectedSpeed, 0, 100, config.min_speed, 255);
	if (speed < 0)
		return -map(corectedSpeed, 0, 100, config.min_speed, 255);
	return 0;
}

int mapStearing(int direction) {
	int corectedDirection = (direction * direction) / 100;

	if (direction >= -5 && direction <= 5) return config.center;
	if (direction > 5)//в право
		return map(corectedDirection, 0, 100, config.center, config.max_right);
	if (direction < 5)
		return  map(corectedDirection, 0, 100, config.center, config.max_left);
}


void loop()
{
	RemoteXY_Handler();
	if (RemoteXY.connect_flag) {
		if (!connected) {
			console.println("Connected!");
			buildinLed.end();
			leftLight.end();
			rightLight.end();
			alarmOff.begin();
			motor.reset();
			config.changed = (config.position != config.center);
			config.position = config.center;
			connected = true;
		}
		int pos;
		int speed;
		switch (RemoteXY.drive_mode)
		{
		case 1: //лівий джойстик повороти, Повзунок - швидкість
			speed = mapSpeed(RemoteXY.speed);
			if (RemoteXY.left_joy_y < -20) speed = -speed;
			motor.setSpeed(speed);
			if (RemoteXY.speed < 10) {
				if (!config.stopped) {
					stopLight.begin();
					config.stopped = true;
				}
			}
			else {
				config.stopped = false;
			}
			pos = mapStearing(RemoteXY.left_joy_x);
			config.changed = (config.position != pos);
			config.position = pos;
			handleTurnLight(RemoteXY.left_joy_x);
			break;

		default: //Все керування лівим джойстиком
			speed = mapSpeed(RemoteXY.left_joy_y);
			motor.setSpeed(speed);
			if (RemoteXY.left_joy_y > -5 && RemoteXY.left_joy_y < 5) {
				if (!config.stopped) {
					stopLight.begin();
					config.stopped = true;
				}
			}
			else {
				config.stopped = false;
			}
			pos = mapStearing(RemoteXY.left_joy_x);
			config.changed = (config.position != pos);
			config.position = pos;
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
	}
	else {
		if (connected) {
			console.println("Disconnected!");
			connected = false;
			buildinLed.begin();
			alarmOn.begin();
			motor.reset();
			config.changed = (config.position != config.center);
			config.position = config.center;
			config.LightMode = 0;
		}
	}
	if (config.debug) {
		serialController.loop();
	}
	motor.loop();
	stearingServo.write(config.position);
	if (config.changed) {
		console.printf("Stearing -> %i\n", config.position);
		config.changed = false;
	}
	leftLight.loop();
	rightLight.loop();
	alarmOff.loop();
	alarmOn.loop();
	buildinLed.loop();
	stopLight.loop();
	webServer.loop();
}