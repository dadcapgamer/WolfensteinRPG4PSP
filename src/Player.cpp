#include <stdexcept>
#include <cstring>

#include "CAppContainer.h"
#include "App.h"
#include "Player.h"
#include "Combat.h"
#include "Canvas.h"
#include "Render.h"
#include "Game.h"
#include "Text.h"
#include "Hud.h"
#include "Enums.h"
#include "TinyGL.h"
#include "JavaStream.h"
#include "Sound.h"
#include "Resource.h"
#include "MenuSystem.h"
#include "Menus.h"

constexpr int Player::WeaponSounds[18];

Player::Player() {
    memset(this, 0, sizeof(Player));
}

Player::~Player() {
}

bool Player::startup() {
    //printf("Player::startup\n");
    this->noclip = false;
    this->god = false;
    this->helpBitmask = 0;
    this->invHelpBitmask = 0;
    this->weaponHelpBitmask = 0;
    this->armorHelpBitmask = 0;
    this->totalDeaths = 0;
    this->enableHelp = true;
    this->baseCe = new CombatEntity();
    this->ce = new CombatEntity();
    this->soundFire = false;
    this->reset();

    return true;
}

bool Player::modifyCollision(Entity* entity) {
    Applet* app = CAppContainer::getInstance()->app;
    return nullptr != entity && entity->def->eType == Enums::ET_SPRITEWALL && (app->render->mapSpriteInfo[(entity->info & 0xFFFF) - 1] & Enums::SPRITE_MASK_SPRITENUMBER) == Enums::TILENUM_OBJ_PICTURE;
}

void Player::advanceTurn() {
    Applet* app = CAppContainer::getInstance()->app;

    this->moves++;
    this->totalMoves++;

    if (app->game->isUnderWater()) {
        if (this->statusEffects[(Enums::OFS_STATUSEFFECT_COUNT + Enums::STATUS_EFFECT_AIR)] == 0) {
            this->addStatusEffect(Enums::STATUS_EFFECT_AIR, 1, 11);
            this->statusEffects[(Enums::OFS_STATUSEFFECT_COUNT + Enums::STATUS_EFFECT_AIR)] = 1;
        }
        this->removeStatusEffect(Enums::STATUS_EFFECT_FIRE);
    }
    else {
        this->removeStatusEffect(Enums::STATUS_EFFECT_AIR);
    }

    Entity* findMapEntity = app->game->findMapEntity(app->canvas->viewX, app->canvas->viewY, 16384);
    if (findMapEntity != nullptr && findMapEntity->def->tileIndex == Enums::TILENUM_AIR_BUBBLES) {
        app->sound->playSound(1088, 0, 4, false);
        this->addStatusEffect(Enums::STATUS_EFFECT_AIR, 1, 11);
        this->statusEffects[(Enums::OFS_STATUSEFFECT_COUNT + Enums::STATUS_EFFECT_AIR)] = 1;
    }


    this->updateStatusEffects();
    bool b = false;

    if (this->buffs[(Enums::OFS_BUFF_COUNT + Enums::BUFF_AIR)] == 0 && app->game->isUnderWater()) {
        app->sound->playSound(1089, 0, 4, false);
        this->painEvent(nullptr, true);
        this->pain(10, nullptr, true);
        app->combat->totalDamage = 10;
        app->hud->damageTime = app->time + app->combat->BOMB_RECOVER_TIME;
        app->hud->addMessage(Strings::FILE_CODESTRINGS, CodeStrings::OUT_OF_AIR, 3);
        b = true;
    }


    if (this->buffs[(Enums::OFS_BUFF_COUNT + Enums::BUFF_REGEN)] > 0) {
        this->addHealth(this->buffs[(Enums::BUFF_MAX + Enums::BUFF_REGEN)], false);
    }

    if (this->statusEffects[(Enums::OFS_STATUSEFFECT_COUNT + Enums::STATUS_EFFECT_COLD)] > 0) {
        this->addHealth(-this->statusEffects[(Enums::OFS_STATUSEFFECT_AMOUNT + Enums::STATUS_EFFECT_COLD)]);
        b = true;
    }

    if (this->statusEffects[(Enums::OFS_STATUSEFFECT_TURNS + Enums::STATUS_EFFECT_DISEASE)] > 0) {
        this->addHealth(this->buffs[(Enums::OFS_BUFF_AMOUNT + Enums::BUFF_DISEASE)]);
        Text* text = app->hud->getMessageBuffer(0);
        app->localization->resetTextArgs();
        app->localization->addTextArg(-this->buffs[(Enums::OFS_BUFF_AMOUNT + Enums::BUFF_DISEASE)]);
        app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::DISEASE_DEALS, text);
        app->hud->finishMessageBuffer();
        b = true;

        if (app->canvas->state == 3) {
            app->game->gsprite_allocAnim(251, 0, 0, 32);
            app->canvas->blockInputTime = app->gameTime + app->combat->BOMB_RECOVER_TIME;
        }
    }

    if (this->statusEffects[(Enums::OFS_STATUSEFFECT_TURNS + Enums::STATUS_EFFECT_FIRE)] > 0) {
        this->addHealth(-3);
        Text* text = app->hud->getMessageBuffer(0);
        app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::IT_BURNS, text);
        text->append(' ');
        app->localization->resetTextArgs();
        app->localization->addTextArg(3);
        app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::X_DAMAGE, text);
        app->hud->finishMessageBuffer();
        b = true;
    }

    if ((this->weapons & (Enums::WP_SHIFT << Enums::WP_TESLA)) != 0) {
        this->give(Enums::IT_AMMO, Enums::AMMO_CHARGE, 17, true);
    }

    if (this->inCombat && this->totalMoves - this->lastCombatTurn >= 4) {
        this->inCombat = false;
    }

    if (this->statusEffects[(Enums::OFS_STATUSEFFECT_TURNS + Enums::STATUS_EFFECT_DRUNK)] > 0) {
        ++this->counters[3];
    }

    this->turnTime = app->time;
    if (b && app->canvas->state == Canvas::ST_AUTOMAP) {
        app->canvas->setState(Canvas::ST_PLAYING);
    }
}

void Player::levelInit() {
    Applet* app = CAppContainer::getInstance()->app;
    this->moves = 0;
    this->numNotebookIndexes = 0;
    this->questComplete = 0;
    this->questFailed = 0;
    this->turnTime = app->time;
    this->inCombat = false;
    if (this->ce->getStat(Enums::STAT_HEALTH) == 0) {
        this->ce->setStat(Enums::STAT_HEALTH, 1);
    }
}

void Player::fillMonsterStats() {
    Applet* app = CAppContainer::getInstance()->app;

    int n = 0;
    int n2 = 0;
    for (int i = 0; i < app->game->numEntities; ++i) {
        Entity* entity = &app->game->entities[i];
        if (entity->monster != nullptr) {
            if (!(entity->monster->flags & Enums::MFLAG_NOTRACK)) {
                ++n;
                if (app->game->entities[i].info & (Entity::ENTITY_FLAG_GIBBED | Entity::ENTITY_FLAG_CORPSE)) {
                    ++n2;
                }
            }
        }
    }

    this->monsterStats[0] = n2;
    this->monsterStats[1] = n;
}

void Player::readyWeapon() {
    CAppContainer::getInstance()->app->canvas->readyWeaponSound = 2;
}

void Player::selectWeapon(int i) {
    Applet* app = CAppContainer::getInstance()->app;

    if ((this->weapons & ~this->disabledWeapons & 1 << i) == 0x0) {
        this->ce->weapon = i;
        this->selectNextWeapon();
    }
    if (this->ce->weapon != i) {
        app->canvas->invalidateRect();
        this->prevWeapon = this->ce->weapon;
    }
    this->ce->weapon = i;
    if (app->canvas->state != Canvas::ST_DIALOG /* && app->canvas->state != Canvas::ST_CAMERA*/) {
        app->canvas->drawPlayingSoftKeys();
    }
    if (i != this->prevWeapon) {
        this->readyWeapon();
    }
    this->activeWeaponDef = app->entityDefManager->find(Enums::ET_ITEM, Enums::IT_WEAPON, i);
    app->canvas->updateFacingEntity = true;
    app->hud->repaintFlags |= 0x4;
}

void Player::selectPrevWeapon() {
    Applet* app = CAppContainer::getInstance()->app;

    if (app->canvas->isChickenKicking) {
        return;
    }

    int weapon = this->ce->weapon;
    int n = this->weapons & ~this->disabledWeapons;
    for (int i = weapon - 1; i >= 0; --i) {
        if ((n & 1 << i) != 0x0) {
            int n2 = i * Combat::WEAPON_MAX_FIELDS;
            if (app->combat->weapons[n2 + Combat::WEAPON_FIELD_AMMOUSAGE] == 0 || this->ammo[app->combat->weapons[n2 + Combat::WEAPON_FIELD_AMMOTYPE]] != 0) {
                this->selectWeapon(i);
                return;
            }
        }
    }
    if (this->ce->weapon == weapon && weapon != Enums::WP_ITEM) {
        for (int j = Enums::WP_ITEM; j > weapon; --j) {
            if ((n & 1 << j) != 0x0) {
                int n3 = j * Combat::WEAPON_MAX_FIELDS;
                if (app->combat->weapons[n3 + Combat::WEAPON_FIELD_AMMOUSAGE] == 0 || this->ammo[app->combat->weapons[n3 + Combat::WEAPON_FIELD_AMMOTYPE]] != 0) {
                    this->selectWeapon(j);
                    return;
                }
            }
        }
    }
}

void Player::selectNextWeapon() {
    Applet* app = CAppContainer::getInstance()->app;

    if (app->canvas->isChickenKicking) {
        return;
    }

    int weapon = this->ce->weapon;
    int n = this->weapons & ~this->disabledWeapons;
    for (int i = weapon + 1; i < Enums::WP_PLAYERMAX; ++i) {
        if ((n & 1 << i) != 0x0) {
            int n2 = i * Combat::WEAPON_MAX_FIELDS;
            if (app->combat->weapons[n2 + Combat::WEAPON_FIELD_AMMOUSAGE] == 0 || this->ammo[app->combat->weapons[n2 + Combat::WEAPON_FIELD_AMMOTYPE]] != 0) {
                this->selectWeapon(i);
                return;
            }
        }
    }
    if (this->ce->weapon == weapon && weapon != 0) {
        for (int j = 0; j < weapon; ++j) {
            if ((n & 1 << j) != 0x0) {
                int n3 = j * Combat::WEAPON_MAX_FIELDS;
                if (app->combat->weapons[n3 + Combat::WEAPON_FIELD_AMMOUSAGE] == 0 || this->ammo[app->combat->weapons[n3 + Combat::WEAPON_FIELD_AMMOTYPE]] != 0) {
                    this->selectWeapon(j);
                    return;
                }
            }
        }
    }
}

void Player::modifyStat(int n, int n2) {
    if (n == 0) {
        this->addHealth(n2);
        if (n2 < 0) {
            this->painEvent(nullptr, false);
        }
    }
    else {
        this->baseCe->addStat(n, n2);
        this->updateStats();
    }
}

bool Player::requireStat(int n, int n2) {
    return this->ce->getStat(n) >= n2;
}

bool Player::requireItem(int n, int n2, int n3, int n4) {
    int n5 = 1 << n2;
    if (n != 1) {
        return n == 0 && this->inventory[n2 - 0] >= n3 && this->inventory[n2 - 0] <= n4;
    }
    if (n4 != 0) {
        return (this->weapons & ((int64_t)n5)) != 0x0;
    }
    return (this->weapons & ((int64_t)n5)) == 0x0;
}

void Player::addXP(int xp) {
    Applet* app = CAppContainer::getInstance()->app;

    app->localization->resetTextArgs();
    app->localization->addTextArg(xp);
    if (xp < 0) {
        app->hud->addMessage(CodeStrings::LOST_X_XP);
    }
    else {
        app->hud->addMessage(CodeStrings::GAINED_X_XP);
    }
    this->currentXP += xp;
    this->xpGained += xp;
    while (this->currentXP >= this->nextLevelXP) {
        this->addLevel();
    }
    this->counters[5] += xp;
}

