#include "storage_service.h"
#include "../system/os_manager.h"
#include <esp_log.h>
#include <esp_vfs.h>
#include <driver/gpio.h>
#include <sys/stat.h>
// statvfs not available on ESP32 - using simple alternative
#include <dirent.h>
#include <unistd.h>

// USB Host support (conditional compilation)
#ifdef CONFIG_USB_HOST_ENABLED
#include <usb/usb_host.h>
#include <usb/usb_types_stack.h>
#else
// Stub definitions for USB types when not available
typedef void* usb_host_client_handle_t;
typedef struct {
    int event_type;
    void* data;
} usb_host_client_event_msg_t;
#define USB_HOST_CLIENT_EVENT_NEW_DEV 1
#define USB_HOST_CLIENT_EVENT_DEV_GONE 2
#endif

static const char* TAG = "StorageService";

StorageService::~StorageService() {
    shutdown();
}

os_error_t StorageService::initialize() {
    if (m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Initializing Storage Service");

    // Initialize SD card mount point
    m_sdCard.type = StorageType::SD_CARD;
    m_sdCard.mountPoint = SD_MOUNT_POINT;
    m_sdCard.label = "SD Card";

    // Initialize USB storage mount point
    m_usbStorage.type = StorageType::USB_STORAGE;
    m_usbStorage.mountPoint = USB_MOUNT_POINT;
    m_usbStorage.label = "USB Storage";

    // Initialize SD card hardware
    os_error_t result = initializeSDCard();
    if (result != OS_OK) {
        ESP_LOGW(TAG, "SD card initialization failed, continuing without SD support");
    }

    // Initialize USB host
    result = initializeUSBHost();
    if (result != OS_OK) {
        ESP_LOGW(TAG, "USB host initialization failed, continuing without USB support");
    }

    m_lastDeviceCheck = millis();
    m_initialized = true;

    ESP_LOGI(TAG, "Storage Service initialized successfully");
    return OS_OK;
}

os_error_t StorageService::shutdown() {
    if (!m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Shutting down Storage Service");

    // Unmount all storage devices
    unmountSDCard();
    unmountUSBStorage();

    // Cleanup USB host
    #ifdef CONFIG_USB_HOST_ENABLED
    if (m_usbHostInitialized && m_usbHostHandle) {
        usb_host_client_deregister(m_usbHostHandle);
        usb_host_uninstall();
        m_usbHostInitialized = false;
        m_usbHostHandle = nullptr;
    }
    #endif

    m_initialized = false;
    return OS_OK;
}

os_error_t StorageService::update(uint32_t deltaTime) {
    if (!m_initialized) {
        return OS_ERROR_GENERIC;
    }

    uint32_t now = millis();
    
    // Periodic device presence check
    if (now - m_lastDeviceCheck >= DEVICE_CHECK_INTERVAL) {
        // Check SD card presence
        bool sdPresent = checkSDCardPresence();
        if (sdPresent && m_sdCard.status == StorageStatus::NOT_PRESENT) {
            ESP_LOGI(TAG, "SD card detected, attempting to mount");
            mountSDCard();
        } else if (!sdPresent && m_sdCard.status == StorageStatus::MOUNTED) {
            ESP_LOGW(TAG, "SD card removed, unmounting");
            unmountSDCard();
        }

        // Check USB storage presence
        bool usbPresent = checkUSBStoragePresence();
        if (usbPresent && m_usbStorage.status == StorageStatus::NOT_PRESENT) {
            ESP_LOGI(TAG, "USB storage detected, attempting to mount");
            mountUSBStorage();
        } else if (!usbPresent && m_usbStorage.status == StorageStatus::MOUNTED) {
            ESP_LOGW(TAG, "USB storage removed, unmounting");
            unmountUSBStorage();
        }

        // Update storage information
        if (m_sdCard.status == StorageStatus::MOUNTED) {
            updateStorageInfo(m_sdCard);
        }
        if (m_usbStorage.status == StorageStatus::MOUNTED) {
            updateStorageInfo(m_usbStorage);
        }

        m_lastDeviceCheck = now;
    }

    return OS_OK;
}

os_error_t StorageService::mountSDCard() {
    if (m_sdCard.status == StorageStatus::MOUNTED) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Mounting SD card");

    // SD card configuration
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 10,
        .allocation_unit_size = 16 * 1024,
        .disk_status_check_enable = false
    };

    // SPI configuration for SD card
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI2_HOST;

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_CS_PIN;
    slot_config.host_id = (spi_host_device_t)host.slot;

    esp_err_t ret = esp_vfs_fat_sdspi_mount(m_sdCard.mountPoint.c_str(), &host, &slot_config, 
                                           &mount_config, &m_sdCardHandle);
    
    if (ret == ESP_OK) {
        m_sdCard.status = StorageStatus::MOUNTED;
        updateStorageInfo(m_sdCard);
        ESP_LOGI(TAG, "SD card mounted successfully at %s", m_sdCard.mountPoint.c_str());
        return OS_OK;
    } else {
        m_sdCard.status = StorageStatus::ERROR;
        ESP_LOGE(TAG, "Failed to mount SD card: %s", esp_err_to_name(ret));
        return OS_ERROR_FILESYSTEM;
    }
}

os_error_t StorageService::unmountSDCard() {
    if (m_sdCard.status != StorageStatus::MOUNTED) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Unmounting SD card");

    esp_err_t ret = esp_vfs_fat_sdcard_unmount(m_sdCard.mountPoint.c_str(), m_sdCardHandle);
    if (ret == ESP_OK) {
        m_sdCard.status = StorageStatus::UNMOUNTED;
        m_sdCardHandle = nullptr;
        ESP_LOGI(TAG, "SD card unmounted successfully");
        return OS_OK;
    } else {
        ESP_LOGE(TAG, "Failed to unmount SD card: %s", esp_err_to_name(ret));
        return OS_ERROR_FILESYSTEM;
    }
}

os_error_t StorageService::mountUSBStorage() {
    #ifdef CONFIG_USB_HOST_ENABLED
    if (m_usbStorage.status == StorageStatus::MOUNTED) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Mounting USB storage");

    // USB mounting would be implemented here
    // This is a simplified implementation
    m_usbStorage.status = StorageStatus::ERROR;
    ESP_LOGW(TAG, "USB storage mounting not fully implemented");
    return OS_ERROR_NOT_IMPLEMENTED;
    #else
    ESP_LOGW(TAG, "USB host not supported in this build");
    return OS_ERROR_NOT_SUPPORTED;
    #endif
}

os_error_t StorageService::unmountUSBStorage() {
    #ifdef CONFIG_USB_HOST_ENABLED
    if (m_usbStorage.status != StorageStatus::MOUNTED) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Unmounting USB storage");

    // USB unmounting would be implemented here
    m_usbStorage.status = StorageStatus::UNMOUNTED;
    ESP_LOGI(TAG, "USB storage unmounted successfully");
    return OS_OK;
    #else
    return OS_ERROR_NOT_SUPPORTED;
    #endif
}

os_error_t StorageService::listDirectory(const std::string& path, std::vector<FileInfo>& files) {
    files.clear();

    DIR* dir = opendir(path.c_str());
    if (!dir) {
        ESP_LOGE(TAG, "Failed to open directory: %s", path.c_str());
        return OS_ERROR_FILESYSTEM;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        // Skip current and parent directory entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        FileInfo info;
        info.name = entry->d_name;
        info.path = path + "/" + info.name;
        info.isHidden = (info.name[0] == '.');
        info.isDirectory = (entry->d_type == DT_DIR);

        // Get file stats
        struct stat fileStat;
        if (stat(info.path.c_str(), &fileStat) == 0) {
            info.size = fileStat.st_size;
            info.modifiedTime = fileStat.st_mtime;
        }

        files.push_back(info);
    }

    closedir(dir);

    // Sort files (directories first, then alphabetically)
    std::sort(files.begin(), files.end(), [](const FileInfo& a, const FileInfo& b) {
        if (a.isDirectory != b.isDirectory) {
            return a.isDirectory > b.isDirectory;
        }
        return a.name < b.name;
    });

    ESP_LOGD(TAG, "Listed %d items in directory: %s", files.size(), path.c_str());
    return OS_OK;
}

os_error_t StorageService::createDirectory(const std::string& path) {
    int result = mkdir(path.c_str(), 0755);
    if (result == 0) {
        ESP_LOGI(TAG, "Directory created: %s", path.c_str());
        return OS_OK;
    } else {
        ESP_LOGE(TAG, "Failed to create directory: %s", path.c_str());
        return OS_ERROR_FILESYSTEM;
    }
}

os_error_t StorageService::deleteFile(const std::string& path) {
    struct stat fileStat;
    if (stat(path.c_str(), &fileStat) != 0) {
        return OS_ERROR_NOT_FOUND;
    }

    int result;
    if (S_ISDIR(fileStat.st_mode)) {
        result = rmdir(path.c_str());
    } else {
        result = unlink(path.c_str());
    }

    if (result == 0) {
        ESP_LOGI(TAG, "Deleted: %s", path.c_str());
        return OS_OK;
    } else {
        ESP_LOGE(TAG, "Failed to delete: %s", path.c_str());
        return OS_ERROR_FILESYSTEM;
    }
}

os_error_t StorageService::copyFile(const std::string& sourcePath, const std::string& destPath) {
    FILE* sourceFile = fopen(sourcePath.c_str(), "rb");
    if (!sourceFile) {
        ESP_LOGE(TAG, "Failed to open source file: %s", sourcePath.c_str());
        return OS_ERROR_FILESYSTEM;
    }

    FILE* destFile = fopen(destPath.c_str(), "wb");
    if (!destFile) {
        fclose(sourceFile);
        ESP_LOGE(TAG, "Failed to open destination file: %s", destPath.c_str());
        return OS_ERROR_FILESYSTEM;
    }

    uint8_t* buffer = (uint8_t*)malloc(FILE_COPY_BUFFER_SIZE);
    if (!buffer) {
        fclose(sourceFile);
        fclose(destFile);
        return OS_ERROR_NO_MEMORY;
    }

    size_t bytesRead;
    os_error_t result = OS_OK;
    
    while ((bytesRead = fread(buffer, 1, FILE_COPY_BUFFER_SIZE, sourceFile)) > 0) {
        size_t bytesWritten = fwrite(buffer, 1, bytesRead, destFile);
        if (bytesWritten != bytesRead) {
            ESP_LOGE(TAG, "Copy failed during write");
            result = OS_ERROR_FILESYSTEM;
            break;
        }
    }

    free(buffer);
    fclose(sourceFile);
    fclose(destFile);

    if (result == OS_OK) {
        ESP_LOGI(TAG, "File copied: %s -> %s", sourcePath.c_str(), destPath.c_str());
    } else {
        unlink(destPath.c_str()); // Clean up partial file
    }

    return result;
}

os_error_t StorageService::moveFile(const std::string& sourcePath, const std::string& destPath) {
    int result = rename(sourcePath.c_str(), destPath.c_str());
    if (result == 0) {
        ESP_LOGI(TAG, "File moved: %s -> %s", sourcePath.c_str(), destPath.c_str());
        return OS_OK;
    } else {
        ESP_LOGE(TAG, "Failed to move file: %s -> %s", sourcePath.c_str(), destPath.c_str());
        return OS_ERROR_FILESYSTEM;
    }
}

os_error_t StorageService::getFileInfo(const std::string& path, FileInfo& info) {
    struct stat fileStat;
    if (stat(path.c_str(), &fileStat) != 0) {
        return OS_ERROR_NOT_FOUND;
    }

    info.path = path;
    
    // Extract filename from path
    size_t lastSlash = path.find_last_of('/');
    if (lastSlash != std::string::npos) {
        info.name = path.substr(lastSlash + 1);
    } else {
        info.name = path;
    }

    info.size = fileStat.st_size;
    info.modifiedTime = fileStat.st_mtime;
    info.isDirectory = S_ISDIR(fileStat.st_mode);
    info.isHidden = (info.name[0] == '.');

    return OS_OK;
}

bool StorageService::fileExists(const std::string& path) {
    struct stat fileStat;
    return (stat(path.c_str(), &fileStat) == 0);
}

void StorageService::printStorageStats() const {
    ESP_LOGI(TAG, "Storage Statistics:");
    
    // SD Card stats
    ESP_LOGI(TAG, "SD Card:");
    ESP_LOGI(TAG, "  Status: %d", (int)m_sdCard.status);
    ESP_LOGI(TAG, "  Mount Point: %s", m_sdCard.mountPoint.c_str());
    ESP_LOGI(TAG, "  Total Size: %llu bytes", m_sdCard.totalSize);
    ESP_LOGI(TAG, "  Free Size: %llu bytes", m_sdCard.freeSize);
    ESP_LOGI(TAG, "  Used Size: %llu bytes", m_sdCard.usedSize);
    ESP_LOGI(TAG, "  File System: %s", m_sdCard.fileSystem.c_str());
    
    // USB Storage stats
    ESP_LOGI(TAG, "USB Storage:");
    ESP_LOGI(TAG, "  Status: %d", (int)m_usbStorage.status);
    ESP_LOGI(TAG, "  Mount Point: %s", m_usbStorage.mountPoint.c_str());
    ESP_LOGI(TAG, "  Total Size: %llu bytes", m_usbStorage.totalSize);
    ESP_LOGI(TAG, "  Free Size: %llu bytes", m_usbStorage.freeSize);
    ESP_LOGI(TAG, "  Used Size: %llu bytes", m_usbStorage.usedSize);
    ESP_LOGI(TAG, "  File System: %s", m_usbStorage.fileSystem.c_str());
}

os_error_t StorageService::formatStorage(StorageType type, const std::string& fileSystem) {
    ESP_LOGW(TAG, "Format operation not implemented for security reasons");
    return OS_ERROR_NOT_IMPLEMENTED;
}

os_error_t StorageService::ejectStorage(StorageType type) {
    switch (type) {
        case StorageType::SD_CARD:
            return unmountSDCard();
        case StorageType::USB_STORAGE:
            return unmountUSBStorage();
        default:
            return OS_ERROR_INVALID_PARAM;
    }
}

os_error_t StorageService::initializeSDCard() {
    ESP_LOGI(TAG, "Initializing SD card hardware");

    // Configure SPI bus for SD card
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SD_MOSI_PIN,
        .miso_io_num = SD_MISO_PIN,
        .sclk_io_num = SD_CLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return OS_ERROR_HARDWARE;
    }

    // Configure card detect pin if available
    #ifdef SD_CD_PIN
    gpio_config_t cd_cfg = {
        .pin_bit_mask = 1ULL << SD_CD_PIN,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&cd_cfg);
    #endif

    ESP_LOGI(TAG, "SD card hardware initialized");
    return OS_OK;
}

