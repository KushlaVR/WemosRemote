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

	int controller_type;
	int min_speed;//�� 0 �� 256
	int inertion;//�� 0 �� 10000
	int potentiometer_linearity;//��� ���������

	int front_light_on;//� ���������
	int high_light_on;//� ���������
	int parking_light_on;//� ���������
	int turn_light_on;//� ���������
	int turn_light_limit;//�� 0 �� 100

	int wipers_speed1;///�� 0 �� 100 �������� ������������
	int wipers_speed2;///�� 0 �� 100 �������� ������������

	int stop_light_duration;//� ����������

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
