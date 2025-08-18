#include "meshtastic_daemon.h"
#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_random.h>
#include <WiFi.h>
#include <cstring>
#include <cmath>
#include <algorithm>

static const char* TAG = "MeshtasticDaemon";

MeshtasticDaemon::MeshtasticDaemon()
    : m_halManager(nullptr)
    , m_initialized(false)
    , m_running(false)
    , m_myNodeId(0)
    , m_nextMessageId(1)
    , m_activeChannel(0)
    , m_loraFrequency(MESHTASTIC_LORA_FREQ_US)
    , m_loraSF(MESHTASTIC_LORA_SPREADING)
    , m_loraBW(MESHTASTIC_LORA_BANDWIDTH)
    , m_loraCR(MESHTASTIC_LORA_CODING_RATE)
    , m_loraPower(14)  // 14 dBm default
    , m_rangeTestActive(false)
    , m_rangeTestTarget(0)
    , m_rangeTestInterval(30)
    , m_rangeTestCount(0)
    , m_rangeTestNext(0)
    , m_totalMessagesRx(0)
    , m_totalMessagesTx(0)
    , m_lastStatsUpdate(0)
    , m_totalRSSI(0)
    , m_totalSNR(0)
    , m_signalSamples(0)
    , m_lastBeaconTime(0)
    , m_lastNeighborCleanup(0)
{
    // Initialize channels with default configuration
    for (uint8_t i = 0; i < MESHTASTIC_MAX_CHANNELS; i++) {
        m_channels[i].index = i;
        m_channels[i].name = "Channel " + std::to_string(i + 1);
        m_channels[i].uplinkEnabled = false;
        m_channels[i].downlinkEnabled = false;
        m_channels[i].lastActivity = 0;
        m_channels[i].messageCount = 0;
        
        // Generate default PSK for each channel
        m_channels[i].psk.resize(32);
        for (size_t j = 0; j < 32; j++) {
            m_channels[i].psk[j] = (uint8_t)esp_random();
        }
    }
    
    // Set default channel 0 as public/open
    m_channels[0].name = "Primary";
    std::fill(m_channels[0].psk.begin(), m_channels[0].psk.end(), 0);
}

MeshtasticDaemon::~MeshtasticDaemon() {
    shutdown();
}

os_error_t MeshtasticDaemon::initialize(HALManager* halManager) {
    if (m_initialized) {
        return OS_OK;
    }
    
    if (!halManager) {
        ESP_LOGE(TAG, "HAL Manager is required");
        return OS_ERROR_INVALID_PARAM;
    }
    
    m_halManager = halManager;
    
    ESP_LOGI(TAG, "Initializing Meshtastic daemon");
    
    // Generate our node ID from MAC address
    uint8_t mac[6];
    WiFi.macAddress(mac);
    m_myNodeId = ((uint32_t)mac[2] << 24) | ((uint32_t)mac[3] << 16) | 
                 ((uint32_t)mac[4] << 8) | (uint32_t)mac[5];
    
    ESP_LOGI(TAG, "Node ID: 0x%08lX", m_myNodeId);
    
    // Initialize our node information
    m_myNode.id = m_myNodeId;
    m_myNode.shortName = "TAB5";
    m_myNode.longName = "M5Stack Tab5";
    
    // Convert MAC to string
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    m_myNode.macaddr = macStr;
    
    m_myNode.hwModel = 42;  // Custom hardware model for Tab5
    m_myNode.latitude = 0.0;
    m_myNode.longitude = 0.0;
    m_myNode.altitude = 0;
    m_myNode.lastSeen = millis();
    m_myNode.batteryLevel = 100;
    m_myNode.voltage = 3.7f;
    m_myNode.uptimeSeconds = 0;
    m_myNode.viaMqtt = false;
    
    // Add ourselves to the nodes list
    m_nodes[m_myNodeId] = m_myNode;
    
    // Initialize LoRa
    os_error_t result = initializeLoRa();
    if (result != OS_OK) {
        ESP_LOGE(TAG, "Failed to initialize LoRa: %d", result);
        return result;
    }
    
    m_initialized = true;
    m_running = true;
    m_lastBeaconTime = millis();
    m_lastNeighborCleanup = millis();
    
    ESP_LOGI(TAG, "Meshtastic daemon initialized successfully");
    return OS_OK;
}

os_error_t MeshtasticDaemon::shutdown() {
    if (!m_initialized) {
        return OS_OK;
    }
    
    ESP_LOGI(TAG, "Shutting down Meshtastic daemon");
    
    m_running = false;
    m_rangeTestActive = false;
    
    // Clear all data structures
    m_nodes.clear();
    m_messageHistory.clear();
    m_messageAcks.clear();
    m_rangeTestResults.clear();
    
    m_initialized = false;
    ESP_LOGI(TAG, "Meshtastic daemon shutdown complete");
    
    return OS_OK;
}

