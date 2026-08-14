#include "octo_menu.h"
#include "lang_manager.h"
#include "ui_utils.h"
#include "config_manager.h"
#include <math.h>

OctoMenu::OctoMenu(TFT_eSPI* tft) : _tft(tft), _bedLevelMenu(tft), _otherCalibMenu(tft), _controlMenu(tft) {
    _bedLevelMenu.init();
    _otherCalibMenu.init();
    _controlMenu.init();
}

void OctoMenu::draw(OctoClient* client) {
    bool meshSavedActive = client ? client->shouldShowMeshSavedPopup() : false;
    bool noMeshActive = client ? client->shouldShowNoMeshPopup() : false;

    if (meshSavedActive || noMeshActive || _showUnsupportedPopup) {
        
        if (meshSavedActive && (!_lastMeshSavedPopupState || _forceRedraw)) {
            drawMeshSavedPopup();
        }
        _lastMeshSavedPopupState = meshSavedActive;

        if (noMeshActive && (!_lastNoMeshPopupState || _forceRedraw)) {
            drawNoMeshPopup();
        }
        _lastNoMeshPopupState = noMeshActive;

        if (_showUnsupportedPopup) {
            if (millis() - _unsupportedPopupStartMs > 3500) {
                _showUnsupportedPopup = false; 
            }
        }

        if (_showUnsupportedPopup && (!_lastUnsupportedPopupState || _forceRedraw)) {
            drawUnsupportedPopup();
        } 
        else if (!_showUnsupportedPopup && _lastUnsupportedPopupState) {
            if (_subState == SUB_MESH) drawMeshMenu(client);
        }
        _lastUnsupportedPopupState = _showUnsupportedPopup;

        _forceRedraw = false;
        return; 
    }

    _lastMeshSavedPopupState = false;
    _lastNoMeshPopupState = false;
    _lastUnsupportedPopupState = false;

    static int lastSubState = -1;
    bool subStateChanged = (_subState != lastSubState) || _forceRedraw;
    lastSubState = _subState;

    switch (_subState) {
        case SUB_PREPARE:   
            {
                static bool prevHoming = false;
                bool homing = client ? client->isHoming() : false;

                if (subStateChanged || (prevHoming && !homing)) {
                    drawPrepareMenu();
                }

                if (homing) {
                    updatePrepareMenu(client); 
                }
                prevHoming = homing;
            }
            break;

        case SUB_CONTROL:   
            _controlMenu.draw(client);
            break;
        case SUB_CALIBRATION: 
            if (subStateChanged) drawCalibrationMenu(); 
            break;
        case SUB_FILAMENT:  
            if (subStateChanged) drawFilamentMenu(); 
            break;
        case SUB_TUNE:      
            {
                static int lastSpd = -1;
                static int lastNzl = -1;
                static int lastBed = -1;

                int spd = client ? client->getData().speed : 100;
                int nzl = client ? (int)client->getData().nozzleTarget : 0;
                int bed = client ? (int)client->getData().bedTarget : 0;

                if (subStateChanged || spd != lastSpd || nzl != lastNzl || bed != lastBed) {
                    drawTuneMenu(client);
                    lastSpd = spd;
                    lastNzl = nzl;
                    lastBed = bed;
                }
            } 
            break;
        case SUB_Z_OFFSET:  
            if (subStateChanged) drawZOffsetMenu(); 
            break;
        case SUB_MESH:      
            {
                static bool prevBuilding = false;
                bool building = client ? client->isMeshBuilding() : false;

                if (subStateChanged || (prevBuilding && !building)) {
                    drawMeshMenu(client);
                } else if (building) {
                    updateMeshButtons(client);
                }
                prevBuilding = building;
            }
            break;
        case SUB_SHOW_MESH: 
            {
                static bool meshWasLoaded = false;
                bool currentLoaded = client ? client->getData().meshLoaded : false;

                if (subStateChanged || _forceRedraw || (!meshWasLoaded && currentLoaded)) {
                    if (currentLoaded) {
                        drawShowMeshMenu(client);
                    } else {
                        drawMeshLoadingScreen();
                    }
                }
                meshWasLoaded = currentLoaded;
            }
            break;
        case SUB_BED_LEVEL: 
            _bedLevelMenu.draw(client);
            break;
        case SUB_OTHER_CALIB:
            _otherCalibMenu.draw(client);
            break;
        default:            
            if (subStateChanged) drawMainMenu(); 
            break;
    }

    _forceRedraw = false;
}

