#include "tetris_game_app.h"
#include "../system/os_manager.h"
#include <esp_log.h>
#include <algorithm>
#include <random>
#include <cstring>

static const char* TAG = "DroppinBlocks";

// Tetromino rotation and validation
void Tetromino::rotateClockwise() {
    rotation = (rotation + 1) % 4;
}

void Tetromino::rotateCounterClockwise() {
    rotation = (rotation + 3) % 4;
}

bool Tetromino::isValidPosition(const std::array<std::array<Block, 10>, 20>& grid, int testX, int testY, int testRotation) const {
    int rot = (testRotation == -1) ? rotation : testRotation;
    
    for (int py = 0; py < 4; py++) {
        for (int px = 0; px < 4; px++) {
            if (shape[py][px]) {
                int gridX = testX + px;
                int gridY = testY + py;
                
                // Check bounds
                if (gridX < 0 || gridX >= TetrisGameApp::GRID_WIDTH || gridY >= TetrisGameApp::GRID_HEIGHT) {
                    return false;
                }
                
                // Check collision with existing blocks (ignore negative Y for spawning)
                if (gridY >= 0 && grid[gridY][gridX].filled) {
                    return false;
                }
            }
        }
    }
    return true;
}

TetrisGameApp::TetrisGameApp() : BaseApp("droppin_blocks", "Droppin Blocks", "1.0") {
    m_gameState = GameState::PLAYING;
    m_score = 0;
    m_lines = 0;
    m_level = 0;
    m_gameTime = 0;
    m_lastDrop = 0;
    m_dropInterval = BASE_DROP_INTERVAL;
    m_lockDelay = 0;
    m_lineClearDelay = 0;
    
    m_lastTouchX = 0;
    m_lastTouchY = 0;
    m_lastTouchTime = 0;
    m_touchActive = false;
    
    // Initialize grid
    for (auto& row : m_grid) {
        for (auto& block : row) {
            block = Block();
        }
    }
    
    // Initialize statistics
    m_pieceCount.fill(0);
    m_singleLines = 0;
    m_doubleLines = 0;
    m_tripleLines = 0;
    m_tetrisLines = 0;
    
    initializeTetrominoes();
}

TetrisGameApp::~TetrisGameApp() {
    shutdown();
}

