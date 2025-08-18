#ifndef MESHTASTIC_GUI_APP_H
#define MESHTASTIC_GUI_APP_H

/**
 * @file meshtastic_gui_app.h
 * @brief Meshtastic GUI Application for M5Stack Tab5
 * 
 * Provides a comprehensive graphical interface for the Meshtastic mesh networking
 * daemon, including node discovery, messaging, range testing, and network analysis.
 * 
 * Features:
 * - Real-time mesh network visualization
 * - Interactive node map with GPS positions
 * - Message composition and history
 * - Channel management and configuration
 * - Range testing tools with statistics
 * - Network diagnostics and performance monitoring
 * - Emergency beacon functionality
 * - Store-and-forward message management
 */

#include "base_app.h"
#include "meshtastic_daemon.h"
#include "../ui/ui_manager.h"
#include <memory>
#include <vector>

// GUI Constants
#define MESHTASTIC_GUI_UPDATE_INTERVAL_MS   1000    // UI update rate
#define MESHTASTIC_MAP_ZOOM_LEVELS          10      // Map zoom levels
#define MESHTASTIC_MESSAGE_DISPLAY_COUNT    20      // Messages to show
#define MESHTASTIC_NODE_DISPLAY_COUNT       50      // Nodes to show
#define MESHTASTIC_CHART_POINTS             100     // Chart data points

// GUI Screen Types
enum class MeshtasticScreen {
    DASHBOARD = 0,      // Main dashboard with network overview
    NODES,              // Node list and details
    MESSAGES,           // Message history and composition
    MAP,                // Network map with GPS positions
    CHANNELS,           // Channel configuration
    RANGE_TEST,         // Range testing tools
    SETTINGS,           // Daemon configuration
    DIAGNOSTICS         // Network diagnostics and logs
};

// Message compose state
struct MessageComposeState {
    std::string text;           // Current message text
    uint32_t targetNode;        // Target node ID (0 = broadcast)
    uint8_t channel;            // Selected channel
    bool wantAck;               // Request acknowledgment
    bool emergency;             // Emergency message
};

// Map display state
struct MapDisplayState {
    double centerLat;           // Map center latitude
    double centerLon;           // Map center longitude
    uint8_t zoomLevel;          // Current zoom level
    uint32_t selectedNode;      // Selected node ID
    bool followMyNode;          // Follow our own position
    bool showTrails;            // Show node movement trails
    bool showRange;             // Show transmission range circles
};

/**
 * @class MeshtasticGUIApp
 * @brief Complete Meshtastic GUI application
 * 
 * Provides an intuitive touch interface for all Meshtastic functionality
 * including network management, messaging, and diagnostics.
 */
class MeshtasticGUIApp : public BaseApp {
public:
    MeshtasticGUIApp();
    virtual ~MeshtasticGUIApp();

    // BaseApp interface implementation
    os_error_t initialize() override;
    os_error_t update(uint32_t deltaTime) override;
    os_error_t shutdown() override;
    os_error_t createUI(lv_obj_t* parent) override;

    // === Initialization ===
    
    /**
     * @brief Set HAL manager for hardware access
     * @param halManager HAL manager instance
     */
    void setHALManager(HALManager* halManager) { m_halManager = halManager; }
    
    // === Screen Management ===
    
    /**
     * @brief Switch to specific screen
     * @param screen Target screen
     */
    void showScreen(MeshtasticScreen screen);
    
    /**
     * @brief Get current active screen
     * @return Current screen
     */
    MeshtasticScreen getCurrentScreen() const { return m_currentScreen; }
    
    // === Node Callbacks (called by daemon) ===
    
    /**
     * @brief Handle node discovery/update
     * @param node Updated node information
     */
    void onNodeUpdate(const MeshtasticNode& node);
    
    /**
     * @brief Handle incoming message
     * @param message Received message
     */
    void onMessageReceived(const MeshtasticMessage& message);
    
    /**
     * @brief Handle range test result
     * @param result Test result
     */
    void onRangeTestResult(const MeshtasticRangeTest& result);

private:
    // === Core Components ===
    std::unique_ptr<MeshtasticDaemon> m_daemon;
    HALManager* m_halManager;
    MeshtasticScreen m_currentScreen;
    uint32_t m_lastUpdate;
    
    // GUI State
    MessageComposeState m_composeState;
    MapDisplayState m_mapState;
    std::vector<MeshtasticMessage> m_displayMessages;
    std::vector<MeshtasticNode> m_displayNodes;
    
