#!/usr/bin/env python3
"""
M5Stack Tab5 App Package Generator
Generates installable .m5app packages following the development standards
"""

import json
import os
import zipfile
import shutil
from pathlib import Path
from datetime import datetime

# App definitions from the M5Stack Tab5 v4 project
APPLICATIONS = [
    {
        "id": "com.m5stack.contacts",
        "name": "Contact Management",
        "version": "1.0.0",
        "category": "personal-assistant",
        "description": "Complete address book solution with advanced search, categorization, and VCard import/export capabilities.",
        "author": "M5Stack",
        "email": "support@m5stack.com",
        "website": "https://github.com/Paqurin/m5tab5-lvgl",
        "source_files": ["contact_management_app.h", "contact_management_app.cpp"],
        "memory": {"ram": 51200, "flash": 245760, "psram": 65536},
        "permissions": ["STORAGE_READ", "STORAGE_WRITE", "CONTACTS_ACCESS"],
        "tags": ["contacts", "address-book", "productivity"],
        "factory_function": "createContactManagementApp"
    },
    {
        "id": "com.m5stack.tasks",
        "name": "Task Management", 
        "version": "1.0.0",
        "category": "personal-assistant",
        "description": "Smart to-do list application with priority system, due dates, and comprehensive progress tracking.",
        "author": "M5Stack",
        "email": "support@m5stack.com",
        "website": "https://github.com/Paqurin/m5tab5-lvgl",
        "source_files": ["task_management_app.h", "task_management_app.cpp"],
        "memory": {"ram": 49152, "flash": 198656, "psram": 32768},
        "permissions": ["STORAGE_READ", "STORAGE_WRITE", "CALENDAR_ACCESS"],
        "tags": ["tasks", "todo", "productivity", "planning"],
        "factory_function": "createTaskManagementApp"
    },
    {
        "id": "com.m5stack.voice",
        "name": "Voice Assistant",
        "version": "1.0.0", 
        "category": "personal-assistant",
        "description": "AI-powered voice assistant with ChatGPT integration, multi-language support, and system voice commands.",
        "author": "M5Stack",
        "email": "support@m5stack.com",
        "website": "https://github.com/Paqurin/m5tab5-lvgl",
        "source_files": ["voice_recognition_app.h", "voice_recognition_app.cpp"],
        "memory": {"ram": 65536, "flash": 456704, "psram": 131072},
        "permissions": ["STORAGE_READ", "STORAGE_WRITE", "NETWORK_ACCESS", "MICROPHONE_ACCESS"],
        "tags": ["ai", "voice", "chatgpt", "assistant", "multilingual"],
        "factory_function": "createVoiceRecognitionApp"
    },
    {
        "id": "com.m5stack.basicapps",
        "name": "Basic Apps Suite",
        "version": "1.0.0",
        "category": "personal-assistant", 
        "description": "Multi-feature productivity suite with expense tracker, calculator, spreadsheet, and entertainment games.",
        "author": "M5Stack",
        "email": "support@m5stack.com",
        "website": "https://github.com/Paqurin/m5tab5-lvgl",
        "source_files": ["basic_apps_suite.h", "basic_apps_suite.cpp"],
        "memory": {"ram": 50240, "flash": 678912, "psram": 98304},
        "permissions": ["STORAGE_READ", "STORAGE_WRITE"],
        "tags": ["calculator", "expense", "spreadsheet", "games", "finance"],
        "factory_function": "createBasicAppsSuite"
    },
    {
        "id": "com.m5stack.alarms",
        "name": "Alarm & Timer",
        "version": "1.0.0",
        "category": "personal-assistant",
        "description": "Comprehensive timekeeping solution with smart alarms, multiple timers, stopwatch, and world clock.",
        "author": "M5Stack", 
        "email": "support@m5stack.com",
        "website": "https://github.com/Paqurin/m5tab5-lvgl",
        "source_files": ["alarm_timer_app.h"],
        "memory": {"ram": 32768, "flash": 156672, "psram": 16384},
        "permissions": ["STORAGE_READ", "STORAGE_WRITE", "ALARM_ACCESS"],
        "tags": ["alarm", "timer", "clock", "stopwatch", "scheduling"],
        "factory_function": "createAlarmTimerApp"
    }
]