os_error_t MeshtasticDaemon::update(uint32_t deltaTime) {
    if (!m_running) {
        return OS_ERROR_BUSY;
    }
    
    uint32_t currentTime = millis();
    
    // Update our own uptime
    m_myNode.uptimeSeconds = currentTime / 1000;
    m_nodes[m_myNodeId] = m_myNode;
    
    // Send periodic beacon
    if (currentTime - m_lastBeaconTime >= MESHTASTIC_BEACON_INTERVAL_MS) {
        sendBeacon();
        m_lastBeaconTime = currentTime;
    }
    
    // Cleanup old nodes and messages
    if (currentTime - m_lastNeighborCleanup >= MESHTASTIC_NEIGHBOR_TIMEOUT_MS / 2) {
        performHousekeeping();
        m_lastNeighborCleanup = currentTime;
    }
    
    // Handle range testing
    if (m_rangeTestActive && currentTime >= m_rangeTestNext && m_rangeTestCount > 0) {
        // Send range test message
        MeshtasticMessage msg;
        msg.id = generateMessageId();
        msg.fromNode = m_myNodeId;
        msg.toNode = m_rangeTestTarget;
        msg.channel = m_activeChannel;
        msg.type = MeshtasticMessageType::RANGE_TEST_CLIENT;
        msg.timestamp = currentTime;
        msg.hopLimit = MESHTASTIC_MAX_HOPS;
        msg.hopStart = MESHTASTIC_MAX_HOPS;
        msg.wantAck = true;
        msg.requestId = 0;
        msg.encrypted = false;
        
        // Range test payload: sequence number + timestamp
        msg.payload.resize(8);
        uint32_t seqNum = m_rangeTestResults.size() + 1;
        memcpy(msg.payload.data(), &seqNum, 4);
        memcpy(msg.payload.data() + 4, &currentTime, 4);
        
        if (transmitMessage(msg) == OS_OK) {
            ESP_LOGI(TAG, "Range test %lu sent to node 0x%08lX", seqNum, m_rangeTestTarget);
        }
        
        m_rangeTestNext = currentTime + (m_rangeTestInterval * 1000);
        m_rangeTestCount--;
        
        if (m_rangeTestCount == 0) {
            m_rangeTestActive = false;
            ESP_LOGI(TAG, "Range test completed");
        }
    }
    
    // TODO: Check for incoming LoRa packets
    // This would be implemented with actual LoRa hardware driver
    
    return OS_OK;
}

os_error_t MeshtasticDaemon::setNodeInfo(const std::string& shortName, const std::string& longName) {
    if (shortName.length() > 8 || longName.length() > 32) {
        return OS_ERROR_INVALID_PARAM;
    }
    
    m_myNode.shortName = shortName;
    m_myNode.longName = longName;
    m_nodes[m_myNodeId] = m_myNode;
    
    ESP_LOGI(TAG, "Node info updated: %s (%s)", longName.c_str(), shortName.c_str());
    return OS_OK;
}

os_error_t MeshtasticDaemon::configureLoRa(float frequency, uint8_t spreadingFactor, 
                                          float bandwidth, uint8_t codingRate, int8_t txPower) {
    // Validate parameters
    if (frequency < 137.0f || frequency > 1020.0f) {
        return OS_ERROR_INVALID_PARAM;
    }
    if (spreadingFactor < 7 || spreadingFactor > 12) {
        return OS_ERROR_INVALID_PARAM;
    }
    if (codingRate < 5 || codingRate > 8) {
        return OS_ERROR_INVALID_PARAM;
    }
    if (txPower < -3 || txPower > 20) {
        return OS_ERROR_INVALID_PARAM;
    }
    
    m_loraFrequency = frequency;
    m_loraSF = spreadingFactor;
    m_loraBW = bandwidth;
    m_loraCR = codingRate;
    m_loraPower = txPower;
    
    // TODO: Apply configuration to actual LoRa hardware
    ESP_LOGI(TAG, "LoRa configured: %.1fMHz, SF%d, BW%.0fkHz, CR4/%d, %ddBm",
             frequency, spreadingFactor, bandwidth, codingRate, txPower);
    
    return OS_OK;
}

