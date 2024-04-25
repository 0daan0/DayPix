#include <LedDriver.h>

LedDriver::LedDriver() {
  // Constructor logic, if needed
  Serial.print("Initialize" );
}
  void LedDriver::initialize(int nrLeds, int DmxAddr, int lPin, int dPin, int cPin) {
    latchPin = lPin;
    dataPin = dPin;
    clockPin = cPin;
    NumberOfLeds = nrLeds;
    DmxAddress = DmxAddr;
    pinMode(latchPin, OUTPUT);
    pinMode(dataPin, OUTPUT);
    pinMode(clockPin, OUTPUT);
    Serial.print("Initialize" + latchPin +dataPin+clockPin+NumberOfLeds+DmxAddress);
  }
  // Send data to the LED strip
  void LedDriver::sendData(uint8_t value, int dataPin, int clockPin) {
    for (int i = 7; i >= 0; i--) {
      digitalWrite(dataPin, (value >> i) & 0x01);
      digitalWrite(clockPin, HIGH);
      digitalWrite(clockPin, LOW);
      digitalWrite(clockPin, HIGH);
      digitalWrite(clockPin, LOW);
    }
  }

  // Write pixelbuffer data=DMX data, length=DMX data lenght, limit=number of leds, start=start addres (DMX addres), ofset=the amount of leds it needs to blank before writing data
  void LedDriver::writePixelBuffer(const uint8_t* data, const uint16_t length, const uint16_t nrOfleds, const uint16_t start, const uint16_t ledOfset) {
    uint16_t limit = nrOfleds * 3;
    // set limit times 3 since each led has 3 values RGB, also add start address to the limit since we shift that amount of addresses.
    uint16_t end = limit + start;

    if (ledOfset > 0) {
      digitalWrite(latchPin, LOW);
      for (int j = 0; j < ledOfset * 3; ++j) {
        sendData(0, dataPin, clockPin);
      }
      digitalWrite(latchPin, HIGH);
    }
    digitalWrite(latchPin, LOW);
    for (int i = start; i < std::min(length, end); i++) {
      sendData(data[i], dataPin, clockPin);
    }
    digitalWrite(latchPin, HIGH);
  }

  void LedDriver::writePixelBuffer_(const uint8_t* data, const uint16_t length, const uint16_t limit, const uint16_t start) {
    digitalWrite(latchPin, LOW);
    // set limit times 3 since each led has 3 values RGB, also add start address to the limit since we shift that amount of addresses.
    uint16_t end = limit * 3 + start;
    for (int i = start; i < length; i++) {
      if (i <= end) {
        sendData(data[i], dataPin, clockPin);
      } else {
        sendData(0, dataPin, clockPin);
      }
    }
    digitalWrite(latchPin, HIGH);
  }

  void LedDriver::blankLEDS(int nrToblank) {
    digitalWrite(latchPin, LOW);
    // blank number times 3 since each led has 3 values
    for (int i = 0; i < nrToblank * 3; ++i) {
      sendData(0, dataPin, clockPin);
    }
    digitalWrite(latchPin, HIGH);
  }

  void LedDriver::ledOn() {
    digitalWrite(latchPin, LOW);
    int nrBlank = (NumberOfLeds - 1) * 3;
    for (int i = 0; i < nrBlank; ++i) {
      sendData(0, dataPin, clockPin);
    }
    sendData(255, dataPin, clockPin);
    digitalWrite(latchPin, HIGH);
  }

  void LedDriver::rgbEffect(void* parameter) {
    int number = reinterpret_cast<int>(parameter);
    // test led strip
    const uint8_t singlePixelData[] = { 0, 125, 0 };
    if (number == 1) {
      for (int i = 0; i < NumberOfLeds; ++i) {
        writePixelBuffer(singlePixelData, sizeof(singlePixelData), NumberOfLeds, DmxAddress, i);
      }
      delay(5000);
      const uint8_t singlePixelDatan[] = { 0, 0, 0 };

      for (int i = 0; i < NumberOfLeds; ++i) {
        writePixelBuffer(singlePixelDatan, sizeof(singlePixelDatan), NumberOfLeds, DmxAddress, i);
      }
    }
    if (number == 2) {
      // Flash 3 times while scrolling red and blue
      for (int t = 0; t < 3; ++t) {
        // Color scroll effect
        for (int color = 0; color < 190; ++color) {
          // Set the RGB color based on the color variable
          const uint8_t singlePixelData[] = { color, 0, 255 - color };  // Example: R-G-B color

          // Display the color on each led on the RGB strip
          for (int i = 0; i < 1; ++i) {
            writePixelBuffer(singlePixelData, sizeof(singlePixelData), NumberOfLeds, DmxAddress, i);
            //sendData(singlePixelData[i], dataPin, clockPin);
          }
          delay(1);  // Adjust delay for speed
        }

        for (int color = 190; color > 0; --color) {
          // Set the RGB color based on the color variable
          const uint8_t singlePixelData[] = { color, 0, 0 + color };  // Example: R-G-B color

          // Display the color on each led on the RGB strip
          for (int i = 0; i < 1; ++i) {
            writePixelBuffer(singlePixelData, sizeof(singlePixelData), NumberOfLeds, DmxAddress, i);
            //sendData(singlePixelData[i], dataPin, clockPin);
          }
          delay(1);  // Adjust delay for speed
        }
        blankLEDS(NumberOfLeds);
      }
    }
    if (number == 4) {
      uint8_t rgb_color[NumberOfLeds];
      // Update the colors.
      byte time = millis() >> 2;
      for (uint16_t i = 0; i < NumberOfLeds * 3; i++) {
        byte x = time - 8 * i;
        rgb_color[i] = x, 255 - x, x;
      }
      writePixelBuffer(rgb_color, sizeof(rgb_color), NumberOfLeds, DmxAddress, 0);
      delay(1000);
    }
  }

  void LedDriver::ledAPmode(void* parameter) {

    // Flash 3 times while scrolling red and blue
    while (true) {
      for (int t = 0; t < 2; ++t) {
        // Color scroll effect
        for (int color = 0; color < 100; ++color) {
          // Set the RGB color based on the color variable
          const uint8_t singlePixelData[] = { color, 10, 255 - color };  // Example: R-G-B color

          // Display the color on each led on the RGB strip
          for (int i = 0; i < 1; ++i) {
            writePixelBuffer(singlePixelData, sizeof(singlePixelData), NumberOfLeds, DmxAddress, 0);
            //sendData(singlePixelData[i], dataPin, clockPin);
          }
          delay(2);  // Adjust delay for speed
        }
        const uint8_t singlePixelDatan[] = { 0, 0, 0 };
        for (int color = 100; color > 0; --color) {
          // Set the RGB color based on the color variable
          const uint8_t singlePixelData[] = { color, 0, 0 + color };  // Example: R-G-B color

          // Display the color on each led on the RGB strip
          for (int i = 0; i < 1; ++i) {
            writePixelBuffer(singlePixelData, sizeof(singlePixelData), NumberOfLeds, DmxAddress, 0);
            //sendData(singlePixelData[i], dataPin, clockPin);
          }
          delay(3);  // Adjust delay for speed
        }
        blankLEDS(NumberOfLeds);
      }
      delay(5000);
    }
  }
  void LedDriver::ledTest() {
    for (int i = 0; i < NumberOfLeds; i++) {
      digitalWrite(latchPin, LOW);
      sendData(0, dataPin, clockPin);
      sendData(0, dataPin, clockPin);
      sendData(175, dataPin, clockPin);
      digitalWrite(latchPin, HIGH);
      delay(10);
    }
  }
