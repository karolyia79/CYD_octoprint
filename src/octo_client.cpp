#include "octo_client.h"
#include "lang_manager.h"

OctoClient::OctoClient(const String& ip, const String& apiKey) : _ip(ip), _apiKey(apiKey) {}

void OctoClient::update() {
    if (WiFi.status() != WL_CONNECTED) {
        _data.status = "wifi_no_wifi"; 
        _data.connected = false;
        return;
    }

    // 1. Job adatok lekérdezése (/api/job)
    HTTPClient http;
    String url = "http://" + _ip + "/api/job?apikey=" + _apiKey;
    http.begin(url);
    http.addHeader("X-Api-Key", _apiKey);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setTimeout(4000); 

    int httpCode = http.GET();
    if (httpCode > 0 && httpCode == HTTP_CODE_OK) {
        DynamicJsonDocument doc(2048);
        deserializeJson(doc, http.getStream()); 

        JsonObject job = doc["job"];
        _data.name = job["file"]["display"].as<String>();
        if (_data.name == "null" || _data.name == "") {
            _data.name = job["file"]["name"].as<String>();
        }
        if (_data.name == "null" || _data.name == "") {
            _data.name = "octo_default_name";
        }

        float estimatedSec = job["estimatedPrintTime"] | 0.0f;
        if (estimatedSec > 0) {
            int totalSec = (int)estimatedSec;
            int hours = totalSec / 3600;
            int minutes = (totalSec % 3600) / 60;
            
            if (hours > 0) {
                _data.totalTime = String(hours) + "h " + String(minutes) + "m";
            } else {
                _data.totalTime = String(minutes) + "m";
            }
        } else {
            _data.totalTime = "";
        }

        JsonObject progress = doc["progress"];
        float rawCompletion = progress["completion"] | 0.0f;
        _data.progress = (int)(rawCompletion + 0.5f);

        int printTimeLeft = progress["printTimeLeft"] | -1;
        if (printTimeLeft >= 0) {
            int hours = printTimeLeft / 3600;
            int mins = (printTimeLeft % 3600) / 60;
            _data.remainingTime = String(hours) + "h " + String(mins) + "m";
        } else {
            _data.remainingTime = "octo_status_unknown";
        }
        _data.connected = true;
        
        String jobState = doc["state"].as<String>();
        if (jobState != "null" && jobState.length() > 0) {
            _data.status = jobState;
        } else {
            _data.status = "octo_status_working";
        }
    } else {
        _data.connected = false;
        _data.status = "wifi_conn_error";
        Serial.printf("[RX] HTTP GET /api/job Error Code: %d\n", httpCode);
    }
    http.end();

    // 2. Hőmérséklet & Státusz (/api/printer)
    HTTPClient httpPrn;
    String prnUrl = "http://" + _ip + "/api/printer?apikey=" + _apiKey;
    httpPrn.begin(prnUrl);
    httpPrn.addHeader("X-Api-Key", _apiKey);
    httpPrn.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    httpPrn.setTimeout(4000);
    
    int codePrn = httpPrn.GET();
    if (codePrn > 0 && codePrn == HTTP_CODE_OK) {
        DynamicJsonDocument pDoc(4096); 
        DeserializationError pErr = deserializeJson(pDoc, httpPrn.getStream()); 

        if (!pErr) {
            JsonObject temp = pDoc["temperature"];
            if (!temp.isNull()) {
                if (temp.containsKey("tool0")) {
                    _data.nozzleTemp = temp["tool0"]["actual"] | 0.0;
                    _data.nozzleTarget = temp["tool0"]["target"] | 0.0;
                }
                if (temp.containsKey("bed")) {
                    _data.bedTemp = temp["bed"]["actual"] | 0.0;
                    _data.bedTarget = temp["bed"]["target"] | 0.0;
                }
            }

            // --- RÉSZLETES RX HŐMÉRSÉKLET ÉS ÁLLAPOT LOGOLÁS ---
            Serial.printf("[RX] Printer State -> Nozzle: %.1fC (Target: %.1fC) | Bed: %.1fC (Target: %.1fC)\n", 
                _data.nozzleTemp, _data.nozzleTarget, _data.bedTemp, _data.bedTarget);

            JsonObject stateObj = pDoc["state"];
            if (!stateObj.isNull()) {
                String printerStateText = stateObj["text"].as<String>();
                if (printerStateText != "null" && printerStateText.length() > 0) {
                    _data.status = printerStateText;
                    Serial.println("[RX] Printer Status Text: " + printerStateText);
                }
            }

            // --- TERMINÁLOS / SERIAL MONITOROZÁS (RX) ---
            JsonArray logs = pDoc["logs"];
            if (!logs.isNull()) {
                for (JsonVariant v : logs) {
                    String line = v.as<String>();
                    handleIncomingLine(line);
                }
            }
        } else {
            Serial.println("[RX] JSON Parse Error in /api/printer");
        }
    } else {
        Serial.printf("[RX] HTTP GET /api/printer Error Code: %d\n", codePrn);
    }
    httpPrn.end();

    // --- IDŐALAPÚ AUTOHOME KEZELÉS ---
    if (_isHoming) {
        if (millis() - _homeTimer > 12000) { 
            Serial.println("[OctoClient] AutoHome időzítés letelt, kész.");
            _isHoming = false;
        }
    }

    // 3. MESH ÉPÍTÉS ÁLLAPOTGÉP
    if (_meshBuildState != 0) {
        uint32_t elapsed = millis() - _meshTimer;

        switch (_meshBuildState) {
            case 1: 
                if (!_isHoming || elapsed > 9500) {
                    Serial.println("[OctoClient] G28 kész. Send: M190 S60 (Fűtés indítása)");
                    setBedTarget(60);
                    sendGcodeCommand("M190 S60");
                    
                    _meshBuildState = 2; 
                    _meshTimer = millis();
                }
                break;

            case 2: 
                if (_data.bedTemp >= 58.0 || elapsed > 180000) {
                    Serial.println("[OctoClient] Tálca elérte a hőmérsékletet. Send: G29 (Mesh mérés)");
                    String meshCmd = "G29";
                    if (_meshBuildSize != 3 && supportsCustomMesh()) {
                        meshCmd = "G29 P" + String(_meshBuildSize);
                    }
                    sendGcodeCommand(meshCmd);
                    
                    _meshBuildState = 3; 
                    _meshTimer = millis();
                }
                break;

            case 3: { 
                uint32_t probingDuration = (_meshBuildSize >= 5) ? 150000 : 75000;
                if (elapsed > probingDuration) {
                    Serial.println("[OctoClient] Mesh kész. Send: M500 (EEPROM mentés)");
                    sendGcodeCommand("M500");
                    
                    _meshBuildState = 4; 
                    _meshTimer = millis();
                }
                break;
            }

            case 4: 
                if (elapsed > 2500) {
                    Serial.println("[OctoClient] M500 kész. Send: M140 S0 (Cooldown)");
                    sendGcodeCommand("M140 S0"); 
                    
                    _meshBuildState = 5; 
                    _meshTimer = millis();
                }
                break;

            case 5: 
                if (elapsed > 1000) {
                    Serial.println("[OctoClient SUCCESS] Mesh folyamat sikeresen lezárult!");
                    _data.bedTarget = 0;
                    _showMeshSavedPopup = true;
                    _popupStartMs = millis();
                    _meshBuildState = 0; 
                }
                break;
        }

        if (millis() - _meshTimer > 360000 && _meshBuildState != 0) {
            Serial.println("[OctoClient TIMEOUT] Mesh építés időtúllépés!");
            _meshBuildState = 0;
        }
    }

    if (_showMeshSavedPopup && (millis() - _popupStartMs > 4000)) {
        _showMeshSavedPopup = false;
    }
}

