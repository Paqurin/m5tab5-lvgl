#include "micropython_launcher_app.h"
#include "../system/os_manager.h"
#include <esp_log.h>
#include <dirent.h>
#include <sys/stat.h>
#include <cstring>
#include <algorithm>

static const char* TAG = "MicroPythonLauncher";

MicroPythonLauncherApp::MicroPythonLauncherApp() 
    : BaseApp("com.m5stack.micropython_launcher", "MicroPython Launcher", "1.0.0") {
    setDescription("Browse and execute MicroPython scripts with console output");
    setAuthor("M5Stack Tab5 OS");
    setPriority(AppPriority::APP_NORMAL);
    
    // Initialize state
    m_currentPath = "/storage/scripts";
    m_scriptRunning = false;
    m_selectedFile = "";
}

MicroPythonLauncherApp::~MicroPythonLauncherApp() {
    shutdown();
}

os_error_t MicroPythonLauncherApp::initialize() {
    if (m_initialized) {
        return OS_OK;
    }

    log(ESP_LOG_INFO, "Initializing MicroPython Launcher");
    
    // Set memory usage estimate
    setMemoryUsage(128 * 1024); // 128KB estimated usage
    
    // Create scripts directory if it doesn't exist
    struct stat st;
    if (stat(m_currentPath.c_str(), &st) != 0) {
        // Create directory structure
        system("mkdir -p /storage/scripts");
        
        // Create a sample script
        FILE* sample = fopen("/storage/scripts/hello.py", "w");
        if (sample) {
            fprintf(sample, "# Sample MicroPython Script\n");
            fprintf(sample, "print('Hello from MicroPython!')\n");
            fprintf(sample, "print('Tab5 OS:', 'Running on ESP32-P4')\n");
            fprintf(sample, "\n");
            fprintf(sample, "import time\n");
            fprintf(sample, "for i in range(5):\n");
            fprintf(sample, "    print(f'Count: {i}')\n");
            fprintf(sample, "    time.sleep(0.5)\n");
            fprintf(sample, "\n");
            fprintf(sample, "print('Script completed!')\n");
            fclose(sample);
        }
    }
    
    // Initial directory scan
    scanDirectory(m_currentPath);
    
    m_initialized = true;
    return OS_OK;
}

os_error_t MicroPythonLauncherApp::update(uint32_t deltaTime) {
    // Periodic directory rescan
    uint32_t currentTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
    if (currentTime - m_lastScanTime > SCAN_INTERVAL_MS) {
        m_lastScanTime = currentTime;
        scanDirectory(m_currentPath);
        updateFileList();
    }
    
    return OS_OK;
}

os_error_t MicroPythonLauncherApp::shutdown() {
    if (!m_initialized) {
        return OS_OK;
    }

    log(ESP_LOG_INFO, "Shutting down MicroPython Launcher");
    
    // Stop any running script
    if (m_scriptRunning) {
        stopScript();
    }
    
    // Clear file list
    m_fileList.clear();
    
    m_initialized = false;
    return OS_OK;
}

