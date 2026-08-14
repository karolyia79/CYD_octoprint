#ifndef OCTO_CONTROL_MENU_MQTT_H
#define OCTO_CONTROL_MENU_MQTT_H

#include <TFT_eSPI.h>
#include "octo_client_mqtt.h"

class OctoControlMenuMqtt {
public:
    OctoControlMenuMqtt(TFT_eSPI* tft);
    void init();
    void draw(OctoClientMqtt* client);
    int handleTouch(uint16_t x, uint16_t y, OctoClientMqtt* client);
    void forceRedraw() { _forceRedraw = true; }
    void open() { _forceRedraw = true; _subState = 0; }

private:
    TFT_eSPI* _tft;
    int _subState = 0; // 0: Fő grid, 1: Mozgás, 2: Hőmérséklet, 3: BL/CR touch, 4: Hűtés
    bool _forceRedraw = true;

    // Tárolt ventilátor fordulatszám (0-100%)
    int _fanPercent = 0;

    void drawMainMenu();
    void drawBltouchMenu();
    void drawFanMenu();
    void drawSubMenu();
};

#endif