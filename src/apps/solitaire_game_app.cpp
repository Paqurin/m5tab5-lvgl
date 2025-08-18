#include "solitaire_game_app.h"
#include "../system/os_manager.h"
#include <esp_log.h>
#include <algorithm>
#include <random>
#include <sstream>

static const char* TAG = "SolitaireGame";

// Card implementation
std::string Card::toString() const {
    return getRankString() + getSuitSymbol();
}

uint16_t Card::getColor() const {
    return isRed() ? 0xF800 : 0x0000; // Red : Black
}

char Card::getSuitSymbol() const {
    switch (suit) {
        case Suit::HEARTS:   return 'H';
        case Suit::DIAMONDS: return 'D';
        case Suit::CLUBS:    return 'C';
        case Suit::SPADES:   return 'S';
        default: return '?';
    }
}

std::string Card::getRankString() const {
    switch (rank) {
        case Rank::ACE:   return "A";
        case Rank::TWO:   return "2";
        case Rank::THREE: return "3";
        case Rank::FOUR:  return "4";
        case Rank::FIVE:  return "5";
        case Rank::SIX:   return "6";
        case Rank::SEVEN: return "7";
        case Rank::EIGHT: return "8";
        case Rank::NINE:  return "9";
        case Rank::TEN:   return "10";
        case Rank::JACK:  return "J";
        case Rank::QUEEN: return "Q";
        case Rank::KING:  return "K";
        default: return "?";
    }
}

// Pile implementation
bool Pile::canAddCard(const Card& card) const {
    if (type == PileType::FOUNDATION) {
        if (isEmpty()) {
            return card.rank == Rank::ACE;
        }
        const Card* topCard = getTopCard();
        return (topCard->suit == card.suit && 
                static_cast<int>(card.rank) == static_cast<int>(topCard->rank) + 1);
    }
    else if (type == PileType::TABLEAU) {
        if (isEmpty()) {
            return card.rank == Rank::KING;
        }
        const Card* topCard = getTopCard();
        return (topCard->isRed() != card.isRed() && 
                static_cast<int>(card.rank) == static_cast<int>(topCard->rank) - 1);
    }
    return false;
}

SolitaireGameApp::SolitaireGameApp() : BaseApp("solitaire", "Solitaire", "1.0") {
    m_gameState = GameState::PLAYING;
    m_score = 0;
    m_gameTime = 0;
    m_moves = 0;
    m_selectedPile = nullptr;
    m_selectedCardIndex = -1;
    
    // Initialize piles
    m_stock.type = PileType::STOCK;
    m_stock.index = 0;
    m_waste.type = PileType::WASTE;
    m_waste.index = 0;
    
    for (int i = 0; i < 4; i++) {
        m_foundation[i].type = PileType::FOUNDATION;
        m_foundation[i].index = i;
    }
    
    for (int i = 0; i < 7; i++) {
        m_tableau[i].type = PileType::TABLEAU;
        m_tableau[i].index = i;
    }
}

SolitaireGameApp::~SolitaireGameApp() {
    shutdown();
}

