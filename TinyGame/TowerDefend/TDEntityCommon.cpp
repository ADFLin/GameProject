#include "TDPCH.h"
#include "TDEntityCommon.h"

#include "TDWorld.h"

#include <algorithm>

namespace TowerDefend
{
	Actor::Actor( ActorId aID )
		:mActorID( aID )
	{
		ADD_ENTITY_TYPE()
		mOwner  = NULL;
		mCurAct = NULL;
	}

	Actor::~Actor()
	{

	}

	class TDMoveHelper
	{
	public:
		void setGoalPos( Vec2f const& pos )
		{
			Vec2f offset = pos - mOwner->getPos();
			float dist = normalize( offset );
		}

		void tick()
		{
			switch( mCurStragegy )
			{
			case MS_MOVE_FORWARD:
				break;
			case MS_DODGE_BLOCKER:
				break;
			case MS_FOLLOW_PATH:
				break;
			}
		}

		enum MoveStrategy
		{
			MS_FOLLOW_PATH   ,
			MS_DODGE_BLOCKER ,
			MS_MOVE_FORWARD  ,
			MS_NONE ,
		};

		enum BlockSolve
		{
			SOLVE_UNIT ,
			SOLVE_TERRAIN ,
		};

		Vec2f        mGoalPos;
		MoveStrategy mCurStragegy;
		MoveStrategy mSolveStragegy;
		Actor*       mOwner;
	};

	void Actor::onDestroy()
	{
		if( mCurAct )
		{
			mCurAct->onEnd( this , false );
			delete mCurAct;
		}
		removeActQueue();
	}

	void Actor::onTick()
	{
		BaseClass::onTick();

		if ( mCurAct == NULL && !mActQueue.empty() )
		{
			mCurAct = mActQueue.front();
			mCurAct->onStart( this );
			mActQueue.pop_front();
		}

		if ( mCurAct )
		{
			if( !mCurAct->onTick( this ) )
			{
				mCurAct->onEnd( this , true );
				delete mCurAct;
				mCurAct = NULL;
			}
		}
	}

	void Actor::changeAction( ActCommand* act , bool beRemoveQueue )
	{
		if ( beRemoveQueue )
			removeActQueue();

		if ( mCurAct )
		{
			mCurAct->onEnd( this , false );
			delete mCurAct;
		}

		mCurAct = act;
		if ( mCurAct )
			mCurAct->onStart( this );

		getWorld()->getEntityMgr().sendEvent( EVT_EN_CHANGE_COM , this );
	}

	void Actor::pushAction( ActCommand* act )
	{
		mActQueue.push_back( act );
		getWorld()->getEntityMgr().sendEvent( EVT_EN_PUSH_COM , this );
	}

	void Actor::addAction( ActCommand* act , unsigned flag )
	{
		if ( flag & CF_OP_APPEND )
		{
			pushAction( act );

		}
		else
		{
			changeAction( act , true );
		}
	}

	void Actor::removeActQueue()
	{
		for( ActComQueue::iterator iter = mActQueue.begin();
			iter != mActQueue.end() ; ++iter )
		{
			(*iter)->onSkip( this );
			delete (*iter);
		}
		mActQueue.clear();
	}

	float Actor::recvDamage( DamageInfo const& info )
	{
		if ( checkFlag( EF_DEAD ) )
			return 0;

		float damageValue = calcDamage( info );

		mLifeValue -= damageValue;

		onTakeDamage( info , damageValue );
		if ( mLifeValue <= 0 )
		{
			addFlag( EF_DEAD );
			getWorld()->getEntityMgr().sendEvent( EVT_EN_KILLED , this );
			onKilled();
		}

		return damageValue;
	}

	bool Actor::testFilter( EntityFilter const& filter )
	{
		if ( !BaseClass::testFilter( filter ) )
			return false;
		if ( filter.bitOwner )
		{
			if ( !( BIT( getOwnerID() ) & filter.bitOwner ) )
				return false;
		}
		return true;
	}

