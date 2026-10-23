#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "Enums.h"
#include "Entity.h"
#include "EntityDef.h"
#include "CombatEntity.h"

class Entity;
class EntityDef;
class CombatEntity;
class Text;
class InputStream;
class OutputStream;
class Graphics;
class ScriptThread;
class GameSprite;


class Player
{
private:

public:
	static constexpr int EXPIRE_DURATION = 5;
	static constexpr int MAX_DISPLAY_BUFFS = 6;
	static constexpr int ICE_FOG_DIST = 1024;
	static constexpr int MAX_NOTEBOOK_INDEXES = 8;
	static constexpr int NUMBER_OF_TARGET_PRACTICE_SHOTS = 8;
	static constexpr int HEAD_SHOT_POINTS = 30;
	static constexpr int BODY_SHOT_POINTS = 20;
	static constexpr int LEG_SHOT_POINTS = 10;
	static constexpr int NUMBER_OF_BITS_PER_VM_TRY = 3;
	static constexpr int BUFF_TURNS = 0;
	static constexpr int BUFF_AMOUNT = 15;
	static constexpr int DEF_STATUS_TURNS = 30;
	static constexpr int ANTI_FIRE_TURNS = 10;
	static constexpr int AGILITY_TURNS = 20;
	static constexpr int PURIFY_TURNS = 10;
	static constexpr int FEAR_TURNS = 6;
	static constexpr int COLD_TURNS = 5;
	static constexpr int BOX_X1 = 17;
	static constexpr int BOX_X2 = 31;
	static constexpr int BOX_X3 = 43;
	static constexpr int BOX_X4 = 55;

	static constexpr int WeaponSounds[18] = { 1137, 1137, 1137, 1130, 1134, 1134, 1113, 1142, 1149 , 1139, 1128, 1127, 1135, 1129, 1143, 1141, 1146, 1149 };

	Entity* facingEntity;
	short inventory[Enums::INV_MAX];
	short ammo[10];
	int64_t weapons;
	short inventoryCopy[Enums::INV_MAX];
	short ammoCopy[10];
	int weaponsCopy;
	int disabledWeapons;
	int level;
	int currentXP;
	int nextLevelXP;
	int gold;
	CombatEntity* baseCe;
	CombatEntity* ce;
	EntityDef* activeWeaponDef;
	bool noclip;
	bool god;
	int playTime;
	int totalTime;
	int moves;
	int totalMoves;
	int completedLevels;
	int killedMonstersLevels;
	int foundSecretsLevels;
	int xpGained;
	int totalDeaths;
	short notebookIndexes[Player::MAX_NOTEBOOK_INDEXES];
	short notebookPositions[Player::MAX_NOTEBOOK_INDEXES];
	uint8_t questComplete;
	uint8_t questFailed;
	int numNotebookIndexes;
	int helpBitmask;
	int invHelpBitmask;
	int ammoHelpBitmask;
	int weaponHelpBitmask;
	int armorHelpBitmask;
	int cocktailDiscoverMask;
	int gamePlayedMask;
	int lastCombatTurn;
	bool inCombat;
	bool enableHelp;
	int turnTime;
	int highestMap;
	int prevWeapon;
	bool noDeathFlag;
	int numStatusEffects;
	int statusEffects[Enums::MAX_STATUS_EFFECTS];
	short buffs[Enums::BUFF_MAX * 2];
	int numbuffs;
	int purchasedWeapons;
	bool gameCompleted;
	GameSprite* playerSprite;
	int pickingStats;
	int oldPickingStats;
	int8_t* allMedals;
	int medals[2];
	uint8_t medalInfo[22];
	int8_t* bookMap;
	uint8_t foundBooks[4];
	bool showBookStat;
	int bookIndex;
	int counters[8];
	int monsterStats[2];
	int pos[3];
	bool soundFire;


	// Constructor
	Player();
	// Destructor
	~Player();

	bool startup();
	bool modifyCollision(Entity* entity);
	void advanceTurn();
	void levelInit();
	void fillMonsterStats();
	void readyWeapon();
	void selectWeapon(int i);
	void selectPrevWeapon();
	void selectNextWeapon();
	void modifyStat(int n, int n2);
	bool requireStat(int n, int n2);
	bool requireItem(int n, int n2, int n3, int n4);
	void addXP(int xp);
	void addLevel();
	int calcLevelXP(int n);
	int calcScore();
	bool addHealth(int i);
	bool addHealth(int i, bool b);
	void reset();
	int calcDamageDir(int x1, int y1, int angle, int x2, int y2);
	void painEvent(Entity* entity, bool b);
	void pain(int n, Entity* entity, bool b);
	void died();
	bool fireWeapon(Entity* entity, int n, int n2);
	bool useItem(int n);
	void giveGold(int n);
	bool give(int n, int n2, int n3);
	bool give(int n, int n2, int n3, bool b);
	void giveAmmoWeapon(int n, bool b);
	void updateQuests(short n, int n2);
	void setQuestTile(int n, int n2, int n3);
	bool isQuestDone(int n);
	bool isQuestFailed(int n);
	void formatTime(int n, Text* text);
	void showInvHelp(int n, bool b);
	void showAmmoHelp(int n, bool b);
	void showHelp(short n, bool b);
	void showWeaponHelp(int n, bool b);
	void showArmorHelp(int n, bool b);
	void drawBuffs(Graphics* graphics);
	bool loadState(InputStream* IS);
	bool saveState(OutputStream* OS);
	void unpause(int n);
	void relink();
	void unlink();
	void link();
	void updateStats();
	void updateStatusEffects();
	void translateStatusEffects();
	void removeStatusEffect(int effect);
	bool addStatusEffect(int effect, int amount, int turns);
	void drawStatusEffectIcon(Graphics* graphics, int n, int n2, int n3, int n4, int n5);
	void resetCounters();
	int* GetPos();
	Entity* getPlayerEnt();
	void setPickUpWeapon(int n);
	void giveAll();
	void equipForLevel(int mapID);
	void addArmor(int n);
	int distFrom(Entity* entity);
	void giveStandardMedal(int n, int n2);
	void giveMedal(int n, ScriptThread* scriptThread);
	void offerBook(int n, ScriptThread* scriptThread);
	void giveBook(int n, ScriptThread* scriptThread);
	uint8_t* getLevelMedalCount();
	bool hasAllKills(int n);
	uint8_t* getLevelMedals(int n, bool b);
	void statusToString(int n, Text* text);
	bool hasPurifyEffect();
	void checkForCloseSoundObjects();
};

#endif