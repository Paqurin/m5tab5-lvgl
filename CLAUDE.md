# M5Stack Tab5 LVGL Operating System

## Project Overview
A comprehensive operating system for the M5Stack Tab5 device built on ESP32-P4 with LVGL GUI framework, featuring extensive application suite and accessibility features.

## Architecture
- **Hardware**: ESP32-P4 with 32MB PSRAM, 16MB Flash, USB OTG, dual microphones, camera support
- **Framework**: Arduino + LVGL 8.3.11
- **Display**: High-resolution with 60Hz refresh rate capability
- **Audio**: Dual microphone support with PDM interface
- **Connectivity**: USB-C OTG, RS485 terminal support

## Key Systems
- **UI Manager**: Theme management, screen transitions, input handling
- **App Manager**: Modular application framework with lifecycle management
- **HAL Layer**: Hardware abstraction for display, touch, audio, storage, power
- **Service Layer**: USB OTG, audio processing, storage management
- **Memory Manager**: PSRAM optimization for large applications

## Applications
- Basic Apps Suite (calculator, games, spreadsheet, expense tracker)
- Contact Management with search and categorization
- Alarm/Timer with multiple alarms and stopwatch
- File Manager with USB OTG support
- Camera application with image capture
- Voice Recognition with dual microphone support
- Task Management for productivity
- MicroPython Launcher for scripting
- RS485 Terminal for industrial communication
- App Store Server integration

## Build Commands
```bash
# Build project
pio run

# Upload to device
pio run --target upload

# Monitor serial output
pio device monitor

# Clean build
pio run --target clean

# Run tests
pio test
```

## Performance Optimization
- Memory-mapped PSRAM usage
- Function/data section optimization (-ffunction-sections, -fdata-sections)
- Dead code elimination (--gc-sections)
- LVGL GPU acceleration disabled for stability
- Custom memory management for large datasets

## Accessibility Features
- High contrast mode for visual impairment
- Voice feedback system (Talkback style)
- Talkback functionality for blind users
- Large touch targets and clear navigation

## Development Notes
- Use `pio run` to build and test changes
- Monitor memory usage with PSRAM_SIZE_MB=32 configuration
- LVGL configuration in include/lv_conf.h
- All HAL interfaces designed for easy hardware abstraction
- Modular app architecture allows easy addition of new applications