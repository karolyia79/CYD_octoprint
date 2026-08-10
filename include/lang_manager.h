#ifndef LANG_MANAGER_H
#define LANG_MANAGER_H

#include <Arduino.h>
#include <SD.h>
#include <map>
#include <vector>

class LangManager {
private:
    static std::map<String, String> _dictionary;
    static String _currentLang;

public:
    static bool loadLanguage(const String& langCode) {
        String path = "/" + langCode + ".lang";
        if (!SD.exists(path)) {
            path = "/en.lang"; // Fallback angolra
            if (!SD.exists(path)) return false;
        }

        File file = SD.open(path, FILE_READ);
        if (!file) return false;

        _dictionary.clear();
        _currentLang = langCode;

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
            }
        }
        file.close();
        Serial.printf("[LANG] Betoltott nyelv: %s (%d kulcs)\n", langCode.c_str(), _dictionary.size());
        return true;
    }

    static String get(const String& key) {
        if (_dictionary.find(key) != _dictionary.end()) {
            return _dictionary[key];
        }
        return key; // Ha hiányzik a kulcs, magát a kulcsot adja vissza
    }

    static String getCurrentLang() {
        return _currentLang;
    }

    static std::vector<String> getAvailableLanguages() {
        std::vector<String> languages;
        File root = SD.open("/");
        if (!root || !root.isDirectory()) return languages;

        File file = root.openNextFile();
        while (file) {
            if (!file.isDirectory()) {
                String fileName = String(file.name());
                if (fileName.endsWith(".lang")) {
                    int dotIndex = fileName.lastIndexOf('.');
                    int slashIndex = fileName.lastIndexOf('/');
                    String langCode = fileName.substring(slashIndex + 1, dotIndex);
                    languages.push_back(langCode);
                }
            }
            file = root.openNextFile();
        }
        return languages;
    }
};

// Statikus tagok definíciója
inline std::map<String, String> LangManager::_dictionary;
inline String LangManager::_currentLang = "hu";

#endif