#include <stdexcept>
#include <cstring>
#include <climits>
#include <assert.h>

#include "SDLGL.h"
#include "App.h"
#include "Image.h"
#include "CAppContainer.h"
#include "Canvas.h"
#include "Graphics.h"
#include "MayaCamera.h"
#include "Game.h"
#include "GLES.h"
#include "TinyGL.h"
#include "Hud.h"
#include "Render.h"
#include "Combat.h"
#include "Player.h"
#include "MenuSystem.h"
#include "ParticleSystem.h"
#include "Text.h"
#include "Button.h"
#include "Sound.h"
#include "Resource.h"
#include "Enums.h"
#include "Utils.h"
#include "Menus.h"
#include "Input.h"
#include "CardGames.h"
#include "DrivingGame.h"

#include "Utils.h"

constexpr int Canvas::viewStepValues[];
constexpr int Canvas::CKPOINTS[20];
constexpr uint32_t Canvas::GRID_COLORS[20];
constexpr char Canvas::numCharTable[10][6];
constexpr int Canvas::ARROW_DATA[16];
constexpr int Canvas::SELECTORPOS[18];
constexpr int Canvas::MAG_DATA[15];

Canvas::Canvas() {
	memset(this, 0, sizeof(Canvas));
}

Canvas::~Canvas() {
}

bool Canvas::isLoaded;

bool Canvas::startup() {
	Applet* app = CAppContainer::getInstance()->app;
	int viewWidth, viewHeight;
	fmButton* button;

	//printf("Canvas::startup\n");

	this->displayRect[0] = 0;
	this->displayRect[1] = 0;
	this->displayRect[2] = app->backBuffer->width;
	this->displayRect[3] = app->backBuffer->height;

	//printf("this->displayRect[0] %d\n", this->displayRect[0]);
	//printf("this->displayRect[1] %d\n", this->displayRect[1]);
	//printf("this->displayRect[2] %d\n", this->displayRect[2]);
	//printf("this->displayRect[3] %d\n", this->displayRect[3]);

	this->graphics.setGraphics();

	this->m_controlButton = 0;
	this->m_controlButtonIsTouched = 0;
	this->m_controlButtonTime = 0;
	this->field_0xacc_ = 0;
	this->field_0xacd_ = 0;
	this->dialogThread = 0;

	this->vibrateEnabled = true;
	this->startupMap = 1;
	this->loadMapStringID = -1;
	this->specialLootIcon = -1;
	this->pacLogoIndex = -1;
	this->ignoreFrameInput = false;
	this->blockInputTime = 0;
	this->showLocation = false;
	this->forceError = false;
	this->lastPacifierUpdate = 0;
	this->numEvents = 0;
	this->dialogItem = nullptr;
	this->dialogViewLines = 0;
	this->lastMapID = 0;
	this->loadMapID = 0;
	this->automapDrawn = false;
	this->loadType = Game::LOAD_NONE;
	this->saveType = Game::SAVE_NONE;
	this->st_count = 0;
	this->knockbackDist = 0;
	this->numHelpMessages = 0;
	this->destZ = 36;
	this->viewZ = 36;
	this->screenRect[2] = 0;
	this->screenRect[3] = 0;
	this->dialogThread = nullptr;

	this->ambientSoundsTime = 0;
	app->setFont(0);

	this->displayRect[2] &= 0xFFFFFFFE;
	this->screenRect[2] = this->displayRect[2];
	this->screenRect[3] = this->displayRect[3];

	this->dialogMaxChars = (this->displayRect[2] - 2) / Applet::CHAR_SPACING[app->fontType];
	this->scrollMaxChars = (this->displayRect[2] - 2) / Applet::CHAR_SPACING[app->fontType];
	this->dialogWithBarMaxChars		= (this->displayRect[2] - 9) / Applet::CHAR_SPACING[app->fontType];
	this->scrollWithBarMaxChars		= (this->displayRect[2] - 9) / Applet::CHAR_SPACING[app->fontType];
	this->menuScrollWithBarMaxChars = (this->displayRect[2] - 9) / Applet::CHAR_SPACING[app->fontType];
	this->ingameScrollWithBarMaxChars = (this->displayRect[2] - 25) / Applet::CHAR_SPACING[app->fontType];
	this->menuHelpMaxChars = (this->displayRect[2] - 16) / Applet::CHAR_SPACING[app->fontType];
	this->subtitleMaxChars = this->displayRect[2] / Applet::CHAR_SPACING[app->fontType];

	if (app->hud->startup()) {

		int n2 = this->screenRect[3] - (42 + 42);
		if (this->displayRect[3] >= 128) {
			this->displaySoftKeys = true;
			this->softKeyY = this->displayRect[3];
			if (this->displayRect[3] - this->screenRect[3] < 0) {
				n2 = this->displayRect[3] - (42 + 42);
			}
		}
		else {
			this->softKeyY = this->displayRect[3];
		}

		uint32_t n3 = n2 & 0xFFFFFFFE;
		this->screenRect[3] = n3 + (42 + 42);
		this->screenRect[0] = (this->displayRect[2] - this->screenRect[2]) / 2;
		if (this->displaySoftKeys) {
			this->screenRect[1] = (this->softKeyY - this->screenRect[3]) / 2;
		}
		else {
			this->screenRect[1] = (this->displayRect[3] - this->screenRect[3]) / 2;
		}

		this->SCR_CX = this->screenRect[2] / 2;
		this->SCR_CY = this->screenRect[3] / 2;
		this->viewRect[0] = this->screenRect[0];
		this->viewRect[1] = this->screenRect[1] + 20;
		this->viewRect[2] = this->screenRect[2];
		this->viewRect[3] = n3;

		// custom
		viewWidth = 0;
		viewHeight = 0;
		if (viewWidth != 0 && viewHeight != 0) {
			this->viewRect[0] += ((this->viewRect[2] - viewWidth) >> 1);
			this->viewRect[1] += ((this->viewRect[3] - viewHeight) >> 1);
			this->viewRect[2] = viewWidth;
			this->viewRect[3] = viewHeight;
		}

		this->hudRect[0] = this->displayRect[0];
		this->hudRect[1] = this->screenRect[1];
		this->hudRect[2] = this->displayRect[2];
		this->hudRect[3] = this->screenRect[3];

		this->menuRect[0] = this->displayRect[0];
		this->menuRect[1] = this->displayRect[1];
		this->menuRect[2] = this->displayRect[2];
		this->menuRect[3] = this->displayRect[3];

		this->CAMERAVIEW_BAR_HEIGHT = ((this->viewRect[3] >> 4) + (this->viewRect[3] >> 2)) / 2;
		
		this->cinRect[0] = 0;//this->viewRect[0];
		this->cinRect[1] = this->viewRect[1] + this->CAMERAVIEW_BAR_HEIGHT;
		this->cinRect[2] = this->viewRect[2];
		this->cinRect[3] = this->viewRect[3] - this->CAMERAVIEW_BAR_HEIGHT;

		if (this->screenRect[1] + this->screenRect[3] == this->softKeyY + -1) {
			this->softKeyY = this->screenRect[1] + this->screenRect[3];
		}

		this->setAnimFrames(13);

		this->startupMap = 1;
		this->skipIntro = false;
		this->tellAFriend = false;

		app->beginImageLoading();
		//this->imgDialogScroll = app->loadImage("DialogScroll.bmp", true);
		this->imgFabricBG = app->loadImage("FabricBG.bmp", true);
		this->imgFonts[0] = app->loadImage("Font_16p_Light.bmp", true);
		this->imgFonts[1] = app->loadImage("Font_16p_Dark.bmp", true);
		this->imgFonts[2] = app->loadImage("Font_18p_Light.bmp", true);
		this->imgFonts[3] = app->loadImage("WarFont.bmp", true);
		this->fontRenderMode = 0;
		this->imgFont = this->imgFonts[0];
		this->imgIcons_Buffs = app->loadImage("Icons_Buffs.bmp", true);
		this->imgIcons_Buffs_Help = app->loadImage("Icons_Buffs_Help.bmp", true);
		app->menuSystem->imgMedal = app->loadImage("medal.bmp", true);
		this->imgNotebookBg = app->loadImage("Notebook.bmp", true);
		app->menuSystem->imgScroll = app->loadImage("Scroll.bmp", true);
		app->hud->imgSoftKeyFill = app->loadImage("Soft_Key_Fill.bmp", true);
		app->finishImageLoading();

		this->lootSource = -1;
		this->imgMixingBG = 0;
		this->imgMixingHeadingPlate = 0;
		this->imgMixingTTRedNormal = 0;
		this->imgMixingTTRedSelected = 0;
		this->imgMixingTTRedEmpty = 0;
		this->imgMixingTTGreenNormal = 0;
		this->imgMixingTTGreenSelected = 0;
		this->imgMixingTTGreenEmpty = 0;
		this->imgMixingTTBludeNormal = 0;
		this->imgMixingTTBludeSelected = 0;
		this->imgMixingTTBludeEmpty = 0;
		this->imgMixingSyringe = 0;
		this->imgMixingSyringeSelected = 0;
		this->imgMixingSyringeRed = 0;
		this->imgMixingSyringeGreen = 0;
		this->imgMixingSyringeBlue = 0;
		this->imgMixingNumbersPlate = 0;
		this->imgMixingSyringeNumbersPlate = 0;
		this->vibrateTime = 0;
		this->highScores[2] = 200;
		this->highScores[3] = 150;
		this->highScores[4] = 145;
		this->highScores[0] = 290;
		this->highScores[1] = 250;
		memcpy(this->highScoreInitials, "BENKDSBLEMCRKAK", sizeof(this->highScoreInitials));
		this->chickenDestFwd = 2;
		this->chickenDestRight = 0;

		this->blendSpecialAlpha = 0.5f;
		this->m_controlLayout = 2;
		this->m_controlFlip = false;
		this->m_controlMode = 1;
		this->vibrateTime = 0;
		this->areSoundsAllowed = false;
		this->m_controlAlpha = 50;
		this->m_controlGraphic = 0;

		char* arrowsFiles[] = {
			"arrow-up.bmp",
			"greenArrow_up.bmp",
			"arrow-up_pressed.bmp",
			"greenArrow_up-pressed.bmp",
			"arrow-down.bmp",
			"greenArrow_down.bmp",
			"arrow-down_pressed.bmp",
			"greenArrow_down-pressed.bmp",
			"arrow-left.bmp",
			"greenArrow_left.bmp",
			"arrow-left_pressed.bmp",
			"greenArrow_left-pressed.bmp",
			"arrow-right.bmp",
			"greenArrow_right.bmp",
			"arrow-right_pressed.bmp",
			"greenArrow_right-pressed.bmp"
		};

		char** files = arrowsFiles;
		Image **imgArrows = this->imgArrows;
		for (int i = 0; i < 2; i++) {
			imgArrows[0] = app->loadImage(files[0], true);
			imgArrows[2] = app->loadImage(files[2], true);
			imgArrows[4] = app->loadImage(files[4], true);
			imgArrows[6] = app->loadImage(files[6], true);
			imgArrows[8] = app->loadImage(files[8], true);
			imgArrows[10] = app->loadImage(files[10], true);
			imgArrows[12] = app->loadImage(files[12], true);
			imgArrows[14] = app->loadImage(files[14], true);
			imgArrows++;
			files++;
		}

		this->imgDpad_default = app->loadImage("dpad_default.bmp", true);
		this->imgDpad_up_press = app->loadImage("dpad_up-press.bmp", true);
		this->imgDpad_down_press = app->loadImage("dpad_down-press.bmp", true);
		this->imgDpad_left_press = app->loadImage("dpad_left-press.bmp", true);
		this->imgDpad_right_press = app->loadImage("dpad_right-press.bmp", true);
		this->imgPageUP_Icon = app->loadImage("pageUP_Icon.bmp", true);
		this->imgPageDOWN_Icon = app->loadImage("pageDOWN_Icon.bmp", true);
		this->imgPageOK_Icon = app->loadImage("pageOK_Icon.bmp", true);
		this->imgSniperScope_Dial = app->loadImage("SniperScope_Dial.bmp", true);
		//this->imgSniperScope_Knob = app->loadImage("SniperScope_Knob.bmp", true);

		this->touched = false;

		// Setup Sniper Scope Dial Scroll Button
		{
			this->m_sniperScopeDialScrollButton = new fmScrollButton(386, 89, this->imgSniperScope_Dial->width >> 1, this->imgSniperScope_Dial->height >> 2, true, 1113);
			this->m_sniperScopeDialScrollButton->SetScrollBox(0, 0, 1, 1, 16);
			this->m_sniperScopeDialScrollButton->field_0x0_ = 1;
		}

		// Setup Sniper Scope Buttons
		{
			this->m_sniperScopeButtons = new fmButtonContainer();
			button = new fmButton(6, 122, 20, 236, 236, -1);
			this->m_sniperScopeButtons->AddButton(button);
		}

		// Setup Control Buttons
		{
			fmButtonContainer** m_controlButtons = this->m_controlButtons;
			for (int i = 0; i < 2; i++) {
				m_controlButtons[0] = new fmButtonContainer();
				m_controlButtons[2] = new fmButtonContainer();
				m_controlButtons[4] = new fmButtonContainer();

				if (i == 1) {
					int v53 = 5;
					int v49 = 117;
					while (1)
					{
						button = new fmButton(5, v53, v53 + 116, 13, v49, -1);
						//button->drawTouchArea = true; // Test
						m_controlButtons[2]->AddButton(button);
						button = new fmButton(7, 140 - v53, v53 + 116, 13, v49, -1);
						//button->drawTouchArea = true; // Test
						m_controlButtons[2]->AddButton(button);
						button = new fmButton(3, v53 + 13, v53 + 103, v49, 13, -1);
						//button->drawTouchArea = true; // Test
						m_controlButtons[0]->AddButton(button);
						button = new fmButton(9, v53 + 13, 243 - v53, v49, 13, -1);
						//button->drawTouchArea = true; // Test
						m_controlButtons[0]->AddButton(button);
						v49 -= 26;
						if (v53 == 44)
							break;
						v53 += 13;
					}
					button = new fmButton(6, 153, 25, 322, 226, -1);
					m_controlButtons[2]->AddButton(button);
					this->m_swipeArea[1] = new fmSwipeArea(0, 0, this->screenRect[2], this->screenRect[3] - 64, 50, 50);
				}
				else {
					button = new fmButton(3, 42, 31, 70, 70, -1);
					button->SetImage(&this->imgArrows[0], 0, true);
					button->SetHighlightImage(&this->imgArrows[2], 0, true);
					button->normalRenderMode = 11;
					button->highlightRenderMode = 11;
					m_controlButtons[0]->AddButton(button);
					button = new fmButton(9, 42, 181, 70, 70, -1);
					button->SetImage(&this->imgArrows[4], 0, true);
					button->SetHighlightImage(&this->imgArrows[6], 0, true);
					button->normalRenderMode = 11;
					button->highlightRenderMode = 11;
					m_controlButtons[0]->AddButton(button);
					button = new fmButton(5, 5, 106, 70, 70, -1);
					button->SetImage(&this->imgArrows[8], 0, true);
					button->SetHighlightImage(&this->imgArrows[10], 0, true);
					button->normalRenderMode = 11;
					button->highlightRenderMode = 11;
					m_controlButtons[2]->AddButton(button);
					button = new fmButton(7, 80, 106, 70, 70, -1);
					button->SetImage(&this->imgArrows[12], 0, true);
					button->SetHighlightImage(&this->imgArrows[14], 0, true);
					button->normalRenderMode = 11;
					button->highlightRenderMode = 11;
					m_controlButtons[2]->AddButton(button);
					button = new fmButton(6, 155, 25, 229, 226, -1);
					m_controlButtons[2]->AddButton(button);
					button = new fmButton(6, 384, 172, 90, 79, -1);
					m_controlButtons[2]->AddButton(button);
					this->m_swipeArea[0] = new fmSwipeArea(0, 0, this->screenRect[2], this->screenRect[3] - 64, 50, 50);
				}

				m_controlButtons++;
			}
		}

		// Setup Dialog Buttons
		{
			this->m_dialogButtons = new fmButtonContainer();
			for (int i = 0; i < 5; i++) {
				button = new fmButton(i, 0, 0, 0, 0, 1027);
				this->m_dialogButtons->AddButton(button);
			}
			button = new fmButton(5, 390, 20, 90, 90, 1027);
			button->SetImage(this->imgPageUP_Icon, true);
			button->SetHighlightImage(this->imgPageUP_Icon, true);
			button->normalRenderMode = 10;
			button->highlightRenderMode = 0;
			this->m_dialogButtons->AddButton(button);
			button = new fmButton(6, 390, 110, 90, 90, 1027);
			button->SetImage(this->imgPageDOWN_Icon, true);
			button->SetHighlightImage(this->imgPageDOWN_Icon, true);
			button->normalRenderMode = 10;
			button->highlightRenderMode = 0;
			this->m_dialogButtons->AddButton(button);
			button = new fmButton(7, 390, 110, 90, 90, 1027);
			button->SetImage(this->imgPageOK_Icon, true);
			button->SetHighlightImage(this->imgPageOK_Icon, true);
			button->normalRenderMode = 10;
			button->highlightRenderMode = 0;
			this->m_dialogButtons->AddButton(button);
			button = new fmButton(8, 0, 0, 0, 0, 1027);
			this->m_dialogButtons->AddButton(button);
		}

		// Setup SoftKey Buttons
		{
			this->m_softKeyButtons = new fmButtonContainer();
			button = new fmButton(19, 0, 250, 100, 70, 1027);
			this->m_softKeyButtons->AddButton(button);
			button = new fmButton(20, 380, 250, 100, 70, 1027);
			this->m_softKeyButtons->AddButton(button);
		}

		// Setup Mixing Buttons
		{
			this->m_mixingButtons = new fmButtonContainer();
			button = new fmButton(0, 20, 108, 60, 100, 1027);
			this->m_mixingButtons->AddButton(button);
			button = new fmButton(1, 82, 108, 60, 100, 1027);
			this->m_mixingButtons->AddButton(button);
			button = new fmButton(2, 144, 108, 60, 100, 1027);
			this->m_mixingButtons->AddButton(button);
			button = new fmButton(3, 0, 250, 100, 70, 1027);
			this->m_mixingButtons->AddButton(button);
			button = new fmButton(4, 380, 250, 100, 70, 1027);
			this->m_mixingButtons->AddButton(button);
			button = new fmButton(5, 222, 115, 255, 72, 1027);
			this->m_mixingButtons->AddButton(button);
		}

		// Setup Story Buttons
		{
			this->m_storyButtons = new fmButtonContainer();
			button = new fmButton(0, 0, 280, 60, 40, 1027); // Old -> (0, 0, 250, 100, 70, 1027);
			this->m_storyButtons->AddButton(button);
			button = new fmButton(1, 380, 280, 100, 40, 1027); // Old -> (1, 380, 250, 100, 70, 1027);
			this->m_storyButtons->AddButton(button);
			button = new fmButton(2, 420, 0, 60, 40, 1027);// Old -> (2, 380, 0, 100, 70, 1027);
			this->m_storyButtons->AddButton(button);
		}

		return true;
	}

	return false;
}

void Canvas::flushGraphics() {
	this->graphics.resetScreenSpace();
	this->backPaint(&this->graphics);
}

void Canvas::backPaint(Graphics* graphics) {
	Applet* app = CAppContainer::getInstance()->app;

	graphics->clearClipRect();

	if (this->repaintFlags & Canvas::REPAINT_CLEAR) {
		this->repaintFlags &= ~Canvas::REPAINT_CLEAR;
		graphics->eraseRgn(this->displayRect);
	}

	if (this->repaintFlags & Canvas::REPAINT_VIEW3D) {
		if (app->render->_gles->isInit) {
			this->repaintFlags &= ~Canvas::REPAINT_VIEW3D;
			if (app->render->isFading()) {
				app->render->fadeScene(graphics);
			}
		}
		else {
			//this->repaintFlags &= ~Canvas::REPAINT_VIEW3D;
			//app->render->Render3dScene();
			//app->render->drawRGB(graphics);
		}
	}

	if (this->repaintFlags & Canvas::REPAINT_PARTICLES) {
		this->repaintFlags &= ~Canvas::REPAINT_PARTICLES;
		app->particleSystem->renderSystems(graphics);
	}

	if (this->state == Canvas::ST_COMBAT || this->state == Canvas::ST_PLAYING) {
#ifndef WOLFENRPG_PSP
		this->m_swipeArea[this->m_controlMode]->Render(graphics);
#endif
	}

	if ((this->repaintFlags & Canvas::REPAINT_HUD) != 0) {
		this->repaintFlags &= ~Canvas::REPAINT_HUD;
		app->hud->draw(graphics);
	}

	if (this->state == Canvas::ST_INTRO_MOVIE) {
		this->playIntroMovie(graphics);
	}
	else if (this->state == Canvas::ST_INTRO) {
		this->drawStory(graphics);
	}
	else if (this->state == Canvas::ST_EPILOGUE) {
		this->drawScrollingText(graphics);
	}
	else if (this->state == Canvas::ST_CREDITS) {
		this->drawCredits(graphics);
	}
	else if (this->state == Canvas::ST_TRAVELMAP) {
		this->drawTravelMap(graphics);
	}
	else if (this->state == Canvas::ST_AUTOMAP) {
		this->drawAutomap(graphics, true); //!this->automapDrawn
		this->m_softKeyButtons->Render(graphics);
		app->hud->drawArrowControls(graphics);
		this->automapDrawn = true;
	}
	else if (this->state == Canvas::ST_MIXING) {
		this->mixingState(graphics);
	}
	else if (this->state == Canvas::ST_DIALOG) {
		this->dialogState(graphics);
	}
	else if (this->state == Canvas::ST_MINI_GAME) {
		app->cardGames->updateGame(graphics);
	}
	else if (this->state == Canvas::ST_PLAYING || this->state == Canvas::ST_COMBAT) {
		if (this->state == Canvas::ST_PLAYING && this->isChickenKicking) {
			if (this->kickingPhase == 5 && this->stateVars[0] < app->gameTime) {
				this->drawHighScore(graphics);
			}
			else if (this->kickingPhase == 4 && this->gridTime >= app->gameTime) {
				this->drawKickingGrid(graphics);
			}
		}
		if (this->isChickenKicking && this->kickingPhase < 4) {
			this->drawKickingBars(graphics);
		}
	}
	else if (this->state == Canvas::ST_ERROR) {
		//this->errorState(graphics);
	}

	/*if ((this->repaintFlags & Canvas::REPAINT_SOFTKEYS) != 0x0) { // REPAINT_SOFTKEYS
		this->repaintFlags &= ~Canvas::REPAINT_SOFTKEYS;
		this->drawSoftKeys(graphics);
	}*/

	if (this->repaintFlags & Canvas::REPAINT_MENU) {
		this->repaintFlags &= ~Canvas::REPAINT_MENU;
		app->menuSystem->paint(graphics);
	}

	if (this->repaintFlags & Canvas::REPAINT_STARTUP_LOGO) {
		this->repaintFlags &= ~Canvas::REPAINT_STARTUP_LOGO;
		//graphics->fillRect(0, 0, this->displayRect[2], this->displayRect[3], -0x1000000);
		graphics->drawImage(this->imgFabricBG, this->displayRect[2] / 2, this->displayRect[3] / 2, 3, 0, 0);
		if (!this->pacLogoIndex) {
			graphics->drawImage(this->imgStartupLogo, this->displayRect[2] / 2, this->displayRect[3] / 2, 3, 0, 0);
		}
	}

	//printf("this->repaintFlags %d\n", this->repaintFlags);
	if (this->repaintFlags & Canvas::REPAINT_LOADING_BAR) { // REPAINT_LOADING_BAR
		this->repaintFlags &= ~Canvas::REPAINT_LOADING_BAR;
		this->drawLoadingBar(graphics);
	}

	if (this->fadeFlags && (app->time < this->fadeTime + this->fadeDuration)) {
		int alpha = ((app->time - this->fadeTime) << 8) / this->fadeDuration;

		if ((this->fadeFlags & Canvas::FADE_FLAG_FADEOUT) != 0) {
			alpha = 256 - alpha;
		}

		graphics->fade(this->fadeRect, alpha, this->fadeColor);
	}
	else {
		this->fadeFlags = Canvas::FADE_FLAG_NONE;
	}

	if (this->state == Canvas::ST_BENCHMARK) {
		if (this->st_enabled) {
			int n = this->viewRect[1] + 20;
			this->debugTime = app->upTimeMs;
			Text* largeBuffer = app->localization->getLargeBuffer();
			largeBuffer->setLength(0);
			largeBuffer->append("Rndr ms: ");
			largeBuffer->append(this->st_fields[0] / this->st_count)->append('.');
			largeBuffer->append(this->st_fields[0] * 100 / this->st_count - this->st_fields[0] / this->st_count * 100);
			graphics->drawString(largeBuffer, this->viewRect[0], n, 0);
			n += Applet::FONT_HEIGHT[app->fontType];
			largeBuffer->setLength(0);
			largeBuffer->append("Bsp ms: ");
			largeBuffer->append(this->st_fields[1] / this->st_count)->append('.');
			largeBuffer->append(this->st_fields[1] * 100 / this->st_count - this->st_fields[1] / this->st_count * 100);
			graphics->drawString(largeBuffer, this->viewRect[0], n, 0);
			n += Applet::FONT_HEIGHT[app->fontType];
			largeBuffer->setLength(0);
			largeBuffer->append("Hud ms: ");
			largeBuffer->append(this->st_fields[2] / this->st_count)->append('.');
			largeBuffer->append(this->st_fields[2] * 100 / this->st_count - this->st_fields[2] / this->st_count * 100);
			graphics->drawString(largeBuffer, this->viewRect[0], n, 0);
			n += Applet::FONT_HEIGHT[app->fontType];
			int n2 = this->st_fields[4] + this->st_fields[5];
			largeBuffer->setLength(0);
			largeBuffer->append("Blit ms: ");
			largeBuffer->append(n2 / this->st_count)->append('.');
			largeBuffer->append(n2 * 100 / this->st_count - n2 / this->st_count * 100);
			graphics->drawString(largeBuffer, this->viewRect[0], n, 0);
			n += Applet::FONT_HEIGHT[app->fontType];
			largeBuffer->setLength(0);
			largeBuffer->append("Paus ms: ");
			largeBuffer->append(this->st_fields[6] / this->st_count)->append('.');
			largeBuffer->append(this->st_fields[6] * 100 / this->st_count - this->st_fields[6] / this->st_count * 100);
			graphics->drawString(largeBuffer, this->viewRect[0], n, 0);
			n += Applet::FONT_HEIGHT[app->fontType];
			largeBuffer->setLength(0);
			largeBuffer->append("Dbg ms: ");
			largeBuffer->append(this->st_fields[9] / this->st_count)->append('.');
			largeBuffer->append(this->st_fields[9] * 100 / this->st_count - this->st_fields[9] / this->st_count * 100);
			graphics->drawString(largeBuffer, this->viewRect[0], n, 0);
			n += Applet::FONT_HEIGHT[app->fontType];
			largeBuffer->setLength(0);
			largeBuffer->append("Loop ms: ");
			largeBuffer->append(this->st_fields[7] / this->st_count)->append('.');
			largeBuffer->append(this->st_fields[7] * 100 / this->st_count - this->st_fields[7] / this->st_count * 100);
			graphics->drawString(largeBuffer, this->viewRect[0], n, 0);
			n += Applet::FONT_HEIGHT[app->fontType];
			largeBuffer->setLength(0);
			largeBuffer->append("Key ms: ");
			largeBuffer->append(this->st_fields[11] - this->st_fields[10]);
			graphics->drawString(largeBuffer, this->viewRect[0], n, 0);
			n += Applet::FONT_HEIGHT[app->fontType];
			largeBuffer->setLength(0);
			largeBuffer->append("State ms: ");
			largeBuffer->append(this->st_fields[12] / this->st_count)->append('.');
			largeBuffer->append(this->st_fields[12] * 100 / this->st_count - this->st_fields[12] / this->st_count * 100);
			graphics->drawString(largeBuffer, this->viewRect[0], n, 0);
			n += Applet::FONT_HEIGHT[app->fontType];
			largeBuffer->setLength(0);
			largeBuffer->append("Totl ms: ");
			largeBuffer->append(this->st_fields[8] / this->st_count)->append('.');
			largeBuffer->append(this->st_fields[8] * 100 / this->st_count - this->st_fields[8] / this->st_count * 100);
			graphics->drawString(largeBuffer, this->viewRect[0], n, 0);
			n += Applet::FONT_HEIGHT[app->fontType];
			largeBuffer->setLength(0);
			largeBuffer->append(this->st_count);
			graphics->drawString(largeBuffer, this->viewRect[0], n, 0);
			n += Applet::FONT_HEIGHT[app->fontType];
			this->debugTime = app->upTimeMs - this->debugTime;
			largeBuffer->dispose();
		}
	}
	else if (this->state == Canvas::ST_CAMERA || this->state == Canvas::ST_PLAYING || this->state == Canvas::ST_COMBAT || this->state == Canvas::ST_DRIVING) {
		int n3 = this->viewRect[1] + 20;
		if (this->showSpeeds) {
			int lastRenderTime = this->afterRender - this->beforeRender;
			if (this->lastFrameTime == app->time) {
				this->afterRender = (this->beforeRender = 0);
				this->lastRenderTime = lastRenderTime;
			}
			int n4 = app->time - this->totalFrameTime;
			this->totalFrameTime = app->time;
			Text* largeBuffer2 = app->localization->getLargeBuffer();
			largeBuffer2->setLength(0);
			largeBuffer2->append("ms: ");
			largeBuffer2->append(this->lastRenderTime)->append('/');
			largeBuffer2->append(app->render->clearColorBuffer)->append('/');
			largeBuffer2->append(app->render->bltTime)->append('/');
			largeBuffer2->append(n4);
			graphics->drawString(largeBuffer2, this->viewRect[0], n3, 0);
			n3 += Applet::FONT_HEIGHT[app->fontType];
			largeBuffer2->setLength(0);
			largeBuffer2->append("li: ");
			largeBuffer2->append(app->render->lineRasterCount)->append('/');
			largeBuffer2->append(app->render->lineCount);
			graphics->drawString(largeBuffer2, this->viewRect[0], n3, 0);
			n3 += Applet::FONT_HEIGHT[app->fontType];
			largeBuffer2->setLength(0);
			largeBuffer2->append("sp: ");
			largeBuffer2->append(app->render->spriteRasterCount)->append('/');
			largeBuffer2->append(app->render->spriteCount)->append('/');
			largeBuffer2->append(app->render->numMapSprites);
			graphics->drawString(largeBuffer2, this->viewRect[0], n3, 0);
			n3 += Applet::FONT_HEIGHT[app->fontType];
			if (app->render->renderMode == 63) {
				largeBuffer2->setLength(0);
				largeBuffer2->append("cnt: ");
				largeBuffer2->append(app->tinyGL->spanCalls)->append('/');
				largeBuffer2->append(app->tinyGL->spanPixels);
				graphics->drawString(largeBuffer2, this->viewRect[0], n3, 0);
				n3 += Applet::FONT_HEIGHT[app->fontType];
				largeBuffer2->setLength(0);
				largeBuffer2->append("tris: ");
				largeBuffer2->append(app->tinyGL->countBackFace)->append('/');
				largeBuffer2->append(app->tinyGL->countDrawn);
				graphics->drawString(largeBuffer2, this->viewRect[0], n3, 0);
				n3 += Applet::FONT_HEIGHT[app->fontType];
			}
			largeBuffer2->setLength(0);
			largeBuffer2->append("OSTime: ");
			int v30 = 0;
			for (int i = 0; i < 8; i++) {
				v30 += app->osTime[i];
			}
			largeBuffer2->append(v30 / 8);
			graphics->drawString(largeBuffer2, this->viewRect[0], n3, 0);
			n3 += Applet::FONT_HEIGHT[app->fontType];
			largeBuffer2->setLength(0);
			largeBuffer2->append("Code: ");
			int v31 = 0;
			for (int i = 0; i < 8; i++) {
				v31 += app->codeTime[i];
			}
			largeBuffer2->append(v31 / 8);
			graphics->drawString(largeBuffer2, this->viewRect[0], n3, 0);
			largeBuffer2->dispose();
			n3 += Applet::FONT_HEIGHT[app->fontType];
		}

		if (this->showLocation) {
			Text* smallBuffer = app->localization->getSmallBuffer();
			smallBuffer->setLength(0);
			smallBuffer->append(this->viewX >> 6);
			smallBuffer->append(' ');
			smallBuffer->append(this->viewY >> 6);
			smallBuffer->append(' ');
			int angle = this->viewAngle & 0x3FF;
			if (angle == Enums::ANGLE_NORTH) {
				smallBuffer->append('N');
			}
			else if (angle == Enums::ANGLE_EAST) {
				smallBuffer->append('E');
			}
			else if (angle == Enums::ANGLE_SOUTH) {
				smallBuffer->append('S');
			}
			else if (angle == Enums::ANGLE_WEST) {
				smallBuffer->append('W');
			}
			else if (angle == Enums::ANGLE_NORTHEAST) {
				smallBuffer->append("NE");
			}
			else if (angle == Enums::ANGLE_NORTHWEST) {
				smallBuffer->append("NW");
			}
			else if (angle == Enums::ANGLE_SOUTHEAST) {
				smallBuffer->append("SE");
			}
			else if (angle == Enums::ANGLE_SOUTHWEST) {
				smallBuffer->append("SW");
			}
			graphics->drawString(smallBuffer, this->viewRect[0] + 2, n3 + 2, 0);


			char tmpBuffer[88];

			n3 += Applet::FONT_HEIGHT[app->fontType];
			sprintf(tmpBuffer, "X: %.4f", app->field_0x414);
			smallBuffer->setLength(0);
			smallBuffer->append(tmpBuffer);
			graphics->drawString(smallBuffer, this->viewRect[0] + 2, n3 + 2, 16);

			n3 += Applet::FONT_HEIGHT[app->fontType];
			sprintf(tmpBuffer, "Y: %.4f", app->field_0x418);
			smallBuffer->setLength(0);
			smallBuffer->append(tmpBuffer);
			graphics->drawString(smallBuffer, this->viewRect[0] + 2, n3 + 2, 16);

			n3 += Applet::FONT_HEIGHT[app->fontType];
			sprintf(tmpBuffer, "Z: %.4f", app->field_0x41c);
			smallBuffer->setLength(0);
			smallBuffer->append(tmpBuffer);
			graphics->drawString(smallBuffer, this->viewRect[0] + 2, n3 + 2, 16);

			smallBuffer->dispose();
		}
	}

	graphics->resetScreenSpace();
}

