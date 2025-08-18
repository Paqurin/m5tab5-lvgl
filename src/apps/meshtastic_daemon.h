#ifndef MESHTASTIC_DAEMON_H
#define MESHTASTIC_DAEMON_H

/**
 * @file meshtastic_daemon.h
 * @brief Meshtastic Daemon for M5Stack Tab5 - LoRa Mesh Networking
 * 
 * Implements a complete Meshtastic-compatible mesh networking daemon with LoRa
 * communication, encrypted messaging, node discovery, and position tracking.
 * 
 * Key Features:
 * - LoRa mesh networking with automatic routing
 * - AES-256 encrypted channels with PSK authentication
 * - Node discovery and neighbor management
 * - Position tracking and broadcasting
 * - Message store-and-forward capability
 * - MQTT gateway integration
 * - Bluetooth connectivity for mobile app interfaces
 * - Range testing and network analysis tools
 * 
 * Architecture:
 * - Event-driven message processing
 * - Multi-channel support (up to 8 channels)
 * - Adaptive transmission power and spreading factor
 * - Battery-aware power management
 * - Comprehensive logging and diagnostics
 */

#include "../system/os_config.h"
#include "../hal/hal_manager.h"
#include <vector>
#include <map>
#include <string>
#include <functional>

// Meshtastic Protocol Constants
#define MESHTASTIC_MAX_CHANNELS         8
#define MESHTASTIC_MAX_HOPS             7
#define MESHTASTIC_MAX_NODES            256
#define MESHTASTIC_MESSAGE_TIMEOUT_MS   300000  // 5 minutes
#define MESHTASTIC_BEACON_INTERVAL_MS   900000  // 15 minutes
#define MESHTASTIC_NEIGHBOR_TIMEOUT_MS  1800000 // 30 minutes

// LoRa Configuration
#define MESHTASTIC_LORA_FREQ_US         915.0   // US frequency (MHz)
#define MESHTASTIC_LORA_FREQ_EU         868.0   // EU frequency (MHz)
#define MESHTASTIC_LORA_BANDWIDTH       125.0   // kHz
#define MESHTASTIC_LORA_SPREADING       12      // SF12 for maximum range
#define MESHTASTIC_LORA_CODING_RATE     8       // 4/8 coding rate
#define MESHTASTIC_LORA_PREAMBLE        8       // Preamble symbols
#define MESHTASTIC_LORA_SYNC_WORD       0x2B    // Private sync word

// Message Types (Meshtastic Protocol)
enum class MeshtasticMessageType : uint8_t {
    TEXT_MESSAGE_UTF8 = 1,
    BINARY_MESSAGE = 2,
    POSITION = 3,
    NODEINFO = 4,
    ROUTING_ERROR = 5,
    ADMIN_MESSAGE = 10,
    TEXT_MESSAGE_COMPRESSED = 11,
    WAYPOINT = 12,
    AUDIO = 13,
    DETECTION_SENSOR = 14,
    REPLY = 32,
    IP_TUNNEL_CLIENT = 33,
    IP_TUNNEL_SERVER = 34,
    PAXCOUNTER = 40,
    MESH_SERIAL = 41,
    STORE_FORWARD_CLIENT = 42,
    STORE_FORWARD_SERVER = 43,
    RANGE_TEST_CLIENT = 44,
    RANGE_TEST_SERVER = 45,
    TELEMETRY = 50,
    ZPS = 51,
    SIMULATOR = 52,
    TRACEROUTE = 53,
    NEIGHBOR_INFO = 54,
    ATAK_PLUGIN = 55,
    MAP_REPORT = 56
};

// Node Information
struct MeshtasticNode {
    uint32_t id;                    // Node ID (32-bit)
    std::string shortName;          // Short display name
    std::string longName;           // Full name
    std::string macaddr;            // MAC address
    uint8_t hwModel;                // Hardware model
    double latitude;                // GPS latitude
    double longitude;               // GPS longitude
    int32_t altitude;               // Altitude in meters
    uint32_t lastSeen;              // Last seen timestamp
    uint8_t snr;                    // Signal-to-noise ratio
    int8_t rssi;                    // Received signal strength
    uint8_t hopLimit;               // Hop limit to reach this node
    bool viaMqtt;                   // Received via MQTT
    uint8_t batteryLevel;           // Battery percentage
    float voltage;                  // Battery voltage
    uint32_t uptimeSeconds;         // Node uptime
};

