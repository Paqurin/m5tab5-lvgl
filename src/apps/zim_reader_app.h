#ifndef ZIM_READER_APP_H
#define ZIM_READER_APP_H

#include "base_app.h"
#include <vector>
#include <string>
#include <map>
#include <memory>

/**
 * @file zim_reader_app.h
 * @brief ZIM file reader application for M5Stack Tab5
 * 
 * Features offline content reading from ZIM files:
 * - Browse Wikipedia and other ZIM archives offline
 * - Search functionality for articles
 * - Article rendering with basic HTML support
 * - Bookmark management
 * - File management and selection
 * - Navigation history
 * - Table of contents support
 * - Image display (if supported in ZIM)
 */

// ZIM file structures (simplified)
struct ZimHeader {
    uint32_t version;
    uint32_t articleCount;
    uint32_t clusterCount;
    uint64_t checksumPos;
    uint64_t titlePtrPos;
    uint64_t urlPtrPos;
    uint64_t mimeListPos;
    std::string mainPage;
    std::string layout;
};

struct ZimArticle {
    std::string url;
    std::string title;
    std::string namespace_char;
    uint32_t cluster;
    uint32_t blob;
    std::string mime_type;
    std::string content;
    bool redirect;
    std::string redirect_url;
};

struct Bookmark {
    std::string title;
    std::string url;
    std::string zimFile;
    time_t timestamp;
};

enum class ViewMode {
    FILE_BROWSER,
    ARTICLE_LIST,
    ARTICLE_READER,
    SEARCH_RESULTS,
    BOOKMARKS
};

enum class ZimReaderState {
    NO_FILE,
    LOADING,
    READY,
    ERROR,
    SEARCHING
};

class ZimReaderApp : public BaseApp {
public:
    ZimReaderApp();
    ~ZimReaderApp() override;

    os_error_t initialize() override;
    os_error_t update(uint32_t deltaTime) override;
    os_error_t shutdown() override;
    os_error_t createUI(lv_obj_t* parent) override;
    os_error_t destroyUI() override;

private:
    // File operations
    bool openZimFile(const std::string& filePath);
    void closeZimFile();
    bool parseZimHeader();
    void loadArticleList();
    std::vector<std::string> scanForZimFiles();
    
    // Article operations
    ZimArticle getArticle(const std::string& url);
    ZimArticle getArticleByIndex(uint32_t index);
    std::string getArticleContent(uint32_t cluster, uint32_t blob);
    std::vector<ZimArticle> searchArticles(const std::string& query);
    void renderArticleContent(const std::string& content);
    
    // Navigation
    void navigateToArticle(const std::string& url);
    void navigateBack();
    void navigateForward();
    void goToMainPage();
    void showTableOfContents();
    
    // UI management
    void createFileBrowser();
    void createArticleList();
    void createArticleReader();
    void createSearchInterface();
    void createBookmarkManager();
    void createNavigationBar();
    void createStatusBar();
    void updateStatusBar();
    void switchViewMode(ViewMode mode);
    
    // Search functionality
    void performSearch(const std::string& query);
    void displaySearchResults();
    void clearSearch();
    
    // Bookmark management
    void addBookmark(const ZimArticle& article);
    void removeBookmark(const std::string& url);
    void loadBookmarks();
    void saveBookmarks();
    bool isBookmarked(const std::string& url);
    
    // Content rendering
    void renderPlainText(const std::string& text);
    void renderBasicHtml(const std::string& html);
    std::string stripHtmlTags(const std::string& html);
    std::string extractLinks(const std::string& html);
    void handleLinkClick(const std::string& url);
    
    // Utility functions
    std::string formatFileSize(uint64_t bytes);
    std::string getFileName(const std::string& path);
    std::string extractTitle(const std::string& content);
    void showError(const std::string& message);
    void showLoading(const std::string& message);
    void hideLoading();
    
