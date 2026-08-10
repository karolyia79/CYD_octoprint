#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <SD.h>
#include <TFT_eSPI.h>

struct ThemeColors {
    uint16_t bg;
    uint16_t cardBg;
    uint16_t text;
    uint16_t subText;
    uint16_t accent;
};

struct PrinterConfig {
    bool octo_enabled;
    String octo_ip;
    String octo_key;
    bool klipper_enabled;
    String klipper_ip;
    int klipper_port;     // Hozzáadva
    String klipper_key;    // Hozzáadva
    String skin;
    String language;
    
    // Wi-Fi és hálózati beállítások (hiányzó mezők pótolva)
    String wifi_ssid;
    String wifi_pass;
    bool use_static_ip;
    String static_ip;
    String gateway;
    String subnet;
    String dns;
};

class ConfigManager {
public:
    static bool init();
    static PrinterConfig loadConfig();
    static bool saveConfig(const PrinterConfig& config);
    static void createDefaultConfig();
};

// Globális dinamikus téma lekérdező
inline ThemeColors getCurrentTheme() {
    PrinterConfig config = ConfigManager::loadConfig();
    
    if (config.skin == "light") {
        return { TFT_WHITE, 0xEF7D, TFT_BLACK, TFT_DARKGREY, TFT_BLUE }; // Világos skin
    } else if (config.skin == "colorfull") {
        return { 0x0010, 0x001F, TFT_YELLOW, TFT_CYAN, TFT_MAGENTA };     // Színes skin
    } else {
        return { 0x0000, 0x2104, TFT_WHITE, TFT_DARKGREY, TFT_BLUE };     // Sötét skin (Alapértelmezett)
    }
}

#endif