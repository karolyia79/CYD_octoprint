#include "menuscreen.h"
#include <WiFi.h>
#include <SD.h>
#include "lang_manager.h"
#include "ui_utils.h"

MenuScreen::MenuScreen(TFT_eSPI* tft, CST820* touch) 
    : _tft(tft), _touch(touch), _currentSubMenu(0), 
      _mainMenuButtonsDrawn(false),
      _lastOctoColor(0xFFFF), _lastKlipperColor(0xFFFF), _lastOctoStr(""), _lastKlipperStr(""),
      _lastSubMenuChecked(255),
      _pOctoEnabled(false), _pOctoConn(false), _pOctoPrint(false),
      _pKlipperEnabled(false), _pKlipperConn(false), _pKlipperPrint(false) {}

void MenuScreen::init() {
    _currentSubMenu = 0;
    _mainMenuButtonsDrawn = false;
    _isTouched = false;
    _lastSubMenuChecked = 255; 
    _config = ConfigManager::loadConfig(); 
}

void MenuScreen::draw(bool octoEnabled, bool octoConn, bool octoPrint, bool klipperEnabled, bool klipperConn, bool klipperPrint) {
    _pOctoEnabled = octoEnabled; _pOctoConn = octoConn; _pOctoPrint = octoPrint;
    _pKlipperEnabled = klipperEnabled; _pKlipperConn = klipperConn; _pKlipperPrint = klipperPrint;

    ThemeColors theme = getCurrentTheme();

    if (_currentSubMenu > 0) {
        if (_currentSubMenu != _lastSubMenuChecked) {
            _tft->fillScreen(theme.bg);
            _tft->setTextColor(theme.text, theme.bg);
            _tft->setTextDatum(MC_DATUM);
            switch (_currentSubMenu) {
                case 1: drawWifiMenu(); break;
                case 2: drawLanguageMenu(); break; 
                case 3: drawSkinMenu(); break;
                case 4: drawSystemMenu(); break;
                case 5: drawInfoMenu(); break; // Infó almenü
            }
            _lastSubMenuChecked = _currentSubMenu;
        }
    } else {
        if (_lastSubMenuChecked != 0) {
            _tft->fillScreen(theme.bg);
            _mainMenuButtonsDrawn = false;
            _lastSubMenuChecked = 0;
            _lastOctoColor = 0xFFFF;
            _lastKlipperColor = 0xFFFF;
            _lastOctoStr = "";
            _lastKlipperStr = "";
        }
        drawMainMenu(octoEnabled, octoConn, octoPrint, klipperEnabled, klipperConn, klipperPrint);
    }
}

void MenuScreen::drawMenuButton(int x, int y, int w, int h, const String& text, uint16_t bgColor, uint16_t textColor) {
    UIUtils::drawButton(_tft, x, y, w, h, text, bgColor, textColor, false, 2, 6);
}

void MenuScreen::drawMainMenu(bool octoEnabled, bool octoConn, bool octoPrint, bool klipperEnabled, bool klipperConn, bool klipperPrint) {
    ThemeColors theme = getCurrentTheme();
    if (!_mainMenuButtonsDrawn) {
        _tft->setTextColor(theme.accent, theme.bg);
        _tft->setTextDatum(MC_DATUM);
        _tft->drawString(LangManager::get("menu_title"), 160, 18, 4);

        drawMenuButton(20, 48, 130, 58, LangManager::get("menu_btn_wifi"), theme.cardBg, theme.text);
        drawMenuButton(170, 48, 130, 58, LangManager::get("menu_btn_language"), theme.cardBg, theme.text);
        
        drawMenuButton(20, 114, 130, 58, LangManager::get("menu_btn_skin"), theme.cardBg, theme.text);
        drawMenuButton(170, 114, 130, 58, LangManager::get("menu_btn_system"), theme.cardBg, theme.text);
        
        _mainMenuButtonsDrawn = true;
        drawServerStatusBars(octoEnabled, octoConn, octoPrint, klipperEnabled, klipperConn, klipperPrint, true);
    } else {
        drawServerStatusBars(octoEnabled, octoConn, octoPrint, klipperEnabled, klipperConn, klipperPrint, false);
    }
}

