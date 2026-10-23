#include <stdexcept>

#include "CAppContainer.h"
#include "App.h"
#include "CombatEntity.h"
#include "JavaStream.h"
#include "EntityMonster.h"
#include "Entity.h"
#include "Combat.h"
#include "Player.h"
#include "Render.h"
#include "Canvas.h"
#include "Game.h"
#include "Enums.h"
#include "Sound.h"

//LISTO VERIFICADO

CombatEntity::CombatEntity() {
    //printf("CombatEntity::init\n");
    this->stats[1] = 0;
    this->stats[2] = 0;
    this->stats[3] = 0;
    this->stats[4] = 0;
    this->stats[5] = 0;
    this->stats[6] = 0;
    this->weapon = -1;
}

CombatEntity::CombatEntity(int health, int armor, int defense, int strength, int accuracy, int agility) {
	//printf("CombatEntity::init2\n");
	this->setStat(Enums::STAT_MAX_HEALTH, health);
	this->setStat(Enums::STAT_HEALTH, health);
	this->setStat(Enums::STAT_ARMOR, armor);
	this->setStat(Enums::STAT_DEFENSE, defense);
	this->setStat(Enums::STAT_STRENGTH, strength);
	this->setStat(Enums::STAT_ACCURACY, accuracy);
	this->setStat(Enums::STAT_AGILITY, agility);
}

CombatEntity::~CombatEntity() {
}

void CombatEntity::clone(CombatEntity* ce) {
    ce->stats[Enums::STAT_MAX_HEALTH] = this->stats[Enums::STAT_MAX_HEALTH];
    ce->stats[Enums::STAT_ARMOR] = this->stats[Enums::STAT_ARMOR];
    ce->stats[Enums::STAT_DEFENSE] = this->stats[Enums::STAT_DEFENSE];
    ce->stats[Enums::STAT_STRENGTH] = this->stats[Enums::STAT_STRENGTH];
    ce->stats[Enums::STAT_ACCURACY] = this->stats[Enums::STAT_ACCURACY];
    ce->stats[Enums::STAT_AGILITY] = this->stats[Enums::STAT_AGILITY];
    ce->setStat(Enums::STAT_HEALTH, this->stats[Enums::STAT_HEALTH]);
    ce->weapon = this->weapon;
}

int CombatEntity::getStat(int i) {
    return this->stats[i];
}

int CombatEntity::getStatPercent(int i) {
    return (this->stats[i] << 8) / 100;
}

int CombatEntity::addStat(int i, int i2) {
    i2 += this->stats[i];
    this->setStat(i, i2);
    return this->stats[i];
}

int CombatEntity::setStat(int i, int i2) {
    if (i2 < 0) {
        i2 = 0;
    }
    switch (i) {
        case Enums::STAT_MAX_HEALTH: {
            break;
        }
        case Enums::STAT_HEALTH: {
            i2 = std::min(i2, this->getStat(Enums::STAT_MAX_HEALTH));
            break;
        }
        default: {
            i2 = std::min(i2, 255);
            break;
        }
    }
    return this->stats[i] = i2;
}

int CombatEntity::calcXP() {
    return ((this->stats[3] + this->stats[4]) * 5 + this->stats[5] * 6 + this->stats[1] * 5 + 49) / 50;
}

void CombatEntity::loadState(InputStream* inputStream, bool b) {
    if (b) {
        this->stats[0] = (int)inputStream->readShort();
        this->stats[1] = (int)inputStream->readShort();
        this->stats[2] = (int)inputStream->readByte();
        this->stats[3] = (int)inputStream->readByte();
        this->stats[4] = (int)inputStream->readByte();
        this->stats[5] = (int)inputStream->readByte();
        this->stats[6] = (int)inputStream->readByte(); // From Doom II RPG
        this->stats[7] = (int)inputStream->readByte();
        this->weapon = (int)inputStream->readByte();
    }
    else {
        this->stats[0] = (int)inputStream->readShort();
    }
}

void CombatEntity::saveState(OutputStream* outputStream, bool b) {
    if (b) {
        outputStream->writeShort((int16_t)this->stats[0]);
        outputStream->writeShort((int16_t)this->stats[1]);
        outputStream->writeByte((uint8_t)this->stats[2]);
        outputStream->writeByte((uint8_t)this->stats[3]);
        outputStream->writeByte((uint8_t)this->stats[4]);
        outputStream->writeByte((uint8_t)this->stats[5]);
        outputStream->writeByte((uint8_t)this->stats[6]); // From Doom II RPG
        outputStream->writeByte((uint8_t)this->stats[7]);
        outputStream->writeByte((uint8_t)this->weapon);
    }
    else {
        outputStream->writeShort((int16_t)this->stats[0]);
    }
}