void Player::addLevel() {
    Applet* app = CAppContainer::getInstance()->app;
    Text* textBuff;
    int stat;

    this->level++;
    this->nextLevelXP = this->calcLevelXP(this->level);

    textBuff = app->localization->getLargeBuffer();
    textBuff->setLength(0);
    app->localization->resetTextArgs();
    app->localization->addTextArg(this->level);
    app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::LEVEL_UP, textBuff);

    int n = 10;
    stat = this->baseCe->getStat(Enums::STAT_MAX_HEALTH);
    if (stat + n > 999) {
        n = 999 - stat;
    }
    if (n != 0) {
        this->baseCe->setStat(Enums::STAT_MAX_HEALTH, stat + n);
        app->localization->resetTextArgs();
        app->localization->addTextArg(n);
        app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::MAX_HEALTH, textBuff);
    }

    n = 1;
    stat = this->baseCe->getStat(Enums::STAT_DEFENSE);
    if (stat + n > 99) {
        n = 99 - stat;
    }
    if (n != 0) {
        this->baseCe->setStat(Enums::STAT_DEFENSE, stat + n);
        app->localization->resetTextArgs();
        app->localization->addTextArg(n);
        app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::DEFENSE, textBuff);
    }

    n = 2;
    stat = this->baseCe->getStat(Enums::STAT_STRENGTH);
    if (stat + n > 99) {
        n = 99 - stat;
    }
    if (n != 0) {
        this->baseCe->setStat(Enums::STAT_STRENGTH, stat + n);
        app->localization->resetTextArgs();
        app->localization->addTextArg(n);
        app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::STRENGTH, textBuff);
    }

    n = 1;
    stat = this->baseCe->getStat(Enums::STAT_ACCURACY);
    if (stat + n > 99) {
        n = 99 - stat;
    }
    if (n != 0) {
        this->baseCe->setStat(Enums::STAT_ACCURACY, stat + n);
        app->localization->resetTextArgs();
        app->localization->addTextArg(n);
        app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::ACCURACY, textBuff);
    }

    n = 3;
    stat = this->baseCe->getStat(Enums::STAT_AGILITY);
    if (stat + n > 99) {
        n = 99 - stat;
    }
    if (n != 0) {
        this->baseCe->setStat(Enums::STAT_AGILITY, stat + n);
        app->localization->resetTextArgs();
        app->localization->addTextArg(n);
        app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::AGILITY, textBuff);
    }

    this->updateStats();
    app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::HEALTH_RESTORED, textBuff);

    this->ce->setStat(Enums::STAT_HEALTH, this->ce->getStat(Enums::STAT_MAX_HEALTH));
    app->hud->repaintFlags |= 0x4;

    bool dispose = true;
    if (app->canvas->state != Canvas::ST_MENU) {
        app->sound->playSound(1076, 0, 6, false);
        dispose = (app->canvas->enqueueHelpDialog(textBuff, Canvas::HELPMSG_TYPE_NONE) ? false : true);
    }

    if (dispose) {
        textBuff->dispose();
    }
}

int Player::calcLevelXP(int n) {
    return 500 * n + 100 * ((n - 1) * (n - 1) * (n - 1) + (n - 1));
}

int Player::calcScore() {
    Applet* app = CAppContainer::getInstance()->app;
    int n = 0;
    bool b = true;
    int i;
    for (i = 0; i <= 8; ++i) {
        if ((this->killedMonstersLevels & 1 << i) != 0x0) {
            n += 1000;
        }
        else {
            b = false;
        }
    }
    if (b) {
        n += 1000;
    }
    if (this->totalDeaths == 0) {
        n += 1000;
    }
    else if (this->totalDeaths < 10) {
        n += (5 - this->totalDeaths) * 50;
    }
    else {
        n -= 250;
    }
    int n2 = (this->totalTime + (app->gameTime - this->playTime)) / 60000;
    if (n2 < 120) {
        n += (120 - n2) * 15;
    }
    if (this->totalMoves < 5000) { // 4600
        n += (5000 - this->totalMoves) / 2;
    }
    bool b2 = true;
    for (i = 0; i <= 8; ++i) {
        if ((this->foundSecretsLevels & 1 << i) != 0x0) {
            n += 1000;
        }
        else {
            b2 = false;
        }
    }
    if (b2) {
        n += 1000;
    }

    if (app->resource->getNumTableBytes(17)) {
        for (int i5 = 0; i5 < 2; i5++) {
            int i6 = this->medals[i5];
            for (int i7 = 0; i7 < 32; i7++) {
                // if ((i6 & 1 << i7) == 1)  // <- [Original Code] A bug? It only verifies one medal per field. / ¿Un error? Solo verifica una medalla por campo.
                if ((i6 & 1 << i7) != 0x0) { // <- [GEC] Allows you to verify all the medals collected. / Permite verificar todas las medallas colectadas.
                    n += 100;
                }
            }
        }
    }
    return n;
}

bool Player::addHealth(int i) {
    return this->addHealth(i, true);
}

bool Player::addHealth(int i, bool b) {
    Applet* app = CAppContainer::getInstance()->app;

    app->hud->repaintFlags |= 0x4;
    int stat;
    int stat2;

    stat = this->ce->getStat(Enums::STAT_HEALTH);
    stat2 = this->ce->getStat(Enums::STAT_MAX_HEALTH);

    if (i > 0) {
        if (stat == stat2) {
            return false;
        }
    }
    else if (this->god) {
        return false;
    }

    app->hud->playerStartHealth = stat;
    this->ce->addStat(Enums::STAT_HEALTH, i);

    int n2 = this->ce->getStat(Enums::STAT_HEALTH);
    if (b && n2 > stat) {
        app->localization->resetTextArgs();
        app->localization->addTextArg(n2 - stat);
        app->hud->addMessage(CodeStrings::GAINED_HEALTH);
    }
    return true;
}

void Player::reset() {
    Applet* app = CAppContainer::getInstance()->app;

    app->hud->msgCount = 0;
    this->numNotebookIndexes = 0;
    this->resetCounters();
    this->level = 1;
    this->currentXP = 0;
    this->nextLevelXP = this->calcLevelXP(this->level);
    this->facingEntity = nullptr;
    this->noclip = false;
    this->questComplete = 0;
    this->questFailed = 0;
    this->purchasedWeapons = 0;
    this->pickingStats = 0;
    this->gold = 0;

    this->give(Enums::IT_MONEY, 0, 2, true);

    app->canvas->viewX = (app->canvas->destX = (app->canvas->saveX = (app->canvas->prevX = 0)));
    app->canvas->viewY = (app->canvas->destY = (app->canvas->saveY = (app->canvas->prevY = 0)));
    app->canvas->viewZ = (app->canvas->destZ = 36);
    app->canvas->viewAngle = (app->canvas->destAngle = (app->canvas->saveAngle = 0));
    app->canvas->viewPitch = (app->canvas->destPitch = (app->canvas->savePitch = 0));
    this->inCombat = false;
    for (int j = 0; j < 10; ++j) {
        this->ammo[j] = 0;
    }
    for (int k = 0; k < Enums::INV_MAX; ++k) {
        this->inventory[k] = 0;
    }
    this->numbuffs = 0;
    for (int l = 0; l < Enums::BUFF_MAX; ++l) {
        this->buffs[Enums::OFS_BUFF_AMOUNT + l] = (this->buffs[Enums::OFS_BUFF_COUNT + l] = 0);
    }
    this->numStatusEffects = 0;
    for (int n = 0; n < Enums::STATUS_EFFECT_ALL; ++n) {
        this->statusEffects[Enums::OFS_STATUSEFFECT_TURNS + n] = 0;
        this->statusEffects[Enums::OFS_STATUSEFFECT_AMOUNT + n] = 0;
        this->statusEffects[Enums::OFS_STATUSEFFECT_COUNT + n] = 0;
    }
    for (int n5 = 0; n5 < 2; ++n5) {
        this->medals[n5] = 0;
    }
    for (int n6 = 0; n6 < 4; ++n6) {
        this->foundBooks[n6] = 0;
    }

    this->weapons = 9LL;
    this->foundSecretsLevels = 0;
    this->killedMonstersLevels = 0;
    this->baseCe->setStat(Enums::STAT_MAX_HEALTH, 100);
    this->baseCe->setStat(Enums::STAT_DEFENSE, 10);
    this->baseCe->setStat(Enums::STAT_STRENGTH, 10);
    this->baseCe->setStat(Enums::STAT_ACCURACY, 95);
    this->baseCe->setStat(Enums::STAT_AGILITY, 8);
    if (app->game->difficulty == 2) {
        this->baseCe->setStat(Enums::STAT_DEFENSE, 0);
    }
    this->updateStats();
    this->ce->setStat(Enums::STAT_HEALTH, 100);
    this->ce->weapon = 0;
    this->totalTime = 0;
    this->totalMoves = 0;
    this->completedLevels = 0;
    this->highestMap = 1;
    this->gameCompleted = false;
    this->cocktailDiscoverMask = 0;
    this->gamePlayedMask = 0;
}

int Player::calcDamageDir(int x1, int y1, int angle, int x2, int y2) {
    int dx = x2 - x1;
    int dy = y2 - y1;
    angle &= Enums::ANGLE_FULL;
    if (dx == 0 && dy == 0) {
        return Enums::DIR_NORTHWEST;
    }
    int ang;
    if (dx > 0) {
        if (dy < 0) {
            ang = Enums::ANGLE_NORTHEAST;
        }
        else if (dy > 0) {
            ang = Enums::ANGLE_SOUTHEAST;
        }
        else {
            ang = Enums::ANGLE_EAST;
        }
    }
    else if (dx < 0) {
        if (dy < 0) {
            ang = Enums::ANGLE_NORTHWEST;
        }
        else if (dy > 0) {
            ang = Enums::ANGLE_SOUTHWEST;
        }
        else {
            ang = Enums::ANGLE_WEST;
        }
    }
    else if (dy > 0) {
        ang = Enums::ANGLE_SOUTH;
    }
    else {
        ang = Enums::ANGLE_NORTH;
    }

    int dAng = (ang - angle) & Enums::ANGLE_FULL;
    if (dAng > Enums::ANGLE_PI) {
        dAng = -(Enums::ANGLE_2PI - dAng);
    }
    int dir = (dAng / Enums::ANGLE_PI_FOURTH) + Enums::DIR_NORTHWEST;
    if (dir < 0) {
        dir = Enums::DIR_SOUTHEAST;
    }
    return dir;
}

void Player::painEvent(Entity* entity, bool b) {
    Applet* app = CAppContainer::getInstance()->app;

    if (!this->god) {
        if (entity == nullptr) {
            app->hud->damageDir = 3;
        }
        app->hud->damageCount = 1;
        if (!b) {
            app->canvas->startShake(500, 2, 150);
        }
        else { // [GEC] Damage From Monster
            app->canvas->startShake(0, 0, 150);
        }
    }
}

void Player::pain(int n, Entity* entity, bool b) {
    Applet* app = CAppContainer::getInstance()->app;
    if (this->god) {
        return;
    }
    if (b) {
        app->localization->resetTextArgs();
        if (entity != nullptr) {
            Text* smallBuffer = app->localization->getSmallBuffer();
            app->localization->composeText(Strings::FILE_ENTITYSTRINGS, (short)(entity->def->name & 0x3FF), smallBuffer);
            app->localization->addTextArg(smallBuffer);
            app->localization->addTextArg(n);
            app->hud->addMessage(Strings::FILE_CODESTRINGS, CodeStrings::X_DID_Y_DAMAGE);
            smallBuffer->dispose();
        }
        else {
            app->localization->addTextArg(n);
            app->hud->addMessage(Strings::FILE_CODESTRINGS, CodeStrings::X_DAMAGE);
        }
    }
    if (n == 0) {
        return;
    }

    int stat = this->ce->getStat(Enums::STAT_HEALTH);
    if (n >= stat && this->noDeathFlag) {
        n = stat - 1;
    }
    int n2 = (stat << 16) / (this->ce->getStat(Enums::STAT_MAX_HEALTH) << 8);
    int n3 = (stat - n << 16) / (this->ce->getStat(Enums::STAT_MAX_HEALTH) << 8);
    if (n3 > 0) {
        if (n2 > 26 && n3 <= 26) {
            app->hud->addMessage(CodeStrings::NEAR_DEATH, 3);
        }
        else if (n2 > 78 && n3 <= 78) {
            app->hud->addMessage(CodeStrings::LOW_HEALTH, 3);
        }
        else if (n2 > 128 && n3 <= 128 && (this->helpBitmask & 0x6000) == 0x0) {
            if (this->inventory[Enums::INV_HEALTH_PACK] != 0 || this->inventory[Enums::INV_HEALTH_RATION_BAR] != 0) {
                this->showHelp(CodeStrings::GENHELP_HEALTH_WARNING1, true);
            }
            else {
                this->showHelp(CodeStrings::GENHELP_HEALTH_WARNING2, true);
            }
        }
    }
    this->addHealth(-n);

    if (app->canvas->state == Canvas::ST_AUTOMAP) {
        app->canvas->setState(Canvas::ST_PLAYING);
    }
    if (this->ce->getStat(Enums::STAT_HEALTH) <= 0) {
        this->died();
    }
}