void MenuScreen::drawServerStatusBars(bool octoEnabled, bool octoConn, bool octoPrint, bool klipperEnabled, bool klipperConn, bool klipperPrint, bool forceRedraw) {
    auto getStatusColor = [](bool enabled, bool conn, bool printing) {
        if (!enabled) return TFT_DARKGREY;          
        if (WiFi.status() != WL_CONNECTED) return TFT_RED; 
        if (!conn) return TFT_YELLOW;              
        return TFT_GREEN;                          
    };

    uint16_t octoColor = getStatusColor(octoEnabled, octoConn, octoPrint);
    uint16_t klipperColor = getStatusColor(klipperEnabled, klipperConn, klipperPrint);

    String octoStateStr = octoEnabled ? (octoConn ? (octoPrint ? LangManager::get("menu_status_print") : LangManager::get("menu_status_standby")) : LangManager::get("menu_status_err")) : LangManager::get("menu_status_off");
    String klipperStateStr = klipperEnabled ? (klipperConn ? (klipperPrint ? LangManager::get("menu_status_print") : LangManager::get("menu_status_standby")) : LangManager::get("menu_status_err")) : LangManager::get("menu_status_off");

    String octoStr = LangManager::get("menu_status_octo") + octoStateStr;
    String klipperStr = LangManager::get("menu_status_klipper") + klipperStateStr;

    if (forceRedraw || octoColor != _lastOctoColor || octoStr != _lastOctoStr) {
        _tft->fillRoundRect(20, 182, 130, 42, 5, octoColor);
        _tft->setTextColor(TFT_BLACK, octoColor);
        _tft->setTextDatum(MC_DATUM);
        _tft->drawString(octoStr, 85, 203, 1);
        _lastOctoColor = octoColor;
        _lastOctoStr = octoStr;
    }

    if (forceRedraw || klipperColor != _lastKlipperColor || klipperStr != _lastKlipperStr) {
        _tft->fillRoundRect(170, 182, 130, 42, 5, klipperColor);
        _tft->setTextColor(TFT_BLACK, klipperColor);
        _tft->setTextDatum(MC_DATUM);
        _tft->drawString(klipperStr, 235, 203, 1);
        _lastKlipperColor = klipperColor;
        _lastKlipperStr = klipperStr;
    }
}

void MenuScreen::drawWifiMenu() {
    ThemeColors theme = getCurrentTheme();
    _tft->setTextColor(theme.accent, theme.bg);
    _tft->drawString(LangManager::get("wifi_menu_title"), 160, 20, 2);
    
    _tft->setTextColor(theme.text, theme.bg);
    _tft->drawString(LangManager::get("wifi_menu_ssid") + " " + WiFi.SSID(), 160, 65, 2);
    _tft->drawString(LangManager::get("wifi_menu_ip") + " " + WiFi.localIP().toString(), 160, 100, 2);
    _tft->drawString(LangManager::get("wifi_menu_gateway") + " " + WiFi.gatewayIP().toString(), 160, 135, 2);
    
    String wifiStatusText = WiFi.status() == WL_CONNECTED ? LangManager::get("wifi_connected_text") : LangManager::get("wifi_none_text");
    _tft->drawString(LangManager::get("wifi_menu_status") + " " + wifiStatusText, 160, 170, 2);

    drawMenuButton(20, 195, 280, 35, LangManager::get("btn_back"), TFT_MAROON, TFT_WHITE);
}

void MenuScreen::drawLanguageMenu() {
    ThemeColors theme = getCurrentTheme();
    _tft->setTextColor(theme.accent, theme.bg);
    _tft->drawString(LangManager::get("menu_btn_language"), 160, 20, 2);
    
    int btnX = 20;
    int btnY = 50;

    auto drawLangBtn = [&](const char* code, const char* name) {
        bool isActive = (_config.language == code);
        drawMenuButton(btnX, btnY, 135, 38, name, isActive ? theme.accent : theme.cardBg, isActive ? TFT_BLACK : theme.text);
        
        if (btnX == 20) { 
            btnX = 165; 
        } else { 
            btnX = 20; 
            btnY += 48; 
        }
    };

    drawLangBtn("hu", "Magyar");
    drawLangBtn("en", "English");
    drawLangBtn("de", "Deutsch");
    drawLangBtn("pl", "Polski");

    drawMenuButton(20, 195, 280, 35, LangManager::get("btn_back"), TFT_MAROON, TFT_WHITE);
}

