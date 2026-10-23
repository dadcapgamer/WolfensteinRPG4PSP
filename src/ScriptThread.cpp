#include <stdexcept>
#include <cstring>

#include "CAppContainer.h"
#include "ScriptThread.h"
#include "JavaStream.h"
#include "Game.h"
#include "Render.h"
#include "Canvas.h"
#include "Enums.h"
#include "MayaCamera.h"
#include "Text.h"
#include "Player.h"
#include "Hud.h"
#include "Entity.h"
#include "EntityDef.h"
#include "Combat.h"
#include "ParticleSystem.h"
#include "MenuSystem.h"
#include "Sound.h"
#include "Menus.h"
#include "CardGames.h"

ScriptThread::ScriptThread() {
}

ScriptThread::~ScriptThread() {
}

void ScriptThread::saveState(OutputStream* OS) {
    if (this->unpauseTime == -1 || this->unpauseTime == 0) {
        OS->writeInt(this->unpauseTime);
    }
    else {
        OS->writeInt(this->unpauseTime - this->app->gameTime);
    }
    OS->writeByte((uint8_t)this->state);
    OS->writeInt(this->IP);
    OS->writeInt(this->FP);
    for (int i = 0; i < this->FP; i++) {
        OS->writeInt(this->scriptStack[i]);
    }
}

void ScriptThread::loadState(InputStream* IS) {
    this->init();
    this->unpauseTime = IS->readInt();
    if (this->unpauseTime != 0 && this->unpauseTime != -1) {
        this->unpauseTime += this->app->gameTime;
    }
    this->state = IS->readByte();
    this->IP = IS->readInt();
    this->FP = IS->readInt();
    for (int i = 0; i < this->FP; i++) {
        this->scriptStack[i] = IS->readInt();
    }
}

uint32_t ScriptThread::executeTile(int x, int y, int flags, bool b) {
    if (x < 0 || x >= 32 || y < 0 || y >= 32) {
        this->state = 0;
        return 0;
    }
    this->app->game->skipAdvanceTurn = false;
    int n4 = (y * 32) + x;
    if (this->app->render->mapFlags[n4] & Canvas::BIT_AM_EVENTS) {
        int run = 0;
        for (int i = this->app->render->findEventIndex(n4); i != -1; i = this->app->render->getNextEventIndex()) {
            //printf("ExecuteTile: (%d, %d), tileEvent[%d] = %x;\n", x, y, i + 1, this->app->render->tileEvents[i + 1]);
            int n5 = this->app->render->tileEvents[i + 1];
            int n6 = n5 & flags;

            if ((n5 & 0x80000) == 0 && (n6 & 0xF) != 0 && (n6 & 0xFF0) != 0 && ((n5 & 0x7000) == 0 && (flags & 0x7000) == 0 || (n6 & 0x7000) != 0)) {
                if ((n5 & 0x40000) != 0x0) {
                    this->app->game->skipAdvanceTurn = true;
                    this->app->game->queueAdvanceTurn = false;
                }
                //printf("ExecuteTile Alloc: (%d, %d), tileEvent[%d] = %x;\n", x, y, i, this->app->render->tileEvents[i]);
                this->alloc(i, flags, b);
                run = this->run();
            }
        }
        return run;
    }
    this->state = 0;
    return 0;
}

int ScriptThread::queueTile(int x, int y, int flags) {
    return this->queueTile(x, y, flags, false);
}

int ScriptThread::queueTile(int x, int y, int flags, bool b) {

    if (x < 0 || x >= 32 || y < 0 || y >= 32) {
        return this->state = 0;
    }
    this->app->game->skipAdvanceTurn = false;
    int n4 = y * 32 + x;
    if (this->app->render->mapFlags[n4] & Canvas::BIT_AM_EVENTS) {
        int eventIndex = this->app->render->findEventIndex(n4);
        if (eventIndex != -1) {
            this->alloc(eventIndex, flags, b);
            this->flags |= 0x2;
            return 2;
        }
    }
    this->state = 0;
    return 0;
}

int ScriptThread::evWait(int time) {
    if (this->app->game->skippingCinematic) {
        return 1;
    }
    this->unpauseTime = this->app->gameTime + time;
    if ((this->flags & 0x1) != 0x0) {
        if (this->app->canvas->state != Canvas::ST_CAMERA) {
            this->app->canvas->blockInputTime = this->unpauseTime;
        }
        if (this->app->canvas->state == Canvas::ST_AUTOMAP || this->app->canvas->state == Canvas::ST_MENU) {
            this->app->canvas->setState(Canvas::ST_PLAYING);
        }
        if (this->app->canvas->state == Canvas::ST_PLAYING) {
            this->app->canvas->clearSoftKeys();
        }
    }
    return 2;
}

bool ScriptThread::evReturn() {
    while (this->FP < this->stackPtr - 2) {
        this->pop();
    }
    this->FP = this->pop();
    int IP = this->pop();
    if (IP != -1) {
        this->IP = IP;
        return false;
    }
    else if (this->stackPtr != 0) {
        this->app->Error("The frame pointer should be zero if the script has completed. %i", Enums::ERR_SCRIPTTHREAD_FREE);
    }
    return true;
}

void ScriptThread::alloc(int n, int type, bool b) {
    this->IP = (this->app->render->tileEvents[n] & (int)0xFFFF0000) >> 16;
    //printf("alloc : this->IP %d\n", this->IP);
    this->FP = 0;
    this->stackPtr = 0;
    this->push(-1);
    this->push(0);
    this->type = type;
    this->flags = 0;
    if (b) {
        this->flags = 1;
    }
}

void ScriptThread::alloc(int ip) {
    this->IP = ip;
    this->FP = 0;
    this->stackPtr = 0;
    this->push(-1);
    this->push(0);
    this->type = 0;
    this->flags = 1;
}

int ScriptThread::peekNextCmd() {
    return this->app->render->mapByteCode[this->IP + 1];
}

void ScriptThread::setupCamera(int n) {
    Game* game = this->app->game;
    //printf("setupCamera %d\n", n);
    game->cinUnpauseTime = this->app->gameTime + 1000;
    game->activeCameraView = true;
    MayaCamera* activeCamera = &game->mayaCameras[n];
    game->activeCameraKey = -1;
    game->activeCamera = activeCamera;
    activeCamera->complete = false;
    game->activeCameraTime = this->app->gameTime;
    game->camPlayerX = this->app->canvas->destX << 3;
    game->camPlayerY = this->app->canvas->destY << 3;
    game->camPlayerZ = this->app->canvas->destZ << 3;
    game->camPlayerYaw = (this->app->canvas->destAngle & 0x3FF);
    game->camPlayerPitch = this->app->canvas->viewPitch;

    activeCamera->x = game->mayaCameraKeys[game->OFS_MAYAKEY_X + activeCamera->keyOffset];
    if (activeCamera->x == -2) {
        activeCamera->x = game->camPlayerX;
    }

    activeCamera->y = game->mayaCameraKeys[game->OFS_MAYAKEY_Y + activeCamera->keyOffset];
    if (activeCamera->y == -2) {
        activeCamera->y = game->camPlayerY;
    }

    activeCamera->z = game->mayaCameraKeys[game->OFS_MAYAKEY_Z + activeCamera->keyOffset];
    if (activeCamera->z == -2) {
        activeCamera->z = game->camPlayerZ;
    }

    activeCamera->x <<= 1;
    activeCamera->y <<= 1;
    activeCamera->z <<= 1;

    activeCamera->pitch = game->mayaCameraKeys[game->OFS_MAYAKEY_PITCH + activeCamera->keyOffset];
    if (activeCamera->pitch == -2) {
        activeCamera->pitch = game->camPlayerPitch;
    }

    activeCamera->yaw = game->mayaCameraKeys[game->OFS_MAYAKEY_YAW + activeCamera->keyOffset];
    if (activeCamera->yaw == -2) {
        activeCamera->yaw = game->camPlayerYaw;
    }

    activeCamera->roll = game->mayaCameraKeys[game->OFS_MAYAKEY_ROLL + activeCamera->keyOffset];
}

