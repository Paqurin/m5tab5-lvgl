#ifndef MICROPYTHON_LAUNCHER_APP_H
#define MICROPYTHON_LAUNCHER_APP_H

#include "base_app.h"
#include <vector>
#include <string>
#include <memory>

/**
 * @file micropython_launcher_app.h
 * @brief MicroPython Script Launcher for M5Stack Tab5 OS
 * 
 * Provides a file browser and script execution environment for MicroPython scripts.
 * Allows users to browse, select, and execute .py files with console output display.
 */

struct FileItem {
    std::string name;
    std::string path;
    bool isDirectory;
    size_t size;
};

class MicroPythonLauncherApp : public BaseApp {
public:
    /**
     * @brief Constructor
     */
    MicroPythonLauncherApp();
    
    /**
     * @brief Destructor
     */
    ~MicroPythonLauncherApp() override;

    // BaseApp interface implementation
    os_error_t initialize() override;
    os_error_t update(uint32_t deltaTime) override;
    os_error_t shutdown() override;
    os_error_t createUI(lv_obj_t* parent) override;
    os_error_t destroyUI() override;
    os_error_t handleEvent(uint32_t eventType, void* eventData, size_t dataSize) override;

private:
    /**
     * @brief Create the main user interface
     */
    void createMainUI();
    
    /**
     * @brief Create file browser interface
     */
    void createFileBrowser();
    
    /**
     * @brief Create console output display
     */
    void createConsoleOutput();
    
    /**
     * @brief Create toolbar with navigation and controls
     */
    void createToolbar();
    
    /**
     * @brief Update file list display
     */
    void updateFileList();
    
    /**
     * @brief Scan directory for Python files
     * @param path Directory path to scan
     * @return OS_OK on success, error code on failure
     */
    os_error_t scanDirectory(const std::string& path);
    
    /**
     * @brief Navigate to directory
     * @param path Directory path
     */
    void navigateToDirectory(const std::string& path);
    
    /**
     * @brief Navigate up one directory level
     */
    void navigateUp();
    
    /**
     * @brief Execute selected Python script
     * @param scriptPath Path to Python script
     * @return OS_OK on success, error code on failure
     */
    os_error_t executeScript(const std::string& scriptPath);
    
    /**
     * @brief Stop currently running script
     */
    void stopScript();
    
    /**
     * @brief Clear console output
     */
    void clearConsole();
    
    /**
     * @brief Add text to console output
     * @param text Text to add
     * @param isError Whether this is error text
     */
    void appendToConsole(const std::string& text, bool isError = false);
    
    /**
     * @brief Update script execution status
     * @param status Status message
     */
    void updateExecutionStatus(const std::string& status);
    
    /**
     * @brief Check if file is a Python script
     * @param filename File name to check
     * @return true if it's a .py file
     */
    bool isPythonFile(const std::string& filename) const;
    
    /**
     * @brief Get file size as formatted string
     * @param size File size in bytes
     * @return Formatted size string
     */
    std::string formatFileSize(size_t size) const;
    
    // UI event callbacks (must be static)
    static void fileListCallback(lv_event_t* e);
    static void backButtonCallback(lv_event_t* e);
    static void refreshButtonCallback(lv_event_t* e);
    static void executeButtonCallback(lv_event_t* e);
    static void stopButtonCallback(lv_event_t* e);
    static void clearButtonCallback(lv_event_t* e);
    
    // File system and execution
    std::string m_currentPath;
    std::vector<FileItem> m_fileList;
    std::string m_selectedFile;
    bool m_scriptRunning;
    
    // UI elements - Main container
    lv_obj_t* m_mainContainer = nullptr;
    
    // UI elements - Toolbar
    lv_obj_t* m_toolbar = nullptr;
    lv_obj_t* m_backButton = nullptr;
    lv_obj_t* m_refreshButton = nullptr;
    lv_obj_t* m_pathLabel = nullptr;
    
    // UI elements - File browser
    lv_obj_t* m_fileBrowserContainer = nullptr;
    lv_obj_t* m_fileListWidget = nullptr;
    
    // UI elements - Console
    lv_obj_t* m_consoleContainer = nullptr;
    lv_obj_t* m_consoleTextArea = nullptr;
    lv_obj_t* m_consoleLabel = nullptr;
    lv_obj_t* m_statusLabel = nullptr;
    
    // UI elements - Control buttons
    lv_obj_t* m_controlContainer = nullptr;
    lv_obj_t* m_executeButton = nullptr;
    lv_obj_t* m_stopButton = nullptr;
    lv_obj_t* m_clearButton = nullptr;
    
    // Execution state
    uint32_t m_lastScanTime = 0;
    static constexpr uint32_t SCAN_INTERVAL_MS = 1000;
    static constexpr size_t MAX_CONSOLE_LINES = 1000;
    static constexpr size_t MAX_FILES_DISPLAYED = 100;
    
    // Color scheme
    static constexpr lv_color_t COLOR_PRIMARY = LV_COLOR_MAKE(0x34, 0x98, 0xDB);
    static constexpr lv_color_t COLOR_SUCCESS = LV_COLOR_MAKE(0x2E, 0xCC, 0x71);
    static constexpr lv_color_t COLOR_ERROR = LV_COLOR_MAKE(0xE7, 0x4C, 0x3C);
    static constexpr lv_color_t COLOR_WARNING = LV_COLOR_MAKE(0xF3, 0x9C, 0x12);
    static constexpr lv_color_t COLOR_BACKGROUND = LV_COLOR_MAKE(0x1E, 0x1E, 0x1E);
    static constexpr lv_color_t COLOR_SURFACE = LV_COLOR_MAKE(0x2C, 0x2C, 0x2C);
};

/**
 * @brief Factory function for creating app instances
 * Required for dynamic loading by the app manager
 * @return Unique pointer to app instance
 */
extern "C" std::unique_ptr<BaseApp> createMicroPythonLauncherApp();

#endif // MICROPYTHON_LAUNCHER_APP_H