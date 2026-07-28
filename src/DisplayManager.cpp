#include "DisplayManager.h"
#include "Theme.h"

DisplayManager* DisplayManager::instance_ = nullptr;

namespace
{
    constexpr int SCREEN_WIDTH = 480;
    constexpr int SCREEN_HEIGHT = 272;
    constexpr int HEADER_HEIGHT = 44;
    constexpr int FOOTER_HEIGHT = 42;
    constexpr int CONTENT_TOP = HEADER_HEIGHT;
    constexpr int CONTENT_HEIGHT = SCREEN_HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT;

    lv_obj_t* createLabel(lv_obj_t* parent, const char* text, lv_color_t color)
    {
        lv_obj_t* label = lv_label_create(parent);
        lv_label_set_text(label, text);
        lv_obj_set_style_text_color(label, color, 0);
        return label;
    }

    void configurePanel(lv_obj_t* object)
    {
        lv_obj_set_style_bg_color(object, Theme::panel(), 0);
        lv_obj_set_style_border_color(object, Theme::border(), 0);
        lv_obj_set_style_border_width(object, 1, 0);
        lv_obj_set_style_radius(object, Theme::CARD_RADIUS, 0);
        lv_obj_set_style_pad_all(object, 10, 0);
        lv_obj_clear_flag(object, LV_OBJ_FLAG_SCROLLABLE);
    }
}

//void DisplayManager::begin(ValveManager& valveManager)
void DisplayManager::begin(
    ValveManager& valveManager,
    Scheduler& scheduler)
{
    scheduler_ = &scheduler;
    instance_ = this;
    valveManager_ = &valveManager;
    valveManager_->setStateChangedCallback(valveStateChanged);

    smartdisplay_init();
    applyBrightness(brightnessPercent_);

    createDashboard();
    showPage(Page::Manual);

    for (uint8_t i = 0; i < AppConfig::DISPLAYED_VALVE_COUNT; ++i)
    {
        refreshValve(i);
    }

    showMessage("GardenFlow bereit");
}

void DisplayManager::update()
{
    static uint32_t lastTickMs = millis();

    const uint32_t now = millis();
    const uint32_t elapsed = now - lastTickMs;

    if (elapsed > 0)
    {
        lv_tick_inc(elapsed);
        lastTickMs = now;
    }

    lv_timer_handler();
    updateClock();
    updateStatus();
    updateToast();
}

void DisplayManager::refreshValve(uint8_t index)
{
    if (valveManager_ == nullptr ||
        index >= AppConfig::DISPLAYED_VALVE_COUNT)
    {
        return;
    }

    const ValveManager::Channel& channel = valveManager_->channel(index);
    ValveWidgets& ui = widgets_[index];

    lv_label_set_text(ui.nameLabel, channel.name);
    lv_label_set_text_fmt(
        ui.counterLabel,
        "Impulse %lu",
        static_cast<unsigned long>(channel.pulseCount)
    );

    if (channel.pulseActive)
    {
        lv_label_set_text(ui.stateLabel, "SCHALTIMPULS");
        lv_obj_set_style_text_color(ui.stateLabel, Theme::pulse(), 0);
        lv_obj_set_style_bg_color(ui.statusDot, Theme::pulse(), 0);
        lv_obj_set_style_bg_color(ui.button, Theme::pulse(), 0);
        lv_label_set_text(ui.buttonLabel, "BITTE WARTEN");
        lv_obj_add_state(ui.button, LV_STATE_DISABLED);
        return;
    }

    lv_obj_clear_state(ui.button, LV_STATE_DISABLED);

    if (channel.assumedOpen)
    {
        lv_label_set_text(ui.stateLabel, "OFFEN");
        lv_obj_set_style_text_color(ui.stateLabel, Theme::open(), 0);
        lv_obj_set_style_bg_color(ui.statusDot, Theme::open(), 0);
        lv_obj_set_style_bg_color(ui.button, Theme::closed(), 0);
        lv_label_set_text(ui.buttonLabel, "SCHLIESSEN");
    }
    else
    {
        lv_label_set_text(ui.stateLabel, "GESCHLOSSEN");
        lv_obj_set_style_text_color(ui.stateLabel, Theme::closed(), 0);
        lv_obj_set_style_bg_color(ui.statusDot, Theme::closed(), 0);
        lv_obj_set_style_bg_color(ui.button, Theme::open(), 0);
        lv_label_set_text(ui.buttonLabel, "OEFFNEN");
    }
}

