#include "main.h"
#include "config_manager.h"
#include "lang_manager.h"
#include "mainscreen.h"
#include "menuscreen.h"
#include "octo_client.h"
#include "klipper_client.h"
#include <CST820.h>

TFT_eSPI tft = TFT_eSPI();
CST820 touch(33, 32, -1, 4);

SplashScreen splash(&tft);
APManager apManager(&splash);

bool isOctoEnabled = false;
bool isKlipperEnabled = false;

OctoClient* octoClient = nullptr;
KlipperClient* klipperClient = nullptr;
MainScreen* mainScreen = nullptr;
MenuScreen* menuScreen = nullptr;

unsigned long lastPrinterUpdate = 0;
const unsigned long UPDATE_INTERVAL = 5000;

TaskHandle_t NetworkTask = nullptr;

void networkTaskCode(void * pvParameters) {
    for(;;) {
        ConnectionManager::update();

        unsigned long currentInterval = UPDATE_INTERVAL;
        if (octoClient && octoClient->isMeshBuilding()) {
            currentInterval = 1000;
        }

        if (millis() - lastPrinterUpdate > currentInterval) {
            if (isOctoEnabled && octoClient) octoClient->update();
            if (isKlipperEnabled && klipperClient) klipperClient->update();
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
    delay(500);

    // 1. Tápellátás bekapcsolása (27-es pin)[cite: 10]
    pinMode(27, OUTPUT);
    digitalWrite(27, LOW);
    delay(50);
    digitalWrite(27, HIGH);
    delay(100); 
    
    Wire.begin(33, 32);
    delay(50);

    // 2. ELŐSZÖR A SPLASH / TFT INICIALIZÁLÁSA[cite: 10]
    splash.init();
    splash.showMessage("System Init...", TFT_WHITE);
    splash.drawProgressBar(10);
    delay(150);

    // 3. SD KÁRTYA ÉS CONFIG[cite: 10]
    if (!SD.begin()) {
        Serial.println("SD Card initialization failed!");
        splash.showMessage("SD Card Error!", TFT_RED);
        delay(1500);
    }
    splash.drawProgressBar(30);
    delay(150);

    // 4. Konfiguráció és nyelv betöltése a JSON-ből[cite: 10]
    if (ConfigManager::init()) {
        PrinterConfig cfg = ConfigManager::loadConfig();
        LangManager::loadLanguage(cfg.language); 
    } else {
        LangManager::loadLanguage("hu"); 
    }

    splash.showMessage(LangManager::get("sys_loading"), TFT_WHITE);
    splash.drawProgressBar(40);
    delay(150);
    
    touch.begin(); 
    pinMode(4, INPUT_PULLUP);

    Logger::init();
    Logger::logSystem(LangManager::get("sys_clean_boot"));
    
    PrinterConfig cfg = ConfigManager::loadConfig();
    isOctoEnabled = cfg.octo_enabled;
    isKlipperEnabled = cfg.klipper_enabled;

    octoClient = new OctoClient(cfg.octo_ip, cfg.octo_key);
    klipperClient = new KlipperClient(cfg.klipper_ip);

    mainScreen = new MainScreen(&tft, &touch, isOctoEnabled, isKlipperEnabled, octoClient);
    menuScreen = new MenuScreen(&tft, &touch); 

    // --- RÉSZLETES BOOT ÉS ALRENDSZER ELLENŐRZÉS PROGRESS BARRAL ---[cite: 10]
    
    // 55%: Hálózati indítás (Gyors háttér WiFi csatlakozás kísérlet UI villanás nélkül)
    splash.showMessage(LangManager::get("sys_net_start"), TFT_ORANGE);
    splash.drawProgressBar(55);

    WiFi.mode(WIFI_STA);
    WiFi.begin(); // Csatlakozás az ESP32-ben mentett adatokkal a háttérben

    unsigned long wifiStart = millis();
    bool wifiConnected = false;
    while (millis() - wifiStart < 4000) { // Max 4 mp várakozás a háttérben
        if (WiFi.status() == WL_CONNECTED) {
            wifiConnected = true;
            break;
        }
        delay(100);
    }

    if (!wifiConnected) {
        // Ha nincs mentett WiFi vagy nem tud csatlakozni, átadjuk az irányítást az APManagernek (AP / Portal mód)
        apManager.begin();
        return; // Ha AP módba lépett, itt megállunk
    }

    // 70%: OctoPrint API kapcsolat ellenőrzése[cite: 10]
    splash.showMessage("OctoPrint API ellenőrzése...", TFT_WHITE);
    splash.drawProgressBar(70);
    if (isOctoEnabled && octoClient) {
        octoClient->update(); 
    }
    delay(300);

    // 85%: MQTT kapcsolat ellenőrzése[cite: 10]
    splash.showMessage("MQTT kapcsolat ellenőrzése...", TFT_WHITE);
    splash.drawProgressBar(85);
    delay(300);

    // 95%: OctoKlipscreenBridge plugin ellenőrzése a szerveren[cite: 10]
    splash.showMessage("Bridge plugin ellenőrzése...", TFT_WHITE);
    splash.drawProgressBar(95);
    if (isOctoEnabled && octoClient) {
        octoClient->checkPluginAvailability(); 
    }
    delay(300);

    // 100%: Teljesen letelt a progress bar[cite: 10]
    splash.showMessage("Rendszer kész.", TFT_GREEN);
    splash.drawProgressBar(100);
    delay(500);

    // --- HIBAKEZELÉS A PROGRESS BAR UTÁN ---[cite: 10]
    bool pluginMissing = (octoClient && octoClient->isPluginMissing());

    if (pluginMissing) {
        // Ha hiányzik a plugin, felrajzoljuk a hibaablakot és várunk érintésre
        splash.showConnectedInfo(WiFi.localIP().toString(), isOctoEnabled, isKlipperEnabled, true);
        Serial.println("[BOOT ERROR] Kritikus hiba / hiányzó plugin! Várakozás érintésre...");
        while (true) {
            uint16_t tx, ty;
            if (splash.getTouch(&tx, &ty)) {
                delay(200);
                break;
            }
            delay(50);
        }
    } else {
        // Nincs hiba -> semmilyen ablak nem villan be, azonnal ugrunk a főképernyőre![cite: 10]
    }

    // Átmenet a főképernyőre[cite: 10]
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
            // Oldal váltás
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