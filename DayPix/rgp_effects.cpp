#include "rgb_effects.h"
#include <Arduino.h>
#include "LedDriver.h"  // Include this line
#include "Config.h"
#include "HwFunctions.h"
ledDriver ledDriverInstance; 

RGBEffects::RGBEffects() {
    // Constructor logic
    Serial.println("RGBEffects Constructor");
}

void RGBEffects::initialize() {
    // Initialization logic
    Serial.println("RGBEffects Initialize");
    // Add any additional initialization steps if needed
}

void RGBEffects::rainbowChase16bit(int delayTime) {
  setDiag(true);
  // Loop through all color steps of the rainbow
  for (int step = 0; step < 256; step++) {
    // Calculate color values for each component
    float ratio = float(step) / 255.0;
    int coarsR = int(255 * (1 - ratio));
    int fineR = int(255 * ratio);
    int coarsG = int(255 * (1 - fmod(ratio + 0.33, 1.0)));
    int fineG = int(255 * fmod(ratio + 0.33, 1.0));
    int coarsB = int(255 * (1 - fmod(ratio + 0.67, 1.0)));
    int fineB = int(255 * fmod(ratio + 0.67, 1.0));

    // Write the color to the LEDs
    led.setLEDColor16bit(coarsR, fineR, coarsG, fineG, coarsB, fineB);

    // Delay for the specified time
    delay(delayTime);
    rstWdt(); 
  }
  for (int step = 255; step >= 0; step--) {
    // Calculate color values for each component in reverse
    float ratio = float(step) / 255.0;
    int coarsR = int(255 * ratio);
    int fineR = int(255 * (1 - ratio));
    int coarsG = int(255 * fmod(ratio + 0.67, 1.0));
    int fineG = int(255 * (1 - fmod(ratio + 0.67, 1.0)));
    int coarsB = int(255 * fmod(ratio + 0.33, 1.0));
    int fineB = int(255 * (1 - fmod(ratio + 0.33, 1.0)));

    // Write the color to the LEDs
    led.setLEDColor16bit(coarsR, fineR, coarsG, fineG, coarsB, fineB);

    // Delay for the specified time
    delay(delayTime);
    rstWdt();
  }
  //setDiag(false);
}
void RGBEffects::colorPulseBreathing(int period, int delayTime) {
    setDiag(true);

    // Gradual increase in intensity
    for (int i = 0; i < 35535; i += 1000) {
        rstWdt();
        led.setLEDColor16bit(i, i, i);
        delay(20);
    }

    // Store the intensity level reached after gradual increase
    int startIntensity = 35535;

    // Start with the intensity level reached after gradual increase
    int intensityR = startIntensity;
    int intensityG = startIntensity;
    int intensityB = startIntensity;

    // Phase offsets for smooth transitions
    float phaseOffsetR = 0.0;  // Phase offset for red channel
    float phaseOffsetG = 0.2;  // Phase offset for green channel
    float phaseOffsetB = 0.4;  // Phase offset for blue channel

    // Pulse for the specified period
    for (int i = 0; i < period; i++) {
        // Calculate intensity for each color channel based on sinusoidal function with smoothing
        intensityR = int(startIntensity * (1 + sin(2 * PI * (float(i) / period + phaseOffsetR))));
        intensityG = int(startIntensity * (1 + sin(2 * PI * (float(i) / period + phaseOffsetG))));
        intensityB = int(startIntensity * (1 + sin(2 * PI * (float(i) / period + phaseOffsetB))));

        // Write the color to the LEDs
        led.setLEDColor16bit(intensityR, intensityG, intensityB);
        // Delay for the specified time
        rstWdt();
      if (period - i == 0) {
          // Check if all intensities are less than 65535
          while (intensityR < 65535 || intensityG < 65535 || intensityB < 65535) {
              rstWdt();
               // Increase intensities to 65535 without exceeding it
              if (intensityR < 65535) intensityR = min(intensityR + 100, 65535);
              if (intensityG < 65535) intensityG = min(intensityG + 100, 65535);
              if (intensityB < 65535) intensityB = min(intensityB + 100, 65535);
              // Set LED color to full intensity
              led.setLEDColor16bit(intensityR, intensityG, intensityB);
              delay(20);
              
          }
      }

     delay(delayTime);        
    }

    // Gradual decrease in intensity for all colors
    for (int i = 65535; i > 0; i -= 1000) {
        rstWdt();
        led.setLEDColor16bit(i, i, i);
        delay(20);
    }

    setDiag(false);
}


