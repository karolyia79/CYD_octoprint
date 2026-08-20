#include "octo_control_menu_mqtt.h"
#include "lang_manager.h"
#include "ui_utils.h"
#include "config_manager.h"

OctoControlMenuMqtt::OctoControlMenuMqtt(TFT_eSPI* tft) : _tft(tft) {}

void OctoControlMenuMqtt::init() {
    _subState = 0;
    _forceRedraw = true;
    _selectedMoveStepIndex = 2; // Alapértelmezett: 0.1mm
    _selectedTempStepIndex = 1; // Alapértelmezett: 10°C
    _showColdWarningPopup = false;
    _pulseState = false;
    _lastPulseTime = 0;
}

void OctoControlMenuMqtt::draw(OctoClientMqtt* client) {
    static int lastSubState = -1;
    bool subStateChanged = (_subState != lastSubState) || _forceRedraw;
    lastSubState = _subState;

    // 1. Ha hőmérséklet menüben vagyunk, csak a hőfok kijelző dobozát pulzáltatjuk (fűtés közben)
    if (_subState == 20 && !_showColdWarningPopup) {
        float actualTemp = 0.0f;
        float targetTemp = 0.0f;

        if (client) {
            actualTemp = (_currentTempTarget == TEMP_NOZZLE) ? client->getData().nozzleTemp : client->getData().bedTemp;
            targetTemp = (_currentTempTarget == TEMP_NOZZLE) ? client->getData().nozzleTarget : client->getData().bedTarget;
        }

        // Fűtés aktív, ha a célhőmérséklet > 0 ÉS az aktuális hőfok kisebb a célnál
        bool isHeating = (targetTemp > 0.0f) && (actualTemp < targetTemp);

        if (isHeating) {
            if (millis() - _lastPulseTime > 500) { // 500ms ütem
                _lastPulseTime = millis();
                _pulseState = !_pulseState;
                drawTempDisplayBox(client); // Csak a kijelző mezőt rajzoljuk újra
            }
        } else if (_pulseState) {
            // Ha megszűnt a fűtés (pl. elértük a hőfokot vagy visszahűl), visszaállítjuk az alapszínt
            _pulseState = false;
            drawTempDisplayBox(client);
        }
    }

    if (!subStateChanged) return;

    if (_subState == 0) {
        drawMainMenu();
    } else if (_subState == 1) {
        drawMoveAxisSelectMenu();
    } else if (_subState == 10) {
        drawMoveControlMenu(client);
    } else if (_subState == 2) {
        drawTempSelectMenu();
    } else if (_subState == 20) {
        drawTempControlMenu(client);
    } else if (_subState == 3) {
        drawBltouchMenu();
    } else if (_subState == 4) {
        drawFanMenu();
    } else {
        drawSubMenu();
    }

    // Ha aktív a felugró figyelmeztetés, rárajzoljuk a képernyőre
    if (_showColdWarningPopup) {
        drawColdWarningPopup();
    }

    _forceRedraw = false;
}

void OctoControlMenuMqtt::drawMainMenu() {
    ThemeColors theme = getCurrentTheme();
    _tft->fillRect(10, 45, 300, 190, theme.bg);
    _tft->setTextColor(theme.accent, theme.bg);
    _tft->setTextDatum(TC_DATUM);
    _tft->drawString(LangManager::get("octo_menu_control_title"), 160, 48, 2);

    UIUtils::drawButton(_tft, 20, 75, 130, 55, LangManager::get("control_btn_move"), theme.cardBg, theme.text, false, 2, 5);
    UIUtils::drawButton(_tft, 170, 75, 130, 55, LangManager::get("control_btn_temp"), theme.cardBg, theme.text, false, 2, 5);
    UIUtils::drawButton(_tft, 20, 138, 130, 55, LangManager::get("control_btn_bltouch"), theme.cardBg, theme.text, false, 2, 5);
    UIUtils::drawButton(_tft, 170, 138, 130, 55, LangManager::get("control_btn_fan"), theme.cardBg, theme.text, false, 2, 5);

    UIUtils::drawButton(_tft, 20, 204, 280, 26, LangManager::get("btn_back"), theme.cardBg, theme.text, false, 1, 4);
}

