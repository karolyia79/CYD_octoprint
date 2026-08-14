#include "octo_control_menu.h"
#include "lang_manager.h"
#include "ui_utils.h"
#include "config_manager.h"

OctoControlMenu::OctoControlMenu(TFT_eSPI* tft) : _tft(tft) {}

void OctoControlMenu::init() {
    _subState = 0;
    _forceRedraw = true;
}

void OctoControlMenu::draw(OctoClient* client) {
    static int lastSubState = -1;
    bool subStateChanged = (_subState != lastSubState) || _forceRedraw;
    lastSubState = _subState;

    if (!subStateChanged) return;

    if (_subState == 0) {
        drawMainMenu();
    } else {
        drawSubMenu();
    }
    _forceRedraw = false;
}

void OctoControlMenu::drawMainMenu() {
    ThemeColors theme = getCurrentTheme();
    _tft->fillRect(10, 45, 300, 190, theme.bg);
    _tft->setTextColor(theme.accent, theme.bg);
    _tft->setTextDatum(TC_DATUM);
    _tft->drawString(LangManager::get("octo_menu_control_title"), 160, 48, 2);

    // 2x2-es elrendezés
    UIUtils::drawButton(_tft, 20, 75, 130, 55, "Mozgás", theme.cardBg, theme.text, false, 2, 5);
    UIUtils::drawButton(_tft, 170, 75, 130, 55, "Hőmérséklet", theme.cardBg, theme.text, false, 2, 5);
    UIUtils::drawButton(_tft, 20, 138, 130, 55, "BL/CR touch", theme.cardBg, theme.text, false, 2, 5);
    UIUtils::drawButton(_tft, 170, 138, 130, 55, "Hűtés", theme.cardBg, theme.text, false, 2, 5);

    UIUtils::drawButton(_tft, 20, 204, 280, 26, LangManager::get("btn_back"), theme.cardBg, theme.text, false, 1, 4);
}

void OctoControlMenu::drawSubMenu() {
    ThemeColors theme = getCurrentTheme();
    _tft->fillRect(10, 45, 300, 190, theme.bg);
    _tft->setTextColor(theme.accent, theme.bg);
    _tft->setTextDatum(TC_DATUM);

    String title = "";
    if (_subState == 1) title = "Mozgás";
    else if (_subState == 2) title = "Hőmérséklet";
    else if (_subState == 3) title = "BL/CR touch";
    else if (_subState == 4) title = "Hűtés";

    _tft->drawString(title, 160, 48, 2);

    _tft->setTextColor(theme.subText, theme.bg);
    _tft->setTextDatum(MC_DATUM);
    _tft->drawString(LangManager::get("octo_coming_soon"), 160, 120, 2);

    UIUtils::drawButton(_tft, 20, 204, 280, 26, LangManager::get("btn_back"), theme.cardBg, theme.text, false, 1, 4);
}

int OctoControlMenu::handleTouch(uint16_t x, uint16_t y, OctoClient* client) {
    ThemeColors theme = getCurrentTheme();

    if (y >= 200) {
        UIUtils::pressFeedback(_tft, 20, 204, 280, 26, LangManager::get("btn_back"), theme.cardBg, theme.text, 1, 4);
        if (_subState == 0) {
            return 0; // Vissza a főmenübe
        } else {
            _subState = 0;
            _forceRedraw = true;
            draw(client);
            return 1;
        }
    }

    if (_subState == 0) {
        if (y >= 75 && y <= 130) {
            if (x >= 20 && x <= 150) {
                UIUtils::pressFeedback(_tft, 20, 75, 130, 55, "Mozgás", theme.cardBg, theme.text, 2, 5);
                _subState = 1;
                _forceRedraw = true;
                draw(client);
                return 1;
            } else if (x >= 170 && x <= 300) {
                UIUtils::pressFeedback(_tft, 170, 75, 130, 55, "Hőmérséklet", theme.cardBg, theme.text, 2, 5);
                _subState = 2;
                _forceRedraw = true;
                draw(client);
                return 1;
            }
        } else if (y >= 138 && y <= 193) {
            if (x >= 20 && x <= 150) {
                UIUtils::pressFeedback(_tft, 20, 138, 130, 55, "BL/CR touch", theme.cardBg, theme.text, 2, 5);
                _subState = 3;
                _forceRedraw = true;
                draw(client);
                return 1;
            } else if (x >= 170 && x <= 300) {
                UIUtils::pressFeedback(_tft, 170, 138, 130, 55, "Hűtés", theme.cardBg, theme.text, 2, 5);
                _subState = 4;
                _forceRedraw = true;
                draw(client);
                return 1;
            }
        }
    }

    return -1;
}