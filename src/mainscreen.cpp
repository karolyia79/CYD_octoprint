#include "mainscreen.h"
#include <WiFi.h>
#include "config_manager.h"
#include "lang_manager.h"
#include "ui_utils.h"

MainScreen::MainScreen(TFT_eSPI* tft, CST820* touch, bool octoEnabled, bool klipperEnabled, OctoClient* octoClient) 
    : _tft(tft), _touch(touch), _octoEnabled(octoEnabled), _klipperEnabled(klipperEnabled), 
      _octoClient(octoClient),
      _isPrinting(false), _octoMenu(tft), _klipperMenu(tft), _menuScreen(tft, touch) {}

void MainScreen::init() {
    _currentPage = 0;
    _menuState = 0;
    _forceRedraw = true;
    _isTouched = false;
    _isPrinting = false;
    _lastMainPage = 0;
    
    _octoLastNozzle = -999; _octoLastBed = -999; _octoLastProgress = -1; _octoLastStatus = "";
    _klipperLastNozzle = -999; _klipperLastBed = -999; _klipperLastProgress = -1; _klipperLastStatus = "";
    
    _menuScreen.init();
    Serial.println("[TRACE] MainScreen initialized.");
}

void MainScreen::draw(const OctoPrinterData& octoData, const KlipperPrinterData& klipperData) {
    ThemeColors theme = getCurrentTheme();

    if (_menuState == 1) {
        static bool lastHomingState = false;
        static unsigned long delayTimer = 0;
        bool rawHoming = _octoClient ? _octoClient->isHoming() : false;

        // Ha épp most futott le a valós homing (azaz átváltott true-ról false-ra)
        if (lastHomingState && !rawHoming) {
            delayTimer = millis(); // Elindítjuk az 1 másodperces utó-időzítőt
        }

        // A gomb addig pulzál, amíg megy a homing, VAGY amíg le nem telt az 1 mp-es ráhúzás
        bool currentHoming = rawHoming || (delayTimer > 0 && (millis() - delayTimer < 1000));

        // Ha letelt az 1 másodperc, nullázzuk az időzítőt
        if (delayTimer > 0 && (millis() - delayTimer >= 1000)) {
            delayTimer = 0;
        }

        // Csak akkor töröljük a képernyőt, ha a flag kéri!
        if (_forceRedraw) {
            _tft->fillScreen(theme.bg);
            _octoMenu.forceRedraw();
        }
        
        drawHeader(octoData.name, octoData.connected);

        // Biztosítjuk a helyes szövegszínt és hívjuk a menüt
        _tft->setTextColor(theme.text, theme.bg);
        _octoMenu.draw(_octoClient);
        
        lastHomingState = currentHoming;
        
        _forceRedraw = false; 
        return;
    }

    int menuPageIndex = (_octoEnabled && _klipperEnabled) ? 2 : 1;

    if (_currentPage == menuPageIndex) {
        if (_forceRedraw) {
            _tft->fillScreen(theme.bg);
            _forceRedraw = false;
        }
        bool octoPrinting = (octoData.progress > 0 && octoData.progress < 100);
        bool klipperPrinting = (klipperData.progress > 0 && klipperData.progress < 100);
        
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
            drawHeader(octoData.name, octoData.connected);
            drawOctoPage(octoData);
        } else if (_currentPage == 1) {
            drawHeader(klipperData.name, klipperData.connected);
            drawKlipperPage(klipperData);
        }
    } else if (_octoEnabled) {
        if (_currentPage == 0) {
            drawHeader(octoData.name, octoData.connected);
            drawOctoPage(octoData);
        }
    } else if (_klipperEnabled) {
        if (_currentPage == 0) {
            drawHeader(klipperData.name, klipperData.connected);
            drawKlipperPage(klipperData);
        }
    } else {
        drawDisabledPage(LangManager::get("main_screen_no_protocol"));
    }

    _forceRedraw = false;
}