os_error_t MeshtasticDaemon::configureChannel(uint8_t index, const std::string& name, 
                                             const std::vector<uint8_t>& psk) {
    if (index >= MESHTASTIC_MAX_CHANNELS) {
        return OS_ERROR_INVALID_PARAM;
    }
    if (psk.size() != 32) {
        return OS_ERROR_INVALID_PARAM;
    }
    
    m_channels[index].name = name;
    m_channels[index].psk = psk;
    
    ESP_LOGI(TAG, "Channel %d configured: %s", index, name.c_str());
    return OS_OK;
}

os_error_t MeshtasticDaemon::setActiveChannel(uint8_t index) {
    if (index >= MESHTASTIC_MAX_CHANNELS) {
        return OS_ERROR_INVALID_PARAM;
    }
    
    m_activeChannel = index;
    ESP_LOGI(TAG, "Active channel set to %d (%s)", index, m_channels[index].name.c_str());
    return OS_OK;
}

uint32_t MeshtasticDaemon::sendTextMessage(const std::string& message, uint32_t toNode, 
                                          uint8_t channel, bool wantAck) {
    if (channel >= MESHTASTIC_MAX_CHANNELS) {
        ESP_LOGE(TAG, "Invalid channel: %d", channel);
        return 0;
    }
    
    if (message.length() > 200) {  // Reasonable message size limit
        ESP_LOGE(TAG, "Message too long: %zu bytes", message.length());
        return 0;
    }
    
    MeshtasticMessage msg;
    msg.id = generateMessageId();
    msg.fromNode = m_myNodeId;
    msg.toNode = toNode;
    msg.channel = channel;
    msg.type = MeshtasticMessageType::TEXT_MESSAGE_UTF8;
    msg.timestamp = millis();
    msg.hopLimit = MESHTASTIC_MAX_HOPS;
    msg.hopStart = MESHTASTIC_MAX_HOPS;
    msg.wantAck = wantAck;
    msg.requestId = 0;
    msg.encrypted = true;
    
    // Copy message text to payload
    msg.payload.resize(message.length());
    memcpy(msg.payload.data(), message.c_str(), message.length());
    
    if (transmitMessage(msg) == OS_OK) {
        // Add to our message history
        m_messageHistory.push_back(msg);
        
        // Limit history size
        if (m_messageHistory.size() > 1000) {
            m_messageHistory.erase(m_messageHistory.begin(), 
                                   m_messageHistory.begin() + 100);
        }
        
        m_channels[channel].messageCount++;
        m_channels[channel].lastActivity = millis();
        
        ESP_LOGI(TAG, "Text message sent: ID=%lu, to=0x%08lX, len=%zu", 
                 msg.id, toNode, message.length());
        
        return msg.id;
    }
    
    return 0;
}

uint32_t MeshtasticDaemon::sendBinaryMessage(const std::vector<uint8_t>& data, uint32_t toNode,
                                            uint8_t channel, MeshtasticMessageType type) {
    if (channel >= MESHTASTIC_MAX_CHANNELS) {
        ESP_LOGE(TAG, "Invalid channel: %d", channel);
        return 0;
    }
    
    if (data.size() > 200) {  // Size limit for LoRa
        ESP_LOGE(TAG, "Binary message too large: %zu bytes", data.size());
        return 0;
    }
    
    MeshtasticMessage msg;
    msg.id = generateMessageId();
    msg.fromNode = m_myNodeId;
    msg.toNode = toNode;
    msg.channel = channel;
    msg.type = type;
    msg.timestamp = millis();
    msg.hopLimit = MESHTASTIC_MAX_HOPS;
    msg.hopStart = MESHTASTIC_MAX_HOPS;
    msg.wantAck = false;
    msg.requestId = 0;
    msg.encrypted = true;
    msg.payload = data;
    
    if (transmitMessage(msg) == OS_OK) {
        m_messageHistory.push_back(msg);
        m_channels[channel].messageCount++;
        m_channels[channel].lastActivity = millis();
        
        ESP_LOGI(TAG, "Binary message sent: ID=%lu, type=%d, len=%zu", 
                 msg.id, (int)type, data.size());
        
        return msg.id;
    }
    
    return 0;
}