// Message Structure
struct MeshtasticMessage {
    uint32_t id;                    // Message ID
    uint32_t fromNode;              // Sender node ID
    uint32_t toNode;                // Destination (0 = broadcast)
    uint8_t channel;                // Channel index
    MeshtasticMessageType type;     // Message type
    std::vector<uint8_t> payload;   // Message payload
    uint32_t timestamp;             // Send timestamp
    uint8_t hopLimit;               // Maximum hops
    uint8_t hopStart;               // Starting hop count
    bool wantAck;                   // Request acknowledgment
    uint32_t requestId;             // Request ID for replies
    int8_t rssi;                    // Received signal strength
    uint8_t snr;                    // Signal-to-noise ratio
    bool encrypted;                 // Message encrypted
};

// Channel Configuration
struct MeshtasticChannel {
    uint8_t index;                  // Channel index (0-7)
    std::string name;               // Channel name
    std::vector<uint8_t> psk;       // Pre-shared key (32 bytes for AES-256)
    bool uplinkEnabled;             // Allow uplink to MQTT
    bool downlinkEnabled;           // Allow downlink from MQTT
    uint32_t lastActivity;          // Last activity timestamp
    uint32_t messageCount;          // Message count
};

// Range Test Result
struct MeshtasticRangeTest {
    uint32_t nodeId;                // Target node ID
    uint32_t sequenceNumber;        // Sequence number
    uint32_t timestamp;             // Test timestamp
    double distance;                // Distance in meters
    int8_t rssi;                    // Signal strength
    uint8_t snr;                    // Signal-to-noise ratio
    bool success;                   // Test success
    uint32_t roundTripTime;         // Round trip time (ms)
};

// Forward declarations
class MeshtasticGUI;

// Event callback types
using NodeUpdateCallback = std::function<void(const MeshtasticNode&)>;
using MessageCallback = std::function<void(const MeshtasticMessage&)>;
using RangeTestCallback = std::function<void(const MeshtasticRangeTest&)>;

/**
 * @class MeshtasticDaemon
 * @brief Core Meshtastic mesh networking daemon
 * 
 * Implements the complete Meshtastic protocol stack including LoRa PHY layer,
 * mesh routing, encryption, node management, and application services.
 */
class MeshtasticDaemon {
public:
    MeshtasticDaemon();
    ~MeshtasticDaemon();

    // === Core Daemon Control ===
    
    /**
     * @brief Initialize Meshtastic daemon
     * @param halManager HAL manager for hardware access
     * @return OS_OK on success
     */
    os_error_t initialize(HALManager* halManager);
    
    /**
     * @brief Shutdown daemon and cleanup resources
     * @return OS_OK on success
     */
    os_error_t shutdown();
    
    /**
     * @brief Main daemon update loop - call from main thread
     * @param deltaTime Time elapsed since last update (ms)
     * @return OS_OK on success
     */
    os_error_t update(uint32_t deltaTime);
    
    /**
     * @brief Check if daemon is initialized and running
     * @return true if running
     */
    bool isRunning() const { return m_running; }
    
    // === Configuration ===
    
    /**
     * @brief Set node information
     * @param shortName Short display name (8 chars max)
     * @param longName Full name (32 chars max)
     * @return OS_OK on success
     */
    os_error_t setNodeInfo(const std::string& shortName, const std::string& longName);
    
    /**
     * @brief Configure LoRa parameters
     * @param frequency Frequency in MHz
     * @param spreadingFactor Spreading factor (7-12)
     * @param bandwidth Bandwidth in kHz
     * @param codingRate Coding rate (5-8)
     * @param txPower Transmit power in dBm
     * @return OS_OK on success
     */
    os_error_t configureLoRa(float frequency, uint8_t spreadingFactor, 
                             float bandwidth, uint8_t codingRate, int8_t txPower);
    
    /**
     * @brief Add/configure channel
     * @param index Channel index (0-7)
     * @param name Channel name
     * @param psk Pre-shared key (32 bytes)
     * @return OS_OK on success
     */
    os_error_t configureChannel(uint8_t index, const std::string& name, 
                                const std::vector<uint8_t>& psk);
    
