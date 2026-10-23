#include <stdexcept>
#include <cstring>

#include "CAppContainer.h"
#include "App.h"
#include "Hud.h"
#include "Text.h"
#include "Canvas.h"
#include "Combat.h"
#include "Player.h"
#include "Render.h"
#include "Button.h"
#include "TinyGL.h"
#include "Image.h"
#include "Graphics.h"
#include "Button.h"
#include "Entity.h"
#include "EntityDef.h"
#include "EntityMonster.h"
#include "CombatEntity.h"
#include "MenuSystem.h"
#include "Enums.h"
#include "Sound.h"
#include "Menus.h"

Hud::Hud() {
	memset(this, 0, sizeof(Hud));
}

Hud::~Hud() {
}

bool Hud::startup() {
	Applet* app = CAppContainer::getInstance()->app;
	//printf("Hud::startup\n");

	for (int i = 0; i < Hud::MAX_MESSAGES; i++) {
		this->messages[i] = new Text(Hud::MS_PER_CHAR);
	}

	app->beginImageLoading();
	this->imgAmmoIcons = app->loadImage("hud_ammo_icons.bmp", true);
	this->imgAttArrow = app->loadImage("Hud_Attack_Arrows.bmp", true);
	this->imgPortraitsSM = app->loadImage("Hud_Portrait_Small.bmp", true);
	this->imgPanelTop = app->loadImage("HUD_Panel_top.bmp", true);
	this->imgShieldNormal = app->loadImage("HUD_Shield_Normal.bmp", true);
	this->imgShieldButtonActive = app->loadImage("Hud_Shield_Button_Active.bmp", true);
	this->imgKeyNormal = app->loadImage("Hud_Key_Normal.bmp", true);
	this->imgKeyActive = app->loadImage("Hud_Key_Active.bmp", true);
	this->imgHealthNormal = app->loadImage("HUD_Health_Normal.bmp", true);
	this->imgHealthButtonActive = app->loadImage("Hud_Health_Button_Active.bmp", true);
	this->imgSwitchRightNormal = app->loadImage("Switch_Right_Normal.bmp", true);
	this->imgSwitchRightActive = app->loadImage("Switch_Right_Active.bmp", true);
	this->imgSwitchLeftNormal = app->loadImage("Switch_Left_Normal.bmp", true);
	this->imgSwitchLeftActive = app->loadImage("Switch_Left_Active.bmp", true);
	this->imgNumbers = app->loadImage("Hud_Numbers.bmp", true);
	this->imgWeaponNormal = app->loadImage("Hud_Weapon_Normal.bmp", true);
	this->imgWeaponActive = app->loadImage("HUD_Weapon_Active.bmp", true);
	this->imgPlayerFaces = app->loadImage("Hud_Player.bmp", true);
	this->imgPlayerActive = app->loadImage("HUD_Player_Active.bmp", true);
	app->finishImageLoading();

	this->m_hudButtons = new fmButtonContainer;

	fmButton *btnSwitchRight = new fmButton(0, 0, 256, 52, 64, 1121);
	this->m_hudButtons->AddButton(btnSwitchRight);

	fmButton* btnSwitchLeft = new fmButton(1, 428, 256, 52, 64, 1121);
	this->m_hudButtons->AddButton(btnSwitchLeft);

	fmButton* btnWeapon = new fmButton(2, 272, 260, this->imgWeaponNormal->width, 41, 1121);
	this->m_hudButtons->AddButton(btnWeapon);

	fmButton* btnPlayer = new fmButton(3, 214, 258, this->imgPlayerFaces->width + 10, 46, 1121);
	this->m_hudButtons->AddButton(btnPlayer);

	fmButton* btnShield = new fmButton(4, 55, 260, this->imgShieldNormal->width, this->imgShieldNormal->height, 1121);
	this->m_hudButtons->AddButton(btnShield);

	fmButton* btnHealth = new fmButton(5, 137, 260, this->imgHealthNormal->width, this->imgHealthNormal->height, 1121);
	this->m_hudButtons->AddButton(btnHealth);

	fmButton* btnKey = new fmButton(6, 380, 260, this->imgKeyNormal->width, 41, 1121);
	this->m_hudButtons->AddButton(btnKey);
	
	this->m_weaponsButtons = new fmButtonContainer;
	for (int i = 0; i < Hud::MAX_WEAPON_BUTTONS; i++) {
		fmButton* btnWpn = new fmButton(i, 480, 320, this->imgWeaponNormal->width, 41, 1027);
		this->m_weaponsButtons->AddButton(btnWpn);
	}

	this->msgCount = 0;
	this->weaponPressTime = 0;
	this->isInWeaponSelect = false;

	return true;
}

void Hud::shiftMsgs() {
	Applet* app = CAppContainer::getInstance()->app;

	if (this->messageFlags[0] & 0x2) {
		app->canvas->invalidateRect();
	}

	for (int i = 0; i < this->msgCount - 1; ++i) {
		this->messages[i]->setLength(0);
		this->messages[i]->append(this->messages[i + 1]);
		this->messageFlags[i] = this->messageFlags[i + 1];
	}

	this->msgCount--;
	this->messages[this->msgCount]->setLength(0);
	this->messageFlags[this->msgCount] = 0;
	if (this->msgCount > 0) {
		this->calcMsgTime();
	}
}

void Hud::calcMsgTime() {
	Applet* app = CAppContainer::getInstance()->app;

	this->msgTime = app->time;
	int length = this->messages[0]->length();
	if (length <= app->canvas->menuHelpMaxChars) {
		this->msgDuration = 700;
	}
	else {
		this->msgDuration = length * 50;
		if ((this->messageFlags[0] & 0x2) != 0x0 && this->msgDuration > 1500) {
			this->msgDuration = 1500;
		}
	}
}

void Hud::addMessage(short i) {
	this->addMessage(Strings::FILE_CODESTRINGS, i, 0);
}

void Hud::addMessage(short i, short i2) {
	this->addMessage((short)i, i2, 0);
}

void Hud::addMessage(short i, int i2) {
	this->addMessage(Strings::FILE_CODESTRINGS, i, i2);
}

void Hud::addMessage(short i, short i2, int i3) {
	Applet* app = CAppContainer::getInstance()->app;

	Text* text = app->localization->getSmallBuffer();
	app->localization->composeText(i, i2, text);
	this->addMessage(text, i3);
	text->dispose();
}

void Hud::addMessage(Text* text) {
	this->addMessage(text, 0);
}

