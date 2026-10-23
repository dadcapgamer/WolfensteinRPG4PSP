#ifndef __MENUSYSTEM_H__
#define __MENUSYSTEM_H__

#include "Text.h"
#include "MenuItem.h"
#include "MenuStrings.h"

class EntityDef;
class Image;
class MenuItem;
class Text;
class fmButtonContainer;
class fmScrollButton;
class Graphics;
class MenuSystem;

class MenuSystem
{
private:

public:
    static constexpr int NO = 0;
    static constexpr int YES = 1;

    static constexpr int INDEX_TREASURE = 0;
    static constexpr int INDEX_WEAPONS = 1;
    static constexpr int INDEX_SYRINGES = 2;
    static constexpr int INDEX_OTHER = 3;
    static constexpr int INDEX_ITEMS = 4;
    static constexpr int INDEX_BOOKS = 5;
    static constexpr int MAX_SAVED_INDEXES = 6;

    static constexpr int BLOCK_LINE_MASK = 63;
    static constexpr int BLOCK_NUMLINES_SHIFT = 26;
    static constexpr int BLOCK_CURLINE_SHIFT = 20;
    static constexpr int BLOCK_OFS_MASK = 1023;
    static constexpr int BLOCK_CHARS_TO_DRAW_SHIFT = 10;

    static constexpr int MAX_MENUITEMS = 50;
    static constexpr int EMPTY_TEXT = Localization::STRINGID(Strings::FILE_MENUSTRINGS, MenuStrings::EMPTY_STRING);

    static constexpr int MAXSTACKCOUNT = 10;
    static constexpr int TOP_Y = 1;
    static constexpr int MAX_MORE_GAMES = 3;

    static constexpr int SM_MEDAL_W = 11;
    static constexpr int SM_MEDAL_H = 12;
    static constexpr int SM_MEDAL_PAD = 5;

    static constexpr int RANK1 = 16000;
    static constexpr int RANK2 = 19000;
    static constexpr int RANK3 = 24000;
    static constexpr int RANK4 = 26000; // [GEC]

    int touchMe;
    uint32_t* menuData;
    uint32_t menuDataCount;
    uint32_t* menuItems;
    uint32_t menuItemsCount;
    short LEVEL_STATS_nextMap;
    int menuParam;
    EntityDef* detailsDef;
    int indexes[MenuSystem::MAX_SAVED_INDEXES * 2];
    Image* imgScroll;
    Image* imgMainBG;
    Image* imgLogo;
    Image* background;
    Image* imgMedal;
    bool drawLogo;
    int lastOffer;
    MenuItem items[MenuSystem::MAX_MENUITEMS];
    int numItems;
    int menu;
    int oldMenu;
    int selectedIndex;
    int scrollIndex;
    int type;
    int maxItems;
    int cheatCombo;
    int startTime;
    int menuMode;
    int stackCount;
    int menuStack[MenuSystem::MAXSTACKCOUNT];
    uint8_t menuIdxStack[MenuSystem::MAXSTACKCOUNT];
    int poppedIdx[1];
    Text* detailsTitle;
    Text* detailsHelpText;
    int medalsMap;
    bool goBackToStation;
    int moreGamesPage;
    bool changeSfxVolume;
    int clipRect[4];
    int oldLanguageSetting;
    int sfxVolumeScroll;
    int musicVolumeScroll;
    int alphaScroll;
    bool updateSlider;
    int sliderID;
    bool drawHelpText;
    int selectedHelpIndex;
    fmButtonContainer* m_menuButtons;
    fmButtonContainer* m_infoButtons;
    fmScrollButton* m_scrollBar;
    int8_t* menuDialA_Anim1;
    int8_t* menuDialC_Anim1;
    int8_t* menuDialA_Anim2;
    int8_t* menuDialC_Anim2;
    int8_t* menuSlideAnim1;
    int8_t* menuSlideAnim2;
    int dialA_Anim1;
    int dialC_Anim1;
    int dialA_Anim2;
    int dialC_Anim2;
    int slideAnim1;
    int slideAnim2;
    int animTime;
    bool isMainMenuScrollBar;
    bool isMainMenu;
    int menuItem_fontPaddingBottom;
    int menuItem_paddingBottom;
    int menuItem_height;
    int menuItem_width;
    Image* imgMenuButtonBackground;
    Image* imgMenuButtonBackgroundOn;
    Image* imgMenuArrowDown;
    Image* imgMenuArrowUp;
    Image* imgMenuDial;
    Image* imgMenuDialOff;
    Image* imgMenuMainBOX;
    Image* imgMainMenuOverLay;
    Image* imgMainHelpOverLay;
    Image* imgMainAboutOverLay;
    Image* imgMenuYesNoBOX;
    Image* imgMenuChooseDIFFBOX;
    Image* imgMenuLanguageBOX;
    Image* imgSwitchUp;
    Image* imgSwitchDown;
    Image* imgMenuOptionBOX3;
    Image* imgMenuOptionBOX4;
    Image* imgMenuOptionSliderON;
    Image* imgMenuOptionSliderOFF;
    Image* imgHudNumbers;
    Image* imgGameMenuPanelbottom;
    Image* imgGameMenuHealth;
    Image* imgGameMenuShield;
    Image* imgGameMenuInfoButtonPressed;
    Image* imgGameMenuInfoButtonNormal;
    Image* imgVendingButtonHelp;
    Image* imgGameMenuTornPage;
    Image* imgMainMenuDialA_Anim;
    Image* imgMainMenuDialC_Anim;
    Image* imgMainMenuSlide_Anim;
    Image* imgGameMenuScrollBar;
    Image* imgGameMenuTopSlider;
    Image* imgGameMenuMidSlider;
    Image* imgGameMenuBottomSlider;

