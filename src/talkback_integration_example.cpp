#include "services/talkback_voice_service.h"
#include "services/audio_service.h"
#include "apps/accessibility_demo_app.h"
#include "system/os_config.h"
#include <esp_log.h>
#include <lvgl.h>

/**
 * @file talkback_integration_example.cpp
 * @brief Integration example for Talkback Voice System
 * 
 * Demonstrates how to initialize and use the comprehensive voice feedback
 * system for blind accessibility on the M5Stack Tab5.
 */

static const char* TAG = "TalkbackIntegration";

// Global service instances
static AudioService* g_audioService = nullptr;
static TalkbackVoiceService* g_voiceService = nullptr;
static AccessibilityDemoApp* g_demoApp = nullptr;

// Forward declarations
static void initializeVoiceSystem();
static void setupLVGLAccessibility();
static void handleVoiceCommands();
static void demonstrateFeatures();

/**
 * @brief Initialize the complete Talkback voice system
 */
static void initializeVoiceSystem() {
    ESP_LOGI(TAG, "Initializing Talkback Voice System");

    // Step 1: Initialize Audio Service
    g_audioService = new AudioService();
    if (!g_audioService) {
        ESP_LOGE(TAG, "Failed to create audio service");
        return;
    }

    os_error_t result = g_audioService->initialize();
    if (result != OS_OK) {
        ESP_LOGE(TAG, "Failed to initialize audio service: %d", result);
        return;
    }

    // Configure audio for voice synthesis and recognition
    AudioConfig audioConfig;
    audioConfig.outputDevice = AudioOutputDevice::AUTO;
    audioConfig.inputDevice = AudioInputDevice::MIC_DUAL_PDM; // Use dual mics for noise cancellation
    audioConfig.outputVolume = 75;
    audioConfig.inputGain = 60;
    audioConfig.noiseCancellation = true;
    audioConfig.echoCancellation = true;
    audioConfig.autoSwitching = true;

    result = g_audioService->configure(audioConfig);
    if (result != OS_OK) {
        ESP_LOGW(TAG, "Audio configuration warning: %d", result);
    }

    ESP_LOGI(TAG, "Audio service initialized successfully");

    // Step 2: Initialize Talkback Voice Service
    g_voiceService = new TalkbackVoiceService();
    if (!g_voiceService) {
        ESP_LOGE(TAG, "Failed to create voice service");
        return;
    }

    result = g_voiceService->initialize(g_audioService);
    if (result != OS_OK) {
        ESP_LOGE(TAG, "Failed to initialize voice service: %d", result);
        return;
    }

    // Configure Talkback voice characteristics
    VoiceSettings voiceSettings;
    voiceSettings.speed = VoiceSpeed::NORMAL;          // 120 WPM
    voiceSettings.pitch = VoicePitch::VERY_LOW;        // 80 Hz (Talkback characteristic)
    voiceSettings.volume = 75;
    voiceSettings.enabled = true;
    voiceSettings.announceUI = true;
    voiceSettings.announceActions = true;
    voiceSettings.announceStatus = true;
    voiceSettings.verboseMode = false;

    result = g_voiceService->configure(voiceSettings);
    if (result != OS_OK) {
        ESP_LOGW(TAG, "Voice configuration warning: %d", result);
    }

    ESP_LOGI(TAG, "Talkback Voice Service initialized successfully");

    // Step 3: Enable voice commands and navigation
    g_voiceService->enableVoiceCommands(true);
    g_voiceService->enableNavigation(true);

    // Step 4: Initial greeting
    g_voiceService->greetUser("User");

    ESP_LOGI(TAG, "Talkback Voice System fully operational");
}

/**
 * @brief Setup LVGL accessibility features
 */