void CombatEntity::calcCombat(CombatEntity* combatEntity, Entity* entity, bool b, int n, int n2) {
    Applet* app = CAppContainer::getInstance()->app;

    app->combat->crDamage = 0;
    app->combat->crArmorDamage = 0;
    CombatEntity* combatEntity2;
    if (!b) {
        combatEntity2 = &entity->monster->ce;
    }
    else {
        combatEntity2 = app->player->ce;
    }
    int crFlags = this->calcHit(combatEntity, combatEntity2, b, n, false);
    if (crFlags & Enums::CR_HITMASK) {
        int calcDamage = 0;
        if (combatEntity->weapon != 11) {
            calcDamage = this->calcDamage(combatEntity, entity, combatEntity2, b, n2);
            if (calcDamage == 0 && app->combat->crArmorDamage == 0) {
                crFlags |= Enums::CR_IMMUNE;
            }
        }
        app->combat->crFlags = crFlags;
        app->combat->crDamage = calcDamage;
    }
}

int CombatEntity::calcHit(CombatEntity* ce, CombatEntity* ce2, bool b, int i, bool b2) {
    Applet* app = CAppContainer::getInstance()->app;
    int attackerWeapon = app->combat->attackerWeapon;
    int attackerWeaponId = app->combat->attackerWeaponId;
    Entity* curTarget = app->combat->curTarget;
    int eType;
    int eSubType;
    if (curTarget == nullptr) {
        eType = Enums::ET_PLAYER;
        eSubType = 0;
    }
    else {
        eType = curTarget->def->eType;
        eSubType = curTarget->def->eSubType;
    }

    if (app->combat->oneShotCheat && !b && attackerWeaponId != Enums::WP_DYNAMITE || (eType == Enums::ET_MONSTER && eSubType == 14 && (curTarget->monster->goalType == EntityMonster::GOAL_STUN || attackerWeaponId == Enums::WP_TESLA))) {
        return app->combat->crFlags |= Enums::CR_HIT;
    }

    if (Entity::CheckWeaponMask(attackerWeaponId, Enums::WP_PUNCH_MASK) && eType == Enums::ET_MONSTER && eSubType == 9) {
        return app->combat->crFlags;
    }
    
    if (Entity::CheckWeaponMask(attackerWeaponId, Enums::WP_SNIPERMASK)) {
        if (eType == Enums::ET_MONSTER) {
            int sprite = curTarget->getSprite();
            int n2 = app->canvas->zoomCollisionX - app->render->mapSprites[app->render->S_X + sprite];
            int n3 = app->canvas->zoomCollisionY - app->render->mapSprites[app->render->S_Y + sprite];
            int n4 = app->canvas->zoomCollisionZ - app->render->mapSprites[app->render->S_Z + sprite];
            int n5 = app->render->mapSpriteInfo[sprite] >> 8 & Enums::MANIM_MASK;
            int(*imageFrameBounds)[4] = app->render->getImageFrameBounds(curTarget->def->tileIndex, 3, 2, 0);

            int n6 = -1;
            for (int i = 0; i < 3; ++i) {
                if (n2 > (imageFrameBounds[i][0]) &&
                    n2 < (imageFrameBounds[i][1]) &&
                    n3 > (imageFrameBounds[i][0]) &&
                    n3 < (imageFrameBounds[i][1]) &&
                    n4 > (imageFrameBounds[i][2]) &&
                    n4 < (imageFrameBounds[i][3])) {
                    n6 = i;
                    break;
                }
            }
            if (n6 != -1) {
                if (n6 == 0 || n5 == Enums::MANIM_IDLE_BACK) {
                    app->combat->crFlags |= Enums::CR_CRIT;
                }
                else if (n6 == 1) {
                    app->combat->crFlags |= Enums::CR_HIT;
                }
                else {
                    app->combat->crFlags |= (Enums::CR_HIT | Enums::CR_WEAK_HIT);
                }
            }
        }
        return app->combat->crFlags;
    }

    bool b3 = false;
    int worldDistToTileDist = app->combat->WorldDistToTileDist(i);
    int n7;
    if (worldDistToTileDist < app->combat->weapons[attackerWeapon + Combat::WEAPON_FIELD_RANGEMIN]) {
        n7 = app->combat->weapons[attackerWeapon + Combat::WEAPON_FIELD_RANGEMIN] - worldDistToTileDist;
    }
    else if (worldDistToTileDist > app->combat->weapons[attackerWeapon + Combat::WEAPON_FIELD_RANGEMAX]) {
        if (attackerWeaponId == Enums::WP_STEN || attackerWeaponId == Enums::WP_SPIKE_PUNCH) {
            b3 = true;
        }
        n7 = worldDistToTileDist - app->combat->weapons[attackerWeapon + Combat::WEAPON_FIELD_RANGEMAX];
    }
    else {
        n7 = 0;
    }
    if (app->combat->crFlags & Enums::CR_IGNORERANGE) {
        n7 = 0;
    }
    else if ((app->combat->weapons[attackerWeapon + Combat::WEAPON_FIELD_RANGEMIN] == app->combat->weapons[attackerWeapon + Combat::WEAPON_FIELD_RANGEMAX] || (app->combat->crFlags & Enums::CR_REQUIRERANGE)) && n7 > 0) {
        return app->combat->crFlags |= Enums::CR_OUTOFRANGE;
    }

    if (b2) {
        return app->combat->crFlags |= Enums::CR_HIT;
    }

    if (ce->weapon == Enums::WP_DYNAMITE) {
        return app->combat->crFlags |= Enums::CR_BOMB_STUCK;
    }

    int stat = ce->getStat(Enums::STAT_ACCURACY);
    int stat2 = ce2->getStat(Enums::STAT_AGILITY);
    if (!Entity::CheckWeaponMask(attackerWeaponId, Enums::WP_ALL_EASY_DODGE)) {
        stat2 = stat2 * 96 >> 8;
    }
    app->combat->crHitChance = (stat - stat2 << 8) / 100;
    app->combat->crHitChance -= 16 * n7;
    if (app->combat->crHitChance < 1) {
        app->combat->crHitChance = 1;
    }
    int nextByte = app->nextByte();
    if (app->combat->punchingMonster != 0) {
        if (app->combat->animLoopCount == 1 || !app->combat->punchMissed) {
            nextByte = 0;
        }
        else if (app->combat->punchMissed) {
            app->combat->playerMissRepetition = 0;
            nextByte = 255;
        }
    }
    int n8 = 1;

    if (eType == Enums::ET_MONSTER && eSubType == 14) {
        n8 = 99;
    }

    if (((b || app->combat->playerMissRepetition >= n8) && (!b || app->combat->monsterMissRepetition >= 2)) || nextByte <= app->combat->crHitChance || Entity::CheckWeaponMask(ce->weapon, Enums::WP_AUTO_HIT) || (!b && app->canvas->loadMapID >= 8 && app->combat->tileDist <= 1 && !(eType == 2 && eSubType == 14))) {
        if (b && app->player->statusEffects[(Enums::OFS_STATUSEFFECT_AMOUNT + Enums::STATUS_EFFECT_REFLECT)] > 0) {
            if (--app->player->statusEffects[(Enums::OFS_STATUSEFFECT_AMOUNT + Enums::STATUS_EFFECT_REFLECT)] == 0) {
                app->player->removeStatusEffect(Enums::STATUS_EFFECT_REFLECT);
            }
            app->player->translateStatusEffects();
            app->combat->crFlags |= Enums::CR_IMMUNE;
        }

        if (Entity::CheckWeaponMask(attackerWeaponId, Enums::WP_PUNCH_MASK)) {
            if (!app->combat->curTarget->hasHead()) {
                return app->combat->crFlags;
            }
            if (eType == Enums::ET_MONSTER && eSubType == 4) {
                return app->combat->crFlags |= Enums::CR_CRIT;
            }
        }

        if (b) {
            app->combat->monsterMissRepetition = 0;
        }
        else {
            app->combat->playerMissRepetition = 0;
        }

        if (!b || app->game->difficulty == 4) {
            app->combat->crCritChance = app->combat->crHitChance / 20;
        }
        else {
            app->combat->crCritChance = 0;
        }

        if (app->nextByte() < app->combat->crCritChance) {
            return app->combat->crFlags |= Enums::CR_CRIT;
        }
        return app->combat->crFlags |= Enums::CR_HIT;
    }

    
    if (b) {
        ++app->combat->monsterMissRepetition;
    }
    else {
        ++app->combat->playerMissRepetition;
    }

    if ((eType != Enums::ET_MONSTER || (eSubType != 7 && eSubType != 8)) &&
        (worldDistToTileDist > 1 || app->player->hasPurifyEffect() || 
            app->player->statusEffects[(Enums::OFS_STATUSEFFECT_TURNS + Enums::STATUS_EFFECT_DIZZY)] != 0 || 
            app->player->statusEffects[(Enums::OFS_STATUSEFFECT_TURNS + Enums::STATUS_EFFECT_DRUNK)] != 0)) {
        return app->combat->crFlags;
    }

    return app->combat->crFlags |= Enums::CR_DODGE;
    
}

