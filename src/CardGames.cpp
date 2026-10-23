#include <stdexcept>
#include <cstring>

#include "CAppContainer.h"
#include "App.h"
#include "Button.h"
#include "CardGames.h"
#include "Game.h"
#include "Canvas.h"
#include "Player.h"
#include "Hud.h"
#include "Text.h"
#include "ParticleSystem.h"
#include "JavaStream.h"
#include "Enums.h"
#include "Sound.h"
#include "MenuSystem.h"
#include "Menus.h"
#include "Image.h"

CardGames::CardGames() {
    memset(this, 0, sizeof(CardGames));
}

CardGames::~CardGames() {
}

void CardGames::initGame(int n, ScriptThread* callingThread, bool savePlayer) {
    Applet* app = CAppContainer::getInstance()->app;

    this->stateVars = app->canvas->stateVars;
    this->numCardsLerping = 0;
    this->callingThread = callingThread;
    app->beginImageLoading();
    this->flakImg = app->loadImage("flak.bmp", true);
    this->imgOtherImgs = app->loadImage("WarImages.bmp", true);
    this->imgTable = app->loadImage("WarTable.bmp", true);
    app->finishImageLoading();
    app->canvas->setState(Canvas::ST_MINI_GAME);
    this->xMin = app->canvas->screenRect[2] / 2 - CardGames::BORDER_W;
    this->xMaxW = std::min((CardGames::BORDER_W * 2), app->canvas->screenRect[2]);
    this->BJPortX = this->xMin + 1;
    this->BJChipsX = this->xMin + (CardGames::PORTRAIT_WH + 6);
    this->GuntherPortX = this->xMin + this->xMaxW - (CardGames::PORTRAIT_WH + 1);
    this->GuntherChipsX = this->xMin + this->xMaxW - (CardGames::PORTRAIT_WH + 6) - CardGames::CHIP_W;
    this->chipsY = app->canvas->screenRect[3] - CardGames::CHIP_H;
    app->canvas->scrollWithBarMaxChars = (this->xMaxW - 10) / Applet::CHAR_SPACING[app->fontType];
    this->PORTRAIT_Y = app->canvas->displayRect[3] /*- (CardGames::PORTRAIT_WH - 1)*/;
    this->PORTRAIT_Y = (this->PORTRAIT_Y - CardGames::BORDER_H) /*/ CardGames::BORDER_H * CardGames::BORDER_H*/;
    this->START_BTM_BG = this->PORTRAIT_Y - CardGames::BORDER_H;
    this->CARDS_Y = (this->START_BTM_BG + CardGames::BORDER_H) / 2 - 79;
    this->numParticles = 0;
    this->clearParticles();
    this->numChips = 0;
    this->numChipsLerping = 0;
    app->canvas->clearSoftKeys();
    if (!savePlayer) {
        this->playerTreasure = 100;
    }
    else {
        this->playerTreasure = app->player->gold + app->player->inventory[Enums::INV_TREASURE_CROSS] + app->player->inventory[Enums::INV_TREASURE_GOBLET] + app->player->inventory[Enums::INV_TREASURE_CROWN];
    }
    if (this->playerTreasure > 10) {
        this->chipAmount = (this->playerTreasure << 8) / 10;
        this->playerChips = (this->dealerChips = 10);
    }
    else {
        this->playerChips = (this->dealerChips = this->playerTreasure);
        this->chipAmount = 256;
    }
    this->setChips(this->playerChips, this->dealerChips);
    this->playerTreasure <<= 8;
    this->savePlayer = savePlayer;
    this->startingChips = this->playerChips;
    this->aiTreasure = this->playerTreasure;
    this->NUM_CARDS_H = 3;
    this->NUM_CARDS_W = 8;
    this->CARDS_X = (app->canvas->screenRect[2] - 293) / 2;
    if (this->CARDS_X <= 0) {
        this->NUM_CARDS_W = 7;
        this->CARDS_X = (app->canvas->screenRect[2] - 256) / 2;
    }
    this->WAR_CARDS_X = (app->canvas->screenRect[2] - 219) / 2;
    this->CARD_TEXT_Y = this->CARDS_Y + (159 + 4);
    this->CARD_WAR_TEXT_Y = this->CARDS_Y + 108;
    this->stateVars[CardGames::CUR_BET] = 1;
    this->updateBetPerc();
    this->numCardsInPlay = 0;
    this->resetHand();
    this->setState(CardGames::WAR_BET);

    this->touched = false;
    this->wasTouched = false;

    if (!this->m_cardGamesButtons) {
        this->imgUpPointerNormal = app->loadImage("War_upPointer_normal.bmp", true);
        this->imgUpPointerPressed = app->loadImage("War_upPointer_pressed.bmp", true);
        this->imgDownPointerNormal = app->loadImage("War_downPointer_normal.bmp", true);
        this->imgDownPointerPressed = app->loadImage("War_downPointer_pressed.bmp", true);
        this->imgSwitchUp = app->loadImage("War_SwitchUp.bmp", true);
        this->imgSwitchDown = app->loadImage("War_SwitchDown.bmp", true);

        this->m_cardGamesButtons = new fmButtonContainer();

        fmButton* button = new fmButton(3, 173, 255, 65, 65, -1);
        button->SetImage(this->imgUpPointerNormal, true);
        button->SetHighlightImage(this->imgUpPointerPressed, true);
        this->m_cardGamesButtons->AddButton(button);

        fmButton* button2 = new fmButton(9, 242, 255, 65, 65, -1);
        button2->SetImage(this->imgDownPointerNormal, true);
        button2->SetHighlightImage(this->imgDownPointerPressed, true);
        this->m_cardGamesButtons->AddButton(button2);

        fmButton* button3 = new fmButton(6, 95, 34, 290, 170, -1);
        this->m_cardGamesButtons->AddButton(button3);

        fmButton* button4 = new fmButton(1, 0, 240, 80, 80, -1);
        button4->SetImage(this->imgSwitchUp, true);
        button4->SetHighlightImage(this->imgSwitchDown, true);
        this->m_cardGamesButtons->AddButton(button4);
    }
}

void CardGames::resetHand() {
    for (int i = 0; i < 12; ++i) {
        this->playersCards[i] = -1;
    }
    for (int j = 0; j < 12; ++j) {
        this->dealersCards[j] = -1;
    }
    this->numPlayerCards = 0;
    this->numDealerCards = 0;
}


void CardGames::endGame(int n) {
    Applet* app = CAppContainer::getInstance()->app;
    app->game->scriptStateVars[Enums::CODEVAR_COMMMAND_RETURN] = (short)n;
    int n2 = (n == 2) ? 5 : 10;
    if (this->savePlayer) {
        int n3 = app->player->gold + app->player->inventory[Enums::INV_TREASURE_CROSS] + app->player->inventory[Enums::INV_TREASURE_GOBLET] + app->player->inventory[Enums::INV_TREASURE_CROWN];
        if ((this->playerTreasure & 0x80) != 0x0) {
            this->playerTreasure += 256;
        }
        this->playerTreasure >>= 8;
        int n4 = -1;
        if (this->playerTreasure > n3) {
            n4 = 1;
        }
        for (int n5 = 0; n3 != this->playerTreasure && n5 == 0; n3 += n4) {
            if (!this->giveTreasure(n4, app->nextInt() % 4) && !this->giveTreasure(n4, 0)) {
                n5 = 1;
            }
        }
    }
    if (this->callingThread != nullptr) {
        app->canvas->setState(Canvas::ST_PLAYING);
        this->callingThread->run();
        this->callingThread = nullptr;
        app->player->addXP(n2);
    }
    else {
        app->menuSystem->setMenu(Menus::MENU_MAIN_MINIGAME);
    }

    this->shutdown();
}

bool CardGames::giveTreasure(int n, int n2) {
    Applet* app = CAppContainer::getInstance()->app;
    bool b = false;
    switch (n2) {
    case 0: {
        if (app->player->gold + n > 0) {
            app->player->gold += n;
            b = true;
            break;
        }
        break;
    }
    case 1: {
        if (app->player->inventory[Enums::INV_TREASURE_CROSS] + n > 0) {
            app->player->inventory[Enums::INV_TREASURE_CROSS] += (short)n;
            b = true;
            break;
        }
        break;
    }
    case 2: {
        if (app->player->inventory[Enums::INV_TREASURE_GOBLET] + n > 0) {
            app->player->inventory[Enums::INV_TREASURE_GOBLET] += (short)n;
            b = true;
            break;
        }
        break;
    }
    case 3: {
        if (app->player->inventory[Enums::INV_TREASURE_CROWN] + n > 0) {
            app->player->inventory[Enums::INV_TREASURE_CROWN] += (short)n;
            b = true;
            break;
        }
        break;
    }
    }
    return b;
}

void CardGames::nextState() {
    this->setState(this->stateVars[CardGames::GAME_STATE] + 1);
}

void CardGames::prevState() {
    this->setState(this->stateVars[CardGames::GAME_STATE] - 1);
}

