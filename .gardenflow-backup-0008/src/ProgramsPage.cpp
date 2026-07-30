#include "ProgramsPage.h"

#include "DisplayManager.h"
#include "Theme.h"

namespace
{
    constexpr const char* WEEKDAY_NAMES[7] =
    {
        "Mo", "Di", "Mi", "Do", "Fr", "Sa", "So"
    };

    void configurePanel(lv_obj_t* object)
    {
        lv_obj_set_style_bg_color(object, Theme::panel(), 0);
        lv_obj_set_style_border_color(object, Theme::border(), 0);
        lv_obj_set_style_border_width(object, 1, 0);
        lv_obj_set_style_radius(object, Theme::CARD_RADIUS, 0);
        lv_obj_set_style_pad_all(object, 8, 0);
        lv_obj_clear_flag(object, LV_OBJ_FLAG_SCROLLABLE);
    }

    void configureButton(lv_obj_t* button)
    {
        lv_obj_set_style_radius(button, 8, 0);
        lv_obj_set_style_border_width(button, 1, 0);
        lv_obj_set_style_border_color(button, Theme::border(), 0);
        lv_obj_set_style_bg_color(button, Theme::panel(), 0);
        lv_obj_set_style_bg_color(
            button,
            lv_palette_main(LV_PALETTE_GREEN),
            LV_STATE_CHECKED
        );
    }

