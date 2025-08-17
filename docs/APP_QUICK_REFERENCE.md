# M5Stack Tab5 OS - App Development Quick Reference

## Essential Information

### System Constants
```cpp
// Display
#define LV_HOR_RES                1280
#define LV_VER_RES                720
#define OS_STATUS_BAR_HEIGHT      30
#define OS_DOCK_HEIGHT            50

// Memory Limits
#define MAX_APP_MEMORY            (1024 * 1024)  // 1MB
#define MAX_UI_OBJECTS            100
#define MAX_BACKGROUND_TASKS      4

// Error Codes
#define OS_OK                     0
#define OS_ERROR_GENERIC          -1
#define OS_ERROR_NO_MEMORY        -2
#define OS_ERROR_INVALID_PARAM    -3
#define OS_ERROR_NOT_FOUND        -4
```

### Required Includes
```cpp
#include "apps/base_app.h"
#include "system/os_manager.h"
#include <lvgl.h>
#include <esp_log.h>
```

## Minimal App Implementation

### Header File
```cpp
#ifndef MY_APP_H
#define MY_APP_H

#include "apps/base_app.h"

class MyApp : public BaseApp {
public:
    MyApp();
    ~MyApp() override = default;

    os_error_t initialize() override;
    os_error_t update(uint32_t deltaTime) override;
    os_error_t shutdown() override;
    os_error_t createUI(lv_obj_t* parent) override;
    os_error_t destroyUI() override;
};

extern "C" std::unique_ptr<BaseApp> createMyApp();

#endif
```

### Implementation File
```cpp
#include "my_app.h"

MyApp::MyApp() : BaseApp("my_app_id", "My App", "1.0.0") {
    setDescription("My application description");
    setAuthor("Developer Name");
    setPriority(AppPriority::APP_NORMAL);
}

os_error_t MyApp::initialize() {
    if (m_initialized) return OS_OK;
    setMemoryUsage(4096); // Set estimated memory usage
    m_initialized = true;
    return OS_OK;
}

os_error_t MyApp::update(uint32_t deltaTime) {
    return OS_OK;
}

os_error_t MyApp::shutdown() {
    m_initialized = false;
    return OS_OK;
}

os_error_t MyApp::createUI(lv_obj_t* parent) {
    m_uiContainer = lv_obj_create(parent);
    lv_obj_set_size(m_uiContainer, LV_HOR_RES, 
                    LV_VER_RES - OS_STATUS_BAR_HEIGHT - OS_DOCK_HEIGHT);
    lv_obj_align(m_uiContainer, LV_ALIGN_CENTER, 0, 0);
    return OS_OK;
}

os_error_t MyApp::destroyUI() {
    if (m_uiContainer) {
        lv_obj_del(m_uiContainer);
        m_uiContainer = nullptr;
    }
    return OS_OK;
}

extern "C" std::unique_ptr<BaseApp> createMyApp() {
    return std::make_unique<MyApp>();
}
```

## Common UI Patterns

### Button with Callback
```cpp
static void buttonCallback(lv_event_t* e) {
    MyApp* app = static_cast<MyApp*>(lv_event_get_user_data(e));
    if (app) app->handleButtonClick();
}

lv_obj_t* btn = lv_btn_create(parent);
lv_obj_set_size(btn, 120, 40);
lv_obj_add_event_cb(btn, buttonCallback, LV_EVENT_CLICKED, this);

lv_obj_t* label = lv_label_create(btn);
lv_label_set_text(label, "Click Me");
lv_obj_center(label);
```

### List with Items
```cpp
lv_obj_t* list = lv_list_create(parent);
lv_obj_set_size(list, 300, 400);

for (int i = 0; i < itemCount; i++) {
    lv_obj_t* item = lv_list_add_btn(list, LV_SYMBOL_FILE, items[i].c_str());
    lv_obj_set_user_data(item, &items[i]);
    lv_obj_add_event_cb(item, itemCallback, LV_EVENT_CLICKED, this);
}
```

### Text Input
```cpp
lv_obj_t* textarea = lv_textarea_create(parent);
lv_obj_set_size(textarea, 250, 40);
lv_textarea_set_placeholder_text(textarea, "Enter text...");
lv_obj_add_event_cb(textarea, textCallback, LV_EVENT_VALUE_CHANGED, this);
```

### Progress Bar
```cpp
lv_obj_t* bar = lv_bar_create(parent);
lv_obj_set_size(bar, 200, 20);
lv_bar_set_range(bar, 0, 100);
lv_bar_set_value(bar, 50, LV_ANIM_ON);
```

## Event System

### Publishing Events
```cpp
PUBLISH_EVENT(EVENT_TYPE, data, dataSize);
```

### Subscribing to Events
```cpp
// In initialize()
SUBSCRIBE_EVENT(EVENT_TYPE, [this](const EventData& event) {
    handleEvent(event);
});
```

### Common Event Types
```cpp
#define EVENT_APP_LAUNCH        0x1000
#define EVENT_APP_EXIT          0x1001
#define EVENT_TOUCH_PRESS       0x2000
#define EVENT_TOUCH_RELEASE     0x2001
#define EVENT_NETWORK_CONNECTED 0x3000
```

## Memory Management

