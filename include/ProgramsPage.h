#pragma once

#include <Arduino.h>
#include <lv_conf.h>
#include <esp32_smartdisplay.h>

#include "Scheduler.h"

class DisplayManager;

class ProgramsPage
{
public:
    void begin(
        lv_obj_t* parent,
        Scheduler& scheduler,
        DisplayManager& displayManager
    );

    void refresh();

private:
    static constexpr uint8_t MAX_VISIBLE_PROGRAMS = 16;

    struct ProgramWidgets
    {
        ProgramsPage* owner = nullptr;
        lv_obj_t* card = nullptr;
        lv_obj_t* title = nullptr;
        lv_obj_t* details = nullptr;
        lv_obj_t* enableSwitch = nullptr;
        uint8_t programIndex = 0;
    };

    lv_obj_t* parent_ = nullptr;
    Scheduler* scheduler_ = nullptr;
    DisplayManager* displayManager_ = nullptr;

    ProgramWidgets widgets_[MAX_VISIBLE_PROGRAMS];

    void rebuildProgramList();

    void createProgramCard(
        ProgramWidgets& ui,
        uint8_t number,
        const char* valveText,
        const char* timeText,
        const char* durationText,
        int y
    );

    static void programSwitchEvent(lv_event_t* event);
};