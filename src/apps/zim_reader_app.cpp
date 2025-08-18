#include "zim_reader_app.h"
#include "../system/os_manager.h"
#include <esp_log.h>
#include <algorithm>
#include <sstream>
#include <regex>
#include <cstring>

static const char* TAG = "ZimReader";

ZimReaderApp::ZimReaderApp() : BaseApp("zim_reader", "ZIM Reader", "1.0") {
    m_viewMode = ViewMode::FILE_BROWSER;
    m_readerState = ZimReaderState::NO_FILE;
    m_historyIndex = -1;
    m_fileLoaded = false;
    m_zimFileHandle = nullptr;
    m_fileSize = 0;
    
    setDescription("Offline content reader for ZIM archives (Wikipedia, etc.)");
    setAuthor("M5Stack");
    setPriority(AppPriority::APP_NORMAL);
}

ZimReaderApp::~ZimReaderApp() {
    shutdown();
}

os_error_t ZimReaderApp::initialize() {
    if (m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Initializing ZIM Reader");
    
    // Scan for ZIM files
    m_zimFiles = scanForZimFiles();
    
    // Load bookmarks
    loadBookmarks();
    
    // Set memory usage estimate
    setMemoryUsage(256000); // 250KB estimated usage
    
    m_initialized = true;
    ESP_LOGI(TAG, "ZIM Reader initialized with %d ZIM files found", m_zimFiles.size());
    return OS_OK;
}

os_error_t ZimReaderApp::update(uint32_t deltaTime) {
    if (!m_initialized) {
        return OS_ERROR_NOT_INITIALIZED;
    }
    
    // Update loading animations, progress, etc.
    if (m_readerState == ZimReaderState::LOADING) {
        // Could add loading progress updates here
    }
    
    return OS_OK;
}

os_error_t ZimReaderApp::shutdown() {
    if (!m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Shutting down ZIM Reader");
    
    // Close any open files
    closeZimFile();
    
    // Save bookmarks
    saveBookmarks();
    
    destroyUI();
    
    m_initialized = false;
    return OS_OK;
}

os_error_t ZimReaderApp::createUI(lv_obj_t* parent) {
    if (!parent) {
        return OS_ERROR_INVALID_PARAM;
    }

    m_uiContainer = lv_obj_create(parent);
    lv_obj_set_size(m_uiContainer, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(m_uiContainer, lv_color_hex(0x1E1E1E), 0);
    lv_obj_clear_flag(m_uiContainer, LV_OBJ_FLAG_SCROLLABLE);

    createNavigationBar();
    createStatusBar();
    
    // Create content area
    m_contentArea = lv_obj_create(m_uiContainer);
    lv_obj_set_size(m_contentArea, LV_PCT(100), LV_PCT(100) - 90);
    lv_obj_align_to(m_contentArea, m_navigationBar, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(m_contentArea, lv_color_hex(0x2C2C2C), 0);
    lv_obj_clear_flag(m_contentArea, LV_OBJ_FLAG_SCROLLABLE);
    
    // Create all view modes
    createFileBrowser();
    createArticleList();
    createArticleReader();
    createSearchInterface();
    createBookmarkManager();
    
    // Start with file browser
    switchViewMode(ViewMode::FILE_BROWSER);
    updateStatusBar();

    ESP_LOGI(TAG, "ZIM Reader UI created");
    return OS_OK;
}

os_error_t ZimReaderApp::destroyUI() {
    closeZimFile();
    
    if (m_uiContainer) {
        lv_obj_del(m_uiContainer);
        m_uiContainer = nullptr;
        
        // Clear all UI references
        m_navigationBar = nullptr;
        m_statusBar = nullptr;
        m_contentArea = nullptr;
        m_fileBrowser = nullptr;
        m_articleList = nullptr;
        m_articleReader = nullptr;
        m_searchInterface = nullptr;
        m_bookmarkManager = nullptr;
        
        for (int i = 0; i < 5; i++) {
            m_viewModeButtons[i] = nullptr;
        }
    }
    return OS_OK;
}

void ZimReaderApp::createNavigationBar() {
    m_navigationBar = lv_obj_create(m_uiContainer);
    lv_obj_set_size(m_navigationBar, LV_PCT(100), 50);
    lv_obj_align(m_navigationBar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(m_navigationBar, lv_color_hex(0x34495E), 0);
    lv_obj_set_style_radius(m_navigationBar, 0, 0);
    
    // Back button
    m_backButton = lv_btn_create(m_navigationBar);
    lv_obj_set_size(m_backButton, 40, 35);
    lv_obj_align(m_backButton, LV_ALIGN_LEFT_MID, 5, 0);
    lv_obj_add_event_cb(m_backButton, backButtonCallback, LV_EVENT_CLICKED, this);
    
    lv_obj_t* backLabel = lv_label_create(m_backButton);
    lv_label_set_text(backLabel, "<");
    lv_obj_center(backLabel);
    
    // Forward button
    m_forwardButton = lv_btn_create(m_navigationBar);
    lv_obj_set_size(m_forwardButton, 40, 35);
    lv_obj_align_to(m_forwardButton, m_backButton, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    lv_obj_add_event_cb(m_forwardButton, forwardButtonCallback, LV_EVENT_CLICKED, this);
    
    lv_obj_t* forwardLabel = lv_label_create(m_forwardButton);
    lv_label_set_text(forwardLabel, ">");
    lv_obj_center(forwardLabel);
    
    // Home button
    m_homeButton = lv_btn_create(m_navigationBar);
    lv_obj_set_size(m_homeButton, 50, 35);
    lv_obj_align_to(m_homeButton, m_forwardButton, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    lv_obj_add_event_cb(m_homeButton, homeButtonCallback, LV_EVENT_CLICKED, this);
    
    lv_obj_t* homeLabel = lv_label_create(m_homeButton);
    lv_label_set_text(homeLabel, "Home");
    lv_obj_center(homeLabel);
    
    // Bookmark button
    m_bookmarkButton = lv_btn_create(m_navigationBar);
    lv_obj_set_size(m_bookmarkButton, 50, 35);
    lv_obj_align(m_bookmarkButton, LV_ALIGN_RIGHT_MID, -60, 0);
    lv_obj_add_event_cb(m_bookmarkButton, bookmarkButtonCallback, LV_EVENT_CLICKED, this);
    
    lv_obj_t* bookmarkLabel = lv_label_create(m_bookmarkButton);
    lv_label_set_text(bookmarkLabel, "★");
    lv_obj_center(bookmarkLabel);
    
    // Refresh button
    m_refreshButton = lv_btn_create(m_navigationBar);
    lv_obj_set_size(m_refreshButton, 50, 35);
    lv_obj_align(m_refreshButton, LV_ALIGN_RIGHT_MID, -5, 0);
    lv_obj_add_event_cb(m_refreshButton, refreshCallback, LV_EVENT_CLICKED, this);
    
    lv_obj_t* refreshLabel = lv_label_create(m_refreshButton);
    lv_label_set_text(refreshLabel, "⟳");
    lv_obj_center(refreshLabel);
}

void ZimReaderApp::createStatusBar() {
    m_statusBar = lv_obj_create(m_uiContainer);
    lv_obj_set_size(m_statusBar, LV_PCT(100), 40);
    lv_obj_align(m_statusBar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(m_statusBar, lv_color_hex(0x2C2C2C), 0);
    lv_obj_set_style_radius(m_statusBar, 0, 0);
    
    // View mode buttons
    const char* modes[] = {"Files", "List", "Read", "Search", "Marks"};
    for (int i = 0; i < 5; i++) {
        m_viewModeButtons[i] = lv_btn_create(m_statusBar);
        lv_obj_set_size(m_viewModeButtons[i], 60, 30);
        lv_obj_set_pos(m_viewModeButtons[i], 10 + i * 65, 5);
        lv_obj_set_user_data(m_viewModeButtons[i], (void*)i);
        lv_obj_add_event_cb(m_viewModeButtons[i], viewModeCallback, LV_EVENT_CLICKED, this);
        
        lv_obj_t* modeLabel = lv_label_create(m_viewModeButtons[i]);
        lv_label_set_text(modeLabel, modes[i]);
        lv_obj_center(modeLabel);
    }
    
    // Status label
    m_statusLabel = lv_label_create(m_statusBar);
    lv_label_set_text(m_statusLabel, "Ready");
    lv_obj_align(m_statusLabel, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_set_style_text_color(m_statusLabel, lv_color_white(), 0);
}

void ZimReaderApp::createFileBrowser() {
    m_fileBrowser = lv_obj_create(m_contentArea);
    lv_obj_set_size(m_fileBrowser, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(m_fileBrowser, lv_color_hex(0x2C2C2C), 0);
    
    // File list
    m_fileList = lv_list_create(m_fileBrowser);
    lv_obj_set_size(m_fileList, LV_PCT(100), LV_PCT(85));
    lv_obj_align(m_fileList, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(m_fileList, lv_color_hex(0x1E1E1E), 0);
    
    // Populate file list
    for (const auto& file : m_zimFiles) {
        lv_obj_t* btn = lv_list_add_btn(m_fileList, LV_SYMBOL_FILE, getFileName(file).c_str());
        lv_obj_set_user_data(btn, (void*)file.c_str());
        lv_obj_add_event_cb(btn, fileListCallback, LV_EVENT_CLICKED, this);
    }
    
    if (m_zimFiles.empty()) {
        lv_obj_t* btn = lv_list_add_btn(m_fileList, LV_SYMBOL_WARNING, "No ZIM files found");
        lv_obj_add_state(btn, LV_STATE_DISABLED);
    }
    
    // File info
    m_fileInfoLabel = lv_label_create(m_fileBrowser);
    lv_label_set_text(m_fileInfoLabel, "Select a ZIM file to open");
    lv_obj_align(m_fileInfoLabel, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_text_color(m_fileInfoLabel, lv_color_hex(0xBDC3C7), 0);
}

void ZimReaderApp::createArticleList() {
    m_articleList = lv_obj_create(m_contentArea);
    lv_obj_set_size(m_articleList, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(m_articleList, lv_color_hex(0x2C2C2C), 0);
    lv_obj_add_flag(m_articleList, LV_OBJ_FLAG_HIDDEN);
    
    // Article count label
    m_articleCountLabel = lv_label_create(m_articleList);
    lv_label_set_text(m_articleCountLabel, "Articles: 0");
    lv_obj_align(m_articleCountLabel, LV_ALIGN_TOP_LEFT, 10, 5);
    lv_obj_set_style_text_color(m_articleCountLabel, lv_color_white(), 0);
    
    // Article list view
    m_articleListView = lv_list_create(m_articleList);
    lv_obj_set_size(m_articleListView, LV_PCT(100), LV_PCT(90));
    lv_obj_align_to(m_articleListView, m_articleCountLabel, LV_ALIGN_OUT_BOTTOM_MID, -10, 5);
    lv_obj_set_style_bg_color(m_articleListView, lv_color_hex(0x1E1E1E), 0);
}

void ZimReaderApp::createArticleReader() {
    m_articleReader = lv_obj_create(m_contentArea);
    lv_obj_set_size(m_articleReader, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(m_articleReader, lv_color_hex(0x2C2C2C), 0);
    lv_obj_add_flag(m_articleReader, LV_OBJ_FLAG_HIDDEN);
    
    // Article title
    m_articleTitle = lv_label_create(m_articleReader);
    lv_label_set_text(m_articleTitle, "No Article Loaded");
    lv_obj_align(m_articleTitle, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_text_color(m_articleTitle, lv_color_white(), 0);
    lv_obj_set_style_text_font(m_articleTitle, &lv_font_montserrat_16, 0);
    
    // Article content scroll view
    m_articleScrollView = lv_obj_create(m_articleReader);
    lv_obj_set_size(m_articleScrollView, LV_PCT(95), LV_PCT(85));
    lv_obj_align_to(m_articleScrollView, m_articleTitle, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_set_style_bg_color(m_articleScrollView, lv_color_hex(0x1E1E1E), 0);
    
    // Article content
    m_articleContent = lv_label_create(m_articleScrollView);
    lv_label_set_text(m_articleContent, "Select an article to read");
    lv_obj_set_width(m_articleContent, LV_PCT(95));
    lv_label_set_long_mode(m_articleContent, LV_LABEL_LONG_WRAP);
    lv_obj_align(m_articleContent, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_set_style_text_color(m_articleContent, lv_color_hex(0xECF0F1), 0);
}

void ZimReaderApp::createSearchInterface() {
    m_searchInterface = lv_obj_create(m_contentArea);
    lv_obj_set_size(m_searchInterface, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(m_searchInterface, lv_color_hex(0x2C2C2C), 0);
    lv_obj_add_flag(m_searchInterface, LV_OBJ_FLAG_HIDDEN);
    
    // Search input
    m_searchInput = lv_textarea_create(m_searchInterface);
    lv_obj_set_size(m_searchInput, LV_PCT(70), 40);
    lv_obj_align(m_searchInput, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_textarea_set_placeholder_text(m_searchInput, "Search articles...");
    lv_textarea_set_one_line(m_searchInput, true);
    lv_obj_add_event_cb(m_searchInput, searchInputCallback, LV_EVENT_READY, this);
    
    // Search button
    m_searchButton = lv_btn_create(m_searchInterface);
    lv_obj_set_size(m_searchButton, LV_PCT(25), 40);
    lv_obj_align_to(m_searchButton, m_searchInput, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_add_event_cb(m_searchButton, searchButtonCallback, LV_EVENT_CLICKED, this);
    
    lv_obj_t* searchLabel = lv_label_create(m_searchButton);
    lv_label_set_text(searchLabel, "Search");
    lv_obj_center(searchLabel);
    
    // Search status
    m_searchStatus = lv_label_create(m_searchInterface);
    lv_label_set_text(m_searchStatus, "Enter search terms above");
    lv_obj_align_to(m_searchStatus, m_searchInput, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
    lv_obj_set_style_text_color(m_searchStatus, lv_color_hex(0xBDC3C7), 0);
    
    // Search results
    m_searchResults = lv_list_create(m_searchInterface);
    lv_obj_set_size(m_searchResults, LV_PCT(100), LV_PCT(70));
    lv_obj_align_to(m_searchResults, m_searchStatus, LV_ALIGN_OUT_BOTTOM_MID, -10, 10);
    lv_obj_set_style_bg_color(m_searchResults, lv_color_hex(0x1E1E1E), 0);
}

void ZimReaderApp::createBookmarkManager() {
    m_bookmarkManager = lv_obj_create(m_contentArea);
    lv_obj_set_size(m_bookmarkManager, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(m_bookmarkManager, lv_color_hex(0x2C2C2C), 0);
    lv_obj_add_flag(m_bookmarkManager, LV_OBJ_FLAG_HIDDEN);
    
    // Bookmark count
    m_bookmarkCount = lv_label_create(m_bookmarkManager);
    char countStr[64];
    snprintf(countStr, sizeof(countStr), "Bookmarks: %d", (int)m_bookmarks.size());
    lv_label_set_text(m_bookmarkCount, countStr);
    lv_obj_align(m_bookmarkCount, LV_ALIGN_TOP_LEFT, 10, 5);
    lv_obj_set_style_text_color(m_bookmarkCount, lv_color_white(), 0);
    
    // Bookmark list
    m_bookmarkList = lv_list_create(m_bookmarkManager);
    lv_obj_set_size(m_bookmarkList, LV_PCT(100), LV_PCT(90));
    lv_obj_align_to(m_bookmarkList, m_bookmarkCount, LV_ALIGN_OUT_BOTTOM_MID, -10, 5);
    lv_obj_set_style_bg_color(m_bookmarkList, lv_color_hex(0x1E1E1E), 0);
    
    // Populate bookmark list
    for (const auto& bookmark : m_bookmarks) {
        lv_obj_t* btn = lv_list_add_btn(m_bookmarkList, LV_SYMBOL_LIST, bookmark.title.c_str());
        lv_obj_set_user_data(btn, (void*)bookmark.url.c_str());
        lv_obj_add_event_cb(btn, articleListCallback, LV_EVENT_CLICKED, this);
    }
}

void ZimReaderApp::switchViewMode(ViewMode mode) {
    m_viewMode = mode;
    
    // Hide all content areas
    lv_obj_add_flag(m_fileBrowser, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(m_articleList, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(m_articleReader, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(m_searchInterface, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(m_bookmarkManager, LV_OBJ_FLAG_HIDDEN);
    
    // Reset button colors
    for (int i = 0; i < 5; i++) {
        lv_obj_set_style_bg_color(m_viewModeButtons[i], lv_color_hex(0x34495E), 0);
    }
    
    // Show selected area and highlight button
    switch (mode) {
        case ViewMode::FILE_BROWSER:
            lv_obj_clear_flag(m_fileBrowser, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_bg_color(m_viewModeButtons[0], lv_color_hex(0x3498DB), 0);
            break;
        case ViewMode::ARTICLE_LIST:
            lv_obj_clear_flag(m_articleList, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_bg_color(m_viewModeButtons[1], lv_color_hex(0x3498DB), 0);
            break;
        case ViewMode::ARTICLE_READER:
            lv_obj_clear_flag(m_articleReader, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_bg_color(m_viewModeButtons[2], lv_color_hex(0x3498DB), 0);
            break;
        case ViewMode::SEARCH_RESULTS:
            lv_obj_clear_flag(m_searchInterface, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_bg_color(m_viewModeButtons[3], lv_color_hex(0x3498DB), 0);
            break;
        case ViewMode::BOOKMARKS:
            lv_obj_clear_flag(m_bookmarkManager, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_bg_color(m_viewModeButtons[4], lv_color_hex(0x3498DB), 0);
            break;
    }
}

std::vector<std::string> ZimReaderApp::scanForZimFiles() {
    std::vector<std::string> files;
    
    // Mock implementation - in real system would scan filesystem
    // For demo purposes, add some example files
    files.push_back("/sdcard/zim/wikipedia_simple.zim");
    files.push_back("/sdcard/zim/wiktionary_en.zim");
    files.push_back("/sdcard/zim/gutenberg_books.zim");
    
    ESP_LOGI(TAG, "Found %d ZIM files", files.size());
    return files;
}

bool ZimReaderApp::openZimFile(const std::string& filePath) {
    ESP_LOGI(TAG, "Opening ZIM file: %s", filePath.c_str());
    
    closeZimFile();
    
    m_zimFileHandle = fopen(filePath.c_str(), "rb");
    if (!m_zimFileHandle) {
        ESP_LOGE(TAG, "Failed to open ZIM file: %s", filePath.c_str());
        showError("Failed to open ZIM file");
        return false;
    }
    
    // Get file size
    fseek(m_zimFileHandle, 0, SEEK_END);
    m_fileSize = ftell(m_zimFileHandle);
    fseek(m_zimFileHandle, 0, SEEK_SET);
    
    m_currentZimFile = filePath;
    m_readerState = ZimReaderState::LOADING;
    
    // Parse header (simplified)
    if (!parseZimHeader()) {
        closeZimFile();
        showError("Invalid ZIM file format");
        return false;
    }
    
    m_fileLoaded = true;
    m_readerState = ZimReaderState::READY;
    
    ESP_LOGI(TAG, "ZIM file opened successfully - %d articles", m_zimHeader.articleCount);
    
    // Update UI
    char infoStr[256];
    snprintf(infoStr, sizeof(infoStr), "Loaded: %s (%d articles)", 
             getFileName(filePath).c_str(), m_zimHeader.articleCount);
    lv_label_set_text(m_fileInfoLabel, infoStr);
    
    // Load article list
    loadArticleList();
    
    return true;
}

void ZimReaderApp::closeZimFile() {
    if (m_zimFileHandle) {
        fclose(m_zimFileHandle);
        m_zimFileHandle = nullptr;
    }
    
    m_fileLoaded = false;
    m_readerState = ZimReaderState::NO_FILE;
    m_currentZimFile.clear();
    m_articles.clear();
    m_navigationHistory.clear();
    m_historyIndex = -1;
}

bool ZimReaderApp::parseZimHeader() {
    if (!m_zimFileHandle) return false;
    
    // Simplified ZIM header parsing
    // In real implementation would parse actual ZIM format
    m_zimHeader.version = 6;
    m_zimHeader.articleCount = 1000; // Mock data
    m_zimHeader.clusterCount = 100;
    m_zimHeader.mainPage = "Main_Page";
    
    return true;
}

void ZimReaderApp::loadArticleList() {
    if (!m_fileLoaded) return;
    
    // Mock article loading - in real implementation would read from ZIM file
    m_articles.clear();
    
    // Add some sample articles
    ZimArticle article1;
    article1.url = "A/Article_1";
    article1.title = "Sample Article 1";
    article1.namespace_char = "A";
    article1.mime_type = "text/html";
    article1.content = "This is the content of sample article 1.";
    m_articles.push_back(article1);
    
    ZimArticle article2;
    article2.url = "A/Article_2"; 
    article2.title = "Sample Article 2";
    article2.namespace_char = "A";
    article2.mime_type = "text/html";
    article2.content = "This is the content of sample article 2.";
    m_articles.push_back(article2);
    
    // Update article list UI
    lv_obj_clean(m_articleListView);
    for (const auto& article : m_articles) {
        lv_obj_t* btn = lv_list_add_btn(m_articleListView, LV_SYMBOL_FILE, article.title.c_str());
        lv_obj_set_user_data(btn, (void*)article.url.c_str());
        lv_obj_add_event_cb(btn, articleListCallback, LV_EVENT_CLICKED, this);
    }
    
    // Update count
    char countStr[64];
    snprintf(countStr, sizeof(countStr), "Articles: %d", (int)m_articles.size());
    lv_label_set_text(m_articleCountLabel, countStr);
}

std::string ZimReaderApp::getFileName(const std::string& path) {
    size_t pos = path.find_last_of("/\\");
    return (pos == std::string::npos) ? path : path.substr(pos + 1);
}

void ZimReaderApp::navigateToArticle(const std::string& url) {
    ESP_LOGI(TAG, "Navigating to article: %s", url.c_str());
    
    // Find article
    ZimArticle article;
    bool found = false;
    for (const auto& a : m_articles) {
        if (a.url == url) {
            article = a;
            found = true;
            break;
        }
    }
    
    if (!found) {
        showError("Article not found");
        return;
    }
    
    // Update navigation history
    if (m_historyIndex >= 0 && m_historyIndex < (int)m_navigationHistory.size() - 1) {
        m_navigationHistory.erase(m_navigationHistory.begin() + m_historyIndex + 1, 
                                 m_navigationHistory.end());
    }
    
    m_navigationHistory.push_back(url);
    m_historyIndex = m_navigationHistory.size() - 1;
    
    // Limit history size
    if (m_navigationHistory.size() > MAX_HISTORY_SIZE) {
        m_navigationHistory.erase(m_navigationHistory.begin());
        m_historyIndex--;
    }
    
    m_currentArticleUrl = url;
    
    // Update UI
    lv_label_set_text(m_articleTitle, article.title.c_str());
    lv_label_set_text(m_articleContent, article.content.c_str());
    
    // Switch to reader view
    switchViewMode(ViewMode::ARTICLE_READER);
    updateStatusBar();
}

void ZimReaderApp::updateStatusBar() {
    if (!m_statusLabel) return;
    
    std::string status;
    switch (m_readerState) {
        case ZimReaderState::NO_FILE:
            status = "No file loaded";
            break;
        case ZimReaderState::LOADING:
            status = "Loading...";
            break;
        case ZimReaderState::READY:
            if (!m_currentZimFile.empty()) {
                status = getFileName(m_currentZimFile) + " ready";
            } else {
                status = "Ready";
            }
            break;
        case ZimReaderState::ERROR:
            status = "Error";
            break;
        case ZimReaderState::SEARCHING:
            status = "Searching...";
            break;
    }
    
    lv_label_set_text(m_statusLabel, status.c_str());
}

void ZimReaderApp::showError(const std::string& message) {
    ESP_LOGE(TAG, "Error: %s", message.c_str());
    lv_label_set_text(m_statusLabel, message.c_str());
    lv_obj_set_style_text_color(m_statusLabel, lv_color_hex(0xE74C3C), 0);
}

void ZimReaderApp::loadBookmarks() {
    // Mock bookmark loading
    m_bookmarks.clear();
    
    Bookmark bookmark1;
    bookmark1.title = "Sample Bookmark 1";
    bookmark1.url = "A/Article_1";
    bookmark1.zimFile = m_currentZimFile;
    bookmark1.timestamp = time(nullptr);
    m_bookmarks.push_back(bookmark1);
}

void ZimReaderApp::saveBookmarks() {
    // Mock bookmark saving
    ESP_LOGI(TAG, "Saving %d bookmarks", (int)m_bookmarks.size());
}

// Event handlers
void ZimReaderApp::fileListCallback(lv_event_t* e) {
    ZimReaderApp* app = static_cast<ZimReaderApp*>(lv_event_get_user_data(e));
    lv_obj_t* btn = lv_event_get_target(e);
    const char* filePath = (const char*)lv_obj_get_user_data(btn);
    
    if (filePath) {
        app->openZimFile(filePath);
        app->switchViewMode(ViewMode::ARTICLE_LIST);
    }
}

void ZimReaderApp::articleListCallback(lv_event_t* e) {
    ZimReaderApp* app = static_cast<ZimReaderApp*>(lv_event_get_user_data(e));
    lv_obj_t* btn = lv_event_get_target(e);
    const char* articleUrl = (const char*)lv_obj_get_user_data(btn);
    
    if (articleUrl) {
        app->navigateToArticle(articleUrl);
    }
}

void ZimReaderApp::viewModeCallback(lv_event_t* e) {
    ZimReaderApp* app = static_cast<ZimReaderApp*>(lv_event_get_user_data(e));
    lv_obj_t* btn = lv_event_get_target(e);
    int mode = (int)(intptr_t)lv_obj_get_user_data(btn);
    
    app->switchViewMode(static_cast<ViewMode>(mode));
}

void ZimReaderApp::backButtonCallback(lv_event_t* e) {
    ZimReaderApp* app = static_cast<ZimReaderApp*>(lv_event_get_user_data(e));
    app->navigateBack();
}

void ZimReaderApp::forwardButtonCallback(lv_event_t* e) {
    ZimReaderApp* app = static_cast<ZimReaderApp*>(lv_event_get_user_data(e));
    app->navigateForward();
}

void ZimReaderApp::homeButtonCallback(lv_event_t* e) {
    ZimReaderApp* app = static_cast<ZimReaderApp*>(lv_event_get_user_data(e));
    app->goToMainPage();
}

void ZimReaderApp::bookmarkButtonCallback(lv_event_t* e) {
    ZimReaderApp* app = static_cast<ZimReaderApp*>(lv_event_get_user_data(e));
    app->switchViewMode(ViewMode::BOOKMARKS);
}

void ZimReaderApp::refreshCallback(lv_event_t* e) {
    ZimReaderApp* app = static_cast<ZimReaderApp*>(lv_event_get_user_data(e));
    app->m_zimFiles = app->scanForZimFiles();
    app->updateStatusBar();
}

void ZimReaderApp::searchButtonCallback(lv_event_t* e) {
    ZimReaderApp* app = static_cast<ZimReaderApp*>(lv_event_get_user_data(e));
    if (app->m_searchInput) {
        std::string query = lv_textarea_get_text(app->m_searchInput);
        app->performSearch(query);
    }
}

void ZimReaderApp::searchInputCallback(lv_event_t* e) {
    ZimReaderApp* app = static_cast<ZimReaderApp*>(lv_event_get_user_data(e));
    if (app->m_searchInput) {
        std::string query = lv_textarea_get_text(app->m_searchInput);
        app->performSearch(query);
    }
}

void ZimReaderApp::navigateBack() {
    if (m_historyIndex > 0) {
        m_historyIndex--;
        navigateToArticle(m_navigationHistory[m_historyIndex]);
    }
}

void ZimReaderApp::navigateForward() {
    if (m_historyIndex < (int)m_navigationHistory.size() - 1) {
        m_historyIndex++;
        navigateToArticle(m_navigationHistory[m_historyIndex]);
    }
}

void ZimReaderApp::goToMainPage() {
    if (!m_zimHeader.mainPage.empty()) {
        navigateToArticle(m_zimHeader.mainPage);
    } else {
        switchViewMode(ViewMode::ARTICLE_LIST);
    }
}

void ZimReaderApp::performSearch(const std::string& query) {
    if (query.empty() || !m_fileLoaded) return;
    
    ESP_LOGI(TAG, "Searching for: %s", query.c_str());
    
    m_currentSearchQuery = query;
    m_readerState = ZimReaderState::SEARCHING;
    updateStatusBar();
    
    // Simple search implementation
    m_searchResultsData.clear();
    for (const auto& article : m_articles) {
        if (article.title.find(query) != std::string::npos ||
            article.content.find(query) != std::string::npos) {
            m_searchResultsData.push_back(article);
        }
    }
    
    // Update search results UI
    lv_obj_clean(m_searchResults);
    for (const auto& result : m_searchResultsData) {
        lv_obj_t* btn = lv_list_add_btn(m_searchResults, LV_SYMBOL_FILE, result.title.c_str());
        lv_obj_set_user_data(btn, (void*)result.url.c_str());
        lv_obj_add_event_cb(btn, articleListCallback, LV_EVENT_CLICKED, this);
    }
    
    // Update status
    char statusStr[128];
    snprintf(statusStr, sizeof(statusStr), "Found %d results for '%s'", 
             (int)m_searchResultsData.size(), query.c_str());
    lv_label_set_text(m_searchStatus, statusStr);
    
    m_readerState = ZimReaderState::READY;
    updateStatusBar();
}