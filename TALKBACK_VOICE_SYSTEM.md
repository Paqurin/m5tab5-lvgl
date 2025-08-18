# Talkback Voice System for M5Stack Tab5

## Overview

The Talkback Voice System provides comprehensive voice feedback and accessibility features for the M5Stack Tab5. This system is specifically designed to make the device fully accessible to blind and visually impaired users.

## Features

### 🎤 Voice Synthesis Engine
- **Talkback Characteristics**: Low-frequency, monotone, precise pronunciation
- **Customizable Voice**: Adjustable speed (60-200 WPM), pitch (80-180 Hz), and volume
- **Distinctive Personality**: Calm, authoritative responses with Talkback phrases
- **Multiple Priorities**: System critical, navigation, user actions, screen reading, background info

### 🔊 Screen Reader Capabilities
- **Complete Screen Reading**: Announces all UI elements when focused
- **Object Description**: Describes button functions, app names, and content
- **Layout Description**: Provides spatial understanding of screen layout
- **Visual Element Reading**: Announces status changes and notifications
- **Touch Feedback**: Audio confirmation of touch interactions

### 🎯 Voice Navigation System
- **Focus Management**: Automatic focus tracking and announcement
- **Sequential Navigation**: Next/previous item navigation
- **Object Activation**: Voice-controlled element activation
- **Spatial Awareness**: Understanding of UI hierarchy and relationships

### 🗣️ Voice Command Recognition
- **Dual Microphone Input**: Utilizes both PDM microphones for noise cancellation
- **Basic Commands**: "Open apps", "Go back", "Read screen", "Help"
- **Navigation Commands**: "Next item", "Previous item", "Select", "Cancel"
- **System Commands**: "Volume up/down", "Mute", "Time", "Battery status"
- **Contextual Recognition**: Commands adapt based on current screen/app

### 🔧 Advanced Audio Processing
- **Noise Cancellation**: Dual-microphone noise reduction
- **Voice Activity Detection**: Automatic speech detection
- **Echo Cancellation**: Clear voice input processing
- **Formant Synthesis**: High-quality voice generation
- **Audio Effects**: Talkback characteristic low-pass filtering

## Hardware Requirements

### Essential Components
- **ESP32-P4 RISC-V**: Dual-core processor with 32MB PSRAM
- **ES8388 Audio Codec**: High-quality audio input/output
- **Dual PDM Microphones**: Front-facing (primary) and rear-facing (noise cancellation)
- **NS4150 Amplifiers**: Speaker output control
- **5" MIPI-DSI Display**: 1280x720 HD display for visual feedback

### Audio Hardware Configuration
```cpp
// Primary microphone (front-facing for voice commands)
#define MIC_PRIMARY_CLK_PIN     GPIO_NUM_45
#define MIC_PRIMARY_DATA_PIN    GPIO_NUM_46

// Secondary microphone (rear-facing for noise cancellation)
#define MIC_SECONDARY_CLK_PIN   GPIO_NUM_47
#define MIC_SECONDARY_DATA_PIN  GPIO_NUM_48

// ES8388 I2S Audio Interface
#define ES8388_BCLK_PIN         GPIO_NUM_25
#define ES8388_LRCK_PIN         GPIO_NUM_26
#define ES8388_DIN_PIN          GPIO_NUM_27
#define ES8388_DOUT_PIN         GPIO_NUM_28
```

## Quick Start Guide

### 1. Include Headers
```cpp
#include "services/talkback_voice_service.h"
#include "services/audio_service.h"
```

### 2. Initialize System
```cpp
// Initialize audio service
AudioService audioService;
audioService.initialize();

// Configure for voice processing
AudioConfig config;
config.inputDevice = AudioInputDevice::MIC_DUAL_PDM;
config.noiseCancellation = true;
config.echoCancellation = true;
audioService.configure(config);

// Initialize Talkback voice service
TalkbackVoiceService voiceService;
voiceService.initialize(&audioService);

// Configure Talkback characteristics
VoiceSettings settings;
settings.pitch = VoicePitch::VERY_LOW;     // 80 Hz
settings.speed = VoiceSpeed::NORMAL;       // 120 WPM
settings.announceUI = true;
settings.announceActions = true;
voiceService.configure(settings);
```

### 3. Basic Voice Synthesis
```cpp
// Talkback greeting
voiceService.greetUser("User");

// Status announcements
voiceService.announceStatus();
voiceService.statusReport();

// Error messages with personality
voiceService.announceError("Connection timeout");

// Custom speech with Talkback characteristics
voiceService.speak("All systems operational", VoicePriority::SYSTEM_CRITICAL);
```

