#include "app_store_server_manager.h"
#include "../system/os_manager.h"
#include <esp_log.h>
#include <esp_http_client.h>
#include <cJSON.h>
#include <fstream>
#include <sstream>
#include <algorithm>

static const char* TAG = "AppStoreServerManager";

// AppStoreServerManager Implementation

os_error_t AppStoreServerManager::initialize() {
    if (m_initialized) {
        return OS_OK;
    }
    
    ESP_LOGI(TAG, "Initializing App Store Server Manager");
    
    // Load configuration from storage
    os_error_t result = loadConfiguration();
    if (result != OS_OK) {
        ESP_LOGW(TAG, "Failed to load configuration, using defaults");
        addDefaultServers();
    }
    
    m_initialized = true;
    ESP_LOGI(TAG, "App Store Server Manager initialized with %zu servers", m_servers.size());
    
    return OS_OK;
}

os_error_t AppStoreServerManager::shutdown() {
    if (!m_initialized) {
        return OS_OK;
    }
    
    ESP_LOGI(TAG, "Shutting down App Store Server Manager");
    
    // Save current configuration
    saveConfiguration();
    
    m_servers.clear();
    m_initialized = false;
    
    return OS_OK;
}

os_error_t AppStoreServerManager::addServer(const AppStoreServer& server) {
    if (!m_initialized) {
        return OS_ERROR_NOT_INITIALIZED;
    }
    
    // Validate server configuration
    ServerValidationResult validation = validateServer(server);
    if (validation != ServerValidationResult::VALID) {
        ESP_LOGW(TAG, "Server validation failed: %d", static_cast<int>(validation));
        return OS_ERROR_INVALID_PARAM;
    }
    
    // Check for duplicate IDs
    for (const auto& existingServer : m_servers) {
        if (existingServer.id == server.id) {
            ESP_LOGW(TAG, "Server with ID '%s' already exists", server.id.c_str());
            return OS_ERROR_ALREADY_EXISTS;
        }
    }
    
    // Add server to registry
    AppStoreServer newServer = server;
    newServer.addedTime = millis();
    newServer.status = ServerStatus::UNKNOWN;
    
    m_servers.push_back(newServer);
    
    // Save configuration
    saveConfiguration();
    
    ESP_LOGI(TAG, "Added server: %s (%s)", newServer.name.c_str(), newServer.url.c_str());
    
    return OS_OK;
}

os_error_t AppStoreServerManager::removeServer(const std::string& serverId) {
    if (!m_initialized) {
        return OS_ERROR_NOT_INITIALIZED;
    }
    
    auto it = std::find_if(m_servers.begin(), m_servers.end(),
        [&serverId](const AppStoreServer& server) {
            return server.id == serverId;
        });
    
    if (it == m_servers.end()) {
        ESP_LOGW(TAG, "Server not found: %s", serverId.c_str());
        return OS_ERROR_NOT_FOUND;
    }
    
    // Don't allow removal of default server if it's the only one
    if (it->isDefault && m_servers.size() == 1) {
        ESP_LOGW(TAG, "Cannot remove the only default server");
        return OS_ERROR_OPERATION_NOT_PERMITTED;
    }
    
    // If removing default server, set another as default
    if (it->isDefault && m_servers.size() > 1) {
        for (auto& server : m_servers) {
            if (server.id != serverId) {
                server.isDefault = true;
                m_defaultServerId = server.id;
                break;
            }
        }
    }
    
    ESP_LOGI(TAG, "Removing server: %s", it->name.c_str());
    m_servers.erase(it);
    
    // Save configuration
    saveConfiguration();
    
    return OS_OK;
}

os_error_t AppStoreServerManager::updateServer(const std::string& serverId, const AppStoreServer& server) {
    if (!m_initialized) {
        return OS_ERROR_NOT_INITIALIZED;
    }
    
    auto it = std::find_if(m_servers.begin(), m_servers.end(),
        [&serverId](const AppStoreServer& server) {
            return server.id == serverId;
        });
    
    if (it == m_servers.end()) {
        return OS_ERROR_NOT_FOUND;
    }
    
    // Validate updated server configuration
    ServerValidationResult validation = validateServer(server);
    if (validation != ServerValidationResult::VALID) {
        return OS_ERROR_INVALID_PARAM;
    }
    
    // Update server while preserving some fields
    uint32_t oldAddedTime = it->addedTime;
    bool oldIsDefault = it->isDefault;
    
    *it = server;
    it->addedTime = oldAddedTime;
    it->isDefault = oldIsDefault;
    it->status = ServerStatus::UNKNOWN; // Reset status for re-validation
    
    // Save configuration
    saveConfiguration();
    
    ESP_LOGI(TAG, "Updated server: %s", server.name.c_str());
    
    return OS_OK;
}