uint32_t ScriptThread::run() {
    this->app->game->updateScriptVars();
    if (this->stackPtr == 0) {
        return 1;
    }

    //return 1;// TEMP


    int n = 1;
    uint8_t* mapByteCode = this->app->render->mapByteCode;
    //printf("mapByteCodeSize -> %d\n", this->app->render->mapByteCodeSize);
    while (this->IP < this->app->render->mapByteCodeSize && n != 2) {
        bool b = true;
        short n2 = 0;

        //printf("mapByteCode[%d] [%d]\n", this->IP, mapByteCode[this->IP]);
        // Event opcodes are signed bytes; 0xFF is the -1 end marker.
        switch ((int8_t)mapByteCode[this->IP]) {
            case Enums::EV_EVAL: {
                //printf("EV_EVAL -> %d\n", this->IP);
                short n3 = (short)this->getByteArg();
                b = false;
                while (true) {
                    if (--n3 < 0) {
                        break;
                    }
                    short uByteArg = this->getByteArg();
                    if ((uByteArg & Enums::EVAL_CONSTFLAG) != 0x0) {
                        int args = this->getByteArg();
                        int args2 = ((((uByteArg & Enums::EVAL_CONSTMASK) << 8) | args) << 18) >> 18;
                        this->push(args2);
                    }
                    else if ((uByteArg & Enums::EVAL_VARFLAG) != 0x0) {
                        this->push(this->app->game->scriptStateVars[uByteArg & Enums::EVAL_VARMASK]);
                    }
                    else {
                        switch (uByteArg) {
                            case Enums::EVAL_AND: { // and
                                this->push(this->pop() == 1 && this->pop() == 1);
                                break;
                            }
                            case Enums::EVAL_OR: { // or
                                this->push(this->pop() == 1 || this->pop() == 1);
                                break;
                            }
                            case Enums::EVAL_LTE: { // less than or equal to
                                int a = this->pop();
                                int b = this->pop();
                                this->push((b <= a));
                                break;
                            }
                            case Enums::EVAL_LT: { // less than
                                int a = this->pop();
                                int b = this->pop();
                                this->push((b < a));
                                break;
                            }
                            case Enums::EVAL_EQ: { // equal to
                                int a = this->pop();
                                int b = this->pop();
                                this->push((a == b));
                                break;
                            }
                            case Enums::EVAL_NEQ: { // not equal to	
                                int a = this->pop();
                                int b = this->pop();
                                this->push((a != b));
                                break;
                            }
                            case Enums::EVAL_NOT: { // negation
                                this->push((this->pop() == 0) ? 1 : 0);
                                break;
                            }
                        }
                    }
                }
                short uByteArg2 = this->getUByteArg();
                if (this->pop() == 0) {
                    this->IP += (int)uByteArg2;
                    break;
                }
                break;
            }

            case Enums::EV_JUMP: {
                //printf("EV_JUMP -> %d\n", this->IP);
                this->IP += this->getUShortArg();
                break;
            }

            case Enums::EV_RETURN: {
                //printf("EV_RETURN -> %d\n", this->IP);
                if (this->evReturn()) {
                    return 1;
                }
                break;
            }

            case Enums::EV_MESSAGE: {
                //printf("EV_MESSAGE -> %d\n", this->IP);
                int uShortArg = this->getUShortArg();
                short n4 = (short)(uShortArg & 0x7FFF);
                if ((uShortArg & 0x8000) >> 15 == 1) {
                    this->app->hud->addMessage(this->app->canvas->loadMapStringID, n4, 3);
                    break;
                }
                if (this->app->canvas->state == Canvas::ST_CAMERA) {
                    this->app->hud->msgCount = 0;
                }
                this->app->hud->addMessage(this->app->canvas->loadMapStringID, n4);
                break;
            }

            case Enums::EV_LERPSPRITE: {
                //printf("EV_LERPSPRITE -> %d\n", this->IP);
                
                int args = this->getIntArg();
                int sprite = (args >> 22);
                int dstX = (args >> 17) & 0x1F;
                int dstY = (args >> 12) & 0x1F;
                int flags = args & 0xF;
                int dstZ = (uint8_t)(args >> 4) - 48; // ((args << 0x14) >> 0x18) - 48;//
                int time = this->getUShortArg();

                bool aSync = (flags & Enums::SCRIPT_LS_FLAG_ASYNC) != 0x0;
                LerpSprite* lerpSprite = this->app->game->allocLerpSprite(this, sprite, (flags & Enums::SCRIPT_LS_FLAG_BLOCK) != 0x0);
                if (lerpSprite == nullptr) {
                    return 0;
                }
                lerpSprite->dstX = 32 + (dstX << 6);
                lerpSprite->dstY = 32 + (dstY << 6);
                short sEnt = this->app->render->mapSprites[this->app->render->S_ENT + sprite];
                if (sEnt != -1) {
                    this->app->game->entities[sEnt].info |= Entity::ENTITY_FLAG_DIRTY;
                }
                lerpSprite->dstZ = this->app->render->getHeight(lerpSprite->dstX, lerpSprite->dstY) + dstZ;
                lerpSprite->srcX = this->app->render->mapSprites[this->app->render->S_X + sprite];
                lerpSprite->srcY = this->app->render->mapSprites[this->app->render->S_Y + sprite];
                lerpSprite->srcZ = this->app->render->mapSprites[this->app->render->S_Z + sprite];
                lerpSprite->srcScale = lerpSprite->dstScale = this->app->render->mapSprites[this->app->render->S_SCALEFACTOR + sprite];
                lerpSprite->startTime = app->gameTime;
                lerpSprite->travelTime = time;
                lerpSprite->flags = flags; //(flags & Enums::SCRIPT_LS_FLAG_ASYNC_BLOCK);
                lerpSprite->calcDist();

                if (time == 0) {
                    if ((this->app->game->updateLerpSprite(lerpSprite) & 0x1) != 0x0) {
                        this->app->canvas->invalidateRect();
                        break;
                    }
                    n = 0;
                    break;
                }
                else {
                    if (!aSync) {
                        this->app->game->skipAdvanceTurn = true;
                        this->app->game->queueAdvanceTurn = false;
                        this->unpauseTime = -1;
                        n = 2;
                        break;
                    }
                    break;
                }
                break;
            }

            case Enums::EV_STARTCINEMATIC: {
                //printf("EV_STARTCINEMATIC -> %d\n", this->IP);
                uint8_t byteArg = this->getByteArg();
                this->setupCamera(byteArg);
                this->app->game->activeCamera->cameraThread = this;
                if (this->app->canvas->state != Canvas::ST_MENU && this->app->canvas->state != Canvas::ST_CAMERA) {
                    this->app->canvas->setState(Canvas::ST_CAMERA);
                }
                this->app->game->skipAdvanceTurn = true;
                this->app->game->queueAdvanceTurn = false;
                break;
            }

            case Enums::EV_SETSTATE: {
                //printf("EV_SETSTATE -> %d\n", this->IP);
                int index = this->getByteArg();
                short args = this->getShortArg();
                this->app->game->scriptStateVars[index] = args;
                if (index == Enums::CODEVAR_OSCILLATE_FOV && args == 1) {
                    this->app->player->removeStatusEffect(Enums::STATUS_EFFECT_FIRE);
                }
                break;
            }

            case Enums::EV_CALL_FUNC: {
                //printf("EV_CALL_FUNC -> %d\n", this->IP);
                int ip = this->getUShortArg();
                int fp = this->FP;
                this->FP = this->stackPtr;
                this->push(this->IP);
                this->push(fp);
                this->IP = ip - 1;
                break;
            }

            case Enums::EV_ITEM_COUNT: {
                //printf("EV_ITEM_COUNT -> %d\n", this->IP);
                int args = this->getUShortArg();
                int n21 = args & 0x1F;
                int n22 = (args & 0x3E0) >> 5;
                short n23 = 0;
                short index = (args & 0xFC00) >> 10;
                if (n21 == 0) {
                    n23 = this->app->player->inventory[n22];
                }
                else if (n21 == 1) {
                    if ((this->app->player->weapons & 1LL << n22) != 0x0) {
                        n23 = 1;
                    }
                }
                else if (n21 == 2) {
                    n23 = this->app->player->ammo[n22];
                }
                this->app->game->scriptStateVars[index] = n23;
                //printf("EventItemCount(%d, %d, %d) = %d;\n", n21, n22, index, n23);
                break;
            }

            case Enums::EV_TILE_EMPTY: {
                //printf("EV_TILE_EMPTY -> %d\n", this->IP);
                int args = this->getUShortArg();
                int srcX = args & 0x1F;
                int srcY = (args >> 5) & 0x1F;
                int index = (args >> 10) & 0x3F;
                short n26 = 1;
                Entity* entity = app->game->entityDb[srcX + (32 * srcY)];
                if (entity != nullptr) {
                    while (entity != nullptr) {
                        if (!(1 << entity->def->eType & Enums::CONTENTS_ISEMPTY_SCRIPT)) {
                            n26 = 0;
                            break;
                        }
                        entity = entity->nextOnTile;
                    }
                }
                this->app->game->scriptStateVars[index] = n26;
                break;
            }

            case Enums::EV_WEAPON_EQUIPPED: {
                //printf("EV_WEAPON_EQUIPPED -> %d\n", this->IP);
                int index = this->getByteArg() & 0x3F;
                this->app->game->scriptStateVars[index] = (short)this->app->player->ce->weapon;
                break;
            }

            case Enums::EV_CHANGE_MAP: {
                //printf("EV_CHANGE_MAP -> %d\n", this->IP);
                int uByteArg4 = this->getUByteArg();
                int uShortArg5 = this->getUShortArg();
                bool showStats = true;
                this->app->player->completedLevels |= 1 << (short)(app->canvas->loadMapID - 1);
                this->app->game->spawnParam = ((((uByteArg4 >> 4) & 0x7) << 10) | (uShortArg5 & 0x3FF));
                this->app->menuSystem->LEVEL_STATS_nextMap = (short)(uByteArg4 & 0xF);
                if (this->app->menuSystem->LEVEL_STATS_nextMap < this->app->canvas->loadMapID) {
                    showStats = false;
                }
                this->app->player->removeStatusEffect(Enums::STATUS_EFFECT_AIR);
                this->app->game->snapAllMovers();
                if ((uByteArg4 & 0x80) != 0x0) {
                    int fadeFlags = Render::FADE_FLAG_FADEOUT;
                    if (showStats) {
                        fadeFlags |= Render::FADE_FLAG_SHOWSTATS;
                    }
                    else {
                        fadeFlags |= Render::FADE_FLAG_CHANGEMAP;
                    }
                    if (this->app->canvas->state == Canvas::ST_AUTOMAP) {
                        this->app->canvas->setState(Canvas::ST_PLAYING);
                    }
                    this->app->render->startFade(1000, fadeFlags);
                }
                else if (showStats) {
                    this->app->canvas->saveState(51, Strings::FILE_MENUSTRINGS, MenuStrings::SAVING_GAME_LABEL);
                }
                else {
                    this->app->canvas->loadMap(this->app->menuSystem->LEVEL_STATS_nextMap, false, false);
                }
                this->app->canvas->changeMapStarted = true;
                break;
            }

            case Enums::EV_CAMERA_STR: {
                //printf("EV_CAMERA_STR -> %d\n", this->IP);
                int uShortArg6 = this->getUShortArg();
                int time = this->getUShortArg();
                if (!this->app->game->skippingCinematic) {
                    short n29 = (short)(uShortArg6 & 0x3FFF);
                    int n30 = (uShortArg6 & 0x8000) >> 15;
                    this->app->hud->showCinPlayer = ((uShortArg6 & 0x4000) != 0x0);
                    if (n30 == 1) {
                        this->app->hud->showCinPlayer = false;
                    }
                    if (n30 == 0) {
                        this->app->hud->subTitleID = Localization::STRINGID(this->app->canvas->loadMapStringID, n29);
                        this->app->hud->subTitleTime = app->gameTime + time;
                    }
                    else {
                        this->app->hud->cinTitleID = Localization::STRINGID(this->app->canvas->loadMapStringID, n29);
                        this->app->hud->cinTitleTime = app->gameTime + time;
                    }
                    this->app->canvas->repaintFlags |= Canvas::REPAINT_HUD;
                    this->app->hud->repaintFlags = 16;
                    break;
                }
                break;
            }

            case Enums::EV_DIALOG: {
                //printf("EV_DIALOG -> %d\n", this->IP);
                short uByteArg5 = this->getUShortArg();
                short uByteArg6 = this->getUByteArg();
                int n31 = uByteArg6 >> 4;
                int n32 = uByteArg6 & 0xF;
                if (this->app->canvas->state == Canvas::ST_AUTOMAP) {
                    this->app->canvas->setState(Canvas::ST_PLAYING);
                    this->app->canvas->invalidateRect();
                }
                if (this->app->game->skipDialog) {
                    break;
                }
                if (n32 == 6 || n32 == 7 || n32 == 1) {
                    this->app->player->inCombat = false;
                }
                if (n32 == 2) {
                    bool enqueueHelpDialog = false;
                    this->app->player->prevWeapon = this->app->player->ce->weapon;
                    for (uint8_t b4 = 0; b4 < 20; ++b4) {
                        if (&this->app->game->scriptThreads[b4] == this) {
                            enqueueHelpDialog = this->app->canvas->enqueueHelpDialog(this->app->canvas->loadMapStringID, uByteArg5, b4);
                            break;
                        }
                    }
                    if (!enqueueHelpDialog) {
                        break;
                    }
                }
                else {
                    if (n32 == 4) {
                        this->app->player->prevWeapon = this->app->player->ce->weapon;
                    }
                    this->app->canvas->startDialog(this, this->app->canvas->loadMapStringID, uByteArg5, n32, n31, true);
                }
                this->app->game->skipAdvanceTurn = true;
                this->app->game->queueAdvanceTurn = false;
                this->unpauseTime = -1;
                n = 2;
                break;
            }

            case Enums::EV_WAIT: {
                //printf("EV_WAIT -> %d\n", this->IP);
                n = this->evWait(this->getShortArg());
                break;
            }

            case Enums::EV_GOTO: {
                //printf("EV_GOTO -> %d\n", this->IP);
                if (this->app->game->interpolatingMonsters) {
                    this->app->game->snapMonsters(true);
                }
                while (this->app->game->combatMonsters != nullptr) {
                    this->app->game->combatMonsters->undoAttack();
                }
                this->app->game->endMonstersTurn();
                int args = this->getUShortArg();
                bool b5 = (args & 0x4000) != 0x0;
                this->app->canvas->destX = (((args >> 5) & 0x1F) << 6) + 32;
                this->app->canvas->destY = ((args & 0x1F) << 6) + 32;
                this->app->canvas->destZ = this->app->render->getHeight(this->app->canvas->destX, this->app->canvas->destY) + 36;
                int n33 = (args >> 10) & 0xF;
                this->app->canvas->viewPitch = (this->app->canvas->destPitch = 0);
                this->app->canvas->viewRoll = (this->app->canvas->destRoll = 0);
                this->app->canvas->knockbackDist = 0;
                if (b5) {
                    if (n33 != 15) {
                        int viewAngle = this->app->canvas->viewAngle & 0x3FF;
                        int destAngle = n33 << 7;
                        if (destAngle - viewAngle > 512) {
                            destAngle -= 1024;
                        }
                        else if (destAngle - viewAngle < -512) {
                            destAngle += 1024;
                        }
                        this->app->canvas->viewAngle = viewAngle;
                        this->app->canvas->destAngle = destAngle;
                    }
                    this->app->canvas->startRotation(false);
                    this->app->canvas->zStep = (std::abs(this->app->canvas->destZ - this->app->canvas->viewZ) + this->app->canvas->animFrames - 1) / this->app->canvas->animFrames;
                    if (this->app->canvas->destX != this->app->canvas->viewX || this->app->canvas->destY != this->app->canvas->viewY || this->app->canvas->viewAngle != this->app->canvas->destAngle) {
                        this->app->canvas->gotoThread = this;
                        this->unpauseTime = -1;
                        n = 2;
                    }
                    else {
                        this->app->canvas->viewPitch = this->app->canvas->destPitch;
                    }
                }
                else {
                    this->app->canvas->viewX = this->app->canvas->destX;
                    this->app->canvas->viewY = this->app->canvas->destY;
                    this->app->canvas->viewZ = this->app->canvas->destZ;
                    if (n33 != 15) {
                        this->app->canvas->destAngle = (this->app->canvas->viewAngle = n33 << 7);
                        this->app->canvas->finishRotation(true);
                    }
                    if ((args & 0x8000) != 0x0) {
                        this->app->game->advanceTurn();
                    }
                    if (this->app->canvas->state != Canvas::ST_CAMERA) {
                        this->app->canvas->startRotation(false);
                        this->app->canvas->viewPitch = this->app->canvas->destPitch;
                    }
                    else {
                        this->app->canvas->viewPitch = (this->app->canvas->destPitch = 0);
                    }
                    this->app->game->gotoTriggered = true;
                    this->app->canvas->automapDrawn = false;
                }
                this->app->player->relink();
                this->app->canvas->clearEvents(1);
                this->app->canvas->updateFacingEntity = true;
                this->app->canvas->invalidateRect();
                break;
            }

            case Enums::EV_ABORT_MOVE: {
                //printf("EV_ABORT_MOVE -> %d\n", this->IP);
                this->app->canvas->abortMove = true;
                break;
            }

            case Enums::EV_ENTITY_FRAME: {
                //printf("EV_ENTITY_FRAME -> %d\n", this->IP);
                short sprite = this->getUShortArg();
                short frame = this->getUByteArg();
                int time = this->getShortArg();

                short sEnt = this->app->render->mapSprites[this->app->render->S_ENT + sprite];
                int tileNum = this->app->render->mapSpriteInfo[sprite] & Enums::SPRITE_MASK_SPRITENUMBER;
                int frameNum = (this->app->render->mapSpriteInfo[sprite] & Enums::SPRITE_MASK_FRAMENUMBER) >> Enums::SPRITE_SHIFT_FRAMENUMBER;
                this->app->render->mapSpriteInfo[sprite] = ((this->app->render->mapSpriteInfo[sprite] & ~Enums::SPRITE_MASK_FRAMENUMBER) | (frame << Enums::SPRITE_SHIFT_FRAMENUMBER));
                this->app->canvas->staleView = true;
                if (sEnt != -1) {
                    this->app->game->entities[sEnt].info |= Entity::ENTITY_FLAG_DIRTY;
                    if (this->app->game->entities[sEnt].monster != nullptr) {
                        this->app->game->entities[sEnt].monster->frameTime = 0x7FFFFFFF;
                    }
                }

                if (tileNum == Enums::TILENUM_SWITCH) {
                    this->app->render->mapSpriteInfo[sprite] &= ~Enums::SPRITE_FLAG_FLIP_VERTICAL;
                    this->app->render->mapSpriteInfo[sprite] |= frame << Enums::SPRITE_SHIFT_FLIP_VERTICAL;

                    this->app->render->mapSprites[this->app->render->S_Z + sprite] = this->app->render->getHeight(
                        this->app->render->mapSprites[this->app->render->S_X + sprite],
                        this->app->render->mapSprites[this->app->render->S_Y + sprite]) + 32; // verificar

                    if (frame == 1) { // iOS
                        this->app->render->mapSprites[this->app->render->S_Z + sprite] -= 14; // 11 -> Old
                    }

                    /*if (frameNum == 0 && frame == 1) { // J2ME
                        this->app->render->mapSprites[this->app->render->S_Z + sprite] -= 11;
                    }
                    else if (frameNum == 1 && frame == 0) {
                        this->app->render->mapSprites[this->app->render->S_Z + sprite] += 11;
                    }*/
                }

                if (time > 0) {
                    n = this->evWait(time);
                    break;
                }
                break;
            }

            case Enums::EV_ADV_CAMERAKEY: {
                //printf("EV_ADV_CAMERAKEY -> %d\n", this->IP);
                if (this->app->canvas->state == Canvas::ST_CAMERA || this->app->canvas->state == Canvas::ST_DRIVING) {
                    this->app->game->activeCamera->keyThreadResumeCount = this->getUByteArg();
                    this->app->game->activeCamera->keyThread = this;
                    this->app->game->activeCamera->NextKey();
                    this->unpauseTime = -1;
                    n = 2;
                    break;
                }
                this->getUByteArg();
                break;
            }

            case Enums::EV_DAMAGEMONSTER: {
                //printf("EV_DAMAGEMONSTER -> %d\n", this->IP);
                short sprite = this->getUShortArg();
                int8_t dmgVal = this->getByteArg();
                short sEnt = this->app->render->mapSprites[this->app->render->S_ENT + sprite];
                if (sEnt != -1) {
                    Entity* entity = &this->app->game->entities[sEnt];
                    if (entity->monster != nullptr) {
                        entity->info |= Entity::ENTITY_FLAG_TAKEDAMAGE;
                        entity->pain(dmgVal, nullptr);
                        if (entity->monster->ce.getStat(Enums::STAT_HEALTH) <= 0) {
                            entity->died(false, nullptr);
                        }
                    }
                    else {
                        entity->died(false, nullptr);
                    }
                }
                this->app->canvas->staleView = true;
                break;
            }

            case Enums::EV_DAMAGEPLAYER: {
                //printf("EV_DAMAGEPLAYER -> %d\n", this->IP);
                int8_t dmgVal = this->getByteArg();
                int8_t dmgArm = this->getByteArg();
                int8_t dmgAng = this->getByteArg();
                if (dmgVal > 0) {
                    this->app->player->painEvent(nullptr, false);
                    this->app->hud->damageTime = this->app->time + 1000;
                    if (dmgAng != -1) {
                        this->app->hud->damageDir = (((uint8_t)dmgAng) + (256 - (this->app->canvas->viewAngle & 0x3FF) >> 7) + 1 & 0x7);
                    }
                    this->app->combat->totalDamage = 1;
                    this->app->player->pain(dmgVal, nullptr, true);
                }
                else if (dmgVal < 0) {
                    this->app->player->addHealth(-dmgVal);
                }
                this->app->player->addArmor(-dmgArm);
                break;
            }

            case Enums::EV_DOOROP: {
                //printf("EV_DOOROP -> %d\n", this->IP);
                int args = this->getUShortArg();
                int n42 = args >> 10;
                int n43 = ((n42 & 0x4) == 0x0 && this->app->canvas->state != Canvas::ST_AUTOMAP) ? 1 : 0;
                int n44 = n42 & 0x3;
                short sEnt = this->app->render->mapSprites[this->app->render->S_ENT + (args & 0x3FF)];
                if (sEnt != -1) {
                    Entity* entity = &this->app->game->entities[sEnt];
                    if (n44 == 1 || n44 == 0) {
                        if (n44 == 1 && entity->def->eType == Enums::ET_DOOR) {
                            this->app->game->setLineLocked(entity, false);
                        }
                        if (this->app->game->performDoorEvent(n44, (n43 != 0) ? this : nullptr, entity, n43, false) && n43) {
                            this->unpauseTime = -1;
                            n = 2;
                        }
                    }
                    else if (n44 == 2) {
                        if (n43 != 0 && !this->app->game->skippingCinematic) {
                            this->app->sound->playSound(1013, 0, 3, 0);
                        }
                        this->app->game->setLineLocked(entity, true);
                    }
                    else {
                        if (n43 != 0 && !this->app->game->skippingCinematic) {
                            this->app->sound->playSound(1015, 0, 3, 0);
                        }
                        this->app->game->setLineLocked(entity, false);
                    }
                    break;
                }
                break;
            }

            case Enums::EV_MONSTERFLAGOP: {
                //printf("EV_MONSTERFLAGOP -> %d\n", this->IP);
                short sprite = this->getUShortArg();
                uint32_t args = this->getUShortArg();
                int n46 = ((args >> 8) & 0xFF) >> 6 & 0x3;
                int n47 = (args & 0x3FFF);
                short n48 = this->app->render->mapSprites[this->app->render->S_ENT + sprite];
                if (n48 != -1) {
                    Entity* entity12 = &this->app->game->entities[n48];
                    if (entity12->monster != nullptr) {
                        if (n46 == 0) {
                            entity12->monster->flags |= (short)n47;
                            if (n47 == Enums::MFLAG_ABILITY && entity12->def->eSubType == Enums::BOSS_HARBINGER) {
                                app->render->mapSpriteInfo[entity12->getSprite()] |= Enums::SPRITE_FLAG_FLIP_HORIZONTAL;
                            }

                            if (entity12->def->eSubType == Enums::BOSS_HARBINGER && n47 == Enums::MFLAG_NOTHINK) {
                                this->app->canvas->field_0xacc_ = false;
                            }
                        }
                        else if (n46 == 1) {
                            entity12->monster->flags &= (short)~n47;

                            if (entity12->def->eSubType == Enums::BOSS_HARBINGER && (n47 == Enums::MFLAG_NOACTIVATE || n47 == Enums::MFLAG_NOTHINK)) {
                                this->app->canvas->field_0xacc_ = true;
                            }
                        }
                        else {
                            entity12->monster->flags = (short)n47;
                        }
                        if (n47 == Enums::MFLAG_NPC_MONSTER && (n46 == 0 || n46 == 2)) {
                            entity12->info &= ~Entity::ENTITY_FLAG_TAKEDAMAGE;
                        }
                        else if (n47 == Enums::MFLAG_NPC_MONSTER) {
                            entity12->info |= Entity::ENTITY_FLAG_TAKEDAMAGE;
                        }
                        entity12->info |= Entity::ENTITY_FLAG_DIRTY;
                    }
                }
                break;
            }

            case Enums::EV_EVENTOP: {
                //printf("EV_EVENTOP -> %d\n", this->IP);
                int uShortArg10 = this->getUShortArg();
                int n49 = (uShortArg10 & 0x8000) << 4;//((uShortArg10 & 0x8000) >> 15) << 19;
                int n50 = uShortArg10 & 0x7FFF;
                this->app->render->tileEvents[n50 * 2 + 1] = ((this->app->render->tileEvents[n50 * 2 + 1] & 0xFFF7FFFF) | n49);
                //printf("EventOP: tileEvents[%d] = %X;\n", (n50 * 2 + 1), this->app->render->tileEvents[n50 * 2 + 1]);
                break;
            }

            case Enums::EV_HIDE: {
                //printf("EV_HIDE -> %d\n", this->IP);
                short sprite = this->getUShortArg();
                this->app->render->mapSpriteInfo[sprite] |= Enums::SPRITE_FLAG_HIDDEN;
                short n52 = this->app->render->mapSprites[this->app->render->S_ENT + sprite];
                if (n52 != -1) {
                    Entity* entity14 = &this->app->game->entities[n52];
                    entity14->info |= Entity::ENTITY_FLAG_DIRTY;
                    this->app->game->unlinkEntity(entity14);
                    EntityDef* def = entity14->def;
                    if (def->eType == Enums::ET_ATTACK_INTERACTIVE) {
                        if (def->eSubType != 8 && def->eSubType != 10) {
                            this->app->game->destroyedObject(sprite);
                        }
                    }
                    else if (def->eType == Enums::ET_ITEM && (def->eSubType == 1 || def->eSubType == 2 || (def->eSubType == 0 && def->parm == 21))) {
                        this->app->game->foundLoot(sprite, 1);
                    }
                    else if (def->eType == Enums::ET_MONSTER) {
                        this->corpsifyMonster(entity14->linkIndex % 32, entity14->linkIndex / 32, entity14, false);
                        this->app->game->removeEntity(entity14);
                        entity14->info |= Entity::ENTITY_FLAG_DIRTY;
                    }
                }
                this->app->canvas->updateFacingEntity = true;
                break;
            }

            case Enums::EV_DROPITEM: {
                //printf("EV_DROPITEM -> %d\n", this->IP);
                int uShortArg11 = this->getUShortArg();
                short uByteArg19 = this->getUByteArg();
                short n56 = (short)(uShortArg11 & 0x1F);
                short n57 = (short)(uShortArg11 >> 5 & 0x1F);
                short n58 = (short)(uShortArg11 >> 10 & 0x1F);
                EntityDef* lookup2 = app->entityDefManager->lookup(uByteArg19);
                if (lookup2 == nullptr) {
                    this->app->Error("Cannot find an entity to drop. Err %i", 109); //ERR_EV_DROPITEM
                }
                this->app->game->spawnDropItem((n56 << 6) + 32, (n57 << 6) + 32, uByteArg19, lookup2->eType, lookup2->eSubType, lookup2->parm, n58, true);
                this->app->canvas->staleView = true;
                break;
            }

            case Enums::EV_PREVSTATE: {
                //printf("EV_PREVSTATE -> %d\n", this->IP);
                int index = this->getByteArg();
                --this->app->game->scriptStateVars[index];
                if (index == Enums::CODEVAR_OSCILLATE_FOV && this->app->game->scriptStateVars[index] == 1) {
                    this->app->player->removeStatusEffect(Enums::STATUS_EFFECT_FIRE);
                }
                break;
            }

            case Enums::EV_NEXTSTATE: {
                //printf("EV_NEXTSTATE -> %d\n", this->IP);
                int index = this->getByteArg();
                ++this->app->game->scriptStateVars[index];
                if (index == Enums::CODEVAR_OSCILLATE_FOV && this->app->game->scriptStateVars[index] == 1) {
                    this->app->player->removeStatusEffect(Enums::STATUS_EFFECT_FIRE);
                }
                break;
            }

            case Enums::EV_WAKEMONSTER: {
                //printf("EV_WAKEMONSTER -> %d\n", this->IP);
                int sprite = this->getUShortArg();
                short n53 = this->app->render->mapSprites[this->app->render->S_ENT + sprite];
                if (n53 == -1 || this->app->game->entities[n53].monster == nullptr) {
                    this->app->Error(23); //ERR_MISC_SCRIPT
                }
                Entity* entity17 = &this->app->game->entities[n53];
                if (entity17->def->eType == Enums::ET_MONSTER) {
                    int sprite = entity17->getSprite();
                    entity17->monster->frameTime = 0;
                    this->app->render->mapSpriteInfo[sprite] = ((this->app->render->mapSpriteInfo[sprite] & ~Enums::SPRITE_MASK_FRAMENUMBER) | (Enums::MANIM_IDLE << Enums::SPRITE_SHIFT_FRAMENUMBER));
                    this->app->game->activate(entity17, true, false, false, true);
                    break;
                }
                break;
            }

            case Enums::EV_SHOW_PLAYERATTACK: {
                //printf("EV_SHOW_PLAYERATTACK -> %d\n", this->IP);
                this->app->game->cinematicWeapon = this->getByteArg();
                if (!this->app->game->skippingCinematic && this->app->canvas->state == Canvas::ST_CAMERA) {
                    int n59 = this->app->game->cinematicWeapon * Combat::WEAPON_MAX_FIELDS;
                    this->app->combat->animLoopCount = this->app->combat->weapons[n59 + Combat::WEAPON_FIELD_NUMSHOTS] - 1;
                    this->app->combat->animTime = this->app->combat->weapons[n59 + Combat::WEAPON_FIELD_SHOTHOLD];
                    this->app->combat->animTime *= 10;
                    this->app->combat->animStartTime = this->app->gameTime;
                    this->app->combat->animEndTime = this->app->combat->animStartTime + this->app->combat->animTime;
                    this->app->combat->flashTime = ((this->app->game->cinematicWeapon == Enums::WP_BOOT) ? 200 : 0);
                    this->app->combat->flashDoneTime = this->app->combat->animStartTime + this->app->combat->flashTime;
                    break;
                }
                break;
            }

            case Enums::EV_MONSTER_PARTICLES: {
                //printf("EV_MONSTER_PARTICLES -> %d\n", this->IP);
                int sprite = this->getUShortArg();
                int sEnt = this->app->render->mapSprites[this->app->render->S_ENT + sprite];
                if (sEnt != -1) {
                    this->app->particleSystem->spawnMonsterBlood(&this->app->game->entities[sEnt], false);
                }
                break;
            }

            case Enums::EV_SPAWN_PARTICLES: {
                //printf("EV_SPAWN_PARTICLES -> %d\n", this->IP);
                short uByteArg21 = this->getUByteArg();
                int n60 = uByteArg21 >> 4 & 0x7;
                int n61 = uByteArg21 & 0xF;
                int uShortArg12 = this->getUShortArg();
                int n62 = (uShortArg12 & 0x3F);
                if ((uByteArg21 & 0x80) != 0x0) {
                    int n63 = ((uShortArg12 >> 11 & 0x1F) << 6) + 32;
                    int n64 = ((uShortArg12 >> 6 & 0x1F) << 6) + 32;
                    this->app->particleSystem->spawnParticles(n60, ParticleSystem::levelColors[n61], n63, n64, n62 + this->app->render->getHeight(n63, n64));
                    break;
                }
                this->app->particleSystem->spawnParticles(n60, ParticleSystem::levelColors[n61], uShortArg12);
                break;
            }

            case Enums::EV_FADEOP: {
                //printf("EV_FADEOP -> %d\n", this->IP);
                int uShortArg13 = this->getUShortArg();
                bool b6 = (uShortArg13 & 0x8000) == 0x8000;
                if (this->app->game->skippingCinematic) {
                    if (b6) {
                        this->app->render->endFade();
                        break;
                    }
                    break;
                }
                else {
                    if (b6) {
                        this->app->render->startFade(uShortArg13 & 0x7FFF, 2);
                        break;
                    }
                    this->app->render->startFade(uShortArg13, 1);
                    break;
                }
                break;
            }

            case Enums::EV_GIVEITEM: {
                //printf("EV_GIVEITEM -> %d\n", this->IP);
                short uByteArg17 = this->getUByteArg();
                short uByteArg18 = this->getUByteArg();
                uint8_t byteArg9 = this->getByteArg();
                if (this->throwAwayLoot) {
                    n2 = 1;
                    this->app->game->foundLoot(this->app->canvas->viewX + this->app->canvas->viewStepX, this->app->canvas->viewY + this->app->canvas->viewStepY, this->app->canvas->viewZ, 1);
                    break;
                }
                if (byteArg9 == 0) {
                    short i = (short)(uByteArg17 << 8 | uByteArg18);
                    short n54 = this->app->render->mapSprites[this->app->render->S_ENT + i];
                    if (n54 == -1) {
                        app->Error("Sprite index %i error. Err %i", i, 16); //ERR_GIVE_ITEM
                    }
                    if (!this->app->game->entities[n54].touched(true)) {
                        n2 = 1;
                    }
                    break;
                }
                EntityDef* lookup = app->entityDefManager->lookup(uByteArg17);
                if (lookup == nullptr) {
                    app->Error("Cannot find an entity to give. Err %i", 109); //ERR_EV_DROPITEM
                }
                short n55 = (short)((int8_t)uByteArg18);
                if (n55 < 0) {
                    if (!this->app->player->give(lookup->eSubType, lookup->parm, n55, true)) {
                        n2 = 1;
                    }
                }
                else {
                    Entity* spawnDropItem = this->app->game->spawnDropItem(0, 0, uByteArg17, lookup, n55, false);
                    uint8_t eType = spawnDropItem->def->eType;
                    if (!spawnDropItem->touched(false)) {
                        this->app->game->removeEntity(spawnDropItem);
                        n2 = 1;
                    }
                    else if (eType != Enums::ET_PLAYERCLIP && n55 > 0) {
                        this->app->game->foundLoot(this->app->canvas->viewX + this->app->canvas->viewStepX, this->app->canvas->viewY + this->app->canvas->viewStepY, this->app->canvas->viewZ, 1);
                    }
                }
                break;
            }

            case Enums::EV_NAMEENTITY: {
                //printf("EV_NAMEENTITY -> %d\n", this->IP);
                short uByteArg7 = this->getUShortArg();
                short uByteArg8 = this->getUShortArg();
                short n37 = this->app->render->mapSprites[this->app->render->S_ENT + uByteArg7];
                if (n37 != -1) {
                    this->app->game->entities[n37].info |= Entity::ENTITY_FLAG_DIRTY;
                    this->app->game->entities[n37].name = (short)(uByteArg8 | this->app->canvas->loadMapStringID << 10);
                    break;
                }
                break;
            }

            case Enums::EV_DROPMONSTERITEM: {
                //printf("EV_DROPMONSTERITEM -> %d\n", this->IP);
                bool b7 = false;
                int uShortArg14 = this->getUShortArg();
                int uByteArg22 = this->getUByteArg();
                if ((uShortArg14 & 0x8000) != 0x0) {
                    uByteArg22 = (uByteArg22 << 8 | this->getUByteArg());
                    uShortArg14 &= 0x7FFF;
                    b7 = true;
                }
                short uByteArg23 = this->getUByteArg();
                short n65 = this->app->render->mapSprites[this->app->render->S_X + uShortArg14];
                short n66 = this->app->render->mapSprites[this->app->render->S_Y + uShortArg14];
                if (!b7) {
                    EntityDef* lookup3 = app->entityDefManager->lookup(uByteArg22);
                    if (lookup3 == nullptr) {
                        this->app->Error("Cannot find an entity to drop. Err %i", 109); //ERR_EV_DROPITEM
                    }
                    this->app->game->spawnDropItem(n65, n66, uByteArg22, lookup3, uByteArg23, true);
                }
                else {
                    Entity* entity18 = &this->app->game->entities[this->app->render->mapSprites[this->app->render->S_ENT + uByteArg22]];
                    int height = app->render->getHeight(n65, n66);
                    this->app->render->mapSprites[app->render->S_X + uByteArg22] = n65;
                    this->app->render->mapSprites[app->render->S_Y + uByteArg22] = n66;
                    this->app->render->mapSprites[app->render->S_Z + uByteArg22] = (short)(32 + height);
                    this->app->render->relinkSprite(uByteArg22);
                    this->app->game->unlinkEntity(entity18);
                    this->app->game->linkEntity(entity18, n65 >> 6, n66 >> 6);
                    this->app->game->throwDropItem(n65, n66, height, entity18);
                    entity18->info |= Entity::ENTITY_FLAG_DIRTY;
                }
                this->app->canvas->staleView = true;
                break;
            }

            case Enums::EV_SETDEATHFUNC: {
                //printf("EV_SETDEATHFUNC -> %d\n", this->IP);
                short uByteArg24 = this->getUShortArg();
                short shortArg = this->getShortArg();
                short n67 = this->app->render->mapSprites[this->app->render->S_ENT + uByteArg24];
                if (n67 != -1) {
                    Entity* entity20 = &this->app->game->entities[n67];
                    if (shortArg != -1) {
                        this->app->game->addEntityDeathFunc(entity20, shortArg);
                    }
                    else {
                        this->app->game->removeEntityFunc(entity20);
                    }
                    break;
                }
                break;
            }

            case Enums::EV_PLAYSOUND: {
                //printf("EV_PLAYSOUND -> %d\n", this->IP);
                short resID = this->getUByteArg();
                int args = this->getUByteArg();
                //printf("resID -> %d\n", resID);
                this->app->sound->playSound(resID + 1000, args >> 4, args & 0xF, 0);
                break;
            }

            case Enums::EV_NPCCHAT: {
                //printf("EV_NPCCHAT -> %d\n", this->IP);
                int uShortArg15 = this->getUShortArg();
                int param = uShortArg15 >> 14  & 0x3;
                int n68 = uShortArg15 & 0x3FFF;
                if (this->app->render->mapSprites[this->app->render->S_ENT + n68] != -1) {
                    Entity* entity21 = &this->app->game->entities[this->app->render->mapSprites[this->app->render->S_ENT + n68]];
                    if (entity21->def->eType == Enums::ET_NPC) {
                        entity21->param = param;
                        break;
                    }
                }
                this->app->Error(14); // ERR_EV_SHOWCHATBUBBLE
                break;
            }

            case Enums::EV_STOCKSTATION: {
                //printf("EV_STOCKSTATION -> %d\n", this->IP);
                short uByteArg22 = this->getUShortArg();
                short uByteArg23 = this->getUByteArg();
                short uByteArg24 = this->getUByteArg();
                short uByteArg25 = this->getUByteArg();
                int mixingIndex = app->game->getMixingIndex(uByteArg22);
                if (mixingIndex != -1) {
                    app->game->mixingStations[mixingIndex + 1] = uByteArg23;
                    app->game->mixingStations[mixingIndex + 2] = uByteArg24;
                    app->game->mixingStations[mixingIndex + 3] = uByteArg25;
                }
                break;
            }

            case Enums::EV_LERPFLAT: {
                //printf("EV_LERPFLAT -> %d\n", this->IP);
                this->getUByteArg();
                this->getUShortArg();
                break;
            }

            case Enums::EV_GIVELOOT: {
                //printf("EV_GIVELOOT -> %d\n", this->IP);
                if (this->app->canvas->showingLoot) {
                    this->unpauseTime = 1;
                    return 2;
                }
                this->composeLootDialog();
                if (!this->throwAwayLoot) {
                    this->app->game->skipAdvanceTurn = true;
                    this->app->game->queueAdvanceTurn = false;
                    this->unpauseTime = -1;
                    n = 2;
                    break;
                }
                break;
            }

            case Enums::EV_MARKTILE: {
                //printf("EV_MARKTILE -> %d\n", this->IP);
                int uShortArg16 = this->getUShortArg();
                int n73 = uShortArg16 & Enums::MARKTILE_MASK;
                bool b8 = (n73 & Enums::UNMARKTILE) != 0x0;
                int n74 = uShortArg16 >> 5 & 0x1F;
                int n75 = uShortArg16 & 0x1F;
                int n76 = n73 & ~Enums::UNMARKTILE;
                if ((n76 & Enums::MARKTILE_MONSTER_CLIP) != 0x0) {
                    if (b8) {
                        app->game->baseVisitedTiles[n75] &= ~(1 << n74);
                    }
                    else {
                        app->game->baseVisitedTiles[n75] |= 1 << n74;
                    }
                }
                int n79 = (n76 & ~Enums::MARKTILE_MONSTER_CLIP) >> 8;
                if (b8) {
                    app->render->mapFlags[n75 * 32 + n74] &= (uint8_t)~n79;
                }
                else {
                    app->render->mapFlags[n75 * 32 + n74] |= (uint8_t)n79;
                }
                break;
            }

            case Enums::EV_UPDATEJOURNAL: {
                //printf("EV_UPDATEJOURNAL -> %d\n", this->IP);
                short uByteArg27 = this->getUByteArg();
                short uByteArg28 = this->getUByteArg();
                this->app->player->updateQuests(uByteArg27, uByteArg28);
                if (uByteArg28 == 0) {
                    this->app->player->showHelp(CodeStrings::GENHELP_JOURNAL, true);
                }
                break;
            }

            case Enums::EV_BRIBE_ENTITY: {
                //printf("EV_BRIBE_ENTITY -> %d\n", this->IP);
                int uShortArg17 = this->getUShortArg();
                int n82 = 0;
                if (app->render->mapSprites[app->render->S_ENT + uShortArg17] != -1) {
                    if (app->player->give(Enums::IT_INVENTORY, Enums::INV_TREASURE_GOBLET, -1, true)) {
                        n82 = Enums::INV_TREASURE_GOBLET;
                    }
                    else if (app->player->give(Enums::IT_INVENTORY, Enums::INV_TREASURE_CROSS, -1, true)) {
                        n82 = Enums::INV_TREASURE_CROSS;
                    }
                    else if (app->player->give(Enums::IT_INVENTORY, Enums::INV_TREASURE_CROWN, -1, true)) {
                        n82 = Enums::INV_TREASURE_CROWN;
                    }
                }
                if (n82 == 0) {
                    app->game->scriptStateVars[Enums::CODEVAR_DIALOG_CHOICE] = 0;
                }
                else {
                    app->game->scriptStateVars[Enums::CODEVAR_DIALOG_CHOICE] = 2;
                }
                break;
            }

            case Enums::EV_PLAYER_ADD_STAT: {
                //printf("EV_PLAYER_ADD_STAT -> %d\n", this->IP);
                short uByteArg29 = this->getUByteArg();
                this->app->player->modifyStat(uByteArg29 >> 5 & 0x7, ((uByteArg29 & 0x1F) << 27) >> 27);
                break;
            }

            case Enums::EV_PLAYER_ADD_RECIPE: {
                //printf("EV_PLAYER_ADD_RECIPE -> %d\n", this->IP);
                app->player->cocktailDiscoverMask |= 1 << this->getUByteArg() - 5;
                break;
            }

            case Enums::EV_RESPAWN_MONSTER: {
                //printf("EV_RESPAWN_MONSTER -> %d\n", this->IP);
                int uShortArg21 = this->getUShortArg();
                short uByteArg30 = this->getUByteArg();
                short uByteArg31 = this->getUByteArg();
                if (this->app->render->mapSprites[this->app->render->S_ENT + uShortArg21] != -1) {
                    Entity* entity23 = &this->app->game->entities[this->app->render->mapSprites[this->app->render->S_ENT + uShortArg21]]; 
                    if ((entity23->info & (Entity::ENTITY_FLAG_GIBBED | Entity::ENTITY_FLAG_CORPSE)) && 
                        this->app->game->findMapEntity(uByteArg30, uByteArg31, (1 << Enums::ET_PLAYER | 1 << Enums::ET_MONSTER | 1 << Enums::ET_ATTACK_INTERACTIVE)) == nullptr) {
                        entity23->resurrect((uByteArg30 << 6) + 32, (uByteArg31 << 6) + 32, 32);
                        break;
                    }
                }
                n2 = -1;
                break;
            }

            case Enums::EV_SCREEN_SHAKE: {
                //printf("EV_SCREEN_SHAKE -> %d\n", this->IP);
                int uShortArg22 = this->getUShortArg();
                int n90 = uShortArg22 >> 14 & 0x3;
                if (n90 >= 0) {
                    ++n90;
                }
                int n91 = uShortArg22 >> 7 & 0x7F;
                if (n91 > 0) {
                    n91 = n91 + 1 << 4;
                }
                int n92 = uShortArg22 & 0x7F;
                if (n92 > 0) {
                    n92 = n92 + 1 << 4;
                }
                this->app->canvas->startShake(n91, n90, n92);
                break;
            }

            case Enums::EV_SPEECHBUBBLE: {
                //printf("EV_SPEECHBUBBLE -> %d\n", this->IP);
                int16_t texId = (int16_t)this->getUShortArg();
                this->app->hud->showSpeechBubble(texId);
                break;
            
            }

            case Enums::EV_AWARDSECRET: {
                //printf("EV_AWARDSECRET -> %d\n", this->IP);
                this->app->game->awardSecret(false);
                break;
            }

            case Enums::EV_AIGOAL: {
                //printf("EV_AIGOAL -> %d\n", this->IP);
                int uShortArg23 = this->getUShortArg();
                int n93 = uShortArg23 >> 12 & 0xF;
                uint8_t byteArg10 = this->getByteArg();
                int n94 = uShortArg23 & 0xFFF;
                if (this->app->render->mapSprites[this->app->render->S_ENT + n94] != -1) {
                    this->setAIGoal(&this->app->game->entities[this->app->render->mapSprites[this->app->render->S_ENT + n94]], n93, byteArg10);
                    break;
                }
                this->app->Error(76); //ERR_EV_AIGOAL
                break;
            }

            case Enums::EV_ADVANCETURN: {
                //printf("EV_ADVANCETURN -> %d\n", this->IP);
                if (this->app->game->interpolatingMonsters) {
                    this->app->game->snapMonsters(true);
                }
                while (this->app->game->combatMonsters != nullptr) {
                    this->app->game->combatMonsters->undoAttack();
                }
                this->app->game->endMonstersTurn();
                this->app->canvas->clearEvents(1);
                this->app->game->advanceTurn();
                break;
            }

            case Enums::EV_MINIGAME: {
                //printf("EV_MINIGAME -> %d\n", this->IP);
                uint8_t byteArg9 = this->getByteArg();
                int n88 = (byteArg9 & 0xF8) >> 3;
                int n89 = byteArg9 & 0x7;
                if (n89 == 1) {
                    this->app->canvas->startKicking(false);
                    this->app->canvas->kickingDir = n88 << 7;
                    break;
                }
                this->app->cardGames->initGame(n89, this, true);
                this->unpauseTime = -1;
                n = 2;
                break;
            }

            case Enums::EV_ENDMINIGAME: {
                //printf("EV_ENDMINIGAME -> %d\n", this->IP);
                if (!this->app->canvas->isChickenKicking) {
                    break;
                }
                app->canvas->endKickingGame();
                if (this->app->canvas->kickingFromMenu) {
                    this->app->canvas->backToMain(false);
                    this->app->menuSystem->setMenu(Menus::MENU_MAIN_MINIGAME);
                    return 1;
                }
                this->app->player->selectPrevWeapon();
                break;
            }

            case Enums::EV_ENDROUND: {
                //printf("EV_ENDROUND -> %d\n", this->IP);
                if (this->app->canvas->isChickenKicking) {
                    this->app->canvas->endKickingRound();
                }
                break;
            }

            case Enums::EV_PLAYERATTACK: {
                //printf("EV_PLAYERATTACK -> %d\n", this->IP);
                int uShortArg24 = this->getUShortArg();
                int weapon = uShortArg24 >> 12 & 0xF;
                int n119 = uShortArg24 & 0xFFF;
                if (weapon == 0) {
                    if ((this->app->player->weapons & 0x2LL) != 0x0LL) {
                        weapon = 1;
                    }
                    else if ((this->app->player->weapons & 0x4LL) != 0x0LL) {
                        weapon = 2;
                    }
                }
                if (this->app->render->mapSprites[this->app->render->S_ENT + n119] != -1) {
                    Entity* entity24 = &this->app->game->entities[this->app->render->mapSprites[this->app->render->S_ENT + n119]];
                    this->app->player->ce->weapon = weapon;
                    this->app->combat->performAttack(nullptr, entity24, 0, 0, true);
                }
                n = this->evWait(1);
                break;
            }

            case Enums::EV_SET_FOG_COLOR: {
                //printf("EV_SET_FOG_COLOR -> %d\n", this->IP);
                this->app->render->buildFogTables(this->getIntArg());
                break;
            }

            case Enums::EV_LERP_FOG: {
                //printf("EV_LERP_FOG -> %d\n", this->IP);
                int intArg2 = this->getIntArg();
                this->app->render->startFogLerp(intArg2 & 0x7FF, (intArg2 >> 11) & 0x7FF, ((uint8_t)(intArg2 >> 22) & 0xFF) * 100);
                break;
            }

            case Enums::EV_LERPSPRITEOFFSET: {
                //printf("EV_LERPSPRITEOFFSET -> %d\n", this->IP);

//#define BYTE2(x) (((x) >> 16) & 0xFF)
//#define HIWORD(x) ((x) >> 16)

                /*short sprite = this->getUByteArg();
                int time = this->getUByteArg() * 100;
                int intArg = this->getIntArg();
                int dstY = intArg & 0x7FF;
                int dstX = intArg >> 11 & 0x7FF;
                int flags = intArg >> 22 & 0x3;
                int dstZ = (intArg >> 24 & 0xFF) - 48;*/


                int args1 = this->getIntArg();
                int args2 = this->getIntArg();

                short sprite = args1 >> 22 & 0x3FF;
                int time = args2 & 0xffff;
                int dstX = args1 & 0x7FF;
                int dstY = args2 >> 16 & 0x7FF;
                int flags = (args1 >> 16 & 0xFF) & 0xF; // (args1 << 0xc) >> 0x1c;//
                int dstZ = this->getUByteArg() - 48;

                LerpSprite* allocLerpSprite2 = this->app->game->allocLerpSprite(this, sprite, (flags & 0x2) != 0x0);
                if (allocLerpSprite2 == nullptr) {
                    return 0;
                }
                allocLerpSprite2->dstX = dstX;
                allocLerpSprite2->dstY = dstY;
                short n15 = this->app->render->mapSprites[this->app->render->S_ENT + sprite];
                if (n15 != -1) {
                    this->app->game->entities[n15].info |= Entity::ENTITY_FLAG_DIRTY;
                    if (this->app->game->entities[n15].monster != nullptr) {
                        this->app->game->entities[n15].monster->flags |= Enums::MFLAG_UNK;
                    }
                }
                allocLerpSprite2->dstZ = this->app->render->getHeight(allocLerpSprite2->dstX, allocLerpSprite2->dstY) + dstZ;
                allocLerpSprite2->srcX = this->app->render->mapSprites[this->app->render->S_X + sprite];
                allocLerpSprite2->srcY = this->app->render->mapSprites[this->app->render->S_Y + sprite];
                allocLerpSprite2->srcZ = this->app->render->mapSprites[this->app->render->S_Z + sprite];
                allocLerpSprite2->srcScale = allocLerpSprite2->dstScale = this->app->render->mapSprites[this->app->render->S_SCALEFACTOR + sprite];
                allocLerpSprite2->startTime = app->gameTime;
                allocLerpSprite2->travelTime = time;
                allocLerpSprite2->calcDist();
                allocLerpSprite2->flags = flags;//(flags & 0x3);
                if (time == 0) {
                    if ((this->app->game->updateLerpSprite(allocLerpSprite2) & 0x1) != 0x0) {
                        this->app->canvas->invalidateRect();
                        break;
                    }
                    n = 0;
                    break;
                }
                else {
                    if (!(allocLerpSprite2->flags & Enums::LS_FLAG_ASYNC)) {
                        this->app->game->skipAdvanceTurn = true;
                        this->app->game->queueAdvanceTurn = false;
                        this->unpauseTime = -1;
                        n = 2;
                        break;
                    }
                    break;
                }
                break;
            }

            case Enums::EV_DISABLED_WEAPONS: {
                //printf("EV_DISABLED_WEAPONS -> %d\n", this->IP);
                this->app->player->disabledWeapons = this->getShortArg();
                if ((this->app->player->disabledWeapons & (1 << this->app->player->ce->weapon)) != 0x0) {
                    this->app->player->selectNextWeapon();
                    break;
                }
                break;
            }

            case Enums::EV_LERPSCALE: {
                //printf("EV_LERPSCALE -> %d\n", this->IP);
                int args = this->getUShortArg();
                int sprite = args & 0xFFF;//uShortArg25 >> 4;
                int flags = args >> 12;
                int time = this->getUShortArg();
                short uByteArg32 = this->getUByteArg();
                LerpSprite* allocLerpSprite3 = this->app->game->allocLerpSprite(this, sprite, (flags & 0x2) != 0x0);
                if (allocLerpSprite3 == nullptr) {
                    return 0;
                }
                allocLerpSprite3->srcX = allocLerpSprite3->dstX = this->app->render->mapSprites[this->app->render->S_X + sprite];
                allocLerpSprite3->srcY = allocLerpSprite3->dstY = this->app->render->mapSprites[this->app->render->S_Y + sprite];
                allocLerpSprite3->srcZ = allocLerpSprite3->dstZ = this->app->render->mapSprites[this->app->render->S_Z + sprite];
                allocLerpSprite3->srcScale = this->app->render->mapSprites[this->app->render->S_SCALEFACTOR + sprite];
                allocLerpSprite3->dstScale = uByteArg32 << 1;
                short n125 = this->app->render->mapSprites[this->app->render->S_ENT + sprite];
                if (n125 != -1) {
                    this->app->game->entities[n125].info |= Entity::ENTITY_FLAG_DIRTY;
                }
                allocLerpSprite3->startTime = app->gameTime;
                allocLerpSprite3->travelTime = time;
                allocLerpSprite3->flags = flags;//(flags & 0x3);
                if (time == 0) {
                    if ((this->app->game->updateLerpSprite(allocLerpSprite3) & 0x1) != 0x0) {
                        this->app->canvas->invalidateRect();
                        break;
                    }
                    n = 0;
                    break;
                }
                else {
                    if (!(flags & Enums::LS_FLAG_ASYNC)) {
                        this->app->game->skipAdvanceTurn = true;
                        this->app->game->queueAdvanceTurn = false;
                        this->unpauseTime = -1;
                        n = 2;
                        break;
                    }
                    break;
                }
                break;
            }

            case Enums::EV_GIVEAWARD: {
                //printf("EV_GIVEAWARD -> %d\n", this->IP);
                app->player->giveMedal(this->getByteArg(), this);
                this->unpauseTime = 0;
                n = 2;
                break;
            }

            case Enums::EV_GIVEBOOK: {
                //printf("EV_GIVEBOOK -> %d\n", this->IP);
                app->player->giveBook(this->getByteArg(), this);
                this->unpauseTime = 0;
                n = 2;
                break;
            }
                 
            case Enums::EV_OFFERBOOK: {
                //printf("EV_OFFERBOOK -> %d\n", this->IP);
                app->player->offerBook(this->getByteArg(), this);
                this->unpauseTime = -1;
                n = 2;
                break;
            }

            case Enums::EV_STARTMIXING: {
                //printf("EV_STARTMIXING -> %d\n", this->IP);
                int mixingIndex2 = app->game->getMixingIndex(this->getShortArg());
                if (mixingIndex2 != -1 && !app->game->mixingStationEmpty(mixingIndex2)) {
                    this->app->canvas->curStation = mixingIndex2;
                    this->app->canvas->setState(Canvas::ST_MIXING);
                    break;
                }
                this->app->hud->addMessage(Strings::FILE_CODESTRINGS, CodeStrings::EMPTY_STATION, 2);
                break;
            }
               
            case Enums::EV_DEBUGPRINT: {
                //printf("EV_DEBUGPRINT -> %d\n", this->IP);
                if (this->debugString == nullptr) {
                    this->debugString = this->app->localization->getLargeBuffer();
                }
                uint8_t byteArg11 = this->getByteArg();
                if (byteArg11 == 0) {
                    for (char c = (char)this->getUByteArg(); c != '\0'; c = (char)this->getUByteArg()) {
                        this->debugString->append(c);
                    }
                }
                else if (byteArg11 == 1) {
                    int index = this->getUByteArg();
                    this->debugString->append(this->app->game->scriptStateVars[index]);
                }
                if (this->peekNextCmd() != 66) {
                    this->debugString->dispose();
                    this->debugString = nullptr;
                    break;
                }
                break;
            }

            case Enums::EV_GOTO_MENU: {
                //printf("EV_GOTO_MENU -> %d\n", this->IP);
                this->app->menuSystem->setMenu(this->getUByteArg());
                break;
            }

            case Enums::EV_START_DRIVING: {
                //printf("EV_START_DRIVING -> %d\n", this->IP);
                short uByteArg33 = this->getUByteArg();
                bool b9 = (uByteArg33 & 0x80) != 0x0;
                this->setupCamera(uByteArg33 & 0x7F);
                this->app->game->activeCamera->cameraThread = this;
                this->app->canvas->setState(Canvas::ST_DRIVING); // ST_DRIVING
                break;
            }

            case Enums::EV_TURN_PLAYER: {
                //printf("EV_TURN_PLAYER -> %d\n", this->IP);
                uint8_t byteArg2 = this->getByteArg();
                int viewAngle2 = this->app->canvas->viewAngle & 0x3FF;
                int destAngle2 = byteArg2 << 7;
                if (destAngle2 - viewAngle2 > 512) {
                    destAngle2 -= 1024;
                }
                else if (destAngle2 - viewAngle2 < -512) {
                    destAngle2 += 1024;
                }
                if (viewAngle2 != destAngle2) {
                    this->app->canvas->viewAngle = viewAngle2;
                    this->app->canvas->destAngle = destAngle2;
                    this->app->canvas->startRotation(false);
                    this->app->canvas->gotoThread = this;
                    this->unpauseTime = -1;
                    n = 2;
                    break;
                }
                break;
            }

            case Enums::EV_STATUS_EFFECT: {
                //printf("EV_STATUS_EFFECT -> %d\n", this->IP);
                short effect = this->getUByteArg();
                if ((effect & 0x80) != 0x0) {
                    app->player->removeStatusEffect(effect & 0x7F);
                    break;
                }
                uint8_t amount = this->getByteArg();
                int truns = 30;
                if (effect == Enums::STATUS_EFFECT_AGILITY) {
                    truns = 20;
                }
                else if (effect == Enums::STATUS_EFFECT_ANTIFIRE) {
                    truns = 10;
                }
                else if (effect == Enums::STATUS_EFFECT_PURIFY) {
                    truns = 10;
                }
                else if (effect == Enums::STATUS_EFFECT_FEAR) {
                    truns = 6;
                }
                else if (effect == Enums::STATUS_EFFECT_COLD) {
                    truns = 5;
                }
                app->player->addStatusEffect(effect, amount, truns);
                app->player->translateStatusEffects();
                break;
            }

            case Enums::EV_JOURNAL_TILE: {
                //printf("EV_JOURNAL_TILE -> %d\n", this->IP);
                int arg1 = this->getUByteArg();
                int tileX = this->getUByteArg() & 0x1F;
                int tileY = this->getUByteArg() & 0x1F;
                app->player->setQuestTile(arg1, tileX, tileY);
                break;
            }

            case Enums::EV_MAKE_CORPSE: {
                //printf("EV_MAKE_CORPSE -> %d\n", this->IP);
                int uShortArg27 = this->getUShortArg();
                short n127 = (short)((this->getUByteArg() << 6) + 32);
                short n128 = (short)((this->getUByteArg() << 6) + 32);
                Entity* entity26 = &this->app->game->entities[this->app->render->mapSprites[this->app->render->S_ENT + uShortArg27]];
                if (entity26->monster != nullptr) {
                    this->corpsifyMonster(n127, n128, entity26, true);
                    break;
                }
                break;
            }

            case Enums::EV_INVENTORY_OP: {
                //printf("EV_INVENTORY_OP -> %d\n", this->IP);
                uint8_t byteArg12 = this->getByteArg();
                if (byteArg12 == 0) {
                    this->stripInventory();
                    break;
                }
                if (byteArg12 == 1) {
                    this->restoreInventory();
                    break;
                }
            }

            case Enums::EV_END_GAME: {
                //printf("EV_END_GAME -> %d\n", this->IP);
                app->player->gameCompleted = true;
                app->canvas->endingGame = true;
                app->canvas->setState(Canvas::ST_EPILOGUE);
                break;
            }

            case Enums::EV_LERPSPRITEPARABOLA: {
                //printf("EV_LERPSPRITEPARABOLA -> %d\n", this->IP);
                int args = this->getIntArg();
                int time = this->getUShortArg();
                int sprite = args >> 22 & 0x3FF;
                int dstX = args >> 17 & 0x1F;
                int dstY = args >> 12 & 0x1F;
                int height = (args >> 4 & 0xFF) - 48;
                int flags = args & 0xF;
                LerpSprite* allocLerpSprite4 = this->app->game->allocLerpSprite(this, sprite, (flags & 0x2) != 0x0);
                if (allocLerpSprite4 == nullptr) {
                    return 0;
                }
                allocLerpSprite4->dstX = 32 + (dstX << 6);
                allocLerpSprite4->dstY = 32 + (dstY << 6);
                short n133 = this->app->render->mapSprites[this->app->render->S_ENT + sprite];
                if (n133 != -1) {
                    this->app->game->entities[n133].info |= Entity::ENTITY_FLAG_DIRTY;
                    if (this->app->game->entities[n133].monster != nullptr) {
                        this->app->game->entities[n133].monster->flags &= ~Enums::MFLAG_UNK;
                    }
                }
                allocLerpSprite4->srcX = this->app->render->mapSprites[this->app->render->S_X + sprite];
                allocLerpSprite4->srcY = this->app->render->mapSprites[this->app->render->S_Y + sprite];
                allocLerpSprite4->srcZ = this->app->render->mapSprites[this->app->render->S_Z + sprite];
                allocLerpSprite4->dstZ = this->app->render->getHeight(allocLerpSprite4->dstX, allocLerpSprite4->dstY) + (allocLerpSprite4->srcZ - this->app->render->getHeight(allocLerpSprite4->srcX, allocLerpSprite4->srcY));
                allocLerpSprite4->srcScale = allocLerpSprite4->dstScale = this->app->render->mapSprites[this->app->render->S_SCALEFACTOR + sprite];
                allocLerpSprite4->height = height;
                allocLerpSprite4->startTime = app->gameTime;
                allocLerpSprite4->travelTime = time;
                allocLerpSprite4->calcDist();
                allocLerpSprite4->flags = flags; //(n132 & 0x3);
                allocLerpSprite4->flags |= Enums::LS_FLAG_PARABOLA;
                if (time == 0) {
                    if ((this->app->game->updateLerpSprite(allocLerpSprite4) & 0x1) != 0x0) {
                        this->app->canvas->invalidateRect();
                        break;
                    }
                    n = 0;
                    break;
                }
                else {
                    if (!(allocLerpSprite4->flags & Enums::LS_FLAG_ASYNC)) {
                        this->app->game->skipAdvanceTurn = true;
                        this->app->game->queueAdvanceTurn = false;
                        this->unpauseTime = -1;
                        n = 2;
                        break;
                    }
                    break;
                }
                break;
            }

            case Enums::EV_STOPSOUND: {
                //printf("EV_STOPSOUND -> %d\n", this->IP);
                int resID = this->getUByteArg();
                app->sound->stopSound(resID + 1000, false);
                break;
            }

            case Enums::EV_END: {
                if (this->stackPtr != 0) {
                    app->Error("The frame pointer should be zero if the script has completed.", 102);
                }
                return 1;
            }

            default: {
                app->Error("Cannot handle event: %d", mapByteCode[this->IP]);
                break;
            }
        }

        if (b) {
            this->app->game->scriptStateVars[Enums::CODEVAR_COMMMAND_RETURN] = n2;
        }
        ++this->IP;
    }
    return n;
}

