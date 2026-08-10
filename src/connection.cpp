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
            Serial.println("[CONNECTION] Wi-Fi kapcsolat megszakadt! Újrakapcsolódási kísérlet...");
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
        case WIFI_STATE_CONNECTED:    return TFT_GREEN;    // Zöld: OK
        case WIFI_STATE_CONNECTING:   return TFT_YELLOW;   // Sárga: Kapcsolódás
        case WIFI_STATE_DISCONNECTED: 
        default:                      return TFT_RED;      // Piros: Nincs kapcsolat
    }
}

// Kompakt Wi-Fi ikon kirajzolása (kb. 13x9 pixel méretben)
void ConnectionManager::drawIcon(TFT_eSPI* tft, int x, int y) {
    uint16_t color = getStatusColor();
    
    // Alap pont (középen alul)
    tft->fillCircle(x, y, 1.5, color);
    
    // Alsó ív
    tft->drawPixel(x - 3, y - 3, color);
    tft->drawPixel(x - 2, y - 4, color);
    tft->drawPixel(x - 1, y - 4, color);
    tft->drawPixel(x,     y - 4, color);
    tft->drawPixel(x + 1, y - 4, color);
    tft->drawPixel(x + 2, y - 4, color);
    tft->drawPixel(x + 3, y - 3, color);

    // Felső ív
    tft->drawPixel(x - 5, y - 6, color);
    tft->drawPixel(x - 4, y - 6, color);
    tft->drawPixel(x - 3, y - 7, color);
    tft->drawPixel(x - 2, y - 7, color);
    tft->drawPixel(x - 1, y - 7, color);
    tft->drawPixel(x,     y - 7, color);
    tft->drawPixel(x + 1, y - 7, color);
    tft->drawPixel(x + 2, y - 7, color);
    tft->drawPixel(x + 3, y - 7, color);
    tft->drawPixel(x + 4, y - 6, color);
    tft->drawPixel(x + 5, y - 6, color);
}