os_error_t AppStoreServerManager::setServerEnabled(const std::string& serverId, bool enabled) {
    auto it = std::find_if(m_servers.begin(), m_servers.end(),
        [&serverId](const AppStoreServer& server) {
            return server.id == serverId;
        });
    
    if (it == m_servers.end()) {
        return OS_ERROR_NOT_FOUND;
    }
    
    it->isEnabled = enabled;
    saveConfiguration();
    
    ESP_LOGI(TAG, "%s server: %s", enabled ? "Enabled" : "Disabled", it->name.c_str());
    
    return OS_OK;
}

ServerValidationResult AppStoreServerManager::validateServer(const AppStoreServer& server) {
    // Check required fields
    if (server.name.empty() || server.url.empty()) {
        return ServerValidationResult::INVALID_URL;
    }
    
    // Validate URL format
    if (!isValidUrl(server.url)) {
        return ServerValidationResult::INVALID_URL;
    }
    
    // Validate protocol
    ServerProtocol detectedProtocol = parseProtocol(server.url);
    if (detectedProtocol == ServerProtocol::HTTPS || detectedProtocol == ServerProtocol::HTTP) {
        // Valid protocols
    } else {
        return ServerValidationResult::INVALID_PROTOCOL;
    }
    
    // Additional validation could include:
    // - DNS resolution check
    // - SSL certificate validation
    // - API endpoint verification
    
    return ServerValidationResult::VALID;
}

os_error_t AppStoreServerManager::testServerConnection(const std::string& serverId) {
    auto it = std::find_if(m_servers.begin(), m_servers.end(),
        [&serverId](const AppStoreServer& server) {
            return server.id == serverId;
        });
    
    if (it == m_servers.end()) {
        return OS_ERROR_NOT_FOUND;
    }
    
    ESP_LOGI(TAG, "Testing connection to server: %s", it->name.c_str());
    it->status = ServerStatus::AUTHENTICATING;
    
    // TODO: Implement actual HTTP/HTTPS connection test
    // For now, simulate connection test
    
    // Simulate network delay
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Mock result based on URL validity
    if (it->url.find("https://") == 0 || it->url.find("http://") == 0) {
        it->status = ServerStatus::ONLINE;
        it->lastChecked = millis();
        ESP_LOGI(TAG, "Server connection test passed: %s", it->name.c_str());
        return OS_OK;
    } else {
        it->status = ServerStatus::ERROR;
        ESP_LOGW(TAG, "Server connection test failed: %s", it->name.c_str());
        return OS_ERROR_NETWORK_UNREACHABLE;
    }
}

std::vector<AppStoreServer> AppStoreServerManager::getConfiguredServers() const {
    return m_servers;
}

std::vector<AppStoreServer> AppStoreServerManager::getEnabledServers() const {
    std::vector<AppStoreServer> enabledServers;
    std::copy_if(m_servers.begin(), m_servers.end(), std::back_inserter(enabledServers),
        [](const AppStoreServer& server) {
            return server.isEnabled;
        });
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
    auto it = std::find_if(m_servers.begin(), m_servers.end(),
        [](const AppStoreServer& server) {
            return server.isDefault;
        });
    
    return (it != m_servers.end()) ? &(*it) : nullptr;
}

os_error_t AppStoreServerManager::setDefaultServer(const std::string& serverId) {
    // Clear current default
    for (auto& server : m_servers) {
        server.isDefault = false;
    }
    
    // Set new default
    auto it = std::find_if(m_servers.begin(), m_servers.end(),
        [&serverId](const AppStoreServer& server) {
            return server.id == serverId;
        });
    
    if (it == m_servers.end()) {
        return OS_ERROR_NOT_FOUND;
    }
    
    it->isDefault = true;
    m_defaultServerId = serverId;
    
    saveConfiguration();
    
    ESP_LOGI(TAG, "Set default server: %s", it->name.c_str());
    
    return OS_OK;
}

