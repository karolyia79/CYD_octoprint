#ifndef OCTO_FILAMENT_MENU_MQTT_H
#define OCTO_FILAMENT_MENU_MQTT_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "octo_client_mqtt.h"

class OctoFilamentMenuMqtt {
public:
    OctoFilamentMenuMqtt(TFT_eSPI* tft);

    void init();
    void open();
    void draw(OctoClientMqtt* client);
    int handleTouch(uint16_t x, uint16_t y, OctoClientMqtt* client);

private:
    TFT_eSPI* _tft;

    enum FilamentStep { STEP_SELECT, STEP_ACTIONS };
    FilamentStep _currentStep = STEP_SELECT;

    enum FilamentMaterial { MAT_PLA, MAT_PETG, MAT_ABS };
    FilamentMaterial _selectedMaterial = MAT_PLA;
    bool _isBowden = false; // false = Direct Drive, true = Bowden

    // Popup allapotok
    enum PopupType { POPUP_NONE, POPUP_WAIT_HEATING, POPUP_START_HEATING };
    PopupType _activePopup = POPUP_NONE;

    bool _forceRedraw = true;
    bool _lastTempReady = false;
    int _lastActualTemp = -1;
    int _lastTargetTemp = -1; // Célhőfok követése a piros piros gombállapothoz

    float getTargetTemp() const;
    bool isTempReady(OctoClientMqtt* client) const;

    void drawSelectStep();
    void drawActionsStep(OctoClientMqtt* client);
    void drawPopupWait();
    void drawPopupStartHeating();
};

#endif