#include "mainscreen.h"
#include <WiFi.h>
#include "config_manager.h"
#include "lang_manager.h"
#include "ui_utils.h"

MainScreen::MainScreen(TFT_eSPI* tft, CST820* touch, bool octoEnabled, bool klipperEnabled, OctoClientMqtt* octoMqtt) 
    : _tft(tft), _touch(touch), _octoEnabled(octoEnabled), _klipperEnabled(klipperEnabled), 
      _octoMqtt(octoMqtt), _isPrinting(false), 
      _octoMenuMqtt(tft), _klipperMenu(tft), _menuScreen(tft, touch), _statusAnim(tft) {}

void MainScreen::init() {
    _currentPage = 0;
    _menuState = 0;
    _forceRedraw = true;
    _isTouched = false;
    _isPrinting = false;
    _lastMainPage = 0;
    _showApiErrorPopup = false;
    _showCancellingPopup = false;
    
    _octoLastNozzle = -999; _octoLastBed = -999; _octoLastProgress = -1; _octoLastStatus = "";
    _klipperLastNozzle = -999; _klipperLastBed = -999; _klipperLastProgress = -1; _klipperLastStatus = "";
    
    _statusAnim.init(215, 110, 85, 70);
    _menuScreen.init();
    Serial.println("[TRACE] MainScreen initialized.");
}

void MainScreen::draw(const OctoPrinterData& octoData, const KlipperPrinterData& klipperData) {
    ThemeColors theme = getCurrentTheme();

    bool octoPrinting = octoData.printingActive;
    bool klipperPrinting = (klipperData.progress > 0 && klipperData.progress < 100);
                        
    String octoHeaderName = octoData.name;
    if (octoPrinting && octoData.printFileName.length() > 0) {
        octoHeaderName = octoData.printFileName;
    }

    if (_menuState != 0) {
        bool isCurrentPrinting = (_currentPage == 0) ? octoPrinting : klipperPrinting;
        if (!isCurrentPrinting) {
            _menuState = 0;
            _octoMenuMqtt.openMainMenu();
            _klipperMenu.openMainMenu();
            _forceRedraw = true;
        }
    }

    if (_menuState == 1) {
        if (_forceRedraw) {
            _tft->fillScreen(theme.bg);
            _octoMenuMqtt.forceRedraw();
        }
        
        if (!_octoMenuMqtt.isCameraActive()) {
            drawHeader(octoHeaderName, octoData.connected, octoData.apiConnected);
        }

        _tft->setTextColor(theme.text, theme.bg);
        _octoMenuMqtt.draw(_octoMqtt);
        
        _forceRedraw = false; 
        return;
    }

    if (_menuState == 2) {
        if (_forceRedraw) {
            _tft->fillScreen(theme.bg);
            _klipperMenu.forceRedraw();
        }
        
        drawHeader(klipperData.name, klipperData.connected, false);
        _tft->setTextColor(theme.text, theme.bg);
        _klipperMenu.draw();
        
        _forceRedraw = false; 
        return;
    }

    int menuPageIndex = (_octoEnabled && _klipperEnabled) ? 2 : 1;

    if (_currentPage == menuPageIndex) {
        if (_forceRedraw) {
            _tft->fillScreen(theme.bg);
            _forceRedraw = false;
        }
        
        _tft->setTextColor(theme.text, theme.bg);
        _menuScreen.draw(_octoEnabled, octoData.connected, octoPrinting, 
                         _klipperEnabled, klipperData.connected, klipperPrinting);
        _isPrinting = false;
        return;
    }

    if (_forceRedraw) {
        _tft->fillScreen(theme.bg);
    }

    _tft->setTextColor(theme.text, theme.bg);

    if (_octoEnabled && _klipperEnabled) {
        if (_currentPage == 0) {
            drawHeader(octoHeaderName, octoData.connected, octoData.apiConnected);
            drawOctoPage(octoData);
        } else if (_currentPage == 1) {
            drawHeader(klipperData.name, klipperData.connected, false);
            drawKlipperPage(klipperData);
        }
    } else if (_octoEnabled) {
        if (_currentPage == 0) {
            drawHeader(octoHeaderName, octoData.connected, octoData.apiConnected);
            drawOctoPage(octoData);
        }
    } else if (_klipperEnabled) {
        if (_currentPage == 0) {
            drawHeader(klipperData.name, klipperData.connected, false);
            drawKlipperPage(klipperData);
        }
    } else {
        drawDisabledPage(LangManager::get("main_screen_no_protocol"));
    }

    if (_showApiErrorPopup) {
        drawApiErrorPopup();
    }

    if (_showCancellingPopup) {
        drawCancellingPopup();
    }

    _forceRedraw = false;
}

