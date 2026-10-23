#ifndef __DRIVINGGAME_H__
#define __DRIVINGGAME_H__

class Image;
class Text;
class Entity;
class EntityDef;
class ScriptThread;
class Graphics;

class fmButtonContainer;
class fmScrollButton;
class fmButton;
class fmSwipeArea;

class DrivingGame
{
private:

public:
	int fireAngle;
	int fireTime;
	int fireCount;
	int monsterFireTime;
	int mode;
	int curIndex;

	// Constructor
	DrivingGame();
	// Destructor
	~DrivingGame();

	void init();
	void drivingState();
	bool handleDrivingEvents(int key, int keyAction);

};
#endif