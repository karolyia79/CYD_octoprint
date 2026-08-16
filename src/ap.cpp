#include "ap.h"
#include "lang_manager.h"
#include <SPI.h>

APManager::APManager(SplashScreen* splash) : _splash(splash), _server(80) {}

void APManager::begin() {
    PrinterConfig cfg = ConfigManager::loadConfig();
    
    digitalWrite(15, HIGH);
    digitalWrite(5, LOW);
    SPI.setFrequency(4000000);
    LangManager::loadLanguage(cfg.language);
    digitalWrite(5, HIGH);

    Serial.println("\n--- AP MANAGER INDITASA ---");
    Serial.println("SSID: [" + cfg.wifi_ssid + "]");
    Serial.println("----------------------------\n");

    startAPMode();
    setupRoutes();
    _server.begin();
    Logger::logSystem("AP Webszerver elinditva.");

    Serial.println("[AP MODE] Varakozas a WebUI beallitasokra vagy kijelzo erintesre...");
    
    uint16_t touchX, touchY;
    while (true) {
        _server.handleClient();
        
        if (_splash->getTouch(&touchX, &touchY)) {
            Serial.println("[AP MODE] Kijelzo megerintve! Tovabblepes...");
            delay(200);
            break;
        }
        delay(10);
    }
}

void APManager::startServer() {
    setupRoutes();
    _server.begin();
    Logger::logSystem("Webszerver elinditva a helyi IP-n: " + WiFi.localIP().toString());
}

void APManager::connectWiFi(const PrinterConfig& cfg) {
    WiFi.mode(WIFI_STA);
    
    if (cfg.use_static_ip) {
        Serial.println("Statikus IP beallitasa...");
        IPAddress ip, gateway, subnet, dns;
        ip.fromString(cfg.static_ip);
        gateway.fromString(cfg.gateway);
        subnet.fromString(cfg.subnet);
        dns.fromString(cfg.dns);
        WiFi.config(ip, gateway, subnet, dns);
    }

    Serial.print("Csatlakozas a(z) '");
    Serial.print(cfg.wifi_ssid);
    Serial.println("' halozathoz...");

    WiFi.begin(cfg.wifi_ssid.c_str(), cfg.wifi_pass.c_str());
    
    int counter = 0;
    int maxAttempts = 200; 
    
    _splash->showMessage(LangManager::get("wifi_connecting"), TFT_ORANGE);
    
    while (WiFi.status() != WL_CONNECTED && counter < maxAttempts) {
        delay(100);
        counter++;
        
        if (counter == 30) {
            _splash->showMessage(LangManager::get("wifi_authenticating"), TFT_ORANGE);
        } else if (counter == 80) {
            _splash->showMessage(LangManager::get("wifi_dhcp"), TFT_ORANGE);
        } else if (counter == 140) {
            _splash->showMessage(LangManager::get("wifi_waiting"), TFT_YELLOW);
        }
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Sikeres Wi-Fi csatlakozas!");
        Serial.print("Eszkoz IP cime: ");
        Serial.println(WiFi.localIP());

        Logger::logSystem("Csatlakozva a Wi-Fi-hez. IP: " + WiFi.localIP().toString());
        _splash->showMessage(LangManager::get("wifi_connected"), TFT_GREEN);
        delay(200);
        _splash->showConnectedInfo(WiFi.localIP().toString(), cfg.octo_enabled, cfg.klipper_enabled);
    } else {
        Serial.println("HIBA: Nem sikerult csatlakozni a Wi-Fi-hez!");
        Logger::logError("Nem sikerult csatlakozni a Wi-Fi-hez, AP modra valtas...");

        String errorReason = "";
        switch(WiFi.status()) {
            case WL_NO_SSID_AVAIL:  errorReason = LangManager::get("wifi_err_no_ssid"); break;
            case WL_CONNECT_FAILED: errorReason = LangManager::get("wifi_err_bad_pass"); break;
            case WL_CONNECTION_LOST:errorReason = LangManager::get("wifi_err_lost"); break;
            default:                errorReason = LangManager::get("wifi_err_timeout"); break;
        }
        
        _lastWifiError = errorReason;
        _splash->showAPInfo("OctoScreen_Setup", "12345678", "192.168.4.1", false, errorReason);
        
        uint32_t errorStartTime = millis();
        uint16_t touchX, touchY;
        while (millis() - errorStartTime < 10000) {
            if (_splash->getTouch(&touchX, &touchY)) {
                delay(200);
                break;
            }
            delay(50);
        }

        startAPMode();
    }
}

