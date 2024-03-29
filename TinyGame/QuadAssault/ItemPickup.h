#ifndef ItemPickup_h__
#define ItemPickup_h__

#include "Object.h"

#include "ColBody.h"

class Player;

class ItemPickup : public LevelObject
{
	DECLARE_OBJECT_CLASS( ItemPickup , LevelObject )
public:
	ItemPickup();
	ItemPickup( Vec2f const& pos );
	virtual void init();
	virtual void tick();
	virtual void onSpawn( unsigned flag );
	virtual void onDestroy( unsigned flag );
	virtual void onPick(Player* player);
	virtual void onBodyCollision( ColBody& self , ColBody& other );

protected:
	ColBody mBody;
};


#endif // ItemPickup_h__