void Canvas::run() {
	Applet* app = CAppContainer::getInstance()->app;
	app->CalcAccelerometerAngles();

	int upTimeMs = app->upTimeMs;
	app->lastTime = app->time;
	app->time = upTimeMs;

	if (this->st_enabled != false) {
		this->st_count = this->st_count + 1;
		this->st_fields[0] = this->st_fields[0] + app->render->frameTime;
		this->st_fields[1] = this->st_fields[1] + app->render->bspTime;
		this->st_fields[2] = this->st_fields[2] + app->hud->drawTime;
		this->st_fields[4] = this->st_fields[4] + app->render->bltTime;
		this->st_fields[5] = (this->pauseTime - this->flushTime) + this->st_fields[5];
		this->st_fields[6] = (this->loopEnd - this->pauseTime) + this->st_fields[6];
		this->st_fields[8] = (app->time - app->lastTime) + this->st_fields[8];
		this->st_fields[3] = this->st_fields[3] + app->combat->renderTime;
		this->st_fields[9] = this->st_fields[9] + this->debugTime;
		this->st_fields[7] = (this->loopEnd - this->loopStart) + this->st_fields[7];
		upTimeMs = app->upTimeMs;
	}
	this->loopStart = upTimeMs;

	if (!app->game->pauseGameTime && this->state != Canvas::ST_MENU) {
		app->gameTime += app->time - app->lastTime;
	}

	if (this->vibrateTime && this->vibrateTime < app->time) {
		this->vibrateTime = 0;
	}

	if (this->state != Canvas::ST_DIALOG && this->state != Canvas::ST_MENU && app->game->isInputBlockedByScript()) {
		this->clearEvents(1);
	}

	this->runInputEvents();

	// [GEC]
	if (this->repaintFlags & Canvas::REPAINT_VIEW3D) { // REPAINT_VIEW3D
		if (!app->render->_gles->isInit) {
			app->render->Render3dScene();
			app->render->drawRGB(&app->canvas->graphics);
			app->tinyGL->applyClearColorBuffer();
		}
	}

	if ((this->state != Canvas::ST_MENU) || (app->menuSystem->menu != Menus::MENU_ENABLE_SOUNDS)) {
		app->game->numTraceEntities = 0;
		app->game->UpdatePlayerVars();
		app->game->gsprite_update(app->time);
		app->game->runScriptThreads(app->gameTime);
	}

	//printf("lastTime %d\n", app->lastTime);
	//printf("time %d\n", app->time);
	//printf("this->loopStart %d\n", this->loopStart);
	int time = app->upTimeMs;
	app->game->updateAutomap = false;
	//printf("this->state %d\n", this->state);

	if (this->state == Canvas::ST_PLAYING) {

		if (this->m_controlButton) {
			this->handleEvent(this->m_controlButton->buttonID);
		}

		app->game->updateAutomap = true;
		if (this->numHelpMessages == 0 && app->game->queueAdvanceTurn) {
			app->game->snapMonsters(true);
			app->game->advanceTurn();
		}

		this->playingState();

		if (this->kickingPhase == 4 && this->gridTime < app->gameTime) {
			app->canvas->kickingPhase = 0;
			app->game->executeStaticFunc(10);
		}

		if (this->state == Canvas::ST_PLAYING) {
			this->repaintFlags |= Canvas::REPAINT_HUD;
			app->hud->repaintFlags |= 11;
		}
	}
	else if (this->state == Canvas::ST_DRIVING) {
		this->repaintFlags |= Canvas::REPAINT_HUD;
		app->hud->repaintFlags |= 11;
		app->drivingGame->drivingState();
	}
	else if (this->state != Canvas::ST_CREDITS && this->state != Canvas::ST_TRAVELMAP && this->state != Canvas::ST_MIXING) {
		if (this->state != Canvas::ST_MINI_GAME) {
			if (this->state == Canvas::ST_COMBAT) {
				app->game->updateAutomap = true;
				this->combatState();
			}
			else if (this->state == Canvas::ST_INTRO_MOVIE) {
				if ((app->game->hasSeenIntro && this->numEvents != 0) || this->scrollingTextDone) {
					this->dialogBuffer->dispose();
					this->dialogBuffer = nullptr;
					app->game->hasSeenIntro = true;
					app->game->saveConfig();
					this->backToMain(false);
				}
			}
			else if (this->state == Canvas::ST_EPILOGUE) {
				if (this->scrollingTextDone) {
					this->disposeEpilogue();
				}
			}
			else if (this->state == Canvas::ST_INTRO) {
				if (this->storyPage >= this->storyTotalPages) {
					this->disposeIntro();
				}
			}
			else if (this->state == Canvas::ST_SAVING) {
				if ((this->saveType & Game::SAVE_WORLD) || (this->saveType & Game::SAVE_PLAYER)) {
					if (this->saveType & Game::SAVE_LOADMAP) {
						if (app->game->spawnParam != 0) {
							int n11 = 32 + ((app->game->spawnParam & 0x1F) << 6);
							int n12 = 32 + ((app->game->spawnParam >> 5 & 0x1F) << 6);
							app->game->saveState(this->lastMapID, this->loadMapID, n11, n12, (app->game->spawnParam >> 10 & 0xFF) << 7, 0, n11, n12, n11, n12, 36, 0, 0, this->saveType);
						}
						else {
							app->game->saveState(this->lastMapID, this->loadMapID, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, this->saveType);
						}
					}
					else if (this->saveType & Game::SAVE_ENDLEVEL) {
						int n13 = 32 + ((app->game->spawnParam & 0x1F) << 6);
						int n14 = 32 + ((app->game->spawnParam >> 5 & 0x1F) << 6);
						app->game->saveState(this->loadMapID, app->menuSystem->LEVEL_STATS_nextMap, n13, n14, (app->game->spawnParam >> 10 & 0xFF) << 7, 0, n13, n14, n13, n14, 36, 0, 0, this->saveType);
					}
					else {
						app->game->saveState(this->loadMapID, this->loadMapID, this->destX, this->destY, this->destAngle, this->viewPitch, this->prevX, this->prevY, this->saveX, this->saveY, this->saveZ, this->saveAngle, this->savePitch, this->saveType);
					}
					app->hud->addMessage(Strings::FILE_CODESTRINGS, CodeStrings::GAME_SAVED);
				}
				else {
					app->Error(48); // ERR_SAVESTATE
				}

				if (this->saveType & Game::SAVE_EXIT) {
					this->backToMain(false);
				}
				else if (this->saveType & Game::SAVE_QUIT) {
					app->shutdown();
				}
				else if (this->saveType & Game::SAVE_LOADMAP) {
					this->setState(Canvas::ST_TRAVELMAP);
				}
				else if (this->saveType & Game::SAVE_ENDLEVEL) {
					app->menuSystem->setMenu(Menus::MENU_LEVEL_STATS);
				}
				else if (this->saveType & Game::SAVE_ENDGAME) {
					app->menuSystem->setMenu(Menus::MENU_END_FINALQUIT);
				}
				else {
					if (this->saveType & Game::SAVE_RETURNTOGAME) {
						app->menuSystem->returnToGame();
					}
					this->setState(Canvas::ST_PLAYING);
				}
				this->saveType = Game::SAVE_NONE;
				this->clearEvents(2);
			}
			else if (this->state == Canvas::ST_LOADING) {
				if (this->loadType == Game::LOAD_NONE) {
					if (!this->loadMedia()) {
						this->flushGraphics();
						return;
					}
				}
				else if (this->loadType == Game::LOAD_CHICKENGAME) {
					bool enableHelp = app->player->enableHelp;
					app->player->enableHelp = false;
					if (!this->loadMedia()) {
						this->flushGraphics();
						return;
					}
					app->player->enableHelp = enableHelp;
					this->startKicking(true);
				}
				else {
					app->game->loadState(this->loadType);
					app->hud->addMessage(Strings::FILE_CODESTRINGS, CodeStrings::GAME_LOADED);
					this->loadType = Game::LOAD_NONE;
				}
			}
			else if (this->state == Canvas::ST_MENU) {
				this->menuState();
			}
			else if (this->state == Canvas::ST_DIALOG) {
				app->game->updateLerpSprites();
				this->updateView();
				this->repaintFlags |= (Canvas::REPAINT_HUD | Canvas::REPAINT_PARTICLES);
				app->hud->repaintFlags |= 15;
			}
			else if (this->state == Canvas::ST_AUTOMAP) {
				if (this->m_controlButton) {
					if (app->gameTime > this->m_controlButtonTime) {
						this->m_controlButtonTime = app->gameTime + 250;
						this->handleEvent(this->m_controlButton->buttonID);
					}
				}
				app->game->updateAutomap = true;
				this->automapState();
			}
			else if (this->state == Canvas::ST_DYING) {
				this->dyingState();
			}
			else if (this->state == Canvas::ST_CAMERA) {
				if (app->game->activeCameraKey != -1) {
					app->game->activeCamera->Update(app->game->activeCameraKey, app->gameTime - app->game->activeCameraTime);
				}
				app->game->updateLerpSprites();
				this->updateView();
				if (this->state == Canvas::ST_CAMERA && app->gameTime > app->game->cinUnpauseTime && this->softKeyRightID == -1) {
					this->clearLeftSoftKey();
					this->setRightSoftKey(Strings::FILE_CODESTRINGS, CodeStrings::SKIP);
				}
			}
			else if (this->state == Canvas::ST_LOGO) {
				this->logoState();
			}
			else if (this->state == Canvas::ST_BENCHMARK) {
				this->renderOnlyState();
			}
			else if (this->state != Canvas::ST_ERROR) {
				app->Error(51); // ERR_STATE
			}
		}
	}

	if (this->state == Canvas::ST_SAVING || this->state == Canvas::ST_LOADING) {
		this->repaintFlags &= ~Canvas::REPAINT_VIEW3D;
	}
	this->st_fields[12] = app->upTimeMs - time;

	this->flushTime = app->upTimeMs;
	this->graphics.resetScreenSpace();
	this->backPaint(&this->graphics);
	if (this->keyPressedTime != 0) {
		this->lastKeyPressedTime = app->upTimeMs - this->keyPressedTime;
		this->keyPressedTime = 0;
	}
	this->pauseTime = app->upTimeMs;
	this->loopEnd = app->upTimeMs;
	this->stateChanged = false;

	app->sound->updateFades();
	if (this->sysSoundDelayTime > 0 && this->sysSoundDelayTime < app->upTimeMs - this->sysSoundTime) {
		app->sound->playSound((app->nextByte() & 0xF) + 1000, 0, 3, 0);
		this->sysSoundTime = app->upTimeMs;
	}
}

void Canvas::clearEvents(int ignoreFrameInput) {
	this->numEvents = 0;
	this->keyDown = false;
	this->keyDownCausedMove = false;
	this->ignoreFrameInput = ignoreFrameInput;
}

void Canvas::loadRuntimeData() {
	Applet* app = CAppContainer::getInstance()->app;

	app->loadRuntimeImages();
	//app->checkPeakMemory("after loadRuntimeData");
}

void Canvas::freeRuntimeData() {
	Applet* app = CAppContainer::getInstance()->app;
	app->freeStaticImages();
}

void Canvas::startShake(int i, int i2, int i3) {
	Applet* app = CAppContainer::getInstance()->app;
	SDLGL* sdlGL = CAppContainer::getInstance()->sdlGL;

	if (app->game->skippingCinematic) {
		return;
	}

	if (i2 != 0) {
		this->shakeTime = app->time + i;
		this->shakeIntensity = 2 * i2;
		this->staleTime += 1;
	}

	if (i3 != 0 && this->vibrateEnabled) {
		controllerVibrate(i3); // [GEC]
		this->vibrateTime = i3 + app->upTimeMs;
	}
}

void Canvas::setState(int state) {
	Applet* app = CAppContainer::getInstance()->app;

	this->stateChanged = true;
	for (int i = 0; i < 9; ++i) {
		this->stateVars[i] = 0;
	}
	this->m_controlButtonIsTouched = false;
	this->m_controlButton = 0;

	app->setFont(0);

	if (this->state == Canvas::ST_AUTOMAP) {
		app->player->unpause(app->time - this->automapTime);
	}
	else if (this->state == Canvas::ST_MENU) {
		app->player->unpause(app->time - app->menuSystem->startTime);
		app->menuSystem->clearStack();
	}
	else if (this->state == Canvas::ST_CAMERA) {
		app->render->disableRenderActivate = false;
		app->game->skippingCinematic = false;
	}
	else if (this->state == Canvas::ST_COMBAT && state != Canvas::ST_COMBAT && app->combat->stage != Canvas::ST_MENU) {
		app->combat->cleanUpAttack();
	}
	else if (this->state == Canvas::ST_CREDITS) {
		this->dialogBuffer->dispose();
		this->dialogBuffer = nullptr;
	}
	else if (this->state == Canvas::ST_MIXING) {
		this->cocktailName->dispose();
		this->mixingInstructions->dispose();

		this->imgMixingBG->~Image();
		this->imgMixingBG = nullptr;
		this->imgMixingHeadingPlate->~Image();
		this->imgMixingHeadingPlate = nullptr;
		this->imgMixingTTRedNormal->~Image();
		this->imgMixingTTRedNormal = nullptr;
		this->imgMixingTTRedSelected->~Image();
		this->imgMixingTTRedSelected = nullptr;
		this->imgMixingTTRedEmpty->~Image();
		this->imgMixingTTRedEmpty = nullptr;
		this->imgMixingTTGreenNormal->~Image();
		this->imgMixingTTGreenNormal = nullptr;
		this->imgMixingTTGreenSelected->~Image();
		this->imgMixingTTGreenSelected = nullptr;
		this->imgMixingTTGreenEmpty->~Image();
		this->imgMixingTTGreenEmpty = nullptr;
		this->imgMixingTTBludeNormal->~Image();
		this->imgMixingTTBludeNormal = nullptr;
		this->imgMixingTTBludeSelected->~Image();
		this->imgMixingTTBludeSelected = nullptr;
		this->imgMixingTTBludeEmpty->~Image();
		this->imgMixingTTBludeEmpty = nullptr;
		this->imgMixingSyringe->~Image();
		this->imgMixingSyringe = nullptr;
		this->imgMixingSyringeSelected->~Image();
		this->imgMixingSyringeSelected = nullptr;
		this->imgMixingSyringeRed->~Image();
		this->imgMixingSyringeRed = nullptr;
		this->imgMixingSyringeGreen->~Image();
		this->imgMixingSyringeGreen = nullptr;
		this->imgMixingSyringeBlue->~Image();
		this->imgMixingSyringeBlue = nullptr;
		this->imgMixingNumbersPlate->~Image();
		this->imgMixingNumbersPlate = nullptr;
		this->imgMixingSyringeNumbersPlate->~Image();
		this->imgMixingSyringeNumbersPlate = nullptr;

	}
	else if (this->state == Canvas::ST_DRIVING) {
		app->player->selectWeapon(3);
	}

	this->oldState = this->state;
	this->state = state;

	//printf("Canvas::setState %d\n", state);
	if (state == Canvas::ST_COMBAT) {
		app->hud->repaintFlags = 15;
		this->repaintFlags |= Canvas::REPAINT_HUD;
		this->clearSoftKeys();
		this->combatDone = false;
	}
	else if (state == Canvas::ST_TRAVELMAP) {
		this->initTravelMap();
	}
	else if (state == Canvas::ST_PLAYING) {
		app->hud->repaintFlags = 15;
		this->repaintFlags |= Canvas::REPAINT_HUD;
		app->game->lastTurnTime = app->time;
		if (app->game->monstersTurn == 0 || this->oldState == Canvas::ST_CAMERA) {
			this->drawPlayingSoftKeys();
		}
		app->player->checkForCloseSoundObjects();
		if (this->oldState == Canvas::ST_COMBAT && app->combat->curTarget != nullptr && app->combat->curTarget->def->eType == 3) {
			app->game->executeStaticFunc(7);
		}
		this->updateFacingEntity = true;
		if (this->oldState != Canvas::ST_COMBAT && this->oldState != Canvas::ST_DIALOG) {
			this->invalidateRect();
		}
	}
	else if (state == Canvas::ST_DRIVING) {
		app->hud->repaintFlags = 15;
		this->repaintFlags |= Canvas::REPAINT_HUD;
		app->drivingGame->init();
	}
	else if (state == Canvas::ST_DIALOG) {
		if (app->canvas->isZoomedIn) {
			this->isZoomedIn = 0;
			app->StopAccelerometer();
			this->destAngle = this->viewAngle = (this->viewAngle + this->zoomAngle + 127) & 0xFFFFFF00;
			app->tinyGL->resetViewPort();
			this->drawPlayingSoftKeys();
		}
		if (app->game->isCameraActive()) {
			app->game->activeCameraTime = app->gameTime - app->game->activeCameraTime;
		}
		app->hud->repaintFlags = 15;
		this->repaintFlags |= Canvas::REPAINT_HUD;
		app->tinyGL->resetViewPort();

		this->clearSoftKeys();
		this->clearEvents(1);
	}
	else if (state == Canvas::ST_DYING) {
		app->hud->repaintFlags = 15;
		this->repaintFlags |= Canvas::REPAINT_HUD;
		if (this->isZoomedIn) {
			this->isZoomedIn = false;
			app->StopAccelerometer();
			this->viewAngle += this->zoomAngle;
			int n3 = 255;
			this->destAngle = (this->viewAngle = (this->viewAngle + (n3 >> 1) & ~n3));
			app->tinyGL->resetViewPort();
			this->drawPlayingSoftKeys();
		}
		this->clearSoftKeys();
		this->deathTime = app->time;
		this->destPitch = 64;
		this->numHelpMessages = 0;
	}
	else if (state == Canvas::ST_EPILOGUE) {
		this->clearSoftKeys();
		app->setFont(2);
		this->loadEpilogueText();
		this->stateVars[0] = 1;
	}
	else if (state == Canvas::ST_CREDITS) {
		app->localization->loadText(Strings::FILE_FILESTRINGS);
		app->setFont(2);
		this->initScrollingText(Strings::FILE_FILESTRINGS, FileStrings::CREDITS, false, Applet::FONT_HEIGHT[app->fontType], 5, 500);
		app->localization->unloadText(Strings::FILE_FILESTRINGS);
	}
	else if (state == Canvas::ST_INTRO) {
		this->clearSoftKeys();
		app->setFont(2);
		this->loadPrologueText();
		this->stateVars[0] = 1;
	}
	else if (state == Canvas::ST_INTRO_MOVIE) {
		app->setFont(2);
		this->repaintFlags |= 4u;
		app->game->skipMovie = false;
	}
	else if (state == Canvas::ST_LOADING || state == Canvas::ST_SAVING) {
		this->repaintFlags &= ~Canvas::REPAINT_HUD;
		this->pacifierX = this->SCR_CX - 116;
		this->updateLoadingBar(false);
	}
	else if (state == Canvas::ST_AUTOMAP) {
		this->automapDrawn = false;
		this->automapTime = app->time;
	}
	else if (state == Canvas::ST_MENU) {
		app->setFont(2);
		app->menuSystem->startTime = app->time;
		if (this->oldState == Canvas::ST_PLAYING) {
			app->menuSystem->soundClick();
		}
		else if (this->oldState == Canvas::ST_MIXING) {
			app->menuSystem->goBackToStation = true;
		}

		if (this->oldState != Canvas::ST_MENU) {
			this->clearEvents(1);
		}
	}
	else if (state == Canvas::ST_CAMERA) {
		app->hud->msgCount = 0;
		app->hud->subTitleID = -1;
		app->hud->cinTitleID = -1;
		app->render->disableRenderActivate = true;
		this->repaintFlags |= Canvas::REPAINT_HUD; // j2me 0x11;
		app->hud->repaintFlags = 16;
		this->clearSoftKeys();
#ifdef WOLFENRPG_PSP
		// The iPhone presentation shrinks cinematics and paints opaque bars
		// around them.  On PSP that produces the large blocks seen during the
		// opening level, so keep the camera in the normal gameplay viewport.
		app->tinyGL->setViewport(this->viewRect[0], this->viewRect[1], this->viewRect[2], this->viewRect[3]);
#else
		app->tinyGL->setViewport(this->cinRect[0], this->cinRect[1], this->cinRect[2], this->cinRect[3]);
#endif
	}
	else if (state == Canvas::ST_MIXING) {
		app->beginImageLoading();
		this->imgMixingBG = app->loadImage("mixing_bgd.bmp", true);
		this->imgMixingHeadingPlate = app->loadImage("MS_heading_plate.bmp", true);
		this->imgMixingTTRedNormal = app->loadImage("MS_TTred_normal.bmp", true);
		this->imgMixingTTRedSelected = app->loadImage("MS_TTred_selected.bmp", true);
		this->imgMixingTTRedEmpty = app->loadImage("MS_TTred_empty.bmp", true);
		this->imgMixingTTGreenNormal = app->loadImage("MS_TTgreen_normal.bmp", true);
		this->imgMixingTTGreenSelected = app->loadImage("MS_TTgreen_selected.bmp", true);
		this->imgMixingTTGreenEmpty = app->loadImage("MS_TTgreen_empty.bmp", true);
		this->imgMixingTTBludeNormal = app->loadImage("MS_TTblue_normal.bmp", true);
		this->imgMixingTTBludeSelected = app->loadImage("MS_TTblue_selected.bmp", true);
		this->imgMixingTTBludeEmpty = app->loadImage("MS_TTblue_empty.bmp", true);
		this->imgMixingSyringe = app->loadImage("MS_syringe.bmp", true);
		this->imgMixingSyringeSelected = app->loadImage("MS_syringe_selected.bmp", true);
		this->imgMixingSyringeRed = app->loadImage("MS_syringe_red.bmp", true);
		this->imgMixingSyringeGreen = app->loadImage("MS_syringe_green.bmp", true);
		this->imgMixingSyringeBlue = app->loadImage("MS_syringe_blue.bmp", true);
		this->imgMixingNumbersPlate = app->loadImage("MS_numbers_plate.bmp", true);
		this->imgMixingSyringeNumbersPlate = app->loadImage("MS_syringe_numbers_plate.bmp", true);

		app->finishImageLoading();
		this->setSoftKeys(Strings::FILE_MENUSTRINGS, MenuStrings::BACK_ITEM, Strings::FILE_MENUSTRINGS, MenuStrings::RECIPE_ITEM);
		this->cocktailName = app->localization->getSmallBuffer();
		this->mixingInstructions = app->localization->getSmallBuffer();
		this->stateVars[1] = -4737097;
		this->stateVars[2] = -4737097;
		this->stateVars[3] = -4737097;
		this->stateVars[6] = app->game->mixingStations[this->curStation + 1];
		this->stateVars[7] = app->game->mixingStations[this->curStation + 2];
		this->stateVars[8] = app->game->mixingStations[this->curStation + 3];
		this->stateVars[4] = 3;

		this->nextMixingIngredient(1);
		this->mixingMsg = false;
		this->msgSeen = false;
		this->updateMixingText();
	}
}

void Canvas::setAnimFrames(int animFrames) {
	this->animFrames = animFrames;
	this->animPos = (64 + this->animFrames - 1) / this->animFrames;
	this->animAngle = (256 + this->animFrames - 1) / this->animFrames;
}

void Canvas::checkFacingEntity() {
	Applet* app = CAppContainer::getInstance()->app;
	if (!this->updateFacingEntity) {
		return;
	}
	int destX = this->destX;
	int destY = this->destY;
	int destZ = this->destZ;
	int n = 21741;
	int *view = app->tinyGL->view;

	int n2 = destZ + (-view[10] * 28 >> 14);
	int n3 = destZ + (6 * -view[10] >> 8);
	if (app->render->chatZoom) {
		n3 = (n2 = -1);
	}

	app->game->trace(destX + (-view[2] * 28 >> 14), destY + (-view[6] * 28 >> 14), n2, destX + (6 * -view[2] >> 8), destY + (6 * -view[6] >> 8), n3, nullptr, n, 2);
	Entity* traceEntity = app->game->traceEntity;
	if (traceEntity != nullptr && (traceEntity->def->eType == Enums::ET_ITEM || 
			traceEntity->def->eType == Enums::ET_MONSTERBLOCK_ITEM || 
			traceEntity->def->eType == Enums::ET_SPRITEWALL || 
			traceEntity->def->eType == Enums::ET_ATTACK_INTERACTIVE || 
			traceEntity->def->eType == Enums::ET_DECOR_NOCLIP)) {
		int i = 0;
		while (i < app->game->numTraceEntities) {
			Entity* entity = app->game->traceEntities[i];
			short linkIndex = entity->linkIndex;
			if (entity->def->eType == Enums::ET_MONSTER) {
				if (traceEntity->def->eType != Enums::ET_SPRITEWALL) {
					traceEntity = entity;
					break;
				}
				break;
			}
			else {
				if (entity->def->eType == Enums::ET_DOOR || entity->def->eType == Enums::ET_PLAYERCLIP) {
					break;
				}
				if (entity->def->eType == Enums::ET_WORLD) {
					break;
				}
				if (entity->def->eType == Enums::ET_SPRITEWALL && (app->render->mapFlags[linkIndex] & Canvas::BIT_AM_SECRET)) {
					break;
				}
				if (entity->def->eType == Enums::ET_DECOR) {
					if (traceEntity->def->eType == Enums::ET_SPRITEWALL) {
						traceEntity = entity;
						break;
					}
					break;
				}
				else {
					if (entity->def->eType == Enums::ET_DECOR_NOCLIP) {
						if (traceEntity->def->eSubType != Enums::DECOR_DYNAMITE) {
							traceEntity = entity;
							break;
						}
					}
					else if (entity->def->eType == Enums::ET_ATTACK_INTERACTIVE && (
						entity->def->eSubType == Enums::INTERACT_BARRICADE || 
						entity->def->eSubType == Enums::INTERACT_BARREL || 
						entity->def->eSubType == Enums::INTERACT_STATUE || 
						entity->def->eSubType == Enums::INTERACT_CRATE ||
						entity->def->eSubType == Enums::INTERACT_PICKUP) && traceEntity != nullptr && traceEntity->def->eType != Enums::ET_ITEM) {
						traceEntity = entity;
						break;
					}
					++i;
				}
			}
		}
	}
	app->player->facingEntity = traceEntity;
	if (app->player->facingEntity != nullptr) {
		Entity* facingEntity = app->player->facingEntity;
		int dist = facingEntity->distFrom(this->viewX, this->viewY);
		bool b = facingEntity->def->eType == Enums::ET_MONSTER && (facingEntity->monster->flags & Enums::MFLAG_NPC_MONSTER) != 0x0;

		if (facingEntity->def->eType != Enums::ET_MONSTER && dist > app->combat->tileDistances[2]) {
			app->player->facingEntity = nullptr;
		}
		else if ((facingEntity->def->eType == Enums::ET_NPC || b) && dist <= app->combat->tileDistances[0]) {
			app->player->showHelp(CodeStrings::GENHELP_NPC, false);
		}
		else if (facingEntity->def->eType == Enums::ET_DECOR_NOCLIP && facingEntity->def->tileIndex == Enums::TILENUM_AIR_BUBBLES && dist <= app->combat->tileDistances[1]) {
			app->player->showHelp(CodeStrings::GENHELP_AIR_BUBBLES, false);
		}
		else if (dist <= app->combat->tileDistances[0]) {
			if (facingEntity->def->eType == Enums::ET_ATTACK_INTERACTIVE) {
				if (facingEntity->def->eSubType == Enums::INTERACT_BARRICADE) {
					app->player->showHelp(CodeStrings::GENHELP_BARRICADE, false);
				}
				else if (facingEntity->def->eSubType == Enums::INTERACT_CRATE) {
					app->player->showHelp(CodeStrings::GENHELP_CRATE, false);
				}
				else if (facingEntity->def->eSubType == Enums::INTERACT_BARREL) {
					app->player->showHelp(CodeStrings::GENHELP_BARRELS, false);
				}
				else if (facingEntity->def->eSubType == Enums::INTERACT_PICKUP) {
					app->player->showHelp(CodeStrings::GENHELP_PICKUP, false);
				}
				else if (facingEntity->def->eSubType != Enums::INTERACT_STATUE) {
					app->player->showHelp(CodeStrings::GENHELP_RUBBLE, false);
				}
			}
			else if (facingEntity->def->eType == Enums::ET_DECOR) {
				if (facingEntity->def->tileIndex == Enums::TILENUM_COCKTAILMIXER) {
					app->player->showHelp(CodeStrings::GENHELP_MIXING_STATION, false);
				}
			}
			else if (facingEntity->def->eType == Enums::ET_DOOR) {
				if (facingEntity->def->eSubType == Enums::DOOR_LOCKED) {
					app->player->showHelp(CodeStrings::GENHELP_LOCKEDOBJECT, false);
				}
				app->player->showHelp(CodeStrings::GENHELP_DOORS, false);
			}
			else if (facingEntity->def->eType == Enums::ET_ITEM && facingEntity->def->eSubType == Enums::IT_FOOD) {
				app->player->showHelp(CodeStrings::GENHELP_FOOD, false);
			}
		}
	}
	Entity* traceEntity2 = app->game->traceEntity;
	int n4 = 4141;
	if (traceEntity2 != nullptr && (1 << traceEntity2->def->eType & n4) == 0x0) {
		for (int j = 1; j < app->game->numTraceEntities; ++j) {
			Entity* entity2 = app->game->traceEntities[j];
			if ((1 << entity2->def->eType & n4) != 0x0) {
				traceEntity2 = entity2;
				break;
			}
		}
	}
	if (traceEntity2 != nullptr) {
		int dist2 = traceEntity2->distFrom(this->viewX, this->viewY);
		bool b2 = traceEntity2->def->eType == Enums::ET_MONSTER && (traceEntity2->monster->flags & Enums::MFLAG_NPC_MONSTER) != 0x0;
		if (dist2 <= app->combat->tileDistances[0] && traceEntity2->def->eType == Enums::ET_WORLD && app->combat->weaponDown) {
			app->combat->shiftWeapon(true);
		}
		else if ((this->state == Canvas::ST_PLAYING || this->state == Canvas::ST_DIALOG) && (!Entity::CheckWeaponMask(app->player->ce->weapon, Enums::WP_MELEEMASK) || dist2 <= app->combat->tileDistances[0])) {
			if (traceEntity2->def->eType == Enums::ET_NPC && b2) {
				app->combat->shiftWeapon(true);
			}
			else if (app->combat->weaponDown) {
				app->combat->shiftWeapon(false);
			}
		}
		else if (this->state == Canvas::ST_DIALOG && this->oldState != Canvas::ST_DRIVING) {
			app->combat->shiftWeapon(true);
		}
		else {
			app->combat->shiftWeapon(false);
		}
	}
	else {
		app->combat->shiftWeapon(false);
	}
	this->updateFacingEntity = false;
}

void Canvas::finishMovement() {
	Applet* app = CAppContainer::getInstance()->app;

	if (this->gotoThread != nullptr && this->viewAngle == this->destAngle) {
		this->gotoThread->run();
		this->gotoThread = nullptr;
	}
	app->game->executeTile(this->destX >> 6, this->destY >> 6, this->flagForFacingDir(8), true);
	app->game->executeTile(this->destX >> 6, this->destY >> 6, app->game->eventFlags[1], true);
	app->game->touchTile(this->destX, this->destY, true);
	if (this->knockbackDist > 0) {
		--this->knockbackDist;
		this->destZ += 12;
		if (this->knockbackDist == 0) {
			this->destZ = 36 + app->render->getHeight(this->destX, this->destY);
		}
	}
	else if (this->gotoThread == nullptr && this->state == Canvas::ST_PLAYING && app->game->monstersTurn == 0) {
		if (this->state != Canvas::ST_AUTOMAP) {
			this->updateFacingEntity = true;
		}
		this->uncoverAutomap();
		app->game->advanceTurn();
	}
	else if (this->state == Canvas::ST_AUTOMAP) {
		this->uncoverAutomap();
		app->game->advanceTurn();
		if (app->game->animatingEffects != 0) {
			this->setState(Canvas::ST_PLAYING);
		}
		else {
			app->game->snapMonsters(true);
			app->game->snapLerpSprites(-1);
		}
	}
}

int Canvas::flagForWeapon(int i) {
	Applet* app = CAppContainer::getInstance()->app;
	if (Entity::CheckWeaponMask(i, Enums::WP_MELEEMASK)) {
		return 4096;
	}
	if (Entity::CheckWeaponMask(i, Enums::WP_EXPLOSIONMASK)) {
		return 16384;
	}
	return 8192;
}

int Canvas::flagForFacingDir(int i) {
	int destAngle = this->destAngle;
	if (i == 4) {
		destAngle += 512;
	}
	if (i == 8 || i == 4) {
		return i | 1 << ((destAngle & 0x3FF) >> 7) + 4;
	}
	return 0;
}

void Canvas::startRotation(bool b) {
	Applet* app = CAppContainer::getInstance()->app;

	int8_t b2 = Canvas::viewStepValues[((this->destAngle & 0x3FF) >> 7 << 1) + 0];
	int8_t b3 = Canvas::viewStepValues[((this->destAngle & 0x3FF) >> 7 << 1) + 1];
	int n = 384;
	app->game->trace(this->destX, this->destY, this->destX + (b2 * n >> 6), this->destY + (b3 * n >> 6), nullptr, 4133, 2);
	Entity* traceEntity = app->game->traceEntity;
	int n2 = app->game->traceFracs[0] * n >> 14;
	int n3;
	int n4;
	if (traceEntity != nullptr && (traceEntity->def->eType == Enums::ET_WORLD || traceEntity->def->eType == Enums::ET_SPRITEWALL) && n2 <= 36) {
		n3 = this->destZ;
		n4 = (b ? 0 : 1);
	}
	else if (traceEntity != nullptr && traceEntity->def->eType == Enums::ET_MONSTER) {
		int* calcPosition = traceEntity->calcPosition();
		n3 = app->render->getHeight(calcPosition[0], calcPosition[1]) + 36;
		n4 = 1;
	}
	else {
		n2 = 64;
		n3 = app->render->getHeight(this->destX + b2, this->destY + b3) + 36;
		n4 = (b ? 0 : 1);
	}
	if (n4 == 0) {
		return;
	}

	if (n2 == 0) {
		this->destPitch = 0;
	}
	else {
		this->destPitch = ((n3 - this->destZ) << 7) / n2;
	}
	if (this->destPitch < -64) {
		this->destPitch = -64;
	}
	else if (this->destPitch > 64) {
		this->destPitch = 64;
	}
	//this->pitchStep = std::abs((this->destPitch - this->viewPitch) / this->animFrames);
	this->pitchStep = (std::abs(this->destPitch - this->viewPitch) + this->animFrames - 1) / this->animFrames;

	//(256 + this->animFrames - 1) / this->animFrames;
}

void Canvas::finishRotation(bool b) {
	Applet* app = CAppContainer::getInstance()->app;

	this->viewSin = app->render->sinTable[this->destAngle & 0x3FF];
	this->viewCos = app->render->sinTable[this->destAngle + 256 & 0x3FF];
	this->viewStepX = Canvas::viewStepValues[(((this->destAngle & 0x3FF) >> 7) << 1) + 0];
	this->viewStepY = Canvas::viewStepValues[(((this->destAngle & 0x3FF) >> 7) << 1) + 1];
	int n = this->destAngle - 256 & 0x3FF;
	this->viewRightStepX = Canvas::viewStepValues[((n >> 7) << 1) + 0];
	this->viewRightStepY = Canvas::viewStepValues[((n >> 7) << 1) + 1];
	if (b && app->hud->msgCount > 0 && (app->hud->messageFlags[0] & 0x2) != 0x0) {
		app->hud->msgTime = 0;
	}
	if (this->gotoThread != nullptr && this->viewX == this->destX && this->viewY == this->destY) {
		ScriptThread* gotoThread = this->gotoThread;
		this->gotoThread = nullptr;
		gotoThread->run();
	}
	if (this->state == Canvas::ST_COMBAT) {
		this->updateFacingEntity = true;
	}
	else {
		app->game->executeTile(this->destX >> 6, this->destY >> 6, this->flagForFacingDir(8), true);
		this->updateFacingEntity = true;
	}
}

/*int KEY_ARROWUP = -1;
int KEY_ARROWDOWN = -2;
int KEY_ARROWLEFT = -3;
int KEY_ARROWRIGHT = -4;
int KEY_OK = -5;
int KEY_CLR = -8;
int KEY_LEFTSOFT = -6;
int KEY_RIGHTSOFT = -7;
int KEY_BACK = -8;*/

#include "Input.h"

#define MOVEFORWARD	1
#define MOVEBACK	2
#define TURNLEFT	3
#define TURNRIGHT	4
#define MENUOPEN	5
#define SELECT		6
#define AUTOMAP		7
//8
#define MOVELEFT		9
#define MOVERIGHT		10
#define PREVWEAPON		11
#define NEXTWEAPON		12
//13
#define PASSTURN		14
//15
#define MENU_UP			16
#define MENU_DOWN		17
#define MENU_PAGE_UP	18
#define MENU_PAGE_DOWN	19
#define MENU_SELECT		20
#define MENU_OPEN		21

#define NUM_CODES 36
int keys_codeActions[NUM_CODES] = {
	AVK_CLR,		Enums::ACTION_BACK,
	AVK_SOFT2,		Enums::ACTION_AUTOMAP,
	AVK_SOFT1,		Enums::ACTION_MENU,
	// New items Only Port
	AVK_STAR,		Enums::ACTION_PREVWEAPON,
	AVK_POUND,		Enums::ACTION_AUTOMAP,
	AVK_NEXTWEAPON, Enums::ACTION_NEXTWEAPON,
	AVK_PREVWEAPON, Enums::ACTION_PREVWEAPON,
	AVK_AUTOMAP,	Enums::ACTION_AUTOMAP,
	AVK_UP,			Enums::ACTION_UP,
	AVK_DOWN,		Enums::ACTION_DOWN,
	AVK_LEFT,		Enums::ACTION_LEFT,
	AVK_RIGHT,		Enums::ACTION_RIGHT,
	AVK_MOVELEFT,	Enums::ACTION_STRAFELEFT,
	AVK_MOVERIGHT,	Enums::ACTION_STRAFERIGHT,
	AVK_SELECT,		Enums::ACTION_FIRE,
	AVK_MENUOPEN,	Enums::ACTION_MENU,
	AVK_PASSTURN,	Enums::ACTION_PASSTURN,
};

