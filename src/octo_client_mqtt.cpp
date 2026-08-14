#include "octo_client_mqtt.h"
#include "lang_manager.h"
#include <ArduinoJson.h>
#include <Preferences.h>

OctoClientMqtt::OctoClientMqtt(OctoClient* baseClient, PubSubClient* mqttClient, MqttMonitor* monitor) 
    : _base(baseClient), _mqtt(mqttClient), _monitor(monitor) {}

bool OctoClientMqtt::begin(const String& brokerIp, int port) {
    _brokerIp = brokerIp;
    _port = port;
    
    if (_brokerIp.length() == 0) return false;

    _mqtt->setServer(_brokerIp.c_str(), _port);
    _mqtt->setBufferSize(2048);
    _mqtt->setKeepAlive(15);

    _mqtt->setCallback([this](char* topic, byte* payload, unsigned int length) {
        String msg = "";
        for (unsigned int i = 0; i < length; i++) {
            msg += (char)payload[i];
        }
        
        String tStr = String(topic);
        
        if (tStr.indexOf("serial") != -1) {
            this->parseSerialMessage(msg);
        } else if (tStr.indexOf("temperature") != -1 || tStr.indexOf("state") != -1 || tStr.indexOf("progress") != -1) {
            this->parseJsonMessage(tStr, msg);
        }

        Serial.printf("[%lu ms] [MQTT INCOMING] Topic: %s | Msg: %s\n", millis(), topic, msg.c_str());
        if (this->_monitor) {
            this->_monitor->processMessage(tStr, msg);
        }
    });

    String clientId = "OctoKlip_" + String((uint32_t)ESP.getEfuseMac(), HEX);

    Serial.println(LangManager::get("mqtt_conn_to_broker") + _brokerIp + ":" + String(_port));
    if (_mqtt->connect(clientId.c_str())) {
        _mqttActive = true;
        _mqtt->subscribe("octoprint/serial/#");
        _mqtt->subscribe("octoprint/temperature/#");
        _mqtt->subscribe("octoprint/printer/state");
        _mqtt->subscribe("octoprint/progress/print");
        Serial.println(LangManager::get("mqtt_success_active"));
    } else {
        _mqttActive = false;
        Serial.printf("%s %d\n", LangManager::get("mqtt_err_failed_code").c_str(), _mqtt->state());
    }
    
    _base->setMqttActive(_mqttActive);
    return _mqttActive;
}

void OctoClientMqtt::parseJsonMessage(const String& topic, const String& payload) {
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, payload);
    if (error) return;

    if (topic.indexOf("temperature/tool0") != -1) {
        if (doc.containsKey("actual")) _base->getData().nozzleTemp = doc["actual"];
        if (doc.containsKey("target")) _base->getData().nozzleTarget = doc["target"];
    } else if (topic.indexOf("temperature/bed") != -1) {
        if (doc.containsKey("actual")) _base->getData().bedTemp = doc["actual"];
        if (doc.containsKey("target")) _base->getData().bedTarget = doc["target"];
    } else if (topic.indexOf("printer/state") != -1) {
        if (doc.containsKey("text")) {
            String stateText = doc["text"].as<String>();
            if (!_base->isHoming() && !_base->isMeshBuilding() && !_pidRunning && !_mpcRunning) {
                _base->getData().status = stateText;
            }
        }
    } else if (topic.indexOf("progress/print") != -1) {
        if (doc.containsKey("progress")) {
            _base->getData().progress = doc["progress"];
        }
    }
}

