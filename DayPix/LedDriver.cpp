#include "LedDriver.h"
#include "Config.h"
#include <iostream>
#include "HwFunctions.h"

ledDriver::ledDriver() {
  // Constructor logic, if needed
  Serial.print("constructor" );
}
  void ledDriver::initialize(int nrLeds, int DmxAddr) {
    Serial.print("start Initialize" );

    NumberOfLeds = nrLeds;
    DmxAddress = DmxAddr;
    pinMode(latchPin, OUTPUT);
    pinMode(dataPin, OUTPUT);
    pinMode(clockPin, OUTPUT);

    pinMode(latchPin2, OUTPUT);
    pinMode(dataPin2, OUTPUT);
    pinMode(clockPin2, OUTPUT);

    blankLEDS(179);
    Serial.print("finish Initialize" + latchPin);
  }
 // Send data to the LED strip
  void ledDriver::sendData(uint8_t value, int dataPin, int clockPin) {
    float normalizedValue = value / 255.0;
    float correctedValue = pow(normalizedValue, GAMMA_CORRECTION) * 65535;
    int bit = static_cast<int>(correctedValue);
       
    for (int i = 15; i >= 0; i--) {
      digitalWrite(dataPin, (bit >> i) & 0x01);
      digitalWrite(clockPin, HIGH);
      digitalWrite(clockPin, LOW);
     }
    }

  void ledDriver::sendDataFast(uint8_t value, int dataPin, int clockPin) {

    for (int i = 7; i >= 0; i--) {
      digitalWrite(dataPin, (value >> i) & 0x01);
      digitalWrite(clockPin, HIGH);
      digitalWrite(clockPin, LOW);
      digitalWrite(clockPin, HIGH);
      digitalWrite(clockPin, LOW);
     }
    }


  void ledDriver::sendData16Bit(uint16_t value, int dataPin, int clockPin) {
    
    float normalizedValue = value / 65535.0; // Assuming value is a 16-bit value now
    float correctedValue = pow(normalizedValue, 2.5) * 65535.0;
    int bit = static_cast<int>(correctedValue);
       
    for (int i = 15; i >= 0; i--) {
      digitalWrite(dataPin, (bit >> i) & 0x01);
      digitalWrite(clockPin, HIGH);
      digitalWrite(clockPin, LOW);
     }
    }
  
void ledDriver::_8bTest(){
  for (int j = 0 ; j <256; j++){
    digitalWrite(latchPin, LOW);
    //feed watchdog 
    rstWdt();
    delay(10);
    sendData(j, dataPin, clockPin);
    digitalWrite(latchPin, HIGH);
  }
  for (int j = 255; j >= 0; j--) {
    digitalWrite(latchPin, LOW);
    // Feed watchdog 
    rstWdt();
    delay(10);
    sendData(j, dataPin, clockPin);
    digitalWrite(latchPin, HIGH);
  }
}
void ledDriver::_16bTest(){
  for (int j = 0 ; j <65535 ; j++){
    digitalWrite(latchPin, LOW);
     //feed watchdog 
    rstWdt();
    delay(0.1);
    sendData16Bit(j, dataPin, clockPin);
    digitalWrite(latchPin, HIGH); 
  }
  for (int j = 65535 ; j >=0 ; j--){
    digitalWrite(latchPin, LOW);
      //feed watchdog 
    rstWdt();
    delay(0.1);
    sendData16Bit(j, dataPin, clockPin); 
    digitalWrite(latchPin, HIGH);
  }
}
void reverseArray(uint8_t* array, const uint16_t size) {
    uint16_t start = 0;
    uint16_t end = size - 1;
    while (start < end) {
        // Swap elements at start and end indices
        uint8_t temp = array[start];
        array[start] = array[end];
        array[end] = temp;
        
        // Move to the next pair of elements
        start++;
        end--;
    }
}

  // Write pixelbuffer data=DMX data, length=DMX data lenght, limit=number of leds, start=start addres (DMX addres), ofset=the amount of leds it needs to blank before writing data
  void ledDriver::writePixelBuffer(const uint8_t* data, const uint16_t length, const uint16_t nrOfleds, const uint16_t start, const uint16_t ledOfset) {
    // reset wdt
    rstWdt(); 
    uint16_t limit = b_16Bit ? nrOfleds * 6 : nrOfleds * 3;
    // set limit times 3 since each led has 3 values RGB, also add start address to the limit since we shift that amount of addresses.
    uint16_t end = limit + start;

      digitalWrite(latchPin, LOW);
      if (b_reverseArray == 0) {
          for (int i = std::min(length, end) - 1; i >= start; i--) {
              if (b_16Bit){
                  int coars = data[i];
                  int fine = data[i-1];
                  // update the counter we took another value 
                  i = i-1;
                  int value = (coars * 256) + fine;
                  sendData16Bit(value, dataPin, clockPin);
              } else {     
                  // re-order RGB                      
                  //sendData(data[i+2], dataPin, clockPin); //R
                  //sendData(data[i+1], dataPin, clockPin); //G
                  sendData(data[i], dataPin, clockPin);   //B
                  //i = i +2;
              }
          }
      } else {
          for (int i = start; i < std::min(length, end); i++) {
              if (b_16Bit){
                  int coars = data[i];
                  int fine = data[i+1];
                  // update the counter we took another value 
                  i = i+1;
                  int value = (coars * 256) + fine;
                  sendData16Bit(value, dataPin, clockPin);
              } else {
                 sendData(data[i], dataPin, clockPin);            
              }
          }
      }
      if (ledOfset > 0) {
      digitalWrite(latchPin, LOW);
      int ofset =  b_16Bit ? ledOfset * 6 : ledOfset * 3;
      for (int j = 0; j < ofset; ++j) {
        // just send data 0 to blank the channels
        sendData(0, dataPin, clockPin);
      }
      digitalWrite(latchPin, HIGH);
    }
 
    
     
    
    digitalWrite(latchPin, HIGH);
  }

