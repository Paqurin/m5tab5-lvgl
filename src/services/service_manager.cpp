#include "service_manager.h"
#include "../system/os_manager.h"
#include <esp_log.h>

static const char* TAG = "ServiceManager";

ServiceManager::~ServiceManager() {
    shutdown();
}

os_error_t ServiceManager::initialize() {
    if (m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Initializing Service Manager");

    // TODO: Register built-in services here
    // Examples: WiFi service, file service, etc.

    m_initialized = true;
    ESP_LOGI(TAG, "Service Manager initialized");

    return OS_OK;
}

os_error_t ServiceManager::shutdown() {
    if (!m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Shutting down Service Manager");

    // Stop all running services
    auto serviceNames = getRunningServices();
    for (const auto& name : serviceNames) {
        stopService(name);
    }

    // Clear all services
    m_services.clear();
    m_serviceFactories.clear();
    m_autoStartServices.clear();

    m_initialized = false;
    ESP_LOGI(TAG, "Service Manager shutdown complete");

    return OS_OK;
}

os_error_t ServiceManager::update(uint32_t deltaTime) {
    if (!m_initialized) {
        return OS_ERROR_GENERIC;
    }

    // Update all running services
    for (auto& [name, service] : m_services) {
        if (service && service->isRunning()) {
            try {
                service->update(deltaTime);
            } catch (...) {
                ESP_LOGE(TAG, "Exception in service '%s' update", name.c_str());
                PUBLISH_EVENT(EVENT_SERVICE_ERROR, (void*)name.c_str(), name.length());
            }
        }
    }

    return OS_OK;
}

os_error_t ServiceManager::registerService(const std::string& name, 
                                          ServiceFactory factory,
                                          bool autoStart) {
    if (!m_initialized || name.empty() || !factory) {
        return OS_ERROR_INVALID_PARAM;
    }

    if (m_serviceFactories.find(name) != m_serviceFactories.end()) {
        ESP_LOGW(TAG, "Service '%s' already registered", name.c_str());
        return OS_ERROR_GENERIC;
    }

    m_serviceFactories[name] = factory;
    
    if (autoStart) {
        m_autoStartServices.push_back(name);
        startService(name);
    }

    ESP_LOGI(TAG, "Registered service '%s' (auto-start: %s)", 
             name.c_str(), autoStart ? "yes" : "no");

    return OS_OK;
}

os_error_t ServiceManager::startService(const std::string& name) {
    if (!m_initialized || name.empty()) {
        return OS_ERROR_INVALID_PARAM;
    }

    // Check if already running
    if (isServiceRunning(name)) {
        ESP_LOGW(TAG, "Service '%s' already running", name.c_str());
        return OS_OK;
    }

    // Find factory
    auto factoryIt = m_serviceFactories.find(name);
    if (factoryIt == m_serviceFactories.end()) {
        ESP_LOGE(TAG, "Service '%s' not registered", name.c_str());
        return OS_ERROR_NOT_FOUND;
    }

    // Create service instance
    try {
        auto service = factoryIt->second();
        if (!service) {
            ESP_LOGE(TAG, "Failed to create service '%s'", name.c_str());
            return OS_ERROR_GENERIC;
        }

        // Initialize service
        os_error_t result = service->initialize();
        if (result != OS_OK) {
            ESP_LOGE(TAG, "Failed to initialize service '%s': %d", name.c_str(), result);
            return result;
        }

        // Start service
        result = service->start();
        if (result != OS_OK) {
            ESP_LOGE(TAG, "Failed to start service '%s': %d", name.c_str(), result);
            service->shutdown();
            return result;
        }

        // Add to running services
        m_services[name] = std::move(service);

        ESP_LOGI(TAG, "Started service '%s'", name.c_str());
        PUBLISH_EVENT(EVENT_SERVICE_START, (void*)name.c_str(), name.length());

        return OS_OK;

    } catch (...) {
        ESP_LOGE(TAG, "Exception creating service '%s'", name.c_str());
        return OS_ERROR_GENERIC;
    }
}

os_error_t ServiceManager::stopService(const std::string& name) {
    if (!m_initialized || name.empty()) {
        return OS_ERROR_INVALID_PARAM;
    }

    auto it = m_services.find(name);
    if (it == m_services.end()) {
        return OS_ERROR_NOT_FOUND;
    }

    auto& service = it->second;
    if (service) {
        service->stop();
        service->shutdown();
    }

    m_services.erase(it);

    ESP_LOGI(TAG, "Stopped service '%s'", name.c_str());
    PUBLISH_EVENT(EVENT_SERVICE_STOP, (void*)name.c_str(), name.length());

    return OS_OK;
}

BaseService* ServiceManager::getService(const std::string& name) const {
    auto it = m_services.find(name);
    return (it != m_services.end()) ? it->second.get() : nullptr;
}

bool ServiceManager::isServiceRunning(const std::string& name) const {
    BaseService* service = getService(name);
    return service && service->isRunning();
}

std::vector<std::string> ServiceManager::getServiceNames() const {
    std::vector<std::string> names;
    for (const auto& [name, factory] : m_serviceFactories) {
        names.push_back(name);
    }
    return names;
}

std::vector<std::string> ServiceManager::getRunningServices() const {
    std::vector<std::string> names;
    for (const auto& [name, service] : m_services) {
        if (service && service->isRunning()) {
            names.push_back(name);
        }
    }
    return names;
}