#include "octo_tune_menu.h"
#include "lang_manager.h"
#include <SD.h>

// TJpgDec statikus kimeneti referencia a kijelzohoz
static TFT_eSPI* g_tftPtr = nullptr;

static bool tjpgOutput(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    if (g_tftPtr) {
        g_tftPtr->pushImage(x, y, w, h, bitmap);
    }
    return true;
}

OctoTuneMenu::OctoTuneMenu(TFT_eSPI* tft) 
    : _tft(tft), _subMenu(TUNE_MAIN), _forceRedraw(true), _cameraLoaded(false) {}

void OctoTuneMenu::init() {
    _subMenu = TUNE_MAIN;
    _cameraLoaded = false;
    _forceRedraw = true;
}

void OctoTuneMenu::draw(OctoClientMqtt* client) {
    switch (_subMenu) {
        case TUNE_MAIN:
            drawMainGrid();
            break;
        case TUNE_TEMP:
            drawTempMenu(client);
            break;
        case TUNE_SPEED:
            drawSpeedMenu(client);
            break;
        case TUNE_ZOFFSET:
            drawZOffsetMenu(client);
            break;
        case TUNE_CAMERA:
            drawCameraMenu(client);
            break;
    }
    _forceRedraw = false;
}

void OctoTuneMenu::drawHeader(const String& title) {
    ThemeColors theme = getCurrentTheme();
    _tft->fillRect(0, 0, 320, 35, theme.cardBg);
    _tft->setTextColor(theme.text, theme.cardBg);
    _tft->setTextDatum(MC_DATUM);
    _tft->drawString(title, 160, 17, 2);
    _tft->drawFastHLine(0, 35, 320, theme.subText);
}

void OctoTuneMenu::drawBackButton() {
    ThemeColors theme = getCurrentTheme();
    UIUtils::drawButton(_tft, 10, 202, 300, 34, LangManager::get("btn_back"), theme.cardBg, theme.text, false, 1, 4);
}

// --- 2x2 FO TUNE MENU RACS ---
void OctoTuneMenu::drawMainGrid() {
    if (!_forceRedraw) return;

    ThemeColors theme = getCurrentTheme();
    _tft->fillScreen(theme.bg);
    drawHeader(LangManager::get("tune_header_main"));

    // 1. Homerseklet (Bal fent)
    UIUtils::drawButton(_tft, 10, 45, 145, 70, LangManager::get("tune_btn_temp"), theme.cardBg, theme.text, false, 2, 5);

    // 2. Sebesseg (Jobb fent)
    UIUtils::drawButton(_tft, 165, 45, 145, 70, LangManager::get("tune_btn_speed"), theme.cardBg, theme.text, false, 2, 5);

    // 3. Z-Offset (Bal lent)
    UIUtils::drawButton(_tft, 10, 125, 145, 70, LangManager::get("tune_btn_zoffset"), theme.cardBg, theme.text, false, 2, 5);

    // 4. Kamerakep (Jobb lent)
    UIUtils::drawButton(_tft, 165, 125, 145, 70, LangManager::get("tune_btn_camera"), theme.cardBg, theme.text, false, 2, 5);

    // Vissza gomb
    drawBackButton();
}