    // LVGL Objects
    lv_obj_t* m_mainContainer;
    lv_obj_t* m_statusBar;
    lv_obj_t* m_contentArea;
    lv_obj_t* m_tabView;
    
    // Screen-specific containers
    lv_obj_t* m_dashboardScreen;
    lv_obj_t* m_nodesScreen;
    lv_obj_t* m_messagesScreen;
    lv_obj_t* m_mapScreen;
    lv_obj_t* m_channelsScreen;
    lv_obj_t* m_rangeTestScreen;
    lv_obj_t* m_settingsScreen;
    lv_obj_t* m_diagnosticsScreen;
    
    // Status bar widgets
    lv_obj_t* m_statusNodeCount;
    lv_obj_t* m_statusSignal;
    lv_obj_t* m_statusBattery;
    lv_obj_t* m_statusActivity;
    
    // === Screen Creation Methods ===
    
    /**
     * @brief Create main UI structure
     * @param parent Parent container (or nullptr for screen)
     */
    void createMainUI(lv_obj_t* parent);
    
    /**
     * @brief Create status bar
     */
    void createStatusBar();
    
    /**
     * @brief Create dashboard screen
     */
    void createDashboardScreen();
    
    /**
     * @brief Create nodes list screen
     */
    void createNodesScreen();
    
    /**
     * @brief Create messages screen
     */
    void createMessagesScreen();
    
    /**
     * @brief Create network map screen
     */
    void createMapScreen();
    
    /**
     * @brief Create channels configuration screen
     */
    void createChannelsScreen();
    
    /**
     * @brief Create range testing screen
     */
    void createRangeTestScreen();
    
    /**
     * @brief Create settings screen
     */
    void createSettingsScreen();
    
    /**
     * @brief Create diagnostics screen
     */
    void createDiagnosticsScreen();
    
    // === Screen Update Methods ===
    
    /**
     * @brief Update dashboard display
     */
    void updateDashboard();
    
    /**
     * @brief Update nodes list
     */
    void updateNodesList();
    
    /**
     * @brief Update messages display
     */
    void updateMessages();
    
    /**
     * @brief Update network map
     */
    void updateMap();
    
    /**
     * @brief Update status bar indicators
     */
    void updateStatusBar();
    
    // === Dashboard Widgets ===
    
    /**
     * @brief Create network status widget
     * @param parent Parent container
     * @return Created widget
     */
    lv_obj_t* createNetworkStatusWidget(lv_obj_t* parent);
    
    /**
     * @brief Create node count widget
     * @param parent Parent container
     * @return Created widget
     */
    lv_obj_t* createNodeCountWidget(lv_obj_t* parent);
    
    /**
     * @brief Create message activity widget
     * @param parent Parent container
     * @return Created widget
     */
    lv_obj_t* createMessageActivityWidget(lv_obj_t* parent);
    
    /**
     * @brief Create signal quality chart
     * @param parent Parent container
     * @return Created chart widget
     */
    lv_obj_t* createSignalChart(lv_obj_t* parent);
    
    // === Node Management Widgets ===
    
    /**
     * @brief Create node list item
     * @param parent Parent list
     * @param node Node information
     * @return List item object
     */
    lv_obj_t* createNodeListItem(lv_obj_t* parent, const MeshtasticNode& node);
    
    /**
     * @brief Show node details popup
     * @param node Node to display
     */
    void showNodeDetails(const MeshtasticNode& node);
    
    /**
     * @brief Create node info card
     * @param parent Parent container
     * @param node Node information
     * @return Info card object
     */
    lv_obj_t* createNodeInfoCard(lv_obj_t* parent, const MeshtasticNode& node);
    
    // === Messaging Widgets ===
    
    /**
     * @brief Create message input area
     * @param parent Parent container
     * @return Input area object
     */
    lv_obj_t* createMessageInputArea(lv_obj_t* parent);
    
    /**
     * @brief Create message history list
     * @param parent Parent container
     * @return History list object
     */
    lv_obj_t* createMessageHistoryList(lv_obj_t* parent);
    
    /**
     * @brief Add message to display
     * @param message Message to add
     */
    void addMessageToDisplay(const MeshtasticMessage& message);
    
    /**
     * @brief Show message compose dialog
     */
    void showMessageComposeDialog();
    
    // === Map Widgets ===
    