void MainScreen::drawHeader(const String& name, bool isServerConnected, bool isApiConnected) {
    static bool oldConn = !isServerConnected;
    static bool oldApiConn = !isApiConnected;
    static uint16_t oldWifiColor = 0xFFFF;
    static String lastRenderedName = "";
    static unsigned long lastScrollTime = 0;

    uint16_t currentWifiColor = ConnectionManager::getStatusColor();
    ThemeColors theme = getCurrentTheme();
    
    bool isLightSkin = (theme.bg == TFT_WHITE || theme.bg > 0xEF5D);
    String baseName = (name.length() > 0) ? name : LangManager::get("OctoPrint");

    String displayName = baseName;
    if (baseName.length() > 18) {
        unsigned long currentMillis = millis();
        int scrollIndex = (currentMillis / 350) % (baseName.length() + 3);
        String padded = baseName + "   " + baseName;
        displayName = padded.substring(scrollIndex, scrollIndex + 18);
    } else {
        displayName = baseName.substring(0, 18);
    }

    bool scrollChanged = false;
    if (baseName.length() > 18) {
        if (millis() - lastScrollTime >= 350) {
            lastScrollTime = millis();
            scrollChanged = true;
        }
    }

    if (lastRenderedName != displayName || oldConn != isServerConnected || oldApiConn != isApiConnected || oldWifiColor != currentWifiColor || _forceRedraw || scrollChanged) {
        _tft->fillRect(0, 0, 320, 35, theme.cardBg);
        _tft->setTextColor(theme.text, theme.cardBg);
        _tft->setTextDatum(ML_DATUM);
        _tft->drawString(displayName, 10, 17, 2);

        _tft->setTextDatum(MR_DATUM);
    
        // --- API SZÍNES SZÖVEG ---
        uint16_t activeStatusColor = isLightSkin ? TFT_BLUE : TFT_GREEN;
        uint16_t apiColor = isApiConnected ? activeStatusColor : TFT_RED;
        _tft->setTextColor(apiColor, theme.cardBg);
        _tft->drawString(LangManager::get("main_screen_api"), 245, 17, 1);

        // --- MQTT SZÖVEG ---
        uint16_t serverColor = isServerConnected ? activeStatusColor : TFT_RED;
        _tft->setTextColor(serverColor, theme.cardBg);
        _tft->drawString(LangManager::get("header_mode_mqtt"), 280, 17, 1);

        // --- WIFI IKON ---
        ConnectionManager::drawIcon(_tft, 293, 19);

        _tft->drawFastHLine(0, 35, 320, theme.subText);
    
        lastRenderedName = displayName;
        oldConn = isServerConnected;
        oldApiConn = isApiConnected;
        oldWifiColor = currentWifiColor;
    }
}

void MainScreen::drawOctoPage(const OctoPrinterData& info) {
    String lowerStatus = info.status;
    lowerStatus.toLowerCase();
    bool isCurrentlyPrinting = (lowerStatus.indexOf("printing") != -1 || lowerStatus.indexOf("paused") != -1 || info.printingActive);

    int currentProgress = isCurrentlyPrinting ? info.progress : 0;
    String timeStr = isCurrentlyPrinting ? info.remainingTime : "0h 00m";
    String totalTimeStr = isCurrentlyPrinting ? info.totalTime : "";

    drawPrinterData(info.status, info.nozzleTemp, info.nozzleTarget, info.bedTemp, info.bedTarget, 
                    currentProgress, timeStr, totalTimeStr,
                    _octoLastNozzle, _octoLastBed, _octoLastProgress, _octoLastStatus);
}

