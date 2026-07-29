#include "Scheduler.h"

namespace
{
    constexpr const char* NVS_NAMESPACE = "scheduler";
    constexpr const char* KEY_VERSION = "version";
    constexpr const char* KEY_PROGRAMS = "programs";
}

void Scheduler::begin()
{
    preferences_.begin(NVS_NAMESPACE, false);

    if (!load())
    {
        Serial.println(
            "Keine gueltigen Programmdaten gefunden"
        );

        restoreDefaults();

        if (save())
        {
            Serial.println(
                "Standardprogramme gespeichert"
            );
        }
        else
        {
            Serial.println(
                "Fehler beim Speichern der Standardprogramme"
            );
        }
    }

    Serial.println("Scheduler initialisiert");
}

void Scheduler::update()
{
    /*
     * Die automatische Programmausführung wird später
     * ergänzt, sobald eine echte Uhrzeit vorhanden ist.
     */
}

Scheduler::IrrigationProgram& Scheduler::program(uint8_t index)
{
    static IrrigationProgram invalidProgram;

    if (!validProgramIndex(index))
    {
        return invalidProgram;
    }

    return programs_[index];
}

const Scheduler::IrrigationProgram& Scheduler::program(
    uint8_t index
) const
{
    static IrrigationProgram invalidProgram;

    if (!validProgramIndex(index))
    {
        return invalidProgram;
    }

    return programs_[index];
}

uint8_t Scheduler::programCount() const
{
    return MAX_PROGRAMS;
}

bool Scheduler::setProgramEnabled(
    uint8_t index,
    bool enabled
)
{
    if (!validProgramIndex(index))
    {
        return false;
    }

    if (programs_[index].enabled == enabled)
    {
        return true;
    }

    programs_[index].enabled = enabled;

    return save();
}

bool Scheduler::setValve(
    uint8_t programIndex,
    uint8_t valveIndex
)
{
    if (!validProgramIndex(programIndex) ||
        !validValveIndex(valveIndex))
    {
        return false;
    }

    if (programs_[programIndex].valveIndex == valveIndex)
    {
        return true;
    }

    programs_[programIndex].valveIndex = valveIndex;

    return save();
}

bool Scheduler::setStartTime(
    uint8_t programIndex,
    uint8_t hour,
    uint8_t minute
)
{
    if (!validProgramIndex(programIndex) ||
        hour > 23 ||
        minute > 59)
    {
        return false;
    }

    IrrigationProgram& selectedProgram =
        programs_[programIndex];

    if (selectedProgram.startHour == hour &&
        selectedProgram.startMinute == minute)
    {
        return true;
    }

    selectedProgram.startHour = hour;
    selectedProgram.startMinute = minute;

    return save();
}

bool Scheduler::setDurationMinutes(
    uint8_t programIndex,
    uint16_t minutes
)
{
    if (!validProgramIndex(programIndex) ||
        minutes < MIN_DURATION_MINUTES ||
        minutes > MAX_DURATION_MINUTES)
    {
        return false;
    }

    const uint32_t durationSeconds =
        static_cast<uint32_t>(minutes) * 60UL;

    if (programs_[programIndex].durationSeconds ==
        durationSeconds)
    {
        return true;
    }

    programs_[programIndex].durationSeconds =
        durationSeconds;

    return save();
}

uint16_t Scheduler::durationMinutes(
    uint8_t programIndex
) const
{
    if (!validProgramIndex(programIndex))
    {
        return 0;
    }

    return static_cast<uint16_t>(
        programs_[programIndex].durationSeconds / 60UL
    );
}

bool Scheduler::setWeekday(
    uint8_t programIndex,
    Weekday weekday,
    bool enabled
)
{
    if (!validProgramIndex(programIndex))
    {
        return false;
    }

    const uint8_t weekdayIndex =
        static_cast<uint8_t>(weekday);

    if (weekdayIndex > 6)
    {
        return false;
    }

    const uint8_t mask =
        static_cast<uint8_t>(1U << weekdayIndex);

    const bool currentlyEnabled =
        (programs_[programIndex].weekdays & mask) != 0;

    if (currentlyEnabled == enabled)
    {
        return true;
    }

    if (enabled)
    {
        programs_[programIndex].weekdays |= mask;
    }
    else
    {
        programs_[programIndex].weekdays &=
            static_cast<uint8_t>(~mask);
    }

    return save();
}

