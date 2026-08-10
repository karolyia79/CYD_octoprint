#include "octo_other_calibration.h"
#include "lang_manager.h"
#include "ui_utils.h"

OctoOtherCalibrationMenu::OctoOtherCalibrationMenu(TFT_eSPI* tft) : _tft(tft) {}

void OctoOtherCalibrationMenu::init() {
    _subState = 0;
    _estepSubState = 0;
    _pidSubState = 0;
    _mpcSubState = 0;
    _showPopup = false;
    _estepDiff = 0.0f;
    _targetTemp = 200;
    _pidCommandSent = false;
    _mpcCommandSent = false;
}

void OctoOtherCalibrationMenu::draw(OctoClient* client) {
    if (_showPopup) {
        if (_forceRedraw) {
            drawPopup();
            _forceRedraw = false;
        }
        return;
    }

    if (_subState == 0 && _forceRedraw) {
        drawMainMenu();
    } else if (_subState == 1) {
        if (_estepSubState == 0 && _forceRedraw) {
            drawEStepMaterialMenu();
        } else if (_estepSubState == 1) {
            drawEStepHeatingMenu(client);
        } else if (_estepSubState == 2 && _forceRedraw) {
            drawEStepMeasureMenu();
        }
    } else if (_subState == 2) {
        if (_pidSubState == 0 && _forceRedraw) {
            drawPidMenu();
        } else if (_pidSubState == 1) {
            drawPidRunningMenu(client);
        }
    } else if (_subState == 3) {
        if (_mpcSubState == 0 && _forceRedraw) {
            drawMpcMenu();
        } else if (_mpcSubState == 1) {
            drawMpcRunningMenu(client);
        }
    }
    _forceRedraw = false;
}

void OctoOtherCalibrationMenu::drawMainMenu() {
    _tft->fillRect(10, 45, 300, 190, TFT_BLACK);
    _tft->setTextColor(TFT_YELLOW, TFT_BLACK);
    _tft->setTextDatum(TC_DATUM);
    _tft->drawString("-- EGYEB KALIBRACIO --", 160, 48, 2);

    UIUtils::drawButton(_tft, 20, 75, 280, 36, "E-Step Kalibralas", _tft->color565(50, 50, 50), TFT_WHITE, false, 2, 5);
    UIUtils::drawButton(_tft, 20, 120, 280, 36, "PID Kalibralas (Auto)", _tft->color565(50, 50, 50), TFT_WHITE, false, 2, 5);
    UIUtils::drawButton(_tft, 20, 165, 280, 36, "MPC Kalibralas (Auto)", _tft->color565(50, 50, 50), TFT_WHITE, false, 2, 5);

    UIUtils::drawButton(_tft, 20, 208, 280, 24, LangManager::get("btn_back"), _tft->color565(100, 30, 30), TFT_WHITE, false, 1, 4);
}

void OctoOtherCalibrationMenu::drawEStepMaterialMenu() {
    _tft->fillRect(10, 45, 300, 190, TFT_BLACK);
    _tft->setTextColor(TFT_YELLOW, TFT_BLACK);
    _tft->setTextDatum(TC_DATUM);
    _tft->drawString("VALASZD KI AZ ANYAGOT", 160, 48, 2);

    _tft->setTextColor(TFT_WHITE, TFT_BLACK);
    _tft->setTextDatum(MC_DATUM);
    _tft->drawString("Futes elinditasa elott:", 160, 80, 1);

    UIUtils::drawButton(_tft, 20, 95, 280, 32, "PLA (200 C)", _tft->color565(50, 50, 50), TFT_WHITE, false, 2, 5);
    UIUtils::drawButton(_tft, 20, 132, 280, 32, "PETG (230 C)", _tft->color565(50, 50, 50), TFT_WHITE, false, 2, 5);
    UIUtils::drawButton(_tft, 20, 169, 280, 32, "ABS (240 C)", _tft->color565(50, 50, 50), TFT_WHITE, false, 2, 5);

    UIUtils::drawButton(_tft, 20, 208, 280, 24, LangManager::get("btn_back"), _tft->color565(100, 30, 30), TFT_WHITE, false, 1, 4);
}

