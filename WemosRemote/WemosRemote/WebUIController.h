#pragma once

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include "Json.h"
#include "Console.h"

class WebUIController :public ESP8266WebServer
{
public:
	WebUIController();
	~WebUIController();

	String apName = "config";

	void setup();
	void loop();
	void ssdp(const char* deviceName);
	char * getContentType(String filename);
	void sendFile(File file, char* contenttype, bool addGzHeader);
	/** Is this an IP? */
	static boolean isIp(String str);

	/** IP to String? */
	static String ipToString(IPAddress ip);

	static boolean captivePortal();
	static void handleRoot();
	static void handleNotFound();
	
	
	
	static bool handleFileRead(String path, bool html = true);
	static String getMinimizedPath(String path);
	static bool replaceMin(String ext, String* path);

	static void jsonOk(JsonString * json);
	static void Ok();
	static void Ok(String name, String value);
};

extern WebUIController webServer;
