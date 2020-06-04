﻿/*
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
#include "Button.h"
#include "SerialController.h"
#include "Blinker.h"
#include <RemoteXY.h> 
#include "WebUIController.h"
#include "SetupController.h"

#define pinServo D5
#define pinMotorA D7//назад
#define pinHighLight D6
#define pinFrontLight D4
#define pinLeftLight D2
#define pinRightLight D1
#define pinBackLight D3
#define pinStopLight D8
#define pinParkingLight D0
#define pinBuzzer RX



// конфигурация интерфейса   
// RemoteXY configurate   
#pragma pack(push, 1) 
uint8_t RemoteXY_CONF[] = {
  255,7,0,0,0,51,0,10,25,0,
  1,0,30,42,17,17,36,31,65,0,
  1,0,51,1,19,19,177,31,72,0,
  1,0,52,42,20,20,132,16,76,0,
  5,1,0,12,42,42,2,26,31,5,
  32,58,6,49,49,176,26,31
};



// this structure defines all the variables of your control interface  
struct {

	// input variable
	uint8_t Alarm; // =1 if button pressed, else =0 
	uint8_t Light_high; // =1 if button pressed, else =0 
	uint8_t Light_low; // =1 if button pressed, else =0 
	int8_t left_joy_x; // =-100..100 x-coordinate joystick position 
	int8_t left_joy_y; // =-100..100 y-coordinate joystick position 
	int8_t right_joy_x; // =-100..100 x-coordinate joystick position 
	int8_t right_joy_y; // =-100..100 y-coordinate joystick position 

	  // other variable
	uint8_t connect_flag;  // =1 if wire connected, else =0 

} RemoteXY;
#pragma pack(pop) 

enum LightMode {
	OFF = 0,
	Parking = 1,
	ON = 2,
	HIGH_LIGHT = 3,
	WAIT_FOR_TIMEOUT = 4
};

struct StateStructure {

	int LightMode;
	unsigned long stoppedTime;
	int backLightMode;

	int speed;
	bool stopped;
	bool emergency;
	bool emergency_btn_pressed;

	bool serialEnabled;

} state;


ConfigStruct config;

///////////////////////////////////////////// 
//           END RemoteXY include          // 
///////////////////////////////////////////// 

#define REMOTEXY_SERVER_PORT 6377 
char SSID[32];
char SSID_password[20];
bool connected = false;


RoboEffects motorEffect = RoboEffects();
MotorBase * motor = nullptr;//= RoboMotor("motor", pinMotorA, pinMotorB, &motorEffect);
Stearing stearingServo = Stearing(pinServo);

SerialController serialController = SerialController();

Blinker leftLight = Blinker("Left light");
Blinker rightLight = Blinker("Right light");
Blinker stopLight = Blinker("Stop light");
Blinker backLight = Blinker("Back light");
Blinker alarmOn = Blinker("Alarm on");
Blinker alarmOff = Blinker("Alarm of");
Beeper alarmBeepOn = Beeper("Alarm beep on");
Beeper alarmBeepOff = Beeper("Alarm beep of");
Beeper turnLightBeeper = Beeper("Turn light beep");

VirtualButton btn_HeadLight = VirtualButton(btn_HeadLight_Pressed, btn_HeadLight_Released, btn_HeadLight_Hold);
VirtualButton btn_HighLight = VirtualButton(btn_HighLight_Pressed, btn_HighLight_Released);

void btn_HeadLight_Pressed() {
	state.LightMode++;
	if (state.LightMode == LightMode::WAIT_FOR_TIMEOUT) state.LightMode = LightMode::OFF;
}
void btn_HeadLight_Released() {

}
void btn_HeadLight_Hold() {
	state.LightMode = LightMode::OFF;
}

void btn_HighLight_Pressed() {
	tone(pinBuzzer, config.beep_freq);
	analogWrite(pinHighLight, map(config.high_light_pwm, 0, 100, 0, 255));
}

void btn_HighLight_Released() {
	noTone(pinBuzzer);
	if (state.LightMode == LightMode::HIGH_LIGHT) {
		analogWrite(pinHighLight, map(config.high_light_pwm, 0, 100, 0, 255));
	}
	else {
		digitalWrite(pinHighLight, LOW);
	}
}

bool turnOffTurnLights = false;
int turnLightsCondition = 10;

void handleTurnLight(int stearing) {
	if (state.emergency) {//Якщо включена аварійка - нічого не робимо
		turnOffTurnLights = false;
		return;
	}
	if (stearing < -turnLightsCondition) { //Включений лівий поворот
		if (!leftLight.isRunning()) { leftLight.begin(); turnLightBeeper.begin(); }
		rightLight.end();
	}
	if (stearing > turnLightsCondition) { //Включений правий поворот
		if (!rightLight.isRunning()) { rightLight.begin(); turnLightBeeper.begin(); }
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
			turnLightBeeper.end();
			turnOffTurnLights = false;
			console.println("Лівий поворот вимкнено.");
		}
		else if (!leftLight.isRunning() && rightLight.isRunning()) {//блимає правий поворот
			rightLight.end();
			turnLightBeeper.end();
			turnOffTurnLights = false;
			console.println("Правий поворот вимкнено.");
		}
	}
}

int handledLightMode = 0;
int handledBackLightMode = 0;
void handleLight() {
	if (!RemoteXY.connect_flag) {
		state.LightMode = LightMode::OFF;
	};
	if (handledLightMode != state.LightMode) {
		handledLightMode = state.LightMode;

		int val;
		switch (state.LightMode)
		{
		case LightMode::OFF:
			if (btn_HighLight.isPressed())
				analogWrite(pinHighLight, map(config.high_light_pwm, 0, 100, 0, 255));
			else
				digitalWrite(pinHighLight, LOW);
			digitalWrite(pinFrontLight, LOW);
			digitalWrite(pinParkingLight, LOW);
			break;
		case LightMode::Parking:
			if (btn_HighLight.isPressed())
				analogWrite(pinHighLight, map(config.high_light_pwm, 0, 100, 0, 255));
			else
				analogWrite(pinHighLight, LOW);
			digitalWrite(pinFrontLight, LOW);
			analogWrite(pinParkingLight, map(config.parking_light_pwm, 0, 100, 0, 255));
			break;
		case LightMode::ON:
			if (btn_HighLight.isPressed())
				analogWrite(pinHighLight, map(config.high_light_pwm, 0, 100, 0, 255));
			else
				analogWrite(pinHighLight, LOW);
			analogWrite(pinFrontLight, map(config.front_light_pwm, 0, 100, 0, 255));
			analogWrite(pinParkingLight, map(config.parking_light_pwm, 0, 100, 0, 255));
			break;
		case LightMode::HIGH_LIGHT:
			analogWrite(pinHighLight, map(config.high_light_pwm, 0, 100, 0, 255));
			analogWrite(pinFrontLight, map(config.front_light_pwm, 0, 100, 0, 255));
			analogWrite(pinParkingLight, map(config.parking_light_pwm, 0, 100, 0, 255));
			break;
		default:
			break;
		}
	}
	if (state.backLightMode != handledBackLightMode) {
		switch (state.backLightMode)
		{
		case LightMode::OFF:
			handledBackLightMode = state.backLightMode;
			backLight.end();
			break;
		case LightMode::WAIT_FOR_TIMEOUT:
			if ((millis() - state.stoppedTime) > config.back_light_timeout) {
				handledBackLightMode = state.backLightMode;
				backLight.end();
			}
			break;
		case LightMode::ON:
			backLight.begin();
			handledBackLightMode = state.backLightMode;
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
		.Add(pinLeftLight, 0, config.turn_light_pwm)
		->Add(pinLeftLight, 500, 0)
		->Add(pinLeftLight, 1000, 0);
	serialController.leftLight = &leftLight;
	rightLight
		.Add(pinRightLight, 0, config.turn_light_pwm)
		->Add(pinRightLight, 500, 0)
		->Add(pinRightLight, 1000, 0);
	serialController.rightLight = &rightLight;

	alarmOff
		.Add(pinLeftLight, 0, config.turn_light_pwm)
		->Add(pinRightLight, 0, config.turn_light_pwm)
		->Add(pinLeftLight, 300, 0)
		->Add(pinRightLight, 300, 0)
		->Add(pinLeftLight, 600, config.turn_light_pwm)
		->Add(pinRightLight, 600, config.turn_light_pwm)
		->Add(pinLeftLight, 900, 0)
		->Add(pinRightLight, 900, 0);
	alarmOff.repeat = false;
	//alarmOff.debug = true;

	alarmOn
		.Add(pinLeftLight, 0, config.turn_light_pwm)
		->Add(pinRightLight, 0, config.turn_light_pwm)
		->Add(pinLeftLight, 600, 0)
		->Add(pinRightLight, 600, 0);
	alarmOn.repeat = false;
	//alarmOn.debug = true;

	stopLight.Add(pinStopLight, 0, 0)
		->Add(pinStopLight, 0, config.stop_light_pwm)
		->Add(pinStopLight, config.stop_light_duration, 0)
		->repeat = false;
	stopLight.debug = true;

	backLight.Add(pinBackLight, 0, 0)
		->Add(pinBackLight, map(100 - config.back_light_pwm, 0, 100, 0, 20), 255)
		->Add(pinBackLight, 20, 0)
		->repeat = true;

}

void setupBeepers() {
	alarmBeepOn
		.Add(pinBuzzer, 1, config.beep_freq)
		->Add(pinBuzzer, config.beep_duration, 0)
		->Add(pinBuzzer, config.beep_duration + config.beep_interval + 1, config.beep_freq)
		->Add(pinBuzzer, config.beep_duration + config.beep_interval + config.beep_duration, 0);
	alarmBeepOn.repeat = false;

	alarmBeepOff
		.Add(pinBuzzer, 1, config.beep_freq)
		->Add(pinBuzzer, config.beep_duration, 0);
	alarmBeepOff.repeat = false;

	turnLightBeeper.Add(pinBuzzer, 0, 1000)
		->Add(pinBuzzer, 1, 0)
		->Add(pinBuzzer, 500, 1000)
		->Add(pinBuzzer, 501, 0)
		->Add(pinBuzzer, 1000, 0);

}

void setupMotor() {
	if (motor != nullptr) {
		motor->isEnabled = false;
		motor->reset();
		motor->loop();
	}
	if (motor == nullptr) {
		motor = new SpeedController("Speed reg D7", pinMotorA, &motorEffect);
	}

	motor->responder = &console;
	motor->setWeight(config.inertion);
	motor->reset();
	motor->isEnabled = RemoteXY.connect_flag;
	serialController.motor = motor;

}

void refreshConfig() {

	setupMotor();

	leftLight.item(0)->value = config.turn_light_pwm;
	rightLight.item(0)->value = config.turn_light_pwm;

	alarmOn.item(0)->value = config.turn_light_pwm;
	alarmOn.item(1)->value = config.turn_light_pwm;

	alarmOff.item(0)->value = config.turn_light_pwm;
	alarmOff.item(1)->value = config.turn_light_pwm;
	alarmOff.item(4)->value = config.turn_light_pwm;
	alarmOff.item(5)->value = config.turn_light_pwm;

	stopLight.item(1)->value = config.stop_light_pwm;
	stopLight.item(2)->offset = config.stop_light_duration;

	backLight.item(0)->offset = map(100 - config.back_light_pwm, 0, 100, 0, 20);

	alarmBeepOn.item(0)->value = config.beep_freq;
	alarmBeepOn.item(2)->value = config.beep_freq;
	alarmBeepOff.item(0)->value = config.beep_freq;

	alarmBeepOn.item(1)->offset = config.beep_duration;
	alarmBeepOn.item(2)->offset = config.beep_duration + config.beep_interval + 1;
	alarmBeepOn.item(3)->offset = config.beep_duration + config.beep_interval + config.beep_duration;

	alarmBeepOff.item(1)->offset = config.beep_duration;

}


void setup()
{
	state.serialEnabled = true;
	Serial.end();
	pinMode(pinBuzzer, OUTPUT);
	digitalWrite(pinBuzzer, LOW);
	//Serial.begin(115200);
	//Serial.println();
	//Serial.println();
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

	setupController.cfg = &config;
	setupController.loadConfig();

	s = config.ssid + "_" + WiFi.macAddress();
	s.replace(":", "");
	strcpy(&SSID[0], s.c_str());

	s = config.password;
	strcpy(&SSID_password[0], s.c_str());


	//if (!config.debug) {
	//	Serial.end();
	//	console.output = nullptr;
	//}

	setupBlinkers();
	setupBeepers();

	stearingServo.max_left = config.max_left;
	stearingServo.max_right = config.max_right;
	stearingServo.center = config.center;
	stearingServo.setPosition(0, (PotentiometerLinearity)config.stearing_linearity);
	stearingServo.isEnabled = false;

	setupMotor();

	serialController.stearing = &stearingServo;
	serialController.motor = motor;

	remotexy = new CRemoteXY(RemoteXY_CONF_PROGMEM, &RemoteXY, REMOTEXY_ACCESS_PASSWORD, SSID, SSID_password, REMOTEXY_SERVER_PORT);//RemoteXY_Init();

	console.println("Start");
	webServer.setup();
	webServer.apName = String(SSID);

	setupController.reloadConfig = &refreshConfig;
}


int mapSpeed(int speed) {
	int corectedSpeed = abs(speed);
	if (config.potentiometer_linearity == PotentiometerLinearity::X2_div_X) {
		corectedSpeed = (speed * speed) / 100;
	}

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
			turnLightBeeper.end();
			state.emergency = false;
			alarmOff.begin();
			alarmBeepOn.begin();
			motor->isEnabled = true;
			motor->reset();
			state.stopped = true;
			stearingServo.setPosition(0, (PotentiometerLinearity)config.stearing_linearity);
			stearingServo.isEnabled = true;
			connected = true;
		}
		int pos;
		int speed;
		switch (config.drive_mode)
		{
		case 1: //лівий джойстик повороти, Повзунок - швидкість
			speed = mapSpeed(RemoteXY.right_joy_y);
			motor->setSpeed(speed);
			if (speed < 0)
				state.backLightMode = LightMode::ON;
			else if (speed == 0) {
				if (state.backLightMode == LightMode::ON)
					state.backLightMode = LightMode::WAIT_FOR_TIMEOUT;
			}
			else
				state.backLightMode = LightMode::OFF;

			if (RemoteXY.right_joy_y > -10 && RemoteXY.right_joy_y < 10) {
				if (!state.stopped) {
					//stopLight.begin();
					state.stopped = true;
					state.stoppedTime = millis();
				}
			}
			else {
				//stopLight.end();
				state.stopped = false;
			}
			stearingServo.setPosition(RemoteXY.left_joy_x, (PotentiometerLinearity)config.stearing_linearity);
			handleTurnLight(RemoteXY.left_joy_x);
			break;

		default: //Все керування лівим джойстиком
			speed = mapSpeed(RemoteXY.left_joy_y);
			motor->setSpeed(speed);
			if (speed < 0)
				state.backLightMode = LightMode::ON;
			else
				state.backLightMode = LightMode::OFF;

			if (RemoteXY.left_joy_y > -5 && RemoteXY.left_joy_y < 5) {
				if (!state.stopped) {
					//stopLight.begin();
					state.stopped = true;
					state.stoppedTime = millis();
				}
			}
			else {
				//stopLight.end();
				state.stopped = false;
			}
			stearingServo.setPosition(RemoteXY.left_joy_x, (PotentiometerLinearity)config.stearing_linearity);
			handleTurnLight(RemoteXY.left_joy_x);
			break;
		}

		if (state.speed != speed) {
			//швидкість змінилась
			if (speed == 0) {//Зупинка
				stopLight.begin();
			}
			else {
				if (abs(speed) > abs(state.speed)) {//Швидкість зросла
					stopLight.end();
				}
				else {
					if (abs(state.speed - speed) > 10) {//Швидкість впала більше ніж на 10
						stopLight.begin();
					}
				}
			}
			state.speed = speed;
		}

		if (RemoteXY.Alarm) {
			if (!state.emergency_btn_pressed) {
				state.emergency_btn_pressed = true;
				if (state.emergency) {
					leftLight.end();
					rightLight.end();
					turnLightBeeper.end();
					state.emergency = false;
				}
				else {
					leftLight.begin();
					rightLight.begin();
					turnLightBeeper.begin();
					state.emergency = true;
				}
			}
		}
		else {
			if (state.emergency_btn_pressed) {
				state.emergency_btn_pressed = false;
			}
		}


		handleLight();
	}
	else {
		if (connected) {
			console.println("Disconnected!");
			connected = false;
			leftLight.end();
			rightLight.end();
			turnLightBeeper.end();
			state.emergency = false;
			alarmOn.begin();
			alarmBeepOff.begin();
			motor->isEnabled = false;
			motor->reset();
			stearingServo.setPosition(0, (PotentiometerLinearity)config.stearing_linearity);
			state.LightMode = 0;
			btn_HeadLight.setValue(RemoteXY.Light_low);
			handleLight();
			stearingServo.isEnabled = false;
		}
	}
	if (config.debug) {
		serialController.loop();
	}
	if (state.serialEnabled) {
		if (alarmBeepOn.isRunning() || alarmBeepOff.isRunning()) {
			//Serial.end();
			state.serialEnabled = false;
		}
	}
	else
	{
		if (!alarmBeepOn.isRunning() && !alarmBeepOff.isRunning()) {
			//Serial.begin(115200);
			state.serialEnabled = true;
		}
	}

	motor->loop();
	stearingServo.loop();
	leftLight.loop();
	rightLight.loop();
	turnLightBeeper.loop();
	alarmOff.loop();
	alarmBeepOff.loop();
	alarmOn.loop();
	alarmBeepOn.loop();
	stopLight.loop();
	backLight.loop();
	btn_HeadLight.handle();
	webServer.loop();
}
