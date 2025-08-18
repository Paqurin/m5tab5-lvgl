#ifndef SUDOKU_GAME_APP_H
#define SUDOKU_GAME_APP_H

#include "base_app.h"
#include <array>
#include <vector>

/**
 * @file sudoku_game_app.h
 * @brief Sudoku puzzle game for M5Stack Tab5
 * 
 * Features classic 9x9 Sudoku with:
 * - Multiple difficulty levels (Easy, Medium, Hard, Expert)
 * - Touch-based number input
 * - Pencil marks for candidate numbers
 * - Hint system and auto-solver
 * - Puzzle validation and checking
 * - Timer and score tracking
 * - Save/load game state
 * - Multiple color themes
 */

enum class SudokuDifficulty {
    EASY = 0,      // 40-45 givens
    MEDIUM = 1,    // 35-39 givens  
    HARD = 2,      // 30-34 givens
    EXPERT = 3     // 25-29 givens
};

enum class CellState {
    EMPTY,         // No number
    GIVEN,         // Pre-filled (immutable)
    FILLED,        // User filled
    PENCIL,        // Pencil marks only
    ERROR,         // Contains error
    HINT          // Provided by hint system
};

struct Cell {
    uint8_t value;              // 0 = empty, 1-9 = number
    CellState state;
    std::array<bool, 9> pencilMarks; // Candidate numbers 1-9
    bool isHighlighted;
    bool isSelected;
    
    Cell() : value(0), state(CellState::EMPTY), isHighlighted(false), isSelected(false) {
        pencilMarks.fill(false);
    }
    
    bool isEmpty() const { return value == 0; }
    bool hasConflict(const std::array<std::array<Cell, 9>, 9>& grid, int row, int col) const;
};

enum class InputMode {
    NUMBER,        // Input numbers directly
    PENCIL,        // Input pencil marks
    ERASE         // Erase mode
};

enum class GameState {
    PLAYING,
    PAUSED,
    COMPLETED,
    CHECKING
};

class SudokuGameApp : public BaseApp {
public:
    SudokuGameApp();
    ~SudokuGameApp() override;

    os_error_t initialize() override;
    os_error_t update(uint32_t deltaTime) override;
    os_error_t shutdown() override;
    os_error_t createUI(lv_obj_t* parent) override;
    os_error_t destroyUI() override;

private:
    // Game logic
    void newGame(SudokuDifficulty difficulty = SudokuDifficulty::MEDIUM);
    void generatePuzzle(SudokuDifficulty difficulty);
    bool solvePuzzle(std::array<std::array<Cell, 9>, 9>& grid);
    void removeCells(int cellsToRemove);
    bool isValidMove(int row, int col, uint8_t value) const;
    bool isComplete() const;
    bool hasErrors() const;
    void checkForErrors();
    void highlightConflicts();
    void clearHighlights();
    
    // Input handling
    void selectCell(int row, int col);
    void inputNumber(uint8_t number);
    void togglePencilMark(int row, int col, uint8_t number);
    void eraseCell(int row, int col);
    void setInputMode(InputMode mode);
    
    // Hint system
    void giveHint();
    std::vector<uint8_t> getCandidates(int row, int col) const;
    void autoFillObviousCells();
    void showAllPencilMarks();
    
    // UI creation
    void createGameGrid();
    void createControlPanel();
    void createStatusBar();
    void createDifficultyMenu();
    void updateGridUI();
    void updateCellUI(int row, int col);
    void updateStatusDisplay();
    
    // Game state management
    void pauseGame();
    void resumeGame();
    void resetGame();
    void saveGame();
    void loadGame();
    
    // Validation
    bool isValidPuzzle() const;
    bool hasUniqueSolution() const;
    
    // Utilities
    int getBoxIndex(int row, int col) const { return (row / 3) * 3 + (col / 3); }
    std::pair<int, int> getBoxTopLeft(int boxIndex) const;
    void shuffleArray(std::vector<uint8_t>& arr);
    
