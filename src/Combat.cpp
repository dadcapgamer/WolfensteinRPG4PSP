#include <stdexcept>
#include <cstring>
#include <climits>

#include "CAppContainer.h"
#include "App.h"
#include "GLES.h"
#include "Combat.h"
#include "CombatEntity.h"
#include "Entity.h"
#include "EntityMonster.h"
#include "EntityDef.h"
#include "Player.h"
#include "Game.h"
#include "Canvas.h"
#include "Render.h"
#include "Hud.h"
#include "Text.h"
#include "Enums.h"
#include "ParticleSystem.h"
#include "DrivingGame.h"
#include "Sound.h"
#include "MenuStrings.h"

Combat::Combat() {
	memset(this, 0, sizeof(Combat));
}

Combat::~Combat() {
}

short Combat::getWeaponWeakness(int weapon, int n2, int n3) {
    return (short)((this->monsterWeakness[(n2 * 3 + n3) * 9 + weapon / 2] >> ((weapon & 0x1) << 2) & 0xF) + 1 << 5);
}

bool Combat::startup() {
	Applet* app = CAppContainer::getInstance()->app;
	//printf("Combat::startup\n");

	for (int n = 0, i = 0; i < 45; ++i, n += 6) {
		this->monsters[i] = new CombatEntity(5 * (this->monsterStats[n + 0] & 0xFF), this->monsterStats[n + 1], this->monsterStats[n + 2], this->monsterStats[n + 3], this->monsterStats[n + 4], this->monsterStats[n + 5]);
	}

	for (int j = 0; j < 16; j++) {
		this->tileDistances[j] = 64 * (j + 1) * (64 * (j + 1));
	}

	this->gotCrit = false;
	this->gotHit = false;
	this->settingDynamite = false;
	this->oneShotCheat = false;

	return true;
}

void Combat::performAttack(Entity* curAttacker, Entity* curTarget, int attackX, int attackY, bool b) {
    Applet* app = CAppContainer::getInstance()->app;

    this->attackX = attackX;
    this->attackY = attackY;
    this->curAttacker = curAttacker;
    this->curTarget = curTarget;
    this->accumRoundDamage = 0;
    this->field_0x110_ = -1;
    if (curAttacker == nullptr || curTarget == nullptr) {
        app->player->updateStats();
    }
    if (curAttacker != nullptr) {
        curAttacker->info |= Entity::ENTITY_FLAG_DROPPED;
    }
    if (curTarget != nullptr) {
        curTarget->info |= Entity::ENTITY_FLAG_DROPPED;
    }
    if (curTarget == nullptr || (curAttacker == nullptr && curTarget->def->eType == Enums::ET_MONSTER)) {
        app->player->lastCombatTurn = app->player->totalMoves;
        app->player->inCombat = true;
    }
    if (curTarget != nullptr) {
        this->targetType = curTarget->def->eType;
        this->targetSubType = curTarget->def->eSubType;
        this->targetMonster = curTarget->monster;
    }
    else {
        this->targetType = (this->targetSubType = 0);
        this->targetMonster = nullptr;
    }
    if (this->curAttacker == nullptr) {
        this->attackerWeaponId = app->player->ce->weapon;

        if (Entity::CheckWeaponMask(this->attackerWeaponId, Enums::WP_PUNCH_MASK)) {
            // Preserve hand alternation even when the target only accepts one
            // hit (doors, props, corpses, and similar interactives).
            this->punchStartsLeft = this->nextPunchStartsLeft;
            this->nextPunchStartsLeft = !this->nextPunchStartsLeft;
            if (!(curTarget == nullptr || curTarget->monster == nullptr)) {
                this->crFlags = 0;
                if ((app->player->ce->calcHit(app->player->ce, &curTarget->monster->ce, false, curTarget->distFrom(app->canvas->viewX, app->canvas->viewY), b) & Enums::CR_HITMASK) != 0 || !curTarget->hasHead()) {
                    if (curTarget->def->eSubType != 10 && curTarget->def->eSubType != 14 && curTarget->def->eSubType != 9) {
                        int sprite = curTarget->getSprite();
                        app->render->mapSpriteInfo[sprite] = (app->render->mapSpriteInfo[sprite] & ~0xFF00) | 0x8000;
                        app->render->chatZoom = true;
                    }
                    this->punchMissed = false;
                }
                else {
                    this->punchMissed = true;
                }
            }
            this->punchingMonster = 1;
        }

        this->attackerMonster = nullptr;
    }
    else {
        this->attackerMonster = this->curAttacker->monster;
        this->attackerWeaponId = this->attackerMonster->ce.weapon;

        if (this->attackerWeaponId == Enums::WP_M_ZOMBIE_SHIELD || this->attackerWeaponId == Enums::WP_M_KICK || this->attackerWeaponId == Enums::WP_M_FLYINGKICK) {
            this->attackFrame = Enums::MANIM_DODGE;
        }
        else if (this->attackerWeaponId == this->getMonsterField(curAttacker->def, 1) || this->attackerWeaponId == Enums::WP_M_TORMENTOR_ABSORB || this->attackerWeaponId == Enums::WP_M_BOSS_HARBINGER_SUCK) {
            this->attackFrame = Enums::MANIM_ATTACK2;
            if (curAttacker->def->eSubType == 2/*MONSTER_WORKER*/ && curAttacker->param == 0) {
                curAttacker->monster->flags |= Enums::MFLAG_WEAPON_ALT;
            }
        }
        else {
            this->attackFrame = Enums::MANIM_ATTACK1;
        }
    }
    this->attackerWeapon = this->attackerWeaponId * Combat::WEAPON_MAX_FIELDS;
    if (this->punchingMonster <= 0 || this->punchMissed) {
        this->stage = 0;
        this->nextStageTime = 0;
    }
    else {
        this->stage = -1;
        this->nextStage = 0;
        this->nextStageTime = app->gameTime + 300;
    }

    this->animEndTime = 0;
    this->animLoopCount = this->weapons[this->attackerWeapon + Combat::WEAPON_FIELD_NUMSHOTS];
    this->attackerWeaponProj = this->weapons[this->attackerWeapon + Combat::WEAPON_FIELD_PROJTYPE];
    if (this->curAttacker != nullptr) {
        this->worldDist = this->curAttacker->distFrom(app->canvas->viewX, app->canvas->viewY);
        this->tileDist = this->WorldDistToTileDist(this->worldDist);
    }
    else {
        this->worldDist = this->curTarget->distFrom(app->canvas->viewX, app->canvas->viewY);
        this->tileDist = this->WorldDistToTileDist(this->worldDist);
    }

    short* mapSprites = app->render->mapSprites;
    if (this->attackerWeaponId == Enums::WP_M_OLARIC_JUMP) {
        int sprite = this->curAttacker->getSprite();
        short sX = mapSprites[app->render->S_X + sprite];
        short sY = mapSprites[app->render->S_Y + sprite];
        LerpSprite* allocLerpSprite = app->game->allocLerpSprite(nullptr, sprite, true);
        allocLerpSprite->startTime = app->gameTime;
        allocLerpSprite->travelTime = WorldDistToTileDist(this->curAttacker->distFrom(app->canvas->viewX, app->canvas->viewY)) * 500;
        allocLerpSprite->flags |= (Enums::LS_FLAG_PARABOLA | Enums::LS_FLAG_TRUNC |Enums::LS_FLAG_ENT_NORELINK);
        allocLerpSprite->srcScale = allocLerpSprite->dstScale = mapSprites[app->render->S_SCALEFACTOR + sprite];
        allocLerpSprite->srcX = mapSprites[app->render->S_X + sprite];
        allocLerpSprite->srcY = mapSprites[app->render->S_Y + sprite];
        allocLerpSprite->srcZ = mapSprites[app->render->S_Z + sprite];
        allocLerpSprite->dstX = app->canvas->viewX;
        allocLerpSprite->dstY = app->canvas->viewY;
        allocLerpSprite->dstZ = app->render->getHeight(allocLerpSprite->dstX, allocLerpSprite->dstY) + 32;
        allocLerpSprite->height = 64;
        app->render->mapSpriteInfo[sprite] = ((app->render->mapSpriteInfo[sprite] & 0xFFFF00FF) | 0x5000);
        this->curAttacker->monster->frameTime = app->time + 130;
        int n4 = sX - app->canvas->viewX;
        int n5 = sY - app->canvas->viewY;
        if (n4 != 0) {
            n4 /= std::abs(n4);
        }
        if (n5 != 0) {
            n5 /= std::abs(n5);
        }
        int n6 = app->canvas->viewX + n4 * 64;
        int n7 = app->canvas->viewY + n5 * 64;
        app->game->unlinkEntity(this->curAttacker);
        app->game->linkEntity(this->curAttacker, n6 >> 6, n7 >> 6);
        this->curAttacker->monster->goalFlags &= ~EntityMonster::GFL_SPECIAL;
        this->stage = -1;
        this->nextStage = 0;
        this->nextStageTime = app->gameTime + allocLerpSprite->travelTime;
    }
    else if (this->attackerWeaponId == Enums::WP_M_ZOMBIE_SHIELD || this->attackerWeaponId == Enums::WP_M_KICK || this->attackerWeaponId == Enums::WP_M_BOSS_HARBINGER_SLAM || this->attackerWeaponId == Enums::WP_M_FLYINGKICK) {
        int sprite = this->curAttacker->getSprite();
        int sX = mapSprites[app->render->S_X + sprite] - app->canvas->viewX;
        int sY = mapSprites[app->render->S_Y + sprite] - app->canvas->viewY;
        int a1 = sX;
        int a2 = sY;
        if (a1 != 0) {
            a1 /= std::abs(a1);
        }
        if (a2 != 0) {
            a2 /= std::abs(a2);
        }
        int n4 = std::abs(sX + sY) >> 6;
        int n5 = app->canvas->viewX + a1 * 64;
        int n6 = app->canvas->viewY + a2 * 64;
        LerpSprite* allocLerpSprite = app->game->allocLerpSprite(nullptr, sprite, true);
        allocLerpSprite->startTime = app->gameTime;
        allocLerpSprite->travelTime = n4 * 200;
        allocLerpSprite->flags |= Enums::LS_FLAG_ENT_NORELINK;
        allocLerpSprite->srcScale = allocLerpSprite->dstScale = app->render->mapSprites[app->render->S_SCALEFACTOR + sprite];
        allocLerpSprite->srcX = mapSprites[app->render->S_X + sprite];
        allocLerpSprite->srcY = mapSprites[app->render->S_Y + sprite];
        allocLerpSprite->srcZ = mapSprites[app->render->S_Z + sprite];
        allocLerpSprite->dstX = n5 - a1 * 28;
        allocLerpSprite->dstY = n6 - a2 * 28;
        allocLerpSprite->dstZ = app->render->getHeight(allocLerpSprite->dstX, allocLerpSprite->dstY) + 32;

        if (this->attackerWeaponId == Enums::WP_M_FLYINGKICK || curAttacker->def->eSubType == 10) {
            allocLerpSprite->flags |= Enums::LS_FLAG_PARABOLA;
            allocLerpSprite->height = this->tileDist * 6;
        }

        app->render->mapSpriteInfo[sprite] = ((app->render->mapSpriteInfo[sprite] & 0xFFFF00FF) | this->attackFrame << 8);
        this->curAttacker->monster->frameTime = INT_MAX;
        app->game->unlinkEntity(this->curAttacker);
        app->game->linkEntity(this->curAttacker, n5 >> 6, n6 >> 6);
        this->stage = -1;
        this->nextStage = 0;
        this->nextStageTime = app->gameTime + allocLerpSprite->travelTime;
        if (this->attackerWeaponId == Enums::WP_M_ZOMBIE_SHIELD) {
            app->sound->playSound(1159, 0, 5, 0);
        }
    }
    app->canvas->setState(Canvas::ST_COMBAT);
}

void Combat::checkMonsterFX() {
    Applet* app = CAppContainer::getInstance()->app;
    EntityMonster* monster = this->curTarget->monster;
    short monsterEffects = monster->monsterEffects;
    bool b = this->targetSubType == Enums::MONSTER_TROOPER || (this->targetSubType == Enums::MONSTER_SKELETON && this->curTarget->def->parm > 0);
    if (!b && Entity::CheckWeaponMask(this->attackerWeaponId, Enums::WP_FIREMASK)) {  // if (!b && (n & 0x2000L) != 0x0L)
        monsterEffects = (short)((short)(monsterEffects | ((EntityMonster::MFX_TURN_MASK << EntityMonster::MFX_FIRE_SHIFT) | EntityMonster::MFX_FIRE)) & EntityMonster::MFX_FREEZE_REMOVE);
    }
    monster->monsterEffects = monsterEffects;
}