void OctoMenu::drawMeshLoadingScreen() {
    ThemeColors theme = getCurrentTheme();
    _tft->fillRect(10, 45, 300, 190, theme.bg);
    _tft->setTextColor(theme.accent, theme.bg);
    _tft->setTextDatum(TC_DATUM);
    _tft->drawString(LangManager::get("octo_title_bed_mesh"), 160, 48, 2);

    _tft->setTextColor(theme.text, theme.bg);
    _tft->setTextDatum(MC_DATUM);
    _tft->drawString(LangManager::get("octo_msg_downloading_mesh"), 160, 120, 2);

    UIUtils::drawButton(_tft, 20, 204, 280, 24, LangManager::get("btn_back"), theme.cardBg, theme.text, false, 1, 4);
}

void OctoMenu::updateMeshButtons(OctoClient* client) {
    ThemeColors theme = getCurrentTheme();
    bool building = client ? client->isMeshBuilding() : false;
    uint16_t btnColor = building ? getPulsingColor() : theme.cardBg;

    UIUtils::drawButton(_tft, 15, 75, 135, 40, building ? LangManager::get("octo_btn_measuring") : LangManager::get("octo_btn_build_3x3"), btnColor, theme.text, false, 2, 5);
    UIUtils::drawButton(_tft, 170, 75, 135, 40, building ? LangManager::get("octo_btn_measuring") : LangManager::get("octo_btn_build_5x5"), btnColor, theme.text, false, 2, 5);
}

void OctoMenu::updatePrepareMenu(OctoClient* client) {
    ThemeColors theme = getCurrentTheme();
    static unsigned long lastPulseUpdate = 0;
    static uint16_t lastBtnColor = 0;

    bool homing = client ? client->isHoming() : false;
    uint16_t btnColor = homing ? getPulsingColor() : theme.cardBg;

    unsigned long now = millis();
    if (now - lastPulseUpdate > 80 || btnColor != lastBtnColor) {
        lastPulseUpdate = now;
        lastBtnColor = btnColor;

        UIUtils::drawButton(_tft, 20, 75, 280, 35, homing ? LangManager::get("octo_btn_homing") : LangManager::get("octo_btn_autohome"), btnColor, theme.text, false, 2, 5);
    }
}

void OctoMenu::drawUnsupportedPopup() {
    _tft->fillRoundRect(25, 75, 270, 105, 8, _tft->color565(40, 10, 10));
    _tft->drawRoundRect(25, 75, 270, 105, 8, TFT_ORANGE);
    _tft->drawRoundRect(26, 76, 268, 103, 7, TFT_ORANGE);

    _tft->setTextDatum(MC_DATUM);
    _tft->setTextColor(TFT_ORANGE, _tft->color565(40, 10, 10));
    _tft->drawString(LangManager::get("popup_unsupported_title"), 160, 100, 2);

    _tft->setTextColor(TFT_WHITE, _tft->color565(40, 10, 10));
    _tft->drawString(LangManager::get("popup_unsupported_line1"), 160, 130, 2);
    _tft->drawString(LangManager::get("popup_unsupported_line2"), 160, 150, 2);
}

void OctoMenu::drawNoMeshPopup() {
    _tft->fillRoundRect(25, 75, 270, 105, 8, _tft->color565(40, 10, 10));
    _tft->drawRoundRect(25, 75, 270, 105, 8, TFT_RED);
    _tft->drawRoundRect(26, 76, 268, 103, 7, TFT_RED);

    _tft->setTextDatum(MC_DATUM);
    _tft->setTextColor(TFT_RED, _tft->color565(40, 10, 10));
    _tft->drawString(LangManager::get("popup_nomesh_title"), 160, 100, 2);

    _tft->setTextColor(TFT_WHITE, _tft->color565(40, 10, 10));
    _tft->drawString(LangManager::get("popup_nomesh_line1"), 160, 130, 2);
    _tft->drawString(LangManager::get("popup_nomesh_line2"), 160, 150, 1);
}