os_error_t TetrisGameApp::initialize() {
    if (m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Initializing Droppin Blocks Game");
    
    newGame();
    
    m_initialized = true;
    ESP_LOGI(TAG, "Droppin Blocks Game initialized");
    return OS_OK;
}

os_error_t TetrisGameApp::update(uint32_t deltaTime) {
    if (!m_initialized) {
        return OS_ERROR_NOT_INITIALIZED;
    }
    
    if (m_gameState == GameState::PLAYING) {
        m_gameTime += deltaTime;
        
        // Handle automatic piece dropping
        uint32_t currentTime = millis();
        if (currentTime - m_lastDrop > m_dropInterval) {
            gameStep();
            m_lastDrop = currentTime;
        }
        
        // Update info display periodically
        static uint32_t lastInfoUpdate = 0;
        if (currentTime - lastInfoUpdate > 500) {
            updateInfoDisplay();
            lastInfoUpdate = currentTime;
        }
    }
    else if (m_gameState == GameState::LINE_CLEARING) {
        // Handle line clearing animation delay
        if (millis() - m_lineClearDelay > LINE_CLEAR_DELAY) {
            // Clear the marked lines and drop above lines
            std::vector<int> linesToClear;
            for (int y = 0; y < GRID_HEIGHT; y++) {
                bool fullLine = true;
                for (int x = 0; x < GRID_WIDTH; x++) {
                    if (!m_grid[y][x].filled) {
                        fullLine = false;
                        break;
                    }
                }
                if (fullLine) {
                    linesToClear.push_back(y);
                }
            }
            
            if (!linesToClear.empty()) {
                clearLines(linesToClear);
                updateScore(linesToClear.size());
                updateLevel();
            }
            
            m_gameState = GameState::PLAYING;
            spawnNewPiece();
            updateGridDisplay();
        }
    }
    
    return OS_OK;
}

os_error_t TetrisGameApp::shutdown() {
    if (!m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Shutting down Droppin Blocks Game");
    destroyUI();
    
    m_initialized = false;
    return OS_OK;
}

os_error_t TetrisGameApp::createUI(lv_obj_t* parent) {
    if (!parent) {
        return OS_ERROR_INVALID_PARAM;
    }

    m_uiContainer = lv_obj_create(parent);
    lv_obj_set_size(m_uiContainer, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(m_uiContainer, lv_color_hex(0x222222), 0);
    lv_obj_clear_flag(m_uiContainer, LV_OBJ_FLAG_SCROLLABLE);

    createGameGrid();
    createInfoPanel();
    createControlButtons();
    updateGridDisplay();
    updateInfoDisplay();

    ESP_LOGI(TAG, "Droppin Blocks UI created");
    return OS_OK;
}

os_error_t TetrisGameApp::destroyUI() {
    if (m_uiContainer) {
        lv_obj_del(m_uiContainer);
        m_uiContainer = nullptr;
        m_gameGrid = nullptr;
        m_infoPanel = nullptr;
        m_controlPanel = nullptr;
        
        // Clear grid cell references
        for (auto& row : m_gridCells) {
            row.fill(nullptr);
        }
        
        // Clear button references
        m_scoreLabel = nullptr;
        m_linesLabel = nullptr;
        m_levelLabel = nullptr;
        m_timeLabel = nullptr;
        m_nextPieceDisplay = nullptr;
        m_pauseButton = nullptr;
        m_newGameButton = nullptr;
        m_leftButton = nullptr;
        m_rightButton = nullptr;
        m_rotateButton = nullptr;
        m_dropButton = nullptr;
    }
    return OS_OK;
}

void TetrisGameApp::newGame() {
    ESP_LOGI(TAG, "Starting new Droppin Blocks game");
    
    // Clear game grid
    for (auto& row : m_grid) {
        for (auto& block : row) {
            block = Block();
        }
    }
    
    // Reset game state
    m_gameState = GameState::PLAYING;
    m_score = 0;
    m_lines = 0;
    m_level = 0;
    m_gameTime = 0;
    m_lastDrop = millis();
    m_dropInterval = BASE_DROP_INTERVAL;
    m_lockDelay = 0;
    m_lineClearDelay = 0;
    
    // Reset statistics
    m_pieceCount.fill(0);
    m_singleLines = 0;
    m_doubleLines = 0;
    m_tripleLines = 0;
    m_tetrisLines = 0;
    
    // Reset touch state
    m_lastTouchX = 0;
    m_lastTouchY = 0;
    m_lastTouchTime = 0;
    m_touchActive = false;
    
    // Spawn first pieces
    m_nextPiece = createTetromino(getRandomTetrominoType());
    spawnNewPiece();
    
    updateGridDisplay();
    updateInfoDisplay();
}

void TetrisGameApp::gameStep() {
    if (m_gameState != GameState::PLAYING) return;
    
    // Try to move current piece down
    if (canMovePiece(0, 1)) {
        movePiece(0, 1);
    } else {
        // Piece can't move down - start lock delay or lock immediately
        if (m_lockDelay == 0) {
            m_lockDelay = millis();
        } else if (millis() - m_lockDelay > LOCK_DELAY_TIME) {
            lockPiece();
        }
    }
}

void TetrisGameApp::spawnNewPiece() {
    m_currentPiece = m_nextPiece;
    m_currentPiece.x = GRID_WIDTH / 2 - 2;
    m_currentPiece.y = -1; // Start above the grid
    m_currentPiece.rotation = 0;
    
    m_nextPiece = createTetromino(getRandomTetrominoType());
    m_pieceCount[static_cast<int>(m_currentPiece.type) - 1]++;
    
    // Check for game over
    if (!m_currentPiece.isValidPosition(m_grid, m_currentPiece.x, m_currentPiece.y)) {
        m_gameState = GameState::GAME_OVER;
        endGame();
    }
}

void TetrisGameApp::movePiece(int deltaX, int deltaY) {
    if (m_currentPiece.isValidPosition(m_grid, m_currentPiece.x + deltaX, m_currentPiece.y + deltaY)) {
        clearTetrominoFromDisplay(m_currentPiece);
        m_currentPiece.x += deltaX;
        m_currentPiece.y += deltaY;
        m_lockDelay = 0; // Reset lock delay on successful move
        updateGridDisplay();
    }
}

void TetrisGameApp::rotatePiece(bool clockwise) {
    int oldRotation = m_currentPiece.rotation;
    
    if (clockwise) {
        m_currentPiece.rotateClockwise();
    } else {
        m_currentPiece.rotateCounterClockwise();
    }
    
    // Try basic rotation first
    if (m_currentPiece.isValidPosition(m_grid, m_currentPiece.x, m_currentPiece.y)) {
        clearTetrominoFromDisplay(m_currentPiece);
        m_lockDelay = 0; // Reset lock delay on successful rotation
        updateGridDisplay();
        return;
    }
    
    // Try wall kicks
    static const int wallKicks[][4][2] = {
        {{0, 0}, {-1, 0}, {-1, 1}, {0, -2}},  // 0->1
        {{0, 0}, {1, 0}, {1, -1}, {0, 2}},    // 1->2
        {{0, 0}, {1, 0}, {1, 1}, {0, -2}},    // 2->3
        {{0, 0}, {-1, 0}, {-1, -1}, {0, 2}}   // 3->0
    };
    
    int kickIndex = clockwise ? oldRotation : (oldRotation + 3) % 4;
    
    for (int i = 0; i < 4; i++) {
        int kickX = wallKicks[kickIndex][i][0];
        int kickY = wallKicks[kickIndex][i][1];
        
        if (m_currentPiece.isValidPosition(m_grid, m_currentPiece.x + kickX, m_currentPiece.y + kickY)) {
            clearTetrominoFromDisplay(m_currentPiece);
            m_currentPiece.x += kickX;
            m_currentPiece.y += kickY;
            m_lockDelay = 0;
            updateGridDisplay();
            return;
        }
    }
    
    // Rotation failed - revert
    m_currentPiece.rotation = oldRotation;
}

void TetrisGameApp::dropPiece() {
    while (canMovePiece(0, 1)) {
        movePiece(0, 1);
        m_score += SCORE_HARD_DROP;
    }
    lockPiece();
}

void TetrisGameApp::lockPiece() {
    // Add current piece to grid
    for (int py = 0; py < 4; py++) {
        for (int px = 0; px < 4; px++) {
            if (m_currentPiece.shape[py][px]) {
                int gridX = m_currentPiece.x + px;
                int gridY = m_currentPiece.y + py;
                
                if (gridX >= 0 && gridX < GRID_WIDTH && gridY >= 0 && gridY < GRID_HEIGHT) {
                    m_grid[gridY][gridX] = Block(m_currentPiece.type, m_currentPiece.color);
                }
            }
        }
    }
    
    m_lockDelay = 0;
    checkForLines();
}

bool TetrisGameApp::canMovePiece(int deltaX, int deltaY) const {
    return m_currentPiece.isValidPosition(m_grid, m_currentPiece.x + deltaX, m_currentPiece.y + deltaY);
}

bool TetrisGameApp::canRotatePiece(bool clockwise) const {
    Tetromino testPiece = m_currentPiece;
    if (clockwise) {
        testPiece.rotateClockwise();
    } else {
        testPiece.rotateCounterClockwise();
    }
    return testPiece.isValidPosition(m_grid, testPiece.x, testPiece.y);
}

void TetrisGameApp::checkForLines() {
    std::vector<int> fullLines;
    
    for (int y = GRID_HEIGHT - 1; y >= 0; y--) {
        bool fullLine = true;
        for (int x = 0; x < GRID_WIDTH; x++) {
            if (!m_grid[y][x].filled) {
                fullLine = false;
                break;
            }
        }
        if (fullLine) {
            fullLines.push_back(y);
        }
    }
    
    if (!fullLines.empty()) {
        m_gameState = GameState::LINE_CLEARING;
        m_lineClearDelay = millis();
        
        // Update line statistics
        switch (fullLines.size()) {
            case 1: m_singleLines++; break;
            case 2: m_doubleLines++; break;
            case 3: m_tripleLines++; break;
            case 4: m_tetrisLines++; break;
        }
    } else {
        spawnNewPiece();
    }
    
    updateGridDisplay();
}

void TetrisGameApp::clearLines(const std::vector<int>& linesToClear) {
    // Clear lines from bottom to top
    for (auto it = linesToClear.rbegin(); it != linesToClear.rend(); ++it) {
        int lineY = *it;
        
        // Move all lines above down by one
        for (int y = lineY; y > 0; y--) {
            for (int x = 0; x < GRID_WIDTH; x++) {
                m_grid[y][x] = m_grid[y - 1][x];
            }
        }
        
        // Clear top line
        for (int x = 0; x < GRID_WIDTH; x++) {
            m_grid[0][x] = Block();
        }
    }
    
    m_lines += linesToClear.size();
}

void TetrisGameApp::dropLinesAbove(int clearedLine) {
    for (int y = clearedLine; y > 0; y--) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            m_grid[y][x] = m_grid[y - 1][x];
        }
    }
    
    // Clear top line
    for (int x = 0; x < GRID_WIDTH; x++) {
        m_grid[0][x] = Block();
    }
}

int TetrisGameApp::countFullLines() const {
    int count = 0;
    for (int y = 0; y < GRID_HEIGHT; y++) {
        bool fullLine = true;
        for (int x = 0; x < GRID_WIDTH; x++) {
            if (!m_grid[y][x].filled) {
                fullLine = false;
                break;
            }
        }
        if (fullLine) count++;
    }
    return count;
}

void TetrisGameApp::initializeTetrominoes() {
    // I piece (line)
    m_tetrominoTemplates[0].type = TetrominoType::I;
    m_tetrominoTemplates[0].color = COLOR_I;
    for (auto& row : m_tetrominoTemplates[0].shape) row.fill(false);
    m_tetrominoTemplates[0].shape[1][0] = true;
    m_tetrominoTemplates[0].shape[1][1] = true;
    m_tetrominoTemplates[0].shape[1][2] = true;
    m_tetrominoTemplates[0].shape[1][3] = true;
    
    // O piece (square)
    m_tetrominoTemplates[1].type = TetrominoType::O;
    m_tetrominoTemplates[1].color = COLOR_O;
    for (auto& row : m_tetrominoTemplates[1].shape) row.fill(false);
    m_tetrominoTemplates[1].shape[0][1] = true;
    m_tetrominoTemplates[1].shape[0][2] = true;
    m_tetrominoTemplates[1].shape[1][1] = true;
    m_tetrominoTemplates[1].shape[1][2] = true;
    
    // T piece
    m_tetrominoTemplates[2].type = TetrominoType::T;
    m_tetrominoTemplates[2].color = COLOR_T;
    for (auto& row : m_tetrominoTemplates[2].shape) row.fill(false);
    m_tetrominoTemplates[2].shape[0][1] = true;
    m_tetrominoTemplates[2].shape[1][0] = true;
    m_tetrominoTemplates[2].shape[1][1] = true;
    m_tetrominoTemplates[2].shape[1][2] = true;
    
    // S piece
    m_tetrominoTemplates[3].type = TetrominoType::S;
    m_tetrominoTemplates[3].color = COLOR_S;
    for (auto& row : m_tetrominoTemplates[3].shape) row.fill(false);
    m_tetrominoTemplates[3].shape[0][1] = true;
    m_tetrominoTemplates[3].shape[0][2] = true;
    m_tetrominoTemplates[3].shape[1][0] = true;
    m_tetrominoTemplates[3].shape[1][1] = true;
    
    // Z piece
    m_tetrominoTemplates[4].type = TetrominoType::Z;
    m_tetrominoTemplates[4].color = COLOR_Z;
    for (auto& row : m_tetrominoTemplates[4].shape) row.fill(false);
    m_tetrominoTemplates[4].shape[0][0] = true;
    m_tetrominoTemplates[4].shape[0][1] = true;
    m_tetrominoTemplates[4].shape[1][1] = true;
    m_tetrominoTemplates[4].shape[1][2] = true;
    
    // J piece
    m_tetrominoTemplates[5].type = TetrominoType::J;
    m_tetrominoTemplates[5].color = COLOR_J;
    for (auto& row : m_tetrominoTemplates[5].shape) row.fill(false);
    m_tetrominoTemplates[5].shape[0][0] = true;
    m_tetrominoTemplates[5].shape[1][0] = true;
    m_tetrominoTemplates[5].shape[1][1] = true;
    m_tetrominoTemplates[5].shape[1][2] = true;
    
    // L piece
    m_tetrominoTemplates[6].type = TetrominoType::L;
    m_tetrominoTemplates[6].color = COLOR_L;
    for (auto& row : m_tetrominoTemplates[6].shape) row.fill(false);
    m_tetrominoTemplates[6].shape[0][2] = true;
    m_tetrominoTemplates[6].shape[1][0] = true;
    m_tetrominoTemplates[6].shape[1][1] = true;
    m_tetrominoTemplates[6].shape[1][2] = true;
}

Tetromino TetrisGameApp::createTetromino(TetrominoType type) const {
    if (type == TetrominoType::NONE || static_cast<int>(type) < 1 || static_cast<int>(type) > 7) {
        return Tetromino();
    }
    
    return m_tetrominoTemplates[static_cast<int>(type) - 1];
}

TetrominoType TetrisGameApp::getRandomTetrominoType() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(1, 7);
    
    return static_cast<TetrominoType>(dis(gen));
}