void OctoControlMenuMqtt::drawMoveAxisSelectMenu() {
    ThemeColors theme = getCurrentTheme();
    _tft->fillRect(10, 45, 300, 190, theme.bg);
    _tft->setTextColor(theme.accent, theme.bg);
    _tft->setTextDatum(TC_DATUM);
    _tft->drawString(LangManager::get("control_btn_move"), 160, 48, 2);

    UIUtils::drawButton(_tft, 20, 75, 130, 55, LangManager::get("control_axis_x"), theme.cardBg, theme.text, false, 2, 5);
    UIUtils::drawButton(_tft, 170, 75, 130, 55, LangManager::get("control_axis_y"), theme.cardBg, theme.text, false, 2, 5);
    UIUtils::drawButton(_tft, 20, 138, 130, 55, LangManager::get("control_axis_z"), theme.cardBg, theme.text, false, 2, 5);
    UIUtils::drawButton(_tft, 170, 138, 130, 55, LangManager::get("control_axis_e"), theme.cardBg, theme.text, false, 2, 5);

    UIUtils::drawButton(_tft, 20, 204, 280, 26, LangManager::get("btn_back"), theme.cardBg, theme.text, false, 1, 4);
}

void OctoControlMenuMqtt::drawMoveControlMenu(OctoClientMqtt* client) {
    ThemeColors theme = getCurrentTheme();
    _tft->fillRect(10, 45, 300, 190, theme.bg);
    _tft->setTextColor(theme.accent, theme.bg);
    _tft->setTextDatum(TC_DATUM);

    String axisName = "";
    switch (_currentAxis) {
        case AXIS_X: axisName = LangManager::get("control_axis_x"); break;
        case AXIS_Y: axisName = LangManager::get("control_axis_y"); break;
        case AXIS_Z: axisName = LangManager::get("control_axis_z"); break;
        case AXIS_E: 
            float currentTemp = client ? client->getData().nozzleTemp : 0.0f;
            axisName = LangManager::get("control_axis_e") + " [" + String((int)currentTemp) + "C]"; 
            break;
    }
    _tft->drawString(axisName, 160, 48, 2);

    UIUtils::drawButton(_tft, 20, 75, 130, 60, "-", theme.cardBg, theme.text, false, 2, 5);
    UIUtils::drawButton(_tft, 170, 75, 130, 60, "+", theme.cardBg, theme.text, false, 2, 5);

    for (int i = 0; i < 4; i++) {
        uint16_t bg  = (_selectedMoveStepIndex == i) ? theme.accent : theme.cardBg;
        uint16_t txt = (_selectedMoveStepIndex == i) ? theme.bg     : theme.text;

        String stepTxt;
        if (_moveStepSizes[i] >= 1.0f) {
            stepTxt = String((int)_moveStepSizes[i]) + "mm";
        } else {
            stepTxt = String(_moveStepSizes[i], 2) + "mm";
        }

        int xPos = 20 + i * 70;
        UIUtils::drawButton(_tft, xPos, 145, 60, 45, stepTxt, bg, txt, false, 1, 4);
    }

    UIUtils::drawButton(_tft, 20, 204, 280, 26, LangManager::get("btn_back"), theme.cardBg, theme.text, false, 1, 4);
}

void OctoControlMenuMqtt::drawTempSelectMenu() {
    ThemeColors theme = getCurrentTheme();
    _tft->fillRect(10, 45, 300, 190, theme.bg);
    _tft->setTextColor(theme.accent, theme.bg);
    _tft->setTextDatum(TC_DATUM);
    _tft->drawString(LangManager::get("control_btn_temp"), 160, 48, 2);

    UIUtils::drawButton(_tft, 20, 85, 130, 80, LangManager::get("control_bed_btn"), theme.cardBg, theme.text, false, 2, 5);
    UIUtils::drawButton(_tft, 170, 85, 130, 80, LangManager::get("control_nozzle_btn"), theme.cardBg, theme.text, false, 2, 5);

    UIUtils::drawButton(_tft, 20, 204, 280, 26, LangManager::get("btn_back"), theme.cardBg, theme.text, false, 1, 4);
}