int Canvas::getKeyAction(int i) {
	Applet* app = CAppContainer::getInstance()->app;
	int iVar1;

	//printf("getKeyAction i %d\n", i);

	if (this->state == Canvas::ST_MENU) {
		if (i & AVK_MENU_UP) {
			return Enums::ACTION_UP;
		}
		if (i & AVK_MENU_DOWN) {
			return Enums::ACTION_DOWN;
		}
		if (i & AVK_MENU_PAGE_UP) {
			return Enums::ACTION_LEFT;
		}
		if (i & AVK_MENU_PAGE_DOWN) {
			return Enums::ACTION_RIGHT;
		}
		if (i & AVK_MENU_SELECT) {
			return Enums::ACTION_FIRE;
		}
		if (i & AVK_ITEMS_INFO) {
			return Enums::ACTION_MENU_ITEM_INFO;
		}
	}

	if (i & AVK_MENU_OPEN) {
		return Enums::ACTION_MENU;
	}

	if (i & AVK_ITEMS_INFO) {
		return Enums::ACTION_ITEMS;
	}

	if (i & AVK_SYRINGES) {
		return Enums::ACTION_ITEMS_SYRINGES;
	}

	if (i & AVK_JOURNAL) {
		return Enums::ACTION_QUESTLOG;
	}

	i &= ~(AVK_MENU_UP | AVK_MENU_DOWN | AVK_MENU_PAGE_UP | AVK_MENU_PAGE_DOWN | AVK_MENU_SELECT | AVK_MENU_OPEN | AVK_ITEMS_INFO | AVK_SYRINGES | AVK_JOURNAL);

	for (int j = 0; j < (NUM_CODES / 2); j++)
	{
		if (keys_codeActions[(j * 2) + 0] == i) {
			//printf("rtn %d\n", keys_codeActions[(j * 2) + 1]);
			return keys_codeActions[(j * 2) + 1];
		}
	}

	if (i - 1U < 10) { // KEY_1 to KEY_9 ... KEY_0
		return this->keys_numeric[i - 1U];
	}

	if (i == 12) { // KEY_STAR
		return Enums::ACTION_PREVWEAPON;
	}
	if (i == 11) { // KEY_POUND
		return Enums::ACTION_AUTOMAP;
	}
	if (i == 14) { // KEY_ARROWLEFT
		return Enums::ACTION_LEFT;
	}
	if (i == 15) { // KEY_ARROWRIGHT
		return Enums::ACTION_RIGHT;
	}
	if (i == 16) { // KEY_ARROWUP
		return Enums::ACTION_UP;
	}
	if (i == 17) { // KEY_ARROWDOWN
		return Enums::ACTION_DOWN;
	}
	if (i == 13) { // KEY_OK
		return Enums::ACTION_FIRE;
	}

	iVar1 = (i ^ i >> 0x1f) - (i >> 0x1f);
	if (iVar1 == 19) { // KEY_LEFTSOFT
		return Enums::ACTION_MENU;
	}
	if (iVar1 == 20) { // KEY_RIGHTSOFT
		return Enums::ACTION_AUTOMAP; // ACTION_AUTOMAP
	}
	if (iVar1 == 18) { // KEY_CLR, KEY_BACK
		return Enums::ACTION_BACK;
	}

	return Enums::ACTION_NONE;
}

void Canvas::attemptMove(int n, int n2) {
	Applet* app = CAppContainer::getInstance()->app;

	if (this->renderOnly) {
		this->destX = n;
		this->destY = n2;
		return;
	}

	if (this->isChickenKicking && this->kickingPhase != 0) {
		return;
	}

	int n3 = app->player->noclip ? 0 : 13501;
	app->game->eventFlagsForMovement(this->viewX, this->viewY, n, n2);
	this->abortMove = false;
	app->game->executeTile(this->viewX >> 6, this->viewY >> 6, app->game->eventFlags[0], true);
	if (!this->abortMove) {
		app->game->trace(this->viewX, this->viewY, n, n2, app->player->getPlayerEnt(), n3, 16);
		if (app->game->traceEntity == nullptr) {
			if (app->hud->msgCount > 0 && (app->hud->messageFlags[0] & 0x2) != 0x0) {
				app->hud->msgTime = 0;
			}
			if (app->game->isUnderWater()) {
				app->sound->playSound(1104, 0, 3, 0);
			}
			this->automapDrawn = false;
			this->destX = n;
			this->destY = n2;
			this->destZ = 36 + app->render->getHeight(this->destX, this->destY);
			this->zStep = (std::abs(this->destZ - this->viewZ) + this->animFrames - 1) / this->animFrames;
			this->prevX = this->viewX;
			this->prevY = this->viewY;
			this->startRotation(false);
			app->player->relink();
			app->player->checkForCloseSoundObjects();
		}
		else if (this->knockbackDist == 0 && this->state == Canvas::ST_AUTOMAP) {
			app->game->advanceTurn();
		}
	}
	else if (this->knockbackDist != 0) {
		this->knockbackDist = 0;
	}
}

void Canvas::keyPressed(int event) {
	Applet* app = CAppContainer::getInstance()->app;
	if (this->numEvents == Canvas::MAX_EVENTS) {
		return;
	}
	this->events[this->numEvents++] = event;
	this->keyPressedTime = app->upTimeMs;
}

void Canvas::loadState(int loadType, short n, short n2) {
	Applet* app = CAppContainer::getInstance()->app;
	this->loadType = loadType;
	app->game->saveConfig();
	this->setLoadingBarText(n, n2);
	this->setState(Canvas::ST_LOADING);
}

void Canvas::saveState(int saveType, short n, short n2) {
	this->saveType = saveType;
	this->setLoadingBarText(n, n2);
	this->setState(Canvas::ST_SAVING);
}

void Canvas::loadMap(int loadMapID, bool b, bool tm_NewGame) {
	Applet* app = CAppContainer::getInstance()->app;

	this->ambientSoundsTime = 0;
	this->lastMapID = this->loadMapID;
	this->loadMapID = loadMapID;
	app->sound->soundStop();
	if (!b && app->game->activeLoadType == Game::LOAD_NONE && this->lastMapID >= 1 && this->lastMapID <= 10) {
		this->saveState((Game::SAVE_PLAYER | Game::SAVE_WORLD | Game::SAVE_LOADMAP | Game::SAVE_BRIEF), Strings::FILE_MENUSTRINGS, MenuStrings::SAVING_GAME_LABEL);
	}
	else if (this->loadType == Game::LOAD_CHICKENGAME) {
		setLoadingBarText(Strings::FILE_CODESTRINGS, CodeStrings::CHICKEN_NAME);
		setState(Canvas::ST_LOADING);
	}
	else {
		this->setLoadingBarText(Strings::FILE_MENUSTRINGS, app->game->levelNames[this->loadMapID - 1]);
		this->setState(Canvas::ST_TRAVELMAP);
	}
}

void Canvas::loadPrologueText() {
	Applet* app = CAppContainer::getInstance()->app;
	Text* text;

	app->beginImageLoading();
	if (this->imgTravelBG == nullptr) {
		this->imgTravelBG = app->loadImage("Story_Background.bmp", true);
	}
	this->initScrollingText(Strings::FILE_CODESTRINGS, CodeStrings::BRIEFING_TXT, true, (Applet::FONT_HEIGHT[app->fontType] * 3) / 2, 0, 1000);
	app->finishImageLoading();

	this->storyPage = 0;
	int spacingLines = this->scrollingTextSpacing * this->scrollingTextLines;
	int spacingHeight = this->scrollingTextSpacingHeight;
	this->storyTotalPages = spacingLines / spacingHeight;
	if (spacingLines % spacingHeight) {
		this->storyTotalPages += 1;
	}
}

void Canvas::loadEpilogueText() {
	Applet* app = CAppContainer::getInstance()->app;

	this->imgProlog = app->loadImage("prolog.bmp", true);
	this->imgProlog_0_1 = app->loadImage("prolog_0_1.bmp", true);
	this->imgProlog_1_0 = app->loadImage("prolog_1_0.bmp", true);
	this->imgProlog_1_1 = app->loadImage("prolog_1_1.bmp", true);
	this->imgProlog_2_0 = app->loadImage("prolog_2_0.bmp", true);
	this->imgProlog_2_1 = app->loadImage("prolog_2_1.bmp", true);

	this->initScrollingText(Strings::FILE_CODESTRINGS, CodeStrings::EPILOGUE_SCROLLING_TXT, true, Applet::FONT_HEIGHT[app->fontType] * 2, 1, 1000);
}

void Canvas::disposeIntro() {
	this->imgTravelBG->~Image();
	this->imgTravelBG = nullptr;
	this->dialogBuffer->dispose();
	this->dialogBuffer = nullptr;
	this->loadMap(this->startupMap, false, true);
}

void Canvas::disposeEpilogue() {
	Applet* app = CAppContainer::getInstance()->app;
	this->imgTravelBG->~Image();
	this->imgTravelBG = nullptr;
	this->dialogBuffer->dispose();
	this->dialogBuffer = nullptr;
	app->menuSystem->setMenu(Menus::MENU_LEVEL_STATS);
}

void Canvas::drawScroll(Graphics* graphics, int n, int n2, int n3, int n4, int n5) { // J2ME
	if (n5 == 0) {
		int i = 0;
		int j;
		int width = this->imgNotebookBg->width;
		int height = this->imgNotebookBg->height;
		int n6 = j = ((width > this->screenRect[2]) ? this->screenRect[2] : width);
		int n7 = 38;
		while (i < n4) {
			height = ((i + height > n4) ? (n4 - i) : height);
			graphics->drawRegion(this->imgNotebookBg, 0, 0, n6, height, n, n2 + i, 0, 0, 0);
			while (j < n3) {
				n7 = ((j + n7 > n3) ? (n3 - j) : n7);
				graphics->drawRegion(this->imgNotebookBg, n6 - 38, 0, 38, height, n + j, n2 + i, 0, 0, 0);
				j += n7;
			}
			j = n6;
			n7 = 38;
			i += height;
		}
	}
	else if (n5 == 1) {
		int width = this->imgDialogScroll->width;
		int height = this->imgDialogScroll->height;
		graphics->drawRegion(this->imgDialogScroll, 0, 0, width, height, n, n2 + n4 - height, 0, 0, 0);
		graphics->drawRegion(this->imgDialogScroll, 0, 0, width, height, n + n3 - width, n2 + n4 - height, 0, 2, 0);
		graphics->drawRegion(this->imgDialogScroll, 0, 0, width, height, n, n2, 0, 1, 0);
		graphics->drawRegion(this->imgDialogScroll, 0, 0, width, height, n + n3 - width, n2, 0, 3, 0);
		graphics->fillRegion(this->imgDialogScroll, width - 14, 0, 14, height, n + width, n2, n3 - 2 * width, height, 3);
		graphics->fillRegion(this->imgDialogScroll, width - 14, 0, 14, height, n + width, n2 + (n4 - height), n3 - 2 * width, height, 0);
		graphics->fillRegion(this->imgDialogScroll, 0, 0, width, 6, n, n2 + height, width, n4 - 2 * height, 0);
		graphics->fillRegion(this->imgDialogScroll, 0, 0, width, 6, n + n3 - width, n2 + height, width, n4 - 2 * height, 3);
		graphics->fillRegion(this->imgDialogScroll, width - 6, 0, 6, 6, n + width, n2 + height, n3 - 2 * width, n4 - 2 * height, 0);
	}
}

void Canvas::initScrollingText(short i, short i2, bool dehyphenate, int spacingHeight, int numLines, int textMSLine) {
	Applet* app = CAppContainer::getInstance()->app;

	if (this->dialogBuffer == nullptr) {
		this->dialogBuffer = app->localization->getLargeBuffer();
	}
	else {
		this->dialogBuffer->setLength(0);
	}

	app->localization->composeText(i, i2, this->dialogBuffer);

	if (dehyphenate) {
		this->dialogBuffer->dehyphenate();
	}

	this->dialogBuffer->wrapText((this->displayRect[2] - 8) / Applet::CHAR_SPACING[app->fontType]);
	this->scrollingTextSpacing = spacingHeight;
	this->scrollingTextStart = -1;
	this->scrollingTextLines = (this->dialogBuffer->getNumLines() + numLines);
	this->scrollingTextMSLine = textMSLine;
	this->scrollingTextDone = false;
	this->scrollingTextFontHeight = Applet::FONT_HEIGHT[app->fontType] + 2;
	this->scrollingTextSpacingHeight = spacingHeight * ((316 - (Applet::FONT_HEIGHT[app->fontType] * 2)) / spacingHeight);
}

void Canvas::drawCredits(Graphics* graphics) {
	Applet* app = CAppContainer::getInstance()->app;
	Text* textBuff;

	graphics->setColor(0x00000000);
	graphics->fillRect(this->screenRect[0], this->screenRect[1], this->screenRect[2], this->screenRect[3]);

	this->drawScrollingText(graphics);
	if (this->scrollingTextDone != false) {
		textBuff = app->localization->getSmallBuffer();
		textBuff->setLength(0);
		app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::PRESS_OK_TO_EXIT, textBuff);
		textBuff->dehyphenate();
		graphics->drawString(textBuff, this->SCR_CX, this->screenRect[3] - Applet::FONT_HEIGHT[app->fontType] * 3, 3);
		textBuff->dispose();
	}
}

void Canvas::drawScrollingText(Graphics* graphics) {
	Applet* app = CAppContainer::getInstance()->app;

	int n = this->scrollingTextMSLine * ((this->scrollingTextSpacing << 16) / Applet::FONT_HEIGHT[app->fontType]) >> 16;
	if (this->scrollingTextStart == -1) {
		this->scrollingTextStart = app->gameTime;
		int n2 = this->screenRect[3] / this->scrollingTextSpacing;
		if (this->state == Canvas::ST_CREDITS) {
			this->scrollingTextEnd = n * this->scrollingTextLines;
		}
		else {
			this->scrollingTextEnd = n * (this->scrollingTextLines + n2);
		}
	}

	int gameTime = app->gameTime;
	int scrollingTextStart = this->scrollingTextStart;
	if (gameTime - scrollingTextStart > this->scrollingTextEnd) {
		scrollingTextStart = gameTime - this->scrollingTextEnd;
		this->scrollingTextDone = true;
	}

	//graphics->eraseRgn(this->displayRect);

	int height = this->displayRect[3];
	if (this->state == Canvas::ST_EPILOGUE || this->state == Canvas::ST_INTRO_MOVIE) {

		if (this->displayRect[3] > 320) {
			graphics->setScreenSpace(0, (this->displayRect[3] - 320) / 2, this->displayRect[2], 320);
			height = Applet::FONT_HEIGHT[app->fontType] + (this->displayRect[3] - 320) / 2 + 320;
		}

		int x2 = 932 - this->SCR_CX;
		int y2 = 386 - this->SCR_CY;
		int y1 = -y2;
		int x1 = this->SCR_CX - 932;
		y2 = 512 - y2;
		graphics->drawRegion(this->imgProlog, 0, 0, 512, 512, x1, y1, 0, 0, 0);
		graphics->drawRegion(this->imgProlog_0_1, 0, 0, 512, 184, x1, y2, 0, 0, 0);
		x1 = 512 - x2;
		graphics->drawRegion(this->imgProlog_1_0, 0, 0, 512, 512, x1, y1, 0, 0, 0);
		graphics->drawRegion(this->imgProlog_1_1, 0, 0, 512, 184, x1, y2, 0, 0, 0);
		x2 = 1024 - x2;
		graphics->drawRegion(this->imgProlog_2_0, 0, 0, 176, 512, x2, y1, 0, 0, 0);
		graphics->drawRegion(this->imgProlog_2_1, 0, 0, 176, 184, x2, y2, 0, 0, 0);
		graphics->fade(this->displayRect, 128, 0);
	}

	graphics->drawString(this->dialogBuffer, this->SCR_CX, height - ((((gameTime - scrollingTextStart) << 8) / n) * (this->scrollingTextSpacing << 8) >> 16), this->scrollingTextSpacing, 1, 0, -1);
	graphics->resetScreenSpace();
}

void Canvas::handleDialogEvents(int key) {
	Applet* app = CAppContainer::getInstance()->app;

	int action = this->getKeyAction(key);
	if (action == Enums::ACTION_FIRE) {
		if (this->dialogTypeLineIdx < this->dialogViewLines && this->dialogTypeLineIdx < this->numDialogLines - this->currentDialogLine) {
			this->dialogTypeLineIdx = this->dialogViewLines;
		}
		else if (this->currentDialogLine < this->numDialogLines - this->dialogViewLines) {
			this->dialogLineStartTime = app->time;
			this->dialogTypeLineIdx = 0;
			this->currentDialogLine += this->dialogViewLines;
			if (((this->dialogFlags & Enums::DLG_FLAG_GAME) || (this->dialogFlags & Enums::DLG_FLAG_YESNO)) && this->currentDialogLine + this->dialogViewLines > this->numDialogLines) {
				this->currentDialogLine = this->numDialogLines - this->dialogViewLines;
			}
		}
		else {
			this->closeDialog(false);
		}
	}
	else if (action == Enums::ACTION_UP) {
		if (this->currentDialogLine >= this->numDialogLines - this->dialogViewLines && (this->dialogFlags & Enums::DLG_FLAG_INTERROGATE)) {
			if (app->game->scriptStateVars[Enums::CODEVAR_DIALOG_CHOICE] == 0) {
				this->currentDialogLine--;
				if (this->currentDialogLine < 0) {
					this->currentDialogLine = 0;
				}
			}
			else {
				app->game->scriptStateVars[Enums::CODEVAR_DIALOG_CHOICE]--;
			}
		}
		else {
			this->currentDialogLine--;
			if (this->currentDialogLine < 0) {
				this->currentDialogLine = 0;
			}
		}
	}
	else if (action == Enums::ACTION_DOWN) {
		if (this->currentDialogLine >= this->numDialogLines - this->dialogViewLines && (this->dialogFlags & Enums::DLG_FLAG_INTERROGATE)) {
			if (app->game->scriptStateVars[Enums::CODEVAR_DIALOG_CHOICE] < 2) {
				app->game->scriptStateVars[Enums::CODEVAR_DIALOG_CHOICE]++;
			}
		}
		else {
			this->currentDialogLine++;
			if (this->currentDialogLine > this->numDialogLines - this->dialogViewLines) {
				this->currentDialogLine = this->numDialogLines - this->dialogViewLines;
				if (!(this->dialogFlags & Enums::DLG_FLAG_INTERROGATE)) {
					if (this->currentDialogLine < 0) {
						this->currentDialogLine = 0;
					}
				}
			}
			else {
				this->dialogLineStartTime = app->time;
				this->dialogTypeLineIdx = this->dialogViewLines - 1;
			}
		}
	}
	else if ((action == Enums::ACTION_LEFT || action == Enums::ACTION_RIGHT) && (this->dialogFlags & (Enums::DLG_FLAG_YESNO | Enums::DLG_FLAG_GAME)) != 0x0 && this->currentDialogLine >= this->numDialogLines - this->dialogViewLines) {
		app->game->scriptStateVars[Enums::CODEVAR_DIALOG_CHOICE] ^= 0x1;
	}
	else if (action == Enums::ACTION_PASSTURN || action == Enums::ACTION_AUTOMAP) {
		this->closeDialog(true);
	}
	else if (action == Enums::ACTION_MENU || action == Enums::ACTION_LEFT) {
		this->currentDialogLine -= this->dialogViewLines;
		if (this->currentDialogLine < 0) {
			this->currentDialogLine = 0;
		}
	}
	else if (action == Enums::ACTION_RIGHT) {
		this->currentDialogLine += this->dialogViewLines;
		if (this->currentDialogLine > this->numDialogLines - this->dialogViewLines) {
			this->currentDialogLine = std::max(this->numDialogLines - this->dialogViewLines, 0);
		}
	}
	if (this->state == Canvas::ST_PLAYING && app->game->monstersTurn == 0) {
		this->dequeueHelpDialog();
	}
}

bool Canvas::handlePlayingEvents(int key, int action) {
	Applet* app = CAppContainer::getInstance()->app;
	//printf("handlePlayingEvents %d, %d\n", key, action);

	bool b = false;
	if (!this->isZoomedIn && (this->viewX != this->destX || this->viewY != this->destY || this->viewAngle != this->destAngle)) {
		return false;
	}

	if (this->knockbackDist != 0 || this->changeMapStarted) {
		return false;
	}

	if (this->renderOnly) {
		this->viewX = this->destX;
		this->viewY = this->destY;
		this->viewZ = this->destZ;
		this->viewAngle = this->destAngle;
		this->viewAngle = this->destPitch;
		this->viewSin = app->render->sinTable[this->destAngle & 0x3FF];
		this->viewCos = app->render->sinTable[this->destAngle + 256 & 0x3FF];
		this->viewStepX = this->viewCos * 64 >> 16;
		this->viewStepY = -this->viewSin * 64 >> 16;
		this->invalidateRect();
	}
	else {
		if (!this->isZoomedIn) {
			if (this->viewX != this->destX || this->viewY != this->destY) {
				b = true;
				this->viewX = this->destX;
				this->viewY = this->destY;
				this->viewZ = this->destZ;
				this->finishMovement();
				this->invalidateRect();
			}
			else if (this->viewAngle != this->destAngle) {
				b = true;
				this->viewAngle = this->destAngle;
				this->viewAngle = this->destPitch;
				this->finishRotation(true);
				this->invalidateRect();
			}
		}
		if (this->blockInputTime != 0) {
			return true;
		}

		if (app->hud->isInWeaponSelect != 0) { // [GEC]
			return true;
		}

		bool b2 = this->state == Canvas::ST_AUTOMAP;
		if (action != Enums::ACTION_PREVWEAPON && action != Enums::ACTION_NEXTWEAPON && action != Enums::ACTION_LEFT && action != Enums::ACTION_RIGHT && (app->game->activePropogators != 0 || !app->game->snapMonsters(b2) || app->game->animatingEffects != 0)) {
			return true;
		}
	}

	if (this->isChickenKicking) {
		if (this->kickingPhase == 5) {
			this->handleHighScoreInput(action, key);
			return true;
		}
		if (this->kickingPhase == 4) {
			if (this->gridTime - app->gameTime >= 1000) {
				this->gridTime = 0;
			}
			return true;
		}
	}

	if (action == Enums::ACTION_AUTOMAP) {
		if (!this->isChickenKicking) {
			this->setState((this->state != Canvas::ST_AUTOMAP) ? Canvas::ST_AUTOMAP : Canvas::ST_PLAYING);
		}
	}
	else if (action == Enums::ACTION_UP) {
		this->attemptMove(this->viewX + this->viewStepX, this->viewY + this->viewStepY);
	}
	else if (action == Enums::ACTION_DOWN) {
		this->attemptMove(this->viewX - this->viewStepX, this->viewY - this->viewStepY);
	}
	else if (action == Enums::ACTION_STRAFELEFT) {
		this->attemptMove(this->viewX + this->viewStepY, this->viewY - this->viewStepX);
	}
	else if (action == Enums::ACTION_STRAFERIGHT) {
		this->attemptMove(this->viewX - this->viewStepY, this->viewY + this->viewStepX);
	}
	else if (action == Enums::ACTION_LEFT || action == Enums::ACTION_RIGHT) {
		if (!this->isChickenKicking || this->kickingPhase == 0) {
			int n3 = 256;
			app->hud->damageTime = 0;
			if (action == Enums::ACTION_RIGHT) {
				n3 = -256;
			}
			this->destAngle += n3;
			this->startRotation(false);
			this->automapDrawn = false;
		}
	}
	else if (action == Enums::ACTION_PREVWEAPON || action == Enums::ACTION_NEXTWEAPON) {
		int weapon = app->player->ce->weapon;
		if (weapon == Enums::WP_ITEM) {
			return true;
		}
		if (action == Enums::ACTION_PREVWEAPON) {
			app->player->selectPrevWeapon();
		}
		else {
			app->player->selectNextWeapon();
		}
		app->sound->playSound(1125, 0, 3, false);
		if (weapon != app->player->ce->weapon) {
			app->hud->addMessage(Strings::FILE_ENTITYSTRINGS, (short)app->player->activeWeaponDef->longName, 1);
		}
		app->player->helpBitmask |= 0x200;
	}
	else if (action == Enums::ACTION_BACK) {
		if (this->state == Canvas::ST_AUTOMAP) {
			this->setState(Canvas::ST_PLAYING);
		}
		else {
			app->hud->msgCount = 0;
			app->menuSystem->setMenu(Menus::MENU_INGAME);
		}
	}
	else if (action == Enums::ACTION_MENU) {
		app->hud->msgCount = 0;
		app->menuSystem->setMenu(Menus::MENU_INGAME);
	}
	else if (action == Enums::ACTION_ITEMS) { // [GEC] From Doom II RPG
		app->hud->msgCount = 0;
		app->menuSystem->setMenu(Menus::MENU_ITEMS);
		app->menuSystem->oldMenu = Menus::MENU_ITEMS; // [GEC]
	}
	else if (action == Enums::ACTION_ITEMS_SYRINGES) { // [GEC] From Doom II RPG
		app->hud->msgCount = 0;
		app->menuSystem->setMenu(Menus::MENU_ITEMS_SYRINGES);
		app->menuSystem->oldMenu = Menus::MENU_ITEMS_SYRINGES; // [GEC]
	}
	else if (action == Enums::ACTION_QUESTLOG) { // [GEC] From Doom II RPG
		app->hud->msgCount = 0;
		app->menuSystem->setMenu(Menus::MENU_INGAME_QUESTLOG);
		app->menuSystem->oldMenu = Menus::MENU_INGAME_QUESTLOG; // [GEC]
	}
	else if (action == Enums::ACTION_FIRE) {
		if (app->player->facingEntity != nullptr && (app->player->facingEntity->def->eType == Enums::ET_ATTACK_INTERACTIVE || app->player->facingEntity->def->eType == Enums::ET_ATTACK_INTERACTIVE)) {
			this->lootSource = app->player->facingEntity->name;
		}
		else {
			this->lootSource = -1;
		}

		int weapon2 = app->player->ce->weapon;
		
		int n4 = 16384;
		int n5 = 13997; // CONTENTS_WEAPONSOLID

		if (Entity::CheckWeaponMask(weapon2, Enums::WP_SPRITE_WALL)) {
			n5 |= 0x2000;
		}

		int n6 = 0;
		int n7 = 6;
		if (Entity::CheckWeaponMask(weapon2, Enums::WP_MELEEMASK)) {
			n7 = 1;
			n5 |= 0x10;
		}

		Entity* entity = nullptr;
		Entity* entity2 = nullptr;

		int n8 = this->viewX + (n7 * -app->tinyGL->view[2] >> 8);
		int n9 = this->viewY + (n7 * -app->tinyGL->view[6] >> 8);
		int n10 = this->viewZ + (n7 * -app->tinyGL->view[10] >> 8);
		app->game->trace(this->viewX, this->viewY, this->viewZ, n8, n9, n10, nullptr, n5, 2);

		int i = 0;
		while (i < app->game->numTraceEntities) {
			Entity* entity3 = app->game->traceEntities[i];
			int n11 = app->game->traceFracs[i];
			int dist = entity3->distFrom(this->viewX, this->viewY);
			uint8_t eType = entity3->def->eType;

			if (eType == Enums::ET_WORLD || eType == Enums::ET_SPRITEWALL || eType == Enums::ET_PLAYERCLIP) {
				if (entity == nullptr) {
					entity = entity3;
					n4 = n11;
					break;
				}
				break;
			}
			else {
				if (eType == Enums::ET_ATTACK_INTERACTIVE) {
					if (!(1 << entity3->def->eSubType & Enums::INTERACT_SHOOT_OVER)) {
						entity = entity3;
						n4 = n11;
						break;
					}
					if (dist <= app->combat->tileDistances[0] && Entity::CheckWeaponMask(weapon2, Enums::WP_MELEEMASK)) {
						entity = entity3;
						n4 = n11;
						break;
					}
				}
				else if (eType == Enums::ET_NONOBSTRUCTING_SPRITEWALL) {
					if (Entity::CheckWeaponMask(weapon2, Enums::WP_SPRITE_WALL)) {
						entity2 = entity3;
					}
				}
				else if (eType == Enums::ET_NPC) {
					if (dist >= 8192) {
						entity = entity3;
						n4 = n11;
						break;
					}
				}
				else {
					if (eType == Enums::ET_MONSTER || eType == Enums::ET_DOOR) {
						entity = entity3;
						n4 = n11;
						break;
					}

					if (eType == Enums::ET_CORPSE) {
						if (weapon2 == Enums::WP_BOOT && dist == app->combat->tileDistances[0]) {
							if (entity != nullptr && entity->def->eType == Enums::ET_CORPSE) {
								if (entity->linkIndex < entity3->linkIndex) {
									entity = entity3;
									n4 = n11;
								}
							}
							else {
								entity = entity3;
								n4 = n11;
							}
						}
					}
					else if (eType == Enums::ET_DECOR) {
						int n11 = app->render->mapSpriteInfo[entity3->getSprite()] & 0xFF;
						if (n11 == Enums::TILENUM_TREE_TOP || n11 == Enums::TILENUM_TREE_TRUNK) {
							entity = entity3;
							n4 = n11;
							break;
						}
					}
					else if (eType != Enums::ET_DECOR && eType != Enums::ET_ITEM && entity == nullptr) {
						entity = entity3;
						n4 = n11;
					}
				}
				++i;
			}
		}

		int dist2 = app->combat->tileDistances[9];
		if (entity != nullptr) {
			dist2 = entity->distFrom(this->viewX, this->viewY);
		}

		int n12 = weapon2 * Combat::WEAPON_MAX_FIELDS;
		if (entity != nullptr && entity->def->eType == Enums::ET_ATTACK_INTERACTIVE && !(1 << entity->def->eSubType & Enums::INTERACT_SHOOT_OVER) && app->combat->WorldDistToTileDist(dist2) > app->combat->weapons[n12 + Combat::WEAPON_FIELD_RANGEMAX]) {
			entity = nullptr;
		}

		if (this->isChickenKicking && entity != nullptr && entity->def->eType == Enums::ET_ATTACK_INTERACTIVE && entity->def->eSubType == Enums::INTERACT_CHICKEN && (this->viewAngle & 0x3FF) == this->kickingDir) {
			if (this->kickingPhase == 0 && this->vKickingBar == nullptr && this->hKickingBar == nullptr && this->imgChickenKicking_ScoreBG == nullptr) {
				app->beginImageLoading();
				this->hKickingBar = app->loadImage("Horz_Kicking.bmp", true);
				this->vKickingBar = app->loadImage("Vert_Kicking.bmp", true);
				this->imgChickenKicking_ScoreBG = app->loadImage("ChickenKicking_ScoreBG.bmp", true);
				this->imgChickenKicking_05Highlight = app->loadImage("ChickenKicking_05Highlight.bmp", true);
				this->imgChickenKicking_20Highlight = app->loadImage("ChickenKicking_20Highlight.bmp", true);
				this->imgChickenKicking_30Highlight = app->loadImage("ChickenKicking_30Highlight.bmp", true);
				app->finishImageLoading();
			}
			++this->kickingPhase;
			this->clearEvents(1);
			this->stateVars[0] = app->upTimeMs;
			if (this->kickingPhase != 3) {
				return true;
			}
		}

		if (entity2 != nullptr && (entity == nullptr || Entity::CheckWeaponMask(weapon2, Enums::WP_PUNCH_MASK) || (entity->def->eType != Enums::ET_MONSTER && entity->def->eType != Enums::ET_CORPSE))) {
			entity = entity2;
		}

		int flagForFacingDir = this->flagForFacingDir(4);
		int n13 = this->destX + this->viewStepX >> 6;
		int n14 = this->destY + this->viewStepY >> 6;
		if (app->game->executeTile(n13, n14, flagForFacingDir, true)) {
			if (!app->game->skipAdvanceTurn && this->state == Canvas::ST_PLAYING) {
				app->game->touchTile(this->destX, this->destY, false);
				app->game->snapMonsters(true);
				app->game->advanceTurn();
			}
		}
		else {
			if (entity != nullptr && 
				entity->def->eType == Enums::ET_ATTACK_INTERACTIVE && entity->def->eSubType == Enums::INTERACT_PICKUP &&
				dist2 <= app->combat->tileDistances[0] && app->player->ammo[9] == 0) {

				int* calcPosition = entity->calcPosition();
				int gridX = calcPosition[0] >> 6;
				int gridY = calcPosition[1] >> 6;

				if (this->loadMapID == 6 && (gridX >= 19 && gridX <= 24 && gridY >= 8 && gridY <= 11)) {
					app->sound->playSound(1146, 0, 5, 0);
				}
				else {
					app->sound->playSound(1144, 0, 5, 0);
				}

				app->player->setPickUpWeapon(entity->def->tileIndex);
				app->player->give(Enums::IT_AMMO, Enums::AMMO_ITEM, 1, true);
				app->player->giveAmmoWeapon(Enums::WP_ITEM, true);

				if (entity->info & Entity::ENTITY_FLAG_LINKED) {
					app->game->unlinkEntity(entity);
				}

				int sprite = entity->getSprite();
				app->render->mapSpriteInfo[sprite] = ((app->render->mapSpriteInfo[sprite] & 0xFFFF00FF) | 0x200);
				entity->info |= Entity::ENTITY_FLAG_DIRTY;

				
				if (this->shouldFakeCombat(gridX, gridY, flagForFacingDir) && app->combat->explodeThread != nullptr) {
					app->combat->explodeThread->run();
					app->combat->explodeThread = nullptr;
				}
				return true;
			}
			if (entity != nullptr && entity->def->eType == Enums::ET_DOOR && dist2 <= app->combat->tileDistances[0] && weapon2 != Enums::WP_ITEM) {
				if (entity->def->eSubType == Enums::DOOR_LOCKED) {
					app->hud->addMessage(CodeStrings::DOOR_IS_LOCKED, 2);
					app->sound->playSound(1013, 0, 3, 0);
				}
				else {
					app->game->performDoorEvent(0, entity, 1);
					app->game->advanceTurn();
				}
				return true;
			}
			if (entity != nullptr && entity->def->eType == Enums::ET_SPRITEWALL && dist2 <= app->combat->tileDistances[0] && (app->render->mapFlags[n14 * 32 + n13] & Canvas::BIT_AM_SECRET_DOOR)) {
				if (app->game->performDoorEvent(0, entity, 1, true)) {
					app->game->awardSecret(true);
				}
				return true;
			}

			if (app->player->ce->weapon != Enums::WP_DYNAMITE && entity != nullptr && entity->def->eType == Enums::ET_WORLD && dist2 <= app->combat->tileDistances[0]) {
				app->combat->shiftWeapon(true);
				this->pushedWall = true;
				this->pushedTime = app->gameTime + 500;
				app->render->rockView(1000, this->viewX + (this->viewStepX >> 6) * 2, this->viewY + (this->viewStepY >> 6) * 2, this->viewZ);
				app->sound->playSound(1146, 0, 4, 0);
				return true;
			}

			if (app->game->isUnderWater() && !Entity::CheckWeaponMask(weapon2, Enums::WP_MELEEMASK) && weapon2 != Enums::WP_DYNAMITE) {
				app->sound->playSound(1132, 0, 4, 0);
				app->player->showHelp(CodeStrings::GENHELP_WATER_WEAPON, false);
				return true;
			}

			if ((entity == nullptr || entity->def->eType == Enums::ET_WORLD) && !this->isZoomedIn && Entity::CheckWeaponMask(weapon2, Enums::WP_SNIPERMASK) && app->player->ammo[app->combat->weapons[n12 + Combat::WEAPON_FIELD_AMMOTYPE]] > 0) {
				this->initZoom();
				return true;
			}

			if (entity != nullptr && entity->def->eType != Enums::ET_WORLD) {

				if (!this->isZoomedIn && Entity::CheckWeaponMask(weapon2, Enums::WP_SNIPERMASK) && app->player->ammo[app->combat->weapons[n12 + Combat::WEAPON_FIELD_AMMOTYPE]] > 0) {
					this->initZoom();
					return true;
				}

				int* calcPosition2 = entity->calcPosition();
				bool shouldFakeCombat = this->shouldFakeCombat(calcPosition2[0] >> 6, calcPosition2[1] >> 6, flagForFacingDir);
				int n15 = weapon2 * Combat::WEAPON_MAX_FIELDS;
				if (shouldFakeCombat || (app->combat->weapons[n15 + Combat::WEAPON_FIELD_RANGEMAX] == 1 && app->combat->weapons[n15 + Combat::WEAPON_FIELD_RANGEMIN] == 1 && dist2 > app->combat->tileDistances[0])) {
					app->game->traceCollisionX = calcPosition2[0];
					app->game->traceCollisionY = calcPosition2[1];
					app->player->fireWeapon(&app->game->entities[0], calcPosition2[0], calcPosition2[1]);
				}
				else {
					if (this->isZoomedIn) {
						this->zoomCollisionX = this->viewX + (n4 * (n8 - this->viewX) >> 14);
						this->zoomCollisionY = this->viewY + (n4 * (n9 - this->viewY) >> 14);
						this->zoomCollisionZ = this->viewZ + (n4 * (n10 - this->viewZ) >> 14);
					}
					app->player->fireWeapon(entity, calcPosition2[0], calcPosition2[1]);
				}
			}
			else {
				this->shouldFakeCombat(app->game->traceCollisionX >> 6, app->game->traceCollisionY >> 6, flagForFacingDir);
				app->player->fireWeapon(&app->game->entities[0], app->game->traceCollisionX, app->game->traceCollisionY);
			}
		}
	}
	else if (action == Enums::ACTION_PASSTURN) {
		app->hud->addMessage(CodeStrings::TURN_PASSED);
		app->game->touchTile(this->destX, this->destY, false);
		app->game->advanceTurn();
		this->invalidateRect();
	}

	return this->endOfHandlePlayingEvent(action, b);
}

