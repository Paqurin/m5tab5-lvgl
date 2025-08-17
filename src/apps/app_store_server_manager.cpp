#include "app_store_server_manager.h"
#include "../system/os_manager.h"
#include <esp_log.h>
#include <cstring>
#include <regex>
#include <fstream>
#include <random>

static const char* TAG = "AppStoreServerManager";

// AppStoreServerManager Implementation

os_error_t AppStoreServerManager::initialize() {
    if (m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Initializing App Store Server Manager");
    
    // Load existing configuration or create default
    os_error_t result = loadConfiguration();
    if (result != OS_OK) {
        ESP_LOGW(TAG, "Failed to load configuration, adding default servers");
        addDefaultServers();
        saveConfiguration();
    }
    
    // Refresh server statuses
    refreshServerStatuses();
    
    m_initialized = true;
    ESP_LOGI(TAG, "App Store Server Manager initialized with %d servers", m_servers.size());
    
    return OS_OK;
}

os_error_t AppStoreServerManager::shutdown() {
    if (!m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Shutting down App Store Server Manager");
    
    // Save current configuration
    saveConfiguration();
    
    // Clear server list
    m_servers.clear();
    m_defaultServerId.clear();
    
    m_initialized = false;
    return OS_OK;
}

os_error_t AppStoreServerManager::addServer(const AppStoreServer& server) {
    if (!m_initialized) {
        return OS_ERROR_GENERIC;
    }
    
    // Validate server configuration
    ServerValidationResult validation = validateServer(server);
    if (validation != ServerValidationResult::VALID) {
        ESP_LOGW(TAG, "Server validation failed: %d", static_cast<int>(validation));
        return OS_ERROR_INVALID_PARAM;
    }
    
    // Check for duplicate URLs
    for (const auto& existingServer : m_servers) {
        if (existingServer.url == server.url) {
            ESP_LOGW(TAG, "Server with URL %s already exists", server.url.c_str());
            return OS_ERROR_GENERIC;
        }
    }
    
    // Create server with generated ID
    AppStoreServer newServer = server;
    newServer.id = generateServerId(server.name);
    newServer.addedTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
    newServer.status = ServerStatus::UNKNOWN;
    newServer.lastChecked = 0;
    newServer.totalApps = 0;
    
    // Add to server list
    m_servers.push_back(newServer);
    
    // Set as default if it's the first server
    if (m_servers.size() == 1 || newServer.isDefault) {
        m_defaultServerId = newServer.id;
    }
    
    // Save configuration
    saveConfiguration();
    
    ESP_LOGI(TAG, "Added server: %s (%s)", newServer.name.c_str(), newServer.url.c_str());
    return OS_OK;
}

os_error_t AppStoreServerManager::removeServer(const std::string& serverId) {
    auto it = std::find_if(m_servers.begin(), m_servers.end(),
                          [&serverId](const AppStoreServer& server) {
                              return server.id == serverId;
                          });
    
    if (it == m_servers.end()) {
        return OS_ERROR_NOT_FOUND;
    }
    
    std::string serverName = it->name;
    
    // If removing default server, find a new default
    if (m_defaultServerId == serverId) {
        m_defaultServerId.clear();
        for (const auto& server : m_servers) {
            if (server.id != serverId && server.isEnabled) {
                m_defaultServerId = server.id;
                break;
            }
        }
    }
    
    m_servers.erase(it);
    saveConfiguration();
    
    ESP_LOGI(TAG, "Removed server: %s", serverName.c_str());
    return OS_OK;
}

os_error_t AppStoreServerManager::updateServer(const std::string& serverId, const AppStoreServer& server) {
    auto it = std::find_if(m_servers.begin(), m_servers.end(),
                          [&serverId](const AppStoreServer& s) {
                              return s.id == serverId;
                          });
    
    if (it == m_servers.end()) {
        return OS_ERROR_NOT_FOUND;
    }
    
    // Validate updated server
    ServerValidationResult validation = validateServer(server);
    if (validation != ServerValidationResult::VALID) {
        return OS_ERROR_INVALID_PARAM;
    }
    
    // Preserve ID and timestamps
    AppStoreServer updatedServer = server;
    updatedServer.id = it->id;
    updatedServer.addedTime = it->addedTime;
    updatedServer.lastChecked = it->lastChecked;
    updatedServer.status = ServerStatus::UNKNOWN; // Will be refreshed
    
    *it = updatedServer;
    saveConfiguration();
    
    ESP_LOGI(TAG, "Updated server: %s", updatedServer.name.c_str());
    return OS_OK;
}

os_error_t AppStoreServerManager::setServerEnabled(const std::string& serverId, bool enabled) {
    auto it = std::find_if(m_servers.begin(), m_servers.end(),
                          [&serverId](AppStoreServer& server) {
                              return server.id == serverId;
                          });
    
    if (it == m_servers.end()) {
        return OS_ERROR_NOT_FOUND;
    }
    
    it->isEnabled = enabled;
    saveConfiguration();
    
    ESP_LOGI(TAG, "Server %s %s", it->name.c_str(), enabled ? "enabled" : "disabled");
    return OS_OK;
}

ServerValidationResult AppStoreServerManager::validateServer(const AppStoreServer& server) {
    // Validate name
    if (server.name.empty() || server.name.length() > 100) {
        return ServerValidationResult::INVALID_URL;
    }
    
    // Validate URL
    if (!isValidUrl(server.url)) {
        return ServerValidationResult::INVALID_URL;
    }
    
    // Validate protocol
    ServerProtocol detectedProtocol = parseProtocol(server.url);
    if (detectedProtocol != server.protocol) {
        return ServerValidationResult::INVALID_PROTOCOL;
    }
    
    // Validate timeout
    if (server.timeout < 1000 || server.timeout > 60000) {
        return ServerValidationResult::INVALID_URL; // Reuse for general validation
    }
    
    return ServerValidationResult::VALID;
}

os_error_t AppStoreServerManager::testServerConnection(const std::string& serverId) {
    const AppStoreServer* server = getServer(serverId);
    if (!server) {
        return OS_ERROR_NOT_FOUND;
    }
    
    ESP_LOGI(TAG, "Testing connection to server: %s", server->name.c_str());
    
    // For now, simulate connection test
    // In a real implementation, this would make HTTP/HTTPS requests
    auto it = std::find_if(m_servers.begin(), m_servers.end(),
                          [&serverId](AppStoreServer& s) {
                              return s.id == serverId;
                          });
    
    if (it != m_servers.end()) {
        it->status = ServerStatus::ONLINE; // Simulate success
        it->lastChecked = xTaskGetTickCount() * portTICK_PERIOD_MS;
        it->totalApps = 15; // Simulate app count
        saveConfiguration();
    }
    
    return OS_OK;
}

std::vector<AppStoreServer> AppStoreServerManager::getConfiguredServers() const {
    return m_servers;
}

std::vector<AppStoreServer> AppStoreServerManager::getEnabledServers() const {
    std::vector<AppStoreServer> enabledServers;
    for (const auto& server : m_servers) {
        if (server.isEnabled) {
            enabledServers.push_back(server);
        }
    }
    return enabledServers;
}

const AppStoreServer* AppStoreServerManager::getServer(const std::string& serverId) const {
    auto it = std::find_if(m_servers.begin(), m_servers.end(),
                          [&serverId](const AppStoreServer& server) {
                              return server.id == serverId;
                          });
    
    return (it != m_servers.end()) ? &(*it) : nullptr;
}

const AppStoreServer* AppStoreServerManager::getDefaultServer() const {
    if (m_defaultServerId.empty()) {
        return nullptr;
    }
    return getServer(m_defaultServerId);
}

os_error_t AppStoreServerManager::setDefaultServer(const std::string& serverId) {
    const AppStoreServer* server = getServer(serverId);
    if (!server) {
        return OS_ERROR_NOT_FOUND;
    }
    
    m_defaultServerId = serverId;
    
    // Update isDefault flags
    for (auto& s : m_servers) {
        s.isDefault = (s.id == serverId);
    }
    
    saveConfiguration();
    
    ESP_LOGI(TAG, "Set default server: %s", server->name.c_str());
    return OS_OK;
}

os_error_t AppStoreServerManager::refreshServerStatuses() {
    ESP_LOGI(TAG, "Refreshing server statuses");
    
    for (auto& server : m_servers) {
        if (server.isEnabled) {
            testServerConnection(server.id);
        } else {
            server.status = ServerStatus::OFFLINE;
        }
    }
    
    m_lastRefresh = xTaskGetTickCount() * portTICK_PERIOD_MS;
    return OS_OK;
}

// Private methods

std::string AppStoreServerManager::generateServerId(const std::string& baseName) {
    std::string baseId = baseName;
    
    // Convert to lowercase and replace spaces with underscores
    std::transform(baseId.begin(), baseId.end(), baseId.begin(), ::tolower);
    std::replace(baseId.begin(), baseId.end(), ' ', '_');
    
    // Remove non-alphanumeric characters except underscores
    baseId.erase(std::remove_if(baseId.begin(), baseId.end(),
                               [](char c) { return !std::isalnum(c) && c != '_'; }),
                baseId.end());
    
    // Add random suffix to ensure uniqueness
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);
    
    return baseId + "_" + std::to_string(dis(gen));
}

bool AppStoreServerManager::isValidUrl(const std::string& url) const {
    if (url.empty() || url.length() > 512) {
        return false;
    }
    
    // Basic URL validation
    std::regex urlRegex(R"(^(https?|ftp)://[^\s/$.?#].[^\s]*$)", std::regex_constants::icase);
    return std::regex_match(url, urlRegex);
}

ServerProtocol AppStoreServerManager::parseProtocol(const std::string& url) const {
    if (url.find("https://") == 0) {
        return ServerProtocol::HTTPS;
    } else if (url.find("http://") == 0) {
        return ServerProtocol::HTTP;
    } else if (url.find("ftp://") == 0) {
        return ServerProtocol::FTP;
    } else if (url.find("sftp://") == 0) {
        return ServerProtocol::SFTP;
    }
    return ServerProtocol::HTTPS; // Default
}

os_error_t AppStoreServerManager::saveConfiguration() {
    // For now, just log - in real implementation would save to JSON file
    ESP_LOGI(TAG, "Configuration saved with %d servers", m_servers.size());
    return OS_OK;
}

os_error_t AppStoreServerManager::loadConfiguration() {
    // For now, return error to trigger default server creation
    return OS_ERROR_NOT_FOUND;
}

void AppStoreServerManager::addDefaultServers() {
    ESP_LOGI(TAG, "Adding default app store servers");
    
    // Official M5Stack App Store
    AppStoreServer officialServer;
    officialServer.name = "M5Stack Official Store";
    officialServer.url = "https://apps.m5stack.com";
    officialServer.description = "Official M5Stack application store";
    officialServer.protocol = ServerProtocol::HTTPS;
    officialServer.requiresAuth = false;
    officialServer.isDefault = true;
    officialServer.isEnabled = true;
    officialServer.timeout = 10000;
    officialServer.category = "Official";
    officialServer.tags = {"official", "m5stack", "verified"};
    
    addServer(officialServer);
    
    // Community App Store
    AppStoreServer communityServer;
    communityServer.name = "Community App Store";
    communityServer.url = "https://community.m5stack.com/apps";
    communityServer.description = "Community-driven application repository";
    communityServer.protocol = ServerProtocol::HTTPS;
    communityServer.requiresAuth = false;
    communityServer.isDefault = false;
    communityServer.isEnabled = true;
    communityServer.timeout = 15000;
    communityServer.category = "Community";
    communityServer.tags = {"community", "open-source", "contrib"};
    
    addServer(communityServer);
}

// AppStoreServerDialog Implementation

AppStoreServerDialog::AppStoreServerDialog(AppStoreServerManager& serverManager, 
                                          std::function<void(const AppStoreServer&)> onServerAdded)
    : m_serverManager(serverManager), m_onServerAdded(onServerAdded) {
}

AppStoreServerDialog::~AppStoreServerDialog() {
    closeDialog();
}

os_error_t AppStoreServerDialog::showAddServerDialog(lv_obj_t* parent) {
    if (!parent) {
        return OS_ERROR_INVALID_PARAM;
    }
    
    closeDialog(); // Close any existing dialog
    
    m_dialogMode = DialogMode::ADD_SERVER;
    m_editingServerId.clear();
    
    createAddServerDialogUI();
    
    // Position dialog in center of parent
    lv_obj_set_parent(m_dialogContainer, parent);
    lv_obj_center(m_dialogContainer);
    
    ESP_LOGI(TAG, "Showing add server dialog");
    return OS_OK;
}

os_error_t AppStoreServerDialog::showEditServerDialog(lv_obj_t* parent, const std::string& serverId) {
    if (!parent || serverId.empty()) {
        return OS_ERROR_INVALID_PARAM;
    }
    
    const AppStoreServer* server = m_serverManager.getServer(serverId);
    if (!server) {
        return OS_ERROR_NOT_FOUND;
    }
    
    closeDialog(); // Close any existing dialog
    
    m_dialogMode = DialogMode::EDIT_SERVER;
    m_editingServerId = serverId;
    
    createEditServerDialogUI();
    fillForm(*server);
    
    // Position dialog in center of parent
    lv_obj_set_parent(m_dialogContainer, parent);
    lv_obj_center(m_dialogContainer);
    
    ESP_LOGI(TAG, "Showing edit server dialog for: %s", server->name.c_str());
    return OS_OK;
}

os_error_t AppStoreServerDialog::showManagementDialog(lv_obj_t* parent) {
    if (!parent) {
        return OS_ERROR_INVALID_PARAM;
    }
    
    closeDialog(); // Close any existing dialog
    
    m_dialogMode = DialogMode::MANAGE_SERVERS;
    m_editingServerId.clear();
    
    createManagementDialogUI();
    updateServerList();
    
    // Position dialog in center of parent
    lv_obj_set_parent(m_dialogContainer, parent);
    lv_obj_center(m_dialogContainer);
    
    ESP_LOGI(TAG, "Showing server management dialog");
    return OS_OK;
}

void AppStoreServerDialog::closeDialog() {
    if (m_dialogContainer) {
        lv_obj_del(m_dialogContainer);
        m_dialogContainer = nullptr;
        
        // Reset all UI element pointers
        m_dialogPanel = nullptr;
        m_titleLabel = nullptr;
        m_formContainer = nullptr;
        m_nameTextArea = nullptr;
        m_urlTextArea = nullptr;
        m_descriptionTextArea = nullptr;
        m_protocolDropdown = nullptr;
        m_requiresAuthCheckbox = nullptr;
        m_usernameTextArea = nullptr;
        m_passwordTextArea = nullptr;
        m_apiKeyTextArea = nullptr;
        m_categoryTextArea = nullptr;
        m_timeoutSpinbox = nullptr;
        m_serverList = nullptr;
        m_statusLabel = nullptr;
        m_buttonContainer = nullptr;
        m_addButton = nullptr;
        m_updateButton = nullptr;
        m_testButton = nullptr;
        m_cancelButton = nullptr;
        m_removeButton = nullptr;
        m_enableButton = nullptr;
        m_setDefaultButton = nullptr;
        m_importButton = nullptr;
        m_exportButton = nullptr;
        m_refreshButton = nullptr;
    }
}

void AppStoreServerDialog::createAddServerDialogUI() {
    // Create modal dialog container
    m_dialogContainer = lv_obj_create(lv_scr_act());
    lv_obj_set_size(m_dialogContainer, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(m_dialogContainer, lv_color_make(0, 0, 0), 0);
    lv_obj_set_style_bg_opa(m_dialogContainer, LV_OPA_50, 0);
    lv_obj_set_style_border_opa(m_dialogContainer, LV_OPA_TRANSP, 0);
    
    // Create dialog panel
    m_dialogPanel = lv_obj_create(m_dialogContainer);
    lv_obj_set_size(m_dialogPanel, 600, 500);
    lv_obj_center(m_dialogPanel);
    lv_obj_set_style_bg_color(m_dialogPanel, COLOR_DIALOG_BG, 0);
    lv_obj_set_style_border_color(m_dialogPanel, COLOR_DIALOG_BORDER, 0);
    lv_obj_set_style_border_width(m_dialogPanel, 2, 0);
    lv_obj_set_style_radius(m_dialogPanel, 10, 0);
    lv_obj_set_style_pad_all(m_dialogPanel, 20, 0);
    
    // Title
    m_titleLabel = lv_label_create(m_dialogPanel);
    lv_label_set_text(m_titleLabel, "Add App Store Server");
    lv_obj_set_style_text_color(m_titleLabel, lv_color_white(), 0);
    lv_obj_set_style_text_font(m_titleLabel, &lv_font_montserrat_16, 0);
    lv_obj_align(m_titleLabel, LV_ALIGN_TOP_MID, 0, 0);
    
    // Create server form
    createServerForm(m_dialogPanel);
    
    // Create buttons
    m_buttonContainer = lv_obj_create(m_dialogPanel);
    lv_obj_set_size(m_buttonContainer, LV_PCT(100), 50);
    lv_obj_align(m_buttonContainer, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_opa(m_buttonContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(m_buttonContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(m_buttonContainer, 0, 0);
    lv_obj_set_flex_flow(m_buttonContainer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(m_buttonContainer, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Add button
    m_addButton = lv_btn_create(m_buttonContainer);
    lv_obj_set_size(m_addButton, 100, 40);
    lv_obj_set_style_bg_color(m_addButton, COLOR_SUCCESS, 0);
    
    lv_obj_t* addLabel = lv_label_create(m_addButton);
    lv_label_set_text(addLabel, "Add");
    lv_obj_center(addLabel);
    
    lv_obj_add_event_cb(m_addButton, addButtonCallback, LV_EVENT_CLICKED, this);
    
    // Test button
    m_testButton = lv_btn_create(m_buttonContainer);
    lv_obj_set_size(m_testButton, 100, 40);
    lv_obj_set_style_bg_color(m_testButton, COLOR_WARNING, 0);
    
    lv_obj_t* testLabel = lv_label_create(m_testButton);
    lv_label_set_text(testLabel, "Test");
    lv_obj_center(testLabel);
    
    lv_obj_add_event_cb(m_testButton, testButtonCallback, LV_EVENT_CLICKED, this);
    
    // Cancel button
    m_cancelButton = lv_btn_create(m_buttonContainer);
    lv_obj_set_size(m_cancelButton, 100, 40);
    lv_obj_set_style_bg_color(m_cancelButton, COLOR_ERROR, 0);
    
    lv_obj_t* cancelLabel = lv_label_create(m_cancelButton);
    lv_label_set_text(cancelLabel, "Cancel");
    lv_obj_center(cancelLabel);
    
    lv_obj_add_event_cb(m_cancelButton, cancelButtonCallback, LV_EVENT_CLICKED, this);
}

void AppStoreServerDialog::createEditServerDialogUI() {
    createAddServerDialogUI(); // Same UI structure
    
    // Update title
    lv_label_set_text(m_titleLabel, "Edit App Store Server");
    
    // Update button
    lv_obj_t* updateLabel = lv_label_create(m_addButton);
    lv_label_set_text(updateLabel, "Update");
    lv_obj_center(updateLabel);
    
    // Change callback
    lv_obj_remove_event_cb(m_addButton, addButtonCallback);
    lv_obj_add_event_cb(m_addButton, updateButtonCallback, LV_EVENT_CLICKED, this);
}

void AppStoreServerDialog::createManagementDialogUI() {
    // Create modal dialog container (larger for management)
    m_dialogContainer = lv_obj_create(lv_scr_act());
    lv_obj_set_size(m_dialogContainer, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(m_dialogContainer, lv_color_make(0, 0, 0), 0);
    lv_obj_set_style_bg_opa(m_dialogContainer, LV_OPA_50, 0);
    lv_obj_set_style_border_opa(m_dialogContainer, LV_OPA_TRANSP, 0);
    
    // Create dialog panel
    m_dialogPanel = lv_obj_create(m_dialogContainer);
    lv_obj_set_size(m_dialogPanel, 800, 600);
    lv_obj_center(m_dialogPanel);
    lv_obj_set_style_bg_color(m_dialogPanel, COLOR_DIALOG_BG, 0);
    lv_obj_set_style_border_color(m_dialogPanel, COLOR_DIALOG_BORDER, 0);
    lv_obj_set_style_border_width(m_dialogPanel, 2, 0);
    lv_obj_set_style_radius(m_dialogPanel, 10, 0);
    lv_obj_set_style_pad_all(m_dialogPanel, 20, 0);
    
    // Title
    m_titleLabel = lv_label_create(m_dialogPanel);
    lv_label_set_text(m_titleLabel, "Manage App Store Servers");
    lv_obj_set_style_text_color(m_titleLabel, lv_color_white(), 0);
    lv_obj_set_style_text_font(m_titleLabel, &lv_font_montserrat_16, 0);
    lv_obj_align(m_titleLabel, LV_ALIGN_TOP_MID, 0, 0);
    
    // Create server list
    createServerList(m_dialogPanel);
    
    // Status label
    m_statusLabel = lv_label_create(m_dialogPanel);
    lv_label_set_text(m_statusLabel, "Select a server to manage");
    lv_obj_set_style_text_color(m_statusLabel, lv_color_white(), 0);
    lv_obj_align_to(m_statusLabel, m_serverList, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
    
    // Create management buttons
    m_buttonContainer = lv_obj_create(m_dialogPanel);
    lv_obj_set_size(m_buttonContainer, LV_PCT(100), 50);
    lv_obj_align(m_buttonContainer, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_opa(m_buttonContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(m_buttonContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(m_buttonContainer, 0, 0);
    lv_obj_set_flex_flow(m_buttonContainer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(m_buttonContainer, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Refresh button
    m_refreshButton = lv_btn_create(m_buttonContainer);
    lv_obj_set_size(m_refreshButton, 80, 35);
    lv_obj_set_style_bg_color(m_refreshButton, COLOR_WARNING, 0);
    
    lv_obj_t* refreshLabel = lv_label_create(m_refreshButton);
    lv_label_set_text(refreshLabel, LV_SYMBOL_REFRESH);
    lv_obj_center(refreshLabel);
    
    lv_obj_add_event_cb(m_refreshButton, refreshButtonCallback, LV_EVENT_CLICKED, this);
    
    // Close button
    m_cancelButton = lv_btn_create(m_buttonContainer);
    lv_obj_set_size(m_cancelButton, 100, 35);
    lv_obj_set_style_bg_color(m_cancelButton, COLOR_ERROR, 0);
    
    lv_obj_t* closeLabel = lv_label_create(m_cancelButton);
    lv_label_set_text(closeLabel, "Close");
    lv_obj_center(closeLabel);
    
    lv_obj_add_event_cb(m_cancelButton, cancelButtonCallback, LV_EVENT_CLICKED, this);
}

void AppStoreServerDialog::createServerForm(lv_obj_t* parent) {
    // Form container
    m_formContainer = lv_obj_create(parent);
    lv_obj_set_size(m_formContainer, LV_PCT(100), 350);
    lv_obj_align_to(m_formContainer, m_titleLabel, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
    lv_obj_set_style_bg_opa(m_formContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(m_formContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(m_formContainer, 5, 0);
    lv_obj_set_scroll_dir(m_formContainer, LV_DIR_VER);
    
    int y_offset = 0;
    
    // Server name
    lv_obj_t* nameLabel = lv_label_create(m_formContainer);
    lv_label_set_text(nameLabel, "Server Name:");
    lv_obj_set_style_text_color(nameLabel, lv_color_white(), 0);
    lv_obj_set_pos(nameLabel, 0, y_offset);
    
    m_nameTextArea = lv_textarea_create(m_formContainer);
    lv_obj_set_size(m_nameTextArea, LV_PCT(100), 35);
    lv_obj_set_pos(m_nameTextArea, 0, y_offset + 25);
    lv_textarea_set_placeholder_text(m_nameTextArea, "Enter server name");
    lv_textarea_set_one_line(m_nameTextArea, true);
    
    y_offset += 70;
    
    // Server URL
    lv_obj_t* urlLabel = lv_label_create(m_formContainer);
    lv_label_set_text(urlLabel, "Server URL:");
    lv_obj_set_style_text_color(urlLabel, lv_color_white(), 0);
    lv_obj_set_pos(urlLabel, 0, y_offset);
    
    m_urlTextArea = lv_textarea_create(m_formContainer);
    lv_obj_set_size(m_urlTextArea, LV_PCT(100), 35);
    lv_obj_set_pos(m_urlTextArea, 0, y_offset + 25);
    lv_textarea_set_placeholder_text(m_urlTextArea, "https://example.com/apps");
    lv_textarea_set_one_line(m_urlTextArea, true);
    
    y_offset += 70;
    
    // Description
    lv_obj_t* descLabel = lv_label_create(m_formContainer);
    lv_label_set_text(descLabel, "Description:");
    lv_obj_set_style_text_color(descLabel, lv_color_white(), 0);
    lv_obj_set_pos(descLabel, 0, y_offset);
    
    m_descriptionTextArea = lv_textarea_create(m_formContainer);
    lv_obj_set_size(m_descriptionTextArea, LV_PCT(100), 60);
    lv_obj_set_pos(m_descriptionTextArea, 0, y_offset + 25);
    lv_textarea_set_placeholder_text(m_descriptionTextArea, "Optional description");
    
    y_offset += 95;
    
    // Protocol dropdown
    lv_obj_t* protocolLabel = lv_label_create(m_formContainer);
    lv_label_set_text(protocolLabel, "Protocol:");
    lv_obj_set_style_text_color(protocolLabel, lv_color_white(), 0);
    lv_obj_set_pos(protocolLabel, 0, y_offset);
    
    m_protocolDropdown = lv_dropdown_create(m_formContainer);
    lv_obj_set_size(m_protocolDropdown, 150, 35);
    lv_obj_set_pos(m_protocolDropdown, 0, y_offset + 25);
    lv_dropdown_set_options(m_protocolDropdown, "HTTPS\nHTTP\nFTP\nSFTP");
    lv_dropdown_set_selected(m_protocolDropdown, 0); // Default to HTTPS
    
    // Requires auth checkbox
    m_requiresAuthCheckbox = lv_checkbox_create(m_formContainer);
    lv_obj_set_pos(m_requiresAuthCheckbox, 200, y_offset + 25);
    lv_checkbox_set_text(m_requiresAuthCheckbox, "Requires Authentication");
    lv_obj_set_style_text_color(m_requiresAuthCheckbox, lv_color_white(), 0);
}

void AppStoreServerDialog::createServerList(lv_obj_t* parent) {
    // Server list container
    m_serverList = lv_list_create(parent);
    lv_obj_set_size(m_serverList, LV_PCT(100), 400);
    lv_obj_align_to(m_serverList, m_titleLabel, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
    lv_obj_set_style_bg_color(m_serverList, lv_color_make(0x1E, 0x1E, 0x1E), 0);
    lv_obj_set_style_border_color(m_serverList, COLOR_DIALOG_BORDER, 0);
    lv_obj_set_style_border_width(m_serverList, 1, 0);
}

void AppStoreServerDialog::updateServerList() {
    if (!m_serverList) return;
    
    // Clear existing items
    lv_obj_clean(m_serverList);
    
    // Add servers to list
    auto servers = m_serverManager.getConfiguredServers();
    for (const auto& server : servers) {
        lv_obj_t* btn = lv_list_add_btn(m_serverList, 
                                        server.isEnabled ? LV_SYMBOL_WIFI : LV_SYMBOL_CLOSE,
                                        server.name.c_str());
        
        // Set color based on status
        lv_color_t statusColor = lv_color_white();
        switch (server.status) {
            case ServerStatus::ONLINE:
                statusColor = COLOR_SUCCESS;
                break;
            case ServerStatus::OFFLINE:
            case ServerStatus::ERROR:
                statusColor = COLOR_ERROR;
                break;
            case ServerStatus::AUTHENTICATING:
                statusColor = COLOR_WARNING;
                break;
            default:
                statusColor = lv_color_make(0x80, 0x80, 0x80);
                break;
        }
        
        lv_obj_set_style_text_color(btn, statusColor, 0);
        
        // Store server ID in user data
        lv_obj_set_user_data(btn, (void*)server.id.c_str());
    }
}

// Continue with remaining implementation...

void AppStoreServerDialog::validateAndAddServer() {
    AppStoreServer server = getServerFromForm();
    
    ServerValidationResult result = m_serverManager.validateServer(server);
    if (result == ServerValidationResult::VALID) {
        os_error_t addResult = m_serverManager.addServer(server);
        if (addResult == OS_OK) {
            if (m_onServerAdded) {
                m_onServerAdded(server);
            }
            closeDialog();
        } else {
            showValidationResult(ServerValidationResult::NETWORK_ERROR);
        }
    } else {
        showValidationResult(result);
    }
}

void AppStoreServerDialog::testServerConnection(const AppStoreServer& server) {
    // Update status label
    if (m_statusLabel) {
        lv_label_set_text(m_statusLabel, "Testing connection...");
        lv_obj_set_style_text_color(m_statusLabel, COLOR_WARNING, 0);
    }
    
    // Simulate connection test
    // In real implementation, this would be async
    ServerValidationResult result = m_serverManager.validateServer(server);
    showValidationResult(result);
}

void AppStoreServerDialog::showValidationResult(ServerValidationResult result) {
    const char* message;
    lv_color_t color;
    
    switch (result) {
        case ServerValidationResult::VALID:
            message = "Server configuration is valid!";
            color = COLOR_SUCCESS;
            break;
        case ServerValidationResult::INVALID_URL:
            message = "Invalid URL format";
            color = COLOR_ERROR;
            break;
        case ServerValidationResult::INVALID_PROTOCOL:
            message = "Protocol mismatch with URL";
            color = COLOR_ERROR;
            break;
        case ServerValidationResult::CONNECTION_FAILED:
            message = "Connection failed";
            color = COLOR_ERROR;
            break;
        case ServerValidationResult::AUTHENTICATION_FAILED:
            message = "Authentication failed";
            color = COLOR_ERROR;
            break;
        case ServerValidationResult::TIMEOUT:
            message = "Connection timeout";
            color = COLOR_WARNING;
            break;
        case ServerValidationResult::UNSUPPORTED_API:
            message = "Unsupported API version";
            color = COLOR_WARNING;
            break;
        default:
            message = "Network error";
            color = COLOR_ERROR;
            break;
    }
    
    if (m_statusLabel) {
        lv_label_set_text(m_statusLabel, message);
        lv_obj_set_style_text_color(m_statusLabel, color, 0);
    }
}

AppStoreServer AppStoreServerDialog::getServerFromForm() {
    AppStoreServer server;
    
    if (m_nameTextArea) {
        server.name = lv_textarea_get_text(m_nameTextArea);
    }
    
    if (m_urlTextArea) {
        server.url = lv_textarea_get_text(m_urlTextArea);
    }
    
    if (m_descriptionTextArea) {
        server.description = lv_textarea_get_text(m_descriptionTextArea);
    }
    
    if (m_protocolDropdown) {
        uint16_t selected = lv_dropdown_get_selected(m_protocolDropdown);
        server.protocol = static_cast<ServerProtocol>(selected);
    }
    
    if (m_requiresAuthCheckbox) {
        server.requiresAuth = lv_obj_has_state(m_requiresAuthCheckbox, LV_STATE_CHECKED);
    }
    
    server.isEnabled = true;
    server.timeout = 10000;
    server.status = ServerStatus::UNKNOWN;
    
    return server;
}

void AppStoreServerDialog::clearForm() {
    if (m_nameTextArea) lv_textarea_set_text(m_nameTextArea, "");
    if (m_urlTextArea) lv_textarea_set_text(m_urlTextArea, "");
    if (m_descriptionTextArea) lv_textarea_set_text(m_descriptionTextArea, "");
    if (m_protocolDropdown) lv_dropdown_set_selected(m_protocolDropdown, 0);
    if (m_requiresAuthCheckbox) lv_obj_clear_state(m_requiresAuthCheckbox, LV_STATE_CHECKED);
}

void AppStoreServerDialog::fillForm(const AppStoreServer& server) {
    if (m_nameTextArea) lv_textarea_set_text(m_nameTextArea, server.name.c_str());
    if (m_urlTextArea) lv_textarea_set_text(m_urlTextArea, server.url.c_str());
    if (m_descriptionTextArea) lv_textarea_set_text(m_descriptionTextArea, server.description.c_str());
    if (m_protocolDropdown) lv_dropdown_set_selected(m_protocolDropdown, static_cast<uint16_t>(server.protocol));
    if (m_requiresAuthCheckbox) {
        if (server.requiresAuth) {
            lv_obj_add_state(m_requiresAuthCheckbox, LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(m_requiresAuthCheckbox, LV_STATE_CHECKED);
        }
    }
}

// Static callback implementations

void AppStoreServerDialog::addButtonCallback(lv_event_t* e) {
    AppStoreServerDialog* dialog = static_cast<AppStoreServerDialog*>(lv_event_get_user_data(e));
    if (dialog) {
        dialog->validateAndAddServer();
    }
}

void AppStoreServerDialog::cancelButtonCallback(lv_event_t* e) {
    AppStoreServerDialog* dialog = static_cast<AppStoreServerDialog*>(lv_event_get_user_data(e));
    if (dialog) {
        dialog->closeDialog();
    }
}

void AppStoreServerDialog::testButtonCallback(lv_event_t* e) {
    AppStoreServerDialog* dialog = static_cast<AppStoreServerDialog*>(lv_event_get_user_data(e));
    if (dialog) {
        AppStoreServer server = dialog->getServerFromForm();
        dialog->testServerConnection(server);
    }
}

void AppStoreServerDialog::refreshButtonCallback(lv_event_t* e) {
    AppStoreServerDialog* dialog = static_cast<AppStoreServerDialog*>(lv_event_get_user_data(e));
    if (dialog) {
        dialog->m_serverManager.refreshServerStatuses();
        dialog->updateServerList();
    }
}