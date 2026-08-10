#ifndef TOUCH_H
#define TOUCH_H

#include <Arduino.h>
#include "CST820.h"

class TouchController {
public:
    static void begin();
    static bool getTouch(uint16_t* x, uint16_t* y);
};

#endif