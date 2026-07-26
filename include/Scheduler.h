#pragma once

#include <Arduino.h>

class Scheduler
{
public:
    static constexpr uint8_t MAX_PROGRAMS = 16;

    enum class Weekday : uint8_t
    {
        Monday = 0,
        Tuesday,
        Wednesday,
        Thursday,
        Friday,
        Saturday,
        Sunday
    };

    struct IrrigationProgram
    {
        bool enabled = false;

        // 0 = Ventil 1, 1 = Ventil 2 usw.
        uint8_t valveIndex = 0;

        uint8_t startHour = 6;
        uint8_t startMinute = 0;

        // Bewässerungsdauer in Sekunden
        uint32_t durationSeconds = 15 * 60;

        // Bit 0 = Montag bis Bit 6 = Sonntag
        uint8_t weekdays = 0;

        bool running = false;
        uint32_t startedAtMs = 0;
    };

    void begin();
    void update();

    IrrigationProgram& program(uint8_t index);
    const IrrigationProgram& program(uint8_t index) const;

    uint8_t programCount() const;

    bool setWeekday(
        uint8_t programIndex,
        Weekday weekday,
        bool enabled
    );

    bool isWeekdayEnabled(
        uint8_t programIndex,
        Weekday weekday
    ) const;

private:
    IrrigationProgram programs_[MAX_PROGRAMS];

    bool validProgramIndex(uint8_t index) const;
};