#ifndef OCTO_MENU_MQTT_H
#define OCTO_MENU_MQTT_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "octo_client_mqtt.h"
#include "octo_bedlevel_menu_mqtt.h"
#include "octo_other_calibration_mqtt.h"
#include "octo_control_menu_mqtt.h"

class OctoMenuMqtt {
public:
    OctoMenuMqtt(TFT_eSPI* tft);

    void draw(OctoClientMqtt* client);
    int handleTouch(uint16_t x, uint16_t y, OctoClientMqtt* client);

    void openMainMenu() { _subState = SUB_MAIN; _forceRedraw = true; }
    void openTuneMenu() { _subState = SUB_TUNE; _forceRedraw = true; }
    void forceRedraw() { _forceRedraw = true; }

private:
    TFT_eSPI* _tft;
    OctoBedLevelMenuMqtt _bedLevelMenu;
    OctoOtherCalibrationMenuMqtt _otherCalibMenu;
    OctoControlMenuMqtt _controlMenu;

    enum SubState {
        SUB_MAIN, SUB_PREPARE, SUB_CONTROL, SUB_CALIBRATION,
        SUB_FILAMENT, SUB_TUNE, SUB_Z_OFFSET, SUB_MESH, SUB_SHOW_MESH,
        SUB_BED_LEVEL, SUB_OTHER_CALIB
    };
    int _subState = SUB_MAIN;
    bool _forceRedraw = true;

    bool _lastMeshSavedPopupState = false;
    bool _lastNoMeshPopupState = false;

    bool _showUnsupportedPopup = false;
    bool _lastUnsupportedPopupState = false;
    unsigned long _unsupportedPopupStartMs = 0;

    void drawMainMenu();
    void drawPrepareMenu(OctoClientMqtt* client = nullptr);
    void drawCalibrationMenu();
    void drawFilamentMenu();
    void drawTuneMenu(OctoClientMqtt* client);
    void drawZOffsetMenu();
    void drawMeshMenu(OctoClientMqtt* client);
    void drawShowMeshMenu(OctoClientMqtt* client);

    void updatePrepareMenu(OctoClientMqtt* client, bool force = false);
    void updateMeshButtons(OctoClientMqtt* client);

    void drawTuneRow(int y, const String& label, const String& valueStr);
    void drawMeshSavedPopup();
    void drawMeshLoadingScreen();
    void drawNoMeshPopup();
    void drawUnsupportedPopup();

    uint16_t getPulsingColor();
    uint16_t getMeshColor(float val);
};

#endif