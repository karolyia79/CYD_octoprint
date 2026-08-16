#include "octo_filament_menu_mqtt.h"
#include "lang_manager.h"
#include "ui_utils.h"
#include "config_manager.h"
#include <math.h>

OctoFilamentMenuMqtt::OctoFilamentMenuMqtt(TFT_eSPI* tft) : _tft(tft) {}

void OctoFilamentMenuMqtt::init() {
    _selectedMaterial = MAT_PLA;
    _isBowden = false;
    _currentStep = STEP_SELECT;
    _activePopup = POPUP_NONE;
    _forceRedraw = true;
    _lastTempReady = false;
    _lastActualTemp = -1;
    _lastTargetTemp = -1;
}

void OctoFilamentMenuMqtt::open() {
    _currentStep = STEP_SELECT;
    _activePopup = POPUP_NONE;
    _forceRedraw = true;
    _lastTempReady = false;
    _lastActualTemp = -1;
    _lastTargetTemp = -1;
}

float OctoFilamentMenuMqtt::getTargetTemp() const {
    if (_selectedMaterial == MAT_PETG) return 240.0f;
    if (_selectedMaterial == MAT_ABS)  return 255.0f;
    return 205.0f; // MAT_PLA
}

bool OctoFilamentMenuMqtt::isTempReady(OctoClientMqtt* client) const {
    if (!client) return false;
    float actualTemp = client->getData().nozzleTemp;
    float targetTemp = getTargetTemp();
    return (fabs(actualTemp - targetTemp) <= 3.0f);
}

void OctoFilamentMenuMqtt::draw(OctoClientMqtt* client) {
    float actualTemp = client ? client->getData().nozzleTemp : 0.0f;
    float targetTemp = client ? client->getData().nozzleTarget : 0.0f;
    bool tempReady = isTempReady(client);
    
    int currentTempInt = (int)actualTemp;
    int currentTargetInt = (int)targetTemp;

    bool stateChanged = false;
    if (_currentStep == STEP_ACTIONS && _activePopup == POPUP_NONE) {
        if (tempReady != _lastTempReady || currentTempInt != _lastActualTemp || currentTargetInt != _lastTargetTemp) {
            stateChanged = true;
            _lastTempReady = tempReady;
            _lastActualTemp = currentTempInt;
            _lastTargetTemp = currentTargetInt;
        }
    }

    if (!_forceRedraw && !stateChanged) return;
    _forceRedraw = false;

    if (_currentStep == STEP_SELECT) {
        drawSelectStep();
    } else {
        drawActionsStep(client);

        if (_activePopup == POPUP_WAIT_HEATING) {
            drawPopupWait();
        } else if (_activePopup == POPUP_START_HEATING) {
            drawPopupStartHeating();
        }
    }
}

void OctoFilamentMenuMqtt::drawSelectStep() {
    ThemeColors theme = getCurrentTheme();
    _tft->fillRect(10, 45, 300, 190, theme.bg);

    // --- 1. Anyagvalasztos gombok (PLA / PETG / ABS) ---
    uint16_t plaBg  = (_selectedMaterial == MAT_PLA)  ? theme.accent : theme.cardBg;
    uint16_t plaTxt = (_selectedMaterial == MAT_PLA)  ? theme.bg     : theme.text;
    
    uint16_t petgBg  = (_selectedMaterial == MAT_PETG) ? theme.accent : theme.cardBg;
    uint16_t petgTxt = (_selectedMaterial == MAT_PETG) ? theme.bg     : theme.text;
    
    uint16_t absBg  = (_selectedMaterial == MAT_ABS)  ? theme.accent : theme.cardBg;
    uint16_t absTxt = (_selectedMaterial == MAT_ABS)  ? theme.bg     : theme.text;

    UIUtils::drawButton(_tft, 15, 55, 90, 38, "PLA (205C)", plaBg, plaTxt, false, 1, 4);
    UIUtils::drawButton(_tft, 115, 55, 90, 38, "PETG (240C)", petgBg, petgTxt, false, 1, 4);
    UIUtils::drawButton(_tft, 215, 55, 90, 38, "ABS (255C)", absBg, absTxt, false, 1, 4);

    // --- 2. Extruder tipus (Direct Drive / Bowden) ---
    uint16_t directBg  = !_isBowden ? theme.accent : theme.cardBg;
    uint16_t directTxt = !_isBowden ? theme.bg     : theme.text;
    
    uint16_t bowdenBg  = _isBowden  ? theme.accent : theme.cardBg;
    uint16_t bowdenTxt = _isBowden  ? theme.bg     : theme.text;

    UIUtils::drawButton(_tft, 15, 105, 140, 38, "Direct Drive", directBg, directTxt, false, 1, 4);
    UIUtils::drawButton(_tft, 165, 105, 140, 38, "Bowden Cso", bowdenBg, bowdenTxt, false, 1, 4);

    // --- 3. Tovabb es Vissza gombok ---
    UIUtils::drawButton(_tft, 15, 155, 290, 38, "Tovabb az akciokhoz >>", TFT_DARKGREEN, TFT_WHITE, false, 2, 5);
    UIUtils::drawButton(_tft, 15, 202, 290, 26, LangManager::get("btn_back"), theme.cardBg, theme.text, false, 1, 4);
}

