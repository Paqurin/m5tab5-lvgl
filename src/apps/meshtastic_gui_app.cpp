#include "meshtastic_gui_app.h"
#include <esp_log.h>
#include <cstdio>
#include <algorithm>

static const char* TAG = "MeshtasticGUI";

// LVGL style colors
#define COLOR_PRIMARY       lv_color_hex(0x2196F3)  // Blue
#define COLOR_SUCCESS       lv_color_hex(0x4CAF50)  // Green
#define COLOR_WARNING       lv_color_hex(0xFF9800)  // Orange
#define COLOR_ERROR         lv_color_hex(0xF44336)  // Red
#define COLOR_BACKGROUND    lv_color_hex(0x121212)  // Dark
#define COLOR_SURFACE       lv_color_hex(0x1E1E1E)  // Dark surface
#define COLOR_TEXT          lv_color_hex(0xFFFFFF)  // White text

MeshtasticGUIApp::MeshtasticGUIApp()
    : BaseApp("meshtastic", "Meshtastic", "1.0.0")
    , m_daemon(std::make_unique<MeshtasticDaemon>())
    , m_halManager(nullptr)
    , m_currentScreen(MeshtasticScreen::DASHBOARD)
    , m_lastUpdate(0)
    , m_mainContainer(nullptr)
    , m_statusBar(nullptr)
    , m_contentArea(nullptr)
    , m_tabView(nullptr)
{
    // Initialize compose state
    m_composeState.text = "";
    m_composeState.targetNode = 0;  // Broadcast
    m_composeState.channel = 0;
    m_composeState.wantAck = false;
    m_composeState.emergency = false;
    
    // Initialize map state
    m_mapState.centerLat = 40.7128;   // Default to NYC
    m_mapState.centerLon = -74.0060;
    m_mapState.zoomLevel = 5;
    m_mapState.selectedNode = 0;
    m_mapState.followMyNode = true;
    m_mapState.showTrails = false;
    m_mapState.showRange = true;
}

MeshtasticGUIApp::~MeshtasticGUIApp() {
    // Destructor cleanup handled by unique_ptr and LVGL
}

os_error_t MeshtasticGUIApp::initialize() {
    ESP_LOGI(TAG, "Initializing Meshtastic GUI");
    
    // Initialize daemon
    if (!m_halManager) {
        ESP_LOGE(TAG, "HAL Manager not available");
        return OS_ERROR_HARDWARE;
    }
    
    os_error_t result = m_daemon->initialize(m_halManager);
    if (result != OS_OK) {
        ESP_LOGE(TAG, "Failed to initialize Meshtastic daemon: %d", result);
        return result;
    }
    
    // Set daemon callbacks
    m_daemon->setNodeUpdateCallback([this](const MeshtasticNode& node) {
        onNodeUpdate(node);
    });
    
    m_daemon->setMessageCallback([this](const MeshtasticMessage& message) {
        onMessageReceived(message);
    });
    
    m_daemon->setRangeTestCallback([this](const MeshtasticRangeTest& result) {
        onRangeTestResult(result);
    });
    
    ESP_LOGI(TAG, "Meshtastic GUI initialized successfully");
    return OS_OK;
}

os_error_t MeshtasticGUIApp::createUI(lv_obj_t* parent) {
    ESP_LOGI(TAG, "Creating Meshtastic GUI");
    
    // Create main UI in the provided parent
    createMainUI(parent);
    
    // Start with dashboard
    showScreen(MeshtasticScreen::DASHBOARD);
    
    m_lastUpdate = millis();
    
    return OS_OK;
}

os_error_t MeshtasticGUIApp::shutdown() {
    ESP_LOGI(TAG, "Shutting down Meshtastic GUI");
    
    // Clean up LVGL objects
    if (m_mainContainer) {
        lv_obj_del(m_mainContainer);
        m_mainContainer = nullptr;
    }
    
    return OS_OK;
}