    void setLabelTextColor(lv_obj_t* label)
    {
        lv_obj_set_style_text_color(label, Theme::text(), 0);
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
    setLabelTextColor(heading);
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
            static_cast<unsigned>(
                scheduler_->durationMinutes(i)
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
    configurePanel(ui.card);
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
    setLabelTextColor(ui.title);
    lv_obj_set_pos(ui.title, 0, 0);

    ui.details = lv_label_create(ui.card);
    lv_obj_set_width(ui.details, 360);
    lv_label_set_long_mode(ui.details, LV_LABEL_LONG_CLIP);
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
        static_cast<unsigned>(
            scheduler_->durationMinutes(programIndex)
        )
    );
}

void ProgramsPage::openEditor(uint8_t programIndex)
{
    if (scheduler_ == nullptr ||
        programIndex >= scheduler_->programCount())
    {
        return;
    }

    closeEditor();

    editedProgramIndex_ = programIndex;

    const auto& program = scheduler_->program(programIndex);
    draftValveIndex_ = program.valveIndex;
    draftHour_ = program.startHour;
    draftMinute_ = program.startMinute;
    draftDurationMinutes_ = scheduler_->durationMinutes(programIndex);
    draftWeekdays_ = program.weekdays;

    editorOverlay_ = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(editorOverlay_);
    lv_obj_set_size(editorOverlay_, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(editorOverlay_, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(editorOverlay_, LV_OPA_60, 0);
    lv_obj_clear_flag(editorOverlay_, LV_OBJ_FLAG_SCROLLABLE);

    editorPanel_ = lv_obj_create(editorOverlay_);
    lv_obj_set_size(editorPanel_, 452, 246);
    lv_obj_center(editorPanel_);
    configurePanel(editorPanel_);

    lv_obj_t* title = lv_label_create(editorPanel_);
    lv_label_set_text_fmt(
        title,
        "Programm %u bearbeiten",
        static_cast<unsigned>(programIndex + 1)
    );
    setLabelTextColor(title);
    lv_obj_set_pos(title, 6, 0);

    lv_obj_t* valveLabel = lv_label_create(editorPanel_);
    lv_label_set_text_fmt(
        valveLabel,
        "Ventil %u",
        static_cast<unsigned>(draftValveIndex_ + 1)
    );
    setLabelTextColor(valveLabel);
    lv_obj_set_pos(valveLabel, 6, 31);

    lv_obj_t* valveHint = lv_label_create(editorPanel_);
    lv_label_set_text(
        valveHint,
        "Ventil ist durch das gewählte Programm festgelegt"
    );
    lv_obj_set_style_text_color(valveHint, Theme::textDim(), 0);
    lv_obj_set_pos(valveHint, 82, 31);

    lv_obj_t* timeLabel = lv_label_create(editorPanel_);
    lv_label_set_text(timeLabel, "Start");
    setLabelTextColor(timeLabel);
    lv_obj_set_pos(timeLabel, 6, 75);

    createTextButton(
        editorPanel_, "-", 70, 64, 42, 38, hourMinusEvent
    );
    hourValueLabel_ = createValueLabel(editorPanel_, 116, 72, 42);
    createTextButton(
        editorPanel_, "+", 162, 64, 42, 38, hourPlusEvent
    );

    lv_obj_t* colon = lv_label_create(editorPanel_);
    lv_label_set_text(colon, ":");
    setLabelTextColor(colon);
    lv_obj_set_pos(colon, 214, 75);

    createTextButton(
        editorPanel_, "-", 230, 64, 42, 38, minuteMinusEvent
    );
    minuteValueLabel_ = createValueLabel(editorPanel_, 276, 72, 42);
    createTextButton(
        editorPanel_, "+", 322, 64, 42, 38, minutePlusEvent
    );

    lv_obj_t* durationLabel = lv_label_create(editorPanel_);
    lv_label_set_text(durationLabel, "Dauer");
    setLabelTextColor(durationLabel);
    lv_obj_set_pos(durationLabel, 6, 117);

    createTextButton(
        editorPanel_, "-", 70, 106, 42, 38, durationMinusEvent
    );
    durationValueLabel_ = createValueLabel(
        editorPanel_, 116, 114, 66
    );
    createTextButton(
        editorPanel_, "+", 186, 106, 42, 38, durationPlusEvent
    );

    lv_obj_t* minutesLabel = lv_label_create(editorPanel_);
    lv_label_set_text(minutesLabel, "Min.");
    lv_obj_set_style_text_color(minutesLabel, Theme::textDim(), 0);
    lv_obj_set_pos(minutesLabel, 238, 117);

    lv_obj_t* weekdayLabel = lv_label_create(editorPanel_);
    lv_label_set_text(weekdayLabel, "Tage");
    setLabelTextColor(weekdayLabel);
    lv_obj_set_pos(weekdayLabel, 6, 158);

    for (uint8_t i = 0; i < WEEKDAY_COUNT; ++i)
    {
        weekdayContexts_[i].owner = this;
        weekdayContexts_[i].weekdayIndex = i;

        weekdayButtons_[i] = createTextButton(
            editorPanel_,
            WEEKDAY_NAMES[i],
            54 + (i * 54),
            148,
            48,
            36,
            weekdayEvent,
            &weekdayContexts_[i]
        );
    }

    createTextButton(
        editorPanel_,
        "Abbrechen",
        74,
        194,
        136,
        40,
        editorCancelEvent
    );

    createTextButton(
        editorPanel_,
        "Speichern",
        238,
        194,
        136,
        40,
        editorSaveEvent
    );

    refreshEditorValues();
    refreshWeekdayButtons();
}

void ProgramsPage::closeEditor()
{
    if (editorOverlay_ != nullptr)
    {
        lv_obj_del(editorOverlay_);
    }

    editorOverlay_ = nullptr;
    editorPanel_ = nullptr;
    hourValueLabel_ = nullptr;
    minuteValueLabel_ = nullptr;
    durationValueLabel_ = nullptr;

    for (uint8_t i = 0; i < WEEKDAY_COUNT; ++i)
    {
        weekdayButtons_[i] = nullptr;
    }
}

bool ProgramsPage::saveEditor()
{
    if (scheduler_ == nullptr)
    {
        return false;
    }

    if (!scheduler_->setStartTime(
            editedProgramIndex_,
            draftHour_,
            draftMinute_
        ))
    {
        return false;
    }

    if (!scheduler_->setDurationMinutes(
            editedProgramIndex_,
            draftDurationMinutes_
        ))
    {
        return false;
    }

    for (uint8_t i = 0; i < WEEKDAY_COUNT; ++i)
    {
        const bool enabled =
            (draftWeekdays_ & static_cast<uint8_t>(1U << i)) != 0;

        if (!scheduler_->setWeekday(
                editedProgramIndex_,
                static_cast<Scheduler::Weekday>(i),
                enabled
            ))
        {
            return false;
        }
    }

    updateProgramCard(editedProgramIndex_);
    return true;
}

void ProgramsPage::refreshEditorValues()
{
    if (hourValueLabel_ != nullptr)
    {
        lv_label_set_text_fmt(
            hourValueLabel_,
            "%02u",
            static_cast<unsigned>(draftHour_)
        );
    }

    if (minuteValueLabel_ != nullptr)
    {
        lv_label_set_text_fmt(
            minuteValueLabel_,
            "%02u",
            static_cast<unsigned>(draftMinute_)
        );
    }

    if (durationValueLabel_ != nullptr)
    {
        lv_label_set_text_fmt(
            durationValueLabel_,
            "%u",
            static_cast<unsigned>(draftDurationMinutes_)
        );
    }
}

void ProgramsPage::refreshWeekdayButtons()
{
    for (uint8_t i = 0; i < WEEKDAY_COUNT; ++i)
    {
        if (weekdayButtons_[i] == nullptr)
        {
            continue;
        }

        const uint8_t mask = static_cast<uint8_t>(1U << i);

        if ((draftWeekdays_ & mask) != 0)
        {
            lv_obj_add_state(
                weekdayButtons_[i],
                LV_STATE_CHECKED
            );
        }
        else
        {
            lv_obj_clear_state(
                weekdayButtons_[i],
                LV_STATE_CHECKED
            );
        }
    }
}

lv_obj_t* ProgramsPage::createTextButton(
    lv_obj_t* parent,
    const char* text,
    int x,
    int y,
    int width,
    int height,
    lv_event_cb_t callback,
    void* userData
)
{
    lv_obj_t* button = lv_btn_create(parent);
    lv_obj_set_size(button, width, height);
    lv_obj_set_pos(button, x, y);
    configureButton(button);

    lv_obj_add_event_cb(
        button,
        callback,
        LV_EVENT_CLICKED,
        userData != nullptr ? userData : this
    );

    lv_obj_t* label = lv_label_create(button);
    lv_label_set_text(label, text);
    lv_obj_center(label);

    return button;
}

lv_obj_t* ProgramsPage::createValueLabel(
    lv_obj_t* parent,
    int x,
    int y,
    int width
)
{
    lv_obj_t* label = lv_label_create(parent);
    lv_obj_set_width(label, width);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    setLabelTextColor(label);
    lv_obj_set_pos(label, x, y);
    return label;
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

    lv_obj_t* sw = static_cast<lv_obj_t*>(
        lv_event_get_target(event)
    );

    const bool enabled =
        lv_obj_has_state(sw, LV_STATE_CHECKED);

    const bool saved = page.scheduler_->setProgramEnabled(
        ui->programIndex,
        enabled
    );

    if (page.displayManager_ != nullptr)
    {
        page.displayManager_->showMessage(
            saved
                ? (enabled
                    ? "Programm aktiviert"
                    : "Programm deaktiviert")
                : "Speichern fehlgeschlagen"
        );
    }
}

void ProgramsPage::programCardEvent(lv_event_t* event)
{
    auto* ui = static_cast<ProgramWidgets*>(
        lv_event_get_user_data(event)
    );

    if (ui != nullptr && ui->owner != nullptr)
    {
        ui->owner->openEditor(ui->programIndex);
    }
}

void ProgramsPage::hourMinusEvent(lv_event_t* event)
{
    auto* page = static_cast<ProgramsPage*>(
        lv_event_get_user_data(event)
    );

    if (page != nullptr)
    {
        page->draftHour_ =
            (page->draftHour_ == 0)
                ? 23
                : page->draftHour_ - 1;
        page->refreshEditorValues();
    }
}

void ProgramsPage::hourPlusEvent(lv_event_t* event)
{
    auto* page = static_cast<ProgramsPage*>(
        lv_event_get_user_data(event)
    );

    if (page != nullptr)
    {
        page->draftHour_ =
            (page->draftHour_ >= 23)
                ? 0
                : page->draftHour_ + 1;
        page->refreshEditorValues();
    }
}

void ProgramsPage::minuteMinusEvent(lv_event_t* event)
{
    auto* page = static_cast<ProgramsPage*>(
        lv_event_get_user_data(event)
    );

    if (page != nullptr)
    {
        page->draftMinute_ =
            (page->draftMinute_ == 0)
                ? 59
                : page->draftMinute_ - 1;
        page->refreshEditorValues();
    }
}

void ProgramsPage::minutePlusEvent(lv_event_t* event)
{
    auto* page = static_cast<ProgramsPage*>(
        lv_event_get_user_data(event)
    );

    if (page != nullptr)
    {
        page->draftMinute_ =
            (page->draftMinute_ >= 59)
                ? 0
                : page->draftMinute_ + 1;
        page->refreshEditorValues();
    }
}

void ProgramsPage::durationMinusEvent(lv_event_t* event)
{
    auto* page = static_cast<ProgramsPage*>(
        lv_event_get_user_data(event)
    );

    if (page != nullptr)
    {
        if (page->draftDurationMinutes_ >
            Scheduler::MIN_DURATION_MINUTES)
        {
            --page->draftDurationMinutes_;
        }

        page->refreshEditorValues();
    }
}

void ProgramsPage::durationPlusEvent(lv_event_t* event)
{
    auto* page = static_cast<ProgramsPage*>(
        lv_event_get_user_data(event)
    );

    if (page != nullptr)
    {
        if (page->draftDurationMinutes_ <
            Scheduler::MAX_DURATION_MINUTES)
        {
            ++page->draftDurationMinutes_;
        }

        page->refreshEditorValues();
    }
}

void ProgramsPage::weekdayEvent(lv_event_t* event)
{
    auto* context = static_cast<WeekdayButtonContext*>(
        lv_event_get_user_data(event)
    );

    if (context == nullptr ||
        context->owner == nullptr ||
        context->weekdayIndex >= WEEKDAY_COUNT)
    {
        return;
    }

    ProgramsPage& page = *context->owner;
    const uint8_t mask =
        static_cast<uint8_t>(1U << context->weekdayIndex);

    page.draftWeekdays_ ^= mask;
    page.refreshWeekdayButtons();
}

void ProgramsPage::editorSaveEvent(lv_event_t* event)
{
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
            saved
                ? "Programm gespeichert"
                : "Speichern fehlgeschlagen"
        );
    }

    if (saved)
    {
        page->closeEditor();
    }
}

void ProgramsPage::editorCancelEvent(lv_event_t* event)
{
    auto* page = static_cast<ProgramsPage*>(
        lv_event_get_user_data(event)
    );

    if (page != nullptr)
    {
        page->closeEditor();
    }
}