void DisplayManager::showMessage(const char* message)
{
    if (toast_ == nullptr)
    {
        return;
    }

    lv_label_set_text(toast_, message);
    lv_obj_clear_flag(toast_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(toast_);
    toastHideAtMs_ = millis() + 1600;
}

void DisplayManager::valveButtonEvent(lv_event_t* event)
{
    if (instance_ == nullptr ||
        instance_->valveManager_ == nullptr ||
        lv_event_get_code(event) != LV_EVENT_CLICKED)
    {
        return;
    }

    const uintptr_t rawIndex =
        reinterpret_cast<uintptr_t>(lv_event_get_user_data(event));
    const uint8_t index = static_cast<uint8_t>(rawIndex);

    if (instance_->valveManager_->toggle(index))
    {
        instance_->showMessage("Ventilimpuls gestartet");
    }
}

void DisplayManager::valveStateChanged(uint8_t index)
{
    if (instance_ != nullptr)
    {
        instance_->refreshValve(index);
    }
}

void DisplayManager::navigationEvent(lv_event_t* event)
{
    if (instance_ == nullptr || lv_event_get_code(event) != LV_EVENT_CLICKED)
    {
        return;
    }

    const uintptr_t rawPage =
        reinterpret_cast<uintptr_t>(lv_event_get_user_data(event));
    instance_->showPage(static_cast<Page>(rawPage));
}

void DisplayManager::programSwitchEvent(lv_event_t* event)
{
    if (instance_ == nullptr ||
        instance_->scheduler_ == nullptr ||
        lv_event_get_code(event) != LV_EVENT_VALUE_CHANGED)
    {
        return;
    }

    lv_obj_t* sw =
        static_cast<lv_obj_t*>(lv_event_get_target(event));

    const uintptr_t rawIndex =
        reinterpret_cast<uintptr_t>(
            lv_event_get_user_data(event)
        );

    const uint8_t programIndex =
        static_cast<uint8_t>(rawIndex);

    if (programIndex >= instance_->scheduler_->programCount())
    {
        return;
    }

    const bool enabled =
        lv_obj_has_state(sw, LV_STATE_CHECKED);

    const bool saved =
        instance_->scheduler_->setProgramEnabled(
            programIndex,
            enabled
        );

    Serial.printf(
        "Programm %u: %s\n",
        static_cast<unsigned>(programIndex + 1),
        enabled ? "aktiviert" : "deaktiviert"
    );

    if (saved)
    {
        instance_->showMessage(
            enabled
                ? "Programm aktiviert"
                : "Programm deaktiviert"
        );
    }
    else
    {
        instance_->showMessage(
            "Speichern fehlgeschlagen"
        );
    }
}

void DisplayManager::brightnessSliderEvent(lv_event_t* event)
{
    if (instance_ == nullptr || lv_event_get_code(event) != LV_EVENT_VALUE_CHANGED)
    {
        return;
    }

    lv_obj_t* slider = static_cast<lv_obj_t*>(lv_event_get_target(event));
    const uint8_t value = static_cast<uint8_t>(lv_slider_get_value(slider));
    instance_->applyBrightness(value);

    if (instance_->brightnessValueLabel_ != nullptr)
    {
        lv_label_set_text_fmt(instance_->brightnessValueLabel_, "%u %%", value);
    }
}

void DisplayManager::pulseSliderEvent(lv_event_t* event)
{
    if (instance_ == nullptr || lv_event_get_code(event) != LV_EVENT_VALUE_CHANGED)
    {
        return;
    }

    lv_obj_t* slider = static_cast<lv_obj_t*>(lv_event_get_target(event));
    const uint32_t value = static_cast<uint32_t>(lv_slider_get_value(slider));
    instance_->applyPulseDuration(value);

    if (instance_->pulseValueLabel_ != nullptr)
    {
        lv_label_set_text_fmt(
            instance_->pulseValueLabel_,
            "%lu ms",
            static_cast<unsigned long>(value)
        );
    }
}

void DisplayManager::createDashboard()
{
    lv_obj_t* screen = lv_screen_active();

    lv_obj_set_style_bg_color(screen, Theme::background(), 0);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    createHeader(screen);
    createPages(screen);
    createFooter(screen);

    toast_ = lv_label_create(screen);
    lv_obj_set_style_bg_color(toast_, lv_color_hex(0x263540), 0);
    lv_obj_set_style_text_color(toast_, Theme::text(), 0);
    lv_obj_set_style_pad_hor(toast_, 14, 0);
    lv_obj_set_style_pad_ver(toast_, 8, 0);
    lv_obj_set_style_radius(toast_, 8, 0);
    lv_obj_align(toast_, LV_ALIGN_BOTTOM_MID, 0, -48);
    lv_obj_add_flag(toast_, LV_OBJ_FLAG_HIDDEN);
}

void DisplayManager::createHeader(lv_obj_t* screen)
{
    lv_obj_t* header = lv_obj_create(screen);
    lv_obj_set_size(header, SCREEN_WIDTH, HEADER_HEIGHT);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_style_bg_color(header, Theme::header(), 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = createLabel(header, "GardenFlow", Theme::text());
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 12, 0);

    pageTitleLabel_ = createLabel(header, "MANUELL", Theme::accent());
    lv_obj_align(pageTitleLabel_, LV_ALIGN_CENTER, 25, 0);

    modeLabel_ = createLabel(header, "HAND", Theme::textDim());
    lv_obj_align(modeLabel_, LV_ALIGN_RIGHT_MID, -70, 0);

    clockLabel_ = createLabel(header, "--:--", Theme::text());
    lv_obj_align(clockLabel_, LV_ALIGN_RIGHT_MID, -12, 0);
}

void DisplayManager::createPages(lv_obj_t* screen)
{
    for (uint8_t i = 0; i < static_cast<uint8_t>(Page::Count); ++i)
    {
        pages_[i] = lv_obj_create(screen);
        lv_obj_set_size(pages_[i], SCREEN_WIDTH, CONTENT_HEIGHT);
        lv_obj_set_pos(pages_[i], 0, CONTENT_TOP);
        lv_obj_set_style_bg_color(pages_[i], Theme::background(), 0);
        lv_obj_set_style_border_width(pages_[i], 0, 0);
        lv_obj_set_style_radius(pages_[i], 0, 0);
        lv_obj_set_style_pad_all(pages_[i], 0, 0);
        lv_obj_clear_flag(pages_[i], LV_OBJ_FLAG_SCROLLABLE);
    }

    createManualPage(pages_[static_cast<uint8_t>(Page::Manual)]);
    createProgramsPage(pages_[static_cast<uint8_t>(Page::Programs)]);
    createStatusPage(pages_[static_cast<uint8_t>(Page::Status)]);
    createSetupPage(pages_[static_cast<uint8_t>(Page::Setup)]);
}

void DisplayManager::createManualPage(lv_obj_t* parent)
{
    createValveCard(parent, 0, 12);
    createValveCard(parent, 1, 246);
}

void DisplayManager::createProgramsPage(lv_obj_t* parent)
{
    lv_obj_t* heading = createLabel(parent, "Wochenprogramme", Theme::text());
    lv_obj_set_pos(heading, 14, 8);

    rebuildProgramList();
}

void DisplayManager::createStatusPage(lv_obj_t* parent)
{
    lv_obj_t* panel = lv_obj_create(parent);
    lv_obj_set_size(panel, 452, 164);
    lv_obj_set_pos(panel, 14, 10);
    configurePanel(panel);

    lv_obj_t* heading = createLabel(panel, "Systemstatus", Theme::text());
    lv_obj_set_pos(heading, 2, 0);

    statusUptimeLabel_ = createLabel(panel, "Laufzeit: --", Theme::textDim());
    lv_obj_set_pos(statusUptimeLabel_, 2, 32);

    statusValve1Label_ = createLabel(panel, "Ventil 1: --", Theme::text());
    lv_obj_set_pos(statusValve1Label_, 2, 60);

    statusValve2Label_ = createLabel(panel, "Ventil 2: --", Theme::text());
    lv_obj_set_pos(statusValve2Label_, 226, 60);

    statusPulseLabel_ = createLabel(panel, "Impulse gesamt: 0", Theme::textDim());
    lv_obj_set_pos(statusPulseLabel_, 2, 92);

    lv_obj_t* display = createLabel(panel, "Display/Touch: BEREIT", Theme::open());
    lv_obj_set_pos(display, 2, 122);

    lv_obj_t* mode = createLabel(panel, "Ventilbetrieb: SIMULATION", Theme::pulse());
    lv_obj_set_pos(mode, 226, 122);
}

void DisplayManager::createSetupPage(lv_obj_t* parent)
{
    lv_obj_t* displayPanel = lv_obj_create(parent);
    lv_obj_set_size(displayPanel, 218, 164);
    lv_obj_set_pos(displayPanel, 14, 10);
    configurePanel(displayPanel);

    lv_obj_t* displayTitle = createLabel(displayPanel, "Display", Theme::text());
    lv_obj_set_pos(displayTitle, 2, 0);

    lv_obj_t* brightnessLabel = createLabel(displayPanel, "Helligkeit", Theme::textDim());
    lv_obj_set_pos(brightnessLabel, 2, 35);

    brightnessValueLabel_ = createLabel(displayPanel, "80 %", Theme::accent());
    lv_obj_align(brightnessValueLabel_, LV_ALIGN_TOP_RIGHT, -2, 35);

    brightnessSlider_ = lv_slider_create(displayPanel);
    lv_obj_set_size(brightnessSlider_, 184, 16);
    lv_obj_set_pos(brightnessSlider_, 2, 70);
    lv_slider_set_range(brightnessSlider_, 10, 100);
    lv_slider_set_value(brightnessSlider_, brightnessPercent_, LV_ANIM_OFF);
    lv_obj_add_event_cb(
        brightnessSlider_,
        brightnessSliderEvent,
        LV_EVENT_VALUE_CHANGED,
        nullptr
    );

    lv_obj_t* touchText = createLabel(displayPanel, "Touch: XPT2046 bereit", Theme::open());
    lv_obj_set_pos(touchText, 2, 112);

    lv_obj_t* valvePanel = lv_obj_create(parent);
    lv_obj_set_size(valvePanel, 218, 164);
    lv_obj_set_pos(valvePanel, 248, 10);
    configurePanel(valvePanel);

    lv_obj_t* valveTitle = createLabel(valvePanel, "Ventile", Theme::text());
    lv_obj_set_pos(valveTitle, 2, 0);

    lv_obj_t* pulseLabel = createLabel(valvePanel, "Impulsdauer", Theme::textDim());
    lv_obj_set_pos(pulseLabel, 2, 35);

    pulseValueLabel_ = createLabel(valvePanel, "250 ms", Theme::accent());
    lv_obj_align(pulseValueLabel_, LV_ALIGN_TOP_RIGHT, -2, 35);

    pulseSlider_ = lv_slider_create(valvePanel);
    lv_obj_set_size(pulseSlider_, 184, 16);
    lv_obj_set_pos(pulseSlider_, 2, 70);
    lv_slider_set_range(pulseSlider_, 100, 1000);
    lv_slider_set_value(
        pulseSlider_,
        static_cast<int32_t>(AppConfig::DEFAULT_PULSE_MS),
        LV_ANIM_OFF
    );
    lv_obj_add_event_cb(
        pulseSlider_,
        pulseSliderEvent,
        LV_EVENT_VALUE_CHANGED,
        nullptr
    );

    lv_obj_t* warning = createLabel(
        valvePanel,
        "GPIO-Ausgaenge noch deaktiviert",
        Theme::pulse()
    );
    lv_obj_set_width(warning, 190);
    lv_label_set_long_mode(warning, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(warning, 2, 108);
}

void DisplayManager::createValveCard(lv_obj_t* parent, uint8_t index, int x)
{
    ValveWidgets& ui = widgets_[index];

    ui.card = lv_obj_create(parent);
    lv_obj_set_size(ui.card, 222, 166);
    lv_obj_set_pos(ui.card, x, 10);
    configurePanel(ui.card);

    ui.statusDot = lv_obj_create(ui.card);
    lv_obj_set_size(ui.statusDot, 12, 12);
    lv_obj_set_style_radius(ui.statusDot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(ui.statusDot, 0, 0);
    lv_obj_align(ui.statusDot, LV_ALIGN_TOP_LEFT, 2, 2);

    ui.nameLabel = createLabel(ui.card, "", Theme::text());
    lv_obj_align(ui.nameLabel, LV_ALIGN_TOP_LEFT, 22, -2);

    ui.stateLabel = createLabel(ui.card, "", Theme::closed());
    lv_obj_align(ui.stateLabel, LV_ALIGN_TOP_MID, 0, 34);

    ui.button = lv_button_create(ui.card);
    lv_obj_set_size(ui.button, 182, 50);
    lv_obj_align(ui.button, LV_ALIGN_CENTER, 0, 32);
    lv_obj_set_style_radius(ui.button, 9, 0);
    lv_obj_set_style_shadow_width(ui.button, 8, 0);
    lv_obj_set_style_shadow_opa(ui.button, LV_OPA_20, 0);

    ui.buttonLabel = createLabel(ui.button, "", Theme::text());
    lv_obj_center(ui.buttonLabel);

    lv_obj_add_event_cb(
        ui.button,
        valveButtonEvent,
        LV_EVENT_CLICKED,
        reinterpret_cast<void*>(static_cast<uintptr_t>(index))
    );

    ui.counterLabel = createLabel(ui.card, "", Theme::textDim());
    lv_obj_align(ui.counterLabel, LV_ALIGN_BOTTOM_MID, 0, 1);
}


void DisplayManager::createProgramCard(
    ProgramWidgets& ui,
    lv_obj_t* parent,
    uint8_t number,
    const char* valve,
    const char* timeText,
    const char* durationText,
    int y
)
{
    ui.card = lv_obj_create(parent);
    lv_obj_set_size(ui.card, 452, 62);
    lv_obj_set_pos(ui.card, 14, y);
    configurePanel(ui.card);

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
        valve,
        timeText,
        durationText
    );
    lv_obj_set_style_text_color(ui.details, Theme::textDim(), 0);
    lv_obj_set_pos(ui.details, 0, 28);

    ui.enableSwitch = lv_switch_create(ui.card);
    lv_obj_set_size(ui.enableSwitch, 52, 28);
    lv_obj_align(ui.enableSwitch, LV_ALIGN_RIGHT_MID, -2, 0);

    ui.programIndex = number - 1;

    lv_obj_add_event_cb(
        ui.enableSwitch,
        programSwitchEvent,
        LV_EVENT_VALUE_CHANGED,
        reinterpret_cast<void*>(
            static_cast<uintptr_t>(ui.programIndex)
        )
    );
}

void DisplayManager::createFooter(lv_obj_t* screen)
{
    static const char* names[] =
    {
        "MANUELL", "PROGRAMME", "STATUS", "SETUP"
    };

    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - FOOTER_HEIGHT);
    lv_obj_set_style_bg_color(footer, Theme::header(), 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_set_style_radius(footer, 0, 0);
    lv_obj_set_style_pad_all(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    for (uint8_t i = 0; i < static_cast<uint8_t>(Page::Count); ++i)
    {
        lv_obj_t* button = lv_button_create(footer);
        navButtons_[i] = button;

        lv_obj_set_size(button, 116, 34);
        lv_obj_set_pos(button, 3 + (i * 119), 4);
        lv_obj_set_style_radius(button, 6, 0);
        lv_obj_set_style_shadow_width(button, 0, 0);
        lv_obj_set_style_bg_color(button, Theme::panelAlt(), 0);
        lv_obj_set_style_bg_color(button, Theme::accent(), LV_STATE_CHECKED);

        navLabels_[i] = createLabel(button, names[i], Theme::textDim());
        lv_obj_center(navLabels_[i]);

        lv_obj_add_flag(button, LV_OBJ_FLAG_CHECKABLE);
        lv_obj_add_event_cb(
            button,
            navigationEvent,
            LV_EVENT_CLICKED,
            reinterpret_cast<void*>(static_cast<uintptr_t>(i))
        );
    }
}

void DisplayManager::showPage(Page page)
{
    const uint8_t selected = static_cast<uint8_t>(page);
    if (selected >= static_cast<uint8_t>(Page::Count))
    {
        return;
    }

    static const char* titles[] =
    {
        "MANUELL", "PROGRAMME", "STATUS", "SETUP"
    };

    activePage_ = page;

    for (uint8_t i = 0; i < static_cast<uint8_t>(Page::Count); ++i)
    {
        if (i == selected)
        {
            lv_obj_clear_flag(pages_[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_state(navButtons_[i], LV_STATE_CHECKED);
            lv_obj_set_style_text_color(navLabels_[i], Theme::text(), 0);
        }
        else
        {
            lv_obj_add_flag(pages_[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(navButtons_[i], LV_STATE_CHECKED);
            lv_obj_set_style_text_color(navLabels_[i], Theme::textDim(), 0);
        }
    }

    if (pageTitleLabel_ != nullptr)
    {
        lv_label_set_text(pageTitleLabel_, titles[selected]);
    }

    if (modeLabel_ != nullptr)
    {
        lv_label_set_text(
            modeLabel_,
            page == Page::Manual ? "HAND" : "SYSTEM"
        );
    }

    updateStatus();
}

void DisplayManager::updateClock()
{
    const uint32_t now = millis();

    if (clockLabel_ == nullptr ||
        (now - lastClockUpdateMs_) < 1000)
    {
        return;
    }

    lastClockUpdateMs_ = now;

    const uint32_t totalSeconds = now / 1000;
    const uint32_t minutes = (totalSeconds / 60) % 60;
    const uint32_t hours = (totalSeconds / 3600) % 24;

    lv_label_set_text_fmt(
        clockLabel_,
        "%02lu:%02lu",
        static_cast<unsigned long>(hours),
        static_cast<unsigned long>(minutes)
    );
}

void DisplayManager::updateStatus()
{
    if (valveManager_ == nullptr ||
        statusUptimeLabel_ == nullptr)
    {
        return;
    }

    const uint32_t now = millis();
    if ((now - lastStatusUpdateMs_) < 500 &&
        activePage_ != Page::Status)
    {
        return;
    }

    lastStatusUpdateMs_ = now;

    const uint32_t totalSeconds = now / 1000;
    const uint32_t hours = totalSeconds / 3600;
    const uint32_t minutes = (totalSeconds / 60) % 60;
    const uint32_t seconds = totalSeconds % 60;

    lv_label_set_text_fmt(
        statusUptimeLabel_,
        "Laufzeit: %02lu:%02lu:%02lu",
        static_cast<unsigned long>(hours),
        static_cast<unsigned long>(minutes),
        static_cast<unsigned long>(seconds)
    );

    const auto& valve1 = valveManager_->channel(0);
    const auto& valve2 = valveManager_->channel(1);

    lv_label_set_text_fmt(
        statusValve1Label_,
        "Ventil 1: %s",
        valve1.pulseActive ? "IMPULS" : (valve1.assumedOpen ? "OFFEN" : "ZU")
    );

    lv_label_set_text_fmt(
        statusValve2Label_,
        "Ventil 2: %s",
        valve2.pulseActive ? "IMPULS" : (valve2.assumedOpen ? "OFFEN" : "ZU")
    );

    lv_label_set_text_fmt(
        statusPulseLabel_,
        "Impulse gesamt: %lu",
        static_cast<unsigned long>(valve1.pulseCount + valve2.pulseCount)
    );
}

void DisplayManager::updateToast()
{
    if (toast_ != nullptr &&
        toastHideAtMs_ != 0 &&
        static_cast<int32_t>(millis() - toastHideAtMs_) >= 0)
    {
        lv_obj_add_flag(toast_, LV_OBJ_FLAG_HIDDEN);
        toastHideAtMs_ = 0;
    }
}

void DisplayManager::applyBrightness(uint8_t percent)
{
    percent = constrain(percent, 10, 100);
    brightnessPercent_ = percent;
    smartdisplay_lcd_set_backlight(static_cast<float>(percent) / 100.0f);
}

void DisplayManager::applyPulseDuration(uint32_t durationMs)
{
    if (valveManager_ == nullptr)
    {
        return;
    }

    durationMs = constrain(durationMs, 100UL, 1000UL);

    valveManager_->setPulseDurationAll(durationMs);

    Serial.printf(
        "Gewuenschte Impulsdauer: %lu ms\n",
        static_cast<unsigned long>(durationMs)
    );
}

void DisplayManager::rebuildProgramList()
{
    if (scheduler_ == nullptr)
    {
        return;
    }

    lv_obj_t* page =
        pages_[static_cast<uint8_t>(Page::Programs)];

    int y = 34;

    const uint8_t count =
        min(
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
            static_cast<unsigned>(program.durationSeconds / 60)
        );

        createProgramCard(
            programWidgets_[i],
            page,
            i + 1,
            valveText,
            timeText,
            durationText,
            y
        );

        if (program.enabled)
        {
            lv_obj_add_state(
                programWidgets_[i].enableSwitch,
                LV_STATE_CHECKED
            );
        }
        else
        {
            lv_obj_clear_state(
                programWidgets_[i].enableSwitch,
                LV_STATE_CHECKED
            );
        }

        y += 71;
    }
}
