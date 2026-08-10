#include "ap.h"
#include "lang_manager.h"

APManager::APManager(SplashScreen* splash) : _splash(splash), _server(80) {}

void APManager::begin() {
    PrinterConfig cfg = ConfigManager::loadConfig();

    Serial.println("\n--- CONFIG BETOLTVE ---");
    Serial.println("SSID: [" + cfg.wifi_ssid + "]");
    Serial.println("Jelszo hossza: " + String(cfg.wifi_pass.length()));
    Serial.println("OctoPrint engedelyezve: " + String(cfg.octo_enabled ? "IGEN" : "NEM"));
    Serial.println("Klipper engedelyezve: " + String(cfg.klipper_enabled ? "IGEN" : "NEM"));
    Serial.println("------------------------\n");

    if (cfg.wifi_ssid != "") {
        connectWiFi(cfg);
    } else {
        Serial.println("Nincs megadva SSID a configban -> AP mod inditasa.");
        startAPMode();
    }

    setupRoutes();
    _server.begin();
    Logger::logSystem("Webszerver elinditva.");
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
        delay(500);
        _splash->showConnectedInfo(WiFi.localIP().toString(), cfg.octo_enabled, cfg.klipper_enabled);
    } else {
        Serial.println("HIBA: Nem sikerult csatlakozni a Wi-Fi-hez! WiFi.status() kod: " + String(WiFi.status()));
        Logger::logError("Nem sikerult csatlakozni a Wi-Fi-hez, hiba kiirasa es AP modra valtas...");

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
                delay(300);
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

    delay(200);

    IPAddress IP = WiFi.softAPIP();
    
    Serial.print("AP Mod elinditva. Hozzalehet csatlakozni. IP: ");
    Serial.println(IP);

    Logger::logSystem("AP Mod (Scan kepes) inditva. IP: " + IP.toString());
    _splash->showAPInfo("OctoScreen_Setup", "12345678", IP.toString(), false, _lastWifiError);
}

