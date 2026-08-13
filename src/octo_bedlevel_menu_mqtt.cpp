#include "octo_bedlevel_menu_mqtt.h"
#include "lang_manager.h"
#include "ui_utils.h"
#include "config_manager.h"

OctoBedLevelMenuMqtt::OctoBedLevelMenuMqtt(TFT_eSPI* tft) : _tft(tft) {}

void OctoBedLevelMenuMqtt::init() {
    _config = OctoConfigManager::loadConfig();
    _isHomed = false;
    _levelingStep = 0;
    _wizProcessing = false;
}

void OctoBedLevelMenuMqtt::draw(OctoClientMqtt* client) {
    if (_subState == 0 && _forceRedraw) {
        drawMainMenu();
    } 
    else if (_subState == 1) {
        drawWizard(client);
    } 
    else if (_subState == 2 && _forceRedraw) {
        drawCoords();
    }
    _forceRedraw = false;
}

void OctoBedLevelMenuMqtt::drawMainMenu() {
    ThemeColors theme = getCurrentTheme();
    _tft->fillRect(10, 45, 300, 190, theme.bg);
    _tft->setTextColor(theme.accent, theme.bg);
    _tft->setTextDatum(TC_DATUM);
    _tft->drawString(LangManager::get("bedlevel_title_mqtt"), 160, 48, 2);

    UIUtils::drawButton(_tft, 20, 75, 280, 45, LangManager::get("bedlevel_btn_wiz"), theme.cardBg, theme.text, false, 2, 5);
    UIUtils::drawButton(_tft, 20, 135, 280, 45, LangManager::get("bedlevel_btn_coords"), theme.cardBg, theme.text, false, 2, 5);
    UIUtils::drawButton(_tft, 20, 204, 280, 26, LangManager::get("btn_back"), theme.cardBg, theme.text, false, 1, 4);
}

void OctoBedLevelMenuMqtt::drawWizard(OctoClientMqtt* client) {
    ThemeColors theme = getCurrentTheme();
    if (_forceRedraw) {
        _tft->fillRect(10, 45, 300, 190, theme.bg);
        _tft->setTextColor(theme.accent, theme.bg);
        _tft->setTextDatum(TC_DATUM);
        _tft->drawString(LangManager::get("bedlevel_wiz_title"), 160, 48, 2);
    }

    if (_wizProcessing) {
        if (millis() - _wizTimer > _wizWaitTime) {
            _wizProcessing = false;
            _forceRedraw = true; 
        }
    }

    uint16_t cornerBg = _isHomed ? _tft->color565(40, 60, 90) : theme.cardBg;
    uint16_t cornerTx = _isHomed ? TFT_WHITE : theme.subText;

    if (_forceRedraw || _wizProcessing) {
        UIUtils::drawButton(_tft, 20, 68, 70, 38, "BL", cornerBg, cornerTx, false, 2, 5);
        UIUtils::drawButton(_tft, 230, 68, 70, 38, "BR", cornerBg, cornerTx, false, 2, 5);
        UIUtils::drawButton(_tft, 20, 114, 70, 38, "FL", cornerBg, cornerTx, false, 2, 5);
        UIUtils::drawButton(_tft, 230, 114, 70, 38, "FR", cornerBg, cornerTx, false, 2, 5);

        UIUtils::drawButton(_tft, 110, 90, 100, 40, "HOME", _tft->color565(120, 60, 0), TFT_WHITE, false, 2, 5);

        String wizText = LangManager::get("bedlevel_wiz_process");
        uint16_t wizColor = theme.cardBg;

        if (!_wizProcessing) {
            switch (_levelingStep) {
                case 0: wizText = LangManager::get("bedlevel_wiz_start"); wizColor = TFT_DARKGREEN; break;
                case 1: wizText = LangManager::get("bedlevel_wiz_next_fr"); wizColor = TFT_ORANGE; break;
                case 2: wizText = LangManager::get("bedlevel_wiz_next_br"); wizColor = TFT_ORANGE; break;
                case 3: wizText = LangManager::get("bedlevel_wiz_next_bl"); wizColor = TFT_ORANGE; break;
                case 4: wizText = LangManager::get("bedlevel_wiz_finish"); wizColor = TFT_BLUE; break;
            }
        }
        UIUtils::drawButton(_tft, 20, 162, 280, 36, wizText, wizColor, TFT_WHITE, false, 2, 5);
        UIUtils::drawButton(_tft, 20, 208, 280, 24, LangManager::get("btn_back"), theme.cardBg, theme.text, false, 1, 4);
    }
}

