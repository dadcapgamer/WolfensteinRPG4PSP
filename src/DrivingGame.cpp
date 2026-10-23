#include <stdexcept>
#include <cstring>

#include "CAppContainer.h"
#include "App.h"
#include "Button.h"
#include "Canvas.h"
#include "Player.h"
#include "Render.h"
#include "Game.h"
#include "MayaCamera.h"
#include "Sound.h"
#include "Hud.h"
#include "MenuSystem.h"
#include "Menus.h"
#include "Enums.h"
#include "DrivingGame.h"

DrivingGame::DrivingGame() {
    memset(this, 0, sizeof(DrivingGame));
}

DrivingGame::~DrivingGame() {
}

void DrivingGame::init() {
	Applet* app = CAppContainer::getInstance()->app;
	app->player->ce->weapon = Enums::WP_MOUNTED_GUN_TURRET;
	app->canvas->clearRightSoftKey();
	app->canvas->setLeftSoftKey(Strings::FILE_CODESTRINGS, CodeStrings::MENU);
	this->mode = 2;
}

void DrivingGame::drivingState()
{
    int* p_sinTable; // r6
    int activeCameraKey; // r1
    struct Canvas* canvas; // r2
    int v6; // r3
    struct Canvas* v7; // r2
    int v8; // r3
    struct Canvas* v9; // r2
    int v10; // r3
    struct Canvas* v11; // r2
    int pitch; // r3
    struct Canvas* v13; // r2
    int yaw; // r3
    struct Canvas* v15; // r2
    int roll; // r3
    struct Canvas* v17; // r3
    int viewX; // r0
    int viewY; // r11
    int v20; // r3
    int v21; // r0
    bool v22; // cc
    Entity* activeMonsters; // r4
    int Sprite; // r0
    struct EntityMonster* monster; // r3
    int v26; // r8
    Game* v27; // r0
    struct Render* render; // lr
    int* data; // r1
    int v30; // r12
    int v31; // r8
    Game* game; // r12
    int numTraceEntities; // lr
    Entity* v34; // r4
    int eType; // r3
    int v36; // r6
    int v37; // r2
    struct Entity** traceEntities; // r1
    int v39; // r3
    int v40; // r3
    int v41; // r3
    int v42; // r3
    int v43; // r2
    int v44; // r2
    int v45; // r2
    int v46; // r3
    int v47; // r1
    int16_t MonsterSound; // r0
    struct Render* v49; // r1
    struct Hud* hud; // r6
    Sound* sound; // [sp+18h] [bp-2Ch]
    int v52; // [sp+1Ch] [bp-28h]
    int viewZ; // [sp+20h] [bp-24h]
    int v54; // [sp+24h] [bp-20h]
    struct Entity* PlayerEnt; // [sp+28h] [bp-1Ch]

    Applet* app = CAppContainer::getInstance()->app;
    p_sinTable = reinterpret_cast<int*>(app->render->sinTable);
    activeCameraKey = app->game->activeCameraKey;
    if (activeCameraKey != -1) {
        app->game->activeCamera->Update(activeCameraKey, app->gameTime - app->game->activeCameraTime);
    }
    app->player->facingEntity = 0;
    app->canvas->updateFacingEntity = 0;
    canvas = app->canvas;
    v6 = app->game->activeCamera->x >> 4;
    canvas->destX = v6;
    canvas->viewX = v6;
    v7 = app->canvas;
    v8 = app->game->activeCamera->y >> 4;
    v7->destY = v8;
    v7->viewY = v8;
    v9 = app->canvas;
    v10 = app->game->activeCamera->z >> 4;
    v9->destZ = v10;
    v9->viewZ = v10;
    v11 = app->canvas;
    pitch = app->game->activeCamera->pitch;
    v11->destPitch = pitch;
    v11->viewPitch = pitch;
    v13 = app->canvas;
    yaw = app->game->activeCamera->yaw;
    v13->destAngle = yaw;
    v13->viewAngle = yaw;
    v15 = app->canvas;
    roll = app->game->activeCamera->roll;
    v15->destRoll = roll;
    v15->viewRoll = roll;
    app->player->relink();
    v17 = app->canvas;
    viewX = v17->viewX;
    v52 = viewX;
    viewY = v17->viewY;
    viewZ = v17->viewZ;
    v20 = (viewX >> 6) + 32 * (viewY >> 6);
    if (v20 != this->curIndex)
    {
        this->curIndex = v20;
        app->game->touchTile(viewX, viewY, true);
    }
    v21 = this->fireCount;

    v22 = v21 <= 0;
    if (v21 > 0)
        v22 = app->gameTime - this->fireTime <= 300;
    if (!v22)
    {
        data = p_sinTable;
        v30 = (app->canvas->viewAngle - 85 * this->fireAngle) & 0x3FF;
        v54 = v52 + ((6 * data[(v30 + 256) & 0x3FF]) >> 10);
        v31 = viewY - ((6 * data[v30]) >> 10);
        this->fireCount = v21 - 1;
        this->fireTime = app->gameTime;
        app->sound->playSound(1149, 0, 4, 0);
        app->game->trace(v52, viewY, viewZ, v54, v31, viewZ, 0, 13997, 25);
        game = app->game;
        numTraceEntities = game->numTraceEntities;
        if (numTraceEntities <= 0)
            goto LABEL_49;
        v34 = game->traceEntities[0];
        eType = v34->def->eType;
        if (eType == Enums::ET_MONSTER || eType == Enums::ET_ATTACK_INTERACTIVE)
        {
            v36 = 0;
        }
        else
        {
            if (v34->def->eType == Enums::ET_WORLD)
            {
                v36 = 0;
            LABEL_48:
                v47 = game->traceFracs[v36];
            LABEL_50:
                game->gsprite_allocAnim(
                    242,
                    v52 + ((v47 * (v54 - v52)) >> 14),
                    viewY + ((v47 * (v31 - viewY)) >> 14),
                    viewZ);
                goto LABEL_9;
            }
            v37 = (numTraceEntities - 1) & 3;
            traceEntities = game->traceEntities;
            v36 = 0;
            if (!v37)
                goto LABEL_35;
            v34 = game->traceEntities[1];
            v36 = 1;
            v39 = v34->def->eType;
            if (v39 != Enums::ET_MONSTER && v39 != Enums::ET_ATTACK_INTERACTIVE)
            {
                traceEntities = &game->traceEntities[1];
                if (v34->def->eType == Enums::ET_WORLD)
                    goto LABEL_48;
                if (v37 == 1)
                    goto LABEL_35;
                if (v37 != 2)
                {
                    v34 = game->traceEntities[2];
                    v36 = 2;
                    v40 = v34->def->eType;
                    if (v40 == Enums::ET_MONSTER || v40 == Enums::ET_ATTACK_INTERACTIVE)
                        goto LABEL_53;
                    traceEntities = &game->traceEntities[2];
                    if (v34->def->eType == Enums::ET_WORLD)
                        goto LABEL_48;
                }
                v34 = traceEntities[1];
                ++v36;
                v41 = v34->def->eType;
                if (v41 != Enums::ET_MONSTER && v41 != Enums::ET_ATTACK_INTERACTIVE)
                {
                    ++traceEntities;
                    if (v34->def->eType == Enums::ET_WORLD)
                        goto LABEL_48;
                LABEL_35:
                    while (1)
                    {
                        v42 = ++v36;
                        if (numTraceEntities == v36)
                            break;
                        v34 = traceEntities[1];
                        v43 = v34->def->eType;
                        if (v43 == Enums::ET_MONSTER || v43 == Enums::ET_ATTACK_INTERACTIVE)
                            goto LABEL_53;
                        if (v34->def->eType == Enums::ET_WORLD)
                            goto LABEL_48;
                        v34 = traceEntities[2];
                        ++v36;
                        v44 = v34->def->eType;
                        if (v44 == Enums::ET_MONSTER || v44 == Enums::ET_ATTACK_INTERACTIVE)
                            goto LABEL_53;
                        if (v34->def->eType == Enums::ET_WORLD)
                            goto LABEL_48;
                        v34 = traceEntities[3];
                        v36 = v42 + 2;
                        v45 = v34->def->eType;
                        if (v45 == Enums::ET_MONSTER || v45 == Enums::ET_ATTACK_INTERACTIVE)
                            goto LABEL_53;
                        if (v34->def->eType == Enums::ET_WORLD)
                            goto LABEL_48;
                        v34 = traceEntities[4];
                        v36 = v42 + 3;
                        v46 = v34->def->eType;
                        if (v46 == Enums::ET_MONSTER || v46 == Enums::ET_ATTACK_INTERACTIVE)
                            goto LABEL_53;
                        traceEntities += 4;
                        if (v34->def->eType == Enums::ET_WORLD)
                            goto LABEL_48;
                    }
                LABEL_49:
                    v47 = 0x4000;
                    goto LABEL_50;
                }
            }
        }
    LABEL_53:
        v34->pain(999, 0);
        v34->died(1, 0);
        game = app->game;
        v47 = game->traceFracs[v36];
        goto LABEL_50;
    }
LABEL_9:
    if (app->game->activeMonsters && app->time > this->curIndex)
    {
        PlayerEnt = app->player->getPlayerEnt();
        activeMonsters = app->game->activeMonsters;
        do
        {
            Sprite = activeMonsters->getSprite();
            monster = activeMonsters->monster;
            v26 = Sprite;
            if (monster->frameTime)
            {
                v27 = app->game;
            }
            else
            {
                render = app->render;
                app->game->trace(
                    render->mapSprites[Sprite + render->S_X],
                    render->mapSprites[Sprite + render->S_Y],
                    render->mapSprites[Sprite + render->S_Z],
                    v52,
                    viewY,
                    viewZ,
                    activeMonsters,
                    5295,
                    2);
                v27 = app->game;
                if (PlayerEnt == v27->traceEntity)
                {
                    sound = app->sound;
                    MonsterSound = v27->getMonsterSound(activeMonsters->def->eSubType, Enums::MSOUND_ATTACK1);
                    sound->playSound(MonsterSound, 0, 4, 0);
                    app->render->mapSpriteInfo[v26] = app->render->mapSpriteInfo[v26] & 0xFFFF00FF | 0x4000;
                    activeMonsters->monster->frameTime = app->time + 2000;
                    v49 = app->render;
                    hud = app->hud;
                    hud->damageDir = app->player->calcDamageDir(
                        v52,
                        viewY,
                        app->canvas->viewAngle,
                        v49->mapSprites[v26 + v49->S_X],
                        v49->mapSprites[v26 + v49->S_Y]);
                    app->hud->damageTime = app->time + 300;
                    this->curIndex = app->time + 500;
                    app->hud->damageCount = 1;
                    app->combat->totalDamage = 10;
                    app->player->pain(5, activeMonsters, 1);
                    break;
                }
                monster = activeMonsters->monster;
            }
            activeMonsters = monster->nextOnList;
        } while (activeMonsters != v27->activeMonsters);
    }
    app->canvas->playingState();
}

