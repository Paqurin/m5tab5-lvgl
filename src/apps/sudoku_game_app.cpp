#include "sudoku_game_app.h"
#include "../system/os_manager.h"
#include <esp_log.h>
#include <algorithm>
#include <random>
#include <cstring>

static const char* TAG = "SudokuGame";

// Cell implementation
bool Cell::hasConflict(const std::array<std::array<Cell, 9>, 9>& grid, int row, int col) const {
    if (isEmpty()) return false;
    
    // Check row conflicts
    for (int c = 0; c < 9; c++) {
        if (c != col && grid[row][c].value == value && !grid[row][c].isEmpty()) {
            return true;
        }
    }
    
    // Check column conflicts
    for (int r = 0; r < 9; r++) {
        if (r != row && grid[r][col].value == value && !grid[r][col].isEmpty()) {
            return true;
        }
    }
    
    // Check 3x3 box conflicts
    int boxRow = (row / 3) * 3;
    int boxCol = (col / 3) * 3;
    for (int r = boxRow; r < boxRow + 3; r++) {
        for (int c = boxCol; c < boxCol + 3; c++) {
            if ((r != row || c != col) && grid[r][c].value == value && !grid[r][c].isEmpty()) {
                return true;
            }
        }
    }
    
    return false;
}

SudokuGameApp::SudokuGameApp() : BaseApp("sudoku", "Sudoku", "1.0") {
    m_gameState = GameState::PLAYING;
    m_difficulty = SudokuDifficulty::MEDIUM;
    m_inputMode = InputMode::NUMBER;
    m_selectedRow = -1;
    m_selectedCol = -1;
    m_gameTime = 0;
    m_moves = 0;
    m_hintsUsed = 0;
    m_errors = 0;
    m_startTime = 0;
    
    // Initialize grid
    for (int r = 0; r < 9; r++) {
        for (int c = 0; c < 9; c++) {
            m_grid[r][c] = Cell();
            m_solution[r][c] = Cell();
            m_cellObjects[r][c] = nullptr;
            m_cellLabels[r][c] = nullptr;
        }
    }
}

SudokuGameApp::~SudokuGameApp() {
    shutdown();
}