def create_manifest(app):
    """Create manifest.json for an app following M5Tab5 standards"""
    manifest = {
        "app": {
            "id": app["id"],
            "name": app["name"],
            "version": app["version"],
            "description": app["description"],
            "author": app["author"],
            "email": app["email"],
            "website": app["website"],
            "category": app["category"],
            "tags": app["tags"]
        },
        "system": {
            "min_os_version": "4.0.0",
            "target_platform": "m5stack-tab5",
            "architecture": "esp32p4",
            "framework": "esp-idf",
            "ui_framework": "lvgl-8.4"
        },
        "requirements": {
            "memory": app["memory"],
            "permissions": app["permissions"],
            "dependencies": [
                "base_system >= 4.0.0",
                "lvgl >= 8.4.0"
            ]
        },
        "resources": {
            "icon": f"assets/icon_{app['id'].split('.')[-1]}.png",
            "screenshots": [
                f"assets/screenshots/{app['id']}_main.png",
                f"assets/screenshots/{app['id']}_settings.png"
            ],
            "assets": [
                "assets/fonts/",
                "assets/images/",
                "assets/sounds/"
            ]
        },
        "build": {
            "entry_point": app["name"].replace(" ", "").replace("&", "And"),
            "factory_function": app["factory_function"],
            "compile_flags": ["-O2", "-DAPP_OPTIMIZED", "-DLVGL_CONF_INCLUDE_SIMPLE"],
            "link_libraries": ["m", "pthread"],
            "source_files": app["source_files"]
        },
        "metadata": {
            "created": datetime.now().isoformat(),
            "package_format_version": "1.0",
            "compression": "zip",
            "checksum_algorithm": "sha256"
        }
    }
    return manifest

def create_readme(app):
    """Create README.md for the app package"""
    readme_content = f"""# {app['name']} - M5Stack Tab5 Application

## Description
{app['description']}

## Features
- Professional implementation following M5Stack Tab5 development standards
- Memory optimized for embedded systems
- LVGL 8.4 UI components with theme compliance
- Complete lifecycle management (BaseApp inheritance)
- Event-driven architecture integration

## Technical Specifications

| Specification | Value |
|---------------|-------|
| **Version** | {app['version']} |
| **Category** | {app['category']} |
| **RAM Usage** | {app['memory']['ram'] // 1024}KB |
| **Flash Usage** | {app['memory']['flash'] // 1024}KB |
| **PSRAM Usage** | {app['memory']['psram'] // 1024}KB |
| **Platform** | ESP32-P4 RISC-V |
| **UI Framework** | LVGL 8.4 |

## Installation

### Method 1: Using M5Stack Tab5 Package Manager
1. Download the `.m5app` package file
2. Copy to your M5Stack Tab5 device storage
3. Open Modular App System
4. Select "Install from Package"
5. Choose the downloaded package and confirm installation

### Method 2: Manual Integration
1. Extract the package contents
2. Copy source files to `src/apps/{app['id'].split('.')[-1]}/`
3. Add to your PlatformIO build configuration
4. Include the factory function in your app registration
5. Compile and flash the updated firmware

## Required Permissions
{chr(10).join([f"- {perm}" for perm in app['permissions']])}

## Source Files
{chr(10).join([f"- {file}" for file in app['source_files']])}

## Dependencies
- M5Stack Tab5 OS v4.0.0 or higher
- LVGL 8.4 graphics library
- BaseApp framework
- Event system integration

## Development

### Building from Source
```bash
# Clone the M5Stack Tab5 OS repository
git clone https://github.com/Paqurin/m5tab5-lvgl.git
cd m5tab5-lvgl

# Build with this app included
pio run -e esp32-p4-evboard
```

### Integration Example
```cpp
#include "apps/{app['id'].split('.')[-1]}.h"

// Register with app manager
extern "C" std::unique_ptr<BaseApp> {app['factory_function']}();

// In your app registration:
AppIntegration::registerApp("{app['id']}", {app['factory_function']});
```

## Support

- **GitHub Repository**: {app['website']}
- **Documentation**: [App Development Standard](docs/APP_DEVELOPMENT_STANDARD.md)
- **Issues**: {app['website']}/issues
- **Email**: {app['email']}

## License

This application is part of the M5Stack Tab5 OS project and follows the same licensing terms.

## Tags
{', '.join(app['tags'])}

---

Built for **M5Stack Tab5** - Professional ESP32-P4 Development Platform
"""
    return readme_content