// Kizárólag a hőmérséklet kijelző dobozát rajzolja újra
void OctoControlMenuMqtt::drawTempDisplayBox(OctoClientMqtt* client) {
    ThemeColors theme = getCurrentTheme();
    float actualTemp = 0.0f;
    float targetTemp = 0.0f;

    if (_currentTempTarget == TEMP_BED) {
        if (client) {
            actualTemp = client->getData().bedTemp;
            targetTemp = client->getData().bedTarget;
        }
    } else {
        if (client) {
            actualTemp = client->getData().nozzleTemp;
            targetTemp = client->getData().nozzleTarget;
        }
    }

    // Valós fűtés állapotának ellenőrzése (cél > 0 ÉS még nem érte el a célhőfokot)
    bool isHeating = (targetTemp > 0.0f) && (actualTemp < targetTemp);

    uint16_t statusBg = theme.cardBg;
    uint16_t statusTxt = theme.text;

    if (isHeating) {
        statusBg = _pulseState ? TFT_RED : TFT_MAROON;
        statusTxt = TFT_WHITE;
    }

    String tempDisplay = String((int)actualTemp) + " / " + String((int)targetTemp) + " C";
    UIUtils::drawButton(_tft, 20, 70, 280, 35, tempDisplay, statusBg, statusTxt, false, 2, 5);
}

void OctoControlMenuMqtt::drawTempControlMenu(OctoClientMqtt* client) {
    ThemeColors theme = getCurrentTheme();
    _tft->fillRect(10, 45, 300, 190, theme.bg);
    _tft->setTextColor(theme.accent, theme.bg);
    _tft->setTextDatum(TC_DATUM);

    String title = (_currentTempTarget == TEMP_BED) ? LangManager::get("control_bed_title") : LangManager::get("control_nozzle_title");
    _tft->drawString(title, 160, 48, 2);

    // Kijelző doboz megrajzolása
    drawTempDisplayBox(client);

    // Mínusz és Plusz gombok
    UIUtils::drawButton(_tft, 20, 112, 130, 42, "-", theme.cardBg, theme.text, false, 2, 5);
    UIUtils::drawButton(_tft, 170, 112, 130, 42, "+", theme.cardBg, theme.text, false, 2, 5);

    // Lépésköz gombok (1°C, 10°C) és Nullázó gomb
    uint16_t bg1  = (_selectedTempStepIndex == 0) ? theme.accent : theme.cardBg;
    uint16_t txt1 = (_selectedTempStepIndex == 0) ? theme.bg     : theme.text;
    uint16_t bg10 = (_selectedTempStepIndex == 1) ? theme.accent : theme.cardBg;
    uint16_t txt10= (_selectedTempStepIndex == 1) ? theme.bg     : theme.text;

    UIUtils::drawButton(_tft, 20, 158, 85, 40, "1 C", bg1, txt1, false, 1, 4);
    UIUtils::drawButton(_tft, 117, 158, 85, 40, "10 C", bg10, txt10, false, 1, 4);
    UIUtils::drawButton(_tft, 215, 158, 85, 40, LangManager::get("control_btn_heat_off"), TFT_DARKGREY, TFT_WHITE, false, 1, 4);

    UIUtils::drawButton(_tft, 20, 204, 280, 26, LangManager::get("btn_back"), theme.cardBg, theme.text, false, 1, 4);
}

void OctoControlMenuMqtt::drawColdWarningPopup() {
    ThemeColors theme = getCurrentTheme();
    
    _tft->fillRect(30, 65, 260, 130, theme.cardBg);
    _tft->drawRect(30, 65, 260, 130, theme.accent);

    _tft->setTextColor(theme.text, theme.cardBg);
    _tft->setTextDatum(TC_DATUM);
    _tft->drawString(LangManager::get("popup_cold_warning_title"), 160, 75, 2);
    _tft->drawString(LangManager::get("popup_cold_warning_msg"), 160, 98, 2);

    UIUtils::drawButton(_tft, 45, 135, 105, 45, LangManager::get("btn_ok"), theme.bg, theme.text, false, 2, 5);
    UIUtils::drawButton(_tft, 170, 135, 105, 45, LangManager::get("control_btn_heat_220"), theme.accent, theme.bg, false, 1, 5);
}