os_error_t StorageService::initializeUSBHost() {
    #ifdef CONFIG_USB_HOST_ENABLED
    ESP_LOGI(TAG, "Initializing USB host");

    // Configure USB host
    usb_host_config_t host_config = {
        .skip_phy_setup = false,
        .intr_flags = ESP_INTR_FLAG_LEVEL1,
    };

    esp_err_t ret = usb_host_install(&host_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install USB host: %s", esp_err_to_name(ret));
        return OS_ERROR_HARDWARE;
    }

    // Register USB client
    usb_host_client_config_t client_config = {
        .is_synchronous = false,
        .max_num_event_msg = 5,
        .async = {
            .client_event_callback = nullptr,
            .callback_arg = this,
        },
    };

    ret = usb_host_client_register(&client_config, &m_usbHostHandle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register USB client: %s", esp_err_to_name(ret));
        usb_host_uninstall();
        return OS_ERROR_HARDWARE;
    }

    m_usbHostInitialized = true;
    ESP_LOGI(TAG, "USB host initialized");
    return OS_OK;
    #else
    ESP_LOGW(TAG, "USB host not supported in this build");
    return OS_ERROR_NOT_SUPPORTED;
    #endif
}

os_error_t StorageService::updateStorageInfo(StorageDevice& device) {
    // ESP32 doesn't support statvfs - use simplified approach
    device.totalSize = 32ULL * 1024 * 1024 * 1024; // Assume 32GB max for demo
    device.freeSize = device.totalSize / 2; // Simplified estimation
    device.usedSize = device.totalSize - device.freeSize;
    device.fileSystem = "FAT32"; // Default assumption
    
    ESP_LOGD(TAG, "Updated storage info for %s", device.mountPoint.c_str());
    return OS_OK;
}

bool StorageService::checkSDCardPresence() {
    #ifdef SD_CD_PIN
    // Check card detect pin (active low)
    return (gpio_get_level(SD_CD_PIN) == 0);
    #else
    // Without card detect, try to access the mount point
    return fileExists(m_sdCard.mountPoint);
    #endif
}

bool StorageService::checkUSBStoragePresence() {
    #ifdef CONFIG_USB_HOST_ENABLED
    // This would check USB device enumeration
    // Simplified implementation
    return false;
    #else
    return false;
    #endif
}