void Hud::addMessage(Text* text, int flags) {
	Applet* app = CAppContainer::getInstance()->app;

	if (text == nullptr) {
		return;
	}

	if (flags & 0x1) {
		this->msgCount = 0;
	}

	if (this->msgCount > 0 && text->compareTo(this->messages[this->msgCount - 1])) {
		return;
	}

	if (this->msgCount == 5) {
		shiftMsgs();
	}

	this->messages[this->msgCount]->setLength(0);
	this->messages[this->msgCount]->append(text);

	if (flags & 0x2) {
		this->messages[this->msgCount]->wrapText((app->canvas->viewRect[2] - 9) / 9);
	}
	else {
		this->messages[this->msgCount]->dehyphenate();
	}

	this->messageFlags[this->msgCount] = flags;
	this->msgCount++;

	if (this->msgCount == 1) {
		this->calcMsgTime();
		if (flags & 0x1) {
			this->msgDuration *= 2;
		}
	}
}

Text* Hud::getMessageBuffer() {
	return this->getMessageBuffer(0);
}

Text* Hud::getMessageBuffer(int flags) {
	if ((flags & 0x1) != 0x0) {
		this->msgCount = 0;
	}
	if (this->msgCount == 5) {
		this->shiftMsgs();
	}
	this->messages[this->msgCount]->setLength(0);
	this->messageFlags[this->msgCount] = flags;
	return this->messages[this->msgCount];
}

void Hud::finishMessageBuffer() {
	this->messages[this->msgCount]->dehyphenate();
	this->msgCount++;
	if (this->msgCount == 1) {
		this->calcMsgTime();
	}
}

bool Hud::isShiftingCenterMsg() {
	Applet* app = CAppContainer::getInstance()->app;

	return ((this->msgCount > 0) && ((app->time - this->msgTime) > this->msgDuration) && (this->messageFlags[0] & 0x2)) ? true : false;
}

void Hud::drawTopBar(Graphics* graphics) {
	Applet* app = CAppContainer::getInstance()->app;

	Text* smallBuffer = app->localization->getSmallBuffer();
	int n = 20;
	Entity* facingEntity = app->player->facingEntity;
	graphics->drawImage(this->imgPanelTop, 240, 0, 17, 0, 0);
	graphics->drawBevel(0xFF9A9987, 0xFF1D1E0C, -app->canvas->screenRect[0], 0, app->canvas->hudRect[2] + 1, 20);

	if (app->canvas->state != Canvas::ST_DYING) {
		this->drawMonsterHealth(graphics);
	}
	if (this->msgCount > 0 && app->time - this->msgTime > this->msgDuration + 100) {
		this->shiftMsgs();
	}
	int n2 = -1;
	bool b = false;
	bool b2 = false;
	int n3 = 0;
	if (this->msgCount > 0) {
		smallBuffer->setLength(0);
		smallBuffer->append(this->messages[0]);
		b2 = ((this->messageFlags[0] & 0x2) != 0x0);
		if (app->time - this->msgTime > this->msgDuration) {
			b = true;
		}
		else {
			int n4 = smallBuffer->length() - app->canvas->menuHelpMaxChars;
			if (n4 > 0) {
				int n5 = app->time - this->msgTime;
				if (n5 > 750) {
					n3 = (n5 - 750) / 50;
					if (n3 > n4) {
						n3 = n4;
					}
				}
			}
		}
	}
	else if (app->canvas->state != Canvas::ST_MENU && facingEntity != nullptr && facingEntity->def->eType != Enums::ET_WORLD && (facingEntity->def->eType != Enums::ET_SPRITEWALL || (facingEntity->info & Entity::ENTITY_FLAG_ACTIVE) != 0x0)) {
		smallBuffer->setLength(0);
		int sprite = facingEntity->getSprite();
		int tileNum = app->render->mapSpriteInfo[sprite] & Enums::SPRITE_MASK_SPRITENUMBER;
		int frameNum = (app->render->mapSpriteInfo[sprite] & Enums::SPRITE_MASK_FRAMENUMBER) >> Enums::SPRITE_SHIFT_FRAMENUMBER;

		uint8_t eType = facingEntity->def->eType;
		if ((facingEntity->name & 0x3FF) != facingEntity->def->name || facingEntity->def->longName == EntityStrings::EMPTY_STRING) {
			app->localization->composeTextField(facingEntity->name, smallBuffer);
		}
		else {
			app->localization->composeTextField(Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, facingEntity->def->longName), smallBuffer);
		}
		smallBuffer->dehyphenate();
		if (eType == Enums::ET_NPC) {
			n2 = 2;
		}
		else if (eType == Enums::ET_MONSTER) {
			smallBuffer->setLength(0);
			if ((facingEntity->name & 0x3FF) != facingEntity->def->name) {
				app->localization->composeTextField(facingEntity->name, smallBuffer);
			}
			else {
				app->localization->composeTextField(Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, facingEntity->def->longName), smallBuffer);
			}

			if (eType == Enums::ET_MONSTER && (facingEntity->monster->flags & Enums::MFLAG_NPC_MONSTER)) {
				n2 = 2;
			}
			else {
				n2 = 1;
			}

			smallBuffer->dehyphenate();
		}
		else if (eType == Enums::ET_ITEM) {
			n2 = 0;
		}
		else if (eType == Enums::ET_DOOR) {
			n2 = 3;
			if (eType == Enums::ET_DOOR && (facingEntity->def->parm & 0x2) != 0x0) {
				b2 = true;
			}
		}
		else if (eType == Enums::ET_SPRITEWALL) {
			b2 = true;
		}
		else if (eType == Enums::ET_DECOR && facingEntity->def->eSubType == Enums::DECOR_EXITHALL) {
			b2 = true;
		}
		else if (eType == Enums::ET_DECOR) {
			if (facingEntity->def->eSubType == Enums::DECOR_STATUE) {
				n2 = 1;
			}
			else if (tileNum == Enums::TILENUM_BOOKSHELF && frameNum == 0) {
				n2 = 3;
			}
			else if (facingEntity->def->parm == 1) {
				n2 = 3;
			}
			else if (facingEntity->def->parm == 2) {
				int n8 = 0;
				if (tileNum == Enums::TILENUM_TOMBSTONE) {
					n8 = 1;
				}
				if (frameNum == n8) {
					n2 = 3;
				}
			}
		}
		else if (eType == Enums::ET_DECOR_NOCLIP) {
			if (tileNum == Enums::TILENUM_EXIT_DUMMY) {
				b2 = true;
			}
			else if (tileNum == Enums::TILENUM_USE_DUMMY) {
				n2 = 3;
			}
			else if (tileNum == Enums::TILENUM_ATTACK_DUMMY) {
				n2 = 1;
			}
			else if (facingEntity->def->eSubType == Enums::DECOR_BOOK_PICKUP) {
				n2 = 0;
			}
		}
		else if (eType == Enums::ET_ATTACK_INTERACTIVE) {
			if (facingEntity->def->eSubType == Enums::INTERACT_PICKUP) {
				n2 = 0;
			}
			else {
				n2 = 1;
			}
		}
	}
	else if (facingEntity != nullptr && facingEntity->def->eType == Enums::ET_SPRITEWALL) {
		int sprite = facingEntity->getSprite();
		int tileNum = app->render->mapSpriteInfo[sprite] & Enums::SPRITE_MASK_SPRITENUMBER;
		int frameNum = (app->render->mapSpriteInfo[sprite] & Enums::SPRITE_MASK_FRAMENUMBER) >> Enums::SPRITE_SHIFT_FRAMENUMBER;

		if (tileNum == Enums::TILENUM_BOOKSHELF && frameNum == 0) {
			n2 = 3;
		}
		else {
			b = true;
		}
	}
	else {
		b = true;
	}

	if (!b) {
		if (b2 && smallBuffer->length() > 0) {
			this->drawCenterMessage(graphics, smallBuffer, 0xFF000000);
		}
		else {
			if (n2 != -1) {
				graphics->drawRegion(this->imgActions, 0, n2 * 18, 18, 18, n - app->canvas->screenRect[0], app->canvas->screenRect[1] + 1, 20, 0, 0);
				n += 20;
			}
			int length = smallBuffer->length();
			if (n + Applet::FONT_WIDTH[app->fontType] + (length * Applet::FONT_WIDTH[app->fontType]) > app->canvas->hudRect[2]) {
				length = (app->canvas->hudRect[2] - n) / Applet::CHAR_SPACING[app->fontType] - 1;
			}
			graphics->drawString(smallBuffer, n - app->canvas->screenRect[0], 11 - Applet::FONT_HEIGHT[app->fontType] / 2, 0, n3, length);
		}
	}
	smallBuffer->dispose();
}