void OctoControlMenuMqtt::drawBltouchMenu() {
    ThemeColors theme = getCurrentTheme();
    _tft->fillRect(10, 45, 300, 190, theme.bg);
    _tft->setTextColor(theme.accent, theme.bg);
    _tft->setTextDatum(TC_DATUM);
    _tft->drawString(LangManager::get("control_bltouch_title"), 160, 48, 2);

    UIUtils::drawButton(_tft, 20, 75, 130, 55, LangManager::get("control_btn_pin_out"), theme.cardBg, theme.text, false, 2, 5);
    UIUtils::drawButton(_tft, 170, 75, 130, 55, LangManager::get("control_btn_pin_in"), theme.cardBg, theme.text, false, 2, 5);
    UIUtils::drawButton(_tft, 20, 138, 130, 55, LangManager::get("control_btn_selftest"), theme.cardBg, theme.text, false, 2, 5);
    UIUtils::drawButton(_tft, 170, 138, 130, 55, LangManager::get("control_btn_reset"), theme.cardBg, theme.text, false, 2, 5);

    UIUtils::drawButton(_tft, 20, 204, 280, 26, LangManager::get("btn_back"), theme.cardBg, theme.text, false, 1, 4);
}

void OctoControlMenuMqtt::drawFanMenu() {
    ThemeColors theme = getCurrentTheme();
    _tft->fillRect(10, 45, 300, 190, theme.bg);
    _tft->setTextColor(theme.accent, theme.bg);
    _tft->setTextDatum(TC_DATUM);
    _tft->drawString(LangManager::get("control_fan_title"), 160, 48, 2);

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

    UIUtils::drawButton(_tft, 20, 70, 280, 35, LangManager::get("control_btn_fan_off"), bg0, txt0, false, 2, 5);
    UIUtils::drawButton(_tft, 20, 112, 130, 42, "25%", bg25, txt25, false, 2, 5);
    UIUtils::drawButton(_tft, 170, 112, 130, 42, "50%", bg50, txt50, false, 2, 5);
    UIUtils::drawButton(_tft, 20, 158, 130, 42, "75%", bg75, txt75, false, 2, 5);
    UIUtils::drawButton(_tft, 170, 158, 130, 42, "100%", bg100, txt100, false, 2, 5);

    UIUtils::drawButton(_tft, 20, 204, 280, 26, LangManager::get("btn_back"), theme.cardBg, theme.text, false, 1, 4);
}

void OctoControlMenuMqtt::drawSubMenu() {
    ThemeColors theme = getCurrentTheme();
    _tft->fillRect(10, 45, 300, 190, theme.bg);
    _tft->setTextColor(theme.accent, theme.bg);
    _tft->setTextDatum(TC_DATUM);

    _tft->setTextColor(theme.subText, theme.bg);
    _tft->setTextDatum(MC_DATUM);
    _tft->drawString(LangManager::get("octo_coming_soon"), 160, 120, 2);

    UIUtils::drawButton(_tft, 20, 204, 280, 26, LangManager::get("btn_back"), theme.cardBg, theme.text, false, 1, 4);
}

void OctoControlMenuMqtt::sendMoveCommand(float distance, OctoClientMqtt* client) {
    if (!client) return;

    client->sendGcodeCommand("G91");

    String gcode = "G1 ";
    switch (_currentAxis) {
        case AXIS_X: gcode += "X"; break;
        case AXIS_Y: gcode += "Y"; break;
        case AXIS_Z: gcode += "Z"; break;
        case AXIS_E: gcode += "E"; break;
    }

    gcode += String(distance, 2);

    if (_currentAxis == AXIS_E) {
        gcode += " F300";
    } else {
        gcode += " F3000";
    }

    client->sendGcodeCommand(gcode);
    client->sendGcodeCommand("G90");
}

void OctoControlMenuMqtt::sendTempCommand(int targetTemp, OctoClientMqtt* client) {
    if (!client) return;

    if (targetTemp < 0) targetTemp = 0;

    String gcode = "";
    if (_currentTempTarget == TEMP_BED) {
        gcode = "M140 S" + String(targetTemp);
    } else {
        gcode = "M104 S" + String(targetTemp);
    }

    client->sendGcodeCommand(gcode);
}

