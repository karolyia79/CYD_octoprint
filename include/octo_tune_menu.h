#ifndef OCTO_TUNE_MENU_H
#define OCTO_TUNE_MENU_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <HTTPClient.h>
#include <TJpg_Decoder.h>
#include "octo_client_mqtt.h"
#include "ui_utils.h"
#include "lang_manager.h"
#include "config_manager.h"

enum TuneSubMenu {
    TUNE_MAIN = 0,
    TUNE_TEMP,
    TUNE_SPEED,
    TUNE_ZOFFSET,
    TUNE_CAMERA
};

class OctoTuneMenu {
public:
    OctoTuneMenu(TFT_eSPI* tft);

    void init();
    void forceRedraw() { _forceRedraw = true; }
    void draw(OctoClientMqtt* client);
    int handleTouch(uint16_t x, uint16_t y, OctoClientMqtt* client);
    
    TuneSubMenu getSubMenu() const { return _subMenu; }
    bool isCameraActive() const { return _subMenu == TUNE_CAMERA; }
    void resetMenu() { _subMenu = TUNE_MAIN; _cameraLoaded = false; _forceRedraw = true; }

private:
    TFT_eSPI* _tft;
    TuneSubMenu _subMenu;
    bool _forceRedraw;
    bool _cameraLoaded;

    void drawMainGrid();
    void drawTempMenu(OctoClientMqtt* client);
    void drawSpeedMenu(OctoClientMqtt* client);
    void drawZOffsetMenu(OctoClientMqtt* client);
    void drawCameraMenu(OctoClientMqtt* client);

    void drawHeader(const String& title);
    void drawBackButton();
};

#endif