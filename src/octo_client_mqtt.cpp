#include "octo_client_mqtt.h"
#include "lang_manager.h"
#include <ArduinoJson.h>
#include <Preferences.h>

OctoClientMqtt::OctoClientMqtt(PubSubClient* mqttClient, MqttMonitor* monitor) 
    : _mqtt(mqttClient), _monitor(monitor) {}

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

        if (this->_monitor) {
            this->_monitor->processMessage(tStr, msg);
        }
    });

    String clientId = "OctoKlip_" + String((uint32_t)ESP.getEfuseMac(), HEX);

    Serial.println(LangManager::get("mqtt_conn_to_broker") + _brokerIp + ":" + String(_port));
    if (_mqtt->connect(clientId.c_str())) {
        _mqttActive = true;
        _data.mqttActive = true;
        _data.connected = true;
        _mqtt->subscribe("octoprint/serial/#");
        _mqtt->subscribe("octoprint/temperature/#");
        _mqtt->subscribe("octoprint/printer/state");
        _mqtt->subscribe("octoprint/progress/print");
        Serial.println(LangManager::get("mqtt_success_active"));
    } else {
        _mqttActive = false;
        _data.mqttActive = false;
        _data.connected = false;
        Serial.printf("%s %d\n", LangManager::get("mqtt_err_failed_code").c_str(), _mqtt->state());
    }
    
    return _mqttActive;
}

void OctoClientMqtt::parseJsonMessage(const String& topic, const String& payload) {
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, payload);
    if (error) return;

    if (topic.indexOf("temperature/tool0") != -1) {
        if (doc.containsKey("actual")) _data.nozzleTemp = doc["actual"];
        if (doc.containsKey("target")) _data.nozzleTarget = doc["target"];
    } else if (topic.indexOf("temperature/bed") != -1) {
        if (doc.containsKey("actual")) _data.bedTemp = doc["actual"];
        if (doc.containsKey("target")) _data.bedTarget = doc["target"];
    } else if (topic.indexOf("printer/state") != -1) {
        if (doc.containsKey("text")) {
            String stateText = doc["text"].as<String>();
            if (!_isHoming && !_meshBuildState && !_pidRunning && !_mpcRunning) {
                _data.status = stateText;
            }
        }
    } else if (topic.indexOf("progress/print") != -1) {
        if (doc.containsKey("progress")) {
            _data.progress = doc["progress"];
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
            _data.nozzleTemp = actualStr.toFloat();
            
            int spacePos = msg.indexOf(' ', slashPos);
            String targetStr = (spacePos != -1) ? msg.substring(slashPos + 1, spacePos) : msg.substring(slashPos + 1);
            targetStr.trim();
            _data.nozzleTarget = targetStr.toFloat();
        }
    }
    
    int bPos = msg.indexOf("B:");
    if (bPos != -1) {
        int slashPos = msg.indexOf('/', bPos);
        if (slashPos != -1) {
            String actualStr = msg.substring(bPos + 2, slashPos);
            actualStr.trim();
            _data.bedTemp = actualStr.toFloat();
            
            int spacePos = msg.indexOf(' ', slashPos);
            String targetStr = (spacePos != -1) ? msg.substring(slashPos + 1, spacePos) : msg.substring(slashPos + 1);
            targetStr.trim();
            _data.bedTarget = targetStr.toFloat();
        }
    }

    bool isG28Command = (msg.indexOf("G28") != -1 && (msg.indexOf("Send:") != -1 || msg.indexOf("echo:") != -1 || msg.indexOf("N") != -1 || msg.indexOf("G28") == 0));
    bool isG29Command = (msg.indexOf("G29") != -1 && (msg.indexOf("Send:") != -1 || msg.indexOf("echo:") != -1 || msg.indexOf("N") != -1 || msg.indexOf("G29") == 0));

    if (isG28Command) {
        if (!_isHoming) {
            Serial.printf("[%lu ms] [PARSER] G28 visszahang erkezett! setHoming(true)\n", millis());
            setHoming(true);
            _seenAxisReport = false;
            _cmdStartTime = millis();
            _lastBusyTime = millis();
        }
    }
    
    if (isG29Command) {
        if (!isMeshBuilding()) {
            Serial.printf("[%lu ms] [PARSER WARNING] külső G29 visszahang! setMeshBuildState(3), Phase(1)\n", millis());
            setMeshBuildState(3);
            setMeshPhase(1);
            _cmdStartTime = millis();
        }
    }

    if (_isHoming && msg.indexOf("X:") != -1 && msg.indexOf("Count") != -1) {
        _seenAxisReport = true;
    }

    bool isTempPollLine = (msg.indexOf("T:") != -1 && msg.indexOf("B:") != -1);

    if (_isHoming) {
        if (msg.indexOf("ok") != -1 && !isTempPollLine) {
            bool timeoutReached = (millis() - _cmdStartTime > 15000);
            if (_seenAxisReport || timeoutReached) {
                setHoming(false);
                _seenAxisReport = false;
            }
        }
    }

    if (_isHoming) {
        _data.status = "Homing";
    } else if (isMeshBuilding()) {
        _data.status = "Mesh Building";
    } else if (_pidRunning) {
        _data.status = "PID Autotune running";
    } else if (_mpcRunning) {
        _data.status = "MPC Autotune running";
    } else {
        if (msg.indexOf("busy: processing") != -1) {
            _data.status = "Working";
        }
    }
}