void ledDriver::reverseArray(uint8_t* array, const uint16_t size) {
    uint16_t start = 0;
    uint16_t end = size - 1;
    while (start < end) {
        // Swap elements at start and end indices
        uint8_t temp = array[start];
        array[start] = array[end];
        array[end] = temp;
        
        // Move to the next pair of elements
        start++;
        end--;
    }
}
/*
void ledDriver::writePixelBuffer(const uint8_t* data, const uint16_t length, const uint16_t nrOfleds, const uint16_t start, const uint16_t ledOfset) {
    // reset wdt
    rstWdt(); 
    uint16_t limit = b_16Bit ? nrOfleds * 6 : nrOfleds * 3;
    // set limit times 3 since each led has 3 values RGB, also add start address to the limit since we shift that amount of addresses.
    uint16_t end = limit + start;

    // Check if the length of the data array needs to be cut with the end index
    uint16_t dataLength = std::min(length, end);
    const uint8_t* processedData = data; // Initialize processedData with the original data

    // If b_reverseArray is true, create a copy of the processed data and reverse it
    if (b_reverseArray == 0) {
        uint8_t reversedData[length];
        memcpy(reversedData, processedData, dataLength); // Make a copy of the processed data
        reverseArray(reversedData, dataLength); // Reverse the copied array
        processedData = reversedData; // Update processedData pointer to point to the reversedData
    }

    if (ledOfset > 0) {
        digitalWrite(latchPin, LOW);
        int offset =  b_16Bit ? ledOfset * 6 : ledOfset * 3;
        for (int j = 0; j < offset; ++j) {
            // just send data 0 to blank the channels
            sendData(0, dataPin, clockPin);
        }
        digitalWrite(latchPin, HIGH);
    }
 
    digitalWrite(latchPin, LOW);
    for (int i = start; i < dataLength; i++) {
        if (b_16Bit){
            int coars = processedData[i];
            int fine = processedData[i+1];
            // update the counter we took another value 
            i = i+1;
            int value = (coars * 256) + fine;
            sendData16Bit(value, dataPin, clockPin);
        }
        else {
            sendData(processedData[i], dataPin, clockPin);
        }
    }
    digitalWrite(latchPin, HIGH);
}
*/
  void ledDriver::showBuffer(){

    digitalWrite(latchPin, HIGH);
    digitalWrite(latchPin, LOW);
    //Serial.println("Latch");
  }
  void ledDriver::showBufferP2(){

    digitalWrite(latchPin2, HIGH);
    digitalWrite(latchPin2, LOW);
    //Serial.println("Latch");
  }
  /*
void ledDriver::writePixelBufferPort2(const uint8_t* data, const uint16_t length, const uint16_t nrOfleds, const uint16_t start, const uint16_t ledOfset) {
    // reset wdt
    rstWdt(); 
    uint16_t limit = b_16Bit ? nrOfleds * 6 : nrOfleds * 3;
    // set limit times 3 since each led has 3 values RGB, also add start address to the limit since we shift that amount of addresses.
    uint16_t end = limit + start;

    // Check if the length of the data array needs to be cut with the end index
    uint16_t dataLength = std::min(length, end);
    const uint8_t* processedData = data; // Initialize processedData with the original data

    // If b_reverseArray is true, create a copy of the processed data and reverse it
    if (b_reverseArray == 0) {
        uint8_t reversedData[length];
        memcpy(reversedData, processedData, dataLength); // Make a copy of the processed data
        reverseArray(reversedData, dataLength); // Reverse the copied array
        processedData = reversedData; // Update processedData pointer to point to the reversedData
    }

    if (ledOfset > 0) {
        digitalWrite(latchPin2, LOW);
        int offset =  b_16Bit ? ledOfset * 6 : ledOfset * 3;
        for (int j = 0; j < offset; ++j) {
            // just send data 0 to blank the channels
            sendData(0, dataPin2, clockPin2);
        }
        digitalWrite(latchPin2, HIGH);
    }
 
    digitalWrite(latchPin2, LOW);
    for (int i = start; i < dataLength; i++) {
        if (b_16Bit){
            int coars = processedData[i];
            int fine = processedData[i+1];
            // update the counter we took another value 
            i = i+1;
            int value = (coars * 256) + fine;
            sendData16Bit(value, dataPin2, clockPin2);
        }
        else {
            sendData(processedData[i], dataPin2, clockPin2);
        }
    }
    digitalWrite(latchPin2, HIGH);
    */

  
  // Write pixelbuffer data=DMX data, length=DMX data lenght, limit=number of leds, start=start addres (DMX addres), ofset=the amount of leds it needs to blank before writing data
  void ledDriver::writePixelBufferPort2(const uint8_t* data, const uint16_t length, const uint16_t nrOfleds, const uint16_t start, const uint16_t ledOfset) {
    // reset wdt
    rstWdt(); 
    uint16_t limit = b_16Bit ? nrOfleds * 6 : nrOfleds * 3;
    // set limit times 3 since each led has 3 values RGB, also add start address to the limit since we shift that amount of addresses.
    uint16_t end = limit + start;
 
    digitalWrite(latchPin2, LOW);
    /*
    for (int i = start; i < std::min(length, end); i++) {
      if (b_16Bit){
        int coars = data[i];
        int fine = data[i+1];
        // update the counter we took another value 
        i = i+1;
        int value = (coars * 256) + fine;
        sendData16Bit(value, dataPin, clockPin);
      }
      else {
        sendData(data[i], dataPin, clockPin);
      }
      */
      if (b_reverseArray == 0) {
          for (int i = std::min(length, end) - 1; i >= start; i--) {
              if (b_16Bit){
                  int coars = data[i];
                  int fine = data[i-1];
                  // update the counter we took another value 
                  i = i-1;
                  int value = (coars * 256) + fine;
                  sendData16Bit(value, dataPin2, clockPin2);
              } else {
                  sendData(data[i], dataPin2, clockPin2);
              }
          }
      } else {
          for (int i = start; i < std::min(length, end); i++) {
              if (b_16Bit){
                  int coars = data[i];
                  int fine = data[i+1];
                  // update the counter we took another value 
                  i = i+1;
                  int value = (coars * 256) + fine;
                  sendData16Bit(value, dataPin2, clockPin2);
              } else {
                  sendData(data[i], dataPin2, clockPin2);
              }
          }
      }

    if (ledOfset > 0) {
      digitalWrite(latchPin2, LOW);
      int ofset =  b_16Bit ? ledOfset * 6 : ledOfset * 3;
      for (int j = 0; j < ofset; ++j) {
        // just send data 0 to blank the channels
        sendData(0, dataPin2, clockPin2);
      }
      digitalWrite(latchPin2, HIGH);
    }
     
    
    digitalWrite(latchPin2, HIGH);
  }

  // Write pixelbuffer data=DMX data, length=DMX data lenght, limit=number of leds, start=start addres (DMX addres), ofset=the amount of leds it needs to blank before writing data
  void ledDriver::writePixelBufferFull(const uint8_t* data, const uint16_t length, const uint16_t nrOfleds, const uint16_t start, const uint16_t ledOfset) {
    // reset wdt
    rstWdt(); 
    //uint16_t limit = b_16Bit ? nrOfleds * 6 : nrOfleds * 3;
    // set limit times 3 since each led has 3 values RGB, also add start address to the limit since we shift that amount of addresses.
    //uint16_t end = limit + start;
    if (ledOfset > 0) {
      digitalWrite(latchPin, LOW);
      int ofset =  b_16Bit ? ledOfset * 6 : ledOfset * 3;
      for (int j = 0; j < ofset; ++j) {
        // just send data 0 to blank the channels
        sendData(0, dataPin, clockPin);
      }
      digitalWrite(latchPin, HIGH);
    }
 
    digitalWrite(latchPin, LOW);
    for (int i = start; i < length; i++) {
      if (b_16Bit){
        int coars = data[i];
        int fine = data[i+1];
        // update the counter we took another value 
        i = i+1;
        int value = (coars * 256) + fine;
        sendData16Bit(value, dataPin, clockPin);
      }
      else {
        sendData(data[i], dataPin, clockPin);
      }
     
    }
    digitalWrite(latchPin, HIGH);
  }