os_error_t MicroPythonLauncherApp::createUI(lv_obj_t* parent) {
    if (!parent) {
        return OS_ERROR_INVALID_PARAM;
    }

    // Create main container
    m_mainContainer = lv_obj_create(parent);
    lv_obj_set_size(m_mainContainer, LV_HOR_RES, 
                    LV_VER_RES - 60 - 40); // Account for status bar and dock
    lv_obj_align(m_mainContainer, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(m_mainContainer, COLOR_BACKGROUND, 0);
    lv_obj_set_style_border_opa(m_mainContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(m_mainContainer, 8, 0);
    
    createMainUI();
    
    return OS_OK;
}

os_error_t MicroPythonLauncherApp::destroyUI() {
    if (m_mainContainer) {
        lv_obj_del(m_mainContainer);
        m_mainContainer = nullptr;
        
        // Reset all UI element pointers
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
    // Handle system events if needed
    return OS_OK;
}

void MicroPythonLauncherApp::createMainUI() {
    createToolbar();
    createFileBrowser();
    createConsoleOutput();
    
    // Initial file list update
    updateFileList();
    
    // Initial status
    updateExecutionStatus("Ready");
}

void MicroPythonLauncherApp::createToolbar() {
    // Create toolbar container
    m_toolbar = lv_obj_create(m_mainContainer);
    lv_obj_set_size(m_toolbar, LV_PCT(100), 50);
    lv_obj_align(m_toolbar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(m_toolbar, COLOR_SURFACE, 0);
    lv_obj_set_style_border_opa(m_toolbar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(m_toolbar, 4, 0);
    lv_obj_set_flex_flow(m_toolbar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(m_toolbar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Back button
    m_backButton = lv_btn_create(m_toolbar);
    lv_obj_set_size(m_backButton, 40, 35);
    lv_obj_set_style_bg_color(m_backButton, COLOR_PRIMARY, 0);
    
    lv_obj_t* backIcon = lv_label_create(m_backButton);
    lv_label_set_text(backIcon, LV_SYMBOL_LEFT);
    lv_obj_center(backIcon);
    
    lv_obj_add_event_cb(m_backButton, backButtonCallback, LV_EVENT_CLICKED, this);
    
    // Refresh button
    m_refreshButton = lv_btn_create(m_toolbar);
    lv_obj_set_size(m_refreshButton, 40, 35);
    lv_obj_set_style_bg_color(m_refreshButton, COLOR_PRIMARY, 0);
    lv_obj_set_style_pad_left(m_refreshButton, 5, 0);
    
    lv_obj_t* refreshIcon = lv_label_create(m_refreshButton);
    lv_label_set_text(refreshIcon, LV_SYMBOL_REFRESH);
    lv_obj_center(refreshIcon);
    
    lv_obj_add_event_cb(m_refreshButton, refreshButtonCallback, LV_EVENT_CLICKED, this);
    
    // Path label
    m_pathLabel = lv_label_create(m_toolbar);
    lv_label_set_text(m_pathLabel, m_currentPath.c_str());
    lv_obj_set_style_text_color(m_pathLabel, lv_color_white(), 0);
    lv_obj_set_style_pad_left(m_pathLabel, 10, 0);
    lv_label_set_long_mode(m_pathLabel, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(m_pathLabel, LV_PCT(60));
}

void MicroPythonLauncherApp::createFileBrowser() {
    // File browser container (left half)
    m_fileBrowserContainer = lv_obj_create(m_mainContainer);
    lv_obj_set_size(m_fileBrowserContainer, LV_PCT(48), LV_PCT(75));
    lv_obj_align_to(m_fileBrowserContainer, m_toolbar, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    lv_obj_set_style_bg_color(m_fileBrowserContainer, COLOR_SURFACE, 0);
    lv_obj_set_style_border_color(m_fileBrowserContainer, COLOR_PRIMARY, 0);
    lv_obj_set_style_border_width(m_fileBrowserContainer, 1, 0);
    lv_obj_set_style_pad_all(m_fileBrowserContainer, 5, 0);
    
    // File list
    m_fileListWidget = lv_list_create(m_fileBrowserContainer);
    lv_obj_set_size(m_fileListWidget, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(m_fileListWidget, COLOR_BACKGROUND, 0);
    lv_obj_set_style_border_opa(m_fileListWidget, LV_OPA_TRANSP, 0);
}

void MicroPythonLauncherApp::createConsoleOutput() {
    // Console container (right half)
    m_consoleContainer = lv_obj_create(m_mainContainer);
    lv_obj_set_size(m_consoleContainer, LV_PCT(48), LV_PCT(75));
    lv_obj_align_to(m_consoleContainer, m_toolbar, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 5);
    lv_obj_set_style_bg_color(m_consoleContainer, COLOR_SURFACE, 0);
    lv_obj_set_style_border_color(m_consoleContainer, COLOR_PRIMARY, 0);
    lv_obj_set_style_border_width(m_consoleContainer, 1, 0);
    lv_obj_set_style_pad_all(m_consoleContainer, 5, 0);
    
    // Console label
    m_consoleLabel = lv_label_create(m_consoleContainer);
    lv_label_set_text(m_consoleLabel, "Console Output");
    lv_obj_set_style_text_color(m_consoleLabel, lv_color_white(), 0);
    lv_obj_align(m_consoleLabel, LV_ALIGN_TOP_LEFT, 0, 0);
    
    // Console text area
    m_consoleTextArea = lv_textarea_create(m_consoleContainer);
    lv_obj_set_size(m_consoleTextArea, LV_PCT(100), LV_PCT(85));
    lv_obj_align_to(m_consoleTextArea, m_consoleLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    lv_obj_set_style_bg_color(m_consoleTextArea, lv_color_black(), 0);
    lv_obj_set_style_text_color(m_consoleTextArea, lv_color_white(), 0);
    lv_obj_set_style_border_color(m_consoleTextArea, COLOR_PRIMARY, 0);
    lv_textarea_set_text(m_consoleTextArea, "MicroPython Console Ready\n");
    lv_textarea_set_cursor_pos(m_consoleTextArea, LV_TEXTAREA_CURSOR_LAST);
    
    // Control buttons container
    m_controlContainer = lv_obj_create(m_mainContainer);
    lv_obj_set_size(m_controlContainer, LV_PCT(100), 40);
    lv_obj_align_to(m_controlContainer, m_fileBrowserContainer, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    lv_obj_set_style_bg_color(m_controlContainer, COLOR_SURFACE, 0);
    lv_obj_set_style_border_opa(m_controlContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(m_controlContainer, 4, 0);
    lv_obj_set_flex_flow(m_controlContainer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(m_controlContainer, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Execute button
    m_executeButton = lv_btn_create(m_controlContainer);
    lv_obj_set_size(m_executeButton, 100, 32);
    lv_obj_set_style_bg_color(m_executeButton, COLOR_SUCCESS, 0);
    
    lv_obj_t* executeLabel = lv_label_create(m_executeButton);
    lv_label_set_text(executeLabel, "Execute");
    lv_obj_center(executeLabel);
    
    lv_obj_add_event_cb(m_executeButton, executeButtonCallback, LV_EVENT_CLICKED, this);
    
    // Stop button
    m_stopButton = lv_btn_create(m_controlContainer);
    lv_obj_set_size(m_stopButton, 80, 32);
    lv_obj_set_style_bg_color(m_stopButton, COLOR_ERROR, 0);
    lv_obj_add_state(m_stopButton, LV_STATE_DISABLED);
    
    lv_obj_t* stopLabel = lv_label_create(m_stopButton);
    lv_label_set_text(stopLabel, "Stop");
    lv_obj_center(stopLabel);
    
    lv_obj_add_event_cb(m_stopButton, stopButtonCallback, LV_EVENT_CLICKED, this);
    
    // Clear button
    m_clearButton = lv_btn_create(m_controlContainer);
    lv_obj_set_size(m_clearButton, 80, 32);
    lv_obj_set_style_bg_color(m_clearButton, COLOR_WARNING, 0);
    
    lv_obj_t* clearLabel = lv_label_create(m_clearButton);
    lv_label_set_text(clearLabel, "Clear");
    lv_obj_center(clearLabel);
    
    lv_obj_add_event_cb(m_clearButton, clearButtonCallback, LV_EVENT_CLICKED, this);
    
    // Status label
    m_statusLabel = lv_label_create(m_controlContainer);
    lv_label_set_text(m_statusLabel, "Ready");
    lv_obj_set_style_text_color(m_statusLabel, lv_color_white(), 0);
}

void MicroPythonLauncherApp::updateFileList() {
    if (!m_fileListWidget) return;
    
    // Clear existing items
    lv_obj_clean(m_fileListWidget);
    
    // Add files to list
    for (const auto& file : m_fileList) {
        lv_obj_t* btn = lv_list_add_btn(m_fileListWidget, 
                                        file.isDirectory ? LV_SYMBOL_DIRECTORY : LV_SYMBOL_FILE,
                                        file.name.c_str());
        
        // Set color based on file type
        if (file.isDirectory) {
            lv_obj_set_style_text_color(btn, lv_color_hex(0xF39C12), 0);
        } else if (isPythonFile(file.name)) {
            lv_obj_set_style_text_color(btn, COLOR_SUCCESS, 0);
        } else {
            lv_obj_set_style_text_color(btn, lv_color_white(), 0);
        }
        
        lv_obj_add_event_cb(btn, fileListCallback, LV_EVENT_CLICKED, this);
        lv_obj_set_user_data(btn, (void*)&file);
    }
}

os_error_t MicroPythonLauncherApp::scanDirectory(const std::string& path) {
    m_fileList.clear();
    
    DIR* dir = opendir(path.c_str());
    if (!dir) {
        log(ESP_LOG_WARN, "Cannot open directory: %s", path.c_str());
        return OS_ERROR_NOT_FOUND;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        // Skip hidden files and current directory
        if (entry->d_name[0] == '.' && strcmp(entry->d_name, "..") != 0) {
            continue;
        }
        
        FileItem item;
        item.name = entry->d_name;
        item.path = path + "/" + item.name;
        item.isDirectory = (entry->d_type == DT_DIR);
        
        // Get file size
        struct stat fileStat;
        if (stat(item.path.c_str(), &fileStat) == 0) {
            item.size = fileStat.st_size;
        } else {
            item.size = 0;
        }
        
        m_fileList.push_back(item);
    }
    
    closedir(dir);
    
    // Sort: directories first, then files alphabetically
    std::sort(m_fileList.begin(), m_fileList.end(), 
              [](const FileItem& a, const FileItem& b) {
                  if (a.isDirectory != b.isDirectory) {
                      return a.isDirectory;
                  }
                  return a.name < b.name;
              });
    
    return OS_OK;
}

void MicroPythonLauncherApp::navigateToDirectory(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
        m_currentPath = path;
        scanDirectory(m_currentPath);
        updateFileList();
        lv_label_set_text(m_pathLabel, m_currentPath.c_str());
    }
}

void MicroPythonLauncherApp::navigateUp() {
    size_t lastSlash = m_currentPath.find_last_of('/');
    if (lastSlash != std::string::npos && lastSlash > 0) {
        std::string parentPath = m_currentPath.substr(0, lastSlash);
        navigateToDirectory(parentPath);
    }
}

os_error_t MicroPythonLauncherApp::executeScript(const std::string& scriptPath) {
    if (m_scriptRunning) {
        appendToConsole("Script already running. Stop current script first.\n", true);
        return OS_ERROR_BUSY;
    }
    
    // Check if file exists
    struct stat st;
    if (stat(scriptPath.c_str(), &st) != 0) {
        appendToConsole("Error: Script file not found\n", true);
        return OS_ERROR_NOT_FOUND;
    }
    
    // For now, simulate script execution by reading and displaying the file
    appendToConsole("\n--- Executing: " + scriptPath + " ---\n");
    
    FILE* file = fopen(scriptPath.c_str(), "r");
    if (!file) {
        appendToConsole("Error: Cannot open script file\n", true);
        return OS_ERROR_GENERIC;
    }
    
    char line[256];
    appendToConsole("Script content:\n");
    while (fgets(line, sizeof(line), file)) {
        appendToConsole("> " + std::string(line));
    }
    fclose(file);
    
    appendToConsole("\n--- Simulation: Script would execute here ---\n");
    appendToConsole("Note: Full MicroPython integration requires custom firmware build\n");
    appendToConsole("--- Execution completed ---\n\n");
    
    m_scriptRunning = false;
    updateExecutionStatus("Execution completed");
    
    // Update button states
    lv_obj_add_state(m_stopButton, LV_STATE_DISABLED);
    lv_obj_clear_state(m_executeButton, LV_STATE_DISABLED);
    
    return OS_OK;
}

void MicroPythonLauncherApp::stopScript() {
    if (!m_scriptRunning) {
        return;
    }
    
    appendToConsole("\n--- Script execution stopped ---\n");
    m_scriptRunning = false;
    updateExecutionStatus("Stopped");
    
    // Update button states
    lv_obj_add_state(m_stopButton, LV_STATE_DISABLED);
    lv_obj_clear_state(m_executeButton, LV_STATE_DISABLED);
}

void MicroPythonLauncherApp::clearConsole() {
    if (m_consoleTextArea) {
        lv_textarea_set_text(m_consoleTextArea, "");
    }
}

void MicroPythonLauncherApp::appendToConsole(const std::string& text, bool isError) {
    if (!m_consoleTextArea) return;
    
    // Get current text
    const char* currentText = lv_textarea_get_text(m_consoleTextArea);
    std::string newText = std::string(currentText) + text;
    
    // Limit console size
    size_t lineCount = std::count(newText.begin(), newText.end(), '\n');
    if (lineCount > MAX_CONSOLE_LINES) {
        size_t firstNewline = newText.find('\n');
        if (firstNewline != std::string::npos) {
            newText = newText.substr(firstNewline + 1);
        }
    }
    
    lv_textarea_set_text(m_consoleTextArea, newText.c_str());
    lv_textarea_set_cursor_pos(m_consoleTextArea, LV_TEXTAREA_CURSOR_LAST);
}

void MicroPythonLauncherApp::updateExecutionStatus(const std::string& status) {
    if (m_statusLabel) {
        lv_label_set_text(m_statusLabel, status.c_str());
    }
}

bool MicroPythonLauncherApp::isPythonFile(const std::string& filename) const {
    return filename.length() > 3 && filename.substr(filename.length() - 3) == ".py";
}

std::string MicroPythonLauncherApp::formatFileSize(size_t size) const {
    if (size < 1024) {
        return std::to_string(size) + " B";
    } else if (size < 1024 * 1024) {
        return std::to_string(size / 1024) + " KB";
    } else {
        return std::to_string(size / (1024 * 1024)) + " MB";
    }
}

// Static callback functions
void MicroPythonLauncherApp::fileListCallback(lv_event_t* e) {
    MicroPythonLauncherApp* app = static_cast<MicroPythonLauncherApp*>(lv_event_get_user_data(e));
    lv_obj_t* btn = lv_event_get_target(e);
    
    if (app && btn) {
        FileItem* file = static_cast<FileItem*>(lv_obj_get_user_data(btn));
        if (file) {
            if (file->isDirectory) {
                if (file->name == "..") {
                    app->navigateUp();
                } else {
                    app->navigateToDirectory(file->path);
                }
            } else {
                app->m_selectedFile = file->path;
                app->appendToConsole("Selected: " + file->name + "\n");
            }
        }
    }
}

void MicroPythonLauncherApp::backButtonCallback(lv_event_t* e) {
    MicroPythonLauncherApp* app = static_cast<MicroPythonLauncherApp*>(lv_event_get_user_data(e));
    if (app) {
        app->navigateUp();
    }
}

void MicroPythonLauncherApp::refreshButtonCallback(lv_event_t* e) {
    MicroPythonLauncherApp* app = static_cast<MicroPythonLauncherApp*>(lv_event_get_user_data(e));
    if (app) {
        app->scanDirectory(app->m_currentPath);
        app->updateFileList();
        app->appendToConsole("Directory refreshed\n");
    }
}

void MicroPythonLauncherApp::executeButtonCallback(lv_event_t* e) {
    MicroPythonLauncherApp* app = static_cast<MicroPythonLauncherApp*>(lv_event_get_user_data(e));
    if (app && !app->m_selectedFile.empty()) {
        if (app->isPythonFile(app->m_selectedFile)) {
            app->updateExecutionStatus("Executing...");
            lv_obj_add_state(app->m_executeButton, LV_STATE_DISABLED);
            lv_obj_clear_state(app->m_stopButton, LV_STATE_DISABLED);
            app->m_scriptRunning = true;
            
            app->executeScript(app->m_selectedFile);
        } else {
            app->appendToConsole("Error: Please select a Python (.py) file\n", true);
        }
    } else {
        app->appendToConsole("Error: No file selected\n", true);
    }
}

void MicroPythonLauncherApp::stopButtonCallback(lv_event_t* e) {
    MicroPythonLauncherApp* app = static_cast<MicroPythonLauncherApp*>(lv_event_get_user_data(e));
    if (app) {
        app->stopScript();
    }
}

void MicroPythonLauncherApp::clearButtonCallback(lv_event_t* e) {
    MicroPythonLauncherApp* app = static_cast<MicroPythonLauncherApp*>(lv_event_get_user_data(e));
    if (app) {
        app->clearConsole();
        app->appendToConsole("MicroPython Console Ready\n");
    }
}

// Factory function for app creation
extern "C" std::unique_ptr<BaseApp> createMicroPythonLauncherApp() {
    return std::make_unique<MicroPythonLauncherApp>();
}