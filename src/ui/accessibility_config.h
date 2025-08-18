#ifndef ACCESSIBILITY_CONFIG_H
#define ACCESSIBILITY_CONFIG_H

#include <lvgl.h>

// Forward declaration - VoicePriority is defined in talkback_voice_service.h
enum class VoicePriority;

/**
 * @file accessibility_config.h
 * @brief Accessibility configuration constants for M5Stack Tab5
 * 
 * Defines WCAG-compliant accessibility standards and configurations
 * for creating an inclusive user interface.
 */

// WCAG Contrast Ratios
#define WCAG_AA_NORMAL_CONTRAST     4.5f    // WCAG AA for normal text
#define WCAG_AA_LARGE_CONTRAST      3.0f    // WCAG AA for large text (18pt+)
#define WCAG_AAA_NORMAL_CONTRAST    7.0f    // WCAG AAA for normal text
#define WCAG_AAA_LARGE_CONTRAST     4.5f    // WCAG AAA for large text

// Touch Target Sizes (in pixels)
#define TOUCH_TARGET_MINIMUM        32      // Absolute minimum
#define TOUCH_TARGET_RECOMMENDED    44      // WCAG recommended
#define TOUCH_TARGET_LARGE          56      // Large accessibility mode

// Font Scaling
#define FONT_SCALE_NORMAL          1.0f     // Normal font size
#define FONT_SCALE_LARGE           1.25f    // Large accessibility fonts
#define FONT_SCALE_EXTRA_LARGE     1.5f     // Extra large fonts

// Border and Focus Widths
#define BORDER_NORMAL              1        // Normal border width
#define BORDER_ACCESSIBILITY       3        // Accessibility border width
#define FOCUS_NORMAL               2        // Normal focus indicator
#define FOCUS_ACCESSIBILITY        4        // Accessibility focus indicator

// High Contrast Color Definitions
namespace AccessibilityColors {
    // Pure contrast colors for maximum visibility
    constexpr uint32_t PURE_WHITE = 0xFFFFFF;
    constexpr uint32_t PURE_BLACK = 0x000000;
    constexpr uint32_t LIGHT_GRAY = 0xCCCCCC;
    constexpr uint32_t DARK_GRAY = 0x333333;
    
    // High contrast amber theme
    constexpr uint32_t AMBER_PRIMARY = 0xFFB000;
    constexpr uint32_t AMBER_SECONDARY = 0xFF8800;
    constexpr uint32_t AMBER_BACKGROUND = 0x1A1A1A;
    
    // Colorblind-friendly colors (protanopia/deuteranopia safe)
    constexpr uint32_t CB_BLUE = 0x0077BE;        // Blue visible to colorblind
    constexpr uint32_t CB_GREEN = 0x009E73;       // Teal/green visible to colorblind
    constexpr uint32_t CB_ORANGE = 0xD55E00;      // Orange visible to colorblind
    constexpr uint32_t CB_YELLOW = 0xF0E442;      // Yellow visible to colorblind
    constexpr uint32_t CB_PURPLE = 0xCC79A7;      // Purple visible to colorblind
    
    // Status colors with high contrast
    constexpr uint32_t ERROR_LIGHT = 0xFF6666;    // For dark backgrounds
    constexpr uint32_t ERROR_DARK = 0xCC0000;     // For light backgrounds
    constexpr uint32_t SUCCESS_LIGHT = 0x66FF66;  // For dark backgrounds
    constexpr uint32_t SUCCESS_DARK = 0x006600;   // For light backgrounds
    constexpr uint32_t WARNING_LIGHT = 0xFFDD44;  // For dark backgrounds
    constexpr uint32_t WARNING_DARK = 0xCC8800;   // For light backgrounds
}

// Animation timing for accessibility
#define ANIM_FAST                  150      // Fast animations (ms)
#define ANIM_NORMAL                300      // Normal animations (ms)
#define ANIM_SLOW                  500      // Slow animations for accessibility
#define ANIM_DISABLED              0        // No animations

// VoicePriority enum is defined in talkback_voice_service.h

