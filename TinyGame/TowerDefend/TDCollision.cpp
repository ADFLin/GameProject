#include "TDPCH.h"
#include "TDCollision.h"

#include "TDEntity.h"

namespace TowerDefend
{
	ColObject::ColObject( Entity& entity ) 
		:mOwner( entity )
		,mHaveTested( false )
	{

	}

	CollisionManager::CollisionManager()
	{
		mNumColObject = 0;
	}


	bool testCollisionOffset( Vector2 const& tPos , float tR , Vector2 const& pos , float r  , Vector2 const& dir , float offset[] )
	{
		Vector2 L = tPos - pos;

		float radius = tR + r;
		float radiusSqure = radius * radius; 

		float LSqure = L.length2();
		if ( LSqure > radiusSqure )
			return false;

		float Ldir = L.dot( dir );

		float temp = Ldir * Ldir + radiusSqure - LSqure;
		assert( temp >= 0 );

		float partA = Ldir;
		float partB = sqrt( temp ); 

		offset[0] = ( partA - partB );
		offset[1] = ( partA + partB );

		return true;
	}

	bool testCollisionFrac( Vector2 const& tPos , float tR , Vector2 const& pos , float r  , Vector2 const& offset , float frac[] )
	{
		Vector2 L = tPos - pos;

		float radius = tR + r;
		float radiusSqure = radius * radius; 

		float LSqure = L.length2();
		if ( LSqure > radiusSqure )
			return false;

		Vector2 dir = offset;
		float offsetDist = normalize( dir );

		float Ldir = L.dot( dir );

		float temp = Ldir * Ldir + radiusSqure - LSqure;
		assert( temp >= 0 );

		float partA = Ldir;
		float partB = sqrt( temp ); 

		frac[0] = ( partA - partB ) / offsetDist;
		frac[1] = ( partA + partB ) / offsetDist;

		return true;
	}

	bool testCollision( Vector2 const& tPos , float tRadius , Vector2 const& pos , float radius  , Vector2& offset )
	{
		Vector2 L = tPos - ( pos + offset );

		float sumRadius = tRadius + radius;
		float sumRadiusSqure = sumRadius * sumRadius; 

		float LSqure = L.length2();
		if ( LSqure > sumRadiusSqure )
			return false;

		Vector2 dir = offset;
		float offsetDist = normalize( dir );

		float Ldir = L.dot( dir );

		float temp = Ldir * Ldir + sumRadiusSqure - LSqure;
		assert( temp >= 0 );

		float frac_a = offsetDist + Ldir ;
		float frac_b = sqrt( temp );
		float frac = frac_a - frac_b;
		if ( frac < 0 )
		{
			//frac = frac_a + frac_b;
			frac = 0;
		}
		offset = frac * dir;
		return true;
	}

	ColObject* CollisionManager::testCollision( Vector2 const& pos , float radius , ColObject* skip , Vector2& offset )
	{
		Vector2 offR( radius , radius );

		Vector2 min = pos - offR;
		Vector2 max = pos + offR;

		if ( offset.x > 0 )
			max.x += offset.x;
		else
			min.x += offset.x;

		if ( offset.y > 0 )
			max.y += offset.y;
		else
			min.y += offset.y;

		unsigned hashValue[4];

		calcHashValue( min , max , hashValue );

		ColObject* colObj = NULL;

		clearTestedObj();

		for( int i = 0 ; i < 4 ; ++i )
		{
			if ( hashValue[i] == INVALID_HASH_VALUE )
				continue;

			Grid& grid = mGrids[ hashValue[i] ];

			for( ColObjList::iterator iter = grid.objs.begin();
				iter != grid.objs.end() ; ++iter )
			{
				ColObject* testObj = (*iter);

				if ( testObj == skip )
					continue;

				if ( !addTestedObj( *iter ) )
					continue;

				if ( TowerDefend::testCollision( testObj->getOwner().getPos() , testObj->getBoundRadius() , pos , radius , offset ) )
				{
					colObj = (*iter);
				}
			}
		}

		return colObj;
	}

	ColObject* CollisionManager::testCollision( ColObject& obj , Vector2& offset )
	{
		return testCollision( obj.getOwner().getPos() , obj.getBoundRadius() , &obj , offset );
	}

	void CollisionManager::testCollision( Vector2 const& min , Vector2 const& max , CollisionCallback const& callback )
	{
		unsigned hashValue[4];
		calcHashValue( min , max , hashValue );

		clearTestedObj();

		for( int i = 0 ; i < 4 ; ++i )
		{
			if ( hashValue[i] == INVALID_HASH_VALUE )
				continue;

			Grid& grid = mGrids[ hashValue[i] ];

			for( ColObjList::iterator iter = grid.objs.begin();
				iter != grid.objs.end() ; ++iter )
			{
				if ( !addTestedObj( *iter ) )
					continue;

				ColObject* tObj = *iter;

				Vector2 const& pos = tObj->getOwner().getPos();
				float radius = tObj->getBoundRadius();

				if ( pos.x + radius < min.x || pos.x - radius > max.x || 
					pos.y + radius < min.y || pos.y - radius > max.y )
					continue;

				if ( !callback( *tObj ) )
				{	
					return;
				}
			}
		}
	}

	ColObject* CollisionManager::getObject( Vector2 const& pos )
	{
		unsigned hashValue = getHash( pos.x , pos.y );

		clearTestedObj();

		Grid& grid = mGrids[ hashValue ];

		for( ColObjList::iterator iter = grid.objs.begin();
			iter != grid.objs.end() ; ++iter )
		{
			ColObject* obj = *iter;
			if ( !addTestedObj( obj ) )
				continue;

			Vector2 dif = obj->getOwner().getPos() - pos;
			if ( dif.length2() < obj->getBoundRadius() * obj->getBoundRadius() )
				return obj;
		}
		return NULL;
	}

