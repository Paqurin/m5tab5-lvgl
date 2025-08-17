# M5Stack Tab5 App Installation Guide

## Overview

This guide explains how to install applications from the M5Stack Tab5 App Store onto your M5Stack Tab5 device. The apps are packaged as `.m5app` files following the official development standards.

## Prerequisites

### Hardware Requirements
- M5Stack Tab5 device with ESP32-P4 processor
- 5-inch 1280×720 display
- Minimum 500KB RAM + 32MB PSRAM
- 16MB Flash storage
- USB-C cable for data transfer

### Software Requirements
- M5Stack Tab5 OS v4.0.0 or higher
- Modular App System (included in v4)
- Available storage space for app installation

## Installation Methods

### Method 1: Direct Package Installation (Recommended)

1. **Download App Package**
   - Visit the M5Stack Tab5 App Store website
   - Browse available applications
   - Click "Download" on desired app
   - Save the `.m5app` file to your computer

2. **Transfer to Device**
   - Connect M5Stack Tab5 to computer via USB-C
   - Copy the `.m5app` file to the device storage
   - Recommended location: `/storage/apps/`

3. **Install via Modular App System**
   - On your M5Stack Tab5, open "Modular App System"
   - Navigate to "Install from Package"
   - Select the downloaded `.m5app` file
   - Review app permissions and requirements
   - Confirm installation
   - Wait for installation to complete

4. **Launch Application**
   - Return to the main app menu
   - Find the newly installed app
   - Tap to launch and enjoy!

### Method 2: Manual Development Integration

For developers who want to integrate apps into their custom builds:

1. **Extract Package Contents**
   ```bash
   unzip com.m5stack.contacts-v1.0.0.m5app -d contact-app/
   ```

2. **Copy Source Files**
   ```bash
   cp contact-app/src/* ~/m5tab5-lvgl/src/apps/contacts/
   ```

3. **Update Build Configuration**
   - Add app to your `platformio.ini`
   - Include factory function in app registration
   - Add required dependencies

4. **Compile and Flash**
   ```bash
   cd ~/m5tab5-lvgl
   pio run -e esp32-p4-evboard
   pio run -e esp32-p4-evboard -t upload
   ```

## App Categories and Features

### Personal Assistant Apps
- **Contact Management**: Full address book with VCard support
- **Task Management**: Smart to-do lists with priorities
- **Voice Assistant**: ChatGPT integration with 6 languages
- **Basic Apps Suite**: Calculator, expense tracker, games
- **Alarm & Timer**: Smart alarms and timekeeping tools

### System Requirements by Category

| Category | RAM Usage | Flash Usage | Special Requirements |
|----------|-----------|-------------|---------------------|
| Personal Assistant | 32-65KB | 150-680KB | Storage access |
| Productivity | 50-130KB | 180-530KB | File system access |
| System Utility | 40-100KB | 180-450KB | Network access |
| Industrial IoT | 65-150KB | 270-610KB | RS-485, Modbus protocols |

## Troubleshooting

### Common Installation Issues

#### "Insufficient Memory" Error
- **Cause**: Not enough available RAM or Flash storage
- **Solution**: 
  - Close running applications
  - Remove unused apps to free up space
  - Check memory requirements in app details

#### "Incompatible OS Version" Error
- **Cause**: App requires newer OS version
- **Solution**: 
  - Update M5Stack Tab5 OS to v4.0.0 or higher
  - Check minimum OS requirements in app manifest

#### "Missing Permissions" Error
- **Cause**: App requires permissions not granted
- **Solution**: 
  - Review and accept required permissions
  - Enable required system features (WiFi, storage, etc.)

#### "Package Corrupted" Error
- **Cause**: Downloaded package is damaged
- **Solution**: 
  - Re-download the app package
  - Verify file integrity
  - Check USB transfer for errors

### Installation Verification

To verify successful installation:

1. **Check App List**
   ```bash
   # If using terminal access
   m5tab5-app list
   ```

2. **Verify Memory Usage**
   - Open System Information app
   - Check available memory
   - Confirm app appears in installed list

3. **Test App Launch**
   - Navigate to app menu
   - Tap app icon
   - Verify app loads correctly

## Security Considerations

### App Verification
- All official apps are signed and verified
- Review permissions before installation
- Only install apps from trusted sources

### Data Privacy
- Apps have sandboxed storage access
- Personal data remains on device
- Review privacy policies for network-enabled apps

### System Security
- Apps cannot modify system files
- Automatic permission management
- Safe installation and removal process

## Advanced Configuration

### Custom Installation Paths
```bash
# Set custom app installation directory
export M5TAB5_APP_DIR="/custom/path/apps"
```

### Batch Installation
```bash
# Install multiple apps from directory
for app in *.m5app; do
    m5tab5-app install "$app"
done
```

### Developer Mode
```bash
# Enable developer mode for unsigned apps
m5tab5-config set developer_mode true
```

## Support and Resources

### Official Resources
- **GitHub Repository**: https://github.com/Paqurin/m5tab5-lvgl
- **Development Guide**: [APP_DEVELOPMENT_STANDARD.md](APP_DEVELOPMENT_STANDARD.md)
- **Community Forum**: M5Stack Tab5 Discussions

### Getting Help
- **Email Support**: support@m5stack-tab5.dev
- **Issue Tracker**: GitHub Issues
- **Documentation**: Complete app documentation available

### Contributing
- Submit bug reports and feature requests
- Contribute to app development
- Share custom applications with the community

## Package Format Specification

### .m5app Package Structure
```
app-name.m5app (ZIP format)
├── manifest.json          # App metadata and requirements
├── README.md              # App documentation
├── LICENSE                # License information
├── CHANGELOG.md           # Version history
├── install.sh             # Installation script
├── src/                   # Source code files
│   ├── app_header.h
│   └── app_implementation.cpp
└── assets/                # App resources
    ├── icon.png
    ├── screenshots/
    ├── fonts/
    ├── images/
    └── sounds/
```

### Manifest Schema
```json
{
  "app": {
    "id": "com.example.app",
    "name": "App Name",
    "version": "1.0.0",
    "description": "App description"
  },
  "system": {
    "min_os_version": "4.0.0",
    "target_platform": "m5stack-tab5"
  },
  "requirements": {
    "memory": {"ram": 65536, "flash": 262144},
    "permissions": ["STORAGE_READ", "NETWORK_ACCESS"]
  }
}
```

---

**M5Stack Tab5 App Store** - Professional applications for your ESP32-P4 device