uint16_t OctoMenu::getPulsingColor() {
    float factor = (sin(millis() / 180.0) + 1.0) / 2.0;
    uint8_t r = 100 + (uint8_t)(155 * factor);
    uint8_t g = 30 + (uint8_t)(110 * factor);
    return _tft->color565(r, g, 0);
}

void OctoMenu::drawMeshSavedPopup() {
    _tft->fillRoundRect(35, 75, 250, 105, 8, _tft->color565(15, 35, 15));
    _tft->drawRoundRect(35, 75, 250, 105, 8, TFT_GREEN);
    _tft->drawRoundRect(36, 76, 248, 103, 7, TFT_GREEN);

    _tft->setTextDatum(MC_DATUM);
    _tft->setTextColor(TFT_GREEN, _tft->color565(15, 35, 15));
    _tft->drawString(LangManager::get("popup_meshsaved_title"), 160, 110, 4);

    _tft->setTextColor(TFT_WHITE, _tft->color565(15, 35, 15));
    _tft->drawString(LangManager::get("popup_meshsaved_line1"), 160, 148, 2);
}

void OctoMenu::drawMainMenu() {
    ThemeColors theme = getCurrentTheme();
    _tft->fillRect(10, 45, 300, 190, theme.bg);
    _tft->setTextColor(theme.accent, theme.bg);
    _tft->setTextDatum(TC_DATUM);
    _tft->drawString(LangManager::get("octo_menu_title"), 160, 48, 2);

    UIUtils::drawButton(_tft, 20, 75, 130, 55, LangManager::get("octo_btn_prepare"), theme.cardBg, theme.text, false, 2, 5);
    UIUtils::drawButton(_tft, 170, 75, 130, 55, LangManager::get("octo_btn_control"), theme.cardBg, theme.text, false, 2, 5);
    UIUtils::drawButton(_tft, 20, 138, 130, 55, LangManager::get("octo_btn_calibration"), theme.cardBg, theme.text, false, 2, 5);
    UIUtils::drawButton(_tft, 170, 138, 130, 55, LangManager::get("octo_btn_filament_btn"), theme.cardBg, theme.text, false, 2, 5);

    UIUtils::drawButton(_tft, 20, 204, 280, 26, LangManager::get("btn_back"), theme.cardBg, theme.text, false, 1, 4);
}

void OctoMenu::drawPrepareMenu() {
    ThemeColors theme = getCurrentTheme();
    _tft->fillRect(10, 45, 300, 190, theme.bg);
    _tft->setTextColor(theme.accent, theme.bg);
    _tft->setTextDatum(TC_DATUM);
    _tft->drawString(LangManager::get("octo_menu_prepare_title"), 160, 48, 2);

    UIUtils::drawButton(_tft, 20, 75, 280, 35, LangManager::get("octo_btn_autohome"), theme.cardBg, theme.text, false, 2, 5);
    UIUtils::drawButton(_tft, 20, 118, 280, 35, LangManager::get("octo_btn_steppers_off"), theme.cardBg, theme.text, false, 2, 5);
    UIUtils::drawButton(_tft, 20, 161, 280, 35, LangManager::get("octo_btn_cooldown"), TFT_MAROON, TFT_WHITE, false, 2, 5);

    UIUtils::drawButton(_tft, 20, 204, 280, 26, LangManager::get("btn_back"), theme.cardBg, theme.text, false, 1, 4);
}

void OctoMenu::drawCalibrationMenu() {
    ThemeColors theme = getCurrentTheme();
    _tft->fillRect(10, 45, 300, 190, theme.bg);
    _tft->setTextColor(theme.accent, theme.bg);
    _tft->setTextDatum(TC_DATUM);
    _tft->drawString(LangManager::get("octo_menu_calib_title"), 160, 48, 2);

    UIUtils::drawButton(_tft, 20, 75, 130, 55, LangManager::get("octo_btn_zoffset"), theme.cardBg, theme.text, false, 2, 5);
    UIUtils::drawButton(_tft, 170, 75, 130, 55, LangManager::get("octo_btn_bedmesh"), theme.cardBg, theme.text, false, 2, 5);
    UIUtils::drawButton(_tft, 20, 138, 130, 55, LangManager::get("octo_btn_leveling"), theme.cardBg, theme.text, false, 2, 5);
    UIUtils::drawButton(_tft, 170, 138, 130, 55, LangManager::get("octo_btn_other"), theme.cardBg, theme.text, false, 2, 5);

    UIUtils::drawButton(_tft, 20, 204, 280, 26, LangManager::get("btn_back"), theme.cardBg, theme.text, false, 1, 4);
}

