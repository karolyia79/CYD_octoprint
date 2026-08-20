#include "main.h"
#include "config_manager.h"
#include "lang_manager.h"
#include "mainscreen.h"
#include "menuscreen.h"
#include "klipper_client.h"
#include "octo_client_mqtt.h"
#include "mqtt_monitor.h"
#include "rgb_led.h"
#include "screensaver.h"
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <CST820.h>
#include <Preferences.h>

TFT_eSPI tft = TFT_eSPI();
CST820 touch(33, 32, -1, 4);

SplashScreen splash(&tft);
APManager apManager(&splash);
RgbLed rgbLed;
Screensaver screensaver(&tft);

bool isOctoEnabled = false;
bool isKlipperEnabled = false;

WiFiClient espClient;
PubSubClient mqttClient(espClient);
MqttMonitor mqttMonitor;

OctoClientMqtt* octoMqtt = nullptr;
KlipperClient* klipperClient = nullptr;
MainScreen* mainScreen = nullptr;
MenuScreen* menuScreen = nullptr;

unsigned long lastKlipperUpdate = 0;
const unsigned long KLIPPER_UPDATE_INTERVAL = 5000;

TaskHandle_t NetworkTask = nullptr;

// Kijelző alvás, screensaver és fényerő kezelő változók
unsigned long lastActivityTime = 0;
bool isScreenAsleep = false;
bool isScreensaverActive = false;

void setBacklightBrightness(int percent) {
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    int duty = map(percent, 0, 100, 0, 255);
    ledcAttach(27, 5000, 8);
    ledcWrite(27, duty);
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String msg;
    for (unsigned int i = 0; i < length; i++) {
        msg += (char)payload[i];
    }
    mqttMonitor.processMessage(String(topic), msg);
}

