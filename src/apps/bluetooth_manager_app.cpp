#include "bluetooth_manager_app.h"
#include <esp_log.h>
#include <cstdio>

// Static instance for event handlers
BluetoothManagerApp* BluetoothManagerApp::s_instance = nullptr;

BluetoothManagerApp::BluetoothManagerApp() :
    BaseApp("bluetooth_manager", "Bluetooth Manager", "1.0.0"),
    m_bluetoothService(nullptr),
    m_currentPage(UIPage::MAIN_MENU),
    m_previousPage(UIPage::MAIN_MENU),
    m_mainContainer(nullptr),
    m_headerBar(nullptr),
    m_headerTitle(nullptr),
    m_backButton(nullptr),
    m_contentArea(nullptr),
    m_navigationBar(nullptr),
    m_messageBox(nullptr),
    m_discoveryInProgress(false),
    m_discoveryStartTime(0),
    m_lastUIUpdate(0),
    m_messageDisplayTime(0),
    m_messageVisible(false) {
    
    // Set app properties
    setPriority(AppPriority::APP_NORMAL);
    setDescription("Comprehensive Bluetooth device manager with pairing and connection management");
    setAuthor("M5Stack Tab5 Team");
    
    s_instance = this;
}

BluetoothManagerApp::~BluetoothManagerApp() {
    shutdown();
    s_instance = nullptr;
}

