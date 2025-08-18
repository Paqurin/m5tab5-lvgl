#include "accessibility_utils.h"
#include <esp_log.h>
#include <cmath>

static const char* TAG = "AccessibilityUtils";

// Static member initialization
lv_group_t* AccessibilityUtils::s_mainFocusGroup = nullptr;
uint32_t AccessibilityUtils::s_lastAnnouncement = 0;

// Global accessibility callback
accessibility_event_cb_t g_accessibility_callback = nullptr;

lv_obj_t* AccessibilityUtils::createAccessibleButton(lv_obj_t* parent, const char* text, ThemeManager* themeManager) {
    lv_obj_t* btn = lv_btn_create(parent);
    
    // Set minimum touch target size
    lv_coord_t minSize = themeManager->getMinTouchTargetSize();
    lv_obj_set_size(btn, LV_SIZE_CONTENT, minSize);
    lv_obj_set_style_min_width(btn, minSize, LV_PART_MAIN);
    lv_obj_set_style_min_height(btn, minSize, LV_PART_MAIN);
    
    // Apply theme styling
    themeManager->applyTheme(btn);
    
    // Enhanced border and focus styling
    lv_obj_set_style_border_width(btn, themeManager->getBorderWidth(), LV_PART_MAIN);
    lv_obj_set_style_border_color(btn, themeManager->getPrimaryColor(), LV_PART_MAIN);
    
    // Enhanced focus indicator
    lv_obj_set_style_outline_width(btn, themeManager->getFocusIndicatorWidth(), LV_STATE_FOCUSED);
    lv_obj_set_style_outline_color(btn, themeManager->getPrimaryColor(), LV_STATE_FOCUSED);
    lv_obj_set_style_outline_pad(btn, 3, LV_STATE_FOCUSED);
    
    // High contrast colors for accessibility themes
    if (themeManager->isHighContrastMode()) {
        lv_obj_set_style_bg_color(btn, themeManager->getBackgroundColor(), LV_PART_MAIN);
        lv_obj_set_style_text_color(btn, themeManager->getTextColor(), LV_PART_MAIN);
        
        // Hover states
        lv_obj_set_style_bg_color(btn, themeManager->getPrimaryColor(), LV_STATE_PRESSED);
        lv_obj_set_style_text_color(btn, themeManager->getBackgroundColor(), LV_STATE_PRESSED);
    }
    
    // Create label with accessible font
    lv_obj_t* label = lv_label_create(btn);
    lv_label_set_text(label, text);
    
    // Use accessibility font if needed
    const lv_font_t* font = themeManager->getAccessibilityFont(16);
    lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
    
    lv_obj_center(label);
    
    ESP_LOGI(TAG, "Created accessible button: %s", text);
    return btn;
}

lv_obj_t* AccessibilityUtils::createAccessibleLabel(lv_obj_t* parent, const char* text, ThemeManager* themeManager) {
    lv_obj_t* label = lv_label_create(parent);
    lv_label_set_text(label, text);
    
    // Apply theme colors
    lv_obj_set_style_text_color(label, themeManager->getTextColor(), LV_PART_MAIN);
    
    // Use accessibility font
    const lv_font_t* font = themeManager->getAccessibilityFont(16);
    lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
    
    // Enhanced text contrast for high contrast themes
    if (themeManager->isHighContrastMode()) {
        // Add text shadow for better readability
        lv_obj_set_style_text_opa(label, LV_OPA_100, LV_PART_MAIN);
    }
    
    return label;
}

