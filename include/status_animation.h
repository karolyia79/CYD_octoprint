#ifndef STATUS_ANIMATION_H
#define STATUS_ANIMATION_H

#include <TFT_eSPI.h>

class StatusAnimation {
private:
    TFT_eSPI* _tft;
    int _x, _y, _w, _h;

    bool _bedHeating;
    bool _hotendHeating;
    bool _moving;
    bool _extruding;

    // Utolsó ismert állapotok a tiszta törléshez
    bool _lastBedHeating;
    bool _lastHotendHeating;
    bool _lastMoving;
    bool _lastExtruding;

    unsigned long _lastAnimTime;
    float _pulsePhase;  
    int _hotendOffsetX; 
    int _moveDirection; 
    int _lastDrawnX;    

    uint16_t getPulsingColor(uint16_t baseColor, uint16_t pulseColor, float phase);
    void drawBed(uint16_t bedColor);
    void drawHotend(int currentX, uint16_t nozzleColor, bool showExtrusion);

public:
    StatusAnimation(TFT_eSPI* tft);
    void init(int x, int y, int w, int h);
    
    void updateStatus(bool bedHeating, bool hotendHeating, bool moving, bool extruding);
    void update(uint16_t bgColor = TFT_BLACK);
};

#endif // STATUS_ANIMATION_H