    // Event handlers
    static void cellClickCallback(lv_event_t* e);
    static void numberButtonCallback(lv_event_t* e);
    static void controlButtonCallback(lv_event_t* e);
    static void difficultyCallback(lv_event_t* e);
    static void menuButtonCallback(lv_event_t* e);
    
    // Game data
    std::array<std::array<Cell, 9>, 9> m_grid;
    std::array<std::array<Cell, 9>, 9> m_solution; // Complete solution for hints
    
    // Game state
    GameState m_gameState;
    SudokuDifficulty m_difficulty;
    InputMode m_inputMode;
    int m_selectedRow;
    int m_selectedCol;
    
    // Statistics
    uint32_t m_gameTime;
    uint32_t m_moves;
    uint32_t m_hintsUsed;
    uint32_t m_errors;
    uint32_t m_startTime;
    
    // UI elements
    lv_obj_t* m_gameGrid = nullptr;
    lv_obj_t* m_controlPanel = nullptr;
    lv_obj_t* m_statusBar = nullptr;
    lv_obj_t* m_difficultyMenu = nullptr;
    
    // Grid cells
    std::array<std::array<lv_obj_t*, 9>, 9> m_cellObjects;
    std::array<std::array<lv_obj_t*, 9>, 9> m_cellLabels;
    
    // Control buttons
    lv_obj_t* m_numberButtons[9] = {nullptr};
    lv_obj_t* m_pencilButton = nullptr;
    lv_obj_t* m_eraseButton = nullptr;
    lv_obj_t* m_hintButton = nullptr;
    lv_obj_t* m_checkButton = nullptr;
    lv_obj_t* m_newGameButton = nullptr;
    lv_obj_t* m_pauseButton = nullptr;
    lv_obj_t* m_undoButton = nullptr;
    
    // Status display
    lv_obj_t* m_timeLabel = nullptr;
    lv_obj_t* m_difficultyLabel = nullptr;
    lv_obj_t* m_movesLabel = nullptr;
    lv_obj_t* m_hintsLabel = nullptr;
    lv_obj_t* m_errorsLabel = nullptr;
    
    // Undo system
    struct GameMove {
        int row, col;
        uint8_t oldValue;
        uint8_t newValue;
        CellState oldState;
        CellState newState;
        std::array<bool, 9> oldPencilMarks;
        std::array<bool, 9> newPencilMarks;
        uint32_t timestamp;
    };
    std::vector<GameMove> m_moveHistory;
    static constexpr size_t MAX_UNDO_MOVES = 100;
    
    // UI styling constants
    static constexpr int CELL_SIZE = 35;
    static constexpr int CELL_SPACING = 2;
    static constexpr int GRID_PADDING = 10;
    static constexpr int THICK_LINE_WIDTH = 3;
    static constexpr int THIN_LINE_WIDTH = 1;
    
    // Colors
    static constexpr uint32_t COLOR_GRID_BG = 0xFFFFFF;
    static constexpr uint32_t COLOR_CELL_EMPTY = 0xF8F8F8;
    static constexpr uint32_t COLOR_CELL_GIVEN = 0xE0E0E0;
    static constexpr uint32_t COLOR_CELL_FILLED = 0xFFFFFF;
    static constexpr uint32_t COLOR_CELL_SELECTED = 0x4CAF50;
    static constexpr uint32_t COLOR_CELL_HIGHLIGHTED = 0xFFF9C4;
    static constexpr uint32_t COLOR_CELL_ERROR = 0xFFCDD2;
    static constexpr uint32_t COLOR_CELL_HINT = 0xE1F5FE;
    static constexpr uint32_t COLOR_TEXT_GIVEN = 0x000000;
    static constexpr uint32_t COLOR_TEXT_FILLED = 0x1976D2;
    static constexpr uint32_t COLOR_TEXT_PENCIL = 0x757575;
    static constexpr uint32_t COLOR_TEXT_ERROR = 0xD32F2F;
    static constexpr uint32_t COLOR_TEXT_HINT = 0x0277BD;
};

#endif // SUDOKU_GAME_APP_H