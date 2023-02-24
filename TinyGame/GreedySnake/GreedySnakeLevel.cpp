#include "GreedySnakePCH.h"
#include "GreedySnakeLevel.h"

namespace GreedySnake
{

	void SnakeBody::reset( Vec2i const& pos , DirType dir , size_t length )
	{
		mIdxTail = 0;
		mIdxHead = length - 1;

		Elements.clear();
		Element body;
		body.pos = pos;
		body.dir = dir;
		Elements.resize( length , body );
	}


	void SnakeBody::moveStep( DirType moveDir )
	{
		Element& body = Elements[ mIdxTail ];

		Element const& head = getHead();
		body.dir = moveDir;
		body.pos = head.pos + getDirOffset( head.dir );

		mIdxHead = mIdxTail;
		++mIdxTail;
		if ( mIdxTail >= Elements.size() )
			mIdxTail = 0;
	}

	void SnakeBody::growBody( size_t num )
	{
		Element newBody = Elements[ mIdxTail ];
		Elements.insert( Elements.begin() + mIdxTail , num , newBody );

		//resolve case : [ Tail ] [][]....[][ Head ]
		if ( mIdxTail <= mIdxHead )
			mIdxHead += num;
	}

	SnakeBody::Element const* SnakeBody::hitTest( Vec2i const& pos )
	{
		int num = (int)Elements.size();
		for( int i = 0 ; i < num ; ++i  )
		{
			if ( pos == Elements[i].pos )
				return &Elements[i];
		}
		return NULL;
	}

	void SnakeBody::warpHeadPos( int w , int h )
	{
		Element& head = Elements[ mIdxHead ];
		if ( w )
		{
			if ( head.pos.x < 0 )
				head.pos.x += w;
			else if ( head.pos.x >= w )
				head.pos.x -= w;
		}
		if ( h )
		{
			if ( head.pos.y < 0 )
				head.pos.y += h;
			else if ( head.pos.y >= h )
				head.pos.y -= h;
		}
	}

	Level::Level( Listener* listener )
	{
		mListener = listener;
		mNumSnakePlay = 0;
		for( int i = 0 ; i < gMaxPlayerNum ; ++i )
		{
			Snake& snake = mSnakes[ i ];
			snake.id    = i;
		}
	}

	Snake* Level::addSnake( Vec2i const& pos , DirType dir , size_t length )
	{
		Snake& snake = mSnakes[ mNumSnakePlay ];

		snake.init( pos , dir , length );
		snake.moveCountAcc = 0;
		snake.moveSpeed    = DefaultMoveSpeed;
		snake.stateBit     = 0;
		++mNumSnakePlay;

		addSnakeMark( snake );

		return &snake;
	}

	Level::HitMask Level::hitTest( Vec2i const& pos , unsigned mask , int& hitResult )
	{
		MapTileData& tile = mMap.getData( pos.x , pos.y );
		if ( mask & eTERRAIN_MASK )
		{
			if ( tile.terrain )
			{
				hitResult = tile.terrain;
				return eTERRAIN_MASK;
			}
		}
		if ( mask & eSNAKE_MASK )
		{
			if ( tile.snakeMask )
			{
				hitResult = tile.snakeMask;
				return eSNAKE_MASK;
			}
		}
		if ( mask & eFOOD_MASK )
		{
			if( tile.itemMask )
			{
				for( FoodVec::iterator iter = mFoodVec.begin();
					iter != mFoodVec.end(); ++iter )
				{
					if( iter->pos == pos )
					{
						hitResult = iter->type;
						return eFOOD_MASK;
					}
				}
			}
		}
		return eNO_HIT_MASK;
	}

	void Level::setupMap(Vec2i const& size, MapBoundType type)
	{
		assert(mNumSnakePlay == 0);
		mMapBoundType = type;
		mMap.resize(size.x, size.y);
		MapTileData tile;
		tile.snakeMask = 0;
		tile.terrain = 0;
		tile.itemMask = 0;
		mMap.fillValue(tile);
	}

	void Level::addSnakeBodyElementMark( unsigned id , Vec2i const& pos )
	{
		if ( !mMap.checkRange( pos.x , pos.y ) )
			return ;
		mMap.getData( pos.x , pos.y ).snakeMask |= BIT( id );
	}

	void Level::removeSnakeBodyElementMark( unsigned id , Vec2i const& pos )
	{
		if ( !mMap.checkRange( pos.x , pos.y ) )
			return;
		mMap.getData( pos.x , pos.y  ).snakeMask &= ~BIT( id );
	}

