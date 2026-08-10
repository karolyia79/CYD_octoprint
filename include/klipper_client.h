#ifndef KLIPPER_CLIENT_H
#define KLIPPER_CLIENT_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

struct KlipperPrinterData {
    String name = "Klipper";
    String status = "Kapcsolódás...";
    float nozzleTemp = 0;
    float nozzleTarget = 0;
    float bedTemp = 0;
    float bedTarget = 0;
    int speed = 100;
    int progress = 0;
    String remainingTime = "-";
    bool connected = false;
};

class KlipperClient {
public:
    KlipperClient(const String& ip);
    void update();
    const KlipperPrinterData& getData() const { return _data; }

    // Instant Moonraker G-code vezérlő funkciók
    void setNozzleTarget(float temp);
    void setBedTarget(float temp);
    void setSpeed(int percent);
    void adjustZOffset(float delta);

private:
    String _ip;
    KlipperPrinterData _data;

    void sendGcode(const String& gcode);
};

#endif