void Player::died() {
    Applet* app = CAppContainer::getInstance()->app;

    if (app->canvas->state == Canvas::ST_DYING) {
        return;
    }
    app->sound->playSound(1087, 0, 5, false);
    this->totalDeaths++;
    app->canvas->startShake(350, 5, 500);
    this->ce->setStat(Enums::STAT_HEALTH, 0);
    app->canvas->setState(Canvas::ST_DYING);
    app->game->combatMonsters = nullptr;
}

bool Player::fireWeapon(Entity* entity, int n, int n2) {
    Applet* app = CAppContainer::getInstance()->app;

    if (app->combat->weaponDown || (this->disabledWeapons != 0 && !(this->weapons & 1 << this->ce->weapon))) {
        return false;
    }

    if (app->combat->lerpingWeapon) {
        if (app->combat->lerpWpDown) {
            return false;
        }
        app->combat->lerpingWeapon = false;
        app->combat->weaponDown = false;
    }

    if (entity->monster != nullptr) {
        entity->monster->flags &= ~Enums::MFLAG_NOACTIVATE;
    }


    int weapon = this->ce->weapon * Combat::WEAPON_MAX_FIELDS;
    if (app->combat->weapons[weapon + Combat::WEAPON_FIELD_AMMOTYPE] != 0) {
        short n4 = this->ammo[app->combat->weapons[weapon + Combat::WEAPON_FIELD_AMMOTYPE]];
        if (app->combat->weapons[weapon + Combat::WEAPON_FIELD_AMMOUSAGE] > 0 && (n4 - app->combat->weapons[weapon + Combat::WEAPON_FIELD_AMMOUSAGE]) < 0) {
            if (this->ce->weapon == Enums::WP_TESLA) {
                app->hud->addMessage(CodeStrings::OUT_OF_ENERGY, 3);
            }
            else {
                app->hud->addMessage(CodeStrings::OUT_OF_AMMO, 3);
            }
            return false;
        }
    }
    app->combat->performAttack(nullptr, entity, n, n2, false);
    return true;
}

bool Player::useItem(int n) {
    Applet* app = CAppContainer::getInstance()->app;
    bool b2, b3, b4;

    if (this->inventory[n] == 0) {
        return false;
    }

    bool b = true;
    switch (n) {
    case Enums::INV_SYRINGE_EVADE:
        if (this->addStatusEffect(Enums::STATUS_EFFECT_EVADE, 20, 31)) {
            this->translateStatusEffects();
            break;
        }
        else {
            return false;
        }
    case Enums::INV_SYRINGE_DEFENSE:
        if (this->addStatusEffect(Enums::STATUS_EFFECT_DEFENSE, 20, 31)) {
            this->translateStatusEffects();
            break;
        }
        else {
            return false;
        }
    case Enums::INV_SYRINGE_ADRENALINE:
        if (this->addStatusEffect(Enums::STATUS_EFFECT_ADRENALINE, 20, 31)) {
            this->translateStatusEffects();
            break;
        }
        else {
            return false;
        }
    case Enums::INV_SYRINGE_FOCUS:
        if (this->addStatusEffect(Enums::STATUS_EFFECT_FOCUS, 5, 31)) {
            this->translateStatusEffects();
            break;
        }
        else {
            return false;
        }
    case Enums::INV_SYRINGE_ANTIFIRE:
        if (this->addStatusEffect(Enums::STATUS_EFFECT_ANTIFIRE, 0, 11)) {
            this->removeStatusEffect(Enums::STATUS_EFFECT_FIRE);
            this->translateStatusEffects();
            break;
        }
        else {
            return false;
        }
    case Enums::INV_SYRINGE_ENRAGE:
        if (this->addStatusEffect(Enums::STATUS_EFFECT_ANGER, 100, 31)) {
            this->translateStatusEffects();
            break;
        }
        else {
            return false;
        }
    case Enums::INV_SYRINGE_AGILITY:
        if (this->statusEffects[(Enums::OFS_STATUSEFFECT_TURNS + Enums::STATUS_EFFECT_AGILITY)] == 0) {
            b = false;
            this->addStatusEffect(Enums::STATUS_EFFECT_AGILITY, 0, 21);
            this->translateStatusEffects();
            break;
        }
        else {
            return false;
        }
    case Enums::INV_SYRINGE_FORTITUDE:
        if (this->addStatusEffect(Enums::STATUS_EFFECT_FORTITUDE, 20, 31)) {
            this->translateStatusEffects();
            this->addHealth(20);
            break;
        }
        else {
            return false;
        }
    case Enums::INV_SYRINGE_REGEN:
        if (this->addStatusEffect(Enums::STATUS_EFFECT_REGEN, 5, 31)) {
            this->translateStatusEffects();
            break;
        }
        else {
            return false;
        }
    case Enums::INV_SYRINGE_PURIFY:
        if (this->statusEffects[(Enums::OFS_STATUSEFFECT_TURNS + Enums::STATUS_EFFECT_PURIFY)] == 0) {
            this->removeStatusEffect(Enums::STATUS_EFFECT_DRUNK);
            this->removeStatusEffect(Enums::STATUS_EFFECT_DISEASE);
            this->removeStatusEffect(Enums::STATUS_EFFECT_DIZZY);
            this->removeStatusEffect(Enums::STATUS_EFFECT_COLD);
            this->addStatusEffect(Enums::STATUS_EFFECT_PURIFY, 0, 11);
            this->translateStatusEffects();
            break;
        }
        else {
            return false;
        }
    case Enums::INV_SYRINGE_REFLECT:
        if (this->addStatusEffect(Enums::STATUS_EFFECT_REFLECT, 5, 31)) {
            this->translateStatusEffects();
            break;
        }
        else {
            return false;
        }
    case Enums::INV_SYRINGE_PROTO:
        b2 = false;
        if (this->addStatusEffect(Enums::STATUS_EFFECT_FORTITUDE, 50, 31)) {
            b2 = true;
        }
        if (this->addStatusEffect(Enums::STATUS_EFFECT_ADRENALINE, 25, 31)) {
            b2 = true;
        }
        if (this->addStatusEffect(Enums::STATUS_EFFECT_DEFENSE, 25, 31)) {
            b2 = true;
        }
        if (b2) {
            this->translateStatusEffects();
            break;
        }
        else {
            return false;
        }
    case Enums::INV_SYRINGE_ELITE:
        b3 = false;
        if (this->addStatusEffect(Enums::STATUS_EFFECT_FOCUS, 15, 31)) {
            b3 = true;
        }
        if (this->addStatusEffect(Enums::STATUS_EFFECT_ANGER, 50, 31)) {
            b3 = true;
        }
        if (this->addStatusEffect(Enums::STATUS_EFFECT_EVADE, 15, 31)) {
            b3 = true;
        }
        if (b3) {
            this->translateStatusEffects();
            break;
        }
        else {
            return false;
        }
    case Enums::INV_SYRINGE_FEAR:
        if (this->statusEffects[(Enums::OFS_STATUSEFFECT_COUNT + Enums::STATUS_EFFECT_FEAR)] == 0 && this->addStatusEffect(Enums::STATUS_EFFECT_FEAR, 50, 7)) {
            this->translateStatusEffects();
            break;
        }
        else {
            return false;
        }
        break;
    case Enums::INV_SYRINGE_ANGER:
        if (this->addStatusEffect(Enums::STATUS_EFFECT_ANGER, 50, 31)) {
            this->translateStatusEffects();
            break;
        }
        else {
            return false;
        }
    case Enums::INV_HEALTH_RATION_BAR:
        if (!this->addHealth(20)) {
            return false;
        }
        break;
    case Enums::INV_HEALTH_PACK:
        if (!this->addHealth(80)) {
            return false;
        }
        break;
    case Enums::INV_OTHER_SCOTCH:
        b4 = false;
        if (this->addHealth(20)) {
            b4 = true;
        }
        if (this->addStatusEffect(Enums::STATUS_EFFECT_DRUNK, 10, 31)) {
            b4 = true;
        }
        if (b4) {
            this->translateStatusEffects();
            break;
        }
        else {
            return false;
        }
    case Enums::INV_OTHER_PACK:
        return true;
    default:
        return false;
    }

    --this->inventory[n];
    if (b) {
        app->game->advanceTurn();
    }
    return true;
}

void Player::giveGold(int n) {
    this->gold += n;
    if (this->gold < 0) {
        this->gold = 0;
    }

}
bool Player::give(int n, int n2, int n3) {
    return this->give(n, n2, n3, false);
}

bool Player::give(int n, int n2, int n3, bool b) {
    Applet* app = CAppContainer::getInstance()->app;
    bool b2, b3, b4;
    int i5, i6;
    if (n3 == 0) {
        return false;
    }
    int i4 = 1 << n2;
    switch (n) {
    case Enums::IT_INVENTORY:
        i5 = n3 + this->inventory[n2 - 0];
        if (i5 > Enums::INV_MAX_ITEM_COUNT) {
            i5 = Enums::INV_MAX_ITEM_COUNT;
        }
        if (i5 < 0) {
            return false;
        }
        this->inventory[n2 - 0] = (short)i5;
        if (n3 > 0 && n2 >= Enums::INV_TREASURE_MIN && n2 <= Enums::INV_TREASURE_MAX) {
            this->counters[0] += n3;
        }
        if (b) {
            return true;
        }
        this->showInvHelp(n2 - 0, false);
        return true;
    case Enums::IT_WEAPON:
        if (n2 == Enums::WP_PISTOL && ((this->weapons & (Enums::WP_SHIFT << Enums::WP_PISTOL)) || (this->weapons & (Enums::WP_SHIFT << Enums::WP_DUAL_PISTOL)))) {
            n2 = Enums::WP_DUAL_PISTOL;
            this->weapons &= (int64_t)(~i4);
            i4 = Enums::WP_SHIFT << Enums::WP_DUAL_PISTOL;
        }
        b2 = this->ce->weapon != Enums::WP_MOUNTED_GUN_TURRET && ((this->weapons & ((int64_t)i4)) == 0);
        if (Entity::CheckWeaponMask(n2, Enums::WP_PUNCH_MASK)) {
            int32_t weap_masks = app->combat->weaponMasks[(2 * Enums::WP_PUNCH_MASK) + 0];
            i4 = std::max(i4, (int)((int32_t)this->weapons & weap_masks));
            this->weapons &= (int64_t)(~weap_masks);
        }
        if (n3 < 0) {
            this->weapons &= (int64_t)(~i4);
            if (n2 != this->ce->weapon) {
                return true;
            }
            this->selectNextWeapon();
            return true;
        }
        if (Entity::CheckWeaponMask(n2, Enums::WP_SNIPERMASK)) {
            int32_t weap_masks = app->combat->weaponMasks[(2 * Enums::WP_SNIPERMASK) + 0];
            i4 = std::max(i4, (int)((int32_t)this->weapons & weap_masks));
            this->weapons &= (int64_t)(~weap_masks);
        }
        this->weapons |= (int64_t)i4;
        if (!b) {
            this->showWeaponHelp(n2, false);
        }
        if (!b2) {
            return true;
        }
        this->selectWeapon(n2);
        return true;
    case Enums::IT_AMMO:
        i6 = n3 + this->ammo[n2];
        if (i6 > Enums::AMMO_MAX_COUNT) {
            i6 = Enums::AMMO_MAX_COUNT;
        }
        if (i6 < 0) {
            return false;
        }
        if (n2 == Enums::AMMO_DYNAMITE) {
            this->give(Enums::IT_WEAPON, Enums::WP_DYNAMITE, 1, true);
        }
        this->ammo[n2] = (short)i6;
        if (!b) {
            this->showAmmoHelp(n2, false);
        }
        app->hud->repaintFlags |= 4;
        return true;
    case Enums::IT_MONEY:
        if (this->gold + n3 < 0) {
            return false;
        }
        this->gold += n3;
        if (n3 <= 0) {
            return true;
        }
        this->counters[0] += n3;
        return true;
    case Enums::IT_FOOD:
        this->addHealth(n3);
        return true;
    case Enums::IT_SACK:
    default:
        return false;
    case Enums::IT_ARMOR:
        this->addArmor(n2 * 50);
        if (n) {
            return true;
        }
        this->showArmorHelp(n2, false);
        return true;
    }
}

void Player::giveAmmoWeapon(int n, bool b) {
    this->weapons |= (int64_t)(1 << (n & 0xffU));
    this->selectWeapon(n);
    if (!b) {
        this->showWeaponHelp(n, false);
    }
}

