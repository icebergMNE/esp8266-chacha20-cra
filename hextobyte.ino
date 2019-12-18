
#include <chacha20.h>
#include <ArduinoJson.hpp>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "FS.h"


struct chacha20_context ctx;
uint8_t testKey[32] = { 0x76, 0x42, 0x84, 0xB4, 0x87, 0x1F, 0x54, 0xAE,
				0x33, 0xBF, 0x79, 0x3C, 0xE2, 0x78, 0x5B, 0x4D,
				0xE7, 0x90, 0xF3, 0x8C, 0xB8, 0xF4, 0xA1, 0x56,
				0x87, 0x8B, 0x54, 0x06, 0xBE, 0x5A, 0x1B, 0x1C };
uint8_t testNonce[12] = { 
	0x76, 0x42, 0x84, 0xB4, 
	0x87, 0x1F, 0x54, 0xAE, 
	0x33, 0xBF, 0x79, 0x3C };
ESP8266WebServer server(80);

StaticJsonDocument<150> configDoc;

const char* ssid;
const char* pass;
const char* apppass;




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
	server.on("/auth", HTTP_POST, authHandler);
	server.begin();

	

	char hexstring[] = "5a52319fe4c9c766c93d56b0228e3bb7c8e4d5be3af1a4c76de9303c52c4832e3dbac98b1e22e08f5ef7c964a098c5c3";
	const int hexLength = (strlen(hexstring) / 2);

	chacha20_init_context(&ctx, testKey, testNonce, 1);
	byte *ok = (byte*)malloc(hexLength * sizeof(byte));

	hexCharacterStringToBytes(ok, hexstring);

	chacha20_xor(&ctx, ok, hexLength);
	Serial.println("idemo string");

		Serial.print((char*)ok);

}

// Add the main program code into the continuous loop() function
void loop()
{
	server.handleClient();
}


byte nibble(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';

	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;

	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;

	return 0;  // Not a valid hexadecimal character
}

void hexCharacterStringToBytes(byte *byteArray, const char *hexString)
{
	bool oddLength = strlen(hexString) & 1;

	byte currentByte = 0;
	byte byteIndex = 0;

	for (byte charIndex = 0; charIndex < strlen(hexString); charIndex++)
	{
		bool oddCharIndex = charIndex & 1;

		if (oddLength)
		{
			// If the length is odd
			if (oddCharIndex)
			{
				// odd characters go in high nibble
				currentByte = nibble(hexString[charIndex]) << 4;
			}
			else
			{
				// Even characters go into low nibble
				currentByte |= nibble(hexString[charIndex]);
				byteArray[byteIndex++] = currentByte;
				currentByte = 0;
			}
		}
		else
		{
			// If the length is even
			if (!oddCharIndex)
			{
				// Odd characters go into the high nibble
				currentByte = nibble(hexString[charIndex]) << 4;
			}
			else
			{
				// Odd characters go into low nibble
				currentByte |= nibble(hexString[charIndex]);
				byteArray[byteIndex++] = currentByte;
				currentByte = 0;
			}
		}
	}
}



void authHandler() {
	const char* data = server.arg("plain").c_str();
	Serial.println("Data HERE");
	Serial.println(data);
	Serial.println("Data ENDS");


	StaticJsonDocument<200> jsonBody;
	deserializeJson(jsonBody, data);
	serializeJsonPretty(jsonBody, Serial);
	const char* hexString = jsonBody["data"];

	const int hexLength = (strlen(hexString) / 2);

	chacha20_init_context(&ctx, testKey, testNonce, 1);
	byte *decrypted = (byte*)malloc(hexLength * sizeof(byte));

	hexCharacterStringToBytes(decrypted, hexString);

	chacha20_xor(&ctx, decrypted, hexLength);

	Serial.println((char*)decrypted);

	deserializeJson(jsonBody, (char*)decrypted);

	if (strcmp(jsonBody["password"], apppass) == 0) {
		server.sendHeader("Access-Control-Allow-Origin", "*");
		server.send(200, "text/plain", "ok");
	}
	else {
		server.sendHeader("Access-Control-Allow-Origin", "*");
		server.send(200, "text/plain", "forbidden");
	}
	
}