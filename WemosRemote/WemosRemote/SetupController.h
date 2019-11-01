#pragma once
#include "WebUIController.h"

struct ConfigStruct {
public:
	String ssid;
	String password;
	int center;//�������
	int max_left;//�������
	int max_right;//�������

	int min_speed;//�� 0 �� 256
	int front_light_on;//� ���������
	int parking_light_on;//� ���������
	int stop_light_duration;//� ����������
	
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
