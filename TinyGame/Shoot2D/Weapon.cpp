#include "Weapon.h"

#include "Object.h"
#include "ObjectFactory.h"
#include "Action.h"

#include <algorithm>

namespace Shoot2D
{
	Weapon::Weapon(BulletGen* gen)
		:gen(gen)
		,owner(NULL)
		,destObj(NULL)
		,cdtime( 500 )
		,maxFireCount(1)
		,maxSaveCount(1)
		,deleyTime(20)
	{
		init();
	}

	Weapon::~Weapon()
	{
		delete gen;
	}

	void Weapon::init()
	{
		bulletType = BT_NORMAL ;
		fireTime = 0;
		idleTime = 0;
		saveCount = 0;
		fireCount = 0;
	}

	void Weapon::update( long time )
	{
		idleTime += time;
		if ( stats == WS_FIRING )
		{
			fireTime += time;
			if ( fireTime > deleyTime )
				fire();
		}
		if ( idleTime > cdtime )
		{
			idleTime -= cdtime;
			saveCount = std::min( saveCount + 1 , maxSaveCount );
		}
	}

	void Weapon::fire()
	{
		gen->generate(this);
		++fireCount;
		fireTime -= deleyTime;

		if ( fireCount >= maxFireCount )
		{
			makeCD();
		}
	}

	bool Weapon::tryFire()
	{
		if ( stats == WS_FIRING || saveCount <= 0 )
			return false;

		stats = WS_FIRING;
		fire();
		return true;
	}

	void Weapon::makeCD()
	{
		stats = WS_CD;
		fireTime = 0;
		fireCount = 0;
		--saveCount;
	}

	Object* Weapon::createBullet()
	{
		Object* out = ObjectCreator::create<Bullet>( bullet , owner , difPos );

		if ( bulletType == Weapon::BT_MISSILE )
		{
			out->setAction( new Follow( bulletSpeed , destObj ) );
		}
		return out;
	}

	void SimpleGen::generate( Weapon* weapon )
	{
		Object* owner = weapon->owner;
		Object* destObj = weapon->destObj;

		Vec2D dif = destObj->getPos() - owner->getPos();
		float s= sqrtf(dif.length2());
		Vec2D speed = ( weapon->bulletSpeed / s ) * dif;

		Object* bullet = weapon->createBullet();
		bullet->setVelocity( speed );
		spawnObject(bullet);
	}

	DirArrayGen::DirArrayGen( Vec2D const& d )
	{
		difPos = Vec2D(0,0);
		dir = ( 1.0f / sqrtf( d.length2() ) ) * d;
		num = 1;
		offset = 2;
	}

	void DirArrayGen::generate( Weapon* weapon )
	{
		float len = ( num - 1 ) * offset;

		Vec2D arDir( dir.y , - dir.x );
		Vec2D d = offset * arDir;
		Vec2D pos  = difPos + ( - float(num) / 2 )* d;

		for( size_t i=0 ; i < num ; ++ i)
		{
			Object* bullet = weapon->createBullet();
			bullet->shiftPos( pos );
			bullet->setVelocity( weapon->bulletSpeed * dir );
			spawnObject(bullet);
			pos += d;
		}
	}

	void DirArrayGen::setDir( Vec2D const& d )
	{
		dir = ( 1.0f / sqrtf( d.length2() ) ) * d;
	}

}//namespace Shoot2D