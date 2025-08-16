# M5Stack Tab5 LVGL Demo Project

A PlatformIO project demonstrating LVGL graphics library on the M5Stack Tab5 ESP32-P4 device with Wokwi simulation support.

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

## Project Features

- ✅ ESP32-P4 RISC-V architecture support
- ✅ Arduino framework compatibility  
- ✅ LVGL 8.x graphics library integration
- ✅ 1280×720 HD display configuration
- ✅ Wokwi simulation setup
- ✅ Memory-optimized display buffer
- ✅ Touch input ready (hardware-dependent)

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

## Wokwi Simulation

The project includes Wokwi simulation files for development and testing:

- **wokwi.toml** - Simulation configuration
- **diagram.json** - Hardware simulation diagram

### Running Simulation
1. Open [Wokwi](https://wokwi.com)
2. Import the project files
3. Run the simulation

**Note**: Wokwi simulation uses ESP32-S3 + ILI9341 (320×240) as ESP32-P4 and 5-inch MIPI-DSI displays are not available in Wokwi.

## File Structure

```
├── src/
│   └── main.cpp           # Main application with LVGL demo
├── include/
│   └── lv_conf.h          # LVGL configuration
├── platformio.ini         # PlatformIO configuration
├── wokwi.toml            # Wokwi simulation config
├── diagram.json          # Wokwi hardware diagram
├── README_M5TAB5.md      # Hardware specifications
└── README.md             # This file
```

## Development Notes

### Display Configuration
The project is configured for the actual M5Tab5 hardware:
- Resolution: 1280×720 (5-inch HD display)
- Color depth: 16-bit (RGB565)
- Buffer: 10 lines (12,800 pixels)

### LVGL Configuration
Key LVGL settings in `include/lv_conf.h`:
- `LV_COLOR_DEPTH 16` - 16-bit color
- Memory and performance optimizations for ESP32-P4

### Arduino Framework
Uses Arduino-ESP32 framework for easier development and library compatibility.

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