uint32_t MeshtasticDaemon::sendPosition(double latitude, double longitude, int32_t altitude) {
    // Update our own position
    m_myNode.latitude = latitude;
    m_myNode.longitude = longitude;
    m_myNode.altitude = altitude;
    m_nodes[m_myNodeId] = m_myNode;
    
    // Create position message payload
    std::vector<uint8_t> payload(20);  // lat(4) + lon(4) + alt(4) + precision(1) + time(4) + flags(3)
    
    // Convert to fixed point (latitude/longitude * 1e7)
    int32_t latFixed = (int32_t)(latitude * 1e7);
    int32_t lonFixed = (int32_t)(longitude * 1e7);
    uint32_t timestamp = millis() / 1000;
    
    memcpy(payload.data(), &latFixed, 4);
    memcpy(payload.data() + 4, &lonFixed, 4);
    memcpy(payload.data() + 8, &altitude, 4);
    payload[12] = 4;  // Precision bits
    memcpy(payload.data() + 13, &timestamp, 4);
    payload[17] = 0;  // Flags
    payload[18] = 0;  // Ground speed
    payload[19] = 0;  // Ground track
    
    return sendBinaryMessage(payload, 0, m_activeChannel, MeshtasticMessageType::POSITION);
}

std::vector<MeshtasticNode> MeshtasticDaemon::getNodes() const {
    std::vector<MeshtasticNode> nodes;
    nodes.reserve(m_nodes.size());
    
    for (const auto& pair : m_nodes) {
        nodes.push_back(pair.second);
    }
    
    // Sort by last seen (most recent first)
    std::sort(nodes.begin(), nodes.end(), 
              [](const MeshtasticNode& a, const MeshtasticNode& b) {
                  return a.lastSeen > b.lastSeen;
              });
    
    return nodes;
}

const MeshtasticNode* MeshtasticDaemon::getNode(uint32_t nodeId) const {
    auto it = m_nodes.find(nodeId);
    return (it != m_nodes.end()) ? &it->second : nullptr;
}

os_error_t MeshtasticDaemon::startRangeTest(uint32_t nodeId, uint32_t interval, uint32_t count) {
    if (m_rangeTestActive) {
        ESP_LOGW(TAG, "Range test already active");
        return OS_ERROR_BUSY;
    }
    
    if (m_nodes.find(nodeId) == m_nodes.end()) {
        ESP_LOGE(TAG, "Target node 0x%08lX not found", nodeId);
        return OS_ERROR_NOT_FOUND;
    }
    
    m_rangeTestActive = true;
    m_rangeTestTarget = nodeId;
    m_rangeTestInterval = interval;
    m_rangeTestCount = count;
    m_rangeTestNext = millis() + 1000;  // Start in 1 second
    m_rangeTestResults.clear();
    
    ESP_LOGI(TAG, "Range test started: target=0x%08lX, interval=%lus, count=%lu", 
             nodeId, interval, count);
    
    return OS_OK;
}

void MeshtasticDaemon::stopRangeTest() {
    if (m_rangeTestActive) {
        m_rangeTestActive = false;
        ESP_LOGI(TAG, "Range test stopped");
    }
}

std::vector<MeshtasticRangeTest> MeshtasticDaemon::getRangeTestResults() const {
    return m_rangeTestResults;
}

void MeshtasticDaemon::getNetworkStats(uint32_t& totalMessages, float& messagesPerHour,
                                      int8_t& avgRSSI, uint8_t& avgSNR) const {
    totalMessages = m_totalMessagesRx + m_totalMessagesTx;
    
    uint32_t uptimeHours = (millis() / 1000) / 3600;
    if (uptimeHours > 0) {
        messagesPerHour = (float)totalMessages / uptimeHours;
    } else {
        messagesPerHour = 0.0f;
    }
    
    if (m_signalSamples > 0) {
        avgRSSI = (int8_t)(m_totalRSSI / m_signalSamples);
        avgSNR = (uint8_t)(m_totalSNR / m_signalSamples);
    } else {
        avgRSSI = -128;
        avgSNR = 0;
    }
}

bool MeshtasticDaemon::getChannelStats(uint8_t channel, uint32_t& messageCount, 
                                      uint32_t& lastActivity) const {
    if (channel >= MESHTASTIC_MAX_CHANNELS) {
        return false;
    }
    
    messageCount = m_channels[channel].messageCount;
    lastActivity = m_channels[channel].lastActivity;
    return true;
}

void MeshtasticDaemon::setNodeUpdateCallback(NodeUpdateCallback callback) {
    m_nodeCallback = callback;
}

void MeshtasticDaemon::setMessageCallback(MessageCallback callback) {
    m_messageCallback = callback;
}

void MeshtasticDaemon::setRangeTestCallback(RangeTestCallback callback) {
    m_rangeTestCallback = callback;
}

// === Private Methods ===

os_error_t MeshtasticDaemon::initializeLoRa() {
    // TODO: Initialize actual LoRa hardware
    // For now, just log the configuration
    ESP_LOGI(TAG, "LoRa initialized (stub): %.1fMHz, SF%d, BW%.0fkHz, %ddBm",
             m_loraFrequency, m_loraSF, m_loraBW, m_loraPower);
    
    return OS_OK;
}