    /**
     * @brief Set active channel for messaging
     * @param index Channel index
     * @return OS_OK on success
     */
    os_error_t setActiveChannel(uint8_t index);
    
    // === Messaging ===
    
    /**
     * @brief Send text message
     * @param message Text message content
     * @param toNode Destination node ID (0 for broadcast)
     * @param channel Channel index
     * @param wantAck Request acknowledgment
     * @return Message ID or 0 on failure
     */
    uint32_t sendTextMessage(const std::string& message, uint32_t toNode = 0, 
                             uint8_t channel = 0, bool wantAck = false);
    
    /**
     * @brief Send binary data
     * @param data Binary payload
     * @param toNode Destination node ID
     * @param channel Channel index
     * @param type Message type
     * @return Message ID or 0 on failure
     */
    uint32_t sendBinaryMessage(const std::vector<uint8_t>& data, uint32_t toNode,
                               uint8_t channel, MeshtasticMessageType type);
    
    /**
     * @brief Send position update
     * @param latitude Latitude in degrees
     * @param longitude Longitude in degrees
     * @param altitude Altitude in meters
     * @return Message ID or 0 on failure
     */
    uint32_t sendPosition(double latitude, double longitude, int32_t altitude = 0);
    
    /**
     * @brief Request node information from specific node
     * @param nodeId Target node ID
     * @return Request ID or 0 on failure
     */
    uint32_t requestNodeInfo(uint32_t nodeId);
    
    // === Node Management ===
    
    /**
     * @brief Get list of discovered nodes
     * @return Vector of node information
     */
    std::vector<MeshtasticNode> getNodes() const;
    
    /**
     * @brief Get specific node information
     * @param nodeId Node ID to lookup
     * @return Node information or nullptr if not found
     */
    const MeshtasticNode* getNode(uint32_t nodeId) const;
    
    /**
     * @brief Get our own node ID
     * @return Our node ID
     */
    uint32_t getMyNodeId() const { return m_myNodeId; }
    
    /**
     * @brief Get number of nodes in routing table
     * @return Node count
     */
    size_t getNodeCount() const { return m_nodes.size(); }
    
    // === Range Testing ===
    
    /**
     * @brief Start range test to specific node
     * @param nodeId Target node ID
     * @param interval Test interval in seconds
     * @param count Number of tests to perform
     * @return OS_OK on success
     */
    os_error_t startRangeTest(uint32_t nodeId, uint32_t interval = 30, uint32_t count = 10);
    
    /**
     * @brief Stop active range test
     */
    void stopRangeTest();
    
    /**
     * @brief Get range test results
     * @return Vector of test results
     */
    std::vector<MeshtasticRangeTest> getRangeTestResults() const;
    
    // === Statistics and Diagnostics ===
    
    /**
     * @brief Get network statistics
     * @param totalMessages Total messages processed
     * @param messagesPerHour Messages per hour rate
     * @param avgRSSI Average signal strength
     * @param avgSNR Average signal-to-noise ratio
     */
    void getNetworkStats(uint32_t& totalMessages, float& messagesPerHour,
                         int8_t& avgRSSI, uint8_t& avgSNR) const;
    
    /**
     * @brief Get channel statistics
     * @param channel Channel index
     * @param messageCount Message count
     * @param lastActivity Last activity timestamp
     * @return true if channel exists
     */
    bool getChannelStats(uint8_t channel, uint32_t& messageCount, 
                         uint32_t& lastActivity) const;
    
    /**
     * @brief Perform network trace to node
     * @param nodeId Target node ID
     * @return Vector of intermediate node IDs in route
     */
    std::vector<uint32_t> traceRoute(uint32_t nodeId);
    
    // === Event Callbacks ===
    
    /**
     * @brief Set callback for node updates
     * @param callback Function to call when nodes are discovered/updated
     */
    void setNodeUpdateCallback(NodeUpdateCallback callback);
    
    /**
     * @brief Set callback for incoming messages
     * @param callback Function to call when messages are received
     */
    void setMessageCallback(MessageCallback callback);
    