void CardGames::setState(int state) {
    Applet* app = CAppContainer::getInstance()->app;
    int curState = this->stateVars[CardGames::GAME_STATE];
    this->stateVars[CardGames::SUB_STATE] = 0;
    this->stateVars[CardGames::PREV_STATE] = curState;
    this->stateVars[CardGames::GAME_STATE] = state;
    bool b = !this->savePlayer || (app->player->gamePlayedMask & 0x1) != 0x0;
    if (state == CardGames::WAR_BET) {
        if (b) {
            app->canvas->setLeftSoftKey(Strings::FILE_CODESTRINGS, CodeStrings::DIALOG_EXIT);
        }
        app->canvas->setRightSoftKey(Strings::FILE_CODESTRINGS, CodeStrings::CONTINUE);
    }
}

void CardGames::updateGame(Graphics* graphics) {
    this->updateWar(graphics);
}

void CardGames::dealerPickCard() {
    Applet* app = CAppContainer::getInstance()->app;
    int i;
    int n;
    for (n = (i = app->nextInt() % this->numCardsInPlay); i >= 0; --i) {
        if (!(this->cardsInPlay[(i * CardGames::CARD_FIELDS) + CardGames::CARD_FLAGS] & (CardGames::CARD_FACE_UP | CardGames::CARD_HIDDEN | CardGames::CARD_DARKEN))) {
            this->stateVars[CardGames::DEALER_SEL_IDX] = i;
            return;
        }
    }
    for (int j = n + 1; j < this->numCardsInPlay; ++j) {
        if (!(this->cardsInPlay[(j * CardGames::CARD_FIELDS) + CardGames::CARD_FLAGS] & (CardGames::CARD_FACE_UP | CardGames::CARD_HIDDEN | CardGames::CARD_DARKEN))) {
            this->stateVars[CardGames::DEALER_SEL_IDX] = j;
            return;
        }
    }
    app->Error("Cannot find a card for the dealer in dealerPickCard (Err id: %d)", Enums::ERR_CARD_BET);
}

bool CardGames::dealerMoveSelector() {
    bool b = false;
    int n = this->stateVars[CardGames::CARD_SEL_IDX];
    int n2 = n % this->curCardsH;
    int n3 = n / this->curCardsH;
    int n4 = this->stateVars[CardGames::DEALER_SEL_IDX] % this->curCardsH;
    int n5 = this->stateVars[CardGames::DEALER_SEL_IDX] / this->curCardsH;
    if (n2 < n4) {
        ++n;
    }
    else if (n2 > n4) {
        --n;
    }
    else if (n3 < n5) {
        n += this->curCardsH;
    }
    else if (n3 > n5) {
        n -= this->curCardsH;
    }
    else {
        b = true;
    }
    this->stateVars[CardGames::CARD_SEL_IDX] = n;
    return b;
}

void CardGames::handleInput(int n, int n2) {
    this->handleWarInput(n, n2);
}

void CardGames::dealFullDeck() {
    Applet* app = CAppContainer::getInstance()->app;
    this->deckMask = 0LL;
    this->numCardsInPlay = 0;
    for (int i = 0; i < this->NUM_CARDS_W; ++i) {
        for (int j = 0; j < this->NUM_CARDS_H; ++j) {
            int n;
            for (n = app->nextInt() % CardGames::NUM_POKER_CARDS; ((long)(1 << n) & this->deckMask) != 0x0LL; n = (n + 1) % CardGames::NUM_POKER_CARDS) {}
            this->deckMask |= 1 << n; 
            this->dealCard(n, this->xMin + this->xMaxW, app->canvas->screenRect[3], this->CARDS_X + (CardGames::CARD_W + 3) * i, this->CARDS_Y + (CardGames::CARD_H + 3) * j, CardGames::CARD_EXLERP, CardGames::DEF_SPEED);
        }
    }
    this->stateVars[CardGames::CARD_SEL_IDX] = 1 + this->NUM_CARDS_W / 2 * this->NUM_CARDS_H;
    this->curCardsW = this->NUM_CARDS_W;
    this->curCardsH = this->NUM_CARDS_H;
}

void CardGames::dealWarHand() {
    Applet* app = CAppContainer::getInstance()->app;
    this->deckMask = 0LL;
    this->numCardsInPlay = 0;
    int n = this->CARDS_Y + (CardGames::CARD_H + 3);
    for (int i = 0; i < CardGames::NUM_WAR_CARDS; ++i) {
        int n2;
        for (n2 = app->nextInt() % CardGames::NUM_POKER_CARDS; ((long)(1 << n2) & this->deckMask) != 0x0LL; n2 = (n2 + 1) % CardGames::NUM_POKER_CARDS) {}
        this->deckMask |= 1 << n2;
        this->dealCard(n2, this->xMin + this->xMaxW, app->canvas->screenRect[3], this->WAR_CARDS_X + (CardGames::CARD_W + 3) * i, n, CardGames::CARD_EXLERP, CardGames::DEF_SPEED);
    }
    this->stateVars[CardGames::CARD_SEL_IDX] = 3;
    this->curCardsW = 6;
    this->curCardsH = 1;
}

void CardGames::reDealCard(int field, int flags, int n3) {
    Applet* app = CAppContainer::getInstance()->app;
    int* cardsInPlay = this->cardsInPlay;
    this->deckMask &= ~(1 << cardsInPlay[field + CardGames::CARD_DESC]);
    int n4;
    for (n4 = app->nextInt() % CardGames::NUM_POKER_CARDS; ((long)(1 << n4) & this->deckMask) != 0x0LL; n4 = (n4 + 1) % CardGames::NUM_POKER_CARDS) {}
    cardsInPlay[field + CardGames::CARD_DESC] = n4;
    cardsInPlay[field + CardGames::CARD_FLAGS] = flags;
    if (flags & CardGames::CARD_LERPING) {
        cardsInPlay[field + CardGames::CARD_START] = app->upTimeMs;
        ++this->numCardsLerping;
    }
    else if (flags & CardGames::CARD_EXLERP) {
        cardsInPlay[field + CardGames::CARD_START] = -1;
        ++this->numCardsLerping;
    }
}

void CardGames::dealCard(int n, int srcX, int srcY, int destX, int destY, int flags, int speed) {
    Applet* app = CAppContainer::getInstance()->app;
    int cardField = this->numCardsInPlay * CardGames::CARD_FIELDS;
    this->cardsInPlay[cardField + CardGames::CARD_DESC] = n;
    this->cardsInPlay[cardField + CardGames::CARD_SRCX] = srcX;
    this->cardsInPlay[cardField + CardGames::CARD_SRCY] = srcY;
    this->cardsInPlay[cardField + CardGames::CARD_DSTX] = destX;
    this->cardsInPlay[cardField + CardGames::CARD_DSTY] = destY;
    this->cardsInPlay[cardField + CardGames::CARD_FLAGS] = flags;
    int posX = std::abs(destX - srcX);
    int posY = std::abs(destY - srcY);
    if (posX > posY) {
        this->cardsInPlay[cardField + CardGames::CARD_LERP_DUR] = ((posX << 15) / speed) >> 8;
    }
    else {
        this->cardsInPlay[cardField + CardGames::CARD_LERP_DUR] = ((posY << 15) / speed) >> 8;
    }
    if(flags & CardGames::CARD_LERPING) {
        this->cardsInPlay[cardField + CardGames::CARD_START] = app->upTimeMs;
        ++this->numCardsLerping;
    }
    else if (flags & CardGames::CARD_EXLERP) {
        this->cardsInPlay[cardField + CardGames::CARD_START] = -1;
        ++this->numCardsLerping;
    }
    ++this->numCardsInPlay;
}