void OctoOtherCalibrationMenu::drawEStepHeatingMenu(OctoClient* client) {
    _tft->fillRect(10, 45, 300, 190, TFT_BLACK);
    _tft->setTextColor(TFT_YELLOW, TFT_BLACK);
    _tft->setTextDatum(TC_DATUM);
    _tft->drawString("FUTES FOLYAMATBAN...", 160, 48, 2);

    float currentTemp = client ? client->getData().nozzleTemp : 0.0f;

    _tft->setTextColor(TFT_WHITE, TFT_BLACK);
    _tft->setTextDatum(MC_DATUM);
    _tft->drawString("Cel homerseklet: " + String(_targetTemp) + " C", 160, 90, 2);
    _tft->drawString("Aktualis hotend: " + String(currentTemp, 1) + " C", 160, 120, 2);

    if (currentTemp >= _targetTemp - 3) {
        _tft->setTextColor(TFT_GREEN, TFT_BLACK);
        _tft->drawString("Homerseklet elerve! Indithato.", 160, 150, 1);
        UIUtils::drawButton(_tft, 20, 172, 280, 32, "Tovabb a mereshez", _tft->color565(0, 100, 0), TFT_WHITE, false, 2, 5);
    } else {
        _tft->setTextColor(TFT_ORANGE, TFT_BLACK);
        _tft->drawString("Futes folyamatban...", 160, 150, 1);
    }

    UIUtils::drawButton(_tft, 20, 208, 280, 24, LangManager::get("btn_back"), _tft->color565(100, 30, 30), TFT_WHITE, false, 1, 4);
}

void OctoOtherCalibrationMenu::drawEStepMeasureMenu() {
    _tft->fillRect(10, 45, 300, 190, TFT_BLACK);
    _tft->setTextColor(TFT_YELLOW, TFT_BLACK);
    _tft->setTextDatum(TC_DATUM);
    _tft->drawString("E-STEP KALIBRALAS", 160, 48, 2);

    UIUtils::drawButton(_tft, 20, 68, 280, 32, "1. 100mm Extrudalas (G1 E100)", _tft->color565(50, 50, 50), TFT_WHITE, false, 2, 5);

    _tft->setTextColor(TFT_WHITE, TFT_BLACK);
    _tft->setTextDatum(MC_DATUM);
    _tft->drawString("Eteres / Maradek (mm):", 160, 108, 1);

    UIUtils::drawButton(_tft, 20, 178, 280, 28, "Mentes EEPROM-ba (M500)", _tft->color565(0, 100, 0), TFT_WHITE, false, 2, 5);
    UIUtils::drawButton(_tft, 20, 208, 280, 24, LangManager::get("btn_back"), _tft->color565(100, 30, 30), TFT_WHITE, false, 1, 4);

    _tft->fillRoundRect(100, 122, 120, 30, 4, _tft->color565(35, 35, 35));
    _tft->setTextColor(TFT_GOLD, _tft->color565(35, 35, 35));
    _tft->setTextDatum(MC_DATUM);
    String diffStr = (_estepDiff > 0 ? "+" : "") + String(_estepDiff, 1) + " mm";
    _tft->drawString(diffStr, 160, 137, 2);

    UIUtils::drawButton(_tft, 20, 122, 45, 30, "-1", _tft->color565(60, 60, 60), TFT_WHITE, false, 1, 4);
    UIUtils::drawButton(_tft, 70, 122, 25, 30, "-0.1", _tft->color565(60, 60, 60), TFT_WHITE, false, 1, 3);
    UIUtils::drawButton(_tft, 225, 122, 25, 30, "+0.1", _tft->color565(60, 60, 60), TFT_WHITE, false, 1, 3);
    UIUtils::drawButton(_tft, 255, 122, 45, 30, "+1", _tft->color565(60, 60, 60), TFT_WHITE, false, 1, 4);
}