/*
void ledDriver::writePixelBuffer(const uint8_t* data, const uint16_t length, const uint16_t nrOfleds, const uint16_t start, const uint16_t ledOffset) {
    // reset wdt
    rstWdt(); 
    uint16_t limit = b_16Bit ? nrOfleds * 6 : nrOfleds * 3;
    // set limit times 3 since each led has 3 values RGB, also add start address to the limit since we shift that amount of addresses.
    uint16_t end = limit + start;
    
    if (ledOffset > 0) {
    
        int offset =  b_16Bit ? ledOffset * 2 : ledOffset;
        
        for (int j = 0; j < offset; ++j) {
            digitalWrite(latchPin, LOW);
            // just send data 0 to blank the channels
            sendData(0, dataPin, clockPin);
            sendData(0, dataPin, clockPin);
            sendData(0, dataPin, clockPin);
            digitalWrite(latchPin, HIGH);
        }
        
    }

    digitalWrite(latchPin, LOW);
    
    // Iterate in reverse order
    for (int i = std::min(length, end) - 1; i >= start; i--) {
        if (b_16Bit) {
            int coarse = data[i];
            int fine = data[i - 1];
            // update the counter we took another value 
            i = i - 1;
            int value = (coarse * 256) + fine;
            sendData16Bit(value, dataPin, clockPin);
        } else {
            sendData(data[i], dataPin, clockPin);
        }
    }

    digitalWrite(latchPin, HIGH);
}
*/
  void ledDriver::writePixelBuffer_(const uint8_t* data, const uint16_t length, const uint16_t limit, const uint16_t start) {
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

  void ledDriver::blankLEDS(int nrToblank) {
    digitalWrite(latchPin, LOW);
    digitalWrite(latchPin2, LOW);
    // blank number times 3 since each led has 3 values
    for (int i = 0; i < nrToblank * 3; ++i) {
      sendData(0, dataPin, clockPin);
      sendData(0, dataPin2, clockPin2);
    }
    digitalWrite(latchPin, HIGH);
    digitalWrite(latchPin2, HIGH);
  }

  void ledDriver::ledOn() {
    digitalWrite(latchPin, LOW);
    int nrBlank = (NumberOfLeds - 1) * 3;
    for (int i = 0; i < nrBlank; ++i) {
      sendData(0, dataPin, clockPin);
    }
    sendData(255, dataPin, clockPin);
    digitalWrite(latchPin, HIGH);
  }
void ledDriver::setLEDColor(int r, int g, int b )
{
   int nrLeds = NrOfLeds;
   setLEDColor(r,g,b,nrLeds);
}
void ledDriver::setLEDColor(int r, int g, int b, int nrLeds)
{
    // Check if any of the color values are greater than 0
    //if (r > 0 || g > 0 || b > 0) {
        setDiag(true);
        digitalWrite(latchPin, LOW);
        for (int i = 0; i < nrLeds ; ++i) {
            sendData(r, dataPin, clockPin);
            sendData(g, dataPin, clockPin);
            sendData(b, dataPin, clockPin);
        }
        digitalWrite(latchPin, HIGH);
      //}
    //else{
        setDiag(false);
      //}   
}
/// this function uses coars and fine to get a 16bit color value 
void ledDriver::setLEDColor16bit(int cR,int fR, int cG,int fG, int cB,int fB)
{
        int r = (cR * 256) + fR;
        int g = (cG * 256) + fG;
        int b = (cB * 256) + fB;

        //Serial.printf("diag: %s\n", diag ? "true" : "false");
        digitalWrite(latchPin, LOW);
        for (int i = 0; i < NrOfLeds ; ++i) {
            sendData(r, dataPin, clockPin);
            sendData(g, dataPin, clockPin);
            sendData(b, dataPin, clockPin);
        }
        digitalWrite(latchPin, HIGH);
        //delay(50);
  
       
}
void ledDriver::setLEDColor16bit(int r ,int g, int b)
{


        //Serial.printf("diag: %s\n", diag ? "true" : "false");
        digitalWrite(latchPin, LOW);
        for (int i = 0; i < NrOfLeds ; ++i) {
            sendData16Bit(r, dataPin, clockPin);
            sendData16Bit(g, dataPin, clockPin);
            sendData16Bit(b, dataPin, clockPin);
        }
        digitalWrite(latchPin, HIGH);
        //delay(50);
  
       
}
  void ledDriver::rgbEffect(void* parameter) {
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
      for (uint8_t t = 0; t < 3; ++t) {
        // Color scroll effect
        for (int color = 0; color < 190; ++color) {
          // Set the RGB color based on the color variable
          const uint8_t singlePixelData[] = { static_cast<uint8_t>(color), 0, static_cast<uint8_t>(255 - color) };  // Example: R-G-B color

          // Display the color on each led on the RGB strip
          for (int i = 0; i < 1; ++i) {
            writePixelBuffer(singlePixelData, sizeof(singlePixelData), 170, 0, i);
            //sendData(singlePixelData[i], dataPin, clockPin);
          }
          
          delay(1);  // Adjust delay for speed
        }

        for (int color = 190; color > 0; --color) {
          // Set the RGB color based on the color variable
          const uint8_t singlePixelData[] = { static_cast<uint8_t>(color), 0,  static_cast<uint8_t>(0 + color) };  // Example: R-G-B color

          // Display the color on each led on the RGB strip
          for (int i = 0; i < 1; ++i) {
            writePixelBuffer(singlePixelData, sizeof(singlePixelData), 170, 0, i);
            //sendData(singlePixelData[i], dataPin, clockPin);
          }
          delay(1);  // Adjust delay for speed
        }
        for (int color = 190; color > 0; --color) {
          // Set the RGB color based on the color variable
          const uint8_t singlePixelData[] = { static_cast<uint8_t>(color), static_cast<uint8_t>(0 + color),0  };  // Example: R-G-B color

          // Display the color on each led on the RGB strip
          for (int i = 0; i < 1; ++i) {
            writePixelBuffer(singlePixelData, sizeof(singlePixelData), 170, 0, i);
            //sendData(singlePixelData[i], dataPin, clockPin);
          }
          delay(1);  // Adjust delay for speed
        }
        //reset watchdog 
        rstWdt(); 
        blankLEDS(170);
      }
    }

  }
  void ledDriver::blink(int nrs, int delayTime){
    blankLEDS(179);
    digitalWrite(latchPin, LOW);
    int nrBlank = (NumberOfLeds - 1) * 3;
    for (int j = 0; j < nrBlank; ++j) {
      sendData(0, dataPin, clockPin);
    }
    for (int j = nrs; j > 0; --j) {
      sendData(255, dataPin, clockPin);
      sendData(0, dataPin, clockPin);
      sendData(0, dataPin, clockPin);
    }
    digitalWrite(latchPin, HIGH);
    delay(delayTime);
    blankLEDS(179);
 }
  
