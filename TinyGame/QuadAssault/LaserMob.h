#ifndef LaserMob_h__
#define LaserMob_h__

#include "Mob.h"

class LaserMob : public Mob
{
	DECLARE_OBJECT_CLASS( LaserMob , Mob )
public:
	virtual void init();
	virtual void onSpawn( unsigned flag );
	virtual void onDestroy( unsigned flag );
	virtual void tick();

	void takeDamage(Bullet* p);
	void shoot( IBulletFactory const& creator );

	virtual IObjectRenderer* getRenderer();

};

#endif // LaserMob_h__