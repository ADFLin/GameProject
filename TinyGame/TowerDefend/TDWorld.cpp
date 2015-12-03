#include "TDPCH.h"
#include "TDWorld.h"

#include "TDEntityCommon.h"

namespace TowerDefend
{
	class BlockUnitSolver
	{
	public:
		BlockUnitSolver(){ result = true; }
		bool testUnit( ColObject& obj )
		{
			if ( builder == &obj.getOwner() )
				return true;

			Unit* unit = obj.getOwner().cast< Unit >();
			if ( unit && world->canControl( pInfo , unit ) )
			{

			}
			//if ( unit->getOwner() == pInfo )
			//{
			//	return true;
			//}

			result = false;
			return false;
		}
		bool trySolve( ColObject& obj )
		{
			if ( builder == &obj.getOwner() )
				return true;

			Unit* unit = obj.getOwner().cast< Unit >();
			if ( unit && world->canControl( pInfo , unit ) )
			{

			}

			result = false;
			return false;
		}

		bool           result;
		Vec2f          max , min;
		PlayerInfo*  pInfo;
		Unit*        builder;
		World*       world;
		bool           beTrySolve;
	};

	World::World() 
		:mEntityMgr( *this )
	{

	}


	bool World::canBuild( ActorId type , Unit* builder , Vec2f const& pos , Vec2i& mapPos , PlayerInfo* pInfo , bool needSolve )
	{
		if ( ! getMap().canBuild( type, pos , mapPos ) )
			return NULL;

		Vec2i size = Building::getBuildingInfo( type ).blgSize;

		BlockUnitSolver solver;
		solver.world   = this;
		solver.pInfo   = pInfo;
		solver.builder = builder;
		solver.min     = gMapCellLength * Vec2f( mapPos - size / 2 );
		solver.max     = solver.min + gMapCellLength * Vec2f( size );

		if ( needSolve )
			getCollisionMgr().testCollision( solver.min , solver.max , CollisionCallback( &solver , &BlockUnitSolver::trySolve ) );
		else
			getCollisionMgr().testCollision( solver.min , solver.max , CollisionCallback( &solver , &BlockUnitSolver::testUnit ) );

		return solver.result;
	}

	bool World::tryPlaceUnit( Unit* unit , Building* building , Vec2f const& targetPos )
	{
		BuildingInfo const& info = building->getBuildingInfo();

		Vec2f blgPos = building->getPos() - Vec2f( info.blgSize / 2 ) * gMapCellLength;
		float radius = unit->getColRadius();

		{
			float maxOffset = info.blgSize.x * gMapCellLength + radius;
			Vec2f startPos = blgPos + Vec2f( radius , info.blgSize.y * gMapCellLength + radius );
			if ( tryPlaceUnitInternal( unit , startPos , Vec2f(1,0) , maxOffset ) )
				return true;
		}

		{
			float maxOffset = info.blgSize.y * gMapCellLength + radius;
			Vec2f startPos = blgPos + Vec2f( info.blgSize.x * gMapCellLength + radius , info.blgSize.y * gMapCellLength - radius );
			if ( tryPlaceUnitInternal( unit , startPos , Vec2f(0,-1) , maxOffset ) )
				return true;
		}

		{
			float maxOffset = info.blgSize.x * gMapCellLength + radius;
			Vec2f startPos = blgPos + Vec2f( info.blgSize.x * gMapCellLength - radius , -radius );
			if ( tryPlaceUnitInternal( unit , startPos , Vec2f(-1,0) ,  maxOffset ) )
				return true;
		}

		{
			float maxOffset = info.blgSize.y * gMapCellLength + radius;
			Vec2f startPos = blgPos + Vec2f( -radius , radius );
			if ( tryPlaceUnitInternal( unit , startPos , Vec2f(0,1) , maxOffset ) )
				return true;
		}

		return false;
	}

	bool World::tryPlaceUnitInternal( Unit* unit , Vec2f const& startPos, Vec2f const& offsetDir, float maxOffset )
	{
		float totalOffset = 0;
		Vec2f testPos = startPos;
		float radius  = unit->getColRadius();

		CollisionLayer layer = unit->checkFlag( EF_FLY ) ? CL_FLY : CL_WALK;
		while( 1 )
		{
			float offset;
			if ( !getCollisionMgr().tryPlaceObject( unit->getColBody() , offsetDir , testPos , offset ) )
				break;

			totalOffset += offset;
			if ( totalOffset > maxOffset  )
				break;

			if ( getMap().checkCollision( testPos , radius , layer ) )
			{
				getEntityMgr().addEntity( unit );
				unit->setPos( testPos );
				return true;
			}
			totalOffset = ( int( totalOffset / gMapCellLength ) + 1 ) * gMapCellLength;
			testPos     = startPos + totalOffset * offsetDir;
		}

		return false;
	}



