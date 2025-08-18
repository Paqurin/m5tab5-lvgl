#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

/**
 * @file hardware_config.h
 * @brief Hardware configuration for M5Stack Tab5 ESP32-P4
 * 
 * Pin mappings and hardware definitions for the M5Stack Tab5
 * ESP32-P4 RISC-V dual-core development kit with 5" MIPI-DSI display
 * and GT911 multi-touch controller.
 */

// Board identification
#ifndef BOARD_M5STACK_TAB5
#define BOARD_M5STACK_TAB5
#endif
#ifndef MCU_ESP32_P4
#define MCU_ESP32_P4
#endif
#ifndef ARCHITECTURE_RISCV
#define ARCHITECTURE_RISCV
#endif

// Display specifications
#define DISPLAY_WIDTH           1280
#define DISPLAY_HEIGHT          720
#define DISPLAY_INTERFACE       MIPI_DSI
#define DISPLAY_COLOR_DEPTH     16    // RGB565
#define DISPLAY_DPI            320

// Touch controller specifications
#define TOUCH_CONTROLLER        GT911
#define TOUCH_MAX_POINTS        10
#define TOUCH_INTERFACE         I2C

// Memory configuration
#define FLASH_SIZE_MB           16
#define PSRAM_SIZE_MB           32
#define INTERNAL_SRAM_KB        500

// GT911 Touch Controller I2C Configuration
#define GT911_I2C_ADDR_1        0x5D
#define GT911_I2C_ADDR_2        0x14
#define GT911_I2C_FREQ          400000  // 400kHz

// GT911 Pin assignments (ESP32-P4 specific)
// Note: These are typical assignments - verify with actual schematic
#define GT911_INT_PIN           GPIO_NUM_7
#define GT911_RST_PIN           GPIO_NUM_8
#define GT911_SDA_PIN           GPIO_NUM_6
#define GT911_SCL_PIN           GPIO_NUM_5

// I2C Bus configuration
#define I2C_MASTER_NUM          I2C_NUM_0
#define I2C_MASTER_FREQ_HZ      400000
#define I2C_MASTER_TX_BUF_LEN   0
#define I2C_MASTER_RX_BUF_LEN   0
#define I2C_MASTER_TIMEOUT_MS   1000

// MIPI-DSI Configuration
// Note: ESP32-P4 has dedicated MIPI-DSI peripheral
#define MIPI_DSI_HOST           0
#define MIPI_DSI_LANES          2       // Dual-lane MIPI-DSI
#define MIPI_DSI_PIXEL_FORMAT   MIPI_DSI_FMT_RGB565

// MIPI-DSI Pin assignments (ESP32-P4 specific)
// Note: These are hardware-fixed pins on ESP32-P4
#define DSI_CLK_P_PIN           GPIO_NUM_45
#define DSI_CLK_N_PIN           GPIO_NUM_46
#define DSI_D0_P_PIN            GPIO_NUM_47
#define DSI_D0_N_PIN            GPIO_NUM_48
#define DSI_D1_P_PIN            GPIO_NUM_21
#define DSI_D1_N_PIN            GPIO_NUM_14

// Display control pins
#define DISPLAY_BL_PIN          GPIO_NUM_9   // Backlight PWM control
#define DISPLAY_RST_PIN         GPIO_NUM_10  // Display reset
#define DISPLAY_TE_PIN          GPIO_NUM_11  // Tearing effect signal

// Power management pins
#define PWR_EN_PIN              GPIO_NUM_12  // Power enable
#define PWR_SENSE_PIN           GPIO_NUM_13  // Power sense/monitoring

// Camera configuration (SC2356 2MP via MIPI-CSI)
#define CAMERA_ENABLED          1
#define CAMERA_MIPI_CSI         1
#define CAMERA_SENSOR_SC2356    1
#define CAMERA_RESOLUTION_W     1600
#define CAMERA_RESOLUTION_H     1200
#define CAMERA_FORMAT_JPEG      1

// MIPI-CSI pins for SC2356 camera
#define CSI_D0_P_PIN            GPIO_NUM_49
#define CSI_D0_N_PIN            GPIO_NUM_50
#define CSI_D1_P_PIN            GPIO_NUM_51
#define CSI_D1_N_PIN            GPIO_NUM_52
#define CSI_CLK_P_PIN           GPIO_NUM_53
#define CSI_CLK_N_PIN           GPIO_NUM_54
#define CAMERA_RST_PIN          GPIO_NUM_21
#define CAMERA_PWR_PIN          GPIO_NUM_22