    /**
     * @brief Create map canvas
     * @param parent Parent container
     * @return Canvas object
     */
    lv_obj_t* createMapCanvas(lv_obj_t* parent);
    
    /**
     * @brief Create map controls
     * @param parent Parent container
     * @return Controls container
     */
    lv_obj_t* createMapControls(lv_obj_t* parent);
    
    /**
     * @brief Draw network map
     * @param canvas Canvas object
     */
    void drawNetworkMap(lv_obj_t* canvas);
    
    /**
     * @brief Convert GPS coordinates to screen pixels
     * @param lat Latitude
     * @param lon Longitude
     * @param x Screen X coordinate (output)
     * @param y Screen Y coordinate (output)
     */
    void gpsToScreen(double lat, double lon, int16_t& x, int16_t& y);
    
    // === Range Testing Widgets ===
    
    /**
     * @brief Create range test controls
     * @param parent Parent container
     * @return Controls container
     */
    lv_obj_t* createRangeTestControls(lv_obj_t* parent);
    
    /**
     * @brief Create range test results display
     * @param parent Parent container
     * @return Results display
     */
    lv_obj_t* createRangeTestResults(lv_obj_t* parent);
    
    /**
     * @brief Update range test display
     */
    void updateRangeTestDisplay();
    
    // === Channel Management ===
    
    /**
     * @brief Create channel list
     * @param parent Parent container
     * @return Channel list object
     */
    lv_obj_t* createChannelList(lv_obj_t* parent);
    
    /**
     * @brief Show channel configuration dialog
     * @param channelIndex Channel to configure
     */
    void showChannelConfigDialog(uint8_t channelIndex);
    
    // === Settings Widgets ===
    
    /**
     * @brief Create LoRa configuration panel
     * @param parent Parent container
     * @return Configuration panel
     */
    lv_obj_t* createLoRaConfigPanel(lv_obj_t* parent);
    
    /**
     * @brief Create node configuration panel
     * @param parent Parent container
     * @return Configuration panel
     */
    lv_obj_t* createNodeConfigPanel(lv_obj_t* parent);
    
    // === Diagnostics Widgets ===
    
    /**
     * @brief Create statistics display
     * @param parent Parent container
     * @return Statistics panel
     */
    lv_obj_t* createStatisticsPanel(lv_obj_t* parent);
    
    /**
     * @brief Create log viewer
     * @param parent Parent container
     * @return Log viewer object
     */
    lv_obj_t* createLogViewer(lv_obj_t* parent);
    
    // === Event Handlers ===
    
    /**
     * @brief Handle tab view change
     * @param event LVGL event
     */
    static void onTabChanged(lv_event_t* event);
    
    /**
     * @brief Handle node list selection
     * @param event LVGL event
     */
    static void onNodeSelected(lv_event_t* event);
    
    /**
     * @brief Handle message send button
     * @param event LVGL event
     */
    static void onMessageSend(lv_event_t* event);
    
    /**
     * @brief Handle map interaction
     * @param event LVGL event
     */
    static void onMapClick(lv_event_t* event);
    
    /**
     * @brief Handle range test start
     * @param event LVGL event
     */
    static void onRangeTestStart(lv_event_t* event);
    
    // === Utility Methods ===
    
    /**
     * @brief Format signal strength for display
     * @param rssi Signal strength in dBm
     * @return Formatted string
     */
    std::string formatSignalStrength(int8_t rssi);
    
    /**
     * @brief Format node distance for display
     * @param distance Distance in meters
     * @return Formatted string
     */
    std::string formatDistance(double distance);
    
    /**
     * @brief Format timestamp for display
     * @param timestamp Unix timestamp
     * @return Formatted time string
     */
    std::string formatTimestamp(uint32_t timestamp);
    
    /**
     * @brief Get signal strength color
     * @param rssi Signal strength in dBm
     * @return LVGL color
     */
    lv_color_t getSignalColor(int8_t rssi);
    
    /**
     * @brief Show confirmation dialog
     * @param title Dialog title
     * @param message Dialog message
     * @param callback Confirmation callback
     */
    void showConfirmDialog(const std::string& title, const std::string& message,
                          std::function<void()> callback);
    
    /**
     * @brief Show error message
     * @param message Error message
     */
    void showError(const std::string& message);
    
    /**
     * @brief Show success message
     * @param message Success message
     */
    void showSuccess(const std::string& message);
};

#endif // MESHTASTIC_GUI_APP_H