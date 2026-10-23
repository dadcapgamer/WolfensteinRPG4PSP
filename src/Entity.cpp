#include <stdexcept>
#include <cstring>
#include <climits>

#include "CAppContainer.h"
#include "App.h"
#include "Entity.h"
#include "EntityDef.h"
#include "EntityMonster.h"
#include "Combat.h"
#include "CombatEntity.h"
#include "Render.h"
#include "Game.h"
#include "Canvas.h"
#include "Player.h"
#include "Hud.h"
#include "Text.h"
#include "ParticleSystem.h"
#include "JavaStream.h"
#include "MenuStrings.h"
#include "Enums.h"
#include "Sound.h"

Entity::Entity() {
    memset(this, 0, sizeof(Entity));
}

Entity::~Entity() {
}

bool Entity::startup() {
	//printf("Entity::startup\n");
	return false;
}


bool Entity::hasHead() {
    return (this->def->eType == Enums::ET_MONSTER && this->def->eSubType == Enums::MONSTER_SKELETON && (this->monster->flags & Enums::MFLAG_ABILITY)) ? false : true;
}

void Entity::reset() {
	this->def = nullptr;
	this->prevOnTile = nullptr;
	this->nextOnTile = nullptr;
	this->linkIndex = 0;
	this->info = 0;
	this->param = 0;
	this->monster = nullptr;
	this->name = -1;
}

void Entity::initspawn() {
    Applet* app = CAppContainer::getInstance()->app;

    uint8_t eType = this->def->eType;
    uint8_t eSubType = this->def->eSubType;
    this->name = Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, this->def->name);
    int sprite = this->getSprite();

    int tileNum = app->render->mapSpriteInfo[sprite] & 0xFF;
    if (eType == Enums::ET_MONSTER) {
        app->combat->monsters[eSubType * 3 + (int8_t)this->def->parm]->clone(&this->monster->ce);

        if (app->game->difficulty == 4 || (app->game->difficulty == 2 && this->def->eSubType < Enums::FIRSTBOSS)) {
            int stat = this->monster->ce.getStat(Enums::STAT_MAX_HEALTH);
            int n2 = stat + (stat >> 2);
            this->monster->ce.setStat(Enums::STAT_MAX_HEALTH, n2);
            this->monster->ce.setStat(Enums::STAT_HEALTH, n2);
        }
        short renderMode = 0;
        short scaleFactor = 64;
        app->render->mapSprites[app->render->S_Z + sprite] = (short)(32 + app->render->getHeight(app->render->mapSprites[app->render->S_X + sprite], app->render->mapSprites[app->render->S_Y + sprite]));
        app->render->relinkSprite(sprite);

        if (eSubType == Enums::MONSTER_ZOMBIE) {
            if (this->def->parm == 0) {
                scaleFactor = 50;
            }
            else if (this->def->parm == 1) {
                scaleFactor = 58;
            }
            else if (this->def->parm == 2) {
                scaleFactor = 66;
            }
        }
        else if (eSubType == Enums::MONSTER_CHICKEN) {
            scaleFactor = 88;
        }

        app->render->mapSprites[app->render->S_SCALEFACTOR + sprite] = scaleFactor;
        app->render->mapSprites[app->render->S_RENDERMODE + sprite] = renderMode;
        this->info |= Entity::ENTITY_FLAG_TAKEDAMAGE;
    }
    else if (eType == Enums::ET_DECOR && eSubType != Enums::DECOR_STATUE) {
        app->render->mapSpriteInfo[sprite] &= ~0x10000;
        if (tileNum == Enums::TILENUM_SWITCH) {
            app->render->mapSprites[app->render->S_SCALEFACTOR + sprite] = 32;
        }
    }
    else if (eType == Enums::ET_ITEM && eSubType == Enums::IT_AMMO && this->def->parm == Enums::AMMO_DYNAMITE) {
        app->render->mapSprites[app->render->S_SCALEFACTOR + sprite] = 32;
    }
    else if (eType == Enums::ET_ATTACK_INTERACTIVE) {
        this->info |= Entity::ENTITY_FLAG_TAKEDAMAGE;
    }
    else if (this->def->eType == Enums::ET_CORPSE && this->def->eSubType == Enums::CORPSE_SKELETON) {
        this->info |= (Entity::ENTITY_FLAG_TAKEDAMAGE | Entity::ENTITY_FLAG_DIRTY);
    }
    else if (eType == Enums::ET_NPC) {
        this->param = 1;
    }
}

int Entity::getSprite() {
	return (this->info & 0xFFFF) - 1; // Enums::SPRITE_MASK_GFX
}

bool Entity::touched(bool b) {
    Applet* app = CAppContainer::getInstance()->app;

    uint8_t eType = this->def->eType;
    //printf("Entity::touched %d -------------------------->\n", eType);
    if (eType == Enums::ET_MONSTERBLOCK_ITEM || eType == Enums::ET_ITEM) {
        if (this->touchedItem(b)) {
            app->game->scriptStateVars[Enums::CODEVAR_PICKUP_ITEM_TILE] = this->def->tileIndex;
            app->game->executeStaticFunc(11);
            if (this->isDroppedEntity()) {
                app->render->mapSprites[app->render->S_ENT + this->getSprite()] = -1;
                this->def = nullptr;
            }
            return true;
        }
    }
    else if (eType == Enums::ET_ENV_DAMAGE) {
        if (this->def->eSubType == Enums::ENV_DAMAGE_FIRE && app->player->buffs[(Enums::OFS_BUFF_COUNT + Enums::BUFF_ANTIFIRE)] == 0) {
            app->player->painEvent(nullptr, false);
            app->player->pain(20, this, true);
            app->player->addStatusEffect(Enums::STATUS_EFFECT_FIRE, 5, 3);
            app->player->translateStatusEffects();
        }
        else if (this->def->eSubType == Enums::ENV_DAMAGE_SPIKES) {
            app->player->painEvent(nullptr, false);
            app->player->pain(20, this, true);
            app->sound->playSound(1103, 0, 3, false);
        }
        return true;
    }
    return false;
}

bool Entity::touchedItem(bool b) {
    Applet* app = CAppContainer::getInstance()->app;

    if (this->def->eSubType == Enums::IT_INVENTORY || this->def->eSubType == Enums::IT_ARMOR) {
        int param = 1;
        if (this->isDroppedEntity()) {
            param = this->param;
        }
        
        if (!app->player->give(this->def->eSubType, this->def->parm, param, false)) {
            Text* messageBuffer = app->hud->getMessageBuffer(2);
            app->localization->resetTextArgs();
            app->localization->addTextArg(Strings::FILE_ENTITYSTRINGS, this->def->name);
            app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::CANT_HOLD_MORE, messageBuffer);
            app->hud->finishMessageBuffer();
            return false;
        }
        app->localization->resetTextArgs();
        if (this->def->eSubType == Enums::IT_INVENTORY && (this->def->parm == Enums::INV_OTHER_SILVER_KEY || this->def->parm == Enums::INV_OTHER_GOLD_KEY)) {
            app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::GOT_KEY, app->hud->getMessageBuffer(3));
            app->hud->repaintFlags |= 0x4;
        }
        else {
            Text* messageBuffer3 = app->hud->getMessageBuffer(1);
            app->localization->addTextArg(Strings::FILE_ENTITYSTRINGS, this->def->longName);
            app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::GOT_X, messageBuffer3);
        }
        app->hud->finishMessageBuffer();
        if (!this->isDroppedEntity() && this->def->parm != 18) {
            app->game->foundLoot(this->getSprite(), 1);
        }

        if (b != false) {
            if ((this->def->eSubType == Enums::IT_INVENTORY) && (this->def->parm == Enums::INV_TREASURE_GOBLET || this->def->parm == Enums::INV_TREASURE_CROSS || this->def->parm == Enums::INV_TREASURE_CROWN)) {
                app->sound->playSound(1057, 0, 4, false); // "Item_Treasure.wav"
            }
            else {
                app->sound->playSound(1054, 0, 4, false); // "Item_Pickup.wav"
            }
        }
    }
    else if (this->def->eSubType == Enums::IT_FOOD) {
        int n;
        if (this->def->parm == Enums::FOOD_LARGE || this->def->parm == Enums::FOOD_BURNT) {
            n = 40;
        }
        else {
            n = 20;
        }
        if (!app->player->addHealth(n)) {
            app->hud->addMessage(CodeStrings::HEALTH_AT_MAXIMUM, 2);
            return false;
        }

        if (b != false) {
            app->sound->playSound(1051, 0, 4, false); // "Item_Food_Pickup.wav"
        }
    }
    else if (this->def->eSubType == Enums::IT_AMMO) {
        int param2;
        if (this->isDroppedEntity()) {
            param2 = this->param;
        }
        else {
            param2 = 2 + app->nextInt() % 4;
        }
        if (!app->player->give(Enums::IT_AMMO, this->def->parm, param2, false)) {
            app->hud->addMessage(CodeStrings::AMMO_AT_MAX);
        }
        else {

            if (b != false) {
                app->sound->playSound(1054, 0, 4, false); // "Item_Pickup.wav"
            }

            Text* messageBuffer4 = app->hud->getMessageBuffer(1);
            app->localization->resetTextArgs();
            app->localization->addTextArg(param2);
            Text* smallBuffer2 = app->localization->getSmallBuffer();
            app->localization->composeTextField(this->name, smallBuffer2);
            app->localization->addTextArg(smallBuffer2);
            app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::GOT_X_Y, messageBuffer4);
            app->hud->finishMessageBuffer();
            smallBuffer2->dispose();
            if (!this->isDroppedEntity()) {
                app->game->foundLoot(this->getSprite(), 1);
            }
        }
    }
    else if (this->def->eSubType == Enums::IT_WEAPON) {
        
        app->player->give(Enums::IT_WEAPON, this->def->parm, 1, false);
        int n2 = this->def->parm * Combat::WEAPON_MAX_FIELDS;
        if (app->combat->weapons[n2 + Combat::WEAPON_FIELD_AMMOUSAGE] != 0) {
            if (Entity::CheckWeaponMask(this->def->parm, Enums::WP_SNIPERMASK)) {
                app->player->give(Enums::IT_AMMO, app->combat->weapons[n2 + Combat::WEAPON_FIELD_AMMOTYPE], 8, true);
            }
            else if (this->def->parm == Enums::WP_TESLA) {
                app->player->give(Enums::IT_AMMO, app->combat->weapons[n2 + Combat::WEAPON_FIELD_AMMOTYPE], 100, true);
            }
            else {
                app->player->give(Enums::IT_AMMO, app->combat->weapons[n2 + Combat::WEAPON_FIELD_AMMOTYPE], 10, true);
            }
        }

        if (b != false) {
            app->sound->playSound(1054, 0, 4, false); // "Item_Pickup.wav"
        }

        Text* messageBuffer5 = app->hud->getMessageBuffer(1);
        app->localization->resetTextArgs();
        app->localization->addTextArg(Strings::FILE_ENTITYSTRINGS, this->def->longName);
        app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::GOT_X, messageBuffer5);
        app->hud->finishMessageBuffer();
        app->player->showWeaponHelp(this->def->parm, false);
        
        if (!this->isDroppedEntity()) {
            app->game->foundLoot(this->getSprite(), 1);
        }
    }
    else if (this->def->eSubType == Enums::IT_SACK) {
        app->sound->playSound(1054, 0, 4, false); // "Item_Pickup.wav"
    }

    app->game->removeEntity(this);
    return true;
}

