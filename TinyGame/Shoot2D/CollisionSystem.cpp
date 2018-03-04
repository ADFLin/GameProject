#include "CollisionSystem.h"

#include "Common.h"
#include "Object.h"
#include "ObjModel.h"

#include <cassert>

namespace Shoot2D
{

	bool CollisionSystem::isNeedTestCollision( Object& obj1 , Object& obj2 )
	{
		if ( &obj1 == &obj2 )
			return false;

		if ( obj1.getStats() != STATS_LIVE ||
			obj2.getStats() != STATS_LIVE )
			return false;

		bool isEmpty = obj1.getTeam() != obj2.getTeam();
		unsigned flag1 = obj1.getColFlag();
		unsigned flag2 = obj2.getColFlag();

		return testCollisionFlag(flag1,flag2,isEmpty);
	}

	bool CollisionSystem::testCollisionFlag( unsigned flag1, unsigned flag2 ,bool isEmpty )
	{
		if ( flag1 == COL_NO_NEED || flag2 == COL_NO_NEED )
			return false;

		if ( isEmpty )
		{
			return ( ( flag1 & COL_IS_BULLET  ) && ( flag2 & COL_EMPTY_BULLET  ) ) ||
				( ( flag1 & COL_IS_VEHICLE ) && ( flag2 & COL_EMPTY_VEHICLE ) ) ||
				( ( flag2 & COL_IS_BULLET  ) && ( flag1 & COL_EMPTY_BULLET  ) ) ||
				( ( flag2 & COL_IS_VEHICLE ) && ( flag1 & COL_EMPTY_VEHICLE ) ) ;
		}
		else 
		{
			return ( ( flag1 & COL_IS_BULLET  ) && ( flag2 & COL_FRIEND_BULLET  ) ) ||
				( ( flag1 & COL_IS_VEHICLE ) && ( flag2 & COL_FRIEND_VEHICLE ) ) ||
				( ( flag2 & COL_IS_BULLET  ) && ( flag1 & COL_FRIEND_BULLET  ) ) ||
				( ( flag2 & COL_IS_VEHICLE ) && ( flag1 & COL_FRIEND_VEHICLE ) );
		}
	}


	bool testColRectRect( Object& obj1 , ObjModel const& md1 , Object& obj2 , ObjModel const& md2 )
	{
		assert( md1.geomType == GEOM_RECT && md2.geomType == GEOM_RECT );
		return testInterection( 
			obj1.getPos() , Vec2D( md1.x , md1.y ) , 
			obj2.getPos() , Vec2D( md2.x , md2.y ) );
	}

	bool testColRectCircle( Object& obj1 , ObjModel const& md1 , Object& obj2 , ObjModel const& md2 )
	{
		assert( md1.geomType == GEOM_RECT && md2.geomType == GEOM_CIRCLE );
		return testInterection( 
			obj1.getPos() , Vec2D( md1.x , md1.y ) , 
			obj2.getPos() , md2.r );
	}

	bool testColCircleRect( Object& obj1 , ObjModel const& md1 , Object& obj2 , ObjModel const& md2 )
	{
		assert( md1.geomType == GEOM_CIRCLE && md2.geomType == GEOM_RECT );
		return testInterection(
			obj2.getPos() , Vec2D( md2.x , md2.y ) ,
			obj1.getPos() , md1.r );
	}

	bool testColCircleCircle( Object& obj1 , ObjModel const& md1 , Object& obj2 , ObjModel const& md2 )
	{
		assert( md1.geomType == GEOM_CIRCLE && md2.geomType == GEOM_CIRCLE );
		return testInterection( 
			obj1.getPos() , md1.r , 
			obj2.getPos() , md2.r );
	}

	typedef bool (*TestColFun)( Object&  , ObjModel const&  , Object&  , ObjModel const&  );
	static TestColFun sColMap[ NUM_GEOM_TYPE ][ NUM_GEOM_TYPE ] =
	{
		{ testColRectRect , testColRectCircle } ,
		{ testColCircleRect , testColCircleCircle } ,
	};

	bool CollisionSystem::testCollision( Object& obj1 , Object& obj2 )
	{
		ObjModelManger& manger = ObjModelManger::Get();

		ObjModel const& md1 = manger.getModel(obj1.getModelId());
		ObjModel const& md2 = manger.getModel(obj2.getModelId());

		TestColFun fun = sColMap[ md2.geomType ][ md1.geomType ];
		return (*fun)( obj1 , md1 , obj2 , md2 );
	}

	void CollisionSystem::processCollision( Object& obj1 , Object& obj2 )
	{
		if ( !isNeedTestCollision(obj1,obj2) )
			return;

		if ( testCollision(obj1,obj2) )
		{
			obj1.processCollision(obj2);
			obj2.processCollision(obj1);
		}
	}

}//namespace Shoot2D