void Player::updateQuests(short n, int n2) {
    Applet* app = CAppContainer::getInstance()->app;

    if (n2 == 0) {
        if (this->numNotebookIndexes == 8) {
            app->Error(Enums::ERR_MAX_NOTEBOOKINDEXES); // J2ME
            return;
        }
        this->questComplete &= (uint8_t)~(1 << this->numNotebookIndexes);
        this->questFailed &= (uint8_t)~(1 << this->numNotebookIndexes);
        this->notebookIndexes[this->numNotebookIndexes++] = n;
    }
    else {
        for (int i = 0; i < this->numNotebookIndexes; ++i) {
            if (n == this->notebookIndexes[i]) {
                if (n2 == 1) {
                    this->questComplete |= (uint8_t)(1 << i);
                }
                else if (n2 == 2) {
                    this->questFailed |= (uint8_t)(1 << i);
                }
                return;
            }
        }
        if (n2 == 1) {
            this->questComplete |= (uint8_t)(1 << this->numNotebookIndexes);
            this->questFailed &= (uint8_t)~(1 << this->numNotebookIndexes);
        }
        else if (n2 == 2) {
            this->questComplete &= (uint8_t)~(1 << this->numNotebookIndexes);
            this->questFailed |= (uint8_t)(1 << this->numNotebookIndexes);
        }
        this->notebookPositions[this->numNotebookIndexes] = 0;
        this->notebookIndexes[this->numNotebookIndexes++] = n;
    }
}

void Player::setQuestTile(int n, int n2, int n3) {
    for (int i = 0; i < this->numNotebookIndexes; ++i) {
        if (n == this->notebookIndexes[i]) {
            this->notebookPositions[i] = (short)(n2 << 5 | n3);
            return;
        }
    }
}

bool Player::isQuestDone(int n) {
    return (this->questComplete & 1 << n) != 0x0;
}

bool Player::isQuestFailed(int n) {
    return (this->questFailed & 1 << n) != 0x0;
}

void Player::formatTime(int n, Text* text) {
    text->setLength(0);
    int n2 = n / 1000;
    int n3 = n2 / 60;
    int n4 = n3 / 60;
    int n5 = n3 % 60;
    text->append(n4);
    text->append(":");
    if (n5 < 10) {
        text->append("0");
    }
    text->append(n5);
    text->append(":");
    int n6 = n2 - n3 * 60;
    if (n6 < 10) {
        text->append("0");
    }
    text->append(n6);
}

void Player::showInvHelp(int n, bool b) {
    Applet* app = CAppContainer::getInstance()->app;

    if (this->enableHelp || b) {
        int n2 = n - 0;
        if (!(this->invHelpBitmask & 1 << n2)) {
            this->invHelpBitmask |= 1 << n2;
            app->canvas->enqueueHelpDialog(app->entityDefManager->find(Enums::ET_ITEM, Enums::IT_INVENTORY, n));
        }
    }
}

void Player::showAmmoHelp(int n, bool b) {
    Applet* app = CAppContainer::getInstance()->app;

    if (this->enableHelp || b) {
        int n2 = 1 << n;
        if (!(this->ammoHelpBitmask & n2)) {
            this->ammoHelpBitmask |= n2;
            app->canvas->enqueueHelpDialog(app->entityDefManager->find(Enums::ET_ITEM, Enums::IT_AMMO, n));
        }
    }
}

void Player::showHelp(short n, bool b) {
    Applet* app = CAppContainer::getInstance()->app;

    if (app->game->isCameraActive()) {
        return;
    }
    if (!this->enableHelp && !b) {
        return;
    }
    if ((this->helpBitmask & 1 << n) != 0x0 && !b) {
        return;
    }
    this->helpBitmask |= 1 << n;
    app->canvas->enqueueHelpDialog(n);
    if (n != CodeStrings::GENHELP_JOURNAL && (app->canvas->state == Canvas::ST_AUTOMAP || app->canvas->state == Canvas::ST_PLAYING)) {
        app->canvas->dequeueHelpDialog();
    }
}

void Player::showWeaponHelp(int n, bool b) {
    Applet* app = CAppContainer::getInstance()->app;

    if ((this->enableHelp || b) && !(this->weaponHelpBitmask & 1 << n)) {
        this->weaponHelpBitmask |= 1 << n;
        app->canvas->enqueueHelpDialog(app->entityDefManager->find(Enums::ET_ITEM, Enums::IT_WEAPON, n));
    }
}

void Player::showArmorHelp(int n, bool b) {
    Applet* app = CAppContainer::getInstance()->app;

    if ((this->enableHelp || b) && !(this->armorHelpBitmask & 1 << n)) {
        this->armorHelpBitmask |= 1 << n;
        app->canvas->enqueueHelpDialog(app->entityDefManager->find(Enums::ET_ITEM, Enums::IT_ARMOR, n));
    }
}

void Player::drawBuffs(Graphics* graphics) {
    Applet* app = CAppContainer::getInstance()->app;
    if (this->numbuffs == 0 || app->canvas->state == Canvas::ST_DIALOG) {
        return;
    }
    int n = app->canvas->viewRect[0] + app->canvas->viewRect[2];
    int n2 = n - 32;
    int n3 = app->canvas->viewRect[1] + 2;
    int n4 = 0;
    bool b = false;
    int numbuffs = this->numbuffs;
    if (numbuffs > 6) {
        numbuffs = 6;
        b = true;
    }
    int n5 = numbuffs * 31 + 6;
    for (int n6 = 0; n6 < Enums::BUFF_MAX && n4 < 6; ++n6) {
        if (this->buffs[Enums::OFS_BUFF_COUNT + n6] > 0 && (1 << n6 & Enums::BUFF_AMT_NOT_DRAWN) == 0x0) {
            if (this->buffs[Enums::OFS_BUFF_AMOUNT + n6] > 99 || this->buffs[Enums::OFS_BUFF_AMOUNT + n6] < -99) {
                n2 = n - 73;
                break;
            }
            if (this->buffs[Enums::OFS_BUFF_AMOUNT + n6] > 9 || this->buffs[Enums::OFS_BUFF_AMOUNT + n6] < -9) {
                n2 = n - 65;
            }
            else if (n2 == n - 32) {
                n2 = n - 57;
            }
        }
    }
    int n7 = n - n2 + 4;
    if (b) {
        n5 += 5;
    }
    graphics->setColor(0);
    graphics->fillRect(n2 - 5, n3 - 2, n7, n5);
    graphics->setColor(0xFFAAAAAA);
    graphics->drawRect(n2 - 5, n3 - 2, n7, n5);
    int n8 = n - 36;
    for (int n9 = 0; n9 < Enums::BUFF_MAX && n4 < 6; ++n9) {
        if (this->buffs[Enums::OFS_BUFF_COUNT + n9] != 0) {
            ++n4;
            this->drawStatusEffectIcon(graphics, n9, this->buffs[Enums::OFS_BUFF_AMOUNT + n9], this->buffs[Enums::OFS_BUFF_COUNT + n9], n8, n3);
            n3 += 31;
        }
    }
    if (b) {
        Text* smallBuffer = app->localization->getSmallBuffer();
        smallBuffer->setLength(0);
        smallBuffer->append('\x85');
        graphics->drawString(smallBuffer, n2 + n7 / 2 - 4, n3 - 4, 1);
        smallBuffer->dispose();
    }
}

bool Player::loadState(InputStream* IS) {
    Applet* app = CAppContainer::getInstance()->app;
    this->baseCe->loadState(IS, true);
    this->ce->loadState(IS, true);
    this->purchasedWeapons = IS->readInt();
    this->weapons = IS->readInt();
    this->weaponsCopy = IS->readInt();
    this->level = (IS->readByte() & 0xFF);
    this->currentXP = IS->readInt();
    this->nextLevelXP = this->calcLevelXP(this->level);
    this->gold = IS->readShort();
    this->totalTime = IS->readInt();
    this->totalMoves = IS->readInt();
    this->completedLevels = IS->readInt();
    this->killedMonstersLevels = IS->readInt();
    this->foundSecretsLevels = IS->readInt();
    this->disabledWeapons = IS->readInt();
    this->pickingStats = IS->readInt();
    this->prevWeapon = IS->readByte();
    this->cocktailDiscoverMask = IS->readInt();
    this->gamePlayedMask = IS->readInt();
    this->lastCombatTurn = IS->readInt();
    this->inCombat = IS->readBoolean();
    this->highestMap = IS->readShort();

    for (int i = 0; i < 9; ++i) {
        this->ammo[i] = IS->readShort();
    }
    for (int j = 0; j < 9; ++j) {
        this->ammoCopy[j] = IS->readShort();
    }
    for (int k = 0; k < Enums::INV_MAX; ++k) {
        this->inventory[k] = IS->readShort();
    }
    for (int l = 0; l < Enums::INV_MAX; ++l) {
        this->inventoryCopy[l] = IS->readShort();
    }
    this->numStatusEffects = IS->readByte();
    if (this->numStatusEffects == 0) {
        for (int n3 = 0; n3 < Enums::STATUS_EFFECT_ALL; ++n3) {
            this->statusEffects[Enums::OFS_STATUSEFFECT_COUNT + n3] = 0;
            this->statusEffects[Enums::OFS_STATUSEFFECT_TURNS + n3] = 0;
            this->statusEffects[Enums::OFS_STATUSEFFECT_AMOUNT + n3] = 0;
        }
    }
    else {
        for (int n4 = 0; n4 < Enums::STATUS_EFFECT_ALL; ++n4) {
            this->statusEffects[Enums::OFS_STATUSEFFECT_COUNT + n4] = IS->readShort();
            this->statusEffects[Enums::OFS_STATUSEFFECT_TURNS + n4] = IS->readShort();
            this->statusEffects[Enums::OFS_STATUSEFFECT_AMOUNT + n4] = IS->readShort();
        }
    }

    for (int n7 = 0; n7 < 8; ++n7) {
        this->counters[n7] = IS->readInt();
    }
    this->gameCompleted = IS->readBoolean();
    for (int i8 = 0; i8 < 2; i8++) {
        this->medals[i8] = IS->readInt();
    }
    for (int i9 = 0; i9 < 4; i9++) {
        this->foundBooks[i9] = IS->readByte();
    }
    this->translateStatusEffects();
    this->updateStats();
    return true;
}

bool Player::saveState(OutputStream* OS) {
    Applet* app = CAppContainer::getInstance()->app;
    this->baseCe->saveState(OS, true);
    this->ce->saveState(OS, true);
    OS->writeInt(this->purchasedWeapons);
    OS->writeInt((int)this->weapons);
    OS->writeInt(this->weaponsCopy);
    OS->writeByte(this->level);
    OS->writeInt(this->currentXP);
    int gameTime = app->gameTime;
    this->totalTime += gameTime - this->playTime;
    this->playTime = gameTime;
    OS->writeShort((int16_t)this->gold);
    OS->writeInt(this->totalTime);
    OS->writeInt(this->totalMoves);
    OS->writeInt(this->completedLevels);
    OS->writeInt(this->killedMonstersLevels);
    OS->writeInt(this->foundSecretsLevels);
    OS->writeInt(this->disabledWeapons);
    OS->writeInt(this->pickingStats);
    OS->writeByte(this->prevWeapon);
    OS->writeInt(this->cocktailDiscoverMask);
    OS->writeInt(this->gamePlayedMask);
    OS->writeInt(this->lastCombatTurn);
    OS->writeBoolean(this->inCombat);
    OS->writeShort(this->highestMap);

    for (int i = 0; i < 9; ++i) {
        OS->writeShort(this->ammo[i]);
    }
    for (int j = 0; j < 9; ++j) {
        OS->writeShort(this->ammoCopy[j]);
    }
    for (int k = 0; k < Enums::INV_MAX; ++k) {
        OS->writeShort(this->inventory[k]);
    }
    for (int l = 0; l < Enums::INV_MAX; ++l) {
        OS->writeShort(this->inventoryCopy[l]);
    }
    OS->writeByte(this->numStatusEffects);
    if (this->numStatusEffects != 0) {
        for (int n3 = 0; n3 < Enums::STATUS_EFFECT_ALL; ++n3) {
            OS->writeShort(this->statusEffects[Enums::OFS_STATUSEFFECT_COUNT + n3]);
            OS->writeShort(this->statusEffects[Enums::OFS_STATUSEFFECT_TURNS + n3]);
            OS->writeShort(this->statusEffects[Enums::OFS_STATUSEFFECT_AMOUNT + n3]);
        }
    }

    for (int n5 = 0; n5 < 8; ++n5) {
        OS->writeInt(this->counters[n5]);
    }

    OS->writeBoolean(this->gameCompleted);

    for (int n6 = 0; n6 < 2; n6++) {
        OS->writeInt(this->medals[n6]);
    }

    for (int n7 = 0; n7 < 4; n7++) {
        OS->writeByte(this->foundBooks[n7]);
    }
    return true;
}

void Player::unpause(int n) {
    if (n <= 0) {
        return;
    }
}

void Player::relink() {
    this->unlink();
    this->link();
}

void Player::unlink() {
    Applet* app = CAppContainer::getInstance()->app;

    Entity* playerEnt = getPlayerEnt();
    if (playerEnt->info & Entity::ENTITY_FLAG_LINKED) {
        app->game->unlinkEntity(playerEnt);
    }
}

