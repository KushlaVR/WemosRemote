#include "SetupController.h"



SetupController::SetupController()
{
	webServer.on("/api/setup", HTTPMethod::HTTP_GET, Setup_Get);
	webServer.on("/api/setup", HTTPMethod::HTTP_POST, Setup_Post);
}

SetupController::~SetupController()
{
}

void SetupController::saveConfig()
{
	JsonString  out = JsonString();
	printConfig(&out);
	File cfgFile = SPIFFS.open("/config.json", "w");
	cfgFile.print(out.c_str());
	cfgFile.flush();
	cfgFile.close();
}

void SetupController::printConfig(JsonString * out)
{
	out->beginObject();
	out->AddValue("ssid", cfg->ssid);
	out->AddValue("password", cfg->password);

	out->AddValue("center", String(cfg->center));
	out->AddValue("max_left", String(cfg->max_left));
	out->AddValue("max_right", String(cfg->max_right));

	out->AddValue("min_speed", String(cfg->min_speed));

	out->AddValue("front_light_on", String(cfg->front_light_on));
	out->AddValue("parking_light_on", String(cfg->parking_light_on));
	out->AddValue("stop_light_duration", String(cfg->stop_light_duration));
	out->AddValue("back_light_timeout", String(cfg->back_light_timeout));

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
	if (webServer.hasArg("min_speed")) { setupController.cfg->min_speed = webServer.arg("min_speed").toInt(); }
	if (webServer.hasArg("front_light_on")) { setupController.cfg->front_light_on = webServer.arg("front_light_on").toInt(); }
	if (webServer.hasArg("parking_light_on")) { setupController.cfg->parking_light_on = webServer.arg("parking_light_on").toInt(); }
	if (webServer.hasArg("stop_light_duration")) { setupController.cfg->stop_light_duration = webServer.arg("stop_light_duration").toInt(); }
	if (webServer.hasArg("back_light_timeout")) { setupController.cfg->back_light_timeout = webServer.arg("back_light_timeout").toInt(); }

	setupController.saveConfig();

	webServer.sendHeader("Location", String("http://") + WebUIController::ipToString(webServer.client().localIP()), true);
	webServer.send(302, "text/plain", "");   // Empty content inhibits Content-length header so we have to close the socket ourselves.
	webServer.client().stop(); // Stop is needed because we sent no content length
}


SetupController setupController;