void Hud::drawCenterMessage(Graphics* graphics, Text* text, int color) {
	Applet* app = CAppContainer::getInstance()->app;

	int w = text->getStringWidth() + 8;
	if (w > app->canvas->hudRect[2]) {
		w = app->canvas->hudRect[2];
	}
	int y = -app->canvas->screenRect[1] + 40;
	int x = -app->canvas->screenRect[0] + (app->canvas->hudRect[2] / 2);

	int numLines = text->getNumLines();
	graphics->setColor(0xFF000000/*color*/);
	graphics->fillRect(x - (w / 2), y, w - 1, (Applet::FONT_HEIGHT[app->fontType] * numLines) + 3);
	graphics->setColor(0xFFAAAAAA);
	graphics->drawRect(x - (w / 2), y, w - 1, (Applet::FONT_HEIGHT[app->fontType] * numLines) + 3);

	y += 3;
	int i = 0;
	Text* textBuff = app->localization->getSmallBuffer();
	int first;
	while ((first = text->findFirstOf('|', i)) >= 0) {
		textBuff->setLength(0);
		text->substring(textBuff, i, first);
		graphics->drawString(textBuff, x, y, 17, 0, first - i);
		y += Applet::FONT_HEIGHT[app->fontType];
		i = first + 1;
	}

	textBuff->setLength(0);
	text->substring(textBuff, i);
	graphics->drawString(textBuff, x, y, 17, 0, text->length() - i);
	textBuff->dispose();
}


void Hud::drawCinematicText(Graphics* graphics) {
	Applet* app = CAppContainer::getInstance()->app;
	Canvas* canvas = CAppContainer::getInstance()->app->canvas;

	int scr_CX = canvas->SCR_CX;
	int flags = 1;
	int width = this->imgPlayerFaces->width;

	int n2 = app->hud->showCinPlayer ? (canvas->subtitleMaxChars - 5) : canvas->subtitleMaxChars;

	Text* largeBuffer = app->localization->getLargeBuffer();

#ifdef WOLFENRPG_PSP
	// Preserve the full scene on PSP.  The original iPhone UI used opaque
	// cinematic masks; compact translucent caption panels remain readable
	// without bars popping in and out as the script changes camera state.
	const int panelX = 8;
	const int panelW = canvas->displayRect[2] - 16;
	const int fontH = Applet::FONT_HEIGHT[app->fontType];

	if (app->hud->cinTitleID != -1 && app->hud->cinTitleTime > app->gameTime) {
		largeBuffer->setLength(0);
		app->localization->composeText(app->hud->cinTitleID, largeBuffer);
		largeBuffer->wrapText(n2, 1, '\n');
		const int panelH = largeBuffer->getNumLines() * fontH + 8;
		graphics->FMGL_fillRect(panelX, 4, panelW, panelH, 0.0f, 0.0f, 0.0f, 0.68f);
		graphics->setColor(0xFFB8B8B8);
		graphics->drawRect(panelX, 4, panelW - 1, panelH - 1);
		graphics->setColor(0xFFFFFFFF);
		graphics->drawString(largeBuffer, canvas->SCR_CX, 8, 1);
	}

	if (app->hud->subTitleID != -1 && app->hud->subTitleTime > app->gameTime) {
		largeBuffer->setLength(0);
		app->localization->composeText(app->hud->subTitleID, largeBuffer);
		largeBuffer->wrapText(n2, 2, '\n');
		const int panelH = largeBuffer->getNumLines() * fontH + 8;
		const int panelY = canvas->displayRect[3] - panelH - 6;
		graphics->FMGL_fillRect(panelX, panelY, panelW, panelH, 0.0f, 0.0f, 0.0f, 0.72f);
		graphics->setColor(0xFFB8B8B8);
		graphics->drawRect(panelX, panelY, panelW - 1, panelH - 1);
		graphics->setColor(0xFFFFFFFF);
		graphics->drawString(largeBuffer, canvas->SCR_CX, panelY + 4, 1);
	}

	largeBuffer->dispose();
	this->drawBubbleText(graphics);
	return;
#else
	graphics->eraseRgn(0, 0, canvas->displayRect[2], canvas->cinRect[1]);
	graphics->eraseRgn(0, canvas->cinRect[1] + canvas->cinRect[3], canvas->displayRect[2], canvas->softKeyY - (canvas->cinRect[1] + canvas->cinRect[3]));

	if (app->hud->cinTitleID != -1 && app->hud->cinTitleTime > app->gameTime) {
		largeBuffer->setLength(0);
		app->localization->composeText(app->hud->cinTitleID, largeBuffer);
		largeBuffer->wrapText(n2, 1, '\n');
		graphics->drawString(largeBuffer, scr_CX, 1, 1);
	}

	if (app->hud->subTitleID != -1 && app->hud->subTitleTime > app->gameTime) {
		int n3 = canvas->cinRect[1] + canvas->cinRect[3];
		int n4 = (n3 + ((canvas->screenRect[3] - n3 - 2 * Applet::FONT_HEIGHT[app->fontType]) >> 1));
		largeBuffer->setLength(0);
		app->localization->composeText(app->hud->subTitleID, largeBuffer);
		largeBuffer->wrapText(n2, 2, '\n');
		int first = largeBuffer->findFirstOf('\n', 0);
		if (first == -1) {
			if (app->hud->showCinPlayer) {
				graphics->drawRegion(app->hud->imgPortraitsSM, 0, 0, this->imgPortraitsSM->width, this->imgPortraitsSM->height >> 1, 5, n4 - 6 /*(width - 32) / 2*/, 0, 0, 0);
				flags = 4;
				scr_CX = width + 10;
			}
			graphics->drawString(largeBuffer, scr_CX, n4, flags);
		}
		else {
			if (app->hud->showCinPlayer) {
				graphics->drawRegion(app->hud->imgPortraitsSM, 0, 0, this->imgPortraitsSM->width, this->imgPortraitsSM->height >> 1, 5, n4 - 6 /*(width - 32) / 2*/, 0, 0, 0);
				flags = 4;
				scr_CX = width + 10;
			}

			graphics->drawString(largeBuffer, scr_CX, n4, flags);
			graphics->drawString(largeBuffer, scr_CX, n4 + Applet::FONT_HEIGHT[app->fontType], flags, first + 1, 9999);
		}
	}
	largeBuffer->dispose();
	this->drawBubbleText(graphics);
#endif
}