void APManager::startAPMode() {
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP("OctoScreen_Setup", "12345678");

    delay(100);

    IPAddress IP = WiFi.softAPIP();
    
    Serial.print("AP Mod elinditva. IP: ");
    Serial.println(IP);

    Logger::logSystem("AP Mod inditva. IP: " + IP.toString());
    _splash->showAPInfo("OctoScreen_Setup", "12345678", IP.toString(), false, _lastWifiError);
}

void APManager::setupRoutes() {
    _server.on("/", HTTP_GET, [this]() {
        PrinterConfig cfg = ConfigManager::loadConfig();
        
        digitalWrite(15, HIGH);
        digitalWrite(5, LOW);
        SPI.setFrequency(4000000);
        LangManager::loadLanguage(cfg.language);
        digitalWrite(5, HIGH);

        String html = "<!DOCTYPE html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'>";
        html += "<style>";
        html += "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background: #0f172a; color: #f8fafc; margin: 0; padding: 20px; }";
        html += ".container { max-width: 600px; margin: auto; background: #1e293b; padding: 25px; border-radius: 12px; box-shadow: 0 4px 20px rgba(0,0,0,0.5); }";
        html += "h2 { color: #38bdf8; margin-top: 0; text-align: center; }";
        html += ".card { background: #334155; padding: 15px; border-radius: 8px; margin-bottom: 20px; }";
        html += "label { display: block; margin-bottom: 5px; font-weight: 600; font-size: 14px; }";
        html += "input[type='text'], input[type='password'], input[type='number'], select { width: 100%; padding: 10px; margin-bottom: 15px; border: 1px solid #475569; border-radius: 6px; background: #0f172a; color: #fff; box-sizing: border-box; }";
        html += ".checkbox-group { display: flex; align-items: center; margin-bottom: 15px; }";
        html += ".checkbox-group input { width: auto; margin-right: 10px; }";
        html += "button, input[type='submit'] { background: #0284c7; color: white; border: none; padding: 12px 20px; border-radius: 6px; cursor: pointer; font-weight: bold; width: 100%; font-size: 16px; transition: background 0.2s; }";
        html += "button:hover, input[type='submit']:hover { background: #0369a1; }";
        html += ".btn-scan { background: #0d9488; margin-bottom: 15px; }";
        html += ".btn-scan:hover { background: #0f766e; }";
        html += ".btn-danger { background: #dc2626; }";
        html += ".btn-danger:hover { background: #b91c1c; }";
        html += ".btn-warning { background: #d97706; margin-bottom: 10px; }";
        html += ".btn-warning:hover { background: #b45309; }";
        html += "</style>";
        html += "<script>";
        // Azonnali nyelvváltás szkript (AJAX hívás az onchange eseményre)
        html += "function changeLanguage(sel) {";
        html += "  let lang = sel.value;";
        html += "  fetch('/savelang?lang=' + lang, { method: 'POST' }).then(res => {";
        html += "    location.reload();";
        html += "  }).catch(err => { alert('Hiba a nyelvváltáskor!'); });";
        html += "}";
        html += "function scanNetworks() {";
        html += "  let btn = document.getElementById('scan_btn');";
        html += "  btn.innerHTML = 'Kereses folyamatban...';";
        html += "  fetch('/scan').then(res => res.json()).then(data => {";
        html += "    let select = document.getElementById('ssid_select');";
        html += "    select.innerHTML = '';";
        html += "    data.forEach(net => {";
        html += "      let opt = document.createElement('option');";
        html += "      opt.value = net.ssid; opt.innerHTML = net.ssid + ' (' + net.rssi + ' dBm)';";
        html += "      select.appendChild(opt);";
        html += "    });";
        html += "    btn.innerHTML = 'Halozatok Keresese';";
        html += "  }).catch(err => { btn.innerHTML = 'Hiba a keresesben!'; });";
        html += "}";
        html += "function togglePassword() {";
        html += "  let pw = document.getElementById('wifi_pass_input');";
        html += "  pw.type = document.getElementById('show_pass_chk').checked ? 'text' : 'password';";
        html += "}";
        html += "function toggleStatic() {";
        html += "  let x = document.getElementById('static_div');";
        html += "  x.style.display = document.getElementById('use_static_ip').checked ? 'block' : 'none';";
        html += "}";
        html += "function restartDevice() {";
        html += "  if(confirm('Biztosan ujra szeretned inditani az eszkozt?')) {";
        html += "    fetch('/restart', { method: 'POST' }).then(res => {";
        html += "      document.open();";
        html += "      document.write('<html><body style=\"background:#0f172a;color:#fff;text-align:center;padding-top:50px;font-family:sans-serif;\"><h2>Eszkoz ujraindul... Kerlek varj.</h2></body></html>');";
        html += "      document.close();";
        html += "      setTimeout(() => { location.reload(); }, 4000);";
        html += "    }).catch(err => { alert('Hiba tortent a muvelet kozben!'); });";
        html += "  }";
        html += "}";
        html += "function formatSD() {";
        html += "  if(confirm('Biztosan torolni szeretned az SD kartya tartalmat? Minden beallitas es log fajl elveszik!')) {";
        html += "    fetch('/format', { method: 'POST' }).then(res => {";
        html += "      document.open();";
        html += "      document.write('<html><body style=\"background:#0f172a;color:#fff;text-align:center;padding-top:50px;font-family:sans-serif;\"><h2>SD kartya torolve. Ujrainditas...</h2></body></html>');";
        html += "      document.close();";
        html += "      setTimeout(() => { location.reload(); }, 3000);";
        html += "    }).catch(err => { alert('Hiba tortent a muvelet kozben!'); });";
        html += "  }";
        html += "}";
        html += "</script></head><body>";
        
        html += "<div class='container'>";
        html += "<h2>" + LangManager::get("ap_title") + "</h2>";
        html += "<form action='/save' method='POST'>";

        html += "<div class='card'>";
        html += "<h3 style='color: #38bdf8; margin-top: 0;'>" + LangManager::get("ap_lang_title") + "</h3>";
        // Itt adjuk hozzá az onchange eseményt az azonnali váltáshoz
        html += "<select name='language' onchange='changeLanguage(this)'>";
        html += "<option value='hu' " + String(cfg.language == "hu" ? "selected" : "") + ">Magyar</option>";
        html += "<option value='en' " + String(cfg.language == "en" ? "selected" : "") + ">English</option>";
        html += "<option value='de' " + String(cfg.language == "de" ? "selected" : "") + ">Deutsch</option>";
        html += "<option value='pl' " + String(cfg.language == "pl" ? "selected" : "") + ">Polski</option>";
        html += "</select>";
        html += "</div>";

        html += "<div class='card'>";
        html += "<h3>" + LangManager::get("ap_wifi_settings") + "</h3>";
        html += "<button type='button' id='scan_btn' class='btn-scan' onclick='scanNetworks()'>" + LangManager::get("ap_scan_networks") + "</button>";
        html += "<label>" + LangManager::get("ap_selected_ssid") + "</label>";
        html += "<select name='wifi_ssid' id='ssid_select'><option value='" + cfg.wifi_ssid + "'>" + (cfg.wifi_ssid == "" ? "-- Valassz vagy keresgelj --" : cfg.wifi_ssid) + "</option></select>";
        html += "<label>" + LangManager::get("ap_manual_ssid") + "</label>";
        html += "<input type='text' name='wifi_ssid_manual' value='' placeholder='" + (cfg.wifi_ssid != "" ? cfg.wifi_ssid : "Kezi SSID megadasa") + "'>";
        
        html += "<label>" + LangManager::get("ap_wifi_pass") + "</label>";
        html += "<input type='password' id='wifi_pass_input' name='wifi_pass' value='" + cfg.wifi_pass + "'>";
        html += "<div class='checkbox-group'>";
        html += "<input type='checkbox' id='show_pass_chk' onclick='togglePassword()'>";
        html += "<label for='show_pass_chk' style='margin-bottom:0;'>" + LangManager::get("ap_show_pass") + "</label>";
        html += "</div>";
        
        html += "<div class='checkbox-group'>";
        html += "<input type='checkbox' id='use_static_ip' name='use_static_ip' value='true' " + String(cfg.use_static_ip ? "checked" : "") + " onclick='toggleStatic()'>";
        html += "<label for='use_static_ip' style='margin-bottom:0;'>" + LangManager::get("ap_use_static") + "</label>";
        html += "</div>";

        html += "<div id='static_div' style='display:" + String(cfg.use_static_ip ? "block" : "none") + ";'>";
        html += "<label>" + LangManager::get("ap_ip_address") + "</label><input type='text' name='static_ip' value='" + cfg.static_ip + "'>";
        html += "<label>" + LangManager::get("ap_gateway") + "</label><input type='text' name='gateway' value='" + cfg.gateway + "'>";
        html += "<label>" + LangManager::get("ap_subnet") + "</label><input type='text' name='subnet' value='" + cfg.subnet + "'>";
        html += "<label>" + LangManager::get("ap_dns") + "</label><input type='text' name='dns' value='" + cfg.dns + "'>";
        html += "</div>";
        html += "</div>";

        html += "<div class='card'>";
        html += "<div class='checkbox-group'>";
        html += "<input type='checkbox' id='octo_enabled' name='octo_enabled' value='true' " + String(cfg.octo_enabled ? "checked" : "") + ">";
        html += "<label for='octo_enabled' style='margin-bottom:0; color:#38bdf8;'>" + LangManager::get("ap_octo_enabled") + "</label>";
        html += "</div>";
        html += "<label>" + LangManager::get("ap_octo_ip") + "</label><input type='text' name='octo_ip' value='" + cfg.octo_ip + "'>";
        html += "</div>";

        html += "<div class='card'>";
        html += "<div class='checkbox-group'>";
        html += "<input type='checkbox' id='klipper_enabled' name='klipper_enabled' value='true' " + String(cfg.klipper_enabled ? "checked" : "") + ">";
        html += "<label for='klipper_enabled' style='margin-bottom:0; color:#38bdf8;'>" + LangManager::get("ap_klipper_enabled") + "</label>";
        html += "</div>";
        html += "<label>" + LangManager::get("ap_klipper_ip") + "</label><input type='text' name='klipper_ip' value='" + cfg.klipper_ip + "'>";
        html += "<label>" + LangManager::get("ap_klipper_port") + "</label><input type='number' name='klipper_port' value='" + String(cfg.klipper_port) + "'>";
        html += "<label>" + LangManager::get("ap_klipper_key") + "</label><input type='text' name='klipper_key' value='" + cfg.klipper_key + "'>";
        html += "</div>";

        html += "<input type='submit' value='" + LangManager::get("ap_save_btn") + "'>";
        html += "</form>";

        html += "<div class='card' style='margin-top: 20px; border: 1px solid #475569;'>";
        html += "<h3 style='color: #38bdf8; margin-top: 0;'>" + LangManager::get("ap_maintenance") + "</h3>";
        
        html += "<p style='font-size: 13px; color: #94a3b8; margin-bottom: 10px;'>" + LangManager::get("ap_restart_desc") + "</p>";
        html += "<button type='button' class='btn-warning' onclick='restartDevice()'>" + LangManager::get("ap_restart_btn") + "</button>";

        html += "<p style='font-size: 13px; color: #94a3b8; margin-top: 15px; margin-bottom: 10px;'>" + LangManager::get("ap_format_desc") + "</p>";
        html += "<button type='button' class='btn-danger' onclick='formatSD()'>" + LangManager::get("ap_format_btn") + "</button>";
        html += "</div>";

        html += "</div></body></html>";

        _server.send(200, "text/html; charset=utf-8", html);
    });

    // Új endpoint az azonnali háttér-nyelvváltáshoz
    _server.on("/savelang", HTTP_POST, [this]() {
        String newLang = _server.arg("lang");
        newLang.trim();
        if (newLang.length() > 0) {
            PrinterConfig cfg = ConfigManager::loadConfig();
            cfg.language = newLang;
            ConfigManager::saveConfig(cfg);

            // Azonnali érvényesítés a memóriában
            digitalWrite(15, HIGH);
            digitalWrite(5, LOW);
            SPI.setFrequency(4000000);
            LangManager::loadLanguage(cfg.language);
            digitalWrite(5, HIGH);

            // Ha AP módban vagyunk, a fizikai kijelzőt is azonnal frissítjük az új nyelven!
            if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
                IPAddress IP = WiFi.softAPIP();
                int currentStations = WiFi.softAPgetStationNum();
                _splash->showAPInfo("OctoScreen_Setup", "12345678", IP.toString(), currentStations > 0, _lastWifiError);
            }

            _server.send(200, "application/json", "{\"status\":\"ok\"}");
        } else {
            _server.send(400, "application/json", "{\"status\":\"error\"}");
        }
    });

    _server.on("/scan", HTTP_GET, [this]() {
        Serial.println("Wi-Fi halozatok scan inditasa...");
        int n = WiFi.scanNetworks();
        Serial.println("Talalt hoxok szama: " + String(n));
        DynamicJsonDocument doc(1024);
        JsonArray array = doc.to<JsonArray>();
        for (int i = 0; i < n; ++i) {
            JsonObject obj = array.createNestedObject();
            obj["ssid"] = WiFi.SSID(i);
            obj["rssi"] = WiFi.RSSI(i);
        }
        String output;
        serializeJson(doc, output);
        _server.send(200, "application/json", output);
    });

    _server.on("/save", HTTP_POST, [this]() {
        Serial.println("\n--- MENTESI KERELES ERKEZETT ---");
        PrinterConfig cfg = ConfigManager::loadConfig(); 

        String selectedLang = _server.arg("language");
        selectedLang.trim();
        if (selectedLang.length() > 0) {
            cfg.language = selectedLang;
        }
        
        String manualSsid = _server.arg("wifi_ssid_manual");
        manualSsid.trim();
        String selSsid = _server.arg("wifi_ssid");
        selSsid.trim();
        
        if (manualSsid.length() > 0) {
            cfg.wifi_ssid = manualSsid;
        } else if (selSsid.length() > 0) {
            cfg.wifi_ssid = selSsid;
        }

        cfg.wifi_pass = _server.arg("wifi_pass");

        cfg.use_static_ip = (_server.arg("use_static_ip") == "true");
        cfg.static_ip = _server.arg("static_ip");
        cfg.gateway = _server.arg("gateway");
        cfg.subnet = _server.arg("subnet");
        cfg.dns = _server.arg("dns");

        cfg.octo_enabled = (_server.arg("octo_enabled") == "true");
        cfg.octo_ip = _server.arg("octo_ip");

        cfg.klipper_enabled = (_server.arg("klipper_enabled") == "true");
        cfg.klipper_ip = _server.arg("klipper_ip");
        int port = _server.arg("klipper_port").toInt();
        cfg.klipper_port = (port > 0) ? port : 7125;
        cfg.klipper_key = _server.arg("klipper_key");

        bool saved = ConfigManager::saveConfig(cfg);
        Serial.println("Mentes eredmenye az SD-re: " + String(saved ? "SIKER" : "HIBA"));

        // Azonnali érvényesítés újraindítás nélkül
        digitalWrite(15, HIGH);
        digitalWrite(5, LOW);
        SPI.setFrequency(4000000);
        LangManager::loadLanguage(cfg.language);
        digitalWrite(5, HIGH);

        if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
            IPAddress IP = WiFi.softAPIP();
            int currentStations = WiFi.softAPgetStationNum();
            _splash->showAPInfo("OctoScreen_Setup", "12345678", IP.toString(), currentStations > 0, _lastWifiError);
        }

        _server.send(200, "text/html; charset=utf-8", 
            "<html><body style='font-family:sans-serif; background:#0f172a; color:#fff; text-align:center; padding-top:50px;'>"
            "<h2>Sikeres mentés! A beállítások azonnal érvénybe léptek.</h2>"
            "<p style='color: #94a3b8; font-size: 14px;'>A főoldal hamarosan automatikusan betöltődik.</p>"
            "<script>"
            "  setTimeout(() => {"
            "    window.location.href = '/';"
            "  }, 2000);"
            "</script>"
            "</body></html>"
        );
    });

    _server.on("/restart", HTTP_POST, [this]() {
        Serial.println("\n--- TAVOLI UJRAINDITASI KERELES ---");
        _server.send(200, "text/html; charset=utf-8", "<html><body style='font-family:sans-serif; background:#0f172a; color:#fff; text-align:center; padding-top:50px;'><h2>Az eszköz újraindul...</h2></body></html>");
        delay(1000);
        ESP.restart();
    });

    _server.on("/format", HTTP_POST, [this]() {
        Serial.println("\n--- SD KARTYA TORLESI KERELES ---");
        digitalWrite(15, HIGH);
        digitalWrite(5, LOW);
        SPI.setFrequency(4000000);
        File root = SD.open("/");
        if (root && root.isDirectory()) {
            File file = root.openNextFile();
            while (file) {
                String fileName = file.name();
                bool isDir = file.isDirectory();
                file.close();
                if (!isDir) {
                    String fullPath = "/" + fileName;
                    Serial.println("Fajl torlese: " + fullPath);
                    SD.remove(fullPath);
                }
                file = root.openNextFile();
            }
            root.close();
        }
        digitalWrite(5, HIGH);

        PrinterConfig defaultCfg;
        ConfigManager::saveConfig(defaultCfg);

        _server.send(200, "text/html; charset=utf-8", "<html><body style='font-family:sans-serif; background:#0f172a; color:#fff; text-align:center; padding-top:50px;'><h2>SD kártya törölve és alaphelyzetbe állítva! Az eszköz újraindul...</h2></body></html>");
        delay(1000);
        ESP.restart();
    });
}

void APManager::handleClient() {
    _server.handleClient();

    if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
        static int lastStations = -1;
        int currentStations = WiFi.softAPgetStationNum();
        if (currentStations != lastStations) {
            lastStations = currentStations;
            IPAddress IP = WiFi.softAPIP();
            _splash->showAPInfo("OctoScreen_Setup", "12345678", IP.toString(), currentStations > 0, _lastWifiError);
        }
    }
}