void CardGames::drawCardHud(Graphics* graphics) {
    Applet* app = CAppContainer::getInstance()->app;
    Text* smallBuffer = app->localization->getSmallBuffer();
    app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::WAR_NAME, smallBuffer);
    graphics->drawString(smallBuffer, app->canvas->SCR_CX, 5, 1);
    smallBuffer->dispose();
    graphics->drawRegion(this->imgOtherImgs, 0, 0, CardGames::PORTRAIT_WH, CardGames::PORTRAIT_WH, this->BJPortX, this->PORTRAIT_Y, 0, 0, 0);
    graphics->drawRegion(this->imgOtherImgs, CardGames::PORTRAIT_WH, 0, CardGames::PORTRAIT_WH, CardGames::PORTRAIT_WH, this->GuntherPortX, this->PORTRAIT_Y, 0, 0, 0);
    int n = this->stateVars[CardGames::GAME_STATE];
    if (this->numCardsLerping == 0) {
        if (n == CardGames::WAR_PICKWARCARD) {
            if (this->numPlayerCards == 0) {
                graphics->drawRegion(this->imgOtherImgs, (CardGames::PORTRAIT_WH * 2), 0, CardGames::PORTRAIT_WH, CardGames::PORTRAIT_WH, this->BJPortX, this->PORTRAIT_Y, 0, 0, 0);
            }
            else {
                graphics->drawRegion(this->imgOtherImgs, (CardGames::PORTRAIT_WH * 2), 0, CardGames::PORTRAIT_WH, CardGames::PORTRAIT_WH, this->GuntherPortX, this->PORTRAIT_Y, 0, 0, 0);
            }
        }
        else if ((1 << n & CardGames::PLAYER_TURN) != 0x0) {
            graphics->drawRegion(this->imgOtherImgs, (CardGames::PORTRAIT_WH * 2), 0, CardGames::PORTRAIT_WH, CardGames::PORTRAIT_WH, this->BJPortX, this->PORTRAIT_Y, 0, 0, 0);
        }
        else if ((1 << n & CardGames::DEALER_TURN) != 0x0) {
            graphics->drawRegion(this->imgOtherImgs, (CardGames::PORTRAIT_WH * 2), 0, CardGames::PORTRAIT_WH, CardGames::PORTRAIT_WH, this->GuntherPortX, this->PORTRAIT_Y, 0, 0, 0);
        }
    }
    this->drawChips(graphics);
    if (this->stateVars[CardGames::GAME_STATE] == 0) {
        int n2 = 111;
        int n3 = this->stateVars[CardGames::CUR_BET];
        int n4 = this->chipsY - (this->stateVars[CardGames::CUR_BET] * 5) + 5;
        int font_W = Applet::FONT_WIDTH[app->fontType];
        int font_H = Applet::FONT_HEIGHT[app->fontType];
        graphics->drawRegion(app->canvas->imgFont, (n2 & 0xF) * font_W, ((n2 & 0xF0) >> 4) * font_H, font_W, font_H, this->BJChipsX + CardGames::CHIP_W + 1, n4 + 4, 0, 0, 0);
        if (n3 == this->playerChips) {
            graphics->drawRegion(this->imgOtherImgs, 196, 0, CardGames::CHIP_W, CardGames::CHIP_H, this->BJChipsX, n4, 0, 0, 0);
        }
        else {
            graphics->drawRegion(this->imgOtherImgs, 162, 32, CardGames::CHIP_W, CardGames::CHIP_PART_H, this->BJChipsX, n4 + 8, 0, 0, 0);
        }
        graphics->drawRegion(app->canvas->imgFont, (n2 & 0xF) * font_W, ((n2 & 0xF0) >> 4) * font_H, font_W, font_H, this->GuntherChipsX - font_W - 1, n4 + 4, 0, 4, 0);
        if (n3 == this->dealerChips) {
            graphics->drawRegion(this->imgOtherImgs, 196, 0, CardGames::CHIP_W, CardGames::CHIP_H, this->GuntherChipsX, n4, 0, 0, 0);
        }
        else {
            graphics->drawRegion(this->imgOtherImgs, 162, 32, CardGames::CHIP_W, CardGames::CHIP_PART_H, this->GuntherChipsX, n4 + 8, 0, 0, 0);
        }
    }
}

void CardGames::drawChips(Graphics* graphics) {
    Applet* app = CAppContainer::getInstance()->app;
    int texX = 162;
    int time = app->upTimeMs;
    this->numChipsLerping = 0;
    for (int i = 0; i < this->numChips; ++i) {
        int filed = i * CardGames::CHIP_FIELDS;
        if (this->chips[filed + CardGames::CHIP_DUR] > 0) {
            int n3 = time - this->chips[filed + CardGames::CHIP_START];
            if (n3 >= this->chips[filed + CardGames::CHIP_DUR]) {
                this->chips[filed + CardGames::CHIP_DUR] = 0;
                graphics->drawRegion(this->imgOtherImgs, texX, 0, CardGames::CHIP_W, CardGames::CHIP_H, this->chips[filed + CardGames::CHIP_DSTX], this->chips[filed + CardGames::CHIP_DSTY], 0, 0, 0);
            }
            else {
                int n4 = (n3 << 16) / (this->chips[filed + CardGames::CHIP_DUR] << 8);
                graphics->drawRegion(this->imgOtherImgs, texX, 0, CardGames::CHIP_W, CardGames::CHIP_H,
                    this->chips[filed + CardGames::CHIP_SRCX] + (n4 * (this->chips[filed + CardGames::CHIP_DSTX] - this->chips[filed + CardGames::CHIP_SRCX] << 8) >> 16),
                    this->chips[filed + CardGames::CHIP_SRCY] + (n4 * (this->chips[filed + CardGames::CHIP_DSTY] - this->chips[filed + CardGames::CHIP_SRCY] << 8) >> 16), 0, 0, 0);
                ++this->numChipsLerping;
            }
        }
        else {
            graphics->drawRegion(this->imgOtherImgs, texX, 0, CardGames::CHIP_W, CardGames::CHIP_H, this->chips[filed + CardGames::CHIP_DSTX], this->chips[filed + CardGames::CHIP_DSTY], 0, 0, 0);
        }
    }
}

void CardGames::drawCards(Graphics* graphics) {
    int n = 0;
    for (int i = 0; i < this->numCardsInPlay; ++i) {
        int field = i * CardGames::CARD_FIELDS;
        int flags = this->cardsInPlay[field + CardGames::CARD_FLAGS];
        if (flags & CardGames::CARD_HIDDEN) {
            graphics->fillRect(this->cardsInPlay[field + CardGames::CARD_DSTX], this->cardsInPlay[field + CardGames::CARD_DSTY], CardGames::CARD_W, CardGames::CARD_H, this->backgroundColor & 0xFF3F3F3F);
        }
        else {
            int n4;
            int n5;
            if ((flags & (CardGames::CARD_LERPING | CardGames::CARD_EXLERP)) != 0x0) {
                if (n == 3) {
                    continue;
                }
                this->updateLerpingCard(i, &n4, &n5);
                if (flags & CardGames::CARD_EXLERP) {
                    ++n;
                }
            }
            else {
                n4 = this->cardsInPlay[field + CardGames::CARD_DSTX];
                n5 = this->cardsInPlay[field + CardGames::CARD_DSTY];
            }
            this->drawCard(graphics, n4, n5, this->cardsInPlay[field + CardGames::CARD_DESC], flags);
        }
    }
}

void CardGames::drawCard(Graphics* graphics, int x, int y, int n3, int flags) {
    Applet* app = CAppContainer::getInstance()->app;
    int fontType = app->fontType;
    
    app->setFont(3);

    int font_W = Applet::FONT_WIDTH[app->fontType];
    int font_H = Applet::FONT_HEIGHT[app->fontType];

    int n5 = n3 % 4;
    int n6 = n3 / 4;
    if (flags & CardGames::CARD_FACE_UP) {
        graphics->drawRegion(this->imgOtherImgs, 0, 54, CardGames::CARD_W, CardGames::CARD_H, x, y, 0, 0, 0);
        int n7 = 0;
        int n8 = 0;

        if (n6 > 0 && n6 < 9) {
            n7 = n6 & 0xF;
            n8 = (n6 + 16) & 0xF0;
        }
        else {
            switch (n6) {
                case 0: {
                    n7 = 0;
                    n8 = 32;
                    break;
                }
                case 9: {
                    n7 = 15;
                    n8 = 0;
                    break;
                }
                case 10: {
                    n7 = 9;
                    n8 = 32;
                    break;
                }
                case 11: {
                    n7 = 0;
                    n8 = 48;
                    break;
                }
                case 12: {
                    n7 = 10;
                    n8 = 32;
                    break;
                }
                default: {
                    app->Error("Invalid Card Number (Err Id:%d || %d)", Enums::ERR_INV_CARD, n6);
                    break;
                }
            }
        }
        
        graphics->drawRegion(app->canvas->imgFont, (n7 & 0xF) * font_W, ((n8 & 0xF0) >> 4) * font_H, font_W, font_H, x + 6, y + 3, 0, 0, 0);

        int n9 = 112 + n5;
        graphics->drawRegion(app->canvas->imgFont, (n5 & 0xF) * font_W, ((n9 & 0xF0) >> 4) * font_H, font_W, font_W, x + 6, y + 23, 0, 0, 0);
    }
    else { //if (!(flags & CardGames::CARD_DARKEN)) { // <- iOS
        int rotateMode = 0;
        if ((flags & CardGames::CARD_ROT90) != 0x0) {
            rotateMode |= 1;
        }
        else if ((flags & CardGames::CARD_ROT180) != 0x0) {
            rotateMode |= 2;
        }
        else if ((flags & CardGames::CARD_ROT270) != 0x0) {
            rotateMode |= 3;
        }
        graphics->drawRegion(this->imgOtherImgs, 34, 54, CardGames::CARD_W, CardGames::CARD_H, x, y, 0, rotateMode, 0);
    }
    if (flags & CardGames::CARD_PLAYER) {
        graphics->drawRegion(this->imgOtherImgs, 102, 54, CardGames::CARD_W, CardGames::CARD_H, x, y, 0, 0, 0);
    }
    else if (flags & CardGames::CARD_DEALER) {
        graphics->drawRegion(this->imgOtherImgs, 136, 54, CardGames::CARD_W, CardGames::CARD_H, x, y, 0, 0, 0);
    }


    if (flags & CardGames::CARD_DARKEN) { // <- J2ME
        graphics->drawRegion(this->imgOtherImgs, 170, 54, CardGames::CARD_W, CardGames::CARD_H, x, y, 0, 0, 14);
    }

    app->setFont(fontType);
}

