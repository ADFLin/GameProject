#include "Collision.h"

#include "Object.h"

#include <algorithm>


ColBody::ColBody()
	:mOffset( 0,0 )
	,halfSize( 0,0 )
{
	idxCell = -1;
	typeMask = 0;
	colMask = 0;
	bUpdateSize = true;
}

void ColBody::updateCache()
{
	cachePos = mOffset + object->getPos();
	boundBox.min = cachePos - halfSize;
	boundBox.max = cachePos + halfSize;
}

CollisionManager::CollisionManager() 
	:mTerrain( NULL )
	,mCellLength( 0 )
{

}

void CollisionManager::setup( float width , float height  , float cellLength  )
{
	int cx = int ( width / cellLength - 1 ) + 1;
	int cy = int ( height / cellLength - 1 ) + 1;
	mCellMap.resize( cx , cy );
	mCellLength = cellLength;
}

bool CollisionManager::testCollision( ColInfo& info , Vec2f const& offset , ColBody& body , unsigned colMaskReplace )
{
	unsigned colMask = ( colMaskReplace ) ? colMaskReplace : body.colMask;

	BoundBox bBox;
	bBox.min = body.boundBox.min + offset;
	bBox.max = body.boundBox.max + offset;

	if ( colMask & COL_TERRAIN )
	{
		Tile* tile = testTerrainCollision( bBox , body.typeMask );
		if ( tile )
		{
			info.isTerrain = true;
			info.tile = tile;
			return true;
		}
	}

	if ( colMask & COL_OBJECT )
	{
		//Test Global Object
		for (ColBody& bodyTest : mGlobalBodies)
		{

			if ( &bodyTest == &body )
				continue;

			if ( ( colMask & bodyTest.typeMask ) == 0 )
				continue;

			if ( !bBox.isIntersect( bodyTest.boundBox ) )
				continue;

			info.isTerrain = false;
			info.body = &bodyTest;

			return true;
		}


		int xMin , xMax , yMin , yMax;
		//
		if ( body.idxCell >= 0 )
		{
			int cx , cy;
			calcCellPos( body.cachePos + offset , cx , cy );
			xMin = std::max( cx - 1 , 0 );
			xMax = std::min( cx + 1 , mCellMap.getSizeX() - 1 );
			yMin = std::max( cy - 1 , 0 );
			yMax = std::min( cy + 1 , mCellMap.getSizeY() - 1 );
		}
		else
		{
			xMin = Math::Clamp( Math::FloorToInt( bBox.min.x / mCellLength ) - 1 , 0 , mCellMap.getSizeX() - 1 );
			xMax = Math::Clamp( Math::FloorToInt( bBox.max.x / mCellLength ) + 1 , 0 , mCellMap.getSizeX() - 1 );
			yMin = Math::Clamp( Math::FloorToInt( bBox.min.y / mCellLength ) - 1 , 0 , mCellMap.getSizeY() - 1 );
			yMax = Math::Clamp( Math::FloorToInt( bBox.max.y / mCellLength ) + 1 , 0 , mCellMap.getSizeY() - 1 );
		}

		for( int cy = yMin ; cy <= yMax ; ++cy )
		{
			for( int cx = xMin ; cx <= xMax ; ++cx )
			{
				assert( mCellMap.checkRange( cx  , cy ) );

				int idxCell = mCellMap.toIndex( cx  , cy );

				Cell& cell = mCellMap[ idxCell ];

				CellBodyList& bodyList = cell.bodies;

				for ( ColBody& bodyTest : bodyList )
				{
					if ( &bodyTest == &body )
						continue;

					if ( ( colMask & bodyTest.typeMask ) == 0 )
						continue;

					if ( !bBox.isIntersect( bodyTest.boundBox ) )
						continue;

					info.isTerrain = false;
					info.body = &bodyTest;

					return true;
				}
			}
		}
	}


	return false;
}

