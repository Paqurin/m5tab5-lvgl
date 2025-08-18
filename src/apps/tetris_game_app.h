#ifndef TETRIS_GAME_APP_H
#define TETRIS_GAME_APP_H

#include "base_app.h"
#include <array>
#include <vector>

/**
 * @file tetris_game_app.h
 * @brief Classic Droppin Blocks puzzle game for M5Stack Tab5
 * 
 * Features classic falling blocks gameplay with:
 * - Standard tetromino pieces (I, O, T, S, Z, J, L)
 * - 10x20 playing field
 * - Touch controls for movement and rotation
 * - Progressive speed increase with level
 * - Line clearing and scoring
 * - Next piece preview
 * - Pause and resume functionality
 * - High score tracking
 */

enum class TetrominoType {
    NONE = 0,
    I = 1,    // Line piece
    O = 2,    // Square piece  
    T = 3,    // T piece
    S = 4,    // S piece
    Z = 5,    // Z piece
    J = 6,    // J piece
    L = 7     // L piece
};

enum class GameState {
    PLAYING,
    PAUSED,
    GAME_OVER,
    LINE_CLEARING
};

struct Block {
    TetrominoType type;
    bool filled;
    uint32_t color;
    
    Block() : type(TetrominoType::NONE), filled(false), color(0x000000) {}
    Block(TetrominoType t, uint32_t c) : type(t), filled(true), color(c) {}
};

struct Tetromino {
    TetrominoType type;
    std::array<std::array<bool, 4>, 4> shape;
    int x, y;  // Position on grid
    int rotation; // 0-3
    uint32_t color;
    
    Tetromino() : type(TetrominoType::NONE), x(0), y(0), rotation(0), color(0x000000) {
        for (auto& row : shape) {
            row.fill(false);
        }
    }
    
    void rotateClockwise();
    void rotateCounterClockwise();
    bool isValidPosition(const std::array<std::array<Block, 10>, 20>& grid, int testX, int testY, int testRotation = -1) const;
};

class TetrisGameApp : public BaseApp {
public:
    // Game configuration constants (public for access)
    static constexpr int GRID_WIDTH = 10;
    static constexpr int GRID_HEIGHT = 20;
    static constexpr int CELL_SIZE = 15;
    static constexpr int GRID_SPACING = 1;
    static constexpr int PREVIEW_CELL_SIZE = 10;

    TetrisGameApp();
    ~TetrisGameApp() override;

    os_error_t initialize() override;
    os_error_t update(uint32_t deltaTime) override;
    os_error_t shutdown() override;
    os_error_t createUI(lv_obj_t* parent) override;
    os_error_t destroyUI() override;

private:
    // Game logic
    void newGame();
    void gameStep();
    void spawnNewPiece();
    void movePiece(int deltaX, int deltaY);
    void rotatePiece(bool clockwise = true);
    void dropPiece();
    void lockPiece();
    bool canMovePiece(int deltaX, int deltaY) const;
    bool canRotatePiece(bool clockwise = true) const;
    
    // Line clearing
    void checkForLines();
    void clearLines(const std::vector<int>& linesToClear);
    void dropLinesAbove(int clearedLine);
    int countFullLines() const;
    
    // Tetromino management
    void initializeTetrominoes();
    Tetromino createTetromino(TetrominoType type) const;
    TetrominoType getRandomTetrominoType();
    uint32_t getTetrominoColor(TetrominoType type) const;
    
    // Scoring and levels
    void updateScore(int linesCleared);
    void updateLevel();
    uint32_t getDropInterval() const;
    
    // Input handling
    void handleTouchInput(int x, int y);
    void handleSwipeGesture(int deltaX, int deltaY);
    
    // UI creation and updates
    void createGameGrid();
    void createInfoPanel();
    void createControlButtons();
    void updateGridDisplay();
    void updateInfoDisplay();
    void drawTetromino(const Tetromino& piece, int offsetX = 0, int offsetY = 0, bool preview = false);
    void clearTetrominoFromDisplay(const Tetromino& piece);
    
