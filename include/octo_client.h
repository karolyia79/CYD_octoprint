#ifndef OCTO_CLIENT_H
#define OCTO_CLIENT_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define MAX_MESH_SIZE 10

struct OctoPrinterData {
    String name = "OctoPrint";
    String status = "Kapcsolódás...";
    float nozzleTemp = 0;
    float nozzleTarget = 0;
    float bedTemp = 0;
    float bedTarget = 0;
    int speed = 100;
    int progress = 0;
    String remainingTime = "-";
    String totalTime;
    bool connected = false;

    int meshRows = 3;
    int meshCols = 3;
    float bedMesh[MAX_MESH_SIZE][MAX_MESH_SIZE] = {0};
};

class OctoClient {
public:
    OctoClient(const String& ip, const String& apiKey);
    void update();
    const OctoPrinterData& getData() const { return _data; }

    void sendGcodeCommand(const String& gcode);
    void setNozzleTarget(float temp);
    void setBedTarget(float temp);
    void setSpeed(int percent);
    void adjustZOffset(float delta);

    bool isPluginMissing() const { return _pluginMissing; }    void checkPluginAvailability();

    void autoHome();
    void disableSteppers(); 
    void homeZ();
    void saveConfig();
    bool supportsCustomMesh() const { return false; }
    void autoBuildMesh(int size = 3);

    void fetchBedMesh();

    bool isMeshBuilding() const { return _meshBuildState != 0; }
    bool isHoming() const { return _isHoming; }

    // PID és MPC kész állapot ellenőrzése
    bool isPidFinished() const { return _pidFinished; }
    bool isMpcFinished() const { return _mpcFinished; }
    void resetCalibrationFlags() { _pidFinished = false; _mpcFinished = false; }

    bool shouldShowMeshSavedPopup() const { return _showMeshSavedPopup; }
    void dismissMeshSavedPopup() { _showMeshSavedPopup = false; }

    bool shouldShowNoMeshPopup() const { return _showNoMeshPopup; }
    void dismissNoMeshPopup() { _showNoMeshPopup = false; }

private:
    String _ip;
    String _apiKey;
    OctoPrinterData _data;

    int _meshBuildState = 0; 
    int _meshBuildSize = 3;
    
    bool _isHoming = false;
    uint32_t _homeTimer = 0;

    bool _pidFinished = false;
    bool _mpcFinished = false;

    bool _showMeshSavedPopup = false;
    bool _showNoMeshPopup = false;
    uint32_t _meshTimer = 0;
    uint32_t _popupStartMs = 0;

    void sendPostRequest(const String& endpoint, const String& jsonPayload);
    void handleIncomingLine(const String& line);
    bool _pluginMissing = false;
};

#endif