#pragma once
#include "WebUIController.h"

struct ConfigStruct {
public:
	String ssid;
	String password;
	int center;//градусів
	int max_left;//градусів
	int max_right;//градусів

	int min_speed;//від 0 до 256
	int front_light_on;//в процентах
	int parking_light_on;//в процентах
	int stop_light_duration;//в мілісекундах
	
	bool debug;

	int LightMode;
	int backLightMode;

	bool stopped;
	bool emergency;
	bool light_btn;

};


class SetupController
{
public:
	ConfigStruct * cfg;

	SetupController();
	~SetupController();

	void saveConfig();
	void printConfig(JsonString * out);

	static void Setup_Get();
	static void Setup_Post();
};


extern SetupController setupController;