bool Entity::pain(int n, Entity* entity) {
    Applet* app = CAppContainer::getInstance()->app;

    bool b = false;
    int sprite = this->getSprite();
    if (!(this->info & Entity::ENTITY_FLAG_TAKEDAMAGE)) {
        return b;
    }
    if (this->def->eType == Enums::ET_MONSTER) {
        int stat = this->monster->ce.getStat(Enums::STAT_HEALTH);
        int stat2 = this->monster->ce.getStat(Enums::STAT_MAX_HEALTH);
        int n2 = stat - n;

        if ((this->monster->flags & Enums::MFLAG_NOKILL) && n2 <= 0) {
            n2 = 1;
        }

        if (this->isBoss()) {
            int n3 = stat2 >> 1;
            int n4 = n3 >> 1;
            int n5 = stat2 - n4;
            if (n2 <= n5 && n2 + n > n5) {
                b = (app->game->executeStaticFunc(2) != 0);
                if (n2 < n3) {
                    n2 = n3 + 1;
                }
            }
            else if (n2 <= n3 && n2 + n > n3) {
                b = (app->game->executeStaticFunc(3) != 0);
                if (n2 < n4) {
                    n2 = n4 + 1;
                }
            }
            else if (n2 <= n4 && n2 + n > n4) {
                b = (app->game->executeStaticFunc(4) != 0);
                if (n2 < 0) {
                    n2 = 1;
                }
            }
            if (b && (this->monster->flags & Enums::MFLAG_NOTHINK)) {
                app->combat->animLoopCount = 1;
            }
        }

        this->monster->ce.setStat(Enums::STAT_HEALTH, n2);
        if (n2 > 0) {
            if ((app->combat->punchingMonster == 0 || app->combat->curTarget->def->eSubType == Enums::MONSTER_CHICKEN) && !(this->monster->monsterEffects & EntityMonster::MFX_FREEZE)) {
                app->render->mapSpriteInfo[sprite] = ((app->render->mapSpriteInfo[sprite] & ~0xFF00u) | 0x6000);
                this->monster->frameTime = app->time + 250;
            }

            if (this->def->eSubType == Enums::BOSS_HARBINGER && this->monster->goalType == EntityMonster::GOAL_STUN) {
                this->monster->frameTime = (INT_MAX - 6);
            }
            else {
                this->monster->resetGoal();
            }

            if (n2 < stat2 >> 1 && this->def->eSubType == Enums::MONSTER_WORKER && !(this->monster->monsterEffects & EntityMonster::MFX_FREEZE) && (app->nextInt() & 0x3) == 0x0) {
                app->localization->resetTextArgs();
                Text* smallBuffer = app->localization->getSmallBuffer();
                app->localization->composeTextField(this->name, smallBuffer);
                app->localization->addTextArg(smallBuffer);
                app->hud->addMessage(CodeStrings::X_FLEES);
                smallBuffer->dispose();
                this->monster->resetGoal();
                this->monster->goalType = EntityMonster::GOAL_FLEE;
                this->monster->goalParam = (short)(2 + (app->nextInt() & 0xFF) % 3);
            }
        }
        else if ((this->monster->monsterEffects & EntityMonster::MFX_FREEZE) == 0x0) {
            app->render->mapSpriteInfo[sprite] = ((app->render->mapSpriteInfo[sprite] & ~0xFF00u) | 0x6000);
            if (app->combat->animLoopCount > 0) {
                this->monster->frameTime = app->time + 250;
            }
            if (app->combat->punchingMonster != 0) {
                this->monster->frameTime = app->combat->animEndTime + 200;
            }
            else {
                this->monster->frameTime = app->time + 250 + 200;
            }
            app->render->chatZoom = false;
        }
    }
    else if (this->isExplodableEntity()) {
        short x = app->render->mapSprites[app->render->S_X + sprite];
        short y = app->render->mapSprites[app->render->S_Y + sprite];
        short z = app->render->mapSprites[app->render->S_Z + sprite];
        if (!app->game->isUnderWater()) {
            app->game->gsprite_allocAnim(Enums::TILENUM_ANIM_FIRE, x, y, z);
        }
        GameSprite* gameSprite = app->game->gsprite_allocAnim(Enums::TILENUM_ANIM_EXPLOSION, x, y, z);
        gameSprite->data = this;
        gameSprite->flags |= GameSprite::FLAG_UNLINK;
        app->game->unlinkEntity(gameSprite->data);
        ++app->game->animatingEffects;
    }
    else if (this->def->eType == Enums::ET_ATTACK_INTERACTIVE) {
        uint8_t eSubType = this->def->eSubType;
        if (eSubType == Enums::INTERACT_STATUE) {
            app->render->mapSpriteInfo[sprite] = ((app->render->mapSpriteInfo[sprite] & ~0xFF00u) | 0x100);
            this->param = app->upTimeMs + 200;
        }
        else if (eSubType == Enums::INTERACT_BARRICADE) {
            app->render->mapSpriteInfo[sprite] = ((app->render->mapSpriteInfo[sprite] & ~0xFF00u) | 0x100);
            app->particleSystem->spawnParticles(2, 0xFF795207, sprite);
            app->sound->playSound(1033, 0, 3, false);
            app->game->removeEntity(this);
        }
        else if (eSubType == Enums::INTERACT_CHICKEN) {
            app->particleSystem->spawnParticles(1, -1, sprite);
            this->died(false, entity);
        }
        else if (eSubType == Enums::INTERACT_PAINTING) {
            int n9 = 1;
            if (app->combat->curTarget == this && CheckWeaponMask(app->player->ce->weapon, Enums::WP_PAINTINGMASK)) {
                ++this->param;
                n9 = ((this->param > 2) ? 1 : 0);
            }
            if (n9 != 0) {
                app->particleSystem->spawnParticles(2, 0xFF795207, sprite);
                app->sound->playSound(1033, 0, 3, 0);
                app->game->removeEntity(this);
                this->info |= Entity::ENTITY_FLAG_DIRTY;
                app->render->mapSpriteInfo[sprite] |= 0x10000;
            }
        }
        else {
            if (eSubType == Enums::INTERACT_CRATE || eSubType == Enums::INTERACT_CHAIR) {
                app->particleSystem->spawnParticles(2, 0xFF795207, sprite);
                app->sound->playSound(1033, 0, 3, 0);
            }
            else {
                if (eSubType == Enums::INTERACT_PICKUP || eSubType == Enums::INTERACT_SINK) {
                    app->particleSystem->spawnParticles(1, 0xFFFFFFFF, sprite);
                    app->game->unlinkEntity(this);
                    app->render->mapSpriteInfo[sprite] = app->render->mapSpriteInfo[sprite] & ~0xFF00u | 0x200;
                    this->info |= Entity::ENTITY_FLAG_DIRTY;
                    return b;
                }
                if (eSubType == Enums::INTERACT_FLAG) {
                    app->particleSystem->spawnParticles(1, 0xFF911C25, sprite);
                }
            }
            app->game->removeEntity(this);
            this->info |= Entity::ENTITY_FLAG_DIRTY;
            app->render->mapSpriteInfo[sprite] |= 0x10000;
        }
    }
    return b;
}

void Entity::checkMonsterDeath(bool b) {
    Applet* app = CAppContainer::getInstance()->app;

    if ((this->monster != nullptr) && !(this->monster->flags & Enums::MFLAG_NOTRACK)) {
        app->player->fillMonsterStats();

        if (b) {
            int calcXP = this->monster->ce.calcXP();
            if (this->isBoss()) {
                calcXP += 130;
            }
            app->player->addXP(calcXP);
        }

        if (app->player->monsterStats[0] == app->player->monsterStats[1] && !(app->player->killedMonstersLevels & 1 << (app->canvas->loadMapID - 1))) {
            app->player->giveStandardMedal(app->canvas->loadMapID, 2);
            app->player->addXP(app->player->calcLevelXP(app->player->level) - app->player->calcLevelXP(app->player->level - 1) >> 4);
            app->player->killedMonstersLevels |= 1 << (app->canvas->loadMapID - 1);
        }
    }
}

void Entity::died(bool b, Entity* entity) {
    Applet* app = CAppContainer::getInstance()->app;

    int sprite = this->getSprite();
    short n = app->render->mapSprites[app->render->S_X + sprite];
    short n2 = app->render->mapSprites[app->render->S_Y + sprite];
    int n3 = app->render->mapSpriteInfo[sprite];
    if (!(this->info & Entity::ENTITY_FLAG_TAKEDAMAGE) || (this->monster != nullptr && (this->monster->flags & Enums::MFLAG_NOKILL) != 0x0)) {
        return;
    }
    this->info &= ~Entity::ENTITY_FLAG_TAKEDAMAGE;
    uint8_t  eType = this->def->eType;
    uint8_t  eSubType = this->def->eSubType;
    
    if (this->isExplodableEntity()) {
        if (app->canvas->state == 1) {
            app->canvas->setState(3);
        }
        int n4 = 40;
        if (eType == Enums::ET_DECOR_NOCLIP && eSubType == Enums::DECOR_DYNAMITE) {
            uint8_t b2 = app->combat->weapons[99];
            n4 = b2 + ((app->combat->weapons[100] - b2) * app->nextByte() >> 8);
        }
        --app->game->animatingEffects;
        app->sound->playSound(1126, 0, 4, 0);
        if (!app->game->isCameraActive()) {
            app->combat->hurtEntityAt(n >> 6, n2 >> 6, n >> 6, n2 >> 6, 0, n4, this, true);
            app->combat->radiusHurtEntities(n >> 6, n2 >> 6, 1, n4 / 2, this);
        }
        app->game->executeTile(n >> 6, n2 >> 6, 20468, true);
        n3 |= 0x10000;
        app->game->removeEntity(this);
        if (this->isDroppedEntity()) {
            this->info &= ~Entity::ENTITY_FLAG_DIRTY;
        }
        else {
            this->info |= Entity::ENTITY_FLAG_DIRTY;
        }
        if (eType == Enums::ET_ATTACK_INTERACTIVE) {
            app->game->destroyedObject(sprite);
        }
        app->canvas->startShake(500, 4, 500);
    }
    else if (eType == Enums::ET_ATTACK_INTERACTIVE) {
        app->localization->resetTextArgs();
        Text* smallBuffer = app->localization->getSmallBuffer();
        app->localization->composeTextField(this->name, smallBuffer);
        app->localization->addTextArg(smallBuffer);
        app->hud->addMessage(CodeStrings::X_DESTROYED);
        smallBuffer->dispose();
        app->player->addXP(5);

        if (eSubType == Enums::INTERACT_STATUE) {
            this->info |= Entity::ENTITY_FLAG_DIRTY;
            app->game->unlinkEntity(this);
            app->sound->playSound(1058, 0, 5, 0);
        }
        else if (eSubType == Enums::INTERACT_CHICKEN) {
            this->dropChickenPlate(entity);
            app->game->removeEntity(this);
            this->info |= Entity::ENTITY_FLAG_DIRTY;
            n3 |= 0x10000;
        }
        if (this->def->eSubType != Enums::INTERACT_CHICKEN && this->def->eSubType != Enums::INTERACT_PICKUP) {
            app->game->destroyedObject(sprite);
        }
    }
    else if (eType == Enums::ET_CORPSE) {
        n3 |= 0x10000;
        this->info &= ~(Entity::ENTITY_FLAG_GIBBED | Entity::ENTITY_FLAG_TAKEDAMAGE | Entity::ENTITY_FLAG_GHOST | Entity::ENTITY_FLAG_DIRTY);
        this->info |= (Entity::ENTITY_FLAG_GIBBED | Entity::ENTITY_FLAG_DIRTY);
        if (this->monster != nullptr) {
            this->monster->monsterEffects = EntityMonster::MFX_NONE;
        }
        app->player->counters[4]++;
        app->game->unlinkEntity(this);
    }
    else if (eType == Enums::ET_MONSTER) {
        int monsterSound = app->game->getMonsterSound(eSubType, Enums::MSOUND_DEATH);
        if (eSubType == Enums::MONSTER_SOLDIER && (int16_t)app->nextByte() > 127) {
            monsterSound = 1101;
        }
        app->sound->playSound(monsterSound, 0, 5, 0);

        this->info |= Entity::ENTITY_FLAG_DIRTY;
        this->monster->resetGoal();
        app->game->snapLerpSprites(this->getSprite());
        n3 = ((n3 & 0xFFFF00FF) | 0x7000);
        this->monster->frameTime = app->time;
        if (this->info & Entity::ENTITY_FLAG_GIBBED) {
            n3 |= 0x17000;
        }
        else {
            this->info |= (Entity::ENTITY_FLAG_TAKEDAMAGE | Entity::ENTITY_FLAG_CORPSE);
            this->trimCorpsePile(n, n2);
        }
        if ((this->monster->monsterEffects & EntityMonster::MFX_FREEZE) || (this->monster->monsterEffects & EntityMonster::MFX_POISON)) {
            this->monster->clearEffects();
        }
        else {
            this->monster->monsterEffects &= EntityMonster::MFX_REMOVE_TURNS;
            this->monster->monsterEffects |= EntityMonster::MFX_ALL_ONE_TURNS;
        }
        if (app->game->difficulty == 4 && !(this->monster->flags & Enums::MFLAG_NORAISE)) {
            int n5 = 2 + app->nextInt() % 3;
            this->monster->monsterEffects &= EntityMonster::MFX_RAISE_REMOVE;
            this->monster->monsterEffects |= n5 << EntityMonster::MFX_RAISE_SHIFT;
            this->monster->monsterEffects |= EntityMonster::MFX_RAISE_TIMER;
        }
        app->game->deactivate(this);
        this->undoAttack();
        if (this->isBoss()) {
            app->player->inCombat = false;
            app->canvas->field_0xacd_ = app->canvas->field_0xacc_;
            app->game->executeStaticFunc(5);
        }
        else if (eSubType == Enums::MONSTER_TORMENTOR) {
            app->game->gsprite_allocAnim(192, n, n2, app->render->mapSprites[app->render->S_Z + sprite]);
            n3 |= 0x10000;
        }
        else if (eSubType == Enums::MONSTER_CHICKEN) {
            n3 |= 0x10000;
        }
        this->checkMonsterDeath(b);
        if ((this->info & Entity::ENTITY_FLAG_GIBBED) != 0x0 || eSubType == Enums::MONSTER_TORMENTOR || eSubType == Enums::MONSTER_CHICKEN) {
            this->info &= ~(Entity::ENTITY_FLAG_GIBBED | Entity::ENTITY_FLAG_TAKEDAMAGE | Entity::ENTITY_FLAG_CORPSE);
            this->info |= Entity::ENTITY_FLAG_GIBBED;
            app->game->unlinkEntity(this);
        }
        this->def = app->entityDefManager->find(Enums::ET_CORPSE, eSubType, this->def->parm);
        this->name = Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, this->def->name);

        if (b && !(this->monster->flags & Enums::MFLAG_NOTRACK) && !(this->info & Entity::ENTITY_FLAG_DEATHFUNC) && !(this->info & Entity::ENTITY_FLAG_GHOST)) {
            if (app->canvas->state != Canvas::ST_DRIVING) {
                if (eSubType == 10) {
                    this->dropChickenPlate(entity);
                }
                else {
                    this->dropItem(0);
                }
            }
        }
        app->canvas->invalidateRect();

        short index = this->getIndex();
        int* placedBombs = app->game->placedBombs;
        for (int i = 0; i < Game::MAX_BOMBS; ++i) {
            if (placedBombs[i] != 0) {
                Entity* entity2 = &app->game->entities[placedBombs[i]];
                int n7 = (entity2->param & 0x7FFFFF00) >> 8;
                if (n7 != 0) {
                    if (n7 == index) {
                        int sprite2 = entity2->getSprite();
                        app->render->mapSprites[app->render->S_Z + sprite2] = (short)(app->render->getHeight(app->render->mapSprites[app->render->S_X + sprite2], app->render->mapSprites[app->render->S_Y + sprite2]) + 32);
                    }
                }
            }
        }

        if (app->game->difficulty == 4 && !(this->monster->flags & Enums::MFLAG_NORAISE) && !(this->info & Entity::ENTITY_FLAG_GHOST)) {
            int n6 = 2 + app->nextInt() % 3;
            this->monster->monsterEffects |= n6 << EntityMonster::MFX_RAISE_SHIFT;
            this->monster->monsterEffects |= EntityMonster::MFX_RAISE_TIMER;
        }
        else if (this->info & Entity::ENTITY_FLAG_GHOST) {
            n3 |= 0x10000;
            this->info &= ~(Entity::ENTITY_FLAG_GIBBED | Entity::ENTITY_FLAG_GHOST | Entity::ENTITY_FLAG_DIRTY);
            this->info |= (Entity::ENTITY_FLAG_GIBBED | Entity::ENTITY_FLAG_DIRTY);
            ++app->player->counters[4];
            app->game->unlinkEntity(this);
            GameSprite* gsprite_allocAnim = app->game->gsprite_allocAnim(Enums::TILENUM_ANIM_SMOKE2, app->render->mapSprites[app->render->S_X + sprite], app->render->mapSprites[app->render->S_Y + sprite], app->render->mapSprites[app->render->S_Z + sprite] - 20);
            gsprite_allocAnim->flags |= GameSprite::FLAG_SCALE;
            gsprite_allocAnim->startScale = 96;
            gsprite_allocAnim->destScale = 127;
            gsprite_allocAnim->scaleStep = 38;
        }
    }
    if (this->info & Entity::ENTITY_FLAG_DEATHFUNC) {
        app->game->executeEntityFunc(this, this->deathByExplosion(entity));
        this->info &= ~Entity::ENTITY_FLAG_DEATHFUNC;
    }
    app->render->mapSpriteInfo[sprite] = n3;
    app->canvas->updateFacingEntity = true;
}

