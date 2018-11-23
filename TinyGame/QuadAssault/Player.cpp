#include "Player.h"

#include "Level.h"
#include "SoundManager.h"

#include "Light.h"
#include "Bullet.h"
#include "Mob.h"
#include "Explosion.h"

#include "Message.h"
#include "RenderUtility.h"

bool gPlayerGodPower = true;

IMPL_OBJECT_CLASS( Player , OT_PLAYER , "Player" )

Vec2f const gWeaponSlotOffset[] = 
{
	Vec2f(-9,-20) ,
	Vec2f( 9,-20) , 
	Vec2f(-24,0 ) ,
	Vec2f( 24,0 ) ,
};



Player::Player()
{
	mPlayerId = -1;
}


void Player::init()
{
	setSize( Vec2f(64,64) );

	acceleration = 0;

	for(int i=0; i<4; i++)
		mWeaponSlot[i]=NULL;

	mHP=100;
	mEnergy=100.0;
	haveShoot=false;
	mIsDead=false;

	shiftTrack=0.0;
	rotation=0.0;

	vel=200;

	mBody.setSize( getSize() - Vec2f(4,4) );
	mBody.setTypeMask( COL_SOILD | COL_PLAYER | COL_RENDER );
	mBody.setColMask( COL_TERRAIN | COL_OBJECT );
}

void Player::onSpawn( unsigned flag )
{
	BaseClass::onSpawn( flag );

	getLevel()->getColManager().addBody( *this , mBody );

	mHeadLight.radius = 1024;
	mHeadLight.host = this;
	mHeadLight.angle  = 2;
	mHeadLight.setColorParam(Vec3f(1.0, 1.0, 1.0), 16);
	mHeadLight.drawShadow = true;
	getLevel()->addLight( mHeadLight );
}

void Player::onDestroy( unsigned flag )
{
	getLevel()->getColManager().removeBody( mBody );
	clearWeapons();
	BaseClass::onDestroy( flag );
}

void Player::update( Vec2f const& aimPos )
{
	if( !mIsDead )
	{
		Vec2f moment = acceleration * GetDirection( rotation - Math::Deg2Rad(90) );

		Vec2f off = ( vel * TICK_TIME ) * moment;

		Vec2f offset = Vec2f( 0 , 0 );

		offset.y += off.y;
		if( testCollision( offset ) )
			offset.y = 0;
		offset.x += off.x;
		if( testCollision( offset ) )
			offset.x = 0;

		mPos += offset;

		if(moment.x!=0 || moment.y!=0)
		{
			if(acceleration>0)
				shiftTrack += TICK_TIME;
			else
				shiftTrack -= TICK_TIME;
			if(shiftTrack>1.0)
				shiftTrack=0.0;
			if(shiftTrack<0.0)
				shiftTrack=1.0;
		}
		acceleration=0;

		Vec2f dir = aimPos - getPos();
		rotationAim = Math::ATan2( dir.y , dir.x );

		for(int i=0; i< NUM_WEAPON_SLOT ; i++)
		{
			if( mWeaponSlot[i] )
				mWeaponSlot[i]->tick();
		}

		updateHeadlight();

		if( !haveShoot )
		{
			mEnergy += 10* TICK_TIME;
			if(mEnergy>100.0)
				mEnergy=100.0;		
		}
		haveShoot=false;

		if( mHP<=0.0 )
		{
			mHP=0.0;
		
			Explosion* e= getLevel()->createExplosion( getPos(),512 );
			e->setParam(256,3000,200);

			getLevel()->playSound("explosion1.wav");		

			Message* gameOverMsg = new Message();
			gameOverMsg->init("Base", "All units lost, mission Failed." , 4, "blip.wav" );
			getLevel()->addMessage( gameOverMsg );

			mHeadLight.setColorParam(Vec3f(0,0,0), 0);

			mIsDead = true;

			LevelEvent event;
			event.id = LevelEvent::ePLAYER_DEAD;
			getLevel()->sendEvent( event );
		}
	}
}

void Player::onBodyCollision( ColBody& self , ColBody& other )
{
	LevelObject* obj = other.getClient();
	switch( obj->getType() )
	{
	case OT_BULLET:
		{
			Bullet* bullet = obj->cast< Bullet >();
			if ( bullet->team == TEAM_EMPTY )
			{
				takeDamage( bullet );
			}
		}
		break;
	}
}

void Player::updateHeadlight()
{
	float angle = rotationAim + Math::Deg2Rad(180);
	Vec2f dir = GetDirection( angle );
	mHeadLight.dir = dir;

	float dist = 34;
	mHeadLight.offset = dist * GetDirection( rotationAim );
}

void Player::addMoment(float x)
{
	acceleration=x;
}


void Player::shoot( Vec2f const& posTaget )
{
	if( mIsDead )
		return;

	Vec2f dir = Math::GetNormal( posTaget - getPos() );

	for(int i=0; i< NUM_WEAPON_SLOT; i++)
	{		
		if( !mWeaponSlot[i] )
			continue;

		if( mEnergy >= mWeaponSlot[i]->getEnergyCast() )
		{
			Vec2f offset = mWeaponSlot[i]->getPos();

			float angle = rotationAim + Math::ATan2(offset.y,offset.x) + Math::Deg2Rad( 90 );

			Vec2f slotDir = GetDirection( angle );
			mWeaponSlot[i]->fire( getPos() + offset.length() * slotDir , dir , TEAM_PLAYER );
			haveShoot=true;
		}
	}

}

void Player::takeDamage(Bullet* p)
{
	if( mIsDead )
		return;

	if ( !gPlayerGodPower )
	{
		mHP -= p->getDamage();
	}
	p->destroy();

}

void Player::addWeapon( Weapon* weapon )
{
	for(int i=0; i<4; i++ )
	{
		if( mWeaponSlot[i] )
			continue;

		weapon->init( this );
		weapon->setPos( gWeaponSlotOffset[i] );

		mWeaponSlot[i] = weapon;
		weapon = NULL;
		break;
	}

	if ( weapon )
	{
		delete weapon;
	}
}

bool Player::isDead()
{
	return mIsDead;
}

void Player::loseEnergy(float e)
{
	mEnergy-=e;
	if(mEnergy<0.0)
		mEnergy=0.0;
}

void Player::addHP(float quantity)
{
	mHP += quantity;
	if(mHP > getMaxHP() )
		mHP = getMaxHP();
}

void Player::clearWeapons()
{
	for(int i=0; i<4; i++)
	{
		if(mWeaponSlot[i])
		{
			delete mWeaponSlot[i];	
			mWeaponSlot[i]=NULL;
		}
	}
}

bool Player::testCollision( Vec2f const& offset )
{
	ColInfo info;
	return getLevel()->getColManager().testCollision( info , offset , mBody , COL_SOILD | COL_TERRAIN );
}

void Player::updateEdit()
{
	BaseClass::updateEdit();
	updateHeadlight();
}
