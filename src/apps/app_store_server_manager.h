#ifndef APP_STORE_SERVER_MANAGER_H
#define APP_STORE_SERVER_MANAGER_H

#include "../system/os_config.h"
#include <string>
#include <vector>
#include <functional>
#include <lvgl.h>

/**
 * @file app_store_server_manager.h
 * @brief App Store Server Management for M5Stack Tab5 OS
 * 
 * Manages custom app store servers, provides dialog for adding servers,
 * and handles server validation and configuration.
 */

enum class ServerProtocol {
    HTTPS,
    HTTP,
    FTP,
    SFTP
};

enum class ServerStatus {
    UNKNOWN,
    ONLINE,
    OFFLINE,
    ERROR,
    AUTHENTICATING
};

struct AppStoreServer {
    std::string id;
    std::string name;
    std::string url;
    std::string description;
    ServerProtocol protocol;
    ServerStatus status;
    bool requiresAuth;
    std::string username;
    std::string password;
    std::string apiKey;
    bool isDefault;
    bool isEnabled;
    uint32_t timeout;
    uint32_t lastChecked;
    uint32_t addedTime;
    size_t totalApps;
    std::string category;
    std::vector<std::string> tags;
};

enum class ServerValidationResult {
    VALID,
    INVALID_URL,
    INVALID_PROTOCOL,
    CONNECTION_FAILED,
    AUTHENTICATION_FAILED,
    TIMEOUT,
    UNSUPPORTED_API,
    NETWORK_ERROR
};

/**
 * @brief App Store Server Manager
 * Handles server configuration, validation, and management
 */
class AppStoreServerManager {
public:
    AppStoreServerManager() = default;
    ~AppStoreServerManager() = default;

    /**
     * @brief Initialize server manager
     * @return OS_OK on success, error code on failure
     */
    os_error_t initialize();

    /**
     * @brief Shutdown server manager
     * @return OS_OK on success, error code on failure
     */
    os_error_t shutdown();

    /**
     * @brief Add a new app store server
     * @param server Server configuration
     * @return OS_OK on success, error code on failure
     */
    os_error_t addServer(const AppStoreServer& server);

    /**
     * @brief Remove an app store server
     * @param serverId Server ID to remove
     * @return OS_OK on success, error code on failure
     */
    os_error_t removeServer(const std::string& serverId);

    /**
     * @brief Update server configuration
     * @param serverId Server ID to update
     * @param server Updated server configuration
     * @return OS_OK on success, error code on failure
     */
    os_error_t updateServer(const std::string& serverId, const AppStoreServer& server);

    /**
     * @brief Enable/disable a server
     * @param serverId Server ID
     * @param enabled Enable state
     * @return OS_OK on success, error code on failure
     */
    os_error_t setServerEnabled(const std::string& serverId, bool enabled);

    /**
     * @brief Validate server configuration
     * @param server Server to validate
     * @return Validation result
     */
    ServerValidationResult validateServer(const AppStoreServer& server);

    /**
     * @brief Test server connectivity
     * @param serverId Server ID to test
     * @return OS_OK on success, error code on failure
     */
    os_error_t testServerConnection(const std::string& serverId);

    /**
     * @brief Get list of configured servers
     * @return Vector of configured servers
     */
    std::vector<AppStoreServer> getConfiguredServers() const;

    /**
     * @brief Get list of enabled servers
     * @return Vector of enabled servers
     */
    std::vector<AppStoreServer> getEnabledServers() const;

    /**
     * @brief Get server by ID
     * @param serverId Server ID
     * @return Pointer to server or nullptr if not found
     */
    const AppStoreServer* getServer(const std::string& serverId) const;

    /**
     * @brief Get default server
     * @return Pointer to default server or nullptr if none set
     */
    const AppStoreServer* getDefaultServer() const;

    /**
     * @brief Set default server
     * @param serverId Server ID to set as default
     * @return OS_OK on success, error code on failure
     */
    os_error_t setDefaultServer(const std::string& serverId);