os_error_t SudokuGameApp::initialize() {
    if (m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Initializing Sudoku Game");
    
    newGame(m_difficulty);
    
    m_initialized = true;
    ESP_LOGI(TAG, "Sudoku Game initialized");
    return OS_OK;
}

os_error_t SudokuGameApp::update(uint32_t deltaTime) {
    if (!m_initialized) {
        return OS_ERROR_NOT_INITIALIZED;
    }
    
    if (m_gameState == GameState::PLAYING) {
        m_gameTime += deltaTime;
        
        // Update time display every second
        static uint32_t lastTimeUpdate = 0;
        if (millis() - lastTimeUpdate > 1000) {
            updateStatusDisplay();
            lastTimeUpdate = millis();
        }
    }
    
    return OS_OK;
}

os_error_t SudokuGameApp::shutdown() {
    if (!m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Shutting down Sudoku Game");
    destroyUI();
    
    m_initialized = false;
    return OS_OK;
}

os_error_t SudokuGameApp::createUI(lv_obj_t* parent) {
    if (!parent) {
        return OS_ERROR_INVALID_PARAM;
    }

    m_uiContainer = lv_obj_create(parent);
    lv_obj_set_size(m_uiContainer, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(m_uiContainer, lv_color_hex(0xF5F5F5), 0);
    lv_obj_clear_flag(m_uiContainer, LV_OBJ_FLAG_SCROLLABLE);

    createStatusBar();
    createGameGrid();
    createControlPanel();
    updateGridUI();
    updateStatusDisplay();

    ESP_LOGI(TAG, "Sudoku UI created");
    return OS_OK;
}

os_error_t SudokuGameApp::destroyUI() {
    if (m_uiContainer) {
        lv_obj_del(m_uiContainer);
        m_uiContainer = nullptr;
        m_gameGrid = nullptr;
        m_controlPanel = nullptr;
        m_statusBar = nullptr;
        
        // Clear cell object references
        for (int r = 0; r < 9; r++) {
            for (int c = 0; c < 9; c++) {
                m_cellObjects[r][c] = nullptr;
                m_cellLabels[r][c] = nullptr;
            }
        }
        
        // Clear button references
        for (int i = 0; i < 9; i++) {
            m_numberButtons[i] = nullptr;
        }
        m_pencilButton = nullptr;
        m_eraseButton = nullptr;
        m_hintButton = nullptr;
        m_checkButton = nullptr;
        m_newGameButton = nullptr;
        m_pauseButton = nullptr;
        m_undoButton = nullptr;
        
        // Clear status labels
        m_timeLabel = nullptr;
        m_difficultyLabel = nullptr;
        m_movesLabel = nullptr;
        m_hintsLabel = nullptr;
        m_errorsLabel = nullptr;
    }
    return OS_OK;
}

void SudokuGameApp::newGame(SudokuDifficulty difficulty) {
    ESP_LOGI(TAG, "Starting new Sudoku game - difficulty: %d", (int)difficulty);
    
    m_difficulty = difficulty;
    m_gameState = GameState::PLAYING;
    m_inputMode = InputMode::NUMBER;
    m_selectedRow = -1;
    m_selectedCol = -1;
    m_gameTime = 0;
    m_moves = 0;
    m_hintsUsed = 0;
    m_errors = 0;
    m_startTime = millis();
    m_moveHistory.clear();
    
    // Clear grid
    for (int r = 0; r < 9; r++) {
        for (int c = 0; c < 9; c++) {
            m_grid[r][c] = Cell();
            m_solution[r][c] = Cell();
        }
    }
    
    generatePuzzle(difficulty);
    updateGridUI();
    updateStatusDisplay();
}

void SudokuGameApp::generatePuzzle(SudokuDifficulty difficulty) {
    // Generate a complete valid solution first
    ESP_LOGI(TAG, "Generating Sudoku puzzle");
    
    // Fill diagonal 3x3 boxes first (they don't affect each other)
    for (int box = 0; box < 3; box++) {
        int startRow = box * 3;
        int startCol = box * 3;
        
        std::vector<uint8_t> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9};
        shuffleArray(numbers);
        
        int idx = 0;
        for (int r = startRow; r < startRow + 3; r++) {
            for (int c = startCol; c < startCol + 3; c++) {
                m_solution[r][c].value = numbers[idx++];
                m_solution[r][c].state = CellState::GIVEN;
            }
        }
    }
    
    // Solve the rest using backtracking
    solvePuzzle(m_solution);
    
    // Copy solution to main grid
    for (int r = 0; r < 9; r++) {
        for (int c = 0; c < 9; c++) {
            m_grid[r][c] = m_solution[r][c];
        }
    }
    
    // Remove cells based on difficulty
    int cellsToRemove;
    switch (difficulty) {
        case SudokuDifficulty::EASY:   cellsToRemove = 81 - 45; break; // 36 removed
        case SudokuDifficulty::MEDIUM: cellsToRemove = 81 - 37; break; // 44 removed  
        case SudokuDifficulty::HARD:   cellsToRemove = 81 - 32; break; // 49 removed
        case SudokuDifficulty::EXPERT: cellsToRemove = 81 - 27; break; // 54 removed
        default: cellsToRemove = 44; break;
    }
    
    removeCells(cellsToRemove);
}

bool SudokuGameApp::solvePuzzle(std::array<std::array<Cell, 9>, 9>& grid) {
    for (int r = 0; r < 9; r++) {
        for (int c = 0; c < 9; c++) {
            if (grid[r][c].isEmpty()) {
                for (uint8_t num = 1; num <= 9; num++) {
                    grid[r][c].value = num;
                    if (!grid[r][c].hasConflict(grid, r, c)) {
                        if (solvePuzzle(grid)) {
                            return true;
                        }
                    }
                    grid[r][c].value = 0;
                }
                return false;
            }
        }
    }
    return true;
}

void SudokuGameApp::removeCells(int cellsToRemove) {
    std::vector<std::pair<int, int>> cells;
    for (int r = 0; r < 9; r++) {
        for (int c = 0; c < 9; c++) {
            cells.push_back({r, c});
        }
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(cells.begin(), cells.end(), gen);
    
    int removed = 0;
    for (auto& cell : cells) {
        if (removed >= cellsToRemove) break;
        
        int r = cell.first;
        int c = cell.second;
        
        // Temporarily remove cell
        uint8_t backup = m_grid[r][c].value;
        m_grid[r][c].value = 0;
        m_grid[r][c].state = CellState::EMPTY;
        
        // Check if puzzle still has unique solution (simplified check)
        // In a full implementation, this would verify uniqueness
        removed++;
    }
    
    ESP_LOGI(TAG, "Removed %d cells from puzzle", removed);
}

void SudokuGameApp::createStatusBar() {
    m_statusBar = lv_obj_create(m_uiContainer);
    lv_obj_set_size(m_statusBar, LV_PCT(100), 35);
    lv_obj_align(m_statusBar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(m_statusBar, lv_color_hex(0x2196F3), 0);
    lv_obj_set_style_radius(m_statusBar, 0, 0);
    
    // Time label
    m_timeLabel = lv_label_create(m_statusBar);
    lv_label_set_text(m_timeLabel, "00:00");
    lv_obj_align(m_timeLabel, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_set_style_text_color(m_timeLabel, lv_color_white(), 0);
    
    // Difficulty label
    m_difficultyLabel = lv_label_create(m_statusBar);
    lv_label_set_text(m_difficultyLabel, "Medium");
    lv_obj_align(m_difficultyLabel, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(m_difficultyLabel, lv_color_white(), 0);
    
    // Moves label
    m_movesLabel = lv_label_create(m_statusBar);
    lv_label_set_text(m_movesLabel, "Moves: 0");
    lv_obj_align(m_movesLabel, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_set_style_text_color(m_movesLabel, lv_color_white(), 0);
}

void SudokuGameApp::createGameGrid() {
    m_gameGrid = lv_obj_create(m_uiContainer);
    lv_obj_set_size(m_gameGrid, 330, 330);
    lv_obj_align(m_gameGrid, LV_ALIGN_CENTER, 0, -20);
    lv_obj_set_style_bg_color(m_gameGrid, lv_color_hex(COLOR_GRID_BG), 0);
    lv_obj_set_style_border_color(m_gameGrid, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(m_gameGrid, THICK_LINE_WIDTH, 0);
    lv_obj_set_style_radius(m_gameGrid, 5, 0);
    lv_obj_clear_flag(m_gameGrid, LV_OBJ_FLAG_SCROLLABLE);
    
    // Create 9x9 cell grid
    for (int r = 0; r < 9; r++) {
        for (int c = 0; c < 9; c++) {
            m_cellObjects[r][c] = lv_obj_create(m_gameGrid);
            lv_obj_set_size(m_cellObjects[r][c], CELL_SIZE, CELL_SIZE);
            
            int x = GRID_PADDING + c * (CELL_SIZE + CELL_SPACING) + (c / 3) * THICK_LINE_WIDTH;
            int y = GRID_PADDING + r * (CELL_SIZE + CELL_SPACING) + (r / 3) * THICK_LINE_WIDTH;
            lv_obj_set_pos(m_cellObjects[r][c], x, y);
            
            lv_obj_set_style_bg_color(m_cellObjects[r][c], lv_color_hex(COLOR_CELL_EMPTY), 0);
            lv_obj_set_style_border_color(m_cellObjects[r][c], lv_color_hex(0x888888), 0);
            lv_obj_set_style_border_width(m_cellObjects[r][c], THIN_LINE_WIDTH, 0);
            lv_obj_set_style_radius(m_cellObjects[r][c], 2, 0);
            
            // Store row/col in user data
            lv_obj_set_user_data(m_cellObjects[r][c], (void*)(r * 9 + c));
            lv_obj_add_event_cb(m_cellObjects[r][c], cellClickCallback, LV_EVENT_CLICKED, this);
            
            // Create label for cell content
            m_cellLabels[r][c] = lv_label_create(m_cellObjects[r][c]);
            lv_label_set_text(m_cellLabels[r][c], "");
            lv_obj_center(m_cellLabels[r][c]);
            lv_obj_set_style_text_font(m_cellLabels[r][c], &lv_font_montserrat_20, 0);
        }
    }
}

void SudokuGameApp::createControlPanel() {
    m_controlPanel = lv_obj_create(m_uiContainer);
    lv_obj_set_size(m_controlPanel, LV_PCT(100), 100);
    lv_obj_align(m_controlPanel, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(m_controlPanel, lv_color_hex(0xE0E0E0), 0);
    lv_obj_set_style_radius(m_controlPanel, 0, 0);
    
    // Number buttons (1-9)
    for (int i = 0; i < 9; i++) {
        m_numberButtons[i] = lv_btn_create(m_controlPanel);
        lv_obj_set_size(m_numberButtons[i], 30, 30);
        
        int x = 20 + (i % 5) * 35;
        int y = 10 + (i / 5) * 35;
        lv_obj_set_pos(m_numberButtons[i], x, y);
        
        lv_obj_set_user_data(m_numberButtons[i], (void*)(i + 1));
        lv_obj_add_event_cb(m_numberButtons[i], numberButtonCallback, LV_EVENT_CLICKED, this);
        
        lv_obj_t* label = lv_label_create(m_numberButtons[i]);
        char numStr[2] = {(char)('1' + i), '\0'};
        lv_label_set_text(label, numStr);
        lv_obj_center(label);
    }
    
    // Control buttons
    m_pencilButton = lv_btn_create(m_controlPanel);
    lv_obj_set_size(m_pencilButton, 50, 25);
    lv_obj_set_pos(m_pencilButton, 200, 10);
    lv_obj_add_event_cb(m_pencilButton, controlButtonCallback, LV_EVENT_CLICKED, this);
    lv_obj_set_user_data(m_pencilButton, (void*)1);
    lv_obj_t* pencilLabel = lv_label_create(m_pencilButton);
    lv_label_set_text(pencilLabel, "Pencil");
    lv_obj_center(pencilLabel);
    
    m_eraseButton = lv_btn_create(m_controlPanel);
    lv_obj_set_size(m_eraseButton, 50, 25);
    lv_obj_set_pos(m_eraseButton, 260, 10);
    lv_obj_add_event_cb(m_eraseButton, controlButtonCallback, LV_EVENT_CLICKED, this);
    lv_obj_set_user_data(m_eraseButton, (void*)2);
    lv_obj_t* eraseLabel = lv_label_create(m_eraseButton);
    lv_label_set_text(eraseLabel, "Erase");
    lv_obj_center(eraseLabel);
    
    m_hintButton = lv_btn_create(m_controlPanel);
    lv_obj_set_size(m_hintButton, 50, 25);
    lv_obj_set_pos(m_hintButton, 200, 40);
    lv_obj_add_event_cb(m_hintButton, controlButtonCallback, LV_EVENT_CLICKED, this);
    lv_obj_set_user_data(m_hintButton, (void*)3);
    lv_obj_t* hintLabel = lv_label_create(m_hintButton);
    lv_label_set_text(hintLabel, "Hint");
    lv_obj_center(hintLabel);
    
    m_newGameButton = lv_btn_create(m_controlPanel);
    lv_obj_set_size(m_newGameButton, 50, 25);
    lv_obj_set_pos(m_newGameButton, 260, 40);
    lv_obj_add_event_cb(m_newGameButton, controlButtonCallback, LV_EVENT_CLICKED, this);
    lv_obj_set_user_data(m_newGameButton, (void*)4);
    lv_obj_t* newLabel = lv_label_create(m_newGameButton);
    lv_label_set_text(newLabel, "New");
    lv_obj_center(newLabel);
    
    m_checkButton = lv_btn_create(m_controlPanel);
    lv_obj_set_size(m_checkButton, 50, 25);
    lv_obj_set_pos(m_checkButton, 200, 70);
    lv_obj_add_event_cb(m_checkButton, controlButtonCallback, LV_EVENT_CLICKED, this);
    lv_obj_set_user_data(m_checkButton, (void*)5);
    lv_obj_t* checkLabel = lv_label_create(m_checkButton);
    lv_label_set_text(checkLabel, "Check");
    lv_obj_center(checkLabel);
}

void SudokuGameApp::updateGridUI() {
    for (int r = 0; r < 9; r++) {
        for (int c = 0; c < 9; c++) {
            updateCellUI(r, c);
        }
    }
}

void SudokuGameApp::updateCellUI(int row, int col) {
    if (!m_cellObjects[row][col] || !m_cellLabels[row][col]) return;
    
    const Cell& cell = m_grid[row][col];
    
    // Update cell background color
    uint32_t bgColor = COLOR_CELL_EMPTY;
    if (cell.isSelected) {
        bgColor = COLOR_CELL_SELECTED;
    } else if (cell.isHighlighted) {
        bgColor = COLOR_CELL_HIGHLIGHTED;
    } else if (cell.state == CellState::ERROR) {
        bgColor = COLOR_CELL_ERROR;
    } else if (cell.state == CellState::GIVEN) {
        bgColor = COLOR_CELL_GIVEN;
    } else if (cell.state == CellState::HINT) {
        bgColor = COLOR_CELL_HINT;
    } else if (!cell.isEmpty()) {
        bgColor = COLOR_CELL_FILLED;
    }
    
    lv_obj_set_style_bg_color(m_cellObjects[row][col], lv_color_hex(bgColor), 0);
    
    // Update cell text
    if (!cell.isEmpty()) {
        char numStr[2] = {(char)('0' + cell.value), '\0'};
        lv_label_set_text(m_cellLabels[row][col], numStr);
        
        // Set text color based on cell state
        uint32_t textColor = COLOR_TEXT_FILLED;
        if (cell.state == CellState::GIVEN) {
            textColor = COLOR_TEXT_GIVEN;
        } else if (cell.state == CellState::ERROR) {
            textColor = COLOR_TEXT_ERROR;
        } else if (cell.state == CellState::HINT) {
            textColor = COLOR_TEXT_HINT;
        }
        
        lv_obj_set_style_text_color(m_cellLabels[row][col], lv_color_hex(textColor), 0);
    } else {
        // Show pencil marks if any
        bool hasPencilMarks = false;
        for (int i = 0; i < 9; i++) {
            if (cell.pencilMarks[i]) {
                hasPencilMarks = true;
                break;
            }
        }
        
        if (hasPencilMarks && m_inputMode == InputMode::PENCIL) {
            // Show condensed pencil marks
            std::string pencilText = "";
            for (int i = 0; i < 9; i++) {
                if (cell.pencilMarks[i]) {
                    pencilText += std::to_string(i + 1);
                }
            }
            lv_label_set_text(m_cellLabels[row][col], pencilText.c_str());
            lv_obj_set_style_text_color(m_cellLabels[row][col], lv_color_hex(COLOR_TEXT_PENCIL), 0);
            lv_obj_set_style_text_font(m_cellLabels[row][col], &lv_font_montserrat_12, 0);
        } else {
            lv_label_set_text(m_cellLabels[row][col], "");
        }
    }
}

void SudokuGameApp::updateStatusDisplay() {
    if (m_timeLabel) {
        int totalSeconds = m_gameTime / 1000;
        int minutes = totalSeconds / 60;
        int seconds = totalSeconds % 60;
        char timeStr[16];
        snprintf(timeStr, sizeof(timeStr), "%02d:%02d", minutes, seconds);
        lv_label_set_text(m_timeLabel, timeStr);
    }
    
    if (m_difficultyLabel) {
        const char* diffStr[] = {"Easy", "Medium", "Hard", "Expert"};
        lv_label_set_text(m_difficultyLabel, diffStr[(int)m_difficulty]);
    }
    
    if (m_movesLabel) {
        char movesStr[32];
        snprintf(movesStr, sizeof(movesStr), "Moves: %d", m_moves);
        lv_label_set_text(m_movesLabel, movesStr);
    }
}

void SudokuGameApp::selectCell(int row, int col) {
    // Clear previous selection
    if (m_selectedRow >= 0 && m_selectedCol >= 0) {
        m_grid[m_selectedRow][m_selectedCol].isSelected = false;
        updateCellUI(m_selectedRow, m_selectedCol);
    }
    
    m_selectedRow = row;
    m_selectedCol = col;
    
    if (row >= 0 && col >= 0) {
        m_grid[row][col].isSelected = true;
        updateCellUI(row, col);
    }
}

void SudokuGameApp::inputNumber(uint8_t number) {
    if (m_selectedRow < 0 || m_selectedCol < 0) return;
    if (m_grid[m_selectedRow][m_selectedCol].state == CellState::GIVEN) return;
    
    Cell& cell = m_grid[m_selectedRow][m_selectedCol];
    
    if (m_inputMode == InputMode::NUMBER) {
        cell.value = number;
        cell.state = CellState::FILLED;
        cell.pencilMarks.fill(false);
        m_moves++;
        
        // Check for completion
        if (isComplete()) {
            m_gameState = GameState::COMPLETED;
            ESP_LOGI(TAG, "Puzzle completed! Time: %d seconds, Moves: %d", m_gameTime / 1000, m_moves);
        }
    } else if (m_inputMode == InputMode::PENCIL) {
        togglePencilMark(m_selectedRow, m_selectedCol, number);
    }
    
    updateCellUI(m_selectedRow, m_selectedCol);
    updateStatusDisplay();
}

void SudokuGameApp::togglePencilMark(int row, int col, uint8_t number) {
    if (number < 1 || number > 9) return;
    
    Cell& cell = m_grid[row][col];
    if (cell.state == CellState::GIVEN || !cell.isEmpty()) return;
    
    cell.pencilMarks[number - 1] = !cell.pencilMarks[number - 1];
    cell.state = CellState::PENCIL;
}

bool SudokuGameApp::isComplete() const {
    for (int r = 0; r < 9; r++) {
        for (int c = 0; c < 9; c++) {
            if (m_grid[r][c].isEmpty()) return false;
            if (m_grid[r][c].hasConflict(m_grid, r, c)) return false;
        }
    }
    return true;
}

void SudokuGameApp::giveHint() {
    if (m_selectedRow < 0 || m_selectedCol < 0) return;
    
    Cell& cell = m_grid[m_selectedRow][m_selectedCol];
    if (cell.state == CellState::GIVEN || !cell.isEmpty()) return;
    
    // Get the correct value from solution
    uint8_t correctValue = m_solution[m_selectedRow][m_selectedCol].value;
    
    cell.value = correctValue;
    cell.state = CellState::HINT;
    cell.pencilMarks.fill(false);
    
    m_hintsUsed++;
    m_moves++;
    
    updateCellUI(m_selectedRow, m_selectedCol);
    updateStatusDisplay();
    
    ESP_LOGI(TAG, "Hint given: Cell (%d,%d) = %d", m_selectedRow, m_selectedCol, correctValue);
}

void SudokuGameApp::shuffleArray(std::vector<uint8_t>& arr) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(arr.begin(), arr.end(), gen);
}

// Event handlers
void SudokuGameApp::cellClickCallback(lv_event_t* e) {
    SudokuGameApp* app = static_cast<SudokuGameApp*>(lv_event_get_user_data(e));
    lv_obj_t* cellObj = lv_event_get_target(e);
    
    int cellIndex = (int)(intptr_t)lv_obj_get_user_data(cellObj);
    int row = cellIndex / 9;
    int col = cellIndex % 9;
    
    app->selectCell(row, col);
}

void SudokuGameApp::numberButtonCallback(lv_event_t* e) {
    SudokuGameApp* app = static_cast<SudokuGameApp*>(lv_event_get_user_data(e));
    lv_obj_t* btn = lv_event_get_target(e);
    
    uint8_t number = (uint8_t)(intptr_t)lv_obj_get_user_data(btn);
    app->inputNumber(number);
}

void SudokuGameApp::controlButtonCallback(lv_event_t* e) {
    SudokuGameApp* app = static_cast<SudokuGameApp*>(lv_event_get_user_data(e));
    lv_obj_t* btn = lv_event_get_target(e);
    
    int action = (int)(intptr_t)lv_obj_get_user_data(btn);
    
    switch (action) {
        case 1: // Pencil mode
            app->m_inputMode = (app->m_inputMode == InputMode::PENCIL) ? 
                               InputMode::NUMBER : InputMode::PENCIL;
            break;
        case 2: // Erase
            if (app->m_selectedRow >= 0 && app->m_selectedCol >= 0) {
                Cell& cell = app->m_grid[app->m_selectedRow][app->m_selectedCol];
                if (cell.state != CellState::GIVEN) {
                    cell.value = 0;
                    cell.state = CellState::EMPTY;
                    cell.pencilMarks.fill(false);
                    app->updateCellUI(app->m_selectedRow, app->m_selectedCol);
                }
            }
            break;
        case 3: // Hint
            app->giveHint();
            break;
        case 4: // New game
            app->newGame(app->m_difficulty);
            break;
        case 5: // Check
            app->checkForErrors();
            break;
    }
}

void SudokuGameApp::checkForErrors() {
    int errorCount = 0;
    
    for (int r = 0; r < 9; r++) {
        for (int c = 0; c < 9; c++) {
            Cell& cell = m_grid[r][c];
            if (!cell.isEmpty() && cell.hasConflict(m_grid, r, c)) {
                cell.state = CellState::ERROR;
                errorCount++;
            } else if (cell.state == CellState::ERROR) {
                cell.state = CellState::FILLED;
            }
            updateCellUI(r, c);
        }
    }
    
    m_errors = errorCount;
    ESP_LOGI(TAG, "Found %d errors", errorCount);
}