int OctoControlMenuMqtt::handleTouch(uint16_t x, uint16_t y, OctoClientMqtt* client) {
    ThemeColors theme = getCurrentTheme();

    // 1. Popup figyelmeztetés kezelése (ha látható)
    if (_showColdWarningPopup) {
        if (y >= 135 && y <= 180 && x >= 45 && x <= 150) {
            UIUtils::pressFeedback(_tft, 45, 135, 105, 45, LangManager::get("btn_ok"), theme.bg, theme.text, 2, 5);
            _showColdWarningPopup = false;
            _forceRedraw = true;
            draw(client);
            return 1;
        }
        if (y >= 135 && y <= 180 && x >= 170 && x <= 275) {
            UIUtils::pressFeedback(_tft, 170, 135, 105, 45, LangManager::get("control_btn_heat_220"), theme.accent, theme.bg, 1, 5);
            if (client) {
                client->sendGcodeCommand("M104 S220");
            }
            _showColdWarningPopup = false;
            _forceRedraw = true;
            draw(client);
            return 1;
        }
        return 1;
    }

    // 2. Vissza gomb kezelése (y >= 200)
    if (y >= 200) {
        UIUtils::pressFeedback(_tft, 20, 204, 280, 26, LangManager::get("btn_back"), theme.cardBg, theme.text, 1, 4);
        if (_subState == 0) {
            return 0;
        } else if (_subState == 10) {
            _subState = 1;
            _forceRedraw = true;
            draw(client);
            return 1;
        } else if (_subState == 20) {
            _subState = 2;
            _forceRedraw = true;
            draw(client);
            return 1;
        } else {
            _subState = 0;
            _forceRedraw = true;
            draw(client);
            return 1;
        }
    }

    // 3. Fő Control menü érintései (_subState == 0)
    if (_subState == 0) {
        if (y >= 75 && y <= 130) {
            if (x >= 20 && x <= 150) {
                UIUtils::pressFeedback(_tft, 20, 75, 130, 55, LangManager::get("control_btn_move"), theme.cardBg, theme.text, 2, 5);
                _subState = 1;
                _forceRedraw = true;
                draw(client);
                return 1;
            } else if (x >= 170 && x <= 300) {
                UIUtils::pressFeedback(_tft, 170, 75, 130, 55, LangManager::get("control_btn_temp"), theme.cardBg, theme.text, 2, 5);
                _subState = 2;
                _forceRedraw = true;
                draw(client);
                return 1;
            }
        } else if (y >= 138 && y <= 193) {
            if (x >= 20 && x <= 150) {
                UIUtils::pressFeedback(_tft, 20, 138, 130, 55, LangManager::get("control_btn_bltouch"), theme.cardBg, theme.text, 2, 5);
                _subState = 3;
                _forceRedraw = true;
                draw(client);
                return 1;
            } else if (x >= 170 && x <= 300) {
                UIUtils::pressFeedback(_tft, 170, 138, 130, 55, LangManager::get("control_btn_fan"), theme.cardBg, theme.text, 2, 5);
                _subState = 4;
                _forceRedraw = true;
                draw(client);
                return 1;
            }
        }
    } 
    // 4. Move tengelyválasztó menü (_subState == 1)
    else if (_subState == 1) {
        if (y >= 75 && y <= 130) {
            if (x >= 20 && x <= 150) {
                UIUtils::pressFeedback(_tft, 20, 75, 130, 55, LangManager::get("control_axis_x"), theme.cardBg, theme.text, 2, 5);
                _currentAxis = AXIS_X;
                _subState = 10;
                _forceRedraw = true;
                draw(client);
                return 1;
            } else if (x >= 170 && x <= 300) {
                UIUtils::pressFeedback(_tft, 170, 75, 130, 55, LangManager::get("control_axis_y"), theme.cardBg, theme.text, 2, 5);
                _currentAxis = AXIS_Y;
                _subState = 10;
                _forceRedraw = true;
                draw(client);
                return 1;
            }
        } else if (y >= 138 && y <= 193) {
            if (x >= 20 && x <= 150) {
                UIUtils::pressFeedback(_tft, 20, 138, 130, 55, LangManager::get("control_axis_z"), theme.cardBg, theme.text, 2, 5);
                _currentAxis = AXIS_Z;
                _subState = 10;
                _forceRedraw = true;
                draw(client);
                return 1;
            } else if (x >= 170 && x <= 300) {
                UIUtils::pressFeedback(_tft, 170, 138, 130, 55, LangManager::get("control_axis_e"), theme.cardBg, theme.text, 2, 5);
                _currentAxis = AXIS_E;
                _subState = 10;
                _forceRedraw = true;
                draw(client);
                return 1;
            }
        }
    } 
    // 5. Kiválasztott tengely mozgatási nézete (_subState == 10)
    else if (_subState == 10) {
        float step = _moveStepSizes[_selectedMoveStepIndex];

        if (y >= 75 && y <= 135) {
            if ((x >= 20 && x <= 150) || (x >= 170 && x <= 300)) {
                bool isPlus = (x >= 170);
                float dirStep = isPlus ? step : -step;
                const char* btnLabel = isPlus ? "+" : "-";
                uint16_t btnX = isPlus ? 170 : 20;

                if (_currentAxis == AXIS_E) {
                    float toolTemp = client ? client->getData().nozzleTemp : 0.0f;
                    if (toolTemp < 220.0f) {
                        _showColdWarningPopup = true;
                        _forceRedraw = true;
                        draw(client);
                        return 1;
                    }
                }

                UIUtils::pressFeedback(_tft, btnX, 75, 130, 60, btnLabel, theme.cardBg, theme.text, 2, 5);
                sendMoveCommand(dirStep, client);
                return 1;
            }
        } else if (y >= 145 && y <= 190) {
            for (int i = 0; i < 4; i++) {
                int xPos = 20 + i * 70;
                if (x >= xPos && x <= (xPos + 60)) {
                    _selectedMoveStepIndex = i;
                    _forceRedraw = true;
                    draw(client);
                    return 1;
                }
            }
        }
    } 
    // 6. Temp választó menü (Bed / Nozzle) (_subState == 2)
    else if (_subState == 2) {
        if (y >= 85 && y <= 165) {
            if (x >= 20 && x <= 150) {
                UIUtils::pressFeedback(_tft, 20, 85, 130, 80, LangManager::get("control_bed_btn"), theme.cardBg, theme.text, 2, 5);
                _currentTempTarget = TEMP_BED;
                _subState = 20;
                _forceRedraw = true;
                draw(client);
                return 1;
            } else if (x >= 170 && x <= 300) {
                UIUtils::pressFeedback(_tft, 170, 85, 130, 80, LangManager::get("control_nozzle_btn"), theme.cardBg, theme.text, 2, 5);
                _currentTempTarget = TEMP_NOZZLE;
                _subState = 20;
                _forceRedraw = true;
                draw(client);
                return 1;
            }
        }
    }
    // 7. Hőmérséklet állító kijelző (_subState == 20)
    else if (_subState == 20) {
        int tempStep = _tempStepSizes[_selectedTempStepIndex];
        float currentTarget = 0.0f;
        if (client) {
            currentTarget = (_currentTempTarget == TEMP_BED) ? client->getData().bedTarget : client->getData().nozzleTarget;
        }

        if (y >= 112 && y <= 154) {
            // Mínusz gomb
            if (x >= 20 && x <= 150) {
                UIUtils::pressFeedback(_tft, 20, 112, 130, 42, "-", theme.cardBg, theme.text, 2, 5);
                sendTempCommand((int)currentTarget - tempStep, client);
                drawTempDisplayBox(client); // Csak a kijelző mezőt frissítjük
                return 1;
            }
            // Plusz gomb
            else if (x >= 170 && x <= 300) {
                UIUtils::pressFeedback(_tft, 170, 112, 130, 42, "+", theme.cardBg, theme.text, 2, 5);
                sendTempCommand((int)currentTarget + tempStep, client);
                drawTempDisplayBox(client); // Csak a kijelző mezőt frissítjük
                return 1;
            }
        } else if (y >= 158 && y <= 198) {
            // 1°C lépésköz választó
            if (x >= 20 && x <= 105) {
                _selectedTempStepIndex = 0;
                _forceRedraw = true;
                draw(client);
                return 1;
            }
            // 10°C lépésköz választó
            else if (x >= 117 && x <= 202) {
                _selectedTempStepIndex = 1;
                _forceRedraw = true;
                draw(client);
                return 1;
            }
            // Fűtés KI (0°C) gomb
            else if (x >= 215 && x <= 300) {
                UIUtils::pressFeedback(_tft, 215, 158, 85, 40, LangManager::get("control_btn_heat_off"), TFT_DARKGREY, TFT_WHITE, 1, 4);
                sendTempCommand(0, client);
                drawTempDisplayBox(client); // Csak a kijelző mezőt frissítjük
                return 1;
            }
        }
    }
    // 8. BLTouch menü érintései (_subState == 3)
    else if (_subState == 3) {
        if (!client) return 1;

        if (y >= 75 && y <= 130) {
            if (x >= 20 && x <= 150) {
                UIUtils::pressFeedback(_tft, 20, 75, 130, 55, LangManager::get("control_btn_pin_out"), theme.cardBg, theme.text, 2, 5);
                client->sendGcodeCommand("M280 P0 S10");
                return 1;
            } else if (x >= 170 && x <= 300) {
                UIUtils::pressFeedback(_tft, 170, 75, 130, 55, LangManager::get("control_btn_pin_in"), theme.cardBg, theme.text, 2, 5);
                client->sendGcodeCommand("M280 P0 S90");
                return 1;
            }
        } else if (y >= 138 && y <= 193) {
            if (x >= 20 && x <= 150) {
                UIUtils::pressFeedback(_tft, 20, 138, 130, 55, LangManager::get("control_btn_selftest"), theme.cardBg, theme.text, 2, 5);
                client->sendGcodeCommand("M280 P0 S120");
                return 1;
            } else if (x >= 170 && x <= 300) {
                UIUtils::pressFeedback(_tft, 170, 138, 130, 55, LangManager::get("control_btn_reset"), theme.cardBg, theme.text, 2, 5);
                client->sendGcodeCommand("M280 P0 S160");
                return 1;
            }
        }
    } 
    // 9. Fan (Ventilátor) menü érintései (_subState == 4)
    else if (_subState == 4) {
        if (y >= 70 && y <= 105 && x >= 20 && x <= 300) {
            UIUtils::pressFeedback(_tft, 20, 70, 280, 35, LangManager::get("control_btn_fan_off"), theme.accent, theme.bg, 2, 5);
            _fanPercent = 0;
            if (client) client->sendGcodeCommand("M106 S0");
            _forceRedraw = true;
            draw(client);
            return 1;
        }

        if (y >= 112 && y <= 154 && x >= 20 && x <= 150) {
            UIUtils::pressFeedback(_tft, 20, 112, 130, 42, "25%", theme.accent, theme.bg, 2, 5);
            _fanPercent = 25;
            int pwmVal = map(25, 0, 100, 0, 255);
            if (client) client->sendGcodeCommand("M106 S" + String(pwmVal));
            _forceRedraw = true;
            draw(client);
            return 1;
        }

        if (y >= 112 && y <= 154 && x >= 170 && x <= 300) {
            UIUtils::pressFeedback(_tft, 170, 112, 130, 42, "50%", theme.accent, theme.bg, 2, 5);
            _fanPercent = 50;
            int pwmVal = map(50, 0, 100, 0, 255);
            if (client) client->sendGcodeCommand("M106 S" + String(pwmVal));
            _forceRedraw = true;
            draw(client);
            return 1;
        }

        if (y >= 158 && y <= 200 && x >= 20 && x <= 150) {
            UIUtils::pressFeedback(_tft, 20, 158, 130, 42, "75%", theme.accent, theme.bg, 2, 5);
            _fanPercent = 75;
            int pwmVal = map(75, 0, 100, 0, 255);
            if (client) client->sendGcodeCommand("M106 S" + String(pwmVal));
            _forceRedraw = true;
            draw(client);
            return 1;
        }

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