void CardGames::clearParticles() {
    for (int i = 0; i < (CardGames::MAX_CARDS * CardGames::PART_FIELDS); ++i) {
        this->particles[i] = 0;
    }
}

void CardGames::drawParticles(Graphics* graphics) {
    Applet* app = CAppContainer::getInstance()->app;
    if (this->numParticles == 0) {
        return;
    }
    int particleFrameTime = app->upTimeMs << 8;
    int n = (particleFrameTime - this->particleFrameTime) / 1000;
    this->particleFrameTime = particleFrameTime;
    int n2 = particleFrameTime >> 8;
    this->numParticles = 0;
    for (int i = 0; i < CardGames::MAX_CARDS; ++i) {
        int filed = i * CardGames::PART_FIELDS;
        if (this->particles[filed + CardGames::PART_ALIVE] > 0 && this->updateParticle(filed, n)) {
            int n4 = n2 / 100 % 4;
            if (this->particles[filed + CardGames::PART_VX] > 0) {
                n4 = (n4 + 2 & 0x3);
            }
            this->drawCard(graphics, this->particles[filed + CardGames::PART_X], this->particles[filed + CardGames::PART_Y], 0, 1 << n4);
            ++this->numParticles;
        }
    }
}

bool CardGames::updateParticle(int filed, int n2) {
    Applet* app = CAppContainer::getInstance()->app;
    short partVX = this->particles[filed + CardGames::PART_VX];
    short partVY = this->particles[filed + CardGames::PART_VY];
    short partX = this->particles[filed + CardGames::PART_X];
    short partY = this->particles[filed + CardGames::PART_Y];
    int newVY = partVY + CardGames::GRAVITY * n2;
    int newX = (partVX >> 8) + partX;
    int newY = (newVY >> 8) + partY;
    if (newX + CardGames::CARD_W <= 0 || newX >= this->xMin + this->xMaxW) {
        this->particles[filed + CardGames::PART_ALIVE] = 0;
        return false;
    }
    if (newY + CardGames::CARD_H <= 0 || newY >= app->canvas->screenRect[1] + app->canvas->screenRect[3]) {
        this->particles[filed + CardGames::PART_ALIVE] = 0;
        return false;
    }
    this->particles[filed + CardGames::PART_VX] = partVX;
    this->particles[filed + CardGames::PART_VY] = (short)newVY;
    this->particles[filed + CardGames::PART_X] = (short)newX;
    this->particles[filed + CardGames::PART_Y] = (short)newY;
    return true;
}

void CardGames::updateLerpingCard(int n, int *posX, int* posY) {
    Applet* app = CAppContainer::getInstance()->app;
    int* cardsInPlay = this->cardsInPlay;
    int field = n * CardGames::CARD_FIELDS;
    int n3 = cardsInPlay[field + CardGames::CARD_SRCX];
    int n4 = cardsInPlay[field + CardGames::CARD_SRCY];
    int n5 = cardsInPlay[field + CardGames::CARD_DSTX];
    int n6 = cardsInPlay[field + CardGames::CARD_DSTY];
    if (cardsInPlay[field + CardGames::CARD_START] == -1) {
        cardsInPlay[field + CardGames::CARD_START] = app->upTimeMs;
        *posX = n3;
        *posY = n4;
        return;
    }
    int n7 = app->upTimeMs - cardsInPlay[field + CardGames::CARD_START];
    if (n7 >= cardsInPlay[field + CardGames::CARD_LERP_DUR]) {
        *posX = n5;
        *posY = n6;
        cardsInPlay[field + CardGames::CARD_FLAGS] &= ~(CardGames::CARD_LERPING | CardGames::CARD_EXLERP);
        --this->numCardsLerping;
    }
    else {
        int n9 = (n7 << 16) / (cardsInPlay[field + CardGames::CARD_LERP_DUR] << 8);
        *posX = n3 + (n9 * (n5 - n3 << 8) >> 16);
        *posY = n4 + (n9 * (n6 - n4 << 8) >> 16);
    }
}

void CardGames::allocChip(int srcX, int srcY, int dstX, int dstY, int start, int dur, int owner) {
    int chipField = this->numChips * CardGames::CHIP_FIELDS;
    this->chips[chipField + CardGames::CHIP_OWNER] = owner;
    this->chips[chipField + CardGames::CHIP_SRCX] = srcX;
    this->chips[chipField + CardGames::CHIP_SRCY] = srcY;
    this->chips[chipField + CardGames::CHIP_DSTX] = dstX;
    this->chips[chipField + CardGames::CHIP_DSTY] = dstY;
    this->chips[chipField + CardGames::CHIP_START] = start;
    this->chips[chipField + CardGames::CHIP_DUR] = dur;
    ++this->numChips;
}

void CardGames::setChips(int n, int n2) {
    this->numChips = 0;
    int chipsY = this->chipsY;
    for (int i = 0; i < n; ++i) {
        this->allocChip(this->BJChipsX, chipsY, this->BJChipsX, chipsY, 0, 0, CardGames::PLAYER_CHIP);
        chipsY -= 5;
    }
    int chipsY2 = this->chipsY;
    for (int j = 0; j < n2; ++j) {
        this->allocChip(this->GuntherChipsX, chipsY2, this->GuntherChipsX, chipsY2, 0, 0, CardGames::DEALER_CHIP);
        chipsY2 -= 5;
    }
}

void CardGames::drawPlayingBG(Graphics* graphics) {
    Applet* app = CAppContainer::getInstance()->app;
    graphics->drawImage(this->imgTable, 0, 0, 0, 0, 0);

    if (this->stateVars[CardGames::GAME_STATE] == 0) {

        this->m_cardGamesButtons->GetButton(3)->Render(graphics);
        this->m_cardGamesButtons->GetButton(9)->Render(graphics);

        if (app->canvas->softKeyLeftID != -1) {
            this->m_cardGamesButtons->GetButton(1)->Render(graphics);

            Text* text = app->localization->getSmallBuffer();
            text->setLength(0);
            app->localization->composeText(app->canvas->softKeyLeftID, text);
            text->dehyphenate();
            graphics->drawString(text, 3, 320, 0x24);
            text->dispose();
        }
    }
}

void CardGames::updateWar(Graphics* graphics) {
    Applet* app = CAppContainer::getInstance()->app;
    graphics->setClipRect(0, 0, Applet::IOS_WIDTH, Applet::IOS_HEIGHT);
    this->drawPlayingBG(graphics);
    this->drawCardHud(graphics);
    this->drawParticles(graphics);

    switch (this->stateVars[CardGames::GAME_STATE]) {
        case CardGames::WAR_BET: {
            this->WAR_BET_FUNC(graphics);
            break;
        }
        case CardGames::WAR_TOSS_CHIPS: {
            this->WAR_TOSS_CHIPS_FUNC();
            break;
        }
        case CardGames::WAR_DEAL: {
            this->WAR_DEAL_FUNC();
            break;
        }
        case CardGames::WAR_PLAYERCHOOSE: {
            this->WAR_PLAYERCHOOSE_FUNC(graphics);
            break;
        }
        case CardGames::WAR_DEALERCHOOSE: {
            this->WAR_DEALERCHOOSE_FUNC(graphics, this->stateVars[CardGames::SUB_STATE]);
            break;
        }
        case CardGames::WAR_PLAYERDISCARD: {
            this->WAR_PLAYERDISCARD_FUNC(graphics);
            break;
        }
        case CardGames::WAR_DEALERDISCARD: {
            this->WAR_DEALERDISCARD_FUNC(graphics, this->stateVars[CardGames::SUB_STATE]);
            break;
        }
        case CardGames::WAR_PICKWARCARD: {
            this->WAR_PICKWARCARD_FUNC(graphics, this->stateVars[CardGames::SUB_STATE]);
            break;
        }
        case CardGames::WAR_RESOLVE:
        case CardGames::WAR_RESOLVEFINAL: {
            this->WAR_RESOLVE_FUNC(graphics, this->stateVars[CardGames::SUB_STATE]);
            break;
        }
        case CardGames::WAR_REDEAL: {
            this->WAR_REDEAL_FUNC(graphics, this->stateVars[CardGames::SUB_STATE]);
            break;
        }
        case CardGames::WAR_MESSAGE: {
            this->WAR_MESSAGE_FUNC(graphics);
            break;
        }
    }
    app->canvas->staleView = true;
    graphics->resetScreenSpace();
}

