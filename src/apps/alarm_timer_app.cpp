#include "alarm_timer_app.h"
#include "../system/os_manager.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

AlarmTimerApp::AlarmTimerApp() 
    : BaseApp("com.m5stack.alarmtimer", "Alarm & Timer", "1.0.0"), 
      m_nextAlarmId(1), m_nextTimerId(1), m_currentMode(AppMode::CLOCK),
      m_24HourFormat(true), m_soundEnabled(true), m_activeAlarmId(0),
      m_stopwatchTime(0), m_stopwatchRunning(false), m_stopwatchStartTime(0), m_stopwatchPauseTime(0) {
    setDescription("Comprehensive alarm clock and timer application with stopwatch functionality");
    setAuthor("M5Stack");
    setPriority(AppPriority::APP_NORMAL);
}

AlarmTimerApp::~AlarmTimerApp() {
    shutdown();
}

os_error_t AlarmTimerApp::initialize() {
    if (m_initialized) {
        return OS_OK;
    }

    log(ESP_LOG_INFO, "Initializing Alarm Timer App");
    
    // Load existing alarms and timers
    loadAlarms();
    loadTimers();
    
    // Set memory usage estimate
    setMemoryUsage(40960); // 40KB estimated usage
    
    m_initialized = true;
    return OS_OK;
}

os_error_t AlarmTimerApp::update(uint32_t deltaTime) {
    if (!m_initialized) {
        return OS_OK;
    }
    
    // Check alarms
    checkAlarms();
    
    // Update running timers
    updateTimers();
    
    // Update stopwatch if running
    if (m_stopwatchRunning) {
        updateStopwatch();
    }
    
    // Update clock display
    updateClock();
    
    return OS_OK;
}

os_error_t AlarmTimerApp::shutdown() {
    if (!m_initialized) {
        return OS_OK;
    }

    log(ESP_LOG_INFO, "Shutting down Alarm Timer App");
    
    // Save alarms and timers
    saveAlarms();
    saveTimers();
    
    // Stop any playing sounds
    stopSound();
    
    m_initialized = false;
    return OS_OK;
}

