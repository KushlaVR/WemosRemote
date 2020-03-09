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
#include <Wire.h>
#include "PCF8574.h"

#include "Console.h"
#include "RoboconMotor.h"
#include "Stearing.h"
#include "Json.h"
#include "SerialController.h"
#include "Blinker.h"
#include "Button.h"
#include <RemoteXY.h> 
#include "WebUIController.h"
#include "SetupController.h"

#define pinServo D5
#define pinServo2 D8

#define pinMotorA D7//H bridge
#define pinMotorB D6//H bridge


#define pinI2C_SCL D1 //pcf8574
#define pinI2C_SDA D2 //pcf8574

//pcf8574 Port usage
#define bitParkingLight 0
#define bitHeadLight 1
#define bitHighLight 2
#define bitLeftLight 3
#define bitRightLight 4
#define bitBackLight 5
#define bitStopLight 6
#define bitFogLight 7

#define pinBlinker D3
//#define pinBuzzer RX


// RemoteXY configurate  
#pragma pack(push, 1)
uint8_t RemoteXY_CONF[] =
{ 255,14,0,0,0,140,0,10,8,0,
4,128,64,1,42,7,2,26,1,1,
31,36,22,12,134,31,226,151,132,0,
1,1,22,49,31,12,160,31,83,0,
1,1,56,49,31,12,48,31,65,0,
1,1,56,36,22,12,134,31,226,150,
186,0,1,1,56,23,22,12,2,31,
77,0,1,1,31,23,22,12,118,31,
76,0,1,1,22,10,31,12,132,31,
226,152,188,0,1,1,56,10,31,12,
175,31,72,0,5,32,1,17,36,36,
207,26,31,5,32,71,13,46,46,207,
26,31,1,1,42,0,17,9,101,31,
226,153,171,0,129,0,1,1,28,8,
40,67,76,65,65,83,0 };

// this structure defines all the variables and events of your control interface 
struct {

	// input variables
	int8_t servo2; // =0..100 slider position 
	uint8_t leftTurn; // =1 if button pressed, else =0 
	uint8_t fogLight; // =1 if button pressed, else =0 
	uint8_t Alarm; // =1 if button pressed, else =0 
	uint8_t rightTurn; // =1 if button pressed, else =0 
	uint8_t Blink; // =1 if button pressed, else =0 
	uint8_t headLight; // =1 if button pressed, else =0 
	uint8_t parkingLight; // =1 if button pressed, else =0 
	uint8_t highLight; // =1 if button pressed, else =0 
	int8_t left_joy_x; // =-100..100 x-coordinate joystick position 
	int8_t left_joy_y; // =-100..100 y-coordinate joystick position 
	int8_t right_joy_x; // =-100..100 x-coordinate joystick position 
	int8_t right_joy_y; // =-100..100 y-coordinate joystick position 
	uint8_t beepButton; // =1 if button pressed, else =0 

	  // other variable
	uint8_t connect_flag;  // =1 if wire connected, else =0 

} RemoteXY;
#pragma pack(pop)


/////////////////////////////////////////////
//           END RemoteXY include          //
/////////////////////////////////////////////

enum LightMode {
	OFF = 0,
	Parking = 1,
	ON = 2,
	FIGH_LIGHT = 3,
	WAIT_FOR_TIMEOUT = 10
};

struct StateStructure {

	unsigned long stoppedTime;
	int backLightMode;

