#ifndef MY_APP_H
#define MY_APP_H

#include "../../src/apps/base_app.h"

/**
 * @file my_app.h
 * @brief Basic App Template for M5Stack Tab5 OS
 * 
 * This template provides a minimal application structure following
 * the M5Stack Tab5 OS app development standard.
 */

class MyApp : public BaseApp {
public:
    /**
     * @brief Constructor
     * Initialize app with basic metadata
     */
    MyApp();
    
    /**
     * @brief Destructor
     */
    ~MyApp() override = default;

    // BaseApp interface implementation
    os_error_t initialize() override;
    os_error_t update(uint32_t deltaTime) override;
    os_error_t shutdown() override;
    os_error_t createUI(lv_obj_t* parent) override;
    os_error_t destroyUI() override;

private:
    /**
     * @brief Create the main user interface
     */
    void createMainUI();
    
    /**
     * @brief Handle button click events
     * @param button The button that was clicked
     */
    void handleButtonClick(lv_obj_t* button);
    
    /**
     * @brief Update status display
     * @param message Status message to display
     */
    void updateStatus(const char* message);
    
    // UI event callbacks (must be static)
    static void buttonCallback(lv_event_t* e);
    
    // UI elements
    lv_obj_t* m_mainButton = nullptr;
    lv_obj_t* m_statusLabel = nullptr;
    lv_obj_t* m_counterLabel = nullptr;
    
    // App state
    bool m_isActive = false;
    uint32_t m_clickCount = 0;
    uint32_t m_lastUpdateTime = 0;
};

/**
 * @brief Factory function for creating app instances
 * Required for dynamic loading by the app manager
 * @return Unique pointer to app instance
 */
extern "C" std::unique_ptr<BaseApp> createMyApp();

#endif // MY_APP_H