#ifndef WebServerFunctions_h
#define WebServerFunctions_h

#include <Arduino.h>
#include <ESPAsyncWebSrv.h>
#include "LedDriver.h"
#include <DNSServer.h>

extern AsyncWebServer server;
extern ledDriver led; 

void setupWebServer(); 
void handleLedControl(AsyncWebServerRequest* request);
void handleRoot(AsyncWebServerRequest* request);
void handleSignalStrength(AsyncWebServerRequest* request);
void handleSave(AsyncWebServerRequest* request);
void handleIdentify(AsyncWebServerRequest* request);
void handleUpdate(AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final);
void handleDiagnostic(AsyncWebServerRequest* request);
void handle8BitTest(AsyncWebServerRequest* request);
void handle16BitTest(AsyncWebServerRequest* request);
void handleBlankLEDSTest(AsyncWebServerRequest* request);
void handleReboot(AsyncWebServerRequest* request);
void handleSetDiag(AsyncWebServerRequest* request);
void handleRainbow(AsyncWebServerRequest* request);
void handleResetToDefault(AsyncWebServerRequest* request);
void handleFileUpload(AsyncWebServerRequest* request);
void handleGamma(AsyncWebServerRequest* request);

String getStoredString(int address);
int getStoredInt(int address);
void storeString(int address, const String& value);
void storeByte(int address, uint8_t value);
void storeFloat(int address, float value);
float getStoredFloat(int address);

void startAccessPoint(bool waitForClient);

#endif