void MainScreen::drawHeader(const String& name, bool isServerConnected) {
    static String oldName = "";
    static bool oldConn = !isServerConnected;
    static uint16_t oldWifiColor = 0xFFFF;
    
    uint16_t currentWifiColor = ConnectionManager::getStatusColor();
    ThemeColors theme = getCurrentTheme();
    
    String displayName = LangManager::get(name);

    if (oldName != displayName || oldConn != isServerConnected || oldWifiColor != currentWifiColor || _forceRedraw) {
        _tft->fillRect(0, 0, 320, 35, theme.cardBg);
        _tft->setTextColor(theme.text, theme.cardBg);
        _tft->setTextDatum(ML_DATUM);
        _tft->drawString(displayName.substring(0, 18), 10, 17, 2);

        ConnectionManager::drawIcon(_tft, 255, 21);

        uint16_t serverColor = isServerConnected ? TFT_GREEN : TFT_RED;
        _tft->fillCircle(295, 17, 5, serverColor);

        _tft->drawFastHLine(0, 35, 320, theme.subText);
        
        oldName = displayName;
        oldConn = isServerConnected;
        oldWifiColor = currentWifiColor;
    }
}

void MainScreen::drawOctoPage(const OctoPrinterData& info) {
    drawPrinterData(info.status, info.nozzleTemp, info.nozzleTarget, info.bedTemp, info.bedTarget, info.progress, info.remainingTime, info.totalTime,
                    _octoLastNozzle, _octoLastBed, _octoLastProgress, _octoLastStatus);
}

void MainScreen::drawKlipperPage(const KlipperPrinterData& info) {
    drawPrinterData(info.status, info.nozzleTemp, info.nozzleTarget, info.bedTemp, info.bedTarget, info.progress, info.remainingTime, "",
                    _klipperLastNozzle, _klipperLastBed, _klipperLastProgress, _klipperLastStatus);
}