void OctoMenu::drawFilamentMenu() {
    ThemeColors theme = getCurrentTheme();
    _tft->fillRect(10, 45, 300, 190, theme.bg);
    _tft->setTextColor(theme.accent, theme.bg);
    _tft->setTextDatum(TC_DATUM);
    _tft->drawString(LangManager::get("octo_menu_filament_title"), 160, 48, 2);

    _tft->setTextColor(theme.subText, theme.bg);
    _tft->setTextDatum(MC_DATUM);
    _tft->drawString(LangManager::get("octo_coming_soon"), 160, 120, 2);

    UIUtils::drawButton(_tft, 20, 204, 280, 26, LangManager::get("btn_back"), theme.cardBg, theme.text, false, 1, 4);
}

void OctoMenu::drawTuneRow(int y, const String& label, const String& valueStr) {
    ThemeColors theme = getCurrentTheme();
    UIUtils::drawButton(_tft, 15, y, 45, 30, "-", theme.cardBg, theme.text, false, 4, 4);

    _tft->fillRoundRect(65, y, 190, 30, 4, theme.cardBg);
    _tft->setTextColor(theme.accent, theme.cardBg);
    _tft->setTextDatum(MC_DATUM);
    _tft->drawString(label + ": " + valueStr, 160, y + 15, 2);

    UIUtils::drawButton(_tft, 260, y, 45, 30, "+", theme.cardBg, theme.text, false, 4, 4);
}

void OctoMenu::drawTuneMenu(OctoClient* client) {
    ThemeColors theme = getCurrentTheme();
    _tft->fillRect(10, 45, 300, 190, theme.bg);
    _tft->setTextColor(theme.accent, theme.bg);
    _tft->setTextDatum(TC_DATUM);
    _tft->drawString(LangManager::get("octo_menu_tune_title_http"), 160, 48, 2);

    int spd = client ? client->getData().speed : 100;
    int nzl = client ? (int)client->getData().nozzleTarget : 0;
    int bed = client ? (int)client->getData().bedTarget : 0;

    drawTuneRow(72,  LangManager::get("octo_label_speed"),   String(spd) + "%");
    drawTuneRow(106, LangManager::get("main_screen_nozzle"), String(nzl) + "C");
    drawTuneRow(140, LangManager::get("main_screen_bed"),    String(bed) + "C");

    UIUtils::drawButton(_tft, 15, 174, 45, 30, "-0.01", theme.cardBg, theme.text, false, 1, 4);

    _tft->fillRoundRect(65, 174, 190, 30, 4, theme.cardBg);
    _tft->setTextColor(theme.accent, theme.cardBg);
    _tft->setTextDatum(MC_DATUM);
    _tft->drawString(LangManager::get("octo_label_zoffset_adj"), 160, 189, 2);

    UIUtils::drawButton(_tft, 260, 174, 45, 30, "+0.01", theme.cardBg, theme.text, false, 1, 4);

    UIUtils::drawButton(_tft, 20, 208, 280, 24, LangManager::get("btn_back"), theme.cardBg, theme.text, false, 1, 4);
}

void OctoMenu::drawZOffsetMenu() {
    ThemeColors theme = getCurrentTheme();
    _tft->fillRect(10, 45, 300, 190, theme.bg);
    _tft->setTextColor(theme.accent, theme.bg);
    _tft->setTextDatum(TC_DATUM);
    _tft->drawString(LangManager::get("octo_menu_zoffset_title"), 160, 48, 2);

    UIUtils::drawButton(_tft, 20, 75, 280, 34, LangManager::get("octo_btn_zhome"), theme.cardBg, theme.text, false, 2, 5);

    UIUtils::drawButton(_tft, 15, 118, 55, 34, "-0.01", theme.cardBg, theme.text, false, 1, 4);

    _tft->fillRoundRect(75, 118, 170, 34, 4, theme.cardBg);
    _tft->setTextColor(theme.accent, theme.cardBg);
    _tft->setTextDatum(MC_DATUM);
    _tft->drawString(LangManager::get("octo_label_zoffset_adj"), 160, 135, 2);

    UIUtils::drawButton(_tft, 250, 118, 55, 34, "+0.01", theme.cardBg, theme.text, false, 1, 4);

    UIUtils::drawButton(_tft, 20, 160, 280, 34, LangManager::get("octo_btn_save"), TFT_DARKGREEN, TFT_WHITE, false, 2, 5);

    UIUtils::drawButton(_tft, 20, 200, 280, 28, LangManager::get("btn_back"), theme.cardBg, theme.text, false, 1, 5);
}

