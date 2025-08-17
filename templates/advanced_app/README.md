# Advanced App Template

This template demonstrates advanced patterns and features for M5Stack Tab5 OS app development.

## Features Demonstrated

### 1. Background Task Management
- Multiple concurrent tasks with different priorities
- Inter-task communication using FreeRTOS queues
- Proper task lifecycle management
- Task synchronization and coordination

### 2. Service Architecture
- Modular service design pattern
- Data service for persistent storage
- Network service for HTTP operations
- Service initialization and cleanup

### 3. Advanced UI Patterns
- Multi-component UI with toolbar and status bar
- Dynamic content updates
- Progress indicators and status displays
- Modal dialogs and settings screens

### 4. Event System Integration
- System event subscriptions
- Custom event publishing
- Event-driven architecture
- Asynchronous event handling

### 5. Data Management
- Configuration persistence
- App state management
- Data serialization/deserialization
- Error handling and recovery

### 6. Network Operations
- HTTP client integration
- Asynchronous network requests
- Connection state management
- Error handling and retries

### 7. Memory Management
- Dynamic memory allocation
- Memory usage tracking
- Resource cleanup
- Memory leak prevention

### 8. Performance Monitoring
- Operation statistics
- Performance metrics
- Memory usage monitoring
- Error rate tracking

## Code Structure

```
advanced_app/
├── advanced_app.h              # Main app header
├── advanced_app.cpp            # Main app implementation
├── services/
│   ├── data_service.cpp        # Data persistence service
│   └── network_service.cpp     # Network operations service
├── ui/
│   ├── toolbar.cpp             # Toolbar component
│   ├── content_area.cpp        # Main content area
│   └── status_bar.cpp          # Status bar component
├── config/
│   └── app_config.json         # Default configuration
├── assets/
│   ├── icon.png                # App icon
│   └── images/                 # UI images
└── manifest.json               # App manifest

```

## Usage

### 1. Background Tasks

The app creates multiple background tasks for different purposes:

```cpp
// Background processing task
static void backgroundTask(void* parameter) {
    AdvancedApp* app = static_cast<AdvancedApp*>(parameter);
    while (app->isTaskRunning()) {
        app->processBackgroundWork();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    vTaskDelete(nullptr);
}
```

### 2. Service Integration

Services are initialized during app startup:

```cpp
os_error_t AdvancedApp::initializeServices() {
    // Initialize data service
    m_dataService = std::make_unique<DataService>();
    if (m_dataService->initialize() != OS_OK) {
        return OS_ERROR_GENERIC;
    }
    
    // Initialize network service
    m_networkService = std::make_unique<NetworkService>();
    if (m_networkService->initialize() != OS_OK) {
        return OS_ERROR_GENERIC;
    }
    
    return OS_OK;
}
```

### 3. Event Handling

The app subscribes to various system events:

```cpp
// Subscribe to network events
SUBSCRIBE_EVENT(EVENT_NETWORK_CONNECTED, [this](const EventData& event) {
    handleNetworkEvent(event);
});

// Subscribe to data update events
SUBSCRIBE_EVENT(EVENT_DATA_UPDATED, [this](const EventData& event) {
    handleDataUpdate(event);
});
```

### 4. UI Components

Complex UI with multiple components:

```cpp
void AdvancedApp::createUI(lv_obj_t* parent) {
    // Create main container
    m_uiContainer = lv_obj_create(parent);
    
    // Create toolbar
    createToolbar();
    
    // Create main content area
    createMainContent();
    
    // Create status bar
    createStatusBar();
}
```

## Configuration

The app uses a JSON configuration file:

```json
{
    "networking": {
        "enabled": true,
        "server_url": "https://api.example.com",
        "timeout": 10000,
        "retries": 3
    },
    "ui": {
        "update_interval": 1000,
        "show_debug_info": false,
        "theme": "dark"
    },
    "data": {
        "auto_save": true,
        "max_items": 100,
        "cache_size": "1MB"
    }
}
```

## Building

Add to your project's app integration:

```cpp
// Register the advanced app
result = appManager.registerApp("advanced_app", createAdvancedApp);
```

## Memory Usage

This advanced template uses more memory due to additional features:

- Base app: ~16KB
- Background tasks: ~24KB (3 tasks × 8KB stacks)
- Services: ~8KB
- UI components: ~16KB
- Data buffers: ~32KB
- **Total estimated**: ~96KB

## Performance Considerations

1. **Task Priorities**: Different tasks have appropriate priorities
2. **Memory Monitoring**: Built-in memory usage tracking
3. **Resource Cleanup**: Proper cleanup in all error paths
4. **Event Throttling**: Prevents event flooding
5. **Background Processing**: Non-blocking operations

## Testing

The template includes test infrastructure:

```cpp
// Unit test example
void testDataService() {
    DataService service;
    TEST_ASSERT_EQUAL(OS_OK, service.initialize());
    
    std::vector<std::string> data = {"test1", "test2"};
    TEST_ASSERT_EQUAL(OS_OK, service.saveData(data));
    
    std::vector<std::string> loaded;
    TEST_ASSERT_EQUAL(OS_OK, service.loadData(loaded));
    TEST_ASSERT_EQUAL(2, loaded.size());
    
    service.shutdown();
}
```

## Customization

### Adding New Services

1. Create service class inheriting from base service interface
2. Implement initialize(), shutdown(), and service-specific methods
3. Add to service initialization in `initializeServices()`
4. Register event handlers if needed

### Adding Background Tasks

1. Create static task function
2. Add task handle to class members
3. Start task in `initializeBackgroundTasks()`
4. Stop task in `shutdownBackgroundTasks()`

### Extending UI

1. Create new UI component methods
2. Add UI elements to class members
3. Call component creation in `createUI()`
4. Add event callbacks as needed

## Best Practices Demonstrated

1. **RAII**: Resource acquisition is initialization
2. **Exception Safety**: Proper cleanup in all paths
3. **Memory Management**: Tracking and limits
4. **Error Handling**: Comprehensive error checking
5. **Logging**: Structured logging throughout
6. **Configuration**: External configuration management
7. **Testing**: Built-in test support
8. **Documentation**: Comprehensive code documentation

This template serves as a comprehensive example of advanced M5Stack Tab5 OS app development patterns and can be adapted for complex real-world applications.