void Entity::dropChickenPlate(Entity* entity) {
    Applet* app = CAppContainer::getInstance()->app;
    int n = 0;
    if (entity != nullptr) {
        uint8_t eType = entity->def->eType;
        uint8_t eSubType = entity->def->eSubType;
        if (this->isExplodableEntity() ||
            (eType == Enums::ET_ENV_DAMAGE && eSubType == Enums::ENV_DAMAGE_FIRE) ||
            (entity == app->player->getPlayerEnt() && (CheckWeaponMask(app->player->ce->weapon, Enums::WP_FIREMASK) || CheckWeaponMask(app->player->ce->weapon, Enums::WP_EXPLOSIONMASK)))) {
            n = 1;
        }
    }
    this->dropItem(n);
}

bool Entity::deathByExplosion(Entity* entity) {
    Applet* app = CAppContainer::getInstance()->app;

    bool b = false;
    if (entity == app->player->getPlayerEnt() && app->player->ce->weapon == Enums::WP_PANZER) {
        b = true;
    }
    else if (entity != nullptr && entity->isExplodableEntity()) {
        b = true;
    }
    return b;
}

void Entity::dropItem(int n) {
    Applet* app = CAppContainer::getInstance()->app;

    int sprite = this->getSprite();
    int n2 = 0;
    uint8_t eSubType = this->def->eSubType;
    int n3 = 1;
    bool b = true;
    short nextByte = app->nextByte();
    short nextByte2 = app->nextByte();
    if ((this->def->eType == Enums::ET_CORPSE && this->def->eSubType == Enums::MONSTER_CHICKEN) || 
        (this->def->eType == Enums::ET_ATTACK_INTERACTIVE && eSubType == Enums::INTERACT_CHICKEN)) {
        if (n == 1) {
            n2 = 118;
        }
        else {
            n2 = 117;
        }
        b = false;
    }
    else if (this->def->eType == Enums::ET_CORPSE) {
        if (!app->player->god && nextByte < 145) {
            return;
        }
        if (eSubType == Enums::MONSTER_SOLDIER || eSubType == Enums::MONSTER_SOLDIER_DUMB) {
            if (nextByte2 < 70) {
                n2 = 113;
            }
            else if (this->def->parm == 2) {
                if (app->game->difficulty == 4) {
                    n2 = 116;
                }
                else {
                    n2 = 89;
                    n3 = 2;
                }
            }
            else if (nextByte2 < 204 || eSubType == Enums::MONSTER_SOLDIER_DUMB || app->game->difficulty == 4) {
                n2 = 85;
                n3 = 4;
            }
            else {
                n2 = 5;
            }
        }
        else if (eSubType == Enums::MONSTER_OFFICER) {
            if (app->game->difficulty == 4 && this->def->parm == 2) {
                n2 = 116;
            }
        }
        else if (eSubType == Enums::MONSTER_ELITE_GUARD) {
            if (app->game->difficulty == 4) {
                if (nextByte2 > 100) {
                    if (this->def->parm > 0) {
                        n2 = 116;
                    }
                    else {
                        n2 = 113;
                    }
                }
                else {
                    n2 = 85;
                    n3 = 4;
                }
            }
            else if ((nextByte2 & 0x1) == 0x0) {
                n2 = 113;
            }
            else {
                n2 = 85;
                n3 = 4;
            }
        }
        else if (eSubType == Enums::MONSTER_TROOPER) {
            if (app->game->difficulty != 4) {
                if (this->def->parm == 0) {
                    n2 = 90;
                    n3 = 4;
                }
                else {
                    n2 = 86;
                    n3 = 9;
                }
            }
            else {
                n2 = 116;
            }
        }
        else if (eSubType == Enums::MONSTER_SUPERS && app->game->difficulty != 4) {
            if (this->def->parm == 0) {
                n2 = 91;
                n3 = 3;
            }
            else {
                n2 = 86;
                n3 = 9;
            }
        }
    }
    if (n2 != 0) {
        EntityDef* lookup = app->entityDefManager->lookup(n2);
        Render* render = app->render;
        app->game->spawnDropItem(render->mapSprites[render->S_X + sprite], render->mapSprites[render->S_Y + sprite], n2, 6, lookup->eSubType, lookup->parm, n3, b);
    }
}

void Entity::aiCalcSimpleGoal(bool b) {
    Applet* app = CAppContainer::getInstance()->app;

    if (this->monster->flags & Enums::MFLAG_NPC_MONSTER) {
        this->monster->flags &= ~Enums::MFLAG_NPC_CHAT;
        this->monster->goalType = EntityMonster::GOAL_FLEE;
        this->monster->goalParam = 3;
        return;
    }

    if (this->def->eSubType == Enums::MONSTER_TORMENTOR) {
        if (this->aiCalcTormentorGoal()) {
            return;
        }
    }
    else if (this->def->eSubType == Enums::BOSS_HARBINGER) {
        if (this->aiCalcHarbingerGoal()) {
            return;
        }
    }

    if (app->player->buffs[(Enums::OFS_BUFF_COUNT + Enums::BUFF_FEAR)] > 0 && !(1 << this->def->eSubType & Enums::FEAR_IMMUNE_MONSTERS)) {
        this->monster->goalType = EntityMonster::GOAL_FLEE;
        this->monster->goalParam = 1;
        return;
    }

    if ((1 << this->def->eSubType & Enums::DYNAMITE_SCARED_MONSTERS)) {
        short index = this->getIndex();
        int* placedBombs = app->game->placedBombs;
        for (int i = 0; i < Game::MAX_BOMBS; ++i) {
            if (placedBombs[i] != 0 && (app->game->entities[placedBombs[i]].param & 0x7FFFFF00) >> 8 == index) {
                this->monster->goalType = EntityMonster::GOAL_FLEE;
                this->monster->goalParam = 1;
                return;
            }
        }
    }
    int aiWeaponForTarget = this->aiWeaponForTarget(&app->game->entities[1]);
    bool b2 = false;
    if (aiWeaponForTarget != -1) {
        this->monster->ce.weapon = aiWeaponForTarget;
        b2 = true;
    }
    if (b2) {
        this->monster->goalType = EntityMonster::GOAL_FIGHT;
        this->monster->goalParam = 1;
        if ((1 << this->def->eSubType & Enums::EVADING_MONSTERS)) {
            this->monster->flags |= Enums::MFLAG_ABILITY;
        }
        if (!b && this->def->eSubType == Enums::MONSTER_SOLDIER_DUMB && app->nextByte() < 30) {
            this->monster->goalType = EntityMonster::GOAL_EVADE;
            this->monster->goalParam = 1;
        }
    }
    else if (this->def->eSubType == Enums::MONSTER_WORKER && (this->monster->flags & Enums::MFLAG_WEAPON_ALT)) {
        this->monster->goalType = EntityMonster::GOAL_FLEE;
        this->monster->goalTurns = 10;
        this->monster->goalParam = 1;
    }
    else {
        this->monster->goalType = EntityMonster::GOAL_MOVETOENTITY;
        this->monster->goalParam = 1;
        if ((1 << this->def->eSubType & Enums::EVADING_MONSTERS)) {
            this->monster->flags &= ~Enums::MFLAG_ABILITY;
        }
    }
}

