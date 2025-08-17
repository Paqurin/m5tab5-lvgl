#ifndef STORAGE_SERVICE_H
#define STORAGE_SERVICE_H

#include "../system/os_config.h"
#include "../hal/hardware_config.h"
#include <esp_vfs_fat.h>
#include <driver/sdspi_host.h>
#include <driver/spi_common.h>
#include <sdmmc_cmd.h>
#include <string>
#include <vector>

/**
 * @file storage_service.h
 * @brief Storage Service for M5Stack Tab5
 * 
 * Provides unified access to SD card and USB storage devices
 * with automatic mounting, file system management, and device detection.
 */

enum class StorageType {
    NONE,
    SD_CARD,
    USB_STORAGE
};

enum class StorageStatus {
    NOT_INITIALIZED,
    MOUNTED,
    UNMOUNTED,
    ERROR,
    NOT_PRESENT
};

struct StorageDevice {
    StorageType type = StorageType::NONE;
    StorageStatus status = StorageStatus::NOT_INITIALIZED;
    std::string mountPoint;
    std::string label;
    uint64_t totalSize = 0;
    uint64_t freeSize = 0;
    uint64_t usedSize = 0;
    std::string fileSystem;
    bool readOnly = false;
};

struct FileInfo {
    std::string name;
    std::string path;
    uint64_t size = 0;
    time_t modifiedTime = 0;
    bool isDirectory = false;
    bool isHidden = false;
};

class StorageService {
public:
    StorageService() = default;
    ~StorageService();

    /**
     * @brief Initialize storage service
     * @return OS_OK on success, error code on failure
     */
    os_error_t initialize();

    /**
     * @brief Shutdown storage service
     * @return OS_OK on success, error code on failure
     */
    os_error_t shutdown();

    /**
     * @brief Update storage service (check for new devices)
     * @param deltaTime Time since last update in milliseconds
     * @return OS_OK on success, error code on failure
     */
    os_error_t update(uint32_t deltaTime);

    /**
     * @brief Mount SD card
     * @return OS_OK on success, error code on failure
     */
    os_error_t mountSDCard();

    /**
     * @brief Unmount SD card
     * @return OS_OK on success, error code on failure
     */
    os_error_t unmountSDCard();

    /**
     * @brief Mount USB storage device
     * @return OS_OK on success, error code on failure
     */
    os_error_t mountUSBStorage();

    /**
     * @brief Unmount USB storage device
     * @return OS_OK on success, error code on failure
     */
    os_error_t unmountUSBStorage();

    /**
     * @brief Get SD card information
     * @return Storage device information
     */
    const StorageDevice& getSDCardInfo() const { return m_sdCard; }

    /**
     * @brief Get USB storage information
     * @return Storage device information
     */
    const StorageDevice& getUSBStorageInfo() const { return m_usbStorage; }

    /**
     * @brief Check if SD card is available
     * @return true if SD card is mounted and available
     */
    bool isSDCardAvailable() const { return m_sdCard.status == StorageStatus::MOUNTED; }

    /**
     * @brief Check if USB storage is available
     * @return true if USB storage is mounted and available
     */
    bool isUSBStorageAvailable() const { return m_usbStorage.status == StorageStatus::MOUNTED; }

    /**
     * @brief List files in directory
     * @param path Directory path
     * @param files Output vector of file information
     * @return OS_OK on success, error code on failure
     */
    os_error_t listDirectory(const std::string& path, std::vector<FileInfo>& files);

    /**
     * @brief Create directory
     * @param path Directory path to create
     * @return OS_OK on success, error code on failure
     */
    os_error_t createDirectory(const std::string& path);

    /**
     * @brief Delete file or directory
     * @param path Path to delete
     * @return OS_OK on success, error code on failure
     */
    os_error_t deleteFile(const std::string& path);

    /**
     * @brief Copy file
     * @param sourcePath Source file path
     * @param destPath Destination file path
     * @return OS_OK on success, error code on failure
     */
    os_error_t copyFile(const std::string& sourcePath, const std::string& destPath);

    /**
     * @brief Move/rename file
     * @param sourcePath Source file path
     * @param destPath Destination file path
     * @return OS_OK on success, error code on failure
     */
    os_error_t moveFile(const std::string& sourcePath, const std::string& destPath);

    /**
     * @brief Get file information
     * @param path File path
     * @param info Output file information
     * @return OS_OK on success, error code on failure
     */
    os_error_t getFileInfo(const std::string& path, FileInfo& info);

    /**
     * @brief Check if file exists
     * @param path File path
     * @return true if file exists
     */
    bool fileExists(const std::string& path);

    /**
     * @brief Get storage statistics
     */
    void printStorageStats() const;

    /**
     * @brief Format storage device
     * @param type Storage type to format
     * @param fileSystem File system type (FAT32, exFAT)
     * @return OS_OK on success, error code on failure
     */
    os_error_t formatStorage(StorageType type, const std::string& fileSystem = "FAT32");

    /**
     * @brief Eject storage device safely
     * @param type Storage type to eject
     * @return OS_OK on success, error code on failure
     */
    os_error_t ejectStorage(StorageType type);

private:
    /**
     * @brief Initialize SD card hardware
     * @return OS_OK on success, error code on failure
     */
    os_error_t initializeSDCard();

    /**
     * @brief Initialize USB host
     * @return OS_OK on success, error code on failure
     */
    os_error_t initializeUSBHost();

    /**
     * @brief Update storage device information
     * @param device Storage device to update
     * @return OS_OK on success, error code on failure
     */
    os_error_t updateStorageInfo(StorageDevice& device);

    /**
     * @brief Check SD card presence
     * @return true if SD card is present
     */
    bool checkSDCardPresence();

    /**
     * @brief Check USB storage presence
     * @return true if USB storage is present
     */
    bool checkUSBStoragePresence();

    /**
     * @brief USB device connection callback
     * @param client_handle USB client handle
     * @param event USB host event
     * @param arg Callback argument
     */
    #ifdef CONFIG_USB_HOST_ENABLED
    static void usbHostCallback(void* client_handle, usb_host_client_event_msg_t* event, void* arg);
    #endif

    // Storage devices
    StorageDevice m_sdCard;
    StorageDevice m_usbStorage;

    // Hardware handles
    sdmmc_card_t* m_sdCardHandle = nullptr;
    wl_handle_t m_sdWlHandle = WL_INVALID_HANDLE;
    
    // USB host
    void* m_usbHostHandle = nullptr;
    bool m_usbHostInitialized = false;

    // Service state
    bool m_initialized = false;
    uint32_t m_lastDeviceCheck = 0;

    // Configuration
    static constexpr uint32_t DEVICE_CHECK_INTERVAL = 5000; // 5 seconds
    static constexpr size_t FILE_COPY_BUFFER_SIZE = 8192;
    static constexpr const char* SD_MOUNT_POINT = "/sdcard";
    static constexpr const char* USB_MOUNT_POINT = "/usb";
};

#endif // STORAGE_SERVICE_H