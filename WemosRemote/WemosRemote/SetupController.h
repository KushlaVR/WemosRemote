#pragma once
#include "WebUIController.h"

enum PotentiometerLinearity {
	Linear,// y=x ˳����� ������������
	X2_div_X// y=X^2 / X
};


struct ConfigStruct {
public:
	String ssid;
	String password;
	bool debug;

	int center;//�������
	int max_left;//�������
	int max_right;//�������
	int stearing_linearity;//��� ���������

	int min_speed;//�� 0 �� 256
	int potentiometer_linearity;//��� ���������

	int front_light_on;//� ���������
	int parking_light_on;//� ���������
	int stop_light_duration;//� ����������
	int back_light_timeout;//� ����������
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