void CardGames::WAR_BET_FUNC(Graphics* graphics) {
    Applet* app = CAppContainer::getInstance()->app;
    Text* largeBuffer = app->localization->getLargeBuffer();
    if (this->playerTreasure == 0) {
        this->stateVars[CardGames::MSG_ID] = CodeStrings::PLAYER_GOLD_ERR;
        this->stateVars[CardGames::SUB_STATE] = 0;
        this->setState(CardGames::WAR_MESSAGE);
        return;
    }
    largeBuffer->setLength(0);
    app->localization->resetTextArgs();
    app->localization->addTextArg(this->betPercentage);
    app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::MOVE_INSTRUCTION, largeBuffer);
    largeBuffer->wrapText(app->canvas->scrollWithBarMaxChars);
    graphics->drawString(largeBuffer, app->canvas->SCR_CX, (this->START_BTM_BG + CardGames::BORDER_H) / 2, 3);
    largeBuffer->dispose();
}

void CardGames::WAR_TOSS_CHIPS_FUNC() {
    Applet* app = CAppContainer::getInstance()->app;
    if (this->stateVars[CardGames::SUB_STATE] == 0) {
        int n = this->BJChipsX + CardGames::CHIP_W + Applet::FONT_WIDTH[app->fontType] + 6;
        int n2 = this->GuntherChipsX - CardGames::CHIP_W - Applet::FONT_WIDTH[app->fontType] - 6 - n;
        int n3 = this->chipsY - 30;
        short n4 = 30;
        int plyChips = this->playerChips - 1;
        int curBet = this->stateVars[CardGames::CUR_BET];
        int startTime = app->upTimeMs;

        for (int i = 0; i < curBet; ++i) {
            int field = plyChips * CardGames::CHIP_FIELDS;
            short nextByte = app->nextByte();
            this->chips[field + CardGames::CHIP_DSTX] = n + nextByte % n2;
            this->chips[field + CardGames::CHIP_DSTY] = n3 + nextByte % n4;
            this->chips[field + CardGames::CHIP_START] = startTime;
            this->chips[field + CardGames::CHIP_DUR] = 200;
            --plyChips;
            ++this->numChipsLerping;
        }

        int dlrChips = this->playerChips + this->dealerChips - 1;
        for (int j = 0; j < curBet; ++j) {
            int field = dlrChips * CardGames::CHIP_FIELDS;
            short nextByte2 = app->nextByte();
            this->chips[field + CardGames::CHIP_DSTX] = n + nextByte2 % n2;
            this->chips[field + CardGames::CHIP_DSTY] = n3 + nextByte2 % n4;
            this->chips[field + CardGames::CHIP_START] = startTime;
            this->chips[field + CardGames::CHIP_DUR] = 200;
            --dlrChips;
            ++this->numChipsLerping;
        }
        ++this->stateVars[CardGames::SUB_STATE];
    }
    if (this->numChipsLerping == 0) {
        this->nextState();
    }
}

void CardGames::WAR_DEAL_FUNC() {
    if (this->numCardsInPlay == 0 || this->numCardsInPlay < CardGames::MAX_CARDS) {
        this->dealFullDeck();
    }
    else {
        for (int i = 0; i < this->numPlayerCards; ++i) {
            this->reDealCard(this->playersCards[i], CardGames::CARD_EXLERP, CardGames::DEF_SPEED);
        }
        for (int j = 0; j < this->numDealerCards; ++j) {
            this->reDealCard(this->dealersCards[j], CardGames::CARD_EXLERP, CardGames::DEF_SPEED);
        }
        this->stateVars[CardGames::CARD_SEL_IDX] = 1 + this->NUM_CARDS_W / 2 * this->NUM_CARDS_H;
    }
    this->resetHand();
    this->setState(CardGames::WAR_PLAYERCHOOSE);
}

void CardGames::WAR_PLAYERCHOOSE_FUNC(Graphics* graphics) {
    Applet* app = CAppContainer::getInstance()->app;
    Text* smallBuffer = app->localization->getSmallBuffer();
    this->drawCards(graphics);
    if (this->numCardsLerping == 0) {
        smallBuffer->setLength(0);
        app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::GAME_PICK, smallBuffer);
        smallBuffer->wrapText(app->canvas->scrollWithBarMaxChars, 2, '\n');
        graphics->drawString(smallBuffer, app->canvas->SCR_CX, this->CARD_TEXT_Y, 1);
        this->drawSelector(graphics);
    }
    smallBuffer->dispose();
}

void CardGames::WAR_PICKWARCARD_FUNC(Graphics* graphics, int n) {
    Applet* app = CAppContainer::getInstance()->app;
    Text* smallBuffer = app->localization->getSmallBuffer();
    this->drawCards(graphics);
    if (this->numPlayerCards == 0 && this->numCardsLerping == 0) {
        smallBuffer->setLength(0);
        app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::WAR_PICK, smallBuffer);
        smallBuffer->wrapText(app->canvas->scrollWithBarMaxChars, 3, '\n');
        graphics->drawString(smallBuffer, app->canvas->SCR_CX, this->CARD_WAR_TEXT_Y, 1);
        this->drawSelector(graphics);
    }
    else if (this->numPlayerCards == 1) {
        this->stateVars[CardGames::SUB_STATE] = this->dealerAI(n);
        smallBuffer->setLength(0);
        app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::DEALER_CHOOSING, smallBuffer);
        smallBuffer->wrapText(app->canvas->scrollWithBarMaxChars, 2, '\n');
        graphics->drawString(smallBuffer, app->canvas->SCR_CX, this->CARD_WAR_TEXT_Y, 1);
        this->drawSelector(graphics);
    }
    smallBuffer->dispose();
}

void CardGames::WAR_DEALERCHOOSE_FUNC(Graphics* graphics, int n) {
    Applet* app = CAppContainer::getInstance()->app;
    this->drawCards(graphics);
    Text* smallBuffer = app->localization->getSmallBuffer();
    this->stateVars[CardGames::SUB_STATE] = this->dealerAI(n);
    smallBuffer->setLength(0);
    app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::DEALER_CHOOSING, smallBuffer);
    smallBuffer->wrapText(app->canvas->scrollWithBarMaxChars, 2, '\n');
    graphics->drawString(smallBuffer, app->canvas->SCR_CX, this->CARD_TEXT_Y, 1);
    this->drawSelector(graphics);
    smallBuffer->dispose();
}

void CardGames::WAR_DEALERDISCARD_FUNC(Graphics* graphics, int n) {
    Applet* app = CAppContainer::getInstance()->app;
    this->drawCards(graphics);
    Text* smallBuffer = app->localization->getSmallBuffer();
    this->stateVars[CardGames::SUB_STATE] = this->dealerAI(n);
    smallBuffer->setLength(0);
    app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::DEALER_CHOOSING, smallBuffer);
    smallBuffer->wrapText(app->canvas->scrollWithBarMaxChars, 2, '\n');
    graphics->drawString(smallBuffer, app->canvas->SCR_CX, this->CARD_WAR_TEXT_Y, 1);
    this->drawSelector(graphics);
    smallBuffer->dispose();
}

void CardGames::WAR_REDEAL_FUNC(Graphics* graphics, int n) {
    Applet* app = CAppContainer::getInstance()->app;
    this->drawCards(graphics);
    if (n == 0) {
        Text* smallBuffer = app->localization->getSmallBuffer();
        app->localization->composeText(Strings::FILE_CODESTRINGS, (short)this->stateVars[CardGames::MSG_ID], smallBuffer);
        smallBuffer->dehyphenate();
        graphics->drawString(smallBuffer, app->canvas->SCR_CX, this->CARD_TEXT_Y, 1);
        smallBuffer->dispose();
    }
    else if (n == 1) {
        this->explodeCards();
        ++this->stateVars[CardGames::SUB_STATE];
        ++n;
    }
    if (n == 2) {
        if (!this->animationDone) {
            int n3 = app->upTimeMs - this->stateVars[CardGames::STATE_TIME];
            int n4 = n3 / 200 & 0x3;
            if (n3 > 800) {
                this->animationDone = true;
            }
            else {
                graphics->drawRegion(this->flakImg, 128 * n4, 0, 128, 128, this->cardsInPlay[this->playersCards[0] + CardGames::CARD_DSTX] - 37, this->cardsInPlay[this->playersCards[0] + CardGames::CARD_DSTY] - 34, 0, 0, 0);
                graphics->drawRegion(this->flakImg, 128 * n4, 0, 128, 128, this->cardsInPlay[this->dealersCards[0] + CardGames::CARD_DSTX] - 37, this->cardsInPlay[this->dealersCards[0] + CardGames::CARD_DSTY] - 34, 0, 0, 0);
            }
        }
        if (this->animationDone && this->numCardsLerping == 0 && this->numParticles == 0) {
            ++this->stateVars[CardGames::SUB_STATE];
        }
    }
    else if (n == 3) {
        this->cardsDiscarded = 0;
        this->resetHand();
        this->dealWarHand();
        ++this->stateVars[CardGames::SUB_STATE];
    }
    else if (n == 4 && this->numCardsLerping == 0) {
        this->setState(CardGames::WAR_PLAYERDISCARD);
    }
    if (n == 0) {
        app->canvas->clearSoftKeys();
    }
}

