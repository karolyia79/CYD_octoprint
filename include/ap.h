#ifndef AP_H
#define AP_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "splashscreen.h"
#include "logger.h"
#include "config_manager.h"

class APManager {
public:
    APManager(SplashScreen* splash);
    void begin();
    void handleClient();

private:
    SplashScreen* _splash;
    WebServer _server;
    String _lastWifiError = "";
    void setupRoutes();
    void connectWiFi(const PrinterConfig& cfg);
    void startAPMode(); 
};

#endif