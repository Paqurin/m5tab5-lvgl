#ifndef CAMERA_APP_H
#define CAMERA_APP_H

#include "base_app.h"
#include "../hal/hardware_config.h"
#include <vector>
#include <string>
#include <memory>

// ESP camera library includes
#ifdef ESP_CAMERA_SUPPORTED
#include "esp_camera.h"
#include "sensor.h"
#else
// Camera types for when ESP camera library is not available
typedef struct {
    uint8_t* buf;
    size_t len;
    size_t width;
    size_t height;
    uint32_t timestamp;
} camera_fb_t;

typedef struct {
    int ledc_channel;
    int ledc_timer;
    int pin_d0;
    int pin_d1;
    int pin_d2;
    int pin_d3;
    int pin_d4;
    int pin_d5;
    int pin_d6;
    int pin_d7;
    int pin_xclk;
    int pin_pclk;
    int pin_vsync;
    int pin_href;
    int pin_sccb_sda;
    int pin_sccb_scl;
    int pin_pwdn;
    int pin_reset;
    int xclk_freq_hz;
    int pixel_format;
    int frame_size;
    int jpeg_quality;
    int fb_count;
    int grab_mode;
} camera_config_t;

typedef struct {
    // Sensor function pointers
    int (*set_brightness)(void* sensor, int level);
    int (*set_contrast)(void* sensor, int level);
    int (*set_saturation)(void* sensor, int level);
    int (*set_special_effect)(void* sensor, int effect);
    int (*set_whitebal)(void* sensor, int enable);
    int (*set_awb_gain)(void* sensor, int enable);
    int (*set_wb_mode)(void* sensor, int mode);
    int (*set_exposure_ctrl)(void* sensor, int enable);
    int (*set_aec2)(void* sensor, int enable);
    int (*set_ae_level)(void* sensor, int level);
    int (*set_aec_value)(void* sensor, int value);
    int (*set_gain_ctrl)(void* sensor, int enable);
    int (*set_agc_gain)(void* sensor, int gain);
    int (*set_gainceiling)(void* sensor, int ceiling);
    int (*set_bpc)(void* sensor, int enable);
    int (*set_wpc)(void* sensor, int enable);
    int (*set_raw_gma)(void* sensor, int enable);
    int (*set_lenc)(void* sensor, int enable);
    int (*set_hmirror)(void* sensor, int enable);
    int (*set_vflip)(void* sensor, int enable);
    int (*set_framesize)(void* sensor, int framesize);
} sensor_t;

// ESP camera enums
typedef enum {
    PIXFORMAT_JPEG = 0,
    PIXFORMAT_RGB565,
    PIXFORMAT_YUV422
} pixformat_t;

typedef enum {
    FRAMESIZE_QVGA = 0,  // 320x240
    FRAMESIZE_VGA,       // 640x480  
    FRAMESIZE_HD,        // 1280x720
    FRAMESIZE_UXGA       // 1600x1200
} framesize_t;

typedef enum {
    CAMERA_GRAB_WHEN_EMPTY = 0,
    CAMERA_GRAB_LATEST
} camera_grab_mode_t;

typedef enum {
    GAINCEILING_2X = 0,
    GAINCEILING_4X,
    GAINCEILING_8X,
    GAINCEILING_16X,
    GAINCEILING_32X,
    GAINCEILING_64X,
    GAINCEILING_128X
} gainceiling_t;

// Function declarations for camera stubs
int esp_camera_init(camera_config_t* config);
void esp_camera_deinit();
camera_fb_t* esp_camera_fb_get();
void esp_camera_fb_return(camera_fb_t* fb);
sensor_t* esp_camera_sensor_get();
#endif

/**
 * @file camera_app.h
 * @brief Camera Application for M5Stack Tab5
 * 
 * Provides camera preview, photo capture, and video recording functionality.
 * Supports various camera configurations and image processing features.
 */

enum class CameraMode {
    PREVIEW,
    PHOTO,
    VIDEO,
    QR_CODE,
    BARCODE
};

