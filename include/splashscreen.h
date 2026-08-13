#ifndef SPLASHSCREEN_H
#define SPLASHSCREEN_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <Wire.h>
#include <SD.h>
#include "CST820.h"

class SplashScreen {
public:
    SplashScreen(TFT_eSPI* tft);
    void init();
    void showMessage(const String& msg, uint16_t color = TFT_WHITE);
    void drawProgressBar(int progress);
    void showAPInfo(const String& ssid, const String& pass, const String& ip, bool clientConnected, const String& wifiError = "");
    void showConnectedInfo(const String& localIp, bool octoActive, bool klipperActive, bool pluginMissing = false, bool mqttConnected = true);
    bool getTouch(uint16_t *x, uint16_t *y);
    bool hasError() const { return _hasError; }

private:
    TFT_eSPI* _tft;
    CST820 _touch;
    bool fontLoaded = false;
    bool _hasError = false;
};

#endif