void OctoOtherCalibrationMenu::drawPidMenu() {
    _tft->fillRect(10, 45, 300, 190, TFT_BLACK);
    _tft->setTextColor(TFT_YELLOW, TFT_BLACK);
    _tft->setTextDatum(TC_DATUM);
    _tft->drawString("PID KALIBRALAS", 160, 48, 2);

    _tft->setTextColor(TFT_WHITE, TFT_BLACK);
    _tft->setTextDatum(MC_DATUM);
    _tft->drawString("Automata hotend hangolas.", 160, 95, 2);
    _tft->drawString("Elinditja az M303 parancsot,", 160, 115, 2);
    _tft->drawString("majd a futes vegen mentheto.", 160, 135, 2);

    UIUtils::drawButton(_tft, 20, 160, 280, 38, "PID Inditasa (M303)", _tft->color565(0, 100, 0), TFT_WHITE, false, 2, 5);
    UIUtils::drawButton(_tft, 20, 208, 280, 24, LangManager::get("btn_back"), _tft->color565(100, 30, 30), TFT_WHITE, false, 1, 4);
}

void OctoOtherCalibrationMenu::drawPidRunningMenu(OctoClient* client) {
    static float lastTemp = -999.0f;
    static bool lastPidDone = false;

    float currentTemp = client ? client->getData().nozzleTemp : 0.0f;
    bool pidDone = client ? client->isPidFinished() : false;

    if (_forceRedraw) {
        _tft->fillRect(10, 45, 300, 190, TFT_BLACK);
        _tft->setTextColor(TFT_YELLOW, TFT_BLACK);
        _tft->setTextDatum(TC_DATUM);
        _tft->drawString("PID FOLYAMATBAN...", 160, 48, 2);

        _tft->setTextColor(TFT_WHITE, TFT_BLACK);
        _tft->setTextDatum(MC_DATUM);
        _tft->drawString("Kilepes es mentes tiltva!", 160, 145, 1);
        
        lastTemp = -999.0f;
        lastPidDone = !pidDone;
    }

    if (abs(currentTemp - lastTemp) >= 0.2f || pidDone != lastPidDone) {
        lastTemp = currentTemp;
        lastPidDone = pidDone;

        _tft->fillRect(20, 75, 280, 65, TFT_BLACK);

        _tft->setTextColor(TFT_WHITE, TFT_BLACK);
        _tft->setTextDatum(MC_DATUM);
        _tft->drawString("Aktualis hotend: " + String(currentTemp, 1) + " C", 160, 90, 2);

        if (pidDone) {
            _tft->setTextColor(TFT_GREEN, TFT_BLACK);
            _tft->drawString("PID Autotune kesz!", 160, 120, 2);
            UIUtils::drawButton(_tft, 20, 155, 280, 38, "Mentes EEPROM-ba (M500)", _tft->color565(0, 100, 0), TFT_WHITE, false, 2, 5);
        } else {
            _tft->setTextColor(TFT_ORANGE, TFT_BLACK);
            _tft->drawString("Varakozas a ciklusokra...", 160, 120, 1);
        }
    }
}

void OctoOtherCalibrationMenu::drawMpcMenu() {
    _tft->fillRect(10, 45, 300, 190, TFT_BLACK);
    _tft->setTextColor(TFT_YELLOW, TFT_BLACK);
    _tft->setTextDatum(TC_DATUM);
    _tft->drawString("MPC KALIBRALAS", 160, 48, 2);

    _tft->setTextColor(TFT_WHITE, TFT_BLACK);
    _tft->setTextDatum(MC_DATUM);
    _tft->drawString("Model Predictive Control", 160, 95, 2);
    _tft->drawString("Automata hangolas es mentes", 160, 115, 2);
    _tft->drawString("az EEPROM-ba.", 160, 135, 2);

    UIUtils::drawButton(_tft, 20, 160, 280, 38, "MPC Inditasa", _tft->color565(0, 100, 0), TFT_WHITE, false, 2, 5);
    UIUtils::drawButton(_tft, 20, 208, 280, 24, LangManager::get("btn_back"), _tft->color565(100, 30, 30), TFT_WHITE, false, 1, 4);
}

