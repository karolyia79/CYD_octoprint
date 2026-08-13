#ifndef OCTO_BEDLEVEL_MENU_MQTT_H
#define OCTO_BEDLEVEL_MENU_MQTT_H

#include <TFT_eSPI.h>
#include "octo_client_mqtt.h"
#include "octo_config.h"

class OctoBedLevelMenuMqtt {
public:
    OctoBedLevelMenuMqtt(TFT_eSPI* tft);
    void init();
    void draw(OctoClientMqtt* client);
    int handleTouch(uint16_t x, uint16_t y, OctoClientMqtt* client);
    void forceRedraw() { _forceRedraw = true; }
    void open() { _forceRedraw = true; _subState = 0; }

private:
    TFT_eSPI* _tft;
    int _subState = 0; // 0: Main, 1: Wizard, 2: Coords
    bool _forceRedraw = true;
    
    OctoConfigData _config;

    int _selectedCorner = 0; // 0: FL, 1: FR, 2: BR, 3: BL
    int _levelingStep = 0;
    bool _isHomed = false;

    bool _wizProcessing = false;
    uint32_t _wizTimer = 0;
    uint32_t _wizWaitTime = 0;

    void drawMainMenu();
    void drawWizard(OctoClientMqtt* client);
    void drawCoords();
};

#endif