	void Level::removeSnakeMark( Snake& snake )
	{
		for( size_t i = 0 ; i < snake.getBody().getLength() ; ++i )
		{
			SnakeBody::Element const& bodyElement = snake.getBody().getElementByIndex( i );
			removeSnakeBodyElementMark( snake.id , bodyElement.pos );
		}
	}
	void Level::addSnakeMark( Snake& snake )
	{
		for( unsigned i = 0 ; i < ( unsigned ) snake.getBody().getLength() ; ++i )
		{
			SnakeBody::Element const& bodyElement = snake.getBody().getElementByIndex( i );
			addSnakeBodyElementMark( snake.id , bodyElement.pos );
		}
	}

	void Level::removeAllSnake()
	{
		for( int i = 0 ; i < mNumSnakePlay ; ++i )
		{
			removeSnakeMark( mSnakes[i] );
		}
		mNumSnakePlay = 0;
	}


	void Level::tick()
	{
		Snake* moveableSnake[gMaxPlayerNum];
		int numMoveableSnake = 0;
		for( int i = 0; i < mNumSnakePlay; ++i )
		{
			Snake& snake = mSnakes[i];

			snake.frameMoveCount = -1;
			if( !snake.canMove() )
			{
				snake.moveCountAcc = 0;
			}
			else
			{
				int moveSpeed = snake.moveSpeed;
				if( snake.stateBit & SS_INC_SPEED )
					moveSpeed *= 2;
				else if( snake.stateBit & SS_DEC_SPEED )
					moveSpeed /= 2;

				snake.moveCountAcc += moveSpeed;
				if( snake.moveCountAcc > MoveCount )
				{
					snake.frameMoveCount = 0;
					moveableSnake[numMoveableSnake] = &snake;
					++numMoveableSnake;
				}
			}
		}

		while( numMoveableSnake )
		{
			for( int i = 0; i < numMoveableSnake; ++i )
			{
				Snake& snake = *moveableSnake[i];

				//Maybe change state after other snake collision
				if( snake.canMove() )
				{
					snake.moveCountAcc -= MoveCount;
					assert(snake.moveCountAcc >= 0);

					Vec2i tailPos = snake.getBody().getTail().pos;
					snake.getBody().moveStep(snake.getMoveDir());

					checkMapBoundCondition(snake);

					//Element of body can locate in same pos
					if( tailPos != snake.getBody().getTail().pos )
					{
						removeSnakeBodyElementMark(snake.id, tailPos);
					}
					addSnakeBodyElementMark(snake.id, snake.getBody().getHead().pos);

					if( !(snake.stateBit & SS_DEAD) )
					{
						detectMoveSnakeCollision(snake);
					}
				}

				if( snake.moveCountAcc < MoveCount || !snake.canMove() )
				{
					--numMoveableSnake;
					if( i != numMoveableSnake )
						moveableSnake[i] = moveableSnake[numMoveableSnake];
				}
			}
		}
	}

	void Level::checkMapBoundCondition(Snake &snake)
	{
		Vec2i headPos = snake.getBody().getHead().pos;

		switch( mMapBoundType )
		{
		case MapBoundType::Cliff:
			if( !mMap.checkRange(headPos.x, headPos.y) )
			{
				mListener->onCollideTerrain(snake, TT_SIDE);
			}
			break;
		case MapBoundType::WarpXY:
			snake.getBody().warpHeadPos(mMap.getSizeX(), mMap.getSizeY());
			break;
		case MapBoundType::WarpX:
			snake.getBody().warpHeadPos(mMap.getSizeX(), 0);
			if( !mMap.checkRange(headPos.x, headPos.y) )
			{
				mListener->onCollideTerrain(snake, TT_SIDE);
			}
			break;
		case MapBoundType::WarpY:
			snake.getBody().warpHeadPos(0, mMap.getSizeY());
			if( !mMap.checkRange(headPos.x, headPos.y) )
			{
				mListener->onCollideTerrain(snake, TT_SIDE);
			}
			break;
		}
	}

	void Level::addFood(Vec2i const& pos, int type)
	{
		assert(mMap.getData(pos.x, pos.y).itemMask == 0);
		FoodInfo info;
		info.pos  = pos;
		info.type = type;
		mMap.getData(pos.x, pos.y).itemMask = 1;
		mFoodVec.push_back( info );
	}

	void Level::removeAllFood()
	{
		for( FoodInfo const& info : mFoodVec)
		{
			mMap.getData(info.pos.x, info.pos.y).itemMask = 0;
		}
		mFoodVec.clear();
	}