int Combat::playerSeq() {
    Applet* app = CAppContainer::getInstance()->app;
    int animLoopCount = 0;
    bool b = false;
    if (this->nextStageTime != 0 && app->gameTime > this->nextStageTime && this->numActiveMissiles == 0 && app->game->animatingEffects == 0) {
        this->stage = this->nextStage;
        this->nextStageTime = 0;
        this->nextStage = -1;
        this->field_0x110_ = -1;
    }
    if (this->stage == 0) {
        this->isGibbed = false;
        this->totalDamage = 0;
        this->totalArmorDamage = 0;
        this->hitType = 0;
        this->deathAmt = 0;
        this->gotCrit = false;
        this->gotHit = false;
        this->crFlags = 0;
        this->damage = 0;
        this->targetKilled = false;
        this->flashTime = 1; // Old -> 0

        if (this->attackerWeaponId != Enums::WP_DYNAMITE) {
            ++app->player->counters[6];
        }
        if (this->punchingMonster == Combat::PUNCH_PREP) {
            this->punchingMonster = this->punchStartsLeft
                ? Combat::PUNCH_LEFTHAND
                : Combat::PUNCH_RIGHTHAND;
        }
        else if (this->punchingMonster == Combat::PUNCH_RIGHTHAND) {
            this->punchingMonster = Combat::PUNCH_LEFTHAND;
        }
        else if (this->punchingMonster == Combat::PUNCH_LEFTHAND) {
            this->punchingMonster = Combat::PUNCH_RIGHTHAND;
        }
        if (this->attackerWeaponId == Enums::WP_BOOT) {
            this->flashTime = 200;
        }
        
        if (!Entity::CheckWeaponMask(this->attackerWeaponId, Enums::WP_NODISTANCECHK)) {
            this->crFlags |= Enums::CR_REQUIRERANGE;
        }
        if (this->weapons[this->attackerWeapon + Combat::WEAPON_FIELD_AMMOTYPE] != 0) {
            b = true;
            animLoopCount = app->player->ammo[this->weapons[this->attackerWeapon + Combat::WEAPON_FIELD_AMMOTYPE]];
        }
        if (b && this->targetType != Enums::ET_MONSTER && this->attackerWeaponId != Enums::WP_ITEM) {
            if (animLoopCount == 2) {
                app->hud->addMessage(CodeStrings::ONE_SHOT_LEFT);
            }
            else if (animLoopCount < 5 && animLoopCount > 1) {
                app->localization->resetTextArgs();
                app->localization->addTextArg(animLoopCount - 1);
                app->hud->addMessage(CodeStrings::X_SHOTS_LEFT);
            }
        }
        if (this->targetType == Enums::ET_MONSTER) {
            if ((this->attackerWeaponId == Enums::WP_PUNCH || this->attackerWeaponId == Enums::WP_BRASS_PUNCH) && this->targetSubType == Enums::MONSTER_SKELETON) {
                this->crFlags |= Enums::CR_CRIT_DAMAGE;
            }
            if (!Entity::CheckWeaponMask(this->attackerWeaponId, Enums::WP_MELEEMASK)) {
                this->crFlags |= Enums::CR_NOSTRBONUS;
            }
            app->player->ce->calcCombat(app->player->ce, this->curTarget, false, this->worldDist, this->targetSubType);
            if (this->crFlags & Enums::CR_HITMASK) {
                this->curTarget->info |= Entity::ENTITY_FLAG_HURT;
                if (this->crFlags & Enums::CR_BOMB_STUCK) {
                    int nextBombIndex = app->game->getNextBombIndex();
                    int sprite = this->curTarget->getSprite();
                    app->game->allocDynamite(app->render->mapSprites[app->render->S_X + sprite], app->render->mapSprites[app->render->S_Y + sprite], app->render->mapSprites[app->render->S_Z + sprite] + 25, 1048576, nextBombIndex, this->curTarget->getIndex());
                }
                if (this->crFlags & Enums::CR_CRIT) {
                    this->gotCrit = true;
                    this->hitType = 2;
                }
                else if (this->crFlags & Enums::CR_HIT) {
                    this->hitType = 1;
                }
                this->gotHit = true;
                this->damage = this->crDamage;
                this->deathAmt = this->targetMonster->ce.getStat(Enums::STAT_HEALTH) - this->damage;
                if (this->damage > 0 && this->gotHit) {
                    app->player->counters[2] += this->damage;
                    if (this->damage > app->player->counters[1]) {
                        app->player->counters[1] = this->damage;
                    }
                }

                if (this->targetSubType == Enums::BOSS_HARBINGER && this->attackerWeaponId == Enums::WP_TESLA) {
                    this->curTarget->monster->resetGoal();
                    this->curTarget->monster->goalType = EntityMonster::GOAL_STUN;
                    this->curTarget->monster->goalTurns = 0;
                    this->curTarget->monster->goalParam = 1;
                }
            }
        }
        else if (this->attackerWeaponId == Enums::WP_DYNAMITE && this->targetType == Enums::ET_ATTACK_INTERACTIVE && (this->targetSubType == Enums::INTERACT_BARREL || this->targetSubType == Enums::INTERACT_CHICKEN)) {
            this->crFlags |= Enums::CR_BOMB_STUCK;
            int nextBombIndex = app->game->getNextBombIndex();
            int sprite = this->curTarget->getSprite();
            app->game->allocDynamite(app->render->mapSprites[app->render->S_X + sprite], app->render->mapSprites[app->render->S_Y + sprite], app->render->mapSprites[app->render->S_Z + sprite] + 25, 1048576, nextBombIndex, this->curTarget->getIndex());
        }
        else {
            this->hitType = calcHit(this->curTarget);
            int damage = this->weapons[this->attackerWeapon + Combat::WEAPON_FIELD_STRMIN];
            int damageMax = this->weapons[this->attackerWeapon + Combat::WEAPON_FIELD_STRMAX];
            if (damage != damageMax) {
                damage += app->nextByte() % (damageMax - damage);
            }
            if (app->game->difficulty == 4) {
                damage -= damage >> 2;
            }
            this->damage = damage;
        }

        int n3 = -1;
        int v24 = Player::WeaponSounds[this->attackerWeaponId];

        if (Entity::CheckWeaponMask(this->attackerWeaponId, Enums::WP_PUNCH_MASK) && this->hitType == 0) {
            if (this->curTarget->def->eType == Enums::ET_ATTACK_INTERACTIVE && this->curTarget->def->eSubType == Enums::INTERACT_PAINTING) {
                n3 = v24 & 0xffff;
            }
            else if (app->game->isUnderWater()) {
                n3 = 1148;
            }
            else {
                n3 = 1138;
            }
        }
        else if (app->game->isUnderWater() && this->attackerWeaponId != Enums::WP_DYNAMITE) {
            n3 = (this->hitType == 0) ? 1148 : 1147;
        }
        else if (this->attackerWeaponId == Enums::WP_BOOT && (this->targetType == Enums::ET_ATTACK_INTERACTIVE && this->targetSubType == Enums::INTERACT_CHICKEN)) {
            n3 = 1008;
        }
        else if (Entity::CheckWeaponMask(this->attackerWeaponId, Enums::WP_PUNCH_MASK) != 0 && (this->curTarget->def->eType == Enums::ET_ENV_DAMAGE && this->curTarget->def->eSubType == 10)) {
            n3 = 255;
        }
        else if ((this->attackerWeaponId == Enums::WP_DESTINY_SPEAR) && (this->hitType == 0)) {
            n3 = 1138;
        }
        else {
            n3 = v24 & 0xffff;
        }
 
        if (n3 != -1) {
            app->sound->playCombatSound(n3, 0, 4);
        }

        this->totalDamage += this->damage;
        this->totalArmorDamage += this->crArmorDamage;
        this->animTime = this->weapons[this->attackerWeapon + Combat::WEAPON_FIELD_SHOTHOLD];
        if (app->player->statusEffects[(Enums::OFS_STATUSEFFECT_TURNS + Enums::STATUS_EFFECT_AGILITY)] > 0) {
            this->animTime *= 5;
        }
        else {
            this->animTime *= 10;
        }
        this->animStartTime = app->gameTime;
        this->animEndTime = this->animStartTime + this->animTime;
        this->flashDone = false;
        this->flashDoneTime = this->animStartTime + this->flashTime;
        if (b && animLoopCount < this->animLoopCount) {
            this->animLoopCount = animLoopCount;
        }
        this->launchProjectile();
        if (!Entity::CheckWeaponMask(this->attackerWeaponId, Enums::WP_NORECOIL) || (this->attackerWeaponId == Enums::WP_FG42 && this->animLoopCount == 1)) {
            int n4 = 2;
            if (this->attackerWeaponId == Enums::WP_PANZER || this->attackerWeaponId == Enums::WP_VENOM) {
                n4 = 6;
            }
            app->render->rockView(this->animTime, app->canvas->viewX - n4 * (app->canvas->viewStepX >> 6), app->canvas->viewY - n4 * (app->canvas->viewStepY >> 6), app->canvas->viewZ);
        }

        int sprite = this->curTarget->getSprite();
        if (this->totalDamage == 0) {
            if (this->attackerWeaponId != Enums::WP_DYNAMITE && this->targetType == Enums::ET_MONSTER) {
                if (this->targetType == Enums::ET_MONSTER) {
                    bool isDodge = (this->crFlags & Enums::CR_DODGE) != 0x0;
                    if (Entity::CheckWeaponMask(this->attackerWeaponId, Enums::WP_PUNCH_MASK) /* || this->curTarget->monster->goalType == 4*/) { // GOAL_FLEE 
                        isDodge = false;
                    }
                    if (isDodge && (this->targetSubType == Enums::MONSTER_ZOMBIE || this->targetSubType == Enums::MONSTER_ELITE_GUARD)) {
                        app->render->mapSpriteInfo[sprite] = ((app->render->mapSpriteInfo[sprite] & 0xFFFF00FF) | 0x9000);
                        this->curTarget->monster->frameTime = app->time + ((this->attackerWeaponId == Enums::WP_FLAMETHROWER || this->attackerWeaponId == Enums::WP_PANZER) ? 750 : 500);
                    }

                    if (this->attackerWeaponId == Enums::WP_TESLA && this->curTarget->monster->goalType == EntityMonster::GOAL_STUN) {
                        app->hud->addMessage(CodeStrings::MONSTER_STUNNED, 1);
                    }
                    else if (this->crFlags & Enums::CR_IMMUNE) {
                        app->hud->addMessage(CodeStrings::DETAILS, 1);
                    }
                    else if (this->crFlags & Enums::CR_OUTOFRANGE) {
                        app->hud->addMessage(CodeStrings::OUT_OF_RANGE);
                    }
                    else if (!(this->crFlags & Enums::CR_HIT)) {
                        if (isDodge && !(this->curTarget->monster->monsterEffects & EntityMonster::MFX_FREEZE)) {
                            app->localization->resetTextArgs();
                            Text* smallBuffer = app->localization->getSmallBuffer();
                            app->localization->composeTextField(this->curTarget->name, smallBuffer);
                            app->localization->addTextArg(smallBuffer);
                            if (this->curTarget->def->eSubType == 8) {
                                app->hud->addMessage(CodeStrings::X_BLOCKED);
                            }
                            else {
                                app->hud->addMessage(CodeStrings::X_DODGED);
                            }
                            smallBuffer->dispose();
                        }
                        else {
                            app->hud->addMessage(CodeStrings::MISSED);
                        }
                    }
                }
            }
        }
        else if (this->targetType == Enums::ET_MONSTER) {
            this->accumRoundDamage += this->totalDamage;
        }
        else if (this->targetType == Enums::ET_ATTACK_INTERACTIVE && (this->targetSubType == Enums::INTERACT_BARREL || this->targetSubType == Enums::INTERACT_CHICKEN)) {
            this->targetKilled = false;
        }
        else if (this->targetType == Enums::ET_ATTACK_INTERACTIVE && this->targetSubType == Enums::INTERACT_PAINTING && (Entity::CheckWeaponMask(this->attackerWeaponId, Enums::WP_PUNCH_MASK) || this->curTarget->param < 2)) {
            this->targetKilled = false;
            if (Entity::CheckWeaponMask(this->attackerWeaponId, Enums::WP_PUNCH_MASK) && !this->punchMissed) {
                app->render->mapSpriteInfo[sprite] ^= 0x20000;
                app->render->mapSpriteInfo[sprite] = ((app->render->mapSpriteInfo[sprite] & 0xFFFF00FF) | 0x100);
            }
            else if (!Entity::CheckWeaponMask(this->attackerWeaponId, Enums::WP_PAINTINGMASK) && app->player->ce->weapon != Enums::WP_DYNAMITE) {
                this->targetKilled = true;
            }
        }
        else if (this->hitType != 0) {
            this->targetKilled = true;
        }

        if (this->attackerWeaponId == Enums::WP_DYNAMITE && !(this->crFlags & Enums::CR_BOMB_STUCK)) {
            int dist = this->curTarget->distFrom(app->canvas->viewX, app->canvas->viewY);
            Entity* curTarget2 = this->curTarget;
            if (WorldDistToTileDist(dist) < 1 && (1 << this->targetType & Enums::CONTENTS_DYNAMITE_SOLID)) {
                if (this->targetType == Enums::ET_WORLD) {
                    this->currentBombIndex = app->game->setDynamite(app->game->traceCollisionX, app->game->traceCollisionY, false);
                }
                else {
                    int* calcPosition = this->curTarget->calcPosition();
                    this->currentBombIndex = app->game->setDynamite(calcPosition[0], calcPosition[1], false);
                }
            }
            else {
                this->settingDynamite = true;
                this->dynamitePlaced = false;
                this->settingDynamiteTime = app->gameTime;
                int n6 = app->canvas->destX + app->canvas->viewStepX;
                int n7 = app->canvas->destY + app->canvas->viewStepY;
                this->currentBombIndex = app->game->setDynamite(n6, n7, true);
                this->animEndTime = INT32_MAX;
                this->placingBombZ = 15;
                int n8 = app->render->getHeight(n6, n7) - app->render->getHeight(app->canvas->destX, app->canvas->destY);
                if (n8 > 0) {
                    this->placingBombZ += std::min(n8, 21);
                }
                else {
                    this->placingBombZ -= std::max(-16, n8) / 4;
                }
            }
        }

        this->stage = -1;
        app->hud->repaintFlags |= 0x4;
        int ammoUsage = this->weapons[this->attackerWeapon + Combat::WEAPON_FIELD_AMMOUSAGE];
        int ammoType = this->weapons[this->attackerWeapon + Combat::WEAPON_FIELD_AMMOTYPE];
        app->player->ammo[ammoType] -= ammoUsage;
        app->player->ammo[ammoType] = (short)std::max((int)app->player->ammo[ammoType], 0);
        this->nextStage = 1;
        this->nextStageTime = this->animEndTime;
        this->updateProjectile();
        app->canvas->invalidateRect();
        if (this->totalDamage == 0 || this->hitType == 0) {
            ++app->player->counters[7];
        }
    }
    else {
        if (this->stage == 1) {
            this->settingDynamite = false;
            this->dynamitePlaced = false;
            app->canvas->invalidateRect();
            if (this->targetType == Enums::ET_MONSTER) {
                this->curTarget->info &= ~Entity::ENTITY_FLAG_HURT;
            }
            if (this->targetType == Enums::ET_ATTACK_INTERACTIVE) {
                int sprite = this->curTarget->getSprite();
                if (this->targetSubType == Enums::INTERACT_SINK) {
                    app->render->mapSpriteInfo[sprite] &= 0xFFF700FF;
                }
                else if (this->targetSubType == Enums::INTERACT_PAINTING && this->animLoopCount == 1) {
                    app->render->mapSpriteInfo[sprite] &= 0xFFFF00FF;
                }
            }
            if (this->targetType == Enums::ET_CORPSE && this->targetSubType == Enums::CORPSE_SKELETON) {
                int sprite = this->curTarget->getSprite();
                app->game->executeTile(app->render->mapSprites[app->render->S_X + sprite] >> 6, app->render->mapSprites[app->render->S_Y + sprite] >> 6, 0xFF4 | app->canvas->flagForWeapon(this->attackerWeaponId), true);
            }

            int n6 = 4385;
            if (this->targetKilled || (this->targetType == Enums::ET_MONSTER && this->targetMonster->ce.getStat(Enums::STAT_HEALTH) <= 0)) {
                this->curTarget->died(true, app->player->getPlayerEnt());
                this->targetKilled = true;
            }
            else if (--this->animLoopCount > 0 && (1 << this->targetType & n6) == 0x0 && !this->punchMissed && (this->targetType != Enums::ET_ATTACK_INTERACTIVE || this->targetSubType == Enums::INTERACT_PAINTING || this->targetSubType == Enums::INTERACT_SINK)) {
                this->stage = 0;
                this->animEndTime = (this->animTime = 0);
                this->nextStageTime = 0;
                return 1;
            }
            if (this->punchingMonster > 0 && !this->targetKilled && !this->punchMissed) {
                int sprite = this->curTarget->getSprite();
                if (sprite != -1 && this->curTarget->monster != nullptr) {
                    app->render->mapSpriteInfo[sprite] = ((app->render->mapSpriteInfo[sprite] & 0xFFFF00FF) | 0x0);
                }
            }
            app->render->chatZoom = false;
            this->punchingMonster = 0;
            this->punchMissed = false;
            if (this->targetType == Enums::ET_MONSTER && this->accumRoundDamage != 0 && (this->curTarget->info & Entity::ENTITY_FLAG_TAKEDAMAGE) != 0x0) {
                Text* messageBuffer = app->hud->getMessageBuffer();
                app->localization->resetTextArgs();
                if (this->gotCrit) {
                    app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::CRIT, messageBuffer);
                }
                app->localization->addTextArg(this->accumRoundDamage);
                app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::X_DAMAGE, messageBuffer);
                app->hud->finishMessageBuffer();
            }
            if (this->attackerWeaponId == Enums::WP_ITEM) {
                app->player->weapons &= ~0x10000;
                if ((app->player->weapons & 0x1LL) != 0x0LL) {
                    app->player->selectWeapon(0);
                }
                else if ((app->player->weapons & 0x2LL) != 0x0LL) {
                    app->player->selectWeapon(1);
                }
                else if ((app->player->weapons & 0x4LL) != 0x0LL) {
                    app->player->selectWeapon(2);
                }
            }
            if (this->attackerWeaponId == Enums::WP_DYNAMITE && app->player->ammo[7] == 0) {
                this->shiftWeapon(true);
            }

            return 0;
        }
        if (this->stage == -1) {
            app->canvas->staleView = true;
            if (this->numActiveMissiles != 0) {
                this->updateProjectile();
            }
        }
    }
    return 1;
}

