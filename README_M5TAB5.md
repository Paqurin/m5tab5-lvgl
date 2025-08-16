# M5Stack Tab5 Hardware Specifications

## Display
- **Size**: 5-inch IPS TFT
- **Resolution**: 1280 × 720 pixels (HD, 16:9 aspect ratio)
- **Interface**: MIPI-DSI
- **Touch Controller**: GT911 (multi-touch capacitive)

## Processor
- **MCU**: ESP32-P4 (RISC-V dual-core)
- **Clock**: 360MHz
- **Architecture**: RISC-V RV32IMAFC
- **RAM**: 500KB internal SRAM + external PSRAM support
- **Flash**: 16MB

## Connectivity
- WiFi 6 (802.11ax)
- Bluetooth 5.0
- USB-C for programming and power

## Wokwi Simulation Notes
The Wokwi simulation uses:
- ESP32-S3 (ESP32-P4 not available in Wokwi)
- ILI9341 320×240 display (5-inch MIPI-DSI not available)
- Basic GPIO connections for SPI simulation

The actual firmware is configured for the real M5Tab5 hardware specifications.