void Hud::drawEffects(Graphics* graphics) {
	Applet* app = CAppContainer::getInstance()->app;

	if (app->canvas->loadMapID >= 8 && app->tinyGL->fogRange < -1) {
		int n = (1024 + (app->tinyGL->fogRange >> 4) << 8) / 1024;
		int n2 = 0;
		int n3 = app->canvas->screenRect[1] -(n * app->hud->imgIce->height >> 8) + 20;
		//graphics->setScreenSpace(app->canvas->viewRect);
		graphics->drawImage(app->hud->imgIce, n2, n3, 0, 0, 0);
		graphics->drawImage(app->hud->imgIce, n2 + app->hud->imgIce->width, n3, 0, 4, 0);
		//graphics->resetScreenSpace();
	}
	if (app->canvas->state != Canvas::ST_DYING) {
		app->player->drawBuffs(graphics);
	}
	if (app->time < this->damageTime && this->damageCount > 0 && app->combat->totalDamage > 0) {
		if ((1 << this->damageDir & 0xC1) != 0x0) {
			int n4 = app->canvas->screenRect[3] - 20 - 25;
			int n5 = 0;
			int n6;
			if (this->damageDir == 0) {
				n6 = app->canvas->screenRect[2] - 20;
				n5 = 24;
			}
			else if (this->damageDir == 6) {
				n6 = 20;
				n5 = 12;
			}
			else {
				n6 = app->canvas->screenRect[2] >> 1;
			}
			graphics->drawRegion(this->imgAttArrow, 0, n5, 12, 12, n6, n4, 3, 0, 0);
		}
	}
	else if (this->damageTime != 0) {
		this->damageTime = 0;
		//this->stopBrightenScreen(); // DOOM 2 RPG
		//this->stopScreenSmack(); // DOOM 2 RPG
	}
	this->drawDamageVignette(graphics);
}

void Hud::drawDamageVignette(Graphics* graphics) {
	Applet* app = CAppContainer::getInstance()->app;

	if (app->time < this->damageTime && this->damageCount >= 0 && app->combat->totalDamage > 0) {
		int n = 0;
		switch (this->damageDir) {
			case 1: {
				n = 2;
				break;
			}
			case 2: {
				n = 3;
				break;
			}
			case 3: {
				n = 15;
				break;
			}
			case 4: {
				n = 5;
				break;
			}
			case 5: {
				n = 4;
				break;
			}
			case 0:
			case 6:
			case 7: {
				n = 8;
				break;
			}
		}
		Image* image = this->imgDamageVignette;
		int width = image->width;
		if (n & 0x1) {
			//graphics->fillRegion(image, app->canvas->viewRect[0], app->canvas->viewRect[1], app->canvas->viewRect[2], width, 1);
			graphics->FMGL_fillRect(
				width + app->canvas->viewRect[0],
				app->canvas->viewRect[1],
				app->canvas->viewRect[2] - 2 * width,
				width,
				0.8,
				0.0,
				0.0,
				0.125);
		}
		if (n & 0x2) {
			//graphics->fillRegion(image, app->canvas->viewRect[0] + (app->canvas->viewRect[2] - width), app->canvas->viewRect[1], width, app->canvas->viewRect[3], 4);
			graphics->FMGL_fillRect(
				app->canvas->viewRect[2] - width + app->canvas->viewRect[0],
				app->canvas->viewRect[1],
				width,
				app->canvas->viewRect[3],
				0.8,
				0.0,
				0.0,
				0.125);
		}
		if (n & 0x4) {
			//graphics->fillRegion(image, app->canvas->viewRect[0], app->canvas->viewRect[1], width, app->canvas->viewRect[3], 0);
			graphics->FMGL_fillRect(
				app->canvas->viewRect[0],
				app->canvas->viewRect[1],
				width,
				app->canvas->viewRect[3],
				0.8,
				0.0,
				0.0,
				0.125);
		}
		if (n & 0x8) {
			//graphics->fillRegion(image, app->canvas->viewRect[0], (256 - width), app->canvas->viewRect[2], width, 3);
			graphics->FMGL_fillRect(
				width + app->canvas->viewRect[0],
				app->canvas->viewRect[3] - width + app->canvas->viewRect[1],
				app->canvas->viewRect[2] - 2 * width,
				width,
				0.8,
				0.0,
				0.0,
				0.125);
		}
	}
}