enum class CameraResolution {
    QVGA_320x240,
    VGA_640x480,
    HD_1280x720,
    FHD_1920x1080,
    AUTO
};

// Additional enum for capture resolution used in implementation
enum class CaptureResolution {
    QVGA_320x240 = 0,
    VGA_640x480,
    HD_1280x720,
    FULL_1600x1200
};

enum class CameraEffect {
    NONE,
    NEGATIVE,
    GRAYSCALE,
    RED_TINT,
    GREEN_TINT,
    BLUE_TINT,
    SEPIA
};

struct CameraConfig {
    CameraResolution resolution = CameraResolution::HD_1280x720;
    uint8_t quality = 85;           // JPEG quality 1-100
    int8_t brightness = 0;          // -2 to 2
    int8_t contrast = 0;            // -2 to 2
    int8_t saturation = 0;          // -2 to 2
    CameraEffect effect = CameraEffect::NONE;
    bool autoExposure = true;
    bool autoWhiteBalance = true;
    bool flipHorizontal = false;
    bool flipVertical = false;
    uint16_t exposureTime = 0;      // 0 = auto
    uint16_t gain = 0;              // 0 = auto
};

struct MediaFile {
    std::string filename;
    std::string fullPath;
    size_t fileSize;
    uint32_t timestamp;
    CameraMode captureMode;
    bool isVideo;
};

class CameraApp : public BaseApp {
public:
    /**
     * @brief Constructor
     */
    CameraApp();
    
    /**
     * @brief Destructor
     */
    ~CameraApp() override;

    // BaseApp interface implementation
    os_error_t initialize() override;
    os_error_t update(uint32_t deltaTime) override;
    os_error_t shutdown() override;
    os_error_t createUI(lv_obj_t* parent) override;
    os_error_t destroyUI() override;
    os_error_t handleEvent(uint32_t eventType, void* eventData, size_t dataSize) override;

    /**
     * @brief Initialize camera hardware
     * @return OS_OK on success, error code on failure
     */
    os_error_t initializeCamera();

    /**
     * @brief Start camera preview
     * @return OS_OK on success, error code on failure
     */
    os_error_t startPreview();

    /**
     * @brief Stop camera preview
     * @return OS_OK on success, error code on failure
     */
    os_error_t stopPreview();

    /**
     * @brief Capture photo
     * @param filename Optional filename (auto-generated if empty)
     * @return OS_OK on success, error code on failure
     */
    os_error_t capturePhoto(const std::string& filename = "");

    /**
     * @brief Start video recording
     * @param filename Optional filename (auto-generated if empty)
     * @return OS_OK on success, error code on failure
     */
    os_error_t startVideoRecording(const std::string& filename = "");

    /**
     * @brief Stop video recording
     * @return OS_OK on success, error code on failure
     */
    os_error_t stopVideoRecording();

    /**
     * @brief Set camera mode
     * @param mode Camera mode to set
     * @return OS_OK on success, error code on failure
     */
    os_error_t setCameraMode(CameraMode mode);

    /**
     * @brief Configure camera settings
     * @param config Camera configuration
     * @return OS_OK on success, error code on failure
     */
    os_error_t configureCameraSettings(const CameraConfig& config);

    /**
     * @brief Set camera resolution
     * @param resolution Resolution to set
     * @return OS_OK on success, error code on failure
     */
    os_error_t setResolution(CaptureResolution resolution);

    /**
     * @brief Toggle camera on/off
     * @return OS_OK on success, error code on failure
     */
    os_error_t toggleCamera();

    /**
     * @brief Print camera statistics to log
     */
    void printCameraStats() const;

    /**
     * @brief Get list of captured media files
     * @return Vector of media files
     */
    std::vector<MediaFile> getMediaFiles() const;

    /**
     * @brief Delete media file
     * @param filename File to delete
     * @return OS_OK on success, error code on failure
     */
    os_error_t deleteMediaFile(const std::string& filename);