os_error_t BluetoothManagerApp::initialize() {
    if (m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Initializing Bluetooth Manager App");

    // Initialize Bluetooth service
    m_bluetoothService = new BluetoothService();
    if (!m_bluetoothService) {
        ESP_LOGE(TAG, "Failed to create Bluetooth service");
        return OS_ERROR_NO_MEMORY;
    }

    BluetoothServiceConfig config;
    config.autoDiscovery = false;  // Manual discovery only
    config.autoConnect = true;
    config.deviceName = "M5Stack Tab5";

    os_error_t result = m_bluetoothService->initialize(config);
    if (result != OS_OK) {
        ESP_LOGE(TAG, "Failed to initialize Bluetooth service");
        delete m_bluetoothService;
        m_bluetoothService = nullptr;
        return result;
    }

    // Register event callback
    m_bluetoothService->setEventCallback([this](const BluetoothServiceEventData& event) {
        this->onBluetoothServiceEvent(event);
    });

    BaseApp::initialize();
    setMemoryUsage(81920); // 80KB for Bluetooth management

    ESP_LOGI(TAG, "Bluetooth Manager App initialized successfully");
    return OS_OK;
}

os_error_t BluetoothManagerApp::start() {
    if (!m_initialized) {
        return OS_ERROR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Starting Bluetooth Manager App");
    
    setState(AppState::RUNNING);
    return OS_OK;
}

os_error_t BluetoothManagerApp::stop() {
    if (getState() != AppState::RUNNING) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Stopping Bluetooth Manager App");
    
    // Stop any ongoing discovery
    if (m_discoveryInProgress && m_bluetoothService) {
        m_bluetoothService->stopDeviceDiscovery();
    }

    setState(AppState::STOPPED);
    return OS_OK;
}

os_error_t BluetoothManagerApp::pause() {
    if (getState() != AppState::RUNNING) {
        return OS_ERROR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Pausing Bluetooth Manager App");
    setState(AppState::PAUSED);
    return OS_OK;
}

os_error_t BluetoothManagerApp::resume() {
    if (getState() != AppState::PAUSED) {
        return OS_ERROR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Resuming Bluetooth Manager App");
    setState(AppState::RUNNING);
    return OS_OK;
}

os_error_t BluetoothManagerApp::update(uint32_t deltaTime) {
    if (!m_initialized || !m_bluetoothService) {
        return OS_ERROR_INVALID_STATE;
    }

    m_lastUIUpdate += deltaTime;

    // Update Bluetooth service
    m_bluetoothService->update(deltaTime);

    // Update UI periodically
    if (m_lastUIUpdate >= UI_UPDATE_INTERVAL) {
        switch (m_currentPage) {
            case UIPage::MAIN_MENU:
                updateMainMenu();
                break;
            case UIPage::DEVICE_DISCOVERY:
                updateDiscoveryPage();
                break;
            case UIPage::PAIRED_DEVICES:
                updatePairedDevicesPage();
                break;
            case UIPage::DEVICE_DETAILS:
                updateDeviceDetailsPage();
                break;
            case UIPage::CONNECTION_STATUS:
                updateConnectionStatusPage();
                break;
            default:
                break;
        }
        
        updateConnectionIndicators();
        m_lastUIUpdate = 0;
    }

    // Handle message timeout
    if (m_messageVisible) {
        m_messageDisplayTime += deltaTime;
        if (m_messageDisplayTime >= MESSAGE_DISPLAY_TIME) {
            hideMessage();
        }
    }

    return OS_OK;
}

os_error_t BluetoothManagerApp::createUI(lv_obj_t* parent) {
    if (!parent || !m_initialized) {
        return OS_ERROR_INVALID_PARAM;
    }

    ESP_LOGI(TAG, "Creating Bluetooth Manager UI");

    // Create main container
    m_mainContainer = lv_obj_create(parent);
    lv_obj_set_size(m_mainContainer, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(m_mainContainer, 0, 0);
    lv_obj_set_style_border_width(m_mainContainer, 0, 0);

    // Create header bar
    createHeaderBar("Bluetooth Manager", false);

    // Create content area
    m_contentArea = lv_obj_create(m_mainContainer);
    lv_obj_set_size(m_contentArea, LV_PCT(100), LV_PCT(85));
    lv_obj_align_to(m_contentArea, m_headerBar, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_pad_all(m_contentArea, 5, 0);
    lv_obj_set_style_border_width(m_contentArea, 0, 0);

    // Create navigation bar
    createNavigationBar();

    // Create all pages (hidden initially)
    createMainMenu();
    createDiscoveryPage();
    createPairedDevicesPage();
    createDeviceDetailsPage();
    createConnectionStatusPage();
    createSettingsPage();

    // Show main menu
    showPage(UIPage::MAIN_MENU);

    ESP_LOGI(TAG, "Bluetooth Manager UI created successfully");
    return OS_OK;
}

os_error_t BluetoothManagerApp::destroyUI() {
    if (m_mainContainer) {
        lv_obj_del(m_mainContainer);
        m_mainContainer = nullptr;
    }

    // Clear references to deleted objects
    m_headerBar = nullptr;
    m_contentArea = nullptr;
    m_navigationBar = nullptr;
    m_messageBox = nullptr;

    ESP_LOGI(TAG, "Bluetooth Manager UI destroyed");
    return OS_OK;
}

os_error_t BluetoothManagerApp::shutdown() {
    if (!m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Shutting down Bluetooth Manager App");

    // Stop the app first
    stop();

    // Destroy UI
    destroyUI();

    // Shutdown Bluetooth service
    if (m_bluetoothService) {
        m_bluetoothService->shutdown();
        delete m_bluetoothService;
        m_bluetoothService = nullptr;
    }

    BaseApp::shutdown();

    ESP_LOGI(TAG, "Bluetooth Manager App shutdown complete");
    return OS_OK;
}

void BluetoothManagerApp::createMainMenu() {
    m_mainMenuPage = lv_obj_create(m_contentArea);
    lv_obj_set_size(m_mainMenuPage, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(m_mainMenuPage, 10, 0);
    lv_obj_set_style_border_width(m_mainMenuPage, 0, 0);

    // Create menu buttons
    lv_obj_t* discoveryBtn = lv_btn_create(m_mainMenuPage);
    lv_obj_set_size(discoveryBtn, LV_PCT(90), 60);
    lv_obj_align(discoveryBtn, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_add_event_cb(discoveryBtn, onDiscoveryButtonClicked, LV_EVENT_CLICKED, this);

    lv_obj_t* discoveryLabel = lv_label_create(discoveryBtn);
    lv_label_set_text(discoveryLabel, LV_SYMBOL_EYE_OPEN " Discover Devices");
    lv_obj_center(discoveryLabel);

    lv_obj_t* pairedBtn = lv_btn_create(m_mainMenuPage);
    lv_obj_set_size(pairedBtn, LV_PCT(90), 60);
    lv_obj_align_to(pairedBtn, discoveryBtn, LV_ALIGN_OUT_BOTTOM_MID, 0, 15);
    lv_obj_add_event_cb(pairedBtn, onPairedDevicesButtonClicked, LV_EVENT_CLICKED, this);

    lv_obj_t* pairedLabel = lv_label_create(pairedBtn);
    lv_label_set_text(pairedLabel, LV_SYMBOL_LIST " Paired Devices");
    lv_obj_center(pairedLabel);

    lv_obj_t* statusBtn = lv_btn_create(m_mainMenuPage);
    lv_obj_set_size(statusBtn, LV_PCT(90), 60);
    lv_obj_align_to(statusBtn, pairedBtn, LV_ALIGN_OUT_BOTTOM_MID, 0, 15);
    lv_obj_add_event_cb(statusBtn, onStatusButtonClicked, LV_EVENT_CLICKED, this);

    lv_obj_t* statusLabel = lv_label_create(statusBtn);
    lv_label_set_text(statusLabel, LV_SYMBOL_WIFI " Connection Status");
    lv_obj_center(statusLabel);

    lv_obj_t* settingsBtn = lv_btn_create(m_mainMenuPage);
    lv_obj_set_size(settingsBtn, LV_PCT(90), 60);
    lv_obj_align_to(settingsBtn, statusBtn, LV_ALIGN_OUT_BOTTOM_MID, 0, 15);
    lv_obj_add_event_cb(settingsBtn, onSettingsButtonClicked, LV_EVENT_CLICKED, this);

    lv_obj_t* settingsLabel = lv_label_create(settingsBtn);
    lv_label_set_text(settingsLabel, LV_SYMBOL_SETTINGS " Settings");
    lv_obj_center(settingsLabel);

    lv_obj_add_flag(m_mainMenuPage, LV_OBJ_FLAG_HIDDEN);
}

void BluetoothManagerApp::createDiscoveryPage() {
    m_discoveryPage = lv_obj_create(m_contentArea);
    lv_obj_set_size(m_discoveryPage, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(m_discoveryPage, 5, 0);
    lv_obj_set_style_border_width(m_discoveryPage, 0, 0);

    // Control buttons
    lv_obj_t* buttonContainer = lv_obj_create(m_discoveryPage);
    lv_obj_set_size(buttonContainer, LV_PCT(100), 50);
    lv_obj_align(buttonContainer, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_set_style_border_width(buttonContainer, 0, 0);
    lv_obj_set_flex_flow(buttonContainer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(buttonContainer, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    m_discoveryStartButton = lv_btn_create(buttonContainer);
    lv_obj_set_size(m_discoveryStartButton, 120, 40);
    lv_obj_add_event_cb(m_discoveryStartButton, onStartDiscoveryClicked, LV_EVENT_CLICKED, this);

    lv_obj_t* startLabel = lv_label_create(m_discoveryStartButton);
    lv_label_set_text(startLabel, "Start Scan");
    lv_obj_center(startLabel);

    m_discoveryStopButton = lv_btn_create(buttonContainer);
    lv_obj_set_size(m_discoveryStopButton, 120, 40);
    lv_obj_add_event_cb(m_discoveryStopButton, onStopDiscoveryClicked, LV_EVENT_CLICKED, this);
    lv_obj_add_state(m_discoveryStopButton, LV_STATE_DISABLED);

    lv_obj_t* stopLabel = lv_label_create(m_discoveryStopButton);
    lv_label_set_text(stopLabel, "Stop Scan");
    lv_obj_center(stopLabel);

    // Status area
    lv_obj_t* statusContainer = lv_obj_create(m_discoveryPage);
    lv_obj_set_size(statusContainer, LV_PCT(100), 40);
    lv_obj_align_to(statusContainer, buttonContainer, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
    lv_obj_set_style_border_width(statusContainer, 0, 0);

    m_discoverySpinner = lv_spinner_create(statusContainer, 1000, 60);
    lv_obj_set_size(m_discoverySpinner, 30, 30);
    lv_obj_align(m_discoverySpinner, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_add_flag(m_discoverySpinner, LV_OBJ_FLAG_HIDDEN);

    m_discoveryStatus = lv_label_create(statusContainer);
    lv_label_set_text(m_discoveryStatus, "Ready to scan for devices");
    lv_obj_align_to(m_discoveryStatus, m_discoverySpinner, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

    // Device list
    m_discoveryList = lv_obj_create(m_discoveryPage);
    lv_obj_set_size(m_discoveryList, LV_PCT(100), LV_PCT(70));
    lv_obj_align_to(m_discoveryList, statusContainer, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
    lv_obj_set_style_pad_all(m_discoveryList, 5, 0);

    lv_obj_add_flag(m_discoveryPage, LV_OBJ_FLAG_HIDDEN);
}

void BluetoothManagerApp::createPairedDevicesPage() {
    m_pairedDevicesPage = lv_obj_create(m_contentArea);
    lv_obj_set_size(m_pairedDevicesPage, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(m_pairedDevicesPage, 5, 0);
    lv_obj_set_style_border_width(m_pairedDevicesPage, 0, 0);

    // Header with device count
    lv_obj_t* headerContainer = lv_obj_create(m_pairedDevicesPage);
    lv_obj_set_size(headerContainer, LV_PCT(100), 40);
    lv_obj_align(headerContainer, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_set_style_border_width(headerContainer, 0, 0);

    m_pairedDevicesCount = lv_label_create(headerContainer);
    lv_label_set_text(m_pairedDevicesCount, "Paired Devices: 0");
    lv_obj_align(m_pairedDevicesCount, LV_ALIGN_LEFT_MID, 10, 0);

    // Device list
    m_pairedDevicesList = lv_obj_create(m_pairedDevicesPage);
    lv_obj_set_size(m_pairedDevicesList, LV_PCT(100), LV_PCT(85));
    lv_obj_align_to(m_pairedDevicesList, headerContainer, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
    lv_obj_set_style_pad_all(m_pairedDevicesList, 5, 0);

    lv_obj_add_flag(m_pairedDevicesPage, LV_OBJ_FLAG_HIDDEN);
}

void BluetoothManagerApp::createDeviceDetailsPage() {
    m_deviceDetailsPage = lv_obj_create(m_contentArea);
    lv_obj_set_size(m_deviceDetailsPage, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(m_deviceDetailsPage, 10, 0);
    lv_obj_set_style_border_width(m_deviceDetailsPage, 0, 0);

    // Device info section
    lv_obj_t* infoContainer = lv_obj_create(m_deviceDetailsPage);
    lv_obj_set_size(infoContainer, LV_PCT(100), 120);
    lv_obj_align(infoContainer, LV_ALIGN_TOP_MID, 0, 0);

    m_detailsDeviceName = lv_label_create(infoContainer);
    lv_label_set_text(m_detailsDeviceName, "Device Name");
    lv_obj_align(m_detailsDeviceName, LV_ALIGN_TOP_LEFT, 10, 10);

    m_detailsDeviceAddress = lv_label_create(infoContainer);
    lv_label_set_text(m_detailsDeviceAddress, "00:00:00:00:00:00");
    lv_obj_align_to(m_detailsDeviceAddress, m_detailsDeviceName, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

    m_detailsDeviceType = lv_label_create(infoContainer);
    lv_label_set_text(m_detailsDeviceType, "Unknown Device");
    lv_obj_align_to(m_detailsDeviceType, m_detailsDeviceAddress, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

    m_detailsConnectionStatus = lv_label_create(infoContainer);
    lv_label_set_text(m_detailsConnectionStatus, "Disconnected");
    lv_obj_align_to(m_detailsConnectionStatus, m_detailsDeviceType, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

    // Control section
    lv_obj_t* controlContainer = lv_obj_create(m_deviceDetailsPage);
    lv_obj_set_size(controlContainer, LV_PCT(100), 200);
    lv_obj_align_to(controlContainer, infoContainer, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    // Auto-connect switch
    lv_obj_t* autoConnectContainer = lv_obj_create(controlContainer);
    lv_obj_set_size(autoConnectContainer, LV_PCT(100), 40);
    lv_obj_align(autoConnectContainer, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_border_width(autoConnectContainer, 0, 0);

    lv_obj_t* autoConnectLabel = lv_label_create(autoConnectContainer);
    lv_label_set_text(autoConnectLabel, "Auto Connect:");
    lv_obj_align(autoConnectLabel, LV_ALIGN_LEFT_MID, 10, 0);

    m_detailsAutoConnectSwitch = lv_switch_create(autoConnectContainer);
    lv_obj_align(m_detailsAutoConnectSwitch, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_add_event_cb(m_detailsAutoConnectSwitch, onAutoConnectSwitchChanged, LV_EVENT_VALUE_CHANGED, this);

    // Trusted device switch
    lv_obj_t* trustedContainer = lv_obj_create(controlContainer);
    lv_obj_set_size(trustedContainer, LV_PCT(100), 40);
    lv_obj_align_to(trustedContainer, autoConnectContainer, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
    lv_obj_set_style_border_width(trustedContainer, 0, 0);

    lv_obj_t* trustedLabel = lv_label_create(trustedContainer);
    lv_label_set_text(trustedLabel, "Trusted Device:");
    lv_obj_align(trustedLabel, LV_ALIGN_LEFT_MID, 10, 0);

    m_detailsTrustedSwitch = lv_switch_create(trustedContainer);
    lv_obj_align(m_detailsTrustedSwitch, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_add_event_cb(m_detailsTrustedSwitch, onTrustedSwitchChanged, LV_EVENT_VALUE_CHANGED, this);

    // Connection buttons
    lv_obj_t* buttonContainer = lv_obj_create(controlContainer);
    lv_obj_set_size(buttonContainer, LV_PCT(100), 50);
    lv_obj_align_to(buttonContainer, trustedContainer, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_set_style_border_width(buttonContainer, 0, 0);
    lv_obj_set_flex_flow(buttonContainer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(buttonContainer, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    m_detailsConnectButton = lv_btn_create(buttonContainer);
    lv_obj_set_size(m_detailsConnectButton, 100, 40);
    lv_obj_add_event_cb(m_detailsConnectButton, onDeviceConnectClicked, LV_EVENT_CLICKED, this);

    lv_obj_t* connectLabel = lv_label_create(m_detailsConnectButton);
    lv_label_set_text(connectLabel, "Connect");
    lv_obj_center(connectLabel);

    m_detailsDisconnectButton = lv_btn_create(buttonContainer);
    lv_obj_set_size(m_detailsDisconnectButton, 100, 40);
    lv_obj_add_event_cb(m_detailsDisconnectButton, onDeviceDisconnectClicked, LV_EVENT_CLICKED, this);

    lv_obj_t* disconnectLabel = lv_label_create(m_detailsDisconnectButton);
    lv_label_set_text(disconnectLabel, "Disconnect");
    lv_obj_center(disconnectLabel);

    m_detailsRemoveButton = lv_btn_create(buttonContainer);
    lv_obj_set_size(m_detailsRemoveButton, 80, 40);
    lv_obj_add_event_cb(m_detailsRemoveButton, onDeviceRemoveClicked, LV_EVENT_CLICKED, this);
    lv_obj_set_style_bg_color(m_detailsRemoveButton, COLOR_ERROR, 0);

    lv_obj_t* removeLabel = lv_label_create(m_detailsRemoveButton);
    lv_label_set_text(removeLabel, "Remove");
    lv_obj_center(removeLabel);

    lv_obj_add_flag(m_deviceDetailsPage, LV_OBJ_FLAG_HIDDEN);
}

void BluetoothManagerApp::createConnectionStatusPage() {
    m_statusPage = lv_obj_create(m_contentArea);
    lv_obj_set_size(m_statusPage, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(m_statusPage, 10, 0);
    lv_obj_set_style_border_width(m_statusPage, 0, 0);

    // Connection indicators
    lv_obj_t* indicatorContainer = lv_obj_create(m_statusPage);
    lv_obj_set_size(indicatorContainer, LV_PCT(100), 120);
    lv_obj_align(indicatorContainer, LV_ALIGN_TOP_MID, 0, 10);

    // Keyboard indicator
    m_statusKeyboardIndicator = lv_obj_create(indicatorContainer);
    lv_obj_set_size(m_statusKeyboardIndicator, LV_PCT(30), 60);
    lv_obj_align(m_statusKeyboardIndicator, LV_ALIGN_TOP_LEFT, 5, 5);

    lv_obj_t* keyboardIcon = lv_label_create(m_statusKeyboardIndicator);
    lv_label_set_text(keyboardIcon, LV_SYMBOL_KEYBOARD);
    lv_obj_align(keyboardIcon, LV_ALIGN_TOP_MID, 0, 5);

    lv_obj_t* keyboardLabel = lv_label_create(m_statusKeyboardIndicator);
    lv_label_set_text(keyboardLabel, "Keyboard");
    lv_obj_align_to(keyboardLabel, keyboardIcon, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);

    // Mouse indicator
    m_statusMouseIndicator = lv_obj_create(indicatorContainer);
    lv_obj_set_size(m_statusMouseIndicator, LV_PCT(30), 60);
    lv_obj_align(m_statusMouseIndicator, LV_ALIGN_TOP_MID, 0, 5);

    lv_obj_t* mouseIcon = lv_label_create(m_statusMouseIndicator);
    lv_label_set_text(mouseIcon, LV_SYMBOL_GPS);
    lv_obj_align(mouseIcon, LV_ALIGN_TOP_MID, 0, 5);

    lv_obj_t* mouseLabel = lv_label_create(m_statusMouseIndicator);
    lv_label_set_text(mouseLabel, "Mouse");
    lv_obj_align_to(mouseLabel, mouseIcon, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);

    // Audio indicator
    m_statusAudioIndicator = lv_obj_create(indicatorContainer);
    lv_obj_set_size(m_statusAudioIndicator, LV_PCT(30), 60);
    lv_obj_align(m_statusAudioIndicator, LV_ALIGN_TOP_RIGHT, -5, 5);

    lv_obj_t* audioIcon = lv_label_create(m_statusAudioIndicator);
    lv_label_set_text(audioIcon, LV_SYMBOL_AUDIO);
    lv_obj_align(audioIcon, LV_ALIGN_TOP_MID, 0, 5);

    lv_obj_t* audioLabel = lv_label_create(m_statusAudioIndicator);
    lv_label_set_text(audioLabel, "Audio");
    lv_obj_align_to(audioLabel, audioIcon, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);

    // Connection count
    m_statusConnectionCount = lv_label_create(m_statusPage);
    lv_label_set_text(m_statusConnectionCount, "Active Connections: 0");
    lv_obj_align_to(m_statusConnectionCount, indicatorContainer, LV_ALIGN_OUT_BOTTOM_LEFT, 5, 20);

    // Statistics area
    m_statusStatsContainer = lv_obj_create(m_statusPage);
    lv_obj_set_size(m_statusStatsContainer, LV_PCT(100), LV_PCT(50));
    lv_obj_align_to(m_statusStatsContainer, m_statusConnectionCount, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);

    lv_obj_add_flag(m_statusPage, LV_OBJ_FLAG_HIDDEN);
}

void BluetoothManagerApp::createSettingsPage() {
    m_settingsPage = lv_obj_create(m_contentArea);
    lv_obj_set_size(m_settingsPage, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(m_settingsPage, 10, 0);
    lv_obj_set_style_border_width(m_settingsPage, 0, 0);

    // Auto-discovery setting
    lv_obj_t* autoDiscoveryContainer = lv_obj_create(m_settingsPage);
    lv_obj_set_size(autoDiscoveryContainer, LV_PCT(100), 50);
    lv_obj_align(autoDiscoveryContainer, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_border_width(autoDiscoveryContainer, 0, 0);

    lv_obj_t* autoDiscoveryLabel = lv_label_create(autoDiscoveryContainer);
    lv_label_set_text(autoDiscoveryLabel, "Auto Discovery:");
    lv_obj_align(autoDiscoveryLabel, LV_ALIGN_LEFT_MID, 10, 0);

    m_settingsAutoDiscoverySwitch = lv_switch_create(autoDiscoveryContainer);
    lv_obj_align(m_settingsAutoDiscoverySwitch, LV_ALIGN_RIGHT_MID, -10, 0);

    // Auto-connect setting
    lv_obj_t* autoConnectContainer = lv_obj_create(m_settingsPage);
    lv_obj_set_size(autoConnectContainer, LV_PCT(100), 50);
    lv_obj_align_to(autoConnectContainer, autoDiscoveryContainer, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_set_style_border_width(autoConnectContainer, 0, 0);

    lv_obj_t* autoConnectLabel = lv_label_create(autoConnectContainer);
    lv_label_set_text(autoConnectLabel, "Auto Connect:");
    lv_obj_align(autoConnectLabel, LV_ALIGN_LEFT_MID, 10, 0);

    m_settingsAutoConnectSwitch = lv_switch_create(autoConnectContainer);
    lv_obj_align(m_settingsAutoConnectSwitch, LV_ALIGN_RIGHT_MID, -10, 0);

    // Device name setting
    lv_obj_t* deviceNameContainer = lv_obj_create(m_settingsPage);
    lv_obj_set_size(deviceNameContainer, LV_PCT(100), 70);
    lv_obj_align_to(deviceNameContainer, autoConnectContainer, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_set_style_border_width(deviceNameContainer, 0, 0);

    lv_obj_t* deviceNameLabel = lv_label_create(deviceNameContainer);
    lv_label_set_text(deviceNameLabel, "Device Name:");
    lv_obj_align(deviceNameLabel, LV_ALIGN_TOP_LEFT, 10, 10);

    m_settingsDeviceNameInput = lv_textarea_create(deviceNameContainer);
    lv_obj_set_size(m_settingsDeviceNameInput, LV_PCT(80), 40);
    lv_obj_align_to(m_settingsDeviceNameInput, deviceNameLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    lv_textarea_set_text(m_settingsDeviceNameInput, "M5Stack Tab5");
    lv_textarea_set_one_line(m_settingsDeviceNameInput, true);

    // Clear profiles button
    m_settingsClearProfilesButton = lv_btn_create(m_settingsPage);
    lv_obj_set_size(m_settingsClearProfilesButton, LV_PCT(80), 50);
    lv_obj_align_to(m_settingsClearProfilesButton, deviceNameContainer, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
    lv_obj_set_style_bg_color(m_settingsClearProfilesButton, COLOR_ERROR, 0);

    lv_obj_t* clearLabel = lv_label_create(m_settingsClearProfilesButton);
    lv_label_set_text(clearLabel, "Clear All Paired Devices");
    lv_obj_center(clearLabel);

    lv_obj_add_flag(m_settingsPage, LV_OBJ_FLAG_HIDDEN);
}

void BluetoothManagerApp::createHeaderBar(const std::string& title, bool showBackButton) {
    m_headerBar = lv_obj_create(m_mainContainer);
    lv_obj_set_size(m_headerBar, LV_PCT(100), 50);
    lv_obj_align(m_headerBar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(m_headerBar, COLOR_BLUETOOTH, 0);
    lv_obj_set_style_border_width(m_headerBar, 0, 0);

    if (showBackButton) {
        m_backButton = lv_btn_create(m_headerBar);
        lv_obj_set_size(m_backButton, 40, 40);
        lv_obj_align(m_backButton, LV_ALIGN_LEFT_MID, 5, 0);
        lv_obj_add_event_cb(m_backButton, onBackButtonClicked, LV_EVENT_CLICKED, this);

        lv_obj_t* backIcon = lv_label_create(m_backButton);
        lv_label_set_text(backIcon, LV_SYMBOL_LEFT);
        lv_obj_center(backIcon);
    }

    m_headerTitle = lv_label_create(m_headerBar);
    lv_label_set_text(m_headerTitle, title.c_str());
    lv_obj_set_style_text_color(m_headerTitle, lv_color_white(), 0);
    lv_obj_center(m_headerTitle);
}

void BluetoothManagerApp::createNavigationBar() {
    m_navigationBar = lv_obj_create(m_mainContainer);
    lv_obj_set_size(m_navigationBar, LV_PCT(100), 50);
    lv_obj_align(m_navigationBar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(m_navigationBar, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_width(m_navigationBar, 0, 0);

    // Navigation buttons would be created here
    // For now, just a simple indicator
    lv_obj_t* indicator = lv_obj_create(m_navigationBar);
    lv_obj_set_size(indicator, LV_PCT(90), 4);
    lv_obj_align(indicator, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_set_style_bg_color(indicator, COLOR_BLUETOOTH, 0);
}

// Event handlers implementation
void BluetoothManagerApp::onDiscoveryButtonClicked(lv_event_t* e) {
    BluetoothManagerApp* app = static_cast<BluetoothManagerApp*>(lv_event_get_user_data(e));
    if (app) {
        app->showPage(UIPage::DEVICE_DISCOVERY);
    }
}

void BluetoothManagerApp::onPairedDevicesButtonClicked(lv_event_t* e) {
    BluetoothManagerApp* app = static_cast<BluetoothManagerApp*>(lv_event_get_user_data(e));
    if (app) {
        app->showPage(UIPage::PAIRED_DEVICES);
    }
}

void BluetoothManagerApp::onSettingsButtonClicked(lv_event_t* e) {
    BluetoothManagerApp* app = static_cast<BluetoothManagerApp*>(lv_event_get_user_data(e));
    if (app) {
        app->showPage(UIPage::SETTINGS);
    }
}

void BluetoothManagerApp::onStatusButtonClicked(lv_event_t* e) {
    BluetoothManagerApp* app = static_cast<BluetoothManagerApp*>(lv_event_get_user_data(e));
    if (app) {
        app->showPage(UIPage::CONNECTION_STATUS);
    }
}

void BluetoothManagerApp::onBackButtonClicked(lv_event_t* e) {
    BluetoothManagerApp* app = static_cast<BluetoothManagerApp*>(lv_event_get_user_data(e));
    if (app) {
        app->navigateBack();
    }
}

void BluetoothManagerApp::onStartDiscoveryClicked(lv_event_t* e) {
    BluetoothManagerApp* app = static_cast<BluetoothManagerApp*>(lv_event_get_user_data(e));
    if (app && app->m_bluetoothService) {
        app->m_bluetoothService->startDeviceDiscovery(30);
        app->m_discoveryInProgress = true;
        app->m_discoveryStartTime = 0; // Will be updated in next update cycle
        
        lv_obj_add_state(app->m_discoveryStartButton, LV_STATE_DISABLED);
        lv_obj_clear_state(app->m_discoveryStopButton, LV_STATE_DISABLED);
        lv_obj_clear_flag(app->m_discoverySpinner, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(app->m_discoveryStatus, "Scanning for devices...");
        
        app->clearDeviceList();
    }
}

void BluetoothManagerApp::onStopDiscoveryClicked(lv_event_t* e) {
    BluetoothManagerApp* app = static_cast<BluetoothManagerApp*>(lv_event_get_user_data(e));
    if (app && app->m_bluetoothService) {
        app->m_bluetoothService->stopDeviceDiscovery();
        app->m_discoveryInProgress = false;
        
        lv_obj_clear_state(app->m_discoveryStartButton, LV_STATE_DISABLED);
        lv_obj_add_state(app->m_discoveryStopButton, LV_STATE_DISABLED);
        lv_obj_add_flag(app->m_discoverySpinner, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(app->m_discoveryStatus, "Scan completed");
    }
}

// Additional event handlers and utility methods would continue here...
// Due to length constraints, I'm showing the pattern for the key methods

void BluetoothManagerApp::showPage(UIPage page) {
    // Hide current page
    switch (m_currentPage) {
        case UIPage::MAIN_MENU:
            if (m_mainMenuPage) lv_obj_add_flag(m_mainMenuPage, LV_OBJ_FLAG_HIDDEN);
            break;
        case UIPage::DEVICE_DISCOVERY:
            if (m_discoveryPage) lv_obj_add_flag(m_discoveryPage, LV_OBJ_FLAG_HIDDEN);
            break;
        case UIPage::PAIRED_DEVICES:
            if (m_pairedDevicesPage) lv_obj_add_flag(m_pairedDevicesPage, LV_OBJ_FLAG_HIDDEN);
            break;
        case UIPage::DEVICE_DETAILS:
            if (m_deviceDetailsPage) lv_obj_add_flag(m_deviceDetailsPage, LV_OBJ_FLAG_HIDDEN);
            break;
        case UIPage::CONNECTION_STATUS:
            if (m_statusPage) lv_obj_add_flag(m_statusPage, LV_OBJ_FLAG_HIDDEN);
            break;
        case UIPage::SETTINGS:
            if (m_settingsPage) lv_obj_add_flag(m_settingsPage, LV_OBJ_FLAG_HIDDEN);
            break;
    }

    // Show new page
    m_previousPage = m_currentPage;
    m_currentPage = page;

    switch (page) {
        case UIPage::MAIN_MENU:
            if (m_mainMenuPage) lv_obj_clear_flag(m_mainMenuPage, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(m_headerTitle, "Bluetooth Manager");
            if (m_backButton) lv_obj_add_flag(m_backButton, LV_OBJ_FLAG_HIDDEN);
            break;
        case UIPage::DEVICE_DISCOVERY:
            if (m_discoveryPage) lv_obj_clear_flag(m_discoveryPage, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(m_headerTitle, "Discover Devices");
            if (m_backButton) lv_obj_clear_flag(m_backButton, LV_OBJ_FLAG_HIDDEN);
            break;
        case UIPage::PAIRED_DEVICES:
            if (m_pairedDevicesPage) lv_obj_clear_flag(m_pairedDevicesPage, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(m_headerTitle, "Paired Devices");
            if (m_backButton) lv_obj_clear_flag(m_backButton, LV_OBJ_FLAG_HIDDEN);
            break;
        case UIPage::DEVICE_DETAILS:
            if (m_deviceDetailsPage) lv_obj_clear_flag(m_deviceDetailsPage, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(m_headerTitle, "Device Details");
            if (m_backButton) lv_obj_clear_flag(m_backButton, LV_OBJ_FLAG_HIDDEN);
            break;
        case UIPage::CONNECTION_STATUS:
            if (m_statusPage) lv_obj_clear_flag(m_statusPage, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(m_headerTitle, "Connection Status");
            if (m_backButton) lv_obj_clear_flag(m_backButton, LV_OBJ_FLAG_HIDDEN);
            break;
        case UIPage::SETTINGS:
            if (m_settingsPage) lv_obj_clear_flag(m_settingsPage, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(m_headerTitle, "Settings");
            if (m_backButton) lv_obj_clear_flag(m_backButton, LV_OBJ_FLAG_HIDDEN);
            break;
    }
}

void BluetoothManagerApp::navigateBack() {
    if (m_currentPage == UIPage::MAIN_MENU) {
        return; // Already at main menu
    }
    showPage(UIPage::MAIN_MENU);
}

void BluetoothManagerApp::updateMainMenu() {
    // Update main menu indicators or status if needed
}

void BluetoothManagerApp::updateDiscoveryPage() {
    if (m_discoveryInProgress && m_bluetoothService) {
        auto devices = m_bluetoothService->getDiscoveredDevices();
        populateDeviceList(devices, m_discoveryList);
        
        char statusText[64];
        snprintf(statusText, sizeof(statusText), "Found %d devices", devices.size());
        lv_label_set_text(m_discoveryStatus, statusText);
    }
}

void BluetoothManagerApp::updatePairedDevicesPage() {
    if (m_bluetoothService) {
        auto profiles = m_bluetoothService->getDeviceProfiles();
        
        char countText[64];
        snprintf(countText, sizeof(countText), "Paired Devices: %d", profiles.size());
        lv_label_set_text(m_pairedDevicesCount, countText);

        // Convert profiles to device info for display
        std::vector<BluetoothDeviceInfo> devices;
        for (const auto& profile : profiles) {
            BluetoothDeviceInfo device;
            device.address = profile.address;
            device.name = profile.customName.empty() ? profile.name : profile.customName;
            device.type = profile.type;
            device.paired = true;
            // Check if connected
            auto connectedDevices = m_bluetoothService->getConnectedDevices();
            device.connected = std::any_of(connectedDevices.begin(), connectedDevices.end(),
                                         [&profile](const BluetoothDeviceInfo& conn) {
                                             return conn.address == profile.address;
                                         });
            devices.push_back(device);
        }
        
        populateDeviceList(devices, m_pairedDevicesList);
    }
}

void BluetoothManagerApp::updateConnectionStatusPage() {
    if (m_bluetoothService) {
        // Update connection indicators
        lv_obj_set_style_bg_color(m_statusKeyboardIndicator, 
                                 m_bluetoothService->isKeyboardActive() ? COLOR_CONNECTED : COLOR_DISCONNECTED, 0);
        lv_obj_set_style_bg_color(m_statusMouseIndicator,
                                 m_bluetoothService->isMouseActive() ? COLOR_CONNECTED : COLOR_DISCONNECTED, 0);
        lv_obj_set_style_bg_color(m_statusAudioIndicator,
                                 m_bluetoothService->isAudioDeviceActive() ? COLOR_CONNECTED : COLOR_DISCONNECTED, 0);

        // Update connection count
        auto connectedDevices = m_bluetoothService->getConnectedDevices();
        char countText[64];
        snprintf(countText, sizeof(countText), "Active Connections: %d", connectedDevices.size());
        lv_label_set_text(m_statusConnectionCount, countText);
    }
}

// Utility methods
std::string BluetoothManagerApp::getDeviceTypeString(BluetoothDeviceType type) {
    switch (type) {
        case BluetoothDeviceType::KEYBOARD: return "Keyboard";
        case BluetoothDeviceType::MOUSE: return "Mouse";
        case BluetoothDeviceType::COMBO_HID: return "Keyboard & Mouse";
        case BluetoothDeviceType::HEADPHONES: return "Headphones";
        case BluetoothDeviceType::SPEAKERS: return "Speakers";
        case BluetoothDeviceType::GAMEPAD: return "Gamepad";
        case BluetoothDeviceType::SERIAL_PORT: return "Serial Port";
        case BluetoothDeviceType::PHONE: return "Phone";
        default: return "Unknown Device";
    }
}

void BluetoothManagerApp::populateDeviceList(const std::vector<BluetoothDeviceInfo>& devices, lv_obj_t* parent) {
    // Clear existing items
    lv_obj_clean(parent);
    
    // Add devices to list
    for (const auto& device : devices) {
        createDeviceListItem(device, parent);
    }
}

void BluetoothManagerApp::createDeviceListItem(const BluetoothDeviceInfo& device, lv_obj_t* parent) {
    lv_obj_t* item = lv_obj_create(parent);
    lv_obj_set_size(item, LV_PCT(98), 80);
    lv_obj_set_style_pad_all(item, 10, 0);
    
    // Device name
    lv_obj_t* nameLabel = lv_label_create(item);
    lv_label_set_text(nameLabel, device.name.c_str());
    lv_obj_align(nameLabel, LV_ALIGN_TOP_LEFT, 5, 5);
    
    // Device address and type
    lv_obj_t* infoLabel = lv_label_create(item);
    std::string info = device.address + " - " + getDeviceTypeString(device.type);
    lv_label_set_text(infoLabel, info.c_str());
    lv_obj_align_to(infoLabel, nameLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 2);
    
    // Connection status
    lv_obj_t* statusLabel = lv_label_create(item);
    lv_label_set_text(statusLabel, device.connected ? "Connected" : (device.paired ? "Paired" : "Available"));
    lv_obj_align(statusLabel, LV_ALIGN_TOP_RIGHT, -5, 5);
    lv_obj_set_style_text_color(statusLabel, device.connected ? COLOR_CONNECTED : COLOR_DISCONNECTED, 0);
    
    // RSSI bar (for discovered devices)
    if (!device.paired && device.rssi != 0) {
        lv_obj_t* rssiBar = lv_bar_create(item);
        lv_obj_set_size(rssiBar, 60, 8);
        lv_obj_align(rssiBar, LV_ALIGN_BOTTOM_RIGHT, -5, -5);
        int rssiPercent = std::max(0, std::min(100, static_cast<int>((device.rssi + 100) * 2))); // Convert RSSI to percentage
        lv_bar_set_value(rssiBar, rssiPercent, LV_ANIM_OFF);
    }
}

void BluetoothManagerApp::clearDeviceList() {
    if (m_discoveryList) {
        lv_obj_clean(m_discoveryList);
    }
}

void BluetoothManagerApp::updateConnectionIndicators() {
    // This method is called regularly to update any connection status indicators
    // Implementation would update visual indicators based on current connection states
}

void BluetoothManagerApp::onBluetoothServiceEvent(const BluetoothServiceEventData& event) {
    ESP_LOGI(TAG, "Bluetooth service event: %d for device %s", 
             static_cast<int>(event.event), event.deviceAddress.c_str());
    
    switch (event.event) {
        case BluetoothServiceEvent::DEVICE_FOUND:
            if (m_currentPage == UIPage::DEVICE_DISCOVERY) {
                // Update discovery page
                updateDiscoveryPage();
            }
            break;
            
        case BluetoothServiceEvent::DEVICE_CONNECTED:
            showMessage("Device Connected", 
                       event.deviceName + " is now connected", false);
            break;
            
        case BluetoothServiceEvent::DEVICE_DISCONNECTED:
            showMessage("Device Disconnected", 
                       event.deviceName + " has been disconnected", false);
            break;
            
        case BluetoothServiceEvent::PAIRING_SUCCESS:
            showMessage("Pairing Successful", 
                       "Successfully paired with " + event.deviceName, false);
            break;
            
        case BluetoothServiceEvent::PAIRING_FAILED:
            showMessage("Pairing Failed", 
                       "Failed to pair with " + event.deviceName + ": " + event.errorMessage, true);
            break;
            
        case BluetoothServiceEvent::ERROR_OCCURRED:
            showMessage("Bluetooth Error", event.errorMessage, true);
            break;
            
        default:
            break;
    }
}

void BluetoothManagerApp::showMessage(const std::string& title, const std::string& message, bool isError) {
    // Implementation for showing temporary message to user
    ESP_LOGI(TAG, "Message: %s - %s", title.c_str(), message.c_str());
    m_messageVisible = true;
    m_messageDisplayTime = 0;
}

void BluetoothManagerApp::hideMessage() {
    m_messageVisible = false;
    m_messageDisplayTime = 0;
}

// Additional implementations for the remaining event handlers would continue...
// The pattern is established for the key functionality