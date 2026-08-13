#include "octo_other_calibration_mqtt.h"
#include "lang_manager.h"
#include "ui_utils.h"
#include "config_manager.h"

OctoOtherCalibrationMenuMqtt::OctoOtherCalibrationMenuMqtt(TFT_eSPI* tft) : _tft(tft) {}

void OctoOtherCalibrationMenuMqtt::init() {
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

void OctoOtherCalibrationMenuMqtt::draw(OctoClientMqtt* client) {
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

void OctoOtherCalibrationMenuMqtt::drawMainMenu() {
    ThemeColors theme = getCurrentTheme();
    _tft->fillRect(10, 45, 300, 190, theme.bg);
    _tft->setTextColor(theme.accent, theme.bg);
    _tft->setTextDatum(TC_DATUM);
    _tft->drawString(LangManager::get("calib_other_title"), 160, 48, 2);

    UIUtils::drawButton(_tft, 20, 75, 280, 36, LangManager::get("calib_btn_estep"), theme.cardBg, theme.text, false, 2, 5);
    UIUtils::drawButton(_tft, 20, 120, 280, 36, LangManager::get("calib_btn_pid"), theme.cardBg, theme.text, false, 2, 5);
    UIUtils::drawButton(_tft, 20, 165, 280, 36, LangManager::get("calib_btn_mpc"), theme.cardBg, theme.text, false, 2, 5);

    UIUtils::drawButton(_tft, 20, 208, 280, 24, LangManager::get("btn_back"), theme.cardBg, theme.text, false, 1, 4);
}

void OctoOtherCalibrationMenuMqtt::drawEStepMaterialMenu() {
    ThemeColors theme = getCurrentTheme();
    _tft->fillRect(10, 45, 300, 190, theme.bg);
    _tft->setTextColor(theme.accent, theme.bg);
    _tft->setTextDatum(TC_DATUM);
    _tft->drawString(LangManager::get("calib_estep_mat_title"), 160, 48, 2);

    _tft->setTextColor(theme.text, theme.bg);
    _tft->setTextDatum(MC_DATUM);
    _tft->drawString(LangManager::get("calib_estep_mat_sub"), 160, 80, 1);

    UIUtils::drawButton(_tft, 20, 95, 280, 32, LangManager::get("calib_mat_pla"), theme.cardBg, theme.text, false, 2, 5);
    UIUtils::drawButton(_tft, 20, 132, 280, 32, LangManager::get("calib_mat_petg"), theme.cardBg, theme.text, false, 2, 5);
    UIUtils::drawButton(_tft, 20, 169, 280, 32, LangManager::get("calib_mat_abs"), theme.cardBg, theme.text, false, 2, 5);

    UIUtils::drawButton(_tft, 20, 208, 280, 24, LangManager::get("btn_back"), theme.cardBg, theme.text, false, 1, 4);
}

void OctoOtherCalibrationMenuMqtt::drawEStepHeatingMenu(OctoClientMqtt* client) {
    ThemeColors theme = getCurrentTheme();
    _tft->fillRect(10, 45, 300, 190, theme.bg);
    _tft->setTextColor(theme.accent, theme.bg);
    _tft->setTextDatum(TC_DATUM);
    _tft->drawString(LangManager::get("calib_heating_title"), 160, 48, 2);

    float currentTemp = client ? client->getData().nozzleTemp : 0.0f;

    _tft->setTextColor(theme.text, theme.bg);
    _tft->setTextDatum(MC_DATUM);
    _tft->drawString(LangManager::get("calib_target_temp") + String(_targetTemp) + " C", 160, 90, 2);
    _tft->drawString(LangManager::get("calib_current_hotend") + String(currentTemp, 1) + " C", 160, 120, 2);

    if (currentTemp >= _targetTemp - 3) {
        _tft->setTextColor(TFT_GREEN, theme.bg);
        _tft->drawString(LangManager::get("calib_temp_reached"), 160, 150, 1);
        UIUtils::drawButton(_tft, 20, 172, 280, 32, LangManager::get("calib_btn_next_measure"), TFT_DARKGREEN, TFT_WHITE, false, 2, 5);
    } else {
        _tft->setTextColor(TFT_ORANGE, theme.bg);
        _tft->drawString(LangManager::get("calib_heating_in_progress"), 160, 150, 1);
    }

    UIUtils::drawButton(_tft, 20, 208, 280, 24, LangManager::get("btn_back"), theme.cardBg, theme.text, false, 1, 4);
}

void OctoOtherCalibrationMenuMqtt::drawEStepMeasureMenu() {
    ThemeColors theme = getCurrentTheme();
    _tft->fillRect(10, 45, 300, 190, theme.bg);
    _tft->setTextColor(theme.accent, theme.bg);
    _tft->setTextDatum(TC_DATUM);
    _tft->drawString(LangManager::get("calib_estep_title"), 160, 48, 2);

    UIUtils::drawButton(_tft, 20, 68, 280, 32, LangManager::get("calib_estep_extrude_btn"), theme.cardBg, theme.text, false, 2, 5);

    _tft->setTextColor(theme.text, theme.bg);
    _tft->setTextDatum(MC_DATUM);
    _tft->drawString(LangManager::get("calib_estep_diff_label"), 160, 108, 1);

    UIUtils::drawButton(_tft, 20, 178, 280, 28, LangManager::get("calib_btn_save_eeprom"), TFT_DARKGREEN, TFT_WHITE, false, 2, 5);
    UIUtils::drawButton(_tft, 20, 208, 280, 24, LangManager::get("btn_back"), theme.cardBg, theme.text, false, 1, 4);

    _tft->fillRoundRect(100, 122, 120, 30, 4, theme.cardBg);
    _tft->setTextColor(TFT_GOLD, theme.cardBg);
    _tft->setTextDatum(MC_DATUM);
    String diffStr = (_estepDiff > 0 ? "+" : "") + String(_estepDiff, 1) + " mm";
    _tft->drawString(diffStr, 160, 137, 2);

    UIUtils::drawButton(_tft, 20, 122, 45, 30, "-1", theme.cardBg, theme.text, false, 1, 4);
    UIUtils::drawButton(_tft, 70, 122, 25, 30, "-0.1", theme.cardBg, theme.text, false, 1, 3);
    UIUtils::drawButton(_tft, 225, 122, 25, 30, "+0.1", theme.cardBg, theme.text, false, 1, 3);
    UIUtils::drawButton(_tft, 255, 122, 45, 30, "+1", theme.cardBg, theme.text, false, 1, 4);
}

void OctoOtherCalibrationMenuMqtt::drawPidMenu() {
    ThemeColors theme = getCurrentTheme();
    _tft->fillRect(10, 45, 300, 190, theme.bg);
    _tft->setTextColor(theme.accent, theme.bg);
    _tft->setTextDatum(TC_DATUM);
    _tft->drawString(LangManager::get("calib_pid_title"), 160, 48, 2);

    _tft->setTextColor(theme.text, theme.bg);
    _tft->setTextDatum(MC_DATUM);
    _tft->drawString(LangManager::get("calib_pid_desc1"), 160, 95, 2);
    _tft->drawString(LangManager::get("calib_pid_desc2"), 160, 115, 2);
    _tft->drawString(LangManager::get("calib_pid_desc3"), 160, 135, 2);

    UIUtils::drawButton(_tft, 20, 160, 280, 38, LangManager::get("calib_pid_start_btn"), TFT_DARKGREEN, TFT_WHITE, false, 2, 5);
    UIUtils::drawButton(_tft, 20, 208, 280, 24, LangManager::get("btn_back"), theme.cardBg, theme.text, false, 1, 4);
}

void OctoOtherCalibrationMenuMqtt::drawPidRunningMenu(OctoClientMqtt* client) {
    ThemeColors theme = getCurrentTheme();
    static float lastTemp = -999.0f;
    static bool lastPidDone = false;

    float currentTemp = client ? client->getData().nozzleTemp : 0.0f;
    bool pidDone = false; 

    if (_forceRedraw) {
        _tft->fillRect(10, 45, 300, 190, theme.bg);
        _tft->setTextColor(theme.accent, theme.bg);
        _tft->setTextDatum(TC_DATUM);
        _tft->drawString(LangManager::get("calib_pid_running_title"), 160, 48, 2);

        _tft->setTextColor(theme.text, theme.bg);
        _tft->setTextDatum(MC_DATUM);
        _tft->drawString(LangManager::get("calib_exit_save_disabled"), 160, 145, 1);
        
        lastTemp = -999.0f;
        lastPidDone = !pidDone;
    }

    if (abs(currentTemp - lastTemp) >= 0.2f || pidDone != lastPidDone) {
        lastTemp = currentTemp;
        lastPidDone = pidDone;

        _tft->fillRect(20, 75, 280, 65, theme.bg);

        _tft->setTextColor(theme.text, theme.bg);
        _tft->setTextDatum(MC_DATUM);
        _tft->drawString(LangManager::get("calib_current_hotend") + String(currentTemp, 1) + " C", 160, 90, 2);

        if (pidDone) {
            _tft->setTextColor(TFT_GREEN, theme.bg);
            _tft->drawString(LangManager::get("calib_pid_done"), 160, 120, 2);
            UIUtils::drawButton(_tft, 20, 155, 280, 38, LangManager::get("calib_btn_save_eeprom"), TFT_DARKGREEN, TFT_WHITE, false, 2, 5);
        } else {
            _tft->setTextColor(TFT_ORANGE, theme.bg);
            _tft->drawString(LangManager::get("calib_waiting_cycles"), 160, 120, 1);
        }
    }
}

void OctoOtherCalibrationMenuMqtt::drawMpcMenu() {
    ThemeColors theme = getCurrentTheme();
    _tft->fillRect(10, 45, 300, 190, theme.bg);
    _tft->setTextColor(theme.accent, theme.bg);
    _tft->setTextDatum(TC_DATUM);
    _tft->drawString(LangManager::get("calib_mpc_title"), 160, 48, 2);

    _tft->setTextColor(theme.text, theme.bg);
    _tft->setTextDatum(MC_DATUM);
    _tft->drawString(LangManager::get("calib_mpc_desc1"), 160, 95, 2);
    _tft->drawString(LangManager::get("calib_mpc_desc2"), 160, 115, 2);
    _tft->drawString(LangManager::get("calib_mpc_desc3"), 160, 135, 2);

    UIUtils::drawButton(_tft, 20, 160, 280, 38, LangManager::get("calib_mpc_start_btn"), TFT_DARKGREEN, TFT_WHITE, false, 2, 5);
    UIUtils::drawButton(_tft, 20, 208, 280, 24, LangManager::get("btn_back"), theme.cardBg, theme.text, false, 1, 4);
}

void OctoOtherCalibrationMenuMqtt::drawMpcRunningMenu(OctoClientMqtt* client) {
    ThemeColors theme = getCurrentTheme();
    static float lastTemp = -999.0f;
    static bool lastMpcDone = false;

    float currentTemp = client ? client->getData().nozzleTemp : 0.0f;
    bool mpcDone = false;

    if (_forceRedraw) {
        _tft->fillRect(10, 45, 300, 190, theme.bg);
        _tft->setTextColor(theme.accent, theme.bg);
        _tft->setTextDatum(TC_DATUM);
        _tft->drawString(LangManager::get("calib_mpc_running_title"), 160, 48, 2);

        _tft->setTextColor(theme.text, theme.bg);
        _tft->setTextDatum(MC_DATUM);
        _tft->drawString(LangManager::get("calib_exit_save_disabled"), 160, 145, 1);
        
        lastTemp = -999.0f;
        lastMpcDone = !mpcDone;
    }

    if (abs(currentTemp - lastTemp) >= 0.2f || mpcDone != lastMpcDone) {
        lastTemp = currentTemp;
        lastMpcDone = mpcDone;

        _tft->fillRect(20, 75, 280, 65, theme.bg);

        _tft->setTextColor(theme.text, theme.bg);
        _tft->setTextDatum(MC_DATUM);
        _tft->drawString(LangManager::get("calib_current_hotend") + String(currentTemp, 1) + " C", 160, 90, 2);

        if (mpcDone) {
            _tft->setTextColor(TFT_GREEN, theme.bg);
            _tft->drawString(LangManager::get("calib_mpc_done"), 160, 120, 2);
            UIUtils::drawButton(_tft, 20, 155, 280, 38, LangManager::get("calib_btn_save_eeprom"), TFT_DARKGREEN, TFT_WHITE, false, 2, 5);
        } else {
            _tft->setTextColor(TFT_ORANGE, theme.bg);
            _tft->drawString(LangManager::get("calib_waiting_cycles"), 160, 120, 1);
        }
    }
}

void OctoOtherCalibrationMenuMqtt::drawPopup() {
    _tft->fillRoundRect(25, 65, 270, 120, 8, _tft->color565(30, 30, 30));
    _tft->drawRoundRect(25, 65, 270, 120, 8, _popupColor);
    _tft->drawRoundRect(26, 66, 268, 118, 7, _popupColor);

    _tft->setTextDatum(MC_DATUM);
    _tft->setTextColor(_popupColor, _tft->color565(30, 30, 30));
    _tft->drawString(_popupTitle, 160, 90, 2);

    _tft->setTextColor(TFT_WHITE, _tft->color565(30, 30, 30));
    _tft->drawString(_popupMsg1, 160, 120, 1);
    _tft->drawString(_popupMsg2, 160, 138, 1);

    UIUtils::drawButton(_tft, 95, 153, 130, 26, LangManager::get("btn_ok"), _tft->color565(60, 60, 60), TFT_WHITE, false, 1, 4);
}

void OctoOtherCalibrationMenuMqtt::saveEStepCalibration(OctoClientMqtt* client) {
    if (client) {
        float actualExtruded = 100.0f - _estepDiff;
        if (actualExtruded > 0) {
            client->sendGcodeCommand("M92 E[CALC_NEW_STEPS]");
        }
        client->sendGcodeCommand("M500");
        client->sendGcodeCommand("M104 S0");
        client->sendGcodeCommand("M140 S0");
    }

    _popupTitle = LangManager::get("calib_popup_save_success");
    _popupMsg1 = LangManager::get("calib_popup_estep_saved");
    _popupMsg2 = LangManager::get("calib_popup_cooldown_started");
    _popupColor = TFT_GREEN;
    _showPopup = true;
    _forceRedraw = true;
}

int OctoOtherCalibrationMenuMqtt::handleTouch(uint16_t x, uint16_t y, OctoClientMqtt* client) {
    ThemeColors theme = getCurrentTheme();
    if (_showPopup) {
        if (y >= 150 && y <= 185 && x >= 95 && x <= 225) {
            _showPopup = false;
            _subState = 0;
            _estepSubState = 0;
            _pidSubState = 0;
            _mpcSubState = 0;
            _pidCommandSent = false;
            _mpcCommandSent = false;
            _forceRedraw = true;
        }
        return 1;
    }

    if (_subState == 0) {
        if (y >= 200) {
            UIUtils::pressFeedback(_tft, 20, 208, 280, 24, LangManager::get("btn_back"), theme.cardBg, theme.text, 1, 4);
            return 0;
        }
        if (y >= 75 && y <= 111) {
            UIUtils::pressFeedback(_tft, 20, 75, 280, 36, LangManager::get("calib_btn_estep"), theme.cardBg, theme.text, 2, 5);
            _subState = 1; _estepSubState = 0; _forceRedraw = true; return 1;
        }
        if (y >= 120 && y <= 156) {
            UIUtils::pressFeedback(_tft, 20, 120, 280, 36, LangManager::get("calib_btn_pid"), theme.cardBg, theme.text, 2, 5);
            _subState = 2; _pidSubState = 0; _pidCommandSent = false; _forceRedraw = true; return 1;
        }
        if (y >= 165 && y <= 201) {
            UIUtils::pressFeedback(_tft, 20, 165, 280, 36, LangManager::get("calib_btn_mpc"), theme.cardBg, theme.text, 2, 5);
            _subState = 3; _mpcSubState = 0; _mpcCommandSent = false; _forceRedraw = true; return 1;
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
                    UIUtils::pressFeedback(_tft, 20, 172, 280, 32, LangManager::get("calib_btn_next_measure"), TFT_DARKGREEN, TFT_WHITE, false, 2, 5);
                    _estepSubState = 2;
                    _forceRedraw = true;
                    return 1;
                }
            }
        } else if (_estepSubState == 2) {
            if (y >= 208) { _subState = 0; _forceRedraw = true; return 1; }
            
            if (y >= 68 && y <= 100) {
                UIUtils::pressFeedback(_tft, 20, 68, 280, 32, LangManager::get("calib_estep_extrude_btn"), theme.cardBg, theme.text, false, 2, 5);
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
                UIUtils::pressFeedback(_tft, 20, 178, 280, 28, LangManager::get("calib_btn_save_eeprom"), TFT_DARKGREEN, TFT_WHITE, false, 2, 5);
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
                    UIUtils::pressFeedback(_tft, 20, 160, 280, 38, LangManager::get("calib_pid_start_btn"), TFT_DARKGREEN, TFT_WHITE, false, 2, 5);
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
        }
        return 1;
    }

    if (_subState == 3) { // MPC
        if (_mpcSubState == 0) {
            if (y >= 208) { _subState = 0; _forceRedraw = true; return 1; }
            if (y >= 160 && y <= 198) {
                if (!_mpcCommandSent) {
                    UIUtils::pressFeedback(_tft, 20, 160, 280, 38, LangManager::get("calib_mpc_start_btn"), TFT_DARKGREEN, TFT_WHITE, false, 2, 5);
                    if (client) {
                        client->sendGcodeCommand("M306 T");
                    }
                    _mpcCommandSent = true;
                    _mpcSubState = 1;
                    _forceRedraw = true;
                }
                return 1;
            }
        }
        return 1;
    }

    return -1;
}