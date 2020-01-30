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




#define pinServo D5//Руль
#define pinMotorA D7//Тяговий мотор
#define pinMotorB D6//Тяговий мотор
#define pinFrontLight D4 //Передні фари
#define pinLeftLight D2 //Повороти
#define pinRightLight D1 //Повороти
#define pinWipers D0//Двірники
#define pinStopLight D8//Стопи та габарити
#define pinParkingLight D3//Підсвітка номера
#define pinBuzzer RX



// конфигурация интерфейса   
// RemoteXY configurate  
#pragma pack(push, 1)
uint8_t RemoteXY_CONF[] =
{ 255,7,0,0,0,51,0,8,25,0,
1,0,47,21,20,20,177,31,72,0,
1,0,54,-2,19,19,233,31,87,0,
1,0,54,44,20,20,132,16,76,0,
5,43,-10,5,52,52,2,26,31,5,
32,61,0,63,63,176,26,31 };

// this structure defines all the variables of your control interface 
struct {

	// input variable
	uint8_t Light_high; // =1 if button pressed, else =0 
	uint8_t Wipers; // =1 if button pressed, else =0 
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
	WAIT_FOR_TIMEOUT = 3
};

struct StateStructure {

	int LightMode;
	unsigned long stoppedTime;

	int speed;
	bool stopped;

	bool light_btn;
	int handledLightMode = 0;

	bool highlight_btn;
	bool handledhigh_light_btn_state = false;

	bool turnOffTurnLights = false;

