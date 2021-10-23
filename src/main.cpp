#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <sstream>

#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ESP8266mDNS.h>

ESP8266WebServer server(80);
int currentValue = 0; //should be dict

template <typename T>
std::string NumberToString ( T Number )
{
   std::ostringstream ss;
   ss << Number;
   return ss.str();
}

void turn_on(int pin, int max)
{
	currentValue = max;
	digitalWrite(pin, 1);
}

void turn_off(int pin, int min)
{
	currentValue = min;
	digitalWrite(pin, 0);
}
//not working
void get_output_value(int pin)
{
	String message = "";
	message += server.arg("plain");

	server.send(200, "text/plain", message);
	Serial.println(message);
	analogWrite(pin, server.arg("plain").toInt());	
}

void sendSensorValue(float (*dataGetter)(void))
{
	float val = dataGetter();
	server.send(200, "text/plain", NumberToString(val).c_str());
}

void set_value(uint8_t pin) {
	if (server.hasArg("plain") == false)
	{ 
		server.send(200, "text/plain", "value not received");
		return;
	}

	String message = "value received:\n";
	message += server.arg("plain");
	message += "\n";
	// currentValue = server.arg("plain");
	Serial.println(message);

	server.send(200, "text/plain", message);
	analogWrite(pin, server.arg("plain").toInt());
}
// void set_color()
void handleNotFound()
{
	String message = "File Not Found\n\n";
	message += "URI: ";
	message += server.uri();
	message += "\nMethod: ";
	message += (server.method() == HTTP_GET) ? "GET" : "POST";
	message += "\nArguments: ";
	message += server.args();
	message += "\n";

	for (uint8_t i = 0; i < server.args(); i++)
	{
		message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
	}

	server.send(404, "text/plain", message);
}

struct OutputConfig {
	int pin;
	int min;
	int max;
	bool is_binary;
	std::string name;
	std::string description;
};

struct SensorConfig {
	std::string name;
	std::string description;
	
	float (*dataGetter)(void);
};

class ApiConfigurator {
	ESP8266WebServer *server;
	std::string outputsCfgJSON;
	std::string sensorsCfgJSON;

	public:

	ApiConfigurator(ESP8266WebServer *srv) {
		server = srv;
	}

	void AddOutputs(std::vector<OutputConfig> oCfg) {
		int i = 0;
		outputsCfgJSON = "'outputs':[";
		for(const auto &cfg : oCfg){
			outputsCfgJSON += "{'name':'"+cfg.name + "','description':'" + cfg.description + "','isBinary':'" + NumberToString(cfg.is_binary)+ "','isColorPicker':'" + NumberToString(!cfg.is_binary)+ "','outputId':'"+NumberToString(i) + "','min':'"+NumberToString(cfg.min) + "','max':'"+NumberToString(cfg.max) +"'},";
			pinMode(cfg.pin, OUTPUT);
			std::string url = std::string("/output/") + NumberToString(i);
			server->on((url + "/turn-on").c_str(), [cfg]() { turn_on(cfg.pin, cfg.max); });
			server->on((url + "/turn-off").c_str(), [cfg]() { turn_off(cfg.pin, cfg.max); });
			// server->on((url + "/get-value").c_str(), [cfg]() { get_output_value(cfg.pin); });
			server->on((url + "/set-value/red/").c_str(), [cfg]() { set_value(D1); });
			server->on((url + "/set-value/green/").c_str(), [cfg]() { set_value(D2); });
			server->on((url + "/set-value/blue/").c_str(), [cfg]() { set_value(D0); });
			//d1, d1, d0

			i++;
		}
		outputsCfgJSON += "]";

	}

	void AddSensors(std::vector<SensorConfig> sCfg) {
		int i = 0;
		sensorsCfgJSON = "'inputs': [";
		for(const auto &cfg : sCfg){
			sensorsCfgJSON += "{'name':'" + cfg.name + "', 'description':'"+cfg.description+ "','inputId':'"+NumberToString(i) + "'},";
			std::string url = std::string("/sensor/") + NumberToString(i);
			server->on(url.c_str(), [cfg]() { sendSensorValue(cfg.dataGetter); });
			i++;
		}
		sensorsCfgJSON += "]";
	}