int Combat::monsterSeq() {
    Applet* app = CAppContainer::getInstance()->app;
    int sprite = this->curAttacker->getSprite();
    int n = app->render->mapSpriteInfo[sprite];
    int n2 = (n & 0xFF00) >> 8;
    int n3 = n2 & 0xF0;
    int n4 = n2 & 0xF;

    if ((n3 == 64 || n3 == 80) && n4 == 0) {
        if (app->time >= this->curAttacker->monster->frameTime) {
            this->curAttacker->monster->frameTime = app->time + this->weapons[this->attackerWeapon + Combat::WEAPON_FIELD_SHOTHOLD] * 10;
            this->nextStageTime = app->gameTime + this->weapons[this->attackerWeapon + Combat::WEAPON_FIELD_SHOTHOLD] * 10;
            app->render->mapSpriteInfo[sprite] = ((n & 0xFFFF00FF) | (n3 | n4 + 1) << 8);
            if (this->curAttacker->def->eSubType == 14 && this->attackerWeaponId == Enums::WP_M_BOSS_HARBINGER_CLAW) {
                app->sound->playSound(1043, 0, 4, 0);
            }
        }
    }
    else if (this->nextStageTime != 0 && app->gameTime > this->nextStageTime && this->numActiveMissiles == 0 && app->game->animatingEffects == 0) {
        if (this->reflectionDmg > 0 && this->curAttacker != nullptr && this->curAttacker->monster != nullptr) {
            app->particleSystem->spawnMonsterBlood(this->curAttacker, false);
            this->curAttacker->pain(this->reflectionDmg / 2, app->player->getPlayerEnt());
            this->reflectionDmg = 0;
            if (this->curAttacker->monster->ce.getStat(Enums::STAT_HEALTH) <= 0) {
                this->curAttacker->died(true, app->player->getPlayerEnt());
            }
            else {
                this->curAttacker->monster->frameTime = app->time + 250;
                this->nextStageTime = app->gameTime + 250;
            }
        }
        else {
            this->stage = this->nextStage;
            this->nextStageTime = 0;
            this->nextStage = -1;
        }
        this->field_0x110_ = -1;
    }
    if (this->stage == 0) {
        this->totalDamage = 0;
        this->totalArmorDamage = 0;
        this->hitType = 0;
        this->crFlags = 0;
        this->gotCrit = false;
        this->gotHit = false;
        this->targetKilled = false;
        this->reflectionDmg = 0;

        int soundType;
        if (this->attackerWeaponId == this->getMonsterField(this->curAttacker->def, 1)) {
            soundType = Enums::MSOUND_ATTACK2;
        }
        else {
            soundType = Enums::MSOUND_ATTACK1;
        }

        int monsterSound;
        if (this->attackerWeaponId == Enums::WP_M_BOSS_HARBINGER_SUCK || this->attackerWeaponId == Enums::WP_M_TORMENTOR_ABSORB) {
            monsterSound = 1048;
        }
        else if(this->attackerWeaponId == Enums::WP_M_BOSS_HARBINGER_FIRE) {
            monsterSound = 1045;
        }
        else if (this->attackerWeaponId == Enums::WP_M_FG42) {
            monsterSound = 1128;
        }
        else {
            monsterSound = app->game->getMonsterSound(this->curAttacker->def->eSubType, soundType);
        }

        app->sound->playCombatSound(monsterSound, 0, 4);

        if (this->weapons[this->attackerWeapon + Combat::WEAPON_FIELD_STRMAX] > 0) {
            if (this->curTarget == nullptr) {
                app->player->ce->calcCombat(&this->attackerMonster->ce, app->player->getPlayerEnt(), true, app->player->distFrom(this->curAttacker), -1);
                if (this->crFlags & Enums::CR_HITMASK) {
                    if (this->crFlags & Enums::CR_CRIT) {
                        this->gotCrit = true;
                        this->hitType = 2;
                    }
                    else {
                        this->hitType = 1;
                    }
                    this->gotHit = true;
                    this->damage = this->crDamage;
                }
                else {
                    this->damage = 0;
                }
            }
            else {
                this->hitType = 1;
                this->damage = 1;
            }
            this->totalDamage += this->damage;
            this->totalArmorDamage += this->crArmorDamage;
        }
        else if (this->attackerWeaponId == Enums::WP_M_TORMENTOR_ABSORB || this->attackerWeaponId == Enums::WP_M_BOSS_HARBINGER_SUCK) {
            this->hitType = 1;
        }

        --this->animLoopCount;
        this->animTime = this->weapons[this->attackerWeapon + Combat::WEAPON_FIELD_SHOTHOLD] * 10;
        this->animStartTime = app->gameTime;
        this->animEndTime = this->animStartTime + this->animTime;
        if (this->attackerWeaponId != Enums::WP_M_ZOMBIE_SHIELD) {
            app->render->mapSpriteInfo[sprite] = ((app->render->mapSpriteInfo[sprite] & 0xFFFF00FF) | this->attackFrame << 8);
            this->curAttacker->monster->frameTime = app->time + this->animTime;

            if (Entity::CheckWeaponMask(this->attackerWeaponId, Enums::WP_INSTANT_ATTACK) && !Entity::CheckWeaponMask(this->attackerWeaponId, Enums::WP_INSTANT_ATTACK_NO_FRAME)) {
                app->render->mapSpriteInfo[sprite] |= 0x100;
            }
        }
        if (this->curTarget != nullptr) {
            if (this->targetType == Enums::ET_ATTACK_INTERACTIVE && this->targetSubType == Enums::INTERACT_BARREL) {
                this->targetKilled = false;
            }
            else {
                this->targetKilled = true;
            }
        }
        if (this->attackerWeaponId == Enums::WP_M_OLARIC_JUMP) {
            LerpSprite* lerpSprite = app->game->allocLerpSprite(nullptr, sprite, true);
            lerpSprite->startTime = app->gameTime;
            lerpSprite->travelTime = 500;
            lerpSprite->flags |= (Enums::LS_FLAG_ENT_NORELINK | Enums::LS_FLAG_PARABOLA);
            lerpSprite->srcScale = lerpSprite->dstScale = app->render->mapSprites[app->render->S_SCALEFACTOR + sprite];
            lerpSprite->srcX = app->render->mapSprites[app->render->S_X + sprite];
            lerpSprite->srcY = app->render->mapSprites[app->render->S_Y + sprite];
            lerpSprite->srcZ = app->render->mapSprites[app->render->S_Z + sprite];
            lerpSprite->dstX = (this->curAttacker->linkIndex % 32 << 6) + 32;
            lerpSprite->dstY = (this->curAttacker->linkIndex / 32 << 6) + 32;
            lerpSprite->dstZ = app->render->getHeight(lerpSprite->dstX, lerpSprite->dstY) + 32;
            lerpSprite->height = 16;
            app->render->mapSpriteInfo[sprite] = ((app->render->mapSpriteInfo[sprite] & 0xFFFF00FF) | 0x0);
            this->curAttacker->monster->frameTime = app->time + lerpSprite->travelTime;
        }
        else if (this->attackerWeaponId == Enums::WP_M_ZOMBIE_SHIELD || this->attackerWeaponId == Enums::WP_M_KICK || this->attackerWeaponId == Enums::WP_M_BOSS_HARBINGER_SLAM || this->attackerWeaponId == Enums::WP_M_FLYINGKICK) {
            LerpSprite* lerpSprite = app->game->allocLerpSprite(nullptr, sprite, true);
            lerpSprite->startTime = app->gameTime;
            lerpSprite->travelTime = 500;
            lerpSprite->flags |= Enums::LS_FLAG_ENT_NORELINK;
            lerpSprite->srcScale = lerpSprite->dstScale = app->render->mapSprites[app->render->S_SCALEFACTOR + sprite];
            lerpSprite->srcX = app->render->mapSprites[app->render->S_X + sprite];
            lerpSprite->srcY = app->render->mapSprites[app->render->S_Y + sprite];
            lerpSprite->srcZ = app->render->mapSprites[app->render->S_Z + sprite];
            lerpSprite->dstX = (this->curAttacker->linkIndex % 32 << 6) + 32;
            lerpSprite->dstY = (this->curAttacker->linkIndex / 32 << 6) + 32;
            lerpSprite->dstZ = app->render->getHeight(lerpSprite->dstX, lerpSprite->dstY) + 32;
            lerpSprite->calcDist();
            if (this->curAttacker->def->eSubType == 10) {
                lerpSprite->travelTime = 200;
                app->render->mapSpriteInfo[sprite] = ((app->render->mapSpriteInfo[sprite] & 0xFFFF00FF) | 0x4000);
            }
            else {
                app->render->mapSpriteInfo[sprite] = ((app->render->mapSpriteInfo[sprite] & 0xFFFF00FF) | 0x0);
            }
            this->curAttacker->monster->frameTime = app->time + lerpSprite->travelTime;
        }

        this->launchProjectile();
        if (this->gotHit && this->attackerWeaponId == Enums::WP_M_FLAME) {
            app->player->addStatusEffect(Enums::STATUS_EFFECT_FIRE, 5, 3);
            app->player->translateStatusEffects();
        }
        this->stage = -1;
        if (this->animLoopCount <= 0) {
            this->nextStage = 1;
        }
        else {
            this->nextStage = 0;
        }
        this->nextStageTime = this->animEndTime;
        if (app->render->isSkeleton(this->curAttacker->def->tileIndex) && this->attackerWeaponId == Enums::WP_M_CLAW && this->animLoopCount == 0) {
            app->render->mapSpriteInfo[sprite] ^= 0x20000;
        }
        this->updateProjectile();
        if (this->crFlags & Enums::CR_DODGE) {
            app->hud->addMessage(CodeStrings::DODGED);
        }
        app->canvas->invalidateRect();
    }
    else {
        if (this->stage == 1) {
            if (this->curAttacker->def->eType == Enums::ET_MONSTER) {
                app->render->mapSpriteInfo[sprite] = ((app->render->mapSpriteInfo[sprite] & 0xFFFF00FF) | 0x0);
            }
            if (this->targetKilled) {
                this->curTarget->died(false, nullptr);
            }
            app->localization->resetTextArgs();
            if (this->accumRoundDamage > 0 && app->player->buffs[(Enums::OFS_BUFF_COUNT + Enums::BUFF_REFLECT)] > 0 && !this->curAttacker->isBoss()) {
                app->localization->addTextArg(this->accumRoundDamage);
                app->hud->addMessage(Strings::FILE_CODESTRINGS, CodeStrings::REFLECTED_X);
            }
            else if (this->accumRoundDamage > 0) {
                app->localization->addTextArg(Strings::FILE_ENTITYSTRINGS, (short)(this->curAttacker->def->name & 0x3FF));
                app->localization->addTextArg(this->accumRoundDamage);
                app->hud->addMessage(Strings::FILE_CODESTRINGS, CodeStrings::X_DID_Y_DAMAGE);
            }
            else if (this->attackerWeaponId == Enums::WP_M_TORMENTOR_ABSORB) {
                app->localization->addTextArg(Strings::FILE_ENTITYSTRINGS, (short)(this->curAttacker->def->name & 0x3FF));
                app->localization->addTextArg(Strings::FILE_ENTITYSTRINGS, (short)(this->curTarget->def->name & 0x3FF));
                app->hud->addMessage(Strings::FILE_CODESTRINGS, CodeStrings::X_CONSUMES_Y);
            }
            else if (this->attackerWeaponId == Enums::WP_M_BOSS_HARBINGER_SUCK) {
                app->localization->addTextArg(Strings::FILE_ENTITYSTRINGS, (short)(this->curAttacker->def->name & 0x3FF));
                app->hud->addMessage(Strings::FILE_CODESTRINGS, CodeStrings::HARBINGER_HEALS);
            }
            app->canvas->shakeTime = 0;
            app->hud->damageDir = 0;
            app->hud->damageTime = 0;
            this->attackerMonster->flags |= Enums::MFLAG_ATTACKING;
            app->game->gsprite_clear(64);
            app->canvas->invalidateRect();
            return 0;
        }
        if (this->stage == -1) {
            app->canvas->staleView = true;
            if (this->numActiveMissiles != 0 || this->exploded) {
                this->updateProjectile();
            }
        }
    }
    if (app->canvas->state == Canvas::ST_DYING) {
        return 0;
    }
    return 1;
}

