#ifndef OCTO_CONFIG_H
#define OCTO_CONFIG_H

#include <Arduino.h>
#include <SD.h>
#include <ArduinoJson.h>

// Bármilyen jövőbeli beállítást ide is felvehetsz!
struct OctoConfigData {
    int fl_x = 30;  int fl_y = 30;
    int fr_x = 200; int fr_y = 30;
    int br_x = 200; int br_y = 200;
    int bl_x = 30;  int bl_y = 200;
};

class OctoConfigManager {
public:
    static void begin();
    static OctoConfigData loadConfig();
    static void saveConfig(const OctoConfigData& config);
    static void createDefaultConfig();
};

#endif