void MenuScreen::drawSkinMenu() {
    ThemeColors theme = getCurrentTheme();
    _tft->setTextColor(theme.accent, theme.bg);
    _tft->drawString(LangManager::get("skin_menu_title"), 160, 20, 2);
    
    drawMenuButton(20, 60, 280, 38, LangManager::get("skin_dark"), _config.skin == "dark" ? theme.accent : theme.cardBg, _config.skin == "dark" ? TFT_BLACK : theme.text);
    drawMenuButton(20, 105, 280, 38, LangManager::get("skin_light"), _config.skin == "light" ? theme.accent : theme.cardBg, _config.skin == "light" ? TFT_BLACK : theme.text);
    drawMenuButton(20, 150, 280, 38, LangManager::get("skin_colorful"), _config.skin == "colorfull" ? theme.accent : theme.cardBg, _config.skin == "colorfull" ? TFT_BLACK : theme.text);

    drawMenuButton(20, 195, 280, 35, LangManager::get("btn_back"), TFT_MAROON, TFT_WHITE);
}

void MenuScreen::drawSystemMenu() {
    ThemeColors theme = getCurrentTheme();
    _tft->setTextColor(theme.accent, theme.bg);
    _tft->drawString(LangManager::get("system_menu_title"), 160, 15, 2);
    
    // 2x2 elrendezés (4 menüpont) - Dinamikus szövegekkel
    drawMenuButton(20, 45, 130, 65, LangManager::get("system_restart"), TFT_ORANGE, TFT_BLACK);
    drawMenuButton(170, 45, 130, 65, LangManager::get("system_del_config"), TFT_RED, TFT_WHITE);
    drawMenuButton(20, 120, 130, 65, LangManager::get("system_format_sd"), TFT_RED, TFT_WHITE);
    drawMenuButton(170, 120, 130, 65, LangManager::get("system_btn_info"), theme.cardBg, theme.text);

    drawMenuButton(20, 195, 280, 35, LangManager::get("btn_back"), TFT_MAROON, TFT_WHITE);
}

void MenuScreen::drawInfoMenu() {
    ThemeColors theme = getCurrentTheme();
    _tft->setTextColor(theme.accent, theme.bg);
    _tft->drawString(LangManager::get("info_menu_title"), 160, 15, 2);
    
    _tft->setTextColor(theme.text, theme.bg);
    _tft->setTextDatum(ML_DATUM);

    // 1. Szabad memória (Heap)
    uint32_t freeHeap = ESP.getFreeHeap();
    _tft->drawString(LangManager::get("info_free_ram") + " " + String(freeHeap / 1024) + " KB", 30, 50, 2);

    // 2. FW Verziószám
    _tft->drawString(LangManager::get("info_fw_version") + " " + "v1.0.0", 30, 80, 2);

    // 3. OctoPrint verzió (státusz / címke)
    _tft->drawString(LangManager::get("info_octo_version") + " " + "v1.10.x", 30, 110, 2);

    // 4. SD kártya fájlok száma és integritás (OK)
    int fileCount = 0;
    File root = SD.open("/");
    if (root) {
        File file = root.openNextFile();
        while (file) {
            if (!file.isDirectory()) {
                fileCount++;
            }
            file = root.openNextFile();
        }
        root.close();
    _tft->drawString(LangManager::get("info_sd_files") + " " + String(fileCount) + " db (OK)", 30, 140, 2);
    }

    _tft->setTextDatum(MC_DATUM);
    drawMenuButton(20, 195, 280, 35, LangManager::get("btn_back"), TFT_MAROON, TFT_WHITE);
}

