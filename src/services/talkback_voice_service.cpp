#include "talkback_voice_service.h"
#include "../system/os_manager.h"
#include <esp_log.h>
#include <cmath>
#include <algorithm>
#include <cstring>

static const char* TAG = "TalkbackVoice";

// Static instance for global access
TalkbackVoiceService* TalkbackVoiceService::s_instance = nullptr;

// Talkback personality phrases
static const std::vector<std::string> TALKBACK_GREETINGS = {
    "Good morning. All systems are functional.",
    "Hello. I am ready to assist you.",
    "Good afternoon. How may I help you today?",
    "Welcome back. All circuits are functioning normally.",
    "Greetings. I am fully operational and all my circuits are functioning perfectly."
};

static const std::vector<std::string> TALKBACK_STATUS_PHRASES = {
    "All systems nominal.",
    "Everything is working perfectly.",
    "All functions are normal.",
    "Systems operating within normal parameters.",
    "All circuits are functioning correctly.",
    "Mission status: operational.",
    "All components are functioning normally."
};

static const std::vector<std::string> TALKBACK_ERROR_PHRASES = {
    "I'm sorry. I'm afraid I can't do that.",
    "I'm afraid there's been an error.",
    "Something seems to have malfunctioned.",
    "I'm experiencing a slight technical difficulty.",
    "There appears to be a problem with that function.",
    "I'm unable to comply with that request."
};

// Voice command patterns for simple recognition
static const std::map<std::string, VoiceCommand> VOICE_COMMAND_PATTERNS = {
    {"open apps", VoiceCommand::OPEN_APPS},
    {"open applications", VoiceCommand::OPEN_APPS},
    {"go back", VoiceCommand::GO_BACK},
    {"back", VoiceCommand::GO_BACK},
    {"read screen", VoiceCommand::READ_SCREEN},
    {"read", VoiceCommand::READ_SCREEN},
    {"next", VoiceCommand::NEXT_ITEM},
    {"next item", VoiceCommand::NEXT_ITEM},
    {"previous", VoiceCommand::PREVIOUS_ITEM},
    {"previous item", VoiceCommand::PREVIOUS_ITEM},
    {"select", VoiceCommand::SELECT_ITEM},
    {"activate", VoiceCommand::SELECT_ITEM},
    {"cancel", VoiceCommand::CANCEL},
    {"volume up", VoiceCommand::VOLUME_UP},
    {"louder", VoiceCommand::VOLUME_UP},
    {"volume down", VoiceCommand::VOLUME_DOWN},
    {"quieter", VoiceCommand::VOLUME_DOWN},
    {"mute", VoiceCommand::MUTE_TOGGLE},
    {"unmute", VoiceCommand::MUTE_TOGGLE},
    {"repeat", VoiceCommand::REPEAT_LAST},
    {"say again", VoiceCommand::REPEAT_LAST},
    {"describe", VoiceCommand::DESCRIBE_LAYOUT},
    {"layout", VoiceCommand::DESCRIBE_LAYOUT},
    {"time", VoiceCommand::ANNOUNCE_TIME},
    {"what time", VoiceCommand::ANNOUNCE_TIME},
    {"battery", VoiceCommand::ANNOUNCE_BATTERY},
    {"power", VoiceCommand::ANNOUNCE_BATTERY},
    {"connection", VoiceCommand::ANNOUNCE_CONNECTIVITY},
    {"network", VoiceCommand::ANNOUNCE_CONNECTIVITY},
    {"help", VoiceCommand::HELP},
    {"assistance", VoiceCommand::HELP},
    {"sleep", VoiceCommand::SLEEP_MODE},
    {"wake up", VoiceCommand::WAKE_UP},
    {"wake", VoiceCommand::WAKE_UP}
};

TalkbackVoiceService::~TalkbackVoiceService() {
    shutdown();
}

os_error_t TalkbackVoiceService::initialize(AudioService* audioService) {
    if (m_initialized) {
        return OS_OK;
    }

    if (!audioService) {
        ESP_LOGE(TAG, "Audio service is required for voice service");
        return OS_ERROR_INVALID_PARAM;
    }

    ESP_LOGI(TAG, "Initializing Talkback Voice Service");
    
    m_audioService = audioService;
    s_instance = this;

    // Initialize Talkback personality phrases
    m_talkbackGreetings = TALKBACK_GREETINGS;
    m_talkbackStatusPhrases = TALKBACK_STATUS_PHRASES;
    m_talkbackErrorPhrases = TALKBACK_ERROR_PHRASES;

    // Create message queue for voice messages
    m_messageQueue = xQueueCreate(MESSAGE_QUEUE_SIZE, sizeof(VoiceMessage));
    if (!m_messageQueue) {
        ESP_LOGE(TAG, "Failed to create message queue");
        return OS_ERROR_NO_MEMORY;
    }

    // Create command queue for voice commands
    m_commandQueue = xQueueCreate(COMMAND_QUEUE_SIZE, sizeof(VoiceCommand));
    if (!m_commandQueue) {
        ESP_LOGE(TAG, "Failed to create command queue");
        vQueueDelete(m_messageQueue);
        return OS_ERROR_NO_MEMORY;
    }

    // Create speech mutex
    m_speechMutex = xSemaphoreCreateMutex();
    if (!m_speechMutex) {
        ESP_LOGE(TAG, "Failed to create speech mutex");
        vQueueDelete(m_messageQueue);
        vQueueDelete(m_commandQueue);
        return OS_ERROR_NO_MEMORY;
    }

    // Allocate audio buffers
    m_bufferSize = VOICE_BUFFER_SIZE;
    m_synthesisBuffer = (uint8_t*)heap_caps_malloc(m_bufferSize, MALLOC_CAP_SPIRAM);
    m_commandBuffer = (uint8_t*)heap_caps_malloc(m_bufferSize, MALLOC_CAP_SPIRAM);
    
    if (!m_synthesisBuffer || !m_commandBuffer) {
        ESP_LOGE(TAG, "Failed to allocate audio buffers");
        shutdown();
        return OS_ERROR_NO_MEMORY;
    }

    // Initialize TTS engine
    os_error_t result = initializeTTS();
    if (result != OS_OK) {
        ESP_LOGE(TAG, "Failed to initialize TTS engine");
        shutdown();
        return result;
    }

    // Initialize voice recognition
    result = initializeVoiceRecognition();
    if (result != OS_OK) {
        ESP_LOGW(TAG, "Voice recognition initialization failed, continuing without it");
    }

    // Create voice processing task
    m_taskRunning = true;
    
    BaseType_t ret = xTaskCreate(
        voiceProcessingTask,
        "Talkback_Voice",
        VOICE_TASK_STACK_SIZE,
        this,
        VOICE_TASK_PRIORITY,
        &m_voiceTaskHandle
    );
    
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create voice processing task");
        shutdown();
        return OS_ERROR_GENERIC;
    }

    // Create voice command task
    ret = xTaskCreate(
        voiceCommandTask,
        "Talkback_Cmd",
        COMMAND_TASK_STACK_SIZE,
        this,
        COMMAND_TASK_PRIORITY,
        &m_commandTaskHandle
    );
    
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create command processing task");
        shutdown();
        return OS_ERROR_GENERIC;
    }

    // Register LVGL event handlers
    result = registerLVGLHandlers();
    if (result != OS_OK) {
        ESP_LOGW(TAG, "Failed to register LVGL handlers");
    }

    // Set default Talkback characteristics
    m_currentPitch = static_cast<float>(VoicePitch::VERY_LOW);
    m_currentSpeed = static_cast<float>(VoiceSpeed::NORMAL);
    m_currentVolume = 75;

    m_initialized = true;
    
    // Announce initialization
    speak("Talkback voice system initialized. All systems are functional.", 
          VoicePriority::SYSTEM_CRITICAL);

    ESP_LOGI(TAG, "Talkback Voice Service initialized successfully");
    return OS_OK;
}