void Combat::drawEffects() {
    Applet* app = CAppContainer::getInstance()->app;
    if (app->player->statusEffects[(Enums::OFS_STATUSEFFECT_TURNS + Enums::STATUS_EFFECT_FIRE)] > 0) {
        int n = 131072;
        int n2 = ((176 * n) / 65536);
        app->render->draw2DSprite(234, app->time / 256 & 0x3, (app->canvas->viewRect[2] / 2) - (n2 / 2), app->canvas->viewRect[3] - ((n2 / 4)), 0, 3, 0, n);
    }
}

void Combat::drawWeapon(int sx, int sy) {
    Applet* app = CAppContainer::getInstance()->app;
    bool b = false;
    int n4 = 0;
    int frame = 0;
    int renderFlags = 0;
    int flags = 0;
    int scrX = app->canvas->SCR_CX;
    int scrY = app->canvas->viewRect[3];
    int loweredWeaponY= Combat::LOWEREDWEAPON_Y;

    if (!app->render->_gles->isInit) {  // [GEC] Adjusted like this to match the XY position on the GL version
        scrY += 9;
        //renderFlags |= Render::RENDER_FLAG_SCALE_WEAPON; // [GEC]
    }

    this->renderTime = app->upTimeMs;
    if (this->punchingMonster == 0 && !this->lerpingWeapon && this->weaponDown) {
        scrY += loweredWeaponY;
    }

    bool b2 = app->canvas->state == Canvas::ST_CAMERA && app->game->cinematicWeapon != -1;
    int weapon = app->player->ce->weapon;

    if (b2) {
        weapon = app->game->cinematicWeapon;
        scrY -= app->canvas->CAMERAVIEW_BAR_HEIGHT;
    }
    int wpnField = weapon * Combat::FIELD_COUNT;
    if (app->canvas->state == Canvas::ST_DYING || app->player->weapons == 0 || weapon == -1) {
        return;
    }
    sy = -std::abs(sy);
    if (!b2 && app->time < app->hud->damageTime && app->hud->damageCount >= 0 && this->totalDamage > 0) {
        renderFlags |= Render::RENDER_FLAG_BRIGHTREDSHIFT;
    }

    int wpIdleX = this->wpinfo[wpnField + Combat::FLD_WPIDLEX];
    int wpIdleY = this->wpinfo[wpnField + Combat::FLD_WPIDLEY];
    int wpAtkX = this->wpinfo[wpnField + Combat::FLD_WPATKX];
    int wpAtkY = this->wpinfo[wpnField + Combat::FLD_WPATKY];

    int wpFlashX = this->wpinfo[wpnField + Combat::FLD_WPFLASHX];
    int wpFlashY = this->wpinfo[wpnField + Combat::FLD_WPFLASHY];

    int wpX = wpIdleX;
    int wpY = wpIdleY;
    int gameTime = app->gameTime;
    int n14 = 1 << weapon;
    if (Entity::CheckWeaponMask(weapon, Enums::WP_PUNCH_MASK) && (this->punchingMonster == 2 || this->punchingMonster == 3)) {
        b = true;
    }
    bool b5 = b2 || (app->canvas->state == Canvas::ST_COMBAT && this->curAttacker == nullptr && this->nextStage == 1);
    bool b6 = false;
    if (b5 && weapon != Enums::WP_PANZER) {
        wpX = wpAtkX;
        wpY = wpAtkY;
        if (weapon == Enums::WP_DYNAMITE) {
            if (this->settingDynamite) {
                int n14 = gameTime - this->settingDynamiteTime << 16;
                int height = app->render->getHeight(app->canvas->viewX, app->canvas->viewY);
                if (!this->dynamitePlaced) {
                    int n15 = n14 / 750;
                    if (this->placingBombZ != 36) {
                        app->canvas->destPitch = (app->canvas->viewPitch = app->canvas->viewPitch * (65536 - n15) / 65536);
                        app->canvas->destZ = (app->canvas->viewZ = height + ((65536 - n15) * 36 + n15 * this->placingBombZ >> 16));
                    }
                    else {
                        n15 = 65536;
                    }
                    if (app->canvas->viewZ <= height + this->placingBombZ) {
                        app->canvas->destZ = (app->canvas->viewZ = height + this->placingBombZ);
                        if (n15 >= 65536) {
                            if (this->placingBombZ != 36) {
                                app->canvas->viewPitch = (app->canvas->destPitch = 0);
                            }
                            this->dynamitePlaced = true;
                            //Sound.playSound(13);
                            this->settingDynamiteTime = gameTime;
                            int sprite = app->game->entities[app->game->placedBombs[this->currentBombIndex]].getSprite();
                            app->render->mapSpriteInfo[sprite] &= ~0x10000;
                        }
                    }
                }
                else {
                    b = (n4 = 1);
                    int n17 = n14 / 500;
                    app->canvas->destZ = (app->canvas->viewZ = height + ((65536 - n17) * this->placingBombZ + n17 * 36 >> 16));
                    if (app->canvas->viewZ >= height + 36) {
                        app->canvas->destZ = (app->canvas->viewZ = height + 36);
                        if (n17 >= 65536) {
                            this->nextStageTime = 1;
                        }
                    }
                }
            }
            else {
                b = (n4 = 1);
            }
        }
        else if (!this->flashDone) {
            b = !Entity::CheckWeaponMask(weapon, Enums::WP_SNIPERMASK);
            if (gameTime >= this->flashDoneTime) {
                this->flashDone = true;
            }
        }
        else {
            int n15 = (gameTime - this->animStartTime << 16) / this->animTime;
            if (n15 >= 65536) {
                wpX = wpIdleX;
                wpY = wpIdleY;
            }
            else {
                wpX = (65536 - n15) * wpAtkX + n15 * wpIdleX >> 16;
                wpY = (65536 - n15) * wpAtkY + n15 * wpIdleY >> 16;
            }
        }
    }
    else {
        if (weapon == Enums::WP_BOOT) {
            this->lerpingWeapon = false;
            this->weaponDown = this->lerpWpDown;
            this->drawEffects();
            return;
        }
        if (this->punchingMonster == 0 && this->lerpingWeapon) {
            if (this->lerpWpDown && this->lerpWpStartTime + this->lerpWpDur > gameTime) {
                scrY += (gameTime - this->lerpWpStartTime << 16) / this->lerpWpDur * loweredWeaponY >> 16;
            }
            else if (!this->lerpWpDown && this->lerpWpStartTime + this->lerpWpDur > gameTime) {
                scrY += (65536 - (gameTime - this->lerpWpStartTime << 16) / this->lerpWpDur) * loweredWeaponY >> 16;
            }
            else {
                this->lerpWpStartTime = 0;
                this->lerpingWeapon = false;
                this->weaponDown = this->lerpWpDown;
                if (this->lerpWpDown) {
                    scrY += loweredWeaponY;
                }
            }
        }
    }
    int renderMode = Render::RENDER_NORMAL;
    int x = scrX + (wpX + sx);
    int y = scrY - (wpY + sy);

    app->render->_gles->SetGLState();
    if (weapon == Enums::WP_DUAL_PISTOL) {
        int n22 = flags & 0xFFFDFFFF;
        if ((this->animLoopCount & 0x1) != 0x0) {
            n22 |= 0x20000;
            x = scrX - 176 - (wpIdleX + sx);
            if (b == 1) {
                x += wpIdleX - wpX;
            }
        }
        if (b == 1) {
            if ((this->animLoopCount & 0x1) != 0x0) {
                wpFlashX -= 16;
            }
            app->render->draw2DSprite(5, 3, x + wpFlashX, y + wpFlashY, n22, 5, renderFlags, 0x8000);
        }
        app->render->draw2DSprite(1 + weapon, 0, x, y, n22, renderMode, renderFlags, 0x10000);
        int n23 = n22 & 0xFFFDFFFF;
        int n24 = scrX + (wpIdleX + sx);
        int n25 = scrY - (wpIdleY + sy);
        if ((this->animLoopCount & 0x1) == 0x0) {
            n23 |= 0x20000;
            n24 = scrX - 176 - (wpIdleX + sx);
        }
        app->render->draw2DSprite(1 + weapon, 0, n24, n25, n23, renderMode, renderFlags, 0x10000);
    }
    else if (Entity::CheckWeaponMask(weapon, Enums::WP_PUNCH_MASK)) {
        // The melee sequence has explicit right- and left-hand states.  Using
        // animLoopCount parity made every newly started attack choose the same
        // hand and left the declared PUNCH_LEFTHAND state unreachable.
        const bool punchLeft = this->punchingMonster == Combat::PUNCH_LEFTHAND;
        int n26 = flags & 0xFFFDFFFF;
        int flashX = wpFlashX;
        if (punchLeft) {
            n26 |= 0x20000;
            flashX = -wpFlashX;
            x = scrX - 176 - (wpIdleX + sx);
            if (b == 1) {
                x += wpIdleX - wpX;
            }
        }
        app->render->draw2DSprite(1 + weapon, b, x, y, n26, renderMode, renderFlags, 0x10000);
        if (b == 0 && (weapon == Enums::WP_BRASS_PUNCH || weapon == Enums::WP_SPIKE_PUNCH)) {
            app->render->draw2DSprite(1 + weapon, 3, x - flashX, y + wpFlashY, n26, renderMode, renderFlags, 0x10000);
        }
        int n27 = n26 & 0xFFFDFFFF;
        int n28 = scrX + (wpIdleX + sx);
        int n29 = scrY - (wpIdleY + sy);
        if (!punchLeft) {
            n27 |= 0x20000;
            n28 = scrX - 176 - (wpIdleX + sx);
        }
        app->render->draw2DSprite(1 + weapon, 0, n28, n29, n27, renderMode, renderFlags, 0x10000);
        if (weapon == Enums::WP_BRASS_PUNCH || weapon == Enums::WP_SPIKE_PUNCH) {
            app->render->draw2DSprite(1 + weapon, 3, n28 + flashX, n29 + wpFlashY, n27, renderMode, renderFlags, 0x10000);
        }
    }
    else if (weapon == Enums::WP_ITEM) {
        if (app->player->ammo[9] > 0) {
            app->render->draw2DSprite(app->player->activeWeaponDef->tileIndex, 1, x + wpFlashX, y + wpFlashY, flags, renderMode, renderFlags, 0x10000);
        }
        else {
            app->render->draw2DSprite(1 + weapon, 0, x, y, flags, renderMode, renderFlags, 0x10000);
        }
    }
    else if (weapon == Enums::WP_MOUNTED_GUN_TURRET) {
        app->render->draw2DSprite(Enums::TILENUM_MILITARY_CAR, 0, app->canvas->viewRect[2] / 2 - 175, app->canvas->viewRect[3] - 176, 0, 0, 0, 0x10000);
        app->render->draw2DSprite(Enums::TILENUM_MILITARY_CAR, 0, app->canvas->viewRect[2] / 2 - 1, app->canvas->viewRect[3] - 176, 0x20000, 0, 0, 0x10000);

        int v78 = ((app->gameTime - app->drivingGame->fireTime) << 8) / 300;
        int v79;
        if (v78 <= 255)
        {
            if (v78 <= 31)
            {
                wpFlashX -= 6; // [GEC]
                if (app->drivingGame->fireAngle != 0) {
                    wpFlashY -= 4; // [GEC]
                }
                app->render->draw2DSprite(Enums::TILENUM_PISTOL, 3, x + wpFlashX + /*40 old*/52 * app->drivingGame->fireAngle, wpFlashY + y, 0, 5, renderFlags, 0x8000);
            }
            v79 = -10 * v78;
        }
        else
        {
            v78 = 256;
            v79 = -2560;
        }
        app->render->draw2DSprite(Enums::TILENUM_TURRET, app->drivingGame->fireAngle & 1, x + ((10 * app->drivingGame->fireAngle * v78) >> 8), y + (v79 >> 8), (app->drivingGame->fireAngle & 2) << 16, 0, renderFlags, 0x10000);
        
    }
    else if (weapon == Enums::WP_DYNAMITE) {
        if (n4 == 1) {
            app->render->draw2DSprite(1 + weapon, 3, x - 20, y, flags, renderMode, renderFlags, 0x10000);
            this->animStartTime = app->gameTime;
            this->animTime = 800;
        }
        else {
            int n30 = scrX + (wpIdleX + sx);
            int n31 = scrY - (wpIdleY + sy);
            int n32 = 128;
            bool b8 = app->player->ammo[7] > 0 || (app->canvas->state == 5 && this->curAttacker == nullptr);
            if (gameTime > this->animStartTime && this->animTime != 0 && this->nextStage != 1 && this->curAttacker == nullptr && b8) {
                n32 = (gameTime - this->animStartTime << 7) / this->animTime;
            }
            int n33;
            if (n32 > 64) {
                if (n32 > 128) {
                    n32 = 128;
                }
                n33 = n31 + (128 - n32);
                if (b8) {
                    app->render->draw2DSprite(1 + weapon, 2, n30 + wpFlashX, n33 + wpFlashY, flags, renderMode, renderFlags, 0x10000);
                }
            }
            else {
                n33 = n31 + n32;
            }
            app->render->draw2DSprite(1 + weapon, 0, n30, n33, flags, renderMode, renderFlags, 0x10000);
        }
    }
    else if (weapon == Enums::WP_PANZER) {
        int n34 = gameTime - this->animStartTime;
        if (b5 && n34 < 300) {
            int n35 = (n34 << 16) / 300;
            x = scrX + ((65536 - n35) * wpAtkX + n35 * wpIdleX >> 16);
            y = scrY - ((65536 - n35) * wpAtkY + n35 * wpIdleY >> 16);
            ++n4;
        }
        else if (b5) {
            int n36 = (this->animTime - 300) / 2;
            n34 -= 300;
            if (n34 <= n36) {
                y = scrY + ((n34 << 16) / n36 * 114 >> 16) - wpIdleY;
                ++n4;
            }
            else if (n34 < this->animTime - 300) {
                if (app->sound->isSoundPlaying(1097) == 0) {
                    app->sound->playSound(1097, 0, 3, false);
                }
                y = scrY + ((65536 - (n34 - n36 << 16) / n36) * 114 >> 16) - wpIdleY;
            }
        }
        app->render->draw2DSprite(1 + weapon, n4, x, y, flags, renderMode, renderFlags, 0x10000);
    }
    else {
        if (b == 1 && Entity::CheckWeaponMask(weapon, Enums::WP_MUZZLE_FLASH)) {
            app->render->draw2DSprite(5, 3, x + wpFlashX, y + wpFlashY, flags, 5, 0, 0x8000);
        }
        else if (!app->game->isUnderWater() && !b5 && weapon == Enums::WP_FLAMETHROWER) {
            int n37 = app->time / 256 & 0x3;
            app->render->draw2DSprite(1 + weapon, 3, x + wpFlashX, y + wpFlashY, (n37 & 0x1) << 17, ((n37 & 0x2) != 0x0) ? 5 : 3, 0, 0x10000);
        }
        app->render->draw2DSprite(1 + weapon, n4, x, y, flags, renderMode, renderFlags, 0x10000);
        if (!app->game->isUnderWater() && weapon == Enums::WP_TESLA) {
            wpFlashY -= 6; // [GEC]
            wpFlashX += 2; // [GEC]
            app->render->draw2DSprite(1 + weapon, 3 + (app->time / 256 & 0x1), x + wpFlashX, y + wpFlashY, flags, 3, 0, 0x10000);
        }
    }

    app->render->_gles->ResetGLState();
    this->drawEffects();
    this->renderTime = app->upTimeMs - this->renderTime;
}