bool Canvas::shouldFakeCombat(int n, int n2, int n3) {
	Applet* app = CAppContainer::getInstance()->app;
	bool b = false;
	int n4 = app->player->ce->weapon * Combat::WEAPON_MAX_FIELDS;
	if (app->combat->weapons[n4 + Combat::WEAPON_FIELD_AMMOTYPE] != 0) {
		short n5 = app->player->ammo[app->combat->weapons[n4 + Combat::WEAPON_FIELD_AMMOTYPE]];
		if (app->combat->weapons[n4 + Combat::WEAPON_FIELD_AMMOUSAGE] > 0 && n5 - app->combat->weapons[n4 + Combat::WEAPON_FIELD_AMMOUSAGE] < 0) {
			return false;
		}
	}
	n3 |= this->flagForWeapon(app->player->ce->weapon);
	if (app->game->doesScriptExist(n, n2, n3)) {
		(app->combat->explodeThread = app->game->allocScriptThread())->queueTile(n, n2, n3);
		b = true;
	}
	return b;
}

bool Canvas::endOfHandlePlayingEvent(int action, bool b) {
	Applet* app = CAppContainer::getInstance()->app;
	if ((action == Enums::ACTION_STRAFELEFT || action == Enums::ACTION_STRAFERIGHT || action == Enums::ACTION_LEFT || action == Enums::ACTION_RIGHT) && (this->viewX != this->destX || this->viewY != this->destY || this->viewAngle != this->destAngle)) {
		app->player->facingEntity = nullptr;
	}
	return true;
}

bool Canvas::handleEvent(int key) {
	Applet* app = CAppContainer::getInstance()->app;

	int state = this->state;
	int keyAction = this->getKeyAction(key);
	int fadeFlags = app->render->getFadeFlags();

	
	//printf("handleEvent key: %d keyAction: %d\n", key, keyAction);
	//printf("this->state %d\n", state);

	if (key == 26)
		return true;

	if (key == 18) {
		switch (state)
		{
		case Canvas::ST_LOGO:
			app->shutdown();
			break;
		case Canvas::ST_MENU:
			if (app->menuSystem->menu == Menus::MENU_ENABLE_SOUNDS) {
				app->shutdown();
			}
			break;
		case Canvas::ST_INTRO_MOVIE:
			this->exitIntroMovie(true);
			app->shutdown();
			break;
		}
		return true;
	}

	if (this->state == Canvas::ST_MENU && app->menuSystem->changeValues) {
		if (app->menuSystem->changeSfxVolume) { // [GEC]
			if (keyAction == Enums::ACTION_RIGHT) {
				app->sound->volumeUp(10);
				//app->menuSystem->soundClick();
				app->menuSystem->refresh();
				return true;
			}
			else if (keyAction == Enums::ACTION_LEFT) {
				app->sound->volumeDown(10);
				//app->menuSystem->soundClick();
				app->menuSystem->refresh();
				return true;
			}
		}
		else if (app->menuSystem->changeMusicVolume) { // [GEC]
			if (keyAction == Enums::ACTION_RIGHT) {
				app->sound->musicVolumeUp(10);
				//app->menuSystem->soundClick();
				app->menuSystem->refresh();
				return true;
			}
			else if (keyAction == Enums::ACTION_LEFT) {
				app->sound->musicVolumeDown(10);
				//app->menuSystem->soundClick();
				app->menuSystem->refresh();
				return true;
			}
		}
		else if (app->menuSystem->changeButtonsAlpha) { // [GEC]
			if (keyAction == Enums::ACTION_RIGHT) {
				this->m_controlAlpha += 10;
				if (this->m_controlAlpha > 100) {
					this->m_controlAlpha = 100;
				}
				//app->menuSystem->soundClick();
				app->menuSystem->refresh();
				return true;
			}
			else if (keyAction == Enums::ACTION_LEFT) {
				this->m_controlAlpha -= 10;
				if (this->m_controlAlpha < 0) {
					this->m_controlAlpha = 0;
				}
				//app->menuSystem->soundClick();
				app->menuSystem->refresh();
				return true;
			}
		}
		else if (app->menuSystem->changeVibrationIntensity) { // [GEC]
			if (keyAction == Enums::ACTION_RIGHT) {
				gVibrationIntensity += 10;
				if (gVibrationIntensity > 100) {
					gVibrationIntensity = 100;
				}
				//app->menuSystem->soundClick();
				app->menuSystem->refresh();
				return true;
			}
			else if (keyAction == Enums::ACTION_LEFT) {
				gVibrationIntensity -= 10;
				if (gVibrationIntensity < 0) {
					gVibrationIntensity = 0;
				}
				//app->menuSystem->soundClick();
				app->menuSystem->refresh();
				return true;
			}
		}
		else if (app->menuSystem->changeDeadzone) { // [GEC]
			if (keyAction == Enums::ACTION_RIGHT) {
				gDeadZone += 5;
				if (gDeadZone > 100) {
					gDeadZone = 100;
				}
				//app->menuSystem->soundClick();
				app->menuSystem->refresh();
				return true;
			}
			else if (keyAction == Enums::ACTION_LEFT) {
				gDeadZone -= 5;
				if (gDeadZone < 0) {
					gDeadZone = 0;
				}
				//app->menuSystem->soundClick();
				app->menuSystem->refresh();
				return true;
			}
		}
	}

#if 0 // IOS
	if (app->sound->allowSounds) {
		bool refresh = false;
		if (this->state == Canvas::ST_MENU && app->menuSystem->changeSfxVolume) {
			refresh = true;
		}
		if (key == 27) {
			app->sound->volumeUp(10);
			if (refresh) {
				app->menuSystem->refresh();
			}
			return true;
		}
		if (key == 28) {
			app->sound->volumeDown(10);
			if (refresh) {
				app->menuSystem->refresh();
			}
			return true;
		}
	}
#endif

	if (this->st_enabled) {
		this->st_enabled = false;
		this->renderOnly = false;
	}

	if (fadeFlags != 0 && (fadeFlags & 0x10) != 0x0) {
		return true;
	}

	if (state == Canvas::ST_ERROR) {
		if (keyAction == Enums::ACTION_FIRE || key == 18) {
			app->shutdown();
		}
	}
	else if (state == Canvas::ST_MENU) {
		app->menuSystem->handleMenuEvents(key, keyAction);
	}
	else if (state == Canvas::ST_INTRO) {
		this->handleStoryInput(key, keyAction);
	}
	else if (state == Canvas::ST_INTRO_MOVIE) {
		app->game->skipMovie = (keyAction == Enums::ACTION_FIRE);
	}
	else if (state == Canvas::ST_EPILOGUE) {
		if (key == 18) {
			this->disposeEpilogue();
		}
	}
	else if (state == Canvas::ST_DIALOG) {
		this->handleDialogEvents(key);
	}
	else if (state == Canvas::ST_MINI_GAME) {
		app->cardGames->handleInput(keyAction, key);
	}
	else if (state == Canvas::ST_BENCHMARK || state == Canvas::ST_BENCHMARKDONE) {
		this->setState(Canvas::ST_PLAYING);
		this->setAnimFrames(this->animFrames);
	}
	else if (state == Canvas::ST_AUTOMAP) {
		this->automapDrawn = false;
		return handlePlayingEvents(key, keyAction);
	}
	else if (state == Canvas::ST_DRIVING) {
		return app->drivingGame->handleDrivingEvents(key, keyAction);
	}
	else if (state == Canvas::ST_PLAYING) {
		if (this->isZoomedIn) {
			return this->handleZoomEvents(key, keyAction);
		}
		return this->handlePlayingEvents(key, keyAction);
	}
	else if (state == Canvas::ST_COMBAT) {
		if (app->combat->curAttacker != nullptr && !this->isZoomedIn) {
			if (keyAction == Enums::ACTION_RIGHT) {
				this->destAngle -= 256;
			}
			else if (keyAction == Enums::ACTION_LEFT) {
				this->destAngle += 256;
			}
			this->startRotation(false);
		}
		if (this->combatDone && !app->game->interpolatingMonsters) {
			this->setState(Canvas::ST_PLAYING);
			if (app->combat->curAttacker == nullptr) {
				app->game->advanceTurn();
				if (state == Canvas::ST_PLAYING) {
					if (this->isZoomedIn) {
						return handleZoomEvents(key, keyAction);
					}
					return this->handlePlayingEvents(key, keyAction);
				}
			}
		}
	}
	else if (state == Canvas::ST_CREDITS) {
		if (this->endingGame) {
			if ((keyAction == Enums::ACTION_FIRE && this->scrollingTextDone) || key == 18) {
				this->endingGame = false;
				app->sound->soundStop();
				app->menuSystem->imgMainBG->~Image();
				app->menuSystem->imgLogo->~Image();
				app->menuSystem->imgMainBG = app->loadImage(Resources::RES_LOGO_BMP_GZ, true);
				app->menuSystem->imgLogo = app->loadImage(Resources::RES_LOGO2_BMP_GZ, true);
				app->menuSystem->background = app->menuSystem->imgMainBG;
				app->menuSystem->setMenu(Menus::MENU_END_FINAL);
			}
		}
		else if (keyAction == Enums::ACTION_BACK || keyAction == Enums::ACTION_FIRE) {
			if (this->loadMapID == 0) {
				int n2 = 3;
				if (app->game->hasSavedState()) {
					++n2;
				}
				app->menuSystem->pushMenu(3, n2, 0, 0, 0);
				app->menuSystem->setMenu(Menus::MENU_MAIN_HELP);
			}
			else {
				app->sound->soundStop();
				app->menuSystem->pushMenu(28, 7, 0, 0, 0);
				app->menuSystem->setMenu(Menus::MENU_INGAME_HELP);
			}
		}
	}
	else if (state == Canvas::ST_CAMERA) {
		if (!this->changeMapStarted && app->gameTime > app->game->cinUnpauseTime && (keyAction == Enums::ACTION_PASSTURN || keyAction == Enums::ACTION_AUTOMAP || keyAction == Enums::ACTION_FIRE || key == 18)) {
			app->game->skipCinematic();
		}
	}
	else if (state == Canvas::ST_DYING) {
		if (this->stateVars[0] > 0 && (keyAction == Enums::ACTION_FIRE || key == 18)) {
			app->menuSystem->setMenu(Menus::MENU_INGAME_DEAD);
		}
	}
	else if (state == Canvas::ST_MIXING) {
		this->handleMixingEvents(keyAction);
		return 1;
	}
	else if (state == Canvas::ST_TRAVELMAP) {
		this->handleTravelMapInput(key, keyAction);
	}
	else {
		return false;
	}

	return true;
}

void Canvas::runInputEvents() {
	Applet* app = CAppContainer::getInstance()->app;

	while (this->blockInputTime == 0) {
		if (this->ignoreFrameInput > 0) {
			this->numEvents = 0;
			this->ignoreFrameInput--;
			return;
		}
		if (this->numEvents == 0) {
			return;
		}

		int ev = this->events[0];
		//printf("this->events[0] %d\n", ev);
		this->st_fields[11] = app->upTimeMs;
		if (!this->handleEvent(ev)) {
			return;
		}

		/*if (this->numEvents > 0) {
			this->numEvents--;
		}*/

		if (this->numEvents <= 0) {
			continue;
		}
		memcpy(this->events, this->events + 1, --this->numEvents * sizeof(int));
	}

	if (app->gameTime > this->blockInputTime) {
		if (this->state == Canvas::ST_PLAYING) {
			this->drawPlayingSoftKeys();
		}
		this->blockInputTime = 0;
	}
	this->clearEvents(1);
}

bool Canvas::loadMedia() {
	Applet* app = CAppContainer::getInstance()->app;

	//printf("Canvas::isLoaded %d\n", Canvas::isLoaded);
	if (Canvas::isLoaded == false) {
		this->updateLoadingBar(Canvas::isLoaded);
		this->drawLoadingBar(&this->graphics);
		Canvas::isLoaded = true;
		
		return false;
	}
	Canvas::isLoaded = false;

	bool allowSounds = app->sound->allowSounds;
	app->canvas->inInitMap = true;
	app->sound->allowSounds = false;

	this->state = 0;
	this->mediaLoading = true;

	bool displaySoftKeys = this->displaySoftKeys;
	this->updateLoadingBar(false);
	this->unloadMedia();
	this->displaySoftKeys = false;
	this->isZoomedIn = false;
	app->StopAccelerometer();
	app->tinyGL->resetViewPort();

	for (int i = 0; i < 64; ++i) {
		app->game->scriptStateVars[i] = 0;
	}

	if (!app->render->beginLoadMap(this->loadMapID)) {
		return false;
	}

	if (this->loadMapID <= 10 && this->loadMapID > app->player->highestMap) {
		app->player->highestMap = this->loadMapID;
	}

	if (app->game->isSaved) {
		this->setLoadingBarText(Strings::FILE_CODESTRINGS, CodeStrings::GAME_SAVED);
	}
	else if (app->game->isLoaded) {
		this->setLoadingBarText(Strings::FILE_CODESTRINGS, CodeStrings::GAME_LOADED);
	}
	else if (this->loadType != Game::LOAD_CHICKENGAME) {
		this->setLoadingBarText(Strings::FILE_MENUSTRINGS, (short)(app->render->mapNameField & 0x3FF));
	}

	this->updateLoadingBar(false);
	app->render->mapMemoryUsage -= 1000000000;
	//app->checkPeakMemory("after map loaded");
	app->game->loadMapEntities();
	app->hud->msgCount = 0;

	this->updateLoadingBar(false);
	app->player->playTime = app->gameTime;
	app->game->curLevelTime = app->gameTime;
	this->clearEvents(1);
	app->particleSystem->freeAllParticles();
	this->displaySoftKeys = displaySoftKeys;
	app->player->levelInit();

	this->updateLoadingBar(false);
	app->game->loadWorldState();

	this->updateLoadingBar(false);
	app->game->spawnPlayer();
	this->knockbackDist = 0;
	if (!app->game->isLoaded && this->loadType != Game::LOAD_CHICKENGAME) {
		app->game->saveLevelSnapshot();
	}

	this->updateLoadingBar(false);

	if (app->game->isLoaded || app->game->hasSavedState() || this->loadType == Game::LOAD_CHICKENGAME) {
		this->loadRuntimeData();
		//app->checkPeakMemory("after loadRuntimeData");
	}
	else {
		app->game->saveState(this->loadMapID, this->loadMapID, this->destX, this->destY, this->destAngle, this->destPitch, this->destX, this->destY, this->saveX, this->saveY, this->saveZ, this->saveAngle, this->savePitch, 3);
	}

	this->updateLoadingBar(false);
	app->player->selectWeapon(app->player->ce->weapon);
	app->game->scriptStateVars[Enums::CODEVAR_DIFFICULTY] = app->game->difficulty;
	app->game->executeStaticFunc(0);

	this->updateLoadingBar(false);
	if (app->player->gameCompleted) {
		app->game->executeStaticFunc(1);
	}

	if (!app->game->isLoaded) {
		this->prevX = this->destX;
		this->prevY = this->destY;
		app->game->executeTile(this->viewX >> 6, this->viewY >> 6, 4081, 1);
		this->finishRotation(false);
		this->dequeueHelpDialog(true);
	}
	this->finishRotation(false);

	app->game->endMonstersTurn();
	this->uncoverAutomap();
	this->updateLoadingBar(false);
	app->game->isSaved = (app->game->isLoaded = false);
	app->game->activeLoadType = Game::LOAD_NONE;
	this->dequeueHelpDialog(true);
	if (this->state == 0) {
		this->setState(Canvas::ST_PLAYING);
	}
	app->game->pauseGameTime = false;
	app->lastTime = (app->time = app->upTimeMs);
	this->blockInputTime = app->gameTime + 200;
	app->sound->allowSounds = allowSounds;
	this->inInitMap = false;
	this->mediaLoading = false;
	this->renderScene(this->viewX, this->viewY, this->viewZ, this->viewAngle, this->viewPitch, 0, 281);

	if (this->state != Canvas::ST_CAMERA) {
		this->repaintFlags |= Canvas::REPAINT_HUD;
	}
	this->repaintFlags |= Canvas::REPAINT_SOFTKEYS;

	if (this->state == Canvas::ST_PLAYING) {
		this->drawPlayingSoftKeys();
	}

	app->render->monsterIdleTime = app->time + 12000;

	return true;
}

void Canvas::combatState() {
	Applet* app = CAppContainer::getInstance()->app;

	app->game->monsterLerp();
	app->game->updateLerpSprites();
	if (this->combatDone) {
		if (!app->game->interpolatingMonsters) {
			if (app->combat->curAttacker == nullptr) {
				app->game->advanceTurn();
			}
			if (!app->game->isCameraActive()) {
				this->setState(Canvas::ST_PLAYING);
			}
			else {
				this->setState(Canvas::ST_CAMERA);
				app->game->activeCamera->cameraThread->run();
			}
		}
	}
	else if (app->combat->runFrame() == 0) {
		if (this->state == Canvas::ST_DYING) {
			while (app->game->combatMonsters != nullptr) {
				app->game->combatMonsters->undoAttack();
			}
			return;
		}
		if (app->combat->curAttacker == nullptr) {
			app->game->touchTile(this->destX, this->destY, false);
			this->combatDone = true;
		}
		else if (this->knockbackDist == 0) {
			Entity* curAttacker = app->combat->curAttacker;
			if (curAttacker->monster->goalFlags & EntityMonster::GFL_ATTACK2EVADE) {
				curAttacker->monster->resetGoal();
				curAttacker->monster->goalType = EntityMonster::GOAL_EVADE;
				curAttacker->monster->goalParam = 1;
				curAttacker->aiThink(false);
			}
			Entity* nextAttacker;
			Entity* nextAttacker2;
			for (nextAttacker = curAttacker->monster->nextAttacker; nextAttacker != nullptr && nextAttacker->monster->target == nullptr && !nextAttacker->aiIsAttackValid(); nextAttacker = nextAttacker2) {
				nextAttacker2 = nextAttacker->monster->nextAttacker;
				nextAttacker->undoAttack();
			}
			if (nextAttacker != nullptr) {
				app->combat->performAttack(nextAttacker, nextAttacker->monster->target, 0, 0, false);
			}
			else {
				app->game->combatMonsters = nullptr;
				if (app->game->interpolatingMonsters) {
					this->setState(Canvas::ST_PLAYING);
				}
				else {
					app->game->endMonstersTurn();
					this->drawPlayingSoftKeys();
					this->combatDone = true;
				}
			}
		}
	}
	this->updateView();
	this->repaintFlags |= Canvas::REPAINT_PARTICLES;
	app->hud->repaintFlags |= 0xB;
	if (!app->game->isCameraActive()) {
		this->repaintFlags |= Canvas::REPAINT_HUD;
	}
}

void Canvas::dialogState(Graphics* graphics) {
	Applet* app = CAppContainer::getInstance()->app;

	if (this->dialogBuffer != nullptr && this->dialogBuffer->length() == 0) {
		return;
	}

	int fontHeight = Applet::FONT_HEIGHT[app->fontType];
	int* dialogRect = this->dialogRect;
	dialogRect[0] = -this->screenRect[0];
	dialogRect[2] = this->hudRect[2];
	dialogRect[3] = this->dialogViewLines * fontHeight + (fontHeight / 2);
	dialogRect[1] = this->screenRect[3] - dialogRect[3] - 1;
	this->dialogTypeLineIdx = this->numDialogLines;
	int n = dialogRect[0] + 1;
	int n2 = 0xFF000000;
	int color = 0xFFFFFFFF;
	int color2 = 0xFF666666;
	switch (this->dialogStyle) {
		case 3: {
			n = -this->screenRect[0] + 1;
			dialogRect[1] = this->hudRect[3] - dialogRect[3] - 10;
			//this->drawScroll(graphics, dialogRect[0], dialogRect[1] - 10, this->hudRect[2], dialogRect[3] + 20);
			graphics->drawImage(this->imgClipboardBG, 0, 320, 32, 0, 0);
			this->m_dialogButtons->GetButton(8)->SetTouchArea(0, 320 - this->imgClipboardBG->height, this->imgClipboardBG->width, this->imgClipboardBG->height);
			break;
		}
		case 9: {
			color2 = 0xFF000066;
			break;
		}
		case 4: {
			if (this->dialogFlags & Enums::DLG_FLAG_YESNO) {
				n2 = 0xFFB18A01;
				break;
			}
			n2 = 0xFF005A00;
			break;
		}
		case 11: {
			n2 = 0xFF800000;
			if (this->dialogFlags & Enums::DLG_FLAG_INTERROGATE) {
				dialogRect[1] = this->hudRect[1] + 20;
				break;
			}
			break;
		}
		case 5: {
			n2 = 0xFF800000;
			if (this->dialogFlags & Enums::DLG_FLAG_INTERROGATE) {
				dialogRect[1] = this->hudRect[1] + 20;
				break;
			}
			break;
		}
		case 8: {
			dialogRect[1] = this->viewRect[1] + this->viewRect[3] - dialogRect[3] - 1; // dialogRect[1] -= 25;
			n2 = Canvas::PLAYER_DLG_COLOR;
			break;
		}
		case 1:
		case 6: {
			n2 = 0xFF002864;
			break;
		}
	}

	if ((this->dialogFlags & Enums::DLG_FLAG_GAME) || (this->dialogFlags & Enums::DLG_FLAG_YESNO)) {
		this->m_dialogButtons->GetButton(3)->drawButton = true;
	}
	else
	{
		this->m_dialogButtons->GetButton(3)->drawButton = false;
	}

	int currentDialogLine = 0;
	if (this->dialogStyle == Enums::DLG_STYLE_HELP || this->dialogStyle == Enums::DLG_STYLE_SPECIAL) {
		currentDialogLine = 1;
		graphics->setColor(n2);
		graphics->fillRect(dialogRect[0], dialogRect[1], dialogRect[2], dialogRect[3]);
		graphics->setColor(color2);
		graphics->fillRect(dialogRect[0], dialogRect[1] - fontHeight - 2, dialogRect[2], fontHeight + 2);
		graphics->setColor(color);
		graphics->drawRect(dialogRect[0], dialogRect[1] - fontHeight - 2, dialogRect[2] - 1, fontHeight + 2);
		graphics->drawRect(dialogRect[0], dialogRect[1], dialogRect[2] - 1, dialogRect[3]);

		this->m_dialogButtons->GetButton(8)->SetTouchArea(*dialogRect, dialogRect[1] - fontHeight - 2, dialogRect[2], fontHeight + dialogRect[3] + 2);

		if (this->specialLootIcon != -1) {
			graphics->drawRegion(app->hud->imgActions, 0, 18 * this->specialLootIcon, 18, 18, dialogRect[0], dialogRect[1] - fontHeight - 1, 0, 0, 0);
			graphics->drawRegion(app->hud->imgActions, 0, 18 * this->specialLootIcon, 18, 18, dialogRect[2] - 18, dialogRect[1] - fontHeight - 1, 0, 0, 0);
		}

		graphics->drawString(this->dialogBuffer, this->SCR_CX, dialogRect[1] - fontHeight, 1, this->dialogIndexes[0], this->dialogIndexes[1]);
	}
	else if (this->dialogStyle == Enums::DLG_STYLE_CHEST) {
		graphics->setColor(n2);
		graphics->fillRect(dialogRect[0], dialogRect[1], dialogRect[2], dialogRect[3]);
		graphics->setColor(color);
		graphics->drawRect(dialogRect[0], dialogRect[1], dialogRect[2] - 1, dialogRect[3]);
		if (this->dialogItem != nullptr) {
			currentDialogLine = 1;
			graphics->setColor(n2);
			graphics->fillRect(dialogRect[0], dialogRect[1] - 12, dialogRect[2], 12);
			graphics->setColor(color);
			graphics->drawRect(dialogRect[0], dialogRect[1] - 12, dialogRect[2] - 1, 12);
			graphics->drawString(this->dialogBuffer, dialogRect[0] + (dialogRect[2] + 10 - 2 >> 1), dialogRect[1] - 5, 3, this->dialogIndexes[0], this->dialogIndexes[1]);

			this->m_dialogButtons->GetButton(8)->SetTouchArea(*dialogRect, dialogRect[1] - 12, dialogRect[2], dialogRect[3] + 12);
		}
		else {
			this->m_dialogButtons->GetButton(8)->SetTouchArea(*dialogRect, dialogRect[1], dialogRect[2], dialogRect[3]);
		}
	}
	else if (this->dialogStyle != Enums::DLG_STYLE_SCROLL) {
		graphics->setColor(n2);
		graphics->fillRect(dialogRect[0], dialogRect[1], dialogRect[2], dialogRect[3]);
		graphics->setColor(color);
		graphics->drawRect(dialogRect[0], dialogRect[1], dialogRect[2] - 1, dialogRect[3]);

		this->m_dialogButtons->GetButton(8)->SetTouchArea(*dialogRect, dialogRect[1], dialogRect[2], dialogRect[3]);

		if (this->dialogStyle == Enums::DLG_STYLE_PLAYER) {
			int n6;
			int n5 = n6 = dialogRect[1] + 1;
			int n7 = n6 + (dialogRect[3] - 1);
			while (++n5 < n7) {
				int n8 = 96 + ((256 - (n5 - n6 << 8) / (n7 - n6)) * 160 >> 8);
				graphics->setColor((((n2 & 0xFF00FF00) >> 8) * n8 & 0xFF00FF00) | ((n2 & 0xFF00FF) * n8 >> 8 & 0xFF00FF) & 0xFF00DE);
				graphics->drawLine(dialogRect[0] + 1, n5, dialogRect[0] + (dialogRect[2] - 2), n5);
			}
			graphics->drawImage(this->imgChatHook_Player, this->SCR_CX + 10, dialogRect[1] + dialogRect[3], 0, 0, 0);

			int width = app->hud->imgPortraitsSM->width;
			int height = app->hud->imgPortraitsSM->height / 2;
			if (this->currentDialogLine < 2) {
				graphics->drawRegion(app->hud->imgPortraitsSM, 0, (this->currentDialogLine * 12) + 5, width, height - (this->currentDialogLine * 12) - 5, dialogRect[0] + 2, dialogRect[1] + 1, 0, 0, 0);
			}
		}
		else if (this->dialogStyle == Enums::DLG_STYLE_MONSTER) {
			if (this->dialogFlags & Enums::DLG_FLAG_INTERROGATE) {
				graphics->drawRegion(this->imgChatHook_Monster, 0, 12, 10, 6, this->SCR_CX - 64, dialogRect[1] + dialogRect[3] + 6, 36, 0, 0);
			}
			else {
				graphics->drawRegion(this->imgChatHook_Monster, 0, 0, 10, 6, this->SCR_CX - 64, dialogRect[1] + 1, 36, 0, 0);
			}
		}
		else if (this->dialogStyle == Enums::DLG_STYLE_NPC) {
			graphics->drawRegion(this->imgChatHook_NPC, 0, 0, 10, 6, this->SCR_CX - 64, dialogRect[1] + 1, 36, 0, 0);
		}
	}
	if (this->currentDialogLine < currentDialogLine) {
		this->currentDialogLine = currentDialogLine;
	}
	int n11 = dialogRect[1] + 2;
	for (int n12 = 0; n12 < this->dialogViewLines && this->currentDialogLine + n12 < this->numDialogLines; ++n12) {
		short n13 = this->dialogIndexes[(this->currentDialogLine + n12) * 2];
		short n14 = this->dialogIndexes[(this->currentDialogLine + n12) * 2 + 1];
		int n15 = 0;
		if (n12 == this->dialogTypeLineIdx) {
			n15 = (app->time - this->dialogLineStartTime) / 25;
			if (n15 >= n14) {
				n15 = n14;
				++this->dialogTypeLineIdx;
				this->dialogLineStartTime = app->time;
			}
		}
		else if (n12 < this->dialogTypeLineIdx) {
			n15 = n14;
		}
		graphics->drawString(this->dialogBuffer, n, n11, 0, n13, n15);
		n11 += fontHeight;
	}
	int8_t b = this->OSC_CYCLE[app->time / 200 % 4];
	short n16 = app->game->scriptStateVars[Enums::CODEVAR_DIALOG_CHOICE];
	if (this->dialogFlags & Enums::DLG_FLAG_INTERROGATE) {
		int y = this->screenRect[3] - 214;
		int x = this->screenRect[0] + 5;
		int fHeignt = 23 - Applet::FONT_HEIGHT[app->fontType] / 2;

		Text* smallBuffer = app->localization->getSmallBuffer();
		Text* smallBuffer2 = app->localization->getSmallBuffer();
		Text* smallBuffer3 = app->localization->getSmallBuffer();

		app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::DIALOG_EXIT, smallBuffer);
		smallBuffer->dehyphenate();
		app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::INTERROGATE_HIT, smallBuffer2);
		smallBuffer2->dehyphenate();
		app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::INTERROGATE_BRIBE, smallBuffer3);
		smallBuffer3->dehyphenate();

		int strX = x + 8; // Old x + 8
		int strY = y + fHeignt; // Old y + 0
		int strH = 45;
		int strW = std::max(std::max(smallBuffer->getStringWidth(), smallBuffer2->getStringWidth()), smallBuffer3->getStringWidth());
		strW += 30;

		//------------------------------------------------------------------------
		int hColor = (this->m_dialogButtons->GetButton(0)->highlighted) ? 0xFF8A8A8A : 0xFF4A4A4A;
		graphics->fillRect(x, y, strW, strH, hColor);
		graphics->drawRect(x, y, strW, strH, -1);
		graphics->drawString(smallBuffer, strX, strY, 4);
		this->m_dialogButtons->GetButton(0)->SetTouchArea(x, y, strW, strH);

		if (n16 == 0) { // J2ME/BREW
			graphics->drawCursor(strX + strW + b - 8, strY, 0x18, false);
		}

		//------------------------------------------------------------------------
		strY = y + 50 + fHeignt;
		hColor = (this->m_dialogButtons->GetButton(1)->highlighted) ? 0xFF8A8A8A : 0xFF4A4A4A;
		graphics->fillRect(x, y + 50, strW, strH, hColor);
		graphics->drawRect(x, y + 50, strW, strH, -1);
		graphics->drawString(smallBuffer2, strX, strY, 4);
		this->m_dialogButtons->GetButton(1)->SetTouchArea(x, y + 50, strW, strH);

		if (n16 == 1) {// J2ME/BREW
			graphics->drawCursor(strX + strW + b - 8, strY, 0x18, false);
		}

		//------------------------------------------------------------------------
		strY = y + 100 + fHeignt;
		hColor = (this->m_dialogButtons->GetButton(2)->highlighted) ? 0xFF8A8A8A : 0xFF4A4A4A;
		graphics->fillRect(x, y + 100, strW, strH, hColor);
		graphics->drawRect(x, y + 100, strW, strH, -1);
		graphics->drawString(smallBuffer3, strX, strY, 4);
		this->m_dialogButtons->GetButton(2)->SetTouchArea(x, y + 100, strW, strH);

		if (n16 == 2) {// J2ME/BREW
			graphics->drawCursor(strX + strW + b - 8, strY, 0x18, false);
		}

		//------------------------------------------------------------------------
		smallBuffer->dispose();
		smallBuffer2->dispose();
		smallBuffer3->dispose();
	}
	else if (this->dialogFlags != Enums::DLG_FLAG_NONE && this->currentDialogLine >= this->numDialogLines - this->dialogViewLines) {
		short n19 = CodeStrings::DIALOG_EXIT;
		short n20 = CodeStrings::DIALOG_PLAY;
		int fHeignt = Applet::FONT_HEIGHT[app->fontType];

		Text* smallBuffer3 = app->localization->getSmallBuffer();
		if (this->dialogFlags & Enums::DLG_FLAG_YESNO) {
			n19 = CodeStrings::NO_LABEL;
			n20 = CodeStrings::YES_LABEL;
		}
		if ((this->dialogFlags & Enums::DLG_FLAG_GAME) || (this->dialogFlags & Enums::DLG_FLAG_YESNO)) {
			int n21 = this->dialogRect[1] + (fHeignt * (this->dialogViewLines - 1)) + 2;

			int v75 = fHeignt + 2;
			int v113 = Applet::FONT_HEIGHT[app->fontType];

			//------------------------------------------------------------------------
			smallBuffer3->setLength(0);
			app->localization->composeText(Strings::FILE_CODESTRINGS, n19, smallBuffer3);
			smallBuffer3->dehyphenate();
	
			int hColor = (this->m_dialogButtons->GetButton(3)->highlighted) ? ((n2 + 0x333333) | 0xFF000000) : n2;
			graphics->fillRect(96, n21, 96, v75, hColor);
			graphics->drawRect(96, n21, 96, v75, -1);
			graphics->drawString(smallBuffer3, 144, n21 + (v75 >> 1) + 2, 3);
			this->m_dialogButtons->GetButton(3)->SetTouchArea(96, n21 - 40, 96, v113 + 42);

			if (n16 == 0) { // J2ME/BREW
				graphics->drawCursor((128 - (smallBuffer3->getStringWidth() >> 1)) + b, n21 + (v113 >> 1) - 5, 0);
			}
			//------------------------------------------------------------------------
			smallBuffer3->setLength(0);
			app->localization->composeText(Strings::FILE_CODESTRINGS, n20, smallBuffer3);
			smallBuffer3->dehyphenate();
			hColor = (this->m_dialogButtons->GetButton(4)->highlighted) ? ((n2 + 0x333333) | 0xFF000000) : n2;
			graphics->fillRect(288, n21, 96, v75, hColor);
			graphics->drawRect(288, n21, 96, v75, -1);
			graphics->drawString(smallBuffer3, 336, n21 + (v75 >> 1) + 2, 3);
			this->m_dialogButtons->GetButton(4)->SetTouchArea(288, n21 - 40, 96, v113 + 42);

			if (n16 == 1) { // J2ME/BREW
				graphics->drawCursor((320 - (smallBuffer3->getStringWidth() >> 1)) + b, n21 + (v113 >> 1) - 5, 4);
			}
		}
		smallBuffer3->dispose();
	}

	if (this->numDialogLines <= this->dialogViewLines) {
		if (!(this->dialogFlags & Enums::DLG_FLAG_INTERROGATE) && !(this->dialogFlags & Enums::DLG_FLAG_GAME) && !(this->dialogFlags & Enums::DLG_FLAG_YESNO)) {
			this->m_dialogButtons->GetButton(7)->Render(graphics);
		}
	}
	else {
		int numDialogLines;
		if (this->currentDialogLine + this->dialogViewLines > this->numDialogLines) {
			numDialogLines = this->numDialogLines;
		}
		else {
			numDialogLines = this->currentDialogLine + this->dialogViewLines;
		}

		this->drawScrollBar(graphics, dialogRect[0] + dialogRect[2] - 1, dialogRect[1] + 2, dialogRect[3] - 4, this->currentDialogLine - currentDialogLine, numDialogLines - currentDialogLine, this->numDialogLines - currentDialogLine, this->dialogViewLines);

		if (this->numDialogLines - currentDialogLine > this->dialogViewLines) {
			if (this->currentDialogLine > 1) {
				this->m_dialogButtons->GetButton(5)->Render(graphics);
			}
			if (this->currentDialogLine < this->numDialogLines - this->dialogViewLines) {
				this->m_dialogButtons->GetButton(6)->Render(graphics);
			}
			else {
				this->m_dialogButtons->GetButton(7)->Render(graphics);
			}
		}
		else {
			this->m_dialogButtons->GetButton(7)->Render(graphics);
		}
	}

	this->m_dialogButtons->GetButton(8)->Render(graphics);
	this->clearLeftSoftKey();

#if 0
	if (this->dialogFlags > this->dialogViewLines) {
		int numDialogLines;
		if (this->currentDialogLine + this->dialogViewLines > this->numDialogLines) {
			numDialogLines = this->numDialogLines;
		}
		else {
			numDialogLines = this->currentDialogLine + this->dialogViewLines;
		}
		if (this->dialogStyle == 3) {
			this->drawScrollBar(graphics, dialogRect[0] + dialogRect[2] - 1, dialogRect[1] - 8, dialogRect[3] + 16, this->currentDialogLine - currentDialogLine, numDialogLines - currentDialogLine, this->numDialogLines - currentDialogLine, this->dialogViewLines);
		}
		else {
			this->drawScrollBar(graphics, dialogRect[0] + dialogRect[2] - 1, dialogRect[1] + 2, dialogRect[3] - 4, this->currentDialogLine - currentDialogLine, numDialogLines - currentDialogLine, this->numDialogLines - currentDialogLine, this->dialogViewLines);
		}
	}
	if (this->currentDialogLine > currentDialogLine) {
		this->setLeftSoftKey((short)0, (short)125);
	}
	else {
		//this->clearLeftSoftKey();
	}
#endif
}

void Canvas::automapState() {
	Applet* app = CAppContainer::getInstance()->app;
	app->game->updateLerpSprites();
	if (!this->automapDrawn && app->game->animatingEffects == 0) {
		this->updateView();
		this->repaintFlags &= ~Canvas::REPAINT_VIEW3D;
		if (this->state != Canvas::ST_AUTOMAP) {
			this->updateView();
		}
	}
	if (this->state == Canvas::ST_AUTOMAP || this->state == Canvas::ST_PLAYING) {
		this->drawPlayingSoftKeys();
	}
}