bool Entity::aiCalcTormentorGoal() {
    Applet* app = CAppContainer::getInstance()->app;
    if (app->nextByte() <= 256 - (this->monster->ce.getStat(Enums::STAT_HEALTH) << 8) / this->monster->ce.getStat(Enums::STAT_MAX_HEALTH)) {
        Entity** gridEntities = app->game->gridEntities;
        int x = this->linkIndex % 32 << 6;
        int y = this->linkIndex / 32 << 6;
        gridEntities[0] = app->game->findMapEntity(x, y, 512);
        gridEntities[1] = app->game->findMapEntity(x - 64, y, 512);
        gridEntities[2] = app->game->findMapEntity(x + 64, y, 512);
        gridEntities[3] = app->game->findMapEntity(x, y - 64, 512);
        gridEntities[4] = app->game->findMapEntity(x, y + 64, 512);
        gridEntities[5] = app->game->findMapEntity(x - 64, y - 64, 512);
        gridEntities[6] = app->game->findMapEntity(x - 64, y + 64, 512);
        gridEntities[7] = app->game->findMapEntity(x + 64, y - 64, 512);
        gridEntities[8] = app->game->findMapEntity(x + 64, y + 64, 512);
        for (int i = 0; i < 9; ++i) {
            if (gridEntities[i] != nullptr) {
                app->game->trace(x + 32, y + 32, (gridEntities[i]->linkIndex % 32 << 6) + 32, (gridEntities[i]->linkIndex / 32 << 6) + 32, this, Enums::CONTENTS_MONSTERSOLID, 16);
                if (app->game->numTraceEntities == 0) {
                    if (i == 0) {
                        this->monster->goalType = EntityMonster::GOAL_FIGHT;
                        this->monster->ce.weapon = Enums::WP_M_TORMENTOR_ABSORB;
                    }
                    else {
                        this->monster->goalType = EntityMonster::GOAL_MOVETOENTITY;
                    }
                    this->monster->goalParam = gridEntities[i]->getIndex();
                    return true;
                }
            }
        }
    }
    return false;
}

bool Entity::aiCalcHarbingerGoal() {
    Applet* app = CAppContainer::getInstance()->app;
    ++this->param;
    if (app->nextByte() > 256 - (this->monster->ce.getStat(Enums::STAT_HEALTH) << 8) / this->monster->ce.getStat(Enums::STAT_MAX_HEALTH) || this->param < 7) {
        return false;
    }
    Entity* entity = app->game->findMapEntity(app->game->HARBINGER_BLOOD_POOLX, app->game->HARBINGER_BLOOD_POOLY);
    Entity* entity2 = nullptr;
    bool b = false;
    while (entity != nullptr) {
        if (entity->def->eType == Enums::ET_DECOR_NOCLIP) {
            entity2 = entity;
        }
        else if (entity->def->eType == Enums::ET_PLAYER) {
            b = true;
        }
        entity = entity->nextOnTile;
    }
    if (b) {
        return false;
    }
    int* calcPosition = entity2->calcPosition();
    if (calcPosition[0] >> 6 != this->linkIndex % 32 || calcPosition[1] >> 6 != this->linkIndex / 32) {
        this->monster->goalType = EntityMonster::GOAL_MOVETOENTITY;
        this->monster->goalParam = entity2->getIndex();
    }
    else {
        this->monster->goalType = EntityMonster::GOAL_FIGHT;
        this->monster->goalParam = entity2->getIndex();
        this->monster->ce.weapon = Enums::WP_M_BOSS_HARBINGER_SUCK;
    }
    return true;
}

void Entity::aiMoveToGoal() {
    Applet* app = CAppContainer::getInstance()->app;

    uint8_t goalType = this->monster->goalType;
    EntityMonster* monster = this->monster;
    if (goalType == EntityMonster::GOAL_MOVETOENTITY || 
        goalType == EntityMonster::GOAL_MOVE || 
        goalType == EntityMonster::GOAL_FLEE || 
        goalType == EntityMonster::GOAL_EVADE) {
        if (!this->aiGoal_MOVE() && this->def->eSubType == Enums::MONSTER_SOLDIER_DUMB && goalType == EntityMonster::GOAL_EVADE) {
            this->aiChooseNewGoal(false);
            this->aiMoveToGoal();
        }
    }
    else if (goalType == EntityMonster::GOAL_FIGHT) {
        if (monster->goalParam == 1) {
            monster->target = nullptr;
        }
        else {
            monster->target = &app->game->entities[monster->goalParam];
        }
        this->attack();
    }
}

void Entity::aiChooseNewGoal(bool b) {
    uint8_t eSubType = this->def->eSubType;
    this->monster->resetGoal();
    this->aiCalcSimpleGoal(b);
    if ((1 << eSubType & Enums::EVADING_MONSTERS) != 0x0 && this->monster->goalType == EntityMonster::GOAL_FIGHT) {
        this->monster->goalFlags |= EntityMonster::GFL_ATTACK2EVADE;
    }
}

bool Entity::aiIsValidGoal() {
    Applet* app = CAppContainer::getInstance()->app;

    uint8_t goalType = this->monster->goalType;
    if (this->monster->goalTurns >= 16 || goalType == EntityMonster::GOAL_FIGHT || goalType == EntityMonster::GOAL_NONE) {
        return false;
    }
    if (goalType == EntityMonster::GOAL_MOVETOENTITY) {
        Entity* entity = &app->game->entities[this->monster->goalParam];
        if (this->monster->goalParam != 1) {
            int* calcPosition = entity->calcPosition();
            Entity* mapEntity = app->game->findMapEntity(calcPosition[0], calcPosition[1], Enums::CONTENTS_MONSTERSOLID);
            if (entity->linkIndex != this->linkIndex && (mapEntity == nullptr || mapEntity == entity)) {
                return true;
            }
        }
    }
    else if (goalType == EntityMonster::GOAL_MOVE) {
        if (this->linkIndex != this->monster->goalX + this->monster->goalY * 32) {
            return true;
        }
    }
    else {
        if (goalType == EntityMonster::GOAL_FLEE || goalType == EntityMonster::GOAL_EVADE) {
            return this->monster->goalTurns < this->monster->goalParam;
        }
        if (goalType == EntityMonster::GOAL_STUN) {
            if (this->monster->goalTurns < this->monster->goalParam) {
                return true;
            }
            if (this->monster->goalTurns == this->monster->goalParam) {
                this->monster->resetGoal();
                return false;
            }
        }
    }
    return false;
}

bool Entity::aiIsAttackValid() {
    Applet* app = CAppContainer::getInstance()->app;

    EntityMonster* monster = this->monster;
    int weapon = monster->ce.weapon;
    /*if (weapon <= -1) { //[GEC]
        return false;
    }*/
    int* calcPosition = this->calcPosition();
    app->game->trace(calcPosition[0], calcPosition[1], app->game->destX, app->game->destY, this, 5295, 2);
    Entity* traceEntity = app->game->traceEntity;
    if (traceEntity != nullptr) {
        bool b = app->combat->weapons[(weapon * Combat::WEAPON_MAX_FIELDS) + Combat::WEAPON_FIELD_RANGEMAX] >= app->combat->WorldDistToTileDist(this->distFrom(app->game->destX, app->game->destY));
        bool b2 = false;
        if (b) {
            int n = calcPosition[0] - app->game->destX;
            int n2 = calcPosition[1] - app->game->destY;
            b2 = ((n != 0 || n2 != 0) && (n == 0 || n2 == 0));
        }
        if (monster->target == nullptr && traceEntity->def->eType == Enums::ET_PLAYER && b && b2) {
            return true;
        }
        if (monster->target == traceEntity && b && b2) {
            return true;
        }
    }
    return false;
}

void Entity::aiThink(bool b) {
    EntityMonster* monster = this->monster;
    if (monster->flags & Enums::MFLAG_ATTACKING) {
        monster->flags &= ~Enums::MFLAG_ATTACKING;
    }

    if (monster->flags & Enums::MFLAG_NOTHINK) {
        return;
    }

    if (!this->aiIsValidGoal()) {
        this->aiChooseNewGoal(b);
    }
    monster->goalTurns++;
    this->aiMoveToGoal();
}

int Entity::aiWeaponForTarget(Entity* entity) {
    Applet* app = CAppContainer::getInstance()->app;

    int sprite = this->getSprite();
    int viewX;
    int viewY;
    if (entity->def->eType == Enums::ET_PLAYER) {
        viewX = app->canvas->viewX;
        viewY = app->canvas->viewY;
    }
    else {
        int sprite2 = entity->getSprite();
        viewX = app->render->mapSprites[app->render->S_X + sprite2];
        viewY = app->render->mapSprites[app->render->S_Y + sprite2];
    }
    int n = viewX - app->render->mapSprites[app->render->S_X + sprite];
    int n2 = viewY - app->render->mapSprites[app->render->S_Y + sprite];
    if (n != 0 && n2 != 0) {
        return -1;
    }
    app->game->trace(app->render->mapSprites[app->render->S_X + sprite], app->render->mapSprites[app->render->S_Y + sprite], viewX, viewY, this, Enums::CONTENTS_MONSTERWPSOLID, 2);
    if (app->game->traceEntity != entity) {
        if (this->def->eSubType != 14) {
            return -1;
        }
        app->game->traceEntity = app->game->findMapEntity(app->render->mapSprites[app->render->S_X + sprite], app->render->mapSprites[app->render->S_Y + sprite], 16384);
        if (app->game->traceEntity != entity) {
            return -1;
        }
    }
    int n3 = n;
    int n4 = n2;
    if (n3 != 0) {
        n3 /= std::abs(n);
    }
    if (n4 != 0) {
        n4 /= std::abs(n2);
    }
    app->game->trace(app->render->mapSprites[app->render->S_X + sprite] + n3 * 18, app->render->mapSprites[app->render->S_Y + sprite] + n4 * 18, viewX, viewY, this, 15791, 2);
    bool b = app->game->traceEntity == entity;
    int monsterField = app->combat->getMonsterField(this->def, 0);
    int monsterField2 = app->combat->getMonsterField(this->def, 1);
    if (!b) {
        if (Entity::CheckWeaponMask(monsterField, Enums::WP_ALL_MELEE))  {
            monsterField = 0;
        }
        if (Entity::CheckWeaponMask(monsterField2, Enums::WP_ALL_MELEE)) {
            monsterField2 = 0;
        }
    }
    if (this->def->eSubType == 4) {
        if (this->def->parm == 2 && !(this->monster->flags & Enums::MFLAG_ABILITY)) {
            monsterField2 = 27;
        }
        if (app->player->buffs[(Enums::OFS_BUFF_COUNT + Enums::BUFF_ANTIFIRE)] > 0) {
            if (Entity::CheckWeaponMask(monsterField, Enums::WP_ANTI_FIRE)) {
                monsterField = 0;
            }
            if (Entity::CheckWeaponMask(monsterField2, Enums::WP_ANTI_FIRE)) {
                monsterField2 = 0;
            }
        }
    }
    int n6;
    int n5 = n6 = -1;
    int worldDistToTileDist = app->combat->WorldDistToTileDist(entity->distFrom(app->render->mapSprites[app->render->S_X + sprite], app->render->mapSprites[app->render->S_Y + sprite]));
    if (monsterField == monsterField2) {
        monsterField2 = 0;
    }
    if (this->def->eSubType == 9 && entity->def->eType == Enums::ET_CORPSE) {
        if (worldDistToTileDist == 0) {
            return 43;
        }
        return -1;
    }
    else {
        if (this->def->eSubType != 14 || entity->def->eType != Enums::ET_DECOR_NOCLIP) {
            int8_t* weapons = app->combat->weapons;
            if (monsterField != 0) {
                int n7 = monsterField * Combat::WEAPON_MAX_FIELDS;
                if (weapons[n7 + Combat::WEAPON_FIELD_RANGEMIN] <= worldDistToTileDist && weapons[n7 + Combat::WEAPON_FIELD_RANGEMAX] >= worldDistToTileDist) {
                    n6 = n7;
                }
            }
            if (monsterField2 != 0) {
                int n8 = monsterField2 * Combat::WEAPON_MAX_FIELDS;
                if (weapons[n8 + Combat::WEAPON_FIELD_RANGEMIN] <= worldDistToTileDist && weapons[n8 + Combat::WEAPON_FIELD_RANGEMAX] >= worldDistToTileDist) {
                    n5 = n8;
                }
            }
            if (this->def->eSubType == 2 && this->def->parm == 0) {
                if (!(this->monster->flags & Enums::MFLAG_WEAPON_ALT) && this->monster->ce.getStat(Enums::STAT_HEALTH) < 10) {
                    n6 = -1;
                }
                else if (this->monster->flags & Enums::MFLAG_WEAPON_ALT) {
                    n5 = (n6 = -1);
                }
                else {
                    n5 = -1;
                }
            }
            else if (this->def->eSubType == 14 && !(this->monster->flags & Enums::MFLAG_ABILITY) && n5 != -1 && (int16_t)app->nextByte() <= 127) {
                monsterField2 = 41;
            }

            if (Entity::CheckWeaponMask(monsterField, Enums::WP_CHARGE_ATTACK) || Entity::CheckWeaponMask(monsterField2, Enums::WP_CHARGE_ATTACK)) {
                bool b2 = true;
                int n9 = app->render->mapSprites[app->render->S_X + sprite] >> 6;
                int n10 = app->render->mapSprites[app->render->S_Y + sprite] >> 6;
                do {
                    if ((app->game->baseVisitedTiles[n10] & 1 << n9) != 0) {
                        b2 = false;
                        break;
                    }
                    n9 += n3;
                    n10 += n4;
                } while (--worldDistToTileDist > 0);
                if (Entity::CheckWeaponMask(monsterField, Enums::WP_CHARGE_ATTACK) && !b2) {
                    n6 = -1;
                }
                if (Entity::CheckWeaponMask(monsterField2, Enums::WP_CHARGE_ATTACK) && !b2) {
                    n5 = -1;
                }
            }

            int n11;
            if (n5 != -1 && n6 != -1) {
                n11 = ((app->nextByte() <= app->combat->getMonsterField(this->def, 2)) ? monsterField : monsterField2);
            }
            else if (n5 != -1) {
                n11 = monsterField2;
            }
            else {
                if (n6 == -1) {
                    return -1;
                }
                n11 = monsterField;
            }
            return n11;
        }

        if (worldDistToTileDist == 0) {
                return 42;
        }
        return -1;
    }
}