void CardGames::WAR_PLAYERDISCARD_FUNC(Graphics* graphics) {
    Applet* app = CAppContainer::getInstance()->app;
    this->drawCards(graphics);
    if (this->numCardsLerping == 0) {
        Text* smallBuffer = app->localization->getSmallBuffer();
        smallBuffer->setLength(0);
        if (this->cardsDiscarded == 0) {
            app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::WAR_TOSS, smallBuffer);
        }
        else {
            app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::WAR_TOSS2, smallBuffer);
        }
        smallBuffer->wrapText(app->canvas->scrollWithBarMaxChars, 3, '\n');
        graphics->drawString(smallBuffer, app->canvas->SCR_CX, this->CARD_WAR_TEXT_Y, 1);
        this->drawSelector(graphics);
        smallBuffer->dispose();
    }
}

void CardGames::WAR_RESOLVE_FUNC(Graphics* graphics, int n) {
    Applet* app = CAppContainer::getInstance()->app;
    this->drawCards(graphics);
    if (n == 0) {
        this->stateVars[CardGames::SUB_STATE] = 1;
        int n2 = this->cardsInPlay[this->playersCards[this->numPlayerCards - 1] + CardGames::CARD_DESC] / 4;
        int n3 = this->cardsInPlay[this->dealersCards[this->numDealerCards - 1] + CardGames::CARD_DESC] / 4;
        int n4 = 1;
        if (n2 == n3) {
            this->stateVars[CardGames::MSG_ID] = CodeStrings::GO_TO_WAR;
            n4 = 3;
        }
        else if (n2 == 0 || (n3 != 0 && n2 > n3)) {
            this->stateVars[CardGames::MSG_ID] = CodeStrings::YOU_WIN;
            n4 = 2;
        }
        else {
            this->stateVars[CardGames::MSG_ID] = CodeStrings::YOU_LOST;
        }
        this->animationDone = false;
        if (n4 != 1 && n4 != 2) {
            this->setState(CardGames::WAR_REDEAL);
            return;
        }
        int n5 = this->stateVars[CardGames::CUR_BET];
        if (n4 == 1) {
            n5 = -n5;
        }
        if (this->savePlayer) {
            app->player->gamePlayedMask |= 0x1;
        }
        this->playerTreasure += n5 * this->chipAmount;
        this->playerChips += n5;
        this->aiTreasure += -n5 * this->chipAmount;
        this->dealerChips += -n5;
        if (this->aiTreasure < 0) {
            this->aiTreasure = 0;
        }
        int min = std::min(this->playerChips, this->dealerChips);
        if (this->stateVars[CardGames::CUR_BET] > min) {
            this->stateVars[CardGames::CUR_BET] = min;
            this->updateBetPerc();
        }
        this->setChips(this->playerChips, this->dealerChips);

        app->sound->playSound(1136, 0, 4, false);
        this->stateVars[CardGames::STATE_TIME] = app->upTimeMs;
        this->stateVars[CardGames::SUB_STATE] = n4;
        this->backgroundColor = 0xFF006C2A;
    }
    if (n != 3) {
        Text* largeBuffer = app->localization->getLargeBuffer();
        largeBuffer->setLength(0);
        app->localization->composeText(Strings::FILE_CODESTRINGS, (short)this->stateVars[CardGames::MSG_ID], largeBuffer);
        largeBuffer->append('|');
        app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::PRESS_OK, largeBuffer);
        largeBuffer->dehyphenate();
        int n6 = this->CARD_TEXT_Y;
        if (this->numCardsInPlay == CardGames::NUM_WAR_CARDS) {
            n6 = this->CARD_WAR_TEXT_Y;
        }
        graphics->drawString(largeBuffer, app->canvas->SCR_CX, n6, 1);
        largeBuffer->dispose();
        this->drawSelector(graphics);
        if (!this->animationDone) {
            int n7 = app->upTimeMs - this->stateVars[CardGames::STATE_TIME];
            int n8 = n7 / 200 & 0x3;
            if (n7 > 800) {
                this->animationDone = true;
            }
            else {
                int n9 = 128;
                int n10 = 128;
                int n11;
                if (this->stateVars[CardGames::SUB_STATE] == 1) {
                    n11 = this->BJPortX + 27 - n9 / 2;
                }
                else {
                    n11 = this->GuntherPortX + 27 - n9 / 2;
                }
                graphics->drawRegion(this->flakImg, 128 * n8, 0, n9, n10, n11, this->PORTRAIT_Y + 27 - n10 / 2, 0, 0, 0);
            }
        }
    }
}

void CardGames::WAR_MESSAGE_FUNC(Graphics* graphics) {
    Applet* app = CAppContainer::getInstance()->app;
    Text* smallBuffer = app->localization->getSmallBuffer();
    smallBuffer->setLength(0);
    app->localization->composeText(Strings::FILE_CODESTRINGS, (short)this->stateVars[CardGames::MSG_ID], smallBuffer);
    app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::DOUBLE_SPACED, smallBuffer);
    app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::PRESS_OK, smallBuffer);
    smallBuffer->wrapText(app->canvas->scrollWithBarMaxChars, 10, '\n');
    graphics->drawString(smallBuffer, app->canvas->SCR_CX, (this->START_BTM_BG + CardGames::BORDER_H) / 2, 3);
    smallBuffer->dispose();
}

void CardGames::drawSelector(Graphics* graphics) {
    int curState = this->stateVars[CardGames::GAME_STATE];
    if (curState != CardGames::WAR_REDEAL) {
        int field = this->stateVars[CardGames::CARD_SEL_IDX] * CardGames::CARD_FIELDS;

        if ((/*(this->touched || this->wasTouched) &&*/ // <- iOS
            (curState == CardGames::WAR_PLAYERCHOOSE || curState == CardGames::WAR_PLAYERDISCARD || curState == CardGames::WAR_PICKWARCARD)) ||
            (curState == CardGames::WAR_DEALERCHOOSE || curState == CardGames::WAR_DEALERDISCARD)) {
            graphics->drawRegion(this->imgOtherImgs, 68, 54, CardGames::CARD_W, CardGames::CARD_H, this->cardsInPlay[field + CardGames::CARD_DSTX], this->cardsInPlay[field + CardGames::CARD_DSTY],0 ,0 ,0);
        }
    }
}

int CardGames::dealerAI(int n) {
    Applet* app = CAppContainer::getInstance()->app;
    int n2 = n;
    if (n == 0) {
        this->dealerPickCard();
        n2 = n + 1;
        this->stateVars[CardGames::STATE_TIME] = app->upTimeMs + 250;
    }
    else if (n == 1) {
        if (this->stateVars[CardGames::STATE_TIME] <= app->upTimeMs) {
            this->stateVars[CardGames::STATE_TIME] = app->upTimeMs + 250;
            if (this->stateVars[CardGames::MSG_TIME] <= app->upTimeMs && this->dealerMoveSelector()) {
                n2 = n + 1;
            }
        }
    }
    else if (n == 2) {
        this->handleWarInput(6, -5);
        n2 = 0;
    }
    return n2;
}

