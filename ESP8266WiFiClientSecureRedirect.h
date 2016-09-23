#pragma once
#include <WiFiClientSecure.h>

class WiFiClientSecureRedirect : public WiFiClientSecure {
  private:
	void writeRequest(char const * const path, char const * const host);
	bool parseHeader(char * const host, size_t const hostLen, char * const path, size_t const pathLen, uint16_t * const port);
  public:
    WiFiClientSecureRedirect(void);
    ~WiFiClientSecureRedirect(void);
    bool request(char const * const dstPath, char const * const dstHost, uint32_t const timeout_ms, char const * const dstFingerprint, char const * const redirFingerprint);
	int connect(char const * const host, uint16_t const port);
};