uint32_t TetrisGameApp::getTetrominoColor(TetrominoType type) const {
    switch (type) {
        case TetrominoType::I: return COLOR_I;
        case TetrominoType::O: return COLOR_O;
        case TetrominoType::T: return COLOR_T;
        case TetrominoType::S: return COLOR_S;
        case TetrominoType::Z: return COLOR_Z;
        case TetrominoType::J: return COLOR_J;
        case TetrominoType::L: return COLOR_L;
        default: return COLOR_EMPTY;
    }
}

void TetrisGameApp::updateScore(int linesCleared) {
    int baseScore = 0;
    switch (linesCleared) {
        case 1: baseScore = SCORE_SINGLE; break;
        case 2: baseScore = SCORE_DOUBLE; break;
        case 3: baseScore = SCORE_TRIPLE; break;
        case 4: baseScore = SCORE_TETRIS; break;
    }
    
    m_score += baseScore * (m_level + 1);
}

void TetrisGameApp::updateLevel() {
    int newLevel = m_lines / 10;
    if (newLevel != m_level) {
        m_level = newLevel;
        m_dropInterval = std::max(MIN_DROP_INTERVAL, 
                                 BASE_DROP_INTERVAL - (m_level * 50));
    }
}

uint32_t TetrisGameApp::getDropInterval() const {
    return m_dropInterval;
}