    /**
     * @brief Refresh all server statuses
     * @return OS_OK on success, error code on failure
     */
    os_error_t refreshServerStatuses();

    /**
     * @brief Import servers from configuration file
     * @param configPath Path to configuration file
     * @return OS_OK on success, error code on failure
     */
    os_error_t importServersFromConfig(const std::string& configPath);

    /**
     * @brief Export servers to configuration file
     * @param configPath Path to save configuration
     * @return OS_OK on success, error code on failure
     */
    os_error_t exportServersToConfig(const std::string& configPath);

    /**
     * @brief Generate unique server ID
     * @param baseName Base name for ID generation
     * @return Unique server ID
     */
    std::string generateServerId(const std::string& baseName);

private:

    /**
     * @brief Validate URL format
     * @param url URL to validate
     * @return true if valid, false otherwise
     */
    bool isValidUrl(const std::string& url) const;

    /**
     * @brief Parse protocol from URL
     * @param url URL to parse
     * @return Detected protocol
     */
    ServerProtocol parseProtocol(const std::string& url) const;

    /**
     * @brief Save server configuration to storage
     * @return OS_OK on success, error code on failure
     */
    os_error_t saveConfiguration();

    /**
     * @brief Load server configuration from storage
     * @return OS_OK on success, error code on failure
     */
    os_error_t loadConfiguration();

    /**
     * @brief Add default servers on first initialization
     */
    void addDefaultServers();

    // Server registry
    std::vector<AppStoreServer> m_servers;
    std::string m_defaultServerId;
    
    // Configuration
    std::string m_configPath = "/config/app_servers.json";
    uint32_t m_defaultTimeout = 10000; // 10 seconds
    
    // State
    bool m_initialized = false;
    uint32_t m_lastRefresh = 0;
    static constexpr uint32_t REFRESH_INTERVAL_MS = 300000; // 5 minutes
};

/**
 * @brief App Store Server Dialog
 * Provides UI for adding and managing app store servers
 */
class AppStoreServerDialog {
public:
    /**
     * @brief Constructor
     * @param serverManager Reference to server manager
     * @param onServerAdded Callback when server is added
     */
    AppStoreServerDialog(AppStoreServerManager& serverManager, 
                        std::function<void(const AppStoreServer&)> onServerAdded = nullptr);
    
    /**
     * @brief Destructor
     */
    ~AppStoreServerDialog();

    /**
     * @brief Show add server dialog
     * @param parent Parent LVGL object
     * @return OS_OK on success, error code on failure
     */
    os_error_t showAddServerDialog(lv_obj_t* parent);

    /**
     * @brief Show edit server dialog
     * @param parent Parent LVGL object
     * @param serverId Server ID to edit
     * @return OS_OK on success, error code on failure
     */
    os_error_t showEditServerDialog(lv_obj_t* parent, const std::string& serverId);

    /**
     * @brief Show server management dialog
     * @param parent Parent LVGL object
     * @return OS_OK on success, error code on failure
     */
    os_error_t showManagementDialog(lv_obj_t* parent);

    /**
     * @brief Close dialog
     */
    void closeDialog();

    /**
     * @brief Check if dialog is open
     * @return true if open, false otherwise
     */
    bool isDialogOpen() const { return m_dialogContainer != nullptr; }

private:
    /**
     * @brief Create add server dialog UI
     */
    void createAddServerDialogUI();

    /**
     * @brief Create edit server dialog UI
     */
    void createEditServerDialogUI();

    /**
     * @brief Create server management dialog UI
     */
    void createManagementDialogUI();

    /**
     * @brief Create server form fields
     * @param parent Parent container
     */
    void createServerForm(lv_obj_t* parent);

    /**
     * @brief Create server list for management
     * @param parent Parent container
     */
    void createServerList(lv_obj_t* parent);

    /**
     * @brief Update server list display
     */
    void updateServerList();

    /**
     * @brief Validate and add server
     */
    void validateAndAddServer();

    /**
     * @brief Validate and update server
     */
    void validateAndUpdateServer();