void MainScreen::drawKlipperPage(const KlipperPrinterData& info) {
    String lowerStatus = info.status;
    lowerStatus.toLowerCase();
    bool isCurrentlyPrinting = (lowerStatus.indexOf("printing") != -1 || lowerStatus.indexOf("paused") != -1);

    int currentProgress = isCurrentlyPrinting ? info.progress : 0;
    String timeStr = isCurrentlyPrinting ? info.remainingTime : "0h 00m";

    drawPrinterData(info.status, info.nozzleTemp, info.nozzleTarget, info.bedTemp, info.bedTarget, 
                    currentProgress, timeStr, "",
                    _klipperLastNozzle, _klipperLastBed, _klipperLastProgress, _klipperLastStatus);
}

void MainScreen::drawPrinterData(String status, float nT, float nTar, float bT, float bTar, int progress, String time, String totalTime,
                                 float& lastN, float& lastB, int& lastP, String& lastS) {
    
    ThemeColors theme = getCurrentTheme();

    bool isNozzleHeating = (nTar > 0 && nT < nTar - 1.0f);
    bool isBedHeating    = (bTar > 0 && bT < bTar - 1.0f);

    float pulseFactor = (sin(millis() / 200.0) + 1.0) / 2.0;
    uint8_t redIntensity = 80 + (uint8_t)(175.0 * pulseFactor);
    uint16_t pulseColor = _tft->color565(redIntensity, 0, 0);

    uint16_t nozzleColor = isNozzleHeating ? pulseColor : theme.accent;
    uint16_t bedColor    = isBedHeating    ? pulseColor : theme.accent;

    static uint16_t lastNozzleColor = 0;
    static uint16_t lastBedColor = 0;
    static float lastNTarVal = -999;
    static float lastBTarVal = -999;
    static String lastTimeVal = "";
    static String lastTotalTimeVal = "";

    if (_forceRedraw) {
        _tft->fillRoundRect(10, 45, 145, 55, 5, theme.cardBg); 
        _tft->fillRoundRect(165, 45, 145, 55, 5, theme.cardBg); 
        _tft->fillRoundRect(10, 105, 300, 80, 5, theme.cardBg);
        _tft->drawRect(20, 138, 185, 14, theme.subText); 
    }

    bool isPausedState = (status == "Paused" || status == "Szüneteltetve" || status == "Pausing" || status == "Szüneteltetés..." || status == LangManager::get("main_screen_resume_progress") || status == "Nyomtatás leállítása..." || status == "Cancelling");
    bool newPrintingState = _octoMqtt ? (_octoMqtt->getData().printingActive || isPausedState) : ((progress > 0 && progress < 100) || status.indexOf("rint") >= 0 || status.indexOf("Work") >= 0 || isPausedState);

    if (newPrintingState != _isPrinting || _forceRedraw) {
        _isPrinting = newPrintingState;
        if (_isPrinting) {
            drawPrintControls();
        } else {
            _tft->fillRect(10, 192, 300, 42, theme.bg); 
            drawMenuButton();

            _octoMenuMqtt.openMainMenu();
            _klipperMenu.openMainMenu();
            if (_menuState != 0) {
                _menuState = 0;
                _forceRedraw = true;
            }
        }
    }

    if (_forceRedraw || nozzleColor != lastNozzleColor || bedColor != lastBedColor) {
        _tft->setTextDatum(TC_DATUM);
        
        _tft->setTextColor(nozzleColor, theme.cardBg);
        _tft->drawString(LangManager::get("main_screen_nozzle"), 82, 52, 1);
        
        _tft->setTextColor(bedColor, theme.cardBg);
        _tft->drawString(LangManager::get("main_screen_bed"), 237, 52, 1);

        lastNozzleColor = nozzleColor;
        lastBedColor = bedColor;
    }

    static unsigned long lastPulseCheck = 0;
    bool isPulsing = (status == "Nyomtatás leállítása..." || status == "Cancelling" || 
                      status == "Szüneteltetés..." || status == "Pausing" || status == LangManager::get("main_screen_resume_progress"));
    
    bool pulseTick = false;
    if (isPulsing && (millis() - lastPulseCheck >= 300)) {
        lastPulseCheck = millis();
        pulseTick = true;
    }

    bool dataChanged = (abs(nT - lastN) >= 0.01f || abs(bT - lastB) >= 0.01f || 
                        nTar != lastNTarVal || bTar != lastBTarVal ||
                        progress != lastP || status != lastS ||
                        time != lastTimeVal || totalTime != lastTotalTimeVal ||
                        _forceRedraw);

    if (_isPrinting && (status != lastS || pulseTick) && !_forceRedraw) {
        drawPrintControls();
    }

    // --- ANIMÁCIÓ ÁLLAPOT DETEKTÁLÁSA ---
    String lowerStatus = status;
    lowerStatus.toLowerCase();

    bool isIdle = (lowerStatus.indexOf("operat") >= 0 || 
                   lowerStatus.indexOf("standby") >= 0 || 
                   lowerStatus.indexOf("ready") >= 0 || 
                   lowerStatus.indexOf("off") >= 0 || 
                   lowerStatus.indexOf("kikapcsolva") >= 0 || 
                   lowerStatus.indexOf("idle") >= 0 || 
                   lowerStatus == "");

    bool isPaused = (lowerStatus.indexOf("pause") >= 0 || lowerStatus.indexOf("szünet") >= 0);

    bool isMoving = (_isPrinting || !isIdle) && !isPaused;
    bool isExtruding = _isPrinting && !isNozzleHeating && !isBedHeating && !isPaused;

    _statusAnim.updateStatus(isBedHeating, isNozzleHeating, isMoving, isExtruding);
    _statusAnim.update(theme.cardBg);

    if (!dataChanged && !pulseTick) return; 

    lastN = nT;
    lastB = bT;
    lastNTarVal = nTar;
    lastBTarVal = bTar;
    lastP = progress;
    lastS = status;
    lastTimeVal = time;
    lastTotalTimeVal = totalTime;

    String displayStatus = LangManager::get(status);
    String fullStatusText = LangManager::get("main_screen_status") + " " + displayStatus;
    if (fullStatusText.length() > 24) {
        fullStatusText = fullStatusText.substring(0, 22) + "...";
    }

    _tft->fillRect(15, 63, 135, 22, theme.cardBg);
    _tft->fillRect(170, 63, 135, 22, theme.cardBg);

    char nozzleBuf[32], bedBuf[32];
    snprintf(nozzleBuf, sizeof(nozzleBuf), "%.2f/%.0fC", nT, nTar);
    snprintf(bedBuf, sizeof(bedBuf), "%.2f/%.0fC", bT, bTar);

    _tft->setTextDatum(TC_DATUM);
    _tft->setTextColor(theme.text, theme.cardBg);
    _tft->drawString(nozzleBuf, 82, 70, 2);
    _tft->drawString(bedBuf, 237, 70, 2);

    _tft->setTextDatum(ML_DATUM);
    _tft->setTextColor(theme.subText, theme.cardBg);
    _tft->fillRect(15, 112, 195, 16, theme.cardBg);
    _tft->drawString(fullStatusText, 20, 120, 2);
    
    int maxBarWidth = 183;
    int barWidth = (maxBarWidth * progress) / 100;
    if (barWidth > maxBarWidth) barWidth = maxBarWidth;
    if (barWidth < 0) barWidth = 0;
    
    _tft->fillRect(21, 139, barWidth, 12, theme.accent);
    _tft->fillRect(21 + barWidth, 139, maxBarWidth - barWidth, 12, theme.cardBg);
    
    _tft->fillRect(20, 158, 195, 18, theme.cardBg);
    _tft->setTextColor(theme.text, theme.cardBg);
    
    String displayTime = LangManager::get(time);
    String timeText = LangManager::get("main_screen_remaining") + " " + displayTime;
    if (totalTime.length() > 0 && totalTime != "--:--") {
        timeText += " / " + totalTime; 
    }
    if (timeText.length() > 24) {
        timeText = timeText.substring(0, 22) + "...";
    }
    
    _tft->drawString(timeText, 20, 168, 2);
}