void OctoFilamentMenuMqtt::drawActionsStep(OctoClientMqtt* client) {
    ThemeColors theme = getCurrentTheme();
    _tft->fillRect(10, 45, 300, 190, theme.bg);

    float targetTemp = getTargetTemp();
    float actualTemp = client ? client->getData().nozzleTemp : 0.0f;
    float currentNozzleTarget = client ? client->getData().nozzleTarget : 0.0f;
    bool tempReady = isTempReady(client);

    // Informacios reszletezo sáv legfelul (Anyag | Extruder | Aktuális / Cél hőfok)
    String matStr = (_selectedMaterial == MAT_PLA) ? "PLA" : (_selectedMaterial == MAT_PETG ? "PETG" : "ABS");
    String extStr = _isBowden ? "Bowden" : "Direct";
    String tempStr = String((int)actualTemp) + " / " + String((int)targetTemp) + "C";
    
    _tft->setTextColor(tempReady ? TFT_GREEN : TFT_ORANGE, theme.bg);
    _tft->setTextDatum(MC_DATUM);
    _tft->drawString(matStr + " | " + extStr + " | " + tempStr, 160, 52, 2);

    // --- 1. Futes / Hutes vezérles (Ha van fűtés -> PIROS gomb) ---
    bool isHeating = (currentNozzleTarget > 0.0f);
    uint16_t heatBg  = isHeating ? TFT_RED : TFT_ORANGE;
    uint16_t heatTxt = isHeating ? TFT_WHITE : TFT_BLACK;

    UIUtils::drawButton(_tft, 15, 68, 140, 32, "Fej Futese", heatBg, heatTxt, false, 1, 4);
    UIUtils::drawButton(_tft, 165, 68, 140, 32, "Fej Hutese (0C)", theme.cardBg, theme.text, false, 1, 4);

    // Dinamikus gomb színezés (Tiltott vs Aktív állapot)
    uint16_t actionBg  = tempReady ? theme.cardBg : _tft->color565(35, 35, 35);
    uint16_t actionTxt = tempReady ? theme.text   : _tft->color565(110, 110, 110);

    // --- 2. Betoltes / Kiadas Akcio gombok ---
    String loadTxt   = tempReady ? "Betoltes (Load)"   : "Betoltes (Tiltva)";
    String unloadTxt = tempReady ? "Kiadas (Unload)"   : "Kiadas (Tiltva)";
    UIUtils::drawButton(_tft, 15, 108, 140, 38, loadTxt, actionBg, actionTxt, false, 1, 4);
    UIUtils::drawButton(_tft, 165, 108, 140, 38, unloadTxt, actionBg, actionTxt, false, 1, 4);

    // --- 3. Kezi adagolas (+10mm / -10mm) ---
    UIUtils::drawButton(_tft, 15, 154, 140, 32, "Adagolas (+10mm)", actionBg, actionTxt, false, 1, 4);
    UIUtils::drawButton(_tft, 165, 154, 140, 32, "Visszahuzas (-10mm)", actionBg, actionTxt, false, 1, 4);

    // --- 4. Vissza a valasztashoz gomb ---
    UIUtils::drawButton(_tft, 15, 196, 290, 30, "<< Vissza a valasztashoz", theme.cardBg, theme.text, false, 1, 4);
}