void OctoClientMqtt::parseSerialMessage(const String& msg) {
    if (msg.indexOf("Unknown command") != -1) {
        _unknownCommandError = true;
        _pidRunning = false;
        _mpcRunning = false;
        Preferences prefs;
        prefs.begin("octoklip", false);
        prefs.putBool("calib_active", false);
        prefs.end();
        Serial.printf("[%lu ms] [MQTT ERROR] Unknown command detected: %s\n", millis(), msg.c_str());
    }

    int tPos = msg.indexOf("T:");
    if (tPos != -1) {
        int slashPos = msg.indexOf('/', tPos);
        if (slashPos != -1) {
            String actualStr = msg.substring(tPos + 2, slashPos);
            actualStr.trim();
            _base->getData().nozzleTemp = actualStr.toFloat();
            
            int spacePos = msg.indexOf(' ', slashPos);
            String targetStr = (spacePos != -1) ? msg.substring(slashPos + 1, spacePos) : msg.substring(slashPos + 1);
            targetStr.trim();
            _base->getData().nozzleTarget = targetStr.toFloat();
        }
    }
    
    int bPos = msg.indexOf("B:");
    if (bPos != -1) {
        int slashPos = msg.indexOf('/', bPos);
        if (slashPos != -1) {
            String actualStr = msg.substring(bPos + 2, slashPos);
            actualStr.trim();
            _base->getData().bedTemp = actualStr.toFloat();
            
            int spacePos = msg.indexOf(' ', slashPos);
            String targetStr = (spacePos != -1) ? msg.substring(slashPos + 1, spacePos) : msg.substring(slashPos + 1);
            targetStr.trim();
            _base->getData().bedTarget = targetStr.toFloat();
        }
    }

    bool isG28Command = (msg.indexOf("G28") != -1 && (msg.indexOf("Send:") != -1 || msg.indexOf("echo:") != -1 || msg.indexOf("N") != -1 || msg.indexOf("G28") == 0));
    bool isG29Command = (msg.indexOf("G29") != -1 && (msg.indexOf("Send:") != -1 || msg.indexOf("echo:") != -1 || msg.indexOf("N") != -1 || msg.indexOf("G29") == 0));

    if (isG28Command) {
        if (!_base->isHoming()) {
            _base->setHoming(true);
            _seenAxisReport = false;
            _cmdStartTime = millis();
            _lastBusyTime = millis();
        }
    }
    
    if (isG29Command) {
        if (!_base->isMeshBuilding()) {
            _base->setMeshBuildState(3);
            _base->setMeshPhase(1);
            _cmdStartTime = millis();
        }
    }

    if (_base->isHoming() && msg.indexOf("X:") != -1 && msg.indexOf("Count") != -1) {
        _seenAxisReport = true;
    }

    bool isTempPollLine = (msg.indexOf("T:") != -1 && msg.indexOf("B:") != -1);

    if (_base->isHoming()) {
        if (msg.indexOf("ok") != -1 && !isTempPollLine) {
            bool timeoutReached = (millis() - _cmdStartTime > 15000);
            if (_seenAxisReport || timeoutReached) {
                _base->setHoming(false);
                _seenAxisReport = false;
            }
        }
    }

    if (_base->isHoming()) {
        _base->getData().status = "Homing";
    } else if (_base->isMeshBuilding()) {
        _base->getData().status = "Mesh Building";
    } else if (_pidRunning) {
        _base->getData().status = "PID Autotune running";
    } else if (_mpcRunning) {
        _base->getData().status = "MPC Autotune running";
    } else {
        if (msg.indexOf("busy: processing") != -1) {
            _base->getData().status = "Working";
        }
    }
}