void OctoOtherCalibrationMenu::drawMpcRunningMenu(OctoClient* client) {
    static float lastTemp = -999.0f;
    static bool lastMpcDone = false;

    float currentTemp = client ? client->getData().nozzleTemp : 0.0f;
    bool mpcDone = client ? client->isMpcFinished() : false;

    if (_forceRedraw) {
        _tft->fillRect(10, 45, 300, 190, TFT_BLACK);
        _tft->setTextColor(TFT_YELLOW, TFT_BLACK);
        _tft->setTextDatum(TC_DATUM);
        _tft->drawString("MPC FOLYAMATBAN...", 160, 48, 2);

        _tft->setTextColor(TFT_WHITE, TFT_BLACK);
        _tft->setTextDatum(MC_DATUM);
        _tft->drawString("Kilepes es mentes tiltva!", 160, 145, 1);
        
        lastTemp = -999.0f;
        lastMpcDone = !mpcDone;
    }

    if (abs(currentTemp - lastTemp) >= 0.2f || mpcDone != lastMpcDone) {
        lastTemp = currentTemp;
        lastMpcDone = mpcDone;

        _tft->fillRect(20, 75, 280, 65, TFT_BLACK);

        _tft->setTextColor(TFT_WHITE, TFT_BLACK);
        _tft->setTextDatum(MC_DATUM);
        _tft->drawString("Aktualis hotend: " + String(currentTemp, 1) + " C", 160, 90, 2);

        if (mpcDone) {
            _tft->setTextColor(TFT_GREEN, TFT_BLACK);
            _tft->drawString("MPC Autotune kesz!", 160, 120, 2);
            UIUtils::drawButton(_tft, 20, 155, 280, 38, "Mentes EEPROM-ba (M500)", _tft->color565(0, 100, 0), TFT_WHITE, false, 2, 5);
        } else {
            _tft->setTextColor(TFT_ORANGE, TFT_BLACK);
            _tft->drawString("Varakozas a ciklusokra...", 160, 120, 1);
        }
    }
}

void OctoOtherCalibrationMenu::drawPopup() {
    _tft->fillRoundRect(25, 65, 270, 120, 8, _tft->color565(30, 30, 30));
    _tft->drawRoundRect(25, 65, 270, 120, 8, _popupColor);
    _tft->drawRoundRect(26, 66, 268, 118, 7, _popupColor);

    _tft->setTextDatum(MC_DATUM);
    _tft->setTextColor(_popupColor, _tft->color565(30, 30, 30));
    _tft->drawString(_popupTitle, 160, 90, 2);

    _tft->setTextColor(TFT_WHITE, _tft->color565(30, 30, 30));
    _tft->drawString(_popupMsg1, 160, 120, 1);
    _tft->drawString(_popupMsg2, 160, 138, 1);

    UIUtils::drawButton(_tft, 95, 153, 130, 26, "OK", _tft->color565(60, 60, 60), TFT_WHITE, false, 1, 4);
}

void OctoOtherCalibrationMenu::saveEStepCalibration(OctoClient* client) {
    if (client) {
        float actualExtruded = 100.0f - _estepDiff;
        if (actualExtruded > 0) {
            client->sendGcodeCommand("M92 E[CALC_NEW_STEPS]");
        }
        client->sendGcodeCommand("M500");
        client->sendGcodeCommand("M104 S0");
        client->sendGcodeCommand("M140 S0");
    }

    _popupTitle = "SIKERES MENTES";
    _popupMsg1 = "E-Step elmentve & Hutes OK";
    _popupMsg2 = "Cooldown (M104/M140 S0) elinditva!";
    _popupColor = TFT_GREEN;
    _showPopup = true;
    _forceRedraw = true;
}