    /**
     * @brief Get current camera configuration
     * @return Current configuration
     */
    const CameraConfig& getConfig() const { return m_config; }

    /**
     * @brief Check if camera is initialized
     * @return true if initialized, false otherwise
     */
    bool isCameraInitialized() const { return m_cameraInitialized; }

    /**
     * @brief Check if preview is active
     * @return true if preview active, false otherwise
     */
    bool isPreviewActive() const { return m_previewActive; }

    /**
     * @brief Check if recording video
     * @return true if recording, false otherwise
     */
    bool isRecording() const { return m_recording; }

private:
    /**
     * @brief Create camera UI components
     */
    void createCameraUI();

    /**
     * @brief Create preview UI elements
     */
    void createPreviewUI();

    /**
     * @brief Create camera control buttons
     */
    void createCameraControls();

    /**
     * @brief Create preview display
     */
    void createPreviewDisplay();

    /**
     * @brief Create control panel
     */
    void createControlPanel();

    /**
     * @brief Create settings dialog
     */
    void createSettingsDialog();

    /**
     * @brief Create gallery view
     */
    void createGalleryView();

    /**
     * @brief Update preview display
     */
    void updatePreviewDisplay();

    /**
     * @brief Update preview with camera frame
     */
    void updatePreview();

    /**
     * @brief Update control buttons
     */
    void updateControlButtons();

    /**
     * @brief Update gallery thumbnails
     */
    void updateGalleryThumbnails();

    /**
     * @brief Process camera frame
     */
    void processCameraFrame();

    /**
     * @brief Save captured image to storage
     * @param fb Frame buffer containing image data
     * @return OS_OK on success, error code on failure
     */
    os_error_t saveImage(camera_fb_t* fb, const std::string& customFilename = "");

    /**
     * @brief Generate unique filename
     * @param extension File extension (.jpg or .mp4)
     * @return Generated filename
     */
    std::string generateFilename(const char* extension);

    /**
     * @brief Scan media directory for files
     * @return OS_OK on success, error code on failure
     */
    os_error_t scanMediaDirectory();

    /**
     * @brief Apply camera settings
     * @return OS_OK on success, error code on failure
     */
    os_error_t applyCameraSettings();

    /**
     * @brief Camera task for frame processing
     * @param parameter Task parameter
     */
    static void cameraTask(void* parameter);

    // UI event callbacks
    static void captureButtonCallback(lv_event_t* e);
    static void recordButtonCallback(lv_event_t* e);
    static void modeButtonCallback(lv_event_t* e);
    static void settingsButtonCallback(lv_event_t* e);
    static void galleryButtonCallback(lv_event_t* e);
    static void backButtonCallback(lv_event_t* e);
    static void saveSettingsCallback(lv_event_t* e);
    static void cancelSettingsCallback(lv_event_t* e);
    static void deleteFileCallback(lv_event_t* e);

    // Camera configuration
    CameraConfig m_config;
    CameraMode m_currentMode = CameraMode::PREVIEW;
    CaptureResolution m_resolution = CaptureResolution::HD_1280x720;
    bool m_cameraInitialized = false;
    bool m_previewActive = false;
    bool m_recording = false;

    // Media storage
    std::vector<MediaFile> m_mediaFiles;
    std::string m_mediaDirectory = "/sdcard/camera";
    std::string m_currentRecordingFile;
    uint32_t m_recordingStartTime = 0;

    // UI elements - Main container
    lv_obj_t* m_cameraContainer = nullptr;
    lv_obj_t* m_previewContainer = nullptr;
    lv_obj_t* m_previewImage = nullptr;
    lv_obj_t* m_controlPanel = nullptr;

    // UI elements - Control buttons
    lv_obj_t* m_captureButton = nullptr;
    lv_obj_t* m_recordButton = nullptr;
    lv_obj_t* m_modeButton = nullptr;
    lv_obj_t* m_settingsButton = nullptr;
    lv_obj_t* m_galleryButton = nullptr;
    lv_obj_t* m_flashButton = nullptr;