void Canvas::renderOnlyState() {
	Applet* app = CAppContainer::getInstance()->app;

	if (this->st_enabled) {
		this->viewAngle = (this->viewAngle + this->animAngle & 0x3FF);
		this->viewPitch = 0;
	}
	else {
		if (this->viewX == this->destX && this->viewY == this->destY && this->viewAngle == this->destAngle) {
			return;
		}
		if (this->viewX < this->destX) {
			this->viewX += this->animPos;
			if (this->viewX > this->destX) {
				this->viewX = this->destX;
			}
		}
		else if (this->viewX > this->destX) {
			this->viewX -= this->animPos;
			if (this->viewX < this->destX) {
				this->viewX = this->destX;
			}
		}
		if (this->viewY < this->destY) {
			this->viewY += this->animPos;
			if (this->viewY >this->destY) {
				this->viewY =this->destY;
			}
		}
		else if (this->viewY > this->destY) {
			this->viewY -= this->animPos;
			if (this->viewY < this->destY) {
				this->viewY = this->destY;
			}
		}
		if (this->viewZ < this->destZ) {
			++this->viewZ;
		}
		else if (this->viewZ > this->destZ) {
			--this->viewZ;
		}
		if (this->viewAngle < this->destAngle) {
			this->viewAngle += this->animAngle;
			if (this->viewAngle > this->destAngle) {
				this->viewAngle = this->destAngle;
			}
		}
		else if (this->viewAngle > this->destAngle) {
			this->viewAngle -= this->animAngle;
			if (this->viewAngle < this->destAngle) {
				this->viewAngle = this->destAngle;
			}
		}
		if (this->viewPitch < this->destPitch) {
			this->viewPitch += this->pitchStep;
			if (this->viewPitch > this->destPitch) {
				this->viewPitch = this->destPitch;
			}
		}
		else if (this->viewPitch > this->destPitch) {
			this->viewPitch -= this->pitchStep;
			if (this->viewPitch <this->destPitch) {
				this->viewPitch =this->destPitch;
			}
		}
	}
	this->lastFrameTime = app->time;
	app->render->render((this->viewX << 4) + 8, (this->viewY << 4) + 8, (this->viewZ << 4) + 8, this->viewAngle, 0, 0, 281);
	app->combat->drawWeapon(0, 0);
	this->repaintFlags |= (Canvas::REPAINT_HUD | Canvas::REPAINT_VIEW3D);
}

void Canvas::playingState() {
	Applet* app = CAppContainer::getInstance()->app;

	if (this->pushedWall && this->pushedTime <= app->gameTime) {
		app->combat->shiftWeapon(false);
		this->pushedWall = false;
	}
	if (app->player->ce->getStat(Enums::STAT_HEALTH) <= 0) {
		app->player->died();
		return;
	}
	if (app->hud->isShiftingCenterMsg()) {
		this->staleView = true;
	}
	if (this->knockbackDist == 0 && app->game->activePropogators == 0 && app->game->animatingEffects == 0 && app->game->monstersTurn != 0 && this->numHelpMessages == 0) {
		app->game->updateMonsters();
	}
	app->game->updateLerpSprites();
	this->updateAmbientSounds();
	this->updateView();
	if (this->state == Canvas::ST_LOADING || this->state == Canvas::ST_SAVING) {
		return;
	}
	if (this->state != Canvas::ST_PLAYING && this->state != Canvas::ST_DRIVING) {
		return;
	}
	this->repaintFlags |= Canvas::REPAINT_PARTICLES;
	if (!app->game->isCameraActive() || this->state == Canvas::ST_DRIVING) {
		this->repaintFlags |= Canvas::REPAINT_HUD;
		app->hud->repaintFlags |= 0xB;
		app->hud->update();
	}
	if (this->state == Canvas::ST_DRIVING || (!app->game->isCameraActive() && this->state == Canvas::ST_PLAYING) || this->state == Canvas::ST_AUTOMAP) {
		this->dequeueHelpDialog();
	}
}

void Canvas::menuState() {
	Applet* app = CAppContainer::getInstance()->app;

	short n = -1;
	int flags = app->menuSystem->items[app->menuSystem->selectedIndex].flags;
	int menu = app->menuSystem->menu;
	if (flags & 0x20) {
		n = 49;
	}
	else if (menu == Menus::MENU_END_RANKING || menu == Menus::MENU_LEVEL_STATS) {
		n = 42;
	}
	else if (menu == Menus::MENU_INGAME_BOOKREAD && app->menuSystem->peekMenu() == -1) {
		n = 39;
	}
	else if (app->menuSystem->type != Menus::MENUTYPE_HELP) {
		if (app->menuSystem->type != Menus::MENUTYPE_NOTEBOOK) {
			if (menu != Menus::MENU_SHOWDETAILS) {
				if (menu == Menus::MENU_INGAME_CONTROLS) { // MENU_SMS_MANUAL
					n = 48;
				}
				else if (app->menuSystem->items[app->menuSystem->selectedIndex].action != 0) {
					n = 127;
				}
			}
		}
	}

	if (menu != Menus::MENU_MAIN_MORE_GAMES) {
		this->clearSoftKeys();
		if (app->menuSystem->getStackCount() != 0 || app->menuSystem->goBackToStation || menu == Menus::MENU_MAIN_MINIGAME) {
			if (app->menuSystem->peekMenu() != 20) {
				this->setLeftSoftKey(Strings::FILE_MENUSTRINGS, MenuStrings::BACK_ITEM);
			}
		}
		else if (menu == Menus::MENU_INGAME || menu == Menus::MENU_INGAME_KICKING || menu == Menus::MENU_INGAME_SNIPER || menu == Menus::MENU_INGAME_HIGH_SCORES) {
			this->setLeftSoftKey(Strings::FILE_CODESTRINGS, CodeStrings::DIALOG_EXIT);
		}

		if (n != -1) {
			if (!app->menuSystem->changeValues) { // Old changeSfxVolume
				this->setRightSoftKey(Strings::FILE_CODESTRINGS, n);
			}
		}
		else if (app->menuSystem->menu == Menus::MENU_SHOWDETAILS) {
			this->setRightSoftKey(Strings::FILE_CODESTRINGS, CodeStrings::SKIP);
		}
	}

	this->repaintFlags |= Canvas::REPAINT_MENU;
}

void Canvas::dyingState() {
	Applet* app = CAppContainer::getInstance()->app;
	app->hud->repaintFlags = 32;
	if (app->time < this->deathTime + 750) {
		int n = (750 - (app->time - this->deathTime) << 16) / 750;
		this->viewZ = app->render->getHeight(this->destX, this->destY) + 16 + (20 * n >> 16);
		this->viewPitch = 96 + (-96 * n >> 16);
		int n2 = 16 + (-16 * n >> 16);
		this->updateView();
		this->renderScene(this->viewX, this->viewY, this->viewZ, this->viewAngle, this->viewPitch, n2, 281);
		this->repaintFlags |= (Canvas::REPAINT_HUD | Canvas::REPAINT_PARTICLES);
	}
	else if (app->time < this->deathTime + 2750) {
		if (!app->render->isFading()) {
			app->render->startFade(2000, 1);
		}
		this->renderScene(this->viewX, this->viewY, this->viewZ, this->viewAngle, this->viewPitch, 16, 281);
		this->repaintFlags |= (Canvas::REPAINT_HUD | Canvas::REPAINT_PARTICLES);
	}
	else {
		app->render->baseDizzy = (app->render->destDizzy = 0);
		app->menuSystem->setMenu(Menus::MENU_INGAME_DEAD);
	}
}

void Canvas::logoState() {
	Applet* app = CAppContainer::getInstance()->app;

	if (this->pacLogoIndex == -1 || app->upTimeMs - this->pacLogoTime > 2500) {
		if (this->pacLogoIndex == 0) {
			if (this->forceError) {
				this->imgStartupLogo->~Image();
				this->imgStartupLogo = nullptr;

				this->setState(Canvas::ST_ERROR);
				this->numEvents = 0;
				this->keyDown = false;
				this->keyDownCausedMove = false;
				this->ignoreFrameInput = 1;
			}
			else {
				this->backToMain(true);
			}
			return;
		}
		this->pacLogoIndex++;
		this->pacLogoTime = app->upTimeMs;
	}
	else {
		if (!app->sound->soundsLoaded) {
			app->sound->cacheSounds();
		}
	}

	if (this->imgStartupLogo == nullptr) {
		this->imgStartupLogo = app->loadImage("newLegal.bmp", true);
	}

	this->repaintFlags |= Canvas::REPAINT_STARTUP_LOGO;
}

void Canvas::drawScrollBar(Graphics* graphics, int i, int i2, int i3, int i4, int i5, int i6, int i7)
{
	Applet* app = CAppContainer::getInstance()->app;
	bool v9;
	int v12;
	int v15; 
	int v16;

	v9 = i7 < 0;
	if (i7) {
		v9 = i7 < i6;
	}
	if (v9)
	{
		v12 = i6 - i7;
		if (i6 - i7 < i4) {
			v12 = i4;
		}
		v15 = 3 * i3 / (4 * ((i7 + i6 - 1) / i7));
		v16 = ((i4 << 16) / (v12 << 8) * ((i3 - v15 - 14) << 8)) >> 16;
		if (i6 == i5) {
			v16 = i3 - 3 * i3 / (4 * ((i7 + i6 - 1) / i7)) - 14;
		}
		graphics->drawRegion(app->menuSystem->imgScroll, 0, 0, 7, 7, i, i2, 24, 0, 0);
		graphics->drawRegion(app->menuSystem->imgScroll, 0, 7, 7, 7, i, i3 + i2, 40, 0, 0);
		graphics->setColor(0xFFB3AA93);
		graphics->fillRect(i - 7, i2 + 7, 7, i3 - 14);
		graphics->setColor(0xFFE7CFAD);
		graphics->fillRect(i - 7, v16 + 7 + i2, 7, v15);
		graphics->setColor(0xFF000000);
		graphics->drawRect(i - 7, v16 + 7 + i2, 6, v15 - 1);
		graphics->drawRect(i - 7, i2, 6, i3 - 1);
	}
}

void Canvas::uncoverAutomap() {
	Applet* app = CAppContainer::getInstance()->app;
	if (!app->game->updateAutomap) {
		return;
	}
	int n = this->destX >> 6;
	int n2 = this->destY >> 6;
	if (n < 0 || n >= 32 || n2 < 0 || n2 >= 32) {
		return;
	}
	for (int i = n2 - 1; i <= n2 + 1; ++i) {
		if (i >= 0) {
			if (i < 31) {
				for (int j = n - 1; j <= n + 1; ++j) {
					if (j >= 0) {
						if (j < 31) {
							uint8_t b = app->render->mapFlags[i * 32 + j];
							if ((j == n && i == n2) || !(b & Canvas::BIT_AM_SECRET)) {
								app->render->mapFlags[i * 32 + j] |= Canvas::BIT_AM_VISITED;
							}
						}
					}
				}
			}
		}
	}
}