LerpSprite* Entity::aiInitLerp(int travelTime) {
    Applet* app = CAppContainer::getInstance()->app;

    int sprite = this->getSprite();
    LerpSprite* allocLerpSprite = app->game->allocLerpSprite(nullptr, sprite, true);
    allocLerpSprite->srcX = app->render->mapSprites[app->render->S_X + sprite];
    allocLerpSprite->srcY = app->render->mapSprites[app->render->S_Y + sprite];
    allocLerpSprite->srcZ = app->render->mapSprites[app->render->S_Z + sprite];
    allocLerpSprite->dstX = 32 + (this->monster->goalX << 6);
    allocLerpSprite->dstY = 32 + (this->monster->goalY << 6);
    allocLerpSprite->dstZ = 32 + app->render->getHeight(allocLerpSprite->dstX, allocLerpSprite->dstY);
    allocLerpSprite->srcScale = allocLerpSprite->dstScale = app->render->mapSprites[app->render->S_SCALEFACTOR + sprite];
    allocLerpSprite->startTime = app->gameTime;
    allocLerpSprite->travelTime = travelTime;
    allocLerpSprite->flags |= (Enums::LS_FLAG_ENT_NORELINK | Enums::LS_FLAG_ASYNC);
    this->monster->frameTime = app->time + travelTime;
    allocLerpSprite->calcDist();
    this->monster->goalFlags |= EntityMonster::GFL_LERPING;
    return allocLerpSprite;
}

void Entity::aiFinishLerp() {
    this->monster->goalFlags &= ~EntityMonster::GFL_LERPING;
    if (this->monster->flags & Enums::MFLAG_KNOCKBACK) {
        this->monster->flags &= ~Enums::MFLAG_KNOCKBACK;
    }
    else {
        this->aiReachedGoal_MOVE();
    }
}

bool Entity::checkLineOfSight(int n, int n2, int n3, int n4, int n5) {
    Applet* app = CAppContainer::getInstance()->app;

    int a = n3 - n;
    int a2 = n4 - n2;
    if (a != 0 && a2 != 0) {
        return false;
    }
    if (a != 0) {
        a /= std::abs(a);
    }
    if (a2 != 0) {
        a2 /= std::abs(a2);
    }
    while (n != n3 && n2 != n4) {
        n += a;
        n2 += a2;
        if (app->game->findMapEntity(n << 6, n2 << 6, n5)) {
            return false;
        }
    }
    return true;
}

bool Entity::calcPath(int n, int n2, int n3, int n4, int n5, bool b) {
    Applet* app = CAppContainer::getInstance()->app;

    int n6 = n3 - n;
    int n7 = n4 - n2;
    int closestPathDist = n6 * n6 + n7 * n7;
    uint8_t* visitOrder = app->game->visitOrder;
    int* visitDist = app->game->visitDist;
    app->game->visitedTiles[n2] |= 1 << n;
    bool checkLineOfSight = this->checkLineOfSight(n, n2, n3, n4, n5 | 0x100);
    if ((app->game->lineOfSight == 0 && !checkLineOfSight) || (app->game->lineOfSight == 1 && checkLineOfSight)) {
        if (b) {
            closestPathDist -= 30;
        }
        else {
            closestPathDist += 30;
        }
    }
    if (checkLineOfSight) {
        closestPathDist += app->game->lineOfSightWeight;
    }
    if (app->game->pathDepth > 0 && ((!b && closestPathDist < app->game->closestPathDist) || (b && closestPathDist > app->game->closestPathDist))) {
        app->game->closestPath = app->game->curPath;
        app->game->closestPathDepth = app->game->pathDepth;
        app->game->closestPathDist = closestPathDist;
    }
    if (n == n3 && n2 == n4) {
        app->game->closestPath = app->game->curPath;
        app->game->closestPathDepth = app->game->pathDepth;
        app->game->closestPathDist = closestPathDist;
        return true;
    }
    if (app->game->pathDepth == app->game->pathSearchDepth) {
        return false;
    }
    int n8 = 0;
    const int* viewStepValues = Canvas::viewStepValues;
    int n9 = 4;
    uint8_t b2 = (uint8_t)(app->nextByte() & 0x3);
    while (--n9 >= 0) {
        b2 = (uint8_t)(b2 + 1 & 0x3);
        int n10 = n + (viewStepValues[(b2 << 2) + 0] >> 6);
        int n11 = n2 + (viewStepValues[(b2 << 2) + 1] >> 6);
        if (n11 >= 0 && n11 < 32 && n10 >= 0) {
            if (n10 >= 32) {
                continue;
            }
            if (app->game->findMapEntity(n10 << 6, n11 << 6, 256) != nullptr) {
                continue;
            }
            if ((app->game->visitedTiles[n11] & 1 << n10) != 0) {
                continue;
            }
            visitOrder[n8] = b2;
            int n12 = n3 - n10;
            int n13 = n4 - n11;
            visitDist[n8] = n12 * n12 + n13 * n13;
            bool checkLineOfSight2 = this->checkLineOfSight(n10, n11, n3, n4, n5 | 0x100);
            if ((app->game->lineOfSight == 0 && !checkLineOfSight2) || (app->game->lineOfSight == 1 && checkLineOfSight2)) {
                if (b) {
                    visitDist[n8] -= 30;
                }
                else {
                    visitDist[n8] += 30;
                }
            }
            if (checkLineOfSight2) {
                visitDist[n8] += app->game->lineOfSightWeight;
            }
            ++n8;
        }
    }
    for (int i = 0; i < n8; ++i) {
        for (int j = 0; j < n8 - i - 1; ++j) {
            int n17 = visitDist[j] - visitDist[j + 1];
            if ((!b && n17 > 0) || (b && n17 < 0)) {
                int n18 = visitDist[j + 1];
                visitDist[j + 1] = visitDist[j];
                visitDist[j] = n18;
                uint8_t b3 = visitOrder[j + 1];
                visitOrder[j + 1] = visitOrder[j];
                visitOrder[j] = b3;
            }
        }
    }
    int n19 = 0;
    int n20 = 0;
    for (int k = 0; k < n8; ++k) {
        n19 |= (visitOrder[k] & 0x3) << n20;
        n20 += 2;
    }
    for (int l = 0; l < n8; ++l) {
        int n21 = n19 & 0x3;
        n19 >>= 2;
        int n22 = n + (viewStepValues[(n21 << 2) + 0] >> 6);
        int n23 = n2 + (viewStepValues[(n21 << 2) + 1] >> 6);
        app->game->trace((n << 6) + 32, (n2 << 6) + 32, (n22 << 6) + 32, (n23 << 6) + 32, app->game->skipEnt, n5, 16);
        if (app->game->findEnt != nullptr && app->game->traceEntity == app->game->findEnt) {
            app->game->closestPath = app->game->curPath;
            app->game->closestPathDepth = app->game->pathDepth;
            app->game->closestPathDist = closestPathDist;
            return true;
        }
        int interactClipMask = app->game->interactClipMask;
        if (app->game->traceEntity != nullptr) {
            interactClipMask = 1 << app->game->traceEntity->def->eType;
        }
        if (interactClipMask == 0 || (interactClipMask & app->game->interactClipMask) != 0x0) {
            ++app->game->pathDepth;
            app->game->curPath >>= 2;
            app->game->curPath &= 0x3FFFFFFFFFFFFFFFLL;
            app->game->curPath |= (int64_t)n21 << 62;
            if (this->calcPath(n22, n23, n3, n4, n5, b)) {
                return true;
            }
            --app->game->pathDepth;
            app->game->curPath <<= 2;
        }
    }
    return false;
}

bool Entity::aiGoal_MOVE() {
    Applet* app = CAppContainer::getInstance()->app;

    bool b = false;
    int sprite = this->getSprite();
    app->game->snapLerpSprites(sprite);
    int sX = (int)app->render->mapSprites[app->render->S_X + sprite];
    int sY = (int)app->render->mapSprites[app->render->S_Y + sprite];
    app->game->closestPath = 0LL;
    app->game->closestPathDepth = 0;
    app->game->closestPathDist = 999999999;
    app->game->curPath = 0LL;
    app->game->pathDepth = 0;
    app->game->pathSearchDepth = 8;
    app->game->findEnt = nullptr;
    app->game->skipEnt = this;
    app->game->lineOfSight = 2;
    app->game->lineOfSightWeight = 0;
    app->game->interactClipMask = 32;
    memcpy(app->game->visitedTiles, app->game->baseVisitedTiles, sizeof(app->game->visitedTiles));
    if (this->monster->goalType == EntityMonster::GOAL_MOVETOENTITY && this->monster->goalParam == 1) {
        app->game->findEnt = &app->game->entities[1];
        this->monster->goalX = app->game->destX >> 6;
        this->monster->goalY = app->game->destY >> 6;
        app->game->lineOfSightWeight = -4;
    }
    else if (this->monster->goalType == EntityMonster::GOAL_EVADE) {
        this->monster->goalX = app->game->destX >> 6;
        this->monster->goalY = app->game->destY >> 6;
        app->game->interactClipMask = 0;
        b = true;
        app->game->lineOfSight = 1;
        app->game->pathSearchDepth = this->monster->goalParam;
    }
    else if (this->monster->goalType == EntityMonster::GOAL_FLEE) {
        this->monster->goalX = app->game->destX >> 6;
        this->monster->goalY = app->game->destY >> 6;
        b = true;
        app->game->lineOfSight = 1;
    }
    else if (this->monster->goalType == EntityMonster::GOAL_MOVETOENTITY) {
        app->game->findEnt = &app->game->entities[this->monster->goalParam];
        this->monster->goalX = app->game->findEnt->linkIndex % 32;
        this->monster->goalY = app->game->findEnt->linkIndex / 32;
    }
    if (b) {
        app->game->closestPathDist = 0;
    }
    int calcPath = this->calcPath(sX >> 6, sY >> 6, this->monster->goalX, this->monster->goalY, 15535, b) ? 1 : 0;
    if (calcPath == 0 && app->game->closestPathDist < 999999999) {
        calcPath = 1;
        app->game->curPath = app->game->closestPath;
        app->game->pathDepth = app->game->closestPathDepth;
    }
    if (calcPath != 0 && app->game->pathDepth > 0) {
        app->game->curPath >>= 64 - app->game->pathDepth * 2;
        this->info &= ~Entity::ENTITY_FLAG_NOSNAP;
        int dX = sX + Canvas::viewStepValues[(int)((app->game->curPath & 0x3LL) << 2) + 0];
        int dY = sY + Canvas::viewStepValues[(int)((app->game->curPath & 0x3LL) << 2) + 1];
        this->monster->goalX = dX >> 6;
        this->monster->goalY = dY >> 6;
        app->game->trace(sX, sY, dX, dY, this, app->game->interactClipMask, 25);
        if (app->game->numTraceEntities == 0) {
            app->game->unlinkEntity(this);
            app->game->linkEntity(this, dX >> 6, dY >> 6);
            if (!app->render->cullBoundingBox(std::min(sX, dX) - 16 << 4, std::min(sY, dY) - 16 << 4, std::max(sX, dX) + 16 << 4, std::max(sY, dY) + 16 << 4, true)) {
                this->info |= Entity::ENTITY_FLAG_NOSNAP;
            }
            app->game->interpolatingMonsters = true;
            this->aiInitLerp(275);
        }
        else {
            this->monster->goalX = sX >> 6;
            this->monster->goalY = sY >> 6;
            if (app->game->traceEntity->def->eType == Enums::ET_DOOR) {
                app->game->performDoorEvent(0, app->game->traceEntity, 2);
            }
        }
        return true;
    }
    return false;
}