	bool wipers_btn;
	int wipers;
	int wiper_increment = 1;
	int handledWipers;

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
Blinker signLight = Blinker("Sign light");
Blinker alarmOn = Blinker("Alarm on");
Blinker alarmOff = Blinker("Alarm of");
Beeper alarmBeepOn = Beeper("Alarm beep on");
Beeper alarmBeepOff = Beeper("Alarm beep of");
Beeper turnLightBeeper = Beeper("Turn light beep");



void handleTurnLight(int stearing) {
	/*if (state.emergency) {//Якщо включена аварійка - нічого не робимо
		turnOffTurnLights = false;
		return;
	}*/
	if (stearing < -config.turn_light_limit) { //Включений лівий поворот
		if (!leftLight.isRunning()) { leftLight.begin(); turnLightBeeper.begin(); }
		rightLight.end();
	}
	if (stearing > config.turn_light_limit) { //Включений правий поворот
		if (!rightLight.isRunning()) { rightLight.begin(); turnLightBeeper.begin(); }
		leftLight.end();
	}
	if (leftLight.isRunning()) {
		if (stearing < -config.turn_light_limit) state.turnOffTurnLights = true;//ставимо флажок, щоб вимкнути поворот після того як руль вернеться в прямк положенн
	}
	else if (rightLight.isRunning()) {
		if (stearing > config.turn_light_limit) state.turnOffTurnLights = true;//ставимо флажок, щоб вимкнути поворот після того як руль вернеться в прямк положенн
	}
	if (state.turnOffTurnLights && stearing > -config.turn_light_limit && stearing < config.turn_light_limit) {//Повернули руль в стартове положення
		if (leftLight.isRunning() && !rightLight.isRunning()) {//блимає лівий поворот
			leftLight.end();
			turnLightBeeper.end();
			state.turnOffTurnLights = false;
			console.println("Лівий поворот вимкнено.");
		}
		else if (!leftLight.isRunning() && rightLight.isRunning()) {//блимає правий поворот
			rightLight.end();
			turnLightBeeper.end();
			state.turnOffTurnLights = false;
			console.println("Правий поворот вимкнено.");
		}
	}
}

void handleLight() {
	if (!RemoteXY.connect_flag) {
		state.LightMode = LightMode::OFF;
	};
	if (state.handledLightMode != state.LightMode || state.handledhigh_light_btn_state != state.highlight_btn) {
		state.handledLightMode = state.LightMode;

		if (state.handledhigh_light_btn_state != state.highlight_btn) {
			state.handledhigh_light_btn_state = state.highlight_btn;
			if (state.highlight_btn) {
				//Бібікаємо
				//analogWriteFreq(config.beep_freq);
				tone(pinBuzzer, config.beep_freq);
			}
			else
			{
				noTone(pinBuzzer);
			}
		}

		int val;
		switch (state.LightMode)
		{
		case LightMode::OFF:
			signLight.end();
			if (state.highlight_btn)
				analogWrite(pinFrontLight, map(config.high_light_on, 0, 100, 0, 255));
			else
				digitalWrite(pinFrontLight, LOW);
			digitalWrite(pinParkingLight, LOW);
			break;
		case LightMode::Parking:
			if (!signLight.isRunning()) signLight.begin();

			if (state.highlight_btn)
				analogWrite(pinFrontLight, map(config.high_light_on, 0, 100, 0, 255));
			else
				analogWrite(pinFrontLight, map(config.parking_light_on, 0, 100, 0, 255));
			analogWrite(pinParkingLight, map(config.parking_light_on, 0, 100, 0, 255));
			break;
		case LightMode::ON:
			if (!signLight.isRunning()) signLight.begin();
			if (state.highlight_btn)
				analogWrite(pinFrontLight, map(config.high_light_on, 0, 100, 0, 255));
			else
				analogWrite(pinFrontLight, map(config.front_light_on, 0, 100, 0, 255));
			analogWrite(pinParkingLight, map(config.parking_light_on, 0, 100, 0, 255));
			break;
		default:
			break;
		}
	}
}

void handleWipers() {
	if (state.handledWipers != state.wipers) {
		state.handledWipers == state.wipers;
		if (state.wipers == 0) {
			analogWrite(pinWipers, 0);
		}
		else if (state.wipers == 1) {
			analogWrite(pinWipers, config.wipers_speed1);
		}
		else if (state.wipers == 2) {
			analogWrite(pinWipers, config.wipers_speed2);
		}
	}
}

void setupBlinkers() {
	//Налаштування фар
	pinMode(pinFrontLight, OUTPUT);
	pinMode(pinLeftLight, OUTPUT);
	pinMode(pinRightLight, OUTPUT);
	pinMode(pinParkingLight, OUTPUT);
	pinMode(pinStopLight, OUTPUT);

	digitalWrite(pinFrontLight, LOW);
	digitalWrite(pinLeftLight, LOW);
	digitalWrite(pinRightLight, LOW);
	digitalWrite(pinParkingLight, LOW);
	digitalWrite(pinStopLight, LOW);

	pinMode(pinParkingLight, OUTPUT);
	digitalWrite(pinParkingLight, LOW);


	//Налаштування поворотників
	leftLight
		.Add(pinLeftLight, 0, config.turn_light_on)
		->Add(pinLeftLight, 500, 0)
		->Add(pinLeftLight, 1000, 0);
	serialController.leftLight = &leftLight;
	rightLight
		.Add(pinRightLight, 0, config.turn_light_on)
		->Add(pinRightLight, 500, 0)
		->Add(pinRightLight, 1000, 0);
	serialController.rightLight = &rightLight;

	alarmOff
		.Add(pinLeftLight, 0, config.turn_light_on)
		->Add(pinRightLight, 0, config.turn_light_on)
		->Add(pinLeftLight, 300, 0)
		->Add(pinRightLight, 300, 0)
		->Add(pinLeftLight, 600, config.turn_light_on)
		->Add(pinRightLight, 600, config.turn_light_on)
		->Add(pinLeftLight, 900, 0)
		->Add(pinRightLight, 900, 0);
	alarmOff.repeat = false;
	//alarmOff.debug = true;

	alarmOn
		.Add(pinLeftLight, 0, config.turn_light_on)
		->Add(pinRightLight, 0, config.turn_light_on)
		->Add(pinLeftLight, 600, 0)
		->Add(pinRightLight, 600, 0);
	alarmOn.repeat = false;
	//alarmOn.debug = true;

	stopLight.Add(pinStopLight, 0, 0)
		->Add(pinStopLight, 0, config.front_light_on)
		->Add(pinStopLight, config.stop_light_duration, 0)
		->repeat = false;
	stopLight.debug = true;

	signLight.Add(pinParkingLight, 0, 0)
		->Add(pinParkingLight, map(100 - config.parking_light_on, 0, 100, 0, 20), 255)
		->Add(pinParkingLight, 20, 0)
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
		if (motor->controllerType != config.controller_type) {
			delete motor;
			motor = nullptr;
		}
	}
	if (motor == nullptr) {
		switch (config.controller_type)
		{
		case 0:
			motor = new HBridge("H-Bridge", pinMotorA, pinMotorB, &motorEffect);
			break;
		case 1:
			motor = new SpeedController("Speed reg D6", pinMotorB, &motorEffect);
			break;
		case 2:
			motor = new SpeedController("Speed reg D7", pinMotorA, &motorEffect);
			break;
		default:
			return;
			break;
		}
	}

