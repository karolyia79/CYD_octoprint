#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>
#include <FS.h>   // SD fájlrendszer támogatáshoz

// Hogy a WebServer lássa a globális FS típust:
using fs::FS;

#include <WebServer.h>
#include <WiFi.h>
#include <SD.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <ArduinoJson.h>

#include "touch.h"
#include "logger.h"
#include "splashscreen.h"
#include "ap.h"
#include "connection.h"

void setup();
void loop();

#endif