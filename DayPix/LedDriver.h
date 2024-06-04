#include "pins_arduino.h"
#ifndef LED_DRIVER_H
#define LED_DRIVER_H

#include <Arduino.h>
// Define macros for different hardware platforms
//#define XIAO_ESP32C3
//#define XIAO_ESP32S3
#define ESP_ETH01


class ledDriver {
public:
// Constructor

  ledDriver();
  
  void initialize(int nrLeds, int DmxAddr);
  void reverseArray(uint8_t* array, const uint16_t size);
  void sendData(uint8_t value, int dataPin, int clockPin);
  void sendDataFast(uint8_t value, int dataPin, int clockPin);
  void sendData16Bit(uint16_t value, int dataPin, int clockPin);
  void writePixelBuffer(const uint8_t* data, const uint16_t length, const uint16_t nrOfleds, const uint16_t start, const uint16_t ledOfset);
  void writePixelBufferFull(const uint8_t* data, const uint16_t length, const uint16_t nrOfleds, const uint16_t start, const uint16_t ledOfset);
  void writePixelBufferPort2(const uint8_t* data, const uint16_t length, const uint16_t nrOfleds, const uint16_t start, const uint16_t ledOfset);
  void writePixelBuffer_(const uint8_t* data, const uint16_t length, const uint16_t limit, const uint16_t start);
  void setLEDColor(int r, int g, int b, int nrLeds);
  void setLEDColor(int r, int g, int b);
  void setLEDColor16bit(int cR,int fR, int cG,int fG, int cB,int fB);
  void setLEDColor16bit(int r ,int g, int b);
  void showBuffer();
  void showBufferP2();
  void blankLEDS(int nrToblank);
  void ledOn();
  void blink(int nrs, int delayTime);
  void _8bTest();
  void _16bTest();
  void rgbEffect(void* parameter);
  static void cFlash();
  void ledTest();

#ifdef XIAO_ESP32S3
    // Pins for ESP32C3
    int latchPin = A0;
    int dataPin = A1;
    int clockPin = A2;

    int latchPin2 = A3;
    int dataPin2 = A4;
    int clockPin2 = A5;

    bool ethCap = false;
    const char* hwVersion = "DP1P.WL.S3";
#elif defined(XIAO_ESP32C3)
    // Pins for ESP32C3
    int latchPin = D0;
    int dataPin = D1;
    int clockPin = D2;

    int latchPin2 = D3;
    int dataPin2 = D4;
    int clockPin2 = D5;

    bool ethCap = false;
    const char* hwVersion = "DP1P.WL.C3";
#elif defined(ESP_ETH01)
    // Pins for ESP_ETH01
    int latchPin = IO15;
    int dataPin = IO14;
    int clockPin = IO12;

    int latchPin2 = IO33;
    int dataPin2 = IO4;
    int clockPin2 = IO2;
    #define ETH_CAP
    bool ethCap = true;
    const char* hwVersion = "DP2P.WL.ETH.LX6";
    #define RST_BTN
    int reset_Button = IO39;
    // Add pins for ESP_ETH01 if needed
#endif
  int NumberOfLeds;
  int DmxAddress;
};

#endif  // LED_DRIVER_H