void ScriptThread::init() {
    //printf("ScriptThread::init\n");
    this->stackPtr = 0;
    this->IP = 0;
    this->FP = 0;
    this->unpauseTime = 0;
    this->state = 2;
    this->throwAwayLoot = false;
}

void ScriptThread::reset() {
    this->inuse = false;
    this->init();
}

int ScriptThread::attemptResume(int n) {
    if (this->stackPtr == 0) {
        return 1;
    }
    if (this->unpauseTime == -1 || n < this->unpauseTime) {
        return 2;
    }
    this->unpauseTime = 0;
    return this->run();
}

int ScriptThread::getIndex() {
    for (int i = 0; i < 20; ++i) {
        if (this == &this->app->game->scriptThreads[i]) {
            return i;
        }
    }
    return -1;
}

int ScriptThread::pop() {
    return this->scriptStack[--this->stackPtr];
}

void ScriptThread::push(bool b) {
    this->push(b ? 1 : 0);
}

void ScriptThread::push(int n) {
    this->scriptStack[this->stackPtr++] = n;
}

short ScriptThread::getUByteArg() {
    return (short)(this->app->render->mapByteCode[++this->IP] & 0xFF);
}

uint8_t ScriptThread::getByteArg() {
    return this->app->render->mapByteCode[++this->IP];
}

