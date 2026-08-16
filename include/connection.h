#ifndef CONNECTION_H
#define CONNECTION_H

#include <Arduino.h>
#include <WiFi.h>
#include <TFT_eSPI.h>

enum WiFiState {
    WIFI_STATE_CONNECTED,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_DISCONNECTED
};

class ConnectionManager {
public:
    static void init();
    static void update(); // Háttérben futó ciklus
    static WiFiState getState();
    static uint16_t getStatusColor();
    
    // Kompakt Wi-Fi ikon kirajzolása a megadott (x, y) koordinátára
    static void drawIcon(TFT_eSPI* tft, int x, int y);
};

#endif