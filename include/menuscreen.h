#ifndef MENUSCREEN_H
#define MENUSCREEN_H

#include <TFT_eSPI.h>
#include <CST820.h>
#include "config_manager.h"

class MenuScreen {
private:
    TFT_eSPI* _tft;
    CST820* _touch;
    uint8_t _currentSubMenu; 
    bool _isTouched;
    PrinterConfig _config;

    bool _mainMenuButtonsDrawn;
    uint16_t _lastOctoColor;
    uint16_t _lastKlipperColor;
    String _lastOctoStr;
    String _lastKlipperStr;
    uint8_t _lastSubMenuChecked;

    bool _pOctoEnabled, _pOctoConn, _pOctoPrint;
    bool _pKlipperEnabled, _pKlipperConn, _pKlipperPrint;

    void drawMainMenu(bool octoEnabled, bool octoConn, bool octoPrint, bool klipperEnabled, bool klipperConn, bool klipperPrint);
    void drawWifiMenu();
    void drawLanguageMenu();
    void drawSkinMenu();
    void drawSystemMenu();
    void drawDisplayMenu();
    void drawInfoMenu(); 
    void drawMenuButton(int x, int y, int w, int h, const String& text, uint16_t bgColor, uint16_t textColor = TFT_WHITE);
    void drawServerStatusBars(bool octoEnabled, bool octoConn, bool octoPrint, bool klipperEnabled, bool klipperConn, bool klipperPrint, bool forceRedraw = false);

public:
    MenuScreen(TFT_eSPI* tft, CST820* touch);
    void init();
    void draw(bool octoEnabled = false, bool octoConn = false, bool octoPrint = false, 
              bool klipperEnabled = false, bool klipperConn = false, bool klipperPrint = false);
    bool handleClick(uint16_t x, uint16_t y);
    uint8_t getCurrentSubMenu() const { return _currentSubMenu; }
};

#endif