lv_obj_t* AccessibilityUtils::createAccessibleInput(lv_obj_t* parent, const char* placeholder, ThemeManager* themeManager) {
    lv_obj_t* textarea = lv_textarea_create(parent);
    lv_textarea_set_placeholder_text(textarea, placeholder);
    
    // Set minimum size for touch targets
    lv_coord_t minSize = themeManager->getMinTouchTargetSize();
    lv_obj_set_size(textarea, LV_SIZE_CONTENT, minSize);
    lv_obj_set_style_min_height(textarea, minSize, LV_PART_MAIN);
    
    // Apply theme styling
    themeManager->applyTheme(textarea);
    
    // Enhanced border
    lv_obj_set_style_border_width(textarea, themeManager->getBorderWidth(), LV_PART_MAIN);
    lv_obj_set_style_border_color(textarea, themeManager->getSecondaryColor(), LV_PART_MAIN);
    
    // Focus styling
    lv_obj_set_style_border_color(textarea, themeManager->getPrimaryColor(), LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(textarea, themeManager->getFocusIndicatorWidth(), LV_STATE_FOCUSED);
    lv_obj_set_style_outline_color(textarea, themeManager->getPrimaryColor(), LV_STATE_FOCUSED);
    
    // Accessibility font
    const lv_font_t* font = themeManager->getAccessibilityFont(16);
    lv_obj_set_style_text_font(textarea, font, LV_PART_MAIN);
    
    // High contrast styling
    if (themeManager->isHighContrastMode()) {
        lv_obj_set_style_bg_color(textarea, themeManager->getBackgroundColor(), LV_PART_MAIN);
        lv_obj_set_style_text_color(textarea, themeManager->getTextColor(), LV_PART_MAIN);
    }
    
    return textarea;
}

lv_obj_t* AccessibilityUtils::createAccessibleSwitch(lv_obj_t* parent, ThemeManager* themeManager) {
    lv_obj_t* sw = lv_switch_create(parent);
    
    // Set minimum touch target size
    lv_coord_t minSize = themeManager->getMinTouchTargetSize();
    lv_obj_set_size(sw, minSize * 1.5, minSize);  // Switches are typically wider
    
    // Apply theme styling
    themeManager->applyTheme(sw);
    
    // Enhanced contrast for switch states
    if (themeManager->isHighContrastMode()) {
        // Off state
        lv_obj_set_style_bg_color(sw, themeManager->getBackgroundColor(), LV_PART_MAIN);
        lv_obj_set_style_border_color(sw, themeManager->getTextColor(), LV_PART_MAIN);
        lv_obj_set_style_border_width(sw, themeManager->getBorderWidth(), LV_PART_MAIN);
        
        // On state
        lv_obj_set_style_bg_color(sw, themeManager->getPrimaryColor(), (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_CHECKED));
        
        // Indicator (knob)
        lv_obj_set_style_bg_color(sw, themeManager->getTextColor(), LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(sw, themeManager->getBackgroundColor(), (lv_style_selector_t)(LV_PART_INDICATOR | LV_STATE_CHECKED));
    }
    
    // Enhanced focus indicator
    lv_obj_set_style_outline_width(sw, themeManager->getFocusIndicatorWidth(), LV_STATE_FOCUSED);
    lv_obj_set_style_outline_color(sw, themeManager->getPrimaryColor(), LV_STATE_FOCUSED);
    
    return sw;
}

lv_obj_t* AccessibilityUtils::createAccessibleSlider(lv_obj_t* parent, ThemeManager* themeManager) {
    lv_obj_t* slider = lv_slider_create(parent);
    
    // Set minimum touch target size for the knob
    lv_coord_t minSize = themeManager->getMinTouchTargetSize();
    lv_obj_set_size(slider, 200, minSize);  // Wide slider with minimum height
    
    // Apply theme styling
    themeManager->applyTheme(slider);
    
    // Enhanced contrast styling
    if (themeManager->isHighContrastMode()) {
        // Main track
        lv_obj_set_style_bg_color(slider, themeManager->getBackgroundColor(), LV_PART_MAIN);
        lv_obj_set_style_border_color(slider, themeManager->getTextColor(), LV_PART_MAIN);
        lv_obj_set_style_border_width(slider, themeManager->getBorderWidth(), LV_PART_MAIN);
        
        // Filled part (indicator)
        lv_obj_set_style_bg_color(slider, themeManager->getPrimaryColor(), LV_PART_INDICATOR);
        
        // Knob
        lv_obj_set_style_bg_color(slider, themeManager->getTextColor(), LV_PART_KNOB);
        lv_obj_set_style_border_color(slider, themeManager->getPrimaryColor(), LV_PART_KNOB);
        lv_obj_set_style_border_width(slider, themeManager->getBorderWidth(), LV_PART_KNOB);
    }
    
    // Enhanced focus indicator
    lv_obj_set_style_outline_width(slider, themeManager->getFocusIndicatorWidth(), LV_STATE_FOCUSED);
    lv_obj_set_style_outline_color(slider, themeManager->getPrimaryColor(), LV_STATE_FOCUSED);
    
    return slider;
}

lv_group_t* AccessibilityUtils::createFocusGroup(lv_obj_t** objects, size_t count) {
    lv_group_t* group = lv_group_create();
    
    for (size_t i = 0; i < count; i++) {
        if (objects[i]) {
            lv_group_add_obj(group, objects[i]);
        }
    }
    
    // Set up keyboard navigation
    setupAccessibilityShortcuts(group);
    
    ESP_LOGI(TAG, "Created focus group with %zu objects", count);
    return group;
}

void AccessibilityUtils::addVoiceSupport(lv_obj_t* obj, const char* description) {
    if (!obj || !description) {
        return;
    }
    
    // Store description as user data for screen reader access
    lv_obj_set_user_data(obj, (void*)description);
    
    // Set up event callback for voice announcements
    // This would integrate with the Talkback voice system
    ESP_LOGI(TAG, "Added voice support: %s", description);
}

void AccessibilityUtils::announceStatus(const char* message, uint8_t priority) {
    if (!message) {
        return;
    }
    
    uint32_t currentTime = lv_tick_get();
    
    // Rate limiting for announcements
    if (currentTime - s_lastAnnouncement < 500 && priority < 2) {
        return;  // Skip low priority announcements if too frequent
    }
    
    s_lastAnnouncement = currentTime;
    
    // Trigger accessibility callback if available
    if (g_accessibility_callback) {
        g_accessibility_callback(AccessibilityEvent::STATE_CHANGED, nullptr, message);
    }
    
    ESP_LOGI(TAG, "Status announcement (priority %d): %s", priority, message);
}

bool AccessibilityUtils::checkContrastRatio(lv_color_t foreground, lv_color_t background, uint8_t level) {
    float fg_luminance = calculateLuminance(foreground);
    float bg_luminance = calculateLuminance(background);
    
    // Calculate contrast ratio
    float lighter = fmax(fg_luminance, bg_luminance);
    float darker = fmin(fg_luminance, bg_luminance);
    float contrast = (lighter + 0.05f) / (darker + 0.05f);
    
    // WCAG standards: AA = 4.5:1, AAA = 7:1
    float requiredRatio = (level == 2) ? 7.0f : 4.5f;
    
    return contrast >= requiredRatio;
}

float AccessibilityUtils::calculateLuminance(lv_color_t color) {
    // Convert to RGB values (0-1)
    float r = lv_color_brightness(color) / 255.0f;
    float g = lv_color_brightness(color) / 255.0f;  // Simplified for RGB565
    float b = lv_color_brightness(color) / 255.0f;
    
    // Apply gamma correction
    auto gamma = [](float c) {
        return (c <= 0.03928f) ? c / 12.92f : powf((c + 0.055f) / 1.055f, 2.4f);
    };
    
    r = gamma(r);
    g = gamma(g);
    b = gamma(b);
    
    // Calculate relative luminance
    return 0.2126f * r + 0.7152f * g + 0.0722f * b;
}

void AccessibilityUtils::applyColorblindPattern(lv_obj_t* obj, uint8_t patternType) {
    if (!obj) {
        return;
    }
    
    // This would apply visual patterns for colorblind users
    // For now, we'll add distinctive borders and textures
    switch (patternType) {
        case 0: // Dots pattern
            lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_FULL, LV_PART_MAIN);
            lv_obj_set_style_border_post(obj, true, LV_PART_MAIN);
            break;
        case 1: // Stripes pattern
            lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_HOR, LV_PART_MAIN);
            break;
        case 2: // Crosshatch pattern
            lv_obj_set_style_border_width(obj, 3, LV_PART_MAIN);
            lv_obj_set_style_outline_width(obj, 1, LV_PART_MAIN);
            break;
    }
    
    ESP_LOGI(TAG, "Applied colorblind pattern %d", patternType);
}