void MainScreen::drawPrinterData(String status, float nT, float nTar, float bT, float bTar, int progress, String time, String totalTime,
                                 float& lastN, float& lastB, int& lastP, String& lastS) {
    
    ThemeColors theme = getCurrentTheme();

    Serial.printf("[DEBUG] Progress: %d | Time: %s\n", progress, time.c_str());

    bool isNozzleHeating = (nTar > 0 && nT < nTar);
    bool isBedHeating    = (bTar > 0 && bT < bTar);

    float pulseFactor = (sin(millis() / 200.0) + 1.0) / 2.0;
    uint8_t redIntensity = 80 + (uint8_t)(175.0 * pulseFactor);
    uint16_t pulseColor = _tft->color565(redIntensity, 0, 0);

    uint16_t nozzleColor = isNozzleHeating ? pulseColor : theme.accent;
    uint16_t bedColor    = isBedHeating    ? pulseColor : theme.accent;

    static uint16_t lastNozzleColor = 0;
    static uint16_t lastBedColor = 0;

    if (_forceRedraw) {
        _tft->fillRoundRect(10, 45, 145, 55, 5, theme.cardBg); 
        _tft->fillRoundRect(165, 45, 145, 55, 5, theme.cardBg); 
        _tft->fillRoundRect(10, 105, 300, 80, 5, theme.cardBg);
        _tft->drawRect(20, 138, 280, 14, theme.subText);
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

    bool dataChanged = (abs(nT - lastN) >= 0.5f || abs(bT - lastB) >= 0.5f || 
                        progress != lastP || status != lastS || _forceRedraw);

    if (!dataChanged) return; 

    lastN = nT;
    lastB = bT;
    lastP = progress;
    lastS = status;

    String displayStatus = LangManager::get(status);
    String displayTime = LangManager::get(time);

    _tft->setTextDatum(TC_DATUM);
    _tft->setTextColor(theme.text, theme.cardBg);
    _tft->drawString(String(nT, 1) + "/" + String(nTar, 0) + "C   ", 82, 70, 2);
    _tft->drawString(String(bT, 1) + "/" + String(bTar, 0) + "C   ", 237, 70, 2);

    _tft->setTextDatum(ML_DATUM);
    _tft->setTextColor(theme.subText, theme.cardBg);
    _tft->fillRect(20, 112, 280, 16, theme.cardBg);
    _tft->drawString(LangManager::get("main_screen_status") + displayStatus, 20, 120, 2);
    
    int barWidth = (278 * progress) / 100;
    _tft->fillRect(21, 139, barWidth, 12, theme.accent);
    _tft->fillRect(21 + barWidth, 139, 278 - barWidth, 12, theme.cardBg);
    
    _tft->fillRect(20, 158, 280, 18, theme.cardBg);
    _tft->setTextColor(theme.text, theme.cardBg);
    
    String timeText = LangManager::get("main_screen_remaining") + displayTime;
    if (totalTime.length() > 0) {
        timeText += " / " + totalTime; 
    }
    
    _tft->drawString(timeText, 20, 168, 2);

    bool newPrintingState = (progress > 0 && progress < 100) || 
                            (status == "octo_status_working") || 
                            (status == "menu_status_print") || 
                            (status.indexOf("rint") >= 0) || 
                            (status.indexOf("Mukodik") >= 0) || 
                            (status.indexOf("Work") >= 0);

    if (newPrintingState != _isPrinting || _forceRedraw) {
        _isPrinting = newPrintingState;
        if (_isPrinting) {
            drawPrintControls();
        } else {
            _tft->fillRect(10, 192, 300, 42, theme.bg); 
            drawMenuButton();
        }
    }
}

void MainScreen::drawMenuButton() {
    ThemeColors theme = getCurrentTheme();
    UIUtils::drawButton(_tft, 10, 192, 300, 42, LangManager::get("main_screen_settings"), theme.cardBg, theme.text, false, 2, 5);
}

void MainScreen::drawPrintControls() {
    ThemeColors theme = getCurrentTheme();
    UIUtils::drawButton(_tft, 10, 192, 92, 42, LangManager::get("main_screen_pause"), TFT_ORANGE, TFT_BLACK, false, 2, 5);
    UIUtils::drawButton(_tft, 114, 192, 92, 42, LangManager::get("main_screen_tune"), theme.cardBg, theme.text, false, 2, 5);
    UIUtils::drawButton(_tft, 218, 192, 92, 42, LangManager::get("main_screen_cancel"), TFT_RED, TFT_WHITE, false, 2, 5);
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
            if (_menuState == 1) menuResult = _octoMenu.handleTouch(startX, startY, _octoClient);
            else if (_menuState == 2) menuResult = _klipperMenu.handleTouch(startX, startY);

            waitForRelease = true;
            if (menuResult == 0) { 
                _menuState = 0;
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
                    UIUtils::pressFeedback(_tft, 10, 192, 92, 42, LangManager::get("main_screen_pause"), TFT_ORANGE, TFT_BLACK, 2, 5);
                    waitForRelease = true; 
                    return isOctoActive ? 2 : 12; // Pause
                }
                else if (startX >= 114 && startX <= 206) {
                    UIUtils::pressFeedback(_tft, 114, 192, 92, 42, LangManager::get("main_screen_tune"), theme.cardBg, theme.text, 2, 5);
                    if (isOctoActive) {
                        _octoMenu.openTuneMenu();
                        _octoMenu.forceRedraw();
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
                    UIUtils::pressFeedback(_tft, 218, 192, 92, 42, LangManager::get("main_screen_cancel"), TFT_RED, TFT_WHITE, 2, 5);
                    waitForRelease = true; 
                    return isOctoActive ? 4 : 14; // Cancel
                }
            } else {
                if (startX >= 10 && startX <= 310) {
                    UIUtils::pressFeedback(_tft, 10, 192, 300, 42, LangManager::get("main_screen_settings"), theme.cardBg, theme.text, 2, 5);
                    if (isOctoActive) {
                        _octoMenu.openMainMenu();
                        _octoMenu.forceRedraw();
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