void Combat::shiftWeapon(bool lerpWpDown) {
    Applet* app = CAppContainer::getInstance()->app;

    if (lerpWpDown == this->lerpWpDown || lerpWpDown == this->weaponDown ||
       (!lerpWpDown && app->player->ce->weapon == Enums::WP_DYNAMITE && app->player->ammo[7] == 0)) {
        return;
    }

    this->lerpingWeapon = true;
    this->lerpWpDown = lerpWpDown;
    this->lerpWpStartTime = app->gameTime;
    this->lerpWpDur = Combat::LOWERWEAPON_TIME;
}

int Combat::runFrame() {
    if (this->curAttacker == nullptr) {
        return this->playerSeq();
    }
    return this->monsterSeq();
}

int Combat::calcHit(Entity* entity) {
    Applet* app = CAppContainer::getInstance()->app;

    if (app->player->ce->weapon == Enums::WP_DYNAMITE) {
        return 0;
    }

    int n = app->player->ce->weapon * Combat::WEAPON_MAX_FIELDS;
    int worldDistToTileDist = this->WorldDistToTileDist(entity->distFrom(app->canvas->destX, app->canvas->destY));
    if (worldDistToTileDist < this->weapons[n + Combat::WEAPON_FIELD_RANGEMIN] || worldDistToTileDist > this->weapons[n + Combat::WEAPON_FIELD_RANGEMAX]) {
        this->crFlags |= Enums::CR_OUTOFRANGE;
        return 0;
    }
    if (!(entity->info & Entity::ENTITY_FLAG_TAKEDAMAGE)) {
        return 0;
    }
    if (entity->def->eType != Enums::ET_ATTACK_INTERACTIVE) {
        return 1;
    }
    if ((this->tableCombatMasks[entity->def->parm] & 1 << app->player->ce->weapon) != 0x0) {
        return 1;
    }
    return 0;
}