	int speed;
	bool stopped;
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

PCF8574 portExt = PCF8574(0x27);
RoboEffects motorEffect = RoboEffects();
MotorBase * motor = nullptr;
Stearing stearingServo = Stearing(pinServo);
Servo servo2 = Servo();

SerialController serialController = SerialController();

extBlinker stopLight = extBlinker("Stop light", &portExt);
extBlinker leftLight = extBlinker("Left light", &portExt);
extBlinker rightLight = extBlinker("Right light", &portExt);
extBlinker alarmOn = extBlinker("Alarm on", &portExt);
extBlinker alarmOff = extBlinker("Alarm of", &portExt);
Blinker lightBlinker = Blinker("Light Blinker");
Beeper alarmBeepOn = Beeper("Alarm beep on");
Beeper alarmBeepOff = Beeper("Alarm beep of");
Beeper turnLightBeeper = Beeper("Turn light beep");

VirtualButton btn_ParkingLight = VirtualButton(btn_ParkingLight_On, nullptr, btn_ParkingLight_Off);
VirtualButton btn_HeadLight = VirtualButton(btn_HeadLight_On, nullptr, btn_HeadLight_Off);
VirtualButton btn_HighLight = VirtualButton(btn_HighLight_On, nullptr, btn_HighLight_Off);
VirtualButton btn_FogLight = VirtualButton(btn_FogLight_On, nullptr, btn_FogLight_Off);
VirtualButton btn_LeftLight = VirtualButton(btn_LeftLight_On, nullptr, btn_LeftLight_Off);
VirtualButton btn_RightLight = VirtualButton(btn_RightLight_On, nullptr, btn_RightLight_Off);
VirtualButton btn_Blink = VirtualButton(btn_Blink_On, nullptr, btn_Blink_Off);
VirtualButton btn_Alarm = VirtualButton(btn_Alarm_On, nullptr, btn_Alarm_Off);
VirtualButton btn_Beeper = VirtualButton(btn_Beeper_On, nullptr, btn_Beeper_Off);

VirtualButton out_BackLight = VirtualButton(out_BackLight_On, nullptr, out_BackLight_Off);

void btn_ParkingLight_On() {
	portExt.write(bitParkingLight, HIGH);
	console.println("btn_ParkingLight_On");
}
void btn_ParkingLight_Off() {
	portExt.write(bitParkingLight, LOW);
	console.println("btn_ParkingLight_Off");
}

void btn_HeadLight_On() {
	portExt.write(bitHeadLight, HIGH);
	console.println("btn_HeadLight_On");
}
void btn_HeadLight_Off() {
	portExt.write(bitHeadLight, LOW);
	console.println("btn_HeadLight_Off");
}

void btn_HighLight_On() {
	portExt.write(bitHighLight, HIGH);
	console.println("btn_HighLight_On");
}
void btn_HighLight_Off() {
	portExt.write(bitHighLight, LOW);
	console.println("btn_HighLight_Off");
}

void btn_FogLight_On() {
	portExt.write(bitFogLight, HIGH);
	console.println("btn_FogLight_On");
}
void btn_FogLight_Off() {
	portExt.write(bitFogLight, LOW);
	console.println("btn_FogLight_Off");
}

void btn_LeftLight_On() {
	btn_Alarm.reset();
	btn_RightLight.reset();
	if (!leftLight.isRunning()) leftLight.begin();
	console.println("btn_LeftLight_On");
}
void btn_LeftLight_Off() {
	if (leftLight.isRunning()) leftLight.end();
	console.println("btn_LeftLight_Off");
}

void btn_RightLight_On() {
	btn_Alarm.reset();
	btn_LeftLight.reset();
	if (!rightLight.isRunning()) rightLight.begin();
	console.println("btn_RightLight_On");

}
void btn_RightLight_Off() {
	if (rightLight.isRunning()) rightLight.end();
	console.println("btn_RightLight_Off");
}

void btn_Blink_On() {
	if (!lightBlinker.isRunning()) lightBlinker.begin();
	console.println("btn_Blink_On");
}
void btn_Blink_Off() {
	if (lightBlinker.isRunning()) lightBlinker.end();
	console.println("btn_Blink_Off");
}

void btn_Alarm_On() {
	if (leftLight.isRunning()) leftLight.end();
	if (rightLight.isRunning()) rightLight.end();
	btn_LeftLight.reset();
	btn_RightLight.reset();
	leftLight.begin();
	rightLight.begin();
	turnLightBeeper.begin();
	console.println("btn_Alarm_On");
}
void btn_Alarm_Off() {
	if (leftLight.isRunning()) leftLight.end();
	if (rightLight.isRunning()) rightLight.end();
	if (turnLightBeeper.isRunning()) turnLightBeeper.end();
	console.println("btn_Alarm_Off");
}

void btn_Beeper_On() {
	//tone(pinBuzzer, config.beep_freq);
	console.println("btn_Beeper_On");
}
void btn_Beeper_Off() {
	//noTone(pinBuzzer);
	console.println("btn_Beeper_Off");
}


void out_BackLight_On() {
	portExt.write(bitBackLight, HIGH);
	console.println("out_BackLight_On");
}
void out_BackLight_Off() {
	portExt.write(bitBackLight, LOW);
	console.println("out_BackLight_Off");
}


bool turnOffTurnLights = false;
int turnLightsCondition = 10;

void handleTurnLight(int stearing) {
	if (btn_Alarm.isToggled) {//Якщо включена аварійка - нічого не робимо
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

void handleLight() {

	switch (state.backLightMode)
	{
	case LightMode::OFF:
		out_BackLight.setValue(LOW);
		break;
	case LightMode::WAIT_FOR_TIMEOUT:
		if ((millis() - state.stoppedTime) > config.back_light_timeout) {
			out_BackLight.setValue(LOW);
		}
		break;
	case LightMode::ON:
		out_BackLight.setValue(HIGH);
		break;
	default:
		break;
	}
}


void setupBlinkers() {

	console.println("Stop");
	stopLight.Add(bitStopLight, 0, LOW)
		->Add(bitStopLight, 0, HIGH)
		->Add(bitStopLight, config.stop_light_duration, LOW)
		->repeat = false;
	stopLight.debug = true;

	//Налаштування поворотників
	leftLight
		.Add(bitLeftLight, 0, HIGH)
		->Add(bitLeftLight, 500, LOW)
		->Add(bitLeftLight, 1000, LOW);
	leftLight.debug = true;
	serialController.leftLight = &leftLight;

	rightLight
		.Add(bitRightLight, 0, HIGH)
		->Add(bitRightLight, 500, LOW)
		->Add(bitRightLight, 1000, LOW);
	rightLight.debug = true;
	serialController.rightLight = &rightLight;

	alarmOff
		.Add(bitLeftLight, 0, HIGH)
		->Add(bitRightLight, 0, HIGH)
		->Add(bitLeftLight, 300, 0)
		->Add(bitRightLight, 300, 0)
		->Add(bitLeftLight, 600, HIGH)
		->Add(bitRightLight, 600, HIGH)
		->Add(bitLeftLight, 900, 0)
		->Add(bitRightLight, 900, 0);
	alarmOff.repeat = false;
	alarmOff.debug = true;

	alarmOn
		.Add(bitLeftLight, 0, HIGH)
		->Add(bitRightLight, 0, HIGH)
		->Add(bitLeftLight, 600, 0)
		->Add(bitRightLight, 600, 0);
	alarmOn.repeat = false;
	alarmOn.debug = true;

	lightBlinker
		.Add(pinBlinker, 0, HIGH)
		->Add(pinBlinker, 200, LOW)
		->Add(pinBlinker, 300, HIGH)
		->Add(pinBlinker, 500, LOW)
		->Add(pinBlinker, 700, HIGH)
		->Add(pinBlinker, 900, LOW)
		->Add(pinBlinker, 1000, HIGH)
		->Add(pinBlinker, 1400, LOW)
		->Add(pinBlinker, 1500, HIGH)
		->Add(pinBlinker, 1900, LOW)
		->Add(pinBlinker, 2000, HIGH)
		->Add(pinBlinker, 2200, LOW)
		->Add(pinBlinker, 2300, HIGH)
		->Add(pinBlinker, 2500, LOW)
		->Add(pinBlinker, 2700, HIGH)
		->Add(pinBlinker, 2900, LOW)
		->Add(pinBlinker, 3000, LOW);
	lightBlinker.debug = true;
}

void setupBeepers() {
	/*alarmBeepOn
		.Add(pinBuzzer, 1, config.beep_freq)
		->Add(pinBuzzer, config.beep_duration, 0)
		->Add(pinBuzzer, config.beep_duration + config.beep_interval + 1, config.beep_freq)
		->Add(pinBuzzer, config.beep_duration + config.beep_interval + config.beep_duration, 0);
	alarmBeepOn.repeat = false;
	alarmBeepOn.debug = true;

	alarmBeepOff
		.Add(pinBuzzer, 1, config.beep_freq)
		->Add(pinBuzzer, config.beep_duration, 0);
	alarmBeepOff.repeat = false;
	alarmBeepOff.debug = true;

	turnLightBeeper.Add(pinBuzzer, 0, 1000)
		->Add(pinBuzzer, 1, 0)
		->Add(pinBuzzer, 500, 1000)
		->Add(pinBuzzer, 501, 0)
		->Add(pinBuzzer, 1000, 0);
	turnLightBeeper.debug = true;*/

}

void setupMotor() {
	pinMode(pinServo2, OUTPUT);
	digitalWrite(pinServo2, LOW);

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

	/*
	leftLight.item(0)->value = config.turn_light_on;
	rightLight.item(0)->value = config.turn_light_on;

	alarmOn.item(0)->value = config.turn_light_on;
	alarmOn.item(1)->value = config.turn_light_on;

	alarmOff.item(0)->value = config.turn_light_on;
	alarmOff.item(1)->value = config.turn_light_on;
	alarmOff.item(4)->value = config.turn_light_on;
	alarmOff.item(5)->value = config.turn_light_on;
	*/

	//stopLight.item(1)->value = config.front_light_on;
	stopLight.item(2)->offset = config.stop_light_duration;

	//backLight.item(0)->offset = map(100 - config.back_light_pwm, 0, 100, 0, 20);

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
	//pinMode(pinBuzzer, OUTPUT);
	//digitalWrite(pinBuzzer, LOW);
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

	console.println(("Start port extender..."));

	portExt.begin(pinI2C_SDA, pinI2C_SCL, 0x00);

	console.println(("Done."));

	btn_ParkingLight.isToggleMode = true;
	btn_HeadLight.isToggleMode = true;
	btn_HighLight.isToggleMode = true;
	btn_FogLight.isToggleMode = true;
	btn_LeftLight.isToggleMode = true;
	btn_RightLight.isToggleMode = true;
	btn_Blink.isToggleMode = true;
	btn_Alarm.isToggleMode = true;

	//out_BackLight.isToggleMode = true;

	console.println(("Blinkers."));
	setupBlinkers();
	console.println(("Beeper."));
	console.flush();
	setupBeepers();

	stearingServo.max_left = config.max_left;
	stearingServo.max_right = config.max_right;
	stearingServo.center = config.center;
	stearingServo.setPosition(0, (PotentiometerLinearity)config.stearing_linearity);
	stearingServo.isEnabled = false;

	setupMotor();

	serialController.stearing = &stearingServo;
	serialController.motor = motor;
	serialController.portExt = &portExt;

	console.println("RemoteXY");

	remotexy = new CRemoteXY(RemoteXY_CONF_PROGMEM, &RemoteXY, REMOTEXY_ACCESS_PASSWORD, SSID, SSID_password, REMOTEXY_SERVER_PORT);//RemoteXY_Init();

	console.println("Start");
	webServer.setup();
	webServer.apName = String(SSID);

	setupController.reloadConfig = &refreshConfig;
	console.println(("Setup complete."));

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

			btn_ParkingLight.reset();
			btn_HeadLight.reset();
			btn_HighLight.reset();
			btn_FogLight.reset();
			btn_LeftLight.reset();
			btn_RightLight.reset();
			btn_Blink.reset();
			btn_Alarm.reset();
			btn_Beeper.reset();

			alarmOff.begin();
			alarmBeepOn.begin();
			motor->isEnabled = true;
			motor->reset();
			state.stopped = true;
			stearingServo.setPosition(0, (PotentiometerLinearity)config.stearing_linearity);
			stearingServo.isEnabled = true;
			servo2.attach(pinServo2);
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

		if (speed < 0)
			state.backLightMode = LightMode::ON;
		else if (speed == 0) {
			if (state.backLightMode == LightMode::ON)
				state.backLightMode = LightMode::WAIT_FOR_TIMEOUT;
		}
		else
			state.backLightMode = LightMode::OFF;


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

		btn_ParkingLight.setValue(RemoteXY.parkingLight);
		btn_HeadLight.setValue(RemoteXY.headLight);
		btn_HighLight.setValue(RemoteXY.highLight);
		btn_FogLight.setValue(RemoteXY.fogLight);
		btn_LeftLight.setValue(RemoteXY.leftTurn);
		btn_RightLight.setValue(RemoteXY.rightTurn);
		btn_Blink.setValue(RemoteXY.Blink);
		btn_Alarm.setValue(RemoteXY.Alarm);
		btn_Beeper.setValue(RemoteXY.beepButton);
		servo2.write(map(RemoteXY.servo2, 0, 100, config.servo2_min, config.servo2_max));
		handleLight();
	}
	else {
		if (connected) {
			console.println("Disconnected!");
			connected = false;
			leftLight.end();
			rightLight.end();
			turnLightBeeper.end();
			alarmOn.begin();
			alarmBeepOff.begin();
			motor->isEnabled = false;
			motor->reset();
			stearingServo.setPosition(0, (PotentiometerLinearity)config.stearing_linearity);
			servo2.detach();
			btn_ParkingLight.reset();
			btn_HeadLight.reset();
			btn_HighLight.reset();
			btn_FogLight.reset();
			btn_LeftLight.reset();
			btn_RightLight.reset();
			btn_Blink.reset();
			btn_Alarm.reset();
			btn_Beeper.reset();

			handleLight();
			stearingServo.isEnabled = false;
		}
	}
	serialController.loop();
	/*if (state.serialEnabled) {
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
	}*/

	motor->loop();
	stearingServo.loop();

	btn_ParkingLight.handle();
	btn_HeadLight.handle();
	btn_HighLight.handle();
	btn_FogLight.handle();
	btn_LeftLight.handle();
	btn_RightLight.handle();
	btn_Blink.handle();
	btn_Alarm.handle();
	btn_Beeper.handle();

	out_BackLight.handle();

	leftLight.loop();
	rightLight.loop();
	turnLightBeeper.loop();
	alarmOff.loop();
	alarmBeepOff.loop();
	alarmOn.loop();
	alarmBeepOn.loop();
	stopLight.loop();
	lightBlinker.loop();
	//portExt.write8(portExt.valueOut());
	webServer.loop();
}
