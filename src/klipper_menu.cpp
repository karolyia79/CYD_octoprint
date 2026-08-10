#include "klipper_menu.h"
#include "lang_manager.h"
#include "ui_utils.h"

KlipperMenu::KlipperMenu(TFT_eSPI* tft) : _tft(tft) {}

void KlipperMenu::draw(KlipperClient* client) {
    static int lastSubState = -1;
    bool subStateChanged = (_subState != lastSubState) || _forceRedraw;
    lastSubState = _subState;

    if (_subState == 1) {
        drawTuneMenu(client);
    } else if (subStateChanged) {
        drawMainMenu();
    }

    _forceRedraw = false;
}

void KlipperMenu::drawMainMenu() {
    _tft->fillRect(10, 45, 300, 190, TFT_BLACK);
    _tft->setTextColor(TFT_CYAN, TFT_BLACK);
    _tft->setTextDatum(TC_DATUM);
    _tft->drawString(LangManager::get("klipper_menu_title"), 160, 50, 2);

    // 1. Gomb: Makrók indítása
    UIUtils::drawButton(_tft, 20, 80, 280, 38, LangManager::get("klipper_btn_macros"), _tft->color565(50, 50, 50), TFT_WHITE, false, 2, 5);

    // 2. Gomb: Hangolás (Tune / Z-offset)
    UIUtils::drawButton(_tft, 20, 128, 280, 38, LangManager::get("klipper_btn_tune"), _tft->color565(50, 50, 50), TFT_WHITE, false, 2, 5);

    // Vissza gomb
    UIUtils::drawButton(_tft, 20, 185, 280, 38, LangManager::get("btn_back"), _tft->color565(100, 30, 30), TFT_WHITE, false, 2, 5);
}

void KlipperMenu::drawTuneRow(int y, const String& label, const String& valueStr) {
    // [-] gomb
    UIUtils::drawButton(_tft, 15, y, 45, 30, "-", _tft->color565(60, 60, 60), TFT_WHITE, false, 4, 4);

    // Érték kijelző
    _tft->fillRoundRect(65, y, 190, 30, 4, _tft->color565(35, 35, 35));
    _tft->setTextColor(TFT_GREENYELLOW, _tft->color565(35, 35, 35));
    _tft->setTextDatum(MC_DATUM);
    _tft->drawString(label + ": " + valueStr, 160, y + 15, 2);

    // [+] gomb
    UIUtils::drawButton(_tft, 260, y, 45, 30, "+", _tft->color565(60, 60, 60), TFT_WHITE, false, 4, 4);
}

void KlipperMenu::drawTuneMenu(KlipperClient* client) {
    _tft->fillRect(10, 45, 300, 190, TFT_BLACK);
    _tft->setTextColor(TFT_GREEN, TFT_BLACK);
    _tft->setTextDatum(TC_DATUM);
    _tft->drawString("KLIPPER TUNE", 160, 48, 2);

    int spd = client ? client->getData().speed : 100;
    int nzl = client ? (int)client->getData().nozzleTarget : 0;
    int bed = client ? (int)client->getData().bedTarget : 0;

    drawTuneRow(72,  "Speed",  String(spd) + "%");
    drawTuneRow(106, "Nozzle", String(nzl) + "C");
    drawTuneRow(140, "Bed",    String(bed) + "C");

    // Z-Offset sor
    UIUtils::drawButton(_tft, 15, 174, 45, 30, "-0.01", _tft->color565(60, 60, 60), TFT_WHITE, false, 1, 4);

    _tft->fillRoundRect(65, 174, 190, 30, 4, _tft->color565(35, 35, 35));
    _tft->setTextColor(TFT_GREENYELLOW, _tft->color565(35, 35, 35));
    _tft->setTextDatum(MC_DATUM);
    _tft->drawString("Z-Offset Adj.", 160, 189, 2);

    UIUtils::drawButton(_tft, 260, 174, 45, 30, "+0.01", _tft->color565(60, 60, 60), TFT_WHITE, false, 1, 4);

    // Vissza gomb a Klipper főmenübe
    UIUtils::drawButton(_tft, 20, 208, 280, 24, LangManager::get("btn_back"), _tft->color565(100, 30, 30), TFT_WHITE, false, 1, 4);
}

