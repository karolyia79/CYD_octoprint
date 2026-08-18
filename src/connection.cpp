#include "connection.h"

static unsigned long lastWifiCheck = 0;
static const unsigned long WIFI_RETRY_INTERVAL = 10000; // 10 másodperc
static bool isConnectingFlag = false;

void ConnectionManager::init() {
    isConnectingFlag = false;
}

void ConnectionManager::update() {
    if (WiFi.status() != WL_CONNECTED) {
        isConnectingFlag = true;
        if (millis() - lastWifiCheck > WIFI_RETRY_INTERVAL) {
            Serial.println("[CONNECTION] Wi-Fi kapcsolat megszakadt! Ujrakapcsolodasi kiserlet...");
            WiFi.reconnect();
            lastWifiCheck = millis();
        }
    } else {
        isConnectingFlag = false;
    }
}

WiFiState ConnectionManager::getState() {
    if (WiFi.status() == WL_CONNECTED) {
        return WIFI_STATE_CONNECTED;
    } else if (isConnectingFlag) {
        return WIFI_STATE_CONNECTING;
    } else {
        return WIFI_STATE_DISCONNECTED;
    }
}

uint16_t ConnectionManager::getStatusColor() {
    WiFiState state = getState();
    switch (state) {
        case WIFI_STATE_CONNECTED:    return TFT_SKYBLUE;    
        case WIFI_STATE_CONNECTING:   return TFT_YELLOW;   
        case WIFI_STATE_DISCONNECTED: 
        default:                      return TFT_RED;     
    }
}

void ConnectionManager::drawIcon(TFT_eSPI* tft, int x, int y) {
    uint16_t color = getStatusColor();
    
    // Alap pont (középen alul)
    tft->fillCircle(x, y, 2, color);
    
    // Alsó ív (magasabbra tolva, hogy ne érjen össze a ponttal)
    int x_bottom[] = {-3, -2, -1, 0, 1, 2, 3};
    int y_bottom[] = {-5, -6, -6, -6, -6, -6, -5};
    for (int i = 0; i < 7; i++) {
        tft->fillRect(x + x_bottom[i], y + y_bottom[i], 1, 2, color);
    }

    // Felső ív (mégjebb tolva és szélesítve a jobb arányért)
    int x_top[] = {-6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6};
    int y_top[] = {-9, -9, -10, -10, -10, -10, -10, -10, -10, -10, -10, -9, -9};
    for (int i = 0; i < 13; i++) {
        tft->fillRect(x + x_top[i], y + y_top[i], 1, 2, color);
    }
}