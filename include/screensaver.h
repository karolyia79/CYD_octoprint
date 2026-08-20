#ifndef SCREENSAVER_H
#define SCREENSAVER_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "octo_client_mqtt.h"

class Screensaver {
private:
    TFT_eSPI* _tft;
    TFT_eSprite _spr;
    unsigned long _cycleStartTime;
    int _posX;
    int _posY;

    // Gyorsítótárazott adatok (ciklikusan frissülnek)
    String _cachedTimeStr;
    String _cachedDateStr;
    String _cachedProgStr;
    bool _isPrinting;
    float _lastAlpha;
    bool _firstRun;

    uint16_t dimColor(uint16_t color, float alpha);
    void pickNewPosition();

public:
    Screensaver(TFT_eSPI* tft);
    ~Screensaver();
    void init();
    void draw(OctoClientMqtt* octoMqtt);
};

#endif // SCREENSAVER_H