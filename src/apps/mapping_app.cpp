#include "mapping_app.h"
#include "../system/memory_manager.h"
#include "../ui/theme_manager.h"
#include <esp_log.h>
#include <cstring>
#include <algorithm>
#include <sys/stat.h>
#include <dirent.h>
#include <cmath>

// Web Mercator projection math constants
static constexpr double TILE_SIZE = 256.0;
static constexpr double INITIAL_RESOLUTION = 2 * M_PI * 6378137.0 / TILE_SIZE;

MappingApp::MappingApp() 
    : BaseApp("mapping", "OpenMaps", "1.0") {
    setDescription("Offline mapping application with GPS tracking and POI management");
    setAuthor("M5Stack Tab5 Team");
    setPriority(AppPriority::APP_NORMAL);
    
    // Initialize viewport with default values (centered on world)
    m_viewport.center = MapCoordinate(0.0, 0.0);
    m_viewport.zoomLevel = 2;
    m_viewport.scale = 1.0;
    m_viewport.pixelOffsetX = 0;
    m_viewport.pixelOffsetY = 0;
    m_viewport.rotation = 0.0;
    
    // Initialize tile cache
    m_tileCache.maxCacheSize = TILE_CACHE_SIZE_MB * 1024 * 1024;
    m_tileCache.currentCacheSize = 0;
    m_tileCache.hitCount = 0;
    m_tileCache.missCount = 0;
    
    // Initialize GPS location as invalid
    m_currentGPSLocation.isValid = false;
    
    ESP_LOGI(TAG, "MappingApp initialized");
}

MappingApp::~MappingApp() {
    shutdown();
}

os_error_t MappingApp::initialize() {
    if (m_initialized) {
        return OS_OK;
    }
    
    ESP_LOGI(TAG, "Initializing mapping application");
    
    // Initialize storage service if not provided
    if (!m_storageService) {
        m_storageService = new StorageService();
        m_ownStorageService = true;
        
        os_error_t result = m_storageService->initialize();
        if (result != OS_OK) {
            ESP_LOGE(TAG, "Failed to initialize storage service: %d", result);
            return result;
        }
    }
    
    // Reserve memory for tile cache and POI data
    m_tileCache.tiles.reserve(1000); // Reserve space for 1000 tiles
    m_pointsOfInterest.reserve(10000); // Reserve space for 10k POIs
    m_poiMarkers.reserve(100); // Reserve space for visible POI markers
    
    // Set memory usage estimate
    setMemoryUsage(TILE_CACHE_SIZE_MB * 1024 * 1024 + 10 * 1024 * 1024); // Cache + 10MB for app data
    
    m_initialized = true;
    ESP_LOGI(TAG, "Mapping application initialized successfully");
    
    return OS_OK;
}

os_error_t MappingApp::update(uint32_t deltaTime) {
    if (getState() != AppState::RUNNING) {
        return OS_OK;
    }
    
    // Update storage service
    if (m_storageService) {
        m_storageService->update(deltaTime);
    }
    
    // Update tile cache management
    static uint32_t lastCacheUpdate = 0;
    lastCacheUpdate += deltaTime;
    if (lastCacheUpdate >= 5000) { // Update cache every 5 seconds
        manageTileCache();
        lastCacheUpdate = 0;
    }
    
    // Update GPS location (if enabled)
    if (m_gpsEnabled) {
        // In real implementation, this would read from GPS HAL
        // For now, we'll just update the GPS status display
        if (m_gpsStatusLabel) {
            if (m_currentGPSLocation.isValid) {
                lv_label_set_text_fmt(m_gpsStatusLabel, "GPS: %.6f, %.6f", 
                                     m_currentGPSLocation.position.latitude,
                                     m_currentGPSLocation.position.longitude);
            } else {
                lv_label_set_text(m_gpsStatusLabel, "GPS: No Fix");
            }
        }
    }
    
    // Update coordinate display
    if (m_coordinateLabel) {
        lv_label_set_text_fmt(m_coordinateLabel, "%.6f, %.6f", 
                             m_viewport.center.latitude, m_viewport.center.longitude);
    }
    
    // Update zoom level display
    if (m_zoomLabel) {
        lv_label_set_text_fmt(m_zoomLabel, "Zoom: %d", m_viewport.zoomLevel);
    }
    
    // Update scale display
    if (m_scaleLabel) {
        double scale = INITIAL_RESOLUTION / (1 << m_viewport.zoomLevel);
        if (scale >= 1000) {
            lv_label_set_text_fmt(m_scaleLabel, "1:%.0fk", scale / 1000);
        } else {
            lv_label_set_text_fmt(m_scaleLabel, "1:%.0f", scale);
        }
    }
    
    m_mapUpdates++;
    return OS_OK;
}