    // [GEC]
    bool changeMusicVolume;
    bool changeButtonsAlpha;
    bool changeValues;
    int old_0x44;
    int old_0x48;
    int scrollY1Stack[MenuSystem::MAXSTACKCOUNT];
    int scrollY2Stack[MenuSystem::MAXSTACKCOUNT];
    int scrollI2Stack[MenuSystem::MAXSTACKCOUNT];
    int nextMsgTime;
    int nextMsg;
    bool setBinding;
    Image* imgMenuButtonBackgroundExt; // [GEC]
    Image* imgMenuButtonBackgroundExtOn; // [GEC]
    Image* imgMenuVideoBOX; // [GEC]
    Image* imgMenuPlaque; // [GEC]
    Image* imgMenuEmptyPlaque; // [GEC]
    Image* imgMenuPlaqueExt; // [GEC]
    Image* imgMenuEmptyPlaqueExt; // [GEC]
    Image* imgMenuPlaqueBG; // [GEC]
    Image* imgMenuEmptyPlaqueBG; // [GEC]
    Image* imgMenuOptionSliderBar; // [GEC]

    Image* imgMenuBtnBackground; // [GEC]
    Image* imgMenuBtnBackgroundOn; // [GEC]
    bool changeVibrationIntensity; // [GEC]
    bool changeDeadzone; // [GEC]
    int vibrationIntensityScroll; // [GEC]
    int deadzoneScroll; // [GEC]
    int resolutionIndex; // [GEC]


    int menuDialA_Anim1_Frames; // [GEC]
    int menuDialC_Anim1_Frames; // [GEC]
    int menuDialA_Anim2_Frames; // [GEC]
    int menuDialC_Anim2_Frames; // [GEC]
    int menuSlideAnim1_Frames; // [GEC]
    int menuSlideAnim2_Frames; // [GEC]

    // Constructor
    MenuSystem();
    // Destructor
    ~MenuSystem();

    bool startup();
    void buildDivider(Text* text, int i);
    bool enterDigit(int i);
    void scrollDown();
    void scrollUp();
    bool scrollPageDown();
    void scrollPageUp();
    bool shiftBlockText(int n, int i, int j);
    void moveDir(int n);
    void doDetailsSelect();
    void back();
    void setMenu(int menu);
    void paint(Graphics* graphics);
    void setItemsFromText(int i, Text* text, int i2, int i3, int i4);
    void returnToGame();
    void initMenu(int menu);
    void gotoMenu(int menu);
    void handleMenuEvents(int key, int keyAction);
    void select(int i);
    void infiniteLoop();
    int infiniteRecursion(int* array);
    void systemTest(int sysType);
    void startGame(bool b);
    void SetYESNO(short textField, int i, int action, int param);
    void SetYESNO(short textField, int i, int action, int param, int action2, int param2);
    void SetYESNO(Text* text, int i, int action, int param, int action2, int param2);
    void LoadHelpResource(short i);
    void LoadAbout();
    void FillRanking();
    void LoadRecipes();
    void LoadNotebook();
    void LoadHelpItems(Text* text, int i);
    void buildFraction(int i, int i2, Text* text);
    void buildModStat(int i, int i2, Text* text);

    void fillStatus(bool b, bool b2, bool b3);
    void saveIndexes(int i);
    void loadIndexes(int i);
    void showDetailsMenu();
    void addItem(int textField, int textField2, int flags, int action, int param, int helpField);
    void addItem(int textField, int textField2, int flags);
    void loadMenuItems(int menu, int begItem, int numItems);
    int onOffValue(bool b);
    void leaveOptionsMenu();
    int getStackCount();
    void clearStack();
    void pushMenu(int i, int i2, int Y1, int Y2, int index2);
    int popMenu(int* array, int* Y1, int* Y2, int* index2);
    int peekMenu();
    int getLastArgString();
    void loadMedalItems(int n, bool b, bool b2, bool b3);
    void getMedalFraction(int n, bool b, Text* text);
    void loadBookList();
    void loadBook(int n);
    void nextGamePage();
    void prevGamePage();

    void setMenuSettings();
    void updateTouchButtonState();
    void handleUserTouch(int x, int y, bool b);
    void handleUserMoved(int x, int y);
    int getScrollPos();
    int getMenuItemHeight(int i);
    int getMenuItemHeight2(int i); // [GEC]
    void drawScrollbar(Graphics* graphics);
    void drawButtonFrame(Graphics* graphics);
    void drawTouchButtons(Graphics* graphics, bool b);
    void drawSoftkeyButtons(Graphics* graphics);
    int drawCustomScrollbar(Graphics* graphics, MenuItem* item, Text* text, int yPos); // [GEC]
    void drawOptionsScreen(Graphics* graphics);
    void drawNumbers(Graphics* graphics, int x, int y, int space, int number);
    bool HasVibration();
    bool isUserMusicOn();
    bool updateVolumeSlider(int buttonId, int x);
    void refresh();
    void soundClick();
};

#endif