void Combat::explodeOnMonster() {
    Applet* app = CAppContainer::getInstance()->app;
    if (this->explodeThread != nullptr) {
        this->explodeThread->run();
        this->explodeThread = nullptr;
    }
    app->render->shotsFired = true;

    if ((Entity::CheckWeaponMask(this->attackerWeaponId, Enums::WP_MELEEMASK) || this->attackerWeaponId == Enums::WP_DYNAMITE) && this->hitType == 0) {
        app->render->shotsFired = false;
    }
    else if (this->curTarget->monster != nullptr && this->curTarget->def->eType == Enums::ET_MONSTER && !(this->curTarget->info & Entity::ENTITY_FLAG_ACTIVE) && !(this->curTarget->monster->flags & Enums::MFLAG_NPC_MONSTER)) {
        app->game->activate(this->curTarget, true, false, true, true);
    }
    if (this->hitType == 0) {
        if (this->targetType != Enums::ET_MONSTER && this->attackerWeaponId == Enums::WP_PANZER) {
            this->hurtEntityAt(this->attackX >> 6, this->attackY >> 6, this->attackX, this->attackY, 0, this->damage + this->crArmorDamage, app->player->getPlayerEnt());
            this->radiusHurtEntities(this->attackX >> 6, this->attackY >> 6, 1, this->damage + this->crArmorDamage, app->player->getPlayerEnt());
        }
        return;
    }
    if (this->targetType == Enums::ET_MONSTER) {
        if (this->totalDamage > 0) {
            this->checkMonsterFX();
            bool pain = this->curTarget->pain(this->totalDamage, app->player->getPlayerEnt());
            if (this->attackerWeaponId != Enums::WP_ITEM) {
                app->particleSystem->spawnMonsterBlood(this->curTarget, false);
            }
            if (this->targetMonster->ce.getStat(Enums::STAT_HEALTH) > 0) {
                int n = 0;
                if (Entity::CheckWeaponMask(this->attackerWeaponId, Enums::WP_PUNCH_MASK) && this->targetSubType == Enums::MONSTER_SKELETON) {
                    if (!(this->curTarget->monster->flags & Enums::MFLAG_ABILITY)) {
                        int sprite = this->curTarget->getSprite();
                        GameSprite* gsprite_alloc = app->game->gsprite_alloc(this->curTarget->def->tileIndex, 16, 16386);

                        short srcX = app->render->mapSprites[app->render->S_X + sprite];
                        app->render->mapSprites[app->render->S_X + gsprite_alloc->sprite] = gsprite_alloc->pos[0] = gsprite_alloc->pos[3] = srcX;

                        short srcY = app->render->mapSprites[app->render->S_Y + sprite];
                        app->render->mapSprites[app->render->S_Y + gsprite_alloc->sprite] = gsprite_alloc->pos[1] = gsprite_alloc->pos[4] = srcY;

                        short srcZ = (short)(app->render->mapSprites[app->render->S_Z + sprite] + 24);
                        app->render->mapSprites[app->render->S_Z + gsprite_alloc->sprite] = gsprite_alloc->pos[2] = gsprite_alloc->pos[5] = srcZ;

                        gsprite_alloc->pos[3] -= (short)app->canvas->viewRightStepX;
                        gsprite_alloc->pos[4] -= (short)app->canvas->viewRightStepY;
                        gsprite_alloc->pos[5] += 20;
                        gsprite_alloc->duration = 500;
                        app->render->mapSprites[app->render->S_SCALEFACTOR + gsprite_alloc->sprite] = 25;

                        gsprite_alloc->vel[0] = (gsprite_alloc->pos[3] - gsprite_alloc->pos[0]) * 1000 / gsprite_alloc->duration;
                        gsprite_alloc->vel[1] = (gsprite_alloc->pos[4] - gsprite_alloc->pos[1]) * 1000 / gsprite_alloc->duration;

                        this->curTarget->monster->flags |= Enums::MFLAG_ABILITY;
                        app->render->relinkSprite(gsprite_alloc->sprite);
                    }
                }
                else if (this->attackerWeaponId == Enums::WP_BOOT) {
                    ++n;
                }

                if ((this->targetMonster->monsterEffects & EntityMonster::MFX_FREEZE) || (this->targetMonster->flags & Enums::MFLAG_KNOCKBACK) || pain) {
                    n = 0;
                }
                if (n > 0) {
                    app->player->unlink();
                    this->curTarget->knockback(app->canvas->viewX, app->canvas->viewY, n);
                    app->player->link();
                }
            }
            if (this->attackerWeaponId == Enums::WP_PANZER) {
                this->radiusHurtEntities(this->attackX >> 6, this->attackY >> 6, 1, this->crDamage >> 2, app->player->getPlayerEnt());
            }
        }
        else if (this->totalDamage < 0) {
            if (this->targetMonster->ce.getStat(Enums::STAT_HEALTH) - this->totalDamage > this->targetMonster->ce.getStat(Enums::STAT_MAX_HEALTH)) {
                this->totalDamage = -(this->targetMonster->ce.getStat(Enums::STAT_MAX_HEALTH) - this->targetMonster->ce.getStat(Enums::STAT_HEALTH));
            }
            this->targetMonster->ce.setStat(Enums::STAT_HEALTH, this->targetMonster->ce.getStat(Enums::STAT_HEALTH) - this->totalDamage);
        }
    }
    else if (this->targetType == Enums::ET_ATTACK_INTERACTIVE) {
        int sprite = this->curTarget->getSprite();
        short srcX = app->render->mapSprites[app->render->S_X + sprite];
        short srcY = app->render->mapSprites[app->render->S_Y + sprite];
        short srcZ = app->render->mapSprites[app->render->S_Z + sprite];
        if (this->attackerWeaponId == Enums::WP_BOOT && this->targetSubType == Enums::INTERACT_BARREL) {
            int n18 = 1;
            short dstX = (short)(srcX + app->canvas->viewStepX * n18);
            short dstY = (short)(srcY + app->canvas->viewStepY * n18);
            app->game->trace(srcX, srcY, dstX, dstY, this->curTarget, 13501, 16);
            if (app->game->traceEntity == nullptr) {
                LerpSprite* lerpSprite = app->game->allocLerpSprite(nullptr, sprite, true);
                lerpSprite->srcX = srcX;
                lerpSprite->srcY = srcY;
                lerpSprite->srcZ = srcZ;
                lerpSprite->srcScale = lerpSprite->dstScale = app->render->mapSprites[app->render->S_SCALEFACTOR + sprite];
                lerpSprite->dstX = dstX;
                lerpSprite->dstY = dstY;
                lerpSprite->dstZ = app->render->getHeight(dstX, dstY) + 32;
                lerpSprite->startTime = app->gameTime - 50;
                lerpSprite->travelTime = 250 * n18;
                app->game->updateLerpSprite(lerpSprite);
                this->curTarget->info |= Entity::ENTITY_FLAG_DIRTY;
            }
            this->totalDamage = 0;
        }
        else if (Entity::CheckWeaponMask(this->attackerWeaponId, Enums::WP_PUNCH_MASK) && this->targetSubType == Enums::INTERACT_CHICKEN) {
            
            /* { // J2ME
                int nextInt = app->nextInt();
                for (int i = 0; i < 4; ++i) {
                    int n20 = nextInt + i & 0x3;
                    if (n20 != app->canvas->viewAngle - 256 >> 8) {
                        int n21 = n20 << 2;
                        short dstX2 = (short)(srcX + app->canvas->viewStepValues[n21 + 0]);
                        short dstY2 = (short)(srcY + app->canvas->viewStepValues[n21 + 1]);
                        app->game->trace(srcX, srcY, dstX2, dstY2, this->curTarget, 15535, 16);
                        if (app->game->numTraceEntities == 0) {
                            app->render->mapSpriteInfo[sprite] |= 0x100;
                            LerpSprite* lerpSprite = app->game->allocLerpSprite(nullptr, sprite, (bool)(1 != 0));
                            lerpSprite->flags |= 0x400;
                            lerpSprite->srcX = srcX;
                            lerpSprite->srcY = srcY;
                            lerpSprite->srcZ = srcZ + 8;
                            lerpSprite->srcScale = lerpSprite->dstScale = app->render->mapSprites[app->render->S_SCALEFACTOR + sprite];
                            lerpSprite->dstX = dstX2;
                            lerpSprite->dstY = dstY2;
                            lerpSprite->dstZ = app->render->getHeight(dstX2, dstY2) + 32;
                            lerpSprite->startTime = app->gameTime - 50;
                            lerpSprite->travelTime = 300;
                            app->game->updateLerpSprite(lerpSprite);
                            this->curTarget->info |= 0x400000;
                            this->totalDamage = 0;
                            return;
                        }
                    }
                }
                if (++this->curTarget->param > 2 || (this->crFlags & 0x2) != 0x0) {
                    this->curTarget->died(true, nullptr);
                    app->particleSystem->spawnParticles(1, 0xFFFFFFFF, sprite);
                    app->sound->playSound(1008, 0, 4, 0);
                    return;
                }
                LerpSprite* lerpSprite = app->game->allocLerpSprite(nullptr, sprite, true);
                app->render->mapSpriteInfo[sprite] |= 0x100;
                this->curTarget->info |= 0x400000;
                this->totalDamage = 0;
                lerpSprite->flags |= 0x404;
                uint8_t b = (uint8_t)srcX;
                lerpSprite->dstX = lerpSprite->srcX = b;
                uint8_t b2 = (uint8_t)srcY;
                lerpSprite->dstY = lerpSprite->srcY = b2;
                lerpSprite->srcZ = app->render->getHeight(lerpSprite->dstX, lerpSprite->dstY) + 32;
                lerpSprite->dstZ = app->render->getHeight(lerpSprite->dstX, lerpSprite->dstY) + 32;
                lerpSprite->height = 4;
                lerpSprite->srcScale = lerpSprite->dstScale = app->render->mapSprites[app->render->S_SCALEFACTOR + sprite];
                lerpSprite->travelTime = 350;
                lerpSprite->startTime = app->gameTime - 50;
                app->game->updateLerpSprite(lerpSprite);
                app->particleSystem->spawnParticles(1, 0xFFFFFFFF, sprite);
            }*/

            // IOS
            {
                if (++this->curTarget->param > 2 || (this->crFlags & Enums::CR_CRIT)) {
                    this->curTarget->died(true, nullptr);
                    app->particleSystem->spawnParticles(1, 0xFFFFFFFF, sprite);
                    app->sound->playSound(1008, 0, 4, 0);
                    return;
                }

                this->curTarget->info |= Entity::ENTITY_FLAG_DIRTY;
                this->totalDamage = 0;
                app->render->mapSpriteInfo[sprite] |= 0x100;
                app->sound->playSound(1137, 0, 4, 0);
                app->particleSystem->spawnParticles(1, 0xFFFFFFFF, sprite);

                LerpSprite* lerpSprite = app->game->allocLerpSprite(nullptr, sprite, true);
                lerpSprite->flags |= 0x400;
                lerpSprite->dstX = lerpSprite->srcX = srcX;
                lerpSprite->dstY = lerpSprite->srcY = srcY;
                lerpSprite->srcZ = app->render->getHeight(lerpSprite->dstX, lerpSprite->dstY) + 32;
                lerpSprite->dstZ = app->render->getHeight(lerpSprite->dstX, lerpSprite->dstY) + 32;
                lerpSprite->srcScale = lerpSprite->dstScale = app->render->mapSprites[app->render->S_SCALEFACTOR + sprite];
                lerpSprite->startTime = app->gameTime - 50;
                lerpSprite->travelTime = 350;

                unsigned int n20 = (((app->canvas->viewAngle) >> 8) & 0xFF) & 3;
                unsigned int n21 = ((unsigned int)(app->canvas->viewAngle + 256) >> 6) & 12;

                short dstX2 = (short)(srcX + app->canvas->viewStepValues[n21 + 0]);
                short dstY2 = (short)(srcY + app->canvas->viewStepValues[n21 + 1]);
                app->game->trace(srcX, srcY, dstX2, dstY2, this->curTarget, 15535, 16);
                if (app->game->numTraceEntities != 0) {
                    n21 = n20 << 2;
                    dstX2 = (short)(srcX + app->canvas->viewStepValues[n21 + 0]);
                    dstY2 = (short)(srcY + app->canvas->viewStepValues[n21 + 1]);
                    app->game->trace(srcX, srcY, dstX2, dstY2, this->curTarget, 15535, 16);
                    if (app->game->numTraceEntities != 0) {
                        lerpSprite->flags |= 4;
                        lerpSprite->height = 4;
                        app->game->updateLerpSprite(lerpSprite);
                    }
                }
                else {
                    lerpSprite->srcZ = srcZ + 8;
                    lerpSprite->dstX = dstX2;
                    lerpSprite->dstY = dstY2;
                    lerpSprite->dstZ = app->render->getHeight(dstX2, dstY2) + 32;
                    lerpSprite->travelTime = 400;
                    app->game->updateLerpSprite(lerpSprite);
                }
            }
            return;
        }
        else if (this->attackerWeaponId == Enums::WP_BOOT && this->targetSubType == Enums::INTERACT_CHICKEN) {
            int n26 = 32;
            if (!app->canvas->isChickenKicking && (++this->curTarget->param > 2 || (this->crFlags & Enums::CR_CRIT))) {
                this->curTarget->died(true, nullptr);
                app->particleSystem->spawnParticles(1, 0xFFFFFFFF, sprite);
                return;
            }
            LerpSprite* lerpSprite = app->game->allocLerpSprite(nullptr, sprite, true);
            app->render->mapSpriteInfo[sprite] |= 0x100;
            this->curTarget->info |= Entity::ENTITY_FLAG_DIRTY;
            this->totalDamage = 0;
            lerpSprite->flags |= 0x404;
            lerpSprite->srcX = srcX;
            lerpSprite->srcY = srcY;
            lerpSprite->srcZ = srcZ;
            lerpSprite->srcScale = lerpSprite->dstScale = app->render->mapSprites[app->render->S_SCALEFACTOR + sprite];
            if (!app->canvas->isChickenKicking || (app->canvas->isChickenKicking && app->canvas->kickingPhase == 0)) {
                int max = std::max(app->player->ce->getStat(Enums::STAT_STRENGTH) / 20 + 1, 3);
                lerpSprite->dstX = (short)(srcX + app->canvas->viewStepX * max);
                lerpSprite->dstY = (short)(srcY + app->canvas->viewStepY * max);
                int farthestKnockbackDist = this->curTarget->getFarthestKnockbackDist(lerpSprite->srcX, lerpSprite->srcY, lerpSprite->dstX, lerpSprite->dstY, this->curTarget, 13501, 16, max);
                if (farthestKnockbackDist == 0) {
                    lerpSprite->dstX = (short)(srcX + (app->canvas->viewStepX >> 6) * 31);
                    lerpSprite->dstY = (short)(srcY + (app->canvas->viewStepY >> 6) * 31);
                    lerpSprite->height = 16;
                    lerpSprite->flags |= 0x8;
                    lerpSprite->travelTime = 350;
                }
                else {
                    if (farthestKnockbackDist < max) {
                        lerpSprite->dstX = (short)(srcX + app->canvas->viewStepX * farthestKnockbackDist + (app->canvas->viewStepX >> 6) * 31);
                        lerpSprite->dstY = (short)(srcY + app->canvas->viewStepY * farthestKnockbackDist + (app->canvas->viewStepY >> 6) * 31);
                        lerpSprite->flags |= 0x8;
                        lerpSprite->travelTime = 350 * farthestKnockbackDist + 250;
                    }
                    else {
                        lerpSprite->travelTime = 350 * farthestKnockbackDist;
                    }
                    lerpSprite->height = farthestKnockbackDist * 16;
                }
            }
            else {
                n26 = 65;
                lerpSprite->dstX = (short)(srcX + app->canvas->viewStepX * app->canvas->chickenDestFwd + app->canvas->viewRightStepX * app->canvas->chickenDestRight + 31 * (app->canvas->viewStepX >> 6));
                lerpSprite->dstY = (short)(srcY + app->canvas->viewStepY * app->canvas->chickenDestFwd + app->canvas->viewRightStepY * app->canvas->chickenDestRight + 31 * (app->canvas->viewStepY >> 6));
                //app->canvas->gridIdx = 19 - ((lerpSprite->dstX >> 6) - 1 + ((lerpSprite->dstY >> 6) - 18) * 5);
                app->canvas->gridIdx = -5 * (lerpSprite->dstY >> 6) - (lerpSprite->dstX >> 6) + 110;
                lerpSprite->travelTime = 350 * app->canvas->chickenDestFwd;
                lerpSprite->height = app->canvas->chickenDestFwd * 18;
            }
            lerpSprite->dstZ = app->render->getHeight(lerpSprite->dstX, lerpSprite->dstY) + n26;
            lerpSprite->startTime = app->gameTime - 50;
            app->game->updateLerpSprite(lerpSprite);
            app->particleSystem->spawnParticles(1, 0xFFFFFFFF, sprite);
        }
        if (this->totalDamage > 0) {
            this->curTarget->pain(this->totalDamage, app->player->getPlayerEnt());
        }
    }
    else if (this->targetType == Enums::ET_CORPSE) {
        int sprite = this->curTarget->getSprite();
        app->render->mapSpriteInfo[sprite] |= 0x10000;
        this->isGibbed = true;
        app->particleSystem->spawnMonsterBlood(this->curTarget, this->isGibbed);
        if (this->attackerWeaponId == Enums::WP_M_TORMENTOR_ABSORB) {
            this->curAttacker->monster->ce.setStat(Enums::STAT_HEALTH, this->curAttacker->monster->ce.getStat(Enums::STAT_MAX_HEALTH));
        }
    }
    else if (this->targetType == Enums::ET_DECOR_NOCLIP && this->curAttacker != nullptr && this->curAttacker->def->eType == Enums::ET_MONSTER && this->curAttacker->def->eSubType == Enums::BOSS_HARBINGER) {
        app->particleSystem->spawnParticles(1, 0xFF660000, this->curTarget->getSprite());
        this->curAttacker->monster->ce.addStat(Enums::STAT_HEALTH, 150);
        this->curAttacker->param = 0;
    }
}

void Combat::explodeOnPlayer() {
    Applet* app = CAppContainer::getInstance()->app;
    if (this->curTarget != nullptr) {
        return;
    }
    int sprite = this->curAttacker->getSprite();
    int viewAngle = app->canvas->viewAngle;
    if (app->canvas->isZoomedIn) {
        viewAngle += app->canvas->zoomAngle;
    }
    app->hud->damageDir = app->player->calcDamageDir(app->canvas->viewX, app->canvas->viewY, viewAngle, app->render->mapSprites[app->render->S_X + sprite], app->render->mapSprites[app->render->S_Y + sprite]);
    if (this->animLoopCount > 0) {
        app->hud->damageTime = app->time + 150;
    }
    else {
        app->hud->damageTime = app->time + 1000;
    }
    if (app->hud->damageDir != 3) {
        app->canvas->staleTime = app->hud->damageTime + 1;
    }
    bool b = false;
    if (app->player->buffs[(Enums::OFS_BUFF_COUNT + Enums::BUFF_REFLECT)] > 0 && !this->curAttacker->isBoss()) {
        b = true;
    }
    if (this->gotHit && (this->totalDamage > 0 || this->totalArmorDamage > 0)) {
        app->player->painEvent(this->curAttacker, true);
        if (this->curAttacker != nullptr) {
            Entity* entity = &app->game->entities[1];
            app->game->unlinkEntity(this->curAttacker);
            if (this->attackerWeaponProj == Enums::WP_PROJ_FIRE) {
                app->player->addStatusEffect(Enums::STATUS_EFFECT_FIRE, 5, 3);
                app->player->translateStatusEffects();
            }
            else if ((this->attackerWeaponId == Enums::WP_M_OLARIC_PUNCH || this->attackerWeaponId == Enums::WP_M_SCREAM) && !app->player->hasPurifyEffect()) {
                app->canvas->startShake(500, 4, 500);
                app->player->addStatusEffect(Enums::STATUS_EFFECT_DIZZY, 20, 1);
                app->player->translateStatusEffects();
            }
            else if (this->attackerWeaponId == Enums::WP_M_SOUL_SUCK && !app->player->hasPurifyEffect()) {
                app->player->addStatusEffect(Enums::STATUS_EFFECT_COLD, 7, 5);
                app->player->translateStatusEffects();
            }
            app->player->addArmor(-this->totalArmorDamage);
            if (this->totalDamage > 0) {
                if (!b) {
                    if (app->game->difficulty != 1) {
                        int loadMapID = app->canvas->loadMapID;
                        if (app->game->difficulty == 4) {
                            loadMapID *= 2;
                        }
                        this->totalDamage += (this->totalDamage >> 1) + loadMapID / this->weapons[this->attackerWeapon + Combat::WEAPON_FIELD_NUMSHOTS];
                    }
                    this->accumRoundDamage += this->totalDamage;
                    app->player->pain(this->totalDamage, this->curAttacker, false);
                    if (app->player->ce->getStat(Enums::STAT_HEALTH) == 0) {
                        return;
                    }
                    if (this->attackerWeaponId == Enums::WP_M_OLARIC_JUMP) {
                        app->render->rockView(1000, app->canvas->viewX, app->canvas->viewY, app->canvas->viewZ - 20);
                    }
                    else if (app->canvas->knockbackDist == 0) {
                        int* calcPosition = this->curAttacker->calcPosition();
                        int a = app->canvas->viewX - calcPosition[0];
                        int a2 = app->canvas->viewY - calcPosition[1];
                        if (a != 0) {
                            a /= std::abs(a);
                        }
                        if (a2 != 0) {
                            a2 /= std::abs(a2);
                        }
                        app->render->rockView(200, app->canvas->viewX + a * 6, app->canvas->viewY + a2 * 6, app->canvas->viewZ);
                    }
                    if (this->attackerWeaponId == Enums::WP_M_SCIENTIST && !app->player->hasPurifyEffect()) {
                        app->player->addStatusEffect(Enums::STATUS_EFFECT_DISEASE, 10, 5);
                        app->player->translateStatusEffects();
                    }
                    if (app->player->ce->getStat(Enums::STAT_HEALTH) > 0 && Entity::CheckWeaponMask(this->attackerWeaponId, Enums::WP_MONSTER_KNOCKBACK) && this->curAttacker->def->eSubType != 10) {
                        entity->knockback(app->render->mapSprites[app->render->S_X + sprite], app->render->mapSprites[app->render->S_Y + sprite], (this->attackerWeaponId == Enums::WP_M_BOSS_HARBINGER_SLAM) ? 2 : 1);
                    }
                }
                else if (this->curAttacker->monster != nullptr) {
                    this->accumRoundDamage = (this->reflectionDmg = this->totalDamage);
                    this->animLoopCount = 0;
                    this->nextStage = 1;
                }
            }
            else {
                app->hud->damageCount = 0;
                this->totalDamage = 0;
            }
            if (this->totalDamage <= 0) {
                app->hud->addMessage(CodeStrings::CRIT);
            }
            app->game->linkEntity(this->curAttacker, this->curAttacker->linkIndex % 32, this->curAttacker->linkIndex / 32);
        }
    }
    else {
        app->hud->damageCount = 0;
        if (this->crFlags & Enums::CR_IMMUNE) {
            app->localization->resetTextArgs();
            app->hud->addMessage(CodeStrings::WEAPON_DEFLECT);
        }
        else if (!(this->crFlags & Enums::CR_DODGE)) {
            app->localization->resetTextArgs();
            Text* smallBuffer = app->localization->getSmallBuffer();
            app->localization->composeTextField(this->curAttacker->name, smallBuffer);
            app->localization->addTextArg(smallBuffer);
            app->hud->addMessage(CodeStrings::X_MISSED);
            smallBuffer->dispose();
        }
        app->hud->damageCount = -1;
    }
}

