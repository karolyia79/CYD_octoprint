#include "logger.h"
#include <SD.h>
#include "lang_manager.h"

void Logger::init() {
    // Ha még nem létezik a logfájl, létrehozzuk egy induló bejegyzéssel
    if (!SD.exists("/octoscreen.log")) {
        File file = SD.open("/octoscreen.log", FILE_WRITE);
        if (file) {
            file.println(String("[SYS] ") + LangManager::get("logger_file_created"));
            file.close();
        }
    }
}

void Logger::logMessage(const String& message) {
    File logFile = SD.open("/octoscreen.log", FILE_APPEND);
    if (logFile) {
        logFile.println(message);
        logFile.close();
    }
}

void Logger::logSystem(const String& msg) {
    String formatted = "[SYS] " + msg;
    Serial.println(formatted);
    logMessage(formatted);
}

void Logger::logError(const String& msg) {
    String formatted = "[HIBA] " + msg;
    Serial.println(formatted);
    logMessage(formatted);
}