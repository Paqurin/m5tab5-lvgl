#ifndef TALKBACK_VOICE_SERVICE_H
#define TALKBACK_VOICE_SERVICE_H

#include "../system/os_config.h"
#include "../hal/hardware_config.h"
#include "audio_service.h"
#include <lvgl.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <vector>
#include <string>
#include <functional>
#include <map>

/**
 * @file talkback_voice_service.h
 * @brief Talkback Style Voice Feedback System for Blind Accessibility
 * 
 * Provides comprehensive voice synthesis, screen reading, and voice commands
 * with the distinctive Talkback personality for enhanced accessibility.
 */

enum class VoiceCommand {
    NONE,
    OPEN_APPS,
    GO_BACK,
    READ_SCREEN,
    NEXT_ITEM,
    PREVIOUS_ITEM,
    SELECT_ITEM,
    CANCEL,
    VOLUME_UP,
    VOLUME_DOWN,
    MUTE_TOGGLE,
    REPEAT_LAST,
    DESCRIBE_LAYOUT,
    ANNOUNCE_TIME,
    ANNOUNCE_BATTERY,
    ANNOUNCE_CONNECTIVITY,
    HELP,
    SLEEP_MODE,
    WAKE_UP
};

enum class VoicePriority {
    SYSTEM_CRITICAL = 0,    // System errors, warnings
    NAVIGATION = 1,         // UI navigation announcements
    USER_ACTION = 2,        // User action confirmations
    SCREEN_READING = 3,     // Screen content reading
    BACKGROUND_INFO = 4     // Status updates, time announcements
};

enum class VoiceSpeed {
    VERY_SLOW = 60,    // 60 WPM
    SLOW = 80,         // 80 WPM
    NORMAL = 120,      // 120 WPM (Talkback default)
    FAST = 160,        // 160 WPM
    VERY_FAST = 200    // 200 WPM
};

enum class VoicePitch {
    VERY_LOW = 80,     // 80 Hz (Talkback characteristic)
    LOW_PITCH = 100,   // 100 Hz
    NORMAL = 120,      // 120 Hz
    HIGH_PITCH = 150,  // 150 Hz
    VERY_HIGH = 180    // 180 Hz
};

struct VoiceMessage {
    std::string text;
    VoicePriority priority;
    bool interrupt = false;
    bool repeat = false;
    uint32_t delay_ms = 0;
    std::function<void()> callback = nullptr;
};

struct VoiceSettings {
    VoiceSpeed speed = VoiceSpeed::NORMAL;
    VoicePitch pitch = VoicePitch::VERY_LOW;
    uint8_t volume = 75;           // 0-100%
    bool enabled = true;
    bool announceUI = true;        // Announce UI elements
    bool announceActions = true;   // Announce user actions
    bool announceStatus = true;    // Announce status changes
    bool announceTime = false;     // Periodic time announcements
    bool verboseMode = false;      // Detailed descriptions
    uint32_t timeInterval = 300000; // 5 minutes for time announcements
};

struct ScreenElement {
    lv_obj_t* obj;
    std::string name;
    std::string description;
    std::string type;
    bool focusable;
    bool clickable;
    lv_area_t area;
};

struct FocusState {
    lv_obj_t* current_obj = nullptr;
    std::vector<lv_obj_t*> focusable_objects;
    int current_index = -1;
    bool navigation_mode = false;
};

/**
 * @brief Talkback Voice Service for Accessibility
 * 
 * Provides text-to-speech synthesis, screen reading capabilities,
 * voice command recognition, and navigation assistance with the
 * distinctive Talkback personality characteristics.
 */
class TalkbackVoiceService {
public:
    TalkbackVoiceService() = default;
    ~TalkbackVoiceService();

    /**
     * @brief Initialize Talkback voice service
     * @param audioService Pointer to initialized audio service
     * @return OS_OK on success, error code on failure
     */
    os_error_t initialize(AudioService* audioService);

    /**
     * @brief Shutdown voice service
     * @return OS_OK on success, error code on failure
     */
    os_error_t shutdown();

    /**
     * @brief Update voice service (process queue, handle commands)
     * @param deltaTime Time since last update in milliseconds
     * @return OS_OK on success, error code on failure
     */
    os_error_t update(uint32_t deltaTime);