void OctoFilamentMenuMqtt::drawPopupWait() {
    _tft->fillRoundRect(25, 60, 270, 140, 8, _tft->color565(30, 20, 10));
    _tft->drawRoundRect(25, 60, 270, 140, 8, TFT_ORANGE);
    _tft->drawRoundRect(26, 61, 268, 138, 7, TFT_ORANGE);

    _tft->setTextDatum(MC_DATUM);
    _tft->setTextColor(TFT_ORANGE, _tft->color565(30, 20, 10));
    _tft->drawString("Varakozas a futesre", 160, 82, 2);

    _tft->setTextColor(TFT_WHITE, _tft->color565(30, 20, 10));
    _tft->drawString("A hotend meg nem erte el", 160, 108, 1);
    _tft->drawString("a celhomersekletet (+-3C)!", 160, 124, 1);

    UIUtils::drawButton(_tft, 105, 152, 110, 36, "OK", TFT_DARKGREEN, TFT_WHITE, false, 2, 5);
}

void OctoFilamentMenuMqtt::drawPopupStartHeating() {
    _tft->fillRoundRect(25, 60, 270, 140, 8, _tft->color565(40, 10, 10));
    _tft->drawRoundRect(25, 60, 270, 140, 8, TFT_RED);
    _tft->drawRoundRect(26, 61, 268, 138, 7, TFT_RED);

    _tft->setTextDatum(MC_DATUM);
    _tft->setTextColor(TFT_RED, _tft->color565(40, 10, 10));
    _tft->drawString("A futes nincs elinditva", 160, 82, 2);

    _tft->setTextColor(TFT_WHITE, _tft->color565(40, 10, 10));
    _tft->drawString("Az akciohoz elobb fel kell", 160, 108, 1);
    _tft->drawString("futenie a hotendet!", 160, 124, 1);

    UIUtils::drawButton(_tft, 35, 152, 100, 36, "OK", _tft->color565(60, 60, 60), TFT_WHITE, false, 2, 5);
    UIUtils::drawButton(_tft, 145, 152, 140, 36, "Futes inditasa", TFT_ORANGE, TFT_BLACK, false, 1, 4);
}

