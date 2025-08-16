# M5Stack Tab5 LVGL test os

A comprehensive operating system framework for the M5Stack Tab5 ESP32-P4 device built with LVGL graphics library and modular architecture.

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

## Operating System Features

- ✅ **Modular OS Architecture** - HAL, System, UI, Apps, Services layers
- ✅ **Application Management** - Dynamic app loading and lifecycle control
- ✅ **Event System** - Inter-component communication framework
- ✅ **Memory Management** - Optimized memory allocation and monitoring
- ✅ **Task Scheduling** - Multi-tasking support with priority management
- ✅ **HAL Abstraction** - Display, Touch, Storage, Power management
- ✅ **UI Framework** - Theme management and screen handling
- ✅ **LVGL Integration** - 1280×720 HD graphics with 16-bit color
- ✅ **ESP32-P4 Support** - RISC-V dual-core architecture optimization

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
- **App Manager** - Dynamic application loading
- **Base App** - Application framework and lifecycle
- **Demo App** - Built-in system information display

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
│   ├── apps/              # Applications
│   │   ├── app_manager.*  # App lifecycle
│   │   └── base_app.*     # App framework
│   └── services/          # Background services
│       └── service_manager.* # Service management
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