void Player::link() {
    Applet* app = CAppContainer::getInstance()->app;

    Entity* playerEnt = getPlayerEnt();
    if (app->canvas->destX >= 0 && app->canvas->destX <= 2047 && app->canvas->destY >= 0 && app->canvas->destY <= 2047) {
        app->game->linkEntity(playerEnt, app->canvas->destX >> 6, app->canvas->destY >> 6);
    }
}

void Player::updateStats() {
    this->ce->setStat(Enums::STAT_MAX_HEALTH, this->baseCe->getStat(Enums::STAT_MAX_HEALTH) + this->buffs[(Enums::OFS_BUFF_AMOUNT + Enums::BUFF_FORTITUDE)]);
    this->ce->setStat(Enums::STAT_HEALTH, this->ce->getStat(Enums::STAT_HEALTH));
    this->ce->setStat(Enums::STAT_STRENGTH, this->baseCe->getStat(Enums::STAT_STRENGTH) + this->buffs[(Enums::OFS_BUFF_AMOUNT + Enums::BUFF_STRENGTH)]);
    this->ce->setStat(Enums::STAT_ACCURACY, this->baseCe->getStat(Enums::STAT_ACCURACY) + this->buffs[(Enums::OFS_BUFF_AMOUNT + Enums::BUFF_FOCUS)] - this->buffs[(Enums::OFS_BUFF_AMOUNT + Enums::BUFF_FIRE)]);
    this->ce->setStat(Enums::STAT_DEFENSE, this->baseCe->getStat(Enums::STAT_DEFENSE) + this->buffs[(Enums::OFS_BUFF_AMOUNT + Enums::BUFF_DEFENSE)]);
    this->ce->setStat(Enums::STAT_AGILITY, this->baseCe->getStat(Enums::STAT_AGILITY) + this->buffs[(Enums::OFS_BUFF_AMOUNT + Enums::BUFF_AGILITY)]);
}

void Player::updateStatusEffects() {
    if (this->numStatusEffects == 0) {
        return;
    }
    for (int i = 0; i < Enums::STATUS_EFFECT_ALL; ++i) {
        if (this->statusEffects[Enums::OFS_STATUSEFFECT_TURNS + i] != 0) {
            if (this->statusEffects[Enums::OFS_STATUSEFFECT_TURNS + i] <= 5 && i != Enums::STATUS_EFFECT_AIR && this->statusEffects[Enums::OFS_STATUSEFFECT_TURNS + i] == 1) {
                this->removeStatusEffect(i);
            }
            else if (this->statusEffects[Enums::OFS_STATUSEFFECT_TURNS + i] != 0) {
                --this->statusEffects[Enums::OFS_STATUSEFFECT_TURNS + i];
            }
        }
    }
    this->translateStatusEffects();
}

void Player::translateStatusEffects() {
    for (int i = 0; i < Enums::BUFF_MAX; i++) {
        this->buffs[Enums::OFS_BUFF_COUNT + i] = (this->buffs[Enums::OFS_BUFF_AMOUNT + i] = 0);
    }

    this->numbuffs = 0;
    for (int j = 0; j < Enums::STATUS_EFFECT_ALL; j++) {
        int n = this->statusEffects[Enums::OFS_STATUSEFFECT_TURNS + j];
        int n2 = this->statusEffects[Enums::OFS_STATUSEFFECT_AMOUNT + j];
        if (n != 0) {
            if (n > 0) {
                switch (j) {
                case Enums::STATUS_EFFECT_DISEASE: {
                    this->buffs[(Enums::OFS_BUFF_AMOUNT + Enums::BUFF_STRENGTH)] -= (short)n2;
                    this->buffs[(Enums::OFS_BUFF_COUNT + Enums::BUFF_STRENGTH)] = (short)n;
                    this->buffs[(Enums::OFS_BUFF_AMOUNT + Enums::BUFF_DEFENSE)] -= (short)n2;
                    this->buffs[(Enums::OFS_BUFF_COUNT + Enums::BUFF_DEFENSE)] = (short)n;
                    this->buffs[(Enums::OFS_BUFF_AMOUNT + Enums::BUFF_DISEASE)] -= (short)(this->statusEffects[Enums::OFS_STATUSEFFECT_COUNT + j] * 4);
                    this->buffs[(Enums::OFS_BUFF_COUNT + Enums::BUFF_DISEASE)] = (short)n;
                    break;
                }
                case Enums::STATUS_EFFECT_DRUNK:
                case Enums::STATUS_EFFECT_COLD: {
                    this->buffs[(Enums::OFS_BUFF_AMOUNT + Enums::BUFF_STRENGTH)] += (short)n2;
                    this->buffs[(Enums::OFS_BUFF_COUNT + Enums::BUFF_STRENGTH)] = (short)n;
                    this->buffs[(Enums::OFS_BUFF_AMOUNT + Enums::BUFF_DEFENSE)] += (short)n2;
                    this->buffs[(Enums::OFS_BUFF_COUNT + Enums::BUFF_DEFENSE)] = (short)n;
                    this->buffs[(Enums::OFS_BUFF_AMOUNT + Enums::BUFF_FOCUS)] -= (short)(n2 + (n2 / 2));
                    this->buffs[(Enums::OFS_BUFF_COUNT + Enums::BUFF_FOCUS)] = (short)n;
                    break;
                }
                case Enums::STATUS_EFFECT_DIZZY: {
                    this->buffs[(Enums::OFS_BUFF_AMOUNT + Enums::BUFF_FOCUS)] -= (short)n2;
                    this->buffs[(Enums::OFS_BUFF_COUNT + Enums::BUFF_FOCUS)] = (short)n;
                    break;
                }
                default: {
                    this->buffs[(Enums::OFS_BUFF_AMOUNT + Enums::BUFF_REFLECT) + j] += (short)n2;
                    this->buffs[(Enums::OFS_BUFF_COUNT + Enums::BUFF_REFLECT) + j] = (short)n;
                    break;
                }
                }
            }
        }
    }

    for (int k = 0; k < Enums::BUFF_MAX; k++) {
        if (this->buffs[Enums::OFS_BUFF_COUNT + k] > 0) {
            if ((1 << k & Enums::BUFF_NO_AMOUNT) == 0x0 && this->buffs[Enums::OFS_BUFF_AMOUNT + k] == 0) {
                this->buffs[Enums::OFS_BUFF_COUNT + k] = 0;
            }
            else {
                this->numbuffs++;
            }
        }
    }
    this->updateStats();
}

void Player::removeStatusEffect(int effect) {
    Applet* app = CAppContainer::getInstance()->app;

    if (effect == Enums::STATUS_EFFECT_ALL) {
        this->numStatusEffects = 0;
        for (int i = 0; i < Enums::STATUS_EFFECT_ALL; i++) {
            this->statusEffects[Enums::OFS_STATUSEFFECT_TURNS + i] = 0;
            this->statusEffects[Enums::OFS_STATUSEFFECT_AMOUNT + i] = 0;
            this->statusEffects[Enums::OFS_STATUSEFFECT_COUNT + i] = 0;
        }
    }
    else {
        if (this->statusEffects[Enums::OFS_STATUSEFFECT_COUNT + effect] == 0) {
            return;
        }

        if (effect == Enums::STATUS_EFFECT_COLD) {
            app->render->startFogLerp(1, 0, 2000);
        }

        this->statusEffects[Enums::OFS_STATUSEFFECT_AMOUNT + effect] = 0;
        this->statusEffects[Enums::OFS_STATUSEFFECT_COUNT + effect] = 0;
        this->statusEffects[Enums::OFS_STATUSEFFECT_TURNS + effect] = 0;
        this->numStatusEffects--;
    }
    this->translateStatusEffects();
}

bool Player::addStatusEffect(int effect, int amount, int turns) {
    Applet* app = CAppContainer::getInstance()->app;

    if (effect == Enums::STATUS_EFFECT_FIRE && this->buffs[(Enums::OFS_BUFF_COUNT + Enums::BUFF_ANTIFIRE)] > 0) {
        return false;
    }

    int count = this->statusEffects[Enums::OFS_STATUSEFFECT_COUNT + effect] + 1;

    if (count > 3 || (effect == Enums::STATUS_EFFECT_ANTIFIRE && count > 1)) {
        if (effect == Enums::STATUS_EFFECT_DISEASE) {
            this->statusEffects[Enums::OFS_STATUSEFFECT_TURNS + effect] = turns;
        }
        return false;
    }

    if (count == 1) {
        this->numStatusEffects++;
        if (effect == Enums::STATUS_EFFECT_COLD && !app->game->isUnderWater()) {
            app->tinyGL->fogMin = 0;
            if (app->tinyGL->fogRange > 0) {
                app->tinyGL->fogRange = -1;
            }
            app->render->startFogLerp(1024, 0, 2000);
        }
    }

    this->statusEffects[Enums::OFS_STATUSEFFECT_AMOUNT + effect] += amount;
    this->statusEffects[Enums::OFS_STATUSEFFECT_TURNS + effect] = turns;
    this->statusEffects[Enums::OFS_STATUSEFFECT_COUNT + effect] = count;
    return true;
}

void Player::drawStatusEffectIcon(Graphics* graphics, int n, int n2, int n3, int n4, int n5) {
    Applet* app = CAppContainer::getInstance()->app;

    Text* smallBuffer = app->localization->getSmallBuffer();
    smallBuffer->setLength(0);
    if (n == 8) {
        smallBuffer->append('%');
        smallBuffer->append(n2);
    }
    else if ((1 << n & 0x3A07) == 0x0) {
        if (n2 >= 0) {
            smallBuffer->append('+');
            smallBuffer->append(n2);
        }
        else {
            smallBuffer->append(n2);
        }
    }
    graphics->drawString(smallBuffer, n4 + 1, n5 + 10, 24);
    graphics->drawBuffIcon(n, n4 + 4, n5 + 1, 0);
    if (app->time - this->turnTime < 600) {
        smallBuffer->setLength(0);
        smallBuffer->append(n3);
        graphics->drawString(smallBuffer, n4 + 17, n5 + 2, 17);
        app->canvas->forcePump = true;
    }
    smallBuffer->dispose();
}

void Player::resetCounters() {
    for (int i = 0; i < 8; ++i) {
        this->counters[i] = 0;
    }
}

int* Player::GetPos() {
    Canvas* canvas = CAppContainer::getInstance()->app->canvas;
    this->pos[0] = canvas->destX;
    this->pos[1] = canvas->destY;
    this->pos[2] = canvas->destAngle;
    return this->pos;
}

Entity* Player::getPlayerEnt() {
    return &CAppContainer::getInstance()->app->game->entities[1];
}

void Player::setPickUpWeapon(int n) {
    Applet* app = CAppContainer::getInstance()->app;

    EntityDef* lookup = nullptr;
    EntityDef* find = app->entityDefManager->find(Enums::ET_ITEM, Enums::IT_WEAPON, 16);
    if (n != Enums::TILENUM_TESLA) {
        lookup = app->entityDefManager->lookup(n);
    }
    if (lookup != nullptr) {
        find->tileIndex = lookup->tileIndex;
        find->name = lookup->name;
        find->longName = lookup->longName;
        find->description = lookup->description;
    }
    else {
        find->tileIndex = Enums::TILENUM_WORLD_WEAPON;
        find->name = EntityStrings::EMPTY_STRING;
        find->longName = EntityStrings::EMPTY_STRING;
        find->description = EntityStrings::EMPTY_STRING;
    }
}

void Player::giveAll() {
    Applet* app = CAppContainer::getInstance()->app;

    this->weapons = 65535LL;

    for (int j = 0; j < 10; ++j) {
        if (j != 9) {
            this->give(Enums::IT_AMMO, j, 100, true);
        }
    }
    for (int k = 0; k < 29; ++k) {
        this->give(Enums::IT_INVENTORY, (uint8_t)k, 999, true);
        app->canvas->numHelpMessages = 0;
    }
    for (int i3 = 0; i3 < 28; i3++) {
        int i4 = i3 / 8;
        this->foundBooks[i4] |= (1 << (i3 & 7));
    }

    this->ce->setStat(Enums::STAT_HEALTH, this->ce->getStat(Enums::STAT_MAX_HEALTH));
}