void OctoClientMqtt::update() {
    if (_pidRunning) {
        _data.status = "PID Autotune running";
    } else if (_mpcRunning) {
        _data.status = "MPC Autotune running";
    }

    if (_pendingClearWatchers) {
        Serial.printf("[%lu ms] [MQTT TRACE] Watcherek torlese (_pendingClearWatchers = true)\n", millis());
        _pendingClearWatchers = false;
        if (_monitor) _monitor->clearWatchers();
    }

    if (WiFi.status() == WL_CONNECTED) {
        if (!_mqtt->connected()) {
            _mqttActive = false;
            _data.mqttActive = false;
            _data.connected = false;
            
            if (millis() - _lastReconnectAttempt > 5000) {
                _lastReconnectAttempt = millis();
                String clientId = "OctoKlip_" + String((uint32_t)ESP.getEfuseMac(), HEX);
                
                if (_mqtt->connect(clientId.c_str())) {
                    _mqtt->subscribe("octoprint/serial/#");
                    _mqtt->subscribe("octoprint/temperature/#");
                    _mqtt->subscribe("octoprint/printer/state");
                    _mqtt->subscribe("octoprint/progress/print");
                    _mqttActive = true;
                    _data.mqttActive = true;
                    _data.connected = true;
                    Serial.println(LangManager::get("mqtt_reconnected_http_off"));
                }
            }
        } else {
            _mqttActive = true;
            _data.mqttActive = true;
            _data.connected = true;
            _mqtt->loop();
        }
    } else {
        _mqttActive = false;
        _data.mqttActive = false;
        _data.connected = false;
    }

    // --- BED MESH FÁZIS-3 -> FÁZIS-4 DIAGNOSZTIKUS VEZÉRLŐ ---
    if (isConnected() && isMeshBuilding() && getMeshPhase() == 2) {
        if (_data.bedTemp >= 59.0f || (_data.bedTarget > 0 && _data.bedTemp >= _data.bedTarget - 1.0f)) {
            setMeshPhase(3);
            int size = _meshBuildState;
            Serial.printf("\n==================================================\n");
            Serial.printf("[%lu ms] [DIAG MESH] BED ELÉRTE A 60 FOKOT! G29 INDÍTÁSA MQTT-N...\n", millis());
            Serial.printf("==================================================\n");

            _monitor->clearWatchers();
            _cmdStartTime = millis();

            static bool meshReportSeen = false;
            meshReportSeen = false;

            _monitor->watchFor("ok", [this](const String& msg) {
                int currentPhase = this->getMeshPhase();
                Serial.printf("[%lu ms] [DIAG WATCHER 'ok'] Msg: '%s' | Phase: %d | meshReportSeen: %d\n", 
                              millis(), msg.c_str(), currentPhase, meshReportSeen);

                if (msg.indexOf("T:") != -1 || msg.indexOf("B:") != -1) {
                    Serial.printf("[%lu ms] [DIAG WATCHER] -> KISZŰRVE (Hőmérséklet lekérdezés volt)\n", millis());
                    return;
                }

                if (msg.indexOf("X:") != -1 || msg.indexOf("Grid") != -1 || msg.indexOf("Mesh") != -1 || msg.indexOf("Count") != -1) {
                    meshReportSeen = true;
                    Serial.printf("[%lu ms] [DIAG WATCHER] -> POZÍCIÓ / MÁTRIX JELENTÉS ÉSZTVE! meshReportSeen = true\n", millis());
                    return;
                }

                if (currentPhase == 3) {
                    bool timeout = (millis() - this->_cmdStartTime > 60000);
                    Serial.printf("[%lu ms] [DIAG Phase 3 Check] meshReportSeen=%d, timeout=%d\n", millis(), meshReportSeen, timeout);
                    
                    if (meshReportSeen || timeout) {
                        Serial.printf("\n--------------------------------------------------\n");
                        Serial.printf("[%lu ms] [DIAG Phase 3 -> Phase 4] G29 OK MEEGÉRKEZETT! M500 KÜLDÉSE...\n", millis());
                        Serial.printf("--------------------------------------------------\n");
                        
                        this->setMeshPhase(4);
                        this->sendGcodeCommand("M500");
                        meshReportSeen = false;
                    } else {
                        Serial.printf("[%lu ms] [DIAG Phase 3] Még NEM volt pozíciójelentés, 'ok' figyelmen kívül hagyva.\n", millis());
                    }
                } 
                else if (currentPhase == 4) {
                    Serial.printf("\n==================================================\n");
                    Serial.printf("[%lu ms] [DIAG Phase 4 LEZÁRÁS] M500 OK MEGERKEZETT! MÉRÉS SIKERESEN KÉSZ!\n", millis());
                    Serial.printf("==================================================\n");
                    
                    this->setMeshBuildState(0);
                    this->setMeshPhase(0);
                    
                    this->setShowMeshSavedPopup(true);

                    this->sendGcodeCommand("M140 S0");
                    this->sendGcodeCommand("M104 S0");

                    this->_data.bedTarget = 0;
                    this->_data.nozzleTarget = 0;

                    this->_pendingClearWatchers = true;
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
    }
}

void OctoClientMqtt::extrudeFilament(float lengthMm, float speedMmMin) {
    sendGcodeCommand("G91");
    sendGcodeCommand("G1 E" + String(lengthMm, 1) + " F" + String((int)speedMmMin));
    sendGcodeCommand("G90");
}

void OctoClientMqtt::loadFilament(bool isBowden) {
    sendGcodeCommand("G91");
    if (isBowden) {
        sendGcodeCommand("G1 E380 F1200");
        sendGcodeCommand("G1 E20 F100");
    } else {
        sendGcodeCommand("G1 E40 F300");
        sendGcodeCommand("G1 E10 F100");
    }
    sendGcodeCommand("G90");
}

void OctoClientMqtt::unloadFilament(bool isBowden) {
    sendGcodeCommand("G91");
    sendGcodeCommand("G1 E2 F100");
    if (isBowden) {
        sendGcodeCommand("G1 E-420 F1800");
    } else {
        sendGcodeCommand("G1 E-50 F1200");
    }
    sendGcodeCommand("G90");
}

void OctoClientMqtt::startZOffsetPrep() {
    if (!isConnected()) {
        _zOffsetPrepRunning = false;
        _zOffsetReady = true;
        _zOffsetPrepPhase = 0;
        return;
    }

    _zOffsetPrepRunning = true;
    _zOffsetReady = false;
    _zOffsetPrepPhase = 1;
    setHoming(true);
    _pendingClearWatchers = false;
    _monitor->clearWatchers();

    _cmdStartTime = millis();
    _lastBusyTime = 0;
    _busySeen = false;

    _monitor->watchFor("busy: processing", [this](const String& msg) {
        this->_busySeen = true;
        this->_lastBusyTime = millis();
    });

    _monitor->watchFor("ok", [this](const String& msg) {
        if (!this->_zOffsetPrepRunning) return;
        if (msg.indexOf("T:") != -1 || msg.indexOf("B:") != -1) return;

        bool busyFinished = this->_busySeen && (millis() - this->_lastBusyTime > 1000);
        bool fallbackTimeout = (millis() - this->_cmdStartTime > 15000);

        if (this->_zOffsetPrepPhase == 1) {
            if (busyFinished || fallbackTimeout || !this->_busySeen) {
                this->_zOffsetPrepPhase = 2;
                this->setHoming(false);
                this->_busySeen = false;
                this->_lastBusyTime = 0;
                this->_cmdStartTime = millis();
                Serial.printf("[%lu ms] [MQTT Z-OFFSET PREP] G28 ok! G1 Z0 F300 kuldese...\n", millis());
                this->sendGcodeCommand("G1 Z0 F300");
            }
        } else if (this->_zOffsetPrepPhase == 2) {
            if (busyFinished || fallbackTimeout || !this->_busySeen) {
                Serial.printf("[%lu ms] [MQTT Z-OFFSET PREP] G1 Z0 F300 ok! Kesz, offset allithato.\n", millis());
                this->_zOffsetPrepRunning = false;
                this->_zOffsetReady = true;
                this->_zOffsetPrepPhase = 0;
                this->_pendingClearWatchers = true;
            }
        }
    });

    sendGcodeCommand("G28");
}

void OctoClientMqtt::startPidAutotune() {
    if (!isConnected()) return;
    
    _pidRunning = true;
    _pidDone = false;
    _pidPhase = 1;
    _pidKp = 0.0f; 
    _pidKi = 0.0f; 
    _pidKd = 0.0f;

    Preferences prefs;
    prefs.begin("octoklip", false);
    prefs.putBool("calib_active", true);
    prefs.end();

    _data.status = "PID Autotune running";
    
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

    _monitor->watchFor("ok", [this](const String& msg) {
        if (!this->_pidRunning) return;
        if (msg.indexOf("T:") != -1 || msg.indexOf("B:") != -1) return;

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

            Preferences prefs;
            prefs.begin("octoklip", false);
            prefs.putBool("calib_active", false);
            prefs.end();

            this->_data.status = "Online";
            this->_pendingClearWatchers = true;
        }
    });

    sendGcodeCommand("M106 S255");
    sendGcodeCommand("M303 E0 S200 C5");
}

void OctoClientMqtt::startMpcAutotune() {
    if (!isConnected()) return;
    _mpcRunning = true;

    Preferences prefs;
    prefs.begin("octoklip", false);
    prefs.putBool("calib_active", true);
    prefs.end();

    _data.status = "MPC Autotune running";
    sendGcodeCommand("M306 T");
}

void OctoClientMqtt::stopCalibration() {
    _pidRunning = false;
    _mpcRunning = false;
    _pidDone = false;

    Preferences prefs;
    prefs.begin("octoklip", false);
    prefs.putBool("calib_active", false);
    prefs.end();

    _data.status = "Online";
    _pendingClearWatchers = true;
}

void OctoClientMqtt::autoHome() {
    if (isConnected()) {
        Serial.printf("[%lu ms] [MQTT EXEC] Auto Home (G28) inditasa MQTT-n!\n", millis());
        setHoming(true);
        
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
            bool matches = this->_isHoming && (msg.indexOf("Y:") != -1 || msg.indexOf("Count") != -1);
            if (matches) {
                Serial.printf("[%lu ms] [MQTT MATCH G28 POS] Pozíciójelentés megérkezett! -> setHoming(false)\n", millis());
                this->setHoming(false);
                this->_pendingClearWatchers = true;
            }
        });

        _monitor->watchFor("ok", [this](const String& msg) {
            bool currentHoming = this->_isHoming;
            if (!currentHoming) return;
            if (msg.indexOf("T:") != -1 || msg.indexOf("B:") != -1) return;

            bool busyFinished = this->_busySeen && (millis() - this->_lastBusyTime > 1200);
            bool fallbackTimeout = !this->_busySeen && (millis() - this->_cmdStartTime > 12000);

            if (busyFinished || fallbackTimeout) {
                Serial.printf("[%lu ms] [MQTT MATCH G28 OK] Lezáró 'ok' elfogadva! -> setHoming(false)\n", millis());
                this->setHoming(false);
                this->_pendingClearWatchers = true;
            }
        });

        sendGcodeCommand("G28");
    }
}

void OctoClientMqtt::disableSteppers() {
    sendGcodeCommand("M84");
}

void OctoClientMqtt::homeZ() {
    if (isConnected()) {
        Serial.printf("[%lu ms] [MQTT EXEC] Z Home (G28 Z) inditasa MQTT-n!\n", millis());
        setHoming(true);

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
            bool matches = this->_isHoming && (msg.indexOf("Count") != -1 || msg.indexOf("E:") != -1);
            if (matches) {
                Serial.printf("[%lu ms] [MQTT MATCH G28 Z POS] Pozíciójelentés megérkezett! -> setHoming(false)\n", millis());
                this->setHoming(false);
                this->_pendingClearWatchers = true;
            }
        });

        _monitor->watchFor("ok", [this](const String& msg) {
            bool currentHoming = this->_isHoming;
            if (!currentHoming) return;
            if (msg.indexOf("T:") != -1 || msg.indexOf("B:") != -1) return;

            bool busyFinished = this->_busySeen && (millis() - this->_lastBusyTime > 1200);
            bool fallbackTimeout = !this->_busySeen && (millis() - this->_cmdStartTime > 8000);

            if (busyFinished || fallbackTimeout) {
                Serial.printf("[%lu ms] [MQTT MATCH G28 Z OK] Lezáró 'ok' elfogadva! -> setHoming(false)\n", millis());
                this->setHoming(false);
                this->_pendingClearWatchers = true;
            }
        });

        sendGcodeCommand("G28 Z");
    }
}

