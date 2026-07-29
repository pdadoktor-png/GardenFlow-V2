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

    lv_obj_t* valve1Button_ = nullptr;
    lv_obj_t* valve2Button_ = nullptr;

    lv_obj_t* hourValueLabel_ = nullptr;
    lv_obj_t* minuteValueLabel_ = nullptr;
    lv_obj_t* durationValueLabel_ = nullptr;

    uint8_t editedProgramIndex_ = 0;
    uint8_t draftValveIndex_ = 0;
    uint8_t draftHour_ = 0;
    uint8_t draftMinute_ = 0;
    uint16_t draftDurationMinutes_ = 1;

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

    void refreshEditorValues();
    void refreshValveButtons();

    lv_obj_t* createTextButton(
        lv_obj_t* parent,
        const char* text,
        int x,
        int y,
        int width,
        int height,
        lv_event_cb_t callback
    );

    lv_obj_t* createValueLabel(
        lv_obj_t* parent,
        int x,
        int y,
        int width
    );

    static void programSwitchEvent(lv_event_t* event);
    static void programCardEvent(lv_event_t* event);

    static void valve1Event(lv_event_t* event);
    static void valve2Event(lv_event_t* event);

    static void hourMinusEvent(lv_event_t* event);
    static void hourPlusEvent(lv_event_t* event);
    static void minuteMinusEvent(lv_event_t* event);
    static void minutePlusEvent(lv_event_t* event);
    static void durationMinusEvent(lv_event_t* event);
    static void durationPlusEvent(lv_event_t* event);

    static void editorSaveEvent(lv_event_t* event);
    static void editorCancelEvent(lv_event_t* event);
};