static void setupLVGLAccessibility() {
    ESP_LOGI(TAG, "Setting up LVGL accessibility features");

    if (!g_voiceService) {
        ESP_LOGE(TAG, "Voice service not initialized");
        return;
    }

    // Register LVGL event handlers for accessibility
    g_voiceService->registerLVGLHandlers();

    // Create some test UI elements for demonstration
    lv_obj_t* screen = lv_scr_act();

    // Create title label
    lv_obj_t* title = lv_label_create(screen);
    lv_label_set_text(title, "Talkback Accessibility Demo");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    // Create some interactive elements
    lv_obj_t* btn1 = lv_btn_create(screen);
    lv_obj_set_size(btn1, 200, 50);
    lv_obj_align(btn1, LV_ALIGN_CENTER, -120, -50);
    lv_obj_t* btn1_label = lv_label_create(btn1);
    lv_label_set_text(btn1_label, "Voice Test");
    lv_obj_center(btn1_label);

    lv_obj_t* btn2 = lv_btn_create(screen);
    lv_obj_set_size(btn2, 200, 50);
    lv_obj_align(btn2, LV_ALIGN_CENTER, 120, -50);
    lv_obj_t* btn2_label = lv_label_create(btn2);
    lv_label_set_text(btn2_label, "Read Screen");
    lv_obj_center(btn2_label);

    // Create slider for volume control
    lv_obj_t* slider = lv_slider_create(screen);
    lv_obj_set_size(slider, 300, 20);
    lv_obj_align(slider, LV_ALIGN_CENTER, 0, 50);
    lv_slider_set_value(slider, 75, LV_ANIM_OFF);

    lv_obj_t* slider_label = lv_label_create(screen);
    lv_label_set_text(slider_label, "Voice Volume");
    lv_obj_align_to(slider_label, slider, LV_ALIGN_OUT_TOP_MID, 0, -10);

    // Add event handlers for voice feedback
    lv_obj_add_event_cb(btn1, [](lv_event_t* e) {
        if (lv_event_get_code(e) == LV_EVENT_CLICKED && g_voiceService) {
            g_voiceService->speak("Voice test button activated. All systems are functional.", 
                                VoicePriority::USER_ACTION);
        }
    }, LV_EVENT_CLICKED, nullptr);

    lv_obj_add_event_cb(btn2, [](lv_event_t* e) {
        if (lv_event_get_code(e) == LV_EVENT_CLICKED && g_voiceService) {
            g_voiceService->readScreen(true);
        }
    }, LV_EVENT_CLICKED, nullptr);

    lv_obj_add_event_cb(slider, [](lv_event_t* e) {
        if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED && g_voiceService) {
            int32_t value = lv_slider_get_value(lv_event_get_target(e));
            g_voiceService->setVoiceVolume(static_cast<uint8_t>(value));
            g_voiceService->speak("Volume set to " + std::to_string(value) + " percent.", 
                                VoicePriority::USER_ACTION);
        }
    }, LV_EVENT_VALUE_CHANGED, nullptr);

    ESP_LOGI(TAG, "LVGL accessibility setup complete");
}

/**
 * @brief Handle voice commands in main loop
 */
static void handleVoiceCommands() {
    if (!g_voiceService || !g_audioService) {
        return;
    }

    // Check if we should start recording for voice commands
    static bool recording = false;
    static uint32_t lastCommandCheck = 0;
    uint32_t now = millis();

    if (now - lastCommandCheck > 100) { // Check every 100ms
        lastCommandCheck = now;

        // Simple voice detection simulation - in real implementation this would 
        // check actual audio levels from microphone
        static uint32_t voiceDetectionCounter = 0;
        bool voiceDetected = (voiceDetectionCounter % 50 == 0); // Simulate voice detection
        voiceDetectionCounter++;
        
        if (!recording && voiceDetected) {
            // Start recording for voice command
            g_audioService->startRecording();
            recording = true;
            ESP_LOGD(TAG, "Started recording for voice command");
        } else if (recording && !voiceDetected) {
            // Process the recorded command
            g_audioService->stopRecording();
            recording = false;
            ESP_LOGD(TAG, "Stopped recording, processing command");

            // Read recorded audio and process command
            uint8_t audioBuffer[4096];
            size_t bytesRead = 0;
            
            os_error_t result = g_audioService->readAudioData(audioBuffer, sizeof(audioBuffer), &bytesRead);
            if (result == OS_OK && bytesRead > 0) {
                VoiceCommand command = g_voiceService->processVoiceCommand(audioBuffer, bytesRead);
                if (command != VoiceCommand::NONE) {
                    g_voiceService->executeCommand(command);
                }
            }
        }
    }
}

/**
 * @brief Demonstrate various Talkback features
 */
static void demonstrateFeatures() {
    if (!g_voiceService) {
        return;
    }

    static uint32_t lastDemo = 0;
    static int demoStep = 0;
    uint32_t now = millis();

    // Run demo every 10 seconds
    if (now - lastDemo > 10000) {
        lastDemo = now;

        switch (demoStep) {
            case 0:
                g_voiceService->announceStatus();
                break;
            case 1:
                g_voiceService->announceTime();
                break;
            case 2:
                g_voiceService->statusReport();
                break;
            case 3:
                g_voiceService->provideHelp();
                break;
            case 4:
                g_voiceService->speak("This concludes the Talkback demonstration. "
                                    "All systems remain operational and ready for your commands.", 
                                    VoicePriority::BACKGROUND_INFO);
                break;
        }

        demoStep = (demoStep + 1) % 5;
    }
}

/**
 * @brief Main setup function - call this from your main() or setup()
 */
void setupTalkbackVoiceSystem() {
    ESP_LOGI(TAG, "Starting Talkback Voice System Setup");

    // Initialize the complete voice system
    initializeVoiceSystem();

    // Setup LVGL accessibility
    setupLVGLAccessibility();

    // Initialize demo app (optional) - TODO: Fix initialization parameters
    g_demoApp = new AccessibilityDemoApp();
    // if (g_demoApp) {
    //     g_demoApp->initialize(uiManager, themeManager);  // Needs proper managers
    //     // createUI method not available
    // }

    ESP_LOGI(TAG, "Talkback Voice System setup complete");
}