	void Level::removeFood( Vec2i const& pos )
	{
		for( FoodVec::iterator iter = mFoodVec.begin();
			 iter != mFoodVec.end(); )
		{
			if( iter->pos == pos )
			{
				iter = mFoodVec.erase(iter);
				
				return;
			}
			else
			{
				++iter;
			}
		}
	}

	void Level::detectMoveSnakeCollision( Snake& snake )
	{
		auto const& head = snake.getBody().getHead();

		MapTileData& tile = mMap.getData( head.pos.x , head.pos.y );

		unsigned snakeMask = tile.snakeMask;
		if (snake.stateBit & SS_INVINCIBLE)
			snakeMask = 0;
		if ( snake.stateBit & SS_TRANSPARENT )
			snakeMask &= ~BIT(snake.id);
		
		if ( snakeMask )
		{
			for( unsigned i = 0 ; i < (unsigned)mNumSnakePlay ; ++i )
			{
				if( ( snakeMask & BIT(i) ) == 0 )
					continue;

				Snake& otherSnake = mSnakes[i];

				bool bCollide = true;
				if(  head.pos == otherSnake.getBody().getHead().pos )
				{
					if (&snake == &otherSnake)
					{
						bCollide = false;
					}
					else if( head.dir == InverseDir(otherSnake.getBody().getHead().dir) ||
					   snake.frameMoveCount == snake.frameMoveCount )
					{
						mListener->onCollideSnake(otherSnake, snake);
					}
				}
				else if( head.pos == otherSnake.getBody().getTail().pos )
				{
					if( snake.frameMoveCount > snake.frameMoveCount )
					{
						bCollide = false;
					}
				}

				if( bCollide )
				{
					mListener->onCollideSnake(snake, otherSnake);
				}
			}
		}
		else if ( tile.terrain )
		{
			mListener->onCollideTerrain( snake , tile.terrain );
		}
		else if ( tile.itemMask )
		{
			for( FoodVec::iterator iter = mFoodVec.begin();
				iter != mFoodVec.end() ; )
			{
				if ( iter->pos == head.pos )
				{
					mListener->onEatFood( snake , *iter );
					mMap.getData(iter->pos.x, iter->pos.y).itemMask = 0;
					iter = mFoodVec.erase( iter );
				}
				else
				{
					++iter;
				}
			}
		}
	}

	bool  Level::addSnakeState( unsigned id , SnakeState state )
	{
		Snake& info = getSnake( id );
		if ( info.stateBit & state )
			return false;

		switch( state )
		{
		case SS_DEAD: removeSnakeMark( info ); break;
		}
		info.stateBit |= state;
		return true;
	}

	bool Level::getMapPos( Vec2i const& pos , DirType dir , Vec2i& result )
	{
		assert( mMap.checkRange( pos.x , pos.y ) );
		result = pos + getDirOffset( dir );
		switch( mMapBoundType )
		{
		case MapBoundType::WarpXY:
			if ( result.x < 0 )
				result.x += mMap.getSizeX();
			else if ( result.x >= mMap.getSizeX() )
				result.x -= mMap.getSizeX();
			if ( result.y < 0 )
				result.y += mMap.getSizeY();
			else if ( result.y >= mMap.getSizeY() )
				result.y -= mMap.getSizeY();
			break;
		case MapBoundType::WarpX:
			if (result.y < 0 || result.y >= mMap.getSizeY())
				return false;
			if ( result.x < 0 )
				result.x += mMap.getSizeX();
			else if ( result.x >= mMap.getSizeX() )
				result.x -= mMap.getSizeX();
			break;
		case MapBoundType::WarpY:
			if (result.x < 0 || result.x >= mMap.getSizeX())
				return false;
			if ( result.y < 0 )
				result.y += mMap.getSizeY();
			else if ( result.y >= mMap.getSizeY() )
				result.y -= mMap.getSizeY();
			break;
		case MapBoundType::Cliff:
			if ( !isMapRange( result ) )
				return false;
			break;
		}
		return true;
	}

	void Snake::init(Vec2i const& pos, DirType dir, size_t length)
	{
		mMoveDir = dir;
		getBody().init(pos, dir, length);
	}

	bool Snake::changeMoveDir(DirType dir)
	{
		if( stateBit & SS_CONFUSE )
			dir = InverseDir(dir);

		if( (dir % 2) == (getBody().getHead().dir % 2) )
			return false;
		mMoveDir = dir;
		return true;
	}

}//namespace GreedySnake