os_error_t AlarmTimerApp::createUI(lv_obj_t* parent) {
    if (!parent) {
        return OS_ERROR_INVALID_PARAM;
    }

    // Create main container
    m_uiContainer = lv_obj_create(parent);
    lv_obj_set_size(m_uiContainer, LV_HOR_RES, 
                    LV_VER_RES - 60 - 40); // Account for status bar and dock
    lv_obj_align(m_uiContainer, LV_ALIGN_CENTER, 0, 0);
    
    // Apply theme
    lv_obj_set_style_bg_color(m_uiContainer, lv_color_hex(0x1E1E1E), 0);
    lv_obj_set_style_border_opa(m_uiContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(m_uiContainer, 5, 0);

    createMainUI();
    return OS_OK;
}

os_error_t AlarmTimerApp::destroyUI() {
    if (m_uiContainer) {
        lv_obj_del(m_uiContainer);
        m_uiContainer = nullptr;
    }
    return OS_OK;
}

void AlarmTimerApp::createMainUI() {
    createTabView();
}

void AlarmTimerApp::createTabView() {
    m_tabView = lv_tabview_create(m_uiContainer, LV_DIR_TOP, 50);
    lv_obj_set_size(m_tabView, LV_PCT(100), LV_PCT(100));
    
    // Create tabs
    m_clockTab = lv_tabview_add_tab(m_tabView, "Clock");
    m_alarmsTab = lv_tabview_add_tab(m_tabView, "Alarms");
    m_timersTab = lv_tabview_add_tab(m_tabView, "Timers");
    m_stopwatchTab = lv_tabview_add_tab(m_tabView, "Stopwatch");
    
    // Setup tab content
    createClockTab();
    createAlarmsTab();
    createTimersTab();
    createStopwatchTab();
    
    lv_obj_add_event_cb(m_tabView, tabChangedCallback, LV_EVENT_VALUE_CHANGED, this);
}

void AlarmTimerApp::createClockTab() {
    createDigitalClock();
    createAnalogClock();
}

void AlarmTimerApp::createDigitalClock() {
    // Main clock display
    m_digitalClock = lv_label_create(m_clockTab);
    lv_obj_set_style_text_font(m_digitalClock, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(m_digitalClock, lv_color_hex(0x3498DB), 0);
    lv_obj_align(m_digitalClock, LV_ALIGN_CENTER, 0, -50);
    
    // Date display
    m_dateLabel = lv_label_create(m_clockTab);
    lv_obj_set_style_text_font(m_dateLabel, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(m_dateLabel, lv_color_hex(0x95A5A6), 0);
    lv_obj_align_to(m_dateLabel, m_digitalClock, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    
    updateClock();
}

void AlarmTimerApp::createAnalogClock() {
    // Simple analog clock representation using canvas or meter
    // For now, we'll create a placeholder
    lv_obj_t* clockFrame = lv_obj_create(m_clockTab);
    lv_obj_set_size(clockFrame, 200, 200);
    lv_obj_align(clockFrame, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_radius(clockFrame, 100, 0);
    lv_obj_set_style_bg_color(clockFrame, lv_color_hex(0x2C2C2C), 0);
    lv_obj_set_style_border_color(clockFrame, lv_color_hex(0x3498DB), 0);
    lv_obj_set_style_border_width(clockFrame, 3, 0);
    
    lv_obj_t* centerDot = lv_obj_create(clockFrame);
    lv_obj_set_size(centerDot, 10, 10);
    lv_obj_center(centerDot);
    lv_obj_set_style_radius(centerDot, 5, 0);
    lv_obj_set_style_bg_color(centerDot, lv_color_hex(0xE74C3C), 0);
}

void AlarmTimerApp::createAlarmsTab() {
    // Toolbar
    lv_obj_t* toolbar = lv_obj_create(m_alarmsTab);
    lv_obj_set_size(toolbar, LV_PCT(100), 50);
    lv_obj_align(toolbar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(toolbar, lv_color_hex(0x2C2C2C), 0);
    
    // Add alarm button
    lv_obj_t* addBtn = lv_btn_create(toolbar);
    lv_obj_set_size(addBtn, 100, LV_PCT(80));
    lv_obj_align(addBtn, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_add_event_cb(addBtn, addAlarmCallback, LV_EVENT_CLICKED, this);
    
    lv_obj_t* addLabel = lv_label_create(addBtn);
    lv_label_set_text(addLabel, "Add Alarm");
    lv_obj_center(addLabel);
    
    // Alarm list
    m_alarmList = lv_list_create(m_alarmsTab);
    lv_obj_set_size(m_alarmList, LV_PCT(100), LV_PCT(100) - 60);
    lv_obj_align_to(m_alarmList, toolbar, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
    lv_obj_set_style_bg_color(m_alarmList, lv_color_hex(0x2C2C2C), 0);
    
    // Load existing alarms into list
    for (const auto& alarm : m_alarms) {
        std::string timeText = formatTime(alarm.hour, alarm.minute);
        std::string alarmText = alarm.name + " - " + timeText;
        
        lv_obj_t* btn = lv_list_add_btn(m_alarmList, LV_SYMBOL_BELL, alarmText.c_str());
        lv_obj_set_user_data(btn, (void*)(uintptr_t)alarm.id);
        
        // Add toggle switch for enabled/disabled
        lv_obj_t* alarmSwitch = lv_switch_create(btn);
        lv_obj_align(alarmSwitch, LV_ALIGN_RIGHT_MID, -10, 0);
        lv_obj_add_event_cb(alarmSwitch, alarmToggleCallback, LV_EVENT_VALUE_CHANGED, this);
        lv_obj_set_user_data(alarmSwitch, (void*)(uintptr_t)alarm.id);
        
        if (alarm.enabled) {
            lv_obj_add_state(alarmSwitch, LV_STATE_CHECKED);
        }
    }
}

void AlarmTimerApp::createTimersTab() {
    // Toolbar
    lv_obj_t* toolbar = lv_obj_create(m_timersTab);
    lv_obj_set_size(toolbar, LV_PCT(100), 50);
    lv_obj_align(toolbar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(toolbar, lv_color_hex(0x2C2C2C), 0);
    
    // Add timer button
    lv_obj_t* addBtn = lv_btn_create(toolbar);
    lv_obj_set_size(addBtn, 100, LV_PCT(80));
    lv_obj_align(addBtn, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_add_event_cb(addBtn, addTimerCallback, LV_EVENT_CLICKED, this);
    
    lv_obj_t* addLabel = lv_label_create(addBtn);
    lv_label_set_text(addLabel, "Add Timer");
    lv_obj_center(addLabel);
    
    // Timer list
    m_timerList = lv_list_create(m_timersTab);
    lv_obj_set_size(m_timerList, LV_PCT(100), LV_PCT(100) - 60);
    lv_obj_align_to(m_timerList, toolbar, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
    lv_obj_set_style_bg_color(m_timerList, lv_color_hex(0x2C2C2C), 0);
    
    // Load existing timers into list
    for (const auto& timer : m_timers) {
        std::string durationText = formatDuration(timer.remainingSeconds);
        std::string timerText = timer.name + " - " + durationText;
        
        lv_obj_t* btn = lv_list_add_btn(m_timerList, LV_SYMBOL_BELL, timerText.c_str());
        lv_obj_set_user_data(btn, (void*)(uintptr_t)timer.id);
        
        // Add control buttons
        lv_obj_t* startBtn = lv_btn_create(btn);
        lv_obj_set_size(startBtn, 40, 30);
        lv_obj_align(startBtn, LV_ALIGN_RIGHT_MID, -70, 0);
        lv_obj_add_event_cb(startBtn, timerStartCallback, LV_EVENT_CLICKED, this);
        lv_obj_set_user_data(startBtn, (void*)(uintptr_t)timer.id);
        
        lv_obj_t* startLabel = lv_label_create(startBtn);
        lv_label_set_text(startLabel, timer.isRunning ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
        lv_obj_center(startLabel);
        
        lv_obj_t* stopBtn = lv_btn_create(btn);
        lv_obj_set_size(stopBtn, 40, 30);
        lv_obj_align(stopBtn, LV_ALIGN_RIGHT_MID, -20, 0);
        lv_obj_add_event_cb(stopBtn, timerStopCallback, LV_EVENT_CLICKED, this);
        lv_obj_set_user_data(stopBtn, (void*)(uintptr_t)timer.id);
        
        lv_obj_t* stopLabel = lv_label_create(stopBtn);
        lv_label_set_text(stopLabel, LV_SYMBOL_STOP);
        lv_obj_center(stopLabel);
    }
}

void AlarmTimerApp::createStopwatchTab() {
    // Stopwatch display
    m_stopwatchDisplay = lv_label_create(m_stopwatchTab);
    lv_obj_set_style_text_font(m_stopwatchDisplay, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(m_stopwatchDisplay, lv_color_hex(0x27AE60), 0);
    lv_label_set_text(m_stopwatchDisplay, "00:00.00");
    lv_obj_align(m_stopwatchDisplay, LV_ALIGN_CENTER, 0, -80);
    
    // Control buttons
    lv_obj_t* buttonContainer = lv_obj_create(m_stopwatchTab);
    lv_obj_set_size(buttonContainer, LV_PCT(80), 60);
    lv_obj_align_to(buttonContainer, m_stopwatchDisplay, LV_ALIGN_OUT_BOTTOM_MID, 0, 30);
    lv_obj_set_style_bg_opa(buttonContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(buttonContainer, LV_OPA_TRANSP, 0);
    
    // Start/Pause button
    m_stopwatchStartBtn = lv_btn_create(buttonContainer);
    lv_obj_set_size(m_stopwatchStartBtn, 80, 50);
    lv_obj_align(m_stopwatchStartBtn, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_add_event_cb(m_stopwatchStartBtn, stopwatchStartCallback, LV_EVENT_CLICKED, this);
    lv_obj_set_style_bg_color(m_stopwatchStartBtn, lv_color_hex(0x27AE60), 0);
    
    lv_obj_t* startLabel = lv_label_create(m_stopwatchStartBtn);
    lv_label_set_text(startLabel, "Start");
    lv_obj_center(startLabel);
    
    // Reset button
    m_stopwatchResetBtn = lv_btn_create(buttonContainer);
    lv_obj_set_size(m_stopwatchResetBtn, 80, 50);
    lv_obj_align(m_stopwatchResetBtn, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_event_cb(m_stopwatchResetBtn, stopwatchResetCallback, LV_EVENT_CLICKED, this);
    lv_obj_set_style_bg_color(m_stopwatchResetBtn, lv_color_hex(0xE74C3C), 0);
    
    lv_obj_t* resetLabel = lv_label_create(m_stopwatchResetBtn);
    lv_label_set_text(resetLabel, "Reset");
    lv_obj_center(resetLabel);
    
    // Lap button
    m_stopwatchLapBtn = lv_btn_create(buttonContainer);
    lv_obj_set_size(m_stopwatchLapBtn, 80, 50);
    lv_obj_align(m_stopwatchLapBtn, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(m_stopwatchLapBtn, stopwatchLapCallback, LV_EVENT_CLICKED, this);
    lv_obj_set_style_bg_color(m_stopwatchLapBtn, lv_color_hex(0x3498DB), 0);
    
    lv_obj_t* lapLabel = lv_label_create(m_stopwatchLapBtn);
    lv_label_set_text(lapLabel, "Lap");
    lv_obj_center(lapLabel);
    
    // Lap times list
    m_lapList = lv_list_create(m_stopwatchTab);
    lv_obj_set_size(m_lapList, LV_PCT(100), LV_PCT(40));
    lv_obj_align(m_lapList, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(m_lapList, lv_color_hex(0x2C2C2C), 0);
}

void AlarmTimerApp::updateClock() {
    if (!m_digitalClock || !m_dateLabel) {
        return;
    }
    
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    
    char timeStr[32];
    char dateStr[64];
    
    if (m_24HourFormat) {
        strftime(timeStr, sizeof(timeStr), "%H:%M:%S", timeinfo);
    } else {
        strftime(timeStr, sizeof(timeStr), "%I:%M:%S %p", timeinfo);
    }
    
    strftime(dateStr, sizeof(dateStr), "%A, %B %d, %Y", timeinfo);
    
    lv_label_set_text(m_digitalClock, timeStr);
    lv_label_set_text(m_dateLabel, dateStr);
}

void AlarmTimerApp::updateStopwatch() {
    if (!m_stopwatchDisplay) {
        return;
    }
    
    uint32_t currentTime;
    if (m_stopwatchRunning) {
        currentTime = m_stopwatchTime + (millis() - m_stopwatchStartTime);
    } else {
        currentTime = m_stopwatchTime;
    }
    
    uint32_t minutes = currentTime / 60000;
    uint32_t seconds = (currentTime % 60000) / 1000;
    uint32_t centiseconds = (currentTime % 1000) / 10;
    
    char timeStr[16];
    sprintf(timeStr, "%02d:%02d.%02d", minutes, seconds, centiseconds);
    lv_label_set_text(m_stopwatchDisplay, timeStr);
}

void AlarmTimerApp::checkAlarms() {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    
    for (auto& alarm : m_alarms) {
        if (!alarm.enabled) continue;
        
        if (timeinfo->tm_hour == alarm.hour && timeinfo->tm_min == alarm.minute && 
            timeinfo->tm_sec == 0) { // Trigger only at the exact minute
            triggerAlarm(alarm);
        }
    }
}

void AlarmTimerApp::updateTimers() {
    for (auto& timer : m_timers) {
        if (!timer.isRunning || timer.isPaused) continue;
        
        time_t now = time(nullptr);
        uint32_t elapsed = (now - timer.startTime);
        
        if (elapsed >= timer.totalSeconds) {
            timer.remainingSeconds = 0;
            timer.isRunning = false;
            checkTimerAlerts();
        } else {
            timer.remainingSeconds = timer.totalSeconds - elapsed;
        }
    }
}

void AlarmTimerApp::triggerAlarm(const Alarm& alarm) {
    m_activeAlarmId = alarm.id;
    playSound(alarm.ringtone, alarm.volume);
    
    // Create alarm alert dialog
    static const char* btns[] = {"Snooze", "Dismiss", ""};
    m_alarmAlert = lv_msgbox_create(lv_scr_act(), "Alarm", alarm.name.c_str(), 
                                   btns, true);
    lv_obj_center(m_alarmAlert);
    
    lv_obj_add_event_cb(m_alarmAlert, snoozeCallback, LV_EVENT_VALUE_CHANGED, this);
}

void AlarmTimerApp::checkTimerAlerts() {
    // Check for completed timers and trigger alerts
    for (const auto& timer : m_timers) {
        if (timer.remainingSeconds == 0 && !timer.isRunning) {
            playSound(timer.ringtone, timer.volume);
            
            static const char* btns[] = {"OK", ""};
            lv_obj_t* alert = lv_msgbox_create(lv_scr_act(), "Timer", 
                                             (timer.name + " completed!").c_str(), 
                                             btns, true);
            lv_obj_center(alert);
        }
    }
}

std::string AlarmTimerApp::formatTime(uint8_t hour, uint8_t minute) {
    char timeStr[16];
    if (m_24HourFormat) {
        sprintf(timeStr, "%02d:%02d", hour, minute);
    } else {
        uint8_t displayHour = hour;
        const char* ampm = "AM";
        if (hour == 0) {
            displayHour = 12;
        } else if (hour > 12) {
            displayHour = hour - 12;
            ampm = "PM";
        } else if (hour == 12) {
            ampm = "PM";
        }
        sprintf(timeStr, "%d:%02d %s", displayHour, minute, ampm);
    }
    return std::string(timeStr);
}

std::string AlarmTimerApp::formatDuration(uint32_t seconds) {
    uint32_t hours = seconds / 3600;
    uint32_t minutes = (seconds % 3600) / 60;
    uint32_t secs = seconds % 60;
    
    char durationStr[16];
    if (hours > 0) {
        sprintf(durationStr, "%02d:%02d:%02d", hours, minutes, secs);
    } else {
        sprintf(durationStr, "%02d:%02d", minutes, secs);
    }
    return std::string(durationStr);
}

void AlarmTimerApp::playSound(const std::string& ringtone, uint8_t volume) {
    if (!m_soundEnabled) return;
    
    // Placeholder for sound playing implementation
    log(ESP_LOG_INFO, "Playing ringtone: %s at volume %d", ringtone.c_str(), volume);
}

void AlarmTimerApp::stopSound() {
    // Placeholder for stopping sound
    log(ESP_LOG_INFO, "Stopping alarm sound");
}

os_error_t AlarmTimerApp::loadAlarms() {
    // Load sample alarms for testing
    Alarm alarm1 = {m_nextAlarmId++, "Morning Alarm", 7, 0, true, true, 0x7F, "default", 80, true, 5, 0};
    Alarm alarm2 = {m_nextAlarmId++, "Lunch Reminder", 12, 0, false, false, 0, "chime", 60, false, 0, 0};
    
    m_alarms.push_back(alarm1);
    m_alarms.push_back(alarm2);
    
    return OS_OK;
}

os_error_t AlarmTimerApp::saveAlarms() {
    // In a real implementation, save to file system
    return OS_OK;
}

os_error_t AlarmTimerApp::loadTimers() {
    // Load sample timers for testing
    Timer timer1 = {m_nextTimerId++, "Cooking Timer", 1800, 1800, false, false, 0, 0, "bell", 70};
    Timer timer2 = {m_nextTimerId++, "Break Timer", 300, 300, false, false, 0, 0, "soft", 50};
    
    m_timers.push_back(timer1);
    m_timers.push_back(timer2);
    
    return OS_OK;
}

os_error_t AlarmTimerApp::saveTimers() {
    // In a real implementation, save to file system
    return OS_OK;
}

void AlarmTimerApp::startStopwatch() {
    if (!m_stopwatchRunning) {
        m_stopwatchRunning = true;
        m_stopwatchStartTime = millis();
        
        if (m_stopwatchStartBtn) {
            lv_obj_t* label = lv_obj_get_child(m_stopwatchStartBtn, 0);
            lv_label_set_text(label, "Pause");
            lv_obj_set_style_bg_color(m_stopwatchStartBtn, lv_color_hex(0xF39C12), 0);
        }
    } else {
        m_stopwatchTime += (millis() - m_stopwatchStartTime);
        m_stopwatchRunning = false;
        
        if (m_stopwatchStartBtn) {
            lv_obj_t* label = lv_obj_get_child(m_stopwatchStartBtn, 0);
            lv_label_set_text(label, "Start");
            lv_obj_set_style_bg_color(m_stopwatchStartBtn, lv_color_hex(0x27AE60), 0);
        }
    }
}

void AlarmTimerApp::resetStopwatch() {
    m_stopwatchRunning = false;
    m_stopwatchTime = 0;
    m_stopwatchStartTime = 0;
    m_lapTimes.clear();
    
    if (m_stopwatchStartBtn) {
        lv_obj_t* label = lv_obj_get_child(m_stopwatchStartBtn, 0);
        lv_label_set_text(label, "Start");
        lv_obj_set_style_bg_color(m_stopwatchStartBtn, lv_color_hex(0x27AE60), 0);
    }
    
    if (m_lapList) {
        lv_obj_clean(m_lapList);
    }
    
    updateStopwatch();
}

void AlarmTimerApp::lapStopwatch() {
    if (m_stopwatchRunning) {
        uint32_t currentTime = m_stopwatchTime + (millis() - m_stopwatchStartTime);
        uint32_t minutes = currentTime / 60000;
        uint32_t seconds = (currentTime % 60000) / 1000;
        uint32_t centiseconds = (currentTime % 1000) / 10;
        
        char lapStr[32];
        sprintf(lapStr, "Lap %d: %02d:%02d.%02d", (int)m_lapTimes.size() + 1, 
                minutes, seconds, centiseconds);
        
        m_lapTimes.push_back(std::string(lapStr));
        
        if (m_lapList) {
            lv_list_add_btn(m_lapList, nullptr, lapStr);
        }
    }
}

// Static callbacks
void AlarmTimerApp::tabChangedCallback(lv_event_t* e) {
    AlarmTimerApp* app = static_cast<AlarmTimerApp*>(lv_event_get_user_data(e));
    lv_obj_t* tabview = lv_event_get_target(e);
    uint32_t activeTab = lv_tabview_get_tab_act(tabview);
    
    switch (activeTab) {
        case 0: app->m_currentMode = AppMode::CLOCK; break;
        case 1: app->m_currentMode = AppMode::ALARMS; break;
        case 2: app->m_currentMode = AppMode::TIMERS; break;
        case 3: app->m_currentMode = AppMode::STOPWATCH; break;
    }
}

void AlarmTimerApp::addAlarmCallback(lv_event_t* e) {
    AlarmTimerApp* app = static_cast<AlarmTimerApp*>(lv_event_get_user_data(e));
    app->showAlarmDialog();
}

void AlarmTimerApp::addTimerCallback(lv_event_t* e) {
    AlarmTimerApp* app = static_cast<AlarmTimerApp*>(lv_event_get_user_data(e));
    app->showTimerDialog();
}

void AlarmTimerApp::stopwatchStartCallback(lv_event_t* e) {
    AlarmTimerApp* app = static_cast<AlarmTimerApp*>(lv_event_get_user_data(e));
    app->startStopwatch();
}

void AlarmTimerApp::stopwatchResetCallback(lv_event_t* e) {
    AlarmTimerApp* app = static_cast<AlarmTimerApp*>(lv_event_get_user_data(e));
    app->resetStopwatch();
}

void AlarmTimerApp::stopwatchLapCallback(lv_event_t* e) {
    AlarmTimerApp* app = static_cast<AlarmTimerApp*>(lv_event_get_user_data(e));
    app->lapStopwatch();
}

void AlarmTimerApp::alarmToggleCallback(lv_event_t* e) {
    AlarmTimerApp* app = static_cast<AlarmTimerApp*>(lv_event_get_user_data(e));
    lv_obj_t* sw = lv_event_get_target(e);
    uint32_t alarmId = (uint32_t)(uintptr_t)lv_obj_get_user_data(sw);
    app->toggleAlarm(alarmId);
}

void AlarmTimerApp::timerStartCallback(lv_event_t* e) {
    AlarmTimerApp* app = static_cast<AlarmTimerApp*>(lv_event_get_user_data(e));
    lv_obj_t* btn = lv_event_get_target(e);
    uint32_t timerId = (uint32_t)(uintptr_t)lv_obj_get_user_data(btn);
    app->startTimer(timerId);
}

void AlarmTimerApp::timerStopCallback(lv_event_t* e) {
    AlarmTimerApp* app = static_cast<AlarmTimerApp*>(lv_event_get_user_data(e));
    lv_obj_t* btn = lv_event_get_target(e);
    uint32_t timerId = (uint32_t)(uintptr_t)lv_obj_get_user_data(btn);
    app->stopTimer(timerId);
}

void AlarmTimerApp::snoozeCallback(lv_event_t* e) {
    AlarmTimerApp* app = static_cast<AlarmTimerApp*>(lv_event_get_user_data(e));
    lv_obj_t* msgbox = lv_event_get_target(e);
    
    const char* btnText = lv_msgbox_get_active_btn_text(msgbox);
    if (btnText && strcmp(btnText, "Snooze") == 0) {
        app->snoozeAlarm(app->m_activeAlarmId);
    } else {
        app->dismissAlarm(app->m_activeAlarmId);
    }
    
    lv_msgbox_close(msgbox);
}

void AlarmTimerApp::toggleAlarm(uint32_t alarmId) {
    for (auto& alarm : m_alarms) {
        if (alarm.id == alarmId) {
            alarm.enabled = !alarm.enabled;
            if (alarm.enabled) {
                calculateNextAlarmTrigger(alarm);
            }
            break;
        }
    }
}

void AlarmTimerApp::startTimer(uint32_t timerId) {
    for (auto& timer : m_timers) {
        if (timer.id == timerId) {
            if (!timer.isRunning) {
                timer.isRunning = true;
                timer.startTime = time(nullptr);
            } else {
                timer.isRunning = false;
                timer.isPaused = true;
                timer.pauseTime = time(nullptr);
            }
            break;
        }
    }
}

void AlarmTimerApp::stopTimer(uint32_t timerId) {
    for (auto& timer : m_timers) {
        if (timer.id == timerId) {
            timer.isRunning = false;
            timer.isPaused = false;
            timer.remainingSeconds = timer.totalSeconds;
            break;
        }
    }
}

void AlarmTimerApp::snoozeAlarm(uint32_t alarmId) {
    for (auto& alarm : m_alarms) {
        if (alarm.id == alarmId && alarm.snoozeEnabled) {
            // Add snooze minutes to current time and set as next trigger
            alarm.nextTrigger = time(nullptr) + (alarm.snoozeMinutes * 60);
            break;
        }
    }
    stopSound();
}

void AlarmTimerApp::dismissAlarm(uint32_t alarmId) {
    stopSound();
    // If it's a repeating alarm, calculate next trigger
    for (auto& alarm : m_alarms) {
        if (alarm.id == alarmId && alarm.repeat) {
            calculateNextAlarmTrigger(alarm);
            break;
        }
    }
}

void AlarmTimerApp::calculateNextAlarmTrigger(Alarm& alarm) {
    // Placeholder implementation for calculating next alarm trigger time
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    
    timeinfo->tm_hour = alarm.hour;
    timeinfo->tm_min = alarm.minute;
    timeinfo->tm_sec = 0;
    
    alarm.nextTrigger = mktime(timeinfo);
    
    // If time has passed today, set for tomorrow
    if (alarm.nextTrigger <= now) {
        alarm.nextTrigger += 24 * 60 * 60;
    }
}

void AlarmTimerApp::showAlarmDialog(Alarm* alarm) {
    // Placeholder - would create alarm configuration dialog
    m_editingAlarm = alarm;
}

void AlarmTimerApp::hideAlarmDialog() {
    // Placeholder - would close alarm dialog
    m_editingAlarm = nullptr;
}

void AlarmTimerApp::showTimerDialog(Timer* timer) {
    // Placeholder - would create timer configuration dialog
    m_editingTimer = timer;
}

void AlarmTimerApp::hideTimerDialog() {
    // Placeholder - would close timer dialog
    m_editingTimer = nullptr;
}

// Empty callback implementations for now
void AlarmTimerApp::alarmEditCallback(lv_event_t* e) {}
void AlarmTimerApp::alarmDeleteCallback(lv_event_t* e) {}
void AlarmTimerApp::saveAlarmCallback(lv_event_t* e) {}
void AlarmTimerApp::cancelAlarmCallback(lv_event_t* e) {}
void AlarmTimerApp::timerPauseCallback(lv_event_t* e) {}
void AlarmTimerApp::timerResetCallback(lv_event_t* e) {}
void AlarmTimerApp::saveTimerCallback(lv_event_t* e) {}
void AlarmTimerApp::cancelTimerCallback(lv_event_t* e) {}
void AlarmTimerApp::stopwatchPauseCallback(lv_event_t* e) {}
void AlarmTimerApp::dismissCallback(lv_event_t* e) {}

extern "C" std::unique_ptr<BaseApp> createAlarmTimerApp() {
    return std::make_unique<AlarmTimerApp>();
}