os_error_t AppStoreServerManager::refreshServerStatuses() {
    if (!m_initialized) {
        return OS_ERROR_NOT_INITIALIZED;
    }
    
    ESP_LOGI(TAG, "Refreshing server statuses");
    
    for (auto& server : m_servers) {
        if (server.isEnabled) {
            testServerConnection(server.id);
        }
    }
    
    m_lastRefresh = millis();
    
    return OS_OK;
}

os_error_t AppStoreServerManager::importServersFromConfig(const std::string& configPath) {
    // TODO: Implement JSON configuration file import
    ESP_LOGI(TAG, "Importing servers from: %s", configPath.c_str());
    
    // Mock implementation
    return OS_ERROR_NOT_IMPLEMENTED;
}

os_error_t AppStoreServerManager::exportServersToConfig(const std::string& configPath) {
    // TODO: Implement JSON configuration file export
    ESP_LOGI(TAG, "Exporting servers to: %s", configPath.c_str());
    
    // Mock implementation
    return OS_ERROR_NOT_IMPLEMENTED;
}

std::string AppStoreServerManager::generateServerId(const std::string& baseName) {
    std::string id = baseName;
    std::transform(id.begin(), id.end(), id.begin(), ::tolower);
    
    // Replace spaces and special characters with underscores
    for (char& c : id) {
        if (!std::isalnum(c)) {
            c = '_';
        }
    }
    
    // Ensure uniqueness
    int counter = 1;
    std::string uniqueId = id;
    
    while (getServer(uniqueId) != nullptr) {
        uniqueId = id + "_" + std::to_string(counter++);
    }
    
    return uniqueId;
}

bool AppStoreServerManager::isValidUrl(const std::string& url) const {
    // Basic URL validation
    if (url.empty() || url.length() < 7) {
        return false;
    }
    
    // Check for valid protocol
    if (url.find("http://") != 0 && url.find("https://") != 0 && 
        url.find("ftp://") != 0 && url.find("sftp://") != 0) {
        return false;
    }
    
    // Additional validation could be added here
    
    return true;
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
    
    return ServerProtocol::HTTP; // Default fallback
}

os_error_t AppStoreServerManager::saveConfiguration() {
    // TODO: Implement persistent storage of server configuration
    // For now, just log the action
    ESP_LOGI(TAG, "Saving configuration with %zu servers", m_servers.size());
    return OS_OK;
}

os_error_t AppStoreServerManager::loadConfiguration() {
    // TODO: Implement loading from persistent storage
    // For now, return error to trigger default server creation
    ESP_LOGI(TAG, "Loading configuration from storage");
    return OS_ERROR_NOT_FOUND;
}

void AppStoreServerManager::addDefaultServers() {
    ESP_LOGI(TAG, "Adding default app store servers");
    
    // Official M5Stack App Store
    AppStoreServer officialServer;
    officialServer.id = "m5stack_official";
    officialServer.name = "M5Stack Official";
    officialServer.url = "https://apps.m5stack.com/api/v1";
    officialServer.description = "Official M5Stack app store with verified applications";
    officialServer.protocol = ServerProtocol::HTTPS;
    officialServer.status = ServerStatus::UNKNOWN;
    officialServer.requiresAuth = false;
    officialServer.isDefault = true;
    officialServer.isEnabled = true;
    officialServer.timeout = m_defaultTimeout;
    officialServer.category = "Official";
    officialServer.tags = {"official", "verified", "m5stack"};
    
    m_servers.push_back(officialServer);
    m_defaultServerId = officialServer.id;
    
    // Community App Store
    AppStoreServer communityServer;
    communityServer.id = "community_store";
    communityServer.name = "Community Store";
    communityServer.url = "https://community.m5stack.com/apps/api";
    communityServer.description = "Community-driven app store with user-contributed applications";
    communityServer.protocol = ServerProtocol::HTTPS;
    communityServer.status = ServerStatus::UNKNOWN;
    communityServer.requiresAuth = false;
    communityServer.isDefault = false;
    communityServer.isEnabled = true;
    communityServer.timeout = m_defaultTimeout;
    communityServer.category = "Community";
    communityServer.tags = {"community", "open-source", "user-contributed"};
    
    m_servers.push_back(communityServer);
    
    ESP_LOGI(TAG, "Added %zu default servers", m_servers.size());
}

// AppStoreServerDialog Implementation

