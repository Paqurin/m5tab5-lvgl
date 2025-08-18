#ifndef BLUETOOTH_MANAGER_APP_H
#define BLUETOOTH_MANAGER_APP_H

#include "base_app.h"
#include "../services/bluetooth_service.h"
#include <lvgl.h>
#include <vector>
#include <string>

/**
 * @file bluetooth_manager_app.h
 * @brief Bluetooth Device Manager Application
 * 
 * Comprehensive Bluetooth management interface providing device discovery,
 * pairing workflows, connection management, and device configuration.
 * Features intuitive touch interface for all Bluetooth operations.
 */

class BluetoothManagerApp : public BaseApp {
public:
    BluetoothManagerApp();
    ~BluetoothManagerApp() override;

    // BaseApp interface implementation
    os_error_t initialize() override;
    os_error_t start() override;
    os_error_t stop() override;
    os_error_t pause() override;
    os_error_t resume() override;
    os_error_t update(uint32_t deltaTime) override;
    os_error_t createUI(lv_obj_t* parent) override;
    os_error_t destroyUI() override;
    os_error_t shutdown() override;

    // App identification - these cannot be overridden since BaseApp methods are non-virtual
    // The values are set in constructor instead

private:
    // UI page types
    enum class UIPage {
        MAIN_MENU,
        DEVICE_DISCOVERY,
        PAIRED_DEVICES,
        DEVICE_DETAILS,
        CONNECTION_STATUS,
        SETTINGS
    };

    // Device list item structure
    struct DeviceListItem {
        BluetoothDeviceInfo device;
        lv_obj_t* container;
        lv_obj_t* nameLabel;
        lv_obj_t* addressLabel;
        lv_obj_t* typeLabel;
        lv_obj_t* rssiBar;
        lv_obj_t* statusIcon;
        lv_obj_t* connectButton;
        bool selected;
    };

    // UI creation methods
    void createMainMenu();
    void createDiscoveryPage();
    void createPairedDevicesPage();
    void createDeviceDetailsPage();
    void createConnectionStatusPage();
    void createSettingsPage();

    // UI update methods
    void updateMainMenu();
    void updateDiscoveryPage();
    void updatePairedDevicesPage();
    void updateDeviceDetailsPage();
    void updateConnectionStatusPage();

    // UI navigation
    void showPage(UIPage page);
    void navigateBack();
    void createNavigationBar();
    void createHeaderBar(const std::string& title, bool showBackButton = false);

    // Device list management
    void populateDeviceList(const std::vector<BluetoothDeviceInfo>& devices, lv_obj_t* parent);
    void createDeviceListItem(const BluetoothDeviceInfo& device, lv_obj_t* parent);
    void updateDeviceListItem(DeviceListItem& item);
    void clearDeviceList();

    // Event handlers
    static void onDiscoveryButtonClicked(lv_event_t* e);
    static void onPairedDevicesButtonClicked(lv_event_t* e);
    static void onSettingsButtonClicked(lv_event_t* e);
    static void onStatusButtonClicked(lv_event_t* e);
    static void onBackButtonClicked(lv_event_t* e);
    static void onStartDiscoveryClicked(lv_event_t* e);
    static void onStopDiscoveryClicked(lv_event_t* e);
    static void onDeviceConnectClicked(lv_event_t* e);
    static void onDeviceDisconnectClicked(lv_event_t* e);
    static void onDeviceRemoveClicked(lv_event_t* e);
    static void onDeviceDetailsClicked(lv_event_t* e);
    static void onAutoConnectSwitchChanged(lv_event_t* e);
    static void onTrustedSwitchChanged(lv_event_t* e);
    static void onVolumeSliderChanged(lv_event_t* e);
    static void onMediaControlClicked(lv_event_t* e);

    // Bluetooth service event handler
    void onBluetoothServiceEvent(const BluetoothServiceEventData& event);

    // Utility methods
    std::string getDeviceTypeString(BluetoothDeviceType type);
    lv_color_t getDeviceTypeColor(BluetoothDeviceType type);
    const char* getDeviceStatusIcon(const BluetoothDeviceInfo& device);
    void showMessage(const std::string& title, const std::string& message, bool isError = false);
    void hideMessage();
    void updateConnectionIndicators();

    // Bluetooth service reference
    BluetoothService* m_bluetoothService;

