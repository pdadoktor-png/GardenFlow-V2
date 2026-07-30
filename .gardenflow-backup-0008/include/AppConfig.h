#pragma once
#include <Arduino.h>

namespace AppConfig
{
    constexpr uint8_t DISPLAYED_VALVE_COUNT = 2;
    constexpr uint8_t MAX_VALVE_COUNT = 8;
    constexpr uint32_t DEFAULT_PULSE_MS = 250;
    constexpr uint32_t BUTTON_LOCKOUT_MS = 500;
    constexpr float BACKLIGHT = 0.80f;

    // -1 = Simulationsbetrieb. Ventilspulen nie direkt an GPIO anschliessen.
    constexpr int8_t VALVE_GPIO[MAX_VALVE_COUNT] =
    {
        -1, -1, -1, -1, -1, -1, -1, -1
    };
}
