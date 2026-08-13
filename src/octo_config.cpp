#include "octo_config.h"
#include "lang_manager.h"

const char* OCTO_CONFIG_FILE = "/octoconfig.json";

void OctoConfigManager::begin() {
    if (!SD.exists(OCTO_CONFIG_FILE)) {
        createDefaultConfig();
    }
}

OctoConfigData OctoConfigManager::loadConfig() {
    OctoConfigData config;
    if (!SD.exists(OCTO_CONFIG_FILE)) {
        createDefaultConfig();
        return config;
    }

    File file = SD.open(OCTO_CONFIG_FILE, FILE_READ);
    if (!file) return config;

    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (!error) {
        config.fl_x = doc["fl_x"] | 30;
        config.fl_y = doc["fl_y"] | 30;
        config.fr_x = doc["fr_x"] | 200;
        config.fr_y = doc["fr_y"] | 30;
        config.br_x = doc["br_x"] | 200;
        config.br_y = doc["br_y"] | 200;
        config.bl_x = doc["bl_x"] | 30;
        config.bl_y = doc["bl_y"] | 200;
    }
    return config;
}

void OctoConfigManager::saveConfig(const OctoConfigData& config) {
    File file = SD.open(OCTO_CONFIG_FILE, FILE_WRITE);
    if (!file) return;

    DynamicJsonDocument doc(1024);
    doc["fl_x"] = config.fl_x;
    doc["fl_y"] = config.fl_y;
    doc["fr_x"] = config.fr_x;
    doc["fr_y"] = config.fr_y;
    doc["br_x"] = config.br_x;
    doc["br_y"] = config.br_y;
    doc["bl_x"] = config.bl_x;
    doc["bl_y"] = config.bl_y;

    serializeJson(doc, file);
    file.close();
}

void OctoConfigManager::createDefaultConfig() {
    OctoConfigData defaultConfig;
    saveConfig(defaultConfig);
    Serial.println(LangManager::get("sys_octoconfig_created"));
}