    /**
     * @brief Configure voice settings
     * @param settings Voice configuration
     * @return OS_OK on success, error code on failure
     */
    os_error_t configure(const VoiceSettings& settings);

    // Text-to-Speech Functions
    /**
     * @brief Speak text with Talkback characteristics
     * @param text Text to speak
     * @param priority Message priority
     * @param interrupt Whether to interrupt current speech
     * @return OS_OK on success, error code on failure
     */
    os_error_t speak(const std::string& text, VoicePriority priority = VoicePriority::USER_ACTION, bool interrupt = false);

    /**
     * @brief Queue voice message for speaking
     * @param message Voice message structure
     * @return OS_OK on success, error code on failure
     */
    os_error_t queueMessage(const VoiceMessage& message);

    /**
     * @brief Stop current speech and clear queue
     * @return OS_OK on success, error code on failure
     */
    os_error_t stopSpeaking();

    /**
     * @brief Check if currently speaking
     * @return true if speaking
     */
    bool isSpeaking() const { return m_speaking; }

    // Screen Reading Functions
    /**
     * @brief Read entire screen content
     * @param detailed Whether to provide detailed descriptions
     * @return OS_OK on success, error code on failure
     */
    os_error_t readScreen(bool detailed = false);

    /**
     * @brief Read specific UI object
     * @param obj LVGL object to read
     * @return OS_OK on success, error code on failure
     */
    os_error_t readObject(lv_obj_t* obj);

    /**
     * @brief Announce UI element focus
     * @param obj Focused LVGL object
     * @return OS_OK on success, error code on failure
     */
    os_error_t announceFocus(lv_obj_t* obj);

    /**
     * @brief Describe layout structure
     * @return OS_OK on success, error code on failure
     */
    os_error_t describeLayout();

    // Navigation Functions
    /**
     * @brief Enable voice navigation mode
     * @param enable true to enable navigation
     * @return OS_OK on success, error code on failure
     */
    os_error_t enableNavigation(bool enable);

    /**
     * @brief Navigate to next focusable element
     * @return OS_OK on success, error code on failure
     */
    os_error_t navigateNext();

    /**
     * @brief Navigate to previous focusable element
     * @return OS_OK on success, error code on failure
     */
    os_error_t navigatePrevious();

    /**
     * @brief Activate current focused element
     * @return OS_OK on success, error code on failure
     */
    os_error_t activateFocused();

    // Voice Command Functions
    /**
     * @brief Process voice command input
     * @param audioBuffer Audio buffer containing voice command
     * @param bufferSize Buffer size in bytes
     * @return Recognized voice command
     */
    VoiceCommand processVoiceCommand(const uint8_t* audioBuffer, size_t bufferSize);

    /**
     * @brief Execute voice command
     * @param command Voice command to execute
     * @return OS_OK on success, error code on failure
     */
    os_error_t executeCommand(VoiceCommand command);

    /**
     * @brief Enable/disable voice command recognition
     * @param enable true to enable voice commands
     * @return OS_OK on success, error code on failure
     */
    os_error_t enableVoiceCommands(bool enable);

    // Status and Information Functions
    /**
     * @brief Announce system status
     * @return OS_OK on success, error code on failure
     */
    os_error_t announceStatus();

    /**
     * @brief Announce current time
     * @return OS_OK on success, error code on failure
     */
    os_error_t announceTime();

    /**
     * @brief Announce battery level
     * @return OS_OK on success, error code on failure
     */
    os_error_t announceBattery();

    /**
     * @brief Announce connectivity status
     * @return OS_OK on success, error code on failure
     */
    os_error_t announceConnectivity();

    /**
     * @brief Provide help information
     * @return OS_OK on success, error code on failure
     */
    os_error_t provideHelp();

    // LVGL Integration Functions
    /**
     * @brief Register LVGL event handlers for voice feedback
     * @return OS_OK on success, error code on failure
     */
    os_error_t registerLVGLHandlers();

    /**
     * @brief Handle LVGL focus events
     * @param event LVGL event structure
     */
    static void lvglFocusEventHandler(lv_event_t* event);

    /**
     * @brief Handle LVGL value change events
     * @param event LVGL event structure
     */
    static void lvglValueChangeHandler(lv_event_t* event);