void Player::equipForLevel(int mapID) {
    Applet* app = CAppContainer::getInstance()->app;

    int viewX = app->canvas->viewX;
    int viewY = app->canvas->viewY;
    int viewAngle = app->canvas->viewAngle;
    this->reset();
    app->canvas->viewX = app->canvas->destX = app->canvas->saveX = app->canvas->prevX = viewX;
    app->canvas->viewY = app->canvas->destY = app->canvas->saveY = app->canvas->prevY = viewY;
    app->canvas->viewZ = app->canvas->destZ = app->render->getHeight(app->canvas->viewX, app->canvas->viewY) + 36;
    app->canvas->viewAngle = app->canvas->destAngle = app->canvas->saveAngle = viewAngle;
    app->canvas->viewPitch = app->canvas->destPitch = 0;
    this->highestMap = mapID;
    bool enableHelp = this->enableHelp;
    this->enableHelp = false;
    this->weapons = 0LL;
    int i5 = 1;
    switch (mapID) {
    case 1:
        this->give(Enums::IT_WEAPON, Enums::WP_PUNCH, 1);
        this->give(Enums::IT_WEAPON, Enums::WP_BOOT, 1);
        break;
    case 2:
        this->give(Enums::IT_WEAPON, Enums::WP_DUAL_PISTOL, 1);
        this->give(Enums::IT_AMMO, Enums::AMMO_9MM, 80);
        this->give(Enums::IT_INVENTORY, Enums::INV_HEALTH_PACK, 10);
        this->give(Enums::IT_INVENTORY, Enums::INV_TREASURE_GOBLET, 6);
        this->give(Enums::IT_INVENTORY, Enums::INV_TREASURE_CROSS, 1);
        this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_EVADE, 1);
        this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_DEFENSE, 1);
        this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_ADRENALINE, 1);
        this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_FOCUS, 1);
        this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_FORTITUDE, 2);
        this->addArmor(70);
        i5++;
        this->give(Enums::IT_WEAPON, Enums::WP_PUNCH, 1);
        this->give(Enums::IT_WEAPON, Enums::WP_BOOT, 1);
        break;
    case 3:
        this->give(Enums::IT_WEAPON, Enums::WP_THOMPSON, 1);
        this->give(Enums::IT_AMMO, Enums::AMMO_45CAL, 13);
        this->give(Enums::IT_WEAPON, Enums::WP_DYNAMITE, 1);
        this->give(Enums::IT_AMMO, Enums::AMMO_DYNAMITE, 10);
        this->give(Enums::IT_INVENTORY, Enums::INV_HEALTH_PACK, 12);
        this->give(Enums::IT_INVENTORY, Enums::INV_EMPTY_SYRINGE, 1);
        this->give(Enums::IT_INVENTORY, Enums::INV_TREASURE_MIN, 3);
        this->give(Enums::IT_INVENTORY, Enums::INV_TREASURE_CROSS, 3);
        this->give(Enums::IT_INVENTORY, Enums::INV_TREASURE_CROWN, 1);
        this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_EVADE, 3);
        this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_DEFENSE, 3);
        this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_ADRENALINE, 5);
        this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_FOCUS, 5);
        this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_ANTIFIRE, 9);
        this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_ANGER, 1);
        this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_REGEN, 2);
        i5++;
        this->give(Enums::IT_WEAPON, Enums::WP_DUAL_PISTOL, 1);
        this->give(Enums::IT_AMMO, Enums::AMMO_9MM, 80);
        this->give(Enums::IT_INVENTORY, Enums::INV_HEALTH_PACK, 10);
        this->give(Enums::IT_INVENTORY, Enums::INV_TREASURE_GOBLET, 6);
        this->give(Enums::IT_INVENTORY, Enums::INV_TREASURE_CROSS, 1);
        this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_EVADE, 1);
        this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_DEFENSE, 1);
        this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_ADRENALINE, 1);
        this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_FOCUS, 1);
        this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_FORTITUDE, 2);

        this->addArmor(70);
        i5++;
        this->give(Enums::IT_WEAPON, Enums::WP_PUNCH, 1);
        this->give(Enums::IT_WEAPON, Enums::WP_BOOT, 1);
        break;
    case 4:
    case 5:
    case 6:
        if (mapID == 6) {
            i5++;
        }
        if (mapID == 4 && app->game->difficulty == 1) {
            i5 = 0;
            this->nextLevelXP = 0;
            this->currentXP = 0;
            this->addXP(this->calcLevelXP(3));
            this->ce->setStat(Enums::STAT_HEALTH, 115);
            this->ce->setStat(Enums::STAT_ARMOR, 100);
            this->give(Enums::IT_INVENTORY, Enums::INV_HEALTH_PACK, 45);
            this->give(Enums::IT_INVENTORY, Enums::INV_HEALTH_RATION_BAR, 11);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_EVADE, 4);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_DEFENSE, 4);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_ADRENALINE, 4);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_FOCUS, 4);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_FEAR, 2);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_REGEN, 2);
            this->give(Enums::IT_INVENTORY, Enums::INV_TREASURE_CROSS, 3);
            this->give(Enums::IT_INVENTORY, Enums::INV_TREASURE_CROWN, 5);
            this->give(Enums::IT_INVENTORY, Enums::INV_TREASURE_GOBLET, 13);
            this->gold = 1;
            this->give(Enums::IT_WEAPON, Enums::WP_PUNCH, 1);
            this->give(Enums::IT_WEAPON, Enums::WP_BOOT, 1);
            this->give(Enums::IT_WEAPON, Enums::WP_BRASS_PUNCH, 1);
            this->give(Enums::IT_WEAPON, Enums::WP_DUAL_PISTOL, 1);
            this->give(Enums::IT_WEAPON, Enums::WP_THOMPSON, 1);
            this->give(Enums::IT_AMMO, Enums::AMMO_9MM, 90);
            this->give(Enums::IT_AMMO, Enums::AMMO_45CAL, 90);
            this->give(Enums::IT_AMMO, Enums::AMMO_DYNAMITE, 13);
        }
        else if (mapID == 5 && app->game->difficulty == 1) {
            i5 = 0;
            this->nextLevelXP = 0;
            this->currentXP = 0;
            this->addXP(4320);
            this->ce->setStat(Enums::STAT_HEALTH, 109);
            this->ce->setStat(Enums::STAT_ARMOR, 51);
            this->give(Enums::IT_INVENTORY, Enums::INV_HEALTH_PACK, 46);
            this->give(Enums::IT_INVENTORY, Enums::INV_HEALTH_RATION_BAR, 27);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_EVADE, 7);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_DEFENSE, 8);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_ADRENALINE, 10);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_FOCUS, 9);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_ANTIFIRE, 7);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_FORTITUDE, 1);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_FEAR, 1);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_REGEN, 1);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_PURIFY, 1);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_ANGER, 6);
            this->give(Enums::IT_INVENTORY, Enums::INV_TREASURE_CROSS, 12);
            this->give(Enums::IT_INVENTORY, Enums::INV_TREASURE_CROWN, 11);
            this->give(Enums::IT_INVENTORY, Enums::INV_TREASURE_GOBLET, 16);
            this->gold = 7;
            this->give(Enums::IT_INVENTORY, Enums::INV_EMPTY_SYRINGE, 5);
            this->give(Enums::IT_INVENTORY, Enums::INV_OTHER_SCOTCH, 2);
            this->give(Enums::IT_WEAPON, Enums::WP_PUNCH, 1);
            this->give(Enums::IT_WEAPON, Enums::WP_BOOT, 1);
            this->give(Enums::IT_WEAPON, Enums::WP_BRASS_PUNCH, 1);
            this->give(Enums::IT_WEAPON, Enums::WP_DUAL_PISTOL, 1);
            this->give(Enums::IT_WEAPON, Enums::WP_THOMPSON, 1);
            this->give(Enums::IT_WEAPON, Enums::WP_MAUSER, 1);
            this->give(Enums::IT_AMMO, Enums::AMMO_9MM, 89);
            this->give(Enums::IT_AMMO, Enums::AMMO_45CAL, 79);
            this->give(Enums::IT_AMMO, Enums::AMMO_30CAL, 12);
            this->give(Enums::IT_AMMO, Enums::AMMO_DYNAMITE, 14);
        }
        else if (mapID == 6 && app->game->difficulty == 4) {
            i5 = 0;
            this->addXP(6135);
            this->ce->setStat(Enums::STAT_HEALTH, 50);
            this->ce->setStat(Enums::STAT_ARMOR, 76);
            this->give(Enums::IT_INVENTORY, Enums::INV_HEALTH_PACK, 31);
            this->give(Enums::IT_INVENTORY, Enums::INV_HEALTH_RATION_BAR, 7);
            this->give(Enums::IT_INVENTORY, Enums::INV_OTHER_SCOTCH, 3);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_EVADE, 7);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_DEFENSE, 8);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_ADRENALINE, 10);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_FOCUS, 8);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_ANTIFIRE, 1);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_REGEN, 4);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_FEAR, 1);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_ANGER, 5);
            this->give(Enums::IT_INVENTORY, Enums::INV_TREASURE_CROSS, 19);
            this->give(Enums::IT_INVENTORY, Enums::INV_TREASURE_CROWN, 17);
            this->give(Enums::IT_INVENTORY, Enums::INV_TREASURE_GOBLET, 27);
            this->gold = 12;
            this->give(Enums::IT_WEAPON, Enums::WP_PUNCH, 1);
            this->give(Enums::IT_WEAPON, Enums::WP_BOOT, 1);
            this->give(Enums::IT_WEAPON, Enums::WP_BRASS_PUNCH, 1);
            this->give(Enums::IT_WEAPON, Enums::WP_DUAL_PISTOL, 1);
            this->give(Enums::IT_AMMO, Enums::AMMO_9MM, 38);
            break;
        }
        else {
            this->give(Enums::IT_WEAPON, Enums::WP_BRASS_PUNCH, 1);
            this->give(Enums::IT_AMMO, Enums::AMMO_45CAL, 74);
            this->give(Enums::IT_AMMO, Enums::AMMO_DYNAMITE, 5);
            this->give(Enums::IT_INVENTORY, Enums::INV_EMPTY_SYRINGE, 1);
            this->give(Enums::IT_INVENTORY, Enums::INV_TREASURE_GOBLET, 6);
            this->give(Enums::IT_INVENTORY, Enums::INV_TREASURE_CROSS, 3);
            this->give(Enums::IT_INVENTORY, Enums::INV_TREASURE_CROWN, 5);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_EVADE, 1);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_ADRENALINE, 4);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_ANGER, 2);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_FEAR, 1);
            i5++;
            this->give(Enums::IT_WEAPON, Enums::WP_THOMPSON, 1);
            this->give(Enums::IT_AMMO, Enums::AMMO_45CAL, 13);
            this->give(Enums::IT_WEAPON, Enums::WP_DYNAMITE, 1);
            this->give(Enums::IT_AMMO, Enums::AMMO_DYNAMITE, 10);
            this->give(Enums::IT_INVENTORY, Enums::INV_HEALTH_PACK, 12);
            this->give(Enums::IT_INVENTORY, Enums::INV_EMPTY_SYRINGE, 1);
            this->give(Enums::IT_INVENTORY, Enums::INV_TREASURE_GOBLET, 3);
            this->give(Enums::IT_INVENTORY, Enums::INV_TREASURE_CROSS, 3);
            this->give(Enums::IT_INVENTORY, Enums::INV_TREASURE_CROWN, 1);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_EVADE, 3);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_DEFENSE, 3);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_ADRENALINE, 5);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_FOCUS, 5);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_ANTIFIRE, 9);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_ANGER, 1);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_REGEN, 2);
            i5++;
            this->give(Enums::IT_WEAPON, Enums::WP_DUAL_PISTOL, 1);
            this->give(Enums::IT_AMMO, Enums::AMMO_9MM, 80);
            this->give(Enums::IT_INVENTORY, Enums::INV_HEALTH_PACK, 10);
            this->give(Enums::IT_INVENTORY, Enums::INV_TREASURE_GOBLET, 6);
            this->give(Enums::IT_INVENTORY, Enums::INV_TREASURE_CROSS, 1);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_EVADE, 1);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_DEFENSE, 1);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_ADRENALINE, 1);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_FOCUS, 1);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_FORTITUDE, 2);
            this->addArmor(70);
            i5++;
            this->give(Enums::IT_WEAPON, Enums::WP_PUNCH, 1);
            this->give(Enums::IT_WEAPON, Enums::WP_BOOT, 1);
        }
        break;

    case 9:
    default:
        if (mapID != 9 || app->game->difficulty != 4) {
            i5 = 1 + 1;
            this->give(Enums::IT_WEAPON, Enums::WP_VENOM, 1);
            this->give(Enums::IT_AMMO, Enums::AMMO_12_7MM, 10);
            if (mapID == 8 && app->game->difficulty == 4) {
                i5 = 0;
                this->nextLevelXP = 0;
                this->currentXP = 0;
                this->addXP(9462);
                this->ce->setStat(Enums::STAT_ARMOR, 50);
                this->ce->setStat(Enums::STAT_HEALTH, 150);
                this->give(Enums::IT_INVENTORY, Enums::INV_HEALTH_RATION_BAR, 12);
                this->give(Enums::IT_INVENTORY, Enums::INV_HEALTH_PACK, 13);
                this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_EVADE, 5);
                this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_DEFENSE, 6);
                this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_ADRENALINE, 5);
                this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_FOCUS, 7);
                this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_ANTIFIRE, 5);
                this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_ENRAGE, 1);
                this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_REGEN, 3);
                this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_PURIFY, 1);
                this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_ELITE, 2);
                this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_FEAR, 1);
                this->give(Enums::IT_INVENTORY, Enums::INV_EMPTY_SYRINGE, 1);
                this->give(Enums::IT_INVENTORY, Enums::INV_OTHER_SCOTCH, 10);
                this->give(Enums::IT_WEAPON, Enums::WP_PUNCH, 1);
                this->give(Enums::IT_WEAPON, Enums::WP_BOOT, 1);
                this->give(Enums::IT_WEAPON, Enums::WP_SPIKE_PUNCH, 1);
                this->give(Enums::IT_WEAPON, Enums::WP_DUAL_PISTOL, 1);
                this->give(Enums::IT_WEAPON, Enums::WP_THOMPSON, 1);
                this->give(Enums::IT_WEAPON, Enums::WP_STEN, 1);
                this->give(Enums::IT_AMMO, Enums::AMMO_9MM, 80);
                this->give(Enums::IT_AMMO, Enums::AMMO_45CAL, 88);
                this->give(Enums::IT_AMMO, Enums::AMMO_DYNAMITE, 1);
                this->give(Enums::IT_INVENTORY, Enums::INV_TREASURE_CROWN, 21);
                this->give(Enums::IT_INVENTORY, Enums::INV_TREASURE_CROSS, 19);
                this->give(Enums::IT_INVENTORY, Enums::INV_TREASURE_GOBLET, 29);
                this->gold = 12;
                break;
            }
            else {
                int i6 = i5 + 1;
                this->give(Enums::IT_WEAPON, Enums::WP_PANZER, 1);
                this->give(Enums::IT_AMMO, Enums::AMMO_ROCKETS, 10);
                if (mapID == 7 && app->game->difficulty == 4) {
                    i5 = 0;
                    this->nextLevelXP = 0;
                    this->currentXP = 0;
                    this->addXP(7617);
                    this->ce->setStat(Enums::STAT_ARMOR, 0);
                    this->ce->setStat(Enums::STAT_HEALTH, 140);
                    this->give(Enums::IT_INVENTORY, Enums::INV_HEALTH_PACK, 18);
                    this->give(Enums::IT_INVENTORY, Enums::INV_HEALTH_RATION_BAR, 6);
                    this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_EVADE, 5);
                    this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_DEFENSE, 7);
                    this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_ADRENALINE, 7);
                    this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_FOCUS, 8);
                    this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_ENRAGE, 2);
                    this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_AGILITY, 1);
                    this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_REGEN, 5);
                    this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_PURIFY, 1);
                    this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_ELITE, 1);
                    this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_FEAR, 1);
                    this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_ANGER, 2);
                    this->give(Enums::IT_INVENTORY, Enums::INV_OTHER_SCOTCH, 6);
                    this->give(Enums::IT_WEAPON, Enums::WP_PUNCH, 1);
                    this->give(Enums::IT_WEAPON, Enums::WP_BOOT, 1);
                    this->give(Enums::IT_WEAPON, Enums::WP_BRASS_PUNCH, 1);
                    this->give(Enums::IT_WEAPON, Enums::WP_DUAL_PISTOL, 1);
                    this->give(Enums::IT_WEAPON, Enums::WP_THOMPSON, 1);
                    this->give(Enums::IT_AMMO, Enums::AMMO_9MM, 10);
                    this->give(Enums::IT_AMMO, Enums::AMMO_45CAL, 39);
                    this->give(Enums::IT_AMMO, Enums::AMMO_DYNAMITE, 3);
                    this->give(Enums::IT_INVENTORY, Enums::INV_TREASURE_CROSS, 19);
                    this->give(Enums::IT_INVENTORY, Enums::INV_TREASURE_CROWN, 19);
                    this->give(Enums::IT_INVENTORY, Enums::INV_TREASURE_GOBLET, 28);
                    this->gold = 12;
                    break;
                }
                else {
                    i5 = 0;
                    this->nextLevelXP = 0;
                    this->currentXP = 0;
                    this->addXP(this->calcLevelXP(4));
                    this->ce->setStat(Enums::STAT_ARMOR, 50);
                    this->ce->setStat(Enums::STAT_HEALTH, 140);
                    this->give(Enums::IT_INVENTORY, Enums::INV_HEALTH_PACK, 80);
                    this->give(Enums::IT_INVENTORY, Enums::INV_HEALTH_RATION_BAR, 38);
                    this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_EVADE, 4);
                    this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_DEFENSE, 5);
                    this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_ADRENALINE, 9);
                    this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_FOCUS, 9);
                    this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_AGILITY, 1);
                    this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_REGEN, 4);
                    this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_PURIFY, 1);
                    this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_REFLECT, 1);
                    this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_ELITE, 1);
                    this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_FEAR, 2);
                    this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_ANGER, 3);
                    this->give(Enums::IT_WEAPON, Enums::WP_PUNCH, 1);
                    this->give(Enums::IT_WEAPON, Enums::WP_BOOT, 1);
                    this->give(Enums::IT_WEAPON, Enums::WP_BRASS_PUNCH, 1);
                    this->give(Enums::IT_WEAPON, Enums::WP_DUAL_PISTOL, 1);
                    this->give(Enums::IT_WEAPON, Enums::WP_THOMPSON, 1);
                    this->give(Enums::IT_WEAPON, Enums::WP_FG42, 1);
                    this->give(Enums::IT_WEAPON, Enums::WP_FLAMETHROWER, 1);
                    this->give(Enums::IT_AMMO, Enums::AMMO_9MM, 10);
                    this->give(Enums::IT_AMMO, Enums::AMMO_45CAL, 62);
                    this->give(Enums::IT_AMMO, Enums::AMMO_DYNAMITE, 8);
                    this->give(Enums::IT_AMMO, Enums::AMMO_30CAL, 25);
                    this->give(Enums::IT_AMMO, Enums::AMMO_FUEL, 32);
                    this->give(Enums::IT_INVENTORY, Enums::INV_TREASURE_CROSS, 19);
                    this->give(Enums::IT_INVENTORY, Enums::INV_TREASURE_CROWN, 19);
                    this->give(Enums::IT_INVENTORY, Enums::INV_TREASURE_GOBLET, 28);
                    this->gold = 12;
                    break;
                }
            }
        }
        else {
            i5 = 0;
            this->nextLevelXP = 0;
            this->currentXP = 0;
            this->addXP(11297);
            this->ce->setStat(Enums::STAT_ARMOR, 50);
            this->ce->setStat(Enums::STAT_HEALTH, 150);
            this->give(Enums::IT_INVENTORY, Enums::INV_HEALTH_PACK, 7);
            this->give(Enums::IT_INVENTORY, Enums::INV_HEALTH_RATION_BAR, 3);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_EVADE, 4);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_DEFENSE, 7);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_ADRENALINE, 5);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_FOCUS, 5);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_ANTIFIRE, 5);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_ENRAGE, 1);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_AGILITY, 1);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_PURIFY, 3);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_ELITE, 1);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_FEAR, 1);
            this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_ANGER, 1);
            this->give(Enums::IT_INVENTORY, Enums::INV_OTHER_SCOTCH, 14);
            this->give(Enums::IT_WEAPON, Enums::WP_PUNCH, 1);
            this->give(Enums::IT_WEAPON, Enums::WP_BOOT, 1);
            this->give(Enums::IT_WEAPON, Enums::WP_SPIKE_PUNCH, 1);
            this->give(Enums::IT_WEAPON, Enums::WP_DUAL_PISTOL, 1);
            this->give(Enums::IT_WEAPON, Enums::WP_THOMPSON, 1);
            this->give(Enums::IT_WEAPON, Enums::WP_STEN, 1);
            this->give(Enums::IT_AMMO, Enums::AMMO_9MM, 62);
            this->give(Enums::IT_AMMO, Enums::AMMO_45CAL, 61);
            this->give(Enums::IT_AMMO, Enums::AMMO_DYNAMITE, 1);
            this->give(Enums::IT_INVENTORY, Enums::INV_TREASURE_CROSS, 19);
            this->give(Enums::IT_INVENTORY, Enums::INV_TREASURE_CROWN, 22);
            this->give(Enums::IT_INVENTORY, Enums::INV_TREASURE_GOBLET, 29);
            this->gold = 12;
            break;
        }

    case 7:
        if (mapID != 7) {
            break;
        }
        i5 = 0;
        this->nextLevelXP = 0;
        this->currentXP = 0;
        this->addXP(this->calcLevelXP(4));
        this->ce->setStat(Enums::STAT_ARMOR, 50);
        this->ce->setStat(Enums::STAT_HEALTH, 140);
        this->give(Enums::IT_INVENTORY, Enums::INV_HEALTH_PACK, 80);
        this->give(Enums::IT_INVENTORY, Enums::INV_HEALTH_RATION_BAR, 38);
        this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_EVADE, 4);
        this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_DEFENSE, 5);
        this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_ADRENALINE, 9);
        this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_FOCUS, 9);
        this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_AGILITY, 1);
        this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_REGEN, 4);
        this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_PURIFY, 1);
        this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_REFLECT, 1);
        this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_ELITE, 1);
        this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_FEAR, 2);
        this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_ANGER, 3);
        this->give(Enums::IT_WEAPON, Enums::WP_PUNCH, 1);
        this->give(Enums::IT_WEAPON, Enums::WP_BOOT, 1);
        this->give(Enums::IT_WEAPON, Enums::WP_BRASS_PUNCH, 1);
        this->give(Enums::IT_WEAPON, Enums::WP_DUAL_PISTOL, 1);
        this->give(Enums::IT_WEAPON, Enums::WP_THOMPSON, 1);
        this->give(Enums::IT_WEAPON, Enums::WP_FG42, 1);
        this->give(Enums::IT_WEAPON, Enums::WP_FLAMETHROWER, 1);
        this->give(Enums::IT_AMMO, Enums::AMMO_9MM, 10);
        this->give(Enums::IT_AMMO, Enums::AMMO_45CAL, 62);
        this->give(Enums::IT_AMMO, Enums::AMMO_DYNAMITE, 8);
        this->give(Enums::IT_AMMO, Enums::AMMO_30CAL, 25);
        this->give(Enums::IT_AMMO, Enums::AMMO_FUEL, 32);
        this->give(Enums::IT_INVENTORY, Enums::INV_TREASURE_CROSS, 19);
        this->give(Enums::IT_INVENTORY, Enums::INV_TREASURE_CROWN, 19);
        this->give(Enums::IT_INVENTORY, Enums::INV_TREASURE_GOBLET, 28);
        this->gold = 12;
        break;
    case 8:
        if (mapID != 8) {
            break;
        }
        int i6 = i5 + 1;
        this->give(Enums::IT_WEAPON, Enums::WP_PANZER, 1);
        this->give(Enums::IT_AMMO, Enums::AMMO_ROCKETS, 10);
        if (mapID != 7) {
        }
        i5 = 0;
        this->nextLevelXP = 0;
        this->currentXP = 0;
        this->addXP(this->calcLevelXP(4));
        this->ce->setStat(Enums::STAT_ARMOR, 50);
        this->ce->setStat(Enums::STAT_HEALTH, 140);
        this->give(Enums::IT_INVENTORY, Enums::INV_HEALTH_PACK, 80);
        this->give(Enums::IT_INVENTORY, Enums::INV_HEALTH_RATION_BAR, 38);
        this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_EVADE, 4);
        this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_DEFENSE, 5);
        this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_ADRENALINE, 9);
        this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_FOCUS, 9);
        this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_AGILITY, 1);
        this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_REGEN, 4);
        this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_PURIFY, 1);
        this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_REFLECT, 1);
        this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_ELITE, 1);
        this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_FEAR, 2);
        this->give(Enums::IT_INVENTORY, Enums::INV_SYRINGE_ANGER, 3);
        this->give(Enums::IT_WEAPON, Enums::WP_PUNCH, 1);
        this->give(Enums::IT_WEAPON, Enums::WP_BOOT, 1);
        this->give(Enums::IT_WEAPON, Enums::WP_BRASS_PUNCH, 1);
        this->give(Enums::IT_WEAPON, Enums::WP_DUAL_PISTOL, 1);
        this->give(Enums::IT_WEAPON, Enums::WP_THOMPSON, 1);
        this->give(Enums::IT_WEAPON, Enums::WP_FG42, 1);
        this->give(Enums::IT_WEAPON, Enums::WP_FLAMETHROWER, 1);
        this->give(Enums::IT_AMMO, Enums::AMMO_9MM, 10);
        this->give(Enums::IT_AMMO, Enums::AMMO_45CAL, 62);
        this->give(Enums::IT_AMMO, Enums::AMMO_DYNAMITE, 8);
        this->give(Enums::IT_AMMO, Enums::AMMO_30CAL, 25);
        this->give(Enums::IT_AMMO, Enums::AMMO_FUEL, 32);
        this->give(Enums::IT_INVENTORY, Enums::INV_TREASURE_CROSS, 19);
        this->give(Enums::IT_INVENTORY, Enums::INV_TREASURE_CROWN, 19);
        this->give(Enums::IT_INVENTORY, Enums::INV_TREASURE_GOBLET, 28);
        this->gold = 12;
        break;
    }
    this->give(Enums::IT_INVENTORY, Enums::INV_OTHER_JOURNAL, 1);
    this->enableHelp = enableHelp;
    this->selectNextWeapon();
    while (this->level <= i5) {
        this->addLevel();
    }
    app->canvas->updateFacingEntity = true;
}

