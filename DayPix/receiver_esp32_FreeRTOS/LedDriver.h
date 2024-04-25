#ifndef LED_DRIVER_H
#define LED_DRIVER_H

#include <Arduino.h>

class LedDriver {
public:
// Constructor
  LedDriver();
  
  void initialize(int nrLeds, int DmxAddr, int lPin, int dPin, int cPin);
  void sendData(uint8_t value, int dataPin, int clockPin);
  void writePixelBuffer(const uint8_t* data, const uint16_t length, const uint16_t nrOfleds, const uint16_t start, const uint16_t ledOfset);
  void writePixelBuffer_(const uint8_t* data, const uint16_t length, const uint16_t limit, const uint16_t start);
  void blankLEDS(int nrToblank);
  void ledOn();
  void rgbEffect(void* parameter);
  void ledAPmode(void* parameter);
  void ledTest();

private:
  int latchPin;
  int dataPin;
  int clockPin;
  int NumberOfLeds;
  int DmxAddress;
};

#endif  // LED_DRIVER_H