os_error_t SolitaireGameApp::initialize() {
    if (m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Initializing Solitaire Game");
    
    newGame();
    
    m_initialized = true;
    ESP_LOGI(TAG, "Solitaire Game initialized");
    return OS_OK;
}

os_error_t SolitaireGameApp::update(uint32_t deltaTime) {
    if (!m_initialized) {
        return OS_ERROR_NOT_INITIALIZED;
    }
    
    if (m_gameState == GameState::PLAYING) {
        m_gameTime += deltaTime;
        
        // Update time display every second
        static uint32_t lastTimeUpdate = 0;
        if (millis() - lastTimeUpdate > 1000) {
            if (m_timeLabel) {
                int minutes = m_gameTime / 60000;
                int seconds = (m_gameTime / 1000) % 60;
                char timeStr[16];
                snprintf(timeStr, sizeof(timeStr), "%02d:%02d", minutes, seconds);
                lv_label_set_text(m_timeLabel, timeStr);
            }
            lastTimeUpdate = millis();
        }
    }
    
    return OS_OK;
}

os_error_t SolitaireGameApp::shutdown() {
    if (!m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Shutting down Solitaire Game");
    destroyUI();
    
    m_initialized = false;
    return OS_OK;
}

os_error_t SolitaireGameApp::createUI(lv_obj_t* parent) {
    if (!parent) {
        return OS_ERROR_INVALID_PARAM;
    }

    m_uiContainer = lv_obj_create(parent);
    lv_obj_set_size(m_uiContainer, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(m_uiContainer, lv_color_hex(0x0400), 0); // Green felt
    lv_obj_clear_flag(m_uiContainer, LV_OBJ_FLAG_SCROLLABLE);

    createStatusBar();
    createMenuButtons();
    createGameArea();
    updateUI();

    ESP_LOGI(TAG, "Solitaire UI created");
    return OS_OK;
}

os_error_t SolitaireGameApp::destroyUI() {
    if (m_uiContainer) {
        lv_obj_del(m_uiContainer);
        m_uiContainer = nullptr;
        m_gameArea = nullptr;
        m_statusBar = nullptr;
        m_scoreLabel = nullptr;
        m_timeLabel = nullptr;
        m_movesLabel = nullptr;
        m_newGameBtn = nullptr;
        m_hintBtn = nullptr;
        m_undoBtn = nullptr;
        m_autoCompleteBtn = nullptr;
    }
    return OS_OK;
}

void SolitaireGameApp::newGame() {
    ESP_LOGI(TAG, "Starting new solitaire game");
    
    // Clear all piles
    m_stock.cards.clear();
    m_waste.cards.clear();
    for (auto& pile : m_foundation) {
        pile.cards.clear();
    }
    for (auto& pile : m_tableau) {
        pile.cards.clear();
    }
    
    // Initialize deck
    int cardIndex = 0;
    for (int suit = 0; suit < 4; suit++) {
        for (int rank = 1; rank <= 13; rank++) {
            m_deck[cardIndex].suit = static_cast<Suit>(suit);
            m_deck[cardIndex].rank = static_cast<Rank>(rank);
            m_deck[cardIndex].faceUp = false;
            cardIndex++;
        }
    }
    
    shuffleDeck();
    dealCards();
    
    // Reset game state
    m_gameState = GameState::PLAYING;
    m_score = 0;
    m_gameTime = 0;
    m_moves = 0;
    m_selectedPile = nullptr;
    m_selectedCardIndex = -1;
    m_moveHistory.clear();
    
    updateUI();
}

void SolitaireGameApp::shuffleDeck() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(m_deck.begin(), m_deck.end(), gen);
}

void SolitaireGameApp::dealCards() {
    int cardIndex = 0;
    
    // Deal to tableau piles (1, 2, 3, 4, 5, 6, 7 cards)
    for (int pile = 0; pile < 7; pile++) {
        for (int card = 0; card <= pile; card++) {
            m_deck[cardIndex].faceUp = (card == pile); // Only top card face up
            m_tableau[pile].cards.push_back(m_deck[cardIndex]);
            cardIndex++;
        }
    }
    
    // Remaining cards go to stock pile
    for (int i = cardIndex; i < 52; i++) {
        m_deck[i].faceUp = false;
        m_stock.cards.push_back(m_deck[i]);
    }
}

void SolitaireGameApp::createStatusBar() {
    m_statusBar = lv_obj_create(m_uiContainer);
    lv_obj_set_size(m_statusBar, LV_PCT(100), 40);
    lv_obj_align(m_statusBar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(m_statusBar, lv_color_hex(0x333333), 0);
    
    // Score label
    m_scoreLabel = lv_label_create(m_statusBar);
    lv_label_set_text(m_scoreLabel, "Score: 0");
    lv_obj_align(m_scoreLabel, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_set_style_text_color(m_scoreLabel, lv_color_white(), 0);
    
    // Time label
    m_timeLabel = lv_label_create(m_statusBar);
    lv_label_set_text(m_timeLabel, "00:00");
    lv_obj_align(m_timeLabel, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(m_timeLabel, lv_color_white(), 0);
    
    // Moves label
    m_movesLabel = lv_label_create(m_statusBar);
    lv_label_set_text(m_movesLabel, "Moves: 0");
    lv_obj_align(m_movesLabel, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_set_style_text_color(m_movesLabel, lv_color_white(), 0);
}

void SolitaireGameApp::createMenuButtons() {
    // Create button container
    lv_obj_t* btnContainer = lv_obj_create(m_uiContainer);
    lv_obj_set_size(btnContainer, LV_PCT(100), 30);
    lv_obj_align(btnContainer, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_opa(btnContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(btnContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(btnContainer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btnContainer, LV_FLEX_ALIGN_SPACE_EVENLY, 
                         LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // New Game button
    m_newGameBtn = lv_btn_create(btnContainer);
    lv_obj_set_size(m_newGameBtn, 80, 25);
    lv_obj_add_event_cb(m_newGameBtn, newGameCallback, LV_EVENT_CLICKED, this);
    lv_obj_t* newGameLabel = lv_label_create(m_newGameBtn);
    lv_label_set_text(newGameLabel, "New");
    lv_obj_center(newGameLabel);
    
    // Hint button
    m_hintBtn = lv_btn_create(btnContainer);
    lv_obj_set_size(m_hintBtn, 80, 25);
    lv_obj_add_event_cb(m_hintBtn, hintCallback, LV_EVENT_CLICKED, this);
    lv_obj_t* hintLabel = lv_label_create(m_hintBtn);
    lv_label_set_text(hintLabel, "Hint");
    lv_obj_center(hintLabel);
    
    // Undo button
    m_undoBtn = lv_btn_create(btnContainer);
    lv_obj_set_size(m_undoBtn, 80, 25);
    lv_obj_add_event_cb(m_undoBtn, undoCallback, LV_EVENT_CLICKED, this);
    lv_obj_t* undoLabel = lv_label_create(m_undoBtn);
    lv_label_set_text(undoLabel, "Undo");
    lv_obj_center(undoLabel);
    
    // Auto-complete button
    m_autoCompleteBtn = lv_btn_create(btnContainer);
    lv_obj_set_size(m_autoCompleteBtn, 80, 25);
    lv_obj_add_event_cb(m_autoCompleteBtn, autoCompleteCallback, LV_EVENT_CLICKED, this);
    lv_obj_t* autoLabel = lv_label_create(m_autoCompleteBtn);
    lv_label_set_text(autoLabel, "Auto");
    lv_obj_center(autoLabel);
}

void SolitaireGameApp::createGameArea() {
    m_gameArea = lv_obj_create(m_uiContainer);
    lv_obj_set_size(m_gameArea, LV_PCT(100), LV_PCT(100) - 70);
    lv_obj_align(m_gameArea, LV_ALIGN_TOP_MID, 0, 40);
    lv_obj_set_style_bg_opa(m_gameArea, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(m_gameArea, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(m_gameArea, LV_OBJ_FLAG_SCROLLABLE);
}

void SolitaireGameApp::updateUI() {
    if (!m_gameArea) return;
    
    // Clear existing card objects
    lv_obj_clean(m_gameArea);
    
    // Update stock pile
    updatePileUI(m_stock);
    
    // Update waste pile
    updatePileUI(m_waste);
    
    // Update foundation piles
    for (auto& pile : m_foundation) {
        updatePileUI(pile);
    }
    
    // Update tableau piles
    for (auto& pile : m_tableau) {
        updatePileUI(pile);
    }
    
    // Update score and moves
    if (m_scoreLabel) {
        char scoreStr[32];
        snprintf(scoreStr, sizeof(scoreStr), "Score: %d", m_score);
        lv_label_set_text(m_scoreLabel, scoreStr);
    }
    
    if (m_movesLabel) {
        char movesStr[32];
        snprintf(movesStr, sizeof(movesStr), "Moves: %d", m_moves);
        lv_label_set_text(m_movesLabel, movesStr);
    }
}

void SolitaireGameApp::updatePileUI(Pile& pile) {
    if (!m_gameArea) return;
    
    lv_coord_t x = 0, y = 0;
    
    // Calculate pile position
    if (pile.type == PileType::STOCK) {
        x = 10;
        y = 10;
    }
    else if (pile.type == PileType::WASTE) {
        x = 10 + PILE_SPACING;
        y = 10;
    }
    else if (pile.type == PileType::FOUNDATION) {
        x = 10 + (3 + pile.index) * PILE_SPACING;
        y = 10;
    }
    else if (pile.type == PileType::TABLEAU) {
        x = 10 + pile.index * PILE_SPACING;
        y = 10 + CARD_HEIGHT + 20;
    }
    
    // Create pile base (empty pile indicator)
    lv_obj_t* pileBase = lv_obj_create(m_gameArea);
    lv_obj_set_size(pileBase, CARD_WIDTH, CARD_HEIGHT);
    lv_obj_set_pos(pileBase, x, y);
    lv_obj_set_style_bg_color(pileBase, lv_color_hex(0x444444), 0);
    lv_obj_set_style_border_color(pileBase, lv_color_hex(0x666666), 0);
    lv_obj_set_style_border_width(pileBase, 1, 0);
    lv_obj_set_style_radius(pileBase, 5, 0);
    lv_obj_add_event_cb(pileBase, pileClickCallback, LV_EVENT_CLICKED, &pile);
    
    // Add cards to pile
    for (size_t i = 0; i < pile.cards.size(); i++) {
        const Card& card = pile.cards[i];
        lv_coord_t cardY = y;
        
        // For tableau piles, offset cards vertically
        if (pile.type == PileType::TABLEAU) {
            cardY += i * TABLEAU_OFFSET;
        }
        
        lv_obj_t* cardObj = lv_obj_create(m_gameArea);
        lv_obj_set_size(cardObj, CARD_WIDTH, CARD_HEIGHT);
        lv_obj_set_pos(cardObj, x, cardY);
        updateCardDisplay(cardObj, card);
        
        // Store card reference for interaction
        lv_obj_set_user_data(cardObj, (void*)&pile);
        lv_obj_add_event_cb(cardObj, cardClickCallback, LV_EVENT_CLICKED, this);
    }
}

void SolitaireGameApp::updateCardDisplay(lv_obj_t* cardObj, const Card& card) {
    lv_obj_set_style_bg_color(cardObj, lv_color_hex(0xFFFF), 0); // White card background
    lv_obj_set_style_border_color(cardObj, lv_color_hex(0x0000), 0); // Black border
    lv_obj_set_style_border_width(cardObj, 1, 0);
    lv_obj_set_style_radius(cardObj, 3, 0);
    
    if (card.faceUp) {
        // Show card face
        lv_obj_t* cardLabel = lv_label_create(cardObj);
        lv_label_set_text(cardLabel, card.toString().c_str());
        lv_obj_set_style_text_color(cardLabel, lv_color_hex(card.getColor()), 0);
        lv_obj_center(cardLabel);
    } else {
        // Show card back
        lv_obj_set_style_bg_color(cardObj, lv_color_hex(0x001F), 0); // Blue card back
        lv_obj_t* backLabel = lv_label_create(cardObj);
        lv_label_set_text(backLabel, "###");
        lv_obj_set_style_text_color(backLabel, lv_color_white(), 0);
        lv_obj_center(backLabel);
    }
}

// Event handlers
void SolitaireGameApp::cardClickCallback(lv_event_t* e) {
    SolitaireGameApp* app = static_cast<SolitaireGameApp*>(lv_event_get_user_data(e));
    lv_obj_t* cardObj = lv_event_get_target(e);
    app->handleCardClick(cardObj);
}

void SolitaireGameApp::pileClickCallback(lv_event_t* e) {
    Pile* pile = static_cast<Pile*>(lv_event_get_user_data(e));
    // Handle pile click (for empty piles or stock flipping)
}

void SolitaireGameApp::newGameCallback(lv_event_t* e) {
    SolitaireGameApp* app = static_cast<SolitaireGameApp*>(lv_event_get_user_data(e));
    app->newGame();
}

void SolitaireGameApp::hintCallback(lv_event_t* e) {
    // TODO: Implement hint system
}

void SolitaireGameApp::undoCallback(lv_event_t* e) {
    // TODO: Implement undo functionality
}

void SolitaireGameApp::autoCompleteCallback(lv_event_t* e) {
    SolitaireGameApp* app = static_cast<SolitaireGameApp*>(lv_event_get_user_data(e));
    app->autoMoveToFoundation();
}

void SolitaireGameApp::handleCardClick(lv_obj_t* cardObj) {
    Pile* pile = static_cast<Pile*>(lv_obj_get_user_data(cardObj));
    if (!pile) return;
    
    ESP_LOGI(TAG, "Card clicked in pile type %d", (int)pile->type);
    
    // Handle stock pile - flip cards
    if (pile->type == PileType::STOCK) {
        flipStockCards();
        return;
    }
    
    // Handle card selection and movement
    handlePileClick(pile);
}

void SolitaireGameApp::handlePileClick(Pile* pile) {
    if (!hasSelection()) {
        // Select card from this pile
        if (!pile->isEmpty() && pile->getTopCard()->faceUp) {
            selectCard(pile, pile->cards.size() - 1);
        }
    } else {
        // Try to move selected card to this pile
        if (canMoveCard(*m_selectedPile->getTopCard(), *pile)) {
            if (moveCard(m_selectedPile, pile)) {
                m_moves++;
                updateScore(10);
                checkAutoComplete();
                if (isGameWon()) {
                    m_gameState = GameState::WON;
                    celebrateWin();
                }
            }
        }
        deselectCard();
        updateUI();
    }
}

void SolitaireGameApp::selectCard(Pile* pile, int cardIndex) {
    m_selectedPile = pile;
    m_selectedCardIndex = cardIndex;
    ESP_LOGI(TAG, "Selected card at index %d from pile type %d", cardIndex, (int)pile->type);
}

void SolitaireGameApp::deselectCard() {
    m_selectedPile = nullptr;
    m_selectedCardIndex = -1;
}

bool SolitaireGameApp::canMoveCard(const Card& card, const Pile& to) const {
    return to.canAddCard(card);
}

bool SolitaireGameApp::moveCard(Pile* from, Pile* to, int cardIndex) {
    if (!from || !to || from->isEmpty()) {
        return false;
    }
    
    if (cardIndex == -1) {
        cardIndex = from->cards.size() - 1;
    }
    
    if (cardIndex < 0 || cardIndex >= static_cast<int>(from->cards.size())) {
        return false;
    }
    
    Card card = from->cards[cardIndex];
    if (!canMoveCard(card, *to)) {
        return false;
    }
    
    // Move card
    to->cards.push_back(card);
    from->cards.erase(from->cards.begin() + cardIndex);
    
    // Flip next card if necessary
    if (!from->isEmpty() && !from->getTopCard()->faceUp && from->type == PileType::TABLEAU) {
        from->getTopCard()->faceUp = true;
    }
    
    ESP_LOGI(TAG, "Moved card %s from pile %d to pile %d", 
             card.toString().c_str(), (int)from->type, (int)to->type);
    
    return true;
}

void SolitaireGameApp::flipStockCards() {
    if (m_stock.isEmpty()) {
        // Move all waste cards back to stock
        while (!m_waste.isEmpty()) {
            Card card = m_waste.cards.back();
            card.faceUp = false;
            m_stock.cards.push_back(card);
            m_waste.cards.pop_back();
        }
    } else {
        // Flip up to 3 cards from stock to waste
        int cardsToFlip = std::min(3, static_cast<int>(m_stock.cards.size()));
        for (int i = 0; i < cardsToFlip; i++) {
            Card card = m_stock.cards.back();
            card.faceUp = true;
            m_waste.cards.push_back(card);
            m_stock.cards.pop_back();
        }
    }
    updateUI();
}

void SolitaireGameApp::autoMoveToFoundation() {
    bool moved = false;
    
    // Try to move cards to foundation from tableau and waste
    for (auto& pile : m_tableau) {
        if (!pile.isEmpty() && pile.getTopCard()->faceUp) {
            for (auto& foundation : m_foundation) {
                if (canMoveCard(*pile.getTopCard(), foundation)) {
                    moveCard(&pile, &foundation);
                    moved = true;
                    updateScore(15);
                    break;
                }
            }
        }
    }
    
    if (!m_waste.isEmpty()) {
        for (auto& foundation : m_foundation) {
            if (canMoveCard(*m_waste.getTopCard(), foundation)) {
                moveCard(&m_waste, &foundation);
                moved = true;
                updateScore(15);
                break;
            }
        }
    }
    
    if (moved) {
        m_moves++;
        updateUI();
        checkAutoComplete();
        
        if (isGameWon()) {
            m_gameState = GameState::WON;
            celebrateWin();
        }
    }
}

void SolitaireGameApp::checkAutoComplete() {
    autoMoveToFoundation();
}

bool SolitaireGameApp::isGameWon() const {
    for (const auto& foundation : m_foundation) {
        if (foundation.cards.size() != 13) {
            return false;
        }
    }
    return true;
}

void SolitaireGameApp::updateScore(int points) {
    m_score += points;
}

void SolitaireGameApp::celebrateWin() {
    ESP_LOGI(TAG, "Congratulations! Game won in %d moves with score %d", m_moves, m_score);
    
    // TODO: Add celebration animation
    if (m_scoreLabel) {
        lv_label_set_text(m_scoreLabel, "YOU WON!");
        lv_obj_set_style_text_color(m_scoreLabel, lv_color_hex(0x00FF00), 0);
    }
}