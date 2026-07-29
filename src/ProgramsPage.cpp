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

    void configureEditorButton(lv_obj_t* button)
    {
        lv_obj_set_style_radius(button, 8, 0);
        lv_obj_set_style_border_width(button, 1, 0);
        lv_obj_set_style_border_color(button, Theme::border(), 0);
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

        if (ui.enableSwitch != nullptr)
        {
            if (program.enabled)
            {
                lv_obj_add_state(ui.enableSwitch, LV_STATE_CHECKED);
            }
            else
            {
                lv_obj_clear_state(ui.enableSwitch, LV_STATE_CHECKED);
            }
        }

        updateProgramCard(i);
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
            "%u Minuten",
            static_cast<unsigned>(scheduler_->durationMinutes(i))
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
    lv_obj_add_flag(ui.card, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_add_event_cb(
        ui.card,
        programCardEvent,
        LV_EVENT_CLICKED,
        &ui
    );

    ui.title = lv_label_create(ui.card);
    lv_label_set_text_fmt(
        ui.title,
        "Programm %u",
        static_cast<unsigned>(number)
    );
    lv_obj_set_style_text_color(ui.title, Theme::text(), 0);
    lv_obj_set_pos(ui.title, 0, 0);

    ui.details = lv_label_create(ui.card);
    lv_label_set_text_fmt(
        ui.details,
        "%s  |  %s  |  %s",
        valveText,
        timeText,
        durationText
    );
    lv_obj_set_style_text_color(ui.details, Theme::textDim(), 0);
    lv_obj_set_pos(ui.details, 0, 28);

    ui.enableSwitch = lv_switch_create(ui.card);
    lv_obj_set_size(ui.enableSwitch, 52, 28);
    lv_obj_align(ui.enableSwitch, LV_ALIGN_RIGHT_MID, -2, 0);

    lv_obj_add_event_cb(
        ui.enableSwitch,
        programSwitchEvent,
        LV_EVENT_VALUE_CHANGED,
        &ui
    );
}

void ProgramsPage::updateProgramCard(uint8_t programIndex)
{
    if (scheduler_ == nullptr ||
        programIndex >= scheduler_->programCount() ||
        programIndex >= MAX_VISIBLE_PROGRAMS)
    {
        return;
    }

    ProgramWidgets& ui = widgets_[programIndex];

    if (ui.details == nullptr)
    {
        return;
    }

    const auto& program = scheduler_->program(programIndex);

    lv_label_set_text_fmt(
        ui.details,
        "Ventil %u  |  %02u:%02u  |  %u Minuten",
        static_cast<unsigned>(program.valveIndex + 1),
        static_cast<unsigned>(program.startHour),
        static_cast<unsigned>(program.startMinute),
        static_cast<unsigned>(scheduler_->durationMinutes(programIndex))
    );
}

void ProgramsPage::openEditor(uint8_t programIndex)
{
    if (parent_ == nullptr ||
        scheduler_ == nullptr ||
        programIndex >= scheduler_->programCount())
    {
        return;
    }

    closeEditor();
    editedProgramIndex_ = programIndex;

    const auto& program = scheduler_->program(programIndex);

    editorOverlay_ = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(editorOverlay_);
    lv_obj_set_size(editorOverlay_, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(editorOverlay_, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(editorOverlay_, LV_OPA_60, 0);
    lv_obj_clear_flag(editorOverlay_, LV_OBJ_FLAG_SCROLLABLE);

    editorPanel_ = lv_obj_create(editorOverlay_);
    lv_obj_set_size(editorPanel_, 440, 236);
    lv_obj_center(editorPanel_);
    configureProgramPanel(editorPanel_);

    lv_obj_t* title = lv_label_create(editorPanel_);
    lv_label_set_text_fmt(
        title,
        "Programm %u bearbeiten",
        static_cast<unsigned>(programIndex + 1)
    );
    lv_obj_set_style_text_color(title, Theme::text(), 0);
    lv_obj_set_pos(title, 10, 4);

    createEditorLabel(editorPanel_, "Ventil", 12, 42);
    valveDropdown_ = lv_dropdown_create(editorPanel_);
    lv_dropdown_set_options(valveDropdown_, "Ventil 1\nVentil 2");
    lv_dropdown_set_selected(valveDropdown_, program.valveIndex);
    lv_obj_set_size(valveDropdown_, 120, 38);
    lv_obj_set_pos(valveDropdown_, 12, 64);

    createEditorLabel(editorPanel_, "Startzeit", 154, 42);

    hourSpinbox_ = createSpinbox(
        editorPanel_, 154, 64, 64,
        0, 23, program.startHour, 2
    );

    lv_obj_t* separator = lv_label_create(editorPanel_);
    lv_label_set_text(separator, ":");
    lv_obj_set_style_text_color(separator, Theme::text(), 0);
    lv_obj_set_pos(separator, 224, 74);

    minuteSpinbox_ = createSpinbox(
        editorPanel_, 242, 64, 64,
        0, 59, program.startMinute, 2
    );

    createEditorLabel(editorPanel_, "Dauer", 326, 42);

    durationSpinbox_ = createSpinbox(
        editorPanel_, 326, 64, 88,
        Scheduler::MIN_DURATION_MINUTES,
        Scheduler::MAX_DURATION_MINUTES,
        scheduler_->durationMinutes(programIndex),
        3
    );

    lv_obj_t* minuteLabel = lv_label_create(editorPanel_);
    lv_label_set_text(minuteLabel, "Minuten");
    lv_obj_set_style_text_color(minuteLabel, Theme::textDim(), 0);
    lv_obj_set_pos(minuteLabel, 340, 106);

    lv_obj_t* hint = lv_label_create(editorPanel_);
    lv_label_set_text(hint, "Werte durch Wischen oder Antippen aendern.");
    lv_obj_set_style_text_color(hint, Theme::textDim(), 0);
    lv_obj_set_pos(hint, 12, 137);

    lv_obj_t* cancelButton = lv_btn_create(editorPanel_);
    lv_obj_set_size(cancelButton, 130, 44);
    lv_obj_set_pos(cancelButton, 84, 174);
    configureEditorButton(cancelButton);
    lv_obj_add_event_cb(
        cancelButton,
        editorCancelEvent,
        LV_EVENT_CLICKED,
        this
    );

    lv_obj_t* cancelLabel = lv_label_create(cancelButton);
    lv_label_set_text(cancelLabel, "Abbrechen");
    lv_obj_center(cancelLabel);

    lv_obj_t* saveButton = lv_btn_create(editorPanel_);
    lv_obj_set_size(saveButton, 130, 44);
    lv_obj_set_pos(saveButton, 228, 174);
    configureEditorButton(saveButton);
    lv_obj_add_event_cb(
        saveButton,
        editorSaveEvent,
        LV_EVENT_CLICKED,
        this
    );

    lv_obj_t* saveLabel = lv_label_create(saveButton);
    lv_label_set_text(saveLabel, "Speichern");
    lv_obj_center(saveLabel);
}

void ProgramsPage::closeEditor()
{
    if (editorOverlay_ != nullptr)
    {
        lv_obj_del(editorOverlay_);
    }

    editorOverlay_ = nullptr;
    editorPanel_ = nullptr;
    valveDropdown_ = nullptr;
    hourSpinbox_ = nullptr;
    minuteSpinbox_ = nullptr;
    durationSpinbox_ = nullptr;
}

bool ProgramsPage::saveEditor()
{
    if (scheduler_ == nullptr ||
        valveDropdown_ == nullptr ||
        hourSpinbox_ == nullptr ||
        minuteSpinbox_ == nullptr ||
        durationSpinbox_ == nullptr)
    {
        return false;
    }

    const uint8_t valveIndex = static_cast<uint8_t>(
        lv_dropdown_get_selected(valveDropdown_)
    );
    const uint8_t hour = static_cast<uint8_t>(
        lv_spinbox_get_value(hourSpinbox_)
    );
    const uint8_t minute = static_cast<uint8_t>(
        lv_spinbox_get_value(minuteSpinbox_)
    );
    const uint16_t durationMinutes = static_cast<uint16_t>(
        lv_spinbox_get_value(durationSpinbox_)
    );

    if (!scheduler_->setValve(editedProgramIndex_, valveIndex))
    {
        return false;
    }

    if (!scheduler_->setStartTime(editedProgramIndex_, hour, minute))
    {
        return false;
    }

    if (!scheduler_->setDurationMinutes(
            editedProgramIndex_,
            durationMinutes
        ))
    {
        return false;
    }

    updateProgramCard(editedProgramIndex_);
    return true;
}

lv_obj_t* ProgramsPage::createEditorLabel(
    lv_obj_t* parent,
    const char* text,
    int x,
    int y
)
{
    lv_obj_t* label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, Theme::text(), 0);
    lv_obj_set_pos(label, x, y);
    return label;
}

lv_obj_t* ProgramsPage::createSpinbox(
    lv_obj_t* parent,
    int x,
    int y,
    int width,
    int32_t minimum,
    int32_t maximum,
    int32_t value,
    uint8_t digitCount
)
{
    lv_obj_t* spinbox = lv_spinbox_create(parent);
    lv_spinbox_set_range(spinbox, minimum, maximum);
    lv_spinbox_set_digit_format(spinbox, digitCount, 0);
    lv_spinbox_set_step(spinbox, 1);
    lv_spinbox_set_value(spinbox, value);
    lv_obj_set_size(spinbox, width, 38);
    lv_obj_set_pos(spinbox, x, y);
    return spinbox;
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

    const bool enabled = lv_obj_has_state(sw, LV_STATE_CHECKED);

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

    page.displayManager_->showMessage(
        saved
            ? (enabled ? "Programm aktiviert" : "Programm deaktiviert")
            : "Speichern fehlgeschlagen"
    );
}

void ProgramsPage::programCardEvent(lv_event_t* event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED)
    {
        return;
    }

    auto* ui = static_cast<ProgramWidgets*>(
        lv_event_get_user_data(event)
    );

    if (ui != nullptr && ui->owner != nullptr)
    {
        ui->owner->openEditor(ui->programIndex);
    }
}

void ProgramsPage::editorSaveEvent(lv_event_t* event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED)
    {
        return;
    }

    auto* page = static_cast<ProgramsPage*>(
        lv_event_get_user_data(event)
    );

    if (page == nullptr)
    {
        return;
    }

    const bool saved = page->saveEditor();

    if (page->displayManager_ != nullptr)
    {
        page->displayManager_->showMessage(
            saved ? "Programm gespeichert" : "Speichern fehlgeschlagen"
        );
    }

    if (saved)
    {
        page->closeEditor();
    }
}

void ProgramsPage::editorCancelEvent(lv_event_t* event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED)
    {
        return;
    }

    auto* page = static_cast<ProgramsPage*>(
        lv_event_get_user_data(event)
    );

    if (page != nullptr)
    {
        page->closeEditor();
    }
}