	motor->responder = &console;
	motor->setWeight(config.inertion);
	motor->reset();
	motor->isEnabled = RemoteXY.connect_flag;
	serialController.motor = motor;

}

void refreshConfig() {

	setupMotor();

	leftLight.item(0)->value = config.turn_light_on;
	rightLight.item(0)->value = config.turn_light_on;

	alarmOn.item(0)->value = config.turn_light_on;
	alarmOn.item(1)->value = config.turn_light_on;

	alarmOff.item(0)->value = config.turn_light_on;
	alarmOff.item(1)->value = config.turn_light_on;
	alarmOff.item(4)->value = config.turn_light_on;
	alarmOff.item(5)->value = config.turn_light_on;

	stopLight.item(1)->value = config.front_light_on;
	stopLight.item(2)->offset = config.stop_light_duration;

	signLight.item(0)->offset = map(100 - config.parking_light_on, 0, 100, 0, 20);

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
			state.wipers = 0;
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

			if (RemoteXY.right_joy_y > -10 && RemoteXY.right_joy_y < 10) {
				if (!state.stopped) {
					state.stopped = true;
					state.stoppedTime = millis();
				}
			}
			else {
				state.stopped = false;
			}
			stearingServo.setPosition(RemoteXY.left_joy_x, (PotentiometerLinearity)config.stearing_linearity);
			handleTurnLight(RemoteXY.left_joy_x);
			break;

		default: //Все керування лівим джойстиком
			speed = mapSpeed(RemoteXY.left_joy_y);
			motor->setSpeed(speed);

			if (RemoteXY.left_joy_y > -5 && RemoteXY.left_joy_y < 5) {
				if (!state.stopped) {
					state.stopped = true;
					state.stoppedTime = millis();
				}
			}
			else {
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

		if (RemoteXY.Wipers) {
			if (!state.wipers_btn) {
				state.wipers_btn = true;
				if (state.wiper_increment == 0) state.wiper_increment = 1;
				state.wipers+= state.wiper_increment;
				if (state.wipers == 2) {
					state.wiper_increment = -1;
				}
				else if (state.wipers == 0) {
					state.wiper_increment = 1;
				}
			}
		}
		else {
			if (state.wipers_btn) {
				state.wipers_btn = false;
			}
		}
		if (RemoteXY.Light_low != state.light_btn) {
			if (RemoteXY.Light_low) {
				state.LightMode++;
				if (state.LightMode > LightMode::ON) state.LightMode = 0;
			}
			state.light_btn = RemoteXY.Light_low;
		}
		if (RemoteXY.Light_high != state.highlight_btn) {
			state.highlight_btn = RemoteXY.Light_high;
		}

		handleLight();
		handleWipers();
	}
	else {
		if (connected) {
			console.println("Disconnected!");
			connected = false;
			leftLight.end();
			rightLight.end();
			turnLightBeeper.end();
			state.wipers = 0;
			alarmOn.begin();
			alarmBeepOff.begin();
			motor->isEnabled = false;
			motor->reset();
			stearingServo.setPosition(0, (PotentiometerLinearity)config.stearing_linearity);
			state.LightMode = 0;
			handleLight();
			handleWipers();
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
	signLight.loop();
	webServer.loop();
}
