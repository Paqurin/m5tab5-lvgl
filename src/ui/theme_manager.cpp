#include "theme_manager.h"
#include <esp_log.h>

static const char* TAG = "ThemeManager";

ThemeManager::~ThemeManager() {
    shutdown();
}

os_error_t ThemeManager::initialize() {
    if (m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Initializing Theme Manager");

    // Initialize all themes
    initializeLightTheme();
    initializeDarkTheme();
    initializeHighContrastDarkTheme();
    initializeHighContrastLightTheme();
    initializeAmberTheme();
    initializeColorblindTheme();

    // Set default theme
    setTheme(ThemeType::LIGHT);

    m_initialized = true;
    ESP_LOGI(TAG, "Theme Manager initialized");

    return OS_OK;
}

os_error_t ThemeManager::shutdown() {
    if (!m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Shutting down Theme Manager");

    // Cleanup themes would go here if needed
    m_initialized = false;

    ESP_LOGI(TAG, "Theme Manager shutdown complete");
    return OS_OK;
}

os_error_t ThemeManager::update(uint32_t deltaTime) {
    // Auto theme switching logic would go here
    return OS_OK;
}

os_error_t ThemeManager::setTheme(ThemeType theme) {
    if (!m_initialized) {
        return OS_ERROR_GENERIC;
    }

    m_currentTheme = theme;

    // Apply theme to current display
    lv_theme_t* activeTheme = nullptr;
    switch (theme) {
        case ThemeType::LIGHT:
            activeTheme = m_lightTheme;
            break;
        case ThemeType::DARK:
            activeTheme = m_darkTheme;
            break;
        case ThemeType::HIGH_CONTRAST_DARK:
            activeTheme = m_highContrastDarkTheme;
            break;
        case ThemeType::HIGH_CONTRAST_LIGHT:
            activeTheme = m_highContrastLightTheme;
            break;
        case ThemeType::HIGH_CONTRAST_AMBER:
            activeTheme = m_amberTheme;
            break;
        case ThemeType::COLORBLIND_FRIENDLY:
            activeTheme = m_colorblindTheme;
            break;
        case ThemeType::AUTO:
            // TODO: Determine based on time/sensor
            activeTheme = m_lightTheme;
            break;
    }

    if (activeTheme) {
        lv_disp_set_theme(nullptr, activeTheme);
    }

    ESP_LOGI(TAG, "Set theme to %d", (int)theme);
    return OS_OK;
}

os_error_t ThemeManager::applyTheme(lv_obj_t* obj) {
    if (!obj) {
        return OS_ERROR_GENERIC;
    }
    
    // Apply accessibility styles if enabled
    if (m_accessibilityConfig.isEnabled) {
        applyAccessibilityStyles(obj);
    }
    
    // Apply theme-specific colors
    lv_obj_set_style_bg_color(obj, getBackgroundColor(), LV_PART_MAIN);
    lv_obj_set_style_text_color(obj, getTextColor(), LV_PART_MAIN);
    
    // Apply enhanced focus styles for high contrast themes
    if (isHighContrastMode()) {
        lv_obj_set_style_outline_width(obj, getFocusIndicatorWidth(), LV_STATE_FOCUSED);
        lv_obj_set_style_outline_color(obj, getPrimaryColor(), LV_STATE_FOCUSED);
    }
    
    return OS_OK;
}

lv_color_t ThemeManager::getPrimaryColor() const {
    switch (m_currentTheme) {
        case ThemeType::DARK:
            return lv_color_hex(0x3498DB);
        case ThemeType::HIGH_CONTRAST_DARK:
            return lv_color_hex(0xFFFFFF);  // Pure white for maximum contrast
        case ThemeType::HIGH_CONTRAST_LIGHT:
            return lv_color_hex(0x000000);  // Pure black for maximum contrast
        case ThemeType::HIGH_CONTRAST_AMBER:
            return lv_color_hex(0xFFB000);  // High contrast amber
        case ThemeType::COLORBLIND_FRIENDLY:
            return lv_color_hex(0x0077BE);  // Blue that's visible to colorblind users
        default:
            return lv_color_hex(0x2980B9);
    }
}

lv_color_t ThemeManager::getSecondaryColor() const {
    switch (m_currentTheme) {
        case ThemeType::DARK:
            return lv_color_hex(0x95A5A6);
        case ThemeType::HIGH_CONTRAST_DARK:
            return lv_color_hex(0xCCCCCC);  // Light gray for secondary elements
        case ThemeType::HIGH_CONTRAST_LIGHT:
            return lv_color_hex(0x333333);  // Dark gray for secondary elements
        case ThemeType::HIGH_CONTRAST_AMBER:
            return lv_color_hex(0xFF8800);  // Darker amber for secondary
        case ThemeType::COLORBLIND_FRIENDLY:
            return lv_color_hex(0x009E73);  // Green that's visible to colorblind users
        default:
            return lv_color_hex(0x7F8C8D);
    }
}

lv_color_t ThemeManager::getBackgroundColor() const {
    switch (m_currentTheme) {
        case ThemeType::DARK:
            return lv_color_hex(0x2C3E50);
        case ThemeType::HIGH_CONTRAST_DARK:
            return lv_color_hex(0x000000);  // Pure black background
        case ThemeType::HIGH_CONTRAST_LIGHT:
            return lv_color_hex(0xFFFFFF);  // Pure white background
        case ThemeType::HIGH_CONTRAST_AMBER:
            return lv_color_hex(0x1A1A1A);  // Very dark background for amber
        case ThemeType::COLORBLIND_FRIENDLY:
            return lv_color_hex(0xF5F5F5);  // Light neutral background
        default:
            return lv_color_hex(0xECF0F1);
    }
}

lv_color_t ThemeManager::getTextColor() const {
    switch (m_currentTheme) {
        case ThemeType::DARK:
            return lv_color_hex(0xECF0F1);
        case ThemeType::HIGH_CONTRAST_DARK:
            return lv_color_hex(0xFFFFFF);  // Pure white text
        case ThemeType::HIGH_CONTRAST_LIGHT:
            return lv_color_hex(0x000000);  // Pure black text
        case ThemeType::HIGH_CONTRAST_AMBER:
            return lv_color_hex(0xFFB000);  // Amber text
        case ThemeType::COLORBLIND_FRIENDLY:
            return lv_color_hex(0x2C3E50);  // High contrast dark text
        default:
            return lv_color_hex(0x2C3E50);
    }
}

void ThemeManager::initializeLightTheme() {
    m_lightTheme = lv_theme_default_init(nullptr, 
                                        lv_color_hex(0x2980B9),  // Primary
                                        lv_color_hex(0x7F8C8D),  // Secondary
                                        false,                    // Dark mode
                                        lv_font_default());
}

void ThemeManager::initializeDarkTheme() {
    m_darkTheme = lv_theme_default_init(nullptr,
                                       lv_color_hex(0x3498DB),   // Primary
                                       lv_color_hex(0x95A5A6),   // Secondary
                                       true,                     // Dark mode
                                       lv_font_default());
}

void ThemeManager::initializeHighContrastDarkTheme() {
    m_highContrastDarkTheme = lv_theme_default_init(nullptr,
                                                    lv_color_hex(0xFFFFFF),  // Pure white primary
                                                    lv_color_hex(0xCCCCCC),  // Light gray secondary
                                                    true,                    // Dark mode
                                                    &lv_font_montserrat_16); // Larger default font
}

void ThemeManager::initializeHighContrastLightTheme() {
    m_highContrastLightTheme = lv_theme_default_init(nullptr,
                                                     lv_color_hex(0x000000),  // Pure black primary
                                                     lv_color_hex(0x333333),  // Dark gray secondary
                                                     false,                   // Light mode
                                                     &lv_font_montserrat_16); // Larger default font
}

void ThemeManager::initializeAmberTheme() {
    m_amberTheme = lv_theme_default_init(nullptr,
                                        lv_color_hex(0xFFB000),  // Amber primary
                                        lv_color_hex(0xFF8800),  // Darker amber secondary
                                        true,                    // Dark background
                                        &lv_font_montserrat_16); // Larger default font
}

void ThemeManager::initializeColorblindTheme() {
    m_colorblindTheme = lv_theme_default_init(nullptr,
                                             lv_color_hex(0x0077BE),  // Colorblind-safe blue
                                             lv_color_hex(0x009E73),  // Colorblind-safe green
                                             false,                   // Light mode
                                             &lv_font_montserrat_16); // Larger default font
}

os_error_t ThemeManager::enableAccessibilityMode(const AccessibilityConfig& config) {
    m_accessibilityConfig = config;
    m_accessibilityConfig.isEnabled = true;
    
    // Switch to preferred accessibility theme
    setTheme(config.preferredTheme);
    
    ESP_LOGI(TAG, "Accessibility mode enabled");
    return OS_OK;
}

os_error_t ThemeManager::disableAccessibilityMode() {
    m_accessibilityConfig.isEnabled = false;
    
    // Return to standard light theme
    setTheme(ThemeType::LIGHT);
    
    ESP_LOGI(TAG, "Accessibility mode disabled");
    return OS_OK;
}

os_error_t ThemeManager::toggleAccessibilityMode() {
    if (m_accessibilityConfig.isEnabled) {
        return disableAccessibilityMode();
    } else {
        AccessibilityConfig config;
        config.largeFonts = true;
        config.boldBorders = true;
        config.enhancedFocus = true;
        config.largeTouchTargets = true;
        config.preferredTheme = ThemeType::HIGH_CONTRAST_DARK;
        return enableAccessibilityMode(config);
    }
}

const lv_font_t* ThemeManager::getAccessibilityFont(uint8_t baseSize) const {
    uint8_t scaledSize = getScaledFontSize(baseSize);
    
    // Return appropriate font based on scaled size
    if (scaledSize >= 48) return &lv_font_montserrat_48;
    if (scaledSize >= 40) return &lv_font_montserrat_40;
    if (scaledSize >= 36) return &lv_font_montserrat_36;
    if (scaledSize >= 32) return &lv_font_montserrat_32;
    if (scaledSize >= 28) return &lv_font_montserrat_28;
    if (scaledSize >= 26) return &lv_font_montserrat_26;
    if (scaledSize >= 24) return &lv_font_montserrat_24;
    if (scaledSize >= 22) return &lv_font_montserrat_22;
    if (scaledSize >= 20) return &lv_font_montserrat_20;
    if (scaledSize >= 18) return &lv_font_montserrat_18;
    if (scaledSize >= 16) return &lv_font_montserrat_16;
    if (scaledSize >= 14) return &lv_font_montserrat_14;
    return &lv_font_montserrat_12;
}

lv_coord_t ThemeManager::getBorderWidth() const {
    if (m_accessibilityConfig.isEnabled && m_accessibilityConfig.boldBorders) {
        return 3;  // Thick borders for accessibility
    }
    return 1;  // Standard border width
}

lv_coord_t ThemeManager::getFocusIndicatorWidth() const {
    if (m_accessibilityConfig.isEnabled && m_accessibilityConfig.enhancedFocus) {
        return 4;  // Extra thick focus indicators
    }
    return 2;  // Standard focus indicator
}

lv_coord_t ThemeManager::getMinTouchTargetSize() const {
    if (m_accessibilityConfig.isEnabled && m_accessibilityConfig.largeTouchTargets) {
        return 44;  // WCAG recommended minimum
    }
    return 32;  // Standard touch target
}

bool ThemeManager::isHighContrastMode() const {
    return m_currentTheme == ThemeType::HIGH_CONTRAST_DARK ||
           m_currentTheme == ThemeType::HIGH_CONTRAST_LIGHT ||
           m_currentTheme == ThemeType::HIGH_CONTRAST_AMBER ||
           m_currentTheme == ThemeType::COLORBLIND_FRIENDLY;
}

lv_color_t ThemeManager::getErrorColor() const {
    switch (m_currentTheme) {
        case ThemeType::HIGH_CONTRAST_DARK:
            return lv_color_hex(0xFF6666);  // Light red for dark background
        case ThemeType::HIGH_CONTRAST_LIGHT:
            return lv_color_hex(0xCC0000);  // Dark red for light background
        case ThemeType::HIGH_CONTRAST_AMBER:
            return lv_color_hex(0xFF4444);  // Bright red for amber theme
        case ThemeType::COLORBLIND_FRIENDLY:
            return lv_color_hex(0xD55E00);  // Orange-red visible to colorblind
        default:
            return lv_color_hex(0xE74C3C);  // Standard red
    }
}

lv_color_t ThemeManager::getSuccessColor() const {
    switch (m_currentTheme) {
        case ThemeType::HIGH_CONTRAST_DARK:
            return lv_color_hex(0x66FF66);  // Light green for dark background
        case ThemeType::HIGH_CONTRAST_LIGHT:
            return lv_color_hex(0x006600);  // Dark green for light background
        case ThemeType::HIGH_CONTRAST_AMBER:
            return lv_color_hex(0x88FF88);  // Bright green for amber theme
        case ThemeType::COLORBLIND_FRIENDLY:
            return lv_color_hex(0x009E73);  // Teal visible to colorblind
        default:
            return lv_color_hex(0x27AE60);  // Standard green
    }
}

lv_color_t ThemeManager::getWarningColor() const {
    switch (m_currentTheme) {
        case ThemeType::HIGH_CONTRAST_DARK:
            return lv_color_hex(0xFFDD44);  // Light yellow for dark background
        case ThemeType::HIGH_CONTRAST_LIGHT:
            return lv_color_hex(0xCC8800);  // Dark orange for light background
        case ThemeType::HIGH_CONTRAST_AMBER:
            return lv_color_hex(0xFFCC00);  // Bright yellow for amber theme
        case ThemeType::COLORBLIND_FRIENDLY:
            return lv_color_hex(0xF0E442);  // Yellow visible to colorblind
        default:
            return lv_color_hex(0xF39C12);  // Standard orange
    }
}

void ThemeManager::applyAccessibilityStyles(lv_obj_t* obj) {
    if (!m_accessibilityConfig.isEnabled || !obj) {
        return;
    }
    
    // Apply enhanced border for interactive elements
    if (lv_obj_has_flag(obj, LV_OBJ_FLAG_CLICKABLE)) {
        lv_obj_set_style_border_width(obj, getBorderWidth(), LV_PART_MAIN);
        lv_obj_set_style_border_color(obj, getPrimaryColor(), LV_PART_MAIN);
    }
    
    // Ensure minimum touch target size
    lv_coord_t minSize = getMinTouchTargetSize();
    if (lv_obj_get_width(obj) < minSize || lv_obj_get_height(obj) < minSize) {
        lv_obj_set_size(obj, LV_MAX(lv_obj_get_width(obj), minSize), 
                            LV_MAX(lv_obj_get_height(obj), minSize));
    }
    
    // Enhanced focus indicators
    lv_obj_set_style_outline_width(obj, getFocusIndicatorWidth(), LV_STATE_FOCUSED);
    lv_obj_set_style_outline_color(obj, getPrimaryColor(), LV_STATE_FOCUSED);
    lv_obj_set_style_outline_pad(obj, 2, LV_STATE_FOCUSED);
}

uint8_t ThemeManager::getScaledFontSize(uint8_t baseSize) const {
    if (m_accessibilityConfig.isEnabled && m_accessibilityConfig.largeFonts) {
        return (uint8_t)(baseSize * 1.25f);  // 125% scaling for accessibility
    }
    return baseSize;
}