#pragma once
#include "WebUIController.h"

enum PotentiometerLinearity {
	Linear,// y=x Лінійний потенціометр
	X2_div_X// y=X^2 / X
};


struct ConfigStruct {
public:
	String ssid;
	String password;
	bool debug;

	int center;//градусів
	int max_left;//градусів
	int max_right;//градусів
	int stearing_linearity;//Тип керування

	int controller_type;
	int min_speed;//від 0 до 256
	int inertion;//від 0 до 10000
	int potentiometer_linearity;//Тип керування

	int servo2_min;
	int servo2_max;

	int servo3_min;
	int servo3_max;

	int servo4_min;
	int servo4_max;

	int stop_light_duration;//в мілісекундах
	int back_light_timeout;//в мілісекундах

	int beep_freq;
	int beep_duration;
	int beep_interval;

	int drive_mode;//Режим керування
};

typedef void(*myFunctionPointer) ();

class SetupController
{
public:
	ConfigStruct * cfg;

	SetupController();
	~SetupController();

	void loadConfig();
	void saveConfig();
	void printConfig(JsonString * out);

	static void Setup_Get();
	static void Setup_Post();

	myFunctionPointer reloadConfig = nullptr;
};


extern SetupController setupController;