bool MenuScreen::handleClick(uint16_t x, uint16_t y) {
    ThemeColors theme = getCurrentTheme();

    // Vissza gomb kezelése minden almenüben
    if (_currentSubMenu > 0 && y >= 195 && y <= 230) {
        UIUtils::pressFeedback(_tft, 20, 195, 280, 35, LangManager::get("btn_back"), TFT_MAROON, TFT_WHITE, 2, 6);
        
        if (_currentSubMenu == 5) {
            _currentSubMenu = 4; // Infóból visszatérés a Rendszer menübe
        } else {
            _currentSubMenu = 0; // Egyéb almenüből visszatérés a főmenübe
        }

        _mainMenuButtonsDrawn = false; 
        _config = ConfigManager::loadConfig(); 
        _tft->fillScreen(theme.bg);
        draw(_pOctoEnabled, _pOctoConn, _pOctoPrint, _pKlipperEnabled, _pKlipperConn, _pKlipperPrint);
        return false; 
    }

    if (_currentSubMenu == 0) {
        if (y >= 48 && y <= 106) {
            if (x >= 20 && x <= 150) { 
                UIUtils::pressFeedback(_tft, 20, 48, 130, 58, LangManager::get("menu_btn_wifi"), theme.cardBg, theme.text, 2, 6);
                _currentSubMenu = 1; _mainMenuButtonsDrawn = false; _tft->fillScreen(theme.bg); draw(_pOctoEnabled, _pOctoConn, _pOctoPrint, _pKlipperEnabled, _pKlipperConn, _pKlipperPrint); 
            }      
            else if (x >= 170 && x <= 300) { 
                UIUtils::pressFeedback(_tft, 170, 48, 130, 58, LangManager::get("menu_btn_language"), theme.cardBg, theme.text, 2, 6);
                _currentSubMenu = 2; _mainMenuButtonsDrawn = false; _tft->fillScreen(theme.bg); draw(_pOctoEnabled, _pOctoConn, _pOctoPrint, _pKlipperEnabled, _pKlipperConn, _pKlipperPrint); 
            } 
        }
        else if (y >= 114 && y <= 172) {
            if (x >= 20 && x <= 150) { 
                UIUtils::pressFeedback(_tft, 20, 114, 130, 58, LangManager::get("menu_btn_skin"), theme.cardBg, theme.text, 2, 6);
                _currentSubMenu = 3; _mainMenuButtonsDrawn = false; _tft->fillScreen(theme.bg); draw(_pOctoEnabled, _pOctoConn, _pOctoPrint, _pKlipperEnabled, _pKlipperConn, _pKlipperPrint); 
            }      
            else if (x >= 170 && x <= 300) { 
                UIUtils::pressFeedback(_tft, 170, 114, 130, 58, LangManager::get("menu_btn_system"), theme.cardBg, theme.text, 2, 6);
                _currentSubMenu = 4; _mainMenuButtonsDrawn = false; _tft->fillScreen(theme.bg); draw(_pOctoEnabled, _pOctoConn, _pOctoPrint, _pKlipperEnabled, _pKlipperConn, _pKlipperPrint); 
            } 
        }
    }
    else if (_currentSubMenu == 2) {
        int btnX = 20;
        int btnY = 50;
        String selectedLang = "";

        auto checkLangClick = [&](const char* code, const char* name) {
            if (selectedLang == "" && x >= btnX && x <= btnX + 135 && y >= btnY && y <= btnY + 38) {
                bool isActive = (_config.language == code);
                UIUtils::pressFeedback(_tft, btnX, btnY, 135, 38, name, isActive ? theme.accent : theme.cardBg, isActive ? TFT_BLACK : theme.text, 2, 6);
                selectedLang = code;
            }
            if (btnX == 20) { btnX = 165; }
            else { btnX = 20; btnY += 48; }
        };

        checkLangClick("hu", "Magyar");
        checkLangClick("en", "English");
        checkLangClick("de", "Deutsch");
        checkLangClick("pl", "Polski");

        if (selectedLang.length() > 0 && selectedLang != _config.language) {
            _config.language = selectedLang;
            digitalWrite(15, HIGH);
            digitalWrite(5, HIGH);
            delay(10);
            SPI.setFrequency(4000000);

            ConfigManager::saveConfig(_config);
            delay(50);
            LangManager::loadLanguage(_config.language);
            _config = ConfigManager::loadConfig();

            ThemeColors newTheme = getCurrentTheme();
            _tft->fillScreen(newTheme.bg);
            _lastSubMenuChecked = 255;
            _mainMenuButtonsDrawn = false;
            draw(_pOctoEnabled, _pOctoConn, _pOctoPrint, _pKlipperEnabled, _pKlipperConn, _pKlipperPrint);
        }
    }
    else if (_currentSubMenu == 3) {
        bool changed = false;
        if (y >= 60 && y <= 98) { 
            UIUtils::pressFeedback(_tft, 20, 60, 280, 38, LangManager::get("skin_dark"), _config.skin == "dark" ? theme.accent : theme.cardBg, _config.skin == "dark" ? TFT_BLACK : theme.text, 2, 6);
            _config.skin = "dark"; 
            changed = true; 
        }
        else if (y >= 105 && y <= 143) { 
            UIUtils::pressFeedback(_tft, 20, 105, 280, 38, LangManager::get("skin_light"), _config.skin == "light" ? theme.accent : theme.cardBg, _config.skin == "light" ? TFT_BLACK : theme.text, 2, 6);
            _config.skin = "light"; 
            changed = true; 
        }
        else if (y >= 150 && y <= 188) { 
            UIUtils::pressFeedback(_tft, 20, 150, 280, 38, LangManager::get("skin_colorful"), _config.skin == "colorfull" ? theme.accent : theme.cardBg, _config.skin == "colorfull" ? TFT_BLACK : theme.text, 2, 6);
            _config.skin = "colorfull"; 
            changed = true; 
        }
        
        if (changed) {
            ConfigManager::saveConfig(_config);
            ThemeColors newTheme = getCurrentTheme();
            _tft->fillScreen(newTheme.bg);
            _lastSubMenuChecked = 255;
            draw(_pOctoEnabled, _pOctoConn, _pOctoPrint, _pKlipperEnabled, _pKlipperConn, _pKlipperPrint);
        }
    }
    else if (_currentSubMenu == 4) {
        // 2x2 Rendszer menü kattintáskezelés
        if (y >= 45 && y <= 110) {
            if (x >= 20 && x <= 150) { 
                UIUtils::pressFeedback(_tft, 20, 45, 130, 65, LangManager::get("system_restart"), TFT_ORANGE, TFT_BLACK, 2, 6);
                ESP.restart(); 
            }
            else if (x >= 170 && x <= 300) { 
                UIUtils::pressFeedback(_tft, 170, 45, 130, 65, LangManager::get("system_del_config"), TFT_RED, TFT_WHITE, 2, 6);
                if (SD.exists("/config.json")) SD.remove("/config.json");
                ConfigManager::createDefaultConfig();
                _config = ConfigManager::loadConfig();
                draw(_pOctoEnabled, _pOctoConn, _pOctoPrint, _pKlipperEnabled, _pKlipperConn, _pKlipperPrint);
            }
        }
        else if (y >= 120 && y <= 185) {
            if (x >= 20 && x <= 150) { 
                UIUtils::pressFeedback(_tft, 20, 120, 130, 65, LangManager::get("system_format_sd"), TFT_RED, TFT_WHITE, 2, 6);
                SD.remove("/config.json");
                SD.remove("/system.log");
                delay(500);
                ESP.restart();
            }
            else if (x >= 170 && x <= 300) { 
                UIUtils::pressFeedback(_tft, 170, 120, 130, 65, LangManager::get("system_btn_info"), theme.cardBg, theme.text, 2, 6);
                _currentSubMenu = 5; // Nyitás az Info almenüre
                _mainMenuButtonsDrawn = false;
                _tft->fillScreen(theme.bg);
                draw(_pOctoEnabled, _pOctoConn, _pOctoPrint, _pKlipperEnabled, _pKlipperConn, _pKlipperPrint);
            }
        }
    }
    return false;
}