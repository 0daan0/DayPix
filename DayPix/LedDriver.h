#include "pins_arduino.h"
#ifndef LED_DRIVER_H
#define LED_DRIVER_H

#include <Arduino.h>
// Define macros for different hardware platforms
#define ESP32C3
//#define ESP_ETH01

class ledDriver {
public:
// Constructor

  ledDriver();
  
  void initialize(int nrLeds, int DmxAddr);
  void sendData(uint8_t value, int dataPin, int clockPin);
  void sendDataFast(uint8_t value, int dataPin, int clockPin);
  void sendData16Bit(uint16_t value, int dataPin, int clockPin);
  void writePixelBuffer(const uint8_t* data, const uint16_t length, const uint16_t nrOfleds, const uint16_t start, const uint16_t ledOfset);
  void writePixelBufferFull(const uint8_t* data, const uint16_t length, const uint16_t nrOfleds, const uint16_t start, const uint16_t ledOfset);
  void writePixelBufferPort2(const uint8_t* data, const uint16_t length, const uint16_t nrOfleds, const uint16_t start, const uint16_t ledOfset);
  void writePixelBuffer_(const uint8_t* data, const uint16_t length, const uint16_t limit, const uint16_t start);
  void showBuffer();
  void showBufferP2();
  void blankLEDS(int nrToblank);
  void ledOn();
  void blink(int nrs);
  void _8bTest();
  void _16bTest();
  void rgbEffect(void* parameter);
  static void cFlash();
  void ledTest();

private:
#ifdef ESP32C3
    // Pins for ESP32C3
    int latchPin = D0;
    int dataPin = D1;
    int clockPin = D2;

    int latchPin2 = D3;
    int dataPin2 = D4;
    int clockPin2 = D5;

#elif defined(ESP_ETH01)
    // Pins for ESP_ETH01
    int latchPin = IO15;
    int dataPin = IO14;
    int clockPin = IO12;

    int latchPin2 = IO33;
    int dataPin2 = IO4;
    int clockPin2 = IO2;

    // Add pins for ESP_ETH01 if needed
#endif
  int NumberOfLeds;
  int DmxAddress;
};

#endif  // LED_DRIVER_H
