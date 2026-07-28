#include "ProgramsPage.h"

#include "DisplayManager.h"
#include "Theme.h"

namespace
{
    void configureProgramPanel(lv_obj_t* object)
    {
        lv_obj_set_style_bg_color(object, Theme::panel(), 0);
        lv_obj_set_style_border_color(object, Theme::border(), 0);
        lv_obj_set_style_border_width(object, 1, 0);
        lv_obj_set_style_radius(object, Theme::CARD_RADIUS, 0);
        lv_obj_set_style_pad_all(object, 10, 0);
        lv_obj_clear_flag(object, LV_OBJ_FLAG_SCROLLABLE);
    }
}

void ProgramsPage::begin(
    lv_obj_t* parent,
    Scheduler& scheduler,
    DisplayManager& displayManager
)
{
    parent_ = parent;
    scheduler_ = &scheduler;
    displayManager_ = &displayManager;

    lv_obj_t* heading = lv_label_create(parent_);
    lv_label_set_text(heading, "Wochenprogramme");
    lv_obj_set_style_text_color(heading, Theme::text(), 0);
    lv_obj_set_pos(heading, 14, 8);

    rebuildProgramList();
}

void ProgramsPage::refresh()
{
    if (scheduler_ == nullptr)
    {
        return;
    }

    const uint8_t count = min(
        scheduler_->programCount(),
        MAX_VISIBLE_PROGRAMS
    );

    for (uint8_t i = 0; i < count; ++i)
    {
        ProgramWidgets& ui = widgets_[i];
        const auto& program = scheduler_->program(i);

        if (ui.enableSwitch == nullptr)
        {
            continue;
        }

        if (program.enabled)
        {
            lv_obj_add_state(
                ui.enableSwitch,
                LV_STATE_CHECKED
            );
        }
        else
        {
            lv_obj_clear_state(
                ui.enableSwitch,
                LV_STATE_CHECKED
            );
        }
    }
}

void ProgramsPage::rebuildProgramList()
{
    if (parent_ == nullptr || scheduler_ == nullptr)
    {
        return;
    }

    int y = 34;

    const uint8_t count = min(
        scheduler_->programCount(),
        MAX_VISIBLE_PROGRAMS
    );

    for (uint8_t i = 0; i < count; ++i)
    {
        const auto& program = scheduler_->program(i);

        char valveText[20];
        snprintf(
            valveText,
            sizeof(valveText),
            "Ventil %u",
            static_cast<unsigned>(program.valveIndex + 1)
        );

        char timeText[10];
        snprintf(
            timeText,
            sizeof(timeText),
            "%02u:%02u",
            static_cast<unsigned>(program.startHour),
            static_cast<unsigned>(program.startMinute)
        );

        char durationText[20];
        snprintf(
            durationText,
            sizeof(durationText),
            "%lu Minuten",
            static_cast<unsigned long>(
                program.durationSeconds / 60UL
            )
        );

        createProgramCard(
            widgets_[i],
            i + 1,
            valveText,
            timeText,
            durationText,
            y
        );

        y += 71;
    }

    refresh();
}

void ProgramsPage::createProgramCard(
    ProgramWidgets& ui,
    uint8_t number,
    const char* valveText,
    const char* timeText,
    const char* durationText,
    int y
)
{
    ui.owner = this;
    ui.programIndex = number - 1;

    ui.card = lv_obj_create(parent_);
    lv_obj_set_size(ui.card, 452, 62);
    lv_obj_set_pos(ui.card, 14, y);
    configureProgramPanel(ui.card);

    ui.title = lv_label_create(ui.card);
    lv_label_set_text_fmt(
        ui.title,
        "Programm %u",
        static_cast<unsigned>(number)
    );
    lv_obj_set_style_text_color(
        ui.title,
        Theme::text(),
        0
    );
    lv_obj_set_pos(ui.title, 0, 0);

    ui.details = lv_label_create(ui.card);
    lv_label_set_text_fmt(
        ui.details,
        "%s  |  %s  |  %s",
        valveText,
        timeText,
        durationText
    );
    lv_obj_set_style_text_color(
        ui.details,
        Theme::textDim(),
        0
    );
    lv_obj_set_pos(ui.details, 0, 28);

    ui.enableSwitch = lv_switch_create(ui.card);
    lv_obj_set_size(ui.enableSwitch, 52, 28);
    lv_obj_align(
        ui.enableSwitch,
        LV_ALIGN_RIGHT_MID,
        -2,
        0
    );

    lv_obj_add_event_cb(
        ui.enableSwitch,
        programSwitchEvent,
        LV_EVENT_VALUE_CHANGED,
        &ui
    );
}

void ProgramsPage::programSwitchEvent(lv_event_t* event)
{
    if (lv_event_get_code(event) != LV_EVENT_VALUE_CHANGED)
    {
        return;
    }

    auto* ui = static_cast<ProgramWidgets*>(
        lv_event_get_user_data(event)
    );

    if (ui == nullptr ||
        ui->owner == nullptr ||
        ui->owner->scheduler_ == nullptr)
    {
        return;
    }

    ProgramsPage& page = *ui->owner;

    if (ui->programIndex >= page.scheduler_->programCount())
    {
        return;
    }

    lv_obj_t* sw = static_cast<lv_obj_t*>(
        lv_event_get_target(event)
    );

    const bool enabled = lv_obj_has_state(
        sw,
        LV_STATE_CHECKED
    );

    const bool saved = page.scheduler_->setProgramEnabled(
        ui->programIndex,
        enabled
    );

    Serial.printf(
        "Programm %u: %s\n",
        static_cast<unsigned>(ui->programIndex + 1),
        enabled ? "aktiviert" : "deaktiviert"
    );

    if (page.displayManager_ == nullptr)
    {
        return;
    }

    if (saved)
    {
        page.displayManager_->showMessage(
            enabled
                ? "Programm aktiviert"
                : "Programm deaktiviert"
        );
    }
    else
    {
        page.displayManager_->showMessage(
            "Speichern fehlgeschlagen"
        );
    }
}