void MainScreen::drawMenuButton() {
    ThemeColors theme = getCurrentTheme();
    UIUtils::drawButton(_tft, 10, 192, 300, 42, LangManager::get("main_screen_settings"), theme.cardBg, theme.text, false, 2, 5);
}

void MainScreen::drawPrintControls() {
    ThemeColors theme = getCurrentTheme();
    String status = "";
    if (_octoMqtt) {
        status = _octoMqtt->getData().status;
    }

    bool pulseState = (millis() / 400) % 2 == 0;

    if (status == "Szüneteltetés..." || status == "Pausing") {
        uint16_t bgCol = pulseState ? TFT_ORANGE : _tft->color565(120, 60, 0);
        UIUtils::drawButton(_tft, 10, 192, 92, 42, LangManager::get("main_screen_pause_progress"), bgCol, TFT_WHITE, false, 2, 5);
    } else if (status == LangManager::get("main_screen_resume_progress")) {
        uint16_t bgCol = pulseState ? TFT_GREEN : _tft->color565(0, 80, 0);
        UIUtils::drawButton(_tft, 10, 192, 92, 42, LangManager::get("main_screen_resume_progress"), bgCol, TFT_WHITE, false, 2, 5);
    } else if (status == "Paused" || status == "Szüneteltetve") {
        UIUtils::drawButton(_tft, 10, 192, 92, 42, LangManager::get("main_screen_resume"), TFT_GREEN, TFT_WHITE, false, 2, 5);
    } else {
        UIUtils::drawButton(_tft, 10, 192, 92, 42, LangManager::get("main_screen_pause"), TFT_ORANGE, TFT_BLACK, false, 2, 5);
    }

    UIUtils::drawButton(_tft, 114, 192, 92, 42, LangManager::get("main_screen_tune"), theme.cardBg, theme.text, false, 2, 5);

    if (status == "Nyomtatás leállítása..." || status == "Cancelling") {
        uint16_t bgCol = pulseState ? TFT_RED : _tft->color565(120, 0, 0);
        UIUtils::drawButton(_tft, 218, 192, 92, 42, LangManager::get("main_screen_cancel_progress"), bgCol, TFT_WHITE, false, 2, 5);
    } else {
        UIUtils::drawButton(_tft, 218, 192, 92, 42, LangManager::get("main_screen_cancel"), TFT_RED, TFT_WHITE, false, 2, 5);
    }
}

