#ifndef MAINSCREEN_H
#define MAINSCREEN_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <CST820.h>
#include "octo_client.h"
#include "octo_client_mqtt.h"
#include "klipper_client.h"
#include "octo_menu.h"
#include "octo_menu_mqtt.h"
#include "klipper_menu.h"
#include "menuscreen.h"
#include "connection.h"

class MainScreen {
public:
    MainScreen(TFT_eSPI* tft, CST820* touch, bool octoEnabled, bool klipperEnabled, OctoClient* octoBase, OctoClientMqtt* octoMqtt);
    
    void init();
    void draw(const OctoPrinterData& octoData, const KlipperPrinterData& klipperData);
    int handleTouch();

private:
    TFT_eSPI* _tft;
    CST820* _touch;
    
    bool _octoEnabled;
    bool _klipperEnabled;
    OctoClient* _octoClient;
    OctoClientMqtt* _octoMqtt;
    
    int _currentPage = 0; 
    int _menuState = 0;   
    bool _forceRedraw = true;
    bool _isTouched = false;
    bool _isPrinting = false;
    int _lastMainPage = 0;

    float _octoLastNozzle = -999;
    float _octoLastBed = -999;
    int _octoLastProgress = -1;
    String _octoLastStatus = "";

    float _klipperLastNozzle = -999;
    float _klipperLastBed = -999;
    int _klipperLastProgress = -1;
    String _klipperLastStatus = "";

    OctoMenu _octoMenu;
    OctoMenuMqtt _octoMenuMqtt;
    KlipperMenu _klipperMenu;
    MenuScreen _menuScreen;

    void drawHeader(const String& name, bool isServerConnected);
    void drawOctoPage(const OctoPrinterData& info);
    void drawKlipperPage(const KlipperPrinterData& info);
    void drawPrinterData(String status, float nT, float nTar, float bT, float bTar, int progress, String time, String totalTime,
                         float& lastN, float& lastB, int& lastP, String& lastS);

    void drawMenuButton();
    void drawPrintControls();
    void drawDisabledPage(const String& title);
};

#endif