bool Scheduler::isWeekdayEnabled(
    uint8_t programIndex,
    Weekday weekday
) const
{
    if (!validProgramIndex(programIndex))
    {
        return false;
    }

    const uint8_t weekdayIndex =
        static_cast<uint8_t>(weekday);

    if (weekdayIndex > 6)
    {
        return false;
    }

    const uint8_t mask =
        static_cast<uint8_t>(1U << weekdayIndex);

    return
        (programs_[programIndex].weekdays & mask) != 0;
}

bool Scheduler::save()
{
    const size_t expectedSize = sizeof(programs_);

    const size_t written =
        preferences_.putBytes(
            KEY_PROGRAMS,
            programs_,
            expectedSize
        );

    if (written != expectedSize)
    {
        Serial.printf(
            "Scheduler speichern fehlgeschlagen: "
            "%u von %u Bytes\n",
            static_cast<unsigned>(written),
            static_cast<unsigned>(expectedSize)
        );

        return false;
    }

    preferences_.putUInt(
        KEY_VERSION,
        STORAGE_VERSION
    );

    Serial.println("Programmdaten gespeichert");

    return true;
}

bool Scheduler::load()
{
    const uint32_t storedVersion =
        preferences_.getUInt(KEY_VERSION, 0);

    if (storedVersion != STORAGE_VERSION)
    {
        Serial.printf(
            "Unbekannte Speicherversion: %lu\n",
            static_cast<unsigned long>(storedVersion)
        );

        return false;
    }

    const size_t expectedSize = sizeof(programs_);

    const size_t storedSize =
        preferences_.getBytesLength(KEY_PROGRAMS);

    if (storedSize != expectedSize)
    {
        Serial.printf(
            "Falsche Programmdatenlaenge: "
            "%u statt %u Bytes\n",
            static_cast<unsigned>(storedSize),
            static_cast<unsigned>(expectedSize)
        );

        return false;
    }

    const size_t loaded =
        preferences_.getBytes(
            KEY_PROGRAMS,
            programs_,
            expectedSize
        );

    if (loaded != expectedSize)
    {
        Serial.println(
            "Programmdaten konnten nicht geladen werden"
        );

        return false;
    }

    /*
     * Laufzeitwerte dürfen nach einem Neustart
     * nicht als aktiv übernommen werden.
     */
    for (uint8_t i = 0; i < MAX_PROGRAMS; ++i)
    {
        programs_[i].running = false;
        programs_[i].startedAtMs = 0;
    }

    Serial.println("Programmdaten geladen");

    return true;
}

void Scheduler::restoreDefaults()
{
    for (uint8_t i = 0; i < MAX_PROGRAMS; ++i)
    {
        programs_[i] = IrrigationProgram{};
    }

    // Programm 1:
    // Ventil 1, Montag bis Freitag, 06:30, 15 Minuten
    programs_[0].enabled = true;
    programs_[0].valveIndex = 0;
    programs_[0].startHour = 6;
    programs_[0].startMinute = 30;
    programs_[0].durationSeconds = 15UL * 60UL;
    programs_[0].weekdays =
        (1U << static_cast<uint8_t>(Weekday::Monday)) |
        (1U << static_cast<uint8_t>(Weekday::Tuesday)) |
        (1U << static_cast<uint8_t>(Weekday::Wednesday)) |
        (1U << static_cast<uint8_t>(Weekday::Thursday)) |
        (1U << static_cast<uint8_t>(Weekday::Friday));

    // Programm 2:
    // Ventil 2, Samstag und Sonntag, 20:00, 10 Minuten
    programs_[1].enabled = false;
    programs_[1].valveIndex = 1;
    programs_[1].startHour = 20;
    programs_[1].startMinute = 0;
    programs_[1].durationSeconds = 10UL * 60UL;
    programs_[1].weekdays =
        (1U << static_cast<uint8_t>(Weekday::Saturday)) |
        (1U << static_cast<uint8_t>(Weekday::Sunday));
}

bool Scheduler::validProgramIndex(uint8_t index) const
{
    return index < MAX_PROGRAMS;
}

bool Scheduler::validValveIndex(uint8_t index) const
{
    return index < VALVE_COUNT;
}