### Safe Allocation
```cpp
void* ptr = heap_caps_malloc(size, MALLOC_CAP_DEFAULT);
if (!ptr) {
    log(ESP_LOG_ERROR, "Memory allocation failed");
    return OS_ERROR_NO_MEMORY;
}
```

### Tracking Usage
```cpp
void updateMemoryUsage() {
    size_t currentUsage = getCurrentMemoryUsage();
    setMemoryUsage(currentUsage);
}
```

## File Operations

### Read File
```cpp
std::string readFile(const std::string& path) {
    FILE* file = fopen(path.c_str(), "r");
    if (!file) return "";
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    std::string content(size, '\0');
    fread(&content[0], 1, size, file);
    fclose(file);
    
    return content;
}
```

### Write File
```cpp
bool writeFile(const std::string& path, const std::string& content) {
    FILE* file = fopen(path.c_str(), "w");
    if (!file) return false;
    
    fwrite(content.c_str(), 1, content.length(), file);
    fclose(file);
    return true;
}
```

## Background Tasks

### Create Task
```cpp
TaskHandle_t taskHandle;
xTaskCreate(backgroundTask, "bg_task", 4096, this, 
            tskIDLE_PRIORITY + 1, &taskHandle);
```

### Task Function
```cpp
static void backgroundTask(void* parameter) {
    MyApp* app = static_cast<MyApp*>(parameter);
    
    while (app->isTaskRunning()) {
        app->processBackgroundWork();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    vTaskDelete(nullptr);
}
```

## Logging

### Log Levels
```cpp
log(ESP_LOG_DEBUG, "Debug message");
log(ESP_LOG_INFO, "Info message");
log(ESP_LOG_WARN, "Warning message");
log(ESP_LOG_ERROR, "Error message");
```

## Theme Colors

### Standard Colors
```cpp
lv_color_t primary = lv_color_hex(0x3498DB);      // Blue
lv_color_t secondary = lv_color_hex(0x2ECC71);    // Green
lv_color_t accent = lv_color_hex(0xE74C3C);       // Red
lv_color_t background = lv_color_hex(0x1E1E1E);   // Dark gray
lv_color_t surface = lv_color_hex(0x2C2C2C);      // Medium gray
lv_color_t text = lv_color_white();               // White
```

## Manifest Template

### Basic Manifest
```json
{
    "app": {
        "id": "com.company.appname",
        "name": "App Name",
        "version": "1.0.0",
        "description": "App description",
        "author": "Developer Name",
        "category": "Productivity"
    },
    "system": {
        "min_os_version": "1.0.0",
        "target_platform": "m5stack-tab5"
    },
    "requirements": {
        "memory": {
            "ram": 65536,
            "flash": 262144
        },
        "permissions": ["STORAGE_READ", "STORAGE_WRITE"]
    },
    "resources": {
        "icon": "assets/icon.png"
    }
}
```

## Build Integration

### Add to CMakeLists.txt
```cmake
# App source files are automatically included via GLOB_RECURSE
```

### Register App
```cpp
// In app_integration.cpp
result = appManager.registerApp("my_app_id", createMyApp);
```

## Common Patterns

### Singleton Service
```cpp
class MyService {
public:
    static MyService& getInstance() {
        static MyService instance;
        return instance;
    }
    
private:
    MyService() = default;
};
```

### State Machine
```cpp
enum class AppState { IDLE, WORKING, ERROR };

void updateState(AppState newState) {
    if (m_state == newState) return;
    
    exitState(m_state);
    m_state = newState;
    enterState(m_state);
}
```

### Configuration Manager
```cpp
class Config {
    std::map<std::string, std::string> m_values;
    
public:
    void load(const std::string& path);
    void save(const std::string& path);
    std::string get(const std::string& key, const std::string& defaultValue = "");
    void set(const std::string& key, const std::string& value);
};
```

## Testing

### Basic Test
```cpp
void testAppInitialization() {
    MyApp app;
    TEST_ASSERT_EQUAL(OS_OK, app.initialize());
    TEST_ASSERT_TRUE(app.isInitialized());
    app.shutdown();
}
```

### Mock Objects
```cpp
class MockService : public BaseService {
public:
    MOCK_METHOD(os_error_t, initialize, (), (override));
    MOCK_METHOD(os_error_t, shutdown, (), (override));
};
```

## Performance Tips

1. **Batch UI Updates**: Hide container, make changes, show container
2. **Use Object Pools**: For frequently created/destroyed objects
3. **Yield in Loops**: Call `vTaskDelay(1)` in long operations
4. **Monitor Memory**: Track allocations and deallocations
5. **Optimize Images**: Use appropriate formats and sizes

## Debug Helpers

### Memory Checker
```cpp
void checkMemory() {
    size_t free = esp_get_free_heap_size();
    size_t min_free = esp_get_minimum_free_heap_size();
    ESP_LOGI("MEM", "Free: %d, Min Free: %d", free, min_free);
}
```

### UI Tree Dumper
```cpp
void dumpUITree(lv_obj_t* obj, int depth = 0) {
    for (int i = 0; i < depth; i++) printf("  ");
    printf("Object: %p, Type: %s\n", obj, lv_obj_get_class(obj)->name);
    
    for (uint32_t i = 0; i < lv_obj_get_child_cnt(obj); i++) {
        dumpUITree(lv_obj_get_child(obj, i), depth + 1);
    }
}
```