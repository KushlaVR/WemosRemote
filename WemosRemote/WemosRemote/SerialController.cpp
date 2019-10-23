#include "SerialController.h"


SerialController::SerialController()
{
}


SerialController::~SerialController()
{
}

void SerialController::loop()
{
	if (Serial.available()) {
		cmd[cmdPos] = Serial.read();
		if (cmd[cmdPos] == 10 || cmd[cmdPos] == 13) {
			if (cmdPos > 0) {
				cmd[cmdPos + 1] = 0;
				String s = String(cmd);
				isRunning = true;
				if (s.startsWith("motor=")) {
					cmdMotor(s.substring(6));
				}
				else if (s.startsWith("stearing=")) {
					cmdStearing(s.substring(9));
				}
				else if (s.startsWith("flash=")) {
					cmdFlash(s.substring(6));
				}
				else if (s.startsWith("end")) {
					isRunning = true;
				}
			}
			cmdPos = 0;
		}
		else if (cmdPos == 255) {
			cmdPos = 0;
		}
		else {
			cmdPos++;
		}
	}
}

void SerialController::cmdMotor(String cmd)
{
	motor->setSpeed(cmd.toInt());

}

void SerialController::cmdStearing(String cmd)
{
	stearing->write(cmd.toInt());
}

void SerialController::cmdFlash(String cmd)
{
	if (cmd.startsWith("left"))
		leftLight->begin();
	else if (cmd.startsWith("right"))
		rightLight->begin();
	else if (cmd.startsWith("alarm")) {
		rightLight->begin();
		leftLight->begin();
	}
	else if (cmd.startsWith("siren1"))
		siren1->begin();
}