void OctoClientMqtt::update() {
    // Státusz védelme külső/HTTP/JSON felülírások ellen kalibráció alatt
    if (_pidRunning) {
        _base->getData().status = "PID Autotune running";
    } else if (_mpcRunning) {
        _base->getData().status = "MPC Autotune running";
    }

    if (_pendingClearWatchers) {
        Serial.printf("[%lu ms] [MQTT TRACE] Watcherek törlése (_pendingClearWatchers = true)\n", millis());
        _pendingClearWatchers = false;
        if (_monitor) _monitor->clearWatchers();
    }

    if (WiFi.status() == WL_CONNECTED) {
        if (!_mqtt->connected()) {
            _mqttActive = false;
            _base->setMqttActive(false);
            
            if (millis() - _lastReconnectAttempt > 5000) {
                _lastReconnectAttempt = millis();
                String clientId = "OctoKlip_" + String((uint32_t)ESP.getEfuseMac(), HEX);
                
                if (_mqtt->connect(clientId.c_str())) {
                    _mqtt->subscribe("octoprint/serial/#");
                    _mqtt->subscribe("octoprint/temperature/#");
                    _mqtt->subscribe("octoprint/printer/state");
                    _mqtt->subscribe("octoprint/progress/print");
                    _mqttActive = true;
                    _base->setMqttActive(true);
                    Serial.println(LangManager::get("mqtt_reconnected_http_off"));
                }
            }

            static unsigned long lastHttpCheck = 0;
            if (millis() - lastHttpCheck > 1000) {
                lastHttpCheck = millis();
                _base->update();
            }

        } else {
            _mqtt->loop();
        }

    } else {
        _mqttActive = false;
        _base->setMqttActive(false);
    }

    if (isConnected() && _base->isMeshBuilding() && _base->getMeshPhase() == 2) {
        if (_base->getData().bedTemp >= 59.0f || (_base->getData().bedTarget > 0 && _base->getData().bedTemp >= _base->getData().bedTarget - 1.0f)) {
            _base->setMeshPhase(3);
            int size = _base->isMeshBuilding();
            Serial.printf("[%lu ms] [MQTT MESH] Bed elerte a 60 fokot! G29 inditasa MQTT-n...\n", millis());

            _monitor->clearWatchers();
            _busySeen = false;
            _lastBusyTime = 0;
            _cmdStartTime = millis();

            _monitor->watchFor("busy: processing", [this](const String& msg) {
                this->_busySeen = true;
                this->_lastBusyTime = millis();
            });

            _monitor->watchFor("Recv: ok", [this](const String& msg) {
                bool busyFinished = this->_busySeen && (millis() - this->_lastBusyTime > 1200);
                bool fallbackTimeout = !this->_busySeen && (millis() - this->_cmdStartTime > 15000);

                if (this->_base->getMeshPhase() == 3) {
                    if (busyFinished || fallbackTimeout) {
                        Serial.printf("[%lu ms] [MQTT RX] G29 kész! M500 (mentés) indítása...\n", millis());
                        this->_base->setMeshPhase(4);
                        this->sendGcodeCommand("M500");
                    }
                } 
                else if (this->_base->getMeshPhase() == 4) {
                    Serial.printf("[%lu ms] [MQTT RX] M500 mentés kész! Cooldown indítása és feloldás.\n", millis());
                    this->_base->setMeshBuildState(0);
                    this->_base->setMeshPhase(0);
                    this->_pendingClearWatchers = true;
                    this->setBedTarget(0);
                }
            });

            if (size == 5) {
                sendGcodeCommand("G29 P1");
            } else {
                sendGcodeCommand("G29");
            }
        }
    }
}

bool OctoClientMqtt::isConnected() {
    return _mqttActive && _mqtt->connected();
}

void OctoClientMqtt::sendGcodeCommand(const String& gcode) {
    if (isConnected()) {
        Serial.printf("[%lu ms] [MQTT TX] %s\n", millis(), gcode.c_str());
        _mqtt->publish("octoprint/serial/command", gcode.c_str());
    } else {
        Serial.printf("[%lu ms] [HTTP FALLBACK TX] %s\n", millis(), gcode.c_str());
        _base->sendGcodeCommand(gcode); 
    }
}

