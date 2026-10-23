#ifndef __CARDGAMES_H__
#define __CARDGAMES_H__

class Image;
class Text;
class Entity;
class EntityDef;
class ScriptThread;
class Graphics;

class fmButtonContainer;
class fmScrollButton;
class fmButton;
class fmSwipeArea;

class CardGames
{
private:

public:
	static constexpr int GRAVITY = 10;
	static constexpr int DEF_VELOCITY = 1024;
	static constexpr int NUM_POKER_CARDS = 51;
	static constexpr int CARD_W = 34;
	static constexpr int CARD_H = 50;
	static constexpr int SUITSTART = 112;

	static constexpr int CARD_ROT0 = 1;
	static constexpr int CARD_ROT90 = 2;
	static constexpr int CARD_ROT180 = 4;
	static constexpr int CARD_ROT270 = 8;
	static constexpr int CARD_FACE_UP = 16;
	static constexpr int CARD_LERPING = 32;
	static constexpr int CARD_EXLERP = 64;
	static constexpr int CARD_HIDDEN = 128;
	static constexpr int CARD_PLAYER = 256;
	static constexpr int CARD_DEALER = 512;
	static constexpr int CARD_DARKEN = 1024;

	static constexpr int CARD_DESC = 0;
	static constexpr int CARD_SRCX = 1;
	static constexpr int CARD_SRCY = 2;
	static constexpr int CARD_DSTX = 3;
	static constexpr int CARD_DSTY = 4;
	static constexpr int CARD_FLAGS = 5;
	static constexpr int CARD_START = 6;
	static constexpr int CARD_LERP_DUR = 7;
	static constexpr int CARD_FIELDS = 8;
	static constexpr int MAX_CARDS = 24;

	static constexpr int PART_X = 0;
	static constexpr int PART_Y = 1;
	static constexpr int PART_VX = 2;
	static constexpr int PART_VY = 3;
	static constexpr int PART_ALIVE = 4;
	static constexpr int PART_FIELDS = 5;

	static constexpr int CHIP_OWNER = 0;
	static constexpr int CHIP_SRCX = 1;
	static constexpr int CHIP_SRCY = 2;
	static constexpr int CHIP_DSTX = 3;
	static constexpr int CHIP_DSTY = 4;
	static constexpr int CHIP_START = 5;
	static constexpr int CHIP_DUR = 6;
	static constexpr int CHIP_FIELDS = 7;
	static constexpr int MAX_CHIPS = 20;

	static constexpr int GAME_STATE = 0;
	static constexpr int PREV_STATE = 1;
	static constexpr int STATE_TIME = 2;
	static constexpr int CUR_BET = 3;
	static constexpr int MSG_ID = 4;
	static constexpr int SUB_STATE = 5;
	static constexpr int CARD_SEL_IDX = 6;
	static constexpr int DEALER_SEL_IDX = 7;
	static constexpr int MSG_TIME = 8;

	static constexpr int INVALID_CARD = -1;
	static constexpr int DEFAULT_TREASURE = 100;
	static constexpr int NUM_WAR_CARDS = 6;
	static constexpr int NUM_CARD_ROWS = 3;

	static constexpr int BORDER_W = 175;
	static constexpr int BORDER_H = 70;
	static constexpr int CHIP_W = 34;
	static constexpr int CHIP_H = 20;
	static constexpr int CHIP_PART_H = 12; 
	static constexpr int PORTRAIT_WH = 54;
	static constexpr int FONT_W = 9;
	static constexpr int FONT_H = 12;
	static constexpr int MAX_LERPING = 3;
	static constexpr int DEF_SPEED = 128;

	static constexpr int PLAYER_CHIP = 1;
	static constexpr int DEALER_CHIP = 2;

	static constexpr int WAR_BET = 0;
	static constexpr int WAR_TOSS_CHIPS = 1;
	static constexpr int WAR_DEAL = 2;
	static constexpr int WAR_PLAYERCHOOSE = 3;
	static constexpr int WAR_DEALERCHOOSE = 4;
	static constexpr int WAR_RESOLVE = 5;
	static constexpr int WAR_REDEAL = 6;
	static constexpr int WAR_PLAYERDISCARD = 7;
	static constexpr int WAR_DEALERDISCARD = 8;
	static constexpr int WAR_PICKWARCARD = 9;
	static constexpr int WAR_RESOLVEFINAL = 10;
	static constexpr int WAR_MESSAGE = 11;

