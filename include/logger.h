#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

class Logger {
public:
    static void init();
    static void logMessage(const String& message);
    static void logSystem(const String& msg);
    static void logError(const String& msg);
};

#endif