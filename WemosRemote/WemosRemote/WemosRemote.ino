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
#include <FS.h>
#include <Servo.h>

#include "RoboconMotor.h"
#include "Json.h"
#include "SerialController.h"
#include "Blinker.h"
#include <RemoteXY.h> 



// конфигурация интерфейса   
// RemoteXY configurate   
#pragma pack(push, 1) 
uint8_t RemoteXY_CONF[] = {
	255,6,0,0,0,56,0,8,25,0,
  5,43,2,10,43,43,2,26,31,1,
  0,57,4,18,18,132,16,72,76,0,
  2,0,57,39,18,9,2,26,31,31,
  69,109,0,79,70,70,0,3,130,55,
  50,22,12,2,26,4,0,82,2,17,
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

struct {
	int center;
	int max_left;
	int max_right;
	int position;
	bool changed;
} stearingParams;

struct {
	int min_speed;
} motorParams;


///////////////////////////////////////////// 
//           END RemoteXY include          // 
///////////////////////////////////////////// 

#define REMOTEXY_SERVER_PORT 6377 
char SSID[20];
char SSID_password[20];
bool connected = false;

RoboEffects motorEffect = RoboEffects();
RoboMotor motor = RoboMotor("motor", D7, D8, &motorEffect);

//RoboEffects stearingEffect = RoboEffects();
//RoboMotor stearing = RoboMotor("stearing", D5, D6, &stearingEffect);
Servo stearingServo = Servo();

SerialController serialController = SerialController();

Blinker leftLight = Blinker("Left light");
Blinker rightLight = Blinker("Right light");
Blinker siren1 = Blinker("Siren");
Blinker buildinLed = Blinker("Build in led");

int FrontLightPin = D4;

bool turnOffTurnLights = false;
int turnLightsCondition = 10;

void handleTurnLight(int stearing) {
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
			Serial.println("Лівий поворот вимкнено.");
		}
		else if (!leftLight.isRunning() && rightLight.isRunning()) {//блимає правий поворот
			rightLight.end();
			turnOffTurnLights = false;
			Serial.println("Правий поворот вимкнено.");
		}
	}
}

void setup()
{
	Serial.begin(115200);
	Serial.println();
	Serial.println();
	analogWriteRange(255);
	String s;
	if (!SPIFFS.begin()) {
		Serial.println(F("No file system!"));
		Serial.println(F("Fomating..."));
		if (SPIFFS.format())
			Serial.println(F("OK"));
		else {
			Serial.println(F("Fail.... rebooting..."));
			while (true);
		}
	}

	if (SPIFFS.exists("/intro.txt")) {
		File f = SPIFFS.open("/intro.txt", "r");
		s = f.readString();
		Serial.println(s.c_str());
	}
	else {
		Serial.println(("Starting..."));
	}

	JsonString cfg = "";
	File cfgFile;
	if (!SPIFFS.exists("/config.json")) {
		Serial.println(("Default setting loaded..."));
		cfg.beginObject();
		cfg.AddValue("ssid", "WEMOS");
		cfg.AddValue("password", "12345678");
		cfg.AddValue("mode", "debug");
		cfg.AddValue("center", "100");
		cfg.AddValue("max_left", "20");
		cfg.AddValue("max_right", "180");
		cfg.AddValue("min_speed", "50");
		cfg.endObject();

		cfgFile = SPIFFS.open("/config.json", "w");
		cfgFile.print(cfg.c_str());
		cfgFile.flush();
		cfgFile.close();
	}
	else {
		Serial.println(("Reading config..."));
		cfgFile = SPIFFS.open("/config.json", "r");
		s = cfgFile.readString();
		cfg = JsonString(s.c_str());
		Serial.print(cfg.c_str());
		Serial.println();
	}

	s = cfg.getValue("ssid") + "_" + WiFi.macAddress();
	s.replace(":", "");
	strcpy(&SSID[0], s.c_str());
	Serial.println(SSID);

	s = cfg.getValue("password");
	strcpy(&SSID_password[0], s.c_str());
	Serial.println(SSID_password);

	//Servo config reading
	s = cfg.getValue("center");
	stearingParams.center = s.toInt();

	s = cfg.getValue("max_left");
	stearingParams.max_left = s.toInt();

	s = cfg.getValue("max_right");
	stearingParams.max_right = s.toInt();
	stearingParams.position = stearingParams.center;
	stearingParams.changed = true;


	stearingServo.attach(D5);
	stearingServo.write(stearingParams.position);

	serialController.stearing = &stearingServo;

	//motor config reading
	s = cfg.getValue("min_speed");
	motorParams.min_speed = s.toInt();
	motor.responder = &Serial;
	motor.setWeight(800);
	motor.reset();

	serialController.motor = &motor;

	//Налаштування поворотників
	leftLight
		.Add(D2, 0, 255)
		->Add(D2, 500, 0)
		->Add(D2, 1000, 0);
	serialController.leftLight = &leftLight;
	rightLight
		.Add(D1, 0, 255)
		->Add(D1, 500, 0)
		->Add(D1, 1000, 0);
	serialController.rightLight = &rightLight;
	//Налаштування сирени
	siren1
		.Add(D3, 0, 0)

		->Add(D0, 0, 255)
		->Add(D0, 100, 0)
		->Add(D0, 200, 255)
		->Add(D0, 300, 0)
		->Add(D0, 400, 255)
		->Add(D0, 600, 0)

		->Add(D3, 700, 255)
		->Add(D3, 800, 0)
		->Add(D3, 900, 255)
		->Add(D3, 1000, 0)
		->Add(D3, 1100, 255)
		->Add(D3, 1200, 0);
	serialController.siren1 = &siren1;
	//Налаштування фар
	pinMode(FrontLightPin, OUTPUT);
	digitalWrite(FrontLightPin, LOW);
	//Блимак встроїного світлодіода
	buildinLed.Add(BUILTIN_LED, 0, 0)
		->Add(BUILTIN_LED, 500, 255)
		->Add(BUILTIN_LED, 1000, 0);
	buildinLed.begin();
	//leftLight.begin();
	//rightLight.begin();

	remotexy = new CRemoteXY(RemoteXY_CONF_PROGMEM, &RemoteXY, REMOTEXY_ACCESS_PASSWORD, SSID, SSID_password, REMOTEXY_SERVER_PORT);//RemoteXY_Init();
	RemoteXY.drive_mode = 1;
	// TODO you setup code 
	Serial.println("Start");
}

