#ifndef OCTO_CONTROL_MENU_MQTT_H
#define OCTO_CONTROL_MENU_MQTT_H

#include <TFT_eSPI.h>
#include "octo_client_mqtt.h"

enum MoveAxis { 
    AXIS_X = 0, 
    AXIS_Y = 1, 
    AXIS_Z = 2, 
    AXIS_E = 3 
};

enum TempTarget {
    TEMP_BED = 0,
    TEMP_NOZZLE = 1
};

class OctoControlMenuMqtt {
public:
    OctoControlMenuMqtt(TFT_eSPI* tft);
    void init();
    void draw(OctoClientMqtt* client);
    int handleTouch(uint16_t x, uint16_t y, OctoClientMqtt* client);
    void forceRedraw() { _forceRedraw = true; }
    void open() { _forceRedraw = true; _subState = 0; _showColdWarningPopup = false; }

private:
    TFT_eSPI* _tft;
    // _subState jelentése:
    // 0: Control főmenü
    // 1: Move tengelyválasztó menü (X, Y, Z, E)
    // 2: Temp almenü választó (Bed / Nozzle)
    // 3: BLTouch menü
    // 4: Fan (Ventilátor) menü
    // 10: Egyedi tengely mozgató képernyő (+ / - / lépésköz)
    // 20: Egyedi hőmérséklet állító képernyő (Bed vagy Nozzle)
    int _subState = 0;
    bool _forceRedraw = true;

    // Tárolt ventilátor fordulatszám (0-100%)
    int _fanPercent = 0;

    // Move funkció állapota
    MoveAxis _currentAxis = AXIS_X;
    float _moveStepSizes[4] = {0.01f, 0.05f, 0.1f, 1.0f};
    int _selectedMoveStepIndex = 2; // Alapértelmezett: 0.1 mm

    // Temperature funkció állapota
    TempTarget _currentTempTarget = TEMP_NOZZLE;
    int _tempStepSizes[2] = {1, 10}; // 1°C és 10°C lépésköz
    int _selectedTempStepIndex = 1;  // Alapértelmezett: 10°C
    bool _pulseState = false;
    unsigned long _lastPulseTime = 0;

    // Cold Extrusion popup állapota
    bool _showColdWarningPopup = false;

    // Kirajzoló privát metódusok
    void drawMainMenu();
    void drawMoveAxisSelectMenu();
    void drawMoveControlMenu(OctoClientMqtt* client);
    void drawTempSelectMenu();
    void drawTempControlMenu(OctoClientMqtt* client);
    void drawTempDisplayBox(OctoClientMqtt* client); // Célzottan csak a hőfok boxot rajzolja újra
    void drawColdWarningPopup();
    void drawBltouchMenu();
    void drawFanMenu();
    void drawSubMenu();

    // Segédfunkciók
    void sendMoveCommand(float distance, OctoClientMqtt* client);
    void sendTempCommand(int targetTemp, OctoClientMqtt* client);
};

#endif