int KlipperMenu::handleTouch(uint16_t x, uint16_t y, KlipperClient* client) {
    // 1. FŐMENÜ ÁLLAPOT (_subState == 0)
    if (_subState == 0) {
        if (y >= 185 && y <= 223 && x >= 20 && x <= 300) {
            UIUtils::pressFeedback(_tft, 20, 185, 280, 38, LangManager::get("btn_back"), _tft->color565(100, 30, 30), TFT_WHITE, 2, 5);
            return 0; // Vissza a főképernyőre
        }
        if (y >= 80 && y <= 118 && x >= 20 && x <= 300) {
            UIUtils::pressFeedback(_tft, 20, 80, 280, 38, LangManager::get("klipper_btn_macros"), _tft->color565(50, 50, 50), TFT_WHITE, 2, 5);
            return 30; // Makrók indítása
        }
        if (y >= 128 && y <= 166 && x >= 20 && x <= 300) {
            UIUtils::pressFeedback(_tft, 20, 128, 280, 38, LangManager::get("klipper_btn_tune"), _tft->color565(50, 50, 50), TFT_WHITE, 2, 5);
            _subState = 1;          // ÁTVÁLTÁS TUNE NÉZETRE!
            _forceRedraw = true;
            drawTuneMenu(client);   // AZONNALI KIRAJZOLÁS
            return 1;
        }
        return -1;
    }

    // 2. TUNE ÁLLAPOT (_subState == 1)
    if (_subState == 1) {
        // Vissza gomb -> visszalép a Klipper főmenübe
        if (y >= 208) {
            UIUtils::pressFeedback(_tft, 20, 208, 280, 24, LangManager::get("btn_back"), _tft->color565(100, 30, 30), TFT_WHITE, 1, 4);
            _subState = 0;
            _forceRedraw = true;
            drawMainMenu();
            return 1;
        }

        if (!client) return 1;
        const KlipperPrinterData& data = client->getData();

        // Speed (- / +)
        if (y >= 72 && y <= 102) {
            if (x >= 15 && x <= 60) { 
                UIUtils::pressFeedback(_tft, 15, 72, 45, 30, "-", _tft->color565(60, 60, 60), TFT_WHITE, 4, 4);
                client->setSpeed(data.speed - 1); 
                drawTuneMenu(client); 
            }
            else if (x >= 260 && x <= 305) { 
                UIUtils::pressFeedback(_tft, 260, 72, 45, 30, "+", _tft->color565(60, 60, 60), TFT_WHITE, 4, 4);
                client->setSpeed(data.speed + 1); 
                drawTuneMenu(client); 
            }
        }
        // Nozzle (- / +)
        else if (y >= 106 && y <= 136) {
            if (x >= 15 && x <= 60) { 
                UIUtils::pressFeedback(_tft, 15, 106, 45, 30, "-", _tft->color565(60, 60, 60), TFT_WHITE, 4, 4);
                client->setNozzleTarget(data.nozzleTarget - 1); 
                drawTuneMenu(client); 
            }
            else if (x >= 260 && x <= 305) { 
                UIUtils::pressFeedback(_tft, 260, 106, 45, 30, "+", _tft->color565(60, 60, 60), TFT_WHITE, 4, 4);
                client->setNozzleTarget(data.nozzleTarget + 1); 
                drawTuneMenu(client); 
            }
        }
        // Bed (- / +)
        else if (y >= 140 && y <= 170) {
            if (x >= 15 && x <= 60) { 
                UIUtils::pressFeedback(_tft, 15, 140, 45, 30, "-", _tft->color565(60, 60, 60), TFT_WHITE, 4, 4);
                client->setBedTarget(data.bedTarget - 1); 
                drawTuneMenu(client); 
            }
            else if (x >= 260 && x <= 305) { 
                UIUtils::pressFeedback(_tft, 260, 140, 45, 30, "+", _tft->color565(60, 60, 60), TFT_WHITE, 4, 4);
                client->setBedTarget(data.bedTarget + 1); 
                drawTuneMenu(client); 
            }
        }
        // Z-Offset (- / + 0.01mm)
        else if (y >= 174 && y <= 204) {
            if (x >= 15 && x <= 60) { 
                UIUtils::pressFeedback(_tft, 15, 174, 45, 30, "-0.01", _tft->color565(60, 60, 60), TFT_WHITE, 1, 4);
                client->adjustZOffset(-0.01f); 
                drawTuneMenu(client); 
            }
            else if (x >= 260 && x <= 305) { 
                UIUtils::pressFeedback(_tft, 260, 174, 45, 30, "+0.01", _tft->color565(60, 60, 60), TFT_WHITE, 1, 4);
                client->adjustZOffset(0.01f);  
                drawTuneMenu(client); 
            }
        }
        return 1;
    }

    return -1;
}