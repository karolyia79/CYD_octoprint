#include "status_animation.h"
#include <math.h>

StatusAnimation::StatusAnimation(TFT_eSPI* tft) 
    : _tft(tft), _x(0), _y(0), _w(0), _h(0),
      _bedHeating(false), _hotendHeating(false), _moving(false), _extruding(false),
      _lastBedHeating(false), _lastHotendHeating(false), _lastMoving(false), _lastExtruding(false),
      _lastAnimTime(0), _pulsePhase(0.0f), _hotendOffsetX(0), _moveDirection(1), _lastDrawnX(-1) {}

void StatusAnimation::init(int x, int y, int w, int h) {
    _x = x;
    _y = y;
    _w = w;
    _h = h;
    _hotendOffsetX = _w / 2;
    _lastDrawnX = _x + _hotendOffsetX;
}

void StatusAnimation::updateStatus(bool bedHeating, bool hotendHeating, bool moving, bool extruding) {
    _bedHeating = bedHeating;
    _hotendHeating = hotendHeating;
    _moving = moving;
    _extruding = extruding;
}

uint16_t StatusAnimation::getPulsingColor(uint16_t baseColor, uint16_t pulseColor, float phase) {
    float factor = (sin(phase) + 1.0f) / 2.0f; 

    uint8_t r1 = (baseColor >> 11) & 0x1F;
    uint8_t g1 = (baseColor >> 5) & 0x3F;
    uint8_t b1 = baseColor & 0x1F;

    uint8_t r2 = (pulseColor >> 11) & 0x1F;
    uint8_t g2 = (pulseColor >> 5) & 0x3F;
    uint8_t b2 = pulseColor & 0x1F;

    uint8_t r = r1 + (uint8_t)((r2 - r1) * factor);
    uint8_t g = g1 + (uint8_t)((g2 - g1) * factor);
    uint8_t b = b1 + (uint8_t)((b2 - b1) * factor);

    return (r << 11) | (g << 5) | b;
}

void StatusAnimation::drawBed(uint16_t bedColor) {
    int bedY = _y + _h - 12;
    int bedMargin = 6;
    
    _tft->fillRoundRect(_x + bedMargin, bedY, _w - (bedMargin * 2), 6, 2, bedColor);
    _tft->fillRect(_x + bedMargin + 6, bedY + 6, 4, 3, TFT_DARKGREY);
    _tft->fillRect(_x + _w - bedMargin - 10, bedY + 6, 4, 3, TFT_DARKGREY);
}

void StatusAnimation::drawHotend(int currentX, uint16_t nozzleColor, bool showExtrusion) {
    int topY = _y + 6;

    _tft->fillRect(currentX - 6, topY, 12, 10, TFT_SILVER);
    _tft->drawFastHLine(currentX - 6, topY + 3, 12, TFT_BLACK);
    _tft->drawFastHLine(currentX - 6, topY + 6, 12, TFT_BLACK);

    _tft->fillRect(currentX - 5, topY + 10, 10, 6, nozzleColor);
    _tft->fillTriangle(currentX - 4, topY + 16, currentX + 4, topY + 16, currentX, topY + 20, nozzleColor);

    int bedY = _y + _h - 12;
    if (showExtrusion) {
        _tft->drawFastVLine(currentX, topY + 20, bedY - (topY + 20), TFT_CYAN);
    }
}

void StatusAnimation::update(uint16_t bgColor) {
    if (millis() - _lastAnimTime < 50) return;
    _lastAnimTime = millis();

    _pulsePhase += 0.15f;
    if (_pulsePhase > 6.28f) _pulsePhase = 0.0f;

    uint16_t bedColor = TFT_DARKGREY;
    if (_bedHeating) {
        bedColor = getPulsingColor(TFT_DARKGREY, TFT_RED, _pulsePhase);
    }

    uint16_t nozzleColor = TFT_ORANGE;
    if (_hotendHeating) {
        nozzleColor = getPulsingColor(TFT_ORANGE, TFT_RED, _pulsePhase);
    }

    int minX = 14;
    int maxX = _w - 14;
    int centerX = _w / 2;

    if (_moving) {
        _hotendOffsetX += _moveDirection * 2;
        if (_hotendOffsetX >= maxX) {
            _hotendOffsetX = maxX;
            _moveDirection = -1;
        } else if (_hotendOffsetX <= minX) {
            _hotendOffsetX = minX;
            _moveDirection = 1;
        }
    } else if (_hotendOffsetX != centerX) {
        // Mozgás leállásakor visszatérünk középre
        if (_hotendOffsetX < centerX) {
            _hotendOffsetX += 2;
            if (_hotendOffsetX > centerX) _hotendOffsetX = centerX;
        } else {
            _hotendOffsetX -= 2;
            if (_hotendOffsetX < centerX) _hotendOffsetX = centerX;
        }
    }

    int currentX = _x + _hotendOffsetX;

    // Állapotváltozás detektálása (pl. extrudálás vagy fűtés vége)
    bool stateChanged = (_lastExtruding != _extruding) || 
                        (_lastHotendHeating != _hotendHeating) || 
                        (_lastBedHeating != _bedHeating) || 
                        (_lastMoving != _moving);

    if (_lastDrawnX != currentX || _extruding || _hotendHeating || _bedHeating || stateChanged) {
        _tft->fillRect(_x, _y, _w, _h, bgColor);
    }

    _lastExtruding = _extruding;
    _lastHotendHeating = _hotendHeating;
    _lastBedHeating = _bedHeating;
    _lastMoving = _moving;

    drawBed(bedColor);
    drawHotend(currentX, nozzleColor, _extruding);

    _lastDrawnX = currentX;
}