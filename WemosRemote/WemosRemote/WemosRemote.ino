/*
 Name:    WemosRemote.ino
 Created: 10/23/2019 7:53:20 PM
 Authors:  KushlaVR@gmail.com, zilibob4ik
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

#include <AudioFileSourceSPIFFS.h>
#include <AudioFileSourceID3.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2SNoDAC.h>


#include "mp3.h"

#define pinServo D5
#define pinMotorA D7//назад
#define pinMotorB D6//вперед
#define pinFrontLight D4
#define pinLeftLight D2
#define pinRightLight D1
#define pinBackLight D3
#define pinStopLight D8
#define pinParkingLight D0
//#define pinBuzzer RX


// Called when a metadata event occurs (i.e. an ID3 tag, an ICY block, etc.
void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string)
{
	(void)cbData;
	Serial.printf("ID3 callback for: %s = '", type);

	if (isUnicode) {
		string += 2;
	}

	while (*string) {
		char a = *(string++);
		if (isUnicode) {
			string++;
		}
		Serial.printf("%c", a);
	}
	Serial.printf("'\n");
	Serial.flush();
}




// конфигурация интерфейса   
// RemoteXY configurate   
#pragma pack(push, 1) 
uint8_t RemoteXY_CONF[] = {
  255,7,0,0,0,51,0,8,25,0,
  1,0,36,18,20,20,36,31,65,0,
  1,0,54,-2,19,19,177,31,72,0,
  1,0,54,44,20,20,132,16,76,0,
  5,43,-10,5,52,52,2,26,31,5,
  32,61,0,63,63,176,26,31
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
	WAIT_FOR_TIMEOUT = 3
};

struct StateStructure {

	int LightMode;
	unsigned long stoppedTime;
	int backLightMode;

	int speed;
	bool stopped;
	bool emergency;
	bool emergency_btn_pressed;
	bool light_btn;
	bool high_light_btn;

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
MotorBase * motor = nullptr;// HBridge("motor", pinMotorA, pinMotorB, &motorEffect);
Stearing stearingServo = Stearing(pinServo);

SerialController serialController = SerialController();

Blinker leftLight = Blinker("Left light");
Blinker rightLight = Blinker("Right light");
Blinker stopLight = Blinker("Stop light");
Blinker backLight = Blinker("Back light");
Blinker alarmOn = Blinker("Alarm on");
Blinker alarmOff = Blinker("Alarm of");
Beeper turnLightBeeper = Beeper("Turn light beep");

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

bool handledhigh_light_btn_state = false;
int handledLightMode = 0;
int handledBackLightMode = 0;
void handleLight() {
	if (!RemoteXY.connect_flag) {
		state.LightMode = LightMode::OFF;
	};
	if (handledLightMode != state.LightMode || handledhigh_light_btn_state != state.high_light_btn) {
		handledLightMode = state.LightMode;
		if (handledhigh_light_btn_state != state.high_light_btn) {
			handledhigh_light_btn_state = state.high_light_btn;
			if (state.high_light_btn) {
				playMP3("/mp3/klakson.mp3", 1000);
			}
			else {
				stopMP3();
			}
		}
		int val;
		switch (state.LightMode)
		{
		case LightMode::OFF:
			if (state.high_light_btn)
				analogWrite(pinFrontLight, map(config.high_light_on, 0, 100, 0, 255));
			else
				digitalWrite(pinFrontLight, LOW);
			digitalWrite(pinParkingLight, LOW);
			break;
		case LightMode::Parking:
			if (state.high_light_btn)
				analogWrite(pinFrontLight, map(config.high_light_on, 0, 100, 0, 255));
			else
				analogWrite(pinFrontLight, map(config.parking_light_on, 0, 100, 0, 255));
			analogWrite(pinParkingLight, map(config.parking_light_on, 0, 100, 0, 255));
			break;
		case LightMode::ON:
			if (state.high_light_btn)
				analogWrite(pinFrontLight, map(config.high_light_on, 0, 100, 0, 255));
			else
				analogWrite(pinFrontLight, map(config.front_light_on, 0, 100, 0, 255));
			analogWrite(pinParkingLight, map(config.parking_light_on, 0, 100, 0, 255));
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

	backLight.Add(pinBackLight, 0, 0)
		->Add(pinBackLight, map(255 - config.back_light_pwm, 0, 255, 0, 20), 255)
		->Add(pinBackLight, 20, 0)
		->repeat = true;

	/*turnLightBeeper.Add(pinBuzzer, 0, 1000)
		->Add(pinBuzzer, 1, 0)
		->Add(pinBuzzer, 500, 1000)
		->Add(pinBuzzer, 501, 0)
		->Add(pinBuzzer, 1000, 0);*/

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
	motor->setWeight(800);
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

	backLight.item(0)->offset = map(100 - config.back_light_pwm, 0, 100, 0, 20);

}


void setup()
{
	state.serialEnabled = true;
	//Serial.end();
	//pinMode(pinBuzzer, OUTPUT);
	//digitalWrite(pinBuzzer, HIGH);
	Serial.begin(115200);
	Serial.println();
	Serial.println();
	SPIFFS.begin();

	/*
	   file = new AudioFileSourceSPIFFS("/alarm.mp3");
	  id3 = new AudioFileSourceID3(file);
	  id3->RegisterMetadataCB(MDCallback, (void*)"ID3TAG");
	  out = new AudioOutputI2SNoDAC();
	  mp3 = new AudioGeneratorMP3();
	  mp3->begin(id3, out);
	*/
	initMP3(); //инициализируем библиотеку МП3


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
	//  Serial.end();
	//  console.output = nullptr;
	//}

	setupBlinkers();

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

	//mp3PlayTrack(signals[0].c_str()); //Воспроизводим сигнал
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
	loopMP3();

	RemoteXY_Handler(); //под вопросом

	stearingServo.max_left = config.max_left;
	stearingServo.max_right = config.max_right;
	stearingServo.center = config.center;

	if (RemoteXY.connect_flag) {
		if (!connected) {
			console.println("Connected!");
			playMP3("/mp3/two-beep.mp3");
			leftLight.end();
			rightLight.end();
			turnLightBeeper.end();
			state.emergency = false;
			alarmOff.begin();
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
		if (RemoteXY.Light_low != state.light_btn) {
			if (RemoteXY.Light_low) {
				state.LightMode++;
				if (state.LightMode > LightMode::ON) state.LightMode = 0;
			}
			state.light_btn = RemoteXY.Light_low;
		}
		if (RemoteXY.Light_high != state.high_light_btn) {
			state.high_light_btn = RemoteXY.Light_high;
		}

		handleLight();
	}
	else {
		if (connected) {
			console.println("Disconnected!");
			playMP3("/mp3/one-beep.mp3", 500);
			connected = false;
			leftLight.end();
			rightLight.end();
			turnLightBeeper.end();
			state.emergency = false;
			alarmOn.begin();
			motor->reset();
			motor->isEnabled = false;
			stearingServo.setPosition(0, (PotentiometerLinearity)config.stearing_linearity);
			state.LightMode = 0;
			handleLight();
			stearingServo.isEnabled = false;
		}
	}
	if (config.debug) {
		serialController.loop();
	}

	motor->loop();
	stearingServo.loop();
	leftLight.loop();
	rightLight.loop();
	turnLightBeeper.loop();
	alarmOff.loop();
	alarmOn.loop();
	stopLight.loop();
	backLight.loop();
	webServer.loop();
}