void CardGames::handleWarInput(int n, int n2) {
    Applet* app = CAppContainer::getInstance()->app;
    int curState = this->stateVars[CardGames::GAME_STATE];
    int curTime = app->upTimeMs;

    if (this->stateVars[CardGames::MSG_TIME] > curTime) {
        return;
    }
    bool b = !this->savePlayer || (app->player->gamePlayedMask & 0x1) != 0x0;
    if (curState == CardGames::WAR_MESSAGE && (n == 6 || n == 15)) {
        if (this->stateVars[CardGames::MSG_ID] == CodeStrings::CARDS_TOTAL_WIN) {
            this->endGame(1);
        }
        else if (this->stateVars[CardGames::MSG_ID] == CodeStrings::CARDS_TOTAL_LOSS) {
            this->endGame(0);
        }
        else if (this->stateVars[CardGames::MSG_ID] == CodeStrings::PLAYER_GOLD_ERR) {
            this->endGame(2);
        }
        else {
            this->setState(this->stateVars[CardGames::PREV_STATE]);
        }
    }
    else if ((curState == CardGames::WAR_RESOLVE || curState == CardGames::WAR_RESOLVEFINAL) && (n == 6 || n == 15)) {
        if (this->playerChips == 0) {
            this->stateVars[CardGames::MSG_ID] = CodeStrings::CARDS_TOTAL_LOSS;
            this->setState(CardGames::WAR_MESSAGE);
        }
        else if (this->dealerChips == 0) {
            this->stateVars[CardGames::MSG_ID] = CodeStrings::CARDS_TOTAL_WIN;
            this->setState(CardGames::WAR_MESSAGE);
        }
        else {
            this->setState(CardGames::WAR_BET);
        }
    }
    else if (curState == CardGames::WAR_REDEAL && (n == 7 || n == 6 || n == 15)) {
        if (this->stateVars[CardGames::SUB_STATE] == 0) {
            ++this->stateVars[CardGames::SUB_STATE];
            this->stateVars[CardGames::STATE_TIME] = curTime;
            this->backgroundColor = 0xFF660000;
        }
        else if (this->stateVars[CardGames::SUB_STATE] == 4) {
            for (int i = 0; i < CardGames::MAX_CARDS; ++i) {
                this->cardsInPlay[(i * CardGames::CARD_FIELDS) + CardGames::CARD_FLAGS] &= ~(CardGames::CARD_LERPING | CardGames::CARD_EXLERP);
            }
            this->numCardsLerping = 0;
        }
    }
    else if (this->numCardsLerping != 0 && curState == CardGames::WAR_PLAYERCHOOSE) {
        for (int j = 0; j < CardGames::MAX_CARDS; ++j) {
            this->cardsInPlay[(j * CardGames::CARD_FIELDS) + CardGames::CARD_FLAGS] &= ~(CardGames::CARD_LERPING | CardGames::CARD_EXLERP);
        }
        this->numCardsLerping = 0;
    }
    else if ((n == 5 || n == 15) && b && curState == CardGames::WAR_BET) {
        this->endGame(2);
    }
    else if (n == 6 || n == 15) {
        short field = (short)(this->stateVars[CardGames::CARD_SEL_IDX] * CardGames::CARD_FIELDS);
        if (curState == CardGames::WAR_BET) {
            this->setState(CardGames::WAR_TOSS_CHIPS);
            app->canvas->clearSoftKeys();
        }
        else if ((curState == CardGames::WAR_PLAYERCHOOSE || curState == CardGames::WAR_DEALERCHOOSE) && this->numCardsLerping == 0) {
            if (this->cardsInPlay[field + CardGames::CARD_FLAGS] & (CardGames::CARD_PLAYER | CardGames::CARD_DEALER)) {
                return;
            }
            if (curState == CardGames::WAR_PLAYERCHOOSE) {
                this->playersCards[this->numPlayerCards++] = field;
                this->cardsInPlay[field + CardGames::CARD_FLAGS] |= (CardGames::CARD_FACE_UP | CardGames::CARD_PLAYER);
                this->nextState();
                this->stateVars[CardGames::MSG_TIME] = curTime + CardGames::DEALER_MOVE_TIMEOUT;
            }
            else {
                this->dealersCards[this->numDealerCards++] = field;
                this->cardsInPlay[field + CardGames::CARD_FLAGS] |= (CardGames::CARD_FACE_UP | CardGames::CARD_DEALER);
                this->nextState();
            }
        }
        else if ((curState == CardGames::WAR_PLAYERDISCARD || curState == CardGames::WAR_DEALERDISCARD) && this->numCardsLerping == 0) {
            if (this->cardsInPlay[field + CardGames::CARD_FLAGS] & CardGames::CARD_DARKEN) {
                return;
            }
            if (curState == CardGames::WAR_PLAYERDISCARD) {
                ++this->cardsDiscarded;
                this->cardsInPlay[field + CardGames::CARD_FLAGS] |= CardGames::CARD_DARKEN;
                this->nextState();
                this->stateVars[CardGames::MSG_TIME] = curTime + CardGames::DEALER_MOVE_TIMEOUT;
            }
            else {
                ++this->cardsDiscarded;
                this->cardsInPlay[field + CardGames::CARD_FLAGS] |= CardGames::CARD_DARKEN;
                if (this->cardsDiscarded < 4) {
                    this->prevState();
                }
                else {
                    this->nextState();
                }
            }
        }
        else if (curState == CardGames::WAR_PICKWARCARD) {
            if (this->cardsInPlay[field + CardGames::CARD_FLAGS] & CardGames::CARD_DARKEN) {
                return;
            }
            if (this->numPlayerCards == 0) {
                this->playersCards[this->numPlayerCards++] = field;
                this->cardsInPlay[field + CardGames::CARD_FLAGS] |= (CardGames::CARD_FACE_UP | CardGames::CARD_PLAYER);
                this->stateVars[CardGames::MSG_TIME] = curTime + CardGames::DEALER_MOVE_TIMEOUT;
            }
            else {
                this->dealersCards[this->numDealerCards++] = field;
                this->cardsInPlay[field + CardGames::CARD_FLAGS] |= (CardGames::CARD_FACE_UP | CardGames::CARD_DEALER);
                this->nextState();
            }
        }
    }
    else if (n == 1) {
        if (curState == CardGames::WAR_BET) {
            int n14 = this->stateVars[CardGames::CUR_BET] + 1;
            if (n14 <= this->playerChips && n14 <= this->dealerChips) {
                ++this->stateVars[CardGames::CUR_BET];
                this->updateBetPerc();
            }
        }
        else if ((curState == CardGames::WAR_PLAYERCHOOSE || curState == CardGames::WAR_PLAYERDISCARD || curState == CardGames::WAR_PICKWARCARD) && this->stateVars[6] - 1 >= 0) {
            --this->stateVars[CardGames::CARD_SEL_IDX];
        }
    }
    else if (n == 2) {
        if (curState == CardGames::WAR_BET) {
            if (this->stateVars[CardGames::CUR_BET] - 1 >= 1) {
                --this->stateVars[CardGames::CUR_BET];
                this->updateBetPerc();
            }
        }
        else if ((curState == CardGames::WAR_PLAYERCHOOSE || curState == CardGames::WAR_PLAYERDISCARD || curState == CardGames::WAR_PICKWARCARD) && this->stateVars[CardGames::CARD_SEL_IDX] + 1 < this->numCardsInPlay) {
            ++this->stateVars[CardGames::CARD_SEL_IDX];
        }
    }
    else if (n == 4 && (curState == CardGames::WAR_PLAYERCHOOSE || curState == CardGames::WAR_PLAYERDISCARD || curState == CardGames::WAR_PICKWARCARD)) {
        int n19 = this->stateVars[CardGames::CARD_SEL_IDX] / this->curCardsH;
        if (++n19 < this->curCardsW) {
            this->stateVars[CardGames::CARD_SEL_IDX] += this->curCardsH;
        }
    }
    else if (n == 3 && (curState == CardGames::WAR_PLAYERCHOOSE || curState == CardGames::WAR_PLAYERDISCARD || curState == CardGames::WAR_PICKWARCARD)) {
        int n21 = this->stateVars[CardGames::CARD_SEL_IDX] / this->curCardsH;
        if (--n21 >= 0) {
            this->stateVars[CardGames::CARD_SEL_IDX] -= this->curCardsH;
        }
    }
    else if (n == 7 && curState == CardGames::WAR_BET) {
        this->setState(CardGames::WAR_TOSS_CHIPS);
        app->canvas->clearSoftKeys();
    }
}

void CardGames::updateBetPerc() {
    this->betPercentage = (this->stateVars[CardGames::CUR_BET] << 16) / this->startingChips * 100;
    if ((this->betPercentage & 0x8000) != 0x0) {
        this->betPercentage += 65536;
    }
    this->betPercentage >>= 16;
}

void CardGames::explodeCards() {
    Applet* app = CAppContainer::getInstance()->app;
    int* cardsInPlay = this->cardsInPlay;
    this->clearParticles();
    int particleFrameTime = app->upTimeMs + 5 << 8;
    int playX = this->cardsInPlay[this->playersCards[0] + CardGames::CARD_DSTX];
    int playY = this->cardsInPlay[this->playersCards[0] + CardGames::CARD_DSTY];
    int dealX = this->cardsInPlay[this->dealersCards[0] + CardGames::CARD_DSTX];
    int dealY = this->cardsInPlay[this->dealersCards[0] + CardGames::CARD_DSTY];
    int velNormal[2];
    for (int i = 0; i < this->numCardsInPlay; ++i) {
        int cardField = i * CardGames::CARD_FIELDS;
        if (!(cardsInPlay[cardField + CardGames::CARD_FLAGS] & (CardGames::CARD_FACE_UP | CardGames::CARD_HIDDEN))) {
            int partField = this->numParticles * CardGames::PART_FIELDS;
            ++this->numParticles;
            this->particles[partField + CardGames::PART_X] = (short)cardsInPlay[cardField + CardGames::CARD_DSTX];
            this->particles[partField + CardGames::PART_Y] = (short)cardsInPlay[cardField + CardGames::CARD_DSTY];
            int cardX = cardsInPlay[cardField + CardGames::CARD_DSTX];
            int cardY = cardsInPlay[cardField + CardGames::CARD_DSTY];
            if ((playX - cardX) * (playX - cardX) + (playY - cardY) * (playY - cardY) <= (dealX - cardX) * (dealX - cardX) + (dealY - cardY) * (dealY - cardY)) {
                app->game->NormalizeVec(cardX - playX, cardY - playY, velNormal);
            }
            else {
                app->game->NormalizeVec(cardX - dealX, cardY - dealY, velNormal);
            }
            this->particles[partField + CardGames::PART_VX] = (short)(velNormal[0] * CardGames::DEF_VELOCITY >> 8);
            this->particles[partField + CardGames::PART_VY] = (short)(velNormal[1] * CardGames::DEF_VELOCITY >> 8);
            this->particles[partField + CardGames::PART_ALIVE] = 1;
        }
    }
    this->particleFrameTime = particleFrameTime;
    this->numCardsInPlay = 0;
    app->sound->playSound(1136, 0, 4, false);
}