void Entity::aiReachedGoal_MOVE() {
    Applet* app = CAppContainer::getInstance()->app;
    EntityMonster* monster = this->monster;
    EntityDef* def = this->def;
    this->info &= ~Entity::ENTITY_FLAG_NOSNAP;
    if (monster->goalType != EntityMonster::GOAL_FLEE && 
        monster->goalType != EntityMonster::GOAL_EVADE && 
        (def->eSubType == Enums::MONSTER_TORMENTOR || ((1 << def->eSubType & Enums::EVADING_MONSTERS) != 0x0 && !(monster->flags & Enums::MFLAG_ABILITY)) || def->eSubType == Enums::BOSS_MARRIANNA || def->eSubType == Enums::BOSS_HARBINGER)) {
        Entity* target = &app->game->entities[1];
        if ((def->eSubType == Enums::MONSTER_TORMENTOR || def->eSubType == Enums::BOSS_HARBINGER) && monster->goalType == EntityMonster::GOAL_MOVETOENTITY) {
            target = &app->game->entities[monster->goalParam];
        }
        int aiWeaponForTarget = this->aiWeaponForTarget(target);
        if (aiWeaponForTarget != -1) {
            if (target == &app->game->entities[1]) {
                monster->target = nullptr;
            }
            else {
                monster->target = target;
            }
            monster->ce.weapon = aiWeaponForTarget;
            this->attack();
            return;
        }
    }
    if ((1 << def->eSubType & Enums::EVADING_MONSTERS) != 0x0) {
        monster->flags |= Enums::MFLAG_ABILITY;
    }
    if (monster->goalFlags & EntityMonster::GFL_MOVEAGAIN) {
        monster->goalFlags &= ~EntityMonster::GFL_MOVEAGAIN;
        this->aiCalcSimpleGoal(false);
        if (monster->goalType == EntityMonster::GOAL_MOVE || monster->goalType == EntityMonster::GOAL_MOVETOENTITY) {
            if (!app->game->tileObstructsAttack(monster->goalX, monster->goalY)) {
                this->aiGoal_MOVE();
            }
            else {
                monster->resetGoal();
            }
        }
    }
}

int Entity::distFrom(int n, int n2) {
    int* calcPosition = this->calcPosition();
    return std::max((n - calcPosition[0]) * (n - calcPosition[0]), (n2 - calcPosition[1]) * (n2 - calcPosition[1]));
}

void Entity::attack() {
    Applet* app = CAppContainer::getInstance()->app;
    if (!(this->monster->flags & Enums::MFLAG_ATTACKING)) {
        this->monster->flags |= Enums::MFLAG_ATTACKING;
        this->monster->nextAttacker = app->game->combatMonsters;
        app->game->combatMonsters = this;
    }
}

void Entity::undoAttack() {
    Applet* app = CAppContainer::getInstance()->app;
    if (!(this->monster->flags & Enums::MFLAG_ATTACKING)) {
        return;
    }
    this->monster->flags &= ~Enums::MFLAG_ATTACKING;
    int sprite = this->getSprite();
    int weapon = this->monster->ce.weapon;
    if (weapon == Enums::WP_M_OLARIC_JUMP || weapon == Enums::WP_M_BOSS_HARBINGER_SLAM || weapon == Enums::WP_M_ZOMBIE_SHIELD) {
        app->render->mapSpriteInfo[sprite] &= 0xFFFF00FF;
    }
    this->monster->resetGoal();
    Entity* entity = app->game->combatMonsters;
    Entity* entity2 = nullptr;
    while (entity != nullptr && entity != this) {
        entity2 = entity;
        entity = entity->monster->nextAttacker;
    }
    if (entity2 != nullptr) {
        entity2->monster->nextAttacker = this->monster->nextAttacker;
    }
    else if (app->game->combatMonsters != nullptr) {
        app->game->combatMonsters = this->monster->nextAttacker;
    }
}

void Entity::trimCorpsePile(int n, int n2) {
    Applet* app = CAppContainer::getInstance()->app;
    Entity* entity = app->game->inactiveMonsters;
    if (entity != nullptr) {
        int n3 = 0;
        do {
            int sprite = entity->getSprite();
            if (app->render->mapSprites[app->render->S_X + sprite] == n && app->render->mapSprites[app->render->S_Y + sprite] == n2 && (entity->info & (Entity::ENTITY_FLAG_GIBBED | Entity::ENTITY_FLAG_CORPSE)) != 0x0 && (app->render->mapSpriteInfo[sprite] & 0x10000) == 0x0 && ++n3 >= 3) {
                app->render->mapSpriteInfo[sprite] |= 0x10000;
                entity->info &= ~Entity::ENTITY_FLAG_CORPSE;
                entity->info |= Entity::ENTITY_FLAG_GIBBED;
                app->game->unlinkEntity(entity);
            }
            entity = entity->monster->nextOnList;
        } while (entity != app->game->inactiveMonsters);
    }
}

void Entity::knockback(int n, int n2, int n3) {
    Applet* app = CAppContainer::getInstance()->app;
    int32_t* knockbackDelta = this->knockbackDelta;
    if (n3 == 0) {
        return;
    }
    int destX;
    int destY;
    int n4;
    if (this->def->eType == Enums::ET_PLAYER) {
        destX = app->game->destX;
        destY = app->game->destY;
        n4 = 13501;
    }
    else {
        int sprite = this->getSprite();
        destX = app->render->mapSprites[app->render->S_X + sprite];
        destY = app->render->mapSprites[app->render->S_Y + sprite];
        n4 = 15535;
    }
    knockbackDelta[0] = destX - n;
    knockbackDelta[1] = destY - n2;
    if (knockbackDelta[0] != 0) {
        knockbackDelta[0] /= std::abs(knockbackDelta[0]);
        app->canvas->knockbackStart = destX;
        app->canvas->knockbackWorldDist = std::abs(64 * knockbackDelta[0] * n3);
    }
    if (knockbackDelta[1] != 0) {
        knockbackDelta[1] /= std::abs(knockbackDelta[1]);
        app->canvas->knockbackStart = destY;
        app->canvas->knockbackWorldDist = std::abs(64 * knockbackDelta[1] * n3);
    }

    int farthestKnockbackDist = this->getFarthestKnockbackDist(destX, destY, destX + 64 * knockbackDelta[0] * n3, destY + 64 * knockbackDelta[1] * n3, this, n4, 16, n3);
    if (farthestKnockbackDist == 0 || (knockbackDelta[0] == 0 && knockbackDelta[1] == 0)) {
        return;
    }
    int goalX = destX + knockbackDelta[0] * farthestKnockbackDist * 64 >> 6;
    int goalY = destY + knockbackDelta[1] * farthestKnockbackDist * 64 >> 6;
    if (this->def->eType == Enums::ET_PLAYER) {
        if (this->def->eSubType != 1) {
            app->canvas->knockbackX = knockbackDelta[0];
            app->canvas->knockbackY = knockbackDelta[1];
            app->canvas->knockbackDist = farthestKnockbackDist;
        }
    }
    else {
        this->monster->goalType = EntityMonster::GOAL_MOVE;
        this->monster->goalX = goalX;
        this->monster->goalY = goalY;
        this->monster->flags |= Enums::MFLAG_KNOCKBACK;
        LerpSprite* aiInitLerp = this->aiInitLerp(400);
        app->game->unlinkEntity(this);
        app->game->linkEntity(this, goalX, goalY);
        app->game->interpolatingMonsters = true;
        app->game->updateLerpSprite(aiInitLerp);
    }
}

int Entity::getFarthestKnockbackDist(int n, int n2, int n3, int n4, Entity* entity, int n5, int n6, int n7) {
    Applet* app = CAppContainer::getInstance()->app;
    int n8 = n7;
    app->game->trace(n, n2, n3, n4, entity, n5, n6);
    if (app->game->traceEntity != nullptr) {
        n8 = n8 * app->game->traceFracs[0] >> 14;
    }
    return n8;
}

void Entity::resurrect(int n, int n2, int n3) {
    Applet* app = CAppContainer::getInstance()->app;
    int sprite = this->getSprite();
    this->def = app->entityDefManager->find(Enums::ET_MONSTER, this->def->eSubType, this->def->parm);
    this->name = Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, this->def->name);
    this->monster->clearEffects();
    app->render->mapSprites[app->render->S_X + sprite] = (short)n;
    app->render->mapSprites[app->render->S_Y + sprite] = (short)n2;
    app->render->mapSprites[app->render->S_Z + sprite] = (short)n3;
    app->render->mapSpriteInfo[sprite] &= 0xFFFC00FF;
    if ((app->nextInt() & 0x1) != 0x0) {
        app->render->mapSpriteInfo[sprite] |= 0x20000;
    }
    app->render->relinkSprite(sprite);
    this->info &= ~(Entity::ENTITY_FLAG_GIBBED | Entity::ENTITY_FLAG_TAKEDAMAGE | Entity::ENTITY_FLAG_CORPSE | Entity::ENTITY_FLAG_RAISETARGET);
    this->info |= Entity::ENTITY_FLAG_TAKEDAMAGE;
    CombatEntity* ce = &this->monster->ce;
    this->initspawn();
    ce->setStat(Enums::STAT_HEALTH, ce->getStat(Enums::STAT_MAX_HEALTH));
    this->monster->flags &= ~Enums::MFLAG_KNOCKBACK;
    app->game->unlinkEntity(this);
    app->game->linkEntity(this, n >> 6, n2 >> 6);
    app->canvas->updateFacingEntity = true;
}

int* Entity::calcPosition() {
    Applet* app = CAppContainer::getInstance()->app;
    int x;
    int y;
    if (this->def->eType == Enums::ET_WORLD) {
        x = app->game->traceCollisionX;
        y = app->game->traceCollisionY;
    }
    else if (this->def->eType == Enums::ET_PLAYER) {
        x = app->canvas->destX;
        y = app->canvas->destY;
    }
    else if (this->def->eType == Enums::ET_MONSTER) {
        int sprite = this->getSprite();
        x = app->render->mapSprites[app->render->S_X + sprite];
        y = app->render->mapSprites[app->render->S_Y + sprite];
    }
    else {
        int sprite = this->getSprite();
        x = app->render->mapSprites[app->render->S_X + sprite];
        y = app->render->mapSprites[app->render->S_Y + sprite];
    }
    this->pos[0] = x;
    this->pos[1] = y;
    return this->pos;
}

bool Entity::isBoss() {
    return this->def->eSubType >= Enums::FIRSTBOSS && this->def->eSubType <= Enums::LASTBOSS;
}

bool Entity::isHasteResistant() {
    return this->def->eType == Enums::ET_MONSTER && (this->def->eSubType == Enums::BOSS_HARBINGER || this->def->eSubType == Enums::BOSS_MARRIANNA || this->def->eSubType == Enums::BOSS_OLARIC);
}

bool Entity::isExplodableEntity() {
    return (this->def->eType == Enums::ET_ATTACK_INTERACTIVE && this->def->eSubType == Enums::INTERACT_BARREL) || 
           (this->def->eType == Enums::ET_DECOR_NOCLIP && this->def->eSubType == Enums::DECOR_DYNAMITE);
}

bool Entity::isDroppedEntity() {
    Applet* app = CAppContainer::getInstance()->app;
    short index = this->getIndex();
    return index >= app->game->firstDropIndex && index < app->game->firstDropIndex + 16;
}

