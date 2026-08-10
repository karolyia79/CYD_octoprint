#ifndef MQTT_MONITOR_H
#define MQTT_MONITOR_H

#include <Arduino.h>
#include <functional>
#include <vector>

// Kulcsszó figyelő struktúra
struct KeywordWatcher {
    String keyword;
    std::function<void(const String& fullMessage)> callback;
};

class MqttMonitor {
public:
    MqttMonitor();

    // Globális bejövő üzenet feldolgozó (ezt hívja az MQTT callback)
    void processMessage(const String& topic, const String& payload);

    // 1. Funkció: Figyeljen egy adott stringre, és ha megjött, jelezzen vissza callback-kel
    void watchFor(const String& keyword, std::function<void(const String&)> callback);

    // 2. Funkció: Törölje az összes aktív figyelőt (pl. menü váltáskor)
    void clearWatchers();

    // 3. Funkció: Teljes forgalom streamelése (pl. terminál nézethez)
    void setStreamCallback(std::function<void(const String& topic, const String& message)> callback);

private:
    std::vector<KeywordWatcher> _watchers;
    std::function<void(const String&, const String&)> _streamCallback = nullptr;
};

#endif