os_error_t MeshtasticGUIApp::update(uint32_t deltaTime) {
    // Update daemon
    m_daemon->update(deltaTime);
    
    uint32_t currentTime = millis();
    
    // Update UI periodically
    if (currentTime - m_lastUpdate >= MESHTASTIC_GUI_UPDATE_INTERVAL_MS) {
        updateStatusBar();
        
        // Update current screen
        switch (m_currentScreen) {
            case MeshtasticScreen::DASHBOARD:
                updateDashboard();
                break;
            case MeshtasticScreen::NODES:
                updateNodesList();
                break;
            case MeshtasticScreen::MESSAGES:
                updateMessages();
                break;
            case MeshtasticScreen::MAP:
                updateMap();
                break;
            default:
                break;
        }
        
        m_lastUpdate = currentTime;
    }
    
    return OS_OK;
}


void MeshtasticGUIApp::showScreen(MeshtasticScreen screen) {
    if (screen == m_currentScreen) {
        return;
    }
    
    m_currentScreen = screen;
    
    // Switch tab view to the appropriate tab
    if (m_tabView) {
        lv_tabview_set_act(m_tabView, (uint16_t)screen, LV_ANIM_ON);
    }
    
    ESP_LOGD(TAG, "Switched to screen: %d", (int)screen);
}

void MeshtasticGUIApp::onNodeUpdate(const MeshtasticNode& node) {
    ESP_LOGI(TAG, "Node update: 0x%08lX (%s)", node.id, node.shortName.c_str());
    
    // Update display nodes list
    auto it = std::find_if(m_displayNodes.begin(), m_displayNodes.end(),
                          [&node](const MeshtasticNode& n) { return n.id == node.id; });
    
    if (it != m_displayNodes.end()) {
        *it = node;
    } else {
        m_displayNodes.push_back(node);
        
        // Limit display list size
        if (m_displayNodes.size() > MESHTASTIC_NODE_DISPLAY_COUNT) {
            std::sort(m_displayNodes.begin(), m_displayNodes.end(),
                     [](const MeshtasticNode& a, const MeshtasticNode& b) {
                         return a.lastSeen > b.lastSeen;
                     });
            m_displayNodes.resize(MESHTASTIC_NODE_DISPLAY_COUNT);
        }
    }
    
    // Update map center if following our node
    if (node.id == m_daemon->getMyNodeId() && m_mapState.followMyNode) {
        if (node.latitude != 0.0 && node.longitude != 0.0) {
            m_mapState.centerLat = node.latitude;
            m_mapState.centerLon = node.longitude;
        }
    }
}

void MeshtasticGUIApp::onMessageReceived(const MeshtasticMessage& message) {
    ESP_LOGI(TAG, "Message received: ID=%lu, from=0x%08lX, type=%d", 
             message.id, message.fromNode, (int)message.type);
    
    addMessageToDisplay(message);
    
    // Handle text messages
    if (message.type == MeshtasticMessageType::TEXT_MESSAGE_UTF8 && 
        message.toNode == 0) {  // Broadcast message
        
        // Show notification for broadcast messages
        std::string text(message.payload.begin(), message.payload.end());
        ESP_LOGI(TAG, "Broadcast message: %s", text.c_str());
        
        // TODO: Show toast notification
    }
}

void MeshtasticGUIApp::onRangeTestResult(const MeshtasticRangeTest& result) {
    ESP_LOGI(TAG, "Range test result: seq=%lu, success=%d, RSSI=%d, distance=%.1fm",
             result.sequenceNumber, result.success, result.rssi, result.distance);
    
    // Update range test display if on range test screen
    if (m_currentScreen == MeshtasticScreen::RANGE_TEST) {
        updateRangeTestDisplay();
    }
}

