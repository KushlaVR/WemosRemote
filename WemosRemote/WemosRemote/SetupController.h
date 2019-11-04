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

	int min_speed;//від 0 до 256
	int potentiometer_linearity;//Тип керування

	int front_light_on;//в процентах
	int parking_light_on;//в процентах
	int stop_light_duration;//в мілісекундах
	int back_light_timeout;//в мілісекундах
	int back_light_pwm;

};


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
};


extern SetupController setupController;