AppStoreServerDialog::AppStoreServerDialog(AppStoreServerManager& serverManager, 
                                          std::function<void(const AppStoreServer&)> onServerAdded)
    : m_serverManager(serverManager), m_onServerAdded(onServerAdded) {
    m_dialogMode = DialogMode::ADD_SERVER;
}

AppStoreServerDialog::~AppStoreServerDialog() {
    closeDialog();
}

os_error_t AppStoreServerDialog::showAddServerDialog(lv_obj_t* parent) {
    if (!parent) {
        return OS_ERROR_INVALID_PARAM;
    }
    
    if (m_dialogContainer) {
        closeDialog();
    }
    
    m_dialogMode = DialogMode::ADD_SERVER;
    
    ESP_LOGI(TAG, "Showing add server dialog");
    
    // Create dialog container
    m_dialogContainer = lv_obj_create(parent);
    lv_obj_set_size(m_dialogContainer, LV_HOR_RES, LV_VER_RES);
    lv_obj_center(m_dialogContainer);
    lv_obj_set_style_bg_color(m_dialogContainer, lv_color_make(0, 0, 0), 0);
    lv_obj_set_style_bg_opa(m_dialogContainer, LV_OPA_50, 0);
    lv_obj_set_style_border_opa(m_dialogContainer, LV_OPA_TRANSP, 0);
    
    // Create dialog panel
    m_dialogPanel = lv_obj_create(m_dialogContainer);
    lv_obj_set_size(m_dialogPanel, LV_HOR_RES - 100, LV_VER_RES - 100);
    lv_obj_center(m_dialogPanel);
    lv_obj_set_style_bg_color(m_dialogPanel, COLOR_DIALOG_BG, 0);
    lv_obj_set_style_border_color(m_dialogPanel, COLOR_DIALOG_BORDER, 0);
    lv_obj_set_style_border_width(m_dialogPanel, 2, 0);
    lv_obj_set_style_pad_all(m_dialogPanel, 20, 0);
    
    createAddServerDialogUI();
    
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
    
    if (m_dialogContainer) {
        closeDialog();
    }
    
    m_dialogMode = DialogMode::EDIT_SERVER;
    m_editingServerId = serverId;
    
    ESP_LOGI(TAG, "Showing edit server dialog for: %s", serverId.c_str());
    
    // Create dialog (similar to add dialog)
    showAddServerDialog(parent);
    
    // Update title and fill form with existing data
    if (m_titleLabel) {
        lv_label_set_text(m_titleLabel, "Edit App Store Server");
    }
    
    fillForm(*server);
    
    return OS_OK;
}

os_error_t AppStoreServerDialog::showManagementDialog(lv_obj_t* parent) {
    if (!parent) {
        return OS_ERROR_INVALID_PARAM;
    }
    
    if (m_dialogContainer) {
        closeDialog();
    }
    
    m_dialogMode = DialogMode::MANAGE_SERVERS;
    
    ESP_LOGI(TAG, "Showing server management dialog");
    
    // Create dialog container and panel (similar to add dialog)
    // Implementation would be similar to showAddServerDialog but with different UI
    
    return OS_ERROR_NOT_IMPLEMENTED; // TODO: Implement management dialog
}

void AppStoreServerDialog::closeDialog() {
    if (m_dialogContainer) {
        lv_obj_del(m_dialogContainer);
        m_dialogContainer = nullptr;
        m_dialogPanel = nullptr;
        
        // Reset all UI pointers
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
        m_buttonContainer = nullptr;
        m_addButton = nullptr;
        m_updateButton = nullptr;
        m_testButton = nullptr;
        m_cancelButton = nullptr;
        
        ESP_LOGI(TAG, "Dialog closed");
    }
}