static bool __OFSUB__(int32_t a , int32_t b) {
    int32_t result = a - b;
    return ((a < 0 && b>0 && result > 0) || (a > 0 && b < 0 && result < 0));
}

bool DrivingGame::handleDrivingEvents(int key, int keyAction)
{
    int v4; // r1
    int mode; // r3
    bool v6; // zf
    struct Canvas* hud; // r2
    int v9; // r3
    bool v10; // nf
    int v11; // r3

    Applet* app = CAppContainer::getInstance()->app;

    if (this->fireCount || app->gameTime - this->fireTime <= 300)
    {
        mode = this->mode;
        v4 = 0;
        if (!mode)
            goto LABEL_6;
    LABEL_13:
        if (mode == 1)
        {
            if (keyAction == 3)
            {
                this->fireAngle = -1;
                goto LABEL_15;
            }
            if (keyAction == 4)
                goto LABEL_29;
        }
        else
        {
            if (mode != 2)
                goto LABEL_15;
            if (keyAction == 3)
            {
                v11 = this->fireAngle - 1;
                v10 = this->fireAngle < 0;
                this->fireAngle = v11;
                if (v10 != __OFSUB__(v11, -1))
                    this->fireAngle = -1;
                goto LABEL_15;
            }
            if (keyAction == 4)
            {
                v9 = this->fireAngle + 1;
                v10 = this->fireAngle < 0;
                this->fireAngle = v9;
                if (!(v10 ^ __OFSUB__(v9, 1) | (v9 == 1)))
                    this->fireAngle = 1;
                goto LABEL_15;
            }
        }
        if (keyAction != 1)
        {
        LABEL_9:
            if (keyAction == 6 && v4)
                goto LABEL_11;
            goto LABEL_15;
        }
        mode = 0;
    LABEL_29:
        this->fireAngle = mode;
        goto LABEL_15;
    }
    v4 = 1;
    mode = this->mode;
    if (mode)
        goto LABEL_13;
LABEL_6:
    if (keyAction == 3)
    {
        this->fireAngle = mode - 1;
        if (v4)
            this->fireCount = 3;
        goto LABEL_15;
    }
    if (keyAction == 4)
    {
        this->fireAngle = 1;
        if (v4)
            this->fireCount = 3;
        goto LABEL_15;
    }
    if (keyAction != 1)
        goto LABEL_9;
    this->fireAngle = mode;
    if (v4)
        LABEL_11:
    this->fireCount = 3;
LABEL_15:
    v6 = keyAction == 15;
    if (keyAction != 15)
        v6 = keyAction == 5;
    if (v6)
    {
        app->hud->msgCount = 0;
        app->menuSystem->setMenu(Menus::MENU_INGAME);
    }
    return true;
}
