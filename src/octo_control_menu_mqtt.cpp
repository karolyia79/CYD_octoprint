#include "octo_control_menu_mqtt.h"
#include "lang_manager.h"
#include "ui_utils.h"
#include "config_manager.h"

OctoControlMenuMqtt::OctoControlMenuMqtt(TFT_eSPI* tft) : _tft(tft) {}

void OctoControlMenuMqtt::init() {
    _subState = 0;
    _forceRedraw = true;
}

void OctoControlMenuMqtt::draw(OctoClientMqtt* client) {
    static int lastSubState = -1;
    bool subStateChanged = (_subState != lastSubState) || _forceRedraw;
    lastSubState = _subState;

    if (!subStateChanged) return;

    if (_subState == 0) {
        drawMainMenu();
    } else if (_subState == 3) {
        drawBltouchMenu();
    } else if (_subState == 4) {
        drawFanMenu();
    } else {
        drawSubMenu();
    }
    _forceRedraw = false;
}

void OctoControlMenuMqtt::drawMainMenu() {
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

void OctoControlMenuMqtt::drawBltouchMenu() {
    ThemeColors theme = getCurrentTheme();
    _tft->fillRect(10, 45, 300, 190, theme.bg);
    _tft->setTextColor(theme.accent, theme.bg);
    _tft->setTextDatum(TC_DATUM);
    _tft->drawString("BL/CR Touch", 160, 48, 2);

    UIUtils::drawButton(_tft, 20, 75, 130, 55, "Pin ki", theme.cardBg, theme.text, false, 2, 5);
    UIUtils::drawButton(_tft, 170, 75, 130, 55, "Pin be", theme.cardBg, theme.text, false, 2, 5);
    UIUtils::drawButton(_tft, 20, 138, 130, 55, "Önteszt", theme.cardBg, theme.text, false, 2, 5);
    UIUtils::drawButton(_tft, 170, 138, 130, 55, "Reset", theme.cardBg, theme.text, false, 2, 5);

    UIUtils::drawButton(_tft, 20, 204, 280, 26, LangManager::get("btn_back"), theme.cardBg, theme.text, false, 1, 4);
}

void OctoControlMenuMqtt::drawFanMenu() {
    ThemeColors theme = getCurrentTheme();
    _tft->fillRect(10, 45, 300, 190, theme.bg);
    _tft->setTextColor(theme.accent, theme.bg);
    _tft->setTextDatum(TC_DATUM);
    _tft->drawString("Hűtés", 160, 48, 2);

    // Gombok színeinek meghatározása az aktív állapot alapján
    uint16_t bg0   = (_fanPercent == 0)   ? theme.accent : theme.cardBg;
    uint16_t txt0  = (_fanPercent == 0)   ? theme.bg     : theme.text;

    uint16_t bg25  = (_fanPercent == 25)  ? theme.accent : theme.cardBg;
    uint16_t txt25 = (_fanPercent == 25)  ? theme.bg     : theme.text;

    uint16_t bg50  = (_fanPercent == 50)  ? theme.accent : theme.cardBg;
    uint16_t txt50 = (_fanPercent == 50)  ? theme.bg     : theme.text;

    uint16_t bg75  = (_fanPercent == 75)  ? theme.accent : theme.cardBg;
    uint16_t txt75 = (_fanPercent == 75)  ? theme.bg     : theme.text;

    uint16_t bg100 = (_fanPercent == 100) ? theme.accent : theme.cardBg;
    uint16_t txt100= (_fanPercent == 100) ? theme.bg     : theme.text;

    // Felső gomb: Ventilátor KI (0%)
    UIUtils::drawButton(_tft, 20, 70, 280, 35, "Ventilátor KI (0%)", bg0, txt0, false, 2, 5);

    // 2x2-es rács gombjai: 25%, 50%, 75%, 100%
    UIUtils::drawButton(_tft, 20, 112, 130, 42, "25%", bg25, txt25, false, 2, 5);
    UIUtils::drawButton(_tft, 170, 112, 130, 42, "50%", bg50, txt50, false, 2, 5);
    UIUtils::drawButton(_tft, 20, 158, 130, 42, "75%", bg75, txt75, false, 2, 5);
    UIUtils::drawButton(_tft, 170, 158, 130, 42, "100%", bg100, txt100, false, 2, 5);

    // Vissza gomb
    UIUtils::drawButton(_tft, 20, 204, 280, 26, LangManager::get("btn_back"), theme.cardBg, theme.text, false, 1, 4);
}

void OctoControlMenuMqtt::drawSubMenu() {
    ThemeColors theme = getCurrentTheme();
    _tft->fillRect(10, 45, 300, 190, theme.bg);
    _tft->setTextColor(theme.accent, theme.bg);
    _tft->setTextDatum(TC_DATUM);

    String title = "";
    if (_subState == 1) title = "Mozgás";
    else if (_subState == 2) title = "Hőmérséklet";

    _tft->drawString(title, 160, 48, 2);

    _tft->setTextColor(theme.subText, theme.bg);
    _tft->setTextDatum(MC_DATUM);
    _tft->drawString(LangManager::get("octo_coming_soon"), 160, 120, 2);

    UIUtils::drawButton(_tft, 20, 204, 280, 26, LangManager::get("btn_back"), theme.cardBg, theme.text, false, 1, 4);
}

int OctoControlMenuMqtt::handleTouch(uint16_t x, uint16_t y, OctoClientMqtt* client) {
    ThemeColors theme = getCurrentTheme();

    // Vissza gomb (y >= 200)
    if (y >= 200) {
        UIUtils::pressFeedback(_tft, 20, 204, 280, 26, LangManager::get("btn_back"), theme.cardBg, theme.text, 1, 4);
        if (_subState == 0) {
            return 0; // Vissza a Control főmenübe
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
    } else if (_subState == 3) {
        if (!client) return 1;

        if (y >= 75 && y <= 130) {
            if (x >= 20 && x <= 150) {
                UIUtils::pressFeedback(_tft, 20, 75, 130, 55, "Pin ki", theme.cardBg, theme.text, 2, 5);
                client->sendGcodeCommand("M280 P0 S10");
                return 1;
            } else if (x >= 170 && x <= 300) {
                UIUtils::pressFeedback(_tft, 170, 75, 130, 55, "Pin be", theme.cardBg, theme.text, 2, 5);
                client->sendGcodeCommand("M280 P0 S90");
                return 1;
            }
        } else if (y >= 138 && y <= 193) {
            if (x >= 20 && x <= 150) {
                UIUtils::pressFeedback(_tft, 20, 138, 130, 55, "Önteszt", theme.cardBg, theme.text, 2, 5);
                client->sendGcodeCommand("M280 P0 S120");
                return 1;
            } else if (x >= 170 && x <= 300) {
                UIUtils::pressFeedback(_tft, 170, 138, 130, 55, "Reset", theme.cardBg, theme.text, 2, 5);
                client->sendGcodeCommand("M280 P0 S160");
                return 1;
            }
        }
    } else if (_subState == 4) {
        // Ventilátor KI gomb (y: 70..105, x: 20..300)
        if (y >= 70 && y <= 105 && x >= 20 && x <= 300) {
            UIUtils::pressFeedback(_tft, 20, 70, 280, 35, "Ventilátor KI (0%)", theme.accent, theme.bg, 2, 5);
            _fanPercent = 0;
            if (client) client->sendGcodeCommand("M106 S0");
            _forceRedraw = true;
            draw(client);
            return 1;
        }

        // 25% gomb (y: 112..154, x: 20..150)
        if (y >= 112 && y <= 154 && x >= 20 && x <= 150) {
            UIUtils::pressFeedback(_tft, 20, 112, 130, 42, "25%", theme.accent, theme.bg, 2, 5);
            _fanPercent = 25;
            int pwmVal = map(25, 0, 100, 0, 255);
            if (client) client->sendGcodeCommand("M106 S" + String(pwmVal));
            _forceRedraw = true;
            draw(client);
            return 1;
        }

        // 50% gomb (y: 112..154, x: 170..300)
        if (y >= 112 && y <= 154 && x >= 170 && x <= 300) {
            UIUtils::pressFeedback(_tft, 170, 112, 130, 42, "50%", theme.accent, theme.bg, 2, 5);
            _fanPercent = 50;
            int pwmVal = map(50, 0, 100, 0, 255);
            if (client) client->sendGcodeCommand("M106 S" + String(pwmVal));
            _forceRedraw = true;
            draw(client);
            return 1;
        }

        // 75% gomb (y: 158..200, x: 20..150)
        if (y >= 158 && y <= 200 && x >= 20 && x <= 150) {
            UIUtils::pressFeedback(_tft, 20, 158, 130, 42, "75%", theme.accent, theme.bg, 2, 5);
            _fanPercent = 75;
            int pwmVal = map(75, 0, 100, 0, 255);
            if (client) client->sendGcodeCommand("M106 S" + String(pwmVal));
            _forceRedraw = true;
            draw(client);
            return 1;
        }

        // 100% gomb (y: 158..200, x: 170..300)
        if (y >= 158 && y <= 200 && x >= 170 && x <= 300) {
            UIUtils::pressFeedback(_tft, 170, 158, 130, 42, "100%", theme.accent, theme.bg, 2, 5);
            _fanPercent = 100;
            int pwmVal = map(100, 0, 100, 0, 255);
            if (client) client->sendGcodeCommand("M106 S" + String(pwmVal));
            _forceRedraw = true;
            draw(client);
            return 1;
        }
    }

    return -1;
}