	static constexpr int PLAYER_TURN = ((1 << WAR_BET) | (1 << WAR_PLAYERCHOOSE) | (1 << WAR_PLAYERDISCARD)); // 137
	static constexpr int DEALER_TURN = ((1 << WAR_DEALERCHOOSE) | (1 << WAR_DEALERDISCARD)); // 272
	static constexpr int DEALER_MOVE_TIMEOUT = 750;

	int touchMe;
	Image* imgOtherImgs;
	Image* imgTable;
	int numPlayerCards;
	int numDealerCards;
	short playersCards[12];
	short dealersCards[12];
	int cardsInPlay[CardGames::MAX_CARDS * CardGames::CARD_FIELDS];
	int numCardsInPlay;
	long long deckMask;
	int numCardsLerping;
	int backgroundColor;
	ScriptThread* callingThread;
	bool savePlayer;
	int playerTreasure;
	int aiTreasure;
	int playerChips;
	int dealerChips;
	int startingChips;
	int chipAmount;
	int betPercentage;
	int CARDS_X;
	int CARDS_Y;
	int NUM_CARDS_W;
	int NUM_CARDS_H;
	int WAR_CARDS_X;
	int START_BTM_BG;
	int PORTRAIT_Y;
	int xMin;
	int xMaxW;
	int BJPortX;
	int BJChipsX;
	int GuntherPortX;
	int GuntherChipsX;
	int chipsY;
	int CARD_TEXT_Y;
	int CARD_WAR_TEXT_Y;
	int curCardsW;
	int curCardsH;
	Image* flakImg;
	bool animationDone;
	int cardsDiscarded;
	int particleFrameTime;
	int numParticles;
	short particles[CardGames::MAX_CARDS * CardGames::PART_FIELDS];
	int numChipsLerping;
	int numChips;
	int chips[CardGames::MAX_CHIPS * CardGames::CHIP_FIELDS];
	int* stateVars;
	Image* imgUpPointerNormal;
	Image* imgUpPointerPressed;
	Image* imgDownPointerNormal;
	Image* imgDownPointerPressed;
	Image* imgSwitchUp;
	Image* imgSwitchDown;
	fmButtonContainer* m_cardGamesButtons;
	bool touched;
	bool wasTouched;

	// Constructor
	CardGames();
	// Destructor
	~CardGames();

	void initGame(int n, ScriptThread* callingThread, bool savePlayer);
	void resetHand();
	void endGame(int n);
	bool giveTreasure(int n, int n2);
	void nextState();
	void prevState();
	void setState(int state);
	void updateGame(Graphics* graphics);
	void dealerPickCard();
	bool dealerMoveSelector();
	void handleInput(int n, int n2);
	void dealFullDeck();
	void dealWarHand();
	void reDealCard(int field, int flags, int n3);
	void dealCard(int n, int srcX, int srcY, int destX, int destY, int flags, int speed);
	void drawCardHud(Graphics* graphics);
	void drawChips(Graphics* graphics);
	void drawCards(Graphics* graphics);
	void drawCard(Graphics* graphics, int x, int y, int n3, int flags);
	void clearParticles();
	void drawParticles(Graphics* graphics);
	bool updateParticle(int filed, int n2);
	void updateLerpingCard(int n, int* posX, int* posY);
	void allocChip(int srcX, int srcY, int dstX, int dstY, int start, int dur, int owner);
	void setChips(int n, int n2);
	void drawPlayingBG(Graphics* graphics);
	void updateWar(Graphics* graphics);
	void WAR_BET_FUNC(Graphics* graphics);
	void WAR_TOSS_CHIPS_FUNC();
	void WAR_DEAL_FUNC();
	void WAR_PLAYERCHOOSE_FUNC(Graphics* graphics);
	void WAR_PICKWARCARD_FUNC(Graphics* graphics, int n);
	void WAR_DEALERCHOOSE_FUNC(Graphics* graphics, int n);
	void WAR_DEALERDISCARD_FUNC(Graphics* graphics, int n);
	void WAR_REDEAL_FUNC(Graphics* graphics, int n);
	void WAR_PLAYERDISCARD_FUNC(Graphics* graphics);
	void WAR_RESOLVE_FUNC(Graphics* graphics, int n);
	void WAR_MESSAGE_FUNC(Graphics* graphics);
	void drawSelector(Graphics* graphics);
	int dealerAI(int n);
	void handleWarInput(int n, int n2);
	void updateBetPerc();
	void explodeCards();
	void shutdown();
	void touchStart(int pressX, int pressY);
	void touchMove(int pressX, int pressY);
	int touchToKey(int pressX, int pressY);
};

#endif