    /**
     * @brief Handle LVGL click events
     * @param event LVGL event structure
     */
    static void lvglClickHandler(lv_event_t* event);

    // Talkback Personality Functions
    /**
     * @brief Generate Talkback style greeting
     * @param userName Optional user name
     * @return OS_OK on success, error code on failure
     */
    os_error_t greetUser(const std::string& userName = "User");

    /**
     * @brief Generate Talkback style error message
     * @param errorDescription Error description
     * @return OS_OK on success, error code on failure
     */
    os_error_t announceError(const std::string& errorDescription);

    /**
     * @brief Generate Talkback style status report
     * @return OS_OK on success, error code on failure
     */
    os_error_t statusReport();

    // Settings and Configuration
    /**
     * @brief Set voice speed
     * @param speed Voice speed setting
     * @return OS_OK on success, error code on failure
     */
    os_error_t setVoiceSpeed(VoiceSpeed speed);

    /**
     * @brief Set voice pitch
     * @param pitch Voice pitch setting
     * @return OS_OK on success, error code on failure
     */
    os_error_t setVoicePitch(VoicePitch pitch);

    /**
     * @brief Set voice volume
     * @param volume Volume level (0-100)
     * @return OS_OK on success, error code on failure
     */
    os_error_t setVoiceVolume(uint8_t volume);

    /**
     * @brief Get current voice settings
     * @return Current voice settings
     */
    VoiceSettings getSettings() const { return m_settings; }

    /**
     * @brief Check if voice service is initialized
     * @return true if initialized
     */
    bool isInitialized() const { return m_initialized; }

    /**
     * @brief Get voice service statistics
     */
    void printVoiceStats() const;

    // Static instance for global access
    static TalkbackVoiceService* getInstance() { return s_instance; }

private:
    /**
     * @brief Initialize text-to-speech engine
     * @return OS_OK on success, error code on failure
     */
    os_error_t initializeTTS();

    /**
     * @brief Initialize voice recognition engine
     * @return OS_OK on success, error code on failure
     */
    os_error_t initializeVoiceRecognition();

    /**
     * @brief Initialize phoneme database for synthesis
     * @return OS_OK on success, error code on failure
     */
    os_error_t initializePhonemeDatabase();

    /**
     * @brief Generate audio samples for text
     * @param text Text to synthesize
     * @param audioBuffer Output audio buffer
     * @param bufferSize Maximum buffer size
     * @param actualSize Actual generated size
     * @return OS_OK on success, error code on failure
     */
    os_error_t synthesizeText(const std::string& text, uint8_t* audioBuffer, 
                             size_t bufferSize, size_t& actualSize);

    /**
     * @brief Apply Talkback voice characteristics
     * @param audioBuffer Audio buffer to process
     * @param bufferSize Buffer size in bytes
     * @return OS_OK on success, error code on failure
     */
    os_error_t applyTalkbackCharacteristics(uint8_t* audioBuffer, size_t bufferSize);

    /**
     * @brief Convert text to phonemes
     * @param text Input text
     * @param phonemes Output phoneme sequence
     * @return OS_OK on success, error code on failure
     */
    os_error_t textToPhonemes(const std::string& text, std::vector<std::string>& phonemes);

    /**
     * @brief Generate audio for phoneme sequence
     * @param phonemes Phoneme sequence
     * @param audioBuffer Output audio buffer
     * @param bufferSize Maximum buffer size
     * @param actualSize Actual generated size
     * @return OS_OK on success, error code on failure
     */
    os_error_t phonemesToAudio(const std::vector<std::string>& phonemes, 
                               uint8_t* audioBuffer, size_t bufferSize, size_t& actualSize);

    /**
     * @brief Voice processing task
     * @param parameter Task parameter
     */
    static void voiceProcessingTask(void* parameter);

    /**
     * @brief Voice command recognition task
     * @param parameter Task parameter
     */
    static void voiceCommandTask(void* parameter);

    /**
     * @brief Process voice message queue
     */
    void processMessageQueue();

    /**
     * @brief Update focus tracking
     */
    void updateFocusTracking();

    /**
     * @brief Scan for focusable objects
     * @param parent Parent object to scan
     */
    void scanFocusableObjects(lv_obj_t* parent);