bool Entity::isBinaryEntity(int* array) {
    Applet* app = CAppContainer::getInstance()->app;
    bool b = false;
    if (this->def == nullptr) {
        return false;
    }
    if (this->isDroppedEntity()) {
        return false;
    }
    switch (this->def->eType) {
        case Enums::ET_ITEM:
        case Enums::ET_ATTACK_INTERACTIVE:
        case Enums::ET_MONSTERBLOCK_ITEM: {
            b = (((this->info & Entity::ENTITY_FLAG_LINKED) != 0x0) ? true : false);
            break;
        }
        case Enums::ET_DOOR: {
            b = ((app->render->mapSprites[app->render->S_SCALEFACTOR + this->getSprite()] != 64) ? true : false);
            if (this->def->eSubType == Enums::DOOR_LOCKED && nullptr != array) {
                array[1] |= 0x200000;
            }
            break;
        }
        case Enums::ET_NONOBSTRUCTING_SPRITEWALL: {
            int sprite = this->getSprite();
            int info = app->render->mapSpriteInfo[sprite] & 0xFF;
            if ((info == Enums::TILENUM_PRISON_BARS) || (info == Enums::TILENUM_NONOBSTRUCTING_SPRITEWALL2)) {
                b = (((app->render->mapSpriteInfo[sprite] & 0x10000) != 0x0) ? true : false);
            }
            break;
        }
        default: {
            return false;
        }
    }
    if (nullptr != array) {
        array[0] = (b ? 1 : 0);
    }
    return true;
}

bool Entity::isNamedEntity(int* array) {
    Applet* app = CAppContainer::getInstance()->app;
    if (this->def == nullptr || this->name == Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, this->def->name) || this->def->eType == Enums::ET_CORPSE) {
        return false;
    }

    if (this->def->eType == Enums::ET_MONSTER && this->def->eSubType == Enums::MONSTER_ZOMBIE) {
        return false;
    }

    array[0] = this->name;
    if (array[0] != -1) {
        return true;
    }
    app->Error(25); // ERR_ISNAMEDENTITY
    return false;
}

void Entity::saveState(OutputStream* OS, int n) {
    Applet* app = CAppContainer::getInstance()->app;

    short* mapSprites = app->render->mapSprites;
    int* mapSpriteInfo = app->render->mapSpriteInfo;
    if ((n & 0x20000) != 0x0) {
        this->isNamedEntity(this->tempSaveBuf);
        OS->writeShort((int16_t)this->tempSaveBuf[0]);
    }
    if ((n & 0x80000) != 0x0) {
        return;
    }
    if (this->def->eType == Enums::ET_ATTACK_INTERACTIVE && this->def->eSubType == Enums::INTERACT_BARRICADE) {
        return;
    }
    int sprite = this->getSprite();
    if (this->def->eType == Enums::ET_MONSTER && (n & 0x200000) != 0x0) {
        OS->writeByte(mapSprites[app->render->S_X + sprite] >> 3);
        OS->writeByte(mapSprites[app->render->S_Y + sprite] >> 3);
        OS->writeByte((mapSpriteInfo[sprite] & Enums::SPRITE_MASK_FRAMENUMBER) >> Enums::SPRITE_SHIFT_FRAMENUMBER);
        OS->writeShort(this->monster->flags);
        return;
    }
    OS->writeByte(this->info >> 16 & 0xFF);
    if (!(this->info & Entity::ENTITY_FLAG_GIBBED)) {
        OS->writeByte((mapSpriteInfo[sprite] & Enums::SPRITE_MASK_LOFLAGS) >> Enums::SPRITE_SHIFT_FLAGS);
        OS->writeByte((mapSpriteInfo[sprite] & Enums::SPRITE_MASK_FRAMENUMBER) >> Enums::SPRITE_SHIFT_FRAMENUMBER);
        if (this->isDroppedEntity() || (app->render->mapSpriteInfo[sprite] & 0xF000000) == 0x0) {
            OS->writeByte(mapSprites[app->render->S_X + sprite] >> 3);
            OS->writeByte(mapSprites[app->render->S_Y + sprite] >> 3);
        }
        if (this->isDroppedEntity()) {
            OS->writeInt(this->param);
        }
        if (!this->isDroppedEntity() && (app->render->mapSpriteInfo[sprite] & 0xF000000) != 0x0) {
            OS->writeShort(this->linkIndex);
        }
    }
    if (this->monster != nullptr) {
        OS->writeShort(this->monster->flags);
        if (!(this->info & Entity::ENTITY_FLAG_GIBBED)) {
            if (this->monster->flags & Enums::MFLAG_SCALED) {
                OS->writeByte(mapSprites[app->render->S_SCALEFACTOR + sprite]);
            }
            if ((n & 0x100000) == 0x0) {
                OS->writeShort(this->monster->monsterEffects);
                this->monster->ce.saveState(OS, false);
                this->monster->saveGoalState(OS);
            }
        }
    }
    else if (this->isDroppedEntity()) {
        OS->writeByte((uint8_t)(this->def->eType | this->def->eSubType << 4));
        OS->writeByte(this->def->parm);
        if (this->def->eType == Enums::ET_DECOR_NOCLIP && this->def->eSubType == Enums::DECOR_DYNAMITE) {
            OS->writeByte((mapSpriteInfo[sprite] & 0xFF000000) >> 24);
        }
    }
    else {
        OS->writeShort(mapSprites[app->render->S_Z + sprite]);
    }
}

void Entity::loadState(InputStream* IS, int n) {
    Applet* app = CAppContainer::getInstance()->app;

    if ((n & 0x20000) != 0x0) {
        this->name = IS->readShort();
    }
    int sprite = this->getSprite();
    int n2 = app->render->mapSpriteInfo[sprite] & Enums::SPRITE_MASK_SPRITENUMBER;
    if ((app->render->mapSpriteInfo[sprite] & 0x400000) != 0x0) {
        n2 += Enums::TILENUM_FIRST_WALL;
    }
    if ((n & 0x40000) != 0x0) {
        app->render->mapSpriteInfo[sprite] &= 0xFFFEFFFF;
        if ((n & 0x80000) != 0x0 && !(this->info & Entity::ENTITY_FLAG_LINKED)) {
            app->game->linkEntity(this, app->render->mapSprites[app->render->S_X + sprite] >> 6, app->render->mapSprites[app->render->S_Y + sprite] >> 6);
        }
    }
    else {
        app->render->mapSpriteInfo[sprite] |= 0x10000;
        if ((this->info & Entity::ENTITY_FLAG_LINKED) != 0x0 && this->def->eType != Enums::ET_DOOR && (n2 < Enums::TILENUM_DUMMY_START || n2 > Enums::TILENUM_DUMMY_END)) {
            app->game->unlinkEntity(this);
        }
        else if (this->def->eType == Enums::ET_ATTACK_INTERACTIVE && this->def->eSubType != Enums::INTERACT_PICKUP) {
            ++app->game->destroyedObj;
        }
    }
    if ((n & 0x1000000) != 0x0) {
        this->info |= Entity::ENTITY_FLAG_DEATHFUNC;
    }
    if (this->isBinaryEntity(nullptr)) {
        this->restoreBinaryState(n);
        if ((n & 0x80000) != 0x0) {
            return;
        }
    }
    if (this->def != nullptr && this->def->eType == Enums::ET_ATTACK_INTERACTIVE && this->def->eSubType == Enums::INTERACT_BARRICADE) {
        return;
    }
    if ((n & 0x100000) != 0x0) {
        if ((n & 0x40000) == 0x0) {
            this->info |= Entity::ENTITY_FLAG_GIBBED;
            if (this->info & Entity::ENTITY_FLAG_LINKED) {
                app->game->unlinkEntity(this);
            }
        }
        else if ((n & 0x80000) != 0x0) {
            app->Error(Enums::ERR_CLEANCORPSE);
        }
        app->render->mapSpriteInfo[sprite] = ((app->render->mapSpriteInfo[sprite] & 0xFFFF00FF) | 0x7000);
        if (!this->isDroppedEntity()) {
            this->def = app->entityDefManager->find(Enums::ET_CORPSE, this->def->eSubType, this->def->parm);
        }
    }
    if (this->def != nullptr && this->def->eType == Enums::ET_NPC) {
        this->param = (((n & 0x200000) != 0x0) ? 1 : 0);
        this->param += (((n & 0x4000000) != 0x0) ? 1 : 0);
    }
    if (this->monster != nullptr) {
        if ((n & 0x800000) != 0x0) {
            this->monster->flags |= Enums::MFLAG_NORESPAWN;
            this->info |= Entity::ENTITY_FLAG_DIRTY;
        }
        if ((n & 0x400000) != 0x0) {
            this->monster->flags |= Enums::MFLAG_NOTRACK;
            this->info |= Entity::ENTITY_FLAG_DIRTY;
        }
    }
    if ((n & 0x80000) != 0x0) {
        return;
    }
    if (this->info & Entity::ENTITY_FLAG_LINKED) {
        app->game->unlinkEntity(this);
    }
    int sprite2 = this->getSprite();
    if (this->def != nullptr && this->def->eType == Enums::ET_MONSTER && (n & 0x200000) != 0x0) {
        short n5 = (short)((IS->readByte() & 0xFF) << 3);
        short n6 = (short)((IS->readByte() & 0xFF) << 3);
        int n7 = (IS->readByte() & 0xFF) << 8;
        app->render->mapSprites[app->render->S_X + sprite2] = n5;
        app->render->mapSprites[app->render->S_Y + sprite2] = n6;
        app->render->mapSprites[app->render->S_Z + sprite2] = (short)(app->render->getHeight(n5, n6) + 32);
        app->render->mapSpriteInfo[sprite2] = ((app->render->mapSpriteInfo[sprite2] & 0xFFFF00FF) | n7);
        app->render->relinkSprite(sprite2);
        if (this->info & Entity::ENTITY_FLAG_LINKED) {
            app->game->unlinkEntity(this);
        }
        if ((n & 0x40000) == 0x0) {
            app->game->deactivate(this);
        }
        else {
            app->game->linkEntity(this, n5 >> 6, n6 >> 6);
        }
        this->monster->flags = IS->readShort();
        if (this->monster->flags != 0 || n7 != 0) {
            this->info |= Entity::ENTITY_FLAG_DIRTY;

            if (this->monster->flags & Enums::MFLAG_NPC_MONSTER) {
                if (n7 >> 8 == Enums::TILENUM_OTHER_SCOTCH) {
                    this->monster->frameTime = INT_MAX;
                }
                this->info &= ~Entity::ENTITY_FLAG_TAKEDAMAGE;
            }
        }
        return;
    }
    this->info = ((this->info & 0xFF00FFFF) | (IS->readByte() & 0xFF) << 16);
    if (!(this->info & Entity::ENTITY_FLAG_GIBBED)) {
        int n8 = IS->readByte() & 0xFF;
        int n9 = IS->readByte() & 0xFF;
        app->render->mapSpriteInfo[sprite2] = ((app->render->mapSpriteInfo[sprite2] & 0xFF0000FF) | n9 << 8 | n8 << 16);
        if (this->isDroppedEntity() || (app->render->mapSpriteInfo[sprite2] & 0xF000000) == 0x0) {
            app->render->mapSprites[app->render->S_X + sprite2] = (short)(IS->readUnsignedByte() << 3);
            app->render->mapSprites[app->render->S_Y + sprite2] = (short)(IS->readUnsignedByte() << 3);
            if (this->monster != nullptr || this->isDroppedEntity()) {
                app->render->mapSprites[app->render->S_Z + sprite2] = (short)(app->render->getHeight(app->render->mapSprites[app->render->S_X + sprite2], app->render->mapSprites[app->render->S_Y + sprite2]) + 32);
            }
            app->render->relinkSprite(sprite2);
        }
        if (this->isDroppedEntity()) {
            this->param = IS->readInt();
        }
        if (!this->isDroppedEntity() && (app->render->mapSpriteInfo[sprite2] & 0xF000000) != 0x0) {
            this->linkIndex = IS->readShort();
        }
        else {
            this->linkIndex = (short)((app->render->mapSprites[app->render->S_X + sprite2] >> 6) + (app->render->mapSprites[app->render->S_Y + sprite2] >> 6) * 32);
        }
        if (((app->render->mapSpriteInfo[sprite2] & 0xF000000) == 0xC000000 || (app->render->mapSpriteInfo[sprite2] & 0xF000000) == 0x3000000) && (this->def->eType == Enums::ET_NONOBSTRUCTING_SPRITEWALL || this->def->eType == Enums::ET_SPRITEWALL)) {
            int n10 = this->linkIndex % 32;
            int n11 = this->linkIndex / 32;
            app->render->mapSprites[app->render->S_X + sprite2] = (short)((n10 << 6) + 32);
            app->render->mapSprites[app->render->S_Y + sprite2] = (short)((n11 << 6) + 32);
            app->render->relinkSprite(sprite2);
        }
        if (n9 != 0) {
            this->info |= Entity::ENTITY_FLAG_DIRTY;
        }
    }
    if ((n & 0x2000000) != 0x0) {
        this->info |= Entity::ENTITY_FLAG_GHOST;
    }
    if (this->monster != nullptr) {
        this->monster->flags = IS->readShort();
        if (!(this->info & Entity::ENTITY_FLAG_GIBBED)) {
            if (this->monster->flags & Enums::MFLAG_SCALED) {
                app->render->mapSprites[app->render->S_SCALEFACTOR + sprite2] = (short)IS->readUnsignedByte();
            }
            if ((n & 0x100000) != 0x0) {
                this->info |= Entity::ENTITY_FLAG_CORPSE;
            }
            else {
                this->monster->monsterEffects = IS->readShort();
                this->monster->ce.loadState(IS, false);
                this->monster->loadGoalState(IS);
            }
        }
        if (this->info & Entity::ENTITY_FLAG_ACTIVE) {
            this->info &= ~Entity::ENTITY_FLAG_ACTIVE;
            app->game->activate(this, false, false, false, true);
        }
        if (this->info & Entity::ENTITY_FLAG_CORPSE) {
            this->def = app->entityDefManager->find(Enums::ET_CORPSE, this->def->eSubType, this->def->parm);
        }
        if (this->monster->flags & Enums::MFLAG_NPC_MONSTER) {
            if ((app->render->mapSpriteInfo[sprite2] & 0xFF00) >> 8 == Enums::TILENUM_OTHER_SCOTCH) {
                this->monster->frameTime = INT_MAX;
            }
            this->info &= ~Entity::ENTITY_FLAG_TAKEDAMAGE;
            this->info |= Entity::ENTITY_FLAG_DIRTY;
        }
    }
    else if (this->isDroppedEntity()) {
        uint8_t byte1 = IS->readByte();
        uint8_t b = (uint8_t)(byte1 & 0xF);
        uint8_t b2 = (uint8_t)(byte1 >> 4 & 0xF);
        uint8_t byte2 = IS->readByte();
        this->def = app->entityDefManager->find(b, b2, byte2);
        if (this->name == -1) {
            this->name = Localization::STRINGID(Strings::FILE_ENTITYSTRINGS, this->def->name);
        }
        short n12 = this->def->tileIndex;
        app->render->mapSpriteInfo[sprite2] = ((app->render->mapSpriteInfo[sprite2] & 0xFFFFFF00) | n12);
        app->render->mapSprites[app->render->S_ENT + sprite2] = this->getIndex();
        if (this->def->eType == Enums::ET_DECOR_NOCLIP && this->def->eSubType == Enums::DECOR_DYNAMITE) {
            int n13 = (IS->readByte() & 0xFF) << 24;
            app->render->mapSpriteInfo[sprite2] |= n13;
            app->render->mapSprites[app->render->S_Z + sprite2] = (short)(app->render->getHeight(app->render->mapSprites[app->render->S_X + sprite2], app->render->mapSprites[app->render->S_Y + sprite2]) + (((app->render->mapSpriteInfo[sprite2] & 0xF000000) != 0x0) ? 32 : 31));
            app->render->mapSprites[app->render->S_SCALEFACTOR + sprite2] = 32;
            app->render->relinkSprite(sprite2);
        }
    }
    else {
        app->render->mapSprites[app->render->S_Z + sprite2] = IS->readShort();
        app->render->relinkSprite(sprite2);
    }
    if (this->info & Entity::ENTITY_FLAG_LINKED) {
        if ((app->render->mapSpriteInfo[sprite2] & 0xF000000) != 0x0) {
            app->game->linkEntity(this, this->linkIndex % 32, this->linkIndex / 32);
        }
        else {
            app->game->linkEntity(this, app->render->mapSprites[app->render->S_X + sprite2] >> 6, app->render->mapSprites[app->render->S_Y + sprite2] >> 6);
        }
    }
}

