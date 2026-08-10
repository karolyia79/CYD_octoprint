#ifndef MAINSCREEN_H
#define MAINSCREEN_H

#include <TFT_eSPI.h>
#include <CST820.h>
#include "octo_menu.h"
#include "klipper_menu.h"
#include "menuscreen.h"
#include "octo_client.h"
#include "klipper_client.h"
#include "connection.h" // <--- ÚJ: Hálózati menedzser beemelése

class MainScreen {
public:
    MainScreen(TFT_eSPI* tft, CST820* touch, bool octoEnabled, bool klipperEnabled, OctoClient* octoClient);
    void init();
    void draw(const OctoPrinterData& octoData, const KlipperPrinterData& klipperData);
    int handleTouch();

    int getCurrentPage() const { return _currentPage; }
    void goToPage(int page) { _currentPage = page; }

private:
    TFT_eSPI* _tft;
    CST820* _touch;
    bool _octoEnabled;
    bool _klipperEnabled;
    
    int _currentPage;
    int _lastMainPage;
    int _menuState; 
    bool _forceRedraw;
    bool _isTouched;
    bool _isPrinting;

    OctoMenu _octoMenu;
    KlipperMenu _klipperMenu;
    MenuScreen _menuScreen;
    OctoClient* _octoClient;

    float _octoLastNozzle = -999, _octoLastBed = -999;
    int _octoLastProgress = -1;
    String _octoLastStatus = "";

    float _klipperLastNozzle = -999, _klipperLastBed = -999;
    int _klipperLastProgress = -1;
    String _klipperLastStatus = "";

    void drawHeader(const String& name, bool isServerConnected); // Módosítva
    void drawOctoPage(const OctoPrinterData& info);
    void drawKlipperPage(const KlipperPrinterData& info);
    void drawPrinterData(String status, float nT, float nTar, float bT, float bTar, int progress, String time, String totalTime,
                     float& lastN, float& lastB, int& lastP, String& lastS);
    void drawMenuButton();
    void drawPrintControls();
    void drawDisabledPage(const String& title);
};

#endif