    /**
     * @brief Get object description for voice
     * @param obj LVGL object
     * @return Description string
     */
    std::string getObjectDescription(lv_obj_t* obj);

    /**
     * @brief Get object type name
     * @param obj LVGL object
     * @return Type name string
     */
    std::string getObjectTypeName(lv_obj_t* obj);

    /**
     * @brief Get object value/text content
     * @param obj LVGL object
     * @return Content string
     */
    std::string getObjectContent(lv_obj_t* obj);

    /**
     * @brief Add Talkback personality to text
     * @param text Input text
     * @return Talkback styled text
     */
    std::string addTalkbackPersonality(const std::string& text);

    /**
     * @brief Generate Talkback status phrase
     * @return Status phrase
     */
    std::string generateStatusPhrase();

    /**
     * @brief Parse simple voice commands using keyword matching
     * @param audioBuffer Audio buffer
     * @param bufferSize Buffer size
     * @return Recognized command
     */
    VoiceCommand parseSimpleCommands(const uint8_t* audioBuffer, size_t bufferSize);

    /**
     * @brief Calculate audio energy for voice activity detection
     * @param audioBuffer Audio buffer
     * @param bufferSize Buffer size
     * @return Energy level
     */
    float calculateAudioEnergy(const uint8_t* audioBuffer, size_t bufferSize);

    // Service state
    bool m_initialized = false;
    bool m_speaking = false;
    bool m_navigationMode = false;
    bool m_voiceCommandsEnabled = true;
    AudioService* m_audioService = nullptr;

    // Voice settings
    VoiceSettings m_settings;

    // Focus tracking
    FocusState m_focusState;

    // Message queue and processing
    QueueHandle_t m_messageQueue = nullptr;
    QueueHandle_t m_commandQueue = nullptr;
    SemaphoreHandle_t m_speechMutex = nullptr;

    // Tasks
    TaskHandle_t m_voiceTaskHandle = nullptr;
    TaskHandle_t m_commandTaskHandle = nullptr;
    bool m_taskRunning = false;

    // Audio buffers
    uint8_t* m_synthesisBuffer = nullptr;
    uint8_t* m_commandBuffer = nullptr;
    size_t m_bufferSize = 0;

    // TTS state
    float m_currentPitch = 80.0f;      // Talkback low frequency
    float m_currentSpeed = 120.0f;     // 120 WPM
    uint8_t m_currentVolume = 75;

    // Timing
    uint32_t m_lastTimeAnnouncement = 0;
    uint32_t m_lastStatusUpdate = 0;
    uint32_t m_speechStartTime = 0;

    // Statistics
    uint32_t m_messagesSpoken = 0;
    uint32_t m_commandsProcessed = 0;
    uint32_t m_navigationActions = 0;
    uint32_t m_screenReadings = 0;

    // Phoneme database for synthesis
    std::map<std::string, std::vector<float>> m_phonemeDatabase;

    // Talkback phrases
    std::vector<std::string> m_talkbackGreetings;
    std::vector<std::string> m_talkbackStatusPhrases;
    std::vector<std::string> m_talkbackErrorPhrases;

    // Configuration constants
    static constexpr size_t VOICE_BUFFER_SIZE = 16384;     // 16KB audio buffer
    static constexpr size_t MESSAGE_QUEUE_SIZE = 32;       // 32 messages
    static constexpr size_t COMMAND_QUEUE_SIZE = 16;       // 16 commands
    static constexpr uint32_t VOICE_TASK_STACK_SIZE = 8192;
    static constexpr uint32_t COMMAND_TASK_STACK_SIZE = 4096;
    static constexpr UBaseType_t VOICE_TASK_PRIORITY = 5;
    static constexpr UBaseType_t COMMAND_TASK_PRIORITY = 6;
    static constexpr uint32_t SPEECH_TIMEOUT_MS = 30000;   // 30 seconds max speech
    static constexpr float VAD_ENERGY_THRESHOLD = 0.01f;   // Voice activity detection
    static constexpr const char* TAG = "TalkbackVoice";

    // Static instance
    static TalkbackVoiceService* s_instance;
};

#endif // TALKBACK_VOICE_SERVICE_H