    // UI state
    UIPage m_currentPage;
    UIPage m_previousPage;
    lv_obj_t* m_mainContainer;
    lv_obj_t* m_headerBar;
    lv_obj_t* m_headerTitle;
    lv_obj_t* m_backButton;
    lv_obj_t* m_contentArea;
    lv_obj_t* m_navigationBar;
    lv_obj_t* m_messageBox;

    // Page containers
    lv_obj_t* m_mainMenuPage;
    lv_obj_t* m_discoveryPage;
    lv_obj_t* m_pairedDevicesPage;
    lv_obj_t* m_deviceDetailsPage;
    lv_obj_t* m_statusPage;
    lv_obj_t* m_settingsPage;

    // Discovery page controls
    lv_obj_t* m_discoveryList;
    lv_obj_t* m_discoveryStartButton;
    lv_obj_t* m_discoveryStopButton;
    lv_obj_t* m_discoverySpinner;
    lv_obj_t* m_discoveryStatus;

    // Paired devices page controls
    lv_obj_t* m_pairedDevicesList;
    lv_obj_t* m_pairedDevicesCount;

    // Device details page controls
    lv_obj_t* m_detailsDeviceName;
    lv_obj_t* m_detailsDeviceAddress;
    lv_obj_t* m_detailsDeviceType;
    lv_obj_t* m_detailsConnectionStatus;
    lv_obj_t* m_detailsAutoConnectSwitch;
    lv_obj_t* m_detailsTrustedSwitch;
    lv_obj_t* m_detailsConnectButton;
    lv_obj_t* m_detailsDisconnectButton;
    lv_obj_t* m_detailsRemoveButton;
    lv_obj_t* m_detailsVolumeSlider;
    lv_obj_t* m_detailsMediaControls;

    // Status page controls
    lv_obj_t* m_statusKeyboardIndicator;
    lv_obj_t* m_statusMouseIndicator;
    lv_obj_t* m_statusAudioIndicator;
    lv_obj_t* m_statusConnectionCount;
    lv_obj_t* m_statusStatsContainer;

    // Settings page controls
    lv_obj_t* m_settingsAutoDiscoverySwitch;
    lv_obj_t* m_settingsAutoConnectSwitch;
    lv_obj_t* m_settingsDeviceNameInput;
    lv_obj_t* m_settingsClearProfilesButton;

    // Device management
    std::vector<DeviceListItem> m_discoveredDeviceItems;
    std::vector<DeviceListItem> m_pairedDeviceItems;
    std::string m_selectedDeviceAddress;
    DeviceProfile m_currentDeviceProfile;

    // App state
    bool m_discoveryInProgress;
    uint32_t m_discoveryStartTime;
    uint32_t m_lastUIUpdate;
    uint32_t m_messageDisplayTime;
    bool m_messageVisible;

    // Configuration
    static constexpr uint32_t UI_UPDATE_INTERVAL = 500;    // 500ms
    static constexpr uint32_t MESSAGE_DISPLAY_TIME = 3000; // 3 seconds
    static constexpr int MAX_DEVICE_LIST_ITEMS = 20;
    static constexpr const char* TAG = "BluetoothManagerApp";

    // Styling
    static constexpr lv_color_t COLOR_BLUETOOTH = LV_COLOR_MAKE(0x00, 0x7F, 0xFF);
    static constexpr lv_color_t COLOR_CONNECTED = LV_COLOR_MAKE(0x00, 0xFF, 0x00);
    static constexpr lv_color_t COLOR_DISCONNECTED = LV_COLOR_MAKE(0xFF, 0x7F, 0x00);
    static constexpr lv_color_t COLOR_ERROR = LV_COLOR_MAKE(0xFF, 0x00, 0x00);
    static constexpr lv_color_t COLOR_KEYBOARD = LV_COLOR_MAKE(0xFF, 0x9F, 0x00);
    static constexpr lv_color_t COLOR_MOUSE = LV_COLOR_MAKE(0x9F, 0xFF, 0x00);
    static constexpr lv_color_t COLOR_AUDIO = LV_COLOR_MAKE(0xFF, 0x00, 0xFF);

    // Static instance for event handlers
    static BluetoothManagerApp* s_instance;
};

#endif // BLUETOOTH_MANAGER_APP_H