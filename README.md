# M5Stack Tab5 - Complete Operating System v4

A comprehensive personal assistant and productivity operating system for the M5Stack Tab5 ESP32-P4 device featuring 14 professional applications, AI integration, and enterprise-grade functionality.

📋 **[View Complete Application List](APPLICATIONS.md)** - Detailed features for all 14 applications

## Hardware Specifications

### M5Stack Tab5
- **MCU**: ESP32-P4 (RISC-V dual-core @ 360MHz)
- **Display**: 5-inch IPS TFT, 1280×720 resolution (HD)
- **Interface**: MIPI-DSI high-speed display
- **Touch**: GT911 multi-touch capacitive controller
- **Memory**: 500KB internal SRAM + PSRAM support
- **Flash**: 16MB
- **Connectivity**: WiFi 6 (802.11ax), Bluetooth 5.0
- **Power**: USB-C

## 🏪 App Store Server

The M5Stack Tab5 now includes a complete app store server for distributing and managing applications:

- **Web-Based App Store** - Modern HTML5 interface for browsing and downloading applications
- **Docker Deployment** - One-command setup with `./docker-commands.sh setup`
- **Package Management** - Automated .m5app package generation and distribution
- **Health Monitoring** - Built-in health checks and status monitoring
- **Production Ready** - Docker Compose with nginx, SSL support, and reverse proxy compatibility

### Quick App Store Setup
```bash
cd app-store-server
./docker-commands.sh setup
# Access at http://localhost:8080
```

📚 **[View App Store Documentation](app-store-server/DOCKER_QUICK_START.md)** - Complete deployment guide

## 🚀 Version 4 - Personal Assistant Suite

### 📱 Complete Application Ecosystem (14 Apps)
- 📞 **Contact Management** - Full address book with search, categories, VCard support
- ✅ **Task Management** - Smart to-do lists with priorities, due dates, progress tracking  
- 🧮 **Basic Apps Suite** - Expense tracker, calculator, spreadsheet, games
- 🎤 **Voice Assistant** - ChatGPT integration, multi-language, voice commands
- ⏰ **Alarm & Timer** - Smart alarms, multiple timers, stopwatch, world clock
- 📁 **File Manager** - Multi-storage support, operations, analytics
- 📷 **Camera** - Photo/video capture, gallery, QR scanner, editing
- 📅 **Calendar** - Event management, reminders, recurring events, iCal support
- 💻 **Enhanced Terminal** - RS-485, Telnet, SSH with multiple sessions
- 🔧 **RS-485 Terminal** - Industrial Modbus RTU/ASCII, device scanning
- 📱 **Modular Apps** - APK-style dynamic loading with integrated app store server
- 🔗 **App Integration** - Inter-app communication, shared services

### 🤖 AI & Smart Features
- **ChatGPT Integration** - Voice assistant with natural language processing
- **Multi-Language Support** - 6 languages (EN, ES, FR, DE, ZH, JA)
- **Voice Commands** - System control via voice ("open calculator", "check battery")
- **Smart Scheduling** - Intelligent task and alarm management
- **Industrial IoT** - RS-485 Modbus communication for industrial applications

### 🛠️ Enterprise-Grade Architecture
- ✅ **Modular OS Architecture** - HAL, System, UI, Apps, Services layers
- ✅ **Dynamic App Loading** - APK-style installation with dependency management
- ✅ **App Store Integration** - Docker-based server for app distribution and updates
- ✅ **Inter-App Communication** - Shared data and service APIs
- ✅ **Memory Optimization** - 16MB Flash + 32MB PSRAM efficient usage
- ✅ **Real-Time Performance** - RISC-V dual-core 360MHz optimization
- ✅ **Industrial Connectivity** - RS-485, Modbus, Telnet, SSH protocols
- ✅ **Data Security** - Encrypted storage, secure API key management
- ✅ **LVGL 8.4 Graphics** - Hardware-accelerated 1280×720 HD display

## Build Configuration

### PlatformIO Environment
```ini
[env:esp32-p4-evboard]
platform = espressif32
board = esp32-p4-evboard
framework = arduino
lib_deps = lvgl/lvgl@^8.3.11
```

### Memory Usage
- **RAM**: 17.3% (88,752 / 512,000 bytes)
- **Flash**: 40.0% (524,909 / 1,310,720 bytes)

## Prerequisites

### Toolchain Requirements
The ESP32-P4 RISC-V architecture requires a specific toolchain:

```bash
# ESP-IDF RISC-V toolchain (required for ESP32-P4)
export PATH="/home/paqurin/.espressif/tools/riscv32-esp-elf/esp-15.1.0_20250607/riscv32-esp-elf/bin:$PATH"
```

### PlatformIO Setup
```bash
# Install PlatformIO
pip install platformio

# Build the project
pio run -e esp32-p4-evboard
```

## OS Architecture

The operating system follows a layered modular architecture:

### System Layer (`src/system/`)
- **OS Manager** - Core system initialization and coordination
- **Event System** - Publish/subscribe event handling
- **Memory Manager** - Dynamic memory allocation and monitoring
- **Task Scheduler** - Multi-tasking and priority management

### HAL Layer (`src/hal/`)
- **Display HAL** - MIPI-DSI display management
- **Touch HAL** - GT911 multi-touch input handling
- **Storage HAL** - Flash and SD card file systems
- **Power HAL** - Battery and power management

### UI Layer (`src/ui/`)
- **UI Manager** - LVGL integration and screen management
- **Theme Manager** - Dark/light theme switching
- **Input Manager** - Touch input processing
- **Screen Manager** - Multi-screen navigation