void TetrisGameApp::handleTouchInput(int x, int y) {
    uint32_t currentTime = millis();
    
    if (!m_touchActive) {
        m_touchActive = true;
        m_lastTouchX = x;
        m_lastTouchY = y;
        m_lastTouchTime = currentTime;
        return;
    }
    
    // Calculate movement delta
    int deltaX = x - m_lastTouchX;
    int deltaY = y - m_lastTouchY;
    uint32_t deltaTime = currentTime - m_lastTouchTime;
    
    // Handle swipe gestures
    if (deltaTime > 100 && (abs(deltaX) > 20 || abs(deltaY) > 20)) {
        handleSwipeGesture(deltaX, deltaY);
        m_lastTouchX = x;
        m_lastTouchY = y;
        m_lastTouchTime = currentTime;
    }
}

void TetrisGameApp::handleSwipeGesture(int deltaX, int deltaY) {
    if (m_gameState != GameState::PLAYING) return;
    
    if (abs(deltaX) > abs(deltaY)) {
        // Horizontal swipe
        if (deltaX > 0) {
            movePiece(1, 0); // Move right
        } else {
            movePiece(-1, 0); // Move left
        }
    } else {
        // Vertical swipe
        if (deltaY > 0) {
            dropPiece(); // Hard drop
        } else {
            rotatePiece(true); // Rotate
        }
    }
}