    /**
     * @brief Test server connection
     * @param server Server to test
     */
    void testServerConnection(const AppStoreServer& server);

    /**
     * @brief Show validation result
     * @param result Validation result
     */
    void showValidationResult(ServerValidationResult result);

    /**
     * @brief Clear form fields
     */
    void clearForm();

    /**
     * @brief Fill form with server data
     * @param server Server data to fill
     */
    void fillForm(const AppStoreServer& server);

    /**
     * @brief Get server data from form
     * @return Server configuration from form
     */
    AppStoreServer getServerFromForm();

    // UI event callbacks
    static void addButtonCallback(lv_event_t* e);
    static void updateButtonCallback(lv_event_t* e);
    static void cancelButtonCallback(lv_event_t* e);
    static void testButtonCallback(lv_event_t* e);
    static void removeButtonCallback(lv_event_t* e);
    static void enableButtonCallback(lv_event_t* e);
    static void setDefaultButtonCallback(lv_event_t* e);
    static void importButtonCallback(lv_event_t* e);
    static void exportButtonCallback(lv_event_t* e);
    static void refreshButtonCallback(lv_event_t* e);

    AppStoreServerManager& m_serverManager;
    std::function<void(const AppStoreServer&)> m_onServerAdded;
    
    // Dialog state
    enum class DialogMode {
        ADD_SERVER,
        EDIT_SERVER,
        MANAGE_SERVERS
    } m_dialogMode;
    
    std::string m_editingServerId;
    
    // UI elements - Dialog container
    lv_obj_t* m_dialogContainer = nullptr;
    lv_obj_t* m_dialogPanel = nullptr;
    lv_obj_t* m_titleLabel = nullptr;
    
    // UI elements - Server form
    lv_obj_t* m_formContainer = nullptr;
    lv_obj_t* m_nameTextArea = nullptr;
    lv_obj_t* m_urlTextArea = nullptr;
    lv_obj_t* m_descriptionTextArea = nullptr;
    lv_obj_t* m_protocolDropdown = nullptr;
    lv_obj_t* m_requiresAuthCheckbox = nullptr;
    lv_obj_t* m_usernameTextArea = nullptr;
    lv_obj_t* m_passwordTextArea = nullptr;
    lv_obj_t* m_apiKeyTextArea = nullptr;
    lv_obj_t* m_categoryTextArea = nullptr;
    lv_obj_t* m_timeoutSpinbox = nullptr;
    
    // UI elements - Server management
    lv_obj_t* m_serverList = nullptr;
    lv_obj_t* m_statusLabel = nullptr;
    
    // UI elements - Buttons
    lv_obj_t* m_buttonContainer = nullptr;
    lv_obj_t* m_addButton = nullptr;
    lv_obj_t* m_updateButton = nullptr;
    lv_obj_t* m_testButton = nullptr;
    lv_obj_t* m_cancelButton = nullptr;
    lv_obj_t* m_removeButton = nullptr;
    lv_obj_t* m_enableButton = nullptr;
    lv_obj_t* m_setDefaultButton = nullptr;
    lv_obj_t* m_importButton = nullptr;
    lv_obj_t* m_exportButton = nullptr;
    lv_obj_t* m_refreshButton = nullptr;
    
    // Dialog styling
    static constexpr lv_color_t COLOR_DIALOG_BG = LV_COLOR_MAKE(0x2C, 0x2C, 0x2C);
    static constexpr lv_color_t COLOR_DIALOG_BORDER = LV_COLOR_MAKE(0x34, 0x98, 0xDB);
    static constexpr lv_color_t COLOR_SUCCESS = LV_COLOR_MAKE(0x2E, 0xCC, 0x71);
    static constexpr lv_color_t COLOR_ERROR = LV_COLOR_MAKE(0xE7, 0x4C, 0x3C);
    static constexpr lv_color_t COLOR_WARNING = LV_COLOR_MAKE(0xF3, 0x9C, 0x12);
};

#endif // APP_STORE_SERVER_MANAGER_H