void MainScreen::drawApiErrorPopup() {
    ThemeColors theme = getCurrentTheme();
    
    _tft->fillRoundRect(20, 45, 280, 150, 8, theme.cardBg);
    _tft->drawRoundRect(20, 45, 280, 150, 8, TFT_RED);
    _tft->drawRoundRect(21, 46, 278, 148, 7, TFT_RED);

    _tft->setTextDatum(TC_DATUM);
    _tft->setTextColor(TFT_RED, theme.cardBg);
    _tft->drawString(LangManager::get("main_screen_api_error_title"), 160, 58, 2);

    _tft->setTextColor(theme.text, theme.cardBg);
    _tft->drawString(LangManager::get("main_screen_api_error_line1"), 160, 88, 2);
    _tft->drawString(LangManager::get("main_screen_api_error_line2"), 160, 110, 2);

    UIUtils::drawButton(_tft, 110, 145, 100, 36, LangManager::get("btn_ok"), TFT_RED, TFT_WHITE, false, 2, 5);
}

void MainScreen::drawCancellingPopup() {
    ThemeColors theme = getCurrentTheme();
    
    _tft->fillRoundRect(20, 45, 280, 150, 8, theme.cardBg);
    _tft->drawRoundRect(20, 45, 280, 150, 8, TFT_ORANGE);
    _tft->drawRoundRect(21, 46, 278, 148, 7, TFT_ORANGE);

    _tft->setTextDatum(TC_DATUM);
    _tft->setTextColor(TFT_ORANGE, theme.cardBg);
    _tft->drawString(LangManager::get("figyelmeztetes"), 160, 58, 2);

    _tft->setTextColor(theme.text, theme.cardBg);
    _tft->drawString(LangManager::get("cancel_wait_msg1"), 160, 88, 2);
    _tft->drawString(LangManager::get("cancel_wait_msg2"), 160, 110, 2);

    UIUtils::drawButton(_tft, 110, 145, 100, 36, LangManager::get("btn_ok"), TFT_ORANGE, TFT_WHITE, false, 2, 5);
}