void OctoClientMqtt::saveConfig() {
    sendGcodeCommand("M500");
}

void OctoClientMqtt::setNozzleTarget(float temp) {
    _data.nozzleTarget = temp;
    sendGcodeCommand("M104 S" + String((int)temp));
}

void OctoClientMqtt::setBedTarget(float temp) {
    _data.bedTarget = temp;
    sendGcodeCommand("M140 S" + String((int)temp));
}

void OctoClientMqtt::setSpeed(int percent) {
    _data.speed = percent;
    sendGcodeCommand("M220 S" + String(percent));
}

void OctoClientMqtt::adjustZOffset(float delta) {
    sendGcodeCommand("M290 Z" + String(delta, 3));
}

void OctoClientMqtt::autoBuildMesh(int size) {
    if (!isConnected()) return;

    setMeshBuildState(size);
    setMeshPhase(1);
    
    _pendingClearWatchers = false;
    _monitor->clearWatchers();

    _cmdStartTime = millis();

    _monitor->watchFor("ok", [this](const String& msg) {
        if (msg.indexOf("T:") != -1 || msg.indexOf("B:") != -1) return;

        if (this->getMeshPhase() == 1) {
            Serial.printf("[%lu ms] [MQTT MESH Phase 1] G28 ok megerkezett! Bed heating (60C)...\n", millis());
            this->setMeshPhase(2);
            this->setBedTarget(60);
            this->_pendingClearWatchers = true;
        }
    });

    sendGcodeCommand("G28");
}

void OctoClientMqtt::fetchBedMesh() {
    if (!isConnected()) return;

    _data.meshLoaded = false;
    
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
                _data.meshRows = parsedRows;
                _data.meshCols = parsedRows;
                _data.meshLoaded = true;
                setShowNoMeshPopup(false);
            } else {
                _data.meshLoaded = true;
                setShowNoMeshPopup(true);
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
                        _data.bedMesh[parsedRows][colIndex] = token.toFloat();
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