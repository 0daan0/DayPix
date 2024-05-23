// Config.cpp

#include "Config.h"
TaskHandle_t dnsTaskHandle = NULL;
uint8_t universe9 = 8;
uint8_t universe8 = 7;
uint8_t universe7 = 6;
uint8_t universe6 = 5;
uint8_t universe5 = 4;
uint8_t universe4 = 3;
uint8_t universe3 = 2;
uint8_t universe2 = 1;
uint8_t universe1 = 0;  // Initialization
String recvUniverse = " ";
int NrOfLeds = 13;
int DmxAddr = 0;
bool b_APmode = false;
int b_16Bit = 0;
int b_failover = 0;
int b_silent = 0;
int b_reverseArray = 0; 
int b_dhcp = 1;
bool identify = false;
bool diag = false;
String IP_ADDR = "192.168.1.123";
String IP_SUBNET = "255.255.255.0";
String IP_GATEWAY = "192.168.1.1";
String IP_DNS = "192.168.1.1";
// hostname prefix 
String H_PRFX = "DayPix";
String DEV_NAME = "";
String HOST_NAME;
const char* www_username = "admin";
const char* www_password = "admin";
String FIRMWARE_VERSION = "1.2.0";
float GAMMA_CORRECTION = 1.8;
const char* APpass = "setupdaypix";


void setDiag(bool value)
{
  diag = value;
};
void setSilent(int value)
{
  b_silent = value;
};
void setReverse(int value)
{
  b_reverseArray = value;
};
void setFailover(int value)
{
  b_failover = value;
};