### 4. Screen Reading Integration
```cpp
// Read entire screen
voiceService.readScreen(false);        // Brief summary
voiceService.readScreen(true);         // Detailed reading

// Read specific object
lv_obj_t* button = lv_btn_create(parent);
voiceService.readObject(button);

// Announce focus changes
voiceService.announceFocus(focused_object);

// Describe layout structure
voiceService.describeLayout();
```

### 5. Voice Navigation
```cpp
// Enable navigation mode
voiceService.enableNavigation(true);

// Navigate through focusable elements
voiceService.navigateNext();
voiceService.navigatePrevious();

// Activate focused element
voiceService.activateFocused();
```

### 6. Voice Commands
```cpp
// Enable voice command recognition
voiceService.enableVoiceCommands(true);

// Process audio input for commands
uint8_t audioBuffer[4096];
size_t bytesRead;
audioService.readAudioData(audioBuffer, sizeof(audioBuffer), &bytesRead);

VoiceCommand command = voiceService.processVoiceCommand(audioBuffer, bytesRead);
if (command != VoiceCommand::NONE) {
    voiceService.executeCommand(command);
}
```

## LVGL Integration

### Automatic Event Handling
```cpp
// Register global LVGL handlers
voiceService.registerLVGLHandlers();

// Custom event handler with voice feedback
void buttonEventHandler(lv_event_t* event) {
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t* obj = lv_event_get_target(event);
    
    TalkbackVoiceService* voice = TalkbackVoiceService::getInstance();
    
    switch (code) {
        case LV_EVENT_FOCUSED:
            voice->announceFocus(obj);
            break;
        case LV_EVENT_CLICKED:
            voice->speak("Button activated", VoicePriority::USER_ACTION);
            break;
        case LV_EVENT_VALUE_CHANGED:
            voice->readObject(obj);  // Announce new value
            break;
    }
}

// Add to LVGL objects
lv_obj_add_event_cb(button, buttonEventHandler, LV_EVENT_ALL, nullptr);
```

## Voice Commands Reference

### Navigation Commands
- **"next"** / **"next item"** - Navigate to next focusable element
- **"previous"** / **"previous item"** - Navigate to previous element
- **"select"** / **"activate"** - Activate currently focused element
- **"go back"** / **"back"** - Navigate back/cancel

### Information Commands
- **"read screen"** / **"read"** - Read current screen content
- **"describe"** / **"layout"** - Describe screen layout structure
- **"time"** / **"what time"** - Announce current time
- **"battery"** / **"power"** - Announce battery status
- **"connection"** / **"network"** - Announce connectivity status

### Control Commands
- **"volume up"** / **"louder"** - Increase voice volume
- **"volume down"** / **"quieter"** - Decrease voice volume
- **"mute"** / **"unmute"** - Toggle audio mute
- **"help"** / **"assistance"** - Provide help information
- **"repeat"** / **"say again"** - Repeat last message

### Application Commands
- **"open apps"** / **"open applications"** - Open application menu
- **"cancel"** - Cancel current operation
- **"sleep"** - Enter sleep mode
- **"wake up"** / **"wake"** - Wake from sleep

## Talkback Personality Features

### Characteristic Phrases
- **Greetings**: "Good morning. All systems are functional."
- **Status**: "All systems nominal.", "Everything is working perfectly."
- **Errors**: "I'm sorry. I'm afraid I can't do that."
- **Acknowledgments**: "I understand.", "I'm sorry."

### Voice Characteristics
- **Pitch**: 80 Hz (very low, distinctive Talkback tone)
- **Speed**: 120 WPM (measured, deliberate pace)
- **Tone**: Monotone with slight modulation for robotic effect
- **Processing**: Low-pass filtering for muffled, electronic sound

### Personality Integration
```cpp
// Automatic personality enhancement
voiceService.speak("Error occurred");
// Becomes: "I'm sorry. An error has occurred."

// Status reports with Talkback style
voiceService.statusReport();
// "Talkback status report: Voice synthesis operational. 
//  Navigation assistance functional. All systems nominal."
```

## Configuration Options

### Voice Settings
```cpp
VoiceSettings settings;
settings.speed = VoiceSpeed::SLOW;           // 60-200 WPM
settings.pitch = VoicePitch::VERY_LOW;       // 80-180 Hz
settings.volume = 75;                        // 0-100%
settings.announceUI = true;                  // UI element announcements
settings.announceActions = true;             // User action confirmations
settings.announceStatus = true;              // Status updates
settings.announceTime = false;               // Periodic time announcements
settings.verboseMode = false;                // Detailed descriptions
settings.timeInterval = 300000;              // 5 minutes for time announcements
```