// --- 1. HOMERSEKLET ALMENU (MQTT) ---
void OctoTuneMenu::drawTempMenu(OctoClientMqtt* client) {
    ThemeColors theme = getCurrentTheme();

    if (_forceRedraw) {
        _tft->fillScreen(theme.bg);
        drawHeader(LangManager::get("tune_header_temp"));

        // FEJ GOMBOK (Y: 62 -> 107, H: 45)
        UIUtils::drawButton(_tft, 10,  62, 70, 45, "-5C", theme.cardBg, theme.text, false, 2, 5);
        UIUtils::drawButton(_tft, 86,  62, 70, 45, "-1C", theme.cardBg, theme.text, false, 2, 5);
        UIUtils::drawButton(_tft, 164, 62, 70, 45, "+1C", theme.cardBg, theme.text, false, 2, 5);
        UIUtils::drawButton(_tft, 240, 62, 70, 45, "+5C", theme.cardBg, theme.text, false, 2, 5);

        // AGY GOMBOK (Y: 138 -> 183, H: 45)
        UIUtils::drawButton(_tft, 10,  138, 70, 45, "-5C", theme.cardBg, theme.text, false, 2, 5);
        UIUtils::drawButton(_tft, 86,  138, 70, 45, "-1C", theme.cardBg, theme.text, false, 2, 5);
        UIUtils::drawButton(_tft, 164, 138, 70, 45, "+1C", theme.cardBg, theme.text, false, 2, 5);
        UIUtils::drawButton(_tft, 240, 138, 70, 45, "+5C", theme.cardBg, theme.text, false, 2, 5);

        drawBackButton();
    }

    if (client) {
        _tft->setTextDatum(MC_DATUM);
        _tft->setTextColor(theme.accent, theme.bg);
        char buf[32];

        // Fej elo homerseklet kijelzes (Y: 48)
        snprintf(buf, sizeof(buf), LangManager::get("tune_head_fmt").c_str(), 
                 client->getData().nozzleTemp, client->getData().nozzleTarget);
        _tft->fillRect(10, 40, 300, 18, theme.bg);
        _tft->drawString(buf, 160, 48, 2);

        // Agy elo homerseklet kijelzes (Y: 124)
        snprintf(buf, sizeof(buf), LangManager::get("tune_bed_fmt").c_str(), 
                 client->getData().bedTemp, client->getData().bedTarget);
        _tft->fillRect(10, 116, 300, 18, theme.bg);
        _tft->drawString(buf, 160, 124, 2);
    }
}

// --- 2. SEBESSEG ALMENU (MQTT) ---
void OctoTuneMenu::drawSpeedMenu(OctoClientMqtt* client) {
    ThemeColors theme = getCurrentTheme();
    if (_forceRedraw) {
        _tft->fillScreen(theme.bg);
        drawHeader(LangManager::get("tune_header_speed"));

        UIUtils::drawButton(_tft, 10, 50, 68, 50, "-10%", theme.cardBg, theme.text, false, 2, 5);
        UIUtils::drawButton(_tft, 86, 50, 68, 50, "-1%", theme.cardBg, theme.text, false, 2, 5);
        UIUtils::drawButton(_tft, 164, 50, 68, 50, "+1%", theme.cardBg, theme.text, false, 2, 5);
        UIUtils::drawButton(_tft, 240, 50, 68, 50, "+10%", theme.cardBg, theme.text, false, 2, 5);

        UIUtils::drawButton(_tft, 60, 120, 200, 45, LangManager::get("tune_btn_default_speed"), theme.cardBg, theme.text, false, 2, 5);

        drawBackButton();
    }

    if (client) {
        _tft->setTextDatum(MC_DATUM);
        _tft->setTextColor(theme.accent, theme.bg);
        String speedStr = LangManager::get("tune_curr_speed") + String(client->getData().speed) + "%";
        _tft->fillRect(10, 178, 300, 20, theme.bg);
        _tft->drawString(speedStr, 160, 185, 2);
    }
}

// --- 3. Z-OFFSET ALMENU (MQTT) ---
void OctoTuneMenu::drawZOffsetMenu(OctoClientMqtt* client) {
    if (!_forceRedraw) return;

    ThemeColors theme = getCurrentTheme();
    _tft->fillScreen(theme.bg);
    drawHeader(LangManager::get("tune_header_zoffset"));

    UIUtils::drawButton(_tft, 10, 50, 92, 45, "-0.10", theme.cardBg, theme.text, false, 2, 5);
    UIUtils::drawButton(_tft, 114, 50, 92, 45, "-0.025", theme.cardBg, theme.text, false, 2, 5);
    UIUtils::drawButton(_tft, 218, 50, 92, 45, "-0.005", theme.cardBg, theme.text, false, 2, 5);

    UIUtils::drawButton(_tft, 10, 105, 92, 45, "+0.10", theme.cardBg, theme.text, false, 2, 5);
    UIUtils::drawButton(_tft, 114, 105, 92, 45, "+0.025", theme.cardBg, theme.text, false, 2, 5);
    UIUtils::drawButton(_tft, 218, 105, 92, 45, "+0.005", theme.cardBg, theme.text, false, 2, 5);

    drawBackButton();
}

