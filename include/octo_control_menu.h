#ifndef OCTO_CONTROL_MENU_H
#define OCTO_CONTROL_MENU_H

#include <TFT_eSPI.h>
#include "octo_client.h"

class OctoControlMenu {
public:
    OctoControlMenu(TFT_eSPI* tft);
    void init();
    void draw(OctoClient* client);
    int handleTouch(uint16_t x, uint16_t y, OctoClient* client);
    void forceRedraw() { _forceRedraw = true; }
    void open() { _forceRedraw = true; _subState = 0; }

private:
    TFT_eSPI* _tft;
    int _subState = 0; // 0: Fő grid, 1: Mozgás, 2: Hőmérséklet, 3: BL/CR touch, 4: Hűtés
    bool _forceRedraw = true;

    void drawMainMenu();
    void drawSubMenu();
};

#endif