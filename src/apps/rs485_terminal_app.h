#ifndef RS485_TERMINAL_APP_H
#define RS485_TERMINAL_APP_H

#include "base_app.h"
#include "../hal/hardware_config.h"
#include <driver/uart.h>
#include <vector>
#include <string>
#include <memory>

/**
 * @file rs485_terminal_app.h
 * @brief RS-485 Terminal Application for M5Stack Tab5
 * 
 * Provides RS-485 communication interface with terminal functionality.
 * Supports configurable baud rates, data formats, and transmission protocols.
 */

enum class RS485Mode {
    MASTER,
    SLAVE,
    SNIFFER
};

enum class RS485BaudRate : uint32_t {
    BAUD_9600 = 9600,
    BAUD_19200 = 19200,
    BAUD_38400 = 38400,
    BAUD_57600 = 57600,
    BAUD_115200 = 115200
};

enum class RS485Parity : uint8_t {
    PARITY_NONE = 0,
    PARITY_EVEN = 1,
    PARITY_ODD = 2
};

enum class RS485StopBits : uint8_t {
    STOP_1_BIT = 1,
    STOP_2_BIT = 2
};

struct RS485Config {
    RS485BaudRate baudRate = RS485BaudRate::BAUD_9600;
    uint8_t dataBits = 8;
    RS485Parity parity = RS485Parity::PARITY_NONE;
    RS485StopBits stopBits = RS485StopBits::STOP_1_BIT;
    bool flowControl = false;
    uint32_t timeout = 1000;
};

class RS485TerminalApp : public BaseApp {
public:
    /**
     * @brief Constructor
     */
    RS485TerminalApp();
    
    /**
     * @brief Destructor
     */
    ~RS485TerminalApp() override;

    // BaseApp interface implementation
    os_error_t initialize() override;
    os_error_t update(uint32_t deltaTime) override;
    os_error_t shutdown() override;
    os_error_t createUI(lv_obj_t* parent) override;
    os_error_t destroyUI() override;
    os_error_t handleEvent(uint32_t eventType, void* eventData, size_t dataSize) override;

    /**
     * @brief Configure RS-485 communication
     * @param config Configuration parameters
     * @return OS_OK on success, error code on failure
     */
    os_error_t configureRS485(const RS485Config& config);

    /**
     * @brief Send data via RS-485
     * @param data Data buffer to send
     * @param length Data length
     * @return OS_OK on success, error code on failure
     */
    os_error_t sendData(const uint8_t* data, size_t length);

    /**
     * @brief Send string via RS-485
     * @param text String to send
     * @return OS_OK on success, error code on failure
     */
    os_error_t sendString(const char* text);

    /**
     * @brief Set RS-485 operation mode
     * @param mode Operation mode
     * @return OS_OK on success, error code on failure
     */
    os_error_t setMode(RS485Mode mode);

    /**
     * @brief Clear terminal display
     */
    void clearTerminal();

    /**
     * @brief Get current configuration
     * @return Current RS-485 configuration
     */
    const RS485Config& getConfig() const { return m_config; }

private:
    /**
     * @brief Initialize UART hardware
     * @return OS_OK on success, error code on failure
     */
    os_error_t initializeUART();

    /**
     * @brief Initialize RS-485 hardware
     * @return OS_OK on success, error code on failure
     */
    os_error_t initializeRS485();

    /**
     * @brief Create terminal UI
     */
    void createTerminalUI();

    /**
     * @brief Create configuration dialog
     */
    void createConfigDialog();

    /**
     * @brief Create control panel UI
     */
    void createControlPanel();

    /**
     * @brief Update terminal display
     */
    void updateTerminalDisplay();

    /**
     * @brief Process received data
     */
    void processReceivedData();

    /**
     * @brief Add text to terminal
     * @param text Text to add
     * @param isTransmitted true for TX data, false for RX data
     */
    void addToTerminal(const char* text, bool isTransmitted = false);

    /**
     * @brief Format data for display
     * @param data Data buffer
     * @param length Data length
     * @param showHex Display as hexadecimal
     * @return Formatted string
     */
    std::string formatData(const uint8_t* data, size_t length, bool showHex = false);

    /**
     * @brief Format data for display in buffer
     * @param data Data buffer
     * @param length Data length
     * @param buffer Output buffer
     * @param bufferSize Buffer size
     */
    void formatDataForDisplay(const uint8_t* data, size_t length, char* buffer, size_t bufferSize);