os_error_t MeshtasticDaemon::transmitMessage(const MeshtasticMessage& msg) {
    // Encrypt payload if required
    std::vector<uint8_t> finalPayload = msg.payload;
    if (msg.encrypted && !m_channels[msg.channel].psk.empty()) {
        finalPayload = encryptPayload(msg.payload, m_channels[msg.channel]);
    }
    
    // TODO: Transmit over actual LoRa radio
    // For now, just log the transmission
    ESP_LOGD(TAG, "TX: ID=%lu, from=0x%08lX, to=0x%08lX, type=%d, len=%zu", 
             msg.id, msg.fromNode, msg.toNode, (int)msg.type, finalPayload.size());
    
    m_totalMessagesTx++;
    return OS_OK;
}

std::vector<uint8_t> MeshtasticDaemon::encryptPayload(const std::vector<uint8_t>& payload,
                                                     const MeshtasticChannel& channel) {
    // TODO: Implement AES-256-CTR encryption
    // For now, return payload as-is
    ESP_LOGV(TAG, "Encrypting payload with channel %d PSK", channel.index);
    return payload;
}

std::vector<uint8_t> MeshtasticDaemon::decryptPayload(const std::vector<uint8_t>& payload,
                                                     const MeshtasticChannel& channel) {
    // TODO: Implement AES-256-CTR decryption
    // For now, return payload as-is
    ESP_LOGV(TAG, "Decrypting payload with channel %d PSK", channel.index);
    return payload;
}

void MeshtasticDaemon::sendBeacon() {
    // Send node info broadcast
    std::vector<uint8_t> payload;
    
    // Simple node info packet: shortName(8) + longName(32) + hwModel(1) + battery(1)
    payload.resize(42);
    
    strncpy((char*)payload.data(), m_myNode.shortName.c_str(), 8);
    strncpy((char*)payload.data() + 8, m_myNode.longName.c_str(), 32);
    payload[40] = m_myNode.hwModel;
    payload[41] = m_myNode.batteryLevel;
    
    sendBinaryMessage(payload, 0, m_activeChannel, MeshtasticMessageType::NODEINFO);
    
    ESP_LOGD(TAG, "Beacon sent");
}

void MeshtasticDaemon::performHousekeeping() {
    uint32_t currentTime = millis();
    
    // Remove old nodes
    auto nodeIt = m_nodes.begin();
    while (nodeIt != m_nodes.end()) {
        if (nodeIt->second.id != m_myNodeId && 
            currentTime - nodeIt->second.lastSeen > MESHTASTIC_NEIGHBOR_TIMEOUT_MS) {
            ESP_LOGD(TAG, "Removing stale node: 0x%08lX", nodeIt->second.id);
            nodeIt = m_nodes.erase(nodeIt);
        } else {
            ++nodeIt;
        }
    }
    
    // Remove old messages
    auto msgIt = m_messageHistory.begin();
    while (msgIt != m_messageHistory.end()) {
        if (currentTime - msgIt->timestamp > MESHTASTIC_MESSAGE_TIMEOUT_MS) {
            msgIt = m_messageHistory.erase(msgIt);
        } else {
            ++msgIt;
        }
    }
    
    // Remove old ACK requests
    auto ackIt = m_messageAcks.begin();
    while (ackIt != m_messageAcks.end()) {
        if (currentTime - ackIt->second > MESHTASTIC_MESSAGE_TIMEOUT_MS) {
            ackIt = m_messageAcks.erase(ackIt);
        } else {
            ++ackIt;
        }
    }
    
    ESP_LOGV(TAG, "Housekeeping: %zu nodes, %zu messages", 
             m_nodes.size(), m_messageHistory.size());
}

uint32_t MeshtasticDaemon::generateMessageId() {
    uint32_t id = m_nextMessageId++;
    if (m_nextMessageId == 0) {
        m_nextMessageId = 1;  // Avoid ID 0
    }
    return id;
}

double MeshtasticDaemon::calculateDistance(double lat1, double lon1, double lat2, double lon2) {
    // Haversine formula
    const double R = 6371000.0;  // Earth's radius in meters
    
    double dLat = (lat2 - lat1) * M_PI / 180.0;
    double dLon = (lon2 - lon1) * M_PI / 180.0;
    
    double a = sin(dLat / 2) * sin(dLat / 2) +
               cos(lat1 * M_PI / 180.0) * cos(lat2 * M_PI / 180.0) *
               sin(dLon / 2) * sin(dLon / 2);
    
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));
    return R * c;
}