// --- 4. KAMERAKEP MEGJELENITESE (SD KARTYAS STREAMELESSEL) ---
void OctoTuneMenu::drawCameraMenu(OctoClientMqtt* client) {
    if (!_forceRedraw || _cameraLoaded) return;

    ThemeColors theme = getCurrentTheme();
    _tft->fillScreen(theme.bg);
    drawHeader(LangManager::get("tune_header_camera"));

    _tft->setTextDatum(MC_DATUM);
    _tft->setTextColor(theme.text, theme.bg);
    _tft->drawString(LangManager::get("tune_cam_downloading"), 160, 120, 2);

    PrinterConfig config = ConfigManager::loadConfig();
    String host = config.octo_ip;
    if (host.startsWith("http://")) host = host.substring(7);
    else if (host.startsWith("https://")) host = host.substring(8);
    if (host.endsWith("/")) host = host.substring(0, host.length() - 1);

    if (host.length() == 0) {
        _tft->fillRect(10, 50, 300, 140, theme.bg);
        _tft->setTextColor(TFT_RED, theme.bg);
        _tft->drawString(LangManager::get("tune_cam_no_ip"), 160, 120, 2);
        drawBackButton();
        _cameraLoaded = true;
        return;
    }

    String url = "http://" + host + "/webcam/?action=snapshot";

    HTTPClient http;
    http.setTimeout(4000);
    http.begin(url);

    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        if (SD.exists("/cam_snap.jpg")) {
            SD.remove("/cam_snap.jpg");
        }

        File file = SD.open("/cam_snap.jpg", FILE_WRITE);
        if (file) {
            WiFiClient* stream = http.getStreamPtr();
            uint8_t buff[512];
            int totalBytes = 0;
            unsigned long lastDataTime = millis();

            while (http.connected() && (millis() - lastDataTime < 4000)) {
                size_t avail = stream->available();
                if (avail > 0) {
                    int c = stream->readBytes(buff, min(avail, sizeof(buff)));
                    file.write(buff, c);
                    totalBytes += c;
                    lastDataTime = millis();
                }
                yield();
            }
            file.close();

            if (totalBytes > 0) {
                _tft->fillScreen(TFT_BLACK);

                g_tftPtr = _tft;
                TJpgDec.setJpgScale(1);
                TJpgDec.setCallback(tjpgOutput);
                TJpgDec.setSwapBytes(true);

                uint16_t w = 0, h = 0;
                TJpgDec.getSdJpgSize(&w, &h, "/cam_snap.jpg");
                
                if (w > 320 || h > 240) {
                    TJpgDec.setJpgScale(2);
                }

                TJpgDec.drawSdJpg(0, 0, "/cam_snap.jpg");
                SD.remove("/cam_snap.jpg");
            } else {
                _tft->fillRect(10, 50, 300, 140, theme.bg);
                _tft->setTextColor(TFT_RED, theme.bg);
                _tft->drawString(LangManager::get("tune_cam_empty_data"), 160, 120, 2);
                drawBackButton();
            }
        } else {
            _tft->fillRect(10, 50, 300, 140, theme.bg);
            _tft->setTextColor(TFT_RED, theme.bg);
            _tft->drawString(LangManager::get("tune_cam_sd_error"), 160, 120, 2);
            drawBackButton();
        }
    } else {
        _tft->fillRect(10, 50, 300, 140, theme.bg);
        _tft->setTextColor(TFT_RED, theme.bg);
        _tft->drawString(LangManager::get("tune_cam_error_http") + String(httpCode) + ")", 160, 120, 2);
        drawBackButton();
    }

    http.end();
    _cameraLoaded = true;
}

