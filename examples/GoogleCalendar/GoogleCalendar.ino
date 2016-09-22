/*
 * Retrieves calendar events from Google Calendar using the ESP8266WiFiClientSecureRedirect library
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
#include <ESP8266WiFiClientSecureRedirect.h>

// replace with your network credentials
char const * const ssid = "replace with your WiFi network name";   // ** UPDATE ME **
char const * const passwd = "replace with your WiFi network password";   // ** UPDATE ME **

// fetch events from Google Calendar
char const * const dstHost = "script.google.com";
char const * const dstPath = "/macros/s/google_random_path__replace_with_yours_see_documentation/exec";  // ** UPDATE ME **
int const dstPort = 443;
int32_t const timeout = 5000;

// On a Linux system with OpenSSL installed, get the SHA1 fingerprint for the destination and redirect hosts:
//   echo -n | openssl s_client -connect script.google.com:443 2>/dev/null | openssl x509  -noout -fingerprint | cut -f2 -d'='
//   echo -n | openssl s_client -connect script.googleusercontent.com:443 2>/dev/null | openssl x509  -noout -fingerprint | cut -f2 -d'='
char const * const dstFingerprint = "C7:4A:32:BC:A0:30:E6:A5:63:D1:8B:F4:2E:AC:19:89:81:20:96:BB";
char const * const redirFingerprint = "E6:88:19:5A:3B:53:09:43:DB:15:56:81:7C:43:30:6D:3E:9D:2F:DE";

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

void loop() {
    static bool done = false;
	if (done) {
		return;
	} 
	else {
		WiFiClientSecureRedirect client;

		Serial.print("Free heap .. "); Serial.println(ESP.getFreeHeap());
		Serial.print("Host connect ");

		if (client.connect(dstHost, dstPort)) {
			Serial.println(" done");

			client.request(dstPath, dstHost, timeout, dstFingerprint, redirFingerprint);
			Serial.println("");

			// print body
			while (client.connected()) {
				String line = client.readStringUntil('\n');
				Serial.println(line);
			}
			client.stop();
			done = true;
		}
		else {
			Serial.println(" error ");
		}
	}
	Serial.print("Free heap .. "); Serial.println(ESP.getFreeHeap());
}