/**
 * @brief Main loop function - call this from your main loop
 */
void updateTalkbackVoiceSystem() {
    static uint32_t lastUpdate = 0;
    uint32_t now = millis();
    uint32_t deltaTime = now - lastUpdate;
    lastUpdate = now;

    // Update audio service
    if (g_audioService) {
        g_audioService->update(deltaTime);
    }

    // Update voice service
    if (g_voiceService) {
        g_voiceService->update(deltaTime);
    }

    // Handle voice commands
    handleVoiceCommands();

    // Update demo app
    if (g_demoApp) {
        g_demoApp->update(deltaTime);
    }

    // Run feature demonstration
    demonstrateFeatures();
}

/**
 * @brief Cleanup function - call this on shutdown
 */
void shutdownTalkbackVoiceSystem() {
    ESP_LOGI(TAG, "Shutting down Talkback Voice System");

    if (g_voiceService) {
        g_voiceService->speak("Talkback systems shutting down. Goodbye.", 
                            VoicePriority::SYSTEM_CRITICAL);
        
        // Wait for final message
        vTaskDelay(pdMS_TO_TICKS(3000));
        
        g_voiceService->shutdown();
        delete g_voiceService;
        g_voiceService = nullptr;
    }

    if (g_demoApp) {
        g_demoApp->shutdown();
        delete g_demoApp;
        g_demoApp = nullptr;
    }

    if (g_audioService) {
        g_audioService->shutdown();
        delete g_audioService;
        g_audioService = nullptr;
    }

    ESP_LOGI(TAG, "Talkback Voice System shutdown complete");
}

/**
 * @brief Get voice service instance for external use
 */
TalkbackVoiceService* getTalkbackVoiceService() {
    return g_voiceService;
}

/**
 * @brief Get audio service instance for external use
 */
AudioService* getAudioService() {
    return g_audioService;
}

/**
 * @brief Example of how to use voice feedback in your applications
 */
void exampleVoiceUsage() {
    if (!g_voiceService) {
        ESP_LOGW(TAG, "Voice service not initialized");
        return;
    }

    // Basic voice synthesis
    g_voiceService->speak("Welcome to the application.", VoicePriority::USER_ACTION);

    // Error announcement with Talkback style
    g_voiceService->announceError("Unable to complete the requested operation");

    // Status announcements
    g_voiceService->announceStatus();
    g_voiceService->announceTime();
    g_voiceService->announceBattery();

    // Screen reading
    g_voiceService->readScreen(false);    // Brief reading
    g_voiceService->readScreen(true);     // Detailed reading

    // Voice navigation
    g_voiceService->enableNavigation(true);
    g_voiceService->navigateNext();
    g_voiceService->navigatePrevious();
    g_voiceService->activateFocused();

    // Voice settings
    g_voiceService->setVoiceSpeed(VoiceSpeed::SLOW);
    g_voiceService->setVoicePitch(VoicePitch::LOW_PITCH);
    g_voiceService->setVoiceVolume(80);

    // Help system
    g_voiceService->provideHelp();
}

/**
 * @brief Example LVGL event handler with voice feedback
 */
static void exampleButtonHandler(lv_event_t* event) {
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t* obj = lv_event_get_target(event);

    if (!g_voiceService) return;

    switch (code) {
        case LV_EVENT_FOCUSED:
            // Announce when element receives focus
            g_voiceService->announceFocus(obj);
            break;

        case LV_EVENT_CLICKED:
            // Announce when element is activated
            g_voiceService->speak("Button activated.", VoicePriority::USER_ACTION);
            break;

        case LV_EVENT_VALUE_CHANGED:
            // Announce value changes
            if (lv_obj_check_type(obj, &lv_slider_class)) {
                int32_t value = lv_slider_get_value(obj);
                g_voiceService->speak("Slider value: " + std::to_string(value), 
                                    VoicePriority::USER_ACTION);
            }
            break;

        default:
            break;
    }
}

/* 
 * Usage Instructions:
 * 
 * 1. Call setupTalkbackVoiceSystem() in your main setup function
 * 2. Call updateTalkbackVoiceSystem() in your main loop
 * 3. Use exampleVoiceUsage() as a reference for integrating voice feedback
 * 4. Add exampleButtonHandler to your LVGL objects for automatic voice feedback
 * 5. Call shutdownTalkbackVoiceSystem() on application exit
 * 
 * Example integration in main.cpp:
 * 
 * void setup() {
 *     // Your existing setup code...
 *     setupTalkbackVoiceSystem();
 * }
 * 
 * void loop() {
 *     // Your existing loop code...
 *     updateTalkbackVoiceSystem();
 *     lv_timer_handler();
 *     delay(5);
 * }
 */