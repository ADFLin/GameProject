#ifndef Player_h__
#define Player_h__

#include "Actor.h"

#include "ColBody.h"
#include "Block.h"
#include "Weapon.h"
#include "Light.h"

class LightObject;

class PlayerStart : public Actor
{
public:
	DECLARE_OBJECT_CLASS(PlayerStart, Actor)

	PlayerStart()
	{
		mSize = Vec2f(100, 100);
	}

	void renderDev(RHIGraphics2D& g, DevDrawMode mode);
};

class Player : public Actor
{
	DECLARE_OBJECT_CLASS( Player , Actor )
public:

	Player();

	int getPlayerId(){ return mPlayerId; }

	virtual void init();
	virtual void onSpawn( unsigned flag );
	virtual void onDestroy( unsigned flag );
	virtual void onBodyCollision( ColBody& self , ColBody& other );
	virtual void updateEdit();
	virtual IObjectRenderer* getRenderer();

	
	void  update( Vec2f const& aimPos );

	void  shoot( Vec2f const& posTaget );
	void  addWeapon(Weapon* o);
	void  addHP(float quantity);
	void  loseEnergy(float e);

	bool  isDead();
	float getEnergy() const { return mEnergy; }
	float getMaxEnergy() const { return 100; }
	float getHP() const { return mHP; }
	float getMaxHP() const { return 100; }

	void addMoment(float x);
	void takeDamage(Bullet* p);
	
private:
	void clearWeapons();
	bool testCollision( Vec2f const& offset );

	void updateHeadlight();


	friend class Level;
	int   mPlayerId;
	

	ColBody mBody;

	float vel;
	bool  mIsDead;

	float acceleration;

	float rotationAim;

	float mHP;
	float mEnergy;

	static int const NUM_WEAPON_SLOT = 4;
	Weapon* mWeaponSlot[ NUM_WEAPON_SLOT ];
	bool    haveShoot;

	Light  mHeadLight;

	float  shiftTrack;

	friend class PlayerRenderer;

};


#endif // Player_h__