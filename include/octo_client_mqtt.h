#ifndef OCTO_CLIENT_MQTT_H
#define OCTO_CLIENT_MQTT_H

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "mqtt_monitor.h"

#define MAX_MESH_SIZE 10

struct OctoPrinterData {
    String name = "OctoPrint";
    String status = "Kapcsolódás...";
    float nozzleTemp = 0;
    float nozzleTarget = 0;
    float bedTemp = 0;
    float bedTarget = 0;
    int speed = 100;
    bool mqttActive = false;
    int progress = 0;
    String remainingTime = "-";
    String totalTime;
    bool connected = false;

    int meshRows = 3;
    int meshCols = 3;
    float bedMesh[MAX_MESH_SIZE][MAX_MESH_SIZE] = {0};
    bool meshLoaded = false;
};

class OctoClientMqtt {
public:
    OctoClientMqtt(PubSubClient* mqttClient, MqttMonitor* monitor);

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

    // Filament kezelő parancsok
    void loadFilament(bool isBowden);
    void unloadFilament(bool isBowden);
    void extrudeFilament(float lengthMm, float speedMmMin = 300.0f);

    // Z-Offset pozicionálás (G28 -> G1 Z0 F300 -> OK várakozás)
    void startZOffsetPrep();
    bool isZOffsetPrepRunning() const { return _zOffsetPrepRunning; }
    bool isZOffsetReady() const { return _zOffsetReady; }
    void resetZOffsetPrep() { _zOffsetPrepRunning = false; _zOffsetReady = false; _zOffsetPrepPhase = 0; }

    // PID és MPC vezérlés
    void startPidAutotune();
    void startMpcAutotune();
    void stopCalibration();
    bool isPidDone() const { return _pidDone; }
    void clearPidDone() { _pidDone = false; }

    // Futási állapotok lekérdezése
    bool isPidRunning() const { return _pidRunning; }
    bool isMpcRunning() const { return _mpcRunning; }

    // Ismeretlen parancs hiba kezelés
    bool hasUnknownCommandError() const { return _unknownCommandError; }
    void clearUnknownCommandError() { _unknownCommandError = false; }

    // Állapotlekérdezések
    const OctoPrinterData& getData() const { return _data; }
    OctoPrinterData& getData() { return _data; }

    bool isHoming() const { return _isHoming; }
    void setHoming(bool h) { _isHoming = h; if (h) _homeTimer = millis(); }

    bool isMeshBuilding() const { return _meshBuildState != 0; }
    void setMeshBuildState(int state) { _meshBuildState = state; }

    int getMeshPhase() const { return _meshPhase; }
    void setMeshPhase(int phase) { _meshPhase = phase; }

    bool isPluginMissing() const { return false; }
    bool supportsCustomMesh() const { return false; }

    // Popup állapotok kezelése
    bool shouldShowMeshSavedPopup() const { return _showMeshSavedPopup; }
    void dismissMeshSavedPopup() { _showMeshSavedPopup = false; }
    void setShowMeshSavedPopup(bool show) { _showMeshSavedPopup = show; }

    bool shouldShowNoMeshPopup() const { return _showNoMeshPopup; }
    void dismissNoMeshPopup() { _showNoMeshPopup = false; }
    void setShowNoMeshPopup(bool show) { _showNoMeshPopup = show; }

    void fetchBedMesh();

private:
    PubSubClient* _mqtt;
    MqttMonitor* _monitor;
    OctoPrinterData _data;

    bool _mqttActive = false;
    bool _isHoming = false;
    uint32_t _homeTimer = 0;
    int _meshBuildState = 0;
    int _meshPhase = 0;

    bool _busySeen = false;
    bool _pendingClearWatchers = false;
    bool _seenAxisReport = false;
    bool _unknownCommandError = false;
    unsigned long _cmdStartTime = 0;
    unsigned long _lastBusyTime = 0;

    // Belső popup állapotok
    bool _showMeshSavedPopup = false;
    bool _showNoMeshPopup = false;

    // Z-Offset előkészítési állapotok
    bool _zOffsetPrepRunning = false;
    bool _zOffsetReady = false;
    int _zOffsetPrepPhase = 0;

    // PID és MPC belső állapotok
    bool _pidRunning = false;
    bool _mpcRunning = false;
    bool _pidDone = false;
    int _pidPhase = 0;
    float _pidKp = 0, _pidKi = 0, _pidKd = 0;

    String _brokerIp;
    int _port;
    unsigned long _lastReconnectAttempt = 0;

    void parseSerialMessage(const String& msg);
    void parseJsonMessage(const String& topic, const String& payload);
};

#endif