int OctoOtherCalibrationMenu::handleTouch(uint16_t x, uint16_t y, OctoClient* client) {
    if (_showPopup) {
        if (y >= 150 && y <= 185 && x >= 95 && x <= 225) {
            _showPopup = false;
            _subState = 0;
            _estepSubState = 0;
            _pidSubState = 0;
            _mpcSubState = 0;
            _pidCommandSent = false;
            _mpcCommandSent = false;
            if (client) client->resetCalibrationFlags();
            _forceRedraw = true;
        }
        return 1;
    }

    if (_subState == 0) {
        if (y >= 200) {
            UIUtils::pressFeedback(_tft, 20, 208, 280, 24, LangManager::get("btn_back"), _tft->color565(100, 30, 30), TFT_WHITE, 1, 4);
            return 0;
        }
        if (y >= 75 && y <= 111) {
            UIUtils::pressFeedback(_tft, 20, 75, 280, 36, "E-Step Kalibralas", _tft->color565(50, 50, 50), TFT_WHITE, 2, 5);
            _subState = 1; _estepSubState = 0; _forceRedraw = true; return 1;
        }
        if (y >= 120 && y <= 156) {
            UIUtils::pressFeedback(_tft, 20, 120, 280, 36, "PID Kalibralas (Auto)", _tft->color565(50, 50, 50), TFT_WHITE, 2, 5);
            _subState = 2; _pidSubState = 0; _pidCommandSent = false; if (client) client->resetCalibrationFlags(); _forceRedraw = true; return 1;
        }
        if (y >= 165 && y <= 201) {
            UIUtils::pressFeedback(_tft, 20, 165, 280, 36, "MPC Kalibralas (Auto)", _tft->color565(50, 50, 50), TFT_WHITE, 2, 5);
            _subState = 3; _mpcSubState = 0; _mpcCommandSent = false; if (client) client->resetCalibrationFlags(); _forceRedraw = true; return 1;
        }
        return -1;
    }

    if (_subState == 1) {
        if (_estepSubState == 0) {
            if (y >= 208) { _subState = 0; _forceRedraw = true; return 1; }
            
            int selectedTemp = 0;
            if (y >= 95 && y <= 127) selectedTemp = 200;
            else if (y >= 132 && y <= 164) selectedTemp = 230;
            else if (y >= 169 && y <= 201) selectedTemp = 240;

            if (selectedTemp > 0) {
                _targetTemp = selectedTemp;
                if (client) {
                    client->sendGcodeCommand("M104 S" + String(_targetTemp));
                }
                _estepSubState = 1;
                _forceRedraw = true;
                return 1;
            }
        } else if (_estepSubState == 1) {
            if (y >= 208) { _subState = 0; _forceRedraw = true; return 1; }
            
            float currentTemp = client ? client->getData().nozzleTemp : 0.0f;
            if (currentTemp >= _targetTemp - 3) {
                if (y >= 172 && y <= 204) {
                    UIUtils::pressFeedback(_tft, 20, 172, 280, 32, "Tovabb a mereshez", _tft->color565(0, 100, 0), TFT_WHITE, 2, 5);
                    _estepSubState = 2;
                    _forceRedraw = true;
                    return 1;
                }
            }
        } else if (_estepSubState == 2) {
            if (y >= 208) { _subState = 0; _forceRedraw = true; return 1; }
            
            if (y >= 68 && y <= 100) {
                UIUtils::pressFeedback(_tft, 20, 68, 280, 32, "1. 100mm Extrudalas (G1 E100)", _tft->color565(50, 50, 50), TFT_WHITE, 2, 5);
                if (client) {
                    float currentTemp = client->getData().nozzleTemp;
                    if (currentTemp >= _targetTemp - 5) {
                        client->sendGcodeCommand("G1 E100 F100");
                    }
                }
                return 1;
            }

            if (y >= 122 && y <= 152) {
                if (x >= 20 && x <= 65)  { _estepDiff -= 1.0f; return 1; }
                if (x >= 70 && x <= 95)  { _estepDiff -= 0.1f; return 1; }
                if (x >= 225 && x <= 250) { _estepDiff += 0.1f; return 1; }
                if (x >= 255 && x <= 300) { _estepDiff += 1.0f; return 1; }
            }

            if (y >= 178 && y <= 206) {
                UIUtils::pressFeedback(_tft, 20, 178, 280, 28, "Mentes EEPROM-ba (M500)", _tft->color565(0, 100, 0), TFT_WHITE, 2, 5);
                saveEStepCalibration(client);
                return 1;
            }
        }
        return 1;
    }

    if (_subState == 2) { // PID
        if (_pidSubState == 0) {
            if (y >= 208) { _subState = 0; _forceRedraw = true; return 1; }
            if (y >= 160 && y <= 198) {
                if (!_pidCommandSent) {
                    UIUtils::pressFeedback(_tft, 20, 160, 280, 38, "PID Inditasa (M303)", _tft->color565(0, 100, 0), TFT_WHITE, 2, 5);
                    if (client) {
                        client->sendGcodeCommand("M106 S255");
                        client->sendGcodeCommand("M303 E0 S200 C5");
                    }
                    _pidCommandSent = true;
                    _pidSubState = 1;
                    _forceRedraw = true;
                }
                return 1;
            }
        } else if (_pidSubState == 1) {
            bool pidDone = client ? client->isPidFinished() : false;
            if (pidDone && y >= 155 && y <= 193) {
                UIUtils::pressFeedback(_tft, 20, 155, 280, 38, "Mentes EEPROM-ba (M500)", _tft->color565(0, 100, 0), TFT_WHITE, 2, 5);
                if (client) {
                    client->sendGcodeCommand("M500");
                }
                _popupTitle = "SIKERES MENTES";
                _popupMsg1 = "PID ertekek elmentve";
                _popupMsg2 = "az EEPROM-ba (M500)!";
                _popupColor = TFT_GREEN;
                _showPopup = true;
                _forceRedraw = true;
                return 1;
            }
        }
        return 1;
    }

    if (_subState == 3) { // MPC
        if (_mpcSubState == 0) {
            if (y >= 208) { _subState = 0; _forceRedraw = true; return 1; }
            if (y >= 160 && y <= 198) {
                if (client && !client->supportsCustomMesh()) {
                    _popupTitle = "HIBA: NEM TAMOGATOTT";
                    _popupMsg1 = "A nyomtato firmware nem";
                    _popupMsg2 = "ismeri az MPC kalibraciot!";
                    _popupColor = TFT_RED;
                    _showPopup = true;
                    _forceRedraw = true;
                    return 1;
                }
                if (!_mpcCommandSent) {
                    UIUtils::pressFeedback(_tft, 20, 160, 280, 38, "MPC Inditasa", _tft->color565(0, 100, 0), TFT_WHITE, 2, 5);
                    if (client) {
                        client->sendGcodeCommand("M306 T");
                    }
                    _mpcCommandSent = true;
                    _mpcSubState = 1;
                    _forceRedraw = true;
                }
                return 1;
            }
        } else if (_mpcSubState == 1) {
            bool mpcDone = client ? client->isMpcFinished() : false;
            if (mpcDone && y >= 155 && y <= 193) {
                UIUtils::pressFeedback(_tft, 20, 155, 280, 38, "Mentes EEPROM-ba (M500)", _tft->color565(0, 100, 0), TFT_WHITE, 2, 5);
                if (client) {
                    client->sendGcodeCommand("M500");
                }
                _popupTitle = "SIKERES MENTES";
                _popupMsg1 = "MPC ertekek elmentve";
                _popupMsg2 = "az EEPROM-ba (M500)!";
                _popupColor = TFT_GREEN;
                _showPopup = true;
                _forceRedraw = true;
                return 1;
            }
        }
        return 1;
    }

    return -1;
}