os_error_t MappingApp::shutdown() {
    if (!m_initialized) {
        return OS_OK;
    }
    
    ESP_LOGI(TAG, "Shutting down mapping application");
    
    // Clear tile cache and free memory
    for (auto& tile : m_tileCache.tiles) {
        if (tile && tile->imageData) {
            // Free LVGL image data
            lv_mem_free((void*)tile->imageData);
            tile->imageData = nullptr;
        }
    }
    m_tileCache.tiles.clear();
    
    // Clear POI markers
    for (auto marker : m_poiMarkers) {
        if (marker) {
            lv_obj_del(marker);
        }
    }
    m_poiMarkers.clear();
    m_pointsOfInterest.clear();
    
    // Cleanup storage service
    if (m_storageService && m_ownStorageService) {
        m_storageService->shutdown();
        delete m_storageService;
        m_storageService = nullptr;
        m_ownStorageService = false;
    }
    
    m_mapDataLoaded = false;
    m_initialized = false;
    
    ESP_LOGI(TAG, "Mapping application shutdown complete");
    return OS_OK;
}

os_error_t MappingApp::createUI(lv_obj_t* parent) {
    if (!parent) {
        ESP_LOGE(TAG, "Parent object is null");
        return OS_ERROR_INVALID_PARAM;
    }
    
    ESP_LOGI(TAG, "Creating mapping UI");
    
    // Create main container
    m_mainContainer = lv_obj_create(parent);
    if (!m_mainContainer) {
        ESP_LOGE(TAG, "Failed to create main container");
        return OS_ERROR_NO_MEMORY;
    }
    
    lv_obj_set_size(m_mainContainer, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(m_mainContainer, 0, 0);
    lv_obj_set_style_border_width(m_mainContainer, 0, 0);
    lv_obj_clear_flag(m_mainContainer, LV_OBJ_FLAG_SCROLLABLE);
    
    // Create map view UI
    createMapViewUI();
    
    // Create navigation controls
    createNavigationControlsUI();
    
    // Create information panel
    createInformationPanelUI();
    
    // Store reference to UI container
    m_uiContainer = m_mainContainer;
    
    ESP_LOGI(TAG, "Mapping UI created successfully");
    return OS_OK;
}

os_error_t MappingApp::destroyUI() {
    ESP_LOGI(TAG, "Destroying mapping UI");
    
    // Clear POI markers
    for (auto marker : m_poiMarkers) {
        if (marker) {
            lv_obj_del(marker);
        }
    }
    m_poiMarkers.clear();
    
    // Delete main container (this will delete all child objects)
    if (m_mainContainer) {
        lv_obj_del(m_mainContainer);
        m_mainContainer = nullptr;
    }
    
    // Reset UI element references
    m_mapCanvas = nullptr;
    m_controlPanel = nullptr;
    m_infoPanel = nullptr;
    m_zoomInButton = nullptr;
    m_zoomOutButton = nullptr;
    m_centerGPSButton = nullptr;
    m_layerButton = nullptr;
    m_searchButton = nullptr;
    m_menuButton = nullptr;
    m_poiButton = nullptr;
    m_coordinateLabel = nullptr;
    m_zoomLabel = nullptr;
    m_gpsStatusLabel = nullptr;
    m_scaleLabel = nullptr;
    m_gpsMarker = nullptr;
    m_routeLine = nullptr;
    
    m_uiContainer = nullptr;
    
    ESP_LOGI(TAG, "Mapping UI destroyed");
    return OS_OK;
}

os_error_t MappingApp::handleEvent(uint32_t eventType, void* eventData, size_t dataSize) {
    // Handle application-specific events
    // This could include GPS updates, touch gestures, etc.
    return OS_OK;
}

void MappingApp::createMapViewUI() {
    // Create map canvas (main drawing area)
    m_mapCanvas = lv_obj_create(m_mainContainer);
    lv_obj_set_size(m_mapCanvas, LV_PCT(100), LV_PCT(85));
    lv_obj_align(m_mapCanvas, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_pad_all(m_mapCanvas, 0, 0);
    lv_obj_set_style_border_width(m_mapCanvas, 1, 0);
    lv_obj_set_style_bg_color(m_mapCanvas, lv_color_hex(0x87CEEB), 0); // Sky blue background
    
    // Enable touch events for pan and zoom
    lv_obj_add_flag(m_mapCanvas, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(m_mapCanvas, mapCanvasCallback, LV_EVENT_ALL, this);
    
    // Create GPS marker (initially hidden)
    m_gpsMarker = lv_obj_create(m_mapCanvas);
    lv_obj_set_size(m_gpsMarker, 20, 20);
    lv_obj_set_style_radius(m_gpsMarker, 10, 0);
    lv_obj_set_style_bg_color(m_gpsMarker, lv_color_hex(0xFF0000), 0); // Red marker
    lv_obj_set_style_border_width(m_gpsMarker, 2, 0);
    lv_obj_set_style_border_color(m_gpsMarker, lv_color_hex(0xFFFFFF), 0);
    lv_obj_add_flag(m_gpsMarker, LV_OBJ_FLAG_HIDDEN);
    
    ESP_LOGI(TAG, "Map view UI created");
}

void MappingApp::createNavigationControlsUI() {
    // Create control panel
    m_controlPanel = lv_obj_create(m_mainContainer);
    lv_obj_set_size(m_controlPanel, LV_PCT(100), LV_PCT(15));
    lv_obj_align(m_controlPanel, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_pad_all(m_controlPanel, 5, 0);
    lv_obj_set_style_bg_color(m_controlPanel, lv_color_hex(0x2C2C2C), 0);
    lv_obj_set_flex_flow(m_controlPanel, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(m_controlPanel, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Create zoom in button
    m_zoomInButton = lv_btn_create(m_controlPanel);
    lv_obj_set_size(m_zoomInButton, 60, 40);
    lv_obj_add_event_cb(m_zoomInButton, zoomInButtonCallback, LV_EVENT_CLICKED, this);
    
    lv_obj_t* zoomInLabel = lv_label_create(m_zoomInButton);
    lv_label_set_text(zoomInLabel, LV_SYMBOL_PLUS);
    lv_obj_center(zoomInLabel);
    
    // Create zoom out button
    m_zoomOutButton = lv_btn_create(m_controlPanel);
    lv_obj_set_size(m_zoomOutButton, 60, 40);
    lv_obj_add_event_cb(m_zoomOutButton, zoomOutButtonCallback, LV_EVENT_CLICKED, this);
    
    lv_obj_t* zoomOutLabel = lv_label_create(m_zoomOutButton);
    lv_label_set_text(zoomOutLabel, LV_SYMBOL_MINUS);
    lv_obj_center(zoomOutLabel);
    
    // Create center GPS button
    m_centerGPSButton = lv_btn_create(m_controlPanel);
    lv_obj_set_size(m_centerGPSButton, 60, 40);
    lv_obj_add_event_cb(m_centerGPSButton, centerGPSButtonCallback, LV_EVENT_CLICKED, this);
    
    lv_obj_t* gpsLabel = lv_label_create(m_centerGPSButton);
    lv_label_set_text(gpsLabel, LV_SYMBOL_GPS);
    lv_obj_center(gpsLabel);
    
    // Create layer button
    m_layerButton = lv_btn_create(m_controlPanel);
    lv_obj_set_size(m_layerButton, 60, 40);
    lv_obj_add_event_cb(m_layerButton, layerButtonCallback, LV_EVENT_CLICKED, this);
    
    lv_obj_t* layerLabel = lv_label_create(m_layerButton);
    lv_label_set_text(layerLabel, LV_SYMBOL_LIST);
    lv_obj_center(layerLabel);
    
    // Create search button
    m_searchButton = lv_btn_create(m_controlPanel);
    lv_obj_set_size(m_searchButton, 60, 40);
    lv_obj_add_event_cb(m_searchButton, searchButtonCallback, LV_EVENT_CLICKED, this);
    
    lv_obj_t* searchLabel = lv_label_create(m_searchButton);
    lv_label_set_text(searchLabel, LV_SYMBOL_EYE_OPEN);
    lv_obj_center(searchLabel);
    
    // Create menu button
    m_menuButton = lv_btn_create(m_controlPanel);
    lv_obj_set_size(m_menuButton, 60, 40);
    lv_obj_add_event_cb(m_menuButton, menuButtonCallback, LV_EVENT_CLICKED, this);
    
    lv_obj_t* menuLabel = lv_label_create(m_menuButton);
    lv_label_set_text(menuLabel, LV_SYMBOL_SETTINGS);
    lv_obj_center(menuLabel);
    
    ESP_LOGI(TAG, "Navigation controls UI created");
}

void MappingApp::createInformationPanelUI() {
    // Create information panel overlay
    m_infoPanel = lv_obj_create(m_mapCanvas);
    lv_obj_set_size(m_infoPanel, 300, 80);
    lv_obj_align(m_infoPanel, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_set_style_bg_opa(m_infoPanel, LV_OPA_80, 0);
    lv_obj_set_style_bg_color(m_infoPanel, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(m_infoPanel, 1, 0);
    lv_obj_set_style_border_color(m_infoPanel, lv_color_hex(0x404040), 0);
    lv_obj_set_style_pad_all(m_infoPanel, 8, 0);
    
    // Create coordinate label
    m_coordinateLabel = lv_label_create(m_infoPanel);
    lv_label_set_text(m_coordinateLabel, "0.000000, 0.000000");
    lv_obj_set_style_text_color(m_coordinateLabel, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(m_coordinateLabel, LV_ALIGN_TOP_LEFT, 0, 0);
    
    // Create zoom label
    m_zoomLabel = lv_label_create(m_infoPanel);
    lv_label_set_text(m_zoomLabel, "Zoom: 2");
    lv_obj_set_style_text_color(m_zoomLabel, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(m_zoomLabel, LV_ALIGN_TOP_LEFT, 0, 20);
    
    // Create GPS status label
    m_gpsStatusLabel = lv_label_create(m_infoPanel);
    lv_label_set_text(m_gpsStatusLabel, "GPS: Disabled");
    lv_obj_set_style_text_color(m_gpsStatusLabel, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(m_gpsStatusLabel, LV_ALIGN_TOP_LEFT, 0, 40);
    
    // Create scale label
    m_scaleLabel = lv_label_create(m_infoPanel);
    lv_label_set_text(m_scaleLabel, "1:295M");
    lv_obj_set_style_text_color(m_scaleLabel, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(m_scaleLabel, LV_ALIGN_TOP_RIGHT, 0, 0);
    
    ESP_LOGI(TAG, "Information panel UI created");
}

os_error_t MappingApp::loadMapData(const std::string& mapDataPath) {
    if (mapDataPath.empty()) {
        ESP_LOGE(TAG, "Map data path is empty");
        return OS_ERROR_INVALID_PARAM;
    }
    
    ESP_LOGI(TAG, "Loading map data from: %s", mapDataPath.c_str());
    
    // Check if map data directory exists
    struct stat st;
    if (stat(mapDataPath.c_str(), &st) != 0 || !S_ISDIR(st.st_mode)) {
        ESP_LOGE(TAG, "Map data directory does not exist: %s", mapDataPath.c_str());
        return OS_ERROR_NOT_FOUND;
    }
    
    m_mapDataPath = mapDataPath;
    
    // Determine map data source type
    std::string mbtilesPath = mapDataPath + "/tiles.mbtiles";
    if (stat(mbtilesPath.c_str(), &st) == 0) {
        ESP_LOGI(TAG, "Detected MBTiles format");
        m_mapDataSource = MapDataSource::MBTiles;
        return parseMBTiles(mbtilesPath);
    }
    
    // Check for standard tile directory structure (zoom/x/y.png)
    std::string tilesPath = mapDataPath + "/tiles";
    if (stat(tilesPath.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
        ESP_LOGI(TAG, "Detected offline tiles format");
        m_mapDataSource = MapDataSource::OFFLINE_TILES;
    }
    
    // Load POI data if available
    std::string poiPath = mapDataPath + "/pois.json";
    if (stat(poiPath.c_str(), &st) == 0) {
        loadPOIData(poiPath);
    }
    
    m_mapDataLoaded = true;
    ESP_LOGI(TAG, "Map data loaded successfully");
    
    return OS_OK;
}

os_error_t MappingApp::setMapCenter(const MapCoordinate& coordinate) {
    if (!coordinate.isValid()) {
        ESP_LOGE(TAG, "Invalid coordinate: lat=%.6f, lon=%.6f", coordinate.latitude, coordinate.longitude);
        return OS_ERROR_INVALID_PARAM;
    }
    
    m_viewport.center = coordinate;
    updateViewport();
    
    ESP_LOGI(TAG, "Map center set to: %.6f, %.6f", coordinate.latitude, coordinate.longitude);
    return OS_OK;
}

os_error_t MappingApp::setZoomLevel(uint8_t zoomLevel) {
    if (zoomLevel < MIN_ZOOM_LEVEL || zoomLevel > MAX_ZOOM_LEVEL) {
        ESP_LOGE(TAG, "Invalid zoom level: %d", zoomLevel);
        return OS_ERROR_INVALID_PARAM;
    }
    
    m_viewport.zoomLevel = zoomLevel;
    updateViewport();
    
    ESP_LOGI(TAG, "Zoom level set to: %d", zoomLevel);
    return OS_OK;
}

os_error_t MappingApp::panMap(int32_t deltaX, int32_t deltaY) {
    // Convert pixel delta to coordinate delta
    double scale = INITIAL_RESOLUTION / (1 << m_viewport.zoomLevel);
    double latDelta = (deltaY * scale) / (M_PI * 6378137.0) * 180.0;
    double lonDelta = (deltaX * scale) / (M_PI * 6378137.0 * std::cos(m_viewport.center.latitude * DEGREES_TO_RADIANS)) * 180.0;
    
    MapCoordinate newCenter(m_viewport.center.latitude - latDelta, m_viewport.center.longitude - lonDelta);
    
    if (newCenter.isValid()) {
        m_viewport.center = newCenter;
        updateViewport();
        return OS_OK;
    }
    
    return OS_ERROR_INVALID_PARAM;
}

os_error_t MappingApp::zoomIn() {
    if (m_viewport.zoomLevel < MAX_ZOOM_LEVEL) {
        return setZoomLevel(m_viewport.zoomLevel + 1);
    }
    return OS_ERROR_INVALID_PARAM;
}

os_error_t MappingApp::zoomOut() {
    if (m_viewport.zoomLevel > MIN_ZOOM_LEVEL) {
        return setZoomLevel(m_viewport.zoomLevel - 1);
    }
    return OS_ERROR_INVALID_PARAM;
}

os_error_t MappingApp::addPOI(const POIData& poi) {
    if (!poi.location.isValid()) {
        ESP_LOGE(TAG, "Invalid POI location");
        return OS_ERROR_INVALID_PARAM;
    }
    
    m_pointsOfInterest.push_back(poi);
    ESP_LOGI(TAG, "POI added: %s at %.6f, %.6f", poi.name.c_str(), poi.location.latitude, poi.location.longitude);
    
    return OS_OK;
}

os_error_t MappingApp::removePOI(const std::string& name) {
    auto it = std::find_if(m_pointsOfInterest.begin(), m_pointsOfInterest.end(),
                          [&name](const POIData& poi) { return poi.name == name; });
    
    if (it != m_pointsOfInterest.end()) {
        m_pointsOfInterest.erase(it);
        ESP_LOGI(TAG, "POI removed: %s", name.c_str());
        return OS_OK;
    }
    
    ESP_LOGW(TAG, "POI not found: %s", name.c_str());
    return OS_ERROR_NOT_FOUND;
}

void MappingApp::setPOILayerVisible(bool visible) {
    m_poiLayerVisible = visible;
    
    // Update visibility of POI markers
    for (auto marker : m_poiMarkers) {
        if (marker) {
            if (visible) {
                lv_obj_clear_flag(marker, LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_add_flag(marker, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
    
    ESP_LOGI(TAG, "POI layer visibility: %s", visible ? "enabled" : "disabled");
}

os_error_t MappingApp::enableGPSTracking(bool enable) {
    m_gpsEnabled = enable;
    
    if (enable) {
        ESP_LOGI(TAG, "GPS tracking enabled");
        // In real implementation, initialize GPS HAL here
    } else {
        ESP_LOGI(TAG, "GPS tracking disabled");
        // Hide GPS marker
        if (m_gpsMarker) {
            lv_obj_add_flag(m_gpsMarker, LV_OBJ_FLAG_HIDDEN);
        }
        m_currentGPSLocation.isValid = false;
    }
    
    return OS_OK;
}

os_error_t MappingApp::updateGPSLocation(const GPSLocation& location) {
    if (!location.isValid) {
        return OS_ERROR_INVALID_PARAM;
    }
    
    m_currentGPSLocation = location;
    
    // Update GPS marker position if visible
    if (m_gpsEnabled && m_gpsMarker) {
        ScreenCoordinate screenPos = mapToScreen(location.position);
        lv_obj_set_pos(m_gpsMarker, screenPos.x - 10, screenPos.y - 10);
        lv_obj_clear_flag(m_gpsMarker, LV_OBJ_FLAG_HIDDEN);
    }
    
    // Auto-center map on GPS location if in follow mode
    if (m_navigationMode == NavigationMode::GPS_FOLLOW) {
        setMapCenter(location.position);
    }
    
    return OS_OK;
}

void MappingApp::setNavigationMode(NavigationMode mode) {
    m_navigationMode = mode;
    ESP_LOGI(TAG, "Navigation mode set to: %d", static_cast<int>(mode));
}

ScreenCoordinate MappingApp::mapToScreen(const MapCoordinate& mapCoord) const {
    // Web Mercator projection
    double scale = TILE_SIZE * (1 << m_viewport.zoomLevel);
    double worldX = (mapCoord.longitude + 180.0) / 360.0 * scale;
    double latRad = mapCoord.latitude * DEGREES_TO_RADIANS;
    double worldY = (1.0 - std::log(std::tan(latRad) + 1.0 / std::cos(latRad)) / M_PI) / 2.0 * scale;
    
    // Convert to screen coordinates relative to viewport center
    double centerX = (m_viewport.center.longitude + 180.0) / 360.0 * scale;
    double centerLatRad = m_viewport.center.latitude * DEGREES_TO_RADIANS;
    double centerY = (1.0 - std::log(std::tan(centerLatRad) + 1.0 / std::cos(centerLatRad)) / M_PI) / 2.0 * scale;
    
    int32_t screenX = static_cast<int32_t>((worldX - centerX) + DISPLAY_WIDTH / 2);
    int32_t screenY = static_cast<int32_t>((worldY - centerY) + DISPLAY_HEIGHT / 2);
    
    return ScreenCoordinate(screenX, screenY);
}

MapCoordinate MappingApp::screenToMap(const ScreenCoordinate& screenCoord) const {
    // Reverse Web Mercator projection
    double scale = TILE_SIZE * (1 << m_viewport.zoomLevel);
    
    // Convert screen to world coordinates
    double centerX = (m_viewport.center.longitude + 180.0) / 360.0 * scale;
    double centerLatRad = m_viewport.center.latitude * DEGREES_TO_RADIANS;
    double centerY = (1.0 - std::log(std::tan(centerLatRad) + 1.0 / std::cos(centerLatRad)) / M_PI) / 2.0 * scale;
    
    double worldX = centerX + (screenCoord.x - DISPLAY_WIDTH / 2);
    double worldY = centerY + (screenCoord.y - DISPLAY_HEIGHT / 2);
    
    // Convert to geographic coordinates
    double longitude = worldX / scale * 360.0 - 180.0;
    double latRad = atan(sinh(M_PI * (1.0 - 2.0 * worldY / scale)));
    double latitude = latRad * RAD_TO_DEG;
    
    return MapCoordinate(latitude, longitude);
}

void MappingApp::updateViewport() {
    // Trigger map redraw
    renderMapTiles();
    renderPOIMarkers();
    
    if (m_gpsEnabled && m_currentGPSLocation.isValid) {
        renderGPSMarker();
    }
}

void MappingApp::renderMapTiles() {
    if (!m_mapDataLoaded || !m_mapCanvas) {
        return;
    }
    
    // Get required tiles for current viewport
    std::vector<MapTile*> requiredTiles;
    getRequiredTiles(requiredTiles);
    
    // Load and render tiles
    for (MapTile* tile : requiredTiles) {
        if (!tile->isLoaded) {
            loadTile(tile);
        }
        
        if (tile->isLoaded && tile->imageData) {
            // Calculate tile position on screen
            uint32_t tileWorldX = tile->x * m_tileSize;
            uint32_t tileWorldY = tile->y * m_tileSize;
            
            // Convert to screen coordinates
            // This is a simplified version - full implementation would handle proper tile positioning
            int32_t screenX = static_cast<int32_t>(tileWorldX - (m_viewport.center.longitude + 180.0) / 360.0 * TILE_SIZE * (1 << m_viewport.zoomLevel)) + DISPLAY_WIDTH / 2;
            int32_t screenY = static_cast<int32_t>(tileWorldY - (1.0 - std::log(std::tan(m_viewport.center.latitude * DEGREES_TO_RADIANS) + 1.0 / std::cos(m_viewport.center.latitude * DEGREES_TO_RADIANS)) / M_PI) / 2.0 * TILE_SIZE * (1 << m_viewport.zoomLevel)) + DISPLAY_HEIGHT / 2;
            
            // Create or update tile image object
            // In full implementation, this would create LVGL image objects for visible tiles
            tile->lastUsed = lv_tick_get();
        }
    }
    
    m_tilesRendered += requiredTiles.size();
}

void MappingApp::renderPOIMarkers() {
    if (!m_poiLayerVisible || !m_mapCanvas) {
        return;
    }
    
    // Clear existing POI markers that are out of view
    for (auto it = m_poiMarkers.begin(); it != m_poiMarkers.end();) {
        lv_obj_t* marker = *it;
        if (marker) {
            // Check if marker is still in viewport (simplified check)
            lv_obj_del(marker);
        }
        it = m_poiMarkers.erase(it);
    }
    
    // Create markers for visible POIs
    for (const auto& poi : m_pointsOfInterest) {
        if (!poi.visible) {
            continue;
        }
        
        ScreenCoordinate screenPos = mapToScreen(poi.location);
        
        // Check if POI is within visible screen area (with margin)
        if (screenPos.x >= -50 && screenPos.x < DISPLAY_WIDTH + 50 &&
            screenPos.y >= -50 && screenPos.y < DISPLAY_HEIGHT + 50) {
            
            // Create POI marker
            lv_obj_t* marker = lv_obj_create(m_mapCanvas);
            lv_obj_set_size(marker, 16, 16);
            lv_obj_set_pos(marker, screenPos.x - 8, screenPos.y - 8);
            lv_obj_set_style_radius(marker, 8, 0);
            lv_obj_set_style_bg_color(marker, lv_color_hex(0x00AA00), 0); // Green marker
            lv_obj_set_style_border_width(marker, 1, 0);
            lv_obj_set_style_border_color(marker, lv_color_hex(0xFFFFFF), 0);
            
            // Add POI name label
            lv_obj_t* nameLabel = lv_label_create(m_mapCanvas);
            lv_label_set_text(nameLabel, poi.name.c_str());
            lv_obj_set_style_text_color(nameLabel, lv_color_hex(0x000000), 0);
            lv_obj_set_style_bg_color(nameLabel, lv_color_hex(0xFFFFFF), 0);
            lv_obj_set_style_bg_opa(nameLabel, LV_OPA_80, 0);
            lv_obj_set_style_pad_all(nameLabel, 2, 0);
            lv_obj_align_to(nameLabel, marker, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);
            
            m_poiMarkers.push_back(marker);
            m_poiMarkers.push_back(nameLabel);
        }
    }
    
    m_poisRendered = m_poiMarkers.size() / 2; // Each POI has marker + label
}

void MappingApp::renderGPSMarker() {
    if (!m_gpsEnabled || !m_currentGPSLocation.isValid || !m_gpsMarker) {
        return;
    }
    
    ScreenCoordinate screenPos = mapToScreen(m_currentGPSLocation.position);
    lv_obj_set_pos(m_gpsMarker, screenPos.x - 10, screenPos.y - 10);
    lv_obj_clear_flag(m_gpsMarker, LV_OBJ_FLAG_HIDDEN);
}

void MappingApp::getRequiredTiles(std::vector<MapTile*>& requiredTiles) {
    // Calculate visible tile range for current viewport
    uint32_t centerTileX, centerTileY;
    calculateTileCoordinates(m_viewport.center, m_viewport.zoomLevel, centerTileX, centerTileY);
    
    // Calculate how many tiles we need to cover the screen
    uint32_t tilesX = (DISPLAY_WIDTH / m_tileSize) + 2; // +2 for partial tiles
    uint32_t tilesY = (DISPLAY_HEIGHT / m_tileSize) + 2;
    
    uint32_t startX = (centerTileX > tilesX/2) ? centerTileX - tilesX/2 : 0;
    uint32_t startY = (centerTileY > tilesY/2) ? centerTileY - tilesY/2 : 0;
    uint32_t endX = centerTileX + tilesX/2;
    uint32_t endY = centerTileY + tilesY/2;
    
    uint32_t maxTiles = 1 << m_viewport.zoomLevel;
    endX = std::min(endX, maxTiles);
    endY = std::min(endY, maxTiles);
    
    requiredTiles.clear();
    
    for (uint32_t y = startY; y <= endY; y++) {
        for (uint32_t x = startX; x <= endX; x++) {
            // Find existing tile or create new one
            MapTile* tile = nullptr;
            
            for (auto& existingTile : m_tileCache.tiles) {
                if (existingTile && existingTile->x == x && existingTile->y == y && existingTile->zoom == m_viewport.zoomLevel) {
                    tile = existingTile.get();
                    break;
                }
            }
            
            if (!tile) {
                // Create new tile
                auto newTile = std::make_unique<MapTile>();
                newTile->x = x;
                newTile->y = y;
                newTile->zoom = m_viewport.zoomLevel;
                newTile->filePath = buildTileFilePath(x, y, m_viewport.zoomLevel);
                newTile->isLoaded = false;
                newTile->imageData = nullptr;
                newTile->lastUsed = lv_tick_get();
                
                tile = newTile.get();
                m_tileCache.tiles.push_back(std::move(newTile));
            }
            
            if (tile) {
                requiredTiles.push_back(tile);
                tile->lastUsed = lv_tick_get();
            }
        }
    }
}

void MappingApp::manageTileCache() {
    // Remove old unused tiles to free memory
    auto currentTime = lv_tick_get();
    constexpr uint32_t MAX_TILE_AGE = 300000; // 5 minutes
    
    m_tileCache.tiles.erase(
        std::remove_if(m_tileCache.tiles.begin(), m_tileCache.tiles.end(),
                      [currentTime](const std::unique_ptr<MapTile>& tile) {
                          if (tile && (currentTime - tile->lastUsed) > MAX_TILE_AGE) {
                              if (tile->imageData) {
                                  lv_mem_free((void*)tile->imageData);
                              }
                              return true;
                          }
                          return false;
                      }),
        m_tileCache.tiles.end());
    
    // Enforce cache size limit
    while (m_tileCache.currentCacheSize > m_tileCache.maxCacheSize && !m_tileCache.tiles.empty()) {
        // Find oldest tile
        auto oldestIt = std::min_element(m_tileCache.tiles.begin(), m_tileCache.tiles.end(),
                                        [](const std::unique_ptr<MapTile>& a, const std::unique_ptr<MapTile>& b) {
                                            return a->lastUsed < b->lastUsed;
                                        });
        
        if (oldestIt != m_tileCache.tiles.end() && *oldestIt) {
            if ((*oldestIt)->imageData) {
                lv_mem_free((void*)(*oldestIt)->imageData);
                m_tileCache.currentCacheSize -= m_tileSize * m_tileSize * 2; // Assuming RGB565
            }
            m_tileCache.tiles.erase(oldestIt);
        } else {
            break;
        }
    }
}

void MappingApp::calculateTileCoordinates(const MapCoordinate& coord, uint8_t zoom, uint32_t& tileX, uint32_t& tileY) const {
    uint32_t n = 1 << zoom;
    tileX = static_cast<uint32_t>((coord.longitude + 180.0) / 360.0 * n);
    
    double latRad = coord.latitude * DEGREES_TO_RADIANS;
    tileY = static_cast<uint32_t>((1.0 - std::asinh(std::tan(latRad)) / M_PI) / 2.0 * n);
    
    // Clamp to valid range
    tileX = std::min(tileX, n - 1);
    tileY = std::min(tileY, n - 1);
}

std::string MappingApp::buildTileFilePath(uint32_t tileX, uint32_t tileY, uint8_t zoom) const {
    if (m_mapDataSource == MapDataSource::OFFLINE_TILES) {
        return m_mapDataPath + "/tiles/" + std::to_string(zoom) + "/" + 
               std::to_string(tileX) + "/" + std::to_string(tileY) + ".png";
    }
    
    return "";
}

os_error_t MappingApp::loadTile(MapTile* tile) {
    if (!tile || tile->filePath.empty()) {
        return OS_ERROR_INVALID_PARAM;
    }
    
    // Check if tile file exists
    struct stat st;
    if (stat(tile->filePath.c_str(), &st) != 0) {
        ESP_LOGD(TAG, "Tile file not found: %s", tile->filePath.c_str());
        return OS_ERROR_NOT_FOUND;
    }
    
    // In a full implementation, this would:
    // 1. Load the image file from storage
    // 2. Convert it to LVGL image format
    // 3. Store it in tile->imageData
    // 4. Update cache size
    
    tile->isLoaded = true;
    m_tileCache.hitCount++;
    
    ESP_LOGD(TAG, "Tile loaded: %s", tile->filePath.c_str());
    return OS_OK;
}

void MappingApp::unloadTile(MapTile* tile) {
    if (!tile || !tile->imageData) {
        return;
    }
    
    lv_mem_free((void*)tile->imageData);
    tile->imageData = nullptr;
    tile->isLoaded = false;
    
    // Update cache size
    m_tileCache.currentCacheSize -= m_tileSize * m_tileSize * 2; // RGB565
}

os_error_t MappingApp::parseMBTiles(const std::string& mbtilesPath) {
    ESP_LOGI(TAG, "Parsing MBTiles database: %s", mbtilesPath.c_str());
    
    // In a full implementation, this would:
    // 1. Open SQLite database
    // 2. Read metadata table for map bounds, zoom levels, etc.
    // 3. Set up tile query infrastructure
    // 4. Extract map metadata
    
    ESP_LOGI(TAG, "MBTiles parsing completed");
    return OS_OK;
}

os_error_t MappingApp::loadPOIData(const std::string& poiFilePath) {
    ESP_LOGI(TAG, "Loading POI data from: %s", poiFilePath.c_str());
    
    // In a full implementation, this would:
    // 1. Parse JSON file containing POI data
    // 2. Create POIData objects
    // 3. Add them to m_pointsOfInterest vector
    
    // Example POI for demonstration
    POIData examplePOI;
    examplePOI.location = MapCoordinate(35.6762, 139.6503); // Tokyo
    examplePOI.name = "Tokyo Station";
    examplePOI.category = "Transportation";
    examplePOI.description = "Major railway station in Tokyo";
    examplePOI.icon = LV_SYMBOL_HOME;
    examplePOI.priority = 255;
    examplePOI.visible = true;
    
    addPOI(examplePOI);
    
    ESP_LOGI(TAG, "POI data loaded");
    return OS_OK;
}

void MappingApp::getMapStatistics(uint32_t& tilesLoaded, uint32_t& poisLoaded, uint32_t& cacheHits, 
                                 uint32_t& cacheMisses, size_t& memoryUsage) {
    tilesLoaded = m_tilesRendered;
    poisLoaded = m_pointsOfInterest.size();
    cacheHits = m_tileCache.hitCount;
    cacheMisses = m_tileCache.missCount;
    memoryUsage = m_tileCache.currentCacheSize;
}

// UI Event Callbacks
void MappingApp::mapCanvasCallback(lv_event_t* e) {
    MappingApp* app = static_cast<MappingApp*>(lv_event_get_user_data(e));
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_PRESSED) {
        lv_indev_t* indev = lv_indev_get_act();
        lv_point_t point;
        lv_indev_get_point(indev, &point);
        
        app->m_lastTouchPoint = ScreenCoordinate(point.x, point.y);
        app->m_isPanning = true;
        app->m_lastTouchTime = lv_tick_get();
        
    } else if (code == LV_EVENT_PRESSING && app->m_isPanning) {
        lv_indev_t* indev = lv_indev_get_act();
        lv_point_t point;
        lv_indev_get_point(indev, &point);
        
        int32_t deltaX = point.x - app->m_lastTouchPoint.x;
        int32_t deltaY = point.y - app->m_lastTouchPoint.y;
        
        if (abs(deltaX) > 5 || abs(deltaY) > 5) { // Minimum movement threshold
            app->panMap(-deltaX, -deltaY); // Negative for natural scrolling
            app->m_lastTouchPoint = ScreenCoordinate(point.x, point.y);
        }
        
    } else if (code == LV_EVENT_RELEASED) {
        app->m_isPanning = false;
        
        // Check for double-tap (zoom in)
        uint32_t currentTime = lv_tick_get();
        if (currentTime - app->m_lastTouchTime < 500) { // 500ms double-tap window
            app->zoomIn();
        }
        app->m_lastTouchTime = currentTime;
    }
}

void MappingApp::zoomInButtonCallback(lv_event_t* e) {
    MappingApp* app = static_cast<MappingApp*>(lv_event_get_user_data(e));
    app->zoomIn();
}

void MappingApp::zoomOutButtonCallback(lv_event_t* e) {
    MappingApp* app = static_cast<MappingApp*>(lv_event_get_user_data(e));
    app->zoomOut();
}

void MappingApp::centerGPSButtonCallback(lv_event_t* e) {
    MappingApp* app = static_cast<MappingApp*>(lv_event_get_user_data(e));
    
    if (app->m_gpsEnabled && app->m_currentGPSLocation.isValid) {
        app->setMapCenter(app->m_currentGPSLocation.position);
        app->setNavigationMode(NavigationMode::GPS_FOLLOW);
    } else {
        app->enableGPSTracking(!app->m_gpsEnabled);
    }
}

void MappingApp::layerButtonCallback(lv_event_t* e) {
    MappingApp* app = static_cast<MappingApp*>(lv_event_get_user_data(e));
    
    // Toggle POI layer visibility
    app->setPOILayerVisible(!app->m_poiLayerVisible);
}

void MappingApp::searchButtonCallback(lv_event_t* e) {
    MappingApp* app = static_cast<MappingApp*>(lv_event_get_user_data(e));
    
    // In full implementation, this would open a search dialog
    ESP_LOGI(TAG, "Search button clicked");
}

void MappingApp::menuButtonCallback(lv_event_t* e) {
    MappingApp* app = static_cast<MappingApp*>(lv_event_get_user_data(e));
    
    // In full implementation, this would open settings menu
    ESP_LOGI(TAG, "Menu button clicked");
}

void MappingApp::poiButtonCallback(lv_event_t* e) {
    MappingApp* app = static_cast<MappingApp*>(lv_event_get_user_data(e));
    
    // In full implementation, this would open POI management
    ESP_LOGI(TAG, "POI button clicked");
}

// Factory function
extern "C" std::unique_ptr<BaseApp> createMappingApp() {
    return std::make_unique<MappingApp>();
}