#pragma once

#include <Arduino.h>
#include <Preferences.h>

class Scheduler
{
public:
    static constexpr uint8_t MAX_PROGRAMS = 16;
    static constexpr uint8_t VALVE_COUNT = 2;
    static constexpr uint16_t MIN_DURATION_MINUTES = 1;
    static constexpr uint16_t MAX_DURATION_MINUTES = 240;

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

        // 0 = Ventil 1, 1 = Ventil 2
        uint8_t valveIndex = 0;

        uint8_t startHour = 6;
        uint8_t startMinute = 0;

        // Bewässerungsdauer in Sekunden
        uint32_t durationSeconds = 15UL * 60UL;

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

    bool setProgramEnabled(uint8_t index, bool enabled);

    bool setValve(uint8_t programIndex, uint8_t valveIndex);

    bool setStartTime(
        uint8_t programIndex,
        uint8_t hour,
        uint8_t minute
    );

    bool setDurationMinutes(
        uint8_t programIndex,
        uint16_t minutes
    );

    uint16_t durationMinutes(uint8_t programIndex) const;

    bool setWeekday(
        uint8_t programIndex,
        Weekday weekday,
        bool enabled
    );

    bool isWeekdayEnabled(
        uint8_t programIndex,
        Weekday weekday
    ) const;

    bool save();
    bool load();
    void restoreDefaults();

private:
    static constexpr uint32_t STORAGE_VERSION = 1;

    Preferences preferences_;
    IrrigationProgram programs_[MAX_PROGRAMS];

    bool validProgramIndex(uint8_t index) const;
    bool validValveIndex(uint8_t index) const;
};