int Combat::getMonsterField(EntityDef* entityDef, int n) {
    return this->monsterAttacks[(entityDef->eSubType * 9) + (entityDef->parm * 3) + n];
}

void Combat::radiusHurtEntities(int n, int n2, int n3, int n4, Entity* entity) {
    Applet* app = CAppContainer::getInstance()->app;
    int n5 = (n << 6) + 32;
    int n6 = (n2 << 6) + 32;
    int n7 = app->render->getHeight(n << 6, n2 << 6) + 32;
    for (int i = n2 - 1; i <= n2 + 1; ++i) {
        for (int j = n - 1; j <= n + 1; ++j) {
            app->game->trace(n5, n6, n7, (j << 6) + 32, (i << 6) + 32, app->render->getHeight(j << 6, i << 6) + 32, nullptr, 4129, 1);
            if (app->game->traceEntity == nullptr) {
                this->hurtEntityAt(j, i, n, n2, (j == n && i == n2) ? 0 : n3, n4, entity);
            }
        }
    }
}

void Combat::hurtEntityAt(int n, int n2, int n3, int n4, int n5, int n6, Entity* entity) {
    this->hurtEntityAt(n, n2, n3, n4, n5, n6, entity, false);
}

void Combat::hurtEntityAt(int n, int n2, int n3, int n4, int n5, int n6, Entity* entity, bool b) {
    Applet* app = CAppContainer::getInstance()->app;
    this->crFlags = Enums::CR_IGNORERANGE;
    app->render->shotsFired = true;
    if (app->canvas->viewX >> 6 == n && app->canvas->viewY >> 6 == n2) {
        if (app->player->buffs[(Enums::OFS_BUFF_COUNT + Enums::BUFF_REFLECT)] > 0) {
            app->localization->resetTextArgs();
            app->localization->addTextArg(n6);
            app->hud->addMessage(Strings::FILE_CODESTRINGS, CodeStrings::REFLECTED_X);
        }
        else {
            int min = std::min(n6 * 10 / 100, app->player->ce->getStat(Enums::STAT_ARMOR));
            n6 -= min;
            n6 -= n6 * app->player->ce->getStat(Enums::STAT_DEFENSE) / 100;
            app->player->painEvent(nullptr, false);
            app->player->getPlayerEnt()->knockback((n3 << 6) + 32, (n4 << 6) + 32, n5);
            if (n6 > 0) {
                this->crArmorDamage = min;
                app->player->pain(n6, nullptr, true);
                app->player->addArmor(-this->crArmorDamage);
                if (!app->game->isUnderWater()) {
                    app->player->addStatusEffect(Enums::STATUS_EFFECT_FIRE, 5, 3);
                }
                app->player->translateStatusEffects();
            }
        }
    }
    else {
        Entity* nextOnTile;
        for (Entity* mapEntity = app->game->findMapEntity(n << 6, n2 << 6, 30381); mapEntity != nullptr; mapEntity = nextOnTile) {
            nextOnTile = mapEntity->nextOnTile;
            if (mapEntity->info & Entity::ENTITY_FLAG_TAKEDAMAGE) {
                if (mapEntity->def->eType == Enums::ET_CORPSE) {
                    if (!b) {
                        return;
                    }
                    mapEntity->died(false, entity);
                    mapEntity->info |= Entity::ENTITY_FLAG_GIBBED;
                    app->particleSystem->spawnMonsterBlood(mapEntity, true);
                }
                else if (mapEntity->monster == nullptr || mapEntity->def->eSubType != 14) {
                    if (mapEntity->monster != nullptr) {
                        mapEntity->info |= Entity::ENTITY_FLAG_DROPPED;
                        if (!(mapEntity->info & Entity::ENTITY_FLAG_ACTIVE)) {
                            app->game->activate(mapEntity, true, false, true, true);
                        }
                        int n7 = this->getWeaponWeakness(Enums::WP_DYNAMITE, mapEntity->def->eSubType, mapEntity->def->parm) * n6 >> 8;
                        int n8 = n7 - (mapEntity->monster->ce.getStatPercent(3) * n7 >> 8);
                        if (n8 > 0) {
                            if (!app->game->isUnderWater() && (mapEntity->def->eSubType != 4 || mapEntity->def->parm <= 0) && mapEntity->def->eSubType != 6) {
                                mapEntity->monster->monsterEffects &= EntityMonster::MFX_FIRE_REMOVE;
                                mapEntity->monster->monsterEffects |= EntityMonster::MFX_FIRE;
                            }
                            bool pain = mapEntity->pain(n8, nullptr);
                            if (!(mapEntity->monster->flags & Enums::MFLAG_KNOCKBACK) && !pain) {
                                mapEntity->knockback((n3 << 6) + 32, (n4 << 6) + 32, n5);
                            }
                            app->localization->resetTextArgs();
                            app->localization->addTextIDArg(mapEntity->name);
                            app->localization->addTextArg(n8);
                            if (mapEntity->monster->ce.getStat(Enums::STAT_HEALTH) <= 0) {
                                app->hud->addMessage(CodeStrings::TOOK_DAMAGE_DEAD);
                                if (b && entity != nullptr && entity->def->eType == Enums::ET_DECOR_NOCLIP && entity->def->eSubType == 6) {
                                    mapEntity->info |= Entity::ENTITY_FLAG_GIBBED;
                                    app->particleSystem->spawnMonsterBlood(mapEntity, true);
                                }
                                mapEntity->died(true, entity);
                            }
                            else {
                                app->hud->addMessage(CodeStrings::TOOK_DAMAGE);
                            }
                        }
                    }
                    else if (mapEntity->def->eType != Enums::ET_ENV_DAMAGE) {
                        if (mapEntity->def->eType == Enums::ET_ATTACK_INTERACTIVE && !(this->tableCombatMasks[mapEntity->def->parm] & (1 << Enums::WP_DYNAMITE))) {
                            return;
                        }
                        mapEntity->pain(n6, entity);
                        if (!mapEntity->isExplodableEntity()) {
                            mapEntity->died(true, entity);
                        }
                    }
                }
            }
        }
    }
}

Text* Combat::getWeaponStatStr(int n) {
    Applet* app = CAppContainer::getInstance()->app;
    int n2 = n * Combat::WEAPON_MAX_FIELDS;
    Text* largeBuffer = app->localization->getLargeBuffer();
     app->localization->resetTextArgs();
     app->localization->addTextArg(this->weapons[n2 + Combat::WEAPON_FIELD_STRMIN]);
     app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::BASE_DAMAGE, largeBuffer);
     app->localization->resetTextArgs();
    if (this->weapons[n2 + Combat::WEAPON_FIELD_RANGEMAX] != 1) {
         app->localization->addTextArg(this->weapons[n2 + Combat::WEAPON_FIELD_RANGEMIN]);
         app->localization->addTextArg(this->weapons[n2 + Combat::WEAPON_FIELD_RANGEMAX]);
         app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::IDEAL_RANGE1, largeBuffer);
    }
    else {
         app->localization->addTextArg(this->weapons[n2 + Combat::WEAPON_FIELD_RANGEMAX]);
         app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::IDEAL_RANGE2, largeBuffer);
    }
    EntityDef* find = app->entityDefManager->find(Enums::ET_ITEM, Enums::IT_AMMO, this->weapons[n2 + Combat::WEAPON_FIELD_AMMOTYPE]);
    if (find != nullptr) {
         app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::WEAPON_AMMO, largeBuffer);
         app->localization->composeText(Strings::FILE_ENTITYSTRINGS, find->name, largeBuffer);
    }
    return largeBuffer;
}

Text* Combat::getArmorStatStr(int n) {
    Applet* app = CAppContainer::getInstance()->app;
    Text* largeBuffer = app->localization->getLargeBuffer();
    if (n != -1) {
        app->localization->resetTextArgs();
        app->localization->addTextArg(app->player->ce->getStat(Enums::STAT_ARMOR));
        app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::DEFLECT_X, largeBuffer);
    }
    app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::FOR_MORE_DETAILS, largeBuffer);
    return largeBuffer;
}

int Combat::WorldDistToTileDist(int n) {
	for (int i = 0; i < (Combat::MAX_TILEDISTANCES - 1); ++i) {
		if (n < this->tileDistances[i]) {
			return i;
		}
	}
	return (Combat::MAX_TILEDISTANCES - 1);
}

void Combat::cleanUpAttack() {
    this->stage = 1;
    this->curAttacker = nullptr;
    this->curTarget = nullptr;
}

void Combat::updateProjectile() {
    Applet* app = CAppContainer::getInstance()->app;

    if (this->curAttacker != nullptr && !Entity::CheckWeaponMask(this->attackerWeaponId, Enums::WP_INSTANT_ATTACK) && ((app->render->mapSpriteInfo[this->curAttacker->getSprite()] & 0xFF00) >> 8 & 0xF) == 0x0) {
        return;
    }

    if (this->numActiveMissiles > 0) {
        int renderMode = 0;
        for (int i = 0; i < this->numActiveMissiles; ++i) {
            GameSprite* actMissile = this->activeMissiles[i];
            if (!(actMissile->flags & GameSprite::FLAG_PROPOGATE)) {
                bool b = true;
                int scaleFactor = 64;
                int x = actMissile->pos[3];
                int y = actMissile->pos[4];
                int z = actMissile->pos[5];
                switch (this->attackerWeaponProj) {
                    case Enums::WP_PROJ_PANZER: {
                        this->missileAnim = Enums::TILENUM_ANIM_EXPLOSION;
                        renderMode = 4;
                        app->canvas->startShake(500, 4, 500);
                        app->sound->playSound(1136, 0, 4, 0);
                        break;
                    }
                    case Enums::WP_PROJ_WRENCH: {
                        app->sound->playSound(1150, 0, 3, 0);
                        break;
                    }
                    case Enums::WP_PROJ_SKULL: {
                        this->missileAnim = Enums::TILENUM_ANIM_SKULL;
                        //Sound.playSound(13);
                        break;
                    }
                    case Enums::WP_PROJ_FLAME: {
                        this->missileAnim = 0;
                    } // Fall through
                    case Enums::WP_PROJ_FIRE: {
                        if (this->curTarget == nullptr && app->combat->hitType != 0 && app->player->buffs[(Enums::OFS_BUFF_COUNT + Enums::BUFF_ANTIFIRE)] > 0) {
                            app->sound->playSound(1105, 0, 5, 0);
                            this->missileAnim = Enums::TILENUM_FOG_GRAY;
                            x += app->canvas->viewStepX >> 1;
                            y += app->canvas->viewStepY >> 1;
                            renderMode = 3;
                            break;
                        }
                        if (this->attackerWeaponProj == Enums::WP_PROJ_FIRE) {
                            app->sound->playSound(1136, 0, 4, 0);
                            this->missileAnim = Enums::TILENUM_ANIM_EXPLOSION;
                            renderMode = 4;
                            break;
                        }
                        break;
                    }
                    case Enums::WP_PROJ_ITEM: {
                        this->missileAnim = 0;
                        app->particleSystem->spawnParticles(1, -1, actMissile->sprite);
                        app->sound->playSound(1145, 0, 5, 0);
                        break;
                    }
                    default: {
                        this->missileAnim = 0;
                        break;
                    }
                }

                if (this->curAttacker != nullptr && (1 << app->hud->damageDir & 0x1C) == 0x0 && !b) {
                    this->missileAnim = 0;
                }

                app->game->gsprite_destroy(actMissile);
                if (this->missileAnim != 0 && (this->curAttacker == nullptr || (this->curTarget == nullptr && this->hitType != 0))) {
                    GameSprite* gSprite = app->game->gsprite_allocAnim(this->missileAnim, x, y, z);
                    gSprite->flags |= GameSprite::FLAG_UNLINK;
                    this->nextStageTime = app->gameTime + gSprite->duration;
                    app->render->mapSprites[app->render->S_RENDERMODE + gSprite->sprite] = (short)renderMode;
                    app->render->mapSprites[app->render->S_SCALEFACTOR + gSprite->sprite] = (short)scaleFactor;
                    if (this->curAttacker != nullptr && (app->render->mapSpriteInfo[(this->curAttacker->info & 0xFFFF) - 1] & 0x20000) != 0x0) {
                        app->render->mapSpriteInfo[gSprite->sprite] |= 0x20000;
                    }
                }
                for (int k = i + 1; k < this->numActiveMissiles; ++k) {
                    this->activeMissiles[k - 1] = this->activeMissiles[k];
                }
                --this->numActiveMissiles;

                if (this->attackerWeaponProj == Enums::WP_PROJ_LIGHTNING && (this->crFlags & Enums::CR_HITMASK)) {
                    Entity* entity = app->game->findMapEntity(x, y, 1028);
                    if (entity != nullptr && (entity->def->eType != Enums::ET_ATTACK_INTERACTIVE || (entity->def->eType == Enums::ET_ATTACK_INTERACTIVE && entity->def->eSubType == 8))) {
                        entity->pain(this->totalDamage, this->curAttacker);
                        if (entity != this->curTarget && entity->monster->ce.getStat(Enums::STAT_HEALTH) <= 0) {
                            entity->died(true, this->curAttacker);
                        }
                    }
                }
                else {
                    this->exploded = true;
                }
            }
        }
    }
    else if ((this->attackerWeaponId == Enums::WP_M_KICK || this->attackerWeaponId == Enums::WP_M_FLYINGKICK) && this->curAttacker != nullptr && this->curAttacker->def->eSubType != 10) {
        this->missileAnim = Enums::TILENUM_MONSTER_KICK;
        int* calcPosition = this->curAttacker->calcPosition();
        if (app->game->isInFront(calcPosition[0] >> 6, calcPosition[1] >> 6)) {
            GameSprite* gSprite = app->game->gsprite_allocAnim(this->missileAnim, app->canvas->destZ, app->canvas->destY, app->canvas->destZ);
            gSprite->flags |= GameSprite::FLAG_UNLINK;
            app->render->mapSprites[app->render->S_RENDERMODE + gSprite->sprite] = 4;
            if ((app->render->mapSpriteInfo[(this->curAttacker->info & 0xFFFF) - 1] & 0x20000) != 0x0) {
                app->render->mapSpriteInfo[gSprite->sprite] |= 0x20000;
            }
        }
    }
    else if (this->attackerWeaponId == Enums::WP_M_CLAW) {
        this->missileAnim = Enums::TILENUM_MONSTER_CLAW;
        int* calcPosition = this->curAttacker->calcPosition();
        if (app->game->isInFront(calcPosition[0] >> 6, calcPosition[1] >> 6)) {
            GameSprite* gSprite = app->game->gsprite_allocAnim(this->missileAnim, app->canvas->destZ, app->canvas->destY, app->canvas->destZ);
            gSprite->flags |= GameSprite::FLAG_UNLINK;
            app->render->mapSprites[app->render->S_RENDERMODE + gSprite->sprite] = 5;
            if ((app->render->mapSpriteInfo[(this->curAttacker->info & 0xFFFF) - 1] & 0x20000) != 0x0) {
                app->render->mapSpriteInfo[gSprite->sprite] |= 0x20000;
            }
        }
    }
    if (this->exploded) {
        if (this->curTarget == nullptr) {
            this->explodeOnPlayer();
            this->exploded = false;
        }
        else {
            this->explodeOnMonster();
            this->exploded = false;
        }
    }
}