void MainScreen::drawDisabledPage(const String& title) {
    if (_forceRedraw) {
        ThemeColors theme = getCurrentTheme();
        _tft->fillScreen(theme.bg);
        _tft->setTextColor(theme.accent, theme.bg);
        _tft->setTextDatum(MC_DATUM);
        _tft->drawString(title, 160, 100, 2);
        _tft->setTextColor(theme.text, theme.bg);
        _tft->drawString(LangManager::get("main_screen_webui_hint"), 160, 130, 1);
    }
}

int MainScreen::handleTouch() {
    uint16_t raw_x = 0, raw_y = 0;
    uint8_t gesture = 0;
    bool touched = _touch->getTouch(&raw_x, &raw_y, &gesture);

    static bool waitForRelease = false;
    static bool isDown = false;
    static uint16_t startX = 0;
    static uint16_t startY = 0;
    static uint16_t currentX = 0;
    static uint16_t currentY = 0;

    // --- API HIBA POPUP CSOPORT ---
    if (_showApiErrorPopup) {
        if (touched && !waitForRelease) {
            currentX = raw_y;
            currentY = 240 - raw_x;
            if (currentX >= 100 && currentX <= 220 && currentY >= 135 && currentY <= 190) {
                UIUtils::pressFeedback(_tft, 110, 145, 100, 36, LangManager::get("btn_ok"), TFT_RED, TFT_WHITE, 2, 5);
                _showApiErrorPopup = false;
                _forceRedraw = true;
                waitForRelease = true;
            }
        }
        if (!touched) waitForRelease = false;
        return 0;
    }

    // --- LEÁLLÍTÁSI POPUP CSOPORT ---
    if (_showCancellingPopup) {
        if (touched && !waitForRelease) {
            currentX = raw_y;
            currentY = 240 - raw_x;
            if (currentX >= 100 && currentX <= 220 && currentY >= 135 && currentY <= 190) {
                UIUtils::pressFeedback(_tft, 110, 145, 100, 36, LangManager::get("btn_ok"), TFT_ORANGE, TFT_WHITE, 2, 5);
                _showCancellingPopup = false;
                _forceRedraw = true;
                waitForRelease = true;
            }
        }
        if (!touched) waitForRelease = false;
        return 0;
    }

    if (waitForRelease) {
        if (!touched) {
            waitForRelease = false;
            isDown = false;
        }
        return 0;
    }

    int maxPage = (_octoEnabled && _klipperEnabled) ? 2 : 1;

    if (!touched) {
        if (isDown) {
            isDown = false;
            int diffX = (int)currentX - (int)startX;
            int diffY = (int)currentY - (int)startY;

            if (_currentPage == maxPage && _menuState == 0) {
                if (diffX > 30 && abs(diffX) > abs(diffY)) {
                    _currentPage = _lastMainPage;
                    _forceRedraw = true;
                    waitForRelease = true;
                    return 1;
                }
                else if (abs(diffX) < 15 && abs(diffY) < 15) {
                    if (_menuScreen.handleClick(startX, startY)) {
                        _currentPage = _lastMainPage; 
                        _forceRedraw = true;
                        waitForRelease = true;
                        return 1;
                    }
                }
            }
            else if (_menuState == 0 && _currentPage != maxPage && abs(diffX) > 25 && abs(diffX) > abs(diffY)) {
                if (diffX < 0) { 
                    if (_currentPage < maxPage) {
                        _lastMainPage = _currentPage;
                        _currentPage++;
                        _forceRedraw = true;
                        if (_currentPage == maxPage) _menuScreen.init();
                        waitForRelease = true;
                        return 1;
                    }
                } else if (diffX > 0) { 
                    if (_currentPage > 0) {
                        _currentPage--;
                        _forceRedraw = true;
                        waitForRelease = true;
                        return 1;
                    }
                }
            }
        }
        return 0;
    }

    currentX = raw_y;
    currentY = 240 - raw_x;

    if (!isDown) {
        isDown = true;
        startX = currentX;
        startY = currentY;

        if (_menuState > 0) {
            int menuResult = -1;
            if (_menuState == 1) {
                menuResult = _octoMenuMqtt.handleTouch(startX, startY, _octoMqtt);
            }
            else if (_menuState == 2) {
                menuResult = _klipperMenu.handleTouch(startX, startY);
            }

            waitForRelease = true;
            if (menuResult == 0) { 
                _menuState = 0;
                _octoMenuMqtt.openMainMenu();
                _klipperMenu.openMainMenu();
                _forceRedraw = true;
                return 0;
            }
            return menuResult;
        }

        if (_currentPage != maxPage && startY >= 192 && startY <= 234) {
            bool isOctoActive = (_octoEnabled && (_currentPage == 0 || !_klipperEnabled));
            ThemeColors theme = getCurrentTheme();

            if (_isPrinting) {
                if (startX >= 10 && startX <= 102) { 
                    if (isOctoActive) {
                        if (_octoMqtt && _octoMqtt->getData().apiConnected) {
                            String currentStatus = _octoMqtt->getData().status;
                            String btnText = (currentStatus == "Paused" || currentStatus == "Szüneteltetve") ? LangManager::get("main_screen_resume") : LangManager::get("main_screen_pause");
                            uint16_t btnBg = (currentStatus == "Paused" || currentStatus == "Szüneteltetve") ? TFT_GREEN : TFT_ORANGE;
                            uint16_t btnFg = (currentStatus == "Paused" || currentStatus == "Szüneteltetve") ? TFT_WHITE : TFT_BLACK;
                            UIUtils::pressFeedback(_tft, 10, 192, 92, 42, btnText, btnBg, btnFg, 2, 5);
                            _octoMqtt->pausePrint();
                        } else {
                            _showApiErrorPopup = true;
                            _forceRedraw = true;
                        }
                    }
                    waitForRelease = true; 
                    return isOctoActive ? 2 : 12; 
                }
                else if (startX >= 114 && startX <= 206) {
                    UIUtils::pressFeedback(_tft, 114, 192, 92, 42, LangManager::get("main_screen_tune"), theme.cardBg, theme.text, 2, 5);
                    if (isOctoActive) {
                        _octoMenuMqtt.openTuneMenu();
                        _octoMenuMqtt.forceRedraw();
                        _menuState = 1;
                    } else {
                        _klipperMenu.openTuneMenu();
                        _klipperMenu.forceRedraw();
                        _menuState = 2;
                    }
                    _forceRedraw = true;
                    waitForRelease = true;
                } 
                else if (startX >= 218 && startX <= 310) { 
                    if (isOctoActive) {
                        if (_octoMqtt) {
                            String currentStatus = _octoMqtt->getData().status;
                            
                            // HA MÁR LEÁLLÍTÁSI FOLYAMATBAN VAN -> POPUP AKTI VÁLÁSA
                            if (currentStatus == "Cancelling" || currentStatus == "Nyomtatás leállítása...") {
                                UIUtils::pressFeedback(_tft, 218, 192, 92, 42, LangManager::get("main_screen_cancel_progress"), TFT_RED, TFT_WHITE, 2, 5);
                                _showCancellingPopup = true;
                                _forceRedraw = true;
                            } 
                            else if (_octoMqtt->getData().apiConnected) {
                                UIUtils::pressFeedback(_tft, 218, 192, 92, 42, LangManager::get("main_screen_cancel"), TFT_RED, TFT_WHITE, 2, 5);
                                _octoMqtt->cancelPrint();
                                _isPrinting = true;
                                _forceRedraw = true;
                            } else {
                                _showApiErrorPopup = true;
                                _forceRedraw = true;
                            }
                        }
                    }
                    waitForRelease = true; 
                    return isOctoActive ? 4 : 14; 
                }
            } else {
                if (startX >= 10 && startX <= 310) {
                    UIUtils::pressFeedback(_tft, 10, 192, 300, 42, LangManager::get("main_screen_settings"), theme.cardBg, theme.text, false, 2, 5);
                    if (isOctoActive) {
                        _octoMenuMqtt.openMainMenu();
                        _octoMenuMqtt.forceRedraw();
                        _menuState = 1;
                    } else {
                        _klipperMenu.openMainMenu();
                        _klipperMenu.forceRedraw();
                        _menuState = 2;
                    }
                    _forceRedraw = true;
                    waitForRelease = true;
                }
            }
        }
    }
    return 0;
}