void Canvas::drawAutomap(Graphics* graphics, bool b) {
	Applet* app = CAppContainer::getInstance()->app;

	int n3 = 8;
	int n6 = 0x400000 / (n3 << 8);
	int n4 = 112;
	int n5 = 32;

	if (b) {
		graphics->drawRegion(this->imgNotebookBg, 0, 0, 480, 320, 0, 0, 0, 0, 0);

		int n7 = 0;
		for (int i = 0; i < 32; ++i) {
			for (int j = 0; j < 32; ++j) {
				uint8_t mapFlags = app->render->mapFlags[n7 + j];
				if (mapFlags & Canvas::BIT_AM_ENTRANCE) {
					graphics->setColor(0xFF00FF00);
					graphics->fillRect(n4 + n3 * j + this->screenRect[0], n5 + n3 * i + this->screenRect[1], n3, n3);
				}
				else if (mapFlags & Canvas::BIT_AM_VISITED) {
					if (mapFlags & Canvas::BIT_AM_EXIT) {
						graphics->setColor(0xFFFF0000);
						graphics->fillRect(n4 + n3 * j + this->screenRect[0], n5 + n3 * i + this->screenRect[1], n3, n3);
					}
					else if (mapFlags & Canvas::BIT_AM_LADDER) {
						graphics->setColor(0xFFFFFF00);
						graphics->fillRect(n4 + n3 * j, n5 + n3 * i, n3, n3);
						graphics->setColor(0xFF000000);
						for (int k = 0; k < n3; k += 2) {
							graphics->drawLine(n4 + n3 * j, n5 + n3 * i + k, n4 + n3 * j + n3, n5 + n3 * i + k);
						}
						graphics->drawLine(n4 + n3 * j, n5 + n3 * i, n4 + n3 * j, n5 + n3 * i + n3);
						graphics->drawLine(n4 + n3 * j + n3, n5 + n3 * i, n4 + n3 * j + n3, n5 + n3 * i + n3);
					}
					else if (!(mapFlags & Canvas::BIT_AM_WALL)) {
						graphics->setColor(0xFFE0CFB0);
						graphics->fillRect(n4 + (n3 * j) + this->screenRect[0], (n5 - 1) + (n3 * i) + this->screenRect[1] + 1, n3, n3);
						graphics->setColor(0xFF746C59);
					}
				}
			}
			n7 += 32;
		}

		graphics->setColor(0xFF746C59);
		for (int n10 = 0; n10 < app->render->numLines; ++n10) {
			int lineFlags = app->render->lineFlags[n10 >> 1] >> ((n10 & 0x1) << 2) & 0xF;
			if (lineFlags & Enums::LINE_FLAG_AUTOMAP) {
				int lineType = lineFlags & Enums::LINE_TYPE_MASK;
				if (lineType == Enums::LINE_TYPE_MAP || lineType == Enums::LINE_TYPE_LINE) {
					graphics->drawLine(n4 + app->render->lineXs[n10 << 1], n5 + app->render->lineYs[n10 << 1], n4 + app->render->lineXs[(n10 << 1) + 1], n5 + app->render->lineYs[(n10 << 1) + 1]);
				}
			}
		}

		int n13 = 0;
		for (int n14 = 0; n14 < 32; ++n14) {
			for (int n15 = 0; n15 < 32; ++n15) {
				uint8_t mapFlags = app->render->mapFlags[n13 + n15];
				if (!(mapFlags & (Canvas::BIT_AM_ENTRANCE | Canvas::BIT_AM_EXIT | Canvas::BIT_AM_LADDER))) {
					for (Entity* nextOnTile = app->game->entityDb[n13 + n15]; nextOnTile != nullptr; nextOnTile = nextOnTile->nextOnTile) {
						if (nextOnTile != &app->game->entities[1]) {
							if (nextOnTile != &app->game->entities[0]) {
								if (!(mapFlags & Canvas::BIT_AM_ENTRANCE)) {
									int sprite = nextOnTile->getSprite();
									int n16 = app->render->mapSpriteInfo[sprite];
									int n17 = (n16 & 0xFF00) >> 8;
									if (0x0 != (n16 & 0x200000)) {
										if (0x0 == (n16 & 0x10000)) {
											int color = 0;
											bool b4 = false;
											bool b5 = false;
											int n18 = Canvas::BIT_AM_SECRET;
											if (nextOnTile->def->eType == Enums::ET_DOOR) {
												short tileIndex = nextOnTile->def->tileIndex;
												if (tileIndex == 271 || tileIndex == 272) {
													color = 0xFF00C0FF;
												}
												else if (tileIndex == 273 || tileIndex == 274) {
													color = 0xFFFF8400;
												}
												else {
													color = 0xFF4A3018;
												}
												b5 = true;
											}
											else if ((n16 & 0x400000) != 0x0) {
												color = 0xFF746C59;
												b5 = true;
											}
											else if (nextOnTile->def->eType == Enums::ET_NONOBSTRUCTING_SPRITEWALL) {
												color = 0xFF8D8068;
												b5 = true;
											}
											else if (nextOnTile->def->eType == Enums::ET_NPC) {
												b4 = true;
												color = 0xFF0000FF;
												n18 = Canvas::BIT_AM_VISITED;
											}
											else if (nextOnTile->def->eType == Enums::ET_ATTACK_INTERACTIVE && n17 == 0) {
												if (nextOnTile->def->eSubType != Enums::INTERACT_CHICKEN) {
													color = 0xFF8000FF;
												}
											}
											else if (nextOnTile->def->eType == Enums::ET_MONSTER) {
												if (nextOnTile->monster->flags & Enums::MFLAG_NPC_MONSTER) {
													b4 = true;
													color = 0xFF0000FF;
												}
												else {
													color = 0xFFFF8000;
												}
												n18 = Canvas::BIT_AM_VISITED;
											}
											else if (nextOnTile->def->eType == Enums::ET_DECOR) {
												color = 0xFF8D8068;
												if (app->player->god && n17 == 0) {
													if (nextOnTile->def->tileIndex == 153 || nextOnTile->def->tileIndex == 121 || nextOnTile->def->tileIndex == 158) {
														color = 0xFF00FFEA;
													}
												}
												else if (nextOnTile->def->tileIndex == 173 || nextOnTile->def->tileIndex == 180) {
													color = 0;
												}
											}
											else if (app->player->god && nextOnTile->def->eType == Enums::ET_ITEM && nextOnTile->def->eSubType != Enums::IT_MONEY) {
												color = 0xFF00FFEA;
											}
											if (color != 0 && ((mapFlags & n18) != 0x0 || !(n18 & Canvas::BIT_AM_VISITED))) {
												graphics->setColor(color);
												if ((n16 & 0xF000000) != 0x0) {
													int n20;
													int n19 = n20 = app->render->mapSprites[app->render->S_X + sprite];
													int n22;
													int n21 = n22 = app->render->mapSprites[app->render->S_Y + sprite];
													if ((n16 & 0x3000000) != 0x0) {
														n20 -= 32;
														n19 += 32;
													}
													else {
														n22 -= 32;
														n21 += 32;
													}
													int n23 = ((n20 << 16) / n6) + 1 + 128 >> 8;
													int n24 = ((n19 << 16) / n6) + 128 >> 8;
													int n25 = ((n22 << 16) / n6) + 128 >> 8;
													int n26 = ((n21 << 16) / n6) + 128 >> 8;
													if (b5) {
														graphics->drawLine(n4 + n23, n5 + n25, n4 + n24, n5 + n26);
													}
													else {
														graphics->fillRect(n4 + (n23 + n24 >> 1) - (n3 / 4), n5 + (n25 + n26 >> 1) - (n3 / 4), n3 / 2, n3 / 2);
													}
												}
												else if (b4) {
													graphics->fillRect(n4 + (n3 * n15) + (n3 / 4), n5 + (n3 * n14) + (n3 / 4), (n3 / 2) + 2, (n3 / 2) + 2);
												}
												else {
													graphics->fillRect(n4 + (n3 * n15) + (n3 / 4) + 1, n5 + (n3 * n14) + (n3 / 4) + 1, (n3 / 2), (n3 / 2));
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
			n13 += 32;
		}
	}

#if 0
	int n27 = 0;
	for (int n28 = 0; n28 < 32; ++n28) {
		for (int n29 = 0; n29 < 32; ++n29) {
			if ((app->render->mapFlags[n27 + n29] & 0x8) != 0x0) {
				graphics->setColor(0xFF526F8B);
				graphics->fillRect(n4 + 8 * n29 + this->screenRect[0], n5 + 8 * n28 + this->screenRect[1] + 1, 8, 8);
			}
		}
		n27 += 32;
	}
#endif

	for (int n30 = 0; n30 < app->player->numNotebookIndexes; ++n30) {
		if (!app->player->isQuestDone(n30)) {
			if (!app->player->isQuestFailed(n30)) {
				int n31 = app->player->notebookPositions[n30] >> 5 & 0x1F;
				int n32 = app->player->notebookPositions[n30] & 0x1F;
				if (n31 + n32 != 0 && (app->render->mapFlags[n32 * 32 + n31] & Canvas::BIT_AM_VISITED) != 0x0) {
					graphics->setColor(((app->time / 1024 & 0x1) == 0x0) ? 0xFFFF0000 : 0xFF00FF00);
					Entity* mapEntity = app->game->findMapEntity(n31 << 6, n32 << 6, 32);
					if (nullptr != mapEntity) {
						int sprite2 = mapEntity->getSprite();
						int n33 = app->render->mapSpriteInfo[sprite2];
						int n35;
						int n34 = n35 = app->render->mapSprites[app->render->S_X + sprite2];
						int n37;
						int n36 = n37 = app->render->mapSprites[app->render->S_Y + sprite2];
						if ((n33 & 0x3000000) != 0x0) {
							n35 -= 32;
							n34 += 32;
						}
						else {
							n37 -= 32;
							n36 += 32;
						}
						graphics->drawLine(n4 + ((n35 << 16) / n6 + 1 + 128 >> 8), n5 + ((n37 << 16) / n6 + 128 >> 8), n4 + ((n34 << 16) / n6 + 128 >> 8), n5 + ((n36 << 16) / n6 + 128 >> 8));
					}
					else {
						graphics->fillRect(n4 + n3 * n31 + n3 / 4, n5 + n3 * n32 + n3 / 4, n3 / 2 + 2, n3 / 2 + 2);
					}
				}
			}
		}
	}

	if (app->time > this->automapBlinkTime) {
		this->automapBlinkTime = app->time + 333;
		this->automapBlinkState = !this->automapBlinkState;
	}

	int n38 = 0;
	switch (this->destAngle & 0x3FF) {
	case Enums::ANGLE_NORTH: {
		n38 = 0;
		break;
	}
	case Enums::ANGLE_SOUTH: {
		n38 = 1;
		break;
	}
	case Enums::ANGLE_EAST: {
		n38 = 2;
		break;
	}
	case Enums::ANGLE_WEST: {
		n38 = 3;
		break;
	}
	}
	int n39 = n38 * 4;
	if (this->automapBlinkState) {
		n39 += 16;
	}
	int n40 = n4 + ((n3 * (this->viewX - 32)) / 64) + (n3 / 2);
	int n41 = n5 + ((n3 * (this->viewY - 32)) / 64) + (n3 / 2);
	if (n41 < this->screenRect[1] + this->screenRect[3]) {
		graphics->drawRegion(this->imgMapCursor, 0, n39, 4, 4, n40, n41, 3, 0, 0);
	}

	Text *LargeBuffer = app->localization->getLargeBuffer();

	LargeBuffer->setLength(0);
	app->localization->composeText(this->softKeyLeftID, LargeBuffer);
	LargeBuffer->dehyphenate();
	app->setFontRenderMode(2);
	if (this->m_softKeyButtons->GetButton(19)->highlighted) {
		app->setFontRenderMode(0);
	}
	graphics->drawString(LargeBuffer, 15, 310, 36); // old 20, 316, 36
	app->setFontRenderMode(0);

	LargeBuffer->setLength(0);
	app->localization->composeText(this->softKeyRightID, LargeBuffer);
	LargeBuffer->dehyphenate();
	app->setFontRenderMode(2);
	if (this->m_softKeyButtons->GetButton(20)->highlighted) {
		app->setFontRenderMode(0);
	}
	graphics->drawString(LargeBuffer, 465, 310, 40);// old 470, 316, 40
	app->setFontRenderMode(0);
	LargeBuffer->dispose();
}

void Canvas::closeDialog(bool skipDialog) {
	Applet* app = CAppContainer::getInstance()->app;

	this->dialogClosing = true;
	this->specialLootIcon = -1;
	this->showingLoot = false;
	app->player->unpause(app->time - this->dialogStartTime);
	this->dialogBuffer->dispose();
	this->dialogBuffer = nullptr;
	if (this->numHelpMessages == 0 && (this->dialogStyle == Enums::DLG_STYLE_SCROLL || this->dialogStyle == Enums::DLG_STYLE_CHEST || (this->dialogStyle == Enums::DLG_STYLE_HELP && this->dialogType == 1))) {
		app->game->queueAdvanceTurn = true;
	}

	if (app->render->chatZoom && app->combat->curTarget != nullptr) {
		int sprite = app->combat->curTarget->getSprite();
		app->render->mapSpriteInfo[sprite] = ((app->render->mapSpriteInfo[sprite] & 0xFFFF00FF) | 0x0);
		app->render->chatZoom = false;
		app->combat->curTarget = nullptr;
	}

	if (this->oldState == Canvas::ST_DRIVING) {
		app->game->activeCameraTime = app->gameTime - app->game->activeCameraTime;
		this->setState(Canvas::ST_DRIVING);
	}
	else if (app->game->isCameraActive()) {
		app->game->activeCameraTime = app->gameTime - app->game->activeCameraTime;
		this->setState(Canvas::ST_CAMERA);
	}
	else if (this->oldState == Canvas::ST_COMBAT && !this->combatDone) {
		this->setState(Canvas::ST_COMBAT);
	}
	else {
		this->setState(Canvas::ST_PLAYING);
	}
	if (this->dialogResumeMenu) {
		this->setState(Canvas::ST_MENU);
	}
	this->dialogClosing = false;
	if (this->dialogResumeScriptAfterClosed) {
		app->game->skipDialog = skipDialog;
		this->dialogThread->run();
		app->game->skipDialog = false;
	}

	this->repaintFlags |= Canvas::REPAINT_VIEW3D;
}

void Canvas::prepareDialog(Text* text, int dialogStyle, int dialogFlags) {
	Applet* app = CAppContainer::getInstance()->app;
	int i = 0;
	int n = 0;
	Text* smallBuffer = app->localization->getSmallBuffer();
	if (dialogStyle == Enums::DLG_STYLE_SCROLL) {
		this->dialogViewLines = 6;
	}
	else if (dialogStyle == Enums::DLG_STYLE_PLAYER) {
		this->dialogViewLines = 3;
	}
	else if (dialogStyle == Enums::DLG_STYLE_HELP) {
		this->dialogViewLines = 3;
	}
	else {
		this->dialogViewLines = 4;
	}
	if (dialogStyle == Enums::DLG_STYLE_NPC || (dialogStyle == Enums::DLG_STYLE_MONSTER && ((dialogFlags & Enums::DLG_FLAG_INTERROGATE) || (dialogFlags & Enums::DLG_FLAG_GAME)))) {
		this->updateFacingEntity = true;
		Entity* facingEntity = app->player->facingEntity;
		if (facingEntity != nullptr && facingEntity->def != nullptr && (facingEntity->def->eType == Enums::ET_MONSTER || facingEntity->def->eType == Enums::ET_NPC)) {
			int n2 = 160;
			app->render->chatZoom = true;
			app->combat->curTarget = facingEntity;
			int sprite = facingEntity->getSprite();
			if (facingEntity->def->eType == Enums::ET_MONSTER) {
				n2 = 128;
			}
			app->render->mapSpriteInfo[sprite] = ((app->render->mapSpriteInfo[sprite] & 0xFFFF00FF) | n2 << 8);
			app->game->scriptStateVars[Enums::CODEVAR_DIALOG_CHOICE] = 0;
		}
	}
	if (this->dialogBuffer == nullptr) {
		this->dialogBuffer = app->localization->getLargeBuffer();
	}
	else {
		this->dialogBuffer->setLength(0);
	}
	if ((dialogFlags & Enums::DLG_FLAG_GAME) || (dialogFlags & Enums::DLG_FLAG_YESNO)) {
		app->game->scriptStateVars[Enums::CODEVAR_DIALOG_CHOICE] = 1;
		smallBuffer->setLength(0);
		app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::DOUBLE_SPACED, smallBuffer);
		text->append(smallBuffer);
	}
	if (dialogStyle == Enums::DLG_STYLE_PLAYER) {
		if (app->player->statusEffects[(Enums::OFS_STATUSEFFECT_TURNS + Enums::STATUS_EFFECT_DRUNK)] > 0) {
			smallBuffer->setLength(0);
			app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::PLAYER_DRUNK, smallBuffer);
			text->append(smallBuffer);
		}

		int n2 = this->dialogMaxChars;
		//int n3 = Hud.imgPortraitsSM.getWidth() / 9 + 1;
		smallBuffer->setLength(0);
		smallBuffer->append("     ");
		int n3 = smallBuffer->length();
		for (int j = 0; j < 2; ++j) {
			Text* dialogBuffer = this->dialogBuffer;
			int length;
			int k;

			k = 0;
			dialogBuffer->append(smallBuffer);
			length = dialogBuffer->length();
			dialogBuffer->append(text, k);
			k += dialogBuffer->wrapText(length, n2 - n3, 1, '|');

			dialogBuffer->append(smallBuffer);
			length = dialogBuffer->length();
			dialogBuffer->append(text, k);
			k += dialogBuffer->wrapText(length, n2 - n3, 1, '|');

			length = dialogBuffer->length();
			dialogBuffer->append(text, k);
			k += dialogBuffer->wrapText(length, n2 , -1, '|');

			if (j == 0) {
				if (dialogBuffer->getNumLines() <= 3) {
					break;
				}
				dialogBuffer->setLength(0);
				n2 = this->dialogWithBarMaxChars;
			}
		}
	}
	else if (dialogStyle == Enums::DLG_STYLE_SCROLL) {
		this->dialogBuffer->append(text);
		this->dialogBuffer->wrapText(this->scrollMaxChars);
		if (this->dialogBuffer->getNumLines() > this->dialogViewLines) {
			this->dialogBuffer->setLength(0);
			this->dialogBuffer->append(text);
			this->dialogBuffer->wrapText(this->scrollWithBarMaxChars);
		}
	}
	else {
		this->dialogBuffer->append(text);
		this->dialogBuffer->wrapText(this->dialogMaxChars);
		int numLines = this->dialogBuffer->getNumLines();
		if (dialogStyle == Enums::DLG_STYLE_HELP || dialogStyle == Enums::DLG_STYLE_SPECIAL) {
			--numLines;
		}
		if (numLines > this->dialogViewLines) {
			this->dialogBuffer->setLength(0);
			this->dialogBuffer->append(text);
			this->dialogBuffer->wrapText(this->dialogWithBarMaxChars);
		}
	}
	int length2 = this->dialogBuffer->length();
	this->numDialogLines = 0;
	while (i < length2) {
		if (this->dialogBuffer->charAt(i) == '|') {
			this->dialogIndexes[this->numDialogLines * 2 + 0] = (short)n;
			this->dialogIndexes[this->numDialogLines * 2 + 1] = (short)(i - n);
			this->numDialogLines++;
			n = i + 1;
		}
		++i;
	}
	this->dialogIndexes[this->numDialogLines * 2 + 0] = (short)n;
	this->dialogIndexes[this->numDialogLines * 2 + 1] = (short)(length2 - n);
	this->numDialogLines++;
	this->currentDialogLine = 0;
	this->dialogLineStartTime = app->time;
	this->dialogTypeLineIdx = 0;
	this->dialogStartTime = app->time;
	this->dialogItem = nullptr;
	this->dialogFlags = dialogFlags;
	this->dialogStyle = dialogStyle;
	smallBuffer->dispose();

	if (dialogStyle == Enums::DLG_STYLE_HELP) {
		app->sound->playSound(1009, 0, 3, 0);
	}
}

void Canvas::startDialog(ScriptThread* scriptThread, short n, int dialogStyle, int dialogFlags) {
	this->startDialog(scriptThread, Strings::FILE_BOOKSTRINGS, n, dialogStyle, dialogFlags, false);
}

void Canvas::startDialog(ScriptThread* scriptThread, short n, short n2, int dialogStyle, int dialogFlags, bool b) {
	Applet* app = CAppContainer::getInstance()->app;

	Text* largeBuffer = app->localization->getLargeBuffer();
	app->localization->composeText(n, n2, largeBuffer);
	this->startDialog(scriptThread, largeBuffer, dialogStyle, dialogFlags, b);
	largeBuffer->dispose();
}

void Canvas::startDialog(ScriptThread* scriptThread, Text* text, int dialogStyle, int dialogFlags) {
	this->startDialog(scriptThread, text, dialogStyle, dialogFlags, false);
}

void Canvas::startDialog(ScriptThread* dialogThread, Text* text, int dialogStyle, int dialogFlags, bool dialogResumeScriptAfterClosed) {
	this->dialogResumeScriptAfterClosed = dialogResumeScriptAfterClosed;
	this->dialogResumeMenu = false;
	this->dialogThread = dialogThread;
	this->readyWeaponSound = 0;
	this->prepareDialog(text, dialogStyle, dialogFlags);
	this->setState(Canvas::ST_DIALOG);
}

void Canvas::renderScene(int viewX, int viewY, int viewZ, int viewAngle, int viewPitch, int viewRoll, int viewFov) {
	Applet* app = CAppContainer::getInstance()->app;

	{ // J2ME
		//this->staleView = true;
		//if (!this->staleView && (this->staleTime == 0 || app->time < this->staleTime)) {
			//return;
		//}
	}

	this->staleView = false;
	this->staleTime = 0;
	this->lastFrameTime = app->time;
	this->beforeRender = app->upTimeMs;
	app->render->render((viewX << 4) + 8, (viewY << 4) + 8, (viewZ << 4) + 8, viewAngle, viewPitch, viewRoll, viewFov);
	this->afterRender = app->upTimeMs;
	++this->renderSceneCount;
	if (!this->isZoomedIn) {
		app->combat->drawWeapon(this->shakeX, this->shakeY);
	}
	this->repaintFlags |= Canvas::REPAINT_VIEW3D;
}

void Canvas::startSpeedTest(bool b) {
	this->renderOnly = true;
	this->st_enabled = true;
	this->st_count = 1;
	for (int i = 0; i < Canvas::SPD_NUM_FIELDS; ++i) {
		this->st_fields[i] = 0;
	}
	if (!b) {
		this->animAngle = 4;
		this->destAngle = this->viewAngle;
		this->setState(Canvas::ST_BENCHMARK);
	}
}

void Canvas::backToMain(bool b) {
	Applet* app = CAppContainer::getInstance()->app;

	if (this->state != 9) {
		app->sound->soundStop();
	}

	this->loadMapID = 0;
	app->freeStaticImages();
	app->player->reset();
	app->game->unloadMapData();
	app->render->unloadMap();
	app->render->endFade();

	// startGame() releases the title images while a map is running.  Calling an
	// explicit destructor through those null pointers crashes real PSP hardware
	// when Save & Exit completes.
	if (app->menuSystem->background == app->menuSystem->imgMainBG) {
		app->menuSystem->background = nullptr;
	}
	if (app->menuSystem->imgMainBG != nullptr) {
		app->menuSystem->imgMainBG->~Image();
	}
	app->menuSystem->imgMainBG = app->loadImage("logo.bmp", true);

	if (app->menuSystem->imgLogo != nullptr) {
		app->menuSystem->imgLogo->~Image();
	}
	app->menuSystem->imgLogo = app->loadImage("logo2.bmp", true);
	app->menuSystem->background = app->menuSystem->imgMainBG;

	if (b) {
		this->clearEvents(1);
		if (this->skipIntro) {
			app->player->reset();
			app->render->unloadMap();
			app->game->unloadMapData();
			this->loadMap(this->startupMap, true, false);
		}
		else if (app->localization->selectLanguage) {
			app->menuSystem->clearStack();
			app->menuSystem->pushMenu(20, app->menuSystem->selectedIndex, 0, 0, 0);
			app->menuSystem->setMenu(Menus::MENU_SELECT_LANGUAGE);
		}
		else {
			app->sound->playSound(1077, 0, 6, false);
			this->setState(Canvas::ST_INTRO_MOVIE);
			this->repaintFlags |= Canvas::REPAINT_MENU;
		}
	}
	else {
		app->menuSystem->setMenu(Menus::MENU_MAIN);
	}
	
}

void Canvas::drawPlayingSoftKeys() {
	Applet* app = CAppContainer::getInstance()->app;

	/*if (this->isZoomedIn) { // J2ME
		this->setSoftKeys(Strings::FILE_CODESTRINGS, CodeStrings::MENU, Strings::FILE_CODESTRINGS, CodeStrings::MAP);
	}
	else */if (this->state == Canvas::ST_AUTOMAP) {
		this->setSoftKeys(Strings::FILE_CODESTRINGS, CodeStrings::MENU, Strings::FILE_CODESTRINGS, CodeStrings::LEAVE);
	}
	else {
		this->setSoftKeys(Strings::FILE_CODESTRINGS, CodeStrings::MENU, Strings::FILE_CODESTRINGS, CodeStrings::MAP);
		if (this->isChickenKicking) {
			this->clearRightSoftKey();
		}
	}
}

void Canvas::changeStoryPage(int i) {
	Applet* app = CAppContainer::getInstance()->app;
	if (i < 0 && this->storyPage == 0) {
		if (this->state == Canvas::ST_INTRO) {
			app->canvas->backToMain(false);
		}
	}
	else {
		this->storyPage += i;
	}
}

void Canvas::drawStory(Graphics* graphics)
{
	Applet* app = CAppContainer::getInstance()->app;

	Text* this_00;
	Text* this_01;
	short i2;
	Text* text;

	if (this->storyPage < this->storyTotalPages) {

		graphics->eraseRgn(this->displayRect);
		graphics->drawImage(this->imgTravelBG, 0, 0, 0, 0, 0);

		graphics->setClipRect(0, this->scrollingTextFontHeight, 480, this->scrollingTextSpacingHeight);
		graphics->drawString(this->dialogBuffer, this->SCR_CX, 
			this->scrollingTextFontHeight - this->storyPage * this->scrollingTextSpacingHeight,
			this->scrollingTextSpacing, 1, 0, -1);
		graphics->clearClipRect();

		this_00 = app->localization->getLargeBuffer();
		if ((this->state != Canvas::ST_EPILOGUE) || (0 < this->storyPage)) {
			this_00->setLength(0);
			app->localization->composeText(Strings::FILE_MENUSTRINGS, MenuStrings::BACK_ITEM, this_00); // "Back"
			this_00->dehyphenate();
			app->setFontRenderMode(10);
			if (this->m_storyButtons->GetButton(0)->highlighted != false) {
				app->setFontRenderMode(0);
			}
			graphics->drawString(this_00, 17, 310, 36); // Old -> 2, 319, 36
			app->setFontRenderMode(0);
		}

		this_00->setLength(0);
		this_01 = app->localization->getSmallBuffer();
		if (this->storyPage < this->storyTotalPages + -1) {
			app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::MORE, this_00); // more

			i2 = CodeStrings::SKIP; // Skip
			text = this_01;
			this->m_storyButtons->GetButton(1)->touchAreaDrawing.x = 420; // [GEC], ajusta la posicion X de la caja de toque
			this->m_storyButtons->GetButton(1)->touchAreaDrawing.w = 60; // [GEC], ajusta el ancho de la caja de toque
		}
		else {
			i2 = CodeStrings::CONTINUE; // Continue
			text = this_00;
			this->m_storyButtons->GetButton(1)->touchAreaDrawing.x = 380; // [GEC], ajusta la posicion X de la caja de toque
			this->m_storyButtons->GetButton(1)->touchAreaDrawing.w = 100; // [GEC], ajusta el ancho de la caja de toque
		}

		app->localization->composeText(Strings::FILE_CODESTRINGS, i2, text);
		#ifdef WOLFENRPG_PSP
		this_00->setLength(0);
		this_00->append(this->storyPage < this->storyTotalPages - 1 ? (char*)"X Next" : (char*)"X Continue");
		this_01->setLength(0);
		this_01->append((char*)"TRIANGLE Skip");
		#endif
		this_00->dehyphenate();
		this_01->dehyphenate();
		app->setFontRenderMode(10);
		if (this->m_storyButtons->GetButton(1)->highlighted != false) {
			app->setFontRenderMode(0);
		}
		graphics->drawString(this_00, 463, 310, 40); // Old -> 478, 319, 40);
		app->setFontRenderMode(0);

		app->setFontRenderMode(10);
		if (this->m_storyButtons->GetButton(2)->highlighted != false) {
			app->setFontRenderMode(0);
		}
		graphics->drawString(this_01, 463, 10, 8); // Old -> 478, 1, 8);

		app->setFontRenderMode(0);
		this_00->dispose();
		this_01->dispose();

		//graphics->drawString(this->dialogBuffer, this->storyX, this->storyY, 21, 0, this->storyIndexes[0],
			//this->storyIndexes[1] - this->storyIndexes[0]);
	}
}

void Canvas::dequeueHelpDialog() {
	this->dequeueHelpDialog(false);
}

void Canvas::dequeueHelpDialog(bool b) {
	Applet* app = CAppContainer::getInstance()->app;

	if (this->numHelpMessages == 0) {
		return;
	}
	if (this->state == Canvas::ST_DIALOG || this->dialogClosing) {
		return;
	}
	if (!b && this->state != Canvas::ST_PLAYING && this->state != Canvas::ST_DRIVING && this->state != Canvas::ST_AUTOMAP && app->game->monstersTurn == 0) {
		return;
	}
	if (app->game->secretActive) {
		return;
	}

	int dialogStyle = Enums::DLG_STYLE_HELP;
	int dialogFlags = Enums::DLG_FLAG_NONE;
	Text* largeBuffer = app->localization->getLargeBuffer();
	this->dialogType = this->helpMessageTypes[0];
	void* object = this->helpMessageObjs[0];
	short n3 = this->helpMessageThreads[0];
	if (this->dialogType == Canvas::HELPMSG_TYPE_ITEM) {
		EntityDef* entityDef = (EntityDef*)object;
		uint8_t eSubType = entityDef->eSubType;
		short n4 = -1;
		if (eSubType == Enums::IT_INVENTORY) {
			if (entityDef->parm >= 0 && entityDef->parm < 15) {
				n4 = CodeStrings::SYRINGE_MENU_HELP;
			}
			else if ((entityDef->parm >= 16 && entityDef->parm < 18)) {
				n4 = CodeStrings::HEALTH_MENU_HELP;
			}
			else if (entityDef->parm == 18) {
				n4 = CodeStrings::JOURNAL_MENU_HELP;
			}
			else if (entityDef->parm == 22) {
				n4 = CodeStrings::OTHER_MENU_HELP;
			}
		}
		else if (eSubType == Enums::IT_WEAPON) {
			n4 = CodeStrings::WEAPON_MENU_HELP;
		}
		else if (eSubType != Enums::IT_ARMOR && eSubType != Enums::IT_AMMO) {
			app->Error(0); // ERR_DEQUEUEHELP
			return;
		}
		if (entityDef != nullptr) {
			app->localization->composeText(Strings::FILE_ENTITYSTRINGS, entityDef->longName, largeBuffer);
			largeBuffer->append("|");
			app->localization->composeText(Strings::FILE_ENTITYSTRINGS, entityDef->description, largeBuffer);
			if (n4 != -1) {
				largeBuffer->append(" ");
				app->localization->composeText(Strings::FILE_CODESTRINGS, n4, largeBuffer);
			}
		}
	}
	else if (this->dialogType == Canvas::HELPMSG_TYPE_CODESTRING) {
		int n5 = this->helpMessageInts[0];
		app->localization->composeText((short)(n5 >> 16), (short)(n5 & 0xFFFF), largeBuffer);
	}
	else if (this->dialogType == Canvas::HELPMSG_TYPE_MEDAL) {
		dialogStyle = Enums::DLG_STYLE_SPECIAL;
		this->specialLootIcon = 4;
		largeBuffer->dispose();
		largeBuffer = (Text*)object;
	}
	else {
		largeBuffer->dispose();
		largeBuffer = (Text*)object;
	}
	for (int i = 0; i < 15; ++i) {
		this->helpMessageTypes[i] = this->helpMessageTypes[i + 1];
		this->helpMessageInts[i] = this->helpMessageInts[i + 1];
		this->helpMessageObjs[i] = this->helpMessageObjs[i + 1];
		this->helpMessageThreads[i] = this->helpMessageThreads[i + 1];
	}
	this->helpMessageObjs[15] = object;
	this->numHelpMessages--;
	if (app->player->enableHelp) {
		if (n3 == -1) {
			this->startDialog(nullptr, largeBuffer, dialogStyle, dialogFlags, false);
		}
		else {
			this->startDialog(&app->game->scriptThreads[n3], largeBuffer, dialogStyle, dialogFlags, true);
		}
	}
	largeBuffer->dispose();
}

void Canvas::enqueueHelpDialog(short n) {
	this->enqueueHelpDialog(Strings::FILE_CODESTRINGS, n, (uint8_t)(-1));
}

bool Canvas::enqueueHelpDialog(short n, short n2, uint8_t b) {
	Applet* app = CAppContainer::getInstance()->app;

	if (!app->player->enableHelp || this->isChickenKicking || this->state == Canvas::ST_DYING) {
		return false;
	}
	if (this->numHelpMessages == 16) {
		app->Error(41); // ERR_MAXHELP
		return false;
	}
	this->helpMessageTypes[this->numHelpMessages] = Canvas::HELPMSG_TYPE_CODESTRING;
	this->helpMessageInts[this->numHelpMessages] = (n << 16 | n2);
	this->helpMessageObjs[this->numHelpMessages] = nullptr;
	this->helpMessageThreads[this->numHelpMessages] = b;
	this->numHelpMessages++;
	if (this->state != Canvas::ST_AUTOMAP && this->state == Canvas::ST_PLAYING) {
		this->dequeueHelpDialog();
	}
	return true;
}

bool Canvas::enqueueHelpDialog(Text* text) {
	return this->enqueueHelpDialog(text, Canvas::HELPMSG_TYPE_NONE);
}

bool Canvas::enqueueHelpDialog(Text* text, int n) {
	Applet* app = CAppContainer::getInstance()->app;

	if (!app->player->enableHelp || this->isChickenKicking || this->state == Canvas::ST_DYING) {
		return false;
	}
	if (this->numHelpMessages == 16) {
		app->Error(41); // ERR_MAXHELP
		return false;
	}
	this->helpMessageTypes[this->numHelpMessages] = n;
	this->helpMessageObjs[this->numHelpMessages] = text;
	this->helpMessageThreads[this->numHelpMessages] = -1;
	this->numHelpMessages++;
	if (this->state != Canvas::ST_AUTOMAP && this->state == Canvas::ST_PLAYING) {
		this->dequeueHelpDialog();
	}
	return true;
}

void Canvas::enqueueHelpDialog(EntityDef* entityDef) {
	Applet* app = CAppContainer::getInstance()->app;

	if (!app->player->enableHelp || this->isChickenKicking || this->state == Canvas::ST_DYING) {
		return;
	}
	if (this->numHelpMessages == 16) {
		app->Error(41); // ERR_MAXHELP
		return;
	}
	this->helpMessageTypes[this->numHelpMessages] = Canvas::HELPMSG_TYPE_ITEM;
	this->helpMessageObjs[this->numHelpMessages] = entityDef;
	this->helpMessageThreads[this->numHelpMessages] = -1;
	this->numHelpMessages++;
	if (this->state != Canvas::ST_AUTOMAP && this->state == Canvas::ST_PLAYING) {
		this->dequeueHelpDialog();
	}
}

void Canvas::updateView() {
	Applet* app = CAppContainer::getInstance()->app;

	if (app->time < this->shakeTime) {
		this->shakeX = app->nextByte() % (this->shakeIntensity * 2) - this->shakeIntensity;
		this->shakeY = app->nextByte() % (this->shakeIntensity * 2) - this->shakeIntensity;
		this->staleView = true;
	}
	else if (this->shakeX != 0 || this->shakeY != 0) {
		this->staleView = true;
		this->shakeX = (this->shakeY = 0);
	}

	if (app->game->isCameraActive() && this->state != Canvas::ST_DRIVING) {
		app->game->activeCamera->Render();
		this->repaintFlags |= (Canvas::REPAINT_HUD | Canvas::REPAINT_PARTICLES);
		//app->hud->repaintFlags &= 0x18;
		return;
	}

	if (this->knockbackDist > 0 && this->viewX == this->destX && this->viewY == this->destY) {
		this->attemptMove(this->viewX + this->knockbackX * 64, this->viewY + this->knockbackY * 64);
	}

	bool b = this->viewX == this->destX && this->viewY == this->destY;
	bool b2 = this->viewAngle == this->destAngle;
	int animPos = this->animPos;
	int animAngle = this->animAngle;

	if (app->player->statusEffects[(Enums::OFS_STATUSEFFECT_TURNS + Enums::STATUS_EFFECT_AGILITY)] > 0 || this->knockbackDist > 0) {
		animPos += animPos / 2;
		animAngle += animAngle / 2;
	}

	if (this->viewX != this->destX || this->viewY != this->destY || this->viewZ != this->destZ || this->viewAngle != this->destAngle) {
		this->invalidateRect();
	}

	if (this->viewX < this->destX) {
		this->viewX += animPos;
		if (this->viewX > this->destX) {
			this->viewX = this->destX;
		}
	}
	else if (this->viewX > this->destX) {
		this->viewX -= animPos;
		if (this->viewX < this->destX) {
			this->viewX = this->destX;
		}
	}

	if (this->viewY < this->destY) {
		this->viewY += animPos;
		if (this->viewY > this->destY) {
			this->viewY = this->destY;
		}
	}
	else if (this->viewY > this->destY) {
		this->viewY -= animPos;
		if (this->viewY < this->destY) {
			this->viewY = this->destY;
		}
	}

	if (this->viewZ < this->destZ) {
		this->viewZ += this->zStep;
		if (this->viewZ > this->destZ) {
			this->viewZ = this->destZ;
		}
	}
	else if (this->viewZ > this->destZ) {
		this->viewZ -= this->zStep;
		if (this->viewZ < this->destZ) {
			this->viewZ = this->destZ;
		}
	}

	if (this->viewAngle < this->destAngle) {
		this->viewAngle += animAngle;
		if (this->viewAngle > this->destAngle) {
			this->viewAngle = this->destAngle;
		}
	}
	else if (this->viewAngle > this->destAngle) {
		this->viewAngle -= animAngle;
		if (this->viewAngle < this->destAngle) {
			this->viewAngle = this->destAngle;
		}
	}

	if (this->viewPitch < this->destPitch) {
		this->viewPitch += this->pitchStep;
		if (this->viewPitch > this->destPitch) {
			this->updateFacingEntity = true;
			this->viewPitch = this->destPitch;
		}
	}
	else if (this->viewPitch > this->destPitch) {
		this->viewPitch -= this->pitchStep;
		if (this->viewPitch < this->destPitch) {
			this->updateFacingEntity = true;
			this->viewPitch = this->destPitch;
		}
	}

	int viewZ = this->viewZ;
	if (this->knockbackDist != 0) {
		int n = this->viewX;
		if (this->knockbackX == 0) {
			n = this->viewY;
		}
		viewZ = this->viewZ + (10 * app->render->sinTable[(std::abs(n - this->knockbackStart) << 9) / this->knockbackWorldDist & 0x3FF] >> 16);
	}

	if (this->state == Canvas::ST_AUTOMAP) {
		this->viewX = this->destX;
		this->viewY = this->destY;
		this->viewZ = this->destZ;
		this->viewAngle = this->destAngle;
		this->viewPitch = this->destPitch;
	}

	if (this->state == Canvas::ST_COMBAT) {
		app->game->gsprite_update(app->time);
	}
	if (app->game->gotoTriggered) {
		app->game->gotoTriggered = false;
		int flagForFacingDir = this->flagForFacingDir(8);
		app->game->eventFlagsForMovement(-1, -1, -1, -1);
		app->game->executeTile(this->destX >> 6, this->destY >> 6, app->game->eventFlags[1], true);
		app->game->executeTile(this->destX >> 6, this->destY >> 6, flagForFacingDir, true);
	}
	else if (!b && this->viewX == this->destX && this->viewY == this->destY) {
		this->finishMovement();
	}

	if (!b2 && this->viewAngle == this->destAngle) {
		this->finishRotation(true);
	}

	if (app->game->isCameraActive() && this->state != Canvas::ST_DRIVING) {
		app->game->activeCamera->Update(app->game->activeCameraKey, app->gameTime - app->game->activeCameraTime);
		app->game->activeCamera->Render();
		this->repaintFlags |= (Canvas::REPAINT_HUD | Canvas::REPAINT_PARTICLES);
		return;
	}

	if (this->isZoomedIn) {
		int n2 = this->zoomAccuracy * app->render->sinTable[(app->time - this->zoomStateTime) / 2 & 0x3FF] >> 24;
		int n3 = this->zoomAccuracy * app->render->sinTable[(app->time - this->zoomStateTime) / 3 & 0x3FF] >> 24;
		int zoomFOV = this->zoomFOV;
		int n4;
		if (app->time < this->zoomTime) {
			n4 = this->zoomDestFOV + (this->zoomFOV - this->zoomDestFOV) * (this->zoomTime - app->time) / 360;
		}
		else {
			this->zoomTime = 0;
			n4 = (this->zoomFOV = this->zoomDestFOV);
		}
		int n5 = this->zoomPitch + this->viewPitch;
		if (app->combat->curAttacker == nullptr && this->state == Canvas::ST_COMBAT && app->combat->nextStage == 1) {
			n5 += app->render->sinTable[512 * ((app->gameTime - app->combat->animStartTime << 16) / app->combat->animTime) >> 16 & 0x3FF] * 28 >> 16;
		}
		this->renderScene(this->viewX, this->viewY, this->viewZ, this->viewAngle + this->zoomAngle + n2, n5 + n3, this->viewRoll, n4);
		this->updateFacingEntity = true;
	}
	else if (this->loadMapID != 0) {
		this->renderScene(this->viewX, this->viewY, viewZ, this->viewAngle, this->viewPitch, this->viewRoll, 281);
	}
}

void Canvas::clearSoftKeys() {
	this->softKeyLeftID = -1;
	this->softKeyRightID = -1;
	this->repaintFlags |= Canvas::REPAINT_SOFTKEYS;
}

void Canvas::clearLeftSoftKey() {
	this->softKeyLeftID = -1;
	this->repaintFlags |= Canvas::REPAINT_SOFTKEYS;
}

void Canvas::clearRightSoftKey() {
	this->softKeyRightID = -1;
	this->repaintFlags |= Canvas::REPAINT_SOFTKEYS;
}

void Canvas::setLeftSoftKey(short i, short i2) {
	if (!this->displaySoftKeys) {
		return;
	}
	this->softKeyLeftID = Localization::STRINGID(i, i2);
	this->repaintFlags |= Canvas::REPAINT_SOFTKEYS;
}

void Canvas::setRightSoftKey(short i, short i2) {
	if (!this->displaySoftKeys) {
		return;
	}
	this->softKeyRightID = Localization::STRINGID(i, i2);
	this->repaintFlags |= Canvas::REPAINT_SOFTKEYS;
}

void Canvas::setSoftKeys(short n, short n2, short n3, short n4) {
	if (!this->displaySoftKeys) {
		return;
	}
	this->softKeyLeftID = Localization::STRINGID(n, n2);
	this->softKeyRightID = Localization::STRINGID(n3, n4);
	this->repaintFlags |= Canvas::REPAINT_SOFTKEYS;
}

void Canvas::drawSoftKeys(Graphics* graphics) {
	Applet* app = CAppContainer::getInstance()->app;
	// J2ME Only
}

void Canvas::setLoadingBarText(short loadingStringType, short loadingStringID) {
	this->loadingStringType = loadingStringType;
	this->loadingStringID = loadingStringID;
	this->loadingFlags |= 0x3;
}

void Canvas::updateLoadingBar(bool b) {
	Applet* app = CAppContainer::getInstance()->app;

	if (b == false) {
		if (app->upTimeMs - this->lastPacifierUpdate < 0x96) {
			return;
		}
		this->lastPacifierUpdate = app->upTimeMs;
	}

	if ((this->loadingStringID == -1) || (this->loadingStringType == -1)) {
		this->setLoadingBarText(Strings::FILE_CODESTRINGS, CodeStrings::PROCESSING);
	}

	this->loadingFlags |= 3;
	this->repaintFlags |= Canvas::REPAINT_LOADING_BAR;
}

void Canvas::drawLoadingBar(Graphics* graphics) {
	Applet* app = CAppContainer::getInstance()->app;
	Text* text;
	int fontHeight = Applet::FONT_HEIGHT[app->fontType];
	int rectW = 250;
	int rectH = (fontHeight * 3 + (fontHeight >> 1) + 2);

	if ((this->loadingFlags & 3U) != 0) {
		int n = this->SCR_CX - rectW / 2;
		int n2 = this->SCR_CY - rectH / 2;

		text = app->localization->getSmallBuffer();
		this->loadingFlags &= 0xFFFFFFFE;
		graphics->eraseRgn(this->displayRect);
		graphics->setColor(0xFFFFFFFF);
		graphics->drawRect(n, n2, rectW, rectH);
		app->localization->composeText(this->loadingStringType, this->loadingStringID, text);
		text->dehyphenate();
		graphics->drawString(text, this->SCR_CX, this->SCR_CY - fontHeight - 6, 17);
		text->setLength(0);
		app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::JUST_A_MOMENT, text);
		text->dehyphenate();
		graphics->drawString(text, this->SCR_CX, this->SCR_CY + -6, 17);
		text->dispose();
		
		this->loadingFlags &= 0xFFFFFFFD;
		int n3 = this->SCR_CX - rectW / 2 + 8;
		int n4 = this->SCR_CY - rectH / 2 + 24;
		int n5 = n3 + 234;

		this->pacifierX += 10;
		if (this->pacifierX >= n5 - 5 || this->pacifierX < n3 + 1) {
			this->pacifierX = n3 + 1;
		}

		graphics->setColor(0xFF000000);
		graphics->fillRect(n3, n4, 234, 12);
		graphics->setColor(0xFFFFFFFF);
		graphics->drawRect(n3, n4, 234, 12);
		graphics->drawRegion(app->hud->imgAmmoIcons, 0, 14, std::min(30, n5 - this->pacifierX), 14, this->pacifierX, n4, 0, 0 ,0);
	}
}

void Canvas::unloadMedia() {
	Applet* app = CAppContainer::getInstance()->app;
	this->freeRuntimeData();
	app->game->unloadMapData();
	app->render->unloadMap();
}

void Canvas::invalidateRect() {
	this->staleView = true;
}

int Canvas::getRecentLoadType() {
	if (this->recentBriefSave) {
		return Game::LOAD_BRIEFSAVE;
	}
	return Game::LOAD_USERSAVE;
}

void Canvas::initZoom() {
	Applet* app = CAppContainer::getInstance()->app;
	this->zoomTime = 0;
	this->zoomCurFOVPercent = 0;
	this->zoomFOV = this->zoomDestFOV = 181;
	this->zoomAngle = 0;
	this->zoomPitch = 0;
	this->zoomTurn = 0;
	this->viewPitch = this->destPitch = 0;
	this->zoomStateTime = app->time;
	this->isZoomedIn = true;
	app->StartAccelerometer();
	this->m_sniperScopeDialScrollButton->Update(0, 320);
	this->zoomAccuracy = 2560 * std::max(0, std::min((256 - app->player->ce->getStatPercent(Enums::STAT_ACCURACY) << 8) / 26, 256)) >> 8;
	if (app->player->ce->weapon == Enums::WP_FG42) {
		this->zoomAccuracy = this->zoomAccuracy * 156 >> 8;
	}
	this->zoomMinFOVPercent = 256;
	this->zoomMaxAngle = 64 - (this->zoomAccuracy >> 8);
	app->render->startFade(500, 2);
	this->drawPlayingSoftKeys();

	CAppContainer::getInstance()->sdlGL->centerMouse(0, -22); // [GEC]
}

void Canvas::zoomOut() {
	Applet* app = CAppContainer::getInstance()->app;
	app->StopAccelerometer();
	this->isZoomedIn = false;
	this->viewAngle += this->zoomAngle;
	int n = 255;
	this->destAngle = this->viewAngle = ((this->viewAngle + (n >> 1)) & ~n);
	app->render->startFade(500, 2);
	this->finishRotation(true);
	app->tinyGL->resetViewPort();
	this->drawPlayingSoftKeys();
}

bool Canvas::handleZoomEvents(int key, int action) {
	return this->handleZoomEvents(key, action, false);
}

bool Canvas::handleZoomEvents(int key, int action, bool b) {
	Applet* app = CAppContainer::getInstance()->app;
	if (!b && ((this->zoomTime != 0) || app->game->activePropogators != 0 || app->game->animatingEffects != 0 || !app->game->snapMonsters(false))) {
		return true;
	}
	int n3 = 5 + ((20 * (256 - this->zoomCurFOVPercent)) >> 8);
	if (action == Enums::ACTION_MENU || action == Enums::ACTION_BACK) {
		this->zoomOut();
		return true;
	}
	if (action == Enums::ACTION_AUTOMAP) {
		app->hud->msgCount = 0;
		app->menuSystem->setMenu(Menus::MENU_INGAME);
		return true;
	}
	else if (action == Enums::ACTION_RIGHT) {
		this->zoomAngle -= n3;
		this->updateFacingEntity = true;
		++this->zoomTurn;
		CAppContainer::getInstance()->sdlGL->centerMouse(0, -22); // [GEC]
		CAppContainer::getInstance()->app->StopAccelerometer(); // [GEC]
	}
	else if (action == Enums::ACTION_LEFT) {
		this->zoomAngle += n3;
		this->updateFacingEntity = true;
		++this->zoomTurn;
		CAppContainer::getInstance()->sdlGL->centerMouse(0, -22); // [GEC]
		CAppContainer::getInstance()->app->StopAccelerometer(); // [GEC]
	}
	else if (action == Enums::ACTION_DOWN) {
		this->zoomPitch -= n3;
		++this->zoomTurn;
		CAppContainer::getInstance()->sdlGL->centerMouse(0, -22); // [GEC]
		CAppContainer::getInstance()->app->StopAccelerometer(); // [GEC]
	}
	else if (action == Enums::ACTION_UP) {
		this->zoomPitch += n3;
		++this->zoomTurn;
		CAppContainer::getInstance()->sdlGL->centerMouse(0, -22); // [GEC]
		CAppContainer::getInstance()->app->StopAccelerometer(); // [GEC]
	}
	else if (action == Enums::ACTION_PASSTURN) {
		app->hud->addMessage(CodeStrings::TURN_PASSED);
		app->game->touchTile(this->destX, this->destY, false);
		app->game->advanceTurn();
		this->invalidateRect();
		this->zoomTurn = 0;
	}
	else if (action == Enums::ACTION_NEXTWEAPON) {
		if (this->zoomCurFOVPercent < this->zoomMinFOVPercent) {
			this->zoomCurFOVPercent += 64;
			this->zoomCurFOVPercent = std::min(this->zoomCurFOVPercent, this->zoomMinFOVPercent); // [GEC]
			++this->zoomTurn;
			this->zoomDestFOV = 181 + ((-101 * this->zoomCurFOVPercent) >> 8);
			this->zoomTime = app->time + 360;

			// [GEC] update scroll bar
			{
				float maxScroll = (float)((this->m_sniperScopeDialScrollButton->barRect).h - this->m_sniperScopeDialScrollButton->field_0x4c_);
				float curFOV = (float)((float)this->zoomCurFOVPercent / (float)this->zoomMinFOVPercent);
				this->m_sniperScopeDialScrollButton->field_0x48_ = (int)(maxScroll - (maxScroll * curFOV));
			}
			app->sound->playSound(1140, 0, 4, false);

		}
	}
	else if (action == Enums::ACTION_PREVWEAPON) {
		if (this->zoomCurFOVPercent > 0) {
			this->zoomCurFOVPercent -= 64;
			this->zoomCurFOVPercent = std::max(this->zoomCurFOVPercent, 0); // [GEC]
			++this->zoomTurn;
			this->zoomDestFOV = 181 + ((-101 * this->zoomCurFOVPercent) >> 8);
			this->zoomTime = app->time + 360;

			// [GEC] update scroll bar
			{
				float maxScroll = (float)((this->m_sniperScopeDialScrollButton->barRect).h - this->m_sniperScopeDialScrollButton->field_0x4c_);
				float curFOV = (float)((float)this->zoomCurFOVPercent / (float)this->zoomMinFOVPercent);
				this->m_sniperScopeDialScrollButton->field_0x48_ = (int)(maxScroll - (maxScroll * curFOV));
			}
			app->sound->playSound(1140, 0, 4, false);
		}
		++this->zoomTurn;
	}
	else if (action == Enums::ACTION_FIRE) {
		this->zoomTurn = 0;
		return handlePlayingEvents(key, action);
	}

	if (this->zoomPitch < -this->zoomMaxAngle) {
		this->zoomPitch = -this->zoomMaxAngle;
	}
	else if (this->zoomPitch > this->zoomMaxAngle) {
		this->zoomPitch = this->zoomMaxAngle;
	}
	if ((this->zoomTurn & 0x7) == 0x7) {
		app->game->advanceTurn();
	}
	return true;
}

void Canvas::mixingState(Graphics *graphics) {
	Applet* app = CAppContainer::getInstance()->app;

	app->localization->resetTextArgs();

	graphics->drawImage(this->imgMixingBG, 0, 0, 0, 0, 0);

	this->drawTouchSoftkeyBar(graphics, this->m_mixingButtons->GetButton(3)->highlighted, this->m_mixingButtons->GetButton(4)->highlighted);

	int x = (480 - this->imgMixingHeadingPlate->width) >> 1;
	int y = 3;
	int w = this->imgMixingHeadingPlate->width >> 1;
	int h = this->imgMixingHeadingPlate->height >> 1;

	graphics->drawImage(this->imgMixingHeadingPlate, x, y, 0, 0, 0);

	Text* smallBuffer = app->localization->getSmallBuffer();
	app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::MIXING_TITLE, smallBuffer);
	smallBuffer->dehyphenate();
	graphics->drawString(smallBuffer, x + w, y + h, 3);

	if (this->mixingMsg && this->msgSeen) {
		this->updateMixingText();
		this->mixingMsg = false;
		this->msgSeen = false;
	}
	if (!this->mixingMsg) {
		int _x = ((this->imgMixingTTBludeNormal->width + 123) >> 1) + 32;
		int _y = 106 - this->imgMixingNumbersPlate->height;

		smallBuffer->setLength(0);
		app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::INGREDIENTS_TITLE, smallBuffer);
		smallBuffer->dehyphenate();
		graphics->drawString(smallBuffer, _x, _y - 8, 33);

		
		this->drawMixingNumbers2(graphics, (((imgMixingTTRedNormal->width - this->imgMixingNumbersPlate->width) >> 1) + 32), _y, this->stateVars[6]);
		if (this->stateVars[6]) {
			if (this->m_mixingButtons->GetButton(0)->highlighted && this->stateVars[0] <= 2) {
				graphics->drawImage(this->imgMixingTTRedSelected, 32, 111, 0, 0, 0);
			}
			else {
				graphics->drawImage(this->imgMixingTTRedNormal, 32, 111, 0, 0, 0);
			}

			if (this->stateVars[4] == 0 && this->stateVars[0] <= 2) { // [GEC]
				graphics->drawImage(this->imgMixingTTRedSelected, 32, 111, 0, 0, 10);
			}
		}
		else {
			graphics->drawImage(this->imgMixingTTRedEmpty, 32, 111, 0, 0, 0);
		}

		this->drawMixingNumbers2(graphics, (((this->imgMixingTTRedNormal->width - this->imgMixingNumbersPlate->width) >> 1) + 92), _y, this->stateVars[7]);
		if (this->stateVars[7]) {
			if (this->m_mixingButtons->GetButton(1)->highlighted && this->stateVars[0] <= 2) {
				graphics->drawImage(this->imgMixingTTGreenSelected, 92, 111, 0, 0, 0);
			}
			else {
				graphics->drawImage(this->imgMixingTTGreenNormal, 92, 111, 0, 0, 0);
			}

			if (this->stateVars[4] == 1 && this->stateVars[0] <= 2) { // [GEC]
				graphics->drawImage(this->imgMixingTTGreenSelected, 92, 111, 0, 0, 10);
			}
		}
		else {
			graphics->drawImage(this->imgMixingTTGreenEmpty, 92, 111, 0, 0, 0);
		}

		this->drawMixingNumbers2(graphics, (((this->imgMixingTTRedNormal->width - this->imgMixingNumbersPlate->width) >> 1) + 155), _y, this->stateVars[8]);
		if (this->stateVars[8]) {
			if (this->m_mixingButtons->GetButton(2)->highlighted && this->stateVars[0] <= 2) {
				graphics->drawImage(this->imgMixingTTBludeSelected, 155, 111, 0, 0, 0);
			}
			else {
				graphics->drawImage(this->imgMixingTTBludeNormal, 155, 111, 0, 0, 0);
			}

			if (this->stateVars[4] == 2 && this->stateVars[0] <= 2) { // [GEC]
				graphics->drawImage(this->imgMixingTTBludeSelected, 155, 111, 0, 0, 10);
			}
		}
		else {
			graphics->drawImage(this->imgMixingTTBludeEmpty, 155, 111, 0, 0, 0);
		}

		if (this->m_mixingButtons->GetButton(5)->highlighted && this->stateVars[0] == 3) {
			graphics->drawImage(this->imgMixingSyringeSelected, 219, 117, 0, 0, 0);
		}
		else {
			graphics->drawImage(this->imgMixingSyringe, 225, 123, 0, 0, 0);
		}

		if (this->stateVars[0] == 3) { // [GEC]
			graphics->drawImage(this->imgMixingSyringeSelected, 219, 117, 0, 0, 10);
		}

		int v18 = 278;
		int imgMixingSyringeWidth = this->imgMixingSyringeRed->width;

		switch (this->stateVars[3])
		{
		case 0xFFCC0000:
			graphics->drawImage(this->imgMixingSyringeRed, v18, 132, 0, 0, 0);
			break;
		case 0xFF00CC00:
			graphics->drawImage(this->imgMixingSyringeGreen, v18, 132, 0, 0, 0);
			break;
		case 0xFF0000CC:
			graphics->drawImage(this->imgMixingSyringeBlue, v18, 132, 0, 0, 0);
			break;
		}

		v18 += imgMixingSyringeWidth;
		switch (this->stateVars[2])
		{
		case 0xFFCC0000:
			graphics->drawImage(this->imgMixingSyringeRed, v18, 132, 0, 0, 0);
			break;
		case 0xFF00CC00:
			graphics->drawImage(this->imgMixingSyringeGreen, v18, 132, 0, 0, 0);
			break;
		case 0xFF0000CC:
			graphics->drawImage(this->imgMixingSyringeBlue, v18, 132, 0, 0, 0);
			break;
		}

		v18 += imgMixingSyringeWidth;
		switch (this->stateVars[1])
		{
		case 0xFFCC0000:
			graphics->drawImage(this->imgMixingSyringeRed, v18, 132, 0, 0, 0);
			break;
		case 0xFF00CC00:
			graphics->drawImage(this->imgMixingSyringeGreen, v18, 132, 0, 0, 0);
			break;
		case 0xFF0000CC:
			graphics->drawImage(this->imgMixingSyringeBlue, v18, 132, 0, 0, 0);
			break;
		}

		this->cocktailName->dehyphenate();
		graphics->drawString(this->cocktailName, _x, ((this->imgMixingTTRedNormal->height - app->menuSystem->imgGameMenuPanelbottom->height + 431) >> 1) - 2, 3);

		int v21 = (this->imgMixingSyringe->height - app->menuSystem->imgGameMenuPanelbottom->height - this->imgMixingSyringeNumbersPlate->height + 443) >> 1;
		this->drawMixingNumbers3(graphics, 380, v21, app->player->inventory[Enums::INV_EMPTY_SYRINGE]);

		smallBuffer->setLength(0);
		app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::SYRINGES_ITEM, smallBuffer);
		smallBuffer->dehyphenate();
		graphics->drawString(smallBuffer, 377, v21 + (this->imgMixingSyringeNumbersPlate->height >> 1), 10);
	}
	int n13 = 305 - ((app->menuSystem->imgGameMenuPanelbottom->height - 15) >> 1);
	if (this->mixingMsg) {
		smallBuffer->setLength(0);
		app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::PRESS_OK, smallBuffer);
		smallBuffer->dehyphenate();
		graphics->drawString(smallBuffer, Applet::IOS_WIDTH >> 1, n13, 1);
		n13 = this->imgMixingHeadingPlate->height + ((-app->menuSystem->imgGameMenuPanelbottom->height - (this->imgMixingHeadingPlate->height + 3) + Applet::IOS_HEIGHT) >> 1) + 3;
	}
	graphics->drawString(this->mixingInstructions, Applet::IOS_WIDTH >> 1, n13, 3);
	smallBuffer->dispose();
}

void Canvas::handleMixingEvents(int action) {
	Applet* app = CAppContainer::getInstance()->app;
	if (this->mixingMsg) {
		if (action == Enums::ACTION_FIRE || action == Enums::ACTION_BACK) {
			this->msgSeen = true;
			this->setSoftKeys(Strings::FILE_MENUSTRINGS, MenuStrings::BACK_ITEM, Strings::FILE_MENUSTRINGS, MenuStrings::RECIPE_ITEM);
		}
		return;
	}
	if (action == Enums::ACTION_MENU || action == Enums::ACTION_BACK) {
		if (this->stateVars[0] == 0) {
			app->game->checkEmptyStation(this->curStation);
			this->setState(Canvas::ST_PLAYING);
		}
		else {
			this->popIngredient();
			if (this->stateVars[4] == 3) {
				this->nextMixingIngredient(1);
			}
		}
	}
	else if (action == Enums::ACTION_FIRE) {
		if (this->stateVars[0] < 3) {
			if (this->stateVars[4] == 3 || app->player->inventory[Enums::INV_EMPTY_SYRINGE] == 0) {
				app->game->checkEmptyStation(this->curStation);
				setState(Canvas::ST_PLAYING);
				return;
			}
			int n2 = 0;
			switch (this->stateVars[4]) {
			case 0: {
				n2 = 0xFFCC0000;
				break;
			}
			case 1: {
				n2 = 0xFF00CC00;
				break;
			}
			case 2: {
				n2 = 0xFF0000CC;
				break;
			}
			}
			this->pushIngredient(n2);
		}
		else {
			this->mixCocktail();
			clearSoftKeys();
		}
	}
	else if (action == Enums::ACTION_AUTOMAP) {
		app->menuSystem->setMenu(Menus::MENU_INGAME_RECIPES);
	}
	else if (action == Enums::ACTION_LEFT) {
		this->nextMixingIngredient(-1);
	}
	else if (action == Enums::ACTION_RIGHT) {
		this->nextMixingIngredient(1);
	}
}

void Canvas::nextMixingIngredient(int n) {
	int n2 = this->stateVars[4];
	do {
		n2 = (n2 + n & 0x3);
		if (n2 == this->stateVars[4]) {
			break;
		}
	} while (n2 == 3 || this->stateVars[6 + n2] == 0);
	if (n2 != 3 && this->stateVars[6 + n2] == 0) {
		n2 = 3;
	}
	this->stateVars[4] = n2;
}

void Canvas::pushIngredient(int n) {
	if (this->stateVars[0] + 1 > 3) {
		return;
	}
	if (--this->stateVars[6 + this->stateVars[4]] == 0) {
		this->nextMixingIngredient(1);
	}
	++this->stateVars[0];
	this->stateVars[this->stateVars[0]] = n;
	this->updateMixingText();
}

void Canvas::popIngredient() {
	if (this->stateVars[0] - 1 < 0) {
		return;
	}
	switch (this->stateVars[this->stateVars[0]]) {
		case 0xFFCC0000: {
			++this->stateVars[6];
			break;
		}
		case 0xFF00CC00: {
			++this->stateVars[7];
			break;
		}
		case 0xFF0000CC: {
			++this->stateVars[8];
			break;
		}
	}
	this->stateVars[this->stateVars[0]] = 0xFFB7B7B7;
	--this->stateVars[0];
	this->updateMixingText();
}

void Canvas::updateMixingText() {
	Applet* app = CAppContainer::getInstance()->app;
	if (this->stateVars[0] != 3) {
		if (this->stateVars[0] == 0) {
			this->setLeftSoftKey(Strings::FILE_MENUSTRINGS, MenuStrings::BACK_ITEM);
		}
		this->cocktailName->setLength(0);
		if (this->stateVars[4] == 3 || app->player->inventory[Enums::INV_EMPTY_SYRINGE] == 0) {
			this->mixingInstructions->setLength(0);
			app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::PRESS_OK_TO_EXIT, this->mixingInstructions);
			this->mixingInstructions->dehyphenate();
		}
		else {
			app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::UNKNOWN_MIX, this->cocktailName);
			this->cocktailName->dehyphenate();
			this->mixingInstructions->setLength(0);
			app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::PRESS_OK_TO_ADD, this->mixingInstructions);
			this->mixingInstructions->dehyphenate();
		}
	}
	else {
		this->mixingInstructions->setLength(0);
		app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::PRESS_OK_TO_MIX, this->mixingInstructions);
		this->mixingInstructions->dehyphenate();
		uint8_t b = 0;
		for (int i = 0; i < 3; ++i) {
			switch (this->stateVars[3 - i]) {
				case 0xFFCC0000: {
					b += 16;
					break;
				}
				case 0xFF00CC00: {
					b += 4;
					break;
				}
				case 0xFF0000CC: {
					++b;
					break;
				}
			}
		}

		for (int j = 0; j < app->resource->getNumTableBytes(12); ++j) {
			if (b == this->cocktailRecipes[j]) {
				if ((app->player->cocktailDiscoverMask & 1 << j) != 0x0) {
					this->cocktailName->setLength(0);
					app->localization->composeText(Strings::FILE_ENTITYSTRINGS, this->cocktailNames[j], this->cocktailName);
					this->cocktailName->dehyphenate();
				}
				this->stateVars[5] = j;
				this->mixingInstructions->setLength(0);
				app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::PRESS_OK_TO_MIX, this->mixingInstructions);
				this->mixingInstructions->dehyphenate();
				return;
			}
		}

		app->Error("Cannot find a match for the cocktail."); // ERR_COCKTAILMIX
	}
	if (this->stateVars[0] != 0) {
		this->setLeftSoftKey(Strings::FILE_CODESTRINGS, CodeStrings::UNDO);
	}
}

