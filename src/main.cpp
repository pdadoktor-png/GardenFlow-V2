#include <Arduino.h>
#include <lv_conf.h>
#include <esp32_smartdisplay.h>

#include "DisplayManager.h"
#include "ValveManager.h"

static ValveManager valveManager;
static DisplayManager displayManager;

void setup()
{
    Serial.begin(115200);
    delay(800);

    Serial.println();
    Serial.println("================================");
    Serial.println("GardenFlow V3 Professional");
    Serial.println("ESP32-4827S043R");
    Serial.println("================================");

    valveManager.begin();
    displayManager.begin(valveManager);

    Serial.println("System bereit");
}

void loop()
{
    valveManager.update();
    displayManager.update();
    delay(5);
}