void AccessibilityUtils::setupAccessibilityShortcuts(lv_group_t* group) {
    if (!group) {
        return;
    }
    
    // Store main focus group reference
    if (!s_mainFocusGroup) {
        s_mainFocusGroup = group;
    }
    
    // Set up keyboard navigation
    lv_group_set_wrap(group, true);  // Allow wrapping around
    
    ESP_LOGI(TAG, "Set up accessibility shortcuts for focus group");
}

lv_obj_t* AccessibilityUtils::createAccessibleDialog(lv_obj_t* parent, const char* title, 
                                                    const char* message, ThemeManager* themeManager) {
    lv_obj_t* msgbox = lv_msgbox_create(parent, title, message, nullptr, true);
    
    // Apply theme styling
    themeManager->applyTheme(msgbox);
    
    // Enhanced visibility for high contrast themes
    if (themeManager->isHighContrastMode()) {
        lv_obj_set_style_bg_color(msgbox, themeManager->getBackgroundColor(), LV_PART_MAIN);
        lv_obj_set_style_text_color(msgbox, themeManager->getTextColor(), LV_PART_MAIN);
        lv_obj_set_style_border_width(msgbox, themeManager->getBorderWidth() * 2, LV_PART_MAIN);
        lv_obj_set_style_border_color(msgbox, themeManager->getPrimaryColor(), LV_PART_MAIN);
    }
    
    // Use accessibility fonts
    const lv_font_t* titleFont = themeManager->getAccessibilityFont(20);
    const lv_font_t* textFont = themeManager->getAccessibilityFont(16);
    
    // Apply to header and content
    lv_obj_set_style_text_font(lv_msgbox_get_title(msgbox), titleFont, LV_PART_MAIN);
    lv_obj_set_style_text_font(lv_msgbox_get_text(msgbox), textFont, LV_PART_MAIN);
    
    // Announce dialog creation
    announceStatus("Dialog opened", 2);
    
    ESP_LOGI(TAG, "Created accessible dialog: %s", title);
    return msgbox;
}