#include "micropython_launcher_app.h"
#include "../system/os_manager.h"
#include <esp_log.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

static const char* TAG = "MicroPythonLauncher";

MicroPythonLauncherApp::MicroPythonLauncherApp() 
    : BaseApp("micropython_launcher", "MicroPython Launcher", "1.0.0") {
    setDescription("Execute MicroPython scripts with file browser and console output");
    setAuthor("M5Stack Tab5 OS");
    setPriority(AppPriority::APP_NORMAL);
    
    m_currentPath = "/sdcard/python";
    m_scriptRunning = false;
}

MicroPythonLauncherApp::~MicroPythonLauncherApp() {
    if (m_uiContainer) {
        destroyUI();
    }
}

os_error_t MicroPythonLauncherApp::initialize() {
    ESP_LOGI(TAG, "Initializing MicroPython Launcher App");
    
    setMemoryUsage(2048); // Estimate 2KB base memory usage
    
    // Create default Python scripts directory if it doesn't exist
    struct stat st;
    if (stat("/sdcard/python", &st) != 0) {
        mkdir("/sdcard/python", 0755);
        ESP_LOGI(TAG, "Created Python scripts directory");
    }
    
    // Scan initial directory
    scanDirectory(m_currentPath);
    
    ESP_LOGI(TAG, "MicroPython Launcher App initialized successfully");
    return OS_OK;
}

os_error_t MicroPythonLauncherApp::update(uint32_t deltaTime) {
    // Periodically refresh file list
    uint32_t currentTime = millis();
    if (currentTime - m_lastScanTime >= SCAN_INTERVAL_MS) {
        scanDirectory(m_currentPath);
        updateFileList();
        m_lastScanTime = currentTime;
    }
    
    return OS_OK;
}

os_error_t MicroPythonLauncherApp::shutdown() {
    ESP_LOGI(TAG, "Shutting down MicroPython Launcher App");
    
    if (m_scriptRunning) {
        stopScript();
    }
    
    return OS_OK;
}