    // Game state management
    void pauseGame();
    void resumeGame();
    void endGame();
    void resetGame();
    
    // Event handlers
    static void gameGridCallback(lv_event_t* e);
    static void controlButtonCallback(lv_event_t* e);
    static void pauseButtonCallback(lv_event_t* e);
    static void newGameButtonCallback(lv_event_t* e);
    
    // Game data
    std::array<std::array<Block, 10>, 20> m_grid;    // 10 wide x 20 high
    Tetromino m_currentPiece;
    Tetromino m_nextPiece;
    std::array<Tetromino, 7> m_tetrominoTemplates;
    
    // Game state
    GameState m_gameState;
    uint32_t m_score;
    uint32_t m_lines;
    uint32_t m_level;
    uint32_t m_gameTime;
    
    // Timing
    uint32_t m_lastDrop;
    uint32_t m_dropInterval;
    uint32_t m_lockDelay;
    uint32_t m_lineClearDelay;
    
    // Statistics
    std::array<uint32_t, 7> m_pieceCount; // Count of each piece type used
    uint32_t m_singleLines;
    uint32_t m_doubleLines;
    uint32_t m_tripleLines;
    uint32_t m_tetrisLines;
    
    // UI elements
    lv_obj_t* m_gameGrid = nullptr;
    lv_obj_t* m_infoPanel = nullptr;
    lv_obj_t* m_controlPanel = nullptr;
    
    // Grid display
    std::array<std::array<lv_obj_t*, 10>, 20> m_gridCells;
    
    // Info display
    lv_obj_t* m_scoreLabel = nullptr;
    lv_obj_t* m_linesLabel = nullptr;
    lv_obj_t* m_levelLabel = nullptr;
    lv_obj_t* m_timeLabel = nullptr;
    lv_obj_t* m_nextPieceDisplay = nullptr;
    
    // Control buttons
    lv_obj_t* m_pauseButton = nullptr;
    lv_obj_t* m_newGameButton = nullptr;
    lv_obj_t* m_leftButton = nullptr;
    lv_obj_t* m_rightButton = nullptr;
    lv_obj_t* m_rotateButton = nullptr;
    lv_obj_t* m_dropButton = nullptr;
    
    // Touch input state
    int m_lastTouchX;
    int m_lastTouchY;
    uint32_t m_lastTouchTime;
    bool m_touchActive;
    
    // Timing constants
    static constexpr uint32_t BASE_DROP_INTERVAL = 1000; // 1 second at level 0
    static constexpr uint32_t MIN_DROP_INTERVAL = 50;    // Fastest speed
    static constexpr uint32_t LOCK_DELAY_TIME = 500;     // Half second lock delay
    static constexpr uint32_t LINE_CLEAR_DELAY = 300;    // Line clear animation time
    
    // Scoring constants
    static constexpr uint32_t SCORE_SINGLE = 100;
    static constexpr uint32_t SCORE_DOUBLE = 300;
    static constexpr uint32_t SCORE_TRIPLE = 500;
    static constexpr uint32_t SCORE_TETRIS = 800;
    static constexpr uint32_t SCORE_SOFT_DROP = 1;
    static constexpr uint32_t SCORE_HARD_DROP = 2;
    
    // Colors for tetromino types
    static constexpr uint32_t COLOR_I = 0x00FFFF; // Cyan
    static constexpr uint32_t COLOR_O = 0xFFFF00; // Yellow
    static constexpr uint32_t COLOR_T = 0x800080; // Purple
    static constexpr uint32_t COLOR_S = 0x00FF00; // Green
    static constexpr uint32_t COLOR_Z = 0xFF0000; // Red
    static constexpr uint32_t COLOR_J = 0x0000FF; // Blue
    static constexpr uint32_t COLOR_L = 0xFFA500; // Orange
    static constexpr uint32_t COLOR_EMPTY = 0x222222; // Dark gray
    static constexpr uint32_t COLOR_GRID_LINE = 0x444444; // Grid lines
    static constexpr uint32_t COLOR_PREVIEW = 0x888888; // Preview piece
};

#endif // TETRIS_GAME_APP_H