void OctoClientMqtt::startPidAutotune() {
    if (!isConnected()) return;
    
    _pidRunning = true;
    _pidDone = false;
    _pidPhase = 1;
    _pidKp = 0.0f; 
    _pidKi = 0.0f; 
    _pidKd = 0.0f;

    // --- NVS: Kalibrációs flag beállítása (Indítás) ---
    Preferences prefs;
    prefs.begin("octoklip", false);
    prefs.putBool("calib_active", true);
    prefs.end();
    // --------------------------------------------------

    _base->getData().status = "PID Autotune running";
    
    _pendingClearWatchers = false;
    _monitor->clearWatchers();

    _monitor->watchFor("DEFAULT_Kp", [this](const String& msg) {
        int pos = msg.lastIndexOf(' ');
        if (pos != -1) this->_pidKp = msg.substring(pos + 1).toFloat();
    });
    
    _monitor->watchFor("DEFAULT_Ki", [this](const String& msg) {
        int pos = msg.lastIndexOf(' ');
        if (pos != -1) this->_pidKi = msg.substring(pos + 1).toFloat();
    });
    
    _monitor->watchFor("DEFAULT_Kd", [this](const String& msg) {
        int pos = msg.lastIndexOf(' ');
        if (pos != -1) this->_pidKd = msg.substring(pos + 1).toFloat();
    });

    _monitor->watchFor("Settings Stored", [this](const String& msg) {
        if (this->_pidRunning && this->_pidPhase == 3) {
            Serial.println("[MQTT PID] M500 'Settings Stored' megerkezett. Varakozas a lezaro 'ok'-ra...");
            this->_pidPhase = 4;
        }
    });

    _monitor->watchFor("Recv: ok", [this](const String& msg) {
        if (!this->_pidRunning) return;

        if (this->_pidPhase == 1 && this->_pidKp > 0 && this->_pidKi > 0 && this->_pidKd > 0) {
            Serial.printf("[MQTT PID] P:%.2f I:%.2f D:%.2f - M301 kuldese...\n", this->_pidKp, this->_pidKi, this->_pidKd);
            this->_pidPhase = 2;
            String m301 = "M301 P" + String(this->_pidKp, 3) + " I" + String(this->_pidKi, 3) + " D" + String(this->_pidKd, 3);
            this->sendGcodeCommand(m301);
        }
        else if (this->_pidPhase == 2) {
            Serial.println("[MQTT PID] M301 nyugtazva (ok), EEPROM mentes (M500) inditasa...");
            this->_pidPhase = 3;
            this->sendGcodeCommand("M500");
        }
        else if (this->_pidPhase == 4) {
            Serial.println("[MQTT PID] EEPROM mentes lezarva (ok), PID Kalibracio teljesen KESZ!");
            this->_pidRunning = false;
            this->_pidDone = true;

            // --- NVS: Kalibrációs flag törlése (Siker) ---
            Preferences prefs;
            prefs.begin("octoklip", false);
            prefs.putBool("calib_active", false);
            prefs.end();
            // ---------------------------------------------

            this->_base->getData().status = "Online";
            this->_pendingClearWatchers = true;
        }
    });

    sendGcodeCommand("M106 S255");
    sendGcodeCommand("M303 E0 S200 C5");
}

void OctoClientMqtt::startMpcAutotune() {
    if (!isConnected()) return;
    _mpcRunning = true;

    // --- NVS: Kalibrációs flag beállítása (Indítás) ---
    Preferences prefs;
    prefs.begin("octoklip", false);
    prefs.putBool("calib_active", true);
    prefs.end();
    // --------------------------------------------------

    _base->getData().status = "MPC Autotune running";
    sendGcodeCommand("M306 T");
}

void OctoClientMqtt::stopCalibration() {
    _pidRunning = false;
    _mpcRunning = false;
    _pidDone = false;

    // --- NVS: Kalibrációs flag törlése (Megszakítva) ---
    Preferences prefs;
    prefs.begin("octoklip", false);
    prefs.putBool("calib_active", false);
    prefs.end();
    // ---------------------------------------------------

    _base->getData().status = "Online";
    _pendingClearWatchers = true;
}

