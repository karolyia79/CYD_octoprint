#include "klipper_client.h"
#include "lang_manager.h"

KlipperClient::KlipperClient(const String& ip) : _ip(ip) {}

void KlipperClient::update() {
    if (WiFi.status() != WL_CONNECTED) {
        _data.status = LangManager::get("wifi_no_wifi");
        _data.connected = false;
        return;
    }

    HTTPClient http;
    String url = "http://" + _ip + "/printer/objects/query?webhooks&print_stats&toolhead&extruder&heater_bed";
    http.begin(url);

    int httpCode = http.GET();
    if (httpCode > 0 && httpCode == HTTP_CODE_OK) {
        DynamicJsonDocument doc(4096);
        deserializeJson(doc, http.getStream());

        JsonObject status = doc["result"]["status"];
        
        String webhooksState = status["webhooks"]["state"].as<String>();
        String printState = status["print_stats"]["state"].as<String>();

        if (printState != "null" && printState != "" && printState != "standby" && printState != "complete") {
            _data.status = printState; 
        } else {
            _data.status = webhooksState; 
        }

        _data.name = status["print_stats"]["filename"].as<String>();
        if (_data.name == "null" || _data.name == "") {
            _data.name = LangManager::get("klipper_default_name");
        }

        float progressFloat = status["print_stats"]["progress"] | 0.0;
        _data.progress = (int)(progressFloat * 100);

        int printTime = status["print_stats"]["print_time"] | 0;
        _data.remainingTime = String(printTime) + "s";

        _data.nozzleTemp = status["extruder"]["temperature"] | 0.0;
        _data.nozzleTarget = status["extruder"]["target"] | 0.0;
        _data.bedTemp = status["heater_bed"]["temperature"] | 0.0;
        _data.bedTarget = status["heater_bed"]["target"] | 0.0;

        _data.connected = true;
    } else {
        _data.connected = false;
        _data.status = LangManager::get("wifi_conn_error");
    }
    http.end();
}

void KlipperClient::sendGcode(const String& gcode) {
    if (WiFi.status() != WL_CONNECTED) return;
    HTTPClient http;
    http.begin("http://" + _ip + "/printer/gcode/script");
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(1500);
    
    String payload = "{\"script\":\"" + gcode + "\"}";
    http.POST(payload);
    http.end();
}

void KlipperClient::setNozzleTarget(float temp) {
    _data.nozzleTarget = temp;
    sendGcode("M104 S" + String((int)temp));
}

void KlipperClient::setBedTarget(float temp) {
    _data.bedTarget = temp;
    sendGcode("M140 S" + String((int)temp));
}

void KlipperClient::setSpeed(int percent) {
    if (percent < 10) percent = 10;
    if (percent > 300) percent = 300;
    _data.speed = percent;
    sendGcode("M220 S" + String(percent));
}

void KlipperClient::adjustZOffset(float delta) {
    sendGcode("SET_GCODE_OFFSET Z_ADJUST=" + String(delta, 3) + " MOVE=1");
}