void Combat::launchProjectile() {
    Applet* app = CAppContainer::getInstance()->app;
    int n = 256;
    int n2;
    int n3 = 16;
    int renderMode = 0;
    this->dodgeDir = (((int)app->nextInt() > 0x3FFFFFF) ? 1 : 0);
    int n5 = ((this->dodgeDir << 1) - 1) * 16;
    int x1;
    int y1;
    int z1;
    if (this->curAttacker == nullptr) {
        x1 = app->game->viewX;
        y1 = app->game->viewY;
        z1 = app->render->getHeight(x1, y1) + 32;
        n2 = 0;
    }
    else {
        int sprite = (this->curAttacker->info & 0xFFFF) - 1;
        x1 = app->render->mapSprites[app->render->S_X + sprite];
        y1 = app->render->mapSprites[app->render->S_Y + sprite];
        z1 = app->render->mapSprites[app->render->S_Z + sprite];
        n2 = 16;
    }
    int x2;
    int y2;
    int z2;
    if (this->curTarget == nullptr) {
        x2 = app->game->viewX;
        y2 = app->game->viewY;
        z2 = app->render->getHeight(x2, y2) + 32;
    }
    else if (this->targetType == Enums::ET_WORLD) {
        x2 = app->game->traceCollisionX;
        y2 = app->game->traceCollisionY;
        z2 = app->game->traceCollisionZ;
    }
    else {
        int n10 = (this->curTarget->info & 0xFFFF) - 1;
        x2 = app->render->mapSprites[app->render->S_X + n10];
        y2 = app->render->mapSprites[app->render->S_Y + n10];
        z2 = app->render->mapSprites[app->render->S_Z + n10];
    }
    bool b = false;
    switch (this->attackerWeaponProj) {
        case Enums::WP_PROJ_PANZER: {
            this->missileAnim = Enums::TILENUM_MISSILE_PANZER;
            renderMode = 0;
            n += 64;
            break;
        }
        case Enums::WP_PROJ_FIRE: {
            this->missileAnim = Enums::TILENUM_ANIM_EXPLOSION;
            b = false;
            renderMode = 3;
            break;
        }
        case Enums::WP_PROJ_SKULL: {
            this->missileAnim = Enums::TILENUM_ANIM_SKULL;
            renderMode = 0;
            this->curAttacker->monster->flags |= Enums::MFLAG_ABILITY;
            z1 -= 16;
            break;
        }
        case Enums::WP_PROJ_ITEM: {
            b = false;
            this->missileAnim = app->player->activeWeaponDef->tileIndex;
            renderMode = 0;
            break;
        }
        case Enums::WP_PROJ_WRENCH: {
            this->missileAnim = Enums::TILENUM_ANIM_WRENCH;
            renderMode = 0;
            break;
        }
        case Enums::WP_PROJ_FLAME: {
            renderMode = 3;
            this->missileAnim = Enums::TILENUM_FIRE_STREAM;
            b = false;
            break;
        }
        case Enums::WP_PROJ_LIGHTNING: {
            renderMode = 3;
            this->missileAnim = Enums::TILENUM_TESLA_BOLT;
            b = false;
            break;
        }
        default: {
            this->missileAnim = 0;
            this->exploded = true;
            return;
        }
    }
    int dx = std::abs(x2 - x1);
    int dy = std::abs(y2 - y1);
    if (this->crFlags & Enums::CR_OUTOFRANGE) {
        int n11 = this->weapons[this->attackerWeapon + Combat::WEAPON_FIELD_RANGEMAX] * 64;
        n3 = 0;
        if (dx != 0 && dx > dy) {
            x2 = x1 + (x2 - x1) * n11 / dx;
        }
        else if (dy != 0) {
            y2 = y1 + (y2 - y1) * n11 / dy;
        }
        dx = std::abs(x2 - x1);
        dy = std::abs(y2 - y1);
        n5 = 0;
    }
    else if (this->hitType == 0) {
        if ((this->targetType == Enums::ET_MONSTER && this->targetSubType != Enums::MONSTER_ZOMBIE) || this->curTarget == nullptr) {
            n3 = 0;
        }
        else {
            n5 = 0;
        }
    }
    else {
        n5 = 0;
    }

    if (dy > dx) {
        int n12 = (((y2 - y1) >> 31) << 1) + 1;
        y1 +=  (n2 * n12);
        y2 += -(n3 * n12);
        x2 += n5;
    }
    else {
        int n15 = (((x2 - x1) >> 31) << 1) + 1;
        x1 +=  (n2 * n15);
        x2 += -(n3 * n15);
        y2 += n5;
    }
    if (this->attackerWeaponProj == Enums::WP_PROJ_PANZER) {
        if (this->curAttacker == nullptr) {
            x1 += 4 * app->canvas->viewRightStepX >> 6;
            y1 += 4 * app->canvas->viewRightStepY >> 6;
            z1 -= 5;
        }
        else {
            int n16 = 16;
            if ((app->render->mapSpriteInfo[this->curAttacker->getSprite()] & 0x20000) != 0x0) {
                n16 = -16;
            }
            x1 += n16 * app->canvas->viewRightStepX >> 6;
            y1 += n16 * app->canvas->viewRightStepY >> 6;
        }
    }

    if (this->attackerWeaponProj == Enums::WP_PROJ_FIRE && this->curAttacker != nullptr && this->curAttacker->def->eSubType == 14) {
        x1 += 28 * app->canvas->viewRightStepX >> 6;
        y1 += 28 * app->canvas->viewRightStepY >> 6;
        z1 += 26;
    }

    int n19 = 1000 * std::max(dx, dy) / n;
    GameSprite* allocMissile = this->allocMissile(x1, y1, z1, x2, y2, z2, n19, renderMode);

    if (this->attackerWeaponProj == Enums::WP_PROJ_FLAME || this->attackerWeaponProj == Enums::WP_PROJ_LIGHTNING) {
        if (this->curAttacker == nullptr) {
            app->render->mapSprites[app->render->S_ENT + allocMissile->sprite] = app->player->getPlayerEnt()->getIndex();
            app->render->relinkSprite(allocMissile->sprite, allocMissile->pos[0] << 4, allocMissile->pos[1] << 4, allocMissile->pos[2] << 4);
        }
        else {
            app->render->mapSprites[app->render->S_ENT + allocMissile->sprite] = this->curAttacker->getIndex();
            app->render->relinkSprite(allocMissile->sprite, allocMissile->pos[3] << 4, allocMissile->pos[4] << 4, allocMissile->pos[5] << 4);
        }
        allocMissile->flags |= GameSprite::FLAG_NORELINK;
        allocMissile->pos[0] = allocMissile->pos[3];
        allocMissile->pos[1] = allocMissile->pos[4];
        allocMissile->pos[2] = allocMissile->pos[5];
        allocMissile->vel[0] = allocMissile->vel[1] = allocMissile->vel[2] = 0;

        if (this->attackerWeaponProj == Enums::WP_PROJ_LIGHTNING && this->curTarget != nullptr && this->curTarget->def->eType == Enums::ET_MONSTER && (this->crFlags & Enums::CR_HITMASK)) {
            short n24 = allocMissile->pos[0];
            short n25 = allocMissile->pos[1];
            for (int i = 0; i < 8; ++i) {
                Entity* mapEntity = app->game->findMapEntity(n24 + app->canvas->viewStepValues[i << 1], n25 + app->canvas->viewStepValues[(i << 1) + 1], 4);
                if (mapEntity != nullptr && !(mapEntity->monster->flags & Enums::MFLAG_NPC_MONSTER)) {
                    int sprite = mapEntity->getSprite();
                    short n26 = app->render->mapSprites[app->render->S_X + sprite];
                    short n27 = app->render->mapSprites[app->render->S_Y + sprite];
                    short n28 = app->render->mapSprites[app->render->S_Z + sprite];
                    GameSprite* allocMissile2 = this->allocMissile(n26, n27, n28, n26, n27, n28, n19, renderMode);
                    allocMissile2->vel[0] = allocMissile2->vel[1] = allocMissile2->vel[2] = 0;
                    allocMissile2->flags |= GameSprite::FLAG_NORELINK;
                    app->render->relinkSprite(allocMissile2->sprite, allocMissile2->pos[3] << 4, allocMissile2->pos[4] << 4, allocMissile2->pos[5] << 4);
                    app->render->mapSprites[app->render->S_ENT + allocMissile2->sprite] = this->curTarget->getIndex();
                    app->render->mapSpriteInfo[allocMissile2->sprite] &= 0xFFF700FF;
                }
            }
        }
    }
    if (!b) {
        int sprite = allocMissile->sprite;
        app->render->mapSpriteInfo[allocMissile->sprite] &= 0xFFF700FF;
    }
    this->exploded = false;
}

GameSprite* Combat::allocMissile(int x1, int y1, int z1, int x2, int y2, int z2, int duration, int renderMode) {
    Applet* app = CAppContainer::getInstance()->app;
    if (this->numActiveMissiles == 8) {
        app->Error("MAX_ACTIVE_MISSILES", Enums::ERR_MAX_MISSILES);
        return nullptr;
    }
    int n8 = app->render->mediaMappings[this->missileAnim + 1] - app->render->mediaMappings[this->missileAnim];
    int numActiveMissiles = this->numActiveMissiles;
    GameSprite* gameSprite = app->game->gsprite_alloc(this->missileAnim, n8, 2562);
    this->activeMissiles[this->numActiveMissiles++] = gameSprite;

    short* mapSprites = app->render->mapSprites;
    mapSprites[app->render->S_ENT + gameSprite->sprite] = -1;
    mapSprites[app->render->S_RENDERMODE + gameSprite->sprite] = (short)renderMode;
    mapSprites[app->render->S_X + gameSprite->sprite] = (gameSprite->pos[0] = (short)x1);
    mapSprites[app->render->S_Y + gameSprite->sprite] = (gameSprite->pos[1] = (short)y1);
    mapSprites[app->render->S_Z + gameSprite->sprite] = (gameSprite->pos[2] = (short)z1);
    app->render->mapSpriteInfo[gameSprite->sprite] |= 0x80000;
    gameSprite->pos[3] = (short)x2;
    gameSprite->pos[4] = (short)y2;
    gameSprite->pos[5] = (short)z2;
    gameSprite->duration = duration;
    if (this->attackerWeaponProj == Enums::WP_PROJ_ITEM) {
        gameSprite->flags |= GameSprite::FLAG_LERP_PARABOLA;
        int n9 = 8;
        if (gameSprite->pos[5] > gameSprite->pos[2]) {
            n9 /= 4;
        }
        gameSprite->pos[5] += (short)(std::min(this->tileDist, 5) * n9);
        if (gameSprite->pos[5] < gameSprite->pos[2]) {
            gameSprite->pos[5] = gameSprite->pos[2];
        }
    }
    if (duration != 0) {
        gameSprite->vel[0] = 1000 * (gameSprite->pos[3] - gameSprite->pos[0]) / duration;
        gameSprite->vel[1] = 1000 * (gameSprite->pos[4] - gameSprite->pos[1]) / duration;
        gameSprite->vel[2] = 1000 * (gameSprite->pos[5] - gameSprite->pos[2]) / duration;
    }
    else {
        gameSprite->vel[0] = gameSprite->vel[1] = gameSprite->vel[2] = 0;
    }
    if (this->missileAnim == 0) {
        app->render->mapSpriteInfo[gameSprite->sprite] |= 0x10000;
        return gameSprite;
    }
    app->render->relinkSprite(gameSprite->sprite);
    return gameSprite;
}