int OctoFilamentMenuMqtt::handleTouch(uint16_t x, uint16_t y, OctoClientMqtt* client) {
    ThemeColors theme = getCurrentTheme();

    // --- POPUP ABLAKOK ÉRINTÉSKEZELÉSE ---
    if (_activePopup != POPUP_NONE) {
        if (_activePopup == POPUP_WAIT_HEATING) {
            if (y >= 148 && y <= 192 && x >= 105 && x <= 215) {
                UIUtils::pressFeedback(_tft, 105, 152, 110, 36, "OK", TFT_DARKGREEN, TFT_WHITE, 2, 5);
            }
            _activePopup = POPUP_NONE;
            _forceRedraw = true;
            draw(client);
            return 1;
        } 
        else if (_activePopup == POPUP_START_HEATING) {
            if (y >= 148 && y <= 192 && x >= 35 && x <= 135) {
                UIUtils::pressFeedback(_tft, 35, 152, 100, 36, "OK", _tft->color565(60, 60, 60), TFT_WHITE, 2, 5);
                _activePopup = POPUP_NONE;
                _forceRedraw = true;
                draw(client);
                return 1;
            }
            if (y >= 148 && y <= 192 && x >= 145 && x <= 285) {
                UIUtils::pressFeedback(_tft, 145, 152, 140, 36, "Futes...", TFT_ORANGE, TFT_BLACK, 1, 4);
                if (client) client->setNozzleTarget(getTargetTemp());
                _activePopup = POPUP_NONE;
                _forceRedraw = true;
                draw(client);
                return 1;
            }
            return 1;
        }
    }

    // --- 1. LÉPÉS ÉRINTÉSKEZELÉSE ---
    if (_currentStep == STEP_SELECT) {
        if (y >= 200) { 
            UIUtils::pressFeedback(_tft, 15, 202, 290, 26, LangManager::get("btn_back"), theme.cardBg, theme.text, 1, 4);
            return 0; 
        }

        if (y >= 55 && y <= 95) {
            if (x >= 15 && x <= 105)       { _selectedMaterial = MAT_PLA;  _forceRedraw = true; draw(client); return 1; }
            else if (x >= 115 && x <= 205) { _selectedMaterial = MAT_PETG; _forceRedraw = true; draw(client); return 1; }
            else if (x >= 215 && x <= 305) { _selectedMaterial = MAT_ABS;  _forceRedraw = true; draw(client); return 1; }
        }

        if (y >= 105 && y <= 145) {
            if (x >= 15 && x <= 155)       { _isBowden = false; _forceRedraw = true; draw(client); return 1; }
            else if (x >= 165 && x <= 305) { _isBowden = true;  _forceRedraw = true; draw(client); return 1; }
        }

        if (y >= 155 && y <= 195) {
            UIUtils::pressFeedback(_tft, 15, 155, 290, 38, "Tovabb az akciokhoz >>", TFT_DARKGREEN, TFT_WHITE, 2, 5);
            _currentStep = STEP_ACTIONS;
            _forceRedraw = true;
            draw(client);
            return 1;
        }

        return -1;
    } 
    // --- 2. LÉPÉS ÉRINTÉSKEZELÉSE ---
    else { 
        if (y >= 194) { 
            UIUtils::pressFeedback(_tft, 15, 196, 290, 30, "<< Vissza a valasztashoz", theme.cardBg, theme.text, 1, 4);
            _currentStep = STEP_SELECT;
            _forceRedraw = true;
            draw(client);
            return 1; 
        }

        float targetTemp = getTargetTemp();
        bool tempReady = isTempReady(client);

        // Futes / Hutes vezérles (y: 68..102)
        if (y >= 68 && y <= 102) {
            if (x >= 15 && x <= 155) {
                UIUtils::pressFeedback(_tft, 15, 68, 140, 32, "Fej Futese", TFT_RED, TFT_WHITE, 1, 4);
                if (client) client->setNozzleTarget(targetTemp);
                _forceRedraw = true;
                draw(client);
                return 1;
            } else if (x >= 165 && x <= 305) {
                UIUtils::pressFeedback(_tft, 165, 68, 140, 32, "Fej Hutese (0C)", theme.cardBg, theme.text, 1, 4);
                if (client) client->setNozzleTarget(0);
                _forceRedraw = true;
                draw(client);
                return 1;
            }
        }

        // --- ZÁROLT ÁLLAPOT KEZELÉSE ---
        if (y >= 108 && y <= 188) {
            if (!tempReady) {
                float currentTarget = client ? client->getData().nozzleTarget : 0.0f;
                
                if (currentTarget >= targetTemp - 5.0f) {
                    _activePopup = POPUP_WAIT_HEATING;
                } else {
                    _activePopup = POPUP_START_HEATING;
                }
                _forceRedraw = true;
                draw(client);
                return 1;
            }

            // Betoltes / Kiadas Akciok (y: 108..148)
            if (y >= 108 && y <= 148) {
                if (x >= 15 && x <= 155) {
                    UIUtils::pressFeedback(_tft, 15, 108, 140, 38, "Betoltes...", theme.cardBg, theme.text, 1, 4);
                    if (client) client->loadFilament(_isBowden);
                    return 1;
                } else if (x >= 165 && x <= 305) {
                    UIUtils::pressFeedback(_tft, 165, 108, 140, 38, "Kiadas...", theme.cardBg, theme.text, 1, 4);
                    if (client) client->unloadFilament(_isBowden);
                    return 1;
                }
            }

            // Kezi adagolas (+10mm / -10mm) (y: 154..188)
            if (y >= 154 && y <= 188) {
                if (x >= 15 && x <= 155) {
                    UIUtils::pressFeedback(_tft, 15, 154, 140, 32, "+10mm", theme.cardBg, theme.text, 1, 4);
                    if (client) client->extrudeFilament(10.0f);
                    return 1;
                } else if (x >= 165 && x <= 305) {
                    UIUtils::pressFeedback(_tft, 165, 154, 140, 32, "-10mm", theme.cardBg, theme.text, 1, 4);
                    if (client) client->extrudeFilament(-10.0f);
                    return 1;
                }
            }
        }

        return -1;
    }
}