	void Build() {
		std::string registerString = "{" + outputsCfgJSON +","+ sensorsCfgJSON + "}";
		server->on("/register", [this, registerString]() { 	this->server->send(200, "text/plain", registerString.c_str());
});
		server->onNotFound(handleNotFound);
		server->begin();
	}
};

void handleRoot()
{
	char temp[400];
	int sec = millis() / 1000;
	int min = sec / 60;
	int hr = min / 60;

	snprintf(temp, 400,

			 "<html>\
  <head>\
    <meta http-equiv='refresh' content='5'/>\
    <title>ESP8266 Demo</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Hello from ESP8266!</h1>\
    <p>Uptime: %02d:%02d:%02d</p>\
    <img src=\"/test.svg\" />\
  </body>\
</html>",

			 hr, min % 60, sec % 60);
	server.send(200, "text/html", temp);
}

void drawGraph()
{
	String out = "";
	char temp[100];
	out += "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" width=\"400\" height=\"150\">\n";
	out += "<rect width=\"400\" height=\"150\" fill=\"rgb(250, 230, 210)\" stroke-width=\"1\" stroke=\"rgb(0, 0, 0)\" />\n";
	out += "<g stroke=\"black\">\n";
	int y = rand() % 130;
	for (int x = 10; x < 390; x += 10)
	{
		int y2 = rand() % 130;
		sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" stroke-width=\"1\" />\n", x, 140 - y, x + 10, 140 - y2);
		out += temp;
		y = y2;
	}
	out += "</g>\n</svg>\n";

	server.send(200, "image/svg+xml", out);
}

void turn_on()
{
	digitalWrite(D7, 1);
	drawGraph();
}

void turn_off()
{
	digitalWrite(D7, 0);
	drawGraph();
}

void handleBrightness()
{ //Handler for the body path

	if (server.hasArg("plain") == false)
	{ //Check if body received

		server.send(200, "text/plain", "Body not received");
		return;
	}

	String message = "Body received:\n";
	message += server.arg("plain");
	message += "\n";

	server.send(200, "text/plain", message);
	Serial.println(message);
	analogWrite(D7, server.arg("plain").toInt());
}

float getLightLevel() {
	// return lightMeter.readLightLevel();
  return 0.0;
}



ApiConfigurator api(&server);

void setup()
{
	delay(5000);
	Serial.begin(9600);
	// Wire.begin(D4, D3);
	// lightMeter.begin();
	

	WiFiManager wifiManager;
	wifiManager.autoConnect("NodeMCU-Arduino-PlatformIO");
	Serial.println("Connected!");
	if (MDNS.begin("esp8266"))
	{
		Serial.println("MDNS responder started");
	}

	api.AddOutputs({{D7,0, 255 ,true, "Turn on/off", ""}, {D7,0, 255 ,false, "Set color", ""}});
	api.AddSensors({});
	api.Build();
	// pinMode(D0, OUTPUT);
	// analogWrite(D0, 234);
	// pinMode(D1, OUTPUT);
	// analogWrite(D1, 46);
	// pinMode(D2, OUTPUT);
	// analogWrite(D2, 22);
	// d0 - niebieski
// d1 - czerwone
//d1, d1, d0
	// server.on("/", handleRoot);
	// server.on("/test.svg", drawGraph);
	// // server.on("/light-on", turn_on);
	// // server.on("/light-off", turn_off);
	// server.on("/brightness", handleBrightness);
	// server.on("/light-meter", handleReadLightMeter);
	// server.on("/inline", []() {
	// 	server.send(200, "text/plain", "this works as well");
	// });
	// server.onNotFound(handleNotFound);
	// server.begin();


	Serial.println("HTTP server started");
	Serial.println("local ip");
	Serial.println(WiFi.localIP());
	Serial.println(WiFi.gatewayIP());
	Serial.println(WiFi.subnetMask());
	Serial.println("Idle...");
}

void loop()
{
	server.handleClient();
}