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
	int inertion;//�� 0 �� 10000
	int potentiometer_linearity;//��� ���������

	int front_light_pwm;//� ���������
	int high_light_pwm;//� ���������
	int parking_light_pwm;//� ���������
	int turn_light_pwm;//� ���������
	int stop_light_pwm;//� ���������
	int back_light_pwm;//� ���������

	int stop_light_duration;//� ����������
	int back_light_timeout;//� ����������

	int beep_freq;
	int beep_duration;
	int beep_interval;


	int drive_mode;//����� ���������
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