### Application Layer (`src/apps/`)
- **App Manager** - Dynamic application loading and lifecycle management
- **Base App Framework** - Standardized application development interface
- **Personal Assistant Apps** - Contact management, task management, voice assistant
- **Productivity Suite** - Calculator, spreadsheet, expense tracker, calendar
- **System Utilities** - File manager, camera, terminal applications
- **Entertainment** - Games, media viewer, interactive demos

### Services Layer (`src/services/`)
- **Service Manager** - Background service management

## File Structure

```
├── src/
│   ├── main.cpp           # Main application entry point
│   ├── system/            # Core OS components
│   │   ├── os_manager.*   # System initialization
│   │   ├── event_system.* # Event handling
│   │   ├── memory_manager.* # Memory management
│   │   └── task_scheduler.* # Task scheduling
│   ├── hal/               # Hardware abstraction
│   │   ├── display_hal.*  # Display management
│   │   ├── touch_hal.*    # Touch input
│   │   ├── storage_hal.*  # File system
│   │   └── power_hal.*    # Power management
│   ├── ui/                # User interface
│   │   ├── ui_manager.*   # LVGL integration
│   │   ├── theme_manager.* # Theme handling
│   │   ├── input_manager.* # Input processing
│   │   └── screen_manager.* # Screen management
│   ├── apps/              # 14 Complete Applications
│   │   ├── contact_management_app.* # Address book with VCard support
│   │   ├── task_management_app.*    # Smart to-do lists with priorities
│   │   ├── voice_recognition_app.*  # ChatGPT voice assistant
│   │   ├── basic_apps_suite.*       # Calculator, expense tracker, games
│   │   ├── alarm_timer_app.*        # Alarms, timers, stopwatch
│   │   ├── file_manager_app.*       # Multi-storage file operations
│   │   ├── camera_app.*             # Photo/video with QR scanner
│   │   ├── calendar_app.*           # Event management with reminders
│   │   ├── enhanced_terminal_app.*  # Telnet, SSH, RS-485 support
│   │   ├── modular_app.*            # APK-style dynamic loading
│   │   ├── app_integration.*        # Inter-app communication
│   │   ├── app_manager.*            # Application lifecycle
│   │   └── base_app.*               # Application framework
│   └── services/          # Background services
│       └── service_manager.* # Service management
├── app-store-server/      # App Store Distribution Server
│   ├── Dockerfile         # Docker container configuration
│   ├── docker-compose.yml # Production deployment setup
│   ├── docker-commands.sh # One-command deployment script
│   ├── index.html         # App store web interface
│   ├── packages/          # Application packages (.m5app files)
│   ├── docs/              # App store documentation
│   └── DOCKER_QUICK_START.md # Quick deployment guide
├── docs/
│   ├── APPLICATIONS.md    # Complete application feature list
│   └── APP_DEVELOPMENT_STANDARD.md # Development guidelines
├── include/
│   └── lv_conf.h          # LVGL configuration
├── platformio.ini         # PlatformIO configuration
└── README.md             # This file
```

## Development Notes

### Operating System Design
The M5Stack Tab5 LVGL test os implements:
- **Singleton Pattern** - Core managers accessible via `OS()` macro
- **Event-Driven Architecture** - Asynchronous communication between components
- **Resource Management** - Automatic cleanup and memory monitoring
- **Modular Design** - Easy to extend with new applications and services

### Display Configuration
Optimized for M5Stack Tab5 hardware:
- Resolution: 1280×720 (5-inch HD display)
- Color depth: 16-bit (RGB565)
- Buffer: 10 lines (12,800 pixels)
- MIPI-DSI interface support

### Application Development
To create new applications:
1. Inherit from `BaseApp` class
2. Implement required lifecycle methods
3. Register with `AppManager`
4. Handle UI creation and events

## Building

### Prerequisites Check
```bash
# Verify RISC-V toolchain
which riscv32-esp-elf-gcc

# Should point to ESP-IDF toolchain location
```

### Build Commands
```bash
# Clean build
pio run -e esp32-p4-evboard -t clean

# Build firmware
export PATH="/path/to/riscv32-esp-elf/bin:$PATH"
pio run -e esp32-p4-evboard

# Upload to device (when connected)
pio run -e esp32-p4-evboard -t upload

# Monitor serial output
pio device monitor
```

## Troubleshooting

### Common Issues

1. **RISC-V Toolchain Not Found**
   ```
   Error: riscv32-esp-elf-gcc: not found
   ```
   Solution: Ensure ESP-IDF RISC-V toolchain is in PATH

2. **Memory Allocation Errors**
   ```
   Failed to allocate display buffer
   ```
   Solution: Reduce buffer size or enable PSRAM

3. **Build Timeouts**
   ```
   Command timed out
   ```
   Solution: Initial builds take longer due to toolchain setup

### ESP32-P4 Specific Notes
- RISC-V architecture requires different toolchain than ESP32/ESP32-S3
- Arduino framework support is still developing for ESP32-P4
- Some libraries may have compatibility issues

## License

This project is provided as-is for educational and development purposes.

## References

- [M5Stack Tab5 Official](https://shop.m5stack.com/products/m5stack-tab5)
- [ESP32-P4 Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32p4/)
- [LVGL Documentation](https://docs.lvgl.io/)
- [PlatformIO ESP32 Platform](https://docs.platformio.org/en/latest/platforms/espressif32.html)