	Building* World::constructBuilding( ActorId type , Unit* builder  , Vec2f const& pos , bool useRes )
	{
		BuildingInfo const& info = Building::getBuildingInfo( type );

		PlayerInfo* pInfo = builder->getOwner();

		Vec2i mapPos;
		if ( !canBuild( type , builder , pos , mapPos , pInfo , true ) )
			return NULL;

		if ( pInfo )
		{
			if ( useRes )
			{
				if ( pInfo->gold < info.goldCast ||
					 pInfo->wood < info.woodCast )
				{
					return NULL;
				}

				pInfo->gold -= info.goldCast;
				pInfo->wood -= info.woodCast;
			}
		}

		Actor* actor = info.factory->create( type );
		Building* result = actor->cast< Building >();

		assert( result );
		//if ( result )
		{
			result->setOwner( pInfo );

			getMap().addBuiding( result , mapPos );
			getEntityMgr().addEntity( result );
		}
		return result;
	}

	bool World::produceUnit( ActorId type , PlayerInfo* pInfo , bool useRes )
	{
		assert( isUnit( type ) );

		UnitInfo const& info = Unit::getUnitInfo( type );

		if ( pInfo )
		{
			if ( useRes )
			{
				if ( info.goldCast > pInfo->gold )
					return false;
				if ( info.woodCast > pInfo->wood )
					return false;

				pInfo->gold -= info.goldCast;
				pInfo->wood -= info.woodCast;
			}

			pInfo->curPopulation += info.population;
		}
		return true;
	}

	bool World::canControl( PlayerInfo* player , Actor* actor )
	{
		if ( actor->getOwner() == player )
			return true;
		//FIXME
		return false;
	}

	PlayerRelationship World::getRelationship( Actor* actor1 , Actor* actor2 )
	{
		if ( actor1->getOwner() == actor2->getOwner() )
			return PR_TEAM;
		return PR_EMPTY;
	}

	bool WorldMap::testCollisionX( Vec2f const& mapPos , float colRadius , CollisionLayer layer , float& offset )
	{
		float testPos = mapPos.x;
		float otherPos = mapPos.y;

		float radiusCheck = colRadius / gMapCellLength;

		int   startOther = (int)floor( otherPos - radiusCheck );
		int   endOther   = (int)ceil( otherPos + radiusCheck );


		assert( startOther <= endOther );

		if ( offset >= 0 )
		{
			int   start = (int)( testPos + radiusCheck );
			int   end   = (int)( testPos + radiusCheck + (offset  / gMapCellLength) );

			assert( start <= end );

			for ( int nTest = start ; nTest <= end ; ++nTest )
			{
				for ( int other = startOther ; other < endOther ; ++other )
				{
					if ( !mMapData.checkRange( nTest , other ) )
						continue;

					if ( mMapData.getData( nTest , other ).blockMask[ layer ] )
					{
						float newOffset = ( nTest - testPos ) * gMapCellLength - colRadius;
						offset = newOffset ;
						return false;
					}
				}
			}
		}
		else
		{
			int   start = (int)( testPos - radiusCheck );
			int   end   = (int)( testPos - radiusCheck + (offset / gMapCellLength) );

			assert( start >= end );

			for ( int nTest = start ; nTest >= end ; --nTest )
			{
				for ( int other = startOther ; other < endOther ; ++other )
				{
					if ( !mMapData.checkRange( nTest , other ) )
						continue;

					if ( mMapData.getData( nTest , other ).blockMask[ layer ] )
					{
						float newOffset = ( ( testPos - nTest - 1 ) * gMapCellLength - colRadius  );
						offset = -newOffset;
						return false;
					}
				}
			}
		}

		return true;
	}

	bool WorldMap::testCollisionY( Vec2f const& mapPos , float colRadius , CollisionLayer layer , float& offset  )
	{
		float testPos = mapPos.y;
		float otherPos = mapPos.x;

		float radiusCheck = colRadius / gMapCellLength;

		int   startOther = (int)floor( otherPos - radiusCheck );
		int   endOther   = (int)ceil( otherPos + radiusCheck );

		assert( startOther <= endOther );

		if ( offset >= 0 )
		{
			int   start = (int)( testPos + radiusCheck );
			int   end   = (int)( testPos + radiusCheck + ( offset / gMapCellLength ) );

			assert( start <= end );

			for ( int nTest = start ; nTest <= end ; ++nTest )
			{
				for ( int other = startOther ; other < endOther ; ++other )
				{
					if ( !mMapData.checkRange( other , nTest ) )
						continue;

					if ( mMapData.getData( other , nTest ).blockMask[ layer ] )
					{
						float newOffset = ( nTest - testPos ) * gMapCellLength - colRadius ;
						offset = newOffset;
						return false;
					}
				}
			}
		}
		else
		{
			int   start = (int)( testPos - radiusCheck );
			int   end   = (int)( testPos - radiusCheck + (offset  / gMapCellLength) );

			assert( start >= end );

			for ( int nTest = start ; nTest >= end ; --nTest )
			{
				for ( int other = startOther ; other < endOther ; ++other )
				{
					if ( !mMapData.checkRange( other , nTest ) )
						continue;

					if ( mMapData.getData( other , nTest ).blockMask[ layer ] )
					{
						float newOffset = ( ( testPos - nTest - 1 ) * gMapCellLength - colRadius );
						offset = -newOffset;
						return false;
					}
				}
			}
		}
		return true;
	}