void OctoClientMqtt::autoHome() {
    if (isConnected()) {
        Serial.printf("[%lu ms] [MQTT EXEC] Auto Home (G28) inditasa MQTT-n! _base->setHoming(true)\n", millis());
        _base->setHoming(true);
        
        _pendingClearWatchers = false;
        _monitor->clearWatchers();

        _cmdStartTime = millis();
        _lastBusyTime = 0;
        _busySeen = false;

        _monitor->watchFor("busy: processing", [this](const String& msg) {
            this->_busySeen = true;
            this->_lastBusyTime = millis();
        });

        _monitor->watchFor("X:", [this](const String& msg) {
            bool matches = this->_base->isHoming() && (msg.indexOf("Y:") != -1 || msg.indexOf("Count") != -1);
            if (matches) {
                Serial.printf("[%lu ms] [MQTT MATCH G28 POS] Pozíciójelentés megérkezett! -> setHoming(false)\n", millis());
                this->_base->setHoming(false);
                this->_pendingClearWatchers = true;
            }
        });

        _monitor->watchFor("Recv: ok", [this](const String& msg) {
            bool currentHoming = this->_base->isHoming();
            bool busyFinished = this->_busySeen && (millis() - this->_lastBusyTime > 1200);
            bool fallbackTimeout = !this->_busySeen && (millis() - this->_cmdStartTime > 12000);

            if (!currentHoming) return;

            if (busyFinished || fallbackTimeout) {
                Serial.printf("[%lu ms] [MQTT MATCH G28 OK] Lezáró 'ok' elfogadva! -> setHoming(false)\n", millis());
                this->_base->setHoming(false);
                this->_pendingClearWatchers = true;
            }
        });

        sendGcodeCommand("G28");
    } else {
        _base->autoHome();
    }
}

void OctoClientMqtt::disableSteppers() {
    if (isConnected()) sendGcodeCommand("M84");
    else _base->disableSteppers();
}

void OctoClientMqtt::homeZ() {
    if (isConnected()) {
        Serial.printf("[%lu ms] [MQTT EXEC] Z Home (G28 Z) inditasa MQTT-n! _base->setHoming(true)\n", millis());
        _base->setHoming(true);

        _pendingClearWatchers = false;
        _monitor->clearWatchers();

        _cmdStartTime = millis();
        _lastBusyTime = 0;
        _busySeen = false;

        _monitor->watchFor("busy: processing", [this](const String& msg) {
            this->_busySeen = true;
            this->_lastBusyTime = millis();
        });

        _monitor->watchFor("Z:", [this](const String& msg) {
            bool matches = this->_base->isHoming() && (msg.indexOf("Count") != -1 || msg.indexOf("E:") != -1);
            if (matches) {
                Serial.printf("[%lu ms] [MQTT MATCH G28 Z POS] Pozíciójelentés megérkezett! -> setHoming(false)\n", millis());
                this->_base->setHoming(false);
                this->_pendingClearWatchers = true;
            }
        });

        _monitor->watchFor("Recv: ok", [this](const String& msg) {
            bool currentHoming = this->_base->isHoming();
            bool busyFinished = this->_busySeen && (millis() - this->_lastBusyTime > 1200);
            bool fallbackTimeout = !this->_busySeen && (millis() - this->_cmdStartTime > 8000);

            if (!currentHoming) return;

            if (busyFinished || fallbackTimeout) {
                Serial.printf("[%lu ms] [MQTT MATCH G28 Z OK] Lezáró 'ok' elfogadva! -> setHoming(false)\n", millis());
                this->_base->setHoming(false);
                this->_pendingClearWatchers = true;
            }
        });

        sendGcodeCommand("G28 Z");
    } else {
        _base->homeZ();
    }
}

void OctoClientMqtt::saveConfig() {
    if (isConnected()) sendGcodeCommand("M500");
    else _base->saveConfig();
}

void OctoClientMqtt::setNozzleTarget(float temp) {
    if (isConnected()) sendGcodeCommand("M104 S" + String((int)temp));
    else _base->setNozzleTarget(temp);
}

void OctoClientMqtt::setBedTarget(float temp) {
    if (isConnected()) sendGcodeCommand("M140 S" + String((int)temp));
    else _base->setBedTarget(temp);
}