    // UI elements - Status display
    lv_obj_t* m_statusLabel = nullptr;
    lv_obj_t* m_resolutionLabel = nullptr;
    lv_obj_t* m_recordingIndicator = nullptr;
    lv_obj_t* m_recordingTimer = nullptr;
    lv_obj_t* m_batteryIndicator = nullptr;
    lv_obj_t* m_storageIndicator = nullptr;
    lv_obj_t* m_uiContainer = nullptr;

    // UI elements - Settings dialog
    lv_obj_t* m_settingsDialog = nullptr;
    lv_obj_t* m_resolutionDropdown = nullptr;
    lv_obj_t* m_qualitySlider = nullptr;
    lv_obj_t* m_brightnessSlider = nullptr;
    lv_obj_t* m_contrastSlider = nullptr;
    lv_obj_t* m_saturationSlider = nullptr;
    lv_obj_t* m_effectDropdown = nullptr;
    lv_obj_t* m_autoExposureSwitch = nullptr;
    lv_obj_t* m_autoWBSwitch = nullptr;
    lv_obj_t* m_flipHSwitch = nullptr;
    lv_obj_t* m_flipVSwitch = nullptr;
    lv_obj_t* m_saveSettingsButton = nullptr;
    lv_obj_t* m_cancelSettingsButton = nullptr;

    // UI elements - Gallery view
    lv_obj_t* m_galleryContainer = nullptr;
    lv_obj_t* m_thumbnailGrid = nullptr;
    lv_obj_t* m_galleryBackButton = nullptr;
    lv_obj_t* m_deleteButton = nullptr;

    // Camera hardware interface
    void* m_cameraHandle = nullptr;
    uint8_t* m_frameBuffer = nullptr;
    size_t m_frameBufferSize = 0;
    
    // Preview buffer for LVGL display
    uint8_t* m_previewBuffer = nullptr;
    size_t m_previewBufferSize = 0;
    lv_img_dsc_t m_previewImageDesc;

    // Task management
    TaskHandle_t m_cameraTaskHandle = nullptr;
    bool m_taskRunning = false;

    // Statistics
    uint32_t m_photosCaptured = 0;
    uint32_t m_videosRecorded = 0;
    uint32_t m_totalStorageUsed = 0;
    uint32_t m_totalFrames = 0;
    float m_frameRate = 0.0f;
    uint32_t m_lastFrameTime = 0;

    // Hardware configuration
    static constexpr size_t FRAME_BUFFER_SIZE = 1024 * 1024; // 1MB
    static constexpr uint32_t CAMERA_TASK_STACK_SIZE = 8192;
    static constexpr UBaseType_t CAMERA_TASK_PRIORITY = 6;
    static constexpr uint32_t PREVIEW_UPDATE_INTERVAL = 33; // ~30 FPS
    static constexpr size_t MAX_MEDIA_FILES = 1000;
    static constexpr size_t MAX_FILENAME_LENGTH = 256;
    static constexpr int PREVIEW_WIDTH = 320;
    static constexpr int PREVIEW_HEIGHT = 240;

    // Color scheme
    static constexpr lv_color_t COLOR_CAPTURE = LV_COLOR_MAKE(0xE7, 0x4C, 0x3C);
    static constexpr lv_color_t COLOR_RECORD = LV_COLOR_MAKE(0xC0, 0x39, 0x2B);
    static constexpr lv_color_t COLOR_SETTINGS = LV_COLOR_MAKE(0x34, 0x98, 0xDB);
    static constexpr lv_color_t COLOR_SUCCESS = LV_COLOR_MAKE(0x2E, 0xCC, 0x71);
};

/**
 * @brief Factory function for creating app instances
 * Required for dynamic loading by the app manager
 * @return Unique pointer to app instance
 */
extern "C" std::unique_ptr<BaseApp> createCameraApp();

#endif // CAMERA_APP_H