void OctoClient::handleIncomingLine(const String& line) {
    // Minden érkező sor kiírása a Serial Monitorra
    Serial.println("[RX] " + line);

    // PID Autotune befejezésének szűrése
    if (line.indexOf("PID Autotune finished!") >= 0) {
        _pidFinished = true;
        Serial.println("[OctoClient] PID kalibrálás sikeresen lefutott!");
    }

    // MPC Autotune befejezésének szűrése
    if ((line.indexOf("MPC") >= 0 || line.indexOf("Model Predictive Control") >= 0) && 
        (line.indexOf("finished") >= 0 || line.indexOf("completed") >= 0 || line.indexOf("ok") >= 0)) {
        _mpcFinished = true;
        Serial.println("[OctoClient] MPC kalibrálás sikeresen lefutott!");
    }
}

void OctoClient::sendPostRequest(const String& endpoint, const String& jsonPayload) {
    if (WiFi.status() != WL_CONNECTED) return;
    HTTPClient http;
    String url = "http://" + _ip + endpoint + "?apikey=" + _apiKey;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-Api-Key", _apiKey);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setTimeout(5000);
    int respCode = http.POST(jsonPayload);
    
    // RX válasz logolása a POST kérésre
    Serial.printf("[RX] POST %s -> HTTP Response Code: %d\n", endpoint.c_str(), respCode);
    http.end();
}

void OctoClient::sendGcodeCommand(const String& gcode) {
    Serial.printf("[TX] Gcode: %s\n", gcode.c_str());
    String payload = "{\"commands\":[\"" + gcode + "\"]}";
    sendPostRequest("/api/printer/command", payload);
}

void OctoClient::setNozzleTarget(float temp) {
    _data.nozzleTarget = temp;
    Serial.printf("[TX] Set Nozzle Target: %.1f C\n", temp);
    String payload = "{\"command\":\"target\",\"targets\":{\"tool0\":" + String((int)temp) + "}}";
    sendPostRequest("/api/printer/tool", payload);
}