void APManager::setupRoutes() {
    _server.on("/", HTTP_GET, [this]() {
        PrinterConfig cfg = ConfigManager::loadConfig();

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
        html += "function scanNetworks() {";
        html += "  let btn = document.getElementById('scan_btn');";
        html += "  btn.innerHTML = 'Keresés folyamatban...';";
        html += "  fetch('/scan').then(res => res.json()).then(data => {";
        html += "    let select = document.getElementById('ssid_select');";
        html += "    select.innerHTML = '';";
        html += "    data.forEach(net => {";
        html += "      let opt = document.createElement('option');";
        html += "      opt.value = net.ssid; opt.innerHTML = net.ssid + ' (' + net.rssi + ' dBm)';";
        html += "      select.appendChild(opt);";
        html += "    });";
        html += "    btn.innerHTML = 'Hálózatok Keresése';";
        html += "  }).catch(err => { btn.innerHTML = 'Hiba a keresésben!'; });";
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
        html += "  if(confirm('Biztosan újra szeretnéd indítani az eszközt?')) {";
        html += "    fetch('/restart', { method: 'POST' }).then(res => {";
        html += "      document.open();";
        html += "      document.write('<html><body style=\"background:#0f172a;color:#fff;text-align:center;padding-top:50px;font-family:sans-serif;\"><h2>Eszköz újraindul... Kérlek várj.</h2></body></html>');";
        html += "      document.close();";
        html += "      setTimeout(() => { location.reload(); }, 4000);";
        html += "    }).catch(err => { alert('Hiba történt a művelet közben!'); });";
        html += "  }";
        html += "}";
        html += "function formatSD() {";
        html += "  if(confirm('Biztosan törölni szeretnéd az SD kártya tartalmát? Minden beállítás és log fájl elveszik!')) {";
        html += "    fetch('/format', { method: 'POST' }).then(res => {";
        html += "      document.open();";
        html += "      document.write('<html><body style=\"background:#0f172a;color:#fff;text-align:center;padding-top:50px;font-family:sans-serif;\"><h2>SD kártya törölve. Újraindítás...</h2></body></html>');";
        html += "      document.close();";
        html += "      setTimeout(() => { location.reload(); }, 3000);";
        html += "    }).catch(err => { alert('Hiba történt a művelet közben!'); });";
        html += "  }";
        html += "}";
        html += "</script></head><body>";
        
        html += "<div class='container'>";
        html += "<h2>OctoScreen Konfiguráció</h2>";
        html += "<form action='/save' method='POST'>";

        html += "<div class='card'>";
        html += "<h3>Wi-Fi Beállítások</h3>";
        html += "<button type='button' id='scan_btn' class='btn-scan' onclick='scanNetworks()'>Hálózatok Keresése</button>";
        html += "<label>Kiválasztott SSID:</label>";
        html += "<select name='wifi_ssid' id='ssid_select'><option value='" + cfg.wifi_ssid + "'>" + (cfg.wifi_ssid == "" ? "-- Válassz vagy keresgélj --" : cfg.wifi_ssid) + "</option></select>";
        html += "<label>Vagy SSID kézzel:</label>";
        html += "<input type='text' name='wifi_ssid_manual' value='" + cfg.wifi_ssid + "'>";
        
        html += "<label>Wi-Fi Jelszó:</label>";
        html += "<input type='password' id='wifi_pass_input' name='wifi_pass' value='" + cfg.wifi_pass + "'>";
        html += "<div class='checkbox-group'>";
        html += "<input type='checkbox' id='show_pass_chk' onclick='togglePassword()'>";
        html += "<label for='show_pass_chk' style='margin-bottom:0;'>Jelszó mutatása</label>";
        html += "</div>";
        
        html += "<div class='checkbox-group'>";
        html += "<input type='checkbox' id='use_static_ip' name='use_static_ip' value='true' " + String(cfg.use_static_ip ? "checked" : "") + " onclick='toggleStatic()'>";
        html += "<label for='use_static_ip' style='margin-bottom:0;'>Statikus IP használata</label>";
        html += "</div>";

        html += "<div id='static_div' style='display:" + String(cfg.use_static_ip ? "block" : "none") + ";'>";
        html += "<label>IP Cím:</label><input type='text' name='static_ip' value='" + cfg.static_ip + "'>";
        html += "<label>Átjáró (Gateway):</label><input type='text' name='gateway' value='" + cfg.gateway + "'>";
        html += "<label>Alhálózati Maszk:</label><input type='text' name='subnet' value='" + cfg.subnet + "'>";
        html += "<label>DNS Szerver:</label><input type='text' name='dns' value='" + cfg.dns + "'>";
        html += "</div>";
        html += "</div>";

        html += "<div class='card'>";
        html += "<div class='checkbox-group'>";
        html += "<input type='checkbox' id='octo_enabled' name='octo_enabled' value='true' " + String(cfg.octo_enabled ? "checked" : "") + ">";
        html += "<label for='octo_enabled' style='margin-bottom:0; color:#38bdf8;'>OctoPrint Kapcsolat Engedélyezése</label>";
        html += "</div>";
        html += "<label>OctoPrint IP / Host:</label><input type='text' name='octo_ip' value='" + cfg.octo_ip + "'>";
        html += "<label>API Kulcs:</label><input type='text' name='octo_key' value='" + cfg.octo_key + "'>";
        html += "</div>";

        html += "<div class='card'>";
        html += "<div class='checkbox-group'>";
        html += "<input type='checkbox' id='klipper_enabled' name='klipper_enabled' value='true' " + String(cfg.klipper_enabled ? "checked" : "") + ">";
        html += "<label for='klipper_enabled' style='margin-bottom:0; color:#38bdf8;'>Klipper (Moonraker) Engedélyezése</label>";
        html += "</div>";
        html += "<label>Klipper IP / Host:</label><input type='text' name='klipper_ip' value='" + cfg.klipper_ip + "'>";
        html += "<label>Port (Moonraker):</label><input type='number' name='klipper_port' value='" + String(cfg.klipper_port) + "'>";
        html += "<label>API Kulcs (Opcionális):</label><input type='text' name='klipper_key' value='" + cfg.klipper_key + "'>";
        html += "</div>";

        html += "<input type='submit' value='Mentés és Újraindítás'>";
        html += "</form>";

        html += "<div class='card' style='margin-top: 20px; border: 1px solid #475569;'>";
        html += "<h3 style='color: #38bdf8; margin-top: 0;'>Karbantartás és Rendszer</h3>";
        
        html += "<p style='font-size: 13px; color: #94a3b8; margin-bottom: 10px;'>Az eszköz azonnali újraindítása.</p>";
        html += "<button type='button' class='btn-warning' onclick='restartDevice()'>Eszköz Újraindítása</button>";

        html += "<p style='font-size: 13px; color: #94a3b8; margin-top: 15px; margin-bottom: 10px;'>Az SD kártya teljes tartalmának törlése és alaphelyzetbe állítása.</p>";
        html += "<button type='button' class='btn-danger' onclick='formatSD()'>SD Kártya Törlése / Formázása</button>";
        html += "</div>";

        html += "</div></body></html>";

        _server.send(200, "text/html; charset=utf-8", html);
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
        PrinterConfig cfg;
        String manualSsid = _server.arg("wifi_ssid_manual");
        cfg.wifi_ssid = (manualSsid != "") ? manualSsid : _server.arg("wifi_ssid");
        cfg.wifi_pass = _server.arg("wifi_pass");

        cfg.use_static_ip = (_server.arg("use_static_ip") == "true");
        cfg.static_ip = _server.arg("static_ip");
        cfg.gateway = _server.arg("gateway");
        cfg.subnet = _server.arg("subnet");
        cfg.dns = _server.arg("dns");

        cfg.octo_enabled = (_server.arg("octo_enabled") == "true");
        cfg.octo_ip = _server.arg("octo_ip");
        cfg.octo_key = _server.arg("octo_key");

        cfg.klipper_enabled = (_server.arg("klipper_enabled") == "true");
        cfg.klipper_ip = _server.arg("klipper_ip");
        cfg.klipper_port = _server.arg("klipper_port").toInt();
        cfg.klipper_key = _server.arg("klipper_key");

        bool saved = ConfigManager::saveConfig(cfg);
        Serial.println("Mentes eredmenye az SD-re: " + String(saved ? "SIKER" : "HIBA"));

        _server.send(200, "text/html; charset=utf-8", 
            "<html><body style='font-family:sans-serif; background:#0f172a; color:#fff; text-align:center; padding-top:50px;'>"
            "<h2>Sikeres mentés! Az eszköz újraindul...</h2>"
            "<p style='color: #94a3b8; font-size: 14px;'>Kérlek várj, a főoldal hamarosan automatikusan betöltődik.</p>"
            "<script>"
            "  setTimeout(() => {"
            "    window.location.href = '/';"
            "  }, 5000);"
            "</script>"
            "</body></html>"
        );
        
        delay(1500);
        ESP.restart();
    });

    _server.on("/restart", HTTP_POST, [this]() {
        Serial.println("\n--- TAVOLI UJRAINDITASI KERELES ---");
        _server.send(200, "text/html; charset=utf-8", "<html><body style='font-family:sans-serif; background:#0f172a; color:#fff; text-align:center; padding-top:50px;'><h2>Az eszköz újraindul...</h2></body></html>");
        delay(1500);
        ESP.restart();
    });

    _server.on("/format", HTTP_POST, [this]() {
        Serial.println("\n--- SD KARTYA TORLESI KERELES ---");
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

        PrinterConfig defaultCfg;
        ConfigManager::saveConfig(defaultCfg);

        _server.send(200, "text/html; charset=utf-8", "<html><body style='font-family:sans-serif; background:#0f172a; color:#fff; text-align:center; padding-top:50px;'><h2>SD kártya törölve és alaphelyzetbe állítva! Az eszköz újraindul...</h2></body></html>");
        delay(1500);
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