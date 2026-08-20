#include "config_manager.h"
#include "lang_manager.h"
#include "logger.h"
#include <SPI.h>
#include <SD.h>

#define SD_CS 5
#define TFT_CS 15

// RAM Cache a villámgyors működéshez
static PrinterConfig g_cachedConfig;
static bool g_configLoaded = false;

bool ConfigManager::init() {
    Serial.println("\n==================================================");
    Serial.println("[SD DIAGNOSTICS] SD kartya es SPI busz inditasa...");
    Serial.println("==================================================");
    
    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH); // TFT CS letiltva

    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH); // SD CS felengedve

    delay(50); // Stabilizációs szünet az SD kártyának

    // KRITIKUS: A kijelző (TFT_eSPI) bootoláskor nyitva hagyhatta a buszt. Itt kényszerítjük a lezárást!
    SPI.endTransaction();

    // Explicit SPI busz indítás (4MHz a gyors és stabil beolvasásért)
    SPI.begin(18, 19, 23, SD_CS);

    if (!SD.begin(SD_CS, SPI, 4000000)) {
        Serial.println("[SD CRITICAL ERROR] Az SD kartya nem inicializalhato!");
        Logger::logError(LangManager::get("config_sd_failed"));
        return false;
    }

    Serial.println("[SD SIKER] SD kartya csatlakoztatva!");

    // Konfiguráció betöltése a RAM-ba kényszerített újraolvasással
    PrinterConfig cfg = loadConfig(true);
    LangManager::loadLanguage(cfg.language);

    return true;
}

void ConfigManager::createDefaultConfig() {
    PrinterConfig defaultCfg;
    saveConfig(defaultCfg);
}

PrinterConfig ConfigManager::loadConfig(bool forceReload) {
    if (g_configLoaded && !forceReload) {
        return g_cachedConfig;
    }

    PrinterConfig config;
    
    // SPI busz beállítása az SD tranzakcióhoz
    digitalWrite(TFT_CS, HIGH);
    digitalWrite(SD_CS, HIGH);
    SPI.endTransaction(); // Zároldás biztosítása
    SPI.setFrequency(4000000);

    String configPath = "";
    if (SD.exists("/config.json")) {
        configPath = "/config.json";
    } else if (SD.exists("/CONFIG.JSON")) {
        configPath = "/CONFIG.JSON";
    }

    if (configPath == "") {
        Serial.println("[CONFIG ERROR] A /config.json fajl NEM TALALHATO!");
        return config;
    }

    File file = SD.open(configPath, FILE_READ);
    if (!file) {
        Serial.println("[CONFIG ERROR] Nem sikerult megnyitni a config.json-t!");
        return config;
    }

    String rawJson = file.readString();
    file.close();

    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, rawJson);

    if (error) {
        Serial.println("[CONFIG PARSE ERROR] JSON értelmezési hiba: " + String(error.c_str()));
        return config;
    }

    if (doc.containsKey("wifi_ssid")) config.wifi_ssid = doc["wifi_ssid"].as<String>();
    if (doc.containsKey("wifi_pass")) config.wifi_pass = doc["wifi_pass"].as<String>();
    if (doc.containsKey("use_static_ip")) config.use_static_ip = doc["use_static_ip"].as<bool>();
    if (doc.containsKey("static_ip")) config.static_ip = doc["static_ip"].as<String>();
    if (doc.containsKey("gateway")) config.gateway = doc["gateway"].as<String>();
    if (doc.containsKey("subnet")) config.subnet = doc["subnet"].as<String>();
    if (doc.containsKey("dns")) config.dns = doc["dns"].as<String>();

    if (doc.containsKey("octo_enabled")) config.octo_enabled = doc["octo_enabled"].as<bool>();
    if (doc.containsKey("octo_ip")) config.octo_ip = doc["octo_ip"].as<String>();
    if (doc.containsKey("octo_key")) config.octo_key = doc["octo_key"].as<String>();

    if (doc.containsKey("led_enabled")) config.led_enabled = doc["led_enabled"].as<bool>();
    
    // Képernyő mód betöltése
    if (doc.containsKey("screen_mode")) {
        config.screen_mode = doc["screen_mode"].as<String>();
    } else if (doc.containsKey("screen_sleep")) {
        config.screen_mode = doc["screen_sleep"].as<bool>() ? "sleep" : "off";
    }

    if (doc.containsKey("screen_timeout")) config.screen_timeout = doc["screen_timeout"].as<int>();
    if (doc.containsKey("screen_brightness")) config.screen_brightness = doc["screen_brightness"].as<int>();

    if (doc.containsKey("klipper_enabled")) config.klipper_enabled = doc["klipper_enabled"].as<bool>();
    if (doc.containsKey("klipper_ip")) config.klipper_ip = doc["klipper_ip"].as<String>();
    if (doc.containsKey("klipper_port")) config.klipper_port = doc["klipper_port"].as<int>();
    if (doc.containsKey("klipper_key")) config.klipper_key = doc["klipper_key"].as<String>();

    if (doc.containsKey("skin")) config.skin = doc["skin"].as<String>();
    if (config.skin.length() == 0) config.skin = "dark";

    if (doc.containsKey("language")) {
        String langVal = doc["language"].as<String>();
        langVal.trim();
        if (langVal.length() > 0) {
            config.language = langVal;
        }
    }

    Serial.println("[CONFIG CACHED] Sikeresen beolvasva es eltárolva a RAM-ban!");
    Serial.println("  > WiFi SSID: [" + config.wifi_ssid + "]");
    Serial.println("  > Nyelv: [" + config.language + "]");
    Serial.println("  > OctoPrint: " + String(config.octo_enabled ? "ENGEDELYEZVE" : "KIKAPCSOLVA"));
    Serial.println("  > Screen mode: [" + config.screen_mode + "]");
    Serial.println("==================================================\n");

    g_cachedConfig = config;
    g_configLoaded = true;

    return g_cachedConfig;
}

bool ConfigManager::saveConfig(const PrinterConfig& config) {
    Serial.println("\n[CONFIG SAVE] Mentés az SD kártyára...");
    
    digitalWrite(TFT_CS, HIGH);
    digitalWrite(SD_CS, HIGH);
    SPI.endTransaction(); // Zároldás biztosítása
    SPI.setFrequency(4000000);

    if (SD.exists("/config.json")) {
        SD.remove("/config.json");
    }

    File file = SD.open("/config.json", FILE_WRITE);
    if (!file) {
        Serial.println("[CONFIG SAVE ERROR] Nem sikerult megnyitni a fajlt irasra!");
        return false;
    }

    DynamicJsonDocument doc(2048);
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

    doc["led_enabled"] = config.led_enabled;
    doc["screen_mode"] = config.screen_mode;
    doc["screen_timeout"] = config.screen_timeout;
    doc["screen_brightness"] = config.screen_brightness;

    doc["skin"] = config.skin;
    doc["language"] = config.language;

    if (serializeJson(doc, file) == 0) {
        Serial.println("[CONFIG SAVE ERROR] Nem sikerult a JSON sorositasa!");
        file.close();
        return false;
    }

    file.close();

    g_cachedConfig = config;
    g_configLoaded = true;

    Serial.println("[CONFIG SAVE SIKER] SD es RAM frissitve! Nyelv: " + config.language);
    return true;
}