#include "octo_menu.h"
#include "lang_manager.h"
#include "ui_utils.h"
#include <math.h>

OctoMenu::OctoMenu(TFT_eSPI* tft) : _tft(tft), _bedLevelMenu(tft), _otherCalibMenu(tft) {
    _bedLevelMenu.init();
    _otherCalibMenu.init();
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
            if (subStateChanged) drawControlMenu(); 
            break;
        case SUB_CALIBRATION: 
            if (subStateChanged) drawCalibrationMenu(); 
            break;
        case SUB_FILAMENT:  
            if (subStateChanged) drawFilamentMenu(); 
            break;
        case SUB_TUNE:      
            drawTuneMenu(client); 
            break;
        case SUB_Z_OFFSET:  
            if (subStateChanged) drawZOffsetMenu(); 
            break;
        case SUB_MESH:      
            if (subStateChanged) {
                drawMeshMenu(client);
            } else if (client && client->isMeshBuilding()) {
                updateMeshButtons(client);
            }
            break;
        case SUB_SHOW_MESH: 
            drawShowMeshMenu(client); 
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

void OctoMenu::updateMeshButtons(OctoClient* client) {
    bool building = client ? client->isMeshBuilding() : false;
    uint16_t btnColor = building ? getPulsingColor() : _tft->color565(50, 50, 50);

    UIUtils::drawButton(_tft, 15, 75, 135, 40, building ? "Meres..." : "Build 3x3", btnColor, TFT_WHITE, false, 2, 5);
    UIUtils::drawButton(_tft, 170, 75, 135, 40, building ? "Meres..." : "Build 5x5", btnColor, TFT_WHITE, false, 2, 5);
}

void OctoMenu::updatePrepareMenu(OctoClient* client) {
    static unsigned long lastPulseUpdate = 0;
    static uint16_t lastBtnColor = 0;

    bool homing = client ? client->isHoming() : false;
    uint16_t btnColor = homing ? getPulsingColor() : _tft->color565(50, 50, 50);

    unsigned long now = millis();
    if (now - lastPulseUpdate > 80 || btnColor != lastBtnColor) {
        lastPulseUpdate = now;
        lastBtnColor = btnColor;

        UIUtils::drawButton(_tft, 20, 75, 280, 40, homing ? "Home folyamatban..." : "Auto Home (G28)", btnColor, TFT_WHITE, false, 2, 5);
    }
}

void OctoMenu::drawUnsupportedPopup() {
    _tft->fillRoundRect(25, 75, 270, 105, 8, _tft->color565(40, 10, 10));
    _tft->drawRoundRect(25, 75, 270, 105, 8, TFT_ORANGE);
    _tft->drawRoundRect(26, 76, 268, 103, 7, TFT_ORANGE);

    _tft->setTextDatum(MC_DATUM);
    _tft->setTextColor(TFT_ORANGE, _tft->color565(40, 10, 10));
    _tft->drawString("NEM TAMOGATOTT!", 160, 100, 2);

    _tft->setTextColor(TFT_WHITE, _tft->color565(40, 10, 10));
    _tft->drawString("A gep nem ismeri az 5x5-ot.", 160, 130, 2);
    _tft->drawString("3x3-as meres indul...", 160, 150, 2);
}

void OctoMenu::drawNoMeshPopup() {
    _tft->fillRoundRect(25, 75, 270, 105, 8, _tft->color565(40, 10, 10));
    _tft->drawRoundRect(25, 75, 270, 105, 8, TFT_RED);
    _tft->drawRoundRect(26, 76, 268, 103, 7, TFT_RED);

    _tft->setTextDatum(MC_DATUM);
    _tft->setTextColor(TFT_RED, _tft->color565(40, 10, 10));
    _tft->drawString("NINCS MESH!", 160, 100, 2);

    _tft->setTextColor(TFT_WHITE, _tft->color565(40, 10, 10));
    _tft->drawString("Elobb letre kell hozni", 160, 130, 2);
    _tft->drawString("egy bed mesht az EEPROM-ban!", 160, 150, 1);
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
    _tft->drawString("MESH SAVED!", 160, 110, 4);

    _tft->setTextColor(TFT_WHITE, _tft->color565(15, 35, 15));
    _tft->drawString("EEPROM & Cooldown OK", 160, 148, 2);
}

void OctoMenu::drawMainMenu() {
    _tft->fillRect(10, 45, 300, 190, TFT_BLACK);
    _tft->setTextColor(TFT_YELLOW, TFT_BLACK);
    _tft->setTextDatum(TC_DATUM);
    _tft->drawString("-- OCTOPRINT MENU --", 160, 48, 2);

    UIUtils::drawButton(_tft, 20, 75, 130, 55, "Elokeszites", _tft->color565(40, 40, 60), TFT_WHITE, false, 2, 5);
    UIUtils::drawButton(_tft, 170, 75, 130, 55, "Iranyitas", _tft->color565(40, 40, 60), TFT_WHITE, false, 2, 5);
    UIUtils::drawButton(_tft, 20, 138, 130, 55, "Kalibracio", _tft->color565(40, 40, 60), TFT_WHITE, false, 2, 5);
    UIUtils::drawButton(_tft, 170, 138, 130, 55, "Filament", _tft->color565(40, 40, 60), TFT_WHITE, false, 2, 5);

    UIUtils::drawButton(_tft, 20, 204, 280, 26, LangManager::get("btn_back"), _tft->color565(100, 30, 30), TFT_WHITE, false, 1, 4);
}

void OctoMenu::drawPrepareMenu() {
    _tft->fillRect(10, 45, 300, 190, TFT_BLACK);
    _tft->setTextColor(TFT_YELLOW, TFT_BLACK);
    _tft->setTextDatum(TC_DATUM);
    _tft->drawString("-- ELOKESZITES --", 160, 48, 2);

    UIUtils::drawButton(_tft, 20, 75, 280, 40, "Auto Home (G28)", _tft->color565(50, 50, 50), TFT_WHITE, false, 2, 5);
    UIUtils::drawButton(_tft, 20, 125, 280, 40, "Steppers Off (M84)", _tft->color565(50, 50, 50), TFT_WHITE, false, 2, 5);

    UIUtils::drawButton(_tft, 20, 204, 280, 26, LangManager::get("btn_back"), _tft->color565(100, 30, 30), TFT_WHITE, false, 1, 4);
}

void OctoMenu::drawCalibrationMenu() {
    _tft->fillRect(10, 45, 300, 190, TFT_BLACK);
    _tft->setTextColor(TFT_YELLOW, TFT_BLACK);
    _tft->setTextDatum(TC_DATUM);
    _tft->drawString("-- KALIBRÁCIÓ --", 160, 48, 2);

    UIUtils::drawButton(_tft, 20, 75, 130, 55, "Z-Offset", _tft->color565(40, 40, 60), TFT_WHITE, false, 2, 5);
    UIUtils::drawButton(_tft, 170, 75, 130, 55, "Bed Mesh", _tft->color565(40, 40, 60), TFT_WHITE, false, 2, 5);
    UIUtils::drawButton(_tft, 20, 138, 130, 55, "Leveling", _tft->color565(40, 40, 60), TFT_WHITE, false, 2, 5);
    UIUtils::drawButton(_tft, 170, 138, 130, 55, "Egyeb", _tft->color565(40, 40, 60), TFT_WHITE, false, 2, 5);

    UIUtils::drawButton(_tft, 20, 204, 280, 26, LangManager::get("btn_back"), _tft->color565(100, 30, 30), TFT_WHITE, false, 1, 4);
}

void OctoMenu::drawControlMenu() {
    _tft->fillRect(10, 45, 300, 190, TFT_BLACK);
    _tft->setTextColor(TFT_YELLOW, TFT_BLACK);
    _tft->setTextDatum(TC_DATUM);
    _tft->drawString("-- IRANYITAS --", 160, 48, 2);
    
    _tft->setTextColor(TFT_DARKGREY, TFT_BLACK);
    _tft->setTextDatum(MC_DATUM);
    _tft->drawString("Hamarosan...", 160, 120, 2);

    UIUtils::drawButton(_tft, 20, 204, 280, 26, LangManager::get("btn_back"), _tft->color565(100, 30, 30), TFT_WHITE, false, 1, 4);
}

void OctoMenu::drawFilamentMenu() {
    _tft->fillRect(10, 45, 300, 190, TFT_BLACK);
    _tft->setTextColor(TFT_YELLOW, TFT_BLACK);
    _tft->setTextDatum(TC_DATUM);
    _tft->drawString("-- FILAMENT --", 160, 48, 2);

    _tft->setTextColor(TFT_DARKGREY, TFT_BLACK);
    _tft->setTextDatum(MC_DATUM);
    _tft->drawString("Hamarosan...", 160, 120, 2);

    UIUtils::drawButton(_tft, 20, 204, 280, 26, LangManager::get("btn_back"), _tft->color565(100, 30, 30), TFT_WHITE, false, 1, 4);
}

void OctoMenu::drawTuneRow(int y, const String& label, const String& valueStr) {
    UIUtils::drawButton(_tft, 15, y, 45, 30, "-", _tft->color565(60, 60, 60), TFT_WHITE, false, 4, 4);

    _tft->fillRoundRect(65, y, 190, 30, 4, _tft->color565(35, 35, 35));
    _tft->setTextColor(TFT_GOLD, _tft->color565(35, 35, 35));
    _tft->setTextDatum(MC_DATUM);
    _tft->drawString(label + ": " + valueStr, 160, y + 15, 2);

    UIUtils::drawButton(_tft, 260, y, 45, 30, "+", _tft->color565(60, 60, 60), TFT_WHITE, false, 4, 4);
}

void OctoMenu::drawTuneMenu(OctoClient* client) {
    _tft->fillRect(10, 45, 300, 190, TFT_BLACK);
    _tft->setTextColor(TFT_YELLOW, TFT_BLACK);
    _tft->setTextDatum(TC_DATUM);
    _tft->drawString("OCTOPRINT TUNE", 160, 48, 2);

    int spd = client ? client->getData().speed : 100;
    int nzl = client ? (int)client->getData().nozzleTarget : 0;
    int bed = client ? (int)client->getData().bedTarget : 0;

    drawTuneRow(72,  "Speed",  String(spd) + "%");
    drawTuneRow(106, "Nozzle", String(nzl) + "C");
    drawTuneRow(140, "Bed",    String(bed) + "C");

    UIUtils::drawButton(_tft, 15, 174, 45, 30, "-0.01", _tft->color565(60, 60, 60), TFT_WHITE, false, 1, 4);

    _tft->fillRoundRect(65, 174, 190, 30, 4, _tft->color565(35, 35, 35));
    _tft->setTextColor(TFT_GOLD, _tft->color565(35, 35, 35));
    _tft->setTextDatum(MC_DATUM);
    _tft->drawString("Z-Offset Adj.", 160, 189, 2);

    UIUtils::drawButton(_tft, 260, 174, 45, 30, "+0.01", _tft->color565(60, 60, 60), TFT_WHITE, false, 1, 4);

    UIUtils::drawButton(_tft, 20, 208, 280, 24, LangManager::get("btn_back"), _tft->color565(100, 30, 30), TFT_WHITE, false, 1, 4);
}

void OctoMenu::drawZOffsetMenu() {
    _tft->fillRect(10, 45, 300, 190, TFT_BLACK);
    _tft->setTextColor(TFT_YELLOW, TFT_BLACK);
    _tft->setTextDatum(TC_DATUM);
    _tft->drawString("Z-OFFSET KALIBRACIO", 160, 48, 2);

    UIUtils::drawButton(_tft, 20, 75, 280, 34, "Z-Home (G28 Z)", _tft->color565(50, 50, 50), TFT_WHITE, false, 2, 5);

    UIUtils::drawButton(_tft, 15, 118, 55, 34, "-0.01", _tft->color565(60, 60, 60), TFT_WHITE, false, 1, 4);

    _tft->fillRoundRect(75, 118, 170, 34, 4, _tft->color565(35, 35, 35));
    _tft->setTextColor(TFT_GOLD, _tft->color565(35, 35, 35));
    _tft->setTextDatum(MC_DATUM);
    _tft->drawString("Z-Offset Adj.", 160, 135, 2);

    UIUtils::drawButton(_tft, 250, 118, 55, 34, "+0.01", _tft->color565(60, 60, 60), TFT_WHITE, false, 1, 4);

    UIUtils::drawButton(_tft, 20, 160, 280, 34, "Mentes (M500)", _tft->color565(0, 100, 0), TFT_WHITE, false, 2, 5);

    UIUtils::drawButton(_tft, 20, 200, 280, 28, LangManager::get("btn_back"), _tft->color565(100, 30, 30), TFT_WHITE, false, 1, 5);
}

void OctoMenu::drawMeshMenu(OctoClient* client) {
    _tft->fillRect(10, 45, 300, 190, TFT_BLACK);
    _tft->setTextColor(TFT_YELLOW, TFT_BLACK);
    _tft->setTextDatum(TC_DATUM);
    _tft->drawString("BED MESH KEZELES", 160, 48, 2);

    updateMeshButtons(client);

    UIUtils::drawButton(_tft, 20, 128, 280, 40, "Show Mesh (Kirajzolas)", _tft->color565(50, 50, 50), TFT_WHITE, false, 2, 5);

    UIUtils::drawButton(_tft, 20, 190, 280, 32, LangManager::get("btn_back"), _tft->color565(100, 30, 30), TFT_WHITE, false, 1, 5);
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
    _tft->fillRect(10, 45, 300, 190, TFT_BLACK);
    _tft->setTextColor(TFT_CYAN, TFT_BLACK);
    _tft->setTextDatum(TC_DATUM);

    const auto& data = client ? client->getData() : OctoPrinterData();
    int rows = data.meshRows > 0 ? data.meshRows : 3;
    int cols = data.meshCols > 0 ? data.meshCols : 3;

    String title = "BED MESH (" + String(rows) + "x" + String(cols) + ")";
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

    UIUtils::drawButton(_tft, 20, 204, 280, 24, LangManager::get("btn_back"), _tft->color565(100, 30, 30), TFT_WHITE, false, 1, 4);
}

int OctoMenu::handleTouch(uint16_t x, uint16_t y, OctoClient* client) {
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
            UIUtils::pressFeedback(_tft, 20, 204, 280, 26, LangManager::get("btn_back"), _tft->color565(100, 30, 30), TFT_WHITE, 1, 4);
            return 0; 
        }
        
        if (y >= 75 && y <= 130) {
            if (x >= 20 && x <= 150) {
                UIUtils::pressFeedback(_tft, 20, 75, 130, 55, "Elokeszites", _tft->color565(40, 40, 60), TFT_WHITE, 2, 5);
                _subState = SUB_PREPARE;
                drawPrepareMenu();
                return 1;
            } else if (x >= 170 && x <= 300) {
                UIUtils::pressFeedback(_tft, 170, 75, 130, 55, "Iranyitas", _tft->color565(40, 40, 60), TFT_WHITE, 2, 5);
                _subState = SUB_CONTROL;
                drawControlMenu();
                return 1;
            }
        } else if (y >= 138 && y <= 193) {
            if (x >= 20 && x <= 150) {
                UIUtils::pressFeedback(_tft, 20, 138, 130, 55, "Kalibracio", _tft->color565(40, 40, 60), TFT_WHITE, 2, 5);
                _subState = SUB_CALIBRATION;
                drawCalibrationMenu();
                return 1;
            } else if (x >= 170 && x <= 300) {
                UIUtils::pressFeedback(_tft, 170, 138, 130, 55, "Filament", _tft->color565(40, 40, 60), TFT_WHITE, 2, 5);
                _subState = SUB_FILAMENT;
                drawFilamentMenu();
                return 1;
            }
        }
        return -1;
    }

    if (_subState == SUB_PREPARE) {
        if (y >= 200) { 
            UIUtils::pressFeedback(_tft, 20, 204, 280, 26, LangManager::get("btn_back"), _tft->color565(100, 30, 30), TFT_WHITE, 1, 4);
            _subState = SUB_MAIN; 
            drawMainMenu(); 
            return 1; 
        }
        
        if (y >= 75 && y <= 115) {
            if (client && client->isHoming()) {
                return 1; 
            }
            
            if (client) {
                UIUtils::pressFeedback(_tft, 20, 75, 280, 40, "Auto Home (G28)", _tft->color565(50, 50, 50), TFT_WHITE, 2, 5);
                client->autoHome();
                updatePrepareMenu(client);
            }
            return 1;
        }

        if (y >= 125 && y <= 165) {
            if (client) {
                UIUtils::pressFeedback(_tft, 20, 125, 280, 40, "Steppers Off (M84)", _tft->color565(50, 50, 50), TFT_WHITE, 2, 5);
                client->disableSteppers();
            }
            return 1;
        }

        return -1;
    }

    if (_subState == SUB_CALIBRATION) {
        if (y >= 200) { 
            UIUtils::pressFeedback(_tft, 20, 204, 280, 26, LangManager::get("btn_back"), _tft->color565(100, 30, 30), TFT_WHITE, 1, 4);
            _subState = SUB_MAIN; 
            drawMainMenu(); 
            return 1; 
        }
        
        if (y >= 75 && y <= 130) {
            if (x >= 20 && x <= 150) {
                UIUtils::pressFeedback(_tft, 20, 75, 130, 55, "Z-Offset", _tft->color565(40, 40, 60), TFT_WHITE, 2, 5);
                _subState = SUB_Z_OFFSET;
                drawZOffsetMenu();
                return 1;
            } else if (x >= 170 && x <= 300) {
                UIUtils::pressFeedback(_tft, 170, 75, 130, 55, "Bed Mesh", _tft->color565(40, 40, 60), TFT_WHITE, 2, 5);
                _subState = SUB_MESH;
                drawMeshMenu(client);
                return 1;
            }
        } else if (y >= 138 && y <= 193) {
            if (x >= 20 && x <= 150) {
                UIUtils::pressFeedback(_tft, 20, 138, 130, 55, "Leveling", _tft->color565(40, 40, 60), TFT_WHITE, 2, 5);
                _subState = SUB_BED_LEVEL;
                _bedLevelMenu.open();
                return 1;
            } else if (x >= 170 && x <= 300) {
                UIUtils::pressFeedback(_tft, 170, 138, 130, 55, "Egyéb", _tft->color565(40, 40, 60), TFT_WHITE, 2, 5);
                _subState = SUB_OTHER_CALIB;
                _otherCalibMenu.open();
                return 1;
            }
        }
        return -1;
    }

    if (_subState == SUB_CONTROL || _subState == SUB_FILAMENT) {
        if (y >= 200) { 
            UIUtils::pressFeedback(_tft, 20, 204, 280, 26, LangManager::get("btn_back"), _tft->color565(100, 30, 30), TFT_WHITE, 1, 4);
            _subState = SUB_MAIN; 
            drawMainMenu(); 
            return 1; 
        }
        return -1;
    }

    if (_subState == SUB_TUNE) {
        if (y >= 208) {
            UIUtils::pressFeedback(_tft, 20, 208, 280, 24, LangManager::get("btn_back"), _tft->color565(100, 30, 30), TFT_WHITE, 1, 4);
            return 0; 
        }
        if (!client) return 1;
        const OctoPrinterData& data = client->getData();

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
        } else if (y >= 106 && y <= 136) {
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
        } else if (y >= 140 && y <= 170) {
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
        } else if (y >= 174 && y <= 204) {
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

    if (_subState == SUB_Z_OFFSET) {
        if (y >= 200) { 
            UIUtils::pressFeedback(_tft, 20, 200, 280, 28, LangManager::get("btn_back"), _tft->color565(100, 30, 30), TFT_WHITE, 1, 5);
            _subState = SUB_CALIBRATION; 
            drawCalibrationMenu(); 
            return 1; 
        } 
        if (y >= 75 && y <= 109) { 
            if (client) {
                UIUtils::pressFeedback(_tft, 20, 75, 280, 34, "Z-Home (G28 Z)", _tft->color565(50, 50, 50), TFT_WHITE, 2, 5);
                client->homeZ(); 
            }
            return 1; 
        }
        if (y >= 118 && y <= 152) {
            if (x >= 15 && x <= 70) { 
                if (client) {
                    UIUtils::pressFeedback(_tft, 15, 118, 55, 34, "-0.01", _tft->color565(60, 60, 60), TFT_WHITE, 1, 4);
                    client->adjustZOffset(-0.01f); 
                }
            }
            if (x >= 250 && x <= 305) { 
                if (client) {
                    UIUtils::pressFeedback(_tft, 250, 118, 55, 34, "+0.01", _tft->color565(60, 60, 60), TFT_WHITE, 1, 4);
                    client->adjustZOffset(0.01f); 
                }
            }
            return 1;
        }
        if (y >= 160 && y <= 194) { 
            if (client) {
                UIUtils::pressFeedback(_tft, 20, 160, 280, 34, "Mentes (M500)", _tft->color565(0, 100, 0), TFT_WHITE, 2, 5);
                client->saveConfig(); 
            }
            return 1; 
        }
        return -1;
    }

    if (_subState == SUB_MESH) {
        if (y >= 185) {
            UIUtils::pressFeedback(_tft, 20, 190, 280, 32, LangManager::get("btn_back"), _tft->color565(100, 30, 30), TFT_WHITE, 1, 5);
            _subState = SUB_CALIBRATION; 
            drawCalibrationMenu();
            return 1;
        }
        if (y >= 75 && y <= 115) {
            if (client && client->isMeshBuilding()) {
                return 1;
            }

            if (x >= 15 && x <= 150) {
                UIUtils::pressFeedback(_tft, 15, 75, 135, 40, "Build 3x3", _tft->color565(50, 50, 50), TFT_WHITE, 2, 5);
                if (client) client->autoBuildMesh(3);
                updateMeshButtons(client);
            } else if (x >= 170 && x <= 305) {
                UIUtils::pressFeedback(_tft, 170, 75, 135, 40, "Build 5x5", _tft->color565(50, 50, 50), TFT_WHITE, 2, 5);
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
                UIUtils::pressFeedback(_tft, 20, 128, 280, 40, "Show Mesh (Kirajzolas)", _tft->color565(50, 50, 50), TFT_WHITE, 2, 5);
                client->fetchBedMesh();
                if (client->shouldShowNoMeshPopup()) {
                    draw(client);
                } else {
                    _subState = SUB_SHOW_MESH;
                    drawShowMeshMenu(client);
                }
            }
            return 1;
        }
        return -1;
    }

    if (_subState == SUB_SHOW_MESH) {
        if (y >= 204) {
            UIUtils::pressFeedback(_tft, 20, 204, 280, 24, LangManager::get("btn_back"), _tft->color565(100, 30, 30), TFT_WHITE, 1, 4);
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