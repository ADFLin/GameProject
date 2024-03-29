#ifndef MOB_H
#define MOB_H

#include "Actor.h"
#include "ColBody.h"
#include "ObjectFactory.h"


class Player;

class Bullet;
typedef IFactoryT< Bullet > IBulletFactory;



class Mob : public Actor
{
	DECLARE_OBJECT_CLASS( Mob , Actor )
public:
	virtual void init();

	virtual void onSpawn( unsigned flag );
	virtual void onDestroy( unsigned flag );
	virtual void tick();
	virtual void onBodyCollision( ColBody& self , ColBody& other );

	virtual void shoot( IBulletFactory const& creator );
	

protected:

	virtual void spawnEffect();
	virtual void takeDamage(Bullet* p);

	bool testCollision( Vec2f const& offset );

	ColBody mBody;
	float   acceleration;

	float charging;

	//NASLIJED:
	float   mSpeed;
	float   mAcc = 100.0f;
	float   mMaxSpeed;
	float   mHP;
	float   chargingRate;
	float   domet;
	Player* mTarget;
	float   mTimeCantView;
	Vec2f   mPosLastView;

};
#endif