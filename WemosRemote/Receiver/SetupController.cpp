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
		cfg.AddValue("ssid", "GAZ13");
		cfg.AddValue("password", "12345678");

		cfg.AddValue("ch1_min", "550");
		cfg.AddValue("ch1_center", "1000");
		cfg.AddValue("ch1_max", "2550");

		cfg.AddValue("ch2_min", "550");
		cfg.AddValue("ch2_center", "1000");
		cfg.AddValue("ch2_max", "2550");

		cfg.AddValue("ch3_min", "550");
		cfg.AddValue("ch3_max", "2550");

		cfg.AddValue("ch4_min", "550");
		cfg.AddValue("ch4_max", "2550");

		cfg.AddValue("turn_light_limit", "70");
		cfg.AddValue("reverce_limit", "10");
		
		cfg.AddValue("stop_light_duration", "2000");
		cfg.AddValue("back_light_timeout", "500");

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

	this->cfg->ch1_min = cfg.getInt("ch1_min");
	this->cfg->ch1_max = cfg.getInt("ch1_max");

	this->cfg->ch2_min = cfg.getInt("ch2_min");
	this->cfg->ch2_max = cfg.getInt("ch2_max");

	this->cfg->ch3_min = cfg.getInt("ch3_min");
	this->cfg->ch3_max = cfg.getInt("ch3_max");

	this->cfg->ch4_min = cfg.getInt("ch4_min");
	this->cfg->ch4_max = cfg.getInt("ch4_max");
	
	this->cfg->turn_light_limit = cfg.getInt("turn_light_limit");
	this->cfg->reverce_limit = cfg.getInt("reverce_limit");

	this->cfg->stop_light_duration = cfg.getInt("stop_light_duration");
	this->cfg->back_light_timeout = cfg.getInt("back_light_timeout");

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


	out->AddValue("ch1_min", String(cfg->ch1_min));
	out->AddValue("ch1_max", String(cfg->ch1_max));

	out->AddValue("ch2_min", String(cfg->ch2_min));
	out->AddValue("ch2_max", String(cfg->ch2_max));

	out->AddValue("ch3_min", String(cfg->ch3_min));
	out->AddValue("ch3_max", String(cfg->ch3_max));

	out->AddValue("ch4_min", String(cfg->ch4_min));
	out->AddValue("ch4_max", String(cfg->ch4_max));

	out->AddValue("turn_light_limit", String(cfg->turn_light_limit));
	out->AddValue("reverce_limit", String(cfg->reverce_limit));
	
	out->AddValue("stop_light_duration", String(cfg->stop_light_duration));
	out->AddValue("back_light_timeout", String(cfg->back_light_timeout));

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
	
	if (webServer.hasArg("ch1_min")) { setupController.cfg->ch1_min = webServer.arg("ch1_min").toInt(); }
	if (webServer.hasArg("ch1_max")) { setupController.cfg->ch1_max = webServer.arg("ch1_max").toInt(); }

	if (webServer.hasArg("ch2_min")) { setupController.cfg->ch2_min = webServer.arg("ch2_min").toInt(); }
	if (webServer.hasArg("ch2_max")) { setupController.cfg->ch2_max = webServer.arg("ch2_max").toInt(); }

	if (webServer.hasArg("ch3_min")) { setupController.cfg->ch3_min = webServer.arg("ch3_min").toInt(); }
	if (webServer.hasArg("ch3_max")) { setupController.cfg->ch3_max = webServer.arg("ch3_max").toInt(); }

	if (webServer.hasArg("ch4_min")) { setupController.cfg->ch4_min = webServer.arg("ch4_min").toInt(); }
	if (webServer.hasArg("ch4_max")) { setupController.cfg->ch4_max = webServer.arg("ch4_max").toInt(); }
	
	if (webServer.hasArg("turn_light_limit")) { setupController.cfg->turn_light_limit = webServer.arg("turn_light_limit").toInt(); }
	if (webServer.hasArg("reverce_limit")) { setupController.cfg->reverce_limit = webServer.arg("reverce_limit").toInt(); }

	if (webServer.hasArg("stop_light_duration")) { setupController.cfg->stop_light_duration = webServer.arg("stop_light_duration").toInt(); }
	if (webServer.hasArg("back_light_timeout")) { setupController.cfg->back_light_timeout = webServer.arg("back_light_timeout").toInt(); }

	setupController.saveConfig();

	webServer.sendHeader("Location", String("http://") + WebUIController::ipToString(webServer.client().localIP()), true);
	webServer.send(302, "text/plain", "");   // Empty content inhibits Content-length header so we have to close the socket ourselves.
	webServer.client().stop(); // Stop is needed because we sent no content length
}


SetupController setupController;
