#ifndef MAPPING_APP_H
#define MAPPING_APP_H

#include "base_app.h"
#include "../services/storage_service.h"
#include <vector>
#include <string>
#include <memory>
#include <cmath>

/**
 * @file mapping_app.h
 * @brief OpenMaps-style Offline Mapping Application for M5Stack Tab5
 * 
 * Provides comprehensive offline mapping functionality including:
 * - Tile-based map rendering with zoom levels
 * - Map data loading from SD card or USB storage
 * - GPS integration and location tracking
 * - Points of interest (POI) management
 * - Route planning and navigation
 * - Touch-based pan and zoom controls
 */

// Map coordinate system (Web Mercator projection)
struct MapCoordinate {
    double latitude;
    double longitude;
    
    MapCoordinate(double lat = 0.0, double lon = 0.0) : latitude(lat), longitude(lon) {}
    
    bool isValid() const {
        return (latitude >= -85.051128 && latitude <= 85.051128 && 
                longitude >= -180.0 && longitude <= 180.0);
    }
};

// Screen coordinates for UI rendering
struct ScreenCoordinate {
    int32_t x;
    int32_t y;
    
    ScreenCoordinate(int32_t px = 0, int32_t py = 0) : x(px), y(py) {}
};

// Map tile information
struct MapTile {
    uint32_t x;          // Tile X coordinate
    uint32_t y;          // Tile Y coordinate
    uint8_t zoom;        // Zoom level (0-20)
    std::string filePath; // Path to tile image file
    bool isLoaded;       // Whether tile is loaded in memory
    lv_img_dsc_t* imageData; // LVGL image descriptor
    uint32_t lastUsed;   // Last access timestamp for cache management
};

// Point of Interest data
struct POIData {
    MapCoordinate location;
    std::string name;
    std::string category;
    std::string description;
    std::string icon;    // Icon symbol or path
    uint8_t priority;    // Display priority (0-255)
    bool visible;        // Whether POI is currently visible
};

// Map layer types
enum class MapLayerType {
    BASE_MAP,
    SATELLITE,
    TERRAIN,
    ROADS,
    POI_LAYER,
    ROUTE_LAYER,
    GPS_LAYER
};

// Map data source types
enum class MapDataSource {
    OFFLINE_TILES,
    MBTiles,
    VECTOR_TILES,
    CUSTOM_FORMAT
};

// Navigation mode
enum class NavigationMode {
    BROWSE,
    GPS_FOLLOW,
    ROUTE_PREVIEW,
    TURN_BY_TURN
};

// Map rendering viewport
struct MapViewport {
    MapCoordinate center;     // Center coordinate
    uint8_t zoomLevel;       // Current zoom level (0-20)
    double scale;            // Current scale factor
    int32_t pixelOffsetX;    // Pixel offset for smooth panning
    int32_t pixelOffsetY;    // Pixel offset for smooth panning
    double rotation;         // Map rotation in degrees
};

// Map tile cache entry
struct TileCache {
    std::vector<std::unique_ptr<MapTile>> tiles;
    uint32_t maxCacheSize;   // Maximum cache size in MB
    uint32_t currentCacheSize; // Current cache usage
    uint32_t hitCount;       // Cache hit counter
    uint32_t missCount;      // Cache miss counter
};

// GPS location data
struct GPSLocation {
    MapCoordinate position;
    double altitude;         // Altitude in meters
    double speed;           // Speed in km/h
    double heading;         // Heading in degrees (0-360)
    double accuracy;        // Accuracy in meters
    uint32_t timestamp;     // GPS timestamp
    bool isValid;           // Whether GPS fix is valid
};

// Route waypoint
struct RouteWaypoint {
    MapCoordinate location;
    std::string instruction;
    double distance;        // Distance to next waypoint in meters
    uint32_t estimatedTime; // Estimated time in seconds
    std::string streetName;
};

class MappingApp : public BaseApp {
public:
    /**
     * @brief Constructor
     */
    MappingApp();
    
    /**
     * @brief Destructor
     */
    ~MappingApp() override;

    // BaseApp interface implementation
    os_error_t initialize() override;
    os_error_t update(uint32_t deltaTime) override;
    os_error_t shutdown() override;
    os_error_t createUI(lv_obj_t* parent) override;
    os_error_t destroyUI() override;
    os_error_t handleEvent(uint32_t eventType, void* eventData, size_t dataSize) override;

    /**
     * @brief Load map data from storage
     * @param mapDataPath Path to map data directory
     * @return OS_OK on success, error code on failure
     */
    os_error_t loadMapData(const std::string& mapDataPath);

    /**
     * @brief Set map center coordinate
     * @param coordinate Center coordinate
     * @return OS_OK on success, error code on failure
     */
    os_error_t setMapCenter(const MapCoordinate& coordinate);