int ScriptThread::getUShortArg() {
    int n = ((this->app->render->mapByteCode[this->IP + 1] & 0xFF) << 8) | (this->app->render->mapByteCode[this->IP + 2] & 0xFF);
    this->IP += 2;
    return n;
}

short ScriptThread::getShortArg() {
    short n = (short)(this->app->render->mapByteCode[this->IP + 1] << 8 | (this->app->render->mapByteCode[this->IP + 2] & 0xFF));
    this->IP += 2;
    return n;
}

int ScriptThread::getIntArg() {
    int n = this->app->render->mapByteCode[this->IP + 1] << 24 | (this->app->render->mapByteCode[this->IP + 2] & 0xFF) << 16 | (this->app->render->mapByteCode[this->IP + 3] & 0xFF) << 8 | (this->app->render->mapByteCode[this->IP + 4] & 0xFF);
    this->IP += 4;
    return n;
}

void ScriptThread::composeLootDialog() {
    Text* largeBuffer = this->app->localization->getLargeBuffer();
    if (this->app->canvas->lootSource != -1) {
        this->app->localization->composeTextField(this->app->canvas->lootSource, largeBuffer);
        this->app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::YOU_RECEIVE, largeBuffer);
        this->app->canvas->lootSource = -1;
    }
    else {
        this->app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::CONTAINS, largeBuffer);
    }
    if (!this->throwAwayLoot) {
        this->app->canvas->showingLoot = true;
        this->app->canvas->setState(Canvas::ST_DIALOG);
    }
    int n = 0;
    uint8_t byteArg = this->getByteArg();
    int n2 = 0;
    for (uint8_t b = 0; b < byteArg; ++b) {
        int uShortArg = this->getUShortArg();
        int n3 = uShortArg >> 12 & 0xF;
        if (n3 == 8) {
            short n4 = (short)(uShortArg & 0xFFF);
            largeBuffer->append('\x88');
            this->app->localization->composeText(this->app->canvas->loadMapStringID, n4, largeBuffer);
            largeBuffer->append("|");
        }
        else if (n3 == 7) {
            this->app->player->updateQuests((short)(uShortArg & 0xFFF), 0);
        }
        else if (n3 == 9) {
            short n4 = (short)(uShortArg & 0xFFF);
            largeBuffer->append('\x88');
            this->app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::BOOK_LABEL, largeBuffer);
            this->app->localization->composeText(Strings::FILE_BOOKSTRINGS, app->player->bookMap[n4 * 4], largeBuffer);
            largeBuffer->append("|");
        }
        else {
            int n5 = (uShortArg & 0xFC0) >> 6;
            int n6 = uShortArg & 0x3F;
            ++n;
            if (!this->throwAwayLoot) {
                this->app->localization->resetTextArgs();
                this->app->localization->addTextArg('\x88');
                switch (n3) {
                    case Enums::IT_INVENTORY:
                    case Enums::IT_FOOD:
                    case Enums::IT_ARMOR: {
                        this->app->player->give(n3, n5, n6, false);
                        EntityDef *find = this->app->entityDefManager->find(Enums::ET_ITEM, n3, n5);
                        this->app->localization->addTextArg(n6);
                        this->app->localization->addTextArg(Strings::FILE_ENTITYSTRINGS, find->longName);
                        this->app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::DASH_X_Y, largeBuffer);
                        break;
                    }
                    case Enums::IT_WEAPON: {
                        this->app->player->give(Enums::IT_WEAPON, n5, n6, true);
                        int n7 = n5 * Combat::WEAPON_MAX_FIELDS;
                        if (this->app->combat->weapons[n7 + Combat::WEAPON_FIELD_AMMOUSAGE] != 0) {
                            this->app->player->give(Enums::IT_AMMO, this->app->combat->weapons[n7 + Combat::WEAPON_FIELD_AMMOTYPE], 10, true);
                        }
                        EntityDef* find = this->app->entityDefManager->find(Enums::ET_ITEM, n3, n5);
                        this->app->localization->addTextArg(Strings::FILE_ENTITYSTRINGS, find->longName);
                        this->app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::DASH_X, largeBuffer);
                        break;
                    }
                    case Enums::IT_AMMO: {
                        if (this->app->game->difficulty != 4 || (1 << n5 & 0x112) != 0x0) {
                            this->app->player->give(Enums::IT_AMMO, n5, n6, false);
                            EntityDef* find = this->app->entityDefManager->find(Enums::ET_ITEM, n3, n5);
                            this->app->localization->addTextArg(n6);
                            this->app->localization->addTextArg(Strings::FILE_ENTITYSTRINGS, find->longName);
                            this->app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::DASH_X_Y, largeBuffer);
                            break;
                        }
                        break;
                    }
                }
            }
        }
    }
    if (!this->throwAwayLoot) {
        largeBuffer->setLength(largeBuffer->length() - 1);
        this->app->canvas->startDialog(this, largeBuffer, Enums::DLG_STYLE_CHEST, Enums::DLG_FLAG_NONE, true);
    }
    else {
        largeBuffer->setLength(0);
        this->app->localization->resetTextArgs();
        this->app->localization->addTextArg(byteArg);
        this->app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::DESTROYED_LOOT, largeBuffer);
        this->app->hud->addMessage(largeBuffer, 3);
    }
    largeBuffer->dispose();
    this->app->game->foundLoot(this->app->canvas->viewX + this->app->canvas->viewStepX, this->app->canvas->viewY + this->app->canvas->viewStepY, this->app->canvas->viewZ, n);
}