void OctoMenu::drawMeshMenu(OctoClient* client) {
    ThemeColors theme = getCurrentTheme();
    _tft->fillRect(10, 45, 300, 190, theme.bg);
    _tft->setTextColor(theme.accent, theme.bg);
    _tft->setTextDatum(TC_DATUM);
    _tft->drawString(LangManager::get("octo_menu_mesh_title"), 160, 48, 2);

    updateMeshButtons(client);

    UIUtils::drawButton(_tft, 20, 128, 280, 40, LangManager::get("octo_btn_show_mesh"), theme.cardBg, theme.text, false, 2, 5);

    UIUtils::drawButton(_tft, 20, 190, 280, 32, LangManager::get("btn_back"), theme.cardBg, theme.text, false, 1, 5);
}

uint16_t OctoMenu::getMeshColor(float val) {
    if (abs(val) < 0.015f) return _tft->color565(40, 40, 40);

    if (val > 0) {
        uint8_t intensity = (uint8_t)min(255.0f, val * 1200.0f);
        return _tft->color565(0, intensity / 2, intensity);
    } else {
        uint8_t intensity = (uint8_t)min(255.0f, abs(val) * 1200.0f);
        return _tft->color565(intensity, 0, 0);
    }
}

void OctoMenu::drawShowMeshMenu(OctoClient* client) {
    ThemeColors theme = getCurrentTheme();
    _tft->fillRect(10, 45, 300, 190, theme.bg);
    _tft->setTextColor(theme.accent, theme.bg);
    _tft->setTextDatum(TC_DATUM);

    const auto& data = client ? client->getData() : OctoPrinterData();
    int rows = data.meshRows > 0 ? data.meshRows : 3;
    int cols = data.meshCols > 0 ? data.meshCols : 3;

    String title = LangManager::get("octo_title_bed_mesh") + " (" + String(rows) + "x" + String(cols) + ")";
    _tft->drawString(title, 160, 47, 1);

    int availW = 290;
    int availH = 135;
    int startX_base = 15;
    int startY_base = 63;

    int gap = (cols > 6 || rows > 6) ? 1 : 3;
    int cellW = (availW - (cols - 1) * gap) / cols;
    int cellH = (availH - (rows - 1) * gap) / rows;

    int totalW = cols * cellW + (cols - 1) * gap;
    int totalH = rows * cellH + (rows - 1) * gap;
    int offsetX = startX_base + (availW - totalW) / 2;
    int offsetY = startY_base + (availH - totalH) / 2;

    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            float val = data.bedMesh[r][c];
            uint16_t bgCol = getMeshColor(val);
            int x = offsetX + c * (cellW + gap);
            int y = offsetY + r * (cellH + gap);

            _tft->fillRect(x, y, cellW, cellH, bgCol);
            
            if (cellW > 8 && cellH > 8) {
                _tft->drawRect(x, y, cellW, cellH, TFT_WHITE);
            }

            if (cellW >= 32 && cellH >= 16) {
                _tft->setTextColor(TFT_WHITE, bgCol);
                _tft->setTextDatum(MC_DATUM);

                String valStr;
                if (cellW < 48) {
                    valStr = String(val, 2);
                } else {
                    valStr = (val >= 0 ? "+" : "") + String(val, 2);
                }
                _tft->drawString(valStr, x + cellW / 2, y + cellH / 2, 1);
            }
        }
    }

    UIUtils::drawButton(_tft, 20, 204, 280, 24, LangManager::get("btn_back"), theme.cardBg, theme.text, false, 1, 4);
}