void Hud::drawBottomBar(Graphics* graphics) {
	Applet* app = CAppContainer::getInstance()->app;

	fmButton* bnt0 = this->m_hudButtons->GetButton(0);
	fmButton* bnt1 = this->m_hudButtons->GetButton(1);
	app->canvas->drawTouchSoftkeyBar(graphics, bnt0->highlighted, bnt1->highlighted);

	this->drawWeapon(graphics, 272, 260, app->player->ce->weapon, this->m_hudButtons->GetButton(2)->highlighted);

	Image* imgShield = (this->m_hudButtons->GetButton(4)->highlighted) ? this->imgShieldButtonActive : this->imgShieldNormal;
	graphics->drawImage(imgShield, 55, 260, 0, 0, 0);

	this->drawNumbers(graphics, 89, 270, 1, app->player->ce->getStat(Enums::STAT_ARMOR));
	this->drawCurrentKeys(graphics, 380, 260);

	Image* imgHealth = (this->m_hudButtons->GetButton(5)->highlighted) ? this->imgHealthButtonActive : this->imgHealthNormal;
	graphics->drawImage(imgHealth, 137, 260, 0, 0, 0);
	this->drawNumbers(graphics, 171, 270, 1, app->player->ce->getStat(Enums::STAT_HEALTH));

	int v28 = app->player->ce->getStat(Enums::STAT_HEALTH);
	int v29 = 5 - 5 * v28 / app->player->ce->getStat(Enums::STAT_MAX_HEALTH);

	Image* imgPlayer = (this->m_hudButtons->GetButton(3)->highlighted) ? this->imgPlayerActive : this->imgPlayerFaces;
	graphics->drawRegion(imgPlayer, 0, 42 * v29, imgPlayer->width, 42, 219, 260, 20, 0, 0);

	if (app->canvas->softKeyLeftID != -1 && app->canvas->softKeyRightID != -1)
	{
		Text* texBuff = app->localization->getSmallBuffer();
		texBuff->setLength(0);
		app->localization->composeText(Strings::FILE_MENUSTRINGS, MenuStrings::WAIT_ITEM, texBuff);
		texBuff->dehyphenate();
		graphics->drawString(texBuff, 240, 320, 33);
		texBuff->dispose();
	}
}

void Hud::draw(Graphics* graphics) {
	//printf("Hud::draw");
	Applet* app = CAppContainer::getInstance()->app;

	this->drawTime = app->upTimeMs;
	if ((this->repaintFlags & 0x1) != 0x0) {
		//this->repaintFlags &= 0xFFFFFFFE;
		if (app->canvas->isZoomedIn) {
			graphics->drawImage(this->imgScope, 0, 20, 0, 0, 0);
		}
		this->drawEffects(graphics);
	}
	if ((this->repaintFlags & 0x2) != 0x0) {
		//this->repaintFlags &= 0xFFFFFFFD;
		app->canvas->checkFacingEntity();
		this->drawTopBar(graphics);
	}
	if ((this->repaintFlags & 0x8) != 0x0) {
		this->drawBubbleText(graphics);
	}
	if ((this->repaintFlags & 0x4) != 0x0) {
		//this->repaintFlags &= 0xFFFFFFFB;
		if (app->canvas->state != Canvas::ST_DIALOG) {
			if (app->canvas->isZoomedIn == false) {
				this->drawArrowControls(graphics);
			}
			else {
				app->canvas->m_sniperScopeButtons->Render(graphics);
				fmScrollButton* m_sniperScopeDialScrollButton = app->canvas->m_sniperScopeDialScrollButton;
				int x = m_sniperScopeDialScrollButton->barRect.x;
				int y = m_sniperScopeDialScrollButton->barRect.y;
				int texX, texY;

				if (m_sniperScopeDialScrollButton->field_0x0_) {
					texX = m_sniperScopeDialScrollButton->field_0x44_ & 3;
					texY = m_sniperScopeDialScrollButton->field_0x44_ >> 2;
				}
				else {
					texX = m_sniperScopeDialScrollButton->field_0x0_;
					texY = m_sniperScopeDialScrollButton->field_0x0_;
				}

				Image* imgSniperScope_Dial = app->canvas->imgSniperScope_Dial;
				int texW = (imgSniperScope_Dial->width >> 2);
				int texH = (imgSniperScope_Dial->height >> 2);
				graphics->drawRegion(imgSniperScope_Dial, texX * texW, texY * texH, texW, texH, x, y, 0, 0, 0);

				/*graphics->drawImage(app->canvas->imgSniperScope_Dial, x, y, 0, 0, 0);
				int knobY = 0;
				if (m_sniperScopeDialScrollButton->field_0x0_) {
					knobY = m_sniperScopeDialScrollButton->field_0x48_ + (m_sniperScopeDialScrollButton->field_0x4c_ >> 1);
				}
				graphics->drawImage(
					app->canvas->imgSniperScope_Knob,
					x - (app->menuSystem->imgMenuDialKnob->width >> 1) + (app->canvas->imgSniperScope_Dial->width >> 1),
					y + (knobY << 2) / 5 - (app->menuSystem->imgMenuDialKnob->height >> 1) + 16,
					0,
					0,
					0);*/
			}
		}
		if (this->isInWeaponSelect != false) {
			this->drawWeaponSelection(graphics);
		}
		else {
			// [GEC] Prevent it from selecting checkboxes and producing sound
			for (int i = 0; i < Hud::MAX_WEAPON_BUTTONS; i++) {
				this->m_weaponsButtons->GetButton(i)->drawButton = false;
			}
		}

		this->drawBottomBar(graphics);
	}

	if (this->cinTitleID != -1 && this->cinTitleTime < app->gameTime) {
		this->cinTitleID = -1;
		if (app->canvas->state == Canvas::ST_CAMERA) {
			this->repaintFlags |= 0x10;
		}
	}
	if (this->subTitleID != -1 && this->subTitleTime < app->gameTime) {
		this->subTitleID = -1;
		if (app->canvas->state == Canvas::ST_CAMERA) {
			this->repaintFlags |= 0x10;
		}
	}
	if ((this->repaintFlags & 0x10) != 0x0) {
		//this->repaintFlags &= 0xFFFFFFEF;
		this->drawCinematicText(graphics);

		if (app->canvas->softKeyRightID != -1) {
			Text* texBuff = app->localization->getSmallBuffer();
			texBuff->setLength(0);
			app->localization->composeText(app->canvas->softKeyRightID, texBuff);
			texBuff->dehyphenate();

			app->setFontRenderMode(2); // [GEC] New
			if (app->canvas->m_softKeyButtons->GetButton(20)->highlighted != false) { // [GEC] New
				app->setFontRenderMode(0); // [GEC] New
			}
			graphics->drawString(texBuff, 478, 320, 40);

			app->setFontRenderMode(0); // [GEC] New
			texBuff->dispose();
		}
	}
	this->drawTime = app->upTimeMs - this->drawTime;
}

