/*
 * Enables redirected HTTPS network connection using the ESP8266 built-in WiFi.
 *
 * Platform: ESP8266 using Arduino IDE
 * Documentation: http://www.coertvonk.com/technology/embedded/esp8266-clock-import-events-from-google-calendar-15809
 * Tested with: Arduino IDE 1.6.11, board package esp8266 2.3.0, Adafruit huzzah feather esp8266
 * Inspired by: Sujay Phadke's HTTPSRedirect
 *
 * MIT license, check the file LICENSE for more information
 * (c) Copyright 2016, Coert Vonk
 * All text above must be included in any redistribution
 */

#include <WiFiClientSecureRedirect.h>

//#define DEBUG
#ifdef DEBUG
  #define DPRINT(...)    Serial.print(__VA_ARGS__)
  #define DPRINTLN(...)  Serial.println(__VA_ARGS__)
#else
  #define DPRINT(...)     //now defines a blank line
  #define DPRINTLN(...)   //now defines a blank line
#endif

WiFiClientSecureRedirect::WiFiClientSecureRedirect(void) {}  // automatically calls base constructor
WiFiClientSecureRedirect::~WiFiClientSecureRedirect(void) {}  // automatically calls base deconstructor

int
WiFiClientSecureRedirect::connect(char const * const host,
	                              uint16_t const port)
{
	if (WiFiClientSecure::connected()) {
		return 1;
	}
	uint8_t tries = 5;
	do {
		DPRINT(".");
		if (WiFiClientSecure::connect(host, port)) {
			return 1;
		}
		delay(1000);
		tries--;
	} while (tries);
	return 0;
}

bool 
WiFiClientSecureRedirect::parseHeader(char * const host,
									  size_t const hostSize,
									  char * const path,
									  size_t const pathSize,
	                                  uint16_t * const port) 
{
	if (find("302 Moved Temporarily\r\n") && find("\nLocation: ") && find("://")) {

		size_t const hostLen = readBytesUntil('/', host, hostSize - 1);
		if (!hostLen) {
			return false;
		}
		host[hostLen] = '\0';

		size_t const pathLen = readBytesUntil('\n', path + 1, pathSize - 2);
		if (!pathLen) {
			return false;
		}
		path[0] = '/';
		path[pathLen] = '\0';
		*port = 443;  // https
		return true;
	}
	return false;
}

void 
WiFiClientSecureRedirect::writeRequest(char const * const path,
	                                   char const * const host)
{
	uint8_t HEAD[] = "GET ";  // 4 characters
	uint8_t MIDL[] = " HTTP/1.1\r\n"
		              "Host: ";  // 17 characters
	uint8_t TAIL[] = "\r\n"  // 45 characters
		             "User-Agent: ESP8266\r\n"
		             "Connection: close\r\n"
		             "\r\n";
	uint8_t const * const uPath = reinterpret_cast<uint8_t const * const>(path);
	uint8_t const * const uHost = reinterpret_cast<uint8_t const * const>(host);

	write(HEAD, sizeof(HEAD) - 1); write(uPath, strlen(path));
	write(MIDL, sizeof(MIDL) - 1); write(uHost, strlen(host));
	write(TAIL, sizeof(TAIL) - 1);
	flush();
}

bool 
WiFiClientSecureRedirect::request(char const * const dstPath,
	                              char const * const dstHost,
	                              uint32_t const timeout_ms,
								  char const * const dstFingerprint,
								  char const * const redirFingerprint)
{
	setTimeout(timeout_ms);
	DPRINT("Host reply ");
	if (!connected()) {
		DPRINTLN(" !con");
		return false;
	}
	if (dstFingerprint && !verify(dstFingerprint, dstHost)) {
		DPRINTLN(" !fp");
		return false;
	}
	writeRequest(dstPath, dstHost);

	char redirHost[30];
	char redirPath[300];
	uint16_t redirPort;

	if (!parseHeader(redirHost, sizeof(redirHost), redirPath, sizeof(redirPath), &redirPort)) {
		DPRINTLN("!parse");
		return false;
	}

	while (connected() && available()) {
		(void)read();  // not sure if we need to empty the Rx buffer before closing the socket
	}
	stop();

	DPRINT(" done\nRedir connect ");

	if (!connect(redirHost, redirPort)) {
		DPRINTLN(" !connect");
		return false;
	}
	setTimeout(timeout_ms);
	if(redirFingerprint && !verify(redirFingerprint, redirHost)) {
		DPRINTLN(" !fp");
		return false;
	}
  
	writeRequest(redirPath, redirHost);

	DPRINT(" done\nRedir reply ..");
	// skip header
	while (connected()) {
		String line = readStringUntil('\n');
		if (line == "\r") {
			break;
		}
	}
	DPRINTLN(" done");
	// connection remains open, caller should read all data and call stop() method
    return true;
}