void ScriptThread::setAIGoal(Entity* entity, int goalType, int goalParam) {
    entity->monster->resetGoal();
    entity->monster->goalType = (uint8_t)goalType;
    if (goalType == EntityMonster::GOAL_MOVETOENTITY || goalType == EntityMonster::GOAL_FIGHT) {
        entity->monster->goalParam = 1;
    }
    else if (goalType == EntityMonster::GOAL_FLEE || goalType == EntityMonster::GOAL_STUN) {
        entity->monster->goalParam = goalParam;
    }
    if (!this->app->player->noclip) {
        if (!(entity->info & Entity::ENTITY_FLAG_ACTIVE)) {
            this->app->game->activate(entity, true, false, false, true);
        }
        entity->aiThink(true);
    }
    if (goalType == EntityMonster::GOAL_FIGHT) {
        entity->monster->goalFlags &= ~EntityMonster::GFL_ATTACK2EVADE;
        if (this->app->game->combatMonsters != nullptr) {
            this->app->combat->performAttack(this->app->game->combatMonsters, this->app->game->combatMonsters->monster->target, 0, 0, false);
        }
    }
}

void ScriptThread::corpsifyMonster(int x, int y, Entity* entity, bool b) {
    int sprite = entity->getSprite();
    this->app->game->snapLerpSprites(sprite);
    entity->monster->resetGoal();
    entity->monster->clearEffects();
    entity->undoAttack();
    this->app->game->deactivate(entity);
    this->app->render->mapSpriteInfo[sprite] = ((this->app->render->mapSpriteInfo[sprite] & ~(Enums::SPRITE_MASK_FRAMENUMBER | Enums::SPRITE_FLAG_HIDDEN)) | (Enums::MANIM_DEAD << Enums::SPRITE_SHIFT_FRAMENUMBER));
    this->app->render->mapSprites[this->app->render->S_X + sprite] = (short)x;
    this->app->render->mapSprites[this->app->render->S_Y + sprite] = (short)y;
    this->app->render->mapSprites[this->app->render->S_Z + sprite] = (short)(this->app->render->getHeight(x, y) + 32);
    this->app->render->relinkSprite(sprite);
    entity->info = ((entity->info & 0xFFFF) | Entity::ENTITY_FLAG_TAKEDAMAGE | Entity::ENTITY_FLAG_DIRTY | Entity::ENTITY_FLAG_CORPSE);
    entity->def = this->app->entityDefManager->find(Enums::ET_CORPSE, entity->def->eSubType, entity->def->parm);
    this->app->game->unlinkEntity(entity);
    this->app->game->linkEntity(entity, x >> 6, y >> 6);
    entity->checkMonsterDeath(false);
}

void ScriptThread::stripInventory() {
    this->app->player->selectWeapon(0);
    memcpy(this->app->player->inventoryCopy, this->app->player->inventory, sizeof(this->app->player->inventoryCopy));
    memcpy(this->app->player->ammoCopy, this->app->player->ammo, sizeof(this->app->player->ammoCopy));
    this->app->player->weaponsCopy = (int)(this->app->player->weapons & -1L);
    for (int i = 0; i < Enums::INV_MAX; ++i) {
        this->app->player->inventory[i] = 0;
    }
    for (int j = 0; j < 10; ++j) {
        this->app->player->ammo[j] = 0;
    }
    this->app->player->weapons = 9LL;
}

void ScriptThread::restoreInventory() {
    for (int i = 0; i < 29; ++i) {
        this->app->player->give(Enums::IT_INVENTORY, i, this->app->player->inventoryCopy[i], true);
    }
    for (int j = 0; j < 10; ++j) {
        this->app->player->give(Enums::IT_AMMO, j, this->app->player->ammoCopy[j], true);
    }
    for (int k = 1; k < 17; ++k) {
        if ((1 << k & this->app->player->weaponsCopy) != 0x0) {
            this->app->player->give(Enums::IT_WEAPON, k, 1, true);
        }
    }
}