int OctoMenu::handleTouch(uint16_t x, uint16_t y, OctoClient* client) {
    ThemeColors theme = getCurrentTheme();

    if (_showUnsupportedPopup) {
        _showUnsupportedPopup = false;
        draw(client);
        return 1;
    }

    if (client && client->shouldShowMeshSavedPopup()) {
        client->dismissMeshSavedPopup();
        draw(client);
        return 1;
    }

    if (client && client->shouldShowNoMeshPopup()) {
        client->dismissNoMeshPopup();
        draw(client);
        return 1;
    }

    if (_subState == SUB_MAIN) {
        if (y >= 195) {
            UIUtils::pressFeedback(_tft, 20, 204, 280, 26, LangManager::get("btn_back"), theme.cardBg, theme.text, 1, 4);
            return 0; 
        }
        
        if (y >= 75 && y <= 130) {
            if (x >= 20 && x <= 150) {
                UIUtils::pressFeedback(_tft, 20, 75, 130, 55, LangManager::get("octo_btn_prepare"), theme.cardBg, theme.text, 2, 5);
                _subState = SUB_PREPARE;
                drawPrepareMenu();
                return 1;
            } else if (x >= 170 && x <= 300) {
                UIUtils::pressFeedback(_tft, 170, 75, 130, 55, LangManager::get("octo_btn_control"), theme.cardBg, theme.text, 2, 5);
                _subState = SUB_CONTROL;
                _controlMenu.open();
                return 1;
            }
        } else if (y >= 138 && y <= 193) {
            if (x >= 20 && x <= 150) {
                UIUtils::pressFeedback(_tft, 20, 138, 130, 55, LangManager::get("octo_btn_calibration"), theme.cardBg, theme.text, 2, 5);
                _subState = SUB_CALIBRATION;
                drawCalibrationMenu();
                return 1;
            } else if (x >= 170 && x <= 300) {
                UIUtils::pressFeedback(_tft, 170, 138, 130, 55, LangManager::get("octo_btn_filament_btn"), theme.cardBg, theme.text, 2, 5);
                _subState = SUB_FILAMENT;
                drawFilamentMenu();
                return 1;
            }
        }
        return -1;
    }

    if (_subState == SUB_PREPARE) {
        if (y >= 200) { 
            UIUtils::pressFeedback(_tft, 20, 204, 280, 26, LangManager::get("btn_back"), theme.cardBg, theme.text, 1, 4);
            _subState = SUB_MAIN; 
            drawMainMenu(); 
            return 1; 
        }
        
        if (y >= 75 && y <= 110) {
            if (client && client->isHoming()) {
                return 1; 
            }
            
            if (client) {
                UIUtils::pressFeedback(_tft, 20, 75, 280, 35, LangManager::get("octo_btn_autohome"), theme.cardBg, theme.text, 2, 5);
                client->autoHome(); 
                updatePrepareMenu(client);
            }
            return 1;
        }

        if (y >= 118 && y <= 153) {
            if (client) {
                UIUtils::pressFeedback(_tft, 20, 118, 280, 35, LangManager::get("octo_btn_steppers_off"), theme.cardBg, theme.text, 2, 5);
                client->disableSteppers();
            }
            return 1;
        }

        if (y >= 161 && y <= 196) {
            if (client) {
                UIUtils::pressFeedback(_tft, 20, 161, 280, 35, LangManager::get("octo_btn_cooldown"), TFT_MAROON, TFT_WHITE, false, 2, 5);
                client->setNozzleTarget(0);
                client->setBedTarget(0);
            }
            return 1;
        }

        return -1;
    }

    if (_subState == SUB_CALIBRATION) {
        if (y >= 200) { 
            UIUtils::pressFeedback(_tft, 20, 204, 280, 26, LangManager::get("btn_back"), theme.cardBg, theme.text, 1, 4);
            _subState = SUB_MAIN; 
            drawMainMenu(); 
            return 1; 
        }
        
        if (y >= 75 && y <= 130) {
            if (x >= 20 && x <= 150) {
                UIUtils::pressFeedback(_tft, 20, 75, 130, 55, LangManager::get("octo_btn_zoffset"), theme.cardBg, theme.text, 2, 5);
                _subState = SUB_Z_OFFSET;
                drawZOffsetMenu();
                return 1;
            } else if (x >= 170 && x <= 300) {
                UIUtils::pressFeedback(_tft, 170, 75, 130, 55, LangManager::get("octo_btn_bedmesh"), theme.cardBg, theme.text, 2, 5);
                _subState = SUB_MESH;
                drawMeshMenu(client);
                return 1;
            }
        } else if (y >= 138 && y <= 193) {
            if (x >= 20 && x <= 150) {
                UIUtils::pressFeedback(_tft, 20, 138, 130, 55, LangManager::get("octo_btn_leveling"), theme.cardBg, theme.text, 2, 5);
                _subState = SUB_BED_LEVEL;
                _bedLevelMenu.open();
                return 1;
            } else if (x >= 170 && x <= 300) {
                UIUtils::pressFeedback(_tft, 170, 138, 130, 55, LangManager::get("octo_btn_other"), theme.cardBg, theme.text, 2, 5);
                _subState = SUB_OTHER_CALIB;
                _otherCalibMenu.open();
                return 1;
            }
        }
        return -1;
    }

    if (_subState == SUB_CONTROL) {
        int result = _controlMenu.handleTouch(x, y, client);
        if (result == 0) {
            _subState = SUB_MAIN;
            drawMainMenu();
            return 1;
        }
        return result;
    }

    if (_subState == SUB_FILAMENT) {
        if (y >= 200) { 
            UIUtils::pressFeedback(_tft, 20, 204, 280, 26, LangManager::get("btn_back"), theme.cardBg, theme.text, 1, 4);
            _subState = SUB_MAIN; 
            drawMainMenu(); 
            return 1; 
        }
        return -1;
    }

    if (_subState == SUB_TUNE) {
        if (y >= 208) {
            UIUtils::pressFeedback(_tft, 20, 208, 280, 24, LangManager::get("btn_back"), theme.cardBg, theme.text, 1, 4);
            return 0; 
        }
        if (!client) return 1;
        const OctoPrinterData& data = client->getData();

        if (y >= 72 && y <= 102) {
            if (x >= 15 && x <= 60) { 
                UIUtils::pressFeedback(_tft, 15, 72, 45, 30, "-", theme.cardBg, theme.text, 4, 4);
                client->setSpeed(data.speed - 1); 
                drawTuneMenu(client); 
            }
            else if (x >= 260 && x <= 305) { 
                UIUtils::pressFeedback(_tft, 260, 72, 45, 30, "+", theme.cardBg, theme.text, 4, 4);
                client->setSpeed(data.speed + 1); 
                drawTuneMenu(client); 
            }
        } else if (y >= 106 && y <= 136) {
            if (x >= 15 && x <= 60) { 
                UIUtils::pressFeedback(_tft, 15, 106, 45, 30, "-", theme.cardBg, theme.text, 4, 4);
                client->setNozzleTarget(data.nozzleTarget - 1); 
                drawTuneMenu(client); 
            }
            else if (x >= 260 && x <= 305) { 
                UIUtils::pressFeedback(_tft, 260, 106, 45, 30, "+", theme.cardBg, theme.text, 4, 4);
                client->setNozzleTarget(data.nozzleTarget + 1); 
                drawTuneMenu(client); 
            }
        } else if (y >= 140 && y <= 170) {
            if (x >= 15 && x <= 60) { 
                UIUtils::pressFeedback(_tft, 15, 140, 45, 30, "-", theme.cardBg, theme.text, 4, 4);
                client->setBedTarget(data.bedTarget - 1); 
                drawTuneMenu(client); 
            }
            else if (x >= 260 && x <= 305) { 
                UIUtils::pressFeedback(_tft, 260, 140, 45, 30, "+", theme.cardBg, theme.text, 4, 4);
                client->setBedTarget(data.bedTarget + 1); 
                drawTuneMenu(client); 
            }
        } else if (y >= 174 && y <= 204) {
            if (x >= 15 && x <= 60) { 
                UIUtils::pressFeedback(_tft, 15, 174, 45, 30, "-0.01", theme.cardBg, theme.text, 1, 4);
                client->adjustZOffset(-0.01f); 
                drawTuneMenu(client); 
            }
            else if (x >= 260 && x <= 305) { 
                UIUtils::pressFeedback(_tft, 260, 174, 45, 30, "+0.01", theme.cardBg, theme.text, 1, 4);
                client->adjustZOffset(0.01f); 
                drawTuneMenu(client); 
            }
        }
        return 1;
    }

    if (_subState == SUB_Z_OFFSET) {
        if (y >= 200) { 
            UIUtils::pressFeedback(_tft, 20, 200, 280, 28, LangManager::get("btn_back"), theme.cardBg, theme.text, 1, 5);
            _subState = SUB_CALIBRATION; 
            drawCalibrationMenu(); 
            return 1; 
        } 
        if (y >= 75 && y <= 109) { 
            if (client) {
                UIUtils::pressFeedback(_tft, 20, 75, 280, 34, LangManager::get("octo_btn_zhome"), theme.cardBg, theme.text, 2, 5);
                client->homeZ(); 
            }
            return 1; 
        }
        if (y >= 118 && y <= 152) {
            if (x >= 15 && x <= 70) { 
                if (client) {
                    UIUtils::pressFeedback(_tft, 15, 118, 55, 34, "-0.01", theme.cardBg, theme.text, 1, 4);
                    client->adjustZOffset(-0.01f); 
                }
            }
            if (x >= 250 && x <= 305) { 
                if (client) {
                    UIUtils::pressFeedback(_tft, 250, 118, 55, 34, "+0.01", theme.cardBg, theme.text, 1, 4);
                    client->adjustZOffset(0.01f); 
                }
            }
            return 1;
        }
        if (y >= 160 && y <= 194) { 
            if (client) {
                UIUtils::pressFeedback(_tft, 20, 160, 280, 34, LangManager::get("octo_btn_save"), TFT_DARKGREEN, TFT_WHITE, false, 2, 5);
                client->saveConfig(); 
            }
            return 1; 
        }
        return -1;
    }

    if (_subState == SUB_MESH) {
        if (y >= 185) {
            UIUtils::pressFeedback(_tft, 20, 190, 280, 32, LangManager::get("btn_back"), theme.cardBg, theme.text, 1, 5);
            _subState = SUB_CALIBRATION; 
            drawCalibrationMenu();
            return 1;
        }
        if (y >= 75 && y <= 115) {
            if (client && client->isMeshBuilding()) {
                return 1;
            }

            if (x >= 15 && x <= 150) {
                UIUtils::pressFeedback(_tft, 15, 75, 135, 40, LangManager::get("octo_btn_build_3x3"), theme.cardBg, theme.text, 2, 5);
                if (client) client->autoBuildMesh(3);
                updateMeshButtons(client);
            } else if (x >= 170 && x <= 305) {
                UIUtils::pressFeedback(_tft, 170, 75, 135, 40, LangManager::get("octo_btn_build_5x5"), theme.cardBg, theme.text, 2, 5);
                if (client && !client->supportsCustomMesh()) {
                    _showUnsupportedPopup = true;
                    _unsupportedPopupStartMs = millis();
                    client->autoBuildMesh(3); 
                    updateMeshButtons(client);
                } else if (client) {
                    client->autoBuildMesh(5);
                    updateMeshButtons(client);
                }
            }
            return 1;
        }
        if (y >= 128 && y <= 168) {
            if (client) {
                UIUtils::pressFeedback(_tft, 20, 128, 280, 40, LangManager::get("octo_btn_show_mesh"), theme.cardBg, theme.text, 2, 5);
                client->fetchBedMesh();
                if (client->shouldShowNoMeshPopup()) {
                    draw(client);
                } else {
                    _subState = SUB_SHOW_MESH;
                }
            }
            return 1;
        }
        return -1;
    }

    if (_subState == SUB_SHOW_MESH) {
        if (y >= 204) {
            UIUtils::pressFeedback(_tft, 20, 204, 280, 24, LangManager::get("btn_back"), theme.cardBg, theme.text, 1, 4);
            _subState = SUB_MESH;
            drawMeshMenu(client);
            return 1;
        }
        return -1;
    }

    if (_subState == SUB_BED_LEVEL) {
        int result = _bedLevelMenu.handleTouch(x, y, client);
        
        if (result == 0) {
            _subState = SUB_CALIBRATION;
            drawCalibrationMenu();
            return 1;
        }
        return result;
    }

    if (_subState == SUB_OTHER_CALIB) {
        int result = _otherCalibMenu.handleTouch(x, y, client);
        
        if (result == 0) {
            _subState = SUB_CALIBRATION;
            drawCalibrationMenu();
            return 1;
        }
        return result;
    }

    return -1;
}