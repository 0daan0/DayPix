// Config.h

#ifndef Config_h
#define Config_h

#include <Arduino.h>

// Device variables
extern TaskHandle_t dnsTaskHandle;
extern uint8_t universe9;
extern uint8_t universe8;
extern uint8_t universe7;
extern uint8_t universe6;
extern uint8_t universe5;
extern uint8_t universe4;
extern uint8_t universe3;
extern uint8_t universe2;
extern uint8_t universe1;  // Declaration
extern String recvUniverse;
extern int NrOfLeds;
extern int DmxAddr;
extern int storedUniverseStart;
extern int storedUniverseEnd;
extern bool b_APmode;
extern int b_16Bit;
extern int b_failover;
extern int b_silent;
extern bool identify;
extern bool diag;
extern String DEV_NAME;
extern String H_PRFX;
extern String HOST_NAME;
extern String FIRMWARE_VERSION;
extern const char* www_username;
extern const char* www_password;
extern float GAMMA_CORRECTION; 

// Add other global variables here
// Define the marker value for EEPROM initialization
const byte INIT_MARKER = 0xAB;

// Address where the initialization marker will be stored
const int INIT_MARKER_ADDR = 0;

const int EEPROM_SIZE = 512;
const int SSID_EEPROM_ADDR = 1;
const int PASS_EEPROM_ADDR = SSID_EEPROM_ADDR + 128;
const int UNIVERSE_EEPROM_ADDR = PASS_EEPROM_ADDR + 20;
const int UNIVERSE_START_EEPROM_ADDR = UNIVERSE_EEPROM_ADDR + 20;
const int UNIVERSE_END_EEPROM_ADDR = UNIVERSE_START_EEPROM_ADDR + 20;
const int NRLEDS_EEPROM_ADDR = UNIVERSE_END_EEPROM_ADDR + 3;
const int DMX_ADDR_EEPROM_ADDR = NRLEDS_EEPROM_ADDR + 10;
const int B_16BIT_EEPROM_ADDR = DMX_ADDR_EEPROM_ADDR + 2;
const int DEV_NAME_EEPROM_ADDR = B_16BIT_EEPROM_ADDR + 64;
const int WIFI_ATTEMPT = 13;
const int B_FAILOVER_EEPROM_ADDR = WIFI_ATTEMPT + 2;
const int B_SILENT_EEPROM_ADDR = B_FAILOVER_EEPROM_ADDR + 2;




// Function to set DEV_NAME dynamically

#endif
