#include "mqtt_monitor.h"

MqttMonitor::MqttMonitor() {}

void MqttMonitor::processMessage(const String& topic, const String& message) {
    // 1. Ha van beállítva élő stream callback, küldjük a forgalmat soronként
    if (_streamCallback) {
        _streamCallback(topic, message);
    }

    // 2. Ellenőrizzük az összes regisztrált figyelőt, hogy benne van-e a kulcsszó
    for (auto& watcher : _watchers) {
        if (message.indexOf(watcher.keyword) >= 0) {
            if (watcher.callback) {
                watcher.callback(message);
            }
        }
    }
}

void MqttMonitor::watchFor(const String& keyword, std::function<void(const String&)> callback) {
    _watchers.push_back({keyword, callback});
}

void MqttMonitor::clearWatchers() {
    _watchers.clear();
}

void MqttMonitor::setStreamCallback(std::function<void(const String&, const String&)> callback) {
    _streamCallback = callback;
}