int Entity::getSaveHandle(bool b) {
    Applet* app = CAppContainer::getInstance()->app;
    int* tempSaveBuf = this->tempSaveBuf;
    tempSaveBuf[tempSaveBuf[0] = 1] = this->getIndex();
    bool droppedEntity = this->isDroppedEntity();
    bool binaryEntity = this->isBinaryEntity(tempSaveBuf);
    if (((this->info & 0xFFFF) == 0x0 || this->def == nullptr) && !binaryEntity) {
        return -1;
    }
    if (droppedEntity && !(this->info & Entity::ENTITY_FLAG_LINKED)) {
        return -1;
    }
    if (droppedEntity && b && this->def->eType == Enums::ET_DECOR_NOCLIP && this->def->eSubType == Enums::DECOR_DYNAMITE) {
        return -1;
    }
    bool b2 = tempSaveBuf[0] != 0;
    int n = tempSaveBuf[1];
    if (binaryEntity && b2) {
        n |= 0x10000;
    }
    if (this->isNamedEntity(this->tempSaveBuf)) {
        n |= 0x20000;
    }
    if ((app->render->mapSpriteInfo[this->getSprite()] & 0x10000) == 0x0) {
        n |= 0x40000;
    }
    if (this->info & Entity::ENTITY_FLAG_DEATHFUNC) {
        n |= 0x1000000;
    }
    if (!(this->info & Entity::ENTITY_FLAG_DIRTY)) {
        n |= 0x80000;
        if ((n & 0x20000) == 0x0 && this->def->eType == Enums::ET_DECOR && this->def->eSubType != Enums::DECOR_STATUE) {
            return -1;
        }
    }
    if (this->info & (Entity::ENTITY_FLAG_GIBBED | Entity::ENTITY_FLAG_CORPSE)) {
        n |= 0x100000;
    }
    if (this->info & Entity::ENTITY_FLAG_GHOST) {
        n |= 0x2000000;
    }
    if (this->info & Entity::ENTITY_FLAG_GIBBED) {
        n = ((n & 0xFFFBFFFF) | 0x100000);
    }
    if (this->def->eType == Enums::ET_NPC && this->param != 0) {
        n |= 0x200000;
        if (this->param == 2) {
            n |= 0x4000000;
        }
    }
    if (this->monster != nullptr) {
        if (this->monster->flags & Enums::MFLAG_NORESPAWN) {
            n |= 0x800000;
        }
        if (this->monster->flags & Enums::MFLAG_NOTRACK) {
            n |= 0x400000;
        }
    }
    if (b) {
        if (this->def->eType == Enums::ET_CORPSE && this->def->eSubType != Enums::CORPSE_SKELETON && !droppedEntity && !(this->monster->flags & Enums::MFLAG_NOTRACK)) {
            n = ((n & 0xFFFBFFFF) | 0x80000);
        }
        else if (this->def->eType == Enums::ET_MONSTER) {
            n |= 0x200000;
        }
    }
    return n;
}

void Entity::restoreBinaryState(int n) {
    Applet* app = CAppContainer::getInstance()->app;
    bool b = (n & 0x10000) != 0x0;
    switch (this->def->eType) {
        case Enums::ET_ITEM:
        case Enums::ET_ATTACK_INTERACTIVE:
        case Enums::ET_MONSTERBLOCK_ITEM: {
            if (b) {
                app->game->unlinkEntity(this);
                app->game->linkEntity(this, this->linkIndex % 32, this->linkIndex / 32);
            }
            else {
                app->game->unlinkEntity(this);
            }
            if (this->def->eType != Enums::ET_ATTACK_INTERACTIVE || this->def->eSubType != Enums::INTERACT_BARRICADE) {
                break;
            }

            int sprite = this->getSprite();
            if (b) {
                app->game->unlinkEntity(this);
                app->game->linkEntity(this, this->linkIndex % 32, this->linkIndex / 32);
                app->render->relinkSprite(sprite);
                break;
            }
            app->render->unlinkSprite(sprite);
            break;
        }
        case Enums::ET_DOOR: {
            bool b2 = this->def->eSubType == Enums::DOOR_LOCKED;
            app->game->setLineLocked(this, false);
            if (b) {
                app->game->performDoorEvent(0, this, 0);
            }
            else {
                app->game->performDoorEvent(1, this, 0);
            }
            if ((n & 0x200000) != 0x0 || b2) {
                app->game->setLineLocked(this, (n & 0x200000) != 0x0);
            }
            break;
        }
        case Enums::ET_NONOBSTRUCTING_SPRITEWALL: {
            if (b) {
                app->game->performDoorEvent(0, this, 0);
                break;
            }
            app->game->performDoorEvent(1, this, 0);
            break;
        }
        default: {
            break;
        }
    }
}

short Entity::getIndex() {
    Applet* app = CAppContainer::getInstance()->app;
    for (short n = 0; n < app->game->numEntities; ++n) {
        if (this == &app->game->entities[n]) {
            return n;
        }
    }
    return -1;
}

void Entity::updateMonsterFX() {
    Applet* app = CAppContainer::getInstance()->app;
    if (nullptr != this->monster) {

        if (this->def->eSubType == Enums::BOSS_MARRIANNA && this->def->eType == Enums::ET_MONSTER && this->monster->ce.getStat(Enums::STAT_HEALTH) < this->monster->ce.getStat(Enums::STAT_MAX_HEALTH)) {
            this->monster->ce.addStat(0, 20);
            Text* smallBuffer = app->localization->getSmallBuffer();
            app->localization->composeText(Strings::FILE_ENTITYSTRINGS, this->def->name, smallBuffer);
            app->localization->resetTextArgs();
            app->localization->addTextArg(smallBuffer);
            app->localization->addTextArg(20);
            app->hud->addMessage(CodeStrings::X_HEALS_Y);
            smallBuffer->dispose();
        }

        for (int i = 0; i < EntityMonster::MFX_COUNT; ++i) {
            int n = 1 << i;
            int n2 = this->monster->monsterEffects;
            if ((n2 & n) != 0x0) {
                int n3 = 5 + (i << 1);
                int n4 = n2 >> n3 & 0x3;
                if (this->def->eType != Enums::ET_CORPSE && (this->info & Entity::ENTITY_FLAG_TAKEDAMAGE) != 0x0) {
                    int n5 = 0;
                    if (n == EntityMonster::MFX_FIRE) {
                        if (this->def->eSubType == Enums::BOSS_OLARIC || (this->def->eSubType == Enums::MONSTER_SKELETON && this->def->parm == 0)) {
                            n5 = 8;
                        }
                        else if (this->def->eSubType == Enums::MONSTER_TORMENTOR) {
                            n5 = 6;
                        }
                        else {
                            n5 = 4;
                        }
                        app->localization->resetTextArgs();
                        app->localization->addTextArg(n5);
                        app->hud->addMessage(Strings::FILE_CODESTRINGS, CodeStrings::X_FIRE_DAMAGE);
                    }
                    else if (n5 > 0) {
                        app->localization->resetTextArgs();
                        app->localization->addTextArg(n5);
                        app->hud->addMessage(Strings::FILE_CODESTRINGS, CodeStrings::X_DAMAGE);
                    }
                    if (n5 > 0) {
                        this->pain(n5, nullptr);
                        n2 = this->monster->monsterEffects;
                        if (this->monster->ce.getStat(Enums::STAT_HEALTH) <= 0) {
                            this->died(true, nullptr);
                            n2 = ((this->monster->monsterEffects & EntityMonster::MFX_REMOVE_TURNS) | EntityMonster::MFX_ALL_ONE_TURNS);
                            n4 = 1;
                        }
                    }
                }
                int monsterEffects;
                if (n4 == 0) {
                    monsterEffects = (n2 & ~n);
                }
                else {
                    --n4;
                    monsterEffects = ((n2 & ~(EntityMonster::MFX_TURN_MASK << n3)) | n4 << n3);
                }
                this->monster->monsterEffects = monsterEffects;
            }
        }
    }
}
