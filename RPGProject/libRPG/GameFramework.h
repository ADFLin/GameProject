#ifndef GameFramework_h__
#define GameFramework_h__

#include "common.h"
#include "ISystem.h"

class GameObjectSystem;
class LevelEditor;
class LevelManager;

class GameFramework
{
public:
	GameFramework();
	~GameFramework();

	bool init( SSystemInitParams& params );
	void update( long time );

	GameObjectSystem* getGameObjectSystem(){ return mObjectSystem; }
	ISystem*          getSystem(){ return mSystem;  }
	LevelEditor*      getLevelEditor(){ return mLevelEditor;  }
	LevelManager*     getLevelManager(){ return mLevelManager;  }
private:
	TPtrHolder< GameObjectSystem >  mObjectSystem;
	TPtrHolder< LevelEditor >       mLevelEditor;
	TPtrHolder< LevelManager >      mLevelManager;

	ISystem*     mSystem;
};
#endif // GameFramework_h__