#include "config_manager.h"
#include "lang_manager.h"
#include <SPI.h>
#include <SD.h>

#define SD_CS 5

bool ConfigManager::init() {
    pinMode(15, OUTPUT);
    digitalWrite(15, HIGH);

    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);

    if (!SD.begin(SD_CS, SPI, 1000000)) {
        Serial.println("[HIBA] SD.begin failed! Nem eri el az SD kartyat.");
        return false;
    }

    if (!SD.exists("/config.json")) {
        createDefaultConfig();
    }

    // A beállított nyelv azonnali betöltése a konfigurációból
    PrinterConfig cfg = loadConfig();
    LangManager::loadLanguage(cfg.language);

    return true;
}

void ConfigManager::createDefaultConfig() {
    PrinterConfig defaultCfg;
    saveConfig(defaultCfg);
}

PrinterConfig ConfigManager::loadConfig() {
    PrinterConfig config;
    if (!SD.exists("/config.json")) {
        createDefaultConfig();
    }

    File file = SD.open("/config.json", FILE_READ);
    if (!file) return config;

    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) return config;

    config.wifi_ssid = doc["wifi_ssid"] | "";
    config.wifi_pass = doc["wifi_pass"] | "";
    config.use_static_ip = doc["use_static_ip"] | false;
    config.static_ip = doc["static_ip"] | "";
    config.gateway = doc["gateway"] | "";
    config.subnet = doc["subnet"] | "";
    config.dns = doc["dns"] | "";

    config.octo_enabled = doc["octo_enabled"] | false;
    config.octo_ip = doc["octo_ip"] | "";
    config.octo_key = doc["octo_key"] | "";

    config.klipper_enabled = doc["klipper_enabled"] | false;
    config.klipper_ip = doc["klipper_ip"] | "";
    config.klipper_port = doc["klipper_port"] | 7125;
    config.klipper_key = doc["klipper_key"] | "";

    config.skin = doc["skin"] | "dark";
    config.language = doc["language"] | "hu";

    return config;
}

bool ConfigManager::saveConfig(const PrinterConfig& config) {
    if (SD.exists("/config.json")) {
        SD.remove("/config.json");
    }

    File file = SD.open("/config.json", FILE_WRITE);
    if (!file) return false;

    StaticJsonDocument<1024> doc;
    doc["wifi_ssid"] = config.wifi_ssid;
    doc["wifi_pass"] = config.wifi_pass;
    doc["use_static_ip"] = config.use_static_ip;
    doc["static_ip"] = config.static_ip;
    doc["gateway"] = config.gateway;
    doc["subnet"] = config.subnet;
    doc["dns"] = config.dns;

    doc["octo_enabled"] = config.octo_enabled;
    doc["octo_ip"] = config.octo_ip;
    doc["octo_key"] = config.octo_key;

    doc["klipper_enabled"] = config.klipper_enabled;
    doc["klipper_ip"] = config.klipper_ip;
    doc["klipper_port"] = config.klipper_port;
    doc["klipper_key"] = config.klipper_key;

    doc["skin"] = config.skin;
    doc["language"] = config.language;

    if (serializeJson(doc, file) == 0) {
        file.close();
        return false;
    }

    file.close();
    return true;
}