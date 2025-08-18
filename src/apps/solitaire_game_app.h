#ifndef SOLITAIRE_GAME_APP_H
#define SOLITAIRE_GAME_APP_H

#include "base_app.h"
#include <vector>
#include <array>

/**
 * @file solitaire_game_app.h
 * @brief Klondike Solitaire game for M5Stack Tab5
 * 
 * Features classic Klondike Solitaire with:
 * - Standard 52-card deck
 * - Seven tableau piles
 * - Four foundation piles (Ace to King by suit)
 * - Stock and waste piles
 * - Touch-based card movement
 * - Auto-complete when possible
 * - Score tracking and timer
 */

enum class Suit {
    HEARTS = 0,
    DIAMONDS = 1,
    CLUBS = 2,
    SPADES = 3
};

enum class Rank {
    ACE = 1,
    TWO = 2, THREE = 3, FOUR = 4, FIVE = 5, SIX = 6,
    SEVEN = 7, EIGHT = 8, NINE = 9, TEN = 10,
    JACK = 11, QUEEN = 12, KING = 13
};

struct Card {
    Suit suit;
    Rank rank;
    bool faceUp;
    bool isRed() const { return suit == Suit::HEARTS || suit == Suit::DIAMONDS; }
    std::string toString() const;
    uint16_t getColor() const;
    char getSuitSymbol() const;
    std::string getRankString() const;
};

enum class PileType {
    STOCK,      // Draw pile
    WASTE,      // Discard pile
    FOUNDATION, // Four suit piles (Ace to King)
    TABLEAU     // Seven main playing piles
};

struct Pile {
    PileType type;
    std::vector<Card> cards;
    int index; // For foundation (0-3) and tableau (0-6) piles
    lv_obj_t* obj = nullptr;
    
    bool canAddCard(const Card& card) const;
    bool isEmpty() const { return cards.empty(); }
    Card* getTopCard() { return isEmpty() ? nullptr : &cards.back(); }
    const Card* getTopCard() const { return isEmpty() ? nullptr : &cards.back(); }
};

enum class GameState {
    PLAYING,
    WON,
    PAUSED
};

class SolitaireGameApp : public BaseApp {
public:
    SolitaireGameApp();
    ~SolitaireGameApp() override;

    os_error_t initialize() override;
    os_error_t update(uint32_t deltaTime) override;
    os_error_t shutdown() override;
    os_error_t createUI(lv_obj_t* parent) override;
    os_error_t destroyUI() override;

private:
    // Game logic
    void newGame();
    void dealCards();
    void shuffleDeck();
    bool moveCard(Pile* from, Pile* to, int cardIndex = -1);
    bool canMoveCard(const Card& card, const Pile& to) const;
    void flipStockCards();
    void checkAutoComplete();
    void autoMoveToFoundation();
    bool isGameWon() const;
    void updateScore(int points);
    
    // UI creation
    void createGameArea();
    void createStatusBar();
    void createMenuButtons();
    void updateUI();
    void updatePileUI(Pile& pile);
    void updateCardDisplay(lv_obj_t* cardObj, const Card& card);
    void createCardObject(lv_obj_t* parent, const Card& card, int index = 0);
    
    // Card interaction
    void handleCardClick(lv_obj_t* cardObj);
    void handlePileClick(Pile* pile);
    void selectCard(Pile* pile, int cardIndex);
    void deselectCard();
    bool hasSelection() const { return m_selectedPile != nullptr; }
    
    // Animation and effects
    void animateCardMove(lv_obj_t* cardObj, lv_coord_t x, lv_coord_t y);
    void celebrateWin();
    
    // Event handlers
    static void cardClickCallback(lv_event_t* e);
    static void pileClickCallback(lv_event_t* e);
    static void newGameCallback(lv_event_t* e);
    static void hintCallback(lv_event_t* e);
    static void undoCallback(lv_event_t* e);
    static void autoCompleteCallback(lv_event_t* e);
    
    // Game state
    std::array<Card, 52> m_deck;
    Pile m_stock;           // Draw pile
    Pile m_waste;           // Discard pile  
    std::array<Pile, 4> m_foundation; // Ace to King by suit
    std::array<Pile, 7> m_tableau;    // Main playing area
    
    GameState m_gameState;
    int m_score;
    uint32_t m_gameTime;
    uint32_t m_moves;
    
    // Selection state
    Pile* m_selectedPile;
    int m_selectedCardIndex;
    
    // Move history for undo
    struct Move {
        Pile* from;
        Pile* to;
        int cardIndex;
        bool wasFlipped;
    };
    std::vector<Move> m_moveHistory;
    
    // UI elements
    lv_obj_t* m_gameArea = nullptr;
    lv_obj_t* m_statusBar = nullptr;
    lv_obj_t* m_scoreLabel = nullptr;
    lv_obj_t* m_timeLabel = nullptr;
    lv_obj_t* m_movesLabel = nullptr;
    
    // Menu buttons
    lv_obj_t* m_newGameBtn = nullptr;
    lv_obj_t* m_hintBtn = nullptr;
    lv_obj_t* m_undoBtn = nullptr;
    lv_obj_t* m_autoCompleteBtn = nullptr;
    
    // Card dimensions and layout
    static constexpr int CARD_WIDTH = 60;
    static constexpr int CARD_HEIGHT = 84;
    static constexpr int CARD_SPACING = 8;
    static constexpr int PILE_SPACING = 75;
    static constexpr int TABLEAU_OFFSET = 20;
    
    // Colors
    static constexpr uint16_t COLOR_RED = 0xF800;
    static constexpr uint16_t COLOR_BLACK = 0x0000;
    static constexpr uint16_t COLOR_CARD_BG = 0xFFFF;
    static constexpr uint16_t COLOR_CARD_BACK = 0x001F;
    static constexpr uint16_t COLOR_GREEN_FELT = 0x0400;
    static constexpr uint16_t COLOR_SELECTED = 0xFFE0;
};

#endif // SOLITAIRE_GAME_APP_H