    // Event handlers
    static void fileListCallback(lv_event_t* e);
    static void articleListCallback(lv_event_t* e);
    static void searchButtonCallback(lv_event_t* e);
    static void searchInputCallback(lv_event_t* e);
    static void backButtonCallback(lv_event_t* e);
    static void forwardButtonCallback(lv_event_t* e);
    static void homeButtonCallback(lv_event_t* e);
    static void bookmarkButtonCallback(lv_event_t* e);
    static void tocButtonCallback(lv_event_t* e);
    static void viewModeCallback(lv_event_t* e);
    static void refreshCallback(lv_event_t* e);
    
    // Data members
    ZimHeader m_zimHeader;
    std::vector<ZimArticle> m_articles;
    std::vector<ZimArticle> m_searchResultsData;
    std::vector<Bookmark> m_bookmarks;
    std::vector<std::string> m_zimFiles;
    std::vector<std::string> m_navigationHistory;
    
    // State
    ViewMode m_viewMode;
    ZimReaderState m_readerState;
    std::string m_currentZimFile;
    std::string m_currentArticleUrl;
    std::string m_currentSearchQuery;
    int m_historyIndex;
    bool m_fileLoaded;
    
    // File handling
    FILE* m_zimFileHandle;
    uint64_t m_fileSize;
    
    // UI elements
    lv_obj_t* m_navigationBar = nullptr;
    lv_obj_t* m_statusBar = nullptr;
    lv_obj_t* m_contentArea = nullptr;
    
    // Navigation buttons
    lv_obj_t* m_backButton = nullptr;
    lv_obj_t* m_forwardButton = nullptr;
    lv_obj_t* m_homeButton = nullptr;
    lv_obj_t* m_bookmarkButton = nullptr;
    lv_obj_t* m_tocButton = nullptr;
    lv_obj_t* m_refreshButton = nullptr;
    
    // View mode buttons
    lv_obj_t* m_viewModeButtons[5] = {nullptr};
    
    // Status labels
    lv_obj_t* m_statusLabel = nullptr;
    lv_obj_t* m_progressBar = nullptr;
    
    // Content areas for different modes
    lv_obj_t* m_fileBrowser = nullptr;
    lv_obj_t* m_articleList = nullptr;
    lv_obj_t* m_articleReader = nullptr;
    lv_obj_t* m_searchInterface = nullptr;
    lv_obj_t* m_bookmarkManager = nullptr;
    
    // File browser
    lv_obj_t* m_fileList = nullptr;
    lv_obj_t* m_fileInfoLabel = nullptr;
    
    // Article list
    lv_obj_t* m_articleListView = nullptr;
    lv_obj_t* m_articleCountLabel = nullptr;
    
    // Article reader
    lv_obj_t* m_articleTitle = nullptr;
    lv_obj_t* m_articleContent = nullptr;
    lv_obj_t* m_articleScrollView = nullptr;
    
    // Search interface
    lv_obj_t* m_searchInput = nullptr;
    lv_obj_t* m_searchButton = nullptr;
    lv_obj_t* m_searchResults = nullptr;
    lv_obj_t* m_searchStatus = nullptr;
    
    // Bookmark manager
    lv_obj_t* m_bookmarkList = nullptr;
    lv_obj_t* m_bookmarkCount = nullptr;
    
    // Loading overlay
    lv_obj_t* m_loadingOverlay = nullptr;
    lv_obj_t* m_loadingSpinner = nullptr;
    lv_obj_t* m_loadingLabel = nullptr;
    
    // Configuration
    static constexpr size_t MAX_ARTICLE_CACHE = 50;
    static constexpr size_t MAX_SEARCH_RESULTS = 100;
    static constexpr size_t MAX_HISTORY_SIZE = 50;
    static constexpr size_t MAX_BOOKMARKS = 200;
    static constexpr size_t CONTENT_BUFFER_SIZE = 32768; // 32KB content buffer
    
    // File paths
    static constexpr const char* ZIM_DIRECTORY = "/sdcard/zim";
    static constexpr const char* BOOKMARKS_FILE = "/sdcard/zim_bookmarks.json";
    static constexpr const char* SETTINGS_FILE = "/sdcard/zim_settings.json";
};

#endif // ZIM_READER_APP_H