os_error_t MicroPythonLauncherApp::createUI(lv_obj_t* parent) {
    if (!parent) {
        return OS_ERROR_INVALID_PARAM;
    }
    
    ESP_LOGI(TAG, "Creating MicroPython Launcher UI");
    
    // Create main container
    m_uiContainer = lv_obj_create(parent);
    lv_obj_set_size(m_uiContainer, LV_HOR_RES, LV_VER_RES - OS_STATUS_BAR_HEIGHT - OS_DOCK_HEIGHT);
    lv_obj_align(m_uiContainer, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(m_uiContainer, COLOR_BACKGROUND, 0);
    lv_obj_set_style_border_opa(m_uiContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(m_uiContainer, 5, 0);
    
    createMainUI();
    
    // Initial file list update
    updateFileList();
    
    ESP_LOGI(TAG, "MicroPython Launcher UI created successfully");
    return OS_OK;
}

os_error_t MicroPythonLauncherApp::destroyUI() {
    ESP_LOGI(TAG, "Destroying MicroPython Launcher UI");
    
    if (m_uiContainer) {
        lv_obj_del(m_uiContainer);
        m_uiContainer = nullptr;
        
        // Reset all UI pointers
        m_mainContainer = nullptr;
        m_toolbar = nullptr;
        m_backButton = nullptr;
        m_refreshButton = nullptr;
        m_pathLabel = nullptr;
        m_fileBrowserContainer = nullptr;
        m_fileListWidget = nullptr;
        m_consoleContainer = nullptr;
        m_consoleTextArea = nullptr;
        m_consoleLabel = nullptr;
        m_statusLabel = nullptr;
        m_controlContainer = nullptr;
        m_executeButton = nullptr;
        m_stopButton = nullptr;
        m_clearButton = nullptr;
    }
    
    return OS_OK;
}

os_error_t MicroPythonLauncherApp::handleEvent(uint32_t eventType, void* eventData, size_t dataSize) {
    // Handle app-specific events
    return OS_OK;
}

void MicroPythonLauncherApp::createMainUI() {
    // Create main container with vertical layout
    m_mainContainer = lv_obj_create(m_uiContainer);
    lv_obj_set_size(m_mainContainer, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(m_mainContainer, COLOR_SURFACE, 0);
    lv_obj_set_style_border_opa(m_mainContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(m_mainContainer, 5, 0);
    
    createToolbar();
    createFileBrowser();
    createConsoleOutput();
}

void MicroPythonLauncherApp::createToolbar() {
    // Create toolbar container
    m_toolbar = lv_obj_create(m_mainContainer);
    lv_obj_set_size(m_toolbar, lv_pct(100), 50);
    lv_obj_align(m_toolbar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(m_toolbar, COLOR_PRIMARY, 0);
    lv_obj_set_style_border_opa(m_toolbar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(m_toolbar, 5, 0);
    lv_obj_set_flex_flow(m_toolbar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(m_toolbar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Back button
    m_backButton = lv_btn_create(m_toolbar);
    lv_obj_set_size(m_backButton, 40, 40);
    lv_obj_add_event_cb(m_backButton, backButtonCallback, LV_EVENT_CLICKED, this);
    lv_obj_t* backLabel = lv_label_create(m_backButton);
    lv_label_set_text(backLabel, LV_SYMBOL_LEFT);
    lv_obj_center(backLabel);
    
    // Refresh button
    m_refreshButton = lv_btn_create(m_toolbar);
    lv_obj_set_size(m_refreshButton, 40, 40);
    lv_obj_add_event_cb(m_refreshButton, refreshButtonCallback, LV_EVENT_CLICKED, this);
    lv_obj_t* refreshLabel = lv_label_create(m_refreshButton);
    lv_label_set_text(refreshLabel, LV_SYMBOL_REFRESH);
    lv_obj_center(refreshLabel);
    
    // Path label
    m_pathLabel = lv_label_create(m_toolbar);
    lv_label_set_text(m_pathLabel, m_currentPath.c_str());
    lv_obj_set_style_text_color(m_pathLabel, lv_color_white(), 0);
    lv_obj_set_flex_grow(m_pathLabel, 1);
}

void MicroPythonLauncherApp::createFileBrowser() {
    // Create file browser container (left half)
    m_fileBrowserContainer = lv_obj_create(m_mainContainer);
    lv_obj_set_size(m_fileBrowserContainer, lv_pct(50), lv_pct(85));
    lv_obj_align_to(m_fileBrowserContainer, m_toolbar, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    lv_obj_set_style_bg_color(m_fileBrowserContainer, COLOR_SURFACE, 0);
    lv_obj_set_style_border_color(m_fileBrowserContainer, COLOR_PRIMARY, 0);
    lv_obj_set_style_border_width(m_fileBrowserContainer, 1, 0);
    lv_obj_set_style_pad_all(m_fileBrowserContainer, 5, 0);
    
    // File list header
    lv_obj_t* headerLabel = lv_label_create(m_fileBrowserContainer);
    lv_label_set_text(headerLabel, "Python Scripts");
    lv_obj_set_style_text_color(headerLabel, COLOR_PRIMARY, 0);
    lv_obj_align(headerLabel, LV_ALIGN_TOP_MID, 0, 5);
    
    // File list widget
    m_fileListWidget = lv_list_create(m_fileBrowserContainer);
    lv_obj_set_size(m_fileListWidget, lv_pct(100), lv_pct(90));
    lv_obj_align_to(m_fileListWidget, headerLabel, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
    lv_obj_set_style_bg_color(m_fileListWidget, COLOR_BACKGROUND, 0);
    lv_obj_add_event_cb(m_fileListWidget, fileListCallback, LV_EVENT_CLICKED, this);
}

void MicroPythonLauncherApp::createConsoleOutput() {
    // Create console container (right half)
    m_consoleContainer = lv_obj_create(m_mainContainer);
    lv_obj_set_size(m_consoleContainer, lv_pct(48), lv_pct(85));
    lv_obj_align_to(m_consoleContainer, m_toolbar, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 5);
    lv_obj_set_style_bg_color(m_consoleContainer, COLOR_SURFACE, 0);
    lv_obj_set_style_border_color(m_consoleContainer, COLOR_PRIMARY, 0);
    lv_obj_set_style_border_width(m_consoleContainer, 1, 0);
    lv_obj_set_style_pad_all(m_consoleContainer, 5, 0);
    
    // Console header
    m_consoleLabel = lv_label_create(m_consoleContainer);
    lv_label_set_text(m_consoleLabel, "Console Output");
    lv_obj_set_style_text_color(m_consoleLabel, COLOR_PRIMARY, 0);
    lv_obj_align(m_consoleLabel, LV_ALIGN_TOP_MID, 0, 5);
    
    // Console text area
    m_consoleTextArea = lv_textarea_create(m_consoleContainer);
    lv_obj_set_size(m_consoleTextArea, lv_pct(100), lv_pct(75));
    lv_obj_align_to(m_consoleTextArea, m_consoleLabel, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
    lv_textarea_set_text(m_consoleTextArea, "MicroPython Console Ready\n");
    lv_obj_set_style_bg_color(m_consoleTextArea, lv_color_black(), 0);
    lv_obj_set_style_text_color(m_consoleTextArea, lv_color_white(), 0);
    lv_obj_set_style_text_font(m_consoleTextArea, &lv_font_montserrat_12, 0);
    
    // Control buttons container
    m_controlContainer = lv_obj_create(m_consoleContainer);
    lv_obj_set_size(m_controlContainer, lv_pct(100), 40);
    lv_obj_align_to(m_controlContainer, m_consoleTextArea, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
    lv_obj_set_style_bg_opa(m_controlContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(m_controlContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(m_controlContainer, 0, 0);
    lv_obj_set_flex_flow(m_controlContainer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(m_controlContainer, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Execute button
    m_executeButton = lv_btn_create(m_controlContainer);
    lv_obj_set_size(m_executeButton, 80, 35);
    lv_obj_add_event_cb(m_executeButton, executeButtonCallback, LV_EVENT_CLICKED, this);
    lv_obj_set_style_bg_color(m_executeButton, COLOR_SUCCESS, 0);
    lv_obj_t* execLabel = lv_label_create(m_executeButton);
    lv_label_set_text(execLabel, "Run");
    lv_obj_center(execLabel);
    
    // Stop button
    m_stopButton = lv_btn_create(m_controlContainer);
    lv_obj_set_size(m_stopButton, 80, 35);
    lv_obj_add_event_cb(m_stopButton, stopButtonCallback, LV_EVENT_CLICKED, this);
    lv_obj_set_style_bg_color(m_stopButton, COLOR_ERROR, 0);
    lv_obj_t* stopLabel = lv_label_create(m_stopButton);
    lv_label_set_text(stopLabel, "Stop");
    lv_obj_center(stopLabel);
    lv_obj_add_flag(m_stopButton, LV_OBJ_FLAG_HIDDEN); // Initially hidden
    
    // Clear button
    m_clearButton = lv_btn_create(m_controlContainer);
    lv_obj_set_size(m_clearButton, 80, 35);
    lv_obj_add_event_cb(m_clearButton, clearButtonCallback, LV_EVENT_CLICKED, this);
    lv_obj_set_style_bg_color(m_clearButton, COLOR_WARNING, 0);
    lv_obj_t* clearLabel = lv_label_create(m_clearButton);
    lv_label_set_text(clearLabel, "Clear");
    lv_obj_center(clearLabel);
    
    // Status label
    m_statusLabel = lv_label_create(m_consoleContainer);
    lv_label_set_text(m_statusLabel, "Ready");
    lv_obj_set_style_text_color(m_statusLabel, COLOR_SUCCESS, 0);
    lv_obj_align(m_statusLabel, LV_ALIGN_BOTTOM_MID, 0, -5);
}

void MicroPythonLauncherApp::updateFileList() {
    if (!m_fileListWidget) return;
    
    // Clear existing list
    lv_obj_clean(m_fileListWidget);
    
    // Add parent directory option if not at root
    if (m_currentPath != "/sdcard" && m_currentPath != "/") {
        lv_obj_t* btn = lv_list_add_btn(m_fileListWidget, LV_SYMBOL_DIRECTORY, "..");
        lv_obj_set_style_bg_color(btn, COLOR_SURFACE, 0);
        lv_obj_set_user_data(btn, (void*)SIZE_MAX); // Special marker for parent directory
    }
    
    // Add directories first
    for (size_t i = 0; i < m_fileList.size(); i++) {
        const FileItem& item = m_fileList[i];
        if (item.isDirectory) {
            lv_obj_t* btn = lv_list_add_btn(m_fileListWidget, LV_SYMBOL_DIRECTORY, item.name.c_str());
            lv_obj_set_style_bg_color(btn, COLOR_SURFACE, 0);
            lv_obj_set_user_data(btn, (void*)i);
        }
    }
    
    // Add Python files
    for (size_t i = 0; i < m_fileList.size(); i++) {
        const FileItem& item = m_fileList[i];
        if (!item.isDirectory && isPythonFile(item.name)) {
            char displayText[128];
            snprintf(displayText, sizeof(displayText), "%s (%s)", 
                    item.name.c_str(), formatFileSize(item.size).c_str());
            
            lv_obj_t* btn = lv_list_add_btn(m_fileListWidget, LV_SYMBOL_FILE, displayText);
            lv_obj_set_style_bg_color(btn, COLOR_SURFACE, 0);
            lv_obj_set_user_data(btn, (void*)i);
        }
    }
}

os_error_t MicroPythonLauncherApp::scanDirectory(const std::string& path) {
    m_fileList.clear();
    
    DIR* dir = opendir(path.c_str());
    if (!dir) {
        ESP_LOGW(TAG, "Failed to open directory: %s", path.c_str());
        return OS_ERROR_GENERIC;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr && m_fileList.size() < MAX_FILES_DISPLAYED) {
        // Skip hidden files and current directory
        if (entry->d_name[0] == '.' && strcmp(entry->d_name, "..") != 0) {
            continue;
        }
        
        std::string fullPath = path + "/" + entry->d_name;
        struct stat fileStat;
        
        if (stat(fullPath.c_str(), &fileStat) == 0) {
            FileItem item;
            item.name = entry->d_name;
            item.path = fullPath;
            item.isDirectory = S_ISDIR(fileStat.st_mode);
            item.size = fileStat.st_size;
            
            m_fileList.push_back(item);
        }
    }
    
    closedir(dir);
    
    // Sort: directories first, then files
    std::sort(m_fileList.begin(), m_fileList.end(), [](const FileItem& a, const FileItem& b) {
        if (a.isDirectory != b.isDirectory) {
            return a.isDirectory > b.isDirectory;
        }
        return a.name < b.name;
    });
    
    return OS_OK;
}

void MicroPythonLauncherApp::navigateToDirectory(const std::string& path) {
    if (scanDirectory(path) == OS_OK) {
        m_currentPath = path;
        if (m_pathLabel) {
            lv_label_set_text(m_pathLabel, m_currentPath.c_str());
        }
        updateFileList();
        ESP_LOGI(TAG, "Navigated to: %s", path.c_str());
    }
}

void MicroPythonLauncherApp::navigateUp() {
    size_t lastSlash = m_currentPath.find_last_of('/');
    if (lastSlash != std::string::npos && lastSlash > 0) {
        std::string parentPath = m_currentPath.substr(0, lastSlash);
        if (parentPath.empty()) {
            parentPath = "/";
        }
        navigateToDirectory(parentPath);
    }
}

os_error_t MicroPythonLauncherApp::executeScript(const std::string& scriptPath) {
    if (m_scriptRunning) {
        ESP_LOGW(TAG, "Script already running");
        return OS_ERROR_BUSY;
    }
    
    ESP_LOGI(TAG, "Executing script: %s", scriptPath.c_str());
    
    m_scriptRunning = true;
    updateExecutionStatus("Running script...");
    
    if (m_executeButton) {
        lv_obj_add_flag(m_executeButton, LV_OBJ_FLAG_HIDDEN);
    }
    if (m_stopButton) {
        lv_obj_clear_flag(m_stopButton, LV_OBJ_FLAG_HIDDEN);
    }
    
    // TODO: Implement actual MicroPython execution
    // For now, simulate script execution
    appendToConsole(">>> exec(open('" + scriptPath + "').read())");
    appendToConsole("Script execution started...");
    appendToConsole("Hello from MicroPython!");
    appendToConsole("Script completed successfully.");
    
    // Simulate script completion after a delay
    // In real implementation, this would be handled by the MicroPython interpreter
    
    return OS_OK;
}

void MicroPythonLauncherApp::stopScript() {
    if (!m_scriptRunning) {
        return;
    }
    
    ESP_LOGI(TAG, "Stopping script execution");
    
    m_scriptRunning = false;
    updateExecutionStatus("Script stopped");
    appendToConsole(">>> KeyboardInterrupt", true);
    
    if (m_executeButton) {
        lv_obj_clear_flag(m_executeButton, LV_OBJ_FLAG_HIDDEN);
    }
    if (m_stopButton) {
        lv_obj_add_flag(m_stopButton, LV_OBJ_FLAG_HIDDEN);
    }
}

void MicroPythonLauncherApp::clearConsole() {
    if (m_consoleTextArea) {
        lv_textarea_set_text(m_consoleTextArea, "MicroPython Console Ready\n");
    }
    ESP_LOGI(TAG, "Console cleared");
}

void MicroPythonLauncherApp::appendToConsole(const std::string& text, bool isError) {
    if (!m_consoleTextArea) return;
    
    const char* currentText = lv_textarea_get_text(m_consoleTextArea);
    std::string newText = std::string(currentText) + text + "\n";
    
    // Limit console text length
    if (newText.length() > MAX_CONSOLE_LINES * 80) {
        size_t newlinePos = newText.find('\n', newText.length() - MAX_CONSOLE_LINES * 80);
        if (newlinePos != std::string::npos) {
            newText = newText.substr(newlinePos + 1);
        }
    }
    
    lv_textarea_set_text(m_consoleTextArea, newText.c_str());
    lv_textarea_set_cursor_pos(m_consoleTextArea, LV_TEXTAREA_CURSOR_LAST);
    
    ESP_LOGI(TAG, "Console: %s", text.c_str());
}

void MicroPythonLauncherApp::updateExecutionStatus(const std::string& status) {
    if (m_statusLabel) {
        lv_label_set_text(m_statusLabel, status.c_str());
        
        if (status.find("Error") != std::string::npos || status.find("Failed") != std::string::npos) {
            lv_obj_set_style_text_color(m_statusLabel, COLOR_ERROR, 0);
        } else if (status.find("Running") != std::string::npos) {
            lv_obj_set_style_text_color(m_statusLabel, COLOR_WARNING, 0);
        } else {
            lv_obj_set_style_text_color(m_statusLabel, COLOR_SUCCESS, 0);
        }
    }
}

bool MicroPythonLauncherApp::isPythonFile(const std::string& filename) const {
    return filename.length() > 3 && filename.substr(filename.length() - 3) == ".py";
}

std::string MicroPythonLauncherApp::formatFileSize(size_t size) const {
    char buffer[32];
    if (size < 1024) {
        snprintf(buffer, sizeof(buffer), "%zu B", size);
    } else if (size < 1024 * 1024) {
        snprintf(buffer, sizeof(buffer), "%.1f KB", size / 1024.0f);
    } else {
        snprintf(buffer, sizeof(buffer), "%.1f MB", size / (1024.0f * 1024.0f));
    }
    return std::string(buffer);
}

// UI Event Callbacks

void MicroPythonLauncherApp::fileListCallback(lv_event_t* e) {
    MicroPythonLauncherApp* app = static_cast<MicroPythonLauncherApp*>(lv_event_get_user_data(e));
    lv_obj_t* btn = lv_event_get_target(e);
    size_t index = reinterpret_cast<size_t>(lv_obj_get_user_data(btn));
    
    if (index == SIZE_MAX) {
        // Parent directory
        app->navigateUp();
    } else if (index < app->m_fileList.size()) {
        const FileItem& item = app->m_fileList[index];
        if (item.isDirectory) {
            app->navigateToDirectory(item.path);
        } else if (app->isPythonFile(item.name)) {
            app->m_selectedFile = item.path;
            app->appendToConsole("Selected: " + item.name);
        }
    }
}

void MicroPythonLauncherApp::backButtonCallback(lv_event_t* e) {
    MicroPythonLauncherApp* app = static_cast<MicroPythonLauncherApp*>(lv_event_get_user_data(e));
    app->navigateUp();
}

void MicroPythonLauncherApp::refreshButtonCallback(lv_event_t* e) {
    MicroPythonLauncherApp* app = static_cast<MicroPythonLauncherApp*>(lv_event_get_user_data(e));
    app->scanDirectory(app->m_currentPath);
    app->updateFileList();
    app->appendToConsole("Directory refreshed");
}

void MicroPythonLauncherApp::executeButtonCallback(lv_event_t* e) {
    MicroPythonLauncherApp* app = static_cast<MicroPythonLauncherApp*>(lv_event_get_user_data(e));
    if (!app->m_selectedFile.empty()) {
        app->executeScript(app->m_selectedFile);
    } else {
        app->appendToConsole("No script selected", true);
    }
}

void MicroPythonLauncherApp::stopButtonCallback(lv_event_t* e) {
    MicroPythonLauncherApp* app = static_cast<MicroPythonLauncherApp*>(lv_event_get_user_data(e));
    app->stopScript();
}

void MicroPythonLauncherApp::clearButtonCallback(lv_event_t* e) {
    MicroPythonLauncherApp* app = static_cast<MicroPythonLauncherApp*>(lv_event_get_user_data(e));
    app->clearConsole();
}

// Factory function for app manager
extern "C" std::unique_ptr<BaseApp> createMicroPythonLauncherApp() {
    return std::make_unique<MicroPythonLauncherApp>();
}