void networkTaskCode(void * pvParameters) {
    for(;;) {
        ConnectionManager::update();

        // MQTT frissítés ciklus
        if (octoMqtt) {
            octoMqtt->update();
        }
        
        bool isWifiOk = (WiFi.status() == WL_CONNECTED);
        bool isMqttOk = octoMqtt ? octoMqtt->isConnected() : false;
        bool isApiOk  = octoMqtt ? octoMqtt->getData().apiConnected : false;
        bool printing = octoMqtt ? octoMqtt->getData().printingActive : false;
        bool paused   = octoMqtt ? (octoMqtt->getData().status == "Paused" || octoMqtt->getData().status == "Szüneteltetve") : false;
        bool heating  = octoMqtt ? (octoMqtt->getData().nozzleTarget > 0 && octoMqtt->getData().nozzleTemp < octoMqtt->getData().nozzleTarget - 2.0f) : false;
        bool homing   = octoMqtt ? octoMqtt->isHoming() : false;
        bool mesh     = octoMqtt ? octoMqtt->isMeshBuilding() : false;

        rgbLed.updateStatus(isWifiOk, isMqttOk, isApiOk, printing, paused, heating, homing, mesh, false);
        
        if (millis() - lastKlipperUpdate > KLIPPER_UPDATE_INTERVAL) {
            if (isKlipperEnabled && klipperClient) {
                klipperClient->update();
            }
            lastKlipperUpdate = millis(); 
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

enum AppState { STATE_MAIN, STATE_MENU };
AppState currentState = STATE_MAIN;

void setup() {
    Serial.begin(115200);
    delay(200);
    rgbLed.init();

    Serial.println("\n==============================================");
    Serial.println("       OCTOKLIPSCREEN BOOT INDITASA           ");
    Serial.println("==============================================\n");

    pinMode(27, OUTPUT);
    digitalWrite(27, LOW);
    delay(30);
    digitalWrite(27, HIGH);
    delay(50); 
    
    Wire.begin(33, 32);

    pinMode(15, OUTPUT);
    digitalWrite(15, HIGH);
    pinMode(5, OUTPUT);
    digitalWrite(5, HIGH);

    splash.init();
    
    bool sdSuccess = ConfigManager::init();
    PrinterConfig cfg = ConfigManager::loadConfig(); 

    // Mentett LED állapot, fényerő és alvási időzítő inicializálása
    rgbLed.setEnabled(cfg.led_enabled);
    setBacklightBrightness(cfg.screen_brightness);
    lastActivityTime = millis();

    if (!sdSuccess) {
        Serial.println("[BOOT ERROR] SD Card / Config initialization failed!");
        splash.showMessage(LangManager::get("sys_sd_error"), TFT_RED);
        delay(1000);
    } else {
        splash.showMessage(LangManager::get("sys_init"), TFT_WHITE);
        splash.drawProgressBar(20);
    }

    splash.showMessage(LangManager::get("sys_loading"), TFT_WHITE);
    splash.drawProgressBar(40);
    
    touch.begin(); 
    pinMode(4, INPUT_PULLUP);

    Logger::init();
    Logger::logSystem(LangManager::get("sys_clean_boot"));
    
    isOctoEnabled = cfg.octo_enabled;
    isKlipperEnabled = cfg.klipper_enabled;

    klipperClient = new KlipperClient(cfg.klipper_ip);

    mqttClient.setCallback(mqttCallback);
    octoMqtt = new OctoClientMqtt(&mqttClient, &mqttMonitor);
    
    // REST API konfiguráció átadása
    octoMqtt->setApiConfig(cfg.octo_ip, cfg.octo_key);

    mainScreen = new MainScreen(&tft, &touch, isOctoEnabled, isKlipperEnabled, octoMqtt);
    menuScreen = new MenuScreen(&tft, &touch); 

    splash.showMessage(LangManager::get("sys_net_start"), TFT_ORANGE);
    splash.drawProgressBar(55);

    bool wifiConnected = false;

    if (cfg.wifi_ssid != "") {
        Serial.println("[NET] Csatlakozás megkísérlése: '" + cfg.wifi_ssid + "'...");
        WiFi.disconnect(true);
        delay(50);
        WiFi.mode(WIFI_STA);
        
        if (cfg.use_static_ip && cfg.static_ip != "") {
            IPAddress ip, gateway, subnet, dns;
            if (ip.fromString(cfg.static_ip) && gateway.fromString(cfg.gateway) &&
                subnet.fromString(cfg.subnet) && dns.fromString(cfg.dns)) {
                WiFi.config(ip, gateway, subnet, dns);
            }
        }
        
        WiFi.begin(cfg.wifi_ssid.c_str(), cfg.wifi_pass.c_str());

        unsigned long wifiStart = millis();
        while (millis() - wifiStart < 8000) { 
            if (WiFi.status() == WL_CONNECTED) {
                wifiConnected = true;
                break;
            }
            delay(100);
        }
    } else {
        Serial.println("[NET] Nincs elmentve SSID!");
    }

    if (!wifiConnected) {
        Serial.println("[NET HIBA] Wi-Fi csatlakozás sikertelen! Átváltás AP módba...");
        apManager.begin(); 
        return; 
    } else {
        Serial.println("[NET SIKER] Csatlakozva a Wi-Fi hálózathoz! IP: " + WiFi.localIP().toString());
        apManager.startServer();
        configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "hu.pool.ntp.org", "pool.ntp.org", "time.google.com");
        Serial.println("[NTP] Időszerver szinkronizáció elindítva (hu.pool.ntp.org)...");
    }

    splash.showMessage(LangManager::get("boot_mqtt_check"), TFT_WHITE);
    splash.drawProgressBar(85);
    bool mqttConnected = false;
    if (isOctoEnabled && octoMqtt) {
        octoMqtt->begin(cfg.octo_ip, 1883);
        
        unsigned long mqttStart = millis();
        while (millis() - mqttStart < 2000) {
            octoMqtt->update();
            if (octoMqtt->isConnected()) break;
            delay(50);
        }
        mqttConnected = octoMqtt->isConnected();
    }

    splash.showMessage(LangManager::get("boot_sys_ready"), TFT_GREEN);
    splash.drawProgressBar(100);
    delay(200);

    Serial.println("\n==============================================");
    Serial.printf("[SYSTEM MEMORY] Indulás utáni szabad heap: %u byte\n", ESP.getFreeHeap());
    Serial.printf("[SYSTEM MEMORY] Minimális szabad heap: %u byte\n", ESP.getMinFreeHeap());
    Serial.println("==============================================\n");

    splash.handleCrashRecovery(octoMqtt, &touch);

    if (isOctoEnabled && !mqttConnected) {
        splash.showConnectedInfo(WiFi.localIP().toString(), isOctoEnabled, isKlipperEnabled, false, mqttConnected);
        while (true) {
            uint16_t raw_x = 0, raw_y = 0;
            uint8_t gesture = 0;
            if (touch.getTouch(&raw_x, &raw_y, &gesture)) {
                delay(150);
                break;
            }
            delay(50);
        }
    }

    tft.fillScreen(TFT_BLACK);
    if (mainScreen) {
        mainScreen->init();
        if (isKlipperEnabled) klipperClient->update();
        mainScreen->draw(octoMqtt->getData(), klipperClient->getData());
    }

    xTaskCreatePinnedToCore(
      networkTaskCode, 
      "NetworkTask",   
      10000,           
      NULL,            
      1,               
      &NetworkTask,    
      0);              
}

void loop() {
    apManager.handleClient();

    PrinterConfig cfg = ConfigManager::loadConfig();

    uint16_t touchX = 0, touchY = 0;
    uint8_t touchGesture = 0;
    bool touched = touch.getTouch(&touchX, &touchY, &touchGesture);

    // Képernyő érintés észlelése bárhol a kijelzőn -> Aktivitási idő frissítése / Ébresztés
    if (touched) {
        lastActivityTime = millis();

        if (isScreenAsleep || isScreensaverActive) {
            isScreenAsleep = false;
            isScreensaverActive = false;
            
            // Fényerő visszaállítása a beállított szintre
            setBacklightBrightness(cfg.screen_brightness);

            // Megvárjuk, amíg elengeded a kijelzőt, így az ébresztő touch nem nyom meg gombot!
            while (touch.getTouch(&touchX, &touchY, &touchGesture)) {
                delay(10);
            }
            delay(50);

            // Képernyő kényszerített újrarajzolása
            tft.fillScreen(TFT_BLACK);
            if (currentState == STATE_MAIN && mainScreen) {
                mainScreen->init();
                mainScreen->draw(octoMqtt->getData(), klipperClient->getData());
            } else if (currentState == STATE_MENU && menuScreen) {
                menuScreen->draw(isOctoEnabled, octoMqtt->isConnected(), false, 
                                 isKlipperEnabled, klipperClient->getData().connected, false);
            }
            return;
        }
    }

    // Inaktivitási időzítő ellenőrzése
    if (cfg.screen_mode != "off" && !isScreenAsleep && !isScreensaverActive) {
        if (millis() - lastActivityTime >= (unsigned long)cfg.screen_timeout * 1000) {
            if (cfg.screen_mode == "sleep") {
                isScreenAsleep = true;
                setBacklightBrightness(0); // Lekapcsolt háttérvilágítás
            } 
            else if (cfg.screen_mode == "saver") {
                isScreensaverActive = true;
                setBacklightBrightness(map(cfg.screen_brightness, 0, 100, 0, 255) / 2); // Kímélő 50% fényerő
                screensaver.init();
            }
        }
    }

    // Ha alvó módban van a képernyő, kihagyjuk a rajzolást és az UI gombok kezelését
    if (isScreenAsleep) {
        delay(20);
        return;
    }

    // Ha a screensaver aktív, kizárólag az Xperia óra/haladás rajzolódik
    if (isScreensaverActive) {
        screensaver.draw(octoMqtt);
        delay(30);
        return;
    }

    if (currentState == STATE_MAIN && mainScreen) {
        mainScreen->draw(octoMqtt->getData(), klipperClient->getData());

        int touchAction = mainScreen->handleTouch();

        if (touchAction == 2) { 
            Serial.println("[ACTION] OctoPrint szüneteltetése (REST API)");
        }
        else if (touchAction == 4) { 
            Serial.println("[ACTION] OctoPrint megszakítása (REST API)");
        }
        else if (touchAction == 12) { 
            Serial.println("[ACTION] Klipper szüneteltetése");
        }
        else if (touchAction == 14) { 
            Serial.println("[ACTION] Klipper megszakítása");
        }
    }
    else if (currentState == STATE_MENU && menuScreen) {
        menuScreen->draw(isOctoEnabled, octoMqtt->isConnected(), false, 
                         isKlipperEnabled, klipperClient->getData().connected, false);
    }

    delay(10);
}