void Canvas::mixCocktail() {
	Applet* app = CAppContainer::getInstance()->app;

	this->mixingMsg = true;
	this->msgSeen = false;
	if (app->player->inventory[Enums::INV_EMPTY_SYRINGE] == 0) {
		this->mixingInstructions->setLength(0);
		app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::MUST_HAVE_EMPTY, this->mixingInstructions);
		return;
	}
	this->mixingInstructions->setLength(0);
	EntityDef* find = app->entityDefManager->find(Enums::ET_ITEM, Enums::IT_INVENTORY, 5 + this->stateVars[5]);
	app->localization->resetTextArgs();
	app->localization->addTextArg(Strings::FILE_ENTITYSTRINGS, find->name);
	if (app->player->give(Enums::IT_INVENTORY, Enums::INV_FIRST_MIXABLE + this->stateVars[5], 1)) {
		for (int i = 0; i < 3; ++i) {
			switch (this->stateVars[3 - i]) {
			case 0xFFCC0000: {
				--app->game->mixingStations[this->curStation + 1];
				break;
			}
			case 0xFF00CC00: {
				--app->game->mixingStations[this->curStation + 2];
				break;
			}
			case 0xFF0000CC: {
				--app->game->mixingStations[this->curStation + 3];
				break;
			}
			}
		}
		app->player->cocktailDiscoverMask |= 1 << this->stateVars[5];
		--app->player->inventory[Enums::INV_EMPTY_SYRINGE];
		this->stateVars[0] = 0;
		this->stateVars[3] = this->stateVars[2] = this->stateVars[1] = 0xFFB7B7B7;
		app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::CREATED_X, this->mixingInstructions);
		app->sound->playSound(1053, 0, 2, 0);
	}
	else {
		app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::CANT_HOLD_MORE, this->mixingInstructions);
	}
	this->mixingInstructions->dehyphenate();
}

void Canvas::recipeToString(int n, Text* text) {
	uint8_t b = this->cocktailRecipes[n];
	for (int n2 = 4, i = 0; i < 3; ++i, n2 -= 2) {
		for (int j = (b & 3 << n2) >> n2; j > 0; --j) {
			if (n2 == 4) {
				text->append('\xbc');
			}
			else if (n2 == 2) {
				text->append('\xbe');
			}
			else {
				text->append('\xbd');
			}
		}
	}
}

void Canvas::drawMixingNumbers2(Graphics* graphics, float x, int y, int num) {
	Applet* app = CAppContainer::getInstance()->app;

	if (num <= 99) {
		graphics->drawImage(this->imgMixingNumbersPlate, x, y, 0, 0, 0);
		graphics->drawRegion(app->hud->imgNumbers, 0, 20 * (9 - num / 10), 10, 20, x + 3, y + 2, 20, 0, 0);
		graphics->drawRegion(app->hud->imgNumbers, 0, 20 * (9 - num % 10), 10, 20, x + 14, y + 2, 20, 0, 0);
	}
	else {
		puts("Error: can only draw numbers between 0 and 99!");
	}
}

void Canvas::drawMixingNumbers3(Graphics* graphics, float x, int y, int num) {
	Applet* app = CAppContainer::getInstance()->app;

	if (num <= 999) {
		graphics->drawImage(this->imgMixingSyringeNumbersPlate, x, y, 0, 0, 0);
		graphics->drawRegion(app->hud->imgNumbers, 0, 20 * (9 - num / 100), 10, 20, x + 3, y + 2, 20, 0, 0);
		graphics->drawRegion(app->hud->imgNumbers, 0, 20 * (9 - num % 100 / 10), 10, 20, x + 14, y + 2, 20, 0, 0);
		graphics->drawRegion(app->hud->imgNumbers, 0, 20 * (9 - num % 100 % 10), 10, 20, x + 25, y + 2, 20, 0, 0);
	}
	else {
		puts("Error: can only draw numbers between 0 and 999!");
	}
}

void Canvas::handleStoryInput(int key, int action) {
	Applet* app = CAppContainer::getInstance()->app;

	if (action == Enums::ACTION_LEFT || action == Enums::ACTION_RIGHT) {
		if (this->stateVars[0] != 2) {
			this->stateVars[0] ^= 1;
		}
	}
	else if (action == Enums::ACTION_UP) {
		if (this->stateVars[0] != 2 && this->storyPage < this->storyTotalPages - 1) {
			this->stateVars[0] = 2;
		}
	}
	else if (action == Enums::ACTION_DOWN) {
		if (this->stateVars[0] == 2) {
			this->stateVars[0] = 1;
		}
	}
	else if (action == Enums::ACTION_FIRE) {
		switch (this->stateVars[0]) {
		case 0: {
			this->changeStoryPage(-1);
			break;
		}
		case 1: {
			this->changeStoryPage(1);
			break;
		}
		case 2: {
			this->storyPage = this->storyTotalPages;
			break;
		}
		}
	}
	else if (action == Enums::ACTION_AUTOMAP) {
		#ifdef WOLFENRPG_PSP
		this->storyPage = this->storyTotalPages;
		#else
		this->changeStoryPage(1);
		this->stateVars[0] = 1;
		#endif
	}
	else if (action == Enums::ACTION_BACK || action == Enums::ACTION_MENU) {
		this->changeStoryPage(-1);
		this->stateVars[0] = 0;
	}
}

void Canvas::drawKickingBars(Graphics* graphics) {
	Applet* app = CAppContainer::getInstance()->app;

	if (this->kickingPhase == 0) {
		return;
	}
	int n = this->SCR_CX - 126;
	int n2 = this->viewRect[1] + this->viewRect[3] - 28;
	int n3 = 1;
	int n4 = 33;

	graphics->drawRegion(this->hKickingBar, 0, 0, 252, 26, n, n2, 0, 0, 0);
	graphics->drawRegion(this->vKickingBar, 0, 0, 26, 210, n3, n4, 0, 0, 0);

	int n5 = app->upTimeMs - this->stateVars[0];
	if (this->kickingPhase == 1) {
		n += 22;
		int n6 = (n5 % 2000 << 16) / 256000;
		int kickHPos;
		if (n6 >= 256) {
			n6 -= 256;
			kickHPos = n + 204 - (n6 * 204 >> 8);
			if (n6 <= 85) {
				this->chickenDestRight = 1;
			}
			else if (n6 <= 170) {
				this->chickenDestRight = 0;
			}
			else {
				this->chickenDestRight = -1;
			}
		}
		else {
			kickHPos = n + (n6 * 204 >> 8);
			if (n6 <= 85) {
				this->chickenDestRight = -1;
			}
			else if (n6 <= 170) {
				this->chickenDestRight = 0;
			}
			else {
				this->chickenDestRight = 1;
			}
		}
		graphics->drawRegion(this->hKickingBar, 253, 0, 7, 26, kickHPos, n2, 0, 0, 0);
		this->kickHPos = kickHPos;
	}
	else if (this->kickingPhase == 2) {
		n4 += 18;
		int n7 = (n5 % 1600 << 16) / 204800;
		int kickVPos;
		if (n7 >= 256) {
			n7 -= 256;
			kickVPos = n4 + 168 - (n7 * 168 >> 8);
			if (n7 <= 64) {
				this->chickenDestFwd = 2;
			}
			else if (n7 <= 128) {
				this->chickenDestFwd = 3;
			}
			else if (n7 <= 192) {
				this->chickenDestFwd = 4;
			}
			else {
				this->chickenDestFwd = 5;
			}
		}
		else {
			kickVPos = n4 + (n7 * 168 >> 8);
			if (n7 <= 64) {
				this->chickenDestFwd = 5;
			}
			else if (n7 <= 128) {
				this->chickenDestFwd = 4;
			}
			else if (n7 <= 192) {
				this->chickenDestFwd = 3;
			}
			else {
				this->chickenDestFwd = 2;
			}
		}
		graphics->drawRegion(this->vKickingBar, 0, 211, 26, 7, n3, kickVPos, 0, 0, 0);
		this->kickVPos = kickVPos;
		graphics->drawRegion(this->hKickingBar, 253, 0, 7, 26, this->kickHPos, n2, 0, 0, 0);
	}
	else if (this->kickingPhase == 3) {
		graphics->drawRegion(this->vKickingBar, 0, 211, 26, 7, n3, this->kickVPos, 0, 0, 0);
		graphics->drawRegion(this->hKickingBar, 253, 0, 7, 26, this->kickHPos, n2, 0, 0, 0);
	}
}

void Canvas::startKicking(bool kickingFromMenu) {
	Applet* app = CAppContainer::getInstance()->app;
	this->isChickenKicking = true;
	this->curScore = 0;
	this->kickingPhase = 0;
	app->player->selectWeapon(3);
	this->kickingFromMenu = kickingFromMenu;
}

void Canvas::endKickingRound() {
	Applet* app = CAppContainer::getInstance()->app;
	if (!this->isChickenKicking) {
		return;
	}
	this->lastScore = this->curScore;
	this->curScore = 0;
	this->highScoreIndex = -1;
	this->stateVars[1] = 0;
	this->stateVars[2] = -1;
	this->stateVars[4] = app->upTimeMs;
	for (int i = 0; i < 5; ++i) {
		if (this->highScores[i] <= this->lastScore) {
			this->highScoreIndex = i;
			break;
		}
	}
	if (this->highScoreIndex != -1) {
		for (int j = 4; j > this->highScoreIndex; --j) {
			this->highScores[j] = this->highScores[j - 1];
			int n = j * 3;
			int n2 = (j - 1) * 3;
			this->highScoreInitials[n + 0] = this->highScoreInitials[n2 + 0];
			this->highScoreInitials[n + 1] = this->highScoreInitials[n2 + 1];
			this->highScoreInitials[n + 2] = this->highScoreInitials[n2 + 2];
		}
		this->highScores[this->highScoreIndex] = (short)this->lastScore;
		this->highScoreInitials[this->highScoreIndex * 3 + 0] = 'B';
		this->highScoreInitials[this->highScoreIndex * 3 + 1] = 'J';
		this->highScoreInitials[this->highScoreIndex * 3 + 2] = 'B';
	}
	app->hud->addMessage(Strings::FILE_CODESTRINGS, CodeStrings::KICKING_DONE, 2);
	this->stateVars[0] = app->gameTime + 2000;
	this->kickingPhase = 5;
}

void Canvas::endKickingGame() {
	Applet* app = CAppContainer::getInstance()->app;
	if (!this->isChickenKicking) {
		return;
	}

	this->hKickingBar->~Image();
	this->hKickingBar = nullptr;
	this->vKickingBar->~Image();
	this->vKickingBar = nullptr;
	this->imgChickenKicking_ScoreBG->~Image();
	this->imgChickenKicking_ScoreBG = nullptr;
	this->imgChickenKicking_05Highlight->~Image();
	this->imgChickenKicking_05Highlight = nullptr;
	this->imgChickenKicking_20Highlight->~Image();
	this->imgChickenKicking_20Highlight = nullptr;
	this->imgChickenKicking_30Highlight->~Image();
	this->imgChickenKicking_30Highlight = nullptr;

	this->kickingPhase = 0;
	this->isChickenKicking = false;
}

int Canvas::getKickPoint(int n, int n2) {
	n = (--n & ~(n >> 31));
	n2 -= 18;
	n2 &= ~(n2 >> 31);
	return Canvas::CKPOINTS[n + n2 * 5];
}

void Canvas::drawHighScore(Graphics* graphics) {
	Applet* app = CAppContainer::getInstance()->app;
	int upTimeMs = app->upTimeMs;
	Text* smallBuffer = app->localization->getSmallBuffer();
	graphics->setColor(0xFF000000);
	graphics->fillRect(this->screenRect[0], this->screenRect[1], this->displayRect[2], this->displayRect[3]);
	smallBuffer->setLength(0);
	app->localization->resetTextArgs();
	app->localization->addTextArg(Strings::FILE_CODESTRINGS, CodeStrings::CHICKEN_NAME);
	app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::HIGH_SCORE_TITLE, smallBuffer);
	smallBuffer->dehyphenate();
	graphics->drawString(smallBuffer, this->SCR_CX, 10, 1);
	graphics->setColor(0xFF666666);
	graphics->drawLine(20, 45, this->screenRect[2] - 20, 45);
	if (this->stateVars[4] <= upTimeMs && this->stateVars[1] != 2 && this->stateVars[2] != -1) {
		this->stateVars[4] = app->upTimeMs + 2000;
		this->stateVars[1]++;
		this->stateVars[2] = -1;
	}
	int n2 = this->stateVars[1];
	int n3 = 70;
	int n4 = 20;
	smallBuffer->setLength(0);
	app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::RANK_TITLE, smallBuffer);
	smallBuffer->dehyphenate();
	graphics->drawString(smallBuffer, n4, n3, 0);
	smallBuffer->setLength(0);
	app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::NAME_TITLE, smallBuffer);
	smallBuffer->dehyphenate();
	graphics->drawString(smallBuffer, this->SCR_CX, n3, 1);
	int n5 = this->screenRect[2] - 20;
	smallBuffer->setLength(0);
	app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::SCORE_TITLE, smallBuffer);
	smallBuffer->dehyphenate();
	graphics->drawString(smallBuffer, n5, n3, 8);
	n3 += 16;

	// [GEC] Restore J2ME
	/*
	if (this->highScoreIndex >= 0 && (upTimeMs / 512 & 0x1) != 0x0) {
		smallBuffer->setLength(0);
		smallBuffer->append('_');
		graphics->drawString(smallBuffer, this->SCR_CX - 10 + n2 * 7, n3 + this->highScoreIndex * 12 + 3, 0);
	}
	*/

	short n6 = CodeStrings::FIRST_SCORE;
	for (int i = 0; i < 5; ++i, ++n6) {
		smallBuffer->setLength(0);
		app->localization->composeText(Strings::FILE_CODESTRINGS, n6, smallBuffer);
		graphics->drawString(smallBuffer, n4, n3, 0);
		smallBuffer->setLength(0);
		smallBuffer->append(this->highScoreInitials[i * 3 + 0]);
		smallBuffer->append(this->highScoreInitials[i * 3 + 1]);
		smallBuffer->append(this->highScoreInitials[i * 3 + 2]);
		graphics->drawString(smallBuffer, this->SCR_CX, n3, 1);
		smallBuffer->setLength(0);
		smallBuffer->append(this->highScores[i]);
		graphics->drawString(smallBuffer, n5, n3, 8);
		n3 += Applet::FONT_HEIGHT[app->fontType];
	}
	n3 += Applet::FONT_HEIGHT[app->fontType] * 4;
	smallBuffer->setLength(0);
	if (this->highScoreIndex == -1) {
		app->localization->resetTextArgs();
		app->localization->addTextArg(this->lastScore);
		app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::YOUR_SCORE, smallBuffer);
		smallBuffer->append('\n');
		//app->localization->composeText(Strings::FILE_CODESTRINGS, (short)100, smallBuffer);
		//smallBuffer->dehyphenate();
		//graphics->drawString(smallBuffer, this->SCR_CX, n3, 1);
	}
	//else if (this->highScoreIndex == -2) {
		app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::PRESS_OK, smallBuffer);
		smallBuffer->dehyphenate();
		graphics->drawString(smallBuffer, this->SCR_CX, n3, 1);
	/* }
	else {
		app->localization->composeText(Strings::FILE_CODESTRINGS, (short)159, smallBuffer);
		smallBuffer->dehyphenate();
		graphics->drawString(smallBuffer, this->SCR_CX, n3, 1);
	}*/
	this->staleView = true;
	smallBuffer->dispose();
}

void Canvas::handleHighScoreInput(int action, int key) {
	Applet* app = CAppContainer::getInstance()->app;
	if (this->stateVars[0] > app->gameTime) {
		this->clearEvents(1);
		return;
	}

	{	// IOS
		app->game->saveConfig();
		this->kickingPhase = 0;
		if (!this->kickingFromMenu) {
			app->player->addXP(10);
		}
	}
	/*if (this->highScoreIndex < 0) {
		if (action == Enums::ACTION_FIRE || action == Enums::ACTION_BACK) {
			this->kickingPhase = 0;
			if (!this->kickingFromMenu) {
				app->player->addXP(10);
			}
			app->hud->repaintFlags |= 0x4;
			this->repaintFlags |= 0x2;
			app->game->saveConfig();
		}
		return;
	}

	int keyAction = this->getKeyAction(key);

	if (key >= 48 && key <= 57) {
		this->stateVars[4] = app->upTimeMs + 2000;
		this->highScoreInitials[this->highScoreIndex * 3 + this->stateVars[1]] = this->getNextChar(key - 48, ++this->stateVars[2]);
	}
	else if (key == 42 || action == Enums::ACTION_LEFT || action == Enums::ACTION_BACK) {
		--this->stateVars[1];
		if (this->stateVars[1] < 0) {
			this->stateVars[1] = 0;
		}
		this->stateVars[2] = -1;
	}
	else if (key == 35 || action == Enums::ACTION_FIRE || action == Enums::ACTION_RIGHT) {
		++this->stateVars[1];
		if (this->stateVars[1] >= 3) {
			this->highScoreIndex = -2;
		}
		else {
			this->stateVars[2] = -1;
		}
	}*/
}

void Canvas::drawKickingGrid(Graphics* graphics) {
	Applet* app = CAppContainer::getInstance()->app;

	int n = this->SCR_CX - (this->imgChickenKicking_ScoreBG->width >> 1);
	int n2 = ((236 - this->imgChickenKicking_ScoreBG->height) >> 1) + 20;
	Text* largeBuffer = app->localization->getLargeBuffer();
	largeBuffer->setLength(0);
	app->localization->resetTextArgs();
	app->localization->addTextArg(this->lastScore);
	app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::POINTS, largeBuffer);
	graphics->drawBoxedString(largeBuffer, this->SCR_CX, n2 - Applet::FONT_HEIGHT[app->fontType] - 6, 2, 0xFF000000, 0xFFFFFFFF);
	graphics->drawImage(this->imgChickenKicking_ScoreBG, n, n2, 0, 0, 0);
	
	// J2ME
	/*int n3 = n;
	int n4 = n2;
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 5; ++j, n3 += 20) {
			int n5 = i * 5 + j;
			int n6 = Canvas::GRID_COLORS[n5];
			if (n5 == this->gridIdx && (app->gameTime & 0x100) != 0x0) {
				n6 = 0xFF176300;
			}
			graphics->fillRect(n3, n4, 20, 20, n6);
			graphics->drawRect(n3, n4, 20, 20, -1);
			largeBuffer->setLength(0);
			largeBuffer->append(Canvas::CKPOINTS[20 - n5 - 1]);
			graphics->drawString(largeBuffer, n3 + 10 - 1, n4 + 10 + 1, 3);
		}
		n3 = n;
		n4 += 20;
	}*/
	if ((app->gameTime & 0x100) != 0x0) {
		//graphics->drawRect(n + this->gridIdx % 5 * 20, n2 + this->gridIdx / 5 * 20, 20, 20, 0xFF000000); // J2ME

		int v9 = Canvas::CKPOINTS[20 - this->gridIdx - 1];
		int v10 = n + (this->gridIdx % 5) * 38 + 1;
		int v11 = n2 + (this->gridIdx / 5) * 38 + 1;
		switch (v9)
		{
		case 20:
			graphics->drawImage(this->imgChickenKicking_20Highlight, v10, v11, 0, 0, 0);
			break;
		case 30:
			graphics->drawImage(this->imgChickenKicking_30Highlight, v10, v11, 0, 0, 0);
			break;
		case 5:
			graphics->drawImage(this->imgChickenKicking_05Highlight, v10, v11, 0, 0, 0);
			break;
		}
	}
	largeBuffer->setLength(0);
	app->localization->resetTextArgs();
	app->localization->addTextArg(app->game->scriptStateVars[Enums::CODEVAR_KICKING_TURN] + 1);
	app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::KICK_TURNS, largeBuffer);
	graphics->drawBoxedString(largeBuffer, this->SCR_CX, n2 + this->imgChickenKicking_ScoreBG->height + 7, 2, 0xFF000000, 0xFFFFFFFF);
	largeBuffer->dispose();
}

char Canvas::getNextChar(int n, int n2) { // J2ME
	int n3 = this->stateVars[3];
	this->stateVars[3] = n;
	if (n != n3) {
		if (this->stateVars[1] < 2 && this->stateVars[2] != 0) {
			++this->stateVars[1];
		}
		n2 = this->stateVars[2] = 0;
	}
	//n2 = (this->stateVars[2] = n2 % Canvas::numCharTable[n].length());
	//return Canvas::numCharTable[n].charAt(n2);
	return 0;
}

void Canvas::drawTravelMap(Graphics* graphics) {
	Applet* app = CAppContainer::getInstance()->app;
	Text* smallBuffer;

	graphics->drawImage(this->imgTravelBG,
		this->SCR_CX,
		(this->displayRect[3] - this->imgTravelBG->height) >> 1,
		17,
		0,
		0);

	int v6 = this->displayRect[2] / 2 - this->imgTravelBG->width / 2;
	if (v6 < 0)
		v6 = 0;

	int v7 = this->displayRect[3] / 2 - this->imgTravelBG->height / 2;
	if (v7 < 0)
		v7 = 0;

	smallBuffer = app->localization->getSmallBuffer();
	smallBuffer->setLength(0);
	app->localization->composeText(Strings::FILE_MENUSTRINGS, app->game->levelNames[this->loadMapID - 1], smallBuffer);
	smallBuffer->dehyphenate();
	graphics->drawString(smallBuffer, this->SCR_CX, 5, 17);

	if (this->stateVars[4] == 1)
	{
		smallBuffer->setLength(0);
		app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::PRESS_OK, smallBuffer);
		smallBuffer->dehyphenate();
		graphics->drawString(smallBuffer, this->SCR_CX, this->displayRect[3] - 2, 33);
	}

	if (this->drawTMArrow(graphics, v6, v7) && this->drawMagGlass(graphics, v6, v7)) {
		this->drawSelector(graphics, v6, v7);
	}

	smallBuffer->dispose();

	this->staleView = true;
}

bool Canvas::drawTMArrow(Graphics* graphics, int n, int n2) {
	Applet* app = CAppContainer::getInstance()->app;

	bool b = true;
	int n3;
	if (this->lastMapID >= 0 && this->lastMapID <= 3 && this->loadMapID == 4) {
		n3 = 0;
	}
	else if (this->lastMapID >= 4 && this->lastMapID <= 6 && this->loadMapID == 3) {
		n3 = 4;
	}
	else if (this->lastMapID >= 4 && this->lastMapID <= 6 && this->loadMapID == 7) {
		n3 = 8;
	}
	else {
		if (this->lastMapID <= 6 || this->loadMapID != 6) {
			return b;
		}
		n3 = 12;
	}

	int n4 = app->upTimeMs - this->stateVars[0];
	if (n4 > 700 && this->stateVars[1] == 0) {
		this->stateVars[1] = 1;
		this->stateVars[0] = app->upTimeMs;
	}
	if (this->stateVars[1] == 1 || this->stateVars[4] == 1) {
		graphics->drawRegion(this->imgTMArrow, 0, 0, this->imgTMArrow->width, this->imgTMArrow->height, Canvas::ARROW_DATA[n3 + 0] + n, Canvas::ARROW_DATA[n3 + 1] + n2, 0, Canvas::ARROW_DATA[n3 + 2], 0);
	}
	else {
		b = false;
		int n5 = (n4 << 16) / 700;
		int texW = n5 * this->imgTMArrow->width >> 16;
		int texH = this->imgTMArrow->height;
		int posX = 0;
		int posY = 0;
		if (Canvas::ARROW_DATA[n3 + 3] == 0) {
			texH = texW;
			texW = this->imgTMArrow->width;
			posX = this->imgTMArrow->height - texH;
		}
		else if (Canvas::ARROW_DATA[n3 + 3] == 1) {
			posX = this->imgTMArrow->width - texW;
		}
		if (texW > 0) {
			graphics->drawRegion(this->imgTMArrow, 0, 0, texW, texH, Canvas::ARROW_DATA[n3 + 0] + posX + n, Canvas::ARROW_DATA[n3 + 1] + posY + n2, 0, Canvas::ARROW_DATA[n3 + 2], 0);
		}
	}
	return b;

}

bool Canvas::drawSelector(Graphics* graphics, int n, int n2) {
	Applet* app = CAppContainer::getInstance()->app;

	int n3 = app->upTimeMs - this->stateVars[0];
	bool b = true;
	int max = std::max(1, std::min(this->loadMapID, 9));
	int max2 = std::max(1, std::min(this->lastMapID, 9));
	int n4 = (max - 1) / 3;
	int n5 = n4 * 5;
	int n6 = (max - 1) % 3;
	int n7 = Canvas::MAG_DATA[n5 + 3] + Canvas::SELECTORPOS[(max - 1) * 2 + 0];
	int n8 = Canvas::MAG_DATA[n5 + 4] + Canvas::SELECTORPOS[(max - 1) * 2 + 1];
	if (this->stateVars[4] == 1 || n3 > 500 || (n6 == 0 && this->lastMapID < this->loadMapID) || n4 != (max2 - 1) / 3) {
		this->stateVars[4] = 1;
		if ((n3 & 0x200) != 0x0) {
			graphics->drawRegion(this->imgTMHighlight, 0, 0, this->imgTMHighlight->width, this->imgTMHighlight->height, n7 + n, n8 + n2, 0, 0, 0);
		}
	}
	else {
		int n9 = (n3 << 16) / 500;
		int n10 = Canvas::MAG_DATA[n5 + 3] + Canvas::SELECTORPOS[(max2 - 1) * 2 + 0];
		int n11 = Canvas::MAG_DATA[n5 + 4] + Canvas::SELECTORPOS[(max2 - 1) * 2 + 1];
		graphics->drawRegion(this->imgTMHighlight, 0, 0, this->imgTMHighlight->width, this->imgTMHighlight->height, n10 + (n9 * (n7 - n10) >> 16) + n, n11 + (n9 * (n8 - n11) >> 16), 0, 0, 0);
		b = false;
	}
	return b;
}

bool Canvas::drawMagGlass(Graphics* graphics, int n, int n2) {
	Applet* app = CAppContainer::getInstance()->app;

	int n3 = app->upTimeMs - this->stateVars[0];
	bool b = true;
	int n4 = 0;
	int n5 = 0;
	int max = std::max(1, std::min(this->loadMapID, 9));
	int n6 = (max - 1) / 3;
	if (n6 != (this->lastMapID - 1) / 3 || max == 1) {
		switch (n6) {
			case 0: {
				n4 = this->displayRect[2] + n;
				n5 = this->SCR_CY + 30 + n2;
				break;
			}
			case 1: {
				n4 = this->displayRect[0] - 244 + n;
				n5 = this->SCR_CY - 20 + n2;
				break;
			}
			default: {
				n4 = this->displayRect[2] + n;
				n5 = this->SCR_CY - 40 + n2;
				break;
			}
		}
	}
	else {
		this->stateVars[2] = 1;
		this->stateVars[3] = 1;
	}
	int n7 = n6 * 5;
	if (n3 > 1200 && this->stateVars[2] == 0) {
		this->stateVars[2] = 1;
		this->stateVars[0] = app->upTimeMs;
		n3 = 0;
	}
	if (this->stateVars[2] == 1 || this->stateVars[4] == 1) {
		graphics->drawRegion(this->imgMagGlass, 0, 0, this->imgMagGlass->width, this->imgMagGlass->height, Canvas::MAG_DATA[n7 + 0] + n, Canvas::MAG_DATA[n7 + 1] + n2, 0, Canvas::MAG_DATA[n7 + 2], 0);
		graphics->drawRegion(this->imgTMLevels, 0, 0, this->imgTMLevels->width, this->imgTMLevels->height, Canvas::MAG_DATA[n7 + 3] + n + 13, Canvas::MAG_DATA[n7 + 4] + n2 + 4, 0, 0, 0);
		if (n3 > 300 && this->stateVars[3] == 0) {
			this->stateVars[3] = 1;
			this->stateVars[0] = (app->upTimeMs | 0x200);
		}
		if (this->stateVars[4] == 0 && this->stateVars[3] == 0) {
			int n8 = n3 / 100;
			b = false;
			graphics->drawRegion(this->imgFlak, 128 * (n8 & 0x3), 0, 128, 128, Canvas::MAG_DATA[n7 + 3] + n, Canvas::MAG_DATA[n7 + 4] + n2, 0, 0, 0);
		}
	}
	else {
		int n9 = (n3 << 16) / 1200;
		int n10 = Canvas::MAG_DATA[n7 + 2];

		int posX = (n4 << 16) + n9 * (Canvas::MAG_DATA[n7 + 0] + n - n4) >> 16;
		int posY = (n5 << 16) + n9 * (Canvas::MAG_DATA[n7 + 1] + n2 - n5) >> 16;
		graphics->drawRegion(this->imgMagGlass, 0, 0, this->imgMagGlass->width, this->imgMagGlass->height, posX, posY, 128, Canvas::MAG_DATA[n7 + 2], 0);

		if (n10 == 4) {
			posX += (244 - 22) - this->imgMagBG->width;
			posY += 7;
		}
		else if (n10 == 6) {
			posX += 22;
			posY += (244 - 7) - this->imgMagBG->height;
		}
		else {
			posX += 22;
			posY += 7;
		}

		graphics->drawRegion(this->imgMagBG, 0, 0, this->imgMagBG->width, this->imgMagBG->height, posX, posY , 128, Canvas::MAG_DATA[n7 + 2], 2);

		b = false;
	}
	return b;
}

void Canvas::initTravelMap() {
	Applet* app = CAppContainer::getInstance()->app;

	int index = (std::max(1, std::min(this->loadMapID, 9)) - 1) / 3;

	app->beginImageLoading();

	this->imgTMArrow = app->loadImage("arrow.bmp", true);
	this->imgFlak = app->loadImage("flak.bmp", true);
	this->imgTMHighlight = app->loadImage("highlight.bmp", true);
	this->imgMagGlass = app->loadImage("magnifyingGlass.bmp", true);
	this->imgMagBG = app->loadImage("magnifyingGlassInsert.bmp", true);

	if (index == 0) {
		this->imgTMLevels = app->loadImage("TM_Levels1.bmp", true);
	}
	else if (index == 1) {
		this->imgTMLevels = app->loadImage("TM_Levels2.bmp", true);
	}
	else if (index == 2) {
		this->imgTMLevels = app->loadImage("TM_Levels3.bmp", true);
	}
	else {
		printf("ERROR: Failed to find background image for index: %d \n", index);
	}

	this->imgTravelBG = app->loadImage("TravelMap.bmp", true);

	app->finishImageLoading();
	this->stateVars[0] = app->upTimeMs;
}

void Canvas::disposeTravelMap() {
	this->imgTMArrow->~Image();
	this->imgTMArrow = nullptr;
	this->imgTravelBG->~Image();
	this->imgTravelBG = nullptr;
	this->imgMagGlass->~Image();
	this->imgMagGlass = nullptr;
	this->imgMagBG->~Image();
	this->imgMagBG = nullptr;
	this->imgTMLevels->~Image();
	this->imgTMLevels = nullptr;
	this->imgTMHighlight->~Image();
	this->imgTMHighlight = nullptr;
	this->imgFlak->~Image();
	this->imgFlak = nullptr;
}

void Canvas::handleTravelMapInput(int key, int action) {
	if (action == Enums::ACTION_MENU) { // [GEC] skip all
		this->finishTravelMapAndLoadLevel();
		return;
	}

	if ((this->stateVars[4] == 1) && ((key == 18) || (action == Enums::ACTION_FIRE))) {
		this->finishTravelMapAndLoadLevel();
	}
	else {
		this->stateVars[4] = 1;
	}
}

void Canvas::finishTravelMapAndLoadLevel() {
	this->clearSoftKeys();
	this->disposeTravelMap();
	this->setLoadingBarText(Strings::FILE_CODESTRINGS, CodeStrings::LOADING);
	this->setState(Canvas::ST_LOADING);
}

