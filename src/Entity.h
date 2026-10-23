#ifndef __ENTITY_H__
#define __ENTITY_H__

#include "Combat.h"
class EntityDef;
class EntityMonster;
class LerpSprite;
class OutputStream;
class InputStream;

class Entity
{
private:

public:

	static constexpr int ENTITY_FLAG_GIBBED = 0X10000; // 65536
	static constexpr int ENTITY_FLAG_TAKEDAMAGE = 0X20000; // 131072
	static constexpr int ENTITY_FLAG_ACTIVE = 0x40000; // 262144
	static constexpr int ENTITY_FLAG_GHOST = 0x80000; // 524288
	static constexpr int ENTITY_FLAG_LINKED = 0x100000; // 1048576
	static constexpr int ENTITY_FLAG_HASFOUGHT = 0x200000; // 2097152
	static constexpr int ENTITY_FLAG_DROPPED = 0x200000; // 2097152
	static constexpr int ENTITY_FLAG_DIRTY = 0x400000; // 4194304
	static constexpr int ENTITY_FLAG_CORPSE = 0x1000000; // 16777216
	static constexpr int ENTITY_FLAG_DEATHFUNC = 0x2000000; // 33554432
	static constexpr int ENTITY_FLAG_HURT = 0x4000000; // 67108864
	static constexpr int ENTITY_FLAG_RAISETARGET = 0x8000000; // 134217728
	static constexpr int ENTITY_FLAG_NOSNAP = 0x10000000; // 268435456

	static constexpr int HARBINGER_HEAL_TURNS = 7;
	static constexpr int DIR_BITS = 2;
	static constexpr int NUM_DIRS = 4;
	static constexpr int DIR_MASK = 3;
	static constexpr int DIR_TABLE_SHIFT = 2;
	static constexpr int PATH_BITS = 64;
	static constexpr int MAX_PATH_DEPTH = 8;
	static constexpr int MAX_DIST = 999999999;

	static constexpr int LINE_OF_SIGHT_YES = 0;
	static constexpr int LINE_OF_SIGHT_NO = 1;
	static constexpr int LINE_OF_SIGHT_BOTH = 2;

	int touchMe;
	EntityDef* def;
	EntityMonster* monster;
	Entity* nextOnTile;
	Entity* prevOnTile;
	short linkIndex;
	short name;
	int info;
	int param;
	int* lootSet;
	int32_t knockbackDelta[2];
	int pos[2];
	int tempSaveBuf[2];

	//static int HARBINGER_BLOOD_POOLX = 0;
	//static int HARBINGER_BLOOD_POOLY = 0;

	// Constructor
	Entity();
	// Destructor
	~Entity();

	bool startup();
	bool hasHead();
	void reset();
	void initspawn();
	int getSprite();
	bool touched(bool b);
	bool touchedItem(bool b);
	bool pain(int n, Entity* entity);
	void checkMonsterDeath(bool b);
	void died(bool b, Entity* entity);
	void dropChickenPlate(Entity* entity);
	bool deathByExplosion(Entity* entity);
	void dropItem(int n);
	void aiCalcSimpleGoal(bool b);
	bool aiCalcTormentorGoal();
	bool aiCalcHarbingerGoal();
	void aiMoveToGoal();
	void aiChooseNewGoal(bool b);
	bool aiIsValidGoal();
	bool aiIsAttackValid();
	void aiThink(bool b);

	static bool CheckWeaponMask(char n1, int n2) {
		Combat* combat = CAppContainer::getInstance()->app->combat;
		if (n1 <= 31) {
			return (combat->weaponMasks[(2 * n2) + 0] >> n1) & 1;
		}
		else {
			return (combat->weaponMasks[(2 * n2) + 1] >> (n1 - 32)) & 1;
		}
	}