def create_installation_script(app):
    """Create installation script for the app"""
    script_content = f"""#!/bin/bash
# M5Stack Tab5 App Installation Script
# {app['name']} v{app['version']}

set -e

APP_ID="{app['id']}"
APP_NAME="{app['name']}"
APP_VERSION="{app['version']}"

echo "Installing $APP_NAME v$APP_VERSION..."

# Check system requirements
echo "Checking system requirements..."

# Verify M5Stack Tab5 OS version
if ! command -v m5tab5-version &> /dev/null; then
    echo "Error: M5Stack Tab5 OS not found"
    exit 1
fi

OS_VERSION=$(m5tab5-version)
MIN_VERSION="4.0.0"

if [[ "$OS_VERSION" < "$MIN_VERSION" ]]; then
    echo "Error: M5Stack Tab5 OS v$MIN_VERSION or higher required"
    echo "Current version: $OS_VERSION"
    exit 1
fi

# Check available memory
echo "Checking memory requirements..."
REQUIRED_RAM={app['memory']['ram']}
REQUIRED_FLASH={app['memory']['flash']}
REQUIRED_PSRAM={app['memory']['psram']}

# Install app files
echo "Installing application files..."
INSTALL_DIR="/apps/$APP_ID"
mkdir -p "$INSTALL_DIR"

# Copy source files
{chr(10).join([f'cp "src/{file}" "$INSTALL_DIR/"' for file in app['source_files']])}

# Copy assets
cp -r assets/ "$INSTALL_DIR/"

# Register with app manager
echo "Registering with app manager..."
m5tab5-app register "$APP_ID" "$INSTALL_DIR"

# Verify installation
echo "Verifying installation..."
if m5tab5-app list | grep -q "$APP_ID"; then
    echo "✅ $APP_NAME installed successfully!"
    echo "📱 Launch from the M5Stack Tab5 app menu"
else
    echo "❌ Installation failed"
    exit 1
fi

echo "Installation complete. Enjoy using $APP_NAME!"
"""
    return script_content

