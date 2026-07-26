#include <Arduino.h>
#include <lv_conf.h>
#include <esp32_smartdisplay.h>

#include "DisplayManager.h"
#include "ValveManager.h"
#include "Scheduler.h"

static ValveManager valveManager;
static DisplayManager displayManager;
static Scheduler scheduler;

void setup()
{
    Serial.begin(115200);
    delay(800);

    Serial.println();
    Serial.println("================================");
    Serial.println("GardenFlow Professional");
    Serial.println("ESP32-4827S043R");
    Serial.println("================================");

    valveManager.begin();
    scheduler.begin();
    displayManager.begin(valveManager);

    Serial.println("System bereit");
}

void loop()
{
    valveManager.update();
    scheduler.update();
    displayManager.update();

    delay(5);
}