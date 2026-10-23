#ifndef __ENTITYMONSTER_H__
#define __ENTITYMONSTER_H__

#include "CombatEntity.h"
class OutputStream;
class InputStream;
class CombatEntity;
class Entity;

class EntityMonster
{
private:

public:

	 static constexpr int GOAL_NONE = 0;
	 static constexpr int GOAL_MOVE = 1;
	 static constexpr int GOAL_MOVETOENTITY = 2;
	 static constexpr int GOAL_FIGHT = 3;
	 static constexpr int GOAL_FLEE = 4;
	 static constexpr int GOAL_EVADE = 5;
	 static constexpr int GOAL_STUN = 6;
	 static constexpr int GFL_LERPING = 1;
	 static constexpr int GFL_SPECIAL = 2;
	 static constexpr int GFL_MOVE2ATTACK = 4;
	 static constexpr int GFL_ATTACK2EVADE = 8;
	 static constexpr int GFL_MOVEAGAIN = 16;
	 static constexpr int GFL_STEALTHMOVE = 32;
	 static constexpr int GFL_RAMBOMOVE = 64;
	 static constexpr int MAX_GOAL_TURNS = 16;

	 static constexpr int DEFAULT_PAIN_TIME = 250;

	 static constexpr int MFX_COUNT = 5;
	 static constexpr int MFX_NONE = 0;
	 static constexpr int MFX_POISON = 1;
	 static constexpr int MFX_FREEZE = 2;
	 static constexpr int MFX_RAISE_TIMER = 4;
	 static constexpr int MFX_FIRE = 8;
	 static constexpr int MFX_SHIELD = 16;

	 static constexpr int MFX_MAX = 16;
	 static constexpr int MFX_MASK_ALL = (MFX_POISON | MFX_FREEZE | MFX_RAISE_TIMER | MFX_FIRE | MFX_SHIELD); // 31

	 static constexpr int MFX_POISON_SHIFT = 5;  // 1 << 5 = 32
	 static constexpr int MFX_FREEZE_SHIFT = 7;  // 1 << 7 = 128
	 static constexpr int MFX_RAISE_SHIFT = 9;	 // 1 << 9 = 512
	 static constexpr int MFX_FIRE_SHIFT = 11;	 // 1 << 11 = 2048
	 static constexpr int MFX_SHIELD_SHIFT = 13; // 1 << 13 = 8192

	 static constexpr int MFX_TURN_MASK = 3;
	 static constexpr int MFX_POISON_REMOVE = ~((MFX_TURN_MASK << MFX_POISON_SHIFT) | MFX_POISON);	// -98
	 static constexpr int MFX_FREEZE_REMOVE = ~((MFX_TURN_MASK << MFX_FREEZE_SHIFT) | MFX_FREEZE);	// -387
	 static constexpr int MFX_RAISE_REMOVE  = ~((MFX_TURN_MASK << MFX_RAISE_SHIFT)  | MFX_RAISE_TIMER);	// -1541
	 static constexpr int MFX_FIRE_REMOVE   = ~((MFX_TURN_MASK << MFX_FIRE_SHIFT)   | MFX_FIRE);	// -6153
	 static constexpr int MFX_SHIELD_REMOVE = ~((MFX_TURN_MASK << MFX_SHIELD_SHIFT) | MFX_SHIELD);	// -24593
	 static constexpr int MFX_REMOVE_TURNS  = ~((MFX_TURN_MASK << MFX_POISON_SHIFT) | (MFX_TURN_MASK << MFX_FREEZE_SHIFT) | (MFX_TURN_MASK << MFX_RAISE_SHIFT) | (MFX_TURN_MASK << MFX_FIRE_SHIFT) | (MFX_TURN_MASK << MFX_SHIELD_SHIFT));	// -32737
	 static constexpr int MFX_ALL_ONE_TURNS = ((1 << MFX_POISON_SHIFT) | (1 << MFX_FREEZE_SHIFT) | (1 << MFX_FIRE_SHIFT) | (1 << MFX_SHIELD_SHIFT) /*| (1 << MFX_RAISE_SHIFT)*/); // 10400

	int touchMe;
	CombatEntity ce;
	Entity* nextOnList;
	Entity* prevOnList;
	Entity* nextAttacker;
	Entity* target;
	int frameTime;
	short flags;
	short monsterEffects;
	uint8_t goalType;
	uint8_t goalFlags;
	uint8_t goalTurns;
	int goalX;
	int goalY;
	int goalParam;

	// Constructor
	EntityMonster();

	void clearEffects();
	void reset();
	void saveGoalState(OutputStream* OS);
	void loadGoalState(InputStream* IS);
	void resetGoal();
};

#endif