// Storage configuration (Official Tab5 pin mapping)
#define SD_CARD_ENABLED         1
#define SD_CS_PIN               GPIO_NUM_40  // Official: G40
#define SD_MOSI_PIN             GPIO_NUM_42  // Official: G42
#define SD_MISO_PIN             GPIO_NUM_39  // Official: G39
#define SD_CLK_PIN              GPIO_NUM_41  // Official: G41

// M5-Bus Expansion Interface (Official Tab5 pin mapping)
#define M5_BUS_ENABLED          1
#define M5_BUS_SDA_PIN          GPIO_NUM_31  // Official: G31 (Int SDA)
#define M5_BUS_SCL_PIN          GPIO_NUM_32  // Official: G32 (Int SCL)
#define M5_BUS_GPIO16           GPIO_NUM_16  // Official: G16
#define M5_BUS_GPIO17           GPIO_NUM_17  // Official: G17 (PB_IN)
#define M5_BUS_MOSI_PIN         GPIO_NUM_18  // Official: G18
#define M5_BUS_GPIO45           GPIO_NUM_45  // Official: G45
#define M5_BUS_MISO_PIN         GPIO_NUM_19  // Official: G19
#define M5_BUS_GPIO52           GPIO_NUM_52  // Official: G52 (PB_OUT)
#define M5_BUS_SCK_PIN          GPIO_NUM_5   // Official: G5
#define M5_BUS_RXD0_PIN         GPIO_NUM_38  // Official: G38
#define M5_BUS_TXD0_PIN         GPIO_NUM_37  // Official: G37
#define M5_BUS_PC_RX_PIN        GPIO_NUM_7   // Official: G7
#define M5_BUS_PC_TX_PIN        GPIO_NUM_6   // Official: G6
#define M5_BUS_GPIO3            GPIO_NUM_3   // Official: G3
#define M5_BUS_GPIO4            GPIO_NUM_4   // Official: G4
#define M5_BUS_GPIO2            GPIO_NUM_2   // Official: G2
#define M5_BUS_GPIO48           GPIO_NUM_48  // Official: G48
#define M5_BUS_GPIO47           GPIO_NUM_47  // Official: G47
#define M5_BUS_GPIO35           GPIO_NUM_35  // Official: G35
#define M5_BUS_GPIO51           GPIO_NUM_51  // Official: G51

// SD Card SPI pins
#define SD_SCK_PIN              SD_CLK_PIN
#define SD_CMD_PIN              SD_MOSI_PIN
#define SD_D0_PIN               SD_MISO_PIN

// GPIO extension header
#define GPIO_EXT_ENABLED        1

// Status LEDs (if available)
#define STATUS_LED_PIN          GPIO_NUM_2
#define STATUS_LED_ACTIVE_LOW   0

// Button configuration (if available)
#define BUTTON_A_PIN            GPIO_NUM_3
#define BUTTON_B_PIN            GPIO_NUM_4

// PWM channels for backlight control
#define PWM_BACKLIGHT_CHANNEL   0
#define PWM_BACKLIGHT_FREQ      1000   // 1kHz
#define PWM_BACKLIGHT_RESOLUTION LEDC_TIMER_8_BIT

// Timing configurations
#define BOOT_DELAY_MS           1000
#define DISPLAY_INIT_DELAY_MS   100
#define TOUCH_INIT_DELAY_MS     200
#define I2C_TIMEOUT_MS          1000

// Memory allocation for display buffers - Optimized for 32MB PSRAM
#define DISPLAY_BUFFER_LINES    60     // Number of lines to buffer (increased for smoother rendering)
#define DISPLAY_BUFFER_SIZE     (DISPLAY_WIDTH * DISPLAY_BUFFER_LINES * 2) // RGB565 = 2 bytes/pixel
#define DISPLAY_DOUBLE_BUFFER   1      // Enable double buffering for smooth animations
#define DISPLAY_FRAMEBUFFER_SIZE (DISPLAY_WIDTH * DISPLAY_HEIGHT * 2) // Full frame buffer

