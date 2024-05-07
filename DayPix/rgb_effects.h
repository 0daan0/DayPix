#ifndef RGB_EFFECTS_H
#define RGB_EFFECTS_H

#include <Arduino.h>
#include "LedDriver.h"
extern ledDriver led; 

class RGBEffects {
public:
    // Constructor
    RGBEffects();
    // Initialize method
    void initialize();

    //effects
    void rainbowChase16bit(int value);
    void colorPulseBreathing(int period, int delayTime);

private:
    // Add any private members if needed
};

#endif  // RGB_EFFECTS_H