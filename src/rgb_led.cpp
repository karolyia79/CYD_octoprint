#include "rgb_led.h"
#include <math.h>

void RgbLed::init() {
    ledcAttach(PIN_RED, FREQ, RESOLUTION);
    ledcAttach(PIN_GREEN, FREQ, RESOLUTION);
    ledcAttach(PIN_BLUE, FREQ, RESOLUTION);

    off();
}

void RgbLed::setColor(uint8_t r, uint8_t g, uint8_t b) {
    ledcWrite(PIN_RED, 255 - r);
    ledcWrite(PIN_GREEN, 255 - g);
    ledcWrite(PIN_BLUE, 255 - b);
}

void RgbLed::off() {
    setColor(0, 0, 0);
}

void RgbLed::updateStatus(bool wifiConnected, bool mqttConnected, bool apiConnected, 
                          bool isPrinting, bool isPaused, bool isHeating, 
                          bool isHoming, bool isMeshBuilding, bool isBooting) {
    
    // Ha a főkapcsolóval ki van kapcsolva, azonnal sötétítünk és kilépünk
    if (!_enabled) {
        off();
        return;
    }

    unsigned long now = millis();

    // 1. Bootolás közben: Zöld pulzálás
    if (isBooting) {
        float factor = (sin(now / 250.0) + 1.0) / 2.0;
        setColor(0, (uint8_t)(255 * factor), 0);
        return;
    }

    // 2. WiFi hiba: Sárga gyors villogás
    if (!wifiConnected) {
        bool flash = (now / 200) % 2 == 0;
        if (flash) setColor(255, 200, 0);
        else off();
        return;
    }

    // 3. MQTT hiba: Sárga - Kék pulzáló átmenet
    if (!mqttConnected) {
        float factor = (sin(now / 350.0) + 1.0) / 2.0;
        uint8_t r = (uint8_t)(255 * (1.0 - factor));
        uint8_t g = (uint8_t)(200 * (1.0 - factor));
        uint8_t b = (uint8_t)(255 * factor);
        setColor(r, g, b);
        return;
    }

    // 4. API hiba: Sárga - Piros pulzáló átmenet
    if (!apiConnected) {
        float factor = (sin(now / 350.0) + 1.0) / 2.0;
        uint8_t r = 255;
        uint8_t g = (uint8_t)(200 * (1.0 - factor));
        uint8_t b = 0;
        setColor(r, g, b);
        return;
    }

    // 5. Homing vagy Mesh építés: Sárga pulzálás
    if (isHoming || isMeshBuilding) {
        float factor = (sin(now / 200.0) + 1.0) / 2.0;
        setColor(255, (uint8_t)(200 * factor), 0);
        return;
    }

    // 6. Szüneteltetve: Lassú narancs/sárga pulzálás
    if (isPaused) {
        float factor = (sin(now / 700.0) + 1.0) / 2.0;
        setColor(255, (uint8_t)(120 * factor), 0);
        return;
    }

    // 7. Nyomtatás közben: Kék, lassú pulzálás
    if (isPrinting) {
        float factor = (sin(now / 500.0) + 1.0) / 2.0;
        setColor(0, 0, (uint8_t)(255 * factor));
        return;
    }

    // 8. Fűtés
    if (isHeating) {
        float factor = (sin(now / 300.0) + 1.0) / 2.0;
        setColor((uint8_t)(255 * factor), 0, (uint8_t)(150 * factor));
        return;
    }

    // 9. Standby (Idle): Zöld nagyon lassú, kellemes pulzálás
    {
        float factor = (sin(now / 1200.0) + 1.0) / 2.0;
        setColor(0, (uint8_t)(160 * factor), 0);
    }
}