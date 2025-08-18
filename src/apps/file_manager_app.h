#ifndef FILE_MANAGER_APP_H
#define FILE_MANAGER_APP_H

#include "base_app.h"
#include "../services/storage_service.h"
#include <vector>
#include <string>
#include <ctime>
#include <memory>

/**
 * @file file_manager_app.h
 * @brief File Manager Application for M5Stack Tab5
 * 
 * Provides comprehensive file system management including browsing,
 * copying, moving, deleting, and viewing files and directories.
 */

// Add the missing FileOperation enum
enum class FileOperation {
    COPY,
    MOVE,
    DELETE,
    RENAME,
    CREATE_FOLDER
};

enum class FileType {
    UNKNOWN,
    DIRECTORY,
    TEXT,
    IMAGE,
    AUDIO,
    VIDEO,
    DOCUMENT,
    ARCHIVE,
    EXECUTABLE,
    CONFIG
};

enum class SortMode {
    NAME_ASC,
    NAME_DESC,
    SIZE_ASC,
    SIZE_DESC,
    DATE_ASC,
    DATE_DESC,
    TYPE_ASC,
    TYPE_DESC
};

enum class ViewMode {
    LIST,
    GRID,
    DETAILS
};

// Use FileInfo from storage_service.h instead of FileSystemItem
// struct FileInfo is already defined in storage_service.h

struct ClipboardItem {
    std::string sourcePath;
    std::string name;
    bool isCutOperation;  // true for cut, false for copy
    time_t clipboardTime;
};

class FileManagerApp : public BaseApp {
public:
    /**
     * @brief Constructor
     */
    FileManagerApp();
    
    /**
     * @brief Destructor
     */
    ~FileManagerApp() override;

    // BaseApp interface implementation
    os_error_t initialize() override;
    os_error_t update(uint32_t deltaTime) override;
    os_error_t shutdown() override;
    os_error_t createUI(lv_obj_t* parent) override;
    os_error_t destroyUI() override;
    os_error_t handleEvent(uint32_t eventType, void* eventData, size_t dataSize) override;

    /**
     * @brief Navigate to directory
     * @param path Directory path to navigate to
     * @return OS_OK on success, error code on failure
     */
    os_error_t navigateToDirectory(const std::string& path);

    /**
     * @brief Navigate up one directory level
     * @return OS_OK on success, error code on failure
     */
    os_error_t navigateUp();

    /**
     * @brief Refresh current directory
     * @return OS_OK on success, error code on failure
     */
    os_error_t refreshDirectory();

    /**
     * @brief Copy file or directory
     * @param sourcePath Source path
     * @param destinationPath Destination path
     * @return OS_OK on success, error code on failure
     */
    os_error_t copyItem(const std::string& sourcePath, const std::string& destinationPath);

    /**
     * @brief Move file or directory
     * @param sourcePath Source path
     * @param destinationPath Destination path
     * @return OS_OK on success, error code on failure
     */
    os_error_t moveItem(const std::string& sourcePath, const std::string& destinationPath);

    /**
     * @brief Delete file or directory
     * @param path Path to delete
     * @return OS_OK on success, error code on failure
     */
    os_error_t deleteItem(const std::string& path);

    /**
     * @brief Create new directory
     * @param path Directory path to create
     * @return OS_OK on success, error code on failure
     */
    os_error_t createDirectory(const std::string& path);

    /**
     * @brief Rename file or directory
     * @param oldPath Current path
     * @param newName New name
     * @return OS_OK on success, error code on failure
     */
    os_error_t renameItem(const std::string& oldPath, const std::string& newName);

    /**
     * @brief Copy item to clipboard
     * @param path Path to copy
     * @param isCut true for cut operation, false for copy
     * @return OS_OK on success, error code on failure
     */
    os_error_t copyToClipboard(const std::string& path, bool isCut = false);

    /**
     * @brief Paste item from clipboard
     * @param destinationPath Destination directory
     * @return OS_OK on success, error code on failure
     */
    os_error_t pasteFromClipboard(const std::string& destinationPath);

    /**
     * @brief Get current directory path
     * @return Current directory path
     */
    const std::string& getCurrentPath() const { return m_currentPath; }

    /**
     * @brief Get list of items in current directory
     * @return Vector of file system items
     */
    const std::vector<FileInfo>& getCurrentItems() const { return m_currentFiles; }

    /**
     * @brief Set view mode
     * @param mode View mode to set
     */
    void setViewMode(ViewMode mode);

    /**
     * @brief Set sort mode
     * @param mode Sort mode to set
     */
    void setSortMode(SortMode mode);