void TetrisGameApp::createGameGrid() {
    m_gameGrid = lv_obj_create(m_uiContainer);
    lv_obj_set_size(m_gameGrid, GRID_WIDTH * CELL_SIZE + 2 * GRID_SPACING, 
                    GRID_HEIGHT * CELL_SIZE + 2 * GRID_SPACING);
    lv_obj_align(m_gameGrid, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_set_style_bg_color(m_gameGrid, lv_color_hex(COLOR_EMPTY), 0);
    lv_obj_set_style_border_color(m_gameGrid, lv_color_hex(COLOR_GRID_LINE), 0);
    lv_obj_set_style_border_width(m_gameGrid, 2, 0);
    lv_obj_clear_flag(m_gameGrid, LV_OBJ_FLAG_SCROLLABLE);
    
    // Create grid cells
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            m_gridCells[y][x] = lv_obj_create(m_gameGrid);
            lv_obj_set_size(m_gridCells[y][x], CELL_SIZE, CELL_SIZE);
            lv_obj_set_pos(m_gridCells[y][x], x * CELL_SIZE + GRID_SPACING, 
                          y * CELL_SIZE + GRID_SPACING);
            lv_obj_set_style_bg_color(m_gridCells[y][x], lv_color_hex(COLOR_EMPTY), 0);
            lv_obj_set_style_border_color(m_gridCells[y][x], lv_color_hex(COLOR_GRID_LINE), 0);
            lv_obj_set_style_border_width(m_gridCells[y][x], 1, 0);
            lv_obj_set_style_radius(m_gridCells[y][x], 1, 0);
            
            lv_obj_add_event_cb(m_gridCells[y][x], gameGridCallback, LV_EVENT_CLICKED, this);
        }
    }
}