int CombatEntity::calcDamage(CombatEntity* ce, Entity* entity, CombatEntity* ce2, bool b, int n) {
    Applet* app = CAppContainer::getInstance()->app;
    int weapon = ce->weapon * 9;
    int dmgStrMin = app->combat->weapons[weapon + Combat::WEAPON_FIELD_STRMIN] & 0xFF;
    int dmgStrMax = app->combat->weapons[weapon + Combat::WEAPON_FIELD_STRMAX] & 0xFF;

    if (entity->def->eType == Enums::ET_MONSTER && entity->def->eSubType == 14 && ce->weapon != 15) {
        app->combat->crArmorDamage = 0;
        app->combat->crDamage = 0;
        return 0;
    }
    else if (entity->def->eType == Enums::ET_MONSTER && entity->def->eSubType == 12) {
        app->sound->playSound(1121, 0, 3, false);
    }

    if (b && app->player->buffs[(Enums::OFS_BUFF_COUNT + Enums::BUFF_REFLECT)] > 0) {
        return app->combat->crDamage = dmgStrMax;
    }
    if (app->game->difficulty == 4) {
        dmgStrMin -= dmgStrMin >> 2;
    }
    
    if ((app->combat->crFlags & Enums::CR_CRIT) || (app->combat->crFlags & Enums::CR_CRIT_DAMAGE)) {
        dmgStrMin = (entity->def->eType != Enums::ET_MONSTER || !Entity::CheckWeaponMask(ce->weapon, Enums::WP_SNIPERMASK)) ? dmgStrMax * 2 : (entity->monster->ce.getStat(Enums::STAT_MAX_HEALTH) * 3) / 2;
    }
    else if (app->combat->crFlags & Enums::CR_WEAK_HIT) {
        dmgStrMin = dmgStrMax / 2;
    }
    else if (dmgStrMax != dmgStrMin) {
        dmgStrMin += app->nextByte() % (dmgStrMax - dmgStrMin);
    }

    if (!(app->combat->crFlags & Enums::CR_NOSTRBONUS)) {
        dmgStrMin += ce->getStatPercent(Enums::STAT_STRENGTH) * dmgStrMin >> 8;
    }
    int armorDamage = 0;
    int damage;
    if (!b) {
        if (app->player->buffs[(Enums::OFS_BUFF_COUNT + Enums::BUFF_ANGER)] > 0) {
            dmgStrMin += (app->player->buffs[(Enums::BUFF_MAX + Enums::BUFF_ANGER)] << 8) / 100 * dmgStrMin >> 8;
        }
        int weaponWeakness = app->combat->getWeaponWeakness(ce->weapon, entity->def->eSubType, entity->def->parm);
        damage = weaponWeakness * dmgStrMin >> 8;
    }
    else {
        armorDamage = std::min((87 * dmgStrMin) >> 8, ce2->getStat(Enums::STAT_ARMOR));
        damage = dmgStrMin - armorDamage;

        if (Entity::CheckWeaponMask(ce->weapon, Enums::WP_ANTI_FIRE) && app->player->buffs[(Enums::OFS_BUFF_COUNT + Enums::BUFF_ANTIFIRE)] > 0) {
            armorDamage = 0;
            damage = 0;
        }
    }

    int crDamage = damage - (ce2->getStatPercent(Enums::STAT_DEFENSE) * damage >> 8);
    if (app->combat->oneShotCheat && !b) {
        crDamage = 999;
    }
    app->combat->crArmorDamage = armorDamage;
    return app->combat->crDamage = crDamage;
}