bool CollisionManager::checkCollision( ColBody& body )
{
	bool result = false;

	if ( body.colMask & COL_TERRAIN )
	{
		Tile* tile = testTerrainCollision( body.boundBox , body.typeMask );

		if ( tile )
		{
			body.object->onTileCollision( body , *tile );
			result = true;
		}
	}

	if ( body.colMask & COL_OBJECT )
	{
		for (ColBody& bodyTest : mGlobalBodies)
		{
			if ( &bodyTest == &body )
				continue;

			unsigned mask = body.colMask & bodyTest.typeMask;

			if ( mask == 0 )
				continue;

			if ( !body.boundBox.isIntersect( bodyTest.boundBox ) )
				continue;

			body.object->onBodyCollision( body , bodyTest );
			result = true;
		}

		int xMin , xMax , yMin , yMax;
		//
		if ( body.idxCell >= 0 )
		{
			int cx , cy;
			calcCellPos( body.cachePos , cx , cy );
			xMin = std::max( cx - 1 , 0 );
			xMax = std::min( cx + 1 , mCellMap.getSizeX() - 1 );
			yMin = std::max( cy - 1 , 0 );
			yMax = std::min( cy + 1 , mCellMap.getSizeY() - 1 );
		}
		else
		{
			BoundBox const& bBox = body.boundBox;
			xMin = Math::Clamp( Math::FloorToInt( bBox.min.x / mCellLength ) - 1 , 0 , mCellMap.getSizeX() - 1 );
			xMax = Math::Clamp( Math::FloorToInt( bBox.max.x / mCellLength ) + 1 , 0 , mCellMap.getSizeX() - 1 );
			yMin = Math::Clamp( Math::FloorToInt( bBox.min.y / mCellLength ) - 1 , 0 , mCellMap.getSizeY() - 1 );
			yMax = Math::Clamp( Math::FloorToInt( bBox.max.y / mCellLength ) + 1 , 0 , mCellMap.getSizeY() - 1 );
		}

		for( int cy = yMin ; cy <= yMax ; ++cy )
		{
			for( int cx = xMin ; cx <= xMax ; ++cx )
			{
				assert( mCellMap.checkRange( cx  , cy  ) );

				int idxCell = mCellMap.toIndex( cx , cy );

				Cell& cell = mCellMap[ idxCell ];

				for (ColBody& bodyTest : cell.bodies)
				{
					if ( &bodyTest == &body )
						continue;

					unsigned mask = body.colMask & bodyTest.typeMask;

					if ( mask == 0 )
						continue;

					if ( !body.boundBox.isIntersect( bodyTest.boundBox ) )
						continue;

					body.object->onBodyCollision( body , bodyTest );
					result = true;
				}
			}
		}
	}

	return result;
}


void CollisionManager::addBody( LevelObject& obj , ColBody& body )
{
	assert( body.idxCell = -1 );
	body.object = &obj;
	body.updateCache();

	float halfLen = mCellLength / 2;
	if ( body.halfSize.x > halfLen || body.halfSize.y > halfLen )
	{
		body.idxCell = IdxGlobalCell;
		mGlobalBodies.push_back( body );
	}
	else
	{
		int cx , cy;
		calcCellPos( body.cachePos , cx , cy );
		int idxCell = mCellMap.toIndex( cx , cy );

		body.idxCell = idxCell;
		mCellMap[ idxCell ].bodies.push_back( body );
	}

	body.bUpdateSize = false;
	mBodies.push_back( body );
	
}

void CollisionManager::removeBody( ColBody& body )
{
	body.idxCell = -1;
	body.cellHook.unlink();
	body.managerHook.unlink();
}

void CollisionManager::updateBody( ColBody& body )
{
	body.updateCache();

	if ( updateBodySize( body ) )
		return;

	if ( body.idxCell != IdxGlobalCell )
	{
		int cx , cy;
		calcCellPos( body.cachePos , cx , cy );

		int idxCell = mCellMap.toIndex( cx , cy );
		if ( idxCell != body.idxCell )
		{
			body.cellHook.unlink();
			mCellMap[ idxCell ].bodies.push_back( body );
			body.idxCell = idxCell;
		}
	}
}

bool CollisionManager::updateBodySize( ColBody& body )
{
	assert( body.idxCell != -1 );

	if ( !body.bUpdateSize )
		return false;

	body.bUpdateSize = false;
	float halfLen = mCellLength / 2;
	bool bLargeSize = body.halfSize.x > halfLen || body.halfSize.y > halfLen;

	if ( body.idxCell == IdxGlobalCell )
	{
		if ( !bLargeSize )
		{
			body.cellHook.unlink();

			int cx , cy;
			calcCellPos( body.cachePos , cx , cy );
			int idxCell = mCellMap.toIndex( cx , cy );

			body.idxCell = idxCell;
			mCellMap[ idxCell ].bodies.push_back( body );
			return true;
		}
	}
	else
	{
		if ( bLargeSize )
		{
			body.cellHook.unlink();
			body.idxCell = IdxGlobalCell;
			mGlobalBodies.push_back( body );
			return true;
		}
	}
	return false;
}

void CollisionManager::update()
{
	for(ColBody& body : mBodies)
	{
		updateBody( body );
	}

	for (ColBody& body : mBodies)
	{
		checkCollision( body );
	}
}

void CollisionManager::calcCellPos( Vec2f const& pos , int& cx , int& cy )
{
	cx = Math::Clamp( int( pos.x / mCellLength ) , 0 , mCellMap.getSizeX() - 1 );
	cy = Math::Clamp( int( pos.y / mCellLength ) , 0 , mCellMap.getSizeY() - 1 );
}

