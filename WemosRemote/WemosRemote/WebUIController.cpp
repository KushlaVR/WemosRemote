#include "WebUIController.h"



WebUIController::WebUIController()
{
}


WebUIController::~WebUIController()
{

}

void WebUIController::setup()
{
	/* Setup web pages: root, wifi config pages, SO captive portal detectors and not found. */
	on("/", handleRoot);
	on("/generate_204", handleRoot);  //Android captive portal. Maybe not needed. Might be handled by notFound handler.
	on("/fwlink", handleRoot);  //Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
	onNotFound(handleNotFound);

	const char * headerkeys[] = { "User-Agent","Cookie" };
	size_t headerkeyssize = sizeof(headerkeys) / sizeof(char*);
	//ask server to track these headers
	collectHeaders(headerkeys, headerkeyssize);

	begin(); // Web server start
	Serial.println("HTTP server started");
}

void WebUIController::loop()
{
	handleClient();
}


char * WebUIController::getContentType(String filename)
{
	if (hasArg("download")) return "application/octet-stream";
	else if (filename.endsWith(".htm")) return "text/html";
	else if (filename.endsWith(".html")) return "text/html";
	else if (filename.endsWith(".css")) return "text/css";
	else if (filename.endsWith(".js")) return "application/javascript";
	else if (filename.endsWith(".png")) return "image/png";
	else if (filename.endsWith(".gif")) return "image/gif";
	else if (filename.endsWith(".jpg")) return "image/jpeg";
	else if (filename.endsWith(".ico")) return "image/x-icon";
	else if (filename.endsWith(".xml")) return "text/xml";
	else if (filename.endsWith(".svg")) return "image/svg+xml";
	else if (filename.endsWith(".pdf")) return "application/x-pdf";
	else if (filename.endsWith(".zip")) return "application/x-zip";
	else if (filename.endsWith(".gz")) return "application/x-gzip";
	return "text/plain";
}

void WebUIController::sendFile(File file, char * contenttype, bool addGzHeader)
{
	String header;
	size_t size_to_send = file.size();
	Serial.println("file=" + String(file.name()) + " filesize=" + String(size_to_send));
	if (addGzHeader) {
		sendHeader("Content-Encoding", "gzip"); // console.print("Header1 ");
		//sendHeader("Cache-Control", "max-age=31536000", true);
	}
	sendHeader("Cache-Control", "max-age=31536000", true);
	_prepareHeader(header, 200, contenttype, size_to_send);
	sendContent(header);
	//	console.print("Header2 ");

	const size_t unit_size = 1024;
	char dataDownloadBuf[unit_size];
	while (size_to_send) {
		size_t will_send = (size_to_send < unit_size) ? size_to_send : unit_size;
		//	console.print("Read ");
		file.readBytes(&dataDownloadBuf[0], will_send);
		//	console.print("Send ");
		size_t sent = _currentClient.write(&dataDownloadBuf[0], will_send);
		//	console.println("Next ");
		if (sent == 0) {
			break;
		}
		size_to_send -= sent;
	}
}

boolean WebUIController::isIp(String str)
{
	for (size_t i = 0; i < str.length(); i++) {
		int c = str.charAt(i);
		if (c != '.' && (c < '0' || c > '9')) {
			return false;
		}
	}
	return true;
}

String WebUIController::ipToString(IPAddress ip)
{
	String res = "";
	for (int i = 0; i < 3; i++) {
		res += String((ip >> (8 * i)) & 0xFF) + ".";
	}
	res += String(((ip >> 8 * 3)) & 0xFF);
	return res;
}

boolean WebUIController::captivePortal()
{
	if (!WebUIController::isIp(webServer.hostHeader()) && webServer.hostHeader() != (webServer.apName + ".local")) {
		Serial.println("Request redirected to captive portal");
		webServer.sendHeader("Location", String("http://") + WebUIController::ipToString(webServer.client().localIP()), true);
		webServer.send(302, "text/plain", "");   // Empty content inhibits Content-length header so we have to close the socket ourselves.
		webServer.client().stop(); // Stop is needed because we sent no content length
		return true;
	}
	return false;
}

void WebUIController::handleRoot()
{
	if (captivePortal())return;
	handleFileRead("/index.html");
}

void WebUIController::handleNotFound()
{
	Serial.println(webServer.uri());
	if (captivePortal()) { // If caprive portal redirect instead of displaying the error page.
		return;
	}
	if (handleFileRead(webServer.uri())) {
		return;
	}
	String message = "File Not Found\n\n";
	message += "URI: ";
	message += webServer.uri();
	message += "\nMethod: ";
	message += (webServer.method() == HTTP_GET) ? "GET" : "POST";
	message += "\nArguments: ";
	message += webServer.args();
	message += "\n";

	for (uint8_t i = 0; i < webServer.args(); i++) {
		message += " " + webServer.argName(i) + ": " + webServer.arg(i) + "\n";
	}
	webServer.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	webServer.sendHeader("Pragma", "no-cache");
	webServer.sendHeader("Expires", "-1");
	webServer.send(404, "text/plain", message);
}

bool WebUIController::handleFileRead(String path, bool html)
{
	if (path.endsWith("/")) path += "index.html";
	if (path.equals("/favicon.ico")) path = "icon.svg";
	if (html) path = "/html" + path;
	char* contentType = webServer.getContentType(path);
	String minimized = webServer.getMinimizedPath(path);
	if (SPIFFS.exists(minimized)) path = minimized;
	Serial.println("path=" + path);
	if (SPIFFS.exists(path)) {
		File file = SPIFFS.open(path, "r");
		webServer.sendFile(file, contentType, false);
		file.close();
		return true;
	}
	Serial.println("Not found!!! " + path);
	return false;
}

String WebUIController::getMinimizedPath(String path)
{
	String ret = path;
	replaceMin("html", &ret) || replaceMin("js", &ret) || replaceMin("css", &ret) || replaceMin("htm", &ret) || replaceMin("svg", &ret);
	return ret;
}

bool WebUIController::replaceMin(String ext, String * path)
{
	bool ret = (path->endsWith(ext));
	if (ret) path->replace("." + ext, ".min." + ext);
	return ret;
}

void WebUIController::jsonOk(JsonString * json)
{
	webServer.send(200, "application/json", *json);
}

void WebUIController::Ok()
{
	webServer.Ok("", "");
}

void WebUIController::Ok(String name, String value)
{
	webServer.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	webServer.sendHeader("Pragma", "no-cache");
	webServer.sendHeader("Expires", "-1");
	webServer.setContentLength(CONTENT_LENGTH_UNKNOWN);

	JsonString ret = "";
	ret.beginObject();
	//ret.AddValue("systime", Utils::FormatTime(webServer.timeZone->toLocal(now())));
	ret.AddValue("uptime", String(millis()));
	ret.AddValue("status", "ok");
	if (name.length() > 0) {
		ret.AddValue(name, value);
	}
	ret.endObject();

	webServer.send(200, "application/json", ret);
}


WebUIController webServer;