void OctoClient::setBedTarget(float temp) {
    _data.bedTarget = temp;
    Serial.printf("[TX] Set Bed Target: %.1f C\n", temp);
    String payload = "{\"command\":\"target\":" + String((int)temp) + "}";
    sendPostRequest("/api/printer/bed", payload);
}

void OctoClient::setSpeed(int percent) {
    if (percent < 10) percent = 10;
    if (percent > 300) percent = 300;
    _data.speed = percent;
    Serial.printf("[TX] Set Speed: %d%%\n", percent);
    String payload = "{\"command\":\"feedrate\",\"factor\":" + String(percent) + "}";
    sendPostRequest("/api/printer/printhead", payload);
}

void OctoClient::adjustZOffset(float delta) {
    sendGcodeCommand("M290 Z" + String(delta, 3));
}

void OctoClient::autoHome() {
    Serial.println("[TX] Auto Home (G28)");
    sendGcodeCommand("G28");
    _isHoming = true;
    _homeTimer = millis();
}

void OctoClient::disableSteppers() {
    Serial.println("[TX] Steppers Off (M84)");
    sendGcodeCommand("M84");
}

void OctoClient::homeZ() {
    sendGcodeCommand("G28 Z");
}

void OctoClient::saveConfig() {
    sendGcodeCommand("M500");
}

void OctoClient::autoBuildMesh(int size) {
    _meshBuildSize = size;
    sendGcodeCommand("G28");
    _isHoming = true;       
    _homeTimer = millis();
    _meshBuildState = 1; 
    _meshTimer = millis();
    _showMeshSavedPopup = false;
}

void OctoClient::fetchBedMesh() {
    if (WiFi.status() != WL_CONNECTED) {
        _showNoMeshPopup = true;
        return;
    }

    String url = "http://" + _ip + "/api/settings?apikey=" + _apiKey;
    HTTPClient http;
    http.begin(url);
    http.addHeader("X-Api-Key", _apiKey);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setTimeout(10000);

    int httpCode = http.GET();
    Serial.printf("[RX] Fetch Bed Mesh HTTP Code: %d\n", httpCode);
    bool success = false;

    if (httpCode > 0 && httpCode == HTTP_CODE_OK) {
        DynamicJsonDocument doc(49152);
        DeserializationError error = deserializeJson(doc, http.getStream());

        if (!error) {
            JsonArray meshArray = doc["plugins"]["bedlevelvisualizer"]["stored_mesh"].as<JsonArray>();
            int rows = meshArray.size();
            if (rows > 0 && rows <= MAX_MESH_SIZE) {
                int cols = meshArray[0].as<JsonArray>().size();
                if (cols > 0 && cols <= MAX_MESH_SIZE) {
                    _data.meshRows = rows;
                    _data.meshCols = cols;

                    for (int r = 0; r < rows; r++) {
                        JsonArray rowArray = meshArray[r].as<JsonArray>();
                        for (int c = 0; c < cols; c++) {
                            _data.bedMesh[r][c] = rowArray[c].as<float>();
                        }
                    }
                    success = true;
                }
            }
        }
    }
    http.end();
    _showNoMeshPopup = !success;
}

void OctoClient::checkPluginAvailability() {
    if (WiFi.status() != WL_CONNECTED) return;

    String url = "http://" + _ip + "/api/plugins?apikey=" + _apiKey;
    HTTPClient http;
    http.begin(url);
    http.addHeader("X-Api-Key", _apiKey);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setTimeout(5000);

    int httpCode = http.GET();
    Serial.printf("[RX] Plugin Check HTTP Code: %d\n", httpCode);

    if (httpCode > 0 && httpCode == HTTP_CODE_OK) {
        DynamicJsonDocument doc(16384);
        DeserializationError error = deserializeJson(doc, http.getStream());

        if (!error) {
            // Megkeressük, hogy a plugin benne van-e az engedélyezett/telepített pluginok között
            // A plugin azonosítója általában "octoklipscreenbridge" vagy a kulcs a JSON-ban
            bool found = false;
            JsonObject root = doc.as<JsonObject>();
            
            for (JsonPair kv : root) {
                String pluginKey = String(kv.key().c_str());
                pluginKey.toLowerCase();
                if (pluginKey.indexOf("klipscreen") >= 0 || pluginKey.indexOf("octoklipscreenbridge") >= 0) {
                    found = true;
                    break;
                }
            }

            if (found) {
                _pluginMissing = false;
                Serial.println("[OctoClient] Az OctoKlipscreenBridge plugin sikeresen detektálva a szerveren!");
            } else {
                _pluginMissing = true;
                Serial.println("==========================================================");
                Serial.println("[HIBA] Az OctoKlipscreenBridge plugin NEM TALÁLHATÓ az OctoPrint szerveren!");
                Serial.println("Kérlek telepítsd innen: https://github.com/karolyia79/OctoklipscreenBridge");
                Serial.println("==========================================================");
            }
        }
    } else {
        Serial.println("[OctoClient Warning] Nem sikerült lekérni a plugin listát az OctoPrint API-tól.");
    }
    http.end();
}