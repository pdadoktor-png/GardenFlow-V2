#include "Scheduler.h"

void Scheduler::begin()
{
    /*
     * Zwei Beispielprogramme.
     * Sie werden noch nicht automatisch ausgeführt.
     */

    programs_[0].enabled = true;
    programs_[0].valveIndex = 0;
    programs_[0].startHour = 6;
    programs_[0].startMinute = 30;
    programs_[0].durationSeconds = 15 * 60;

    setWeekday(0, Weekday::Monday, true);
    setWeekday(0, Weekday::Tuesday, true);
    setWeekday(0, Weekday::Wednesday, true);
    setWeekday(0, Weekday::Thursday, true);
    setWeekday(0, Weekday::Friday, true);

    programs_[1].enabled = false;
    programs_[1].valveIndex = 1;
    programs_[1].startHour = 20;
    programs_[1].startMinute = 0;
    programs_[1].durationSeconds = 10 * 60;

    setWeekday(1, Weekday::Saturday, true);
    setWeekday(1, Weekday::Sunday, true);

    Serial.println("Scheduler initialisiert");
}

void Scheduler::update()
{
    /*
     * Noch keine automatische Ausführung.
     *
     * Später werden hier geprüft:
     * - aktuelle Uhrzeit
     * - aktueller Wochentag
     * - aktivierte Programme
     * - Start und Ende der Bewässerung
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

    const uint8_t mask = 1U << weekdayIndex;

    if (enabled)
    {
        programs_[programIndex].weekdays |= mask;
    }
    else
    {
        programs_[programIndex].weekdays &= ~mask;
    }

    return true;
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

    const uint8_t mask = 1U << weekdayIndex;

    return (programs_[programIndex].weekdays & mask) != 0;
}

bool Scheduler::validProgramIndex(uint8_t index) const
{
    return index < MAX_PROGRAMS;
}