void MeshtasticGUIApp::createMainUI(lv_obj_t* parent) {
    // Create main container
    m_mainContainer = lv_obj_create(parent ? parent : lv_scr_act());
    lv_obj_set_size(m_mainContainer, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(m_mainContainer, COLOR_BACKGROUND, LV_PART_MAIN);
    lv_obj_set_style_border_width(m_mainContainer, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(m_mainContainer, 0, LV_PART_MAIN);
    
    // Create status bar
    createStatusBar();
    
    // Create tab view for different screens
    m_tabView = lv_tabview_create(m_mainContainer, LV_DIR_TOP, 40);
    lv_obj_set_pos(m_tabView, 0, 30);
    lv_obj_set_size(m_tabView, LV_HOR_RES, LV_VER_RES - 30);
    lv_obj_set_style_bg_color(m_tabView, COLOR_SURFACE, LV_PART_MAIN);
    
    // Add tabs
    lv_obj_t* tab1 = lv_tabview_add_tab(m_tabView, "Dashboard");
    lv_obj_t* tab2 = lv_tabview_add_tab(m_tabView, "Nodes");
    lv_obj_t* tab3 = lv_tabview_add_tab(m_tabView, "Messages");
    lv_obj_t* tab4 = lv_tabview_add_tab(m_tabView, "Map");
    lv_obj_t* tab5 = lv_tabview_add_tab(m_tabView, "Channels");
    lv_obj_t* tab6 = lv_tabview_add_tab(m_tabView, "Range Test");
    lv_obj_t* tab7 = lv_tabview_add_tab(m_tabView, "Settings");
    lv_obj_t* tab8 = lv_tabview_add_tab(m_tabView, "Diagnostics");
    
    // Store tab references
    m_dashboardScreen = tab1;
    m_nodesScreen = tab2;
    m_messagesScreen = tab3;
    m_mapScreen = tab4;
    m_channelsScreen = tab5;
    m_rangeTestScreen = tab6;
    m_settingsScreen = tab7;
    m_diagnosticsScreen = tab8;
    
    // Create screen content
    createDashboardScreen();
    createNodesScreen();
    createMessagesScreen();
    createMapScreen();
    createChannelsScreen();
    createRangeTestScreen();
    createSettingsScreen();
    createDiagnosticsScreen();
    
    // Set tab change callback
    lv_obj_add_event_cb(m_tabView, onTabChanged, LV_EVENT_VALUE_CHANGED, this);
}

void MeshtasticGUIApp::createStatusBar() {
    m_statusBar = lv_obj_create(m_mainContainer);
    lv_obj_set_size(m_statusBar, LV_HOR_RES, 30);
    lv_obj_set_pos(m_statusBar, 0, 0);
    lv_obj_set_style_bg_color(m_statusBar, COLOR_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_border_width(m_statusBar, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(m_statusBar, 5, LV_PART_MAIN);
    lv_obj_set_flex_flow(m_statusBar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(m_statusBar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Node count indicator
    m_statusNodeCount = lv_label_create(m_statusBar);
    lv_label_set_text(m_statusNodeCount, "Nodes: 0");
    lv_obj_set_style_text_color(m_statusNodeCount, COLOR_TEXT, LV_PART_MAIN);
    
    // Signal indicator  
    m_statusSignal = lv_label_create(m_statusBar);
    lv_label_set_text(m_statusSignal, "Signal: --");
    lv_obj_set_style_text_color(m_statusSignal, COLOR_TEXT, LV_PART_MAIN);
    
    // Activity indicator
    m_statusActivity = lv_label_create(m_statusBar);
    lv_label_set_text(m_statusActivity, "●");
    lv_obj_set_style_text_color(m_statusActivity, COLOR_SUCCESS, LV_PART_MAIN);
}

void MeshtasticGUIApp::createDashboardScreen() {
    // Create scrollable container
    lv_obj_t* scroll = lv_obj_create(m_dashboardScreen);
    lv_obj_set_size(scroll, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(scroll, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(scroll, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(scroll, 10, LV_PART_MAIN);
    lv_obj_set_flex_flow(scroll, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scroll, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    
    // Network status widget
    createNetworkStatusWidget(scroll);
    
    // Node count widget
    createNodeCountWidget(scroll);
    
    // Message activity widget
    createMessageActivityWidget(scroll);
    
    // Signal quality chart
    createSignalChart(scroll);
}

void MeshtasticGUIApp::createNodesScreen() {
    // Create nodes list
    lv_obj_t* list = lv_list_create(m_nodesScreen);
    lv_obj_set_size(list, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(list, COLOR_SURFACE, LV_PART_MAIN);
}

void MeshtasticGUIApp::createMessagesScreen() {
    // Create message layout
    lv_obj_t* container = lv_obj_create(m_messagesScreen);
    lv_obj_set_size(container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(container, 5, LV_PART_MAIN);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    
    // Message history (80% height)
    createMessageHistoryList(container);
    
    // Message input area (20% height)
    createMessageInputArea(container);
}

void MeshtasticGUIApp::createMapScreen() {
    // Create map layout
    lv_obj_t* container = lv_obj_create(m_mapScreen);
    lv_obj_set_size(container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(container, 0, LV_PART_MAIN);
    
    // Map canvas
    createMapCanvas(container);
    
    // Map controls
    createMapControls(container);
}

void MeshtasticGUIApp::createChannelsScreen() {
    // Create channel list
    createChannelList(m_channelsScreen);
}

void MeshtasticGUIApp::createRangeTestScreen() {
    // Create range test layout
    lv_obj_t* container = lv_obj_create(m_rangeTestScreen);
    lv_obj_set_size(container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(container, 10, LV_PART_MAIN);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    
    // Controls (30% height)
    createRangeTestControls(container);
    
    // Results (70% height)
    createRangeTestResults(container);
}

void MeshtasticGUIApp::createSettingsScreen() {
    // Create settings layout
    lv_obj_t* scroll = lv_obj_create(m_settingsScreen);
    lv_obj_set_size(scroll, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(scroll, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(scroll, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(scroll, 10, LV_PART_MAIN);
    lv_obj_set_flex_flow(scroll, LV_FLEX_FLOW_COLUMN);
    
    // LoRa configuration panel
    createLoRaConfigPanel(scroll);
    
    // Node configuration panel
    createNodeConfigPanel(scroll);
}

void MeshtasticGUIApp::createDiagnosticsScreen() {
    // Create diagnostics layout
    lv_obj_t* container = lv_obj_create(m_diagnosticsScreen);
    lv_obj_set_size(container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(container, 10, LV_PART_MAIN);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    
    // Statistics panel (50% height)
    createStatisticsPanel(container);
    
    // Log viewer (50% height)
    createLogViewer(container);
}

// === Widget Creation Methods ===

lv_obj_t* MeshtasticGUIApp::createNetworkStatusWidget(lv_obj_t* parent) {
    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_set_size(card, LV_PCT(100), 80);
    lv_obj_set_style_bg_color(card, COLOR_SURFACE, LV_PART_MAIN);
    lv_obj_set_style_border_color(card, COLOR_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_border_width(card, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card, 10, LV_PART_MAIN);
    
    lv_obj_t* title = lv_label_create(card);
    lv_label_set_text(title, "Network Status");
    lv_obj_set_style_text_color(title, COLOR_TEXT, LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, LV_PART_MAIN);
    
    lv_obj_t* status = lv_label_create(card);
    lv_label_set_text(status, "Online - Ready for mesh communication");
    lv_obj_set_style_text_color(status, COLOR_SUCCESS, LV_PART_MAIN);
    lv_obj_align_to(status, title, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    
    return card;
}

lv_obj_t* MeshtasticGUIApp::createNodeCountWidget(lv_obj_t* parent) {
    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_set_size(card, LV_PCT(100), 60);
    lv_obj_set_style_bg_color(card, COLOR_SURFACE, LV_PART_MAIN);
    lv_obj_set_style_border_color(card, COLOR_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_border_width(card, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card, 10, LV_PART_MAIN);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    lv_obj_t* label = lv_label_create(card);
    lv_label_set_text(label, "Discovered Nodes");
    lv_obj_set_style_text_color(label, COLOR_TEXT, LV_PART_MAIN);
    
    lv_obj_t* count = lv_label_create(card);
    lv_label_set_text(count, "0");
    lv_obj_set_style_text_color(count, COLOR_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(count, &lv_font_montserrat_20, LV_PART_MAIN);
    
    return card;
}

lv_obj_t* MeshtasticGUIApp::createMessageActivityWidget(lv_obj_t* parent) {
    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_set_size(card, LV_PCT(100), 60);
    lv_obj_set_style_bg_color(card, COLOR_SURFACE, LV_PART_MAIN);
    lv_obj_set_style_border_color(card, COLOR_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_border_width(card, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card, 10, LV_PART_MAIN);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    lv_obj_t* label = lv_label_create(card);
    lv_label_set_text(label, "Messages/Hour");
    lv_obj_set_style_text_color(label, COLOR_TEXT, LV_PART_MAIN);
    
    lv_obj_t* rate = lv_label_create(card);
    lv_label_set_text(rate, "0.0");
    lv_obj_set_style_text_color(rate, COLOR_WARNING, LV_PART_MAIN);
    lv_obj_set_style_text_font(rate, &lv_font_montserrat_20, LV_PART_MAIN);
    
    return card;
}

lv_obj_t* MeshtasticGUIApp::createSignalChart(lv_obj_t* parent) {
    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_set_size(card, LV_PCT(100), 150);
    lv_obj_set_style_bg_color(card, COLOR_SURFACE, LV_PART_MAIN);
    lv_obj_set_style_border_color(card, COLOR_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_border_width(card, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card, 10, LV_PART_MAIN);
    
    lv_obj_t* title = lv_label_create(card);
    lv_label_set_text(title, "Signal Quality");
    lv_obj_set_style_text_color(title, COLOR_TEXT, LV_PART_MAIN);
    
    // TODO: Create actual chart widget
    lv_obj_t* placeholder = lv_label_create(card);
    lv_label_set_text(placeholder, "Signal quality chart will be displayed here");
    lv_obj_set_style_text_color(placeholder, lv_color_hex(0x808080), LV_PART_MAIN);
    lv_obj_align_to(placeholder, title, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 20);
    
    return card;
}

lv_obj_t* MeshtasticGUIApp::createMessageHistoryList(lv_obj_t* parent) {
    lv_obj_t* list = lv_list_create(parent);
    lv_obj_set_size(list, LV_PCT(100), LV_PCT(80));
    lv_obj_set_style_bg_color(list, COLOR_SURFACE, LV_PART_MAIN);
    
    return list;
}

lv_obj_t* MeshtasticGUIApp::createMessageInputArea(lv_obj_t* parent) {
    lv_obj_t* container = lv_obj_create(parent);
    lv_obj_set_size(container, LV_PCT(100), LV_PCT(20));
    lv_obj_set_style_bg_color(container, COLOR_SURFACE, LV_PART_MAIN);
    lv_obj_set_style_border_width(container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(container, 5, LV_PART_MAIN);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Text area
    lv_obj_t* textarea = lv_textarea_create(container);
    lv_obj_set_size(textarea, LV_PCT(75), LV_SIZE_CONTENT);
    lv_textarea_set_placeholder_text(textarea, "Type message...");
    lv_textarea_set_one_line(textarea, true);
    
    // Send button
    lv_obj_t* btn = lv_btn_create(container);
    lv_obj_set_size(btn, LV_PCT(20), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(btn, COLOR_PRIMARY, LV_PART_MAIN);
    lv_obj_add_event_cb(btn, onMessageSend, LV_EVENT_CLICKED, this);
    
    lv_obj_t* btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "Send");
    lv_obj_center(btn_label);
    
    return container;
}

lv_obj_t* MeshtasticGUIApp::createMapCanvas(lv_obj_t* parent) {
    lv_obj_t* canvas = lv_canvas_create(parent);
    lv_obj_set_size(canvas, LV_PCT(100), LV_PCT(85));
    lv_obj_set_pos(canvas, 0, 0);
    
    // TODO: Initialize canvas buffer and draw map
    
    return canvas;
}

lv_obj_t* MeshtasticGUIApp::createMapControls(lv_obj_t* parent) {
    lv_obj_t* controls = lv_obj_create(parent);
    lv_obj_set_size(controls, LV_PCT(100), LV_PCT(15));
    lv_obj_set_pos(controls, 0, LV_PCT(85));
    lv_obj_set_style_bg_color(controls, COLOR_SURFACE, LV_PART_MAIN);
    lv_obj_set_style_border_width(controls, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(controls, 5, LV_PART_MAIN);
    lv_obj_set_flex_flow(controls, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(controls, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Zoom in button
    lv_obj_t* zoom_in = lv_btn_create(controls);
    lv_obj_set_size(zoom_in, 50, 30);
    lv_obj_t* zi_label = lv_label_create(zoom_in);
    lv_label_set_text(zi_label, "+");
    lv_obj_center(zi_label);
    
    // Zoom out button  
    lv_obj_t* zoom_out = lv_btn_create(controls);
    lv_obj_set_size(zoom_out, 50, 30);
    lv_obj_t* zo_label = lv_label_create(zoom_out);
    lv_label_set_text(zo_label, "-");
    lv_obj_center(zo_label);
    
    // Center button
    lv_obj_t* center = lv_btn_create(controls);
    lv_obj_set_size(center, 60, 30);
    lv_obj_t* c_label = lv_label_create(center);
    lv_label_set_text(c_label, "Center");
    lv_obj_center(c_label);
    
    return controls;
}

lv_obj_t* MeshtasticGUIApp::createChannelList(lv_obj_t* parent) {
    lv_obj_t* list = lv_list_create(parent);
    lv_obj_set_size(list, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(list, COLOR_SURFACE, LV_PART_MAIN);
    
    // Add channel items
    for (int i = 0; i < MESHTASTIC_MAX_CHANNELS; i++) {
        lv_obj_t* item = lv_list_add_btn(list, LV_SYMBOL_WIFI, ("Channel " + std::to_string(i + 1)).c_str());
        lv_obj_set_style_text_color(item, COLOR_TEXT, LV_PART_MAIN);
    }
    
    return list;
}

lv_obj_t* MeshtasticGUIApp::createRangeTestControls(lv_obj_t* parent) {
    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_set_size(card, LV_PCT(100), LV_PCT(30));
    lv_obj_set_style_bg_color(card, COLOR_SURFACE, LV_PART_MAIN);
    lv_obj_set_style_border_color(card, COLOR_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_border_width(card, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card, 10, LV_PART_MAIN);
    
    lv_obj_t* title = lv_label_create(card);
    lv_label_set_text(title, "Range Test Controls");
    lv_obj_set_style_text_color(title, COLOR_TEXT, LV_PART_MAIN);
    
    // Start button
    lv_obj_t* start_btn = lv_btn_create(card);
    lv_obj_set_size(start_btn, 100, 40);
    lv_obj_set_pos(start_btn, 10, 40);
    lv_obj_set_style_bg_color(start_btn, COLOR_SUCCESS, LV_PART_MAIN);
    lv_obj_add_event_cb(start_btn, onRangeTestStart, LV_EVENT_CLICKED, this);
    
    lv_obj_t* start_label = lv_label_create(start_btn);
    lv_label_set_text(start_label, "Start Test");
    lv_obj_center(start_label);
    
    return card;
}

lv_obj_t* MeshtasticGUIApp::createRangeTestResults(lv_obj_t* parent) {
    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_set_size(card, LV_PCT(100), LV_PCT(70));
    lv_obj_set_style_bg_color(card, COLOR_SURFACE, LV_PART_MAIN);
    lv_obj_set_style_border_color(card, COLOR_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_border_width(card, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card, 10, LV_PART_MAIN);
    
    lv_obj_t* title = lv_label_create(card);
    lv_label_set_text(title, "Test Results");
    lv_obj_set_style_text_color(title, COLOR_TEXT, LV_PART_MAIN);
    
    // Results list
    lv_obj_t* list = lv_list_create(card);
    lv_obj_set_size(list, LV_PCT(100), LV_PCT(85));
    lv_obj_set_pos(list, 0, 30);
    lv_obj_set_style_bg_color(list, lv_color_hex(0x202020), LV_PART_MAIN);
    
    return card;
}

lv_obj_t* MeshtasticGUIApp::createLoRaConfigPanel(lv_obj_t* parent) {
    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_set_size(card, LV_PCT(100), 150);
    lv_obj_set_style_bg_color(card, COLOR_SURFACE, LV_PART_MAIN);
    lv_obj_set_style_border_color(card, COLOR_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_border_width(card, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card, 10, LV_PART_MAIN);
    
    lv_obj_t* title = lv_label_create(card);
    lv_label_set_text(title, "LoRa Configuration");
    lv_obj_set_style_text_color(title, COLOR_TEXT, LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, LV_PART_MAIN);
    
    // TODO: Add LoRa configuration controls
    
    return card;
}

lv_obj_t* MeshtasticGUIApp::createNodeConfigPanel(lv_obj_t* parent) {
    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_set_size(card, LV_PCT(100), 150);
    lv_obj_set_style_bg_color(card, COLOR_SURFACE, LV_PART_MAIN);
    lv_obj_set_style_border_color(card, COLOR_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_border_width(card, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card, 10, LV_PART_MAIN);
    
    lv_obj_t* title = lv_label_create(card);
    lv_label_set_text(title, "Node Configuration");
    lv_obj_set_style_text_color(title, COLOR_TEXT, LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, LV_PART_MAIN);
    
    // TODO: Add node configuration controls
    
    return card;
}

lv_obj_t* MeshtasticGUIApp::createStatisticsPanel(lv_obj_t* parent) {
    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_set_size(card, LV_PCT(100), LV_PCT(50));
    lv_obj_set_style_bg_color(card, COLOR_SURFACE, LV_PART_MAIN);
    lv_obj_set_style_border_color(card, COLOR_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_border_width(card, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card, 10, LV_PART_MAIN);
    
    lv_obj_t* title = lv_label_create(card);
    lv_label_set_text(title, "Network Statistics");
    lv_obj_set_style_text_color(title, COLOR_TEXT, LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, LV_PART_MAIN);
    
    // TODO: Add statistics display
    
    return card;
}

lv_obj_t* MeshtasticGUIApp::createLogViewer(lv_obj_t* parent) {
    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_set_size(card, LV_PCT(100), LV_PCT(50));
    lv_obj_set_style_bg_color(card, COLOR_SURFACE, LV_PART_MAIN);
    lv_obj_set_style_border_color(card, COLOR_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_border_width(card, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card, 10, LV_PART_MAIN);
    
    lv_obj_t* title = lv_label_create(card);
    lv_label_set_text(title, "System Logs");
    lv_obj_set_style_text_color(title, COLOR_TEXT, LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, LV_PART_MAIN);
    
    // TODO: Add log viewer
    
    return card;
}

// === Update Methods ===

void MeshtasticGUIApp::updateStatusBar() {
    if (!m_statusNodeCount || !m_statusSignal || !m_statusActivity) {
        return;
    }
    
    // Update node count
    size_t nodeCount = m_daemon->getNodeCount();
    char nodeText[32];
    snprintf(nodeText, sizeof(nodeText), "Nodes: %zu", nodeCount);
    lv_label_set_text(m_statusNodeCount, nodeText);
    
    // Update signal strength
    uint32_t totalMessages;
    float messagesPerHour;
    int8_t avgRSSI;
    uint8_t avgSNR;
    m_daemon->getNetworkStats(totalMessages, messagesPerHour, avgRSSI, avgSNR);
    
    char signalText[32];
    if (avgRSSI > -128) {
        snprintf(signalText, sizeof(signalText), "RSSI: %ddBm", avgRSSI);
        lv_obj_set_style_text_color(m_statusSignal, getSignalColor(avgRSSI), LV_PART_MAIN);
    } else {
        snprintf(signalText, sizeof(signalText), "Signal: --");
        lv_obj_set_style_text_color(m_statusSignal, lv_color_hex(0x808080), LV_PART_MAIN);
    }
    lv_label_set_text(m_statusSignal, signalText);
    
    // Activity indicator
    static bool activityBlink = false;
    activityBlink = !activityBlink;
    lv_obj_set_style_text_color(m_statusActivity, 
                                activityBlink ? COLOR_SUCCESS : lv_color_hex(0x404040), 
                                LV_PART_MAIN);
}

void MeshtasticGUIApp::updateDashboard() {
    // Dashboard updates happen through individual widget updates
}

void MeshtasticGUIApp::updateNodesList() {
    // TODO: Update nodes list display
}

void MeshtasticGUIApp::updateMessages() {
    // TODO: Update message display
}

void MeshtasticGUIApp::updateMap() {
    // TODO: Update map display
}

void MeshtasticGUIApp::updateRangeTestDisplay() {
    // TODO: Update range test results display
}

// === Helper Methods ===

void MeshtasticGUIApp::addMessageToDisplay(const MeshtasticMessage& message) {
    m_displayMessages.push_back(message);
    
    // Limit display list size
    if (m_displayMessages.size() > MESHTASTIC_MESSAGE_DISPLAY_COUNT) {
        m_displayMessages.erase(m_displayMessages.begin());
    }
}

std::string MeshtasticGUIApp::formatSignalStrength(int8_t rssi) {
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%d dBm", rssi);
    return std::string(buffer);
}

std::string MeshtasticGUIApp::formatDistance(double distance) {
    if (distance < 1000) {
        char buffer[16];
        snprintf(buffer, sizeof(buffer), "%.0f m", distance);
        return std::string(buffer);
    } else {
        char buffer[16];
        snprintf(buffer, sizeof(buffer), "%.1f km", distance / 1000.0);
        return std::string(buffer);
    }
}

std::string MeshtasticGUIApp::formatTimestamp(uint32_t timestamp) {
    uint32_t seconds = timestamp / 1000;
    uint32_t minutes = seconds / 60;
    uint32_t hours = minutes / 60;
    
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%02lu:%02lu:%02lu", 
             hours % 24, minutes % 60, seconds % 60);
    return std::string(buffer);
}

lv_color_t MeshtasticGUIApp::getSignalColor(int8_t rssi) {
    if (rssi >= -70) {
        return COLOR_SUCCESS;  // Excellent
    } else if (rssi >= -85) {
        return COLOR_WARNING;  // Good
    } else if (rssi >= -100) {
        return lv_color_hex(0xFFA500);  // Fair
    } else {
        return COLOR_ERROR;    // Poor
    }
}

// === Event Handlers ===

void MeshtasticGUIApp::onTabChanged(lv_event_t* event) {
    MeshtasticGUIApp* app = (MeshtasticGUIApp*)lv_event_get_user_data(event);
    uint16_t tab_id = lv_tabview_get_tab_act(app->m_tabView);
    app->m_currentScreen = (MeshtasticScreen)tab_id;
}

void MeshtasticGUIApp::onNodeSelected(lv_event_t* event) {
    // TODO: Handle node selection
}

void MeshtasticGUIApp::onMessageSend(lv_event_t* event) {
    MeshtasticGUIApp* app = (MeshtasticGUIApp*)lv_event_get_user_data(event);
    
    // TODO: Get message text from textarea and send
    std::string message = "Hello from M5Stack Tab5!";
    
    uint32_t msgId = app->m_daemon->sendTextMessage(message, 0, 0, false);
    if (msgId > 0) {
        ESP_LOGI(TAG, "Message sent: ID=%lu", msgId);
    }
}

void MeshtasticGUIApp::onMapClick(lv_event_t* event) {
    // TODO: Handle map interaction
}

void MeshtasticGUIApp::onRangeTestStart(lv_event_t* event) {
    MeshtasticGUIApp* app = (MeshtasticGUIApp*)lv_event_get_user_data(event);
    
    // Start range test with first available node
    auto nodes = app->m_daemon->getNodes();
    if (nodes.size() > 1) {  // Exclude ourselves
        for (const auto& node : nodes) {
            if (node.id != app->m_daemon->getMyNodeId()) {
                app->m_daemon->startRangeTest(node.id, 30, 10);
                ESP_LOGI(TAG, "Range test started to node 0x%08lX", node.id);
                break;
            }
        }
    }
}