	int aiWeaponForTarget(Entity* entity);
	LerpSprite* aiInitLerp(int travelTime);
	void aiFinishLerp();
	bool checkLineOfSight(int n, int n2, int n3, int n4, int n5);
	bool calcPath(int n, int n2, int n3, int n4, int n5, bool b);
	bool aiGoal_MOVE();
	void aiReachedGoal_MOVE();
	int distFrom(int n, int n2);
	void attack();
	void undoAttack();
	void trimCorpsePile(int n, int n2);
	void knockback(int n, int n2, int n3);
	int getFarthestKnockbackDist(int n, int n2, int n3, int n4, Entity* entity, int n5, int n6, int n7);
	void resurrect(int n, int n2, int n3);
	int* calcPosition();
	bool isBoss();
	bool isHasteResistant();
	bool isExplodableEntity();
	bool isDroppedEntity();
	bool isBinaryEntity(int* array);
	bool isNamedEntity(int* array);
	void saveState(OutputStream* OS, int n);
	void loadState(InputStream* IS, int n);
	int getSaveHandle(bool b);
	void restoreBinaryState(int n);
	short getIndex();
	void updateMonsterFX();
};

// ----------------
// EntityStrings Class
// ----------------

class EntityStrings
{
public:
    static constexpr int16_t DEF_PUNCH_NAME = 0;
    static constexpr int16_t DEF_PUNCH_LONGNAME = 0;
    static constexpr int16_t DEF_PUNCH_DESC = 1;
    static constexpr int16_t DEF_BRASS_PUNCH_NAME = 2;
    static constexpr int16_t DEF_BRASS_PUNCH_LONGNAME = 2;
    static constexpr int16_t DEF_BRASS_PUNCH_DESC = 3;
    static constexpr int16_t DEF_SPIKE_PUNCH_NAME = 4;
    static constexpr int16_t DEF_SPIKE_PUNCH_LONGNAME = 4;
    static constexpr int16_t DEF_SPIKE_PUNCH_DESC = 5;
    static constexpr int16_t DEF_BOOT_NAME = 6;
    static constexpr int16_t DEF_BOOT_LONGNAME = 6;
    static constexpr int16_t DEF_BOOT_DESC = 7;
    static constexpr int16_t DEF_THOMPSON_NAME = 8;
    static constexpr int16_t DEF_THOMPSON_LONGNAME = 8;
    static constexpr int16_t DEF_THOMPSON_DESC = 9;
    static constexpr int16_t DEF_STEN_NAME = 10;
    static constexpr int16_t DEF_STEN_LONGNAME = 10;
    static constexpr int16_t DEF_STEN_DESC = 11;
    static constexpr int16_t DEF_MAUSER_NAME = 12;
    static constexpr int16_t DEF_MAUSER_LONGNAME = 13;
    static constexpr int16_t DEF_MAUSER_DESC = 14;
    static constexpr int16_t DEF_FG42_NAME = 15;
    static constexpr int16_t DEF_FG42_LONGNAME = 16;
    static constexpr int16_t DEF_FG42_DESC = 17;
    static constexpr int16_t DEF_PANZER_NAME = 18;
    static constexpr int16_t DEF_PANZER_LONGNAME = 18;
    static constexpr int16_t DEF_PANZER_DESC = 19;
    static constexpr int16_t DEF_DYNAMITE_NAME = 20;
    static constexpr int16_t DEF_DYNAMITE_LONGNAME = 20;
    static constexpr int16_t DEF_DYNAMITE_DESC = 21;
    static constexpr int16_t DEF_PISTOL_NAME = 22;
    static constexpr int16_t DEF_PISTOL_LONGNAME = 22;
    static constexpr int16_t DEF_PISTOL_DESC = 23;
    static constexpr int16_t DEF_DUAL_PISTOL_NAME = 24;
    static constexpr int16_t DEF_DUAL_PISTOL_LONGNAME = 24;
    static constexpr int16_t DEF_DUAL_PISTOL_DESC = 25;
    static constexpr int16_t DEF_FLAMETHROWER_NAME = 26;
    static constexpr int16_t DEF_FLAMETHROWER_LONGNAME = 27;
    static constexpr int16_t DEF_FLAMETHROWER_DESC = 28;
    static constexpr int16_t DEF_VENOM_NAME = 29;
    static constexpr int16_t DEF_VENOM_LONGNAME = 29;
    static constexpr int16_t DEF_VENOM_DESC = 30;
    static constexpr int16_t DEF_TESLA_NAME = 31;
    static constexpr int16_t DEF_TESLA_LONGNAME = 31;
    static constexpr int16_t DEF_TESLA_DESC = 32;
    static constexpr int16_t DEF_SPEAR_NAME = 33;
    static constexpr int16_t DEF_SPEAR_LONGNAME = 33;
    static constexpr int16_t DEF_SPEAR_DESC = 34;
    static constexpr int16_t DEF_TORTURE_TABLE_NAME = 35;
    static constexpr int16_t DEF_PAINTING_NAME = 36;
    static constexpr int16_t DEF_SPIKES = 37;
    static constexpr int16_t MONSTER_SOLDIER1 = 38;
    static constexpr int16_t MONSTER_SOLDIER2 = 39;
    static constexpr int16_t MONSTER_SOLDIER3 = 40;
    static constexpr int16_t MONSTER_WORKER1 = 41;
    static constexpr int16_t MONSTER_WORKER2 = 42;
    static constexpr int16_t MONSTER_WORKER3 = 43;
    static constexpr int16_t MONSTER_SKELETON1 = 44;
    static constexpr int16_t MONSTER_SKELETON2 = 45;
    static constexpr int16_t MONSTER_SKELETON3 = 46;
    static constexpr int16_t MONSTER_OFFICER1 = 47;
    static constexpr int16_t MONSTER_OFFICER2 = 48;
    static constexpr int16_t MONSTER_OFFICER3 = 49;
    static constexpr int16_t MONSTER_OFFICER4 = 50;
    static constexpr int16_t MONSTER_SUPERS1 = 51;
    static constexpr int16_t MONSTER_SUPERS2 = 52;
    static constexpr int16_t MONSTER_SUPERS3 = 53;
    static constexpr int16_t MONSTER_TROOPER = 54;
    static constexpr int16_t MONSTER_TROOPER1 = 55;
    static constexpr int16_t MONSTER_TROOPER2 = 56;
    static constexpr int16_t MONSTER_TROOPER3 = 57;
    static constexpr int16_t MONSTER_EGUARD1 = 58;
    static constexpr int16_t MONSTER_EGUARD2 = 59;
    static constexpr int16_t MONSTER_EGUARD3 = 60;
    static constexpr int16_t MONSTER_ZOMBIE = 61;
    static constexpr int16_t MONSTER_ZOMBIE1 = 62;
    static constexpr int16_t MONSTER_ZOMBIE2 = 63;
    static constexpr int16_t MONSTER_ZOMBIE3 = 64;
    static constexpr int16_t MONSTER_TORMENTOR1 = 65;
    static constexpr int16_t MONSTER_TORMENTOR2 = 66;
    static constexpr int16_t MONSTER_CHICKEN = 67;
    static constexpr int16_t MONSTER_CHICKEN1 = 68;
    static constexpr int16_t BOSS_OLARIC = 69;
    static constexpr int16_t BOSS_MARRIANNA = 70;
    static constexpr int16_t BOSS_HARBINGER = 71;
    static constexpr int16_t KEY_SILVER_NAME = 72;
    static constexpr int16_t KEY_SILVER_DESC = 73;
    static constexpr int16_t KEY_GOLD_NAME = 74;
    static constexpr int16_t KEY_GOLD_DESC = 75;
    static constexpr int16_t ARMOR_HELMET_NAME = 76;
    static constexpr int16_t ARMOR_HELMET_LONGNAME = 77;
    static constexpr int16_t ARMOR_HELMET_DESC = 78;
    static constexpr int16_t AMMO_9MM_NAME = 79;
    static constexpr int16_t AMMO_9MM_LONGNAME = 79;
    static constexpr int16_t AMMO_9MM_DESC = 80;
    static constexpr int16_t AMMO_12_7MM_NAME = 81;
    static constexpr int16_t AMMO_12_7MM_LONGNAME = 81;
    static constexpr int16_t AMMO_12_7MM_DESC = 82;
    static constexpr int16_t AMMO_30CAL_NAME = 83;
    static constexpr int16_t AMMO_30CAL_LONGNAME = 83;
    static constexpr int16_t AMMO_30CAL_DESC = 84;
    static constexpr int16_t AMMO_45CAL_NAME = 85;
    static constexpr int16_t AMMO_45CAL_LONGNAME = 85;
    static constexpr int16_t AMMO_45CAL_DESC = 86;
    static constexpr int16_t AMMO_FUEL_NAME = 87;
    static constexpr int16_t AMMO_FUEL_LONGNAME = 88;
    static constexpr int16_t AMMO_FUEL_DESC = 89;
    static constexpr int16_t AMMO_ROCKETS_NAME = 90;
    static constexpr int16_t AMMO_ROCKETS_LONGNAME = 90;
    static constexpr int16_t AMMO_ROCKETS_DESC = 91;
    static constexpr int16_t AMMO_TESLA_DESC = 92;
    static constexpr int16_t EMPTY_STRING = 93;
    static constexpr int16_t OTHER_SCOTCH = 94;
    static constexpr int16_t OTHER_SCOTCH_LONGNAME = 95;
    static constexpr int16_t OTHER_SCOTCH_DESC = 96;
    static constexpr int16_t OTHER_PACK = 97;
    static constexpr int16_t OTHER_WORKER_PACK = 98;
    static constexpr int16_t OTHER_WORKER_PACK_LONGNAME = 98;
    static constexpr int16_t OTHER_WORKER_PACK_DESC = 99;
    static constexpr int16_t SYRINGE_EVADE_NAME = 100;
    static constexpr int16_t SYRINGE_EVADE_LONGNAME = 101;
    static constexpr int16_t SYRINGE_EVADE_DESC = 102;
    static constexpr int16_t SYRINGE_DEFENSE_NAME = 103;
    static constexpr int16_t SYRINGE_DEFENSE_LONGNAME = 104;
    static constexpr int16_t SYRINGE_DEFENSE_DESC = 105;
    static constexpr int16_t SYRINGE_ADRENALINE_NAME = 106;
    static constexpr int16_t SYRINGE_ADRENALINE_LONGNAME = 107;
    static constexpr int16_t SYRINGE_ADRENALINE_DESC = 108;
    static constexpr int16_t SYRINGE_FOCUS_NAME = 109;
    static constexpr int16_t SYRINGE_FOCUS_LONGNAME = 110;
    static constexpr int16_t SYRINGE_FOCUS_DESC = 111;
    static constexpr int16_t SYRINGE_FIRE_NAME = 112;
    static constexpr int16_t SYRINGE_FIRE_LONGNAME = 113;
    static constexpr int16_t SYRINGE_FIRE_DESC = 114;
    static constexpr int16_t SYRINGE_ENRAGE_NAME = 115;
    static constexpr int16_t SYRINGE_ENRAGE_LONGNAME = 116;
    static constexpr int16_t SYRINGE_ENRAGE_DESC = 117;
    static constexpr int16_t SYRINGE_AGILITY_NAME = 118;
    static constexpr int16_t SYRINGE_AGILITY_LONGNAME = 119;
    static constexpr int16_t SYRINGE_AGILITY_DESC = 120;
    static constexpr int16_t SYRINGE_FORTITUDE_NAME = 121;
    static constexpr int16_t SYRINGE_FORTITUDE_LONGNAME = 122;
    static constexpr int16_t SYRINGE_FORTITUDE_DESC = 123;
    static constexpr int16_t SYRINGE_REGEN_NAME = 124;
    static constexpr int16_t SYRINGE_REGEN_LONGNAME = 125;
    static constexpr int16_t SYRINGE_REGEN_DESC = 126;
    static constexpr int16_t SYRINGE_PURIFY_NAME = 127;
    static constexpr int16_t SYRINGE_PURIFY_LONGNAME = 128;
    static constexpr int16_t SYRINGE_PURIFY_DESC = 129;
    static constexpr int16_t SYRINGE_REFLECT_NAME = 130;
    static constexpr int16_t SYRINGE_REFLECT_LONGNAME = 131;
    static constexpr int16_t SYRINGE_REFLECT_DESC = 132;
    static constexpr int16_t SYRINGE_PROTO_NAME = 133;
    static constexpr int16_t SYRINGE_PROTO_LONGNAME = 134;
    static constexpr int16_t SYRINGE_PROTO_DESC = 135;
    static constexpr int16_t SYRINGE_ELITE_NAME = 136;
    static constexpr int16_t SYRINGE_ELITE_LONGNAME = 137;
    static constexpr int16_t SYRINGE_ELITE_DESC = 138;
    static constexpr int16_t SYRINGE_FEAR_NAME = 139;
    static constexpr int16_t SYRINGE_FEAR_LONGNAME = 140;
    static constexpr int16_t SYRINGE_FEAR_DESC = 141;
    static constexpr int16_t SYRINGE_ANGER_NAME = 142;
    static constexpr int16_t SYRINGE_ANGER_LONGNAME = 143;
    static constexpr int16_t SYRINGE_ANGER_DESC = 144;
    static constexpr int16_t MISC_COCKTAIL_MIXER = 145;
    static constexpr int16_t MISC_AIR_BUBBLES = 146;
    static constexpr int16_t MISC_ALARM = 147;
    static constexpr int16_t MISC_SWITCH = 148;
    static constexpr int16_t MISC_BOOKSHELF = 149;
    static constexpr int16_t MISC_BOOK = 150;
    static constexpr int16_t MISC_TOMBSTONE = 151;
    static constexpr int16_t MISC_SARCOPHAGUS = 152;
    static constexpr int16_t MISC_CLIPBOARD = 153;
    static constexpr int16_t MISC_FIRE = 154;
    static constexpr int16_t MISC_TOILET = 155;
    static constexpr int16_t MISC_SINK = 156;
    static constexpr int16_t MISC_CHAIR = 157;
    static constexpr int16_t MISC_TABLE = 158;
    static constexpr int16_t MISC_BARREL = 159;
    static constexpr int16_t MISC_CRATE = 160;
    static constexpr int16_t MISC_BARRICADE = 161;
    static constexpr int16_t MISC_SILVER_DOOR = 162;
    static constexpr int16_t MISC_LOCKED_SILVER_DOOR = 163;
    static constexpr int16_t MISC_GOLD_DOOR = 164;
    static constexpr int16_t MISC_LOCKED_GOLD_DOOR = 165;
    static constexpr int16_t MISC_LOCKED_DOOR = 166;
    static constexpr int16_t MISC_UNLOCKED_DOOR = 167;
    static constexpr int16_t MISC_FLAG = 168;
    static constexpr int16_t INTERACT_PICKUP_DESC = 169;
    static constexpr int16_t STATUE_ARMORED = 170;
    static constexpr int16_t STATUE_ARMORED_LONGNAME = 171;
    static constexpr int16_t NPC_CYPRIAN = 172;
    static constexpr int16_t NPC_POW = 172;
    static constexpr int16_t NPC_CIVILIAN = 173;
    static constexpr int16_t NPC_SPY = 174;
    static constexpr int16_t NPC_CRAZY_NAZI = 175;
    static constexpr int16_t FOOD_RATION = 176;
    static constexpr int16_t FOOD_RATION_DESC = 177;
    static constexpr int16_t HEALTH_PACK = 178;
    static constexpr int16_t HEALTH_PACK_DESC = 179;
    static constexpr int16_t FOOD_PLATE = 180;
    static constexpr int16_t BURNT_FOOD_PLATE = 181;
    static constexpr int16_t EXIT = 182;
    static constexpr int16_t DEF_NOB_SPRITEWALL = 183;
    static constexpr int16_t DEF_SPRITEWALL = 184;
    static constexpr int16_t DEF_SECRET_SPRITEWALL = 185;
    static constexpr int16_t DEF_WORLD = 186;
    static constexpr int16_t DEF_JOURNAL = 187;
    static constexpr int16_t DEF_JOURNAL_DESCRIPTION = 188;
    static constexpr int16_t DEF_GOLD = 189;
    static constexpr int16_t DEF_GOLD_DESC = 190;
    static constexpr int16_t DEF_GOLD_GOBLET = 191;
    static constexpr int16_t DEF_GOLD_GOBLET_DESC = 192;
    static constexpr int16_t DEF_GOLD_CROSS = 193;
    static constexpr int16_t DEF_GOLD_CROSS_DESC = 194;
    static constexpr int16_t DEF_GOLD_CROWN = 195;
    static constexpr int16_t DEF_GOLD_CROWN_DESC = 196;
    static constexpr int16_t EMPTY_SYRINGE_NAME = 197;
    static constexpr int16_t EMPTY_SYRINGE_DESC = 198;
    static constexpr int16_t MAX = 199;
};

#endif