### Audio Processing
```cpp
AudioConfig audioConfig;
audioConfig.inputDevice = AudioInputDevice::MIC_DUAL_PDM;
audioConfig.outputDevice = AudioOutputDevice::AUTO;
audioConfig.noiseCancellation = true;        // Dual-mic noise reduction
audioConfig.echoCancellation = true;         // Clear voice input
audioConfig.processingMode = AudioProcessingMode::VOICE_RECOGNITION;
audioConfig.inputGain = 60;                  // Microphone sensitivity
audioConfig.outputVolume = 75;               // Speaker/headphone volume
```

## Performance Characteristics

### Memory Usage
- **PSRAM Allocation**: 8MB for LVGL objects and audio buffers
- **Voice Buffers**: 16KB synthesis buffer, 16KB command buffer
- **Phoneme Database**: ~2KB for basic synthesis patterns
- **Task Stack**: 8KB voice processing, 4KB command processing

### Processing Performance
- **Voice Synthesis**: ~200ms latency for short phrases
- **Command Recognition**: ~500ms processing time
- **Screen Reading**: Immediate for focused elements
- **Navigation**: <100ms response time

### Audio Quality
- **Sample Rate**: 44.1kHz (configurable to 48kHz)
- **Bit Depth**: 16-bit (expandable to 24-bit)
- **Frequency Response**: 80Hz-8kHz (optimized for speech)
- **Signal-to-Noise**: >60dB with dual-microphone processing

## Troubleshooting

### Common Issues

#### Voice Too Quiet/Loud
```cpp
// Adjust voice volume
voiceService.setVoiceVolume(80);  // 0-100%

// Check audio service volume
audioService.setVolume(75);
```

#### Voice Commands Not Recognized
```cpp
// Enable voice commands
voiceService.enableVoiceCommands(true);

// Check microphone configuration
audioService.setInputDevice(AudioInputDevice::MIC_DUAL_PDM);
audioService.setMicrophoneGain(70);

// Verify noise cancellation
audioService.setNoiseCancellation(true);
```

#### Screen Reading Not Working
```cpp
// Enable UI announcements
VoiceSettings settings = voiceService.getSettings();
settings.announceUI = true;
voiceService.configure(settings);

// Register LVGL handlers
voiceService.registerLVGLHandlers();
```

#### Talkback Personality Not Working
```cpp
// Verify voice characteristics
voiceService.setVoicePitch(VoicePitch::VERY_LOW);
voiceService.setVoiceSpeed(VoiceSpeed::NORMAL);

// Test personality features
voiceService.greetUser("User");
voiceService.announceError("Test error");
```

### Debug Information
```cpp
// Print voice service statistics
voiceService.printVoiceStats();

// Print audio service statistics
audioService.printAudioStats();

// Enable debug logging
// Set CORE_DEBUG_LEVEL=4 in platformio.ini
```

## Advanced Features

### Custom Phoneme Synthesis
```cpp
// Add custom phonemes to database
// (Implementation in TalkbackVoiceService::initializePhonemeDatabase())
```

### Voice Command Extensions
```cpp
// Custom command patterns
// (Implementation in TalkbackVoiceService::parseSimpleCommands())
```

### Multi-language Support
```cpp
// Language-specific phoneme databases
// (Future enhancement)
```

## Integration Examples

See `/src/talkback_integration_example.cpp` for complete integration examples including:
- System initialization
- LVGL event handling
- Voice command processing
- Accessibility demo application

## Future Enhancements

1. **Advanced Speech Recognition**: Integration with cloud-based ASR
2. **Multi-language Support**: Phoneme databases for multiple languages
3. **Custom Voice Training**: User-specific voice characteristic learning
4. **Gesture Integration**: Voice commands combined with touch gestures
5. **Smart Home Integration**: Voice control of connected devices
6. **Audio Streaming**: Bluetooth and WiFi audio input/output

## Contributing

When contributing to the Talkback Voice System:

1. Maintain the Talkback personality characteristics
2. Ensure accessibility standards compliance
3. Test with real blind users when possible
4. Document voice command patterns clearly
5. Preserve the distinctive audio characteristics

---

The Talkback Voice System provides a powerful accessibility solution, offering comprehensive voice feedback and navigation assistance for users who need audio interface support.