void Hud::drawMonsterHealth(Graphics* graphics) {
	Applet* app = CAppContainer::getInstance()->app;

	Entity* facingEntity = app->player->facingEntity;
	if (facingEntity == nullptr || facingEntity->monster == nullptr) {
		if (this->lastTarget != nullptr) {
			app->canvas->invalidateRect();
		}
		this->lastTarget = nullptr;
		return;
	}
	if (facingEntity->def->eType == Enums::ET_NPC || (facingEntity->def->eType == Enums::ET_MONSTER && !(facingEntity->info & Entity::ENTITY_FLAG_TAKEDAMAGE))) {
		return;
	}
	int stat = facingEntity->monster->ce.getStat(Enums::STAT_MAX_HEALTH);
	int stat2 = facingEntity->monster->ce.getStat(Enums::STAT_HEALTH);
	if (facingEntity != this->lastTarget) {
		if (this->lastTarget != nullptr) {
			app->canvas->invalidateRect();
		}
		this->lastTarget = facingEntity;
		this->monsterDestHealth = (this->monsterStartHealth = stat2);
		this->monsterHealthChangeTime = 0;
	}
	else if (stat2 != this->monsterDestHealth) {
		this->monsterStartHealth = this->monsterDestHealth;
		this->monsterDestHealth = stat2;
		this->monsterHealthChangeTime = app->time;
	}
	if (this->monsterStartHealth > stat) {
		this->monsterStartHealth = stat;
	}
	if (app->time - this->monsterHealthChangeTime > 250) {
		this->monsterStartHealth = stat2;
	}
	else {
		stat2 = this->monsterStartHealth - (this->monsterStartHealth - this->monsterDestHealth) * (app->time - this->monsterHealthChangeTime) / 250;
	}
	int n = 25;
	if (facingEntity->def->eSubType == 13) {
		n = (3072 * ((stat << 16) / 51200) >> 8) + 256 - 1 >> 8;
	}
	else if (stat <= 200 && facingEntity->def->eSubType != 11) {
		n = (12800 * ((stat << 16) / 51200) >> 8) + 256 - 1 >> 8;
	}
	int n2 = ((n << 8) * ((stat2 << 16) / (stat << 8)) >> 8) + 256 - 1 >> 8;
	if (n2 == 0 && stat2 > 0) {
		n2 = 1;
	}
	int n3 = 6;
	if (facingEntity->def->eSubType == 14) {
		n3 = 0;
	}
	int n4 = 2 * (app->canvas->screenRect[2] << 8) / 128 >> 8;
	if ((n4 & 0x1) != 0x0) {
		++n4;
	}
	int n5 = 2 + n4 * n;
	int n6 = app->canvas->SCR_CX - (n5 >> 1);
	graphics->setColor(0xFF000000);
	graphics->fillRect(n6, 20 + n3, n5, n4 * 2 + 1);
	graphics->setColor(0xFFAAAAAA);
	graphics->drawRect(n6, 20 + n3, n5, n4 * 2 + 1);
	if (n2 <= n / 3) {
		graphics->setColor(0xFFFF0000);
	}
	else if (n - n2 <= n / 3) {
		graphics->setColor(0xFF00FF00);
	}
	else {
		graphics->setColor(0xFFFF8800);
	}
	n6 += 2;
	for (int i = 0; i < n2; ++i) {
		graphics->fillRect(n6, 20 + n3 + 2, n4 - 1, n4 * 2 - 2);
		n6 += n4;
	}
}

void Hud::showSpeechBubble(int i) {
	Applet* app = CAppContainer::getInstance()->app;

	if (this->bubbleText == nullptr) {
		this->bubbleText = app->localization->getSmallBuffer();
	}

	app->localization->composeText((int16_t)app->canvas->loadMapStringID, (int16_t)i, this->bubbleText);
	this->bubbleText->dehyphenate();
	this->bubbleTextTime = app->time + 1500;
}

void Hud::drawBubbleText(Graphics* graphics) {

	Applet* app = CAppContainer::getInstance()->app;

	if (this->bubbleText == nullptr) {
		return;
	}
	if (app->time >= this->bubbleTextTime) {
		this->bubbleTextTime = 0;
		this->bubbleText->dispose();
		this->bubbleText = nullptr;
		return;
	}
	int n = app->canvas->screenRect[1] + 1 + 20;
	int n2 = app->canvas->SCR_CX + 5;
	int n3 = 0;
	int n4 = 6;

	if (app->canvas->state == Canvas::ST_CAMERA) {
		n = app->canvas->cinRect[1] + 1;
	}
	if (app->player->facingEntity != nullptr && this->bubbleColor != Canvas::PLAYER_DLG_COLOR) {
		if (app->player->facingEntity->distFrom(app->canvas->destX, app->canvas->destY) <= app->combat->tileDistances[0]) {
			n += 10;
		}
		else {
			n += 25;
		}
	}
	int n5 = this->bubbleText->length() * Applet::CHAR_SPACING[app->fontType] + 6;
	int n6 = Applet::FONT_HEIGHT[app->fontType] + 4;
	int n7 = n2 - std::max(0, n5 + 2 - (app->canvas->screenRect[2] - n2));
	if (n7 + 15 < app->canvas->SCR_CX) {
		n4 = 12;
	}

	graphics->setColor(0xFF800000);
	graphics->fillRect(n7, n, n5, n6);
	graphics->setColor(-1);
	graphics->drawLine(n7, n, n7 + n5, n);
	graphics->drawLine(n7, n, n7, n + n6);
	graphics->drawLine(n7 + n5, n, n7 + n5, n + n6);
	graphics->drawLine(n7, n + n6, n7 + n5, n + n6);
	graphics->drawString(this->bubbleText, n7 + 2, n + 3, 4);
	graphics->drawRegion(app->canvas->imgChatHook_Monster, n3, n4, 10, 6, n7 + 5, n + n6, 0, 0, 0);
}