int mapSpeed(int speed) {
	int corectedSpeed = (speed * speed) / 100;

	if (speed > 0)
		return map(corectedSpeed, 0, 100, motorParams.min_speed, 255);
	if (speed < 0)
		return -map(corectedSpeed, 0, 100, motorParams.min_speed, 255);
	return 0;
}

int mapStearing(int direction) {
	int corectedDirection = (direction * direction) / 100;

	if (direction >= -5 && direction <= 5) return stearingParams.center;
	if (direction > 5)//в право
		return map(corectedDirection, 0, 100, stearingParams.center, stearingParams.max_right);
	if (direction < 5)
		return  map(corectedDirection, 0, 100, stearingParams.center, stearingParams.max_left);
}


void loop()
{
	RemoteXY_Handler();
	if (!serialController.isRunning) {
		// используйте структуру RemoteXY для передачи данных 
		if (RemoteXY.connect_flag) {
			if (!connected) {
				Serial.println("Connected!");
				digitalWrite(FrontLightPin, HIGH);
				buildinLed.end();
				leftLight.end();
				rightLight.end();
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
				pos = mapStearing(RemoteXY.left_joy_x);
				stearingParams.changed = (stearingParams.position != pos);
				stearingParams.position = pos;
				handleTurnLight(RemoteXY.left_joy_x);
				break;

			default: //Все керування лівим джойстиком
				speed = mapSpeed(RemoteXY.left_joy_y);
				motor.setSpeed(speed);
				pos = mapStearing(RemoteXY.left_joy_x);
				stearingParams.changed = (stearingParams.position != pos);
				stearingParams.position = pos;
				handleTurnLight(RemoteXY.left_joy_x);
				break;

			}
			/*if (RemoteXY.Siren == 1) {
				if (!siren1.isRunning()) siren1.begin();
			}
			else {
				if (siren1.isRunning()) siren1.end();
			}*/
		}
		else {
			if (connected) {
				Serial.println("Disconnected!");
				digitalWrite(FrontLightPin, LOW);
				connected = false;
				buildinLed.begin();
				//leftLight.begin();
				//rightLight.begin();
			}
			motor.reset();
			stearingParams.position = stearingParams.center;
		}
	}
	serialController.loop();
	motor.loop();
	stearingServo.write(stearingParams.position);
	if (stearingParams.changed) {
		Serial.printf("Stearing -> %i\n", stearingParams.position);
		stearingParams.changed = false;
	}
	leftLight.loop();
	rightLight.loop();
	siren1.loop();
	buildinLed.loop();
}