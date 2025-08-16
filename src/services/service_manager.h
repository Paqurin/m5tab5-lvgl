#ifndef SERVICE_MANAGER_H
#define SERVICE_MANAGER_H

#include "../system/os_config.h"
#include <string>
#include <map>
#include <memory>
#include <functional>

/**
 * @file service_manager.h
 * @brief Service Manager for M5Stack Tab5
 * 
 * Manages system services that run in the background
 * and provide functionality to applications.
 */

enum class ServiceState {
    STOPPED,
    STARTING,
    RUNNING,
    STOPPING,
    ERROR
};

class BaseService {
public:
    BaseService(const std::string& name) : m_name(name) {}
    virtual ~BaseService() = default;

    virtual os_error_t initialize() = 0;
    virtual os_error_t start() = 0;
    virtual os_error_t stop() = 0;
    virtual os_error_t shutdown() = 0;
    virtual os_error_t update(uint32_t deltaTime) = 0;

    const std::string& getName() const { return m_name; }
    ServiceState getState() const { return m_state; }
    bool isRunning() const { return m_state == ServiceState::RUNNING; }

protected:
    void setState(ServiceState state) { m_state = state; }

private:
    std::string m_name;
    ServiceState m_state = ServiceState::STOPPED;
};

typedef std::function<std::unique_ptr<BaseService>()> ServiceFactory;

class ServiceManager {
public:
    ServiceManager() = default;
    ~ServiceManager();

    /**
     * @brief Initialize service manager
     * @return OS_OK on success, error code on failure
     */
    os_error_t initialize();

    /**
     * @brief Shutdown service manager
     * @return OS_OK on success, error code on failure
     */
    os_error_t shutdown();

    /**
     * @brief Update all running services
     * @param deltaTime Time elapsed since last update in milliseconds
     * @return OS_OK on success, error code on failure
     */
    os_error_t update(uint32_t deltaTime);

    /**
     * @brief Register a service
     * @param name Service name
     * @param factory Service factory function
     * @param autoStart True to start service automatically
     * @return OS_OK on success, error code on failure
     */
    os_error_t registerService(const std::string& name, 
                              ServiceFactory factory,
                              bool autoStart = false);

    /**
     * @brief Start a service
     * @param name Service name
     * @return OS_OK on success, error code on failure
     */
    os_error_t startService(const std::string& name);

    /**
     * @brief Stop a service
     * @param name Service name
     * @return OS_OK on success, error code on failure
     */
    os_error_t stopService(const std::string& name);

    /**
     * @brief Get service by name
     * @param name Service name
     * @return Pointer to service or nullptr if not found
     */
    BaseService* getService(const std::string& name) const;

    /**
     * @brief Check if service is running
     * @param name Service name
     * @return true if running, false otherwise
     */
    bool isServiceRunning(const std::string& name) const;

    /**
     * @brief Get list of all services
     * @return Vector of service names
     */
    std::vector<std::string> getServiceNames() const;

    /**
     * @brief Get list of running services
     * @return Vector of running service names
     */
    std::vector<std::string> getRunningServices() const;

private:
    std::map<std::string, ServiceFactory> m_serviceFactories;
    std::map<std::string, std::unique_ptr<BaseService>> m_services;
    std::vector<std::string> m_autoStartServices;
    bool m_initialized = false;
};

#endif // SERVICE_MANAGER_H