void Canvas::playIntroMovie(Graphics* graphics) {
	Applet* app = CAppContainer::getInstance()->app;

	if (app->canvas->skipIntro != false) {
		this->backToMain(true);
		return;
	}

	if (app->game->hasSeenIntro && app->game->skipMovie) {
		this->exitIntroMovie(false);
		return;
	}

	if (this->stateVars[1] == 0) {  // load table camera
		this->stateVars[1] = app->gameTime;
		app->game->loadTableCamera(14, 15);
		this->numEvents = 0;
		this->keyDown = false;
		this->keyDownCausedMove = false;
		this->ignoreFrameInput = 1;
		this->imgProlog = app->loadImage("prolog.bmp", true);
		this->imgProlog_0_1 = app->loadImage("prolog_0_1.bmp", true);
		this->imgProlog_1_0 = app->loadImage("prolog_1_0.bmp", true);
		this->imgProlog_1_1 = app->loadImage("prolog_1_1.bmp", true);
		this->imgProlog_2_0 = app->loadImage("prolog_2_0.bmp", true);
		this->imgProlog_2_1 = app->loadImage("prolog_2_1.bmp", true);

		app->beginImageLoading();
		this->imgFire = app->loadImage("fire.bmp", true);
		this->imgFlak = app->loadImage("flak.bmp", true);
		app->finishImageLoading();
	}

	if (app->game) {
		if (app->game->mayaCameras)
		{
			if (this->stateVars[3] == 0) { // init movie prolog
				if (this->stateVars[1] < app->gameTime) {
					app->game->activeCameraKey = -1;
					app->game->mayaCameras->NextKey();
					this->stateVars[3] = 0;
					this->stateVars[1] = app->gameTime;
					this->stateVars[2] = 0;
					this->stateVars[0] = 0;
					this->fadeRect = this->displayRect;
					this->fadeFlags = Canvas::FADE_FLAG_FADEIN;
					this->fadeColor = 0;
					this->fadeTime = app->time;
					this->fadeDuration = 1500;
					this->stateVars[3] = 1;
				}
			}
			else if (this->stateVars[3] == 1) { // draw movie prolog
				if (this->displayRect[3] > 320) {
					graphics->clipRect(0, (this->displayRect[3] - 320) / 2, this->displayRect[2], 320);
				}

				this->stateVars[0] = app->gameTime - app->game->activeCameraTime;
				this->stateVars[2] = app->gameTime - this->stateVars[1];

				app->game->mayaCameras->Update(app->game->activeCameraKey, this->stateVars[0]);

				int uVar8 = app->game->posShift - 1;
				int posY = (app->game->mayaCameras->y >> (uVar8 & 0xff)) - this->SCR_CY - 60;
				int posX = (app->game->mayaCameras->x >> (uVar8 & 0xff)) - this->SCR_CX;
				int posX1, posX2, posX3;
				int posY1, posY2, posY3;
				
				if (posX < 0) {
					posX = 0;
					posX1 = 0;
					posX2 = 512;
					posX3 = 1024;
				}
				else {
					posX1 = -posX;
					posX2 = 512 - posX;
					posX3 = 1024 - posX;
				}

				if (posY < 0) {
					posY1 = 0;
					posY2 = 512;
					posY3 = 0;
				}
				else {
					posY1 = -posY;
					posY2 = 512 - posY;
					posY3 = posY;
				}

				posY = -posY;
				if (posY < 0) {
					posY = 0;
				}

				graphics->drawRegion(this->imgProlog, 0, 0, 512, 512, posX1, posY1, 0, 0, 0);
				graphics->drawRegion(this->imgProlog_0_1, 0, 0, 512, 184, posX1, posY2, 0, 0, 0);
				graphics->drawRegion(this->imgProlog_1_0, 0, 0, 512, 512, posX2, posY1, 0, 0, 0);
				graphics->drawRegion(this->imgProlog_1_1, 0, 0, 512, 184, posX2, posY2, 0, 0, 0);
				graphics->drawRegion(this->imgProlog_2_0, 0, 0, 176, 512, posX3, posY1, 0, 0, 0);
				graphics->drawRegion(this->imgProlog_2_1, 0, 0, 176, 184, posX3, posY2, 0, 0, 0);

				this->drawMovieEffects(graphics, this->stateVars[2], posX, posY3 - posY);

				if (app->game->mayaCameras->complete) {
					this->stateVars[3] += 1;
				}
			}
			else if (this->stateVars[3] == 2) { // init Scrolling Text
				app->sound->playSound(1074, 0, 6, false);
				this->initScrollingText(Strings::FILE_CODESTRINGS, CodeStrings::PROLOGUE_SCROLLING_TXT, true, Applet::FONT_HEIGHT[app->fontType] * 2, 1, 760);
				this->stateVars[3] += 1;
			}
			else if (this->stateVars[3] == 3) { // draw Scrolling Text
				this->drawScrollingText(graphics);
				if (this->scrollingTextDone) {
					this->exitIntroMovie(false);
				}
			}

			this->staleView = true;
			return;
		}
	}
}

void Canvas::drawMovieEffects(Graphics* graphics, int time, int x, int y)
{
	Applet* app = CAppContainer::getInstance()->app;
	Image* img;
	int32_t* effectData = this->movieEffects;
	int effectCount = app->resource->getNumTableInts(8) / 6;

	if (effectCount == 0 || !effectData) return;

	const int effectInterval = 1337;
	const int endTime = time + (effectCount * effectInterval);

	for (int i = 0, currentTime = time; i < effectCount; ++i, currentTime += effectInterval) {
		int32_t* data = &effectData[i * 6];

		// Validación de la cámara activa
		if (data[3] <= app->game->activeCameraKey && data[4] == -1) {
			data[4] = time;
			data[5] += time;
		}

		int startTime = data[4];
		int endTimeEffect = data[5];

		if (startTime != -1 && startTime <= time && time < endTimeEffect) {
			int texX, renderMode, imgSize, imgCenter;

			if (data[0] == 0) {  // Flak Effect
				img = this->imgFlak;
				texX = ((time - startTime) * 0x10000) / 0xc80000;
				renderMode = 2;
				imgSize = 128;
			}
			else {  // Fire Effect
				img = this->imgFire;
				texX = (currentTime / 256) & 3;
				renderMode = 3;
				imgSize = 88;
			}

			imgCenter = imgSize / 2;
			int posX = (data[1] * 2 - imgCenter) - x;
			int posY = (data[2] * 2 - imgCenter) - y;

			graphics->drawRegion(img, texX * imgSize, 0, imgSize, imgSize, posX, posY, 0, 0, renderMode);
		}
	}
}

void Canvas::exitIntroMovie(bool b) {
	Applet* app = CAppContainer::getInstance()->app;

	app->game->cleanUpCamMemory();

	this->imgProlog->~Image();
	this->imgProlog = nullptr;
	this->imgProlog_0_1->~Image();
	this->imgProlog_0_1 = nullptr;
	this->imgProlog_1_0->~Image();
	this->imgProlog_1_0 = nullptr;
	this->imgProlog_1_1->~Image();
	this->imgProlog_1_1 = nullptr;
	this->imgProlog_2_0->~Image();
	this->imgProlog_2_0 = nullptr;
	this->imgProlog_2_1->~Image();
	this->imgProlog_2_1 = nullptr;
	this->imgFire->~Image();
	this->imgFire = nullptr;
	this->imgFlak->~Image();
	this->imgFlak = nullptr;

	if (this->dialogBuffer) {
		this->dialogBuffer->dispose();
		this->dialogBuffer = nullptr;
	}

	app->sound->soundStop();
	if (b == false) {
		app->game->hasSeenIntro = true;
		app->game->saveConfig();
		this->backToMain(false);
		app->sound->playSound(1078, 1u, 6, 0);
	}
}

void Canvas::setMenuDimentions(int x, int y, int w, int h)
{
	this->menuRect[0] = x;
	this->menuRect[1] = y;
	this->menuRect[2] = w;
	this->menuRect[3] = h;
}

void Canvas::setBlendSpecialAlpha(float alpha) {

	if (alpha < 0.0f) {
		alpha = 0.0f;
	}
	else if (alpha > 1.0f) {
		alpha = 1.0f;
	}

	this->blendSpecialAlpha = alpha;
}

void Canvas::touchStart(int pressX, int pressY) {
	Applet* app = CAppContainer::getInstance()->app;
	this->touched = false;

	if (this->state == Canvas::ST_MENU) {
		app->menuSystem->handleUserTouch(pressX, pressY, true);
	}
	else if (this->state == Canvas::ST_MINI_GAME) {
		app->cardGames->touchStart(pressX, pressY);
	}
	else if ((this->state == Canvas::ST_PLAYING) || (this->state == Canvas::ST_COMBAT) || (this->state == Canvas::ST_DRIVING)) {
		
		if (this->isChickenKicking && (this->kickingPhase == 1 || this->kickingPhase == 2)) {
			this->keyPressed(6);
		}
		else if (!this->isZoomedIn) {
			app->hud->handleUserTouch(pressX, pressY, true);

			if (!app->hud->isInWeaponSelect) {
				this->m_controlButtons[this->m_controlMode + 0]->HighlightButton(pressX, pressY, true);
				this->m_controlButtons[this->m_controlMode + 2]->HighlightButton(pressX, pressY, true);
				this->m_controlButtons[this->m_controlMode + 4]->HighlightButton(pressX, pressY, true);

				this->m_controlButton = nullptr;
				fmButton* button;
				if (!this->isZoomedIn && (
					(button = this->m_controlButtons[this->m_controlMode + 0]->GetTouchedButton(pressX, pressY)) ||
					(button = this->m_controlButtons[this->m_controlMode + 2]->GetTouchedButton(pressX, pressY)) ||
					(button = this->m_controlButtons[this->m_controlMode + 4]->GetTouchedButton(pressX, pressY))) &&
					(button->buttonID != 6)) {
					this->m_controlButton = button;
					this->m_controlButtonIsTouched = true;
				}
				else if (this->m_swipeArea[this->m_controlMode]->rect.ContainsPoint(pressX, pressY))
				{
					this->m_swipeArea[this->m_controlMode]->touched = true;
					this->m_swipeArea[this->m_controlMode]->begX = pressX;
					this->m_swipeArea[this->m_controlMode]->begY = pressY;
					this->m_swipeArea[this->m_controlMode]->curX = -1;
					this->m_swipeArea[this->m_controlMode]->curY = -1;
					return;
				}
				this->m_swipeArea[this->m_controlMode]->touched = false;
			}
		}
		else {
			if (this->m_sniperScopeDialScrollButton->field_0x0_ &&
				this->m_sniperScopeDialScrollButton->barRect.ContainsPoint(pressX, pressY)) {
				this->m_sniperScopeDialScrollButton->SetTouchOffset(pressX, pressY);
				this->m_sniperScopeDialScrollButton->field_0x14_ = 1;
			}
			else {
				app->hud->handleUserTouch(pressX, pressY, true);
				if (!app->hud->isInWeaponSelect) {
					this->m_sniperScopeButtons->HighlightButton(pressX, pressY, true);
				}
			}
		}
	}
	else if (this->state == Canvas::ST_AUTOMAP) {
		//puts("touch in automap!");
		this->m_softKeyButtons->HighlightButton(pressX, pressY, true);
		this->m_controlButtons[this->m_controlMode + 0]->HighlightButton(pressX, pressY, true);
		this->m_controlButtons[this->m_controlMode + 2]->HighlightButton(pressX, pressY, true);
		this->m_controlButton = nullptr;
		fmButton* button;
		if (((button = this->m_controlButtons[this->m_controlMode + 0]->GetTouchedButton(pressX, pressY)) ||
			(button = this->m_controlButtons[this->m_controlMode + 2]->GetTouchedButton(pressX, pressY))) &&
			(button->buttonID != 6)) {
			this->m_controlButton = button;
			this->m_controlButtonIsTouched = true;
		}
	}
	else if (this->state == Canvas::ST_MIXING) {
		this->m_mixingButtons->HighlightButton(pressX, pressY, true);
	}
	else if (this->state == Canvas::ST_INTRO) {
		this->m_storyButtons->HighlightButton(pressX, pressY, true);
	}
	else if (this->state == Canvas::ST_DIALOG) {
		this->m_dialogButtons->HighlightButton(pressX, pressY, true);
	}
	else if (this->state == Canvas::ST_CAMERA) { // [GEC ]Port: New
		this->m_softKeyButtons->HighlightButton(pressX, pressY, true);
	}
}

void Canvas::touchMove(int pressX, int pressY) {
	Applet* app = CAppContainer::getInstance()->app;
	fmSwipeArea::SwipeDir swDir;

	//this->touched = true; // Old

	// [GEC] Evita falsos toques en la pantalla
	const int begMouseX = (int)(gBegMouseX * Applet::IOS_WIDTH);
	const int begMouseY = (int)(gBegMouseY * Applet::IOS_HEIGHT);
	if (!pointInRectangle(pressX, pressY, begMouseX - 3, begMouseY - 3, 6, 6)) {
		this->touched = true;
	}

	if (this->state == Canvas::ST_MENU) {
		app->menuSystem->handleUserMoved(pressX, pressY);
	}
	else if (this->state == Canvas::ST_MINI_GAME) {
		app->cardGames->touchMove(pressX, pressY);
	}
	else if ((this->state == Canvas::ST_PLAYING) || (this->state == Canvas::ST_COMBAT) || (this->state == Canvas::ST_DRIVING)) {

		if (this->isZoomedIn) {
			if (this->m_sniperScopeDialScrollButton->field_0x14_) {
				this->m_sniperScopeDialScrollButton->Update(pressX, pressY);

				int field_0x0 = this->m_sniperScopeDialScrollButton->field_0x0_;
				if (this->m_sniperScopeDialScrollButton->field_0x0_) {
					field_0x0 = 101 * this->m_sniperScopeDialScrollButton->field_0x44_;
				}
				this->zoomDestFOV = field_0x0 / 15 + 80;

				// [GEC] update zoomCurFOVPercent
				{
					float maxScroll = (float)((this->m_sniperScopeDialScrollButton->barRect).h - this->m_sniperScopeDialScrollButton->field_0x4c_);
					float yScroll = (float)((float)this->m_sniperScopeDialScrollButton->field_0x48_ / maxScroll);
					this->zoomCurFOVPercent = this->zoomMinFOVPercent - (int)((float)this->zoomMinFOVPercent * yScroll);
				}
			}
			else {
				if (this->m_sniperScopeDialScrollButton->field_0x0_
					&& this->m_sniperScopeDialScrollButton->barRect.ContainsPoint(pressX, pressY))
				{
					this->m_sniperScopeDialScrollButton->SetTouchOffset(pressX, pressY);
					this->m_sniperScopeDialScrollButton->field_0x14_ = 1;
					return;
				}

				app->hud->handleUserMoved(pressX, pressY);
				if (!app->hud->isInWeaponSelect) {
					this->m_sniperScopeButtons->HighlightButton(pressX, pressY, true);
				}
			}
		}
		else {
			app->hud->handleUserMoved(pressX, pressY);
			if (!app->hud->isInWeaponSelect) {
				if (this->m_swipeArea[this->m_controlMode]->touched) {
					if (this->m_swipeArea[this->m_controlMode]->UpdateSwipe(pressX, pressY, &swDir)) {
						this->touched = true;
						this->touchSwipe(swDir);
						return;
					}
				}
				else {
					this->m_controlButtons[this->m_controlMode + 0]->HighlightButton(pressX, pressY, true);
					this->m_controlButtons[this->m_controlMode + 2]->HighlightButton(pressX, pressY, true);
					this->m_controlButtons[this->m_controlMode + 4]->HighlightButton(pressX, pressY, true);

					if (this->m_controlButton && !this->m_controlButton->highlighted) {
						this->m_controlButton = nullptr;
					}
					this->m_controlButton = nullptr;

					fmButton* button;
					if (!this->isZoomedIn && (
						(button = this->m_controlButtons[this->m_controlMode + 0]->GetTouchedButton(pressX, pressY)) ||
						(button = this->m_controlButtons[this->m_controlMode + 2]->GetTouchedButton(pressX, pressY)) ||
						(button = this->m_controlButtons[this->m_controlMode + 4]->GetTouchedButton(pressX, pressY))) &&
						(button->buttonID != 6)) {
						this->m_controlButton = button;
						this->m_controlButtonIsTouched = true;
					}
				}
			}
		}
	}
	else if (this->state == Canvas::ST_AUTOMAP) {
		this->m_softKeyButtons->HighlightButton(pressX, pressY, true);
		this->m_controlButtons[this->m_controlMode + 0]->HighlightButton(pressX, pressY, true);
		this->m_controlButtons[this->m_controlMode + 2]->HighlightButton(pressX, pressY, true);

		if (this->m_controlButton && !this->m_controlButton->highlighted) {
			this->m_controlButton = nullptr;
		}
		this->m_controlButton = nullptr;

		fmButton* button;
		if (((button = this->m_controlButtons[this->m_controlMode + 0]->GetTouchedButton(pressX, pressY)) ||
			(button = this->m_controlButtons[this->m_controlMode + 2]->GetTouchedButton(pressX, pressY))) &&
			(button->buttonID != 6)) {
			this->m_controlButton = button;
			this->m_controlButtonIsTouched = true;
		}
	}
	else if (this->state == Canvas::ST_MIXING) {
		this->m_mixingButtons->HighlightButton(pressX, pressY, true);
	}
	else if (this->state == Canvas::ST_INTRO) {
		this->m_storyButtons->HighlightButton(pressX, pressY, true);
	}
	else if (this->state == Canvas::ST_DIALOG) {
		this->m_dialogButtons->HighlightButton(pressX, pressY, true);
	}
	else if (this->state == Canvas::ST_CAMERA) { // [GEC]: New
		this->m_softKeyButtons->HighlightButton(pressX, pressY, true);
	}
}

void Canvas::touchEnd(int pressX, int pressY) {
	Applet* app = CAppContainer::getInstance()->app;
	uint32_t result;

	//printf("state %d\n", this->state);
	if (this->state == Canvas::ST_MENU) {
		app->menuSystem->handleUserTouch(pressX, pressY, false);
	}
	else if (this->state == Canvas::ST_INTRO) {
		result = this->m_storyButtons->GetTouchedButtonID(pressX, pressY);
		if (result != 1) {
			if (result == 2) {
				if (this->storyPage >= this->storyTotalPages - 1) {
					return;
				}
			}
			else if (result != 0) {
				return;
			}
		}
		this->stateVars[0] = result;
		this->keyPressed(6);
		return;
	}
	else if (this->state == Canvas::ST_DIALOG) {
		uint32_t result = this->m_dialogButtons->GetTouchedButtonID(pressX, pressY);
		if (((result - 7 <= 1) || (result == 6)) && (this->currentDialogLine >= this->numDialogLines - this->dialogViewLines)) {
			if ((this->dialogFlags & Enums::DLG_FLAG_INTERROGATE) || (this->dialogFlags & Enums::DLG_FLAG_GAME) || (this->dialogFlags & Enums::DLG_FLAG_YESNO)) {
				return;
			}

			this->keyPressed(6);
			return;
		}
		else {
			if (result - 5 <= 1 && this->numDialogLines > this->dialogViewLines) {
				if (result == 5) {
					this->keyPressed(19);
				}
				else {
					this->keyPressed(6);
				}
				return;
			}

			if (this->dialogFlags & Enums::DLG_FLAG_INTERROGATE) {
				if (result <= 2) {
					app->game->scriptStateVars[Enums::CODEVAR_DIALOG_CHOICE] = result;
					this->keyPressed(6);
				}
				return;
			}
			else if (this->dialogFlags != Enums::DLG_FLAG_NONE) {
				if (this->currentDialogLine >= this->numDialogLines - this->dialogViewLines &&
					((this->dialogFlags & Enums::DLG_FLAG_GAME) || (this->dialogFlags & Enums::DLG_FLAG_YESNO))) {
					if (result - 3 <= 1) {
						if (result == 3) {
							app->game->scriptStateVars[Enums::CODEVAR_DIALOG_CHOICE] = 0;
						}
						else {
							app->game->scriptStateVars[Enums::CODEVAR_DIALOG_CHOICE] = 1;
						}
						this->keyPressed(6);
					}
				}
			}
		}
	}
	else if (this->state == Canvas::ST_MINI_GAME) {
		result = app->cardGames->touchToKey(pressX, pressY);
		this->m_controlButtonIsTouched = false;
		if (result != -1) {
			this->keyPressed(result);
		}
		return;
	}
	else if (this->state == Canvas::ST_AUTOMAP) {
		this->m_controlButton = nullptr;
		result = this->m_softKeyButtons->GetTouchedButtonID(pressX, pressY);
		this->m_controlButtonTime = app->gameTime - 1;
		this->m_controlButtonIsTouched = false;
		if (result != -1) {
			this->keyPressed(result);
		}
		return;
	}
	else if (this->state == Canvas::ST_COMBAT || this->state == Canvas::ST_PLAYING || this->state == Canvas::ST_DRIVING) {
		result = this->touchToKey_Play(pressX, pressY);
		this->m_controlButtonIsTouched = false;
		if (result != -1) {
			this->keyPressed(result);
		}
		return;
	}
	else if (this->state == Canvas::ST_MIXING) {
		if (this->mixingMsg) {
			this->keyPressed(6);
			return;
		}
		result = this->m_mixingButtons->GetTouchedButtonID(pressX, pressY);
		switch (result) {
		case 0:
		case 1:
		case 2:
			if (this->stateVars[result + 6])
			{
				this->stateVars[4] = result;
				if (this->stateVars[0] <= 2) {
					this->keyPressed(6);
				}
			}
			return;
		case 3:
			this->keyPressed(19);
			return;
		case 4:
			this->keyPressed(20);
			return;
		case 5:
			if (this->stateVars[0] == 3) {
				this->keyPressed(6);
			}
			return;
		default:
			return;
		}
	}
	else if (this->state != Canvas::ST_CAMERA || (app->canvas->softKeyRightID != -1) && (result = this->m_softKeyButtons->GetTouchedButtonID(pressX, pressY), result == 20)) {
		this->m_controlButtonIsTouched = false;
		this->keyPressed(6);
		return;
	}
	else {
		this->m_controlButtonIsTouched = false;
	}
}

void Canvas::touchEndUnhighlight() {
	Applet* app = CAppContainer::getInstance()->app;
	if (this->state == Canvas::ST_MINI_GAME) {
		app->cardGames->m_cardGamesButtons->HighlightButton(0, 0, false);
	}
	else if (this->state == Canvas::ST_MIXING) {
		this->m_mixingButtons->HighlightButton(0, 0, false);
	}
	else if (this->state == Canvas::ST_INTRO) {
		this->m_storyButtons->HighlightButton(0, 0, false);
	}

	if (this->state == Canvas::ST_PLAYING || this->state == Canvas::ST_COMBAT || this->state == Canvas::ST_DRIVING || this->state == Canvas::ST_AUTOMAP || this->state == Canvas::ST_DIALOG || this->state == Canvas::ST_CAMERA) {
		this->m_controlButtons[this->m_controlMode]->HighlightButton(0, 0, false);
		this->m_controlButtons[this->m_controlMode + 2]->HighlightButton(0, 0, false);
		this->m_controlButtons[this->m_controlMode + 4]->HighlightButton(0, 0, false);
		this->m_sniperScopeButtons->HighlightButton(0, 0, false);
		this->m_softKeyButtons->HighlightButton(0, 0, false);
		this->m_dialogButtons->HighlightButton(0, 0, false);
	}
}

int Canvas::touchToKey_Play(int pressX, int pressY) {
	Applet* app = CAppContainer::getInstance()->app;
	int result;

	this->m_controlButton = 0;
	if (this->m_sniperScopeDialScrollButton->field_0x14_)
	{
		this->m_sniperScopeDialScrollButton->field_0x14_ = 0;
		return -1;
	}
	this->m_swipeArea[this->m_controlMode]->touched = false;

	if (this->isChickenKicking)
	{
		if (this->kickingPhase >= 1)
		{
			if (this->kickingPhase <= 2)
				return -1;
			if (this->kickingPhase == 5)
				return 6;
		}
	}

	if (((pressY > 256)) || (app->hud->isInWeaponSelect))
	{
		app->hud->handleUserTouch(pressX, pressY, false);
		return -1;
	}
	if (this->touched) {
		return -1;
	}
	if (this->isZoomedIn) {
		return this->m_sniperScopeButtons->GetTouchedButtonID(pressX, pressY);
	}
	if (this->m_controlButtonIsTouched) {
		return -1;
	}

	result = this->m_controlButtons[this->m_controlMode + 0]->GetTouchedButtonID(pressX, pressY);
	if (result == -1) {
		result = this->m_controlButtons[this->m_controlMode + 2]->GetTouchedButtonID(pressX, pressY);
		if (result == -1) {
			result = this->m_controlButtons[this->m_controlMode + 4]->GetTouchedButtonID(pressX, pressY);
		}
	}

	if (result != 6 && this->state != Canvas::ST_DRIVING) {
		return -1;
	}

	return result;
}

void Canvas::drawTouchSoftkeyBar(Graphics* graphics, bool highlighted_Left, bool highlighted_Right) {
	Applet* app = CAppContainer::getInstance()->app;
	Text* SmallBuffer = app->localization->getSmallBuffer();

	graphics->drawImage(app->menuSystem->imgGameMenuPanelbottom,
		0, 320 - app->menuSystem->imgGameMenuPanelbottom->height, 0, 0, 0);

	if (!highlighted_Left || (highlighted_Left && this->softKeyLeftID == -1)) {
		graphics->drawImage(app->hud->imgSwitchLeftNormal, 9, 258, 0, 0, 0);
	}
	else {
		graphics->drawImage(app->hud->imgSwitchLeftActive, 9, 258, 0, 0, 0);
	}

	if (this->softKeyLeftID != -1) {
		SmallBuffer->setLength(0);
		app->localization->composeText(this->softKeyLeftID, SmallBuffer);
		SmallBuffer->dehyphenate();
		graphics->drawString(SmallBuffer, 2, 320, 36);
	}

	if (!highlighted_Right || (highlighted_Right && this->softKeyRightID == -1)) {
		graphics->drawImage(app->hud->imgSwitchRightNormal, 438, 258, 0, 0, 0);
	}
	else {
		graphics->drawImage(app->hud->imgSwitchRightActive, 438, 258, 0, 0, 0);
	}

	if (this->softKeyRightID != -1) {
		SmallBuffer->setLength(0);
		app->localization->composeText(this->softKeyRightID, SmallBuffer);
		SmallBuffer->dehyphenate();
		graphics->drawString(SmallBuffer, 478, 320, 40);
	}
	SmallBuffer->dispose();
}

void Canvas::touchSwipe(int swDir) {
	Applet* app = CAppContainer::getInstance()->app;
	int iVar1;
	int iVar2;

	if (this->state == Canvas::ST_PLAYING || this->state == Canvas::ST_COMBAT) {
#if 0 // IOS
		switch (swDir) {
			case fmSwipeArea::SwipeDir::Left:
				iVar1 = 2;
				break;
			case fmSwipeArea::SwipeDir::Right:
				iVar1 = 4;
				break;
			case fmSwipeArea::SwipeDir::Down:
				iVar1 = AVK_RIGHT;
				break;
			case fmSwipeArea::SwipeDir::Up:
				iVar1 = AVK_LEFT;
				break;
			default:
				return;
		}

		if (this->isZoomedIn != false) {
			bool bVar4 = iVar1 == 2;
			if (bVar4) {
				iVar1 = 5;
			}
			if ((!bVar4) && (iVar1 == 4)) {
				iVar1 = 7;
			}
		}
#else
		switch (swDir) {
			case fmSwipeArea::SwipeDir::Left:
				iVar1 = AVK_MOVELEFT;
				break;
			case fmSwipeArea::SwipeDir::Right:
				iVar1 = AVK_MOVERIGHT;
				break;
			case fmSwipeArea::SwipeDir::Down:
				iVar1 = AVK_DOWN;
				break;
			case fmSwipeArea::SwipeDir::Up:
				iVar1 = AVK_UP;
				break;
			default:
				return;
		}
#endif

		this->keyPressed(iVar1);
	}
}

void Canvas::flipControls() {
	int v1; // r10
	fmButtonContainer** v2; // r6
	int x; // r8
	fmButton* Button; // r5
	fmButton* v5; // r0
	bool v6; // zf
	fmButton* v7; // r4
	fmButton* v8; // r4
	fmButton* v9; // r0
	bool v10; // zf
	fmButton* v11; // r4
	fmButton* v12; // r0
	bool v13; // zf
	fmButton* v14; // r5
	int v15; // r8

	v1 = 0;
	v2 = &this->m_controlButtons[4];
	this->m_controlFlip ^= 1u;
	do
	{
		v2[-4]->FlipButtons();
		v2[-2]->FlipButtons();
		v2[0]->FlipButtons();
		while (1)
		{
			Button = v2[-2]->GetButton(5);
			v5 = v2[-2]->GetButton(7);
			v6 = Button == 0;
			if (Button)
				v6 = v5 == 0;
			v7 = v5;
			if (v6)
				break;
			x = Button->touchArea.x;
			Button->SetTouchArea(v5->touchArea.x, Button->touchArea.y, Button->touchArea.w, Button->touchArea.h);
			v7->SetTouchArea(x, v7->touchArea.y, v7->touchArea.w, v7->touchArea.h);
			Button->buttonID = -2;
			v7->buttonID = -3;
		}
		while (1)
		{
			v8 = v2[-2]->GetButton(-2);
			v9 = v2[-2]->GetButton(-3);
			v10 = v8 == 0;
			if (v8)
				v10 = v9 == 0;
			if (v10)
				break;
			v8->buttonID = 5;
			v9->buttonID = 7;
		}
		v11 = v2[0]->GetButton(2);
		v12 = v2[0]->GetButton(4);
		v13 = v11 == 0;
		if (v11)
			v13 = v12 == 0;
		v14 = v12;
		if (!v13)
		{
			v15 = v11->touchArea.x;
			v11->SetTouchArea(v12->touchArea.x, v11->touchArea.y, v11->touchArea.w, v11->touchArea.h);
			v14->SetTouchArea(v15, v14->touchArea.y, v14->touchArea.w, v14->touchArea.h);
		}
		++v1;
		++v2;
	} while (v1 != 2);
}

void Canvas::setControlLayout() {
	if (this->m_controlLayout == 2) {
		this->m_controlGraphic = 0;
		this->m_controlMode = 1;
	}
	else {
		this->m_controlGraphic = this->m_controlLayout;
		this->m_controlMode = 0;
	}

	for (int i = 0; i < 2; i++) {
		this->m_controlButtons[i + 0]->SetGraphic(this->m_controlGraphic);
		this->m_controlButtons[i + 2]->SetGraphic(this->m_controlGraphic);
	}
}

void Canvas::updateAmbientSounds() {
	Applet* app = CAppContainer::getInstance()->app;

	int loadMapID; // r4
	int ambientSoundsTime; // r0
	int v6; // r4
	int v7; // r10
	int v8; // r11
	int v9; // r8
	int gameTime; // r4
	int16_t Byte; // r0
	bool v12; // [sp+4h] [bp-1Ch]

	if (app->player->ce->weapon == Enums::WP_FLAMETHROWER && !app->game->isUnderWater()) { // [GEC] Add check under water
		if (!app->sound->isSoundPlaying(1029)) {
			app->sound->playSound(1029, 1u, 3, 1);
			app->sound->updateVolume();
		}
	}
	else {
		app->sound->stopSound(1029, true);
	}

	if (app->player->ce->weapon == Enums::WP_TESLA && !app->game->isUnderWater()) { // [GEC] Add check under water
		if (!app->sound->isSoundPlaying(1112)) {
			app->sound->playSound(1112, 1u, 3, 1);
			app->sound->updateVolume();
		}
	}
	else {
		app->sound->stopSound(1112, true);
	}

	if (app->player->statusEffects[(Enums::OFS_STATUSEFFECT_TURNS + Enums::STATUS_EFFECT_FIRE)] <= 0) {
		if (!app->player->soundFire) {
			app->sound->stopSound(1027, true);
			loadMapID = this->loadMapID;
			if (loadMapID != 1)
				goto LABEL_9;
			goto LABEL_27;
		}
	}
	else if (!app->sound->isSoundPlaying( 1027)) {
		app->sound->playSound(1027, 1u, 3, 1);
		app->sound->updateVolume();
	}

	loadMapID = this->loadMapID;
	if (loadMapID != 1) {
		goto LABEL_9;
	}

LABEL_27:
	if (!app->game->scriptStateVars[Enums::CODEVAR_DRAW_SKYMAP]) {
		app->sound->stopSound(1153, true);
		loadMapID = this->loadMapID;
		if ((unsigned int)(loadMapID - 2) > 1) {
			goto LABEL_10;
		}
		goto LABEL_29;
	}
	if (!app->sound->isSoundPlaying(1153)) {
		app->sound->playSound(1153, 1u, 2, 1);
	}
	loadMapID = this->loadMapID;

LABEL_9:
	if ((unsigned int)(loadMapID - 2) > 1) {
		goto LABEL_10;
	}

LABEL_29:
	if (app->game->scriptStateVars[Enums::CODEVAR_DRAW_SKYMAP]) {
		app->sound->stopSound(1152, true);
		loadMapID = this->loadMapID;
		if (loadMapID != 4) {
			goto LABEL_11;
		}
		goto LABEL_31;
	}

	if (!app->sound->isSoundPlaying(1152)) {
		app->sound->playSound(1152, 1u, 2, 1);
	}
	loadMapID = this->loadMapID;

LABEL_10:
	if (loadMapID != 4) {
		goto LABEL_11;
	}
LABEL_31:
	if (!app->game->scriptStateVars[Enums::CODEVAR_DRAW_SKYMAP]) {
		app->sound->stopSound(1028, true);
		loadMapID = this->loadMapID;
		if (loadMapID != 5) {
			goto LABEL_12;
		}
		goto LABEL_33;
	}
	if (!app->sound->isSoundPlaying(1028)) {
		app->sound->playSound(1028, 1u, 2, 1);
	}
	loadMapID = this->loadMapID;
LABEL_11:
	if (loadMapID != 5) {
		goto LABEL_12;
	}

LABEL_33:
	if (this->state != 4) {
		app->sound->stopSound(1111, true);
		loadMapID = this->loadMapID;
		if (loadMapID != 6) {
			goto LABEL_13;
		}
		goto LABEL_35;
	}

	if (!app->sound->isSoundPlaying(1111)) {
		app->sound->playSound(1111, 1u, 4, 1);
	}
	loadMapID = this->loadMapID;

LABEL_12:
	if (loadMapID != 6) {
		goto LABEL_13;
	}

LABEL_35:
	if (!app->game->scriptStateVars[Enums::CODEVAR_DRAW_SKYMAP]) {
		app->sound->stopSound(1081, true);
		if (this->loadMapID != 8) {
			goto LABEL_14;
		}
		goto LABEL_37;
	}
	if (!app->sound->isSoundPlaying(1081)) {
		app->sound->playSound(1081, 1u, 2, 1);
	}
	loadMapID = this->loadMapID;

LABEL_13:
	if (loadMapID != 8) {
		goto LABEL_14;
	}

LABEL_37:
	if (app->game->scriptStateVars[Enums::CODEVAR_DRAW_SKYMAP]) {
		app->sound->stopSound(1151, true);
		if (!app->sound->isSoundPlaying(1081)) {
			app->sound->playSound(1081, 1u, 2, 1);
		}
	}
	else {
		app->sound->stopSound(1081, true);
		if (!app->sound->isSoundPlaying(1151)) {
			app->sound->playSound(1151, 1u, 2, 1);
		}
	}

LABEL_14:
	ambientSoundsTime = this->ambientSoundsTime;
	if (!ambientSoundsTime){
		gameTime = app->gameTime;
		ambientSoundsTime = gameTime + (((int16_t)app->nextByte() % 5 + 5) << 10);
		this->ambientSoundsTime = ambientSoundsTime;
	}

	v6 = app->gameTime;
	if (ambientSoundsTime < v6)
	{
		switch (this->loadMapID)
		{
		case 4:
			if (!app->game->scriptStateVars[Enums::CODEVAR_DRAW_SKYMAP]) {
				goto LABEL_20;
			}
			app->nextByte();
			v12 = 0;
			v7 = 1059;
			v6 = app->gameTime;
			v8 = 6;
			v9 = 15;
			break;
		case 5:
		case 6:
			if (!app->game->scriptStateVars[Enums::CODEVAR_DRAW_SKYMAP]) {
				goto LABEL_20;
			}
			v7 = 1060;
			v8 = 20;
			v9 = 40;
			v12 = 0;
			break;
		case 7:
			Byte = app->nextByte();
			v12 = 1;
			if ((int16_t)((((uint8_t)Byte + (Byte < 0)) & 1) - ((uint32_t)Byte >> 31)) == 1) {
				v7 = 1091;
				v6 = app->gameTime;
				v8 = 14;
			}
			else {
				v7 = 1018;
				v6 = app->gameTime;
				v8 = 10;
			}
			v9 = 15;
			break;
		case 9:
			if (!this->field_0xacc_ || this->field_0xacd_) {
				goto LABEL_20;
			}
			switch ((int16_t)app->nextByte() % 5)
			{
			case 1:
				v12 = 0;
				v7 = 1119;
				v6 = app->gameTime;
				v8 = 16;
				v9 = 4;
				break;
			case 2:
				v12 = 0;
				v7 = 1035;
				v6 = app->gameTime;
				v8 = 6;
				v9 = 4;
				break;
			case 3:
				v8 = 4;
				v12 = 0;
				v7 = 1036;
				v6 = app->gameTime;
				v9 = 4;
				break;
			case 4:
				v8 = 4;
				v12 = 0;
				v7 = 1037;
				v6 = app->gameTime;
				v9 = 4;
				break;
			default:
				v12 = 1;
				v7 = 1153;
				v6 = app->gameTime;
				v8 = 14;
				v9 = 4;
				break;
			}
			break;
		default:
		LABEL_20:
			v7 = -1;
			v8 = 8;
			v9 = 15;
			v12 = 0;
			break;
		}
		this->ambientSoundsTime = v6 + 1000 * (v8 + (int16_t)app->nextByte() % v9);
		if (v7 != -1 && !app->sound->isSoundPlaying(v7)) {
			app->sound->playSound(v7, 0, 2, v12);
		}
	}
}
