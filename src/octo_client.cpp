#include "octo_client.h"
#include "lang_manager.h"

OctoClient::OctoClient(const String& ip, const String& apiKey) 
    : _ip(ip), _apiKey(apiKey) {}

void OctoClient::setHoming(bool h) {
    _isHoming = h;
    if (h) _homeTimer = millis();
    Serial.printf("[%lu ms] [OCTO_BASE] setHoming(%s) hívva, _homeTimer frissítve.\n", millis(), h ? "TRUE" : "FALSE");
}

void OctoClient::update() {
    if (WiFi.status() != WL_CONNECTED) {
        _data.connected = false;
        return;
    }

    HTTPClient http;
    String url = "http://" + _ip + "/api/printer?apikey=" + _apiKey;
    http.begin(url);
    http.setTimeout(3000);

    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        _data.connected = true;
        DynamicJsonDocument doc(4096);
        deserializeJson(doc, http.getStream());

        JsonObject temperature = doc["temperature"];
        _data.nozzleTemp = temperature["tool0"]["actual"] | 0.0f;
        _data.nozzleTarget = temperature["tool0"]["target"] | 0.0f;
        _data.bedTemp = temperature["bed"]["actual"] | 0.0f;
        _data.bedTarget = temperature["bed"]["target"] | 0.0f;

        String stateFlags = doc["state"]["text"] | "Operational";
        _data.status = stateFlags;

        Serial.printf("[%lu ms] [OCTO_BASE HTTP] GET /api/printer OK | Status: %s | Temp: N=%.1f/%.1f B=%.1f/%.1f\n",
                      millis(), _data.status.c_str(), _data.nozzleTemp, _data.nozzleTarget, _data.bedTemp, _data.bedTarget);
    } else {
        Serial.printf("[%lu ms] [OCTO_BASE HTTP] GET /api/printer HIBA! Kod: %d\n", millis(), httpCode);
        _data.connected = false;
    }
    http.end();

    // --- HOMING TIMEOUT KEZELÉS ---
    // HA MQTT AKTÍV: Az MQTT watcher felel a homing lezárásáért!
    // A 12 másodperces vak időzítő CSAK sima HTTP (REST API) módban futhat!
    if (_isHoming) {
        if (!_data.mqttActive) {
            unsigned long homingDuration = millis() - _homeTimer;
            Serial.printf("[%lu ms] [OCTO_BASE MONITOR (HTTP MÓD)] _isHoming aktív, eltelt idő: %lu ms\n", millis(), homingDuration);
            if (homingDuration > 12000) { 
                Serial.printf("[%lu ms] [OCTO_BASE TIMEOUT] _isHoming KIKAPCSOLVA 12s Időtúllépés miatt!\n", millis());
                _isHoming = false;
            }
        } else {
            Serial.printf("[%lu ms] [OCTO_BASE MONITOR (MQTT MÓD)] _isHoming aktív, MQTT watcher vezérli (HTTP Timeout letiltva).\n", millis());
        }
    }

    // --- MESH BUILD & FŰTÉS FÁZISKEZELÉS (HTTP Fallback) ---
    if (_meshBuildState != 0) {
        Serial.printf("[%lu ms] [OCTO_BASE MESH] Fázis: %d, Eltelt idő: %lu ms\n", millis(), _meshPhase, millis() - _meshTimer);
        if (_meshPhase == 1) {
            // 1. Fázis: Várjuk, hogy a G28 lemenjen (időzítve HTTP módban)
            if (!_data.mqttActive && (millis() - _meshTimer > 8000)) {
                _meshPhase = 2; // 2. Fázis: Fűtés indítása 60 fokra
                Serial.printf("[%lu ms] %s\n", millis(), LangManager::get("octo_log_g28_done_heating").c_str());
                setBedTarget(60);
                _meshTimer = millis();
            }
        } else if (_meshPhase == 2) {
            // 2. Fázis: Várjuk, hogy a bed elérje a 60 fokot
            if (_data.bedTemp >= 59.0f || (_data.bedTarget > 0 && _data.bedTemp >= _data.bedTarget - 1.0f)) {
                _meshPhase = 3; // 3. Fázis: G29 indítása
                Serial.printf("[%lu ms] %s\n", millis(), LangManager::get("octo_log_bed_60_g29_start").c_str());
                int size = _meshBuildState;
                if (size == 5) sendGcodeCommand("G29 P1");
                else sendGcodeCommand("G29");
                _meshTimer = millis();
            }
        } else if (_meshPhase == 3) {
            // 3. Fázis: G29 mérés várakozás
            if (!_data.mqttActive && (millis() - _meshTimer > 30000)) {
                _meshPhase = 4; // 4. Fázis: M500 mentés
                Serial.printf("[%lu ms] %s\n", millis(), LangManager::get("octo_log_g29_done_m500").c_str());
                sendGcodeCommand("M500");
                _meshTimer = millis();
            }
        } else if (_meshPhase == 4) {
            // 4. Fázis: M500 mentés után Cooldown és gombok feloldása
            if (!_data.mqttActive && (millis() - _meshTimer > 2000)) {
                _meshPhase = 0;
                _meshBuildState = 0; // Gombok visszaváltanak
                Serial.printf("[%lu ms] %s\n", millis(), LangManager::get("octo_log_save_done_cooldown").c_str());
                setBedTarget(0);
            }
        }
    }
}

