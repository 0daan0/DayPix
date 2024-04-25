#ifndef HwFunctions_h
#define HwFunctions_h

#include <Arduino.h>
#include <SPIFFS.h>
#include <FS.h>

void rstWdt(); 
void listFiles(fs::FS &fs, const char *dirname);
void setupHw();
void initializeEEPROM();

#endif