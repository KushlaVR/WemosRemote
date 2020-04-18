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

	int ch1_min;
	int ch1_center;
	int ch1_max;

	int ch2_min;
	int ch2_center;
	int ch2_max;

	int ch3_min;
	int ch3_max;

	int ch4_min;
	int ch4_max;

	int port_addr;

	int turn_light_limit;
	int reverce_limit;

	int wiper0;
	int wiper180;

	int wiper1Duration;
	int wiper1Pause;

	int wiper2Duration;
	int wiper2Pause;

	int stop_light_duration;//в мілісекундах
	int back_light_timeout;//в мілісекундах
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