    /**
     * @brief Set zoom level
     * @param zoomLevel Zoom level (0-20)
     * @return OS_OK on success, error code on failure
     */
    os_error_t setZoomLevel(uint8_t zoomLevel);

    /**
     * @brief Pan map by pixel offset
     * @param deltaX X offset in pixels
     * @param deltaY Y offset in pixels
     * @return OS_OK on success, error code on failure
     */
    os_error_t panMap(int32_t deltaX, int32_t deltaY);

    /**
     * @brief Zoom in by one level
     * @return OS_OK on success, error code on failure
     */
    os_error_t zoomIn();

    /**
     * @brief Zoom out by one level
     * @return OS_OK on success, error code on failure
     */
    os_error_t zoomOut();

    /**
     * @brief Add point of interest
     * @param poi POI data to add
     * @return OS_OK on success, error code on failure
     */
    os_error_t addPOI(const POIData& poi);

    /**
     * @brief Remove point of interest
     * @param name POI name to remove
     * @return OS_OK on success, error code on failure
     */
    os_error_t removePOI(const std::string& name);

    /**
     * @brief Show/hide POI layer
     * @param visible Whether POI layer should be visible
     */
    void setPOILayerVisible(bool visible);

    /**
     * @brief Enable GPS tracking
     * @param enable Whether to enable GPS tracking
     * @return OS_OK on success, error code on failure
     */
    os_error_t enableGPSTracking(bool enable);

    /**
     * @brief Update GPS location
     * @param location Current GPS location
     * @return OS_OK on success, error code on failure
     */
    os_error_t updateGPSLocation(const GPSLocation& location);

    /**
     * @brief Set navigation mode
     * @param mode Navigation mode to set
     */
    void setNavigationMode(NavigationMode mode);

    /**
     * @brief Calculate route between two points
     * @param start Start coordinate
     * @param end End coordinate
     * @param waypoints Output route waypoints
     * @return OS_OK on success, error code on failure
     */
    os_error_t calculateRoute(const MapCoordinate& start, const MapCoordinate& end, 
                             std::vector<RouteWaypoint>& waypoints);

    /**
     * @brief Search for location by name
     * @param searchQuery Search query string
     * @param results Output search results
     * @return OS_OK on success, error code on failure
     */
    os_error_t searchLocation(const std::string& searchQuery, std::vector<POIData>& results);

    /**
     * @brief Convert map coordinate to screen coordinate
     * @param mapCoord Map coordinate
     * @return Screen coordinate
     */
    ScreenCoordinate mapToScreen(const MapCoordinate& mapCoord) const;

    /**
     * @brief Convert screen coordinate to map coordinate
     * @param screenCoord Screen coordinate
     * @return Map coordinate
     */
    MapCoordinate screenToMap(const ScreenCoordinate& screenCoord) const;

    /**
     * @brief Get current map center
     * @return Current map center coordinate
     */
    const MapCoordinate& getMapCenter() const { return m_viewport.center; }

    /**
     * @brief Get current zoom level
     * @return Current zoom level
     */
    uint8_t getZoomLevel() const { return m_viewport.zoomLevel; }

    /**
     * @brief Get current GPS location
     * @return Current GPS location (if available)
     */
    const GPSLocation& getCurrentGPSLocation() const { return m_currentGPSLocation; }

    /**
     * @brief Check if map data is loaded
     * @return true if map data is available
     */
    bool isMapDataLoaded() const { return m_mapDataLoaded; }

    /**
     * @brief Get map statistics
     */
    void getMapStatistics(uint32_t& tilesLoaded, uint32_t& poisLoaded, uint32_t& cacheHits, 
                         uint32_t& cacheMisses, size_t& memoryUsage);

private:
    /**
     * @brief Create map view UI
     */
    void createMapViewUI();

    /**
     * @brief Create navigation controls UI
     */
    void createNavigationControlsUI();

    /**
     * @brief Create information panel UI
     */
    void createInformationPanelUI();

    /**
     * @brief Render map tiles
     */
    void renderMapTiles();

    /**
     * @brief Render POI markers
     */
    void renderPOIMarkers();

    /**
     * @brief Render GPS location marker
     */
    void renderGPSMarker();

    /**
     * @brief Render route path
     */
    void renderRoutePath();

    /**
     * @brief Update viewport based on current settings
     */
    void updateViewport();

    /**
     * @brief Load tile from storage
     * @param tile Tile to load
     * @return OS_OK on success, error code on failure
     */
    os_error_t loadTile(MapTile* tile);

    /**
     * @brief Unload tile from memory
     * @param tile Tile to unload
     */
    void unloadTile(MapTile* tile);

    /**
     * @brief Get required tiles for current viewport
     * @param requiredTiles Output vector of required tiles
     */
    void getRequiredTiles(std::vector<MapTile*>& requiredTiles);

    /**
     * @brief Manage tile cache (LRU eviction)
     */
    void manageTileCache();