    /**
     * @brief UART receive task
     * @param parameter Task parameter
     */
    static void uartReceiveTask(void* parameter);

    /**
     * @brief UART task for background processing
     * @param parameter Task parameter
     */
    static void uartTask(void* parameter);

    /**
     * @brief Set transmit mode for RS-485
     * @param transmit true for transmit mode, false for receive mode
     */
    void setTransmitMode(bool transmit);

    /**
     * @brief Print RS-485 statistics
     */
    void printRS485Stats() const;

    // UI event callbacks
    static void sendButtonCallback(lv_event_t* e);
    static void clearButtonCallback(lv_event_t* e);
    static void configButtonCallback(lv_event_t* e);
    static void saveConfigCallback(lv_event_t* e);
    static void cancelConfigCallback(lv_event_t* e);
    static void modeButtonCallback(lv_event_t* e);

    // RS-485 configuration
    RS485Config m_config;
    RS485Mode m_mode = RS485Mode::MASTER;
    bool m_uartInitialized = false;

    // Communication buffers
    uint8_t* m_rxBuffer = nullptr;
    uint8_t* m_txBuffer = nullptr;
    size_t m_rxBufferSize = 0;
    size_t m_txBufferSize = 0;
    static constexpr size_t RX_BUFFER_SIZE = 2048;
    static constexpr size_t TX_BUFFER_SIZE = 1024;

    // UI elements
    lv_obj_t* m_terminalContainer = nullptr;
    lv_obj_t* m_terminalTextArea = nullptr;
    lv_obj_t* m_inputTextArea = nullptr;
    lv_obj_t* m_sendButton = nullptr;
    lv_obj_t* m_clearButton = nullptr;
    lv_obj_t* m_configButton = nullptr;
    lv_obj_t* m_modeButton = nullptr;
    lv_obj_t* m_statusLabel = nullptr;
    lv_obj_t* m_controlPanel = nullptr;
    lv_obj_t* m_configLabel = nullptr;

    // Configuration dialog
    lv_obj_t* m_configDialog = nullptr;
    lv_obj_t* m_baudRateDropdown = nullptr;
    lv_obj_t* m_dataBitsDropdown = nullptr;
    lv_obj_t* m_parityDropdown = nullptr;
    lv_obj_t* m_stopBitsDropdown = nullptr;
    lv_obj_t* m_saveConfigButton = nullptr;
    lv_obj_t* m_cancelConfigButton = nullptr;

    // Task management
    TaskHandle_t m_receiveTaskHandle = nullptr;
    TaskHandle_t m_uartTaskHandle = nullptr;
    bool m_taskRunning = false;

    // Display settings
    bool m_showHex = false;
    bool m_hexDisplay = false;
    bool m_showTimestamp = true;
    bool m_autoScroll = true;

    // RS-485 state
    bool m_rs485Initialized = false;
    bool m_transmitMode = false;

    // Statistics
    uint32_t m_bytesReceived = 0;
    uint32_t m_bytesTransmitted = 0;
    uint32_t m_packetsReceived = 0;
    uint32_t m_packetsTransmitted = 0;
    uint32_t m_errorCount = 0;

    // UART hardware configuration - Updated to match hardware_config.h
    static constexpr uart_port_t UART_PORT = UART_NUM_2;
    static constexpr int TX_PIN = RS485_TX_PIN;
    static constexpr int RX_PIN = RS485_RX_PIN;
    static constexpr int RTS_PIN = RS485_RTS_PIN;
    static constexpr int CTS_PIN = -1; // Not used for RS485
    static constexpr int DE_PIN = RS485_DE_PIN;
    static constexpr int RE_PIN = RS485_RE_PIN;
    static constexpr UBaseType_t TASK_PRIORITY = 5;
    static constexpr uint32_t TASK_STACK_SIZE = 4096;
    static constexpr uint32_t UART_TASK_STACK_SIZE = 4096;
    static constexpr UBaseType_t UART_TASK_PRIORITY = 5;
    static constexpr size_t TERMINAL_BUFFER_SIZE = 8192;
};

/**
 * @brief Factory function for creating app instances
 * Required for dynamic loading by the app manager
 * @return Unique pointer to app instance
 */
extern "C" std::unique_ptr<BaseApp> createRS485TerminalApp();

#endif // RS485_TERMINAL_APP_H