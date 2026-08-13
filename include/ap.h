#ifndef AP_H
#define AP_H

#include <WiFi.h>
#include <WebServer.h>
#include <SD.h>
#include <ArduinoJson.h>
#include "config_manager.h"
#include "logger.h"
#include "splashscreen.h"

class APManager {
public:
    APManager(SplashScreen* splash);
    void begin();
    void startServer();
    void handleClient();
    void setupRoutes();

private:
    SplashScreen* _splash;
    WebServer _server;
    String _lastWifiError;
    void connectWiFi(const PrinterConfig& cfg);
    void startAPMode();
};

#endif