    /**
     * @brief Set show hidden files
     * @param show Whether to show hidden files
     */
    void setShowHiddenFiles(bool show);

    /**
     * @brief Get file type from filename
     * @param filename File name
     * @return Detected file type
     */
    static FileType getFileType(const std::string& filename);

    /**
     * @brief Get file type icon
     * @param fileInfo File information
     * @return LVGL symbol for file type
     */
    static const char* getFileTypeIcon(const FileInfo& fileInfo);

    /**
     * @brief Format file size
     * @param size File size in bytes
     * @param buffer Output buffer
     * @param bufferSize Size of output buffer
     */
    static void formatFileSize(uint64_t size, char* buffer, size_t bufferSize);

    /**
     * @brief Format timestamp
     * @param timestamp Timestamp to format
     * @param buffer Output buffer
     * @param bufferSize Size of output buffer
     */
    static void formatTimestamp(time_t timestamp, char* buffer, size_t bufferSize);

    /**
     * @brief Execute file operation
     * @param operation File operation type
     * @param targetPath Target path for operation (optional)
     * @return OS_OK on success, error code on failure
     */
    os_error_t executeFileOperation(FileOperation operation, const std::string& targetPath = "");

    /**
     * @brief Select file by index
     * @param index File index to select
     */
    void selectFile(size_t index);

    /**
     * @brief Switch storage type
     * @param type Storage type to switch to
     * @return OS_OK on success, error code on failure
     */
    os_error_t switchStorage(StorageType type);

private:
    /**
     * @brief Create file browser UI
     */
    void createFileBrowserUI();

    /**
     * @brief Create toolbar UI
     */
    void createToolbarUI();

    /**
     * @brief Create status bar UI
     */
    void createStatusBarUI();

    /**
     * @brief Update file list display
     */
    void updateFileList();

    /**
     * @brief Update breadcrumb display
     */
    void updateBreadcrumb();

    /**
     * @brief Update storage information
     */
    void updateStorageInfo();

    /**
     * @brief Update storage indicator
     */
    void updateStorageIndicator();


    // UI event callbacks
    static void fileListCallback(lv_event_t* e);
    static void upButtonCallback(lv_event_t* e);
    static void refreshButtonCallback(lv_event_t* e);
    static void homeButtonCallback(lv_event_t* e);
    static void copyButtonCallback(lv_event_t* e);
    static void deleteButtonCallback(lv_event_t* e);
    static void storageButtonCallback(lv_event_t* e);
    static void propertiesButtonCallback(lv_event_t* e);
    static void newFolderButtonCallback(lv_event_t* e);
    static void moveButtonCallback(lv_event_t* e);

    // Storage service
    StorageService* m_storageService = nullptr;
    bool m_ownStorageService = false;
    StorageType m_currentStorage = StorageType::NONE;

    // File system navigation
    std::string m_currentPath;
    std::vector<FileInfo> m_currentFiles;

    // Selection and operations
    int m_selectedIndex = -1;
    FileOperation m_pendingOperation;
    std::string m_operationSourcePath;

    // Statistics
    uint32_t m_filesAccessed = 0;
    uint32_t m_operationsPerformed = 0;

    // UI elements - Main containers
    lv_obj_t* m_mainContainer = nullptr;
    lv_obj_t* m_toolbarContainer = nullptr;
    lv_obj_t* m_fileListContainer = nullptr;
    lv_obj_t* m_statusContainer = nullptr;

    // UI elements - Toolbar buttons
    lv_obj_t* m_upButton = nullptr;
    lv_obj_t* m_homeButton = nullptr;
    lv_obj_t* m_refreshButton = nullptr;
    lv_obj_t* m_copyButton = nullptr;
    lv_obj_t* m_moveButton = nullptr;
    lv_obj_t* m_deleteButton = nullptr;
    lv_obj_t* m_propertiesButton = nullptr;
    lv_obj_t* m_storageButton = nullptr;
    lv_obj_t* m_newFolderButton = nullptr;

    // UI elements - File browser
    lv_obj_t* m_breadcrumbLabel = nullptr;
    lv_obj_t* m_fileList = nullptr;

    // UI elements - Status bar
    lv_obj_t* m_storageLabel = nullptr;
    lv_obj_t* m_selectionLabel = nullptr;

    // UI elements - Dialogs
    lv_obj_t* m_dialogContainer = nullptr;
    lv_obj_t* m_confirmDialog = nullptr;

};

/**
 * @brief Factory function for creating app instances
 * Required for dynamic loading by the app manager
 * @return Unique pointer to app instance
 */
extern "C" std::unique_ptr<BaseApp> createFileManagerApp();

#endif // FILE_MANAGER_APP_H