// --- ERINTESKEZELES ---
int OctoTuneMenu::handleTouch(uint16_t x, uint16_t y, OctoClientMqtt* client) {
    ThemeColors theme = getCurrentTheme();

    // 1. FO TUNE MENU ERINTESEK (2x2 RACS)
    if (_subMenu == TUNE_MAIN) {
        if (x >= 10 && x <= 155 && y >= 45 && y <= 115) {
            UIUtils::pressFeedback(_tft, 10, 45, 145, 70, LangManager::get("tune_btn_temp"), theme.cardBg, theme.text, 2, 5);
            _subMenu = TUNE_TEMP;
            _forceRedraw = true;
            return 1;
        }
        else if (x >= 165 && x <= 310 && y >= 45 && y <= 115) {
            UIUtils::pressFeedback(_tft, 165, 45, 145, 70, LangManager::get("tune_btn_speed"), theme.cardBg, theme.text, 2, 5);
            _subMenu = TUNE_SPEED;
            _forceRedraw = true;
            return 1;
        }
        else if (x >= 10 && x <= 155 && y >= 125 && y <= 195) {
            UIUtils::pressFeedback(_tft, 10, 125, 145, 70, LangManager::get("tune_btn_zoffset"), theme.cardBg, theme.text, 2, 5);
            _subMenu = TUNE_ZOFFSET;
            _forceRedraw = true;
            return 1;
        }
        else if (x >= 165 && x <= 310 && y >= 125 && y <= 195) {
            UIUtils::pressFeedback(_tft, 165, 125, 145, 70, LangManager::get("tune_btn_camera"), theme.cardBg, theme.text, 2, 5);
            _subMenu = TUNE_CAMERA;
            _cameraLoaded = false;
            _forceRedraw = true;
            return 1;
        }
        else if (x >= 10 && x <= 310 && y >= 202 && y <= 236) {
            UIUtils::pressFeedback(_tft, 10, 202, 300, 34, LangManager::get("btn_back"), theme.cardBg, theme.text, 2, 5);
            return 0; 
        }
    }

    // 2. HOMERSEKLET MENU ERINTESEK
    else if (_subMenu == TUNE_TEMP) {
        if (client) {
            float nTar = client->getData().nozzleTarget;
            float bTar = client->getData().bedTarget;

            // Fej gombok (Y: 62 -> 107)
            if (y >= 62 && y <= 107) {
                if (x >= 10 && x <= 80)        client->setNozzleTarget(max(0.0f, nTar - 5.0f));
                else if (x >= 86 && x <= 156)  client->setNozzleTarget(max(0.0f, nTar - 1.0f));
                else if (x >= 164 && x <= 234) client->setNozzleTarget(nTar + 1.0f);
                else if (x >= 240 && x <= 310) client->setNozzleTarget(nTar + 5.0f);
            }
            // Agy gombok (Y: 138 -> 183)
            else if (y >= 138 && y <= 183) {
                if (x >= 10 && x <= 80)        client->setBedTarget(max(0.0f, bTar - 5.0f));
                else if (x >= 86 && x <= 156)  client->setBedTarget(max(0.0f, bTar - 1.0f));
                else if (x >= 164 && x <= 234) client->setBedTarget(bTar + 1.0f);
                else if (x >= 240 && x <= 310) client->setBedTarget(bTar + 5.0f);
            }
        }

        if (x >= 10 && x <= 310 && y >= 202 && y <= 236) {
            _subMenu = TUNE_MAIN;
            _forceRedraw = true;
            return 1;
        }
    }

    // 3. SEBESSEG MENU ERINTESEK
    else if (_subMenu == TUNE_SPEED) {
        if (client) {
            int spd = client->getData().speed;
            if (y >= 50 && y <= 100) {
                if (x >= 10 && x <= 78) client->setSpeed(max(10, spd - 10));
                else if (x >= 86 && x <= 154) client->setSpeed(max(10, spd - 1));
                else if (x >= 164 && x <= 232) client->setSpeed(min(300, spd + 1));
                else if (x >= 240 && x <= 308) client->setSpeed(min(300, spd + 10));
            } else if (x >= 60 && x <= 260 && y >= 120 && y <= 165) {
                client->setSpeed(100);
            }
        }

        if (x >= 10 && x <= 310 && y >= 202 && y <= 236) {
            _subMenu = TUNE_MAIN;
            _forceRedraw = true;
            return 1;
        }
    }

    // 4. Z-OFFSET MENU ERINTESEK
    else if (_subMenu == TUNE_ZOFFSET) {
        if (client) {
            if (y >= 50 && y <= 95) {
                if (x >= 10 && x <= 102) client->adjustZOffset(-0.10f);
                else if (x >= 114 && x <= 206) client->adjustZOffset(-0.025f);
                else if (x >= 218 && x <= 310) client->adjustZOffset(-0.005f);
            } else if (y >= 105 && y <= 150) {
                if (x >= 10 && x <= 102) client->adjustZOffset(0.10f);
                else if (x >= 114 && x <= 206) client->adjustZOffset(0.025f);
                else if (x >= 218 && x <= 310) client->adjustZOffset(0.005f);
            }
        }

        if (x >= 10 && x <= 310 && y >= 202 && y <= 236) {
            _subMenu = TUNE_MAIN;
            _forceRedraw = true;
            return 1;
        }
    }

    // 5. KAMERAKEP ERINTES: BARHOL RANYOMVA BEZAR
    else if (_subMenu == TUNE_CAMERA) {
        _subMenu = TUNE_MAIN;
        _cameraLoaded = false;
        _forceRedraw = true;
        return 1;
    }

    return 1;
}