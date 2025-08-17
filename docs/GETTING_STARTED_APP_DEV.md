# Getting Started with M5Stack Tab5 OS App Development

## Table of Contents

1. [Quick Start](#quick-start)
2. [Development Environment Setup](#development-environment-setup)
3. [Your First App](#your-first-app)
4. [Building and Testing](#building-and-testing)
5. [App Packaging](#app-packaging)
6. [Publishing](#publishing)
7. [Common Patterns](#common-patterns)
8. [Troubleshooting](#troubleshooting)
9. [Next Steps](#next-steps)

## Quick Start

### Prerequisites

- C++ development experience
- Basic LVGL knowledge (recommended)
- Git installed
- PlatformIO IDE or CLI

### 5-Minute App

1. **Clone the OS repository**
   ```bash
   git clone https://github.com/Paqurin/m5tab5-lvgl.git
   cd m5tab5-lvgl
   ```

2. **Copy the basic template**
   ```bash
   cp -r templates/basic_app src/apps/my_first_app
   ```

3. **Customize your app**
   ```bash
   cd src/apps/my_first_app
   # Edit my_app.h and my_app.cpp
   # Update manifest.json with your app details
   ```

4. **Register your app**
   ```cpp
   // Add to src/apps/app_integration.cpp
   result = appManager.registerApp("my_first_app", createMyApp);
   ```

5. **Build and test**
   ```bash
   pio run
   ```

## Development Environment Setup

### Required Software

1. **PlatformIO**
   ```bash
   # Install via pip
   pip install platformio
   
   # Or install PlatformIO IDE
   # https://platformio.org/platformio-ide
   ```

2. **Git**
   ```bash
   # Ubuntu/Debian
   sudo apt install git
   
   # macOS
   brew install git
   
   # Windows
   # Download from https://git-scm.com/
   ```

3. **Code Editor** (choose one)
   - PlatformIO IDE (recommended)
   - Visual Studio Code + PlatformIO extension
   - CLion + PlatformIO plugin
   - Any C++ IDE

### Project Setup

1. **Fork the repository**
   ```bash
   # Fork on GitHub, then clone your fork
   git clone https://github.com/YOUR_USERNAME/m5tab5-lvgl.git
   cd m5tab5-lvgl
   ```

2. **Create development branch**
   ```bash
   git checkout -b feature/my-app-development
   ```

3. **Build base system**
   ```bash
   pio run
   ```

4. **Verify build**
   ```bash
   # Check build output for errors
   # Verify memory usage is within limits
   ```

### Development Tools

1. **Code Completion**
   ```bash
   # Generate compile database for better IDE support
   pio run --target compiledb
   ```

2. **Static Analysis** (optional)
   ```bash
   # Install clang-tidy
   sudo apt install clang-tidy
   
   # Run analysis
   clang-tidy src/apps/your_app/*.cpp
   ```

3. **Memory Debugging** (optional)
   ```bash
   # Enable in platformio.ini
   build_flags = -DDEBUG_MEMORY
   ```

## Your First App

### Step 1: Create App Structure

```bash
mkdir src/apps/hello_world
cd src/apps/hello_world
```

### Step 2: Create Header File (hello_world.h)

```cpp
#ifndef HELLO_WORLD_H
#define HELLO_WORLD_H

#include "../base_app.h"

class HelloWorldApp : public BaseApp {
public:
    HelloWorldApp();
    ~HelloWorldApp() override = default;

    os_error_t initialize() override;
    os_error_t update(uint32_t deltaTime) override;
    os_error_t shutdown() override;
    os_error_t createUI(lv_obj_t* parent) override;
    os_error_t destroyUI() override;

private:
    void createGreeting();
    static void buttonCallback(lv_event_t* e);
    
    lv_obj_t* m_greetingLabel = nullptr;
    lv_obj_t* m_button = nullptr;
    bool m_showingHello = true;
};

extern "C" std::unique_ptr<BaseApp> createHelloWorldApp();

#endif
```

### Step 3: Create Implementation (hello_world.cpp)

```cpp
#include "hello_world.h"
#include "../../system/os_manager.h"

HelloWorldApp::HelloWorldApp() 
    : BaseApp("hello_world", "Hello World", "1.0.0") {
    setDescription("My first M5Stack Tab5 app");
    setAuthor("Your Name");
    setPriority(AppPriority::APP_NORMAL);
}

os_error_t HelloWorldApp::initialize() {
    if (m_initialized) return OS_OK;
    
    setMemoryUsage(2048); // 2KB
    m_initialized = true;
    return OS_OK;
}

os_error_t HelloWorldApp::update(uint32_t deltaTime) {
    return OS_OK;
}

os_error_t HelloWorldApp::shutdown() {
    m_initialized = false;
    return OS_OK;
}

os_error_t HelloWorldApp::createUI(lv_obj_t* parent) {
    m_uiContainer = lv_obj_create(parent);
    lv_obj_set_size(m_uiContainer, LV_HOR_RES, 
                    LV_VER_RES - OS_STATUS_BAR_HEIGHT - OS_DOCK_HEIGHT);
    lv_obj_align(m_uiContainer, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(m_uiContainer, lv_color_hex(0x1E1E1E), 0);
    
    createGreeting();
    return OS_OK;
}

os_error_t HelloWorldApp::destroyUI() {
    if (m_uiContainer) {
        lv_obj_del(m_uiContainer);
        m_uiContainer = nullptr;
        m_greetingLabel = nullptr;
        m_button = nullptr;
    }
    return OS_OK;
}

void HelloWorldApp::createGreeting() {
    // Create greeting label
    m_greetingLabel = lv_label_create(m_uiContainer);
    lv_label_set_text(m_greetingLabel, "Hello, M5Stack Tab5!");
    lv_obj_set_style_text_color(m_greetingLabel, lv_color_white(), 0);
    lv_obj_set_style_text_font(m_greetingLabel, &lv_font_montserrat_24, 0);
    lv_obj_center(m_greetingLabel);
    
    // Create toggle button
    m_button = lv_btn_create(m_uiContainer);
    lv_obj_set_size(m_button, 120, 40);
    lv_obj_align(m_button, LV_ALIGN_BOTTOM_MID, 0, -50);
    
    lv_obj_t* btnLabel = lv_label_create(m_button);
    lv_label_set_text(btnLabel, "Toggle");
    lv_obj_center(btnLabel);
    
    lv_obj_add_event_cb(m_button, buttonCallback, LV_EVENT_CLICKED, this);
}

void HelloWorldApp::buttonCallback(lv_event_t* e) {
    HelloWorldApp* app = static_cast<HelloWorldApp*>(lv_event_get_user_data(e));
    if (app) {
        app->m_showingHello = !app->m_showingHello;
        const char* text = app->m_showingHello ? 
            "Hello, M5Stack Tab5!" : "Goodbye, World!";
        lv_label_set_text(app->m_greetingLabel, text);
    }
}

extern "C" std::unique_ptr<BaseApp> createHelloWorldApp() {
    return std::make_unique<HelloWorldApp>();
}
```

### Step 4: Create Manifest (manifest.json)

```json
{
    "app": {
        "id": "com.yourname.helloworld",
        "name": "Hello World",
        "version": "1.0.0",
        "description": "My first M5Stack Tab5 application",
        "author": "Your Name",
        "category": "Example"
    },
    "system": {
        "min_os_version": "1.0.0",
        "target_platform": "m5stack-tab5"
    },
    "requirements": {
        "memory": {
            "ram": 2048,
            "flash": 16384
        },
        "permissions": []
    }
}
```

### Step 5: Register App

Add to `src/apps/app_integration.cpp`:

```cpp
// In registerAllApps() function
result = appManager.registerApp("hello_world", createHelloWorldApp);
if (result != OS_OK) {
    ESP_LOGW(TAG, "Failed to register hello world app: %d", result);
} else {
    ESP_LOGI(TAG, "Registered Hello World App");
}
```

Add factory function declaration to `app_integration.h`:

```cpp
static std::unique_ptr<BaseApp> createHelloWorldApp();
```

## Building and Testing

### Build Your App

```bash
# Clean build
pio run --target clean

# Build with verbose output
pio run --verbose

# Check for warnings
pio check
```

### Test on Device

```bash
# Upload to device (if connected)
pio run --target upload

# Monitor serial output
pio device monitor
```

### Test in Simulator

```bash
# Build for native platform (if configured)
pio run --environment native

# Run simulator
./.pio/build/native/program
```

### Debug Common Issues

1. **Compilation Errors**
   ```bash
   # Check include paths
   pio run --target compiledb
   
   # Verify all files are included in CMakeLists.txt
   ```

2. **Memory Issues**
   ```bash
   # Check memory usage in build output
   # Verify setMemoryUsage() is called correctly
   ```

3. **UI Not Showing**
   ```cpp
   // Add debug prints
   ESP_LOGI("UI", "Creating UI container");
   ESP_LOGI("UI", "Container created: %p", m_uiContainer);
   ```

## App Packaging

### Create Package Structure

```bash
mkdir my_app_package
cd my_app_package

# Copy source files
mkdir src
cp -r ../src/apps/hello_world src/

# Copy assets
mkdir assets
# Add icon.png (64x64)
# Add screenshots

# Copy manifest
cp src/hello_world/manifest.json .
```

### Create Package Archive

```bash
# Create .m5app package (ZIP format)
zip -r hello_world-v1.0.0.m5app ./*
```

### Validate Package

```bash
# Validate package format (if validator available)
m5app-validator hello_world-v1.0.0.m5app

# Manual validation checklist:
# ✓ manifest.json is valid
# ✓ All source files included
# ✓ Icon is 64x64 PNG
# ✓ Memory requirements are reasonable
# ✓ No malicious code
```

## Publishing

### Prepare for Publication

1. **Code Review Checklist**
   - [ ] No hardcoded secrets or passwords
   - [ ] Proper error handling
   - [ ] Memory usage within limits
   - [ ] UI follows design guidelines
   - [ ] Code is well-documented

2. **Testing Checklist**
   - [ ] App builds without warnings
   - [ ] UI displays correctly
   - [ ] No memory leaks
   - [ ] Graceful error handling
   - [ ] Works with other apps

3. **Documentation**
   - [ ] README.md with usage instructions
   - [ ] Screenshots of app in action
   - [ ] Changelog for version history
   - [ ] License file

### Submit to App Store

1. **Create GitHub Repository**
   ```bash
   git init
   git add .
   git commit -m "Initial app version"
   git remote add origin https://github.com/yourusername/hello-world-m5app.git
   git push -u origin main
   ```

2. **Create Release**
   ```bash
   git tag v1.0.0
   git push origin v1.0.0
   ```

3. **Submit Package**
   - Upload .m5app file to release
   - Submit to M5Stack Tab5 App Store (when available)
   - Share on community forums

## Common Patterns

### Data Storage

```cpp
// Save app data
os_error_t HelloWorldApp::saveData() {
    std::string dataPath = getAppDataPath() + "/data.json";
    // Implement JSON serialization
    return OS_OK;
}

// Load app data  
os_error_t HelloWorldApp::loadData() {
    std::string dataPath = getAppDataPath() + "/data.json";
    // Implement JSON deserialization
    return OS_OK;
}
```

### Settings Dialog

```cpp
void HelloWorldApp::createSettingsDialog() {
    lv_obj_t* dialog = lv_obj_create(lv_scr_act());
    lv_obj_set_size(dialog, 400, 300);
    lv_obj_center(dialog);
    
    // Add settings controls
    lv_obj_t* closeBtn = lv_btn_create(dialog);
    // Configure close button
}
```

### Background Task

```cpp
static void backgroundTask(void* parameter) {
    HelloWorldApp* app = static_cast<HelloWorldApp*>(parameter);
    
    while (app->isTaskRunning()) {
        // Background processing
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    vTaskDelete(nullptr);
}
```

### Network Request

```cpp
os_error_t HelloWorldApp::fetchData() {
    // Use system HTTP client
    HTTPClient client;
    client.begin("https://api.example.com/data");
    
    int httpCode = client.GET();
    if (httpCode == 200) {
        String response = client.getString();
        // Process response
    }
    
    client.end();
    return OS_OK;
}
```

## Troubleshooting

### Build Issues

**Problem**: "BaseApp not found"
```bash
# Solution: Check include path
#include "../base_app.h"  # If in subdirectory
#include "apps/base_app.h"  # If in apps directory
```

**Problem**: LVGL functions not found
```bash
# Solution: Include LVGL header
#include <lvgl.h>
```

**Problem**: Memory allocation fails
```cpp
// Solution: Check available memory
size_t free = esp_get_free_heap_size();
ESP_LOGI("MEM", "Free memory: %d bytes", free);
```

### Runtime Issues

**Problem**: App doesn't appear in launcher
```cpp
// Solution: Verify registration
ESP_LOGI("APP", "Registering app...");
os_error_t result = appManager.registerApp("my_app", createMyApp);
ESP_LOGI("APP", "Registration result: %d", result);
```

**Problem**: UI elements not visible
```cpp
// Solution: Check container hierarchy
ESP_LOGI("UI", "Parent: %p, Container: %p", parent, m_uiContainer);
ESP_LOGI("UI", "Container size: %dx%d", 
         lv_obj_get_width(m_uiContainer), 
         lv_obj_get_height(m_uiContainer));
```

**Problem**: App crashes on startup
```cpp
// Solution: Add safety checks
if (!m_initialized) {
    ESP_LOGE("APP", "App not initialized");
    return OS_ERROR_GENERIC;
}
```

### Memory Issues

**Problem**: Out of memory
```cpp
// Solution: Monitor memory usage
void checkMemory() {
    size_t free = esp_get_free_heap_size();
    size_t total = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);
    ESP_LOGI("MEM", "Memory: %d/%d bytes free", free, total);
}
```

## Next Steps

### Learn Advanced Features

1. **Study Example Apps**
   - Calendar app (complex UI)
   - Terminal app (networking)
   - Advanced template (services)

2. **Explore System APIs**
   - Event system
   - Storage services
   - Network stack
   - Hardware abstraction

3. **Advanced UI Patterns**
   - Custom widgets
   - Animations
   - Themes
   - Touch gestures

### Join the Community

1. **GitHub Discussions**
   - Share your apps
   - Ask questions
   - Report bugs

2. **Documentation**
   - Contribute to docs
   - Write tutorials
   - Create examples

3. **Code Contributions**
   - Fix bugs
   - Add features
   - Improve performance

### Resources

- **API Reference**: [docs/API_REFERENCE.md](API_REFERENCE.md)
- **App Standard**: [docs/APP_DEVELOPMENT_STANDARD.md](APP_DEVELOPMENT_STANDARD.md)
- **Quick Reference**: [docs/APP_QUICK_REFERENCE.md](APP_QUICK_REFERENCE.md)
- **Examples**: [templates/](../templates/)
- **Community**: [GitHub Discussions](https://github.com/Paqurin/m5tab5-lvgl/discussions)

Happy coding! 🚀