// Accessibility feature flags
struct AccessibilityFeatures {
    static constexpr uint32_t NONE = 0x00000000;
    static constexpr uint32_t HIGH_CONTRAST = 0x00000001;
    static constexpr uint32_t LARGE_FONTS = 0x00000002;
    static constexpr uint32_t BOLD_BORDERS = 0x00000004;
    static constexpr uint32_t LARGE_TOUCH_TARGETS = 0x00000008;
    static constexpr uint32_t ENHANCED_FOCUS = 0x00000010;
    static constexpr uint32_t VOICE_FEEDBACK = 0x00000020;
    static constexpr uint32_t SLOW_ANIMATIONS = 0x00000040;
    static constexpr uint32_t COLORBLIND_PATTERNS = 0x00000080;
    static constexpr uint32_t KEYBOARD_NAVIGATION = 0x00000100;
    
    // Preset combinations
    static constexpr uint32_t VISUAL_IMPAIRMENT = HIGH_CONTRAST | LARGE_FONTS | BOLD_BORDERS | ENHANCED_FOCUS;
    static constexpr uint32_t MOTOR_IMPAIRMENT = LARGE_TOUCH_TARGETS | SLOW_ANIMATIONS;
    static constexpr uint32_t COLORBLIND = COLORBLIND_PATTERNS | HIGH_CONTRAST;
    static constexpr uint32_t FULL_ACCESSIBILITY = 0xFFFFFFFF;
};

// Keyboard shortcuts for accessibility
enum class AccessibilityShortcut {
    TOGGLE_ACCESSIBILITY,       // Alt + A
    CYCLE_THEMES,              // Alt + T
    INCREASE_FONT_SIZE,        // Ctrl + Plus
    DECREASE_FONT_SIZE,        // Ctrl + Minus
    TOGGLE_HIGH_CONTRAST,      // Alt + H
    TOGGLE_VOICE_FEEDBACK,     // Alt + V
    FOCUS_NEXT,               // Tab
    FOCUS_PREVIOUS,           // Shift + Tab
    ACTIVATE_ELEMENT,         // Space/Enter
    ESCAPE_CONTEXT            // Escape
};

// Screen reader compatibility
struct ScreenReaderConfig {
    bool enabled = false;
    bool announceStateChanges = true;
    bool announceNavigation = true;
    bool announceNotifications = true;
    uint32_t speakingRate = 200;        // Words per minute
    uint32_t announcementDelay = 500;   // ms delay between announcements
};

// Gesture accessibility
struct GestureConfig {
    bool threeFingerTap = true;         // Toggle accessibility mode
    bool twoFingerSwipe = true;         // Navigate between sections
    bool longPress = true;              // Context menu/help
    uint32_t longPressDelay = 800;      // ms for long press recognition
    uint32_t multiTouchTimeout = 300;   // ms timeout for multi-touch
};

// Performance considerations for accessibility
struct PerformanceConfig {
    bool reducedAnimations = false;     // Reduce motion for sensitive users
    bool simplifiedEffects = false;    // Disable complex visual effects
    bool prioritizeAccessibility = true; // Prioritize a11y over visual flair
    uint32_t maxTextLength = 1000;     // Max characters for voice feedback
};

// Default accessibility configuration
inline AccessibilityConfig getDefaultAccessibilityConfig() {
    AccessibilityConfig config;
    config.isEnabled = false;
    config.largeFonts = false;
    config.boldBorders = false;
    config.enhancedFocus = false;
    config.largeTouchTargets = false;
    config.preferredTheme = ThemeType::HIGH_CONTRAST_DARK;
    return config;
}

// Quick setup presets
inline AccessibilityConfig getVisualImpairmentConfig() {
    AccessibilityConfig config;
    config.isEnabled = true;
    config.largeFonts = true;
    config.boldBorders = true;
    config.enhancedFocus = true;
    config.largeTouchTargets = true;
    config.preferredTheme = ThemeType::HIGH_CONTRAST_DARK;
    return config;
}

inline AccessibilityConfig getMotorImpairmentConfig() {
    AccessibilityConfig config;
    config.isEnabled = true;
    config.largeFonts = false;
    config.boldBorders = true;
    config.enhancedFocus = true;
    config.largeTouchTargets = true;
    config.preferredTheme = ThemeType::LIGHT;
    return config;
}

inline AccessibilityConfig getColorblindConfig() {
    AccessibilityConfig config;
    config.isEnabled = true;
    config.largeFonts = false;
    config.boldBorders = true;
    config.enhancedFocus = false;
    config.largeTouchTargets = false;
    config.preferredTheme = ThemeType::COLORBLIND_FRIENDLY;
    return config;
}

#endif // ACCESSIBILITY_CONFIG_H