void Player::addArmor(int n) {
    Applet* app = CAppContainer::getInstance()->app;

    app->hud->repaintFlags |= 0x4;
    if (this->ce->addStat(Enums::STAT_ARMOR, n) >= 200) {
        this->ce->setStat(Enums::STAT_ARMOR, 200);
    }
}

int Player::distFrom(Entity* entity) {
    int* pos = this->GetPos();
    return entity->distFrom(pos[0], pos[1]);
}

void Player::giveStandardMedal(int n, int n2) {
    if (n <= 10) {
        this->giveMedal(((n - 1) * 5) + n2, nullptr);
    }
}

void Player::giveMedal(int n, ScriptThread* scriptThread) {
    Applet* app = CAppContainer::getInstance()->app;
    this->medals[n / 32] |= (1 << (n % 32));

    if (app->canvas->state != Canvas::ST_DRIVING)
    {
        Text* smallBuffer = app->localization->getSmallBuffer();
        Text* smallBuffer2 = app->localization->getSmallBuffer();
        if (n < 50) {
            app->localization->composeText(Strings::FILE_MEDALSTRINGS, (short)this->allMedals[(n * 3) + 1], smallBuffer2);
        }
        else {
            for (int i3 = 150; i3 < 159; i3 += 3) {
                if (this->allMedals[i3 + 2] == n) {
                    app->localization->composeText(Strings::FILE_MEDALSTRINGS, (short)this->allMedals[i3 + 1], smallBuffer2);
                }
            }
        }
        app->localization->resetTextArgs();
        app->localization->addTextArg(smallBuffer2);
        smallBuffer2->dispose();
        app->sound->playSound(1076, 0, 6, false);
        app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::MEDAL_AWARDED, smallBuffer);
        if (!app->canvas->enqueueHelpDialog(smallBuffer, Canvas::HELPMSG_TYPE_MEDAL)) {
            smallBuffer->dispose();
        }
    }
}

