#ifndef ADVANCED_APP_H
#define ADVANCED_APP_H

#include "../../src/apps/base_app.h"
#include <memory>
#include <vector>
#include <string>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

/**
 * @file advanced_app.h
 * @brief Advanced App Template for M5Stack Tab5 OS
 * 
 * This template demonstrates advanced features including:
 * - Background task management
 * - Service integration
 * - Event handling
 * - Data persistence
 * - Network operations
 * - Advanced UI patterns
 */

// Forward declarations
class DataService;
class NetworkService;

/**
 * @brief Message structure for inter-task communication
 */
struct AppMessage {
    uint32_t type;
    void* data;
    size_t size;
    uint32_t timestamp;
};

/**
 * @brief App configuration structure
 */
struct AppConfig {
    bool enableNetworking = true;
    bool enableLogging = true;
    uint32_t updateInterval = 1000;
    std::string serverUrl = "https://api.example.com";
    uint32_t maxRetries = 3;
};

/**
 * @brief Advanced application demonstrating complex patterns
 */
class AdvancedApp : public BaseApp {
public:
    AdvancedApp();
    ~AdvancedApp() override;

    // BaseApp interface
    os_error_t initialize() override;
    os_error_t update(uint32_t deltaTime) override;
    os_error_t shutdown() override;
    os_error_t createUI(lv_obj_t* parent) override;
    os_error_t destroyUI() override;
    os_error_t handleEvent(uint32_t eventType, void* eventData, size_t dataSize) override;

    // Lifecycle methods
    os_error_t start() override;
    os_error_t stop() override;
    os_error_t pause() override;
    os_error_t resume() override;

private:
    // Initialization helpers
    os_error_t initializeServices();
    os_error_t initializeBackgroundTasks();
    os_error_t loadConfiguration();
    
    // Shutdown helpers
    void shutdownServices();
    void shutdownBackgroundTasks();
    void saveConfiguration();
    
    // UI creation methods
    void createToolbar();
    void createMainContent();
    void createStatusBar();
    void createSettingsDialog();
    
    // UI update methods
    void updateMainContent();
    void updateStatusBar();
    void refreshDataDisplay();
    
    // Event handlers
    void handleNetworkEvent(const EventData& event);
    void handleUserInput(const EventData& event);
    void handleDataUpdate(const EventData& event);
    void handleSystemEvent(const EventData& event);
    
    // Background task management
    static void backgroundTask(void* parameter);
    static void networkTask(void* parameter);
    static void dataTask(void* parameter);
    
    void processBackgroundWork();
    void processNetworkWork();
    void processDataWork();
    
    // Message handling
    void sendMessage(uint32_t type, void* data, size_t size);
    void processMessages();
    
    // Data management
    os_error_t loadAppData();
    os_error_t saveAppData();
    void resetAppData();
    
    // Network operations
    os_error_t connectToServer();
    void disconnectFromServer();
    os_error_t sendDataToServer(const std::string& data);
    void handleServerResponse(const std::string& response);
    
    // Utility methods
    std::string getAppDataPath() const;
    std::string getConfigPath() const;
    void logAppEvent(const std::string& event, const std::string& details = "");
    
    // UI event callbacks
    static void toolbarButtonCallback(lv_event_t* e);
    static void contentButtonCallback(lv_event_t* e);
    static void settingsButtonCallback(lv_event_t* e);
    static void dataListCallback(lv_event_t* e);
    static void refreshButtonCallback(lv_event_t* e);
    
    // Services
    std::unique_ptr<DataService> m_dataService;
    std::unique_ptr<NetworkService> m_networkService;
    
    // Background tasks
    TaskHandle_t m_backgroundTaskHandle = nullptr;
    TaskHandle_t m_networkTaskHandle = nullptr;
    TaskHandle_t m_dataTaskHandle = nullptr;
    QueueHandle_t m_messageQueue = nullptr;
    bool m_tasksRunning = false;
    
    // UI elements
    lv_obj_t* m_toolbar = nullptr;
    lv_obj_t* m_titleLabel = nullptr;
    lv_obj_t* m_settingsButton = nullptr;
    lv_obj_t* m_refreshButton = nullptr;
    
    lv_obj_t* m_contentArea = nullptr;
    lv_obj_t* m_dataList = nullptr;
    lv_obj_t* m_progressBar = nullptr;
    lv_obj_t* m_statusText = nullptr;
    
    lv_obj_t* m_statusBar = nullptr;
    lv_obj_t* m_connectionStatus = nullptr;
    lv_obj_t* m_memoryStatus = nullptr;
    lv_obj_t* m_taskStatus = nullptr;
    
    lv_obj_t* m_settingsDialog = nullptr;
    
    // App state
    struct {
        bool isConnected = false;
        bool isDataLoaded = false;
        bool isProcessing = false;
        uint32_t dataCount = 0;
        uint32_t errorCount = 0;
        uint32_t lastUpdateTime = 0;
        std::vector<std::string> dataItems;
    } m_appState;
    
    // Configuration
    AppConfig m_config;
    
    // Statistics
    struct {
        uint32_t totalOperations = 0;
        uint32_t successfulOperations = 0;
        uint32_t failedOperations = 0;
        uint32_t networkRequests = 0;
        uint32_t dataUpdates = 0;
        size_t peakMemoryUsage = 0;
    } m_statistics;
    
    // Constants
    static constexpr size_t MAX_MESSAGE_QUEUE_SIZE = 32;
    static constexpr size_t BACKGROUND_TASK_STACK_SIZE = 8192;
    static constexpr size_t NETWORK_TASK_STACK_SIZE = 8192;
    static constexpr size_t DATA_TASK_STACK_SIZE = 4096;
    static constexpr UBaseType_t BACKGROUND_TASK_PRIORITY = 3;
    static constexpr UBaseType_t NETWORK_TASK_PRIORITY = 4;
    static constexpr UBaseType_t DATA_TASK_PRIORITY = 2;
    static constexpr uint32_t TASK_DELAY_MS = 100;
    static constexpr uint32_t NETWORK_TIMEOUT_MS = 10000;
    static constexpr size_t MAX_DATA_ITEMS = 100;
    static constexpr size_t MAX_APP_MEMORY = 512 * 1024; // 512KB
};

/**
 * @brief Simple data service for app data management
 */
class DataService {
public:
    DataService() = default;
    ~DataService() = default;
    
    os_error_t initialize();
    os_error_t shutdown();
    
    os_error_t loadData(std::vector<std::string>& data);
    os_error_t saveData(const std::vector<std::string>& data);
    os_error_t addItem(const std::string& item);
    os_error_t removeItem(size_t index);
    void clearData();
    
    size_t getItemCount() const { return m_itemCount; }
    
private:
    size_t m_itemCount = 0;
    bool m_initialized = false;
};

/**
 * @brief Simple network service for HTTP operations
 */
class NetworkService {
public:
    NetworkService() = default;
    ~NetworkService() = default;
    
    os_error_t initialize();
    os_error_t shutdown();
    
    bool isConnected() const { return m_connected; }
    os_error_t connect(const std::string& url);
    void disconnect();
    
    os_error_t sendRequest(const std::string& data, std::string& response);
    os_error_t sendRequestAsync(const std::string& data);
    
private:
    bool m_connected = false;
    bool m_initialized = false;
    std::string m_serverUrl;
};

/**
 * @brief Factory function for creating advanced app instances
 */
extern "C" std::unique_ptr<BaseApp> createAdvancedApp();

#endif // ADVANCED_APP_H