    /**
     * @brief Set callback for range test results
     * @param callback Function to call when range tests complete
     */
    void setRangeTestCallback(RangeTestCallback callback);

private:
    // === Internal State ===
    HALManager* m_halManager;
    bool m_initialized;
    bool m_running;
    uint32_t m_myNodeId;
    MeshtasticNode m_myNode;
    
    // Message and routing
    std::map<uint32_t, MeshtasticNode> m_nodes;
    std::vector<MeshtasticMessage> m_messageHistory;
    std::map<uint32_t, uint32_t> m_messageAcks;  // Message ID -> timestamp
    uint32_t m_nextMessageId;
    
    // Channels and encryption
    std::array<MeshtasticChannel, MESHTASTIC_MAX_CHANNELS> m_channels;
    uint8_t m_activeChannel;
    
    // LoRa configuration
    float m_loraFrequency;
    uint8_t m_loraSF;
    float m_loraBW;
    uint8_t m_loraCR;
    int8_t m_loraPower;
    
    // Range testing
    bool m_rangeTestActive;
    uint32_t m_rangeTestTarget;
    uint32_t m_rangeTestInterval;
    uint32_t m_rangeTestCount;
    uint32_t m_rangeTestNext;
    std::vector<MeshtasticRangeTest> m_rangeTestResults;
    
    // Statistics
    uint32_t m_totalMessagesRx;
    uint32_t m_totalMessagesTx;
    uint32_t m_lastStatsUpdate;
    int32_t m_totalRSSI;
    uint32_t m_totalSNR;
    uint32_t m_signalSamples;
    
    // Callbacks
    NodeUpdateCallback m_nodeCallback;
    MessageCallback m_messageCallback;
    RangeTestCallback m_rangeTestCallback;
    
    // Timing
    uint32_t m_lastBeaconTime;
    uint32_t m_lastNeighborCleanup;
    
    // === Internal Methods ===
    
    /**
     * @brief Initialize LoRa radio hardware
     * @return OS_OK on success
     */
    os_error_t initializeLoRa();
    
    /**
     * @brief Process received LoRa packet
     * @param data Received data
     * @param rssi Signal strength
     * @param snr Signal-to-noise ratio
     */
    void processReceivedPacket(const std::vector<uint8_t>& data, int8_t rssi, uint8_t snr);
    
    /**
     * @brief Send message over LoRa
     * @param msg Message to send
     * @return OS_OK on success
     */
    os_error_t transmitMessage(const MeshtasticMessage& msg);
    
    /**
     * @brief Encrypt message payload
     * @param payload Data to encrypt
     * @param channel Channel configuration
     * @return Encrypted data
     */
    std::vector<uint8_t> encryptPayload(const std::vector<uint8_t>& payload,
                                        const MeshtasticChannel& channel);
    
    /**
     * @brief Decrypt message payload
     * @param payload Encrypted data
     * @param channel Channel configuration
     * @return Decrypted data
     */
    std::vector<uint8_t> decryptPayload(const std::vector<uint8_t>& payload,
                                        const MeshtasticChannel& channel);
    
    /**
     * @brief Route message to next hop
     * @param msg Message to route
     * @return OS_OK if routed, error if no route found
     */
    os_error_t routeMessage(MeshtasticMessage& msg);
    
    /**
     * @brief Update node information
     * @param node Node to update
     */
    void updateNode(const MeshtasticNode& node);
    
    /**
     * @brief Send periodic beacon
     */
    void sendBeacon();
    
    /**
     * @brief Cleanup old nodes and messages
     */
    void performHousekeeping();
    
    /**
     * @brief Process range test message
     * @param msg Range test message
     */
    void handleRangeTest(const MeshtasticMessage& msg);
    
    /**
     * @brief Generate unique message ID
     * @return New message ID
     */
    uint32_t generateMessageId();
    
    /**
     * @brief Calculate distance between two GPS points
     * @param lat1 Latitude 1
     * @param lon1 Longitude 1
     * @param lat2 Latitude 2  
     * @param lon2 Longitude 2
     * @return Distance in meters
     */
    double calculateDistance(double lat1, double lon1, double lat2, double lon2);
};

#endif // MESHTASTIC_DAEMON_H