void AppStoreServerDialog::createAddServerDialogUI() {
    // Create title
    m_titleLabel = lv_label_create(m_dialogPanel);
    lv_label_set_text(m_titleLabel, "Add App Store Server");
    lv_obj_set_style_text_font(m_titleLabel, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(m_titleLabel, lv_color_white(), 0);
    lv_obj_align(m_titleLabel, LV_ALIGN_TOP_MID, 0, 10);
    
    // Create form
    createServerForm(m_dialogPanel);
    
    // Create buttons
    m_buttonContainer = lv_obj_create(m_dialogPanel);
    lv_obj_set_size(m_buttonContainer, lv_pct(100), 60);
    lv_obj_align(m_buttonContainer, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_opa(m_buttonContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(m_buttonContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(m_buttonContainer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(m_buttonContainer, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Add button
    m_addButton = lv_btn_create(m_buttonContainer);
    lv_obj_set_size(m_addButton, 100, 40);
    lv_obj_add_event_cb(m_addButton, addButtonCallback, LV_EVENT_CLICKED, this);
    lv_obj_set_style_bg_color(m_addButton, COLOR_SUCCESS, 0);
    lv_obj_t* addLabel = lv_label_create(m_addButton);
    lv_label_set_text(addLabel, "Add");
    lv_obj_center(addLabel);
    
    // Test button
    m_testButton = lv_btn_create(m_buttonContainer);
    lv_obj_set_size(m_testButton, 100, 40);
    lv_obj_add_event_cb(m_testButton, testButtonCallback, LV_EVENT_CLICKED, this);
    lv_obj_set_style_bg_color(m_testButton, COLOR_WARNING, 0);
    lv_obj_t* testLabel = lv_label_create(m_testButton);
    lv_label_set_text(testLabel, "Test");
    lv_obj_center(testLabel);
    
    // Cancel button
    m_cancelButton = lv_btn_create(m_buttonContainer);
    lv_obj_set_size(m_cancelButton, 100, 40);
    lv_obj_add_event_cb(m_cancelButton, cancelButtonCallback, LV_EVENT_CLICKED, this);
    lv_obj_set_style_bg_color(m_cancelButton, COLOR_ERROR, 0);
    lv_obj_t* cancelLabel = lv_label_create(m_cancelButton);
    lv_label_set_text(cancelLabel, "Cancel");
    lv_obj_center(cancelLabel);
}

void AppStoreServerDialog::createServerForm(lv_obj_t* parent) {
    // Create form container with scrollable content
    m_formContainer = lv_obj_create(parent);
    lv_obj_set_size(m_formContainer, lv_pct(100), lv_pct(70));
    lv_obj_align_to(m_formContainer, m_titleLabel, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
    lv_obj_set_style_bg_opa(m_formContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(m_formContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_scroll_dir(m_formContainer, LV_DIR_VER);
    
    // Server name field
    lv_obj_t* nameLabel = lv_label_create(m_formContainer);
    lv_label_set_text(nameLabel, "Server Name:");
    lv_obj_set_style_text_color(nameLabel, lv_color_white(), 0);
    
    m_nameTextArea = lv_textarea_create(m_formContainer);
    lv_obj_set_size(m_nameTextArea, lv_pct(100), 40);
    lv_obj_align_to(m_nameTextArea, nameLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    lv_textarea_set_placeholder_text(m_nameTextArea, "Enter server name");
    lv_textarea_set_one_line(m_nameTextArea, true);
    
    // Server URL field
    lv_obj_t* urlLabel = lv_label_create(m_formContainer);
    lv_label_set_text(urlLabel, "Server URL:");
    lv_obj_set_style_text_color(urlLabel, lv_color_white(), 0);
    lv_obj_align_to(urlLabel, m_nameTextArea, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 15);
    
    m_urlTextArea = lv_textarea_create(m_formContainer);
    lv_obj_set_size(m_urlTextArea, lv_pct(100), 40);
    lv_obj_align_to(m_urlTextArea, urlLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    lv_textarea_set_placeholder_text(m_urlTextArea, "https://example.com/api");
    lv_textarea_set_one_line(m_urlTextArea, true);
    
    // Description field
    lv_obj_t* descLabel = lv_label_create(m_formContainer);
    lv_label_set_text(descLabel, "Description:");
    lv_obj_set_style_text_color(descLabel, lv_color_white(), 0);
    lv_obj_align_to(descLabel, m_urlTextArea, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 15);
    
    m_descriptionTextArea = lv_textarea_create(m_formContainer);
    lv_obj_set_size(m_descriptionTextArea, lv_pct(100), 60);
    lv_obj_align_to(m_descriptionTextArea, descLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    lv_textarea_set_placeholder_text(m_descriptionTextArea, "Server description (optional)");
    
    // Additional form fields would be added here...
    // For brevity, I'm showing just the essential fields
}

void AppStoreServerDialog::validateAndAddServer() {
    AppStoreServer server = getServerFromForm();
    
    // Generate unique ID
    server.id = m_serverManager.generateServerId(server.name);
    
    // Validate server
    ServerValidationResult validation = m_serverManager.validateServer(server);
    if (validation != ServerValidationResult::VALID) {
        showValidationResult(validation);
        return;
    }
    
    // Add server
    os_error_t result = m_serverManager.addServer(server);
    if (result == OS_OK) {
        ESP_LOGI(TAG, "Server added successfully: %s", server.name.c_str());
        
        if (m_onServerAdded) {
            m_onServerAdded(server);
        }
        
        closeDialog();
    } else {
        ESP_LOGE(TAG, "Failed to add server: %d", result);
        // Show error message to user
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
    
    // Set defaults
    server.protocol = ServerProtocol::HTTPS;
    server.status = ServerStatus::UNKNOWN;
    server.requiresAuth = false;
    server.isDefault = false;
    server.isEnabled = true;
    server.timeout = 10000;
    server.category = "Custom";
    
    return server;
}

void AppStoreServerDialog::fillForm(const AppStoreServer& server) {
    if (m_nameTextArea) {
        lv_textarea_set_text(m_nameTextArea, server.name.c_str());
    }
    
    if (m_urlTextArea) {
        lv_textarea_set_text(m_urlTextArea, server.url.c_str());
    }
    
    if (m_descriptionTextArea) {
        lv_textarea_set_text(m_descriptionTextArea, server.description.c_str());
    }
    
    // Fill other form fields as needed
}

void AppStoreServerDialog::showValidationResult(ServerValidationResult result) {
    const char* message;
    
    switch (result) {
        case ServerValidationResult::INVALID_URL:
            message = "Invalid URL format";
            break;
        case ServerValidationResult::INVALID_PROTOCOL:
            message = "Unsupported protocol";
            break;
        case ServerValidationResult::CONNECTION_FAILED:
            message = "Connection failed";
            break;
        case ServerValidationResult::AUTHENTICATION_FAILED:
            message = "Authentication failed";
            break;
        case ServerValidationResult::TIMEOUT:
            message = "Connection timeout";
            break;
        default:
            message = "Validation failed";
            break;
    }
    
    ESP_LOGW(TAG, "Server validation failed: %s", message);
    
    // TODO: Show user-friendly error dialog
}

// UI Event Callbacks

void AppStoreServerDialog::addButtonCallback(lv_event_t* e) {
    AppStoreServerDialog* dialog = static_cast<AppStoreServerDialog*>(lv_event_get_user_data(e));
    if (dialog->m_dialogMode == DialogMode::ADD_SERVER) {
        dialog->validateAndAddServer();
    } else if (dialog->m_dialogMode == DialogMode::EDIT_SERVER) {
        dialog->validateAndUpdateServer();
    }
}

void AppStoreServerDialog::cancelButtonCallback(lv_event_t* e) {
    AppStoreServerDialog* dialog = static_cast<AppStoreServerDialog*>(lv_event_get_user_data(e));
    dialog->closeDialog();
}

void AppStoreServerDialog::testButtonCallback(lv_event_t* e) {
    AppStoreServerDialog* dialog = static_cast<AppStoreServerDialog*>(lv_event_get_user_data(e));
    
    AppStoreServer server = dialog->getServerFromForm();
    ServerValidationResult validation = dialog->m_serverManager.validateServer(server);
    dialog->showValidationResult(validation);
}

void AppStoreServerDialog::validateAndUpdateServer() {
    // Similar to validateAndAddServer but for updates
    // Implementation would call updateServer instead of addServer
}

// Placeholder implementations for other callbacks
void AppStoreServerDialog::updateButtonCallback(lv_event_t* e) {}
void AppStoreServerDialog::removeButtonCallback(lv_event_t* e) {}
void AppStoreServerDialog::enableButtonCallback(lv_event_t* e) {}
void AppStoreServerDialog::setDefaultButtonCallback(lv_event_t* e) {}
void AppStoreServerDialog::importButtonCallback(lv_event_t* e) {}
void AppStoreServerDialog::exportButtonCallback(lv_event_t* e) {}
void AppStoreServerDialog::refreshButtonCallback(lv_event_t* e) {}

// Placeholder implementations for other methods
void AppStoreServerDialog::createEditServerDialogUI() {}
void AppStoreServerDialog::createManagementDialogUI() {}
void AppStoreServerDialog::createServerList(lv_obj_t* parent) {}
void AppStoreServerDialog::updateServerList() {}
void AppStoreServerDialog::testServerConnection(const AppStoreServer& server) {}
void AppStoreServerDialog::clearForm() {}