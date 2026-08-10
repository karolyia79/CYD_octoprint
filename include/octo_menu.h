#ifndef OCTO_MENU_H
#define OCTO_MENU_H

#include <TFT_eSPI.h>
#include "octo_client.h"
#include "octo_bedlevel_menu.h"
#include "octo_other_calibration.h" // ✅ ÚJ: Bekötöttük az egyéb kalibrációs menüt is

class OctoMenu {
public:
    enum SubState {
        SUB_MAIN = 0,       // Karbantartási Főmenü (2x2 rács)
        SUB_TUNE = 1,       // Élő Hangolás Nézet (Nyomtatás közben)
        SUB_Z_OFFSET = 2,   // Z-Offset Kalibrációs Nézet
        SUB_MESH = 3,       // Bed Mesh Kezelő Menü
        SUB_SHOW_MESH = 4,  // Bed Mesh Négyzethálós Kirajzolás
        SUB_PREPARE = 5,    // Előkészítés almenü (AutoHome)
        SUB_CONTROL = 6,    // Irányítás almenü (üres)
        SUB_CALIBRATION = 7,// Kalibráció almenü (Z-offset, Bed mesh)
        SUB_FILAMENT = 8,   // Filament csere almenü (üres)
        SUB_BED_LEVEL = 9,  // Asztalszintezés (Leveling) állapota[cite: 15]
        SUB_OTHER_CALIB = 10 // ✅ ÚJ: Egyéb kalibrációk (E-Step, PID, MPC)
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
    
    OctoBedLevelMenu _bedLevelMenu;       // A Leveling menü objektuma[cite: 15]
    OctoOtherCalibrationMenu _otherCalibMenu; // ✅ ÚJ: Az egyéb kalibrációk objektuma
    
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
    void drawControlMenu();
    void drawCalibrationMenu();
    void drawFilamentMenu();

    void drawTuneMenu(OctoClient* client);
    void drawZOffsetMenu();
    void drawMeshMenu(OctoClient* client = nullptr);
    void drawShowMeshMenu(OctoClient* client);
    void updatePrepareMenu(OctoClient* client);

    // Segédfunkciók az animációhoz és a felugró ablakhoz
    void drawMeshSavedPopup();
    void updateMeshButtons(OctoClient* client);
    uint16_t getPulsingColor();

    void drawTuneRow(int y, const String& label, const String& valueStr);
    uint16_t getMeshColor(float val);
};

#endif