	bool Unit::MoveAirAct::onTick( Actor* actor )
	{
		float const MinDistance = 0.1f;
		Unit* unit = actor->castFast< Unit >();
		Vec2f delta = destPos - unit->getPos();

		Vec2f dir = delta;
		float dist = normalize( dir );

		if ( dist < MinDistance )
			return false;

		Vec2f offset = std::min( unit->getMoveSpeed() , dist ) * dir;
		unit->shiftPos( offset );

		return true;
	}

	void Unit::MoveAirAct::describe( ActEnumer& enumer )
	{
		enumer.haveGoalPos( destPos );
	}

	bool Unit::MoveLandAct::onTick( Actor* actor )
	{
		float const MinDistance = 0.1f;
		Unit* unit = actor->castFast< Unit >();
		Vec2f delta = destPos - unit->getPos();

		Vec2f dir = delta;
		float dist = normalize( dir );

		if ( dist < MinDistance )
			return false;

		static int count = 0; 


		if ( mColEntity )
		{
			Vec2f temp = mColEntity->getPos() - actor->getPos();
			float cross = dir.x * temp.y - dir.y * temp.x; 

			if ( fabs( cross ) < 1e-4 )
			{
				++count;
				if ( count % 2 )
				{
					dir.x = temp.y;
					dir.y = -temp.x;
				}
				else
				{
					dir.x = -temp.y;
					dir.y = temp.x;
				}
			}
			else if ( cross > 0 )
			{
				dir.x = temp.y;
				dir.y = -temp.x;
			}
			else
			{
				dir.x = -temp.y;
				dir.y = temp.x;
			}
			normalize( dir );
		}

		Vec2f offset = std::min( unit->getMoveSpeed() , dist ) * dir;

		float maxOffsetSqure = unit->getMoveSpeed() * unit->getMoveSpeed();

		Vec2f mapPos = unit->getPos() / gMapCellLength;


		WorldMap& map = actor->getWorld()->getMap();
		bool xCol = false;
		bool yCol = false;

		float radius = unit->getColRadius();

		++count;
		if ( count % 2 )
		{
			if ( !map.testCollisionX( mapPos , radius , CL_WALK , offset.x ) )
			{
				xCol = true;
			}
			mapPos.x += offset.x / gMapCellLength;
			if ( !map.testCollisionY( mapPos , radius , CL_WALK , offset.y ) )
			{
				yCol = true;
			}
			mapPos.y += offset.y / gMapCellLength;
		}
		else
		{
			if ( !map.testCollisionY( mapPos , radius , CL_WALK , offset.y ) )
			{
				yCol = true;
			}
			mapPos.y += offset.y / gMapCellLength;

			if ( !map.testCollisionX( mapPos , radius , CL_WALK , offset.x ) )
			{
				xCol = true;
			}
			mapPos.x += offset.x / gMapCellLength;
		}

		if ( xCol && yCol )
		{
			if ( mMode != TM_NONE )
				return false;
			mMode = TM_BLOCK;
		}

		float minError = 1.f;
		if ( xCol )
		{
			if ( fabs( delta.y + offset.y ) < minError )
				return false;

			float temp =  maxOffsetSqure - offset.x * offset.x;

			if ( temp < 0 )
			{
				return false;
			}
			float testOffset = sqrt( temp ) - fabs( offset.y );

			if ( offset.y < 0 )
				testOffset = -testOffset;

			map.testCollisionY( mapPos , radius , CL_WALK , testOffset );
			offset.y += testOffset;
		}
		else if ( yCol )
		{
			if ( fabs( delta.x + offset.x ) < minError )
				return false;

			float temp =  maxOffsetSqure - offset.y * offset.y;

			if ( temp < 0 )
			{
				return false;
			}

			float testOffset = sqrt( temp ) - fabs( offset.x );

			if ( offset.x < 0 )
				testOffset = -testOffset;

			map.testCollisionX( mapPos , radius , CL_WALK , testOffset );
			offset.x += testOffset;
		}
		else
		{
			mMode = TM_NONE;
		}

		ColObject* obj = actor->getWorld()->getCollisionMgr().testCollision( unit->mColBody , offset );
		mColEntity = ( obj ) ? &obj->getOwner() : NULL;

		unit->shiftPos( offset );
		return true;
	}