void Hud::drawArrowControls(Graphics* graphics) {
#ifdef WOLFENRPG_PSP
	// PSP has a physical D-pad and analog stick.  Do not obscure the play area
	// with the touch-only arrows and swipe controls from the iPhone release.
	return;
#endif
	Applet* app = CAppContainer::getInstance()->app;
	Image* imgDpad;

	if (app->canvas->m_controlMode == 1) {
		app->canvas->setBlendSpecialAlpha((float)(app->canvas->m_controlAlpha * 0.01f));

		int x = app->canvas->m_controlButtons[app->canvas->m_controlMode + 2]->GetButton(5)->touchArea.x + 13;
		int y = app->canvas->m_controlButtons[app->canvas->m_controlMode + 0]->GetButton(3)->touchArea.y + 13;

		int buttonID = app->canvas->m_controlButtons[app->canvas->m_controlMode + 2]->GetHighlightedButtonID();
		if (buttonID == 5) {
			imgDpad = app->canvas->imgDpad_left_press;
		}
		else if (buttonID == 7) {
			imgDpad = app->canvas->imgDpad_right_press;
		}
		else {
			buttonID = app->canvas->m_controlButtons[app->canvas->m_controlMode + 0]->GetHighlightedButtonID();
			if ((buttonID == 16) || (buttonID == 3)) {
				imgDpad = app->canvas->imgDpad_up_press;
			}
			else if ((buttonID == 17) || (buttonID == 9)) {
				imgDpad = app->canvas->imgDpad_down_press;
			}
			else {
				imgDpad = app->canvas->imgDpad_default;
			}
		}
		graphics->drawImage(imgDpad, x, y, 0, 0, 13);
	}

	app->canvas->m_controlButtons[app->canvas->m_controlMode + 2]->Render(graphics);
	app->canvas->m_controlButtons[app->canvas->m_controlMode + 0]->Render(graphics);
	app->canvas->m_controlButtons[app->canvas->m_controlMode + 4]->Render(graphics);
}

void Hud::drawWeapon(Graphics* graphics, int x, int y, int weapon, bool highlighted) {
	int texY;
	bool drawNumbers;
	Applet* app = CAppContainer::getInstance()->app;

	switch (weapon)
	{
	case Enums::WP_PUNCH:
	case Enums::WP_BRASS_PUNCH:
	case Enums::WP_SPIKE_PUNCH:
	case Enums::WP_ITEM:
	case Enums::WP_MOUNTED_GUN_TURRET:
		texY = 0;
		drawNumbers = false;
		break;
	case Enums::WP_BOOT:
		texY = 1;
		drawNumbers = false;
		break;
	case Enums::WP_PISTOL:
	case Enums::WP_DUAL_PISTOL:
		texY = 2;
		drawNumbers = true;
		break;
	case Enums::WP_THOMPSON:
		texY = 6;
		drawNumbers = true;
		break;
	case Enums::WP_STEN:
		texY = 5;
		drawNumbers = true;
		break;
	case Enums::WP_VENOM:
		texY = 11;
		drawNumbers = true;
		break;
	case Enums::WP_MAUSER:
		texY = 7;
		drawNumbers = true;
		break;
	case Enums::WP_FG42:
		texY = 8;
		drawNumbers = true;
		break;
	case Enums::WP_DYNAMITE:
		texY = 3;
		drawNumbers = true;
		break;
	case Enums::WP_PANZER:
		texY = 10;
		drawNumbers = true;
		break;
	case Enums::WP_FLAMETHROWER:
		texY = 9;
		drawNumbers = true;
		break;
	case Enums::WP_TESLA:
		texY = 12;
		drawNumbers = true;
		break;
	case Enums::WP_DESTINY_SPEAR:
		texY = 4;
		drawNumbers = false;
		break;
	default:
		//printf("ERROR: no matching image for weapon: %d \n", app->player->ce->weapon);
		return;
	}

	Image* imgWeapon = (highlighted) ? this->imgWeaponActive : this->imgWeaponNormal;
	graphics->drawRegion(imgWeapon, 0, texY * 41, imgWeapon->width, 41, x, y, 20, 0, 0);

	if (drawNumbers) {
		this->drawNumbers(graphics, x + 6, y + 8, 1, app->player->ammo[app->combat->weapons[(weapon * Combat::WEAPON_MAX_FIELDS) + Combat::WEAPON_FIELD_AMMOTYPE]]);
	}
}

void Hud::drawNumbers(Graphics* graphics, int x, int y, int space, int num) {
	int v11;
	int v12;
	int v15;
	Applet* app = CAppContainer::getInstance()->app;

	if (num < 1000) {
		v11 = num / 100;
		v12 = num % 100 / 10;
		v15 = num % 100 % 10;
		graphics->drawRegion(this->imgNumbers, 0, 20 * (9 - v11), 10, 20, x, y, 20, 0, 0);
		int posX = x + space + 10;
		graphics->drawRegion(this->imgNumbers, 0, 20 * (9 - v12), 10, 20, posX, y, 20, 0, 0);
		graphics->drawRegion(this->imgNumbers, 0, 20 * (9 - v15), 10, 20, space + posX + 10, y, 20, 0, 0);
	}
	else {
		puts("ERROR: drawnumbers() does not currently support values over 999 ");
	}
}

void Hud::drawCurrentKeys(Graphics* graphics, int x, int y) {
	Applet* app = CAppContainer::getInstance()->app;
	int v9;

	Player* player = app->player;
	if (player->inventory[Enums::INV_OTHER_GOLD_KEY] <= 0) {
		if (player->inventory[Enums::INV_OTHER_SILVER_KEY] <= 0) {
			v9 = 0;
		}
		else {
			v9 = 1;
		}
	}
	else if (player->inventory[Enums::INV_OTHER_SILVER_KEY] <= 0) {
		v9 = 2;
	}
	else {
		v9 = 3;
	}

	Image* imgKey = (this->m_hudButtons->GetButton(6)->highlighted) ? this->imgKeyActive : this->imgKeyNormal;
	graphics->drawRegion(imgKey, 0, 41 * v9, imgKey->width, 41, x, y, 20, 0, 0);
}