    /**
     * @brief Calculate tile coordinates for zoom level
     * @param coord Map coordinate
     * @param zoom Zoom level
     * @param tileX Output tile X coordinate
     * @param tileY Output tile Y coordinate
     */
    void calculateTileCoordinates(const MapCoordinate& coord, uint8_t zoom, 
                                 uint32_t& tileX, uint32_t& tileY) const;

    /**
     * @brief Build tile file path
     * @param tileX Tile X coordinate
     * @param tileY Tile Y coordinate
     * @param zoom Zoom level
     * @return Tile file path
     */
    std::string buildTileFilePath(uint32_t tileX, uint32_t tileY, uint8_t zoom) const;

    /**
     * @brief Parse MBTiles database
     * @param mbtilesPath Path to MBTiles file
     * @return OS_OK on success, error code on failure
     */
    os_error_t parseMBTiles(const std::string& mbtilesPath);

    /**
     * @brief Load POI data from file
     * @param poiFilePath Path to POI data file
     * @return OS_OK on success, error code on failure
     */
    os_error_t loadPOIData(const std::string& poiFilePath);

    // UI event callbacks
    static void mapCanvasCallback(lv_event_t* e);
    static void zoomInButtonCallback(lv_event_t* e);
    static void zoomOutButtonCallback(lv_event_t* e);
    static void centerGPSButtonCallback(lv_event_t* e);
    static void layerButtonCallback(lv_event_t* e);
    static void searchButtonCallback(lv_event_t* e);
    static void menuButtonCallback(lv_event_t* e);
    static void poiButtonCallback(lv_event_t* e);

    // Storage service
    StorageService* m_storageService = nullptr;
    bool m_ownStorageService = false;

    // Map data
    std::string m_mapDataPath;
    MapDataSource m_mapDataSource = MapDataSource::OFFLINE_TILES;
    bool m_mapDataLoaded = false;
    MapViewport m_viewport;

    // Tile management
    TileCache m_tileCache;
    std::vector<std::unique_ptr<MapTile>> m_visibleTiles;
    uint32_t m_tileSize = 256; // Standard tile size in pixels
    
    // POI management
    std::vector<POIData> m_pointsOfInterest;
    bool m_poiLayerVisible = true;

    // GPS tracking
    bool m_gpsEnabled = false;
    GPSLocation m_currentGPSLocation;
    NavigationMode m_navigationMode = NavigationMode::BROWSE;
    std::vector<RouteWaypoint> m_currentRoute;

    // Touch interaction
    bool m_isPanning = false;
    ScreenCoordinate m_lastTouchPoint;
    uint32_t m_lastTouchTime = 0;
    
    // Statistics
    uint32_t m_mapUpdates = 0;
    uint32_t m_tilesRendered = 0;
    uint32_t m_poisRendered = 0;

    // UI elements - Main containers
    lv_obj_t* m_mainContainer = nullptr;
    lv_obj_t* m_mapCanvas = nullptr;
    lv_obj_t* m_controlPanel = nullptr;
    lv_obj_t* m_infoPanel = nullptr;

    // UI elements - Navigation controls
    lv_obj_t* m_zoomInButton = nullptr;
    lv_obj_t* m_zoomOutButton = nullptr;
    lv_obj_t* m_centerGPSButton = nullptr;
    lv_obj_t* m_layerButton = nullptr;
    lv_obj_t* m_searchButton = nullptr;
    lv_obj_t* m_menuButton = nullptr;
    lv_obj_t* m_poiButton = nullptr;

    // UI elements - Information display
    lv_obj_t* m_coordinateLabel = nullptr;
    lv_obj_t* m_zoomLabel = nullptr;
    lv_obj_t* m_gpsStatusLabel = nullptr;
    lv_obj_t* m_scaleLabel = nullptr;

    // UI elements - Map overlay
    lv_obj_t* m_gpsMarker = nullptr;
    lv_obj_t* m_routeLine = nullptr;
    std::vector<lv_obj_t*> m_poiMarkers;

    // Configuration constants
    static constexpr uint8_t MIN_ZOOM_LEVEL = 0;
    static constexpr uint8_t MAX_ZOOM_LEVEL = 20;
    static constexpr uint32_t TILE_CACHE_SIZE_MB = 64;
    static constexpr uint32_t GPS_UPDATE_INTERVAL = 1000; // 1 second
    static constexpr double EARTH_RADIUS = 6378137.0; // Earth radius in meters
    static constexpr double DEGREES_TO_RADIANS = M_PI / 180.0;
    static constexpr double RADIANS_TO_DEGREES = 180.0 / M_PI;
    static constexpr const char* TAG = "MappingApp";
};

/**
 * @brief Factory function for creating app instances
 * Required for dynamic loading by the app manager
 * @return Unique pointer to app instance
 */
extern "C" std::unique_ptr<BaseApp> createMappingApp();

#endif // MAPPING_APP_H