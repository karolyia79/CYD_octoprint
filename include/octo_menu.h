#ifndef OCTO_MENU_H
#define OCTO_MENU_H

#include <TFT_eSPI.h>
#include "octo_client.h"
#include "octo_bedlevel_menu.h"
#include "octo_other_calibration.h"
#include "octo_control_menu.h"

class OctoMenu {
public:
    enum SubState {
        SUB_MAIN = 0,
        SUB_TUNE = 1,
        SUB_Z_OFFSET = 2,
        SUB_MESH = 3,
        SUB_SHOW_MESH = 4,
        SUB_PREPARE = 5,
        SUB_CONTROL = 6,
        SUB_CALIBRATION = 7,
        SUB_FILAMENT = 8,
        SUB_BED_LEVEL = 9,
        SUB_OTHER_CALIB = 10
    };

    OctoMenu(TFT_eSPI* tft);
    void draw(OctoClient* client = nullptr);
    int handleTouch(uint16_t x, uint16_t y, OctoClient* client = nullptr);

    void openMainMenu() { _subState = SUB_MAIN; _forceRedraw = true; }
    void openTuneMenu() { _subState = SUB_TUNE; _forceRedraw = true; }
    int getSubState() const { return _subState; }
    void forceRedraw() { _forceRedraw = true; }

    bool isShowingUnsupportedPopup() const { return _showUnsupportedPopup; }

private:
    TFT_eSPI* _tft;
    int _subState = SUB_MAIN;
    bool _forceRedraw = true;
    
    OctoBedLevelMenu _bedLevelMenu;
    OctoOtherCalibrationMenu _otherCalibMenu;
    OctoControlMenu _controlMenu;
    
    bool _showUnsupportedPopup = false;
    uint32_t _unsupportedPopupStartMs = 0;
    bool _lastMeshSavedPopupState = false;
    bool _lastNoMeshPopupState = false;
    bool _lastUnsupportedPopupState = false;
    bool _lastHomingActive = false;
    
    void drawUnsupportedPopup();
    void drawNoMeshPopup();

    void drawMainMenu();
    void drawPrepareMenu();
    void drawCalibrationMenu();
    void drawFilamentMenu();

    void drawTuneMenu(OctoClient* client);
    void drawZOffsetMenu();
    void drawMeshMenu(OctoClient* client = nullptr);
    void drawMeshLoadingScreen();
    void drawShowMeshMenu(OctoClient* client);
    void updatePrepareMenu(OctoClient* client);

    void drawMeshSavedPopup();
    void updateMeshButtons(OctoClient* client);
    uint16_t getPulsingColor();

    void drawTuneRow(int y, const String& label, const String& valueStr);
    uint16_t getMeshColor(float val);
};

#endif