void OctoClientMqtt::setSpeed(int percent) {
    if (isConnected()) sendGcodeCommand("M220 S" + String(percent));
    else _base->setSpeed(percent);
}

void OctoClientMqtt::adjustZOffset(float delta) {
    if (isConnected()) sendGcodeCommand("M290 Z" + String(delta, 3));
    else _base->adjustZOffset(delta);
}

void OctoClientMqtt::autoBuildMesh(int size) {
    if (!isConnected()) {
        _base->autoBuildMesh(size);
        return;
    }

    _base->setMeshBuildState(size);
    _base->setMeshPhase(1);
    
    _pendingClearWatchers = false;
    _monitor->clearWatchers();

    _cmdStartTime = millis();
    _lastBusyTime = 0;
    _busySeen = false;

    _monitor->watchFor("busy: processing", [this](const String& msg) {
        this->_busySeen = true;
        this->_lastBusyTime = millis();
    });

    _monitor->watchFor("X:", [this](const String& msg) {
        if (this->_base->getMeshPhase() == 1 && (msg.indexOf("Y:") != -1 || msg.indexOf("Count") != -1)) {
            this->_base->setMeshPhase(2);
            this->setBedTarget(60);
            this->_pendingClearWatchers = true;
        }
    });

    _monitor->watchFor("Recv: ok", [this](const String& msg) {
        if (this->_base->getMeshPhase() != 1) return;

        bool busyFinished = this->_busySeen && (millis() - this->_lastBusyTime > 1200);
        bool fallbackTimeout = !this->_busySeen && (millis() - this->_cmdStartTime > 12000);

        if (busyFinished || fallbackTimeout) {
            this->_base->setMeshPhase(2);
            this->setBedTarget(60);
            this->_pendingClearWatchers = true;
        }
    });

    sendGcodeCommand("G28");
}

void OctoClientMqtt::fetchBedMesh() {
    if (!isConnected()) {
        _base->fetchBedMesh();
        return;
    }

    _base->getData().meshLoaded = false;
    
    static int parsedRows = 0;
    static bool gridStarted = false;

    _monitor->clearWatchers();

    _monitor->watchFor("Bilinear Leveling Grid:", [this](const String& msg) {
        gridStarted = true;
        parsedRows = 0;
    });

    _monitor->watchFor("Recv: ", [this](const String& msg) {
        if (!gridStarted) return;

        String line = msg;
        line.replace("Recv: ", "");
        line.trim();

        if (line.startsWith("echo:") || line.startsWith("ok") || line.length() == 0) {
            if (parsedRows > 0) {
                _base->getData().meshRows = parsedRows;
                _base->getData().meshCols = parsedRows;
                _base->getData().meshLoaded = true;
                _base->setShowNoMeshPopup(false);
            } else {
                _base->getData().meshLoaded = true;
                _base->setShowNoMeshPopup(true);
            }
            gridStarted = false;
            return;
        }

        if (line.startsWith("0") && line.indexOf("1") > 0) return;

        int colIndex = 0;
        int lastSpace = 0;
        int currentSpace = 0;
        bool isRowIndexParsed = false;

        while ((currentSpace = line.indexOf(' ', lastSpace)) != -1 || lastSpace < line.length()) {
            String token = (currentSpace != -1) ? line.substring(lastSpace, currentSpace) : line.substring(lastSpace);
            token.trim();
            
            if (token.length() > 0) {
                if (!isRowIndexParsed) {
                    isRowIndexParsed = true;
                } else {
                    if (parsedRows < MAX_MESH_SIZE && colIndex < MAX_MESH_SIZE) {
                        _base->getData().bedMesh[parsedRows][colIndex] = token.toFloat();
                    }
                    colIndex++;
                }
            }

            if (currentSpace == -1) break;
            lastSpace = currentSpace + 1;
        }

        if (colIndex > 0) {
            parsedRows++;
        }
    });

    sendGcodeCommand("M420 V");
}