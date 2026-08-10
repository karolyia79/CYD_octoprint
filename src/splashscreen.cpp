#include "splashscreen.h"
#include "lang_manager.h"

SplashScreen::SplashScreen(TFT_eSPI* tft) : _tft(tft) {}

void SplashScreen::init() {
    _tft->init();
    _tft->setRotation(1);
    _tft->fillScreen(TFT_BLACK);
    _hasError = false;
    
    _tft->setTextColor(TFT_WHITE, TFT_BLACK);
    _tft->setTextDatum(MC_DATUM);
    _tft->drawString(LangManager::get("OctoKlipScreen"), 160, 60, 4);
}

void SplashScreen::showMessage(const String& msg, uint16_t color) {
    _tft->fillRect(0, 100, 320, 30, TFT_BLACK);
    _tft->setTextColor(color, TFT_BLACK);
    _tft->setTextDatum(MC_DATUM);
    _tft->drawString(msg, 160, 115, 2);
}

void SplashScreen::drawProgressBar(int progress) {
    _tft->drawRect(40, 150, 240, 16, TFT_WHITE);
    int fillWidth = (236 * progress) / 100;
    if (fillWidth > 0) {
        _tft->fillRect(42, 152, fillWidth, 12, TFT_DARKGREEN);
    }
}

void SplashScreen::showAPInfo(const String& ssid, const String& pass, const String& ip, bool clientConnected, const String& wifiError) {
    _tft->fillScreen(TFT_BLACK);
    _hasError = false;
    
    _tft->setTextColor(TFT_ORANGE, TFT_BLACK);
    _tft->setTextDatum(MC_DATUM);
    _tft->drawString(LangManager::get("splash_ap_title"), 160, 15, 2);

    if (wifiError.length() > 0) {
        _tft->setTextColor(TFT_RED, TFT_BLACK);
        _tft->setTextDatum(MC_DATUM);
        _tft->drawString(LangManager::get("splash_error") + wifiError, 160, 38, 1);
    }

    _tft->setTextColor(TFT_WHITE, TFT_BLACK);
    _tft->setTextDatum(ML_DATUM);
    
    _tft->drawString(LangManager::get("splash_ssid") + ssid, 20, 60, 2);
    _tft->drawString(LangManager::get("splash_pass") + pass, 20, 90, 2);
    _tft->drawString(LangManager::get("splash_webui_ip") + ip, 20, 120, 2);
    
    _tft->setTextColor(TFT_CYAN, TFT_BLACK);
    _tft->drawString(LangManager::get("splash_open_browser"), 20, 160, 1);

    if (clientConnected) {
        _tft->fillCircle(28, 205, 6, TFT_GREEN);
        _tft->setTextColor(TFT_GREEN, TFT_BLACK);
        _tft->setTextDatum(ML_DATUM);
        _tft->drawString(LangManager::get("splash_client_connected"), 42, 205, 1);
    } else {
        _tft->fillCircle(28, 205, 6, TFT_DARKGREY);
        _tft->setTextColor(TFT_DARKGREY, TFT_BLACK);
        _tft->setTextDatum(ML_DATUM);
        _tft->drawString(LangManager::get("splash_waiting_client"), 42, 205, 1);
    }
}

void SplashScreen::showConnectedInfo(const String& localIp, bool octoActive, bool klipperActive, bool pluginMissing) {
    _tft->fillScreen(TFT_BLACK);
    _hasError = pluginMissing; 

    if (_hasError) {
        // --- HIBA: HIÁNYZÓ OCTOPRINT PLUGIN ---
        _tft->setTextColor(TFT_RED, TFT_BLACK);
        _tft->setTextDatum(MC_DATUM);
        _tft->drawString("HIBA: HIANYZO PLUGIN!", 160, 20, 2);

        _tft->setTextColor(TFT_WHITE, TFT_BLACK);
        _tft->setTextDatum(ML_DATUM);
        _tft->drawString("OctoKlipscreenBridge szukseges!", 20, 60, 1);
        
        _tft->setTextColor(TFT_YELLOW, TFT_BLACK);
        _tft->drawString("Telepitsd innen:", 20, 95, 1);
        _tft->setTextColor(TFT_CYAN, TFT_BLACK);
        _tft->drawString("github.com/karolyia79/", 20, 115, 1);
        _tft->drawString("OctoklipscreenBridge", 20, 132, 1);

        _tft->setTextColor(TFT_GREEN, TFT_BLACK);
        _tft->setTextDatum(MC_DATUM);
        _tft->drawString("Erints meg a kepernyot a folytatashoz", 160, 195, 1);
    } else {
        // --- NORMÁL CSATLAKOZOTT NÉZET ---
        _tft->setTextColor(TFT_GREEN, TFT_BLACK);
        _tft->setTextDatum(MC_DATUM);
        _tft->drawString(LangManager::get("splash_connected_title"), 160, 20, 2);

        _tft->setTextColor(TFT_WHITE, TFT_BLACK);
        _tft->setTextDatum(ML_DATUM);
        
        _tft->drawString(LangManager::get("splash_device_ip") + localIp, 20, 65, 2);
        
        _tft->drawString(LangManager::get("splash_octoprint"), 20, 105, 2);
        uint16_t octoColor = octoActive ? TFT_GREEN : TFT_DARKGREY;
        _tft->fillCircle(200, 112, 6, octoColor);
        _tft->drawCircle(200, 112, 6, TFT_WHITE);
        _tft->drawString(octoActive ? LangManager::get("splash_active") : LangManager::get("splash_disabled"), 220, 105, 2);

        _tft->drawString(LangManager::get("splash_klipper"), 20, 145, 2);
        uint16_t klipperColor = klipperActive ? TFT_GREEN : TFT_DARKGREY;
        _tft->fillCircle(200, 152, 6, klipperColor);
        _tft->drawCircle(200, 152, 6, TFT_WHITE);
        _tft->drawString(klipperActive ? LangManager::get("splash_active") : LangManager::get("splash_disabled"), 220, 145, 2);

        _tft->setTextColor(TFT_YELLOW, TFT_BLACK);
        _tft->drawString(LangManager::get("splash_webui_available"), 20, 195, 1);
    }
}

bool SplashScreen::getTouch(uint16_t *x, uint16_t *y) {
    uint8_t gesture = 0;
    return _touch.getTouch(x, y, &gesture);
}