void Player::offerBook(int n, ScriptThread* scriptThread) {
    Applet* app = CAppContainer::getInstance()->app;
    Text* smallBuffer = app->localization->getSmallBuffer();
    app->localization->resetTextArgs();
    app->localization->addTextArg(Strings::FILE_BOOKSTRINGS, (int16_t)this->bookMap[n * 4]);
    app->localization->composeText(Strings::FILE_CODESTRINGS, CodeStrings::READ_OPTION, smallBuffer);
    app->canvas->startDialog(scriptThread, smallBuffer, Enums::DLG_STYLE_CHEST, Enums::DLG_STYLE_NPC, true);
    smallBuffer->dispose();
}

void Player::giveBook(int n, ScriptThread* scriptThread) {
    Applet* app = CAppContainer::getInstance()->app;
    this->foundBooks[n / 8] |= (uint8_t)((1 << n % 8));
    this->showBookStat = true;
    app->menuSystem->menuParam = (uint8_t)n;
    app->menuSystem->setMenu(Menus::MENU_INGAME_BOOKREAD);
}

uint8_t* Player::getLevelMedalCount() {
    int i = 0;
    for (int i2 = 1; i2 < 11; i2++) {
        int i3 = 0;
        int i4 = 0;
        for (int i5 = 0; i5 < 5; i5++) {
            uint8_t b = this->allMedals[((i2 - 1) * 3 * 5) + (i5 * 3) + 2];
            if ((this->medals[b / 32] & (1 << (b % 32))) != 0) {
                i4++;
            }
            i3++;
            i += 2;
        }

        bool z = false;
        for (int i6 = 150; i6 < 159; i6 += 3) {
            if (this->allMedals[i6] == i2) {
                uint8_t b2 = this->allMedals[i6 + 2];
                if ((this->medals[b2 / 32] & (1 << (b2 % 32))) != 0) {
                    i4++;
                }
                i3++;
                i += 2;
                z = true;
            }
            else if (z) {
                break;
            }
        }
        int i7 = (i2 - 1) << 1;
        this->medalInfo[i7 + 0] = (uint8_t)i4;
        this->medalInfo[i7 + 1] = (uint8_t)i3;
    }
    return this->medalInfo;
}

bool Player::hasAllKills(int n) {
    uint8_t b = this->allMedals[(n * 3 * 5) + 6 + 2];
    return (this->medals[b / 32] & (1 << (b % 32))) != 0;
}

uint8_t* Player::getLevelMedals(int n, bool b) {
    int n2;
    for (n2 = 0; n2 < 22; ++n2) {
        this->medalInfo[n2] = -1;
    }
    bool bl2 = false;
    int n3 = 0;
    uint8_t by = 0;
    for (n2 = 0; n2 < 5; ++n2) {
        int n4 = (n - 1) * 3 * 5 + n2 * 3;
        by = this->allMedals[n4 + 2];
        bool bl3 = (this->medals[by / 32] & 1 << by % 32) != 0;
        if (b && !bl3) continue;
        this->medalInfo[n3 + 0] = this->allMedals[n4 + 1];
        this->medalInfo[n3 + 1] = bl3 ? (uint8_t)1 : 0;
        n3 += 2;
    }
    for (n2 = 150; n2 < 159; n2 += 3) {
        if (this->allMedals[n2] == n) {
            by = this->allMedals[n2 + 2];
            bool bl3 = (this->medals[by / 32] & 1 << by % 32) != 0;
            if (b && !bl3) continue;
            this->medalInfo[n3 + 0] = this->allMedals[n2 + 1];
            this->medalInfo[n3 + 1] = bl3 ? (uint8_t)1 : 0;
            n3 += 2;
            bl2 = true;
            continue;
        }
        if (bl2) break;
    }
    return this->medalInfo;
}

void Player::statusToString(int n, Text* text) {
    Applet* app = CAppContainer::getInstance()->app;
    short s = 0;
    switch (n) {
    case Enums::STATUS_EFFECT_REFLECT:
        s = CodeStrings::EFFECT_REFLECT;
        break;
    case Enums::STATUS_EFFECT_PURIFY:
        s = CodeStrings::EFFECT_PURIFY;
        break;
    case Enums::STATUS_EFFECT_AGILITY:
        s = CodeStrings::EFFECT_AGILITY;
        break;
    case Enums::STATUS_EFFECT_REGEN:
        s = CodeStrings::EFFECT_REGEN;
        break;
    case Enums::STATUS_EFFECT_DEFENSE:
        s = CodeStrings::EFFECT_DEFENSE;
        break;
    case Enums::STATUS_EFFECT_ADRENALINE:
        s = CodeStrings::EFFECT_ADRENALINE;
        break;
    case Enums::STATUS_EFFECT_EVADE:
        s = CodeStrings::EFFECT_EVADE;
        break;
    case Enums::STATUS_EFFECT_FOCUS:
        s = CodeStrings::EFFECT_FOCUS;
        break;
    case Enums::STATUS_EFFECT_ANGER:
        s = CodeStrings::EFFECT_ANGER;
        break;
    case Enums::STATUS_EFFECT_ANTIFIRE:
        s = CodeStrings::EFFECT_ANTIFIRE;
        break;
    case Enums::STATUS_EFFECT_FORTITUDE:
        s = CodeStrings::EFFECT_FORTITUDE;
        break;
    case Enums::STATUS_EFFECT_FEAR:
        s = CodeStrings::EFFECT_FEAR;
        break;
    case Enums::STATUS_EFFECT_AIR:
        s = CodeStrings::EFFECT_AIR;
        break;
    case Enums::STATUS_EFFECT_FIRE:
        s = CodeStrings::EFFECT_FIRE;
        break;
    case Enums::STATUS_EFFECT_DISEASE:
        s = CodeStrings::EFFECT_DISEASE;
        break;
    case Enums::STATUS_EFFECT_DRUNK:
        s = CodeStrings::EFFECT_DRUNK;
        break;
    case Enums::STATUS_EFFECT_DIZZY:
        s = CodeStrings::EFFECT_DIZZY;
        break;
    }
    app->localization->composeText(Strings::FILE_CODESTRINGS, s, text);
}

bool Player::hasPurifyEffect() {
    return this->statusEffects[(Enums::OFS_STATUSEFFECT_TURNS + Enums::STATUS_EFFECT_PURIFY)] != 0;
}

void Player::checkForCloseSoundObjects() {
    Applet* app = CAppContainer::getInstance()->app;
    Render* render;
    Canvas* canvas;
    EntityMonster* monster;

    this->soundFire = false;

    canvas = app->canvas;
    render = app->render;

    int posX = canvas->destX >> 6;
    int posY = canvas->destY >> 6;
    int posZ = canvas->destZ >> 6;
    int local_38 = 0;
    int local_30 = 0;
    int local_34 = 0;

    for (int sprite = 0; sprite < render->numSprites; sprite++) {
        int spriteInfo = render->mapSpriteInfo[sprite];
        int flags = spriteInfo;

        if (flags & Enums::SPRITE_FLAG_HIDDEN) {
            continue;
        }

        int sX = render->mapSprites[sprite + render->S_X] >> 6;
        int sY = render->mapSprites[sprite + render->S_Y] >> 6;
        int sZ = render->mapSprites[sprite + render->S_Z] >> 6;

        if (std::abs(posX - sX) < 2 &&
            std::abs(posY - sY) < 2 &&
            std::abs(posZ - sZ) < 1) {

            int tileNum = spriteInfo & Enums::SPRITE_MASK_SPRITENUMBER;
            int frame = (spriteInfo & Enums::SPRITE_MASK_FRAMENUMBER) >> Enums::SPRITE_SHIFT_FRAMENUMBER;

            if (flags & Enums::SPRITE_FLAG_TILE) {
                tileNum += Enums::TILENUM_FIRST_WALL;
            }

            monster = nullptr;
            if (render->mapSprites[sprite + render->S_ENT] != -1) {
                monster = app->game->entities[render->mapSprites[sprite + render->S_ENT]].monster;
            }

            if (tileNum == Enums::TILENUM_OBJ_FIRE) {
                this->soundFire = true;
            }
            else if (tileNum <= Enums::TILENUM_FIRST_DECOR) {
                if (((tileNum == Enums::TILENUM_TOILET) || (tileNum == Enums::TILENUM_SINK)) && (frame == 2)) {
                    local_34 = 1;
                }
            }
            else if (tileNum == Enums::TILENUM_ANIM_FIRE) {
                local_38 = 1;
            }
            else if (tileNum == Enums::TILENUM_OBJ_WALLTORCH || tileNum == Enums::TILENUM_OBJ_TORCHIERE) {
                local_30 = 1;
            }

            if (monster) {
                if (monster->monsterEffects & EntityMonster::MFX_FIRE || tileNum == Enums::TILENUM_MONSTER_SKELETON2 || tileNum == Enums::TILENUM_MONSTER_SKELETON3) {
                    local_38 = 1;
                }
            }
        }
    }

    if (local_38 == 0) {
        app->sound->stopSound(1025, true);
    }
    else if (!app->sound->isSoundPlaying(1025)) {
        app->sound->playSound(1025, 1, 2, true); // "fire1.wav"
    }

    if (local_34 == 0) {
        app->sound->stopSound(1122, true);
    }
    else if (!app->sound->isSoundPlaying(1122)) {
        app->sound->playSound(1122, 1, 2, true); // "waterfall.wav"
    }

    if (local_30 == 0) {
        app->sound->stopSound(1026, true);
    }
    else if (!app->sound->isSoundPlaying(1026)) {
        app->sound->playSound(1026, 1, 2, true); // "firesoft.wav",
    }

    if (!this->soundFire && this->statusEffects[(Enums::OFS_STATUSEFFECT_TURNS + Enums::STATUS_EFFECT_FIRE)] <= 0) {
        app->sound->stopSound(1027, true);
    }
    else if (!app->sound->isSoundPlaying(1027)) {
        app->sound->playSound(1027, 1, 2, true); // "fl_fire.wav",
    }
}
