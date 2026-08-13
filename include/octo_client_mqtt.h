#ifndef OCTO_CLIENT_MQTT_H
#define OCTO_CLIENT_MQTT_H

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "octo_client.h"
#include "mqtt_monitor.h"

class OctoClientMqtt {
public:
    OctoClientMqtt(OctoClient* baseClient, PubSubClient* mqttClient, MqttMonitor* monitor);

    bool begin(const String& brokerIp, int port = 1883);
    void update();
    bool isConnected();

    void sendGcodeCommand(const String& gcode);

    // Parancsok
    void autoHome();
    void disableSteppers();
    void homeZ();
    void saveConfig();
    void autoBuildMesh(int size);

    void setNozzleTarget(float temp);
    void setBedTarget(float temp);
    void setSpeed(int percent);
    void adjustZOffset(float delta);

    // Átirányított állapotlekérdezések
    OctoClient* getBaseClient() const { return _base; }
    const OctoPrinterData& getData() const { return _base->getData(); }
    OctoPrinterData& getData() { return _base->getData(); }

    bool isHoming() const { return _base->isHoming(); }
    bool isMeshBuilding() const { return _base->isMeshBuilding(); }
    bool isPluginMissing() const { return _base->isPluginMissing(); }
    bool supportsCustomMesh() const { return _base->supportsCustomMesh(); }

    bool shouldShowMeshSavedPopup() const { return _base->shouldShowMeshSavedPopup(); }
    void dismissMeshSavedPopup() { _base->dismissMeshSavedPopup(); }

    bool shouldShowNoMeshPopup() const { return _base->shouldShowNoMeshPopup(); }
    void dismissNoMeshPopup() { _base->dismissNoMeshPopup(); }

    void fetchBedMesh();

private:
    OctoClient* _base;
    PubSubClient* _mqtt;
    MqttMonitor* _monitor;
    
    bool _mqttActive = false;
    bool _busySeen = false;
    bool _pendingClearWatchers = false;
    bool _seenAxisReport = false;
    unsigned long _cmdStartTime = 0;
    unsigned long _lastBusyTime = 0;
    
    String _brokerIp;
    int _port;
    unsigned long _lastReconnectAttempt = 0;

    void parseSerialMessage(const String& msg);
    void parseJsonMessage(const String& topic, const String& payload);
};

#endif