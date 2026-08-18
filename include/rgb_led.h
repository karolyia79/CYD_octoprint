#ifndef RGB_LED_H
#define RGB_LED_H

#include <Arduino.h>

class RgbLed {
public:
    void init();
    void updateStatus(bool wifiConnected, bool mqttConnected, bool apiConnected, 
                      bool isPrinting, bool isPaused, bool isHeating, 
                      bool isHoming, bool isMeshBuilding, bool isBooting);

    // --- FŐKAPCSOLÓ METÓDUSOK ---
    void setEnabled(bool enabled) { 
        _enabled = enabled; 
        if (!_enabled) off(); 
    }
    bool isEnabled() const { return _enabled; }
    void toggleEnabled() { setEnabled(!_enabled); }

private:
    const uint8_t PIN_RED = 4;
    const uint8_t PIN_GREEN = 16;
    const uint8_t PIN_BLUE = 17;

    const int FREQ = 5000;
    const int RESOLUTION = 8; // 0-255

    bool _enabled = true; // Alapértelmezésben bekapcsolva

    void setColor(uint8_t r, uint8_t g, uint8_t b);
    void off();
};

#endif