// LVGL Memory Configuration
#define LVGL_BUFFER_LINES       120    // Large buffer for smooth scrolling
#define LVGL_BUFFER_SIZE        (DISPLAY_WIDTH * LVGL_BUFFER_LINES * 2)
#define LVGL_MEM_SIZE           (8 * 1024 * 1024)  // 8MB for LVGL objects in PSRAM

// Debug configuration
#define DEBUG_SERIAL_ENABLED    1
#define DEBUG_BAUD_RATE         115200

// Hardware revision detection
#define HW_REV_PIN_0            GPIO_NUM_0
#define HW_REV_PIN_1            GPIO_NUM_1

// Validation macros
#if DISPLAY_WIDTH != 1280 || DISPLAY_HEIGHT != 720
#error "Display resolution mismatch for M5Stack Tab5"
#endif

#if DISPLAY_COLOR_DEPTH != 16
#error "Color depth must be 16-bit for M5Stack Tab5"
#endif

// Audio configuration (ES8388 codec + NS4150 amplifiers)
#define AUDIO_ENABLED           1
#define AUDIO_CODEC_ES8388      1
#define AUDIO_AMP_NS4150        1

// ES8388 I2C and I2S configuration
#define ES8388_I2C_ADDR         0x10
#define ES8388_I2C_SDA_PIN      GPIO_NUM_22
#define ES8388_I2C_SCL_PIN      GPIO_NUM_23
#define ES8388_MCLK_PIN         GPIO_NUM_24
#define ES8388_BCLK_PIN         GPIO_NUM_25
#define ES8388_LRCK_PIN         GPIO_NUM_26
#define ES8388_DIN_PIN          GPIO_NUM_27
#define ES8388_DOUT_PIN         GPIO_NUM_28

// I2S Pin aliases for audio service compatibility
#define I2S_BCK_PIN             ES8388_BCLK_PIN
#define I2S_WS_PIN              ES8388_LRCK_PIN
#define I2S_DO_PIN              ES8388_DOUT_PIN
#define I2S_DI_PIN              ES8388_DIN_PIN
#define I2S_MCLK_PIN            ES8388_MCLK_PIN

// Audio control pins
#define HEADPHONE_DETECT_PIN    GPIO_NUM_29
#define SPEAKER_EN_PIN          GPIO_NUM_30
#define AUDIO_PWR_PIN           GPIO_NUM_31
#define NS4150_EN_PIN           SPEAKER_EN_PIN  // NS4150 amplifier enable
#define ES8388_PWR_PIN          AUDIO_PWR_PIN   // ES8388 power control

// Dual microphone configuration
#define DUAL_MIC_ENABLED        1
#define MIC_PRIMARY_ENABLED     1               // Primary microphone (front-facing)
#define MIC_SECONDARY_ENABLED   1               // Secondary microphone (noise cancellation)
#define MIC_PDM_ENABLED         1               // PDM interface for digital microphones
#define MIC_ANALOG_ENABLED      1               // Analog microphone via ES8388 ADC

// Primary microphone (PDM) - Front-facing for voice commands
#define MIC_PRIMARY_CLK_PIN     GPIO_NUM_45     // PDM clock
#define MIC_PRIMARY_DATA_PIN    GPIO_NUM_46     // PDM data

// Secondary microphone (PDM) - Rear-facing for noise cancellation
#define MIC_SECONDARY_CLK_PIN   GPIO_NUM_47     // PDM clock (can share with primary)
#define MIC_SECONDARY_DATA_PIN  GPIO_NUM_48     // PDM data

// Analog microphone via ES8388 (backup/additional input)
#define MIC_ANALOG_INPUT_PIN    ES8388_DIN_PIN  // Uses ES8388 ADC input
#define MIC_BIAS_EN_PIN         GPIO_NUM_55     // Microphone bias enable
#define MIC_GAIN_CTRL_PIN       GPIO_NUM_56     // Microphone gain control

// RS-485 configuration (Official Tab5 pin mapping)
#define RS485_ENABLED           1
#define RS485_TX_PIN            GPIO_NUM_20  // Official: G20
#define RS485_RX_PIN            GPIO_NUM_21  // Official: G21
#define RS485_DE_PIN            GPIO_NUM_34  // Official: G34 (DIR pin)
#define RS485_RE_PIN            GPIO_NUM_34  // Same as DE pin (SIT3088 uses single DIR)
#define RS485_RTS_PIN           RS485_DE_PIN // Same as DE pin
#define RS485_CTS_PIN           (-1)         // Not used

