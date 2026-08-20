#include "screensaver.h"
#include <time.h>

Screensaver::Screensaver(TFT_eSPI* tft) 
    : _tft(tft), _spr(tft), _cycleStartTime(0), _posX(160), _posY(120),
      _cachedTimeStr("--:--"), _cachedDateStr(""), _cachedProgStr(""),
      _isPrinting(false), _lastAlpha(-1.0f), _firstRun(true) {}

Screensaver::~Screensaver() {
    if (_spr.created()) {
        _spr.deleteSprite();
    }
}

// Szoftveres RGB565 alfa-halványítás
uint16_t Screensaver::dimColor(uint16_t color, float alpha) {
    if (alpha <= 0.0f) return TFT_BLACK;
    if (alpha >= 1.0f) return color;

    uint8_t r = (uint8_t)((((color >> 11) & 0x1F) << 3) * alpha);
    uint8_t g = (uint8_t)((((color >> 5) & 0x3F) << 2) * alpha);
    uint8_t b = (uint8_t)(((color & 0x1F) << 3) * alpha);

    return _tft->color565(r, g, b);
}

// Új véletlenszerű pozíció választása
void Screensaver::pickNewPosition() {
    int oldX = _posX;
    int oldY = _posY;

    for (int i = 0; i < 10; i++) {
        _posX = random(145, 176); // 145 .. 175 px
        _posY = random(100, 131); // 100 .. 130 px

        if (abs(_posX - oldX) > 10 || abs(_posY - oldY) > 10) {
            break;
        }
    }
}

void Screensaver::init() {
    _cycleStartTime = millis();
    _firstRun = true;
    _lastAlpha = -1.0f;
    
    if (!_spr.created()) {
        _spr.setColorDepth(16);
        if (!_spr.createSprite(280, 170)) {
            _spr.setColorDepth(8);
            _spr.createSprite(280, 170);
        }
    }

    _tft->fillScreen(TFT_BLACK);
    Serial.println("[SCREENSAVER] Pixelpontos elrendezésű képernyővédő aktiválva.");
}

void Screensaver::draw(OctoClientMqtt* octoMqtt) {
    if (!_spr.created()) return;

    unsigned long now = millis();
    unsigned long elapsed = now - _cycleStartTime;

    // 1. CIKLUS ELEJÉN ADATOK FRISSÍTÉSE (Kizárólag Fade Out után, sötétben)
    if (elapsed >= 7000 || _firstRun) {
        _cycleStartTime = now;
        elapsed = 0;
        _firstRun = false;

        struct tm timeinfo;
        char timeBuf[16] = "--:--";
        char dateBuf[32] = "";

        if (getLocalTime(&timeinfo, 50) && timeinfo.tm_year > 70) {
            strftime(timeBuf, sizeof(timeBuf), "%H:%M", &timeinfo);
            strftime(dateBuf, sizeof(dateBuf), "%Y. %b. %d.", &timeinfo);
        }

        _cachedTimeStr = String(timeBuf);
        _cachedDateStr = String(dateBuf);

        _cachedProgStr = "";
        _isPrinting = false;
        if (octoMqtt) {
            OctoPrinterData data = octoMqtt->getData();
            _isPrinting = data.printingActive || data.status == "Cancelling" || data.status == "Nyomtatás leállítása...";
            if (_isPrinting) {
                _cachedProgStr = String(data.progress) + "%";
            }
        }

        pickNewPosition();
        _tft->fillScreen(TFT_BLACK);
        _lastAlpha = -1.0f;
    }

    // 2. ALFA FÁZIS KISZÁMÍTÁSA
    float alpha = 0.0f;
    if (elapsed < 1000) {
        alpha = (float)elapsed / 1000.0f; // Fade In
    } else if (elapsed < 6000) {
        alpha = 1.0f;                      // Hold 5 mp
    } else {
        alpha = 1.0f - ((float)(elapsed - 6000) / 1000.0f); // Fade Out
    }

    if (alpha < 0.0f) alpha = 0.0f;
    if (alpha > 1.0f) alpha = 1.0f;

    // Ha nincs változás a Hold fázisban, kihagyjuk a rajzolást
    if (abs(alpha - _lastAlpha) < 0.01f && elapsed >= 1000 && elapsed <= 6000) {
        return;
    }
    _lastAlpha = alpha;

    // 3. RAJZOLÁS A PUFFERRE
    uint16_t rawClockColor = _tft->color565(235, 238, 242);
    uint16_t rawDateColor  = _tft->color565(170, 178, 188);

    uint16_t clockColor = dimColor(rawClockColor, alpha);
    uint16_t dateColor  = dimColor(rawDateColor, alpha);

    _spr.fillSprite(TFT_BLACK);

    // 1. ÓRA (Alsó szél igazítással: BC_DATUM)
    _spr.setFreeFont(&FreeSans24pt7b);
    _spr.setTextSize(1);
    _spr.setTextDatum(BC_DATUM);
    _spr.setTextColor(clockColor, TFT_BLACK);
    
    int clockBottomY = 72; // Óra aljának pozíciója a Sprite-ban
    _spr.drawString(_cachedTimeStr, 140, clockBottomY);

    // 2. VÉKONY VÁLASZTÓVONAL (Pontosan 5 pixelre az óra alatt, 1px vastag)
    int lineY = clockBottomY + 5; 
    int lineWidth = 90; // Elegáns, szűk választóvonal
    _spr.drawFastHLine(140 - (lineWidth / 2), lineY, lineWidth, dateColor);

    // 3. DÁTUM (Pontosan 3 pixelre a vonal alatt, felső igazítással: TC_DATUM)
    if (_cachedDateStr.length() > 0) {
        _spr.setFreeFont(&FreeSans9pt7b);
        _spr.setTextDatum(TC_DATUM);
        _spr.setTextColor(dateColor, TFT_BLACK);
        
        int dateTopY = lineY + 1 + 3; // vonal Y + 1px vonalvastagság + 3px hézag
        _spr.drawString(_cachedDateStr, 140, dateTopY);
    }

    // 4. NYOMTATÁSI HALADÁS (Ha nyomtat a gép)
    if (_isPrinting && _cachedProgStr.length() > 0) {
        float pulseFactor = (sin(now / 400.0) + 1.0) / 2.0;
        uint8_t cyanG = 140 + (uint8_t)(115.0 * pulseFactor);
        uint16_t baseCyan = _tft->color565(0, cyanG, 255);
        uint16_t progColor = dimColor(baseCyan, alpha);

        _spr.setFreeFont(&FreeSans12pt7b);
        _spr.setTextDatum(TC_DATUM);
        _spr.setTextColor(progColor, TFT_BLACK);
        _spr.drawString(_cachedProgStr, 140, 120);
    }

    // Puffer megjelenítése a kijelzőn
    int screenX = _posX - 140;
    int screenY = _posY - 85;
    _spr.pushSprite(screenX, screenY);
}