void CardGames::shutdown() {
    this->imgOtherImgs->~Image();
    this->imgOtherImgs = nullptr;
    this->imgTable->~Image();
    this->imgTable = nullptr;
    this->flakImg->~Image();
    this->flakImg = nullptr;
    this->imgUpPointerNormal->~Image();
    this->imgUpPointerNormal = nullptr;
    this->imgUpPointerPressed->~Image();
    this->imgUpPointerPressed = nullptr;
    this->imgDownPointerNormal->~Image();
    this->imgDownPointerNormal = nullptr;
    this->imgDownPointerPressed->~Image();
    this->imgDownPointerPressed = nullptr;
    this->imgSwitchUp->~Image();
    this->imgSwitchUp = nullptr;
    this->imgSwitchDown->~Image();
    this->imgSwitchDown = nullptr;
    this->imgSwitchDown->~Image();
    this->imgSwitchDown = nullptr;

    if (this->m_cardGamesButtons) {
        this->m_cardGamesButtons->~fmButtonContainer();
        operator delete(this->m_cardGamesButtons);
    }
    this->m_cardGamesButtons = nullptr;
}

void CardGames::touchStart(int pressX, int pressY) {

    int iVar1;
    int iVar2;
    uint32_t uVar3;
    int iVar4;
    uint32_t uVar5;
    int iVar6;
    uint32_t uVar7;
    int iVar8;

    this->touched = true;
    int curState = this->stateVars[CardGames::GAME_STATE];

    if (curState == CardGames::WAR_PLAYERCHOOSE || curState == CardGames::WAR_PLAYERDISCARD || curState == CardGames::WAR_PICKWARCARD) {
        if (curState != CardGames::WAR_PLAYERCHOOSE) {
            uVar7 = pressX - this->WAR_CARDS_X;
            iVar8 = 219;
            uVar5 = 53;
            iVar2 = 50;
            if ((int)uVar7 < 220) {
                uVar5 = 0;
            }
            if (219 < (int)uVar7) {
                uVar5 = 1;
            }
            uVar5 = uVar5 | uVar7 >> 31;
            uVar3 = (pressY - this->CARDS_Y) - 53;
            iVar6 = 6;
            iVar4 = 1;
        }
        else {
            uVar7 = pressX - this->CARDS_X;
            iVar6 = this->NUM_CARDS_W;
            uVar3 = pressY - this->CARDS_Y;
            iVar4 = 3;
            iVar8 = iVar6 * 37 + -3;
            uVar5 = 159;
            iVar2 = 156;
            if ((int)uVar7 <= iVar8) {
                uVar5 = 0;
            }
            if (iVar8 < (int)uVar7) {
                uVar5 = 1;
            }
            uVar5 = uVar5 | uVar7 >> 31;
        }
        if (uVar5 == 0) {
            if ((int)uVar3 <= iVar2) {
                uVar5 = 0;
            }
            if (iVar2 < (int)uVar3) {
                uVar5 = 1;
            }
            if ((uVar5 | uVar3 >> 31) == 0) {
                this->wasTouched = true;
                iVar1 = (iVar4 * uVar3) / iVar2;
                iVar2 = (iVar6 * uVar7) / iVar8;
                if (iVar4 <= iVar1) {
                    iVar1 = iVar4 + -1;
                }
                if (iVar6 <= iVar2) {
                    iVar2 = iVar6 + -1;
                }
                iVar1 = iVar4 * iVar2 + iVar1;
                if (iVar1 == -1) {
                    return;
                }
                this->stateVars[CardGames::CARD_SEL_IDX] = iVar1;
                return;
            }
        }
        this->wasTouched = false;
    }
    else {
        this->m_cardGamesButtons->HighlightButton(pressX, pressY, true);
    }
}

void CardGames::touchMove(int pressX, int pressY) {

    int iVar1;
    int iVar2;
    uint32_t uVar3;
    int iVar4;
    uint32_t uVar5;
    int iVar6;
    uint32_t uVar7;
    int iVar8;

    int curState = this->stateVars[CardGames::GAME_STATE];
    if (curState == CardGames::WAR_PLAYERCHOOSE || curState == CardGames::WAR_PLAYERDISCARD || curState == CardGames::WAR_PICKWARCARD) {
        if (curState != CardGames::WAR_PLAYERCHOOSE) {
            uVar7 = pressX - this->WAR_CARDS_X;
            iVar8 = 219;
            uVar5 = 53;
            iVar2 = 50;
            if ((int)uVar7 < 220) {
                uVar5 = 0;
            }
            if (219 < (int)uVar7) {
                uVar5 = 1;
            }
            uVar5 = uVar5 | uVar7 >> 31;
            uVar3 = (pressY - this->CARDS_Y) - 53;
            iVar6 = 6;
            iVar4 = 1;
        }
        else {
            uVar7 = pressX - this->CARDS_X;
            iVar6 = this->NUM_CARDS_W;
            uVar3 = pressY - this->CARDS_Y;
            iVar4 = 3;
            iVar8 = iVar6 * 37 + -3;
            uVar5 = 159;
            iVar2 = 156;
            if ((int)uVar7 <= iVar8) {
                uVar5 = 0;
            }
            if (iVar8 < (int)uVar7) {
                uVar5 = 1;
            }
            uVar5 = uVar5 | uVar7 >> 31;
        }
        if (uVar5 == 0) {
            if ((int)uVar3 <= iVar2) {
                uVar5 = 0;
            }
            if (iVar2 < (int)uVar3) {
                uVar5 = 1;
            }
            if ((uVar5 | uVar3 >> 31) == 0) {
                this->wasTouched = true;
                iVar1 = (iVar4 * uVar3) / iVar2;
                iVar2 = (iVar6 * uVar7) / iVar8;
                if (iVar4 <= iVar1) {
                    iVar1 = iVar4 + -1;
                }
                if (iVar6 <= iVar2) {
                    iVar2 = iVar6 + -1;
                }
                iVar1 = iVar4 * iVar2 + iVar1;
                if (iVar1 == -1) {
                    return;
                }
                this->stateVars[CardGames::CARD_SEL_IDX] = iVar1;
                return;
            }
        }
        this->wasTouched = false;
    }
    else {
        this->m_cardGamesButtons->HighlightButton(pressX, pressY, true);
    }
}

int CardGames::touchToKey(int pressX, int pressY) {
    int uVar1;
    int iVar1;
    int iVar2;
    uint32_t uVar3;
    int iVar4;
    uint32_t uVar5;
    int iVar6;
    uint32_t uVar7;
    int iVar9;
    bool bVar10;

    this->touched = false;
    int curState = this->stateVars[CardGames::GAME_STATE];
    if (curState == CardGames::WAR_BET) {
        return this->m_cardGamesButtons->GetTouchedButtonID(pressX, pressY);
    }

    if (curState == CardGames::WAR_PLAYERCHOOSE || curState == CardGames::WAR_PLAYERDISCARD || curState == CardGames::WAR_PICKWARCARD) {
        if (curState != CardGames::WAR_PLAYERCHOOSE) {
            iVar2 = this->WAR_CARDS_X;
            uVar3 = (pressY - this->CARDS_Y) - 53;
            iVar6 = 6;
            iVar9 = 1;
            iVar4 = 222;
            uVar5 = 53;
        }
        else {
            iVar2 = this->CARDS_X;
            iVar6 = this->NUM_CARDS_W;
            iVar4 = iVar6 * 37;
            uVar3 = pressY - this->CARDS_Y;
            iVar9 = 3;
            uVar5 = 0x9f;
        }
        uVar7 = pressX - iVar2;
        iVar4 = iVar4 + -3;
        iVar2 = uVar5 - 3;
        if ((int)uVar7 <= iVar4) {
            uVar5 = 0;
        }
        if (iVar4 < (int)uVar7) {
            uVar5 = 1;
        }
        uVar5 = uVar5 | uVar7 >> 31;
        if (uVar5 == 0) {
            if ((int)uVar3 <= iVar2) {
                uVar5 = 0;
            }
            if (iVar2 < (int)uVar3) {
                uVar5 = 1;
            }
            if ((uVar5 | uVar3 >> 31) == 0) {
                this->wasTouched = true;
                iVar1 = (iVar9 * uVar3) / iVar2;
                iVar2 = (iVar6 * uVar7) / iVar4;
                if (iVar9 <= iVar1) {
                    iVar1 = iVar9 + -1;
                }
                if (iVar6 <= iVar2) {
                    iVar2 = iVar6 + -1;
                }
                iVar1 = iVar9 * iVar2 + iVar1;
                if (iVar1 == -1) {
                    return -1;
                }
                this->stateVars[CardGames::CARD_SEL_IDX] = iVar1;
                goto LAB_000948f0;
            }
        }
        this->wasTouched = false;
        uVar1 = -1;
    }
    else {
    LAB_000948f0:
        uVar1 = 6;
    }
    return uVar1;
}