void ledDriver::setGamma(float gamma){
  //logToFile(String(gamma));
  GAMMA_CORRECTION = gamma;
}
void ledDriver::cFlash() {
  ledDriver instance;  // Create an instance of the ledDriver class
    for (int t = 0; t < 2; ++t) {
      for (int color = 0; color < 100; ++color) {
        const uint8_t singlePixelData[] = { static_cast<uint8_t>(color), 16, static_cast<uint8_t>(255 - color) };
        for (int i = 0; i < 1; ++i) {
          instance.writePixelBuffer(singlePixelData, sizeof(singlePixelData), 179, 0, 0);
        }
        delay(2);
      }
      for (int color = 100; color > 0; --color) {
        const uint8_t singlePixelData[] = { static_cast<uint8_t>(color), 0, static_cast<uint8_t>(0 + color) };
        for (int i = 0; i < 1; ++i) {
          instance.writePixelBuffer(singlePixelData, sizeof(singlePixelData), 179, 0, 0);
        }
        delay(3);
      }
      instance.blankLEDS(179);
    }
  
}

  void ledDriver::ledTest() {
    for (int i = 0; i < NumberOfLeds; i++) {
      digitalWrite(latchPin, LOW);
      sendData(0, dataPin, clockPin);
      sendData(0, dataPin, clockPin);
      sendData(175, dataPin, clockPin);
      digitalWrite(latchPin, HIGH);
      delay(10);
    }
    for (int i = 0; i < NumberOfLeds; i++) {
      digitalWrite(latchPin2, LOW);
      sendData(0, dataPin2, clockPin2);
      sendData(0, dataPin2, clockPin2);
      sendData(175, dataPin2, clockPin2);
      digitalWrite(latchPin2, HIGH);
      delay(10);
    }
  }
