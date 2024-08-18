#include "TinyGamePCH.h"
#include "Object.h"

#include "Common.h"
#include "ObjModel.h"
#include "Action.h"
#include "Weapon.h"
#include "ObjectFactory.h"

namespace Shoot2D
{

	ObjModel const& getModel( Object& obj )
	{
		return ObjModelManger::Get().getModel( obj.getModelId() );
	}


	Object::Object( ModelId id ,Vec2D const& pos) 
		:mModelId(id)
		,mPos(pos)
		,mVel( 0.0f , 0.0f )
	{
		frame = 0;
		curDir = 0;
		nextDir = 0;

		m_HP = 1;
		m_Action = nullptr;
		m_Stats = STATS_LIVE;
	}

	void Object::setAction( Action* paction )
	{
		delete m_Action;
		m_Action = paction;
	}


	bool isNeedUpdate( ObjStats stats )
	{
		return stats == STATS_LIVE;
	}

	void Object::update(long time)
	{
		if ( m_Action )
			m_Action->update( *this , time );

		float dt = float(time) / 1000.f;
		mPos += dt * mVel ;

		int difDir = ( nextDir - curDir + 32 ) % 32;
		if ( difDir != 0 )
		{
			if ( difDir > 16 )
				curDir = (curDir + 32 - 1) % 32;
			else
				curDir = (curDir + 32 + 1) % 32;
		}

		++frame;
	}

	void Object::processCollision( Object& obj )
	{
		--m_HP;
		if ( m_HP <= 0 )
		{
			m_Stats = STATS_DEAD;
			ObjModel const& model = getModel(*this);
			if ( model.id != MD_NO_MODEL)
			{
				Ghost* bow = ObjectCreator::create<Ghost>( model.id , getCenterPos(*this) );
				bow->setVelocity( Random(0.2, 0.4 ) * getVelocity() );
				bow->setAction( new Dying( 400 ) );
				spawnObject(  bow );
			}
		}
	}

	void Object::updateDir()
	{
		if ( mVel.length2() < 0.00001 )
			return;

		float theta = atan2( mVel.y , mVel.x );
		nextDir = int( (theta + PI/2 )/ PI * 16 );
		nextDir = ( nextDir + 32 ) % 32;

	}

	Vehicle::Vehicle(ModelId id,Vec2D const& pos) 
		:Object(id,pos)
		,weapon( nullptr )
	{
		m_HP = 10;
		colFlag = COL_DEFAULT_VEHICLE;
	}

	void Vehicle::setWeapon( Weapon* w )
	{
		delete weapon;
		weapon = w;
		weapon->owner = this;
	}

	void Vehicle::update(long time)
	{
		if ( isNeedUpdate(m_Stats) )

			Object::update(time);
		if (weapon) 
			weapon->update(time);
		return;
	}

	bool Vehicle::tryFire()
	{
		if ( weapon )
			return weapon->tryFire();
		return false;
	}

	Vehicle::~Vehicle()
	{
		delete weapon;
	}

	Bullet::Bullet( ModelId id , Object* owner , Vec2D& difPos /*= Vec2D(0,0) */ ) 
		:Object( id )
	{
		setCenterPos( *this , getCenterPos(*owner) );
		colFlag =  COL_DEFAULT_BULLET ;
		m_Team = owner->getTeam();
		shiftPos( difPos );
	}

	Vec2D getCenterPos( Object& obj )
	{
		return obj.getPos() + getModel(obj).getCenterOffset();
	}

	void  setCenterPos( Object& obj , Vec2D const& pos)
	{
		obj.setPos( pos - getModel(obj).getCenterOffset() );
	}


	Ghost::Ghost( ModelId id , Vec2D& pos ) 
		:Object( id , pos )
	{
		colFlag = COL_NO_NEED;
		m_Team = TEAM_FRIEND;
	}

}//namespace Shoot2D