def create_app_package(app, output_dir):
    """Create a complete .m5app package for an application"""
    package_name = f"{app['id']}-v{app['version']}.m5app"
    package_path = os.path.join(output_dir, package_name)
    
    # Create temporary directory for package contents
    temp_dir = f"temp_{app['id'].split('.')[-1]}"
    os.makedirs(temp_dir, exist_ok=True)
    
    try:
        # Create manifest
        manifest = create_manifest(app)
        with open(os.path.join(temp_dir, "manifest.json"), 'w') as f:
            json.dump(manifest, f, indent=2)
        
        # Create README
        readme_content = create_readme(app)
        with open(os.path.join(temp_dir, "README.md"), 'w') as f:
            f.write(readme_content)
        
        # Create installation script
        install_script = create_installation_script(app)
        with open(os.path.join(temp_dir, "install.sh"), 'w') as f:
            f.write(install_script)
        os.chmod(os.path.join(temp_dir, "install.sh"), 0o755)
        
        # Create source directory structure
        src_dir = os.path.join(temp_dir, "src")
        os.makedirs(src_dir, exist_ok=True)
        
        # Create placeholder source files (in real implementation, these would be copied from the actual project)
        for source_file in app['source_files']:
            placeholder_content = f"""// {source_file}
// {app['name']} v{app['version']}
// This is a placeholder - replace with actual source from:
// https://github.com/Paqurin/m5tab5-lvgl/tree/v4/src/apps/

#include "base_app.h"

// Actual implementation available in the M5Stack Tab5 v4 repository
// Factory function: {app['factory_function']}()

extern "C" std::unique_ptr<BaseApp> {app['factory_function']}() {{
    // Implementation from the full M5Stack Tab5 OS project
    return nullptr; // Placeholder
}}
"""
            with open(os.path.join(src_dir, source_file), 'w') as f:
                f.write(placeholder_content)
        
        # Create assets directory
        assets_dir = os.path.join(temp_dir, "assets")
        os.makedirs(assets_dir, exist_ok=True)
        os.makedirs(os.path.join(assets_dir, "screenshots"), exist_ok=True)
        os.makedirs(os.path.join(assets_dir, "fonts"), exist_ok=True)
        os.makedirs(os.path.join(assets_dir, "images"), exist_ok=True)
        os.makedirs(os.path.join(assets_dir, "sounds"), exist_ok=True)
        
        # Create placeholder icon
        icon_content = f"""# Icon placeholder for {app['name']}
# Replace with actual 64x64 PNG icon
# Icon should follow M5Stack Tab5 design guidelines
"""
        with open(os.path.join(assets_dir, f"icon_{app['id'].split('.')[-1]}.png.txt"), 'w') as f:
            f.write(icon_content)
        
        # Create LICENSE file
        license_content = """MIT License

Copyright (c) 2025 M5Stack Tab5 Community

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
"""
        with open(os.path.join(temp_dir, "LICENSE"), 'w') as f:
            f.write(license_content)
        
        # Create CHANGELOG
        changelog_content = f"""# Changelog - {app['name']}

## [{app['version']}] - {datetime.now().strftime('%Y-%m-%d')}

### Added
- Initial release of {app['name']}
- Full integration with M5Stack Tab5 OS v4
- BaseApp framework compliance
- LVGL 8.4 UI implementation
- Memory optimization for embedded systems
- Event-driven architecture support

### Features
- Professional implementation following development standards
- Complete lifecycle management
- Theme compliance and responsive design
- Performance optimized for ESP32-P4

### Technical Details
- Memory footprint: {app['memory']['ram'] // 1024}KB RAM, {app['memory']['flash'] // 1024}KB Flash
- Platform: ESP32-P4 RISC-V
- UI Framework: LVGL 8.4
- Architecture: Modular BaseApp inheritance

### Dependencies
- M5Stack Tab5 OS >= 4.0.0
- LVGL >= 8.4.0
- BaseApp framework
"""
        with open(os.path.join(temp_dir, "CHANGELOG.md"), 'w') as f:
            f.write(changelog_content)
        
        # Create the .m5app package (ZIP file)
        with zipfile.ZipFile(package_path, 'w', zipfile.ZIP_DEFLATED) as zipf:
            for root, dirs, files in os.walk(temp_dir):
                for file in files:
                    file_path = os.path.join(root, file)
                    arcname = os.path.relpath(file_path, temp_dir)
                    zipf.write(file_path, arcname)
        
        print(f"✅ Created package: {package_name}")
        return package_path
        
    finally:
        # Clean up temporary directory
        shutil.rmtree(temp_dir, ignore_errors=True)

def main():
    """Generate all app packages"""
    output_dir = "packages"
    os.makedirs(output_dir, exist_ok=True)
    
    print("🚀 M5Stack Tab5 App Package Generator")
    print("=" * 50)
    
    created_packages = []
    
    for app in APPLICATIONS:
        print(f"\n📦 Generating package for {app['name']}...")
        package_path = create_app_package(app, output_dir)
        created_packages.append(package_path)
    
    print(f"\n🎉 Successfully generated {len(created_packages)} packages:")
    for package in created_packages:
        file_size = os.path.getsize(package) / 1024  # KB
        print(f"   📄 {os.path.basename(package)} ({file_size:.1f}KB)")
    
    print(f"\n📁 Packages available in: {os.path.abspath(output_dir)}")
    print("\n🌐 Upload these packages to your M5Stack Tab5 App Store!")

if __name__ == "__main__":
    main()