	bool CollisionManager::tryPlaceObject( ColObject& obj , Vector2 const& offsetDir , Vector2& pos , float& totalOffset  )
	{
		Vector2 min = pos;
		Vector2 max = pos + Vector2( gColCellLength , gColCellLength );
		unsigned hashValue[4];
		calcHashValue( min , max , hashValue );

		clearTestedObj();

		totalOffset = 0;

		for( int i = 0 ; i < 4 ; ++i )
		{
			if ( hashValue[i] == INVALID_HASH_VALUE )
				continue;

			Grid& grid = mGrids[ hashValue[i] ];

			for( ColObjList::iterator iter = grid.objs.begin();
				iter != grid.objs.end() ; ++iter )
			{
				ColObject* obj = *iter;
				if ( !addTestedObj( obj ) )
					continue;
				float offset[2];
				if ( testCollisionOffset( 
					obj->getOwner().getPos() , obj->getBoundRadius() ,
					pos , obj->getBoundRadius() , offsetDir , offset ) )
				{
					assert( offset[1] >= 0 ); 
					totalOffset += offset[1];
					if ( totalOffset > gColCellLength )
						return false;
					pos += offset[1] * offsetDir;
				}
			}
		}
		return true;
	}

	void CollisionManager::removeObject( ColObject& obj )
	{
		for( int i = 0 ; i < 4 ; ++i )
		{
			unsigned hashVal = obj.mColHashValue[ i ];
			if ( hashVal == INVALID_HASH_VALUE )
				continue;

			mGrids[ hashVal ].objs.remove( &obj );
		}
		--mNumColObject;
	}



	void CollisionManager::addObject( ColObject& obj )
	{
		Vector2 const& pos = obj.getOwner().getPos();
		Vector2 offR( obj.getBoundRadius() , obj.getBoundRadius() );

		calcHashValue( pos - offR , pos + offR , obj.mColHashValue );

		for( int i = 0 ; i < 4 ; ++i )
		{
			unsigned hashVal = obj.mColHashValue[ i ];
			if ( hashVal == INVALID_HASH_VALUE )
				continue;
			mGrids[ hashVal ].objs.push_back( &obj );
		}

		++mNumColObject;
	}

	void CollisionManager::updateColObject( ColObject& obj )
	{
		unsigned newHash[4];

		Vector2 const& pos = obj.getOwner().getPos();
		Vector2 offR( obj.getBoundRadius() , obj.getBoundRadius() );
		calcHashValue( pos - offR , pos + offR , newHash );

		for( int i = 0 ; i < 4 ; ++i )
		{
			unsigned hashVal = obj.mColHashValue[ i ];
			if ( hashVal != newHash[i] )
			{
				if ( hashVal != INVALID_HASH_VALUE )
					mGrids[ hashVal ].objs.remove( &obj );
			}
		}

		for( int i = 0 ; i < 4 ; ++i )
		{
			unsigned hashVal = obj.mColHashValue[ i ];
			if ( hashVal != newHash[i] )
			{
				if ( newHash[i] != INVALID_HASH_VALUE )
					mGrids[ newHash[i] ].objs.push_back( &obj );
			}
			obj.mColHashValue[i] = newHash[i];
		}
	}

	void CollisionManager::calcHashValue( Vector2 const& min , Vector2 const& max , unsigned value[] )
	{
		assert( max.x - min.x <= gColCellLength &&
			max.y - min.y <= gColCellLength );

		value[0] = getHash( min.x , min.y );
		value[1] = getHash( max.x , min.y );
		value[2] = getHash( min.x , max.y );
		value[3] = getHash( max.x , max.y );

		for( int i = 0 ; i < 4 ; ++i )
		{
			for( int n = 0 ; n < i ; ++n )
			{
				if ( value[i] == value[n] )
				{
					value[i] = INVALID_HASH_VALUE;
					break;
				}
			}
		}
	}

	void CollisionManager::init( Vec2i const& size )
	{
		assert( mNumColObject == 0 );

		mGridSize = size / gColCellScale;

		if ( size.x % gColCellScale )
			mGridSize.x += 1;
		if ( size.y & gColCellScale )
			mGridSize.y += 1;

		mGrids.resize( mGridSize.x * mGridSize.y );
	}

	bool CollisionManager::addTestedObj( ColObject* obj )
	{
		if ( obj->getOwner().checkFlag( EF_DEAD | EF_DESTROY ) )
			return false;

		if ( obj->mHaveTested )
			return false;

		mTestedUnit.push_back( obj );
		obj->mHaveTested = true;
		return true;
	}

	void CollisionManager::clearTestedObj()
	{
		for( ColObjVec::iterator iter = mTestedUnit.begin();
			iter != mTestedUnit.end() ; ++iter )
		{
			(*iter)->mHaveTested = false;
		}

		mTestedUnit.clear();
	}

	unsigned CollisionManager::getHash( float x , float y )
	{
		int gx = int( x / gColCellLength );
		int gy = int( y / gColCellLength );

		if ( gx < 0 ) gx = 0;
		else if ( gx >= mGridSize.x ) gx = mGridSize.x - 1;

		if ( gy < 0 ) gy = 0;
		else if ( gy >= mGridSize.y ) gy = mGridSize.y - 1;

		return (unsigned)( gx + gy * mGridSize.x );
	}

}//namespace TowerDefend
