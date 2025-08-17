# M5Stack Tab5 OS - App Development Standard

## Table of Contents

1. [Overview](#overview)
2. [Prerequisites](#prerequisites)
3. [App Architecture](#app-architecture)
4. [BaseApp Interface](#baseapp-interface)
5. [App Lifecycle](#app-lifecycle)
6. [UI Development Guidelines](#ui-development-guidelines)
7. [Memory Management](#memory-management)
8. [Event System](#event-system)
9. [Resource Management](#resource-management)
10. [Package Format](#package-format)
11. [Installation System](#installation-system)
12. [App Templates](#app-templates)
13. [Testing Guidelines](#testing-guidelines)
14. [Publishing](#publishing)
15. [Best Practices](#best-practices)
16. [Troubleshooting](#troubleshooting)

## Overview

The M5Stack Tab5 OS provides a modular application framework that allows developers to create installable applications similar to mobile app stores. This document defines the standard for developing compatible applications.

### System Specifications

- **Platform**: ESP32-P4 RISC-V 360MHz
- **Display**: 5-inch 1280x720 MIPI-DSI
- **Memory**: 500KB RAM, 32MB PSRAM, 16MB Flash
- **UI Framework**: LVGL 8.4
- **Programming Language**: C++17
- **Build System**: PlatformIO with ESP-IDF

## Prerequisites

### Development Environment

```bash
# Install PlatformIO
pip install platformio

# Clone the OS repository
git clone https://github.com/Paqurin/m5tab5-lvgl.git
cd m5tab5-lvgl

# Build the base system
pio run
```

### Required Knowledge

- C++17 programming
- LVGL UI framework basics
- ESP-IDF fundamentals
- FreeRTOS concepts
- Git version control

## App Architecture

### Core Components

Every app must inherit from `BaseApp` and implement required virtual methods:

```cpp
#include "apps/base_app.h"

class MyApp : public BaseApp {
public:
    MyApp();
    ~MyApp() override;

    // Required overrides
    os_error_t initialize() override;
    os_error_t update(uint32_t deltaTime) override;
    os_error_t shutdown() override;
    os_error_t createUI(lv_obj_t* parent) override;
    os_error_t destroyUI() override;

    // Optional overrides
    os_error_t handleEvent(uint32_t eventType, void* eventData, size_t dataSize) override;
    os_error_t start() override;
    os_error_t stop() override;
    os_error_t pause() override;
    os_error_t resume() override;
};
```

### File Structure

```
src/apps/my_app/
├── my_app.h              # Header file
├── my_app.cpp            # Implementation
├── my_app_config.h       # Configuration constants
├── ui/                   # UI components
│   ├── main_screen.cpp
│   ├── settings_dialog.cpp
│   └── components/
├── services/             # App-specific services
│   ├── data_service.cpp
│   └── network_service.cpp
├── resources/            # Assets
│   ├── icons/
│   ├── fonts/
│   └── images/
└── manifest.json         # App metadata
```

## BaseApp Interface

### Constructor Requirements

```cpp
MyApp::MyApp() : BaseApp("app_id", "App Name", "1.0.0") {
    setDescription("Description of what the app does");
    setAuthor("Your Name or Organization");
    setPriority(AppPriority::APP_NORMAL);
    setCategory("Productivity"); // Productivity, Entertainment, System, etc.
}
```

### Mandatory Method Implementations

#### initialize()

```cpp
os_error_t MyApp::initialize() {
    if (m_initialized) {
        return OS_OK;
    }

    log(ESP_LOG_INFO, "Initializing %s", getName().c_str());

    // Initialize your app's resources
    // Allocate memory
    // Setup configurations
    // Initialize services

    // Set memory usage estimate (important for system management)
    setMemoryUsage(estimated_memory_bytes);

    m_initialized = true;
    return OS_OK;
}
```

#### update()

```cpp
os_error_t MyApp::update(uint32_t deltaTime) {
    // Called every frame while app is running
    // deltaTime = milliseconds since last update
    
    // Update app logic
    // Process background tasks
    // Update animations
    // Handle periodic operations
    
    return OS_OK;
}
```

#### shutdown()

```cpp
os_error_t MyApp::shutdown() {
    if (!m_initialized) {
        return OS_OK;
    }

    log(ESP_LOG_INFO, "Shutting down %s", getName().c_str());

    // Clean up resources
    // Save state if needed
    // Stop background tasks
    // Free allocated memory

    m_initialized = false;
    return OS_OK;
}
```

#### createUI()

```cpp
os_error_t MyApp::createUI(lv_obj_t* parent) {
    if (!parent) {
        return OS_ERROR_INVALID_PARAM;
    }

    // Create main container
    m_uiContainer = lv_obj_create(parent);
    lv_obj_set_size(m_uiContainer, LV_HOR_RES, 
                    LV_VER_RES - OS_STATUS_BAR_HEIGHT - OS_DOCK_HEIGHT);
    lv_obj_align(m_uiContainer, LV_ALIGN_CENTER, 0, 0);
    
    // Apply theme
    lv_obj_set_style_bg_color(m_uiContainer, lv_color_hex(0x1E1E1E), 0);
    lv_obj_set_style_border_opa(m_uiContainer, LV_OPA_TRANSP, 0);

    // Create your UI components
    createMainUI();

    return OS_OK;
}
```

#### destroyUI()

```cpp
os_error_t MyApp::destroyUI() {
    if (m_uiContainer) {
        lv_obj_del(m_uiContainer);
        m_uiContainer = nullptr;
        // All child objects are automatically deleted
    }
    return OS_OK;
}
```

## App Lifecycle

### State Transitions

```
STOPPED → STARTING → RUNNING → PAUSED → RUNNING → STOPPING → STOPPED
     ↓                                                    ↑
   ERROR ←──────────────────────────────────────────────┘
```

### Lifecycle Methods

1. **Construction**: App object created
2. **initialize()**: Setup resources and configuration
3. **start()**: Begin execution (calls createUI)
4. **update()**: Called continuously while RUNNING
5. **pause()**: App goes to background
6. **resume()**: App returns to foreground
7. **stop()**: End execution (calls destroyUI)
8. **shutdown()**: Cleanup and destruction

### State Management

```cpp
void MyApp::onStateChange(AppState newState) {
    switch(newState) {
        case AppState::STARTING:
            // Prepare for execution
            break;
        case AppState::RUNNING:
            // App is active
            break;
        case AppState::PAUSED:
            // App is in background
            pauseOperations();
            break;
        case AppState::STOPPING:
            // Prepare for shutdown
            saveState();
            break;
    }
}
```

## UI Development Guidelines

### LVGL Integration

#### Theme Compliance

```cpp
// Use system theme colors
lv_color_t primary = lv_color_hex(0x3498DB);
lv_color_t secondary = lv_color_hex(0x2ECC71);
lv_color_t background = lv_color_hex(0x1E1E1E);
lv_color_t surface = lv_color_hex(0x2C2C2C);
lv_color_t text = lv_color_white();
```

#### Responsive Design

```cpp
// Adapt to different screen sizes
lv_coord_t screen_width = LV_HOR_RES;
lv_coord_t screen_height = LV_VER_RES;
lv_coord_t usable_height = screen_height - OS_STATUS_BAR_HEIGHT - OS_DOCK_HEIGHT;

// Use relative sizing
lv_obj_set_size(widget, LV_PCT(90), LV_PCT(80));
```

#### UI Components

```cpp
// Standard button creation
lv_obj_t* createStandardButton(lv_obj_t* parent, const char* text, 
                               lv_event_cb_t callback, void* user_data) {
    lv_obj_t* btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 120, 40);
    
    lv_obj_t* label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);
    
    if (callback) {
        lv_obj_add_event_cb(btn, callback, LV_EVENT_CLICKED, user_data);
    }
    
    return btn;
}
```

#### Event Handling

```cpp
// Event callback pattern
static void buttonCallback(lv_event_t* e) {
    MyApp* app = static_cast<MyApp*>(lv_event_get_user_data(e));
    lv_obj_t* target = lv_event_get_target(e);
    
    if (app) {
        app->handleButtonClick(target);
    }
}

// Register callback
lv_obj_add_event_cb(button, buttonCallback, LV_EVENT_CLICKED, this);
```

### Touch Input

```cpp
// Handle touch events
os_error_t MyApp::handleEvent(uint32_t eventType, void* eventData, size_t dataSize) {
    switch(eventType) {
        case EVENT_TOUCH_PRESS:
            // Handle touch press
            break;
        case EVENT_TOUCH_RELEASE:
            // Handle touch release
            break;
        case EVENT_GESTURE_SWIPE:
            // Handle swipe gesture
            break;
    }
    return OS_OK;
}
```

## Memory Management

### Memory Allocation

```cpp
// Use system heap for app data
void* MyApp::allocateMemory(size_t size) {
    void* ptr = heap_caps_malloc(size, MALLOC_CAP_DEFAULT);
    if (ptr) {
        m_allocatedMemory += size;
        updateMemoryUsage();
    }
    return ptr;
}

// Use DMA heap for hardware interfaces
void* MyApp::allocateDMAMemory(size_t size) {
    return heap_caps_malloc(size, MALLOC_CAP_DMA);
}
```

### Memory Limits

```cpp
// Respect memory limits
static constexpr size_t MAX_APP_MEMORY = 1024 * 1024; // 1MB per app
static constexpr size_t MAX_UI_OBJECTS = 100;         // UI object limit
static constexpr size_t MAX_BACKGROUND_TASKS = 4;     // Task limit
```

### Memory Monitoring

```cpp
void MyApp::updateMemoryUsage() {
    size_t currentUsage = getCurrentMemoryUsage();
    setMemoryUsage(currentUsage);
    
    if (currentUsage > MAX_APP_MEMORY) {
        log(ESP_LOG_WARN, "Memory usage exceeded limit: %d bytes", currentUsage);
        // Trigger cleanup or notify system
    }
}
```

## Event System

### Event Types

```cpp
// System events
#define EVENT_APP_LAUNCH        0x1000
#define EVENT_APP_EXIT          0x1001
#define EVENT_APP_PAUSE         0x1002
#define EVENT_APP_RESUME        0x1003

// Input events  
#define EVENT_TOUCH_PRESS       0x2000
#define EVENT_TOUCH_RELEASE     0x2001
#define EVENT_KEY_PRESS         0x2002

// Custom app events
#define EVENT_MY_APP_CUSTOM     0x8000
```

### Publishing Events

```cpp
// Publish custom event
EventData eventData;
eventData.type = EVENT_MY_APP_CUSTOM;
eventData.data = customData;
eventData.size = dataSize;

PUBLISH_EVENT(EVENT_MY_APP_CUSTOM, customData, dataSize);
```

### Subscribing to Events

```cpp
// Subscribe to events in initialize()
SUBSCRIBE_EVENT(EVENT_SYSTEM_TIME_CHANGED, 
    [this](const EventData& event) { handleTimeChange(event); });
```

## Resource Management

### File System Access

```cpp
// Read configuration file
os_error_t MyApp::loadConfiguration() {
    std::string configPath = getAppDataPath() + "/config.json";
    
    FILE* file = fopen(configPath.c_str(), "r");
    if (!file) {
        return OS_ERROR_NOT_FOUND;
    }
    
    // Read and parse configuration
    // ...
    
    fclose(file);
    return OS_OK;
}
```

### Asset Management

```cpp
// Load app icon
lv_obj_t* MyApp::createAppIcon(lv_obj_t* parent) {
    std::string iconPath = getResourcePath() + "/icon.png";
    
    lv_obj_t* img = lv_img_create(parent);
    lv_img_set_src(img, iconPath.c_str());
    
    return img;
}
```

### Data Persistence

```cpp
// Save app state
os_error_t MyApp::saveState() {
    std::string statePath = getAppDataPath() + "/state.bin";
    
    FILE* file = fopen(statePath.c_str(), "wb");
    if (!file) {
        return OS_ERROR_GENERIC;
    }
    
    AppState state = getCurrentState();
    fwrite(&state, sizeof(state), 1, file);
    fclose(file);
    
    return OS_OK;
}
```

## Package Format

### Manifest File (manifest.json)

```json
{
    "app": {
        "id": "com.example.myapp",
        "name": "My Application",
        "version": "1.0.0",
        "description": "A sample application for M5Stack Tab5",
        "author": "Developer Name",
        "email": "developer@example.com",
        "website": "https://example.com",
        "category": "Productivity",
        "tags": ["utility", "productivity", "demo"]
    },
    "system": {
        "min_os_version": "1.0.0",
        "target_platform": "m5stack-tab5",
        "architecture": "esp32p4"
    },
    "requirements": {
        "memory": {
            "ram": 65536,
            "flash": 262144,
            "psram": 131072
        },
        "permissions": [
            "STORAGE_READ",
            "STORAGE_WRITE", 
            "NETWORK_ACCESS",
            "CAMERA_ACCESS"
        ],
        "dependencies": [
            "base_system >= 1.0.0",
            "network_service >= 1.1.0"
        ]
    },
    "resources": {
        "icon": "assets/icon.png",
        "screenshots": [
            "assets/screenshot1.png",
            "assets/screenshot2.png"
        ],
        "assets": [
            "assets/fonts/",
            "assets/images/",
            "assets/sounds/"
        ]
    },
    "build": {
        "entry_point": "MyApp",
        "factory_function": "createMyApp",
        "compile_flags": ["-O2", "-DAPP_OPTIMIZED"],
        "link_libraries": ["m", "pthread"]
    }
}
```

### Package Structure

```
myapp.m5app (ZIP format)
├── manifest.json
├── src/
│   ├── my_app.cpp
│   ├── my_app.h
│   └── ui/
├── assets/
│   ├── icon.png
│   ├── images/
│   └── fonts/
├── docs/
│   ├── README.md
│   └── CHANGELOG.md
└── LICENSE
```

### Package Creation

```bash
# Create package directory
mkdir myapp-package
cd myapp-package

# Copy source files
cp -r ../src/apps/my_app ./src/
cp -r ../assets ./
cp manifest.json ./

# Create package
zip -r myapp-v1.0.0.m5app ./*
```

## Installation System

### App Factory Function

```cpp
// Required factory function for dynamic loading
extern "C" std::unique_ptr<BaseApp> createMyApp() {
    return std::make_unique<MyApp>();
}

// Registration with app manager
AppIntegration::registerApp("com.example.myapp", createMyApp);
```

### Installation Hooks

```cpp
// Pre-installation validation
os_error_t MyApp::validateInstallation(const AppPackage& package) {
    // Check system requirements
    if (package.requirements.memory.ram > getAvailableRAM()) {
        return OS_ERROR_NO_MEMORY;
    }
    
    // Verify dependencies
    for (const auto& dep : package.requirements.dependencies) {
        if (!isDependencyMet(dep)) {
            return OS_ERROR_DEPENDENCY;
        }
    }
    
    return OS_OK;
}

// Post-installation setup
os_error_t MyApp::postInstall() {
    // Create app data directory
    createAppDataDirectory();
    
    // Initialize default configuration
    createDefaultConfiguration();
    
    // Register with system services
    registerWithServices();
    
    return OS_OK;
}
```

## App Templates

### Basic App Template

```cpp
// my_app.h
#ifndef MY_APP_H
#define MY_APP_H

#include "apps/base_app.h"

class MyApp : public BaseApp {
public:
    MyApp();
    ~MyApp() override;

    os_error_t initialize() override;
    os_error_t update(uint32_t deltaTime) override;
    os_error_t shutdown() override;
    os_error_t createUI(lv_obj_t* parent) override;
    os_error_t destroyUI() override;

private:
    void createMainUI();
    void handleButtonClick(lv_obj_t* button);
    
    static void buttonCallback(lv_event_t* e);
    
    lv_obj_t* m_mainButton = nullptr;
    lv_obj_t* m_statusLabel = nullptr;
    
    bool m_isActive = false;
};

#endif // MY_APP_H
```

```cpp
// my_app.cpp
#include "my_app.h"
#include "../system/os_manager.h"

MyApp::MyApp() : BaseApp("com.example.myapp", "My App", "1.0.0") {
    setDescription("A sample application template");
    setAuthor("Developer Name");
    setPriority(AppPriority::APP_NORMAL);
}

MyApp::~MyApp() {
    shutdown();
}

os_error_t MyApp::initialize() {
    if (m_initialized) {
        return OS_OK;
    }

    log(ESP_LOG_INFO, "Initializing My App");
    setMemoryUsage(4096); // 4KB estimated usage
    
    m_initialized = true;
    return OS_OK;
}

os_error_t MyApp::update(uint32_t deltaTime) {
    // Update app logic here
    return OS_OK;
}

os_error_t MyApp::shutdown() {
    if (!m_initialized) {
        return OS_OK;
    }

    log(ESP_LOG_INFO, "Shutting down My App");
    m_initialized = false;
    return OS_OK;
}

os_error_t MyApp::createUI(lv_obj_t* parent) {
    m_uiContainer = lv_obj_create(parent);
    lv_obj_set_size(m_uiContainer, LV_HOR_RES, 
                    LV_VER_RES - OS_STATUS_BAR_HEIGHT - OS_DOCK_HEIGHT);
    lv_obj_align(m_uiContainer, LV_ALIGN_CENTER, 0, 0);
    
    createMainUI();
    return OS_OK;
}

os_error_t MyApp::destroyUI() {
    if (m_uiContainer) {
        lv_obj_del(m_uiContainer);
        m_uiContainer = nullptr;
    }
    return OS_OK;
}

void MyApp::createMainUI() {
    // Create main button
    m_mainButton = lv_btn_create(m_uiContainer);
    lv_obj_set_size(m_mainButton, 200, 50);
    lv_obj_center(m_mainButton);
    
    lv_obj_t* btnLabel = lv_label_create(m_mainButton);
    lv_label_set_text(btnLabel, "Click Me!");
    lv_obj_center(btnLabel);
    
    lv_obj_add_event_cb(m_mainButton, buttonCallback, LV_EVENT_CLICKED, this);
    
    // Create status label
    m_statusLabel = lv_label_create(m_uiContainer);
    lv_label_set_text(m_statusLabel, "Ready");
    lv_obj_align(m_statusLabel, LV_ALIGN_BOTTOM_MID, 0, -20);
}

void MyApp::handleButtonClick(lv_obj_t* button) {
    m_isActive = !m_isActive;
    lv_label_set_text(m_statusLabel, m_isActive ? "Active" : "Inactive");
}

void MyApp::buttonCallback(lv_event_t* e) {
    MyApp* app = static_cast<MyApp*>(lv_event_get_user_data(e));
    lv_obj_t* button = lv_event_get_target(e);
    
    if (app) {
        app->handleButtonClick(button);
    }
}

// Factory function for app creation
extern "C" std::unique_ptr<BaseApp> createMyApp() {
    return std::make_unique<MyApp>();
}
```

### Advanced App Template (with Services)

```cpp
class AdvancedApp : public BaseApp {
public:
    AdvancedApp();
    ~AdvancedApp() override;

    os_error_t initialize() override;
    os_error_t update(uint32_t deltaTime) override;
    os_error_t shutdown() override;
    os_error_t createUI(lv_obj_t* parent) override;
    os_error_t destroyUI() override;
    os_error_t handleEvent(uint32_t eventType, void* eventData, size_t dataSize) override;

private:
    // Service management
    void initializeServices();
    void shutdownServices();
    
    // Background tasks
    static void backgroundTask(void* parameter);
    void processBackgroundWork();
    
    // Data management
    os_error_t loadData();
    os_error_t saveData();
    
    // UI components
    void createToolbar();
    void createContent();
    void createStatusBar();
    
    // Event handlers
    void handleNetworkEvent(const EventData& event);
    void handleUserInput(const EventData& event);
    
    // Services
    class DataService* m_dataService = nullptr;
    class NetworkService* m_networkService = nullptr;
    
    // Background processing
    TaskHandle_t m_backgroundTaskHandle = nullptr;
    bool m_backgroundTaskRunning = false;
    
    // UI elements
    lv_obj_t* m_toolbar = nullptr;
    lv_obj_t* m_contentArea = nullptr;
    lv_obj_t* m_statusBar = nullptr;
    
    // App state
    struct AppData {
        uint32_t counter;
        bool isConnected;
        std::string lastMessage;
    } m_appData;
};
```

## Testing Guidelines

### Unit Testing

```cpp
// test_my_app.cpp
#include "unity.h"
#include "my_app.h"

class MyAppTest {
public:
    void setUp() {
        app = std::make_unique<MyApp>();
    }
    
    void tearDown() {
        app.reset();
    }
    
    void testInitialization() {
        os_error_t result = app->initialize();
        TEST_ASSERT_EQUAL(OS_OK, result);
        TEST_ASSERT_TRUE(app->isInitialized());
    }
    
    void testMemoryUsage() {
        app->initialize();
        size_t usage = app->getMemoryUsage();
        TEST_ASSERT_GREATER_THAN(0, usage);
        TEST_ASSERT_LESS_THAN(MAX_APP_MEMORY, usage);
    }
    
private:
    std::unique_ptr<MyApp> app;
};
```

### Integration Testing

```cpp
// Integration test with app manager
void testAppManagerIntegration() {
    AppManager manager;
    manager.initialize();
    
    // Register app
    os_error_t result = manager.registerApp("test_app", []() {
        return std::make_unique<MyApp>();
    });
    TEST_ASSERT_EQUAL(OS_OK, result);
    
    // Launch app
    result = manager.launchApp("test_app");
    TEST_ASSERT_EQUAL(OS_OK, result);
    
    // Verify app is running
    TEST_ASSERT_TRUE(manager.isAppRunning("test_app"));
    
    // Clean up
    manager.killApp("test_app");
    manager.shutdown();
}
```

### UI Testing

```cpp
// UI component testing
void testUICreation() {
    lv_obj_t* parent = lv_obj_create(lv_scr_act());
    
    MyApp app;
    app.initialize();
    
    os_error_t result = app.createUI(parent);
    TEST_ASSERT_EQUAL(OS_OK, result);
    
    // Verify UI elements exist
    TEST_ASSERT_NOT_NULL(app.getUIContainer());
    
    app.destroyUI();
    lv_obj_del(parent);
}
```

## Publishing

### App Store Submission

1. **Package Validation**
   ```bash
   # Validate package format
   m5app-validator myapp-v1.0.0.m5app
   
   # Check manifest
   m5app-validator --check-manifest manifest.json
   
   # Verify dependencies
   m5app-validator --check-deps myapp-v1.0.0.m5app
   ```

2. **Metadata Requirements**
   - Complete manifest.json
   - High-quality app icon (64x64 PNG)
   - Screenshots (1280x720 PNG)
   - Detailed description
   - Version changelog

3. **Code Review Checklist**
   - Memory usage within limits
   - Proper error handling
   - UI follows design guidelines
   - No system API misuse
   - Security best practices

### Versioning

```json
{
    "version": "1.2.3",
    "version_code": 123,
    "min_os_version": "1.0.0",
    "changelog": [
        {
            "version": "1.2.3",
            "date": "2024-01-15",
            "changes": [
                "Fixed memory leak in background task",
                "Improved UI responsiveness",
                "Added dark mode support"
            ]
        }
    ]
}
```

## Best Practices

### Performance

1. **Memory Efficiency**
   ```cpp
   // Use stack allocation when possible
   char buffer[256];  // Good
   char* buffer = malloc(256);  // Use only when necessary
   
   // Free resources promptly
   void cleanup() {
       if (buffer) {
           free(buffer);
           buffer = nullptr;
       }
   }
   ```

2. **UI Optimization**
   ```cpp
   // Batch UI updates
   lv_obj_add_flag(container, LV_OBJ_FLAG_HIDDEN);
   // Make multiple changes
   lv_obj_clear_flag(container, LV_OBJ_FLAG_HIDDEN);
   
   // Use object pools for frequent creation/destruction
   class UIObjectPool {
       std::vector<lv_obj_t*> pool;
   public:
       lv_obj_t* acquire() { /* ... */ }
       void release(lv_obj_t* obj) { /* ... */ }
   };
   ```

3. **Background Processing**
   ```cpp
   // Use appropriate task priorities
   xTaskCreate(backgroundTask, "bg_task", 4096, this, 
               tskIDLE_PRIORITY + 1, &taskHandle);
   
   // Yield regularly in long operations
   void longOperation() {
       for (int i = 0; i < 10000; i++) {
           processItem(i);
           if (i % 100 == 0) {
               vTaskDelay(1); // Yield to other tasks
           }
       }
   }
   ```

### Security

1. **Input Validation**
   ```cpp
   bool validateUserInput(const std::string& input) {
       // Check length
       if (input.length() > MAX_INPUT_LENGTH) {
           return false;
       }
       
       // Sanitize special characters
       for (char c : input) {
           if (!isalnum(c) && c != ' ' && c != '-' && c != '_') {
               return false;
           }
       }
       
       return true;
   }
   ```

2. **Secure Storage**
   ```cpp
   // Encrypt sensitive data
   os_error_t saveSecureData(const std::string& data) {
       std::string encrypted = encryptData(data, getAppKey());
       return writeToFile(getSecureDataPath(), encrypted);
   }
   ```

3. **Network Security**
   ```cpp
   // Use secure connections
   bool connectSecurely(const std::string& host, uint16_t port) {
       // Prefer HTTPS/TLS
       if (port == 80) {
           log(ESP_LOG_WARN, "Using insecure HTTP connection");
       }
       
       // Verify certificates
       return establishSecureConnection(host, port, true);
   }
   ```

### Code Organization

1. **Modular Design**
   ```cpp
   // Separate concerns
   class MyApp : public BaseApp {
   private:
       std::unique_ptr<UIManager> m_uiManager;
       std::unique_ptr<DataManager> m_dataManager;
       std::unique_ptr<NetworkManager> m_networkManager;
   };
   ```

2. **Error Handling**
   ```cpp
   // Consistent error handling
   os_error_t performOperation() {
       if (!isInitialized()) {
           log(ESP_LOG_ERROR, "App not initialized");
           return OS_ERROR_GENERIC;
       }
       
       try {
           // Perform operation
           return OS_OK;
       } catch (const std::exception& e) {
           log(ESP_LOG_ERROR, "Operation failed: %s", e.what());
           return OS_ERROR_GENERIC;
       }
   }
   ```

3. **Logging**
   ```cpp
   // Use appropriate log levels
   log(ESP_LOG_DEBUG, "Debug information");    // Development only
   log(ESP_LOG_INFO, "General information");   // Normal operation
   log(ESP_LOG_WARN, "Warning condition");     // Potential issues
   log(ESP_LOG_ERROR, "Error occurred");       // Errors
   ```

## Troubleshooting

### Common Issues

1. **Memory Leaks**
   ```bash
   # Monitor memory usage
   pio device monitor --filter esp32_exception_decoder
   
   # Check heap usage
   ESP_LOGI("HEAP", "Free heap: %d bytes", esp_get_free_heap_size());
   ```

2. **UI Not Displaying**
   ```cpp
   // Debug UI creation
   void debugUI() {
       ESP_LOGI("UI", "Container: %p", m_uiContainer);
       ESP_LOGI("UI", "Parent: %p", lv_obj_get_parent(m_uiContainer));
       ESP_LOGI("UI", "Size: %dx%d", lv_obj_get_width(m_uiContainer), 
                lv_obj_get_height(m_uiContainer));
   }
   ```

3. **App Crashes**
   ```cpp
   // Add safety checks
   os_error_t safeOperation() {
       if (!m_initialized) {
           ESP_LOGE("APP", "Not initialized");
           return OS_ERROR_GENERIC;
       }
       
       if (!m_uiContainer) {
           ESP_LOGE("APP", "No UI container");
           return OS_ERROR_GENERIC;
       }
       
       // Perform operation safely
       return OS_OK;
   }
   ```

### Debug Tools

1. **Memory Debugging**
   ```cpp
   #ifdef DEBUG_MEMORY
   void* debug_malloc(size_t size, const char* file, int line) {
       void* ptr = malloc(size);
       ESP_LOGI("MEM", "Alloc %p (%d bytes) at %s:%d", ptr, size, file, line);
       return ptr;
   }
   
   #define malloc(size) debug_malloc(size, __FILE__, __LINE__)
   #endif
   ```

2. **Performance Profiling**
   ```cpp
   class PerformanceTimer {
       uint64_t start_time;
   public:
       PerformanceTimer() : start_time(esp_timer_get_time()) {}
       ~PerformanceTimer() {
           uint64_t duration = esp_timer_get_time() - start_time;
           ESP_LOGI("PERF", "Operation took %llu microseconds", duration);
       }
   };
   
   // Usage
   void expensiveOperation() {
       PerformanceTimer timer;
       // Operation code here
   }
   ```

### Support Resources

- **Documentation**: [M5Stack Tab5 OS Docs](https://github.com/Paqurin/m5tab5-lvgl/docs)
- **Examples**: [App Examples Repository](https://github.com/Paqurin/m5tab5-apps)
- **Community**: [Developer Forum](https://forum.m5stack.com/category/tab5)
- **Issue Tracker**: [GitHub Issues](https://github.com/Paqurin/m5tab5-lvgl/issues)

---

This standard provides a comprehensive foundation for developing applications for the M5Stack Tab5 OS. Follow these guidelines to ensure your apps are compatible, efficient, and provide a great user experience.