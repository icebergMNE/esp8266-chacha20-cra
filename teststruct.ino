// Define User Types below here or use a .h file
//
#include <ArduinoJson.hpp>
#include <ArduinoJson.h>
#include <Hash.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "FS.h"

// Define Function Prototypes that use User Types below here or use a .h file
//


// Define Functions below here or use other .ino or cpp files
//
#define codesLength 5
ESP8266WebServer server(80);

StaticJsonDocument<150> configDoc;
const char* ssid;
const char* pass;
const char* apppass;



struct code_t
{
	int code;
	long codeTime;
	bool used;
};

code_t codes[codesLength];

// The setup() function runs once each time the micro-controller starts
void setup()
{

	Serial.begin(115200);

	FSConfig cfg;
	cfg.setAutoFormat(false);
	SPIFFS.setConfig(cfg);
	SPIFFS.begin();

	File configFile = SPIFFS.open("/db.json", "r+");

	if (!configFile) {
		Serial.println("Failed to open config file");
	}

	DeserializationError error = deserializeJson(configDoc, configFile);
	if (error)
		Serial.println(F("Failed to read file"));

	serializeJson(configDoc, Serial);
	ssid = configDoc["ssid"];
	pass = configDoc["pass"];
	apppass = configDoc["apppass"];

	WiFi.begin(ssid, pass);

	Serial.print("Connecting");
	while (WiFi.status() != WL_CONNECTED)
	{
		delay(500);
		Serial.print(".");
	}
	Serial.println();

	Serial.print("Connected, IP address: ");
	Serial.println(WiFi.localIP());

	server.serveStatic("/", SPIFFS, "/index.html");
	server.on("/challenge", challengeRequest);
	server.on("/auth", HTTP_POST, checkToken);
	server.on("/test", HTTP_POST, testJSON);
	server.begin();
}

// Add the main program code into the continuous loop() function
void loop()
{
	server.handleClient();
}


void printAllChallanges() {
	int i;
	for (i = 0; i < codesLength; i++) {
		Serial.println(i);
		Serial.println(codes[i].code);
		Serial.println(codes[i].codeTime);
		Serial.println(codes[i].used);
		Serial.println("______");
	}
}

void challengeRequest() {
	server.sendHeader("Access-Control-Allow-Origin", "*");
	server.send(200, "text/plain", (String)addChallenge());
}


int addChallenge() {
	int randomn = random(10000, 100000);

	//String challenge = sha1(apppass + (String)randomn);
	Serial.println("random number generateed is");
	Serial.println(randomn);

	int i;
	for (i = 0; i < codesLength; i++) {
		if (codes[i].used == true) {
			Serial.println("prvi if");

			codes[i].code = randomn;
			codes[i].codeTime = millis();
			codes[i].used = false;
			break;
		}
		else if (millis() - codes[i].codeTime > 5000) {
			Serial.println("drugi if");

			codes[i].code = randomn;
			codes[i].codeTime = millis();
			codes[i].used = false;
			break;

		}
		else {
			Serial.println("wtf ne radi");
		}

	}
	printAllChallanges();
	return randomn;
}

void checkToken() {
	String token = server.arg("token");
	//Serial.println("TOKEN HERE");
	//Serial.println(token);
	//Serial.println("TOKEN ENDS");

	if (tokenExist(server.arg("token").c_str())) {
		//printAllChallanges();
		server.sendHeader("Access-Control-Allow-Origin", "*");
		server.send(200, "text/plain", "ok");
	}
	else {
		//printAllChallanges();
		server.sendHeader("Access-Control-Allow-Origin", "*");
		server.send(200, "text/plain", "forbidden");
	}


}

void testJSON() {
	StaticJsonDocument<200> body;
	deserializeJson(body, server.arg("plain"));

	server.send(200, "text/plain", body["test"]);
}
bool tokenExist(const char* hashI) {
	int i;
	int comparison;
	String hash;
	Serial.println(hashI);

	for (i = 0; i < codesLength; i++) {
		//strcmp
		//strcmp
		//Serial.println(sha1(apppass + (String)codes[i].code).c_str());
		Serial.println("comparison");
		Serial.println(strcmp(sha1(apppass + (String)codes[i].code).c_str(), hashI));

			if (strcmp(sha1(apppass + (String)codes[i].code).c_str(), hashI) == 0) {
				codes[i].used = true;
				return true;
			}
	}
	return false;
}