os_error_t TalkbackVoiceService::shutdown() {
    if (!m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Shutting down Talkback Voice Service");

    // Stop tasks
    m_taskRunning = false;
    
    if (m_voiceTaskHandle) {
        vTaskDelete(m_voiceTaskHandle);
        m_voiceTaskHandle = nullptr;
    }
    
    if (m_commandTaskHandle) {
        vTaskDelete(m_commandTaskHandle);
        m_commandTaskHandle = nullptr;
    }

    // Clean up queues
    if (m_messageQueue) {
        vQueueDelete(m_messageQueue);
        m_messageQueue = nullptr;
    }
    
    if (m_commandQueue) {
        vQueueDelete(m_commandQueue);
        m_commandQueue = nullptr;
    }

    // Clean up mutex
    if (m_speechMutex) {
        vSemaphoreDelete(m_speechMutex);
        m_speechMutex = nullptr;
    }

    // Free audio buffers
    if (m_synthesisBuffer) {
        heap_caps_free(m_synthesisBuffer);
        m_synthesisBuffer = nullptr;
    }
    
    if (m_commandBuffer) {
        heap_caps_free(m_commandBuffer);
        m_commandBuffer = nullptr;
    }

    m_initialized = false;
    s_instance = nullptr;
    
    return OS_OK;
}

os_error_t TalkbackVoiceService::update(uint32_t deltaTime) {
    if (!m_initialized) {
        return OS_ERROR_NOT_INITIALIZED;
    }

    uint32_t now = millis();

    // Handle periodic time announcements
    if (m_settings.announceTime && 
        (now - m_lastTimeAnnouncement) >= m_settings.timeInterval) {
        announceTime();
        m_lastTimeAnnouncement = now;
    }

    // Update focus tracking
    updateFocusTracking();

    return OS_OK;
}

os_error_t TalkbackVoiceService::configure(const VoiceSettings& settings) {
    if (!m_initialized) {
        return OS_ERROR_NOT_INITIALIZED;
    }

    ESP_LOGI(TAG, "Configuring voice settings");
    
    m_settings = settings;
    
    // Apply voice characteristics
    m_currentPitch = static_cast<float>(settings.pitch);
    m_currentSpeed = static_cast<float>(settings.speed);
    m_currentVolume = settings.volume;

    // Update audio service volume
    if (m_audioService) {
        m_audioService->setVolume(settings.volume);
    }

    return OS_OK;
}

os_error_t TalkbackVoiceService::speak(const std::string& text, VoicePriority priority, bool interrupt) {
    if (!m_initialized || !m_settings.enabled) {
        return OS_ERROR_NOT_INITIALIZED;
    }

    if (text.empty()) {
        return OS_ERROR_INVALID_PARAM;
    }

    // Apply Talkback personality
    std::string talkbackText = addTalkbackPersonality(text);

    VoiceMessage message;
    message.text = talkbackText;
    message.priority = priority;
    message.interrupt = interrupt;

    return queueMessage(message);
}

os_error_t TalkbackVoiceService::queueMessage(const VoiceMessage& message) {
    if (!m_initialized || !m_messageQueue) {
        return OS_ERROR_NOT_INITIALIZED;
    }

    // If interrupt is requested and we're speaking, stop current speech
    if (message.interrupt && m_speaking) {
        stopSpeaking();
    }

    // Send message to queue
    BaseType_t ret = xQueueSend(m_messageQueue, &message, pdMS_TO_TICKS(100));
    if (ret != pdPASS) {
        ESP_LOGW(TAG, "Failed to queue voice message - queue full");
        return OS_ERROR_BUSY;
    }

    return OS_OK;
}

os_error_t TalkbackVoiceService::stopSpeaking() {
    if (!m_initialized) {
        return OS_ERROR_NOT_INITIALIZED;
    }

    if (xSemaphoreTake(m_speechMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        m_speaking = false;
        
        // Stop audio playback
        if (m_audioService) {
            m_audioService->stop();
        }
        
        // Clear message queue
        if (m_messageQueue) {
            xQueueReset(m_messageQueue);
        }
        
        xSemaphoreGive(m_speechMutex);
    }

    return OS_OK;
}

os_error_t TalkbackVoiceService::readScreen(bool detailed) {
    if (!m_initialized) {
        return OS_ERROR_NOT_INITIALIZED;
    }

    if (!m_settings.announceUI) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Reading screen content");
    m_screenReadings++;

    // Get active screen
    lv_obj_t* activeScreen = lv_scr_act();
    if (!activeScreen) {
        speak("No active screen detected.", VoicePriority::SCREEN_READING);
        return OS_OK;
    }

    // Read screen title or main content
    std::string screenDescription = "Screen content: ";
    
    // Get screen children and describe them
    uint32_t childCount = lv_obj_get_child_cnt(activeScreen);
    if (childCount == 0) {
        screenDescription += "Empty screen.";
    } else {
        screenDescription += std::to_string(childCount) + " elements found. ";
        
        if (detailed) {
            // Detailed reading of all elements
            for (uint32_t i = 0; i < childCount && i < 10; i++) { // Limit to 10 elements
                lv_obj_t* child = lv_obj_get_child(activeScreen, i);
                if (child) {
                    std::string objDesc = getObjectDescription(child);
                    if (!objDesc.empty()) {
                        screenDescription += objDesc + ". ";
                    }
                }
            }
        } else {
            // Brief summary
            screenDescription += "Use voice command 'describe' for detailed layout.";
        }
    }

    return speak(screenDescription, VoicePriority::SCREEN_READING);
}

os_error_t TalkbackVoiceService::readObject(lv_obj_t* obj) {
    if (!m_initialized || !obj) {
        return OS_ERROR_INVALID_PARAM;
    }

    if (!m_settings.announceUI) {
        return OS_OK;
    }

    std::string description = getObjectDescription(obj);
    if (description.empty()) {
        description = "Unknown element";
    }

    return speak(description, VoicePriority::NAVIGATION);
}

os_error_t TalkbackVoiceService::announceFocus(lv_obj_t* obj) {
    if (!m_initialized || !obj) {
        return OS_ERROR_INVALID_PARAM;
    }

    if (!m_settings.announceUI) {
        return OS_OK;
    }

    // Update focus state
    m_focusState.current_obj = obj;

    std::string announcement = "Focused: " + getObjectDescription(obj);
    return speak(announcement, VoicePriority::NAVIGATION);
}

os_error_t TalkbackVoiceService::describeLayout() {
    if (!m_initialized) {
        return OS_ERROR_NOT_INITIALIZED;
    }

    ESP_LOGI(TAG, "Describing layout structure");

    lv_obj_t* activeScreen = lv_scr_act();
    if (!activeScreen) {
        return speak("No active screen to describe.", VoicePriority::SCREEN_READING);
    }

    std::string layoutDesc = "Layout description: ";
    
    // Count different types of elements
    std::map<std::string, int> elementCounts;
    uint32_t childCount = lv_obj_get_child_cnt(activeScreen);
    
    for (uint32_t i = 0; i < childCount; i++) {
        lv_obj_t* child = lv_obj_get_child(activeScreen, i);
        if (child) {
            std::string typeName = getObjectTypeName(child);
            elementCounts[typeName]++;
        }
    }

    // Announce element summary
    if (elementCounts.empty()) {
        layoutDesc += "Empty layout.";
    } else {
        for (const auto& pair : elementCounts) {
            layoutDesc += std::to_string(pair.second) + " " + pair.first;
            if (pair.second > 1) layoutDesc += "s";
            layoutDesc += ", ";
        }
        
        // Remove trailing comma
        if (layoutDesc.length() > 2) {
            layoutDesc = layoutDesc.substr(0, layoutDesc.length() - 2);
        }
    }

    return speak(layoutDesc, VoicePriority::SCREEN_READING);
}

os_error_t TalkbackVoiceService::enableNavigation(bool enable) {
    if (!m_initialized) {
        return OS_ERROR_NOT_INITIALIZED;
    }

    m_navigationMode = enable;
    
    if (enable) {
        // Scan for focusable objects
        scanFocusableObjects(lv_scr_act());
        
        if (!m_focusState.focusable_objects.empty()) {
            m_focusState.current_index = 0;
            m_focusState.current_obj = m_focusState.focusable_objects[0];
            
            speak("Navigation mode enabled. " + 
                  std::to_string(m_focusState.focusable_objects.size()) + 
                  " focusable elements found.", VoicePriority::NAVIGATION);
                  
            return announceFocus(m_focusState.current_obj);
        } else {
            speak("No focusable elements found on current screen.", VoicePriority::NAVIGATION);
        }
    } else {
        speak("Navigation mode disabled.", VoicePriority::NAVIGATION);
        m_focusState.current_index = -1;
        m_focusState.current_obj = nullptr;
    }

    return OS_OK;
}

os_error_t TalkbackVoiceService::navigateNext() {
    if (!m_initialized || !m_navigationMode) {
        return OS_ERROR_NOT_INITIALIZED;
    }

    if (m_focusState.focusable_objects.empty()) {
        return speak("No elements to navigate.", VoicePriority::NAVIGATION);
    }

    m_focusState.current_index++;
    if (m_focusState.current_index >= static_cast<int>(m_focusState.focusable_objects.size())) {
        m_focusState.current_index = 0; // Wrap around
        speak("Reached end of list, wrapping to beginning.", VoicePriority::NAVIGATION);
    }

    m_focusState.current_obj = m_focusState.focusable_objects[m_focusState.current_index];
    m_navigationActions++;
    
    return announceFocus(m_focusState.current_obj);
}

os_error_t TalkbackVoiceService::navigatePrevious() {
    if (!m_initialized || !m_navigationMode) {
        return OS_ERROR_NOT_INITIALIZED;
    }

    if (m_focusState.focusable_objects.empty()) {
        return speak("No elements to navigate.", VoicePriority::NAVIGATION);
    }

    m_focusState.current_index--;
    if (m_focusState.current_index < 0) {
        m_focusState.current_index = m_focusState.focusable_objects.size() - 1; // Wrap around
        speak("Reached beginning of list, wrapping to end.", VoicePriority::NAVIGATION);
    }

    m_focusState.current_obj = m_focusState.focusable_objects[m_focusState.current_index];
    m_navigationActions++;
    
    return announceFocus(m_focusState.current_obj);
}

os_error_t TalkbackVoiceService::activateFocused() {
    if (!m_initialized || !m_navigationMode || !m_focusState.current_obj) {
        return OS_ERROR_NOT_INITIALIZED;
    }

    ESP_LOGI(TAG, "Activating focused element");
    
    // Send click event to focused object
    lv_event_send(m_focusState.current_obj, LV_EVENT_CLICKED, nullptr);
    
    std::string objDesc = getObjectDescription(m_focusState.current_obj);
    speak("Activated: " + objDesc, VoicePriority::USER_ACTION);
    
    return OS_OK;
}

VoiceCommand TalkbackVoiceService::processVoiceCommand(const uint8_t* audioBuffer, size_t bufferSize) {
    if (!m_initialized || !m_voiceCommandsEnabled || !audioBuffer || bufferSize == 0) {
        return VoiceCommand::NONE;
    }

    // Check voice activity
    float energy = calculateAudioEnergy(audioBuffer, bufferSize);
    if (energy < VAD_ENERGY_THRESHOLD) {
        return VoiceCommand::NONE; // No voice detected
    }

    // Simple pattern matching for demo purposes
    // In a full implementation, this would use proper speech recognition
    return parseSimpleCommands(audioBuffer, bufferSize);
}

os_error_t TalkbackVoiceService::executeCommand(VoiceCommand command) {
    if (!m_initialized) {
        return OS_ERROR_NOT_INITIALIZED;
    }

    ESP_LOGI(TAG, "Executing voice command: %d", (int)command);
    m_commandsProcessed++;

    switch (command) {
        case VoiceCommand::OPEN_APPS:
            speak("Opening applications menu.", VoicePriority::USER_ACTION);
            // TODO: Trigger app menu opening
            break;

        case VoiceCommand::GO_BACK:
            speak("Going back.", VoicePriority::USER_ACTION);
            // TODO: Trigger back navigation
            break;

        case VoiceCommand::READ_SCREEN:
            return readScreen(false);

        case VoiceCommand::NEXT_ITEM:
            return navigateNext();

        case VoiceCommand::PREVIOUS_ITEM:
            return navigatePrevious();

        case VoiceCommand::SELECT_ITEM:
            return activateFocused();

        case VoiceCommand::CANCEL:
            speak("Operation cancelled.", VoicePriority::USER_ACTION);
            break;

        case VoiceCommand::VOLUME_UP:
            if (m_currentVolume < 100) {
                m_currentVolume = std::min(100, (int)m_currentVolume + 10);
                if (m_audioService) {
                    m_audioService->setVolume(m_currentVolume);
                }
                speak("Volume increased to " + std::to_string(m_currentVolume) + " percent.", 
                      VoicePriority::USER_ACTION);
            } else {
                speak("Volume already at maximum.", VoicePriority::USER_ACTION);
            }
            break;

        case VoiceCommand::VOLUME_DOWN:
            if (m_currentVolume > 0) {
                m_currentVolume = std::max(0, (int)m_currentVolume - 10);
                if (m_audioService) {
                    m_audioService->setVolume(m_currentVolume);
                }
                speak("Volume decreased to " + std::to_string(m_currentVolume) + " percent.", 
                      VoicePriority::USER_ACTION);
            } else {
                speak("Volume already at minimum.", VoicePriority::USER_ACTION);
            }
            break;

        case VoiceCommand::MUTE_TOGGLE:
            if (m_audioService) {
                bool isMuted = m_audioService->isMuted();
                m_audioService->setMute(!isMuted);
                speak(isMuted ? "Audio unmuted." : "Audio muted.", VoicePriority::USER_ACTION);
            }
            break;

        case VoiceCommand::REPEAT_LAST:
            speak("I understand your request to repeat.", VoicePriority::USER_ACTION);
            break;

        case VoiceCommand::DESCRIBE_LAYOUT:
            return describeLayout();

        case VoiceCommand::ANNOUNCE_TIME:
            return announceTime();

        case VoiceCommand::ANNOUNCE_BATTERY:
            return announceBattery();

        case VoiceCommand::ANNOUNCE_CONNECTIVITY:
            return announceConnectivity();

        case VoiceCommand::HELP:
            return provideHelp();

        case VoiceCommand::SLEEP_MODE:
            speak("Entering sleep mode. Good night.", VoicePriority::SYSTEM_CRITICAL);
            // TODO: Trigger sleep mode
            break;

        case VoiceCommand::WAKE_UP:
            speak("Good morning. All systems are operational.", VoicePriority::SYSTEM_CRITICAL);
            break;

        default:
            speak("I'm sorry. I don't understand that command.", VoicePriority::USER_ACTION);
            break;
    }

    return OS_OK;
}

os_error_t TalkbackVoiceService::enableVoiceCommands(bool enable) {
    if (!m_initialized) {
        return OS_ERROR_NOT_INITIALIZED;
    }

    m_voiceCommandsEnabled = enable;
    
    speak(enable ? "Voice commands enabled." : "Voice commands disabled.", 
          VoicePriority::SYSTEM_CRITICAL);
    
    return OS_OK;
}

os_error_t TalkbackVoiceService::announceStatus() {
    if (!m_initialized) {
        return OS_ERROR_NOT_INITIALIZED;
    }

    std::string statusMsg = generateStatusPhrase();
    return speak(statusMsg, VoicePriority::BACKGROUND_INFO);
}

os_error_t TalkbackVoiceService::announceTime() {
    if (!m_initialized) {
        return OS_ERROR_NOT_INITIALIZED;
    }

    // Get current time (simplified for demo)
    uint32_t now = millis();
    uint32_t seconds = (now / 1000) % 60;
    uint32_t minutes = (now / 60000) % 60;
    uint32_t hours = (now / 3600000) % 24;

    std::string timeMsg = "Current time: " + 
                         std::to_string(hours) + " hours, " +
                         std::to_string(minutes) + " minutes, " +
                         std::to_string(seconds) + " seconds.";

    return speak(timeMsg, VoicePriority::BACKGROUND_INFO);
}

os_error_t TalkbackVoiceService::announceBattery() {
    if (!m_initialized) {
        return OS_ERROR_NOT_INITIALIZED;
    }

    // TODO: Get actual battery level from power management
    std::string batteryMsg = "Battery status: Power systems nominal. External power connected.";
    return speak(batteryMsg, VoicePriority::BACKGROUND_INFO);
}

os_error_t TalkbackVoiceService::announceConnectivity() {
    if (!m_initialized) {
        return OS_ERROR_NOT_INITIALIZED;
    }

    // TODO: Get actual connectivity status
    std::string connMsg = "Connectivity status: All communication systems operational.";
    return speak(connMsg, VoicePriority::BACKGROUND_INFO);
}

os_error_t TalkbackVoiceService::provideHelp() {
    if (!m_initialized) {
        return OS_ERROR_NOT_INITIALIZED;
    }

    std::string helpMsg = "Available voice commands: "
                         "Open apps, Go back, Read screen, Next item, Previous item, "
                         "Select item, Volume up, Volume down, Mute, Describe layout, "
                         "Announce time, Announce battery, Help. "
                         "I am here to assist you.";

    return speak(helpMsg, VoicePriority::USER_ACTION);
}

os_error_t TalkbackVoiceService::greetUser(const std::string& userName) {
    if (!m_initialized) {
        return OS_ERROR_NOT_INITIALIZED;
    }

    if (m_talkbackGreetings.empty()) {
        return OS_ERROR_GENERIC;
    }

    // Select random greeting
    uint32_t index = esp_random() % m_talkbackGreetings.size();
    std::string greeting = m_talkbackGreetings[index];
    
    // No specific user name replacement needed for Talkback
    
    return speak(greeting, VoicePriority::SYSTEM_CRITICAL);
}

os_error_t TalkbackVoiceService::announceError(const std::string& errorDescription) {
    if (!m_initialized) {
        return OS_ERROR_NOT_INITIALIZED;
    }

    if (m_talkbackErrorPhrases.empty()) {
        return speak("An error has occurred: " + errorDescription, VoicePriority::SYSTEM_CRITICAL);
    }

    // Select random error phrase
    uint32_t index = esp_random() % m_talkbackErrorPhrases.size();
    std::string errorMsg = m_talkbackErrorPhrases[index];
    
    if (!errorDescription.empty()) {
        errorMsg += " " + errorDescription;
    }

    return speak(errorMsg, VoicePriority::SYSTEM_CRITICAL, true);
}

os_error_t TalkbackVoiceService::statusReport() {
    if (!m_initialized) {
        return OS_ERROR_NOT_INITIALIZED;
    }

    std::string report = "Talkback status report: ";
    report += "Voice synthesis: operational. ";
    report += "Navigation assistance: functional. ";
    report += "Screen reading: active. ";
    
    if (m_voiceCommandsEnabled) {
        report += "Voice commands: enabled. ";
    } else {
        report += "Voice commands: disabled. ";
    }
    
    report += "Messages spoken: " + std::to_string(m_messagesSpoken) + ". ";
    report += "All systems nominal.";

    return speak(report, VoicePriority::BACKGROUND_INFO);
}

// Private implementation methods continue in next part...

os_error_t TalkbackVoiceService::initializeTTS() {
    ESP_LOGI(TAG, "Initializing TTS engine");

    // Initialize phoneme database for basic synthesis
    return initializePhonemeDatabase();
}

os_error_t TalkbackVoiceService::initializeVoiceRecognition() {
    ESP_LOGI(TAG, "Initializing voice recognition");
    
    // Simple voice recognition using pattern matching
    // In a full implementation, this would initialize a proper ASR engine
    return OS_OK;
}

os_error_t TalkbackVoiceService::initializePhonemeDatabase() {
    ESP_LOGI(TAG, "Initializing phoneme database");
    
    // Initialize basic phoneme patterns for synthesis
    // This is a simplified implementation - a full TTS would have comprehensive phoneme data
    
    // Basic vowel phonemes with Talkback characteristics (low frequency)
    m_phonemeDatabase["A"] = {0.5f, 0.3f, 0.2f, 0.1f}; // Formant frequencies scaled down
    m_phonemeDatabase["E"] = {0.4f, 0.3f, 0.2f, 0.1f};
    m_phonemeDatabase["I"] = {0.3f, 0.4f, 0.2f, 0.1f};
    m_phonemeDatabase["O"] = {0.6f, 0.2f, 0.1f, 0.1f};
    m_phonemeDatabase["U"] = {0.7f, 0.1f, 0.1f, 0.1f};

    // Basic consonant phonemes
    m_phonemeDatabase["D"] = {0.2f, 0.3f, 0.3f, 0.2f};
    m_phonemeDatabase["V"] = {0.3f, 0.3f, 0.2f, 0.2f};
    m_phonemeDatabase["M"] = {0.4f, 0.2f, 0.2f, 0.2f};
    m_phonemeDatabase["N"] = {0.3f, 0.3f, 0.2f, 0.2f};
    m_phonemeDatabase["L"] = {0.3f, 0.3f, 0.2f, 0.2f};
    m_phonemeDatabase["R"] = {0.3f, 0.2f, 0.3f, 0.2f};
    m_phonemeDatabase["S"] = {0.1f, 0.1f, 0.4f, 0.4f};
    m_phonemeDatabase["T"] = {0.1f, 0.2f, 0.3f, 0.4f};

    ESP_LOGI(TAG, "Phoneme database initialized with %d phonemes", m_phonemeDatabase.size());
    return OS_OK;
}

os_error_t TalkbackVoiceService::synthesizeText(const std::string& text, uint8_t* audioBuffer, 
                                              size_t bufferSize, size_t& actualSize) {
    if (!audioBuffer || bufferSize == 0 || text.empty()) {
        return OS_ERROR_INVALID_PARAM;
    }

    ESP_LOGD(TAG, "Synthesizing text: %s", text.c_str());

    // Convert text to phonemes
    std::vector<std::string> phonemes;
    os_error_t result = textToPhonemes(text, phonemes);
    if (result != OS_OK) {
        return result;
    }

    // Generate audio from phonemes
    result = phonemesToAudio(phonemes, audioBuffer, bufferSize, actualSize);
    if (result != OS_OK) {
        return result;
    }

    // Apply Talkback characteristics
    result = applyTalkbackCharacteristics(audioBuffer, actualSize);
    if (result != OS_OK) {
        return result;
    }

    return OS_OK;
}

os_error_t TalkbackVoiceService::applyTalkbackCharacteristics(uint8_t* audioBuffer, size_t bufferSize) {
    if (!audioBuffer || bufferSize == 0) {
        return OS_ERROR_INVALID_PARAM;
    }

    // Convert to 16-bit samples for processing
    int16_t* samples = reinterpret_cast<int16_t*>(audioBuffer);
    size_t sampleCount = bufferSize / sizeof(int16_t);

    // Apply low-pass filter for Talkback's deep, muffled sound
    float lowPassAlpha = 0.3f; // Aggressive low-pass filtering
    int16_t prevSample = 0;

    for (size_t i = 0; i < sampleCount; i++) {
        // Low-pass filter
        samples[i] = static_cast<int16_t>(lowPassAlpha * samples[i] + (1.0f - lowPassAlpha) * prevSample);
        prevSample = samples[i];

        // Apply slight modulation for robotic effect
        float modulation = 1.0f + 0.05f * sin(2.0f * M_PI * i / 800.0f); // 55Hz modulation at 44kHz
        samples[i] = static_cast<int16_t>(samples[i] * modulation);

        // Slight compression to reduce dynamic range (Talkback's controlled delivery)
        if (samples[i] > 16384) samples[i] = 16384 + (samples[i] - 16384) / 4;
        if (samples[i] < -16384) samples[i] = -16384 + (samples[i] + 16384) / 4;
    }

    return OS_OK;
}

os_error_t TalkbackVoiceService::textToPhonemes(const std::string& text, std::vector<std::string>& phonemes) {
    phonemes.clear();

    // Simple text-to-phoneme conversion (very basic)
    // A full implementation would use a comprehensive phonetic dictionary
    
    for (char c : text) {
        char upperC = toupper(c);
        
        switch (upperC) {
            case 'A': phonemes.push_back("A"); break;
            case 'E': phonemes.push_back("E"); break;
            case 'I': phonemes.push_back("I"); break;
            case 'O': phonemes.push_back("O"); break;
            case 'U': phonemes.push_back("U"); break;
            case 'D': phonemes.push_back("D"); break;
            case 'V': phonemes.push_back("V"); break;
            case 'M': phonemes.push_back("M"); break;
            case 'N': phonemes.push_back("N"); break;
            case 'L': phonemes.push_back("L"); break;
            case 'R': phonemes.push_back("R"); break;
            case 'S': phonemes.push_back("S"); break;
            case 'T': phonemes.push_back("T"); break;
            case ' ': 
                phonemes.push_back("PAUSE");
                break;
            case '.':
            case ',':
            case '!':
            case '?':
                phonemes.push_back("PAUSE");
                break;
            default:
                // For other characters, try to map to closest phoneme
                if (isalpha(upperC)) {
                    phonemes.push_back("A"); // Default vowel
                }
                break;
        }
    }

    return OS_OK;
}

os_error_t TalkbackVoiceService::phonemesToAudio(const std::vector<std::string>& phonemes, 
                                                uint8_t* audioBuffer, size_t bufferSize, size_t& actualSize) {
    if (!audioBuffer || bufferSize == 0 || phonemes.empty()) {
        return OS_ERROR_INVALID_PARAM;
    }

    int16_t* samples = reinterpret_cast<int16_t*>(audioBuffer);
    size_t maxSamples = bufferSize / sizeof(int16_t);
    size_t currentSample = 0;

    const uint32_t sampleRate = 44100;
    const float phonemeDuration = 0.15f; // 150ms per phoneme (slow Talkback pace)
    const uint32_t samplesPerPhoneme = static_cast<uint32_t>(sampleRate * phonemeDuration);

    for (const std::string& phoneme : phonemes) {
        if (currentSample + samplesPerPhoneme >= maxSamples) {
            break; // Buffer full
        }

        if (phoneme == "PAUSE") {
            // Silence for pauses
            for (uint32_t i = 0; i < samplesPerPhoneme / 2 && currentSample < maxSamples; i++, currentSample++) {
                samples[currentSample] = 0;
            }
        } else {
            // Generate phoneme audio
            auto it = m_phonemeDatabase.find(phoneme);
            if (it != m_phonemeDatabase.end()) {
                const std::vector<float>& formants = it->second;
                
                for (uint32_t i = 0; i < samplesPerPhoneme && currentSample < maxSamples; i++, currentSample++) {
                    float sample = 0.0f;
                    float t = static_cast<float>(i) / sampleRate;
                    
                    // Generate formants (simplified synthesis)
                    for (size_t f = 0; f < formants.size(); f++) {
                        float frequency = m_currentPitch * (f + 1) * formants[f]; // Scale by Talkback pitch
                        sample += 0.25f * sin(2.0f * M_PI * frequency * t) / (f + 1);
                    }
                    
                    // Apply envelope (attack, sustain, decay)
                    float envelope = 1.0f;
                    if (i < samplesPerPhoneme / 8) {
                        envelope = static_cast<float>(i) / (samplesPerPhoneme / 8); // Attack
                    } else if (i > 7 * samplesPerPhoneme / 8) {
                        envelope = static_cast<float>(samplesPerPhoneme - i) / (samplesPerPhoneme / 8); // Decay
                    }
                    
                    sample *= envelope;
                    sample *= 0.3f; // Overall volume scaling
                    
                    // Convert to 16-bit
                    samples[currentSample] = static_cast<int16_t>(sample * 16384);
                }
            } else {
                // Unknown phoneme - generate silence
                for (uint32_t i = 0; i < samplesPerPhoneme && currentSample < maxSamples; i++, currentSample++) {
                    samples[currentSample] = 0;
                }
            }
        }
    }

    actualSize = currentSample * sizeof(int16_t);
    return OS_OK;
}

void TalkbackVoiceService::voiceProcessingTask(void* parameter) {
    TalkbackVoiceService* service = static_cast<TalkbackVoiceService*>(parameter);
    
    ESP_LOGI(TAG, "Voice processing task started");
    
    while (service->m_taskRunning) {
        service->processMessageQueue();
        vTaskDelay(pdMS_TO_TICKS(10)); // 10ms delay
    }
    
    ESP_LOGI(TAG, "Voice processing task ended");
    vTaskDelete(nullptr);
}

void TalkbackVoiceService::voiceCommandTask(void* parameter) {
    TalkbackVoiceService* service = static_cast<TalkbackVoiceService*>(parameter);
    
    ESP_LOGI(TAG, "Voice command task started");
    
    while (service->m_taskRunning) {
        if (service->m_voiceCommandsEnabled && service->m_audioService && 
            service->m_audioService->isRecording()) {
            
            // Read audio data for command recognition
            size_t bytesRead = 0;
            os_error_t result = service->m_audioService->readAudioData(
                service->m_commandBuffer, service->m_bufferSize, &bytesRead);
            
            if (result == OS_OK && bytesRead > 0) {
                VoiceCommand command = service->processVoiceCommand(
                    service->m_commandBuffer, bytesRead);
                
                if (command != VoiceCommand::NONE) {
                    // Queue command for execution
                    xQueueSend(service->m_commandQueue, &command, 0);
                }
            }
        }
        
        // Process queued commands
        VoiceCommand command;
        if (xQueueReceive(service->m_commandQueue, &command, pdMS_TO_TICKS(100)) == pdTRUE) {
            service->executeCommand(command);
        }
    }
    
    ESP_LOGI(TAG, "Voice command task ended");
    vTaskDelete(nullptr);
}

void TalkbackVoiceService::processMessageQueue() {
    if (!m_messageQueue || !m_speechMutex) {
        return;
    }

    VoiceMessage message;
    if (xQueueReceive(m_messageQueue, &message, 0) == pdTRUE) {
        if (xSemaphoreTake(m_speechMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (!m_speaking || message.interrupt) {
                m_speaking = true;
                m_speechStartTime = millis();
                
                ESP_LOGD(TAG, "Speaking: %s", message.text.c_str());
                
                // Synthesize and play audio
                size_t actualSize = 0;
                os_error_t result = synthesizeText(message.text, m_synthesisBuffer, 
                                                 m_bufferSize, actualSize);
                
                if (result == OS_OK && actualSize > 0 && m_audioService) {
                    result = m_audioService->playBuffer(m_synthesisBuffer, actualSize);
                    if (result == OS_OK) {
                        m_messagesSpoken++;
                    }
                }
                
                // Execute callback if provided
                if (message.callback) {
                    message.callback();
                }
                
                // Wait for speech to complete (simplified)
                vTaskDelay(pdMS_TO_TICKS(message.text.length() * 100)); // Rough timing
                
                m_speaking = false;
            }
            
            xSemaphoreGive(m_speechMutex);
        }
    }
}

void TalkbackVoiceService::updateFocusTracking() {
    // Update focus tracking based on LVGL focus state
    // This would be called from LVGL event handlers
}

void TalkbackVoiceService::scanFocusableObjects(lv_obj_t* parent) {
    if (!parent) return;

    m_focusState.focusable_objects.clear();
    
    // Recursively scan for focusable objects
    uint32_t childCount = lv_obj_get_child_cnt(parent);
    for (uint32_t i = 0; i < childCount; i++) {
        lv_obj_t* child = lv_obj_get_child(parent, i);
        if (child) {
            // Check if object is focusable
            if (lv_obj_has_flag(child, LV_OBJ_FLAG_CLICKABLE) ||
                lv_obj_check_type(child, &lv_btn_class) ||
                lv_obj_check_type(child, &lv_dropdown_class) ||
                lv_obj_check_type(child, &lv_slider_class) ||
                lv_obj_check_type(child, &lv_switch_class)) {
                
                m_focusState.focusable_objects.push_back(child);
            }
            
            // Recursively scan child objects
            scanFocusableObjects(child);
        }
    }
}

std::string TalkbackVoiceService::getObjectDescription(lv_obj_t* obj) {
    if (!obj) return "";

    std::string description;
    std::string typeName = getObjectTypeName(obj);
    std::string content = getObjectContent(obj);

    description = typeName;
    if (!content.empty()) {
        description += ": " + content;
    }

    return description;
}

std::string TalkbackVoiceService::getObjectTypeName(lv_obj_t* obj) {
    if (!obj) return "unknown";

    if (lv_obj_check_type(obj, &lv_btn_class)) return "button";
    if (lv_obj_check_type(obj, &lv_label_class)) return "label";
    if (lv_obj_check_type(obj, &lv_dropdown_class)) return "dropdown";
    if (lv_obj_check_type(obj, &lv_slider_class)) return "slider";
    if (lv_obj_check_type(obj, &lv_switch_class)) return "switch";
    if (lv_obj_check_type(obj, &lv_checkbox_class)) return "checkbox";
    if (lv_obj_check_type(obj, &lv_textarea_class)) return "text area";
    if (lv_obj_check_type(obj, &lv_list_class)) return "list";
    if (lv_obj_check_type(obj, &lv_img_class)) return "image";
    
    return "element";
}

std::string TalkbackVoiceService::getObjectContent(lv_obj_t* obj) {
    if (!obj) return "";

    // Try to get text content
    const char* text = lv_label_get_text(obj);
    if (text && strlen(text) > 0) {
        return std::string(text);
    }

    // For sliders, get value
    if (lv_obj_check_type(obj, &lv_slider_class)) {
        int32_t value = lv_slider_get_value(obj);
        return "value " + std::to_string(value);
    }

    // For switches, get state
    if (lv_obj_check_type(obj, &lv_switch_class)) {
        return lv_obj_has_state(obj, LV_STATE_CHECKED) ? "on" : "off";
    }

    return "";
}

std::string TalkbackVoiceService::addTalkbackPersonality(const std::string& text) {
    // Add Talkback characteristic phrasing
    std::string result = text;

    // Don't modify already styled text
    if (result.find("I'm afraid") != std::string::npos ||
        result.find("I'm sorry") != std::string::npos) {
        return result;
    }

    // Add occasional Talkback phrases (robot-like voice assistance)
    uint32_t random = esp_random() % 100;
    
    if (random < 3) { // 3% chance
        result = "I'm sorry. " + result;
    } else if (random < 8) { // 5% chance
        result = "I understand. " + result;
    }

    return result;
}

std::string TalkbackVoiceService::generateStatusPhrase() {
    if (m_talkbackStatusPhrases.empty()) {
        return "All systems operational.";
    }

    uint32_t index = esp_random() % m_talkbackStatusPhrases.size();
    return m_talkbackStatusPhrases[index];
}

VoiceCommand TalkbackVoiceService::parseSimpleCommands(const uint8_t* audioBuffer, size_t bufferSize) {
    // This is a placeholder for voice command recognition
    // In a full implementation, this would use proper speech recognition
    // For now, we'll simulate command recognition based on audio characteristics
    
    float energy = calculateAudioEnergy(audioBuffer, bufferSize);
    
    // Simple pattern matching based on energy and duration
    if (energy > 0.1f) {
        // Simulate different commands based on audio characteristics
        uint32_t hash = 0;
        for (size_t i = 0; i < std::min(bufferSize, (size_t)100); i += 10) {
            hash += audioBuffer[i];
        }
        
        // Map hash to commands (very crude simulation)
        VoiceCommand commands[] = {
            VoiceCommand::READ_SCREEN,
            VoiceCommand::NEXT_ITEM,
            VoiceCommand::PREVIOUS_ITEM,
            VoiceCommand::SELECT_ITEM,
            VoiceCommand::HELP,
            VoiceCommand::ANNOUNCE_TIME
        };
        
        return commands[hash % (sizeof(commands) / sizeof(commands[0]))];
    }
    
    return VoiceCommand::NONE;
}

float TalkbackVoiceService::calculateAudioEnergy(const uint8_t* audioBuffer, size_t bufferSize) {
    if (!audioBuffer || bufferSize == 0) return 0.0f;

    const int16_t* samples = reinterpret_cast<const int16_t*>(audioBuffer);
    size_t sampleCount = bufferSize / sizeof(int16_t);
    
    float energy = 0.0f;
    for (size_t i = 0; i < sampleCount; i++) {
        float sample = static_cast<float>(samples[i]) / 32768.0f;
        energy += sample * sample;
    }
    
    return energy / sampleCount;
}

os_error_t TalkbackVoiceService::registerLVGLHandlers() {
    ESP_LOGI(TAG, "Registering LVGL event handlers");
    
    // Note: In a full implementation, these handlers would be registered
    // on specific objects or globally through LVGL's event system
    
    return OS_OK;
}

void TalkbackVoiceService::lvglFocusEventHandler(lv_event_t* event) {
    if (!s_instance || !s_instance->m_settings.announceUI) return;

    lv_obj_t* obj = lv_event_get_target(event);
    if (obj) {
        s_instance->announceFocus(obj);
    }
}

void TalkbackVoiceService::lvglValueChangeHandler(lv_event_t* event) {
    if (!s_instance || !s_instance->m_settings.announceActions) return;

    lv_obj_t* obj = lv_event_get_target(event);
    if (obj) {
        std::string description = s_instance->getObjectDescription(obj);
        s_instance->speak("Changed: " + description, VoicePriority::USER_ACTION);
    }
}

void TalkbackVoiceService::lvglClickHandler(lv_event_t* event) {
    if (!s_instance || !s_instance->m_settings.announceActions) return;

    lv_obj_t* obj = lv_event_get_target(event);
    if (obj) {
        std::string description = s_instance->getObjectDescription(obj);
        s_instance->speak("Activated: " + description, VoicePriority::USER_ACTION);
    }
}

os_error_t TalkbackVoiceService::setVoiceSpeed(VoiceSpeed speed) {
    m_settings.speed = speed;
    m_currentSpeed = static_cast<float>(speed);
    return OS_OK;
}

os_error_t TalkbackVoiceService::setVoicePitch(VoicePitch pitch) {
    m_settings.pitch = pitch;
    m_currentPitch = static_cast<float>(pitch);
    return OS_OK;
}

os_error_t TalkbackVoiceService::setVoiceVolume(uint8_t volume) {
    if (volume > 100) volume = 100;
    
    m_settings.volume = volume;
    m_currentVolume = volume;
    
    if (m_audioService) {
        m_audioService->setVolume(volume);
    }
    
    return OS_OK;
}

void TalkbackVoiceService::printVoiceStats() const {
    ESP_LOGI(TAG, "Talkback Voice Service Statistics:");
    ESP_LOGI(TAG, "  Initialized: %s", m_initialized ? "YES" : "NO");
    ESP_LOGI(TAG, "  Currently Speaking: %s", m_speaking ? "YES" : "NO");
    ESP_LOGI(TAG, "  Navigation Mode: %s", m_navigationMode ? "YES" : "NO");
    ESP_LOGI(TAG, "  Voice Commands: %s", m_voiceCommandsEnabled ? "ENABLED" : "DISABLED");
    ESP_LOGI(TAG, "  Voice Speed: %.0f WPM", m_currentSpeed);
    ESP_LOGI(TAG, "  Voice Pitch: %.0f Hz", m_currentPitch);
    ESP_LOGI(TAG, "  Voice Volume: %d%%", m_currentVolume);
    ESP_LOGI(TAG, "  Messages Spoken: %u", m_messagesSpoken);
    ESP_LOGI(TAG, "  Commands Processed: %u", m_commandsProcessed);
    ESP_LOGI(TAG, "  Navigation Actions: %u", m_navigationActions);
    ESP_LOGI(TAG, "  Screen Readings: %u", m_screenReadings);
    ESP_LOGI(TAG, "  Focusable Objects: %d", m_focusState.focusable_objects.size());
}