	bool WorldMap::canBuild( ActorId type , Vec2f const& pos , Vec2i& mapPos )
	{
		mapPos.setValue( int( pos.x / gMapCellLength ) , int( pos.y / gMapCellLength ) );

		if ( !mMapData.checkRange( mapPos.x , mapPos.y ) ) 
			return false;

		Vec2i size = Building::getBuildingInfo( type ).blgSize;
		Vec2i start = mapPos - size / 2;
		Vec2i end   = start + size;

		if ( !mMapData.checkRange( start.x , start.y ) )
			return false;
		if ( !mMapData.checkRange( end.x - 1 , end.y - 1 ) )
			return false;

		for ( int i = start.x ; i < end.x ; ++i )
		{
			for ( int j = start.y ; j < end.y ; ++j )
			{
				if ( mMapData.getData( i , j ).blockMask[ CL_BUILD ] )
					return false;
			}
		}

		return true;
	}

	void WorldMap::addBuiding( Building* building , Vec2i const& mapPos )
	{
		assert( mMapData.checkRange( mapPos.x , mapPos.y ) ) ;

		building->setPos( gMapCellLength * Vec2f( mapPos ) );

		Vec2i size = building->getBuildingInfo().blgSize;
		Vec2i start = mapPos - size / 2;
		Vec2i end   = start + size;

		for ( int i = start.x ; i < end.x ; ++i )
		{
			for ( int j = start.y ; j < end.y ; ++j )
			{
				assert( mMapData.checkRange( i , j ) );
				TileData& data = mMapData.getData( i , j );

				data.blockMask[ CL_WALK ]  |= BM_BUILDING;
				data.blockMask[ CL_BUILD ] |= BM_BUILDING;
				data.building = building;
			}
		}
	}

	void WorldMap::removeBuilding( Building* building )
	{

		Vec2i mapPos = Vec2i( building->getPos() / gMapCellLength );
		assert( mMapData.checkRange( mapPos.x , mapPos.y ) ) ;

		Vec2i size  = building->getBuildingInfo().blgSize;
		Vec2i start = mapPos - size / 2;
		Vec2i end   = start + size;

		for ( int i = start.x ; i < end.x ; ++i )
		{
			for ( int j = start.y ; j < end.y ; ++j )
			{
				assert( mMapData.checkRange( i , j ) );

				TileData& data = mMapData.getData( i , j );
				assert( data.building = building );

				data.blockMask[ CL_BUILD ] &= ~BM_BUILDING;
				data.blockMask[ CL_WALK  ] &= ~BM_BUILDING;
				data.building = NULL;
			}
		}
	}

	bool WorldMap::checkCollision( Unit* unit )
	{
		if ( unit->checkFlag( EF_FLY ))
			return checkCollision( unit->getPos() , unit->getColRadius() , CL_FLY );
		return checkCollision( unit->getPos() , unit->getColRadius() , CL_WALK );
	}

	bool WorldMap::checkCollision( Vec2f const& pos , float radius , CollisionLayer layer )
	{
		Vec2f mapPos = pos / gMapCellLength;
		float checkLen = radius / gMapCellLength ;
		Vec2i start( (int)floor( mapPos.x - checkLen ) , (int)floor( mapPos.y - checkLen ) );
		Vec2i end  ( (int)ceil( mapPos.x + checkLen ) , (int)ceil( mapPos.y + checkLen ) );

		for ( int i = start.x ; i < end.x ; ++i )
		{
			for ( int j = start.y ; j < end.y ; ++j )
			{
				if ( !mMapData.checkRange( i , j ) )
					continue;

				TileData& data = mMapData.getData( i , j );

				if ( data.blockMask[ layer ] )
					return false;
			}
		}
		return true;
	}

	Building* WorldMap::getBuilding( Vec2f const& wPos )
	{
		Vec2i mapPos = Vec2i( wPos / gMapCellLength );

		if ( !mMapData.checkRange( mapPos.x , mapPos.y ) )
			return NULL;
		return mMapData.getData( mapPos.x , mapPos.y ).building;
	}

	void WorldMap::setup( int cx , int cy )
	{
		mMapData.resize( cx , cy );
		memset( mMapData.getRawData() , 0 , cx * cy * sizeof( TileData ) );
	}

}//namespace TowerDefend