void OctoClient::checkPluginAvailability() {
    if (WiFi.status() != WL_CONNECTED) return;

    String url = "http://" + _ip + "/api/settings?apikey=" + _apiKey;
    HTTPClient http;
    http.begin(url);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setTimeout(5000);

    int httpCode = http.GET();
    if (httpCode > 0 && httpCode == HTTP_CODE_OK) {
        DynamicJsonDocument doc(16384);
        DeserializationError error = deserializeJson(doc, http.getStream());

        if (!error) {
            bool found = false;
            JsonObject pluginsObj = doc["plugins"].as<JsonObject>();
            for (JsonPair kv : pluginsObj) {
                String pluginKey = String(kv.key().c_str());
                pluginKey.toLowerCase();
                if (pluginKey.indexOf("klipscreen") >= 0 || pluginKey.indexOf("octoklipscreenbridge") >= 0) {
                    found = true;
                    break;
                }
            }
            _pluginMissing = !found;
            Serial.printf("[%lu ms] [OCTO_BASE] Plugin ellenőrzés lefutott! Hiányzik: %s\n", millis(), _pluginMissing ? "IGEN" : "NEM");
        }
    }
    http.end();
}

void OctoClient::sendGcodeCommand(const String& gcode) {
    if (WiFi.status() != WL_CONNECTED) return;

    HTTPClient http;
    String url = "http://" + _ip + "/api/printer/command?apikey=" + _apiKey;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    DynamicJsonDocument doc(256);
    JsonArray commands = doc.createNestedArray("commands");
    commands.add(gcode);

    String jsonPayload;
    serializeJson(doc, jsonPayload);

    Serial.printf("[%lu ms] [OCTO_BASE HTTP TX] Parancs küldése: %s\n", millis(), gcode.c_str());
    http.POST(jsonPayload);
    http.end();
}

void OctoClient::autoHome() {
    Serial.printf("[%lu ms] [OCTO_BASE] autoHome() hívva -> setHoming(true)\n", millis());
    setHoming(true);
    sendGcodeCommand("G28");
}

void OctoClient::disableSteppers() {
    sendGcodeCommand("M84");
}

void OctoClient::homeZ() {
    Serial.printf("[%lu ms] [OCTO_BASE] homeZ() hívva -> setHoming(true)\n", millis());
    setHoming(true);
    sendGcodeCommand("G28 Z");
}

void OctoClient::saveConfig() {
    sendGcodeCommand("M500");
}

void OctoClient::setNozzleTarget(float temp) {
    sendGcodeCommand("M104 S" + String((int)temp));
}

void OctoClient::setBedTarget(float temp) {
    sendGcodeCommand("M140 S" + String((int)temp));
}

void OctoClient::setSpeed(int percent) {
    sendGcodeCommand("M220 S" + String(percent));
}

void OctoClient::adjustZOffset(float delta) {
    sendGcodeCommand("M290 Z" + String(delta, 3));
}

void OctoClient::autoBuildMesh(int size) {
    _meshBuildState = size;
    _meshPhase = 1; // 1. Fázis: Homing (G28) indítása
    _meshTimer = millis();
    Serial.printf("[%lu ms] [OctoClient] Bed Mesh indítva: G28 Homing...\n", millis());
    sendGcodeCommand("G28");
}

void OctoClient::fetchBedMesh() {
    if (WiFi.status() != WL_CONNECTED) return;
    _data.meshLoaded = false;

    HTTPClient http;
    String url = "http://" + _ip + "/api/settings?apikey=" + _apiKey;
    http.begin(url);
    http.setTimeout(5000);

    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        DynamicJsonDocument doc(16384);
        DeserializationError error = deserializeJson(doc, http.getStream());

        if (!error) {
            JsonObject plugins = doc["plugins"].as<JsonObject>();
            JsonArray rows;

            if (plugins.containsKey("octoklipscreenbridge") && plugins["octoklipscreenbridge"].containsKey("mesh")) {
                rows = plugins["octoklipscreenbridge"]["mesh"].as<JsonArray>();
            } else if (plugins.containsKey("klipscreen") && plugins["klipscreen"].containsKey("mesh")) {
                rows = plugins["klipscreen"]["mesh"].as<JsonArray>();
            }

            if (!rows.isNull() && rows.size() > 0) {
                _data.meshRows = rows.size();
                _data.meshCols = rows[0].as<JsonArray>().size();

                for (int r = 0; r < _data.meshRows && r < 7; r++) {
                    JsonArray col = rows[r].as<JsonArray>();
                    for (int c = 0; c < _data.meshCols && c < 7; c++) {
                        _data.bedMesh[r][c] = col[c] | 0.0f;
                    }
                }
                _data.meshLoaded = true;
                _showNoMeshPopup = false;
                Serial.printf("[%lu ms] %s\n", millis(), LangManager::get("octo_log_mesh_download_ok").c_str());
            } else {
                _data.meshLoaded = true;
                _showNoMeshPopup = true;
                Serial.printf("[%lu ms] %s\n", millis(), LangManager::get("octo_log_mesh_download_empty").c_str());
            }
        } else {
            _data.meshLoaded = true;
            _showNoMeshPopup = true;
        }
    } else {
        _data.meshLoaded = true;
        _showNoMeshPopup = true;
    }
    http.end();
}