Tile* CollisionManager::rayTerrainTest( Vec2f const& from , Vec2f const& to , unsigned typeMask  )
{
	Vec2i tpFrom = Vec2i( Math::Floor( from.x / BLOCK_SIZE ) , Math::Floor( from.y / BLOCK_SIZE ) );

	Tile* out;

	if ( out = rayBlockTest( tpFrom , from , to , typeMask ) )
		return out;

	Vec2i tpCur  = tpFrom;
	Vec2i tpTo   = Vec2i( Math::Floor( to.x / BLOCK_SIZE ) , Math::Floor( to.y / BLOCK_SIZE ) );
	Vec2i tpDif  = tpTo - tpFrom;

	if ( tpDif.x == 0 )
	{
		int delta = tpDif.y > 0 ? 1 : -1;
		while( tpCur.y != tpTo.y )
		{
			tpCur.y += delta;
			if ( out = rayBlockTest( tpCur , from , to , typeMask ) )
				return out;	
		}
	}
	else if ( tpDif.y == 0 )
	{
		int delta = tpDif.x > 0 ? 1 : -1;
		while( tpCur.x != tpTo.x )
		{
			tpCur.x += delta;
			if ( out = rayBlockTest( tpCur , from , to , typeMask ) )
				return out;	
		}
	}
	else
	{
		Vec2f frac = from / float( BLOCK_SIZE ) - Vec2f( tpFrom );
		Vec2f dif = to - from;
		float slopeFactor = dif.y / dif.x;
		if ( slopeFactor < 0 )
			slopeFactor = -slopeFactor;

		int deltaX = -1;
		if ( tpDif.x > 0 )
		{
			frac.x = 1 - frac.x;
			deltaX = 1;
		}

		int deltaY = -1;
		if ( tpDif.y > 0 )
		{
			frac.y = 1 - frac.y;
			deltaY = 1;
		}

		for(;;)
		{
			float yOff = frac.x * slopeFactor;

			if ( frac.y > yOff )
			{
				frac.y -= yOff;
				frac.x = 1;
				tpCur.x += deltaX;
			}
			else
			{
				frac.x -= frac.y / slopeFactor;
				frac.y = 1;
				tpCur.y += deltaY;
			}

			if ( out = rayBlockTest( tpCur , from , to , typeMask ) )
				return out;

			if ( tpCur.x == tpTo.x && tpCur.y == tpTo.y )
				break;
		}
	}
	return NULL;
}

Tile* CollisionManager::rayBlockTest( Vec2i const& tPos , Vec2f const& from , Vec2f const& to , unsigned typeMask )
{
	if ( !mTerrain->checkRange( tPos.x , tPos.y ) )
		return NULL;

	Tile& tile = mTerrain->getData( tPos.x , tPos.y );
	Block* block = Block::Get( tile.id );

	if ( ( block->getColMask() & typeMask ) == 0 )
		return NULL;

	if ( block->checkFlag( BF_NONSIMPLE ) )
	{
		if ( !block->rayTest( tile , from , to ) )
			return NULL;
	}

	return &tile;
}

Tile* CollisionManager::testTerrainCollision( BoundBox const& bBox , unsigned typeMask )
{
	TileMap& terrain = *mTerrain;

	int xMin = Math::Clamp( Math::FloorToInt( bBox.min.x / BLOCK_SIZE ) , 0 , terrain.getSizeX() - 1 );
	int xMax = Math::Clamp( Math::FloorToInt( bBox.max.x / BLOCK_SIZE ) , 0 , terrain.getSizeX() - 1 );
	int yMin = Math::Clamp( Math::FloorToInt( bBox.min.y / BLOCK_SIZE ) , 0 , terrain.getSizeY() - 1 );
	int yMax = Math::Clamp( Math::FloorToInt( bBox.max.y / BLOCK_SIZE ) , 0 , terrain.getSizeY() - 1 );

	for( int x = xMin; x <= xMax ; ++x )
	{
		for(int y = yMin; y <= yMax; ++y  )
		{
			Tile& tile = terrain.getData( x , y );
			Block* block = Block::Get( tile.id );

			if ( ( block->getColMask() & typeMask ) == 0 )
				continue;

			BoundBox bBoxOther;
			bBoxOther.min = tile.pos;
			bBoxOther.max = tile.pos + Vec2f(BLOCK_SIZE,BLOCK_SIZE);

			if( !bBox.isIntersect(bBoxOther) )
				continue;

			if ( block->checkFlag( BF_NONSIMPLE ) )
			{
				if ( !block->testIntersect( tile , bBox ) )
					continue;
			}

			return &tile;
		}
	}
	return NULL;
}

void CollisionManager::findBody( BoundBox const& bBox , unsigned colMask , ColBodyVec& out )
{
	visitBodies(bBox.min, bBox.max, 
		[&bBox, colMask, &out](ColBody& bodyTest)
		{
			if ((colMask & bodyTest.typeMask) == 0)
				return false;

			if (!bBox.isInside(bodyTest.cachePos))
				return false;

			out.push_back(&bodyTest);
			return false;
		}
	);
}