void TetrisGameApp::createInfoPanel() {
    m_infoPanel = lv_obj_create(m_uiContainer);
    lv_obj_set_size(m_infoPanel, 120, LV_PCT(80));
    lv_obj_align(m_infoPanel, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_set_style_bg_color(m_infoPanel, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_color(m_infoPanel, lv_color_hex(COLOR_GRID_LINE), 0);
    lv_obj_set_style_border_width(m_infoPanel, 1, 0);
    lv_obj_set_style_radius(m_infoPanel, 5, 0);
    
    // Score label
    m_scoreLabel = lv_label_create(m_infoPanel);
    lv_label_set_text(m_scoreLabel, "Score: 0");
    lv_obj_align(m_scoreLabel, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_text_color(m_scoreLabel, lv_color_white(), 0);
    
    // Lines label
    m_linesLabel = lv_label_create(m_infoPanel);
    lv_label_set_text(m_linesLabel, "Lines: 0");
    lv_obj_align(m_linesLabel, LV_ALIGN_TOP_MID, 0, 35);
    lv_obj_set_style_text_color(m_linesLabel, lv_color_white(), 0);
    
    // Level label
    m_levelLabel = lv_label_create(m_infoPanel);
    lv_label_set_text(m_levelLabel, "Level: 0");
    lv_obj_align(m_levelLabel, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_text_color(m_levelLabel, lv_color_white(), 0);
    
    // Time label
    m_timeLabel = lv_label_create(m_infoPanel);
    lv_label_set_text(m_timeLabel, "00:00");
    lv_obj_align(m_timeLabel, LV_ALIGN_TOP_MID, 0, 85);
    lv_obj_set_style_text_color(m_timeLabel, lv_color_white(), 0);
    
    // Next piece display
    m_nextPieceDisplay = lv_obj_create(m_infoPanel);
    lv_obj_set_size(m_nextPieceDisplay, 60, 60);
    lv_obj_align(m_nextPieceDisplay, LV_ALIGN_TOP_MID, 0, 120);
    lv_obj_set_style_bg_color(m_nextPieceDisplay, lv_color_hex(COLOR_EMPTY), 0);
    lv_obj_set_style_border_color(m_nextPieceDisplay, lv_color_hex(COLOR_GRID_LINE), 0);
    lv_obj_set_style_border_width(m_nextPieceDisplay, 1, 0);
    
    lv_obj_t* nextLabel = lv_label_create(m_infoPanel);
    lv_label_set_text(nextLabel, "Next:");
    lv_obj_align(nextLabel, LV_ALIGN_TOP_MID, 0, 105);
    lv_obj_set_style_text_color(nextLabel, lv_color_white(), 0);
}

void TetrisGameApp::createControlButtons() {
    m_controlPanel = lv_obj_create(m_uiContainer);
    lv_obj_set_size(m_controlPanel, LV_PCT(100), 50);
    lv_obj_align(m_controlPanel, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(m_controlPanel, lv_color_hex(0x333333), 0);
    lv_obj_set_style_radius(m_controlPanel, 0, 0);
    
    // Pause button
    m_pauseButton = lv_btn_create(m_controlPanel);
    lv_obj_set_size(m_pauseButton, 60, 30);
    lv_obj_align(m_pauseButton, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_add_event_cb(m_pauseButton, pauseButtonCallback, LV_EVENT_CLICKED, this);
    lv_obj_t* pauseLabel = lv_label_create(m_pauseButton);
    lv_label_set_text(pauseLabel, "Pause");
    lv_obj_center(pauseLabel);
    
    // New Game button
    m_newGameButton = lv_btn_create(m_controlPanel);
    lv_obj_set_size(m_newGameButton, 60, 30);
    lv_obj_align(m_newGameButton, LV_ALIGN_CENTER, -35, 0);
    lv_obj_add_event_cb(m_newGameButton, newGameButtonCallback, LV_EVENT_CLICKED, this);
    lv_obj_t* newGameLabel = lv_label_create(m_newGameButton);
    lv_label_set_text(newGameLabel, "New");
    lv_obj_center(newGameLabel);
    
    // Control buttons
    m_leftButton = lv_btn_create(m_controlPanel);
    lv_obj_set_size(m_leftButton, 40, 30);
    lv_obj_align(m_leftButton, LV_ALIGN_CENTER, 20, 0);
    lv_obj_add_event_cb(m_leftButton, controlButtonCallback, LV_EVENT_CLICKED, this);
    lv_obj_set_user_data(m_leftButton, (void*)1); // Left
    lv_obj_t* leftLabel = lv_label_create(m_leftButton);
    lv_label_set_text(leftLabel, "<");
    lv_obj_center(leftLabel);
    
    m_rightButton = lv_btn_create(m_controlPanel);
    lv_obj_set_size(m_rightButton, 40, 30);
    lv_obj_align(m_rightButton, LV_ALIGN_CENTER, 65, 0);
    lv_obj_add_event_cb(m_rightButton, controlButtonCallback, LV_EVENT_CLICKED, this);
    lv_obj_set_user_data(m_rightButton, (void*)2); // Right
    lv_obj_t* rightLabel = lv_label_create(m_rightButton);
    lv_label_set_text(rightLabel, ">");
    lv_obj_center(rightLabel);
    
    m_rotateButton = lv_btn_create(m_controlPanel);
    lv_obj_set_size(m_rotateButton, 50, 30);
    lv_obj_align(m_rotateButton, LV_ALIGN_RIGHT_MID, -80, 0);
    lv_obj_add_event_cb(m_rotateButton, controlButtonCallback, LV_EVENT_CLICKED, this);
    lv_obj_set_user_data(m_rotateButton, (void*)3); // Rotate
    lv_obj_t* rotateLabel = lv_label_create(m_rotateButton);
    lv_label_set_text(rotateLabel, "Rot");
    lv_obj_center(rotateLabel);
    
    m_dropButton = lv_btn_create(m_controlPanel);
    lv_obj_set_size(m_dropButton, 50, 30);
    lv_obj_align(m_dropButton, LV_ALIGN_RIGHT_MID, -25, 0);
    lv_obj_add_event_cb(m_dropButton, controlButtonCallback, LV_EVENT_CLICKED, this);
    lv_obj_set_user_data(m_dropButton, (void*)4); // Drop
    lv_obj_t* dropLabel = lv_label_create(m_dropButton);
    lv_label_set_text(dropLabel, "Drop");
    lv_obj_center(dropLabel);
}

void TetrisGameApp::updateGridDisplay() {
    if (!m_gameGrid) return;
    
    // Update grid cells with placed blocks
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            if (m_gridCells[y][x]) {
                uint32_t color = m_grid[y][x].filled ? m_grid[y][x].color : COLOR_EMPTY;
                lv_obj_set_style_bg_color(m_gridCells[y][x], lv_color_hex(color), 0);
            }
        }
    }
    
    // Draw current falling piece
    if (m_gameState == GameState::PLAYING) {
        drawTetromino(m_currentPiece);
    }
}

void TetrisGameApp::updateInfoDisplay() {
    if (m_scoreLabel) {
        char scoreStr[32];
        snprintf(scoreStr, sizeof(scoreStr), "Score: %d", m_score);
        lv_label_set_text(m_scoreLabel, scoreStr);
    }
    
    if (m_linesLabel) {
        char linesStr[32];
        snprintf(linesStr, sizeof(linesStr), "Lines: %d", m_lines);
        lv_label_set_text(m_linesLabel, linesStr);
    }
    
    if (m_levelLabel) {
        char levelStr[32];
        snprintf(levelStr, sizeof(levelStr), "Level: %d", m_level);
        lv_label_set_text(m_levelLabel, levelStr);
    }
    
    if (m_timeLabel) {
        int minutes = m_gameTime / 60000;
        int seconds = (m_gameTime / 1000) % 60;
        char timeStr[16];
        snprintf(timeStr, sizeof(timeStr), "%02d:%02d", minutes, seconds);
        lv_label_set_text(m_timeLabel, timeStr);
    }
    
    // Update next piece preview
    if (m_nextPieceDisplay) {
        // Clear and redraw next piece
        lv_obj_clean(m_nextPieceDisplay);
        drawTetromino(m_nextPiece, 0, 0, true);
    }
}

void TetrisGameApp::drawTetromino(const Tetromino& piece, int offsetX, int offsetY, bool preview) {
    if (piece.type == TetrominoType::NONE) return;
    
    lv_obj_t* parent = preview ? m_nextPieceDisplay : m_gameGrid;
    int cellSize = preview ? PREVIEW_CELL_SIZE : CELL_SIZE;
    
    for (int py = 0; py < 4; py++) {
        for (int px = 0; px < 4; px++) {
            if (piece.shape[py][px]) {
                int drawX = piece.x + px + offsetX;
                int drawY = piece.y + py + offsetY;
                
                if (preview) {
                    // Draw in preview area
                    lv_obj_t* cell = lv_obj_create(parent);
                    lv_obj_set_size(cell, cellSize, cellSize);
                    lv_obj_set_pos(cell, px * cellSize + 5, py * cellSize + 5);
                    lv_obj_set_style_bg_color(cell, lv_color_hex(piece.color), 0);
                    lv_obj_set_style_border_width(cell, 1, 0);
                    lv_obj_set_style_radius(cell, 1, 0);
                } else {
                    // Draw on game grid
                    if (drawX >= 0 && drawX < GRID_WIDTH && drawY >= 0 && drawY < GRID_HEIGHT) {
                        if (m_gridCells[drawY][drawX]) {
                            lv_obj_set_style_bg_color(m_gridCells[drawY][drawX], lv_color_hex(piece.color), 0);
                        }
                    }
                }
            }
        }
    }
}

void TetrisGameApp::clearTetrominoFromDisplay(const Tetromino& piece) {
    if (piece.type == TetrominoType::NONE) return;
    
    for (int py = 0; py < 4; py++) {
        for (int px = 0; px < 4; px++) {
            if (piece.shape[py][px]) {
                int drawX = piece.x + px;
                int drawY = piece.y + py;
                
                if (drawX >= 0 && drawX < GRID_WIDTH && drawY >= 0 && drawY < GRID_HEIGHT) {
                    if (m_gridCells[drawY][drawX] && !m_grid[drawY][drawX].filled) {
                        lv_obj_set_style_bg_color(m_gridCells[drawY][drawX], lv_color_hex(COLOR_EMPTY), 0);
                    }
                }
            }
        }
    }
}

void TetrisGameApp::pauseGame() {
    if (m_gameState == GameState::PLAYING) {
        m_gameState = GameState::PAUSED;
    } else if (m_gameState == GameState::PAUSED) {
        m_gameState = GameState::PLAYING;
        m_lastDrop = millis(); // Reset drop timer
    }
}

void TetrisGameApp::resumeGame() {
    if (m_gameState == GameState::PAUSED) {
        m_gameState = GameState::PLAYING;
        m_lastDrop = millis();
    }
}

void TetrisGameApp::endGame() {
    m_gameState = GameState::GAME_OVER;
    ESP_LOGI(TAG, "Game Over! Final Score: %d, Lines: %d, Level: %d", m_score, m_lines, m_level);
    
    // Show game over message
    if (m_scoreLabel) {
        lv_label_set_text(m_scoreLabel, "GAME OVER");
        lv_obj_set_style_text_color(m_scoreLabel, lv_color_hex(0xFF0000), 0);
    }
}

void TetrisGameApp::resetGame() {
    newGame();
}

// Event handlers
void TetrisGameApp::gameGridCallback(lv_event_t* e) {
    TetrisGameApp* app = static_cast<TetrisGameApp*>(lv_event_get_user_data(e));
    if (!app) return;
    
    // Handle touch input for gestures
    lv_point_t point;
    lv_indev_get_point(lv_indev_get_act(), &point);
    app->handleTouchInput(point.x, point.y);
}

void TetrisGameApp::controlButtonCallback(lv_event_t* e) {
    TetrisGameApp* app = static_cast<TetrisGameApp*>(lv_event_get_user_data(e));
    lv_obj_t* btn = lv_event_get_target(e);
    
    if (!app || app->m_gameState != GameState::PLAYING) return;
    
    int action = (int)(intptr_t)lv_obj_get_user_data(btn);
    
    switch (action) {
        case 1: // Left
            app->movePiece(-1, 0);
            break;
        case 2: // Right
            app->movePiece(1, 0);
            break;
        case 3: // Rotate
            app->rotatePiece(true);
            break;
        case 4: // Drop
            app->dropPiece();
            break;
    }
}

void TetrisGameApp::pauseButtonCallback(lv_event_t* e) {
    TetrisGameApp* app = static_cast<TetrisGameApp*>(lv_event_get_user_data(e));
    if (app) {
        app->pauseGame();
    }
}

void TetrisGameApp::newGameButtonCallback(lv_event_t* e) {
    TetrisGameApp* app = static_cast<TetrisGameApp*>(lv_event_get_user_data(e));
    if (app) {
        app->newGame();
    }
}