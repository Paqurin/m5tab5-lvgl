#ifndef THEME_MANAGER_H
#define THEME_MANAGER_H

#include "../system/os_config.h"
#include <lvgl.h>
#include <string>

/**
 * @file theme_manager.h
 * @brief Theme management system for M5Stack Tab5
 * 
 * Manages UI themes, colors, fonts, and visual styling.
 */

enum class ThemeType {
    LIGHT,
    DARK,
    AUTO,  // Automatic based on time or ambient light
    HIGH_CONTRAST_DARK,   // White text on black background
    HIGH_CONTRAST_LIGHT,  // Black text on white background
    HIGH_CONTRAST_AMBER,  // Amber text on dark background
    COLORBLIND_FRIENDLY   // High contrast with patterns for colorblind users
};

/**
 * @brief Accessibility configuration structure
 */
struct AccessibilityConfig {
    bool isEnabled = false;
    bool largeFonts = false;      // 125% font scaling
    bool boldBorders = false;     // Thick borders on interactive elements
    bool enhancedFocus = false;   // Extra thick focus indicators
    bool largeTouchTargets = false; // Minimum 44x44px touch targets
    ThemeType preferredTheme = ThemeType::HIGH_CONTRAST_DARK;
};

class ThemeManager {
public:
    ThemeManager() = default;
    ~ThemeManager();

    /**
     * @brief Initialize theme manager
     * @return OS_OK on success, error code on failure
     */
    os_error_t initialize();

    /**
     * @brief Shutdown theme manager
     * @return OS_OK on success, error code on failure
     */
    os_error_t shutdown();

    /**
     * @brief Update theme manager
     * @param deltaTime Time elapsed since last update in milliseconds
     * @return OS_OK on success, error code on failure
     */
    os_error_t update(uint32_t deltaTime);

    /**
     * @brief Set active theme
     * @param theme Theme type to activate
     * @return OS_OK on success, error code on failure
     */
    os_error_t setTheme(ThemeType theme);

    /**
     * @brief Get current theme type
     * @return Current theme type
     */
    ThemeType getCurrentTheme() const { return m_currentTheme; }

    /**
     * @brief Apply theme to an object
     * @param obj LVGL object to apply theme to
     * @return OS_OK on success, error code on failure
     */
    os_error_t applyTheme(lv_obj_t* obj);

    /**
     * @brief Get primary color for current theme
     * @return Primary color
     */
    lv_color_t getPrimaryColor() const;

    /**
     * @brief Get secondary color for current theme
     * @return Secondary color
     */
    lv_color_t getSecondaryColor() const;

    /**
     * @brief Get background color for current theme
     * @return Background color
     */
    lv_color_t getBackgroundColor() const;

    /**
     * @brief Get text color for current theme
     * @return Text color
     */
    lv_color_t getTextColor() const;

    /**
     * @brief Enable accessibility mode
     * @param config Accessibility configuration
     * @return OS_OK on success, error code on failure
     */
    os_error_t enableAccessibilityMode(const AccessibilityConfig& config);

    /**
     * @brief Disable accessibility mode
     * @return OS_OK on success, error code on failure
     */
    os_error_t disableAccessibilityMode();

    /**
     * @brief Get current accessibility configuration
     * @return Current accessibility config
     */
    const AccessibilityConfig& getAccessibilityConfig() const { return m_accessibilityConfig; }

    /**
     * @brief Toggle accessibility mode quickly
     * @return OS_OK on success, error code on failure
     */
    os_error_t toggleAccessibilityMode();

    /**
     * @brief Get font for accessibility mode
     * @param baseSize Base font size
     * @return Appropriate font for accessibility
     */
    const lv_font_t* getAccessibilityFont(uint8_t baseSize) const;

    /**
     * @brief Get border width for interactive elements
     * @return Border width in pixels
     */
    lv_coord_t getBorderWidth() const;

    /**
     * @brief Get focus indicator width
     * @return Focus border width in pixels
     */
    lv_coord_t getFocusIndicatorWidth() const;

    /**
     * @brief Get minimum touch target size
     * @return Minimum size in pixels
     */
    lv_coord_t getMinTouchTargetSize() const;

    /**
     * @brief Check if current theme is high contrast
     * @return True if using high contrast theme
     */
    bool isHighContrastMode() const;

    /**
     * @brief Get error color for current theme
     * @return Error color with high contrast
     */
    lv_color_t getErrorColor() const;

    /**
     * @brief Get success color for current theme
     * @return Success color with high contrast
     */
    lv_color_t getSuccessColor() const;

    /**
     * @brief Get warning color for current theme
     * @return Warning color with high contrast
     */
    lv_color_t getWarningColor() const;

private:
    /**
     * @brief Initialize light theme
     */
    void initializeLightTheme();

    /**
     * @brief Initialize dark theme
     */
    void initializeDarkTheme();

    /**
     * @brief Initialize high contrast dark theme
     */
    void initializeHighContrastDarkTheme();

    /**
     * @brief Initialize high contrast light theme
     */
    void initializeHighContrastLightTheme();

    /**
     * @brief Initialize amber high contrast theme
     */
    void initializeAmberTheme();

    /**
     * @brief Initialize colorblind friendly theme
     */
    void initializeColorblindTheme();

    /**
     * @brief Apply accessibility styles to an object
     * @param obj LVGL object to style
     */
    void applyAccessibilityStyles(lv_obj_t* obj);

    /**
     * @brief Get scaled font size for accessibility
     * @param baseSize Base font size
     * @return Scaled font size
     */
    uint8_t getScaledFontSize(uint8_t baseSize) const;

    ThemeType m_currentTheme = ThemeType::LIGHT;
    lv_theme_t* m_lightTheme = nullptr;
    lv_theme_t* m_darkTheme = nullptr;
    lv_theme_t* m_highContrastDarkTheme = nullptr;
    lv_theme_t* m_highContrastLightTheme = nullptr;
    lv_theme_t* m_amberTheme = nullptr;
    lv_theme_t* m_colorblindTheme = nullptr;
    AccessibilityConfig m_accessibilityConfig;
    bool m_initialized = false;
};

#endif // THEME_MANAGER_H