// USB Host configuration (USB-A port)
#define USB_HOST_ENABLED        1
#define USB_HOST_DP_PIN         GPIO_NUM_36
#define USB_HOST_DM_PIN         GPIO_NUM_37

// USB-C OTG configuration
#define USB_OTG_ENABLED         1
#define USB_OTG_DP_PIN          GPIO_NUM_19
#define USB_OTG_DM_PIN          GPIO_NUM_20
#define USB_OTG_VBUS_PIN        GPIO_NUM_4   // VBUS control for power delivery
#define USB_OTG_ID_PIN          GPIO_NUM_0   // OTG ID pin for host/device detection

// Power management (PMS150G interface)
#define PMS150G_ENABLED         1
#define PMS150G_INT_PIN         GPIO_NUM_38  // Interrupt from PMS150G
#define PMS150G_PWR_BTN_PIN     GPIO_NUM_39  // Power button input
#define PMS150G_RESET_PIN       GPIO_NUM_40  // Reset control

// LPW5209 5V power switch controls
#define LPW5209_ENABLED         1
#define LPW5209_EN1_PIN         GPIO_NUM_41  // First 5V output enable
#define LPW5209_EN2_PIN         GPIO_NUM_42  // Second 5V output enable
#define LPW5209_FAULT1_PIN      GPIO_NUM_43  // First output fault detection
#define LPW5209_FAULT2_PIN      GPIO_NUM_44  // Second output fault detection

// ESP32-C6 SDIO Interface (Official Tab5 pin mapping)
#define ESP32_C6_ENABLED        1
#define ESP32_C6_SDIO2_D0       GPIO_NUM_11  // Official: G11
#define ESP32_C6_SDIO2_D1       GPIO_NUM_10  // Official: G10
#define ESP32_C6_SDIO2_D2       GPIO_NUM_9   // Official: G9
#define ESP32_C6_SDIO2_D3       GPIO_NUM_8   // Official: G8
#define ESP32_C6_SDIO2_CMD      GPIO_NUM_13  // Official: G13
#define ESP32_C6_SDIO2_CK       GPIO_NUM_12  // Official: G12
#define ESP32_C6_RESET          GPIO_NUM_15  // Official: G15
#define ESP32_C6_IO2            GPIO_NUM_14  // Official: G14

// HY2.0-4P Expansion Port (Official Tab5 pin mapping)
#define HY2_EXPANSION_ENABLED   1
#define HY2_PORT_A_PIN1         GPIO_NUM_53  // Official: G53 (Yellow wire)
#define HY2_PORT_A_PIN2         GPIO_NUM_54  // Official: G54 (White wire)

// Wireless control
#define WIFI_EN_PIN             GPIO_NUM_1   // WiFi enable control
#define BT_EN_PIN               GPIO_NUM_0   // Bluetooth enable control

// Sleep/Wake configuration
#define SLEEP_MODE_ENABLED      1
#define WAKE_UP_PIN             PMS150G_INT_PIN
#define SLEEP_TIMEOUT_MS        (30 * 60 * 1000)  // 30 minutes

// Hardware capability flags
#define HW_HAS_PSRAM            1
#define HW_HAS_TOUCH            1
#define HW_HAS_DISPLAY          1
#define HW_HAS_CAMERA           1
#define HW_HAS_SD_CARD          1
#define HW_HAS_USB_HOST         1
#define HW_HAS_USB_OTG          1
#define HW_HAS_AUDIO            1
#define HW_HAS_DUAL_MIC         1
#define HW_HAS_NOISE_CANCEL     1
#define HW_HAS_RS485            1
#define HW_HAS_WIFI             1
#define HW_HAS_BLE              1
#define HW_HAS_POWER_MGMT       1

// Performance optimization flags
#define PSRAM_SPEED_120MHZ      1      // Use high-speed PSRAM configuration
#define ENABLE_PSRAM_CACHE      1      // Enable PSRAM caching for better performance
#define DMA_BURST_SIZE          64     // Optimized DMA burst size for PSRAM

#endif // HARDWARE_CONFIG_H