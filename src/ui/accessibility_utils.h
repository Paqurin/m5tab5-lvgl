#ifndef ACCESSIBILITY_UTILS_H
#define ACCESSIBILITY_UTILS_H

#include <lvgl.h>
#include "theme_manager.h"
#include "accessibility_config.h"

/**
 * @file accessibility_utils.h
 * @brief Accessibility utilities for M5Stack Tab5
 * 
 * Provides helper functions for creating accessible UI components
 * that work well with screen readers and assistive technologies.
 */

class AccessibilityUtils {
public:
    /**
     * @brief Create an accessible button with proper styling
     * @param parent Parent object
     * @param text Button text
     * @param themeManager Theme manager instance
     * @return Created button object
     */
    static lv_obj_t* createAccessibleButton(lv_obj_t* parent, const char* text, ThemeManager* themeManager);

    /**
     * @brief Create an accessible label with proper contrast
     * @param parent Parent object
     * @param text Label text
     * @param themeManager Theme manager instance
     * @return Created label object
     */
    static lv_obj_t* createAccessibleLabel(lv_obj_t* parent, const char* text, ThemeManager* themeManager);

    /**
     * @brief Create an accessible input field
     * @param parent Parent object
     * @param placeholder Placeholder text
     * @param themeManager Theme manager instance
     * @return Created textarea object
     */
    static lv_obj_t* createAccessibleInput(lv_obj_t* parent, const char* placeholder, ThemeManager* themeManager);

    /**
     * @brief Create an accessible switch with enhanced visibility
     * @param parent Parent object
     * @param themeManager Theme manager instance
     * @return Created switch object
     */
    static lv_obj_t* createAccessibleSwitch(lv_obj_t* parent, ThemeManager* themeManager);

    /**
     * @brief Create an accessible slider with enhanced contrast
     * @param parent Parent object
     * @param themeManager Theme manager instance
     * @return Created slider object
     */
    static lv_obj_t* createAccessibleSlider(lv_obj_t* parent, ThemeManager* themeManager);

    /**
     * @brief Apply focus group for keyboard navigation
     * @param objects Array of objects to include in focus group
     * @param count Number of objects
     * @return Created input group
     */
    static lv_group_t* createFocusGroup(lv_obj_t** objects, size_t count);

    /**
     * @brief Add voice navigation support to an object
     * @param obj Object to enhance
     * @param description Voice description for screen readers
     */
    static void addVoiceSupport(lv_obj_t* obj, const char* description);

    /**
     * @brief Create status announcement for screen readers
     * @param message Message to announce
     * @param priority Announcement priority (0-2, higher is more important)
     */
    static void announceStatus(const char* message, uint8_t priority = 1);

    /**
     * @brief Check if contrast ratio meets WCAG standards
     * @param foreground Foreground color
     * @param background Background color
     * @param level WCAG level (1=AA, 2=AAA)
     * @return True if contrast is sufficient
     */
    static bool checkContrastRatio(lv_color_t foreground, lv_color_t background, uint8_t level = 1);

    /**
     * @brief Calculate luminance of a color
     * @param color Color to calculate
     * @return Relative luminance (0.0-1.0)
     */
    static float calculateLuminance(lv_color_t color);

    /**
     * @brief Create visual pattern for colorblind users
     * @param obj Object to apply pattern to
     * @param patternType Pattern type (0=dots, 1=stripes, 2=crosshatch)
     */
    static void applyColorblindPattern(lv_obj_t* obj, uint8_t patternType);

    /**
     * @brief Set up keyboard shortcuts for accessibility
     * @param group Input group to configure
     */
    static void setupAccessibilityShortcuts(lv_group_t* group);

    /**
     * @brief Create accessible confirmation dialog
     * @param parent Parent object
     * @param title Dialog title
     * @param message Dialog message
     * @param themeManager Theme manager instance
     * @return Created message box
     */
    static lv_obj_t* createAccessibleDialog(lv_obj_t* parent, const char* title, 
                                           const char* message, ThemeManager* themeManager);

private:
    static lv_group_t* s_mainFocusGroup;  // Main focus group for navigation
    static uint32_t s_lastAnnouncement;   // Last status announcement time
};

// Accessibility event types for voice system integration
enum class AccessibilityEvent {
    FOCUS_CHANGED,
    SELECTION_CHANGED,
    STATE_CHANGED,
    ERROR_OCCURRED,
    SUCCESS_NOTIFICATION
};

// Callback type for accessibility events
typedef void (*accessibility_event_cb_t)(AccessibilityEvent event, lv_obj_t* obj, const char* message);

// Global accessibility event handler
extern accessibility_event_cb_t g_accessibility_callback;

#endif // ACCESSIBILITY_UTILS_H