void Hud::drawWeaponSelection(Graphics* graphics) {
	Applet* app = CAppContainer::getInstance()->app;

	int weapons = ~app->player->disabledWeapons & app->player->weapons;
	int x = (app->canvas->screenRect[2] - 16 - 4 * this->imgWeaponNormal->width) / 2;
	int y = app->canvas->screenRect[3] - 105;
	graphics->FMGL_fillRect(0, this->imgPanelTop->height + app->canvas->screenRect[1], app->canvas->screenRect[2], app->canvas->screenRect[3] - this->imgPanelTop->height - 64, 0.0f, 0.0f, 0.0f, 0.5f);

	for (int i = 0; i < Hud::MAX_WEAPON_BUTTONS; i++) {
		this->m_weaponsButtons->GetButton(i)->drawButton = false;
	}

	int posY = -4;
	int v8 = 0;
	int posX = 0;
	for (int i = 0; i < Hud::MAX_WEAPON_BUTTONS; i++) {
		if (((weapons >> i) & 1) != 0) {
			if (v8 > 0 && (v8 & 3) == 0)
			{
				posY -= 45;
				posX = 0;
			}
			v8++;

			fmButton* button = this->m_weaponsButtons->GetButton(i);
			button->drawButton = true;
			this->drawWeapon(graphics, posX + x, posY + y, i, button->highlighted);
			button->SetTouchArea(posX + x, posY + y, this->imgWeaponNormal->width, 41);
			posX += this->imgWeaponNormal->width + 4;
		}
	}
}

void Hud::handleUserMoved(int pressX, int pressY) {
	for (int i = 0; i < 7; i++) {
		this->m_hudButtons->GetButton(i)->SetHighlighted(false);
	}

	for (int i = 0; i < Hud::MAX_WEAPON_BUTTONS; i++) {
		this->m_weaponsButtons->GetButton(i)->SetHighlighted(false);
	}

	int buttonID_1 = this->m_hudButtons->GetTouchedButtonID(pressX, pressY);
	int buttonID_2 = this->m_weaponsButtons->GetTouchedButtonID(pressX, pressY);

	if (buttonID_2 >= 0 && this->m_weaponsButtons->GetButton(buttonID_2)->drawButton) {
		this->m_weaponsButtons->GetButton(buttonID_2)->SetHighlighted(true);
	}
	else if (buttonID_1 >= 0 && this->m_hudButtons->GetButton(buttonID_1)->drawButton) {
		this->m_hudButtons->GetButton(buttonID_1)->SetHighlighted(true);
	}
}

void Hud::handleUserTouch(int pressX, int pressY, bool highlighted) {
	Applet* app = CAppContainer::getInstance()->app;
	int buttonID;

	for (int i = 0; i < 7; i++) {
		this->m_hudButtons->GetButton(i)->SetHighlighted(false);
	}

	for (int i = 0; i < Hud::MAX_WEAPON_BUTTONS; i++) {
		this->m_weaponsButtons->GetButton(i)->SetHighlighted(false);
	}

	if (app->canvas->touched) { return; }

	int buttonID_1 = this->m_hudButtons->GetTouchedButtonID(pressX, pressY);
	int buttonID_2 = this->m_weaponsButtons->GetTouchedButtonID(pressX, pressY);

	if (buttonID_2 >= 0 && this->isInWeaponSelect) {
		if (highlighted) {
			this->m_weaponsButtons->GetButton(buttonID_2)->SetHighlighted(true);
		}
		else {
			app->player->selectWeapon(buttonID_2);
			this->isInWeaponSelect = false;
		}
	}
	else if (buttonID_1 >= 0) {
		if (highlighted) {
			this->m_hudButtons->GetButton(buttonID_1)->SetHighlighted(true);
			if (buttonID_1 == 2) {
				this->weaponPressTime = app->upTimeMs;
			}
		}
		else
		{
			if (buttonID_1 != 2) {
				this->isInWeaponSelect = false;
			}

			if (app->canvas->state != Canvas::ST_COMBAT) {
				if (app->canvas->state == Canvas::ST_DRIVING || app->canvas->isChickenKicking)
				{
					if (buttonID_1 == 0) {
						app->sound->playSound(1009, 0, 5, 0);
						app->canvas->handlePlayingEvents(0, Enums::ACTION_MENU);
					}
				}
				else {
					switch (buttonID_1)
					{
					case 0:
						if (app->canvas->isZoomedIn) {
							app->canvas->handleZoomEvents(0, Enums::ACTION_BACK);
						}
						app->canvas->handlePlayingEvents(0, Enums::ACTION_MENU);
						break;
					case 1:
						app->sound->playSound(1009, 0, 5, 0);
						app->canvas->handlePlayingEvents(0, Enums::ACTION_AUTOMAP);
						break;
					case 2:
						if (app->canvas->isZoomedIn) {
							app->canvas->handleZoomEvents(0, Enums::ACTION_BACK);
						}
						else {
							if ((uint32_t)(app->upTimeMs - this->weaponPressTime) >= 300) {
								break;
							}
							else {
								if (this->isInWeaponSelect) {
									this->isInWeaponSelect = false;
								}
								else {
									app->canvas->handlePlayingEvents(this->isInWeaponSelect, Enums::ACTION_NEXTWEAPON);
								}
								this->weaponPressTime = 0;
							}
						}
						break;
					case 3:
						app->canvas->handlePlayingEvents(0, Enums::ACTION_PASSTURN);
						break;
					case 4:
						//app->menuSystem->setMenu(Menus::MENU_ITEMS_SYRINGES); // Old
						app->canvas->handlePlayingEvents(0, Enums::ACTION_ITEMS_SYRINGES); // [GEC] From Doom II RPG
						break;
					case 5:
						//app->menuSystem->setMenu(Menus::MENU_ITEMS); // Old
						app->canvas->handlePlayingEvents(0, Enums::ACTION_ITEMS); // [GEC] From Doom II RPG
						break;
					case 6:
						//app->menuSystem->setMenu(Menus::MENU_INGAME_QUESTLOG); // Old
						app->canvas->handlePlayingEvents(0, Enums::ACTION_QUESTLOG); // [GEC] From Doom II RPG
						break;
					default:
						printf("ERROR: undefined touch button ID: %d \n", buttonID_1);
						break;
					}
				}
			}
		}
	}
}

void Hud::update() {
	Applet* app = CAppContainer::getInstance()->app;

	if (this->m_hudButtons->GetButton(2)->drawButton && this->m_hudButtons->GetButton(2)->highlighted) {
		if (this->weaponPressTime) {
			if ((uint32_t)(app->upTimeMs - this->weaponPressTime) >= 300) {
				if (app->canvas->state != Canvas::ST_DRIVING && app->canvas->state != Canvas::ST_COMBAT && !app->canvas->isChickenKicking) {
					this->isInWeaponSelect = true;
				}
				this->weaponPressTime = 0;
			}
		}
	}
}
