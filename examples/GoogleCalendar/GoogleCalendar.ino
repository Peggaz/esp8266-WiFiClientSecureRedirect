/*
 * Asynchronously retrieves calendar events from Google Calendar using the WiFiClientSecureRedirect library
 * Update the values of ssid, passwd and dstPath before use.  REFER TO DOCUMENTATION (see below for URL)
 *
 * Platform: ESP8266 using Arduino IDE
 * Documentation: http://www.coertvonk.com/technology/embedded/esp8266-clock-import-events-from-google-calendar-15809
 * Tested with: Arduino IDE 1.6.11, board package esp8266 2.3.0, Adafruit huzzah feather esp8266
 *
 * MIT license, check the file LICENSE for more information
 * (c) Copyright 2016, Coert Vonk
 * All text above must be included in any redistribution
 */

#include <ESP8266WiFi.h>
#include <WiFiClientSecureRedirect.h>

// replace with your network credentials
char const * const ssid = "replace with your WiFi network name";   // ** UPDATE ME **
char const * const passwd = "replace with your WiFi network password";   // ** UPDATE ME **

// fetch events from Google Calendar
char const * const dstHost = "script.google.com";
char const * const dstPath = "/macros/s/google_random_path__replace_with_yours_see_documentation/exec";  // ** UPDATE ME **
int const dstPort = 443;
int32_t const timeout = 10000;

// On a Linux system with OpenSSL installed, get the SHA1 fingerprint for the destination and redirect hosts:
//   echo -n | openssl s_client -connect script.google.com:443 2>/dev/null | openssl x509  -noout -fingerprint | cut -f2 -d'='
//   echo -n | openssl s_client -connect script.googleusercontent.com:443 2>/dev/null | openssl x509  -noout -fingerprint | cut -f2 -d'='
char const * const dstFingerprint = "C7:4A:32:BC:A0:30:E6:A5:63:D1:8B:F4:2E:AC:19:89:81:20:96:BB";
char const * const redirFingerprint = "E6:88:19:5A:3B:53:09:43:DB:15:56:81:7C:43:30:6D:3E:9D:2F:DE";

#define DEBUG
#ifdef DEBUG
#define DPRINT(...)    Serial.print(__VA_ARGS__)
#define DPRINTLN(...)  Serial.println(__VA_ARGS__)
#else
#define DPRINT(...)     //now defines a blank line
#define DPRINTLN(...)   //now defines a blank line
#endif

void setup() {
    Serial.begin(115200);
	delay(500);

	Serial.print("\n\nWifi ");
    WiFi.begin(ssid, passwd);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println(" done");
}

WiFiClientSecureRedirect client;

typedef enum {
	CONNECT = 0,
	WAIT4CONNECT,
	WAIT4RESPONSE,
	COUNT
} state_t;

state_t state = CONNECT;

uint32_t eventTimeouts[COUNT] = {
	0,     // CONNECT
	7000,  // WAIT4CONNECT
	2000   // WAIT4RESPONSE
};

uint32_t beginWait;
uint32_t const timeout_ms = 5000;

uint8_t const  // returns 0 on success
connect()
{
	return !client.connect(dstHost, dstPort);
}

uint8_t const  // returns 0 on success
sendAlarmRequest()
{
	return client.request(dstPath, dstHost, timeout_ms, dstFingerprint, redirFingerprint);
}

uint8_t const  // returns 0 on success
receiveAlarmResponse()
{
	Serial.print("<RESPONSE>");
	while (client.available()) {
		String line = client.readStringUntil('\n');
		Serial.println(line);
	}
	Serial.print("</RESPONSE>");
	client.stop();
	return 0;
}


void loop() {
	client.tick();
#if 0
	static uint16_t count = 0;
	if (count++ % 100000 == 0) {
		DPRINT("<HEAP>"); DPRINT(ESP.getFreeHeap()); DPRINTLN("</HEAP>");
	}
#endif

	state_t prevState;
	do {
		prevState = state;

		uint32_t const timeout = eventTimeouts[state];
		if (timeout && millis() - beginWait >= timeout) {
			DPRINT(__func__); DPRINT(": timeout in state "); DPRINTLN(state);
			client.stop();
			state = CONNECT;
			beginWait = millis();
		}

		bool error = false;
		switch (state) {
			case CONNECT:
				DPRINT("Alarm sync .. ");
				if (!(error = connect())) {
					state = WAIT4CONNECT;
				}
				break;
			case WAIT4CONNECT:
				if (client.connected()) {
					if (!(error = sendAlarmRequest())) {
						state = WAIT4RESPONSE;
					}
				}
				break;
			case WAIT4RESPONSE:
				if (client.response()) {
					if (!(error = receiveAlarmResponse())) {
						DPRINTLN("done");
						state = CONNECT;
					}
				}
				break;
			case COUNT:
				break; // to please the compiler
		}
		if (error) {
			DPRINT(__func__); DPRINT(": error in state "); DPRINTLN(state);
			state = CONNECT;
			client.stop();
		}
		if (state != prevState) {
			beginWait = millis();
		}
		//DPRINT(__func__); DPRINT(prevState); DPRINT(">"); DPRINTLN(state);

	} while (state != prevState && eventTimeouts[state]);
}
