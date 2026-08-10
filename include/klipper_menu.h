#ifndef KLIPPER_MENU_H
#define KLIPPER_MENU_H

#include <TFT_eSPI.h>
#include "klipper_client.h"

class KlipperMenu {
public:
    KlipperMenu(TFT_eSPI* tft);
    void draw(KlipperClient* client = nullptr);
    int handleTouch(uint16_t x, uint16_t y, KlipperClient* client = nullptr);
    
    // Állapotváltó metódusok
    void openMainMenu() { _subState = 0; _forceRedraw = true; }
    void openTuneMenu() { _subState = 1; _forceRedraw = true; }
    int getSubState() const { return _subState; }
    void forceRedraw() { _forceRedraw = true; }

private:
    TFT_eSPI* _tft;
    int _subState = 0; // 0: Klipper Főmenü, 1: Tune (Hangolás) Nézet
    bool _forceRedraw = true;

    void drawMainMenu();
    void drawTuneMenu(KlipperClient* client);
    void drawTuneRow(int y, const String& label, const String& valueStr);
};

#endif