void OctoBedLevelMenuMqtt::drawCoords() {
    ThemeColors theme = getCurrentTheme();
    _tft->fillRect(10, 45, 300, 190, theme.bg);
    _tft->setTextColor(theme.accent, theme.bg);
    _tft->setTextDatum(TC_DATUM);
    _tft->drawString(LangManager::get("bedlevel_coords_title"), 160, 48, 2);

    String cName = "";
    int cX = 0, cY = 0;
    if (_selectedCorner == 0)      { cName = LangManager::get("bedlevel_corner_fl"); cX = _config.fl_x; cY = _config.fl_y; }
    else if (_selectedCorner == 1) { cName = LangManager::get("bedlevel_corner_fr"); cX = _config.fr_x; cY = _config.fr_y; }
    else if (_selectedCorner == 2) { cName = LangManager::get("bedlevel_corner_br"); cX = _config.br_x; cY = _config.br_y; }
    else if (_selectedCorner == 3) { cName = LangManager::get("bedlevel_corner_bl"); cX = _config.bl_x; cY = _config.bl_y; }

    UIUtils::drawButton(_tft, 20, 70, 40, 35, "<", theme.cardBg, theme.text, false, 2, 5);
    _tft->setTextColor(theme.accent, theme.bg);
    _tft->setTextDatum(MC_DATUM);
    _tft->drawString(cName, 160, 87, 2);
    UIUtils::drawButton(_tft, 260, 70, 40, 35, ">", theme.cardBg, theme.text, false, 2, 5);

    // X
    _tft->setTextColor(theme.text, theme.bg);
    _tft->setTextDatum(ML_DATUM);
    _tft->drawString("X:", 25, 132, 2);
    UIUtils::drawButton(_tft, 50, 115, 45, 35, "-10", _tft->color565(80,30,30), TFT_WHITE, false, 2, 5);
    UIUtils::drawButton(_tft, 100, 115, 40, 35, "-1", theme.cardBg, theme.text, false, 2, 5);
    _tft->setTextColor(TFT_GOLD, theme.bg);
    _tft->setTextDatum(MC_DATUM);
    _tft->drawString(String(cX), 160, 132, 2);
    UIUtils::drawButton(_tft, 180, 115, 40, 35, "+1", theme.cardBg, theme.text, false, 2, 5);
    UIUtils::drawButton(_tft, 225, 115, 45, 35, "+10", _tft->color565(30,80,30), TFT_WHITE, false, 2, 5);

    // Y
    _tft->setTextColor(theme.text, theme.bg);
    _tft->setTextDatum(ML_DATUM);
    _tft->drawString("Y:", 25, 177, 2);
    UIUtils::drawButton(_tft, 50, 160, 45, 35, "-10", _tft->color565(80,30,30), TFT_WHITE, false, 2, 5);
    UIUtils::drawButton(_tft, 100, 160, 40, 35, "-1", theme.cardBg, theme.text, false, 2, 5);
    _tft->setTextColor(TFT_GOLD, theme.bg);
    _tft->setTextDatum(MC_DATUM);
    _tft->drawString(String(cY), 160, 177, 2);
    UIUtils::drawButton(_tft, 180, 160, 40, 35, "+1", theme.cardBg, theme.text, false, 2, 5);
    UIUtils::drawButton(_tft, 225, 160, 45, 35, "+10", _tft->color565(30,80,30), TFT_WHITE, false, 2, 5);

    UIUtils::drawButton(_tft, 20, 208, 280, 24, LangManager::get("btn_back"), theme.cardBg, theme.text, false, 1, 4);
}

