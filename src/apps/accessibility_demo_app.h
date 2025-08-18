#ifndef ACCESSIBILITY_DEMO_APP_H
#define ACCESSIBILITY_DEMO_APP_H

#include "../system/os_config.h"
#include "../ui/ui_manager.h"
#include "../ui/theme_manager.h"
#include "../ui/accessibility_utils.h"
#include <lvgl.h>

/**
 * @file accessibility_demo_app.h
 * @brief Accessibility demonstration application for M5Stack Tab5
 * 
 * Showcases all accessibility features including high contrast themes,
 * voice navigation, large touch targets, and keyboard shortcuts.
 */

class AccessibilityDemoApp {
public:
    AccessibilityDemoApp() = default;
    ~AccessibilityDemoApp();

    /**
     * @brief Initialize the accessibility demo app
     * @param uiManager UI manager instance
     * @param themeManager Theme manager instance
     * @return OS_OK on success, error code on failure
     */
    os_error_t initialize(UIManager* uiManager, ThemeManager* themeManager);

    /**
     * @brief Shutdown the app
     * @return OS_OK on success, error code on failure
     */
    os_error_t shutdown();

    /**
     * @brief Update the app
     * @param deltaTime Time elapsed since last update in milliseconds
     * @return OS_OK on success, error code on failure
     */
    os_error_t update(uint32_t deltaTime);

    /**
     * @brief Show the app interface
     * @return OS_OK on success, error code on failure
     */
    os_error_t show();

    /**
     * @brief Hide the app interface
     * @return OS_OK on success, error code on failure
     */
    os_error_t hide();

    /**
     * @brief Handle input events
     * @param event Input event
     * @return OS_OK if handled, OS_ERROR_NOT_HANDLED if not
     */
    os_error_t handleInput(const input_event_t& event);

private:
    /**
     * @brief Create the main interface
     */
    void createInterface();

    /**
     * @brief Create theme selection section
     */
    void createThemeSection();

    /**
     * @brief Create accessibility controls section
     */
    void createAccessibilitySection();

    /**
     * @brief Create UI elements demonstration section
     */
    void createDemoSection();

    /**
     * @brief Create voice navigation demo
     */
    void createVoiceSection();

    /**
     * @brief Handle theme change
     * @param theme New theme type
     */
    void onThemeChanged(ThemeType theme);

    /**
     * @brief Handle accessibility toggle
     * @param enabled Whether accessibility is enabled
     */
    void onAccessibilityToggled(bool enabled);

    /**
     * @brief Handle voice command test
     */
    void onVoiceTest();

    /**
     * @brief Update all UI elements with current theme
     */
    void updateUIElements();

    /**
     * @brief Setup keyboard navigation
     */
    void setupKeyboardNavigation();

    // Event handlers
    static void themeButtonEventHandler(lv_event_t* e);
    static void accessibilityToggleHandler(lv_event_t* e);
    static void voiceTestHandler(lv_event_t* e);
    static void demoButtonHandler(lv_event_t* e);

    UIManager* m_uiManager = nullptr;
    ThemeManager* m_themeManager = nullptr;
    lv_obj_t* m_containerObj = nullptr;
    lv_group_t* m_focusGroup = nullptr;
    bool m_initialized = false;
    bool m_isVisible = false;

    // UI Elements
    lv_obj_t* m_themeSection = nullptr;
    lv_obj_t* m_accessibilitySection = nullptr;
    lv_obj_t* m_demoSection = nullptr;
    lv_obj_t* m_voiceSection = nullptr;

    // Theme buttons
    lv_obj_t* m_lightThemeBtn = nullptr;
    lv_obj_t* m_darkThemeBtn = nullptr;
    lv_obj_t* m_highContrastDarkBtn = nullptr;
    lv_obj_t* m_highContrastLightBtn = nullptr;
    lv_obj_t* m_amberThemeBtn = nullptr;
    lv_obj_t* m_colorblindThemeBtn = nullptr;

    // Accessibility controls
    lv_obj_t* m_accessibilitySwitch = nullptr;
    lv_obj_t* m_largeFontsSwitch = nullptr;
    lv_obj_t* m_boldBordersSwitch = nullptr;
    lv_obj_t* m_largeTouchSwitch = nullptr;

    // Demo elements
    lv_obj_t* m_demoButton = nullptr;
    lv_obj_t* m_demoSlider = nullptr;
    lv_obj_t* m_demoInput = nullptr;
    lv_obj_t* m_demoSwitch = nullptr;

    // Voice elements
    lv_obj_t* m_voiceTestBtn = nullptr;
    lv_obj_t* m_voiceStatusLabel = nullptr;

    // Status display
    lv_obj_t* m_statusLabel = nullptr;
    lv_obj_t* m_contrastLabel = nullptr;
};

#endif // ACCESSIBILITY_DEMO_APP_H