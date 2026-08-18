#include "octo_menu_mqtt.h"
#include "lang_manager.h"
#include "ui_utils.h"
#include "config_manager.h"
#include <math.h>

OctoMenuMqtt::OctoMenuMqtt(TFT_eSPI* tft) 
    : _tft(tft), _bedLevelMenu(tft), _otherCalibMenu(tft), _controlMenu(tft), _filamentMenu(tft), _tuneMenu(tft) {
    _bedLevelMenu.init();
    _otherCalibMenu.init();
    _controlMenu.init();
    _filamentMenu.init();
    _tuneMenu.init();
}

void OctoMenuMqtt::draw(OctoClientMqtt* client) {
    bool meshSavedActive = client ? client->shouldShowMeshSavedPopup() : false;
    bool noMeshActive = client ? client->shouldShowNoMeshPopup() : false;

    // Ha megszűnt bármelyik popup, azonnal kényszerítsük a menü újrarajzolását!
    if ((_lastMeshSavedPopupState && !meshSavedActive) || 
        (_lastNoMeshPopupState && !noMeshActive)) {
        _forceRedraw = true;
    }

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
                _forceRedraw = true;
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
                    drawPrepareMenu(client);
                } else if (homing) {
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
            _filamentMenu.draw(client);
            break;
        case SUB_TUNE:      
            _tuneMenu.draw(client);
            break;
        case SUB_Z_OFFSET:  
            {
                static bool prevPrepRunning = false;
                static bool prevReady = false;
                bool currPrepRunning = client ? client->isZOffsetPrepRunning() : false;
                bool currReady = client ? client->isZOffsetReady() : _zHomed;

                if (subStateChanged || currPrepRunning != prevPrepRunning || currReady != prevReady) {
                    drawZOffsetMenu(client);
                    prevPrepRunning = currPrepRunning;
                    prevReady = currReady;
                } else if (currPrepRunning) {
                    updateZOffsetPrepButton(client);
                }
            }
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

void OctoMenuMqtt::drawMeshLoadingScreen() {
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

void OctoMenuMqtt::updateMeshButtons(OctoClientMqtt* client) {
    ThemeColors theme = getCurrentTheme();
    bool building = client ? client->isMeshBuilding() : false;
    uint16_t btnColor = building ? getPulsingColor() : theme.cardBg;

    UIUtils::drawButton(_tft, 15, 75, 135, 40, building ? LangManager::get("octo_btn_measuring") : LangManager::get("octo_btn_build_3x3"), btnColor, theme.text, false, 2, 5);
    UIUtils::drawButton(_tft, 170, 75, 135, 40, building ? LangManager::get("octo_btn_measuring") : LangManager::get("octo_btn_build_5x5"), btnColor, theme.text, false, 2, 5);
}

void OctoMenuMqtt::updateZOffsetPrepButton(OctoClientMqtt* client, bool force) {
    static unsigned long lastPulseUpdate = 0;
    bool isPrepRunning = client ? client->isZOffsetPrepRunning() : false;

    if (!isPrepRunning) return;

    unsigned long now = millis();
    if (force || now - lastPulseUpdate > 80) {
        lastPulseUpdate = now;
        uint16_t btnBg = getPulsingColor();
        UIUtils::drawButton(_tft, 20, 75, 280, 40, LangManager::get("zoffset_positioning"), btnBg, TFT_BLACK, false, 2, 5);
    }
}

void OctoMenuMqtt::updatePrepareMenu(OctoClientMqtt* client, bool force) {
    ThemeColors theme = getCurrentTheme();
    static unsigned long lastPulseUpdate = 0;
    static uint16_t lastBtnColor = 0;

    bool homing = client ? client->isHoming() : false;
    uint16_t btnColor = homing ? getPulsingColor() : theme.cardBg;

    unsigned long now = millis();
    if (force || now - lastPulseUpdate > 80 || btnColor != lastBtnColor) {
        lastPulseUpdate = now;
        lastBtnColor = btnColor;

        String homeTxt = homing ? LangManager::get("octo_btn_homing") : LangManager::get("octo_btn_autohome");

        UIUtils::drawButton(_tft, 20, 75, 280, 40, homeTxt, btnColor, theme.text, false, 2, 5);
    }
}

void OctoMenuMqtt::drawUnsupportedPopup() {
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

void OctoMenuMqtt::drawNoMeshPopup() {
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

uint16_t OctoMenuMqtt::getPulsingColor() {
    float factor = (sin(millis() / 180.0) + 1.0) / 2.0;
    uint8_t r = 100 + (uint8_t)(155 * factor);
    uint8_t g = 30 + (uint8_t)(110 * factor);
    return _tft->color565(r, g, 0);
}

void OctoMenuMqtt::drawMeshSavedPopup() {
    _tft->fillRoundRect(35, 75, 250, 105, 8, _tft->color565(15, 35, 15));
    _tft->drawRoundRect(35, 75, 250, 105, 8, TFT_GREEN);
    _tft->drawRoundRect(36, 76, 248, 103, 7, TFT_GREEN);

    _tft->setTextDatum(MC_DATUM);
    _tft->setTextColor(TFT_GREEN, _tft->color565(15, 35, 15));
    _tft->drawString(LangManager::get("popup_meshsaved_title"), 160, 110, 4);

    _tft->setTextColor(TFT_WHITE, _tft->color565(15, 35, 15));
    _tft->drawString(LangManager::get("popup_meshsaved_line1"), 160, 148, 2);
}

void OctoMenuMqtt::drawMainMenu() {
    ThemeColors theme = getCurrentTheme();
    _tft->fillRect(10, 45, 300, 190, theme.bg);
    _tft->setTextColor(theme.accent, theme.bg);
    _tft->setTextDatum(TC_DATUM);
    _tft->drawString(LangManager::get("octo_menu_mqtt_title"), 160, 48, 2);

    UIUtils::drawButton(_tft, 20, 75, 130, 55, LangManager::get("octo_btn_prepare"), theme.cardBg, theme.text, false, 2, 5);
    UIUtils::drawButton(_tft, 170, 75, 130, 55, LangManager::get("octo_btn_control"), theme.cardBg, theme.text, false, 2, 5);
    UIUtils::drawButton(_tft, 20, 138, 130, 55, LangManager::get("octo_btn_calibration"), theme.cardBg, theme.text, false, 2, 5);
    UIUtils::drawButton(_tft, 170, 138, 130, 55, LangManager::get("octo_btn_filament_btn"), theme.cardBg, theme.text, false, 2, 5);

    UIUtils::drawButton(_tft, 20, 204, 280, 26, LangManager::get("btn_back"), theme.cardBg, theme.text, false, 1, 4);
}

void OctoMenuMqtt::drawPrepareMenu(OctoClientMqtt* client) {
    ThemeColors theme = getCurrentTheme();
    _tft->fillRect(10, 45, 300, 190, theme.bg);
    _tft->setTextColor(theme.accent, theme.bg);
    _tft->setTextDatum(TC_DATUM);
    _tft->drawString(LangManager::get("octo_menu_prepare_title"), 160, 48, 2);

    updatePrepareMenu(client, true);
    UIUtils::drawButton(_tft, 20, 120, 280, 35, LangManager::get("octo_btn_steppers_off"), theme.cardBg, theme.text, false, 2, 5);
    
    UIUtils::drawButton(_tft, 20, 160, 280, 35, LangManager::get("octo_btn_cooldown"), theme.cardBg, theme.text, false, 2, 5);

    UIUtils::drawButton(_tft, 20, 204, 280, 26, LangManager::get("btn_back"), theme.cardBg, theme.text, false, 1, 4);
}

void OctoMenuMqtt::drawCalibrationMenu() {
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

void OctoMenuMqtt::drawZOffsetMenu(OctoClientMqtt* client) {
    ThemeColors theme = getCurrentTheme();
    _tft->fillRect(10, 45, 300, 190, theme.bg);
    _tft->setTextColor(theme.accent, theme.bg);
    _tft->setTextDatum(TC_DATUM);
    _tft->drawString(LangManager::get("octo_menu_zoffset_title"), 160, 48, 2);

    bool isReady = client ? client->isZOffsetReady() : _zHomed;
    bool isPrepRunning = client ? client->isZOffsetPrepRunning() : false;

    if (!isReady) {
        uint16_t btnBg = isPrepRunning ? getPulsingColor() : TFT_ORANGE;
        uint16_t btnTxt = TFT_BLACK;
        String btnText = isPrepRunning ? LangManager::get("zoffset_positioning") : LangManager::get("zoffset_start_calib");

        UIUtils::drawButton(_tft, 20, 75, 280, 40, btnText, btnBg, btnTxt, false, 2, 5);

        _tft->fillRoundRect(20, 125, 280, 70, 6, theme.cardBg);
        _tft->setTextColor(isPrepRunning ? theme.accent : TFT_RED, theme.cardBg);
        _tft->setTextDatum(MC_DATUM);
        
        if (isPrepRunning) {
            _tft->drawString(LangManager::get("zoffset_g28_g1_progress"), 160, 148, 1);
            _tft->drawString(LangManager::get("zoffset_wait_ok"), 160, 168, 1);
        } else {
            _tft->drawString(LangManager::get("zoffset_start_req"), 160, 145, 2);
            _tft->drawString(LangManager::get("zoffset_press_btn_above"), 160, 168, 1);
        }

        UIUtils::drawButton(_tft, 20, 204, 280, 26, LangManager::get("btn_back"), theme.cardBg, theme.text, false, 1, 4);
    } else {
        UIUtils::drawButton(_tft, 15, 65, 60, 45, "-", theme.cardBg, theme.text, false, 4, 4);

        _tft->fillRoundRect(82, 65, 156, 45, 6, theme.cardBg);
        _tft->drawRoundRect(82, 65, 156, 45, 6, theme.accent);
        _tft->setTextColor(theme.accent, theme.cardBg);
        _tft->setTextDatum(MC_DATUM);
        String valStr = (_zOffsetDelta >= 0 ? "+" : "") + String(_zOffsetDelta, 3) + " mm";
        _tft->drawString(valStr, 160, 82, 4);
        _tft->setTextColor(theme.subText, theme.cardBg);
        _tft->drawString(LangManager::get("zoffset_mod_label"), 160, 100, 1);

        UIUtils::drawButton(_tft, 245, 65, 60, 45, "+", theme.cardBg, theme.text, false, 4, 4);

        auto drawStepBtn = [&](int x, float stepVal, const char* label) {
            bool isActive = (fabs(_zStepSize - stepVal) < 0.001f);
            uint16_t bg = isActive ? theme.accent : theme.cardBg;
            uint16_t txt = isActive ? theme.bg : theme.text;
            UIUtils::drawButton(_tft, x, 118, 64, 32, label, bg, txt, false, 2, 4);
        };

        drawStepBtn(15,  0.01f, "0.01");
        drawStepBtn(89,  0.05f, "0.05");
        drawStepBtn(163, 0.10f, "0.10");
        drawStepBtn(237, 0.50f, "0.50");

        UIUtils::drawButton(_tft, 15, 158, 290, 36, LangManager::get("zoffset_save_rehome"), TFT_DARKGREEN, TFT_WHITE, false, 2, 5);

        UIUtils::drawButton(_tft, 15, 202, 290, 28, LangManager::get("btn_back"), theme.cardBg, theme.text, false, 1, 4);
    }
}

void OctoMenuMqtt::drawMeshMenu(OctoClientMqtt* client) {
    ThemeColors theme = getCurrentTheme();
    _tft->fillRect(10, 45, 300, 190, theme.bg);
    _tft->setTextColor(theme.accent, theme.bg);
    _tft->setTextDatum(TC_DATUM);
    _tft->drawString(LangManager::get("octo_menu_mesh_title"), 160, 48, 2);

    updateMeshButtons(client);

    UIUtils::drawButton(_tft, 20, 128, 280, 40, LangManager::get("octo_btn_show_mesh"), theme.cardBg, theme.text, false, 2, 5);

    UIUtils::drawButton(_tft, 20, 190, 280, 32, LangManager::get("btn_back"), theme.cardBg, theme.text, false, 1, 5);
}

uint16_t OctoMenuMqtt::getMeshColor(float val) {
    if (abs(val) < 0.015f) return _tft->color565(40, 40, 40);

    if (val > 0) {
        uint8_t intensity = (uint8_t)min(255.0f, val * 1200.0f);
        return _tft->color565(0, intensity / 2, intensity);
    } else {
        uint8_t intensity = (uint8_t)min(255.0f, abs(val) * 1200.0f);
        return _tft->color565(intensity, 0, 0);
    }
}

void OctoMenuMqtt::drawShowMeshMenu(OctoClientMqtt* client) {
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

int OctoMenuMqtt::handleTouch(uint16_t x, uint16_t y, OctoClientMqtt* client) {
    ThemeColors theme = getCurrentTheme();

    if (_showUnsupportedPopup) {
        _showUnsupportedPopup = false;
        _forceRedraw = true;
        draw(client);
        return 1;
    }

    if (client && client->shouldShowMeshSavedPopup()) {
        client->dismissMeshSavedPopup();
        _forceRedraw = true;
        draw(client);
        return 1;
    }

    if (client && client->shouldShowNoMeshPopup()) {
        client->dismissNoMeshPopup();
        _forceRedraw = true;
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
                drawPrepareMenu(client);
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
                _filamentMenu.open();
                _filamentMenu.draw(client);
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
        
        if (y >= 75 && y <= 115) {
            if (client && client->isHoming()) {
                return 1; 
            }
            
            if (client) {
                UIUtils::pressFeedback(_tft, 20, 75, 280, 40, LangManager::get("octo_btn_autohome"), theme.cardBg, theme.text, 2, 5);
                client->autoHome(); 
                updatePrepareMenu(client, true);
            }
            return 1;
        }

        if (y >= 120 && y <= 155) {
            if (client) {
                UIUtils::pressFeedback(_tft, 20, 120, 280, 35, LangManager::get("octo_btn_steppers_off"), theme.cardBg, theme.text, 2, 5);
                client->disableSteppers();
            }
            return 1;
        }

        if (y >= 160 && y <= 195) {
            if (client) {
                UIUtils::pressFeedback(_tft, 20, 160, 280, 35, LangManager::get("octo_btn_cooldown"), theme.cardBg, theme.text, 2, 5);
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
                _zHomed = false;
                _zOffsetDelta = 0.000f;
                if (client) client->resetZOffsetPrep();
                drawZOffsetMenu(client);
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
        int result = _filamentMenu.handleTouch(x, y, client);
        if (result == 0) {
            _subState = SUB_MAIN;
            drawMainMenu();
            return 1;
        }
        return result;
    }

    if (_subState == SUB_TUNE) {
        int result = _tuneMenu.handleTouch(x, y, client);
        if (result == 0) {
            _subState = SUB_MAIN;
            drawMainMenu();
            return 0;
        }
        return result;
    }

    if (_subState == SUB_Z_OFFSET) {
        bool isReady = client ? client->isZOffsetReady() : _zHomed;
        bool isPrepRunning = client ? client->isZOffsetPrepRunning() : false;

        if (!isReady) {
            if (y >= 200) { 
                UIUtils::pressFeedback(_tft, 20, 204, 280, 26, LangManager::get("btn_back"), theme.cardBg, theme.text, 1, 4);
                _subState = SUB_CALIBRATION; 
                _zHomed = false; 
                if (client) client->resetZOffsetPrep();
                drawCalibrationMenu(); 
                return 1; 
            } 

            if (y >= 75 && y <= 115) { 
                if (client && !isPrepRunning) {
                    UIUtils::pressFeedback(_tft, 20, 75, 280, 40, LangManager::get("zoffset_positioning_short"), theme.cardBg, theme.text, 2, 5);
                    client->startZOffsetPrep(); 
                    _zOffsetDelta = 0.000f;
                    drawZOffsetMenu(client); 
                }
                return 1; 
            }

            return 1; 
        }

        if (y >= 200) { 
            UIUtils::pressFeedback(_tft, 15, 202, 290, 28, LangManager::get("btn_back"), theme.cardBg, theme.text, 1, 4);
            _subState = SUB_CALIBRATION; 
            _zHomed = false; 
            if (client) client->resetZOffsetPrep();
            drawCalibrationMenu(); 
            return 1; 
        }

        if (y >= 65 && y <= 110 && x >= 15 && x <= 75) {
            if (client) {
                UIUtils::pressFeedback(_tft, 15, 65, 60, 45, "-", theme.cardBg, theme.text, 4, 4);
                client->adjustZOffset(-_zStepSize);
                _zOffsetDelta -= _zStepSize;
                drawZOffsetMenu(client);
            }
            return 1;
        }

        if (y >= 65 && y <= 110 && x >= 245 && x <= 305) {
            if (client) {
                UIUtils::pressFeedback(_tft, 245, 65, 60, 45, "+", theme.cardBg, theme.text, 4, 4);
                client->adjustZOffset(_zStepSize);
                _zOffsetDelta += _zStepSize;
                drawZOffsetMenu(client);
            }
            return 1;
        }

        if (y >= 118 && y <= 150) {
            if (x >= 15 && x <= 79)        { _zStepSize = 0.01f; drawZOffsetMenu(client); return 1; }
            else if (x >= 89 && x <= 153)  { _zStepSize = 0.05f; drawZOffsetMenu(client); return 1; }
            else if (x >= 163 && x <= 227) { _zStepSize = 0.10f; drawZOffsetMenu(client); return 1; }
            else if (x >= 237 && x <= 301) { _zStepSize = 0.50f; drawZOffsetMenu(client); return 1; }
        }

        if (y >= 158 && y <= 195) { 
            if (client) {
                UIUtils::pressFeedback(_tft, 15, 158, 290, 36, LangManager::get("zoffset_saving"), TFT_DARKGREEN, TFT_WHITE, 2, 5);
                client->saveConfig(); 
                client->autoHome(); 
                client->resetZOffsetPrep();
                _zHomed = false;
                drawZOffsetMenu(client);
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