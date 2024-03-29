#include "Mob.h"

#include "Level.h"
#include "Bullet.h"
#include "Explosion.h"
#include "Player.h"

#include "DebrisPickup.h"
#include "RenderUtility.h"

IMPL_OBJECT_CLASS( Mob , OT_MOB , "Mob" )

void Mob::init()
{
	BaseClass::init();

	mTarget = NULL;
	mSize.x=64;
	mSize.y=64;

	rotation=0;
	acceleration=0;
	mSpeed = 100;
	mMaxSpeed=100;	

	chargingRate=250;
	charging=0;
	domet=512;

	mHP=100;	
}

void Mob::onSpawn( unsigned flag )
{
	BaseClass::onSpawn( flag );
	mBody.setSize( getSize()- Vec2f(4,4) );
	mBody.setTypeMask( COL_SOILD | COL_RENDER );
	mBody.setColMask( COL_BULLET | COL_SOILD | COL_TERRAIN );
	getLevel()->getColManager().addBody( *this , mBody );

	if ( flag & SDF_CAST_EFFECT )
		spawnEffect();
}

void Mob::onDestroy( unsigned flag )
{
	getLevel()->getColManager().removeBody( mBody );

	if ( flag & SDF_CAST_EFFECT )
	{
		Explosion* e = getLevel()->createExplosion( getPos(), 128 );
		e->setParam(128,3000,200);
		for(int i=0; i<4; i++)
		{
			DebrisPickup* c = new DebrisPickup( getPos() );
			c->init();
			getLevel()->addItem( c );
		}
	}

	BaseClass::onDestroy( flag );
}

void Mob::tick()
{
	BaseClass::tick();

	float MaxMobViewDistance = 1000;

	Player* player = getLevel()->getPlayer();
	Vec2f offset = player->getPos() - getPos();

	if ( offset.length2() < MaxMobViewDistance * MaxMobViewDistance &&
		 !getLevel()->getColManager().rayTerrainTest( getPos() , player->getPos() , COL_VIEW ) )
	{
		mTarget = player;
		mTimeCantView = 0;
		mPosLastView = mTarget->getPos();
	}
	else
	{
		mTimeCantView += TICK_TIME;

		if ( mTimeCantView > 3 )
			mTarget = NULL;
	}

	acceleration = 1;

	if ( mTarget )
	{
		Vec2f dir;
		dir = mPosLastView - getPos();
		dir = Math::GetNormal( dir );

		rotation = atan2(dir.y, dir.x) + Math::DegToRad(90);

		float angle = rotation - Math::DegToRad( 90 );

		Vec2f monent = acceleration *  GetDirection(angle);
		Vec2f off = ( mSpeed * TICK_TIME ) * dir;

		Vec2f offset = Vec2f( 0 , 0 );

		offset.y += off.y;
		if( testCollision( offset ) )
			offset.y = 0;
		offset.x += off.x;
		if( testCollision( offset ) )
			offset.x = 0;

		mPos += offset;
	}

	if( charging < 100 )
	{
		charging += chargingRate * TICK_TIME;
	}

	if( mHP<=0 )
	{
		destroy();
	}

	acceleration=0;

}

void Mob::spawnEffect()
{
	Explosion* e = getLevel()->createExplosion( getPos(),512 );
	e->setParam(32,1000,100);
	e->setColor(Vec3f(0.25,0.5,1.0));
}

bool Mob::testCollision( Vec2f const& offset )
{
	ColInfo info;
	return getLevel()->getColManager().testCollision( info , offset , mBody , COL_SOILD | COL_TERRAIN );
}

void Mob::takeDamage(Bullet* bullet)
{
	mHP -= bullet->getDamage();
	bullet->destroy();
}

void Mob::shoot( IBulletFactory const& creator )
{
	if ( !mTarget )
		return;

	if( mTarget->isDead() )
		return;

	if( charging >= 100 )
	{
		Vec2f offset= mPosLastView - getPos();
		if( offset.length2() < domet * domet )
		{
			offset = Math::GetNormal( offset );
			Bullet* p = creator.create();
			p->init();
			p->setup( getPos() ,offset , TEAM_EMPTY );	
			getLevel()->addBullet(p);
		}	
		charging = 0;
	}
}

void Mob::onBodyCollision( ColBody& self , ColBody& other )
{
	LevelObject* obj = other.getClient();
	switch( obj->getType() )
	{
	case OT_BULLET:
		{
			Bullet* bullet = obj->castChecked< Bullet >();
			if ( bullet->team == TEAM_PLAYER )
			{
				takeDamage( bullet );
			}
		}
		break;
	}	
}

