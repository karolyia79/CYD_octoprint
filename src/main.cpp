#include "main.h"
#include "config_manager.h"
#include "lang_manager.h"
#include "mainscreen.h"
#include "menuscreen.h"
#include "octo_client.h"
#include "klipper_client.h"
#include "octo_client_mqtt.h"
#include "mqtt_monitor.h"
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <CST820.h>

TFT_eSPI tft = TFT_eSPI();
CST820 touch(33, 32, -1, 4);

SplashScreen splash(&tft);
APManager apManager(&splash);

bool isOctoEnabled = false;
bool isKlipperEnabled = false;

WiFiClient espClient;
PubSubClient mqttClient(espClient);
MqttMonitor mqttMonitor;

OctoClient* octoClient = nullptr;
OctoClientMqtt* octoMqtt = nullptr;
KlipperClient* klipperClient = nullptr;
MainScreen* mainScreen = nullptr;
MenuScreen* menuScreen = nullptr;

unsigned long lastPrinterUpdate = 0;
const unsigned long UPDATE_INTERVAL = 5000;

TaskHandle_t NetworkTask = nullptr;

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

        if (octoMqtt) {
            octoMqtt->update();
        }

        unsigned long currentInterval = UPDATE_INTERVAL;
        if (octoClient && octoClient->isMeshBuilding()) {
            currentInterval = 1000;
        }

        bool isMqttConnected = (octoMqtt && octoMqtt->isConnected());

        if (millis() - lastPrinterUpdate > currentInterval) {
            if (isOctoEnabled && octoClient && !isMqttConnected) {
                octoClient->update(); 
            }
            if (isKlipperEnabled && klipperClient) {
                klipperClient->update();
            }
            lastPrinterUpdate = millis(); 
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

enum AppState { STATE_MAIN, STATE_MENU };
AppState currentState = STATE_MAIN;

static int savedPage = 0;

void setup() {
    Serial.begin(115200);
    delay(200);

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

    if (!sdSuccess) {
        Serial.println("[BOOT ERROR] SD Card / Config initialization failed!");
        splash.showMessage("SD Card Error!", TFT_RED);
        delay(1000);
    } else {
        splash.showMessage("System Init...", TFT_WHITE);
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

    octoClient = new OctoClient(cfg.octo_ip, cfg.octo_key);
    klipperClient = new KlipperClient(cfg.klipper_ip);

    mqttClient.setCallback(mqttCallback);
    octoMqtt = new OctoClientMqtt(octoClient, &mqttClient, &mqttMonitor);

    mainScreen = new MainScreen(&tft, &touch, isOctoEnabled, isKlipperEnabled, octoClient, octoMqtt);
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
    }

    splash.showMessage(LangManager::get("boot_octo_check"), TFT_WHITE);
    splash.drawProgressBar(70);
    if (isOctoEnabled && octoClient) {
        octoClient->update(); 
    }

    splash.showMessage(LangManager::get("boot_mqtt_check"), TFT_WHITE);
    splash.drawProgressBar(85);
    bool mqttConnected = true;
    if (isOctoEnabled && octoMqtt) {
        octoMqtt->begin(cfg.octo_ip, 1883);
        
        unsigned long mqttStart = millis();
        while (millis() - mqttStart < 1500) {
            octoMqtt->update();
            if (octoMqtt->isConnected()) break;
            delay(50);
        }
        mqttConnected = octoMqtt->isConnected();
    }

    splash.showMessage(LangManager::get("boot_bridge_check"), TFT_WHITE);
    splash.drawProgressBar(95);
    if (isOctoEnabled && octoClient) {
        octoClient->checkPluginAvailability(); 
    }

    // Rendszer kész - Szabad memória kiírása a terminálba
    splash.showMessage(LangManager::get("boot_sys_ready"), TFT_GREEN);
    splash.drawProgressBar(100);
    delay(200);

    Serial.println("\n==============================================");
    Serial.printf("[SYSTEM MEMORY] Indulás utáni szabad heap: %u byte\n", ESP.getFreeHeap());
    Serial.printf("[SYSTEM MEMORY] Minimális szabad heap: %u byte\n", ESP.getMinFreeHeap());
    Serial.println("==============================================\n");

    bool pluginMissing = (octoClient && octoClient->isPluginMissing());
    bool mqttError = (isOctoEnabled && !pluginMissing && !mqttConnected);

    if (pluginMissing || mqttError) {
        splash.showConnectedInfo(WiFi.localIP().toString(), isOctoEnabled, isKlipperEnabled, pluginMissing, mqttConnected);
        while (true) {
            uint16_t tx, ty;
            if (splash.getTouch(&tx, &ty)) {
                delay(150);
                break;
            }
            delay(50);
        }
    }

    tft.fillScreen(TFT_BLACK);
    if (mainScreen) {
        mainScreen->init();
        
        if (isOctoEnabled) octoClient->update();
        if (isKlipperEnabled) klipperClient->update();
        mainScreen->draw(octoClient->getData(), klipperClient->getData());
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

    if (currentState == STATE_MAIN && mainScreen) {
        mainScreen->draw(octoClient->getData(), klipperClient->getData());

        int touchAction = mainScreen->handleTouch();

        if (touchAction == 1) { 
        } 
        else if (touchAction == 2) { 
            Serial.println("[ACTION] OctoPrint szüneteltetése");
        }
        else if (touchAction == 4) { 
            Serial.println("[ACTION] OctoPrint megszakítása");
        }
        else if (touchAction == 12) { 
            Serial.println("[ACTION] Klipper szüneteltetése");
        }
        else if (touchAction == 14) { 
            Serial.println("[ACTION] Klipper megszakítása");
        }
        else if (touchAction == 20) {
            Serial.println("[MENU ACTION] OctoPrint Filament / Temp menü");
        }
        else if (touchAction == 21) {
            Serial.println("[MENU ACTION] OctoPrint Speed / Flow menü");
        }
        else if (touchAction == 30) {
            Serial.println("[MENU ACTION] Klipper Makrók menü");
        }
        else if (touchAction == 31) {
            Serial.println("[MENU ACTION] Klipper Tune / Z-Offset menü");
        }
    }
    else if (currentState == STATE_MENU && menuScreen) {
        menuScreen->draw(isOctoEnabled, octoClient->getData().connected, false, 
                         isKlipperEnabled, klipperClient->getData().connected, false);
    }

    delay(10);
}