int OctoBedLevelMenuMqtt::handleTouch(uint16_t x, uint16_t y, OctoClientMqtt* client) {
    ThemeColors theme = getCurrentTheme();
    if (_subState == 0) {
        if (y >= 200) {
            UIUtils::pressFeedback(_tft, 20, 204, 280, 26, LangManager::get("btn_back"), theme.cardBg, theme.text, 1, 4);
            return 0;
        }
        if (y >= 75 && y <= 120) {
            UIUtils::pressFeedback(_tft, 20, 75, 280, 45, LangManager::get("bedlevel_btn_wiz"), theme.cardBg, theme.text, 2, 5);
            _levelingStep = 0; 
            _wizProcessing = false;
            _subState = 1; 
            _forceRedraw = true;
            return 1;
        }
        if (y >= 135 && y <= 180) {
            UIUtils::pressFeedback(_tft, 20, 135, 280, 45, LangManager::get("bedlevel_btn_coords"), theme.cardBg, theme.text, 2, 5);
            _subState = 2; 
            _forceRedraw = true;
            return 1;
        }
        return -1;
    }

    if (_subState == 1) { // WIZARD
        if (y >= 208) { 
            UIUtils::pressFeedback(_tft, 20, 208, 280, 24, LangManager::get("btn_back"), theme.cardBg, theme.text, 1, 4);
            _subState = 0; 
            _wizProcessing = false; 
            _forceRedraw = true; 
            return 1;
        }

        if (!_wizProcessing && client) {
            if (y >= 162 && y <= 198) {
                if (_levelingStep == 0) {
                    client->sendGcodeCommand("G28");
                    client->sendGcodeCommand("G0 Z15 F600");
                    client->sendGcodeCommand("G0 X" + String(_config.fl_x) + " Y" + String(_config.fl_y) + " F3000");
                    client->sendGcodeCommand("G0 Z0 F600");
                    _wizProcessing = true; _wizTimer = millis(); _wizWaitTime = 12000;
                    _levelingStep = 1; _isHomed = true;
                } else if (_levelingStep == 1) {
                    client->sendGcodeCommand("G0 Z15 F600");
                    client->sendGcodeCommand("G0 X" + String(_config.fr_x) + " Y" + String(_config.fr_y) + " F3000");
                    client->sendGcodeCommand("G0 Z0 F600");
                    _wizProcessing = true; _wizTimer = millis(); _wizWaitTime = 4000;
                    _levelingStep = 2;
                } else if (_levelingStep == 2) {
                    client->sendGcodeCommand("G0 Z15 F600");
                    client->sendGcodeCommand("G0 X" + String(_config.br_x) + " Y" + String(_config.br_y) + " F3000");
                    client->sendGcodeCommand("G0 Z0 F600");
                    _wizProcessing = true; _wizTimer = millis(); _wizWaitTime = 4000;
                    _levelingStep = 3;
                } else if (_levelingStep == 3) {
                    client->sendGcodeCommand("G0 Z15 F600");
                    client->sendGcodeCommand("G0 X" + String(_config.bl_x) + " Y" + String(_config.bl_y) + " F3000");
                    client->sendGcodeCommand("G0 Z0 F600");
                    _wizProcessing = true; _wizTimer = millis(); _wizWaitTime = 4000;
                    _levelingStep = 4;
                } else if (_levelingStep == 4) {
                    client->sendGcodeCommand("G28");
                    _wizProcessing = true; _wizTimer = millis(); _wizWaitTime = 8000;
                    _levelingStep = 0;
                }
                _forceRedraw = true;
                return 1;
            }

            if (y >= 90 && y <= 130 && x >= 110 && x <= 210) {
                UIUtils::pressFeedback(_tft, 110, 90, 100, 40, "HOME", _tft->color565(120, 60, 0), TFT_WHITE, 2, 5);
                client->sendGcodeCommand("G28");
                _isHomed = true;
                _levelingStep = 0; 
                _wizProcessing = true; _wizTimer = millis(); _wizWaitTime = 8000;
                _forceRedraw = true;
                return 1;
            }

            if (_isHomed) {
                auto sendCornerGcode = [&](int cx, int cy, int bx, int by, const String& label) {
                    UIUtils::pressFeedback(_tft, bx, by, 70, 38, label, _tft->color565(40, 60, 90), TFT_WHITE, 2, 5);
                    client->sendGcodeCommand("G0 Z15 F600");
                    client->sendGcodeCommand("G0 X" + String(cx) + " Y" + String(cy) + " F3000");
                    client->sendGcodeCommand("G0 Z0 F600");
                    _wizProcessing = true; _wizTimer = millis(); _wizWaitTime = 4000;
                    _forceRedraw = true;
                };

                if (y >= 68 && y <= 106) {
                    if (x >= 20 && x <= 90) sendCornerGcode(_config.bl_x, _config.bl_y, 20, 68, "BL");
                    if (x >= 230 && x <= 300) sendCornerGcode(_config.br_x, _config.br_y, 230, 68, "BR");
                }
                if (y >= 114 && y <= 152) {
                    if (x >= 20 && x <= 90) sendCornerGcode(_config.fl_x, _config.fl_y, 20, 114, "FL");
                    if (x >= 230 && x <= 300) sendCornerGcode(_config.fr_x, _config.fr_y, 230, 114, "FR");
                }
            }
        }
        return 1; 
    }

    if (_subState == 2) { // COORDS
        if (y >= 200) {
            UIUtils::pressFeedback(_tft, 20, 208, 280, 24, LangManager::get("btn_back"), theme.cardBg, theme.text, 1, 4);
            OctoConfigManager::saveConfig(_config);
            _subState = 0; 
            _forceRedraw = true; 
            return 1;
        }
        
        if (y >= 70 && y <= 105) {
            if (x >= 20 && x <= 60) {
                UIUtils::pressFeedback(_tft, 20, 70, 40, 35, "<", theme.cardBg, theme.text, 2, 5);
                _selectedCorner = (_selectedCorner + 3) % 4;
                _forceRedraw = true;
            }
            if (x >= 260 && x <= 300) {
                UIUtils::pressFeedback(_tft, 260, 70, 40, 35, ">", theme.cardBg, theme.text, 2, 5);
                _selectedCorner = (_selectedCorner + 1) % 4;
                _forceRedraw = true;
            }
            return 1;
        }

        int* activeX = nullptr;
        int* activeY = nullptr;
        if (_selectedCorner == 0)      { activeX = &_config.fl_x; activeY = &_config.fl_y; }
        else if (_selectedCorner == 1) { activeX = &_config.fr_x; activeY = &_config.fr_y; }
        else if (_selectedCorner == 2) { activeX = &_config.br_x; activeY = &_config.br_y; }
        else if (_selectedCorner == 3) { activeX = &_config.bl_x; activeY = &_config.bl_y; }

        if (activeX && activeY) {
            if (y >= 115 && y <= 150) {
                if (x >= 50 && x <= 95)   { UIUtils::pressFeedback(_tft, 50, 115, 45, 35, "-10", _tft->color565(80,30,30), TFT_WHITE, 2, 5); *activeX = max(0, *activeX - 10); _forceRedraw = true; }
                if (x >= 100 && x <= 140) { UIUtils::pressFeedback(_tft, 100, 115, 40, 35, "-1", theme.cardBg, theme.text, 2, 5); *activeX = max(0, *activeX - 1); _forceRedraw = true; }
                if (x >= 180 && x <= 220) { UIUtils::pressFeedback(_tft, 180, 115, 40, 35, "+1", theme.cardBg, theme.text, 2, 5); *activeX = min(350, *activeX + 1); _forceRedraw = true; }
                if (x >= 225 && x <= 270) { UIUtils::pressFeedback(_tft, 225, 115, 45, 35, "+10", _tft->color565(30,80,30), TFT_WHITE, 2, 5); *activeX = min(350, *activeX + 10); _forceRedraw = true; }
                return 1;
            }
            if (y >= 160 && y <= 195) {
                if (x >= 50 && x <= 95)   { UIUtils::pressFeedback(_tft, 50, 160, 45, 35, "-10", _tft->color565(80,30,30), TFT_WHITE, 2, 5); *activeY = max(0, *activeY - 10); _forceRedraw = true; }
                if (x >= 100 && x <= 140) { UIUtils::pressFeedback(_tft, 100, 160, 40, 35, "-1", theme.cardBg, theme.text, 2, 5); *activeY = max(0, *activeY - 1); _forceRedraw = true; }
                if (x >= 180 && x <= 220) { UIUtils::pressFeedback(_tft, 180, 160, 40, 35, "+1", theme.cardBg, theme.text, 2, 5); *activeY = min(350, *activeY + 1); _forceRedraw = true; }
                if (x >= 225 && x <= 270) { UIUtils::pressFeedback(_tft, 225, 160, 45, 35, "+10", _tft->color565(30,80,30), TFT_WHITE, 2, 5); *activeY = min(350, *activeY + 10); _forceRedraw = true; }
                return 1;
            }
        }
    }
    return -1;
}