	Unit::Unit( ActorId aID ) 
		:BaseClass( aID )
		,mColBody( *this )
	{
		assert( isUnit( aID ) );

		ADD_ENTITY_TYPE();

		UnitInfo const& info = getUnitInfo( aID );
		assert( info.actorID == aID );

		mLifeValue = info.maxHealthValue;

		mColBody.setBoundRadius( info.colRadius );
	}

	void Unit::onSpawn()
	{
		getWorld()->getCollisionMgr().addObject( mColBody );
	}

	void Unit::onDestroy()
	{
		getWorld()->getCollisionMgr().removeObject( mColBody );
	}

	void Unit::onChangePos()
	{
		if ( getWorld() )
			getWorld()->getCollisionMgr().updateColObject( mColBody );
	}

	bool Unit::evalDefaultCommand( Actor* target , Vec2f const& pos , unsigned flag )
	{
		if ( BaseClass::evalDefaultCommand( target , pos , flag ) )
			return true;

		if ( target )
		{
			if ( target != this )
			{
				if ( getWorld()->getRelationship( this , target ) == PR_EMPTY )
				{
					return false;
				}
				else
				{
					FollowAct* act = new FollowAct;
					act->target = target;
					addAction( act , flag );
					return true;
				}
			}
		}
		return false;
	}

	void Unit::addMoveActionCom( Vec2f const& pos , unsigned flag )
	{
		ActCommand* act;
		if ( checkFlag( EF_FLY ) )
		{
			MoveAirAct* moveAct = new MoveAirAct;
			moveAct->destPos = pos;
			act = moveAct;
		}
		else
		{
			MoveLandAct* moveAct = new MoveLandAct;
			moveAct->destPos = pos;
			act = moveAct;
		}
		addAction( act , flag );
	}

	bool Unit::evalCommand( ComID comID , Actor* target , Vec2f const& pos , unsigned flag )
	{
		switch( comID )
		{
		case CID_MOVE:
			if ( target )
			{

			}
			else
			{
				addMoveActionCom( pos , flag );
				return true;
			}
			break;
		case CID_ATTACK:
			break;
		}

		return false;
	}

	bool Building::evalActorCommand( ComID comID , ActorId aID , Vec2f const& pos , unsigned flag )
	{
		switch( comID )
		{
		case CID_NULL:
			if ( isUnit( aID ) )
			{
				if ( !getWorld()->produceUnit( aID , getOwner() , true ) )
					return false;

				ProductUnitAct* act = new ProductUnitAct;
				act->unitID = aID;
				addAction( act , flag | CF_OP_APPEND );
				return true;
			}
			break;
		}
		return false;
	}

	Building::Building( ActorId aID ) :BaseClass( aID )
	{
		assert( isBuilding( aID ) );
		ADD_ENTITY_TYPE()
		mLifeValue = 0.8f * getBuildingInfo().maxHealthValue;
	}

	Building::ConstructAct* Building::setupBuild()
	{
		ConstructAct* act = new ConstructAct;
		changeAction( act , true );
		return act;
	}

	bool Building::ProductUnitAct::onTick( Actor* actor )
	{
		switch( mStep )
		{
		case eCheckPopu:
			{
				UnitInfo const& info = Unit::getUnitInfo( unitID );
				PlayerInfo* pInfo = actor->getOwner();
				if ( info.population + pInfo->curPopulation > pInfo->maxPopulation )
				{

					return true;
				}
				pInfo->curPopulation += info.population;
				mStep = eProduce;
			}
		case eProduce:
			{
				progressCount += 1000.0f / gTowerDefendFPS;
				if ( progressCount < Unit::getUnitInfo( unitID ).produceTime )
					return true;
				UnitInfo const& info = Unit::getUnitInfo( unitID );
				mUnit = info.factory->create( unitID )->castFast< Unit >();
				mUnit->setOwner( actor->getOwner() );
				mStep = ePutUnit;
			}
		case ePutUnit:
			{
				Building* building = actor->castFast< Building >();
				if ( !actor->getWorld()->tryPlaceUnit( mUnit , building , Vec2f(0,0) ) )
					return true;
			}
		}
		return false;
	}

	void Building::ProductUnitAct::onEnd( Actor* actor , bool beCompelete )
	{
		if ( !beCompelete )
		{
			PlayerInfo* pInfo = actor->getOwner();
			UnitInfo const& info = Unit::getUnitInfo( unitID );
			pInfo->curPopulation -= info.population;

			pInfo->gold += int ( info.goldCast );
			pInfo->wood += int ( info.woodCast );
		}
	}

	void Building::ProductUnitAct::onSkip( Actor* actor )
	{
		PlayerInfo* pInfo = actor->getOwner();
		if ( pInfo )
		{
			UnitInfo const& info = Unit::getUnitInfo( unitID );
			pInfo->curPopulation -= info.population;
			pInfo->gold += int ( info.goldCast );
			pInfo->wood += int ( info.woodCast );
		}
	}

	void Building::ProductUnitAct::onStart( Actor* actor )
	{
		mStep = eCheckPopu;
		mUnit = NULL;
		progressCount = 0;
	}


	bool Unit::FollowAct::onTick( Actor* actor )
	{
		Actor* ptr = target;
		if ( ptr == NULL )
			return false;
		if ( ptr->checkFlag( EF_DEAD ) )
			return false;

		float const MinMoveDist = 100;

		Vec2f dir = ptr->getPos() - actor->getPos();
		float dist = normalize( dir );

		if ( dist > MinMoveDist )
		{
			actor->shiftPos( std::min( dist , actor->castFast< Unit >()->getMoveSpeed() ) * dir );
		}
		return true;
	}

	void Unit::FollowAct::describe( ActEnumer& enumer )
	{
		enumer.haveTarget( target );
	}

	void Building::ConstructAct::onStart( Actor* actor )
	{
		progressCount = 0;
		Building* building = actor->castFast< Building >();
		building->addFlag( EF_BUILDING );
	}

	bool Building::ConstructAct::onTick( Actor* actor )
	{
		Building* building = actor->castFast< Building >();

		if ( progressCount > building->getBuildingInfo().buildTime )
		{
			building->removeFlag( EF_BUILDING );
			return false;
		}
		return true;
	}

	void Building::ConstructAct::onSkip( Actor* actor )
	{
		actor->destroy();
	}

	void Building::ConstructAct::onEnd( Actor* actor , bool beCompelete )
	{
		if ( !beCompelete )
			actor->destroy();
	}

	bool Tower::AttackCom::onTick( Actor* actor )
	{
		if ( target == NULL )
			return false;

		Tower* tower = actor->castFast< Tower >();
		Vec2f offset = tower->getPos() - target->getPos();

		float dist = normalize( offset );
		if ( dist > tower->getAttackRange() )
			return false;

		attackCount += tower->getActtackSpeed() / gTowerDefendFPS;
		if ( attackCount >= 1.0f )
		{
			if ( actor->attack( target , offset ) )
				attackCount = 0;
		}
		return true;
	}

	Tower::Tower( ActorId type ) 
		:BaseClass( type )
	{
		ADD_ENTITY_TYPE();

		mATRange = 200;
		mATSpeed = 1;
		mATPower = 1;
	}


	void Tower::onTick()
	{
		BaseClass::onTick();

		if ( mCurAct == NULL && !checkFlag( EF_BUILDING ) )
		{
			EntityFilter filter;
			filter.bitOwner = ~BIT( getOwnerID() );
			filter.bitType  = ET_UNIT;
			Entity* entity = getWorld()->getEntityMgr().findEntity( getPos() , mATRange , false , filter );

			if ( entity )
			{
				Actor* actor = entity->castFast< Actor >();

				AttackCom* act = new AttackCom;
				act->target = actor;
				pushAction( act );
			}
		}
	}

	bool Tower::attack( Actor* entity , Vec2f const& pos )
	{
		switch( getActorID() )
		{
		case AID_BT_TOWER_CUBE:
		case AID_BT_TOWER_TRIANGLE:
		case AID_BT_TOWER_DIAMON:
		case AID_BT_TOWER_CIRCLE:
			{
				Bullet* bullet = new Bullet( 
					Bullet::MoveFun( this , &Tower::bulletNormalMove ) );
				if ( entity )
				{
					bullet->mTarget        = entity;
					bullet->mLocationParam = entity->getPos();
				}
				else
				{
					bullet->mLocationParam = pos;
				}
				bullet->setHitFun( 
					Bullet::HitFun( this , &Tower::bulletNormalHit ));
				bullet->setPos( getPos() );
				getWorld()->getEntityMgr().addEntity( bullet );
			}
			return true;
		}
		return false;
	}

	void Tower::bulletNormalHit( Bullet& bullet )
	{
		if ( bullet.mTarget )
		{
			DamageInfo info;
			info.power   = getAttackPower();
			info.attacker = this;
			bullet.mTarget->recvDamage( info );
		}
	}

	bool Tower::evalDefaultCommand( Actor* target , Vec2f const& pos , unsigned flag )
	{
		//FIXME
		if ( target && target != this )
		{
			if ( target->getOwner() != getOwner() )
			{
				AttackCom* act = new AttackCom;
				act->target = target;
				addAction( act , flag );
			}
		}
		else
		{

		}
		return false;
	}

	bool Tower::bulletNormalMove( Bullet& bullet )
	{
		if ( bullet.mTarget )
			bullet.mLocationParam = bullet.mTarget->getPos();

		Vec2f dir = bullet.mLocationParam  - bullet.getPos();
		float dist = normalize( dir );

		float const MinHitDist = 5.0f;

		float const MoveSpeed = 2.5f;

		if ( dist < MinHitDist )
			return false;

		bullet.shiftPos( std::min( dist , MoveSpeed ) * dir );
		return true;
	}

	void Builder::BuildAct::onStart( Actor* actor )
	{
		Builder* builder = actor->castFast< Builder >();

		building = actor->getWorld()->constructBuilding( blgID , builder , destPos , true );
		if ( building )
		{
			constructAct = building->setupBuild();
			actor->addFlag( EF_BUILDING | EF_INVISIBLE );
		}
	}

	bool Builder::BuildAct::onTick( Actor* actor )
	{
		if ( building == NULL )
			return false;

		Builder* builder = actor->castFast< Builder >();

		if ( !building->checkFlag( EF_BUILDING ) )
			return false;

		ActCommand* com = building->getCurActCommand();

		constructAct->progressCount += 1000.0f / gTowerDefendFPS;
		return true;
	}

	void Builder::BuildAct::onEnd( Actor* actor , bool beCompelete )
	{
		actor->removeFlag( EF_BUILDING | EF_INVISIBLE );
	}


	Builder::Builder( ActorId aID ) :BaseClass( aID )
	{
		ADD_ENTITY_TYPE()
	}

	bool Builder::evalActorCommand( ComID comID , ActorId aID , Vec2f const& pos , unsigned flag )
	{
		switch( comID )
		{
		case CID_BUILD:
			{
				addMoveActionCom( pos , flag );
				BuildAct*  act = new BuildAct;
				act->blgID   = aID;
				act->destPos = pos;
				pushAction( act );
			}
			return true;
		}

		return false;
	}

	bool Builder::evalDefaultCommand( Actor* target , Vec2f const& pos , unsigned flag )
	{
		if ( BaseClass::evalDefaultCommand( target , pos , flag ) )
			return true;

		if ( target == NULL || target == this )
		{
			addMoveActionCom( pos , flag );
			return true;
		}

		return false;
	}

	void Builder::addMoveActionCom( Vec2f const& pos , unsigned flag )
	{
		MoveAirAct* act = new MoveAirAct;
		act->destPos = pos;
		addAction( act , flag );
	}

	void Bullet::onTick()
	{
		if ( !mMoveFun( *this ) )
		{
			if ( mHitFun )
			{
				mHitFun( *this );
			}
			destroy();
		}
	}

}//namespace TowerDefend
