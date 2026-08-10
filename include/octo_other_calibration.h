#ifndef OCTO_OTHER_CALIBRATION_H
#define OCTO_OTHER_CALIBRATION_H

#include <TFT_eSPI.h>
#include "octo_client.h"

class OctoOtherCalibrationMenu {
public:
    OctoOtherCalibrationMenu(TFT_eSPI* tft);
    void init();
    void draw(OctoClient* client);
    int handleTouch(uint16_t x, uint16_t y, OctoClient* client);
    void forceRedraw() { _forceRedraw = true; }
    void open() { _forceRedraw = true; _subState = 0; _estepSubState = 0; _pidSubState = 0; _mpcSubState = 0; _showPopup = false; _pidCommandSent = false; _mpcCommandSent = false; }

private:
    TFT_eSPI* _tft;
    int _subState = 0;        // 0: Main, 1: E-Step, 2: PID, 3: MPC
    int _estepSubState = 0;   // 0: Material, 1: Heating, 2: Measure
    int _pidSubState = 0;     // 0: Menu, 1: Running / Wait
    int _mpcSubState = 0;     // 0: Menu, 1: Running / Wait
    bool _forceRedraw = true;

    bool _pidCommandSent = false; // Duplikált küldés védelmére
    bool _mpcCommandSent = false; // Duplikált küldés védelmére

    float _estepDiff = 0.0f;
    int _targetTemp = 200;

    bool _showPopup = false;
    String _popupTitle = "";
    String _popupMsg1 = "";
    String _popupMsg2 = "";
    uint16_t _popupColor = TFT_BLUE;

    void drawMainMenu();
    void drawEStepMaterialMenu();
    void drawEStepHeatingMenu(OctoClient* client);
    void drawEStepMeasureMenu();
    void drawPidMenu();
    void drawPidRunningMenu(OctoClient* client);
    void drawMpcMenu();
    void drawMpcRunningMenu(OctoClient* client);
    void drawPopup();

    void saveEStepCalibration(OctoClient* client);
};

#endif