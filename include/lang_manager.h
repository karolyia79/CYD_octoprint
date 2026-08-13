#ifndef LANG_MANAGER_H
#define LANG_MANAGER_H

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <map>

#define SD_CS_PIN 5
#define TFT_CS_PIN 15

class LangManager {
private:
    static std::map<String, String> _dictionary;
    static String _currentLang;

    // Hardveres SPI busz feloldása és SD kártyára állítása
    static void releaseAndPrepareSPI() {
        // 1. TFT CS hatástalanítása
        pinMode(TFT_CS_PIN, OUTPUT);
        digitalWrite(TFT_CS_PIN, HIGH);

        // 2. SD CS felengedése
        pinMode(SD_CS_PIN, OUTPUT);
        digitalWrite(SD_CS_PIN, HIGH);

        // 3. KRITIKUS: A TFT_eSPI által nyitva hagyott SPI tranzakció hardveres lezárása!
        SPI.endTransaction();

        // 4. Órajel visszaállítása az SD kártya stabil 4 MHz-es sebességére
        SPI.setFrequency(4000000);
        delay(5);
    }

public:
    static bool loadLanguage(const String& langCode) {
        releaseAndPrepareSPI();

        String code = langCode;
        code.trim();
        code.toLowerCase();
        if (code.length() == 0) code = "hu";

        Serial.printf("\n[LANG DIAG] ===> loadLanguage('%s') hívva\n", code.c_str());

        String path = "/" + code + ".lang";
        bool found = SD.exists(path);

        if (!found) {
            releaseAndPrepareSPI();
            path = "/" + code + ".LANG";
            found = SD.exists(path);
        }

        if (!found) {
            releaseAndPrepareSPI();
            String upperCode = code;
            upperCode.toUpperCase();
            path = "/" + upperCode + ".LANG";
            found = SD.exists(path);
        }

        if (!found) {
            Serial.printf("[LANG DIAG CRITICAL] HIBA: A(z) '%s' nyelvi fájl nem olvasható az SD kártyáról!\n", code.c_str());
            return false;
        }

        releaseAndPrepareSPI();
        File file = SD.open(path, FILE_READ);
        if (!file) {
            Serial.printf("[LANG DIAG ERROR] SD.open(%s) meghiúsult!\n", path.c_str());
            return false;
        }

        _dictionary.clear();
        _currentLang = code;
        int loadedKeys = 0;

        while (file.available()) {
            String line = file.readStringUntil('\n');
            line.trim();

            if (line.length() == 0 || line.startsWith("#")) continue;

            int separator = line.indexOf('=');
            if (separator > 0) {
                String key = line.substring(0, separator);
                String value = line.substring(separator + 1);
                key.trim();
                value.trim();
                _dictionary[key] = value;
                loadedKeys++;
            }
        }
        file.close();

        // Művelet végén lezárjuk az SD tranzakciót is
        digitalWrite(SD_CS_PIN, HIGH);
        SPI.endTransaction();

        Serial.printf("[LANG DIAG SIKER] %d kulcs sikeresen beolvasva a memóriába (%s)!\n\n", loadedKeys, path.c_str());
        return true;
    }

    static String get(const String& key) {
        if (_dictionary.find(key) != _dictionary.end()) {
            return _dictionary[key];
        }
        return key;
    }

    static String getCurrentLang() { return _currentLang; }
};

inline std::map<String, String> LangManager::_dictionary;
inline String LangManager::_currentLang = "hu";

#endif