#include "SetupController.h"



SetupController::SetupController()
{
	webServer.on("/api/setup", HTTPMethod::HTTP_GET, Setup_Get);
	webServer.on("/api/setup", HTTPMethod::HTTP_POST, Setup_Post);
}

SetupController::~SetupController()
{
}

void SetupController::loadConfig()
{
	String s;
	JsonString cfg = "";
	File cfgFile;
	if (!SPIFFS.exists("/config.json")) {
		console.println(("Default setting loaded..."));
		cfg.beginObject();
		cfg.AddValue("ssid", "WEMOS");
		cfg.AddValue("password", "12345678");
		cfg.AddValue("mode", "debug");

		cfg.AddValue("center", "90");
		cfg.AddValue("max_left", "120");
		cfg.AddValue("max_right", "60");
		cfg.AddValue("stearing_linearity", "1");

		cfg.AddValue("controller_type", "0");
		cfg.AddValue("min_speed", "50");
		cfg.AddValue("inertion", "800");
		cfg.AddValue("potentiometer_linearity", "1");

		cfg.AddValue("front_light_on", "40");
		cfg.AddValue("high_light_on", "80");
		cfg.AddValue("parking_light_on", "10");
		cfg.AddValue("turn_light_on", "80");
		cfg.AddValue("stop_light_duration", "2000");
		cfg.AddValue("back_light_timeout", "500");
		cfg.AddValue("back_light_pwm", "70");
		cfg.AddValue("beep_freq", "1000");
		cfg.AddValue("beep_duration", "150");
		cfg.AddValue("beep_interval", "50");
		cfg.AddValue("drive_mode", "1");

		cfg.endObject();

		cfgFile = SPIFFS.open("/config.json", "w");
		cfgFile.print(cfg.c_str());
		cfgFile.flush();
		cfgFile.close();
	}
	else {
		console.println(("Reading config..."));
		cfgFile = SPIFFS.open("/config.json", "r");
		s = cfgFile.readString();
		cfg = JsonString(s.c_str());
		cfgFile.close();
	}

	this->cfg->ssid = String(cfg.getValue("ssid"));
	this->cfg->password = String(cfg.getValue("password"));

	s = cfg.getValue("mode");
	if (s == "debug")
		this->cfg->debug = true;
	else
		this->cfg->debug = false;

	//Servo config reading
	this->cfg->center = cfg.getInt("center");
	this->cfg->max_left = cfg.getInt("max_left");
	this->cfg->max_right = cfg.getInt("max_right");
	this->cfg->stearing_linearity = cfg.getInt("stearing_linearity");

	//motor config reading
	this->cfg->controller_type = cfg.getInt("controller_type");
	this->cfg->min_speed = cfg.getInt("min_speed");
	this->cfg->potentiometer_linearity = cfg.getInt("potentiometer_linearity");

	this->cfg->front_light_on = cfg.getInt("front_light_on");
	this->cfg->high_light_on = cfg.getInt("high_light_on");
	this->cfg->parking_light_on = cfg.getInt("parking_light_on");
	this->cfg->turn_light_on = cfg.getInt("turn_light_on");

	this->cfg->stop_light_duration = cfg.getInt("stop_light_duration");
	this->cfg->back_light_timeout = cfg.getInt("back_light_timeout");
	this->cfg->back_light_pwm = cfg.getInt("back_light_pwm");

	this->cfg->beep_freq = cfg.getInt("beep_freq");
	this->cfg->beep_duration = cfg.getInt("beep_duration");
	this->cfg->beep_interval = cfg.getInt("beep_interval");

	this->cfg->drive_mode = cfg.getInt("drive_mode");

}

void SetupController::saveConfig()
{
	JsonString  out = JsonString();
	printConfig(&out);
	File cfgFile = SPIFFS.open("/config.json", "w");
	cfgFile.print(out.c_str());
	cfgFile.flush();
	cfgFile.close();
	if (setupController.reloadConfig != nullptr) setupController.reloadConfig();
}

void SetupController::printConfig(JsonString * out)
{
	out->beginObject();
	out->AddValue("ssid", cfg->ssid);
	out->AddValue("password", cfg->password);

	out->AddValue("center", String(cfg->center));
	out->AddValue("max_left", String(cfg->max_left));
	out->AddValue("max_right", String(cfg->max_right));
	out->AddValue("stearing_linearity", String(cfg->stearing_linearity));

	out->AddValue("controller_type", String(cfg->controller_type));
	out->AddValue("min_speed", String(cfg->min_speed));
	out->AddValue("potentiometer_linearity", String(cfg->potentiometer_linearity));

	out->AddValue("front_light_on", String(cfg->front_light_on));
	out->AddValue("high_light_on", String(cfg->high_light_on));
	out->AddValue("parking_light_on", String(cfg->parking_light_on));
	out->AddValue("turn_light_on", String(cfg->turn_light_on));
	out->AddValue("stop_light_duration", String(cfg->stop_light_duration));
	out->AddValue("back_light_timeout", String(cfg->back_light_timeout));
	out->AddValue("back_light_pwm", String(cfg->back_light_pwm));

	out->AddValue("beep_freq", String(cfg->beep_freq));
	out->AddValue("beep_duration", String(cfg->beep_duration));
	out->AddValue("beep_interval", String(cfg->beep_interval));

	out->AddValue("drive_mode", String(cfg->drive_mode));
	out->AddValue("debug", String(cfg->debug));
	out->endObject();
}

void SetupController::Setup_Get()
{
	JsonString ret = JsonString();
	setupController.printConfig(&ret);
	webServer.jsonOk(&ret);
}

void SetupController::Setup_Post()
{

	if (webServer.hasArg("ssid")) { setupController.cfg->ssid = webServer.arg("ssid"); }
	if (webServer.hasArg("password")) { setupController.cfg->password = webServer.arg("password"); }

	if (webServer.hasArg("center")) { setupController.cfg->center = webServer.arg("center").toInt(); }
	if (webServer.hasArg("max_left")) { setupController.cfg->max_left = webServer.arg("max_left").toInt(); }
	if (webServer.hasArg("max_right")) { setupController.cfg->max_right = webServer.arg("max_right").toInt(); }
	if (webServer.hasArg("stearing_linearity")) { setupController.cfg->stearing_linearity = webServer.arg("stearing_linearity").toInt(); }

	if (webServer.hasArg("controller_type")) { setupController.cfg->controller_type = webServer.arg("controller_type").toInt(); }
	if (webServer.hasArg("min_speed")) { setupController.cfg->min_speed = webServer.arg("min_speed").toInt(); }
	if (webServer.hasArg("potentiometer_linearity")) { setupController.cfg->potentiometer_linearity = webServer.arg("potentiometer_linearity").toInt(); }

	if (webServer.hasArg("front_light_on")) { setupController.cfg->front_light_on = webServer.arg("front_light_on").toInt(); }
	if (webServer.hasArg("high_light_on")) { setupController.cfg->high_light_on = webServer.arg("high_light_on").toInt(); }
	if (webServer.hasArg("parking_light_on")) { setupController.cfg->parking_light_on = webServer.arg("parking_light_on").toInt(); }
	if (webServer.hasArg("turn_light_on")) { setupController.cfg->turn_light_on = webServer.arg("turn_light_on").toInt(); }
	if (webServer.hasArg("stop_light_duration")) { setupController.cfg->stop_light_duration = webServer.arg("stop_light_duration").toInt(); }
	if (webServer.hasArg("back_light_timeout")) { setupController.cfg->back_light_timeout = webServer.arg("back_light_timeout").toInt(); }
	if (webServer.hasArg("back_light_pwm")) { setupController.cfg->back_light_pwm = webServer.arg("back_light_pwm").toInt(); }
	
	if (webServer.hasArg("beep_freq")) { setupController.cfg->beep_freq = webServer.arg("beep_freq").toInt(); }
	if (webServer.hasArg("beep_duration")) { setupController.cfg->beep_duration = webServer.arg("beep_duration").toInt(); }
	if (webServer.hasArg("beep_interval")) { setupController.cfg->beep_interval = webServer.arg("beep_interval").toInt(); }
	
	if (webServer.hasArg("drive_mode")) { setupController.cfg->drive_mode = webServer.arg("drive_mode").toInt(); }

	setupController.saveConfig();

	webServer.sendHeader("Location", String("http://") + WebUIController::ipToString(webServer.client().localIP()), true);
	webServer.send(302, "text/plain", "");   // Empty content inhibits Content-length header so we have to close the socket ourselves.
	webServer.client().stop(); // Stop is needed because we sent no content length
}


SetupController setupController;
