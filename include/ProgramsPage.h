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

    lv_obj_t* editorOverlay_ = nullptr;
    lv_obj_t* editorPanel_ = nullptr;
    lv_obj_t* valveDropdown_ = nullptr;
    lv_obj_t* hourSpinbox_ = nullptr;
    lv_obj_t* minuteSpinbox_ = nullptr;
    lv_obj_t* durationSpinbox_ = nullptr;

    uint8_t editedProgramIndex_ = 0;

    void rebuildProgramList();

    void createProgramCard(
        ProgramWidgets& ui,
        uint8_t number,
        const char* valveText,
        const char* timeText,
        const char* durationText,
        int y
    );

    void updateProgramCard(uint8_t programIndex);

    void openEditor(uint8_t programIndex);
    void closeEditor();
    bool saveEditor();

    lv_obj_t* createEditorLabel(
        lv_obj_t* parent,
        const char* text,
        int x,
        int y
    );

    lv_obj_t* createSpinbox(
        lv_obj_t* parent,
        int x,
        int y,
        int width,
        int32_t minimum,
        int32_t maximum,
        int32_t value,
        uint8_t digitCount
    );

    static void programSwitchEvent(lv_event_t* event);
    static void programCardEvent(lv_event_t* event);
    static void editorSaveEvent(lv_event_t* event);
    static void editorCancelEvent(lv_event_t* event);
};
