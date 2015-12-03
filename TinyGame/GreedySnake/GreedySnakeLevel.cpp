#include "GreedySnakePCH.h"
#include "GreedySnakeLevel.h"

namespace GreedySnake
{

	void Snake::_reset( Vec2i const& pos , DirType dir , size_t length )
	{
		mMoveDir = dir;
		mIdxTail = 0;
		mIdxHead = length - 1;

		mBodies.clear();
		Body body;
		body.pos = pos;
		body.dir = dir;
		mBodies.resize( length , body );
	}

	bool  Snake::changeMoveDir( DirType dir )
	{
		if ( ( dir % 2 ) == ( getHead().dir % 2 ) )
			return false;
		mMoveDir = dir;
		return true;
	}

	void Snake::moveStep()
	{
		Body& body = mBodies[ mIdxTail ];

		Body const& head = getHead();
		body.dir = mMoveDir;
		body.pos = head.pos + getDirOffset( head.dir );

		mIdxHead = mIdxTail;
		++mIdxTail;
		if ( mIdxTail >= mBodies.size() )
			mIdxTail = 0;
	}

	void Snake::growBody( size_t num )
	{
		Body newBody = mBodies[ mIdxTail ];
		mBodies.insert( mBodies.begin() + mIdxTail , num , newBody );

		//solve this example
		//[ Tail ] [][]....[][ Head ]
		if ( mIdxTail <= mIdxHead )
			mIdxHead += num;
	}

	Snake::Body const* Snake::hitTest( Vec2i const& pos )
	{
		int num = (int)mBodies.size();
		for( int i = 0 ; i < num ; ++i  )
		{
			if ( pos == mBodies[i].pos )
				return &mBodies[i];
		}
		return NULL;
	}

	void Snake::warpHeadPos( int w , int h )
	{
		Body& head = mBodies[ mIdxHead ];
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
		mNumSnake = 0;
		for( int i = 0 ; i < gMaxPlayerNum ; ++i )
		{
			SnakeInfo& info = mSnakeInfo[ i ];
			info.id    = i;
			info.snake = NULL;
		}
	}

	SnakeInfo* Level::addSnake( Vec2i const& pos , DirType dir , size_t length )
	{
		SnakeInfo& info = mSnakeInfo[ mNumSnake ];
		if ( info.snake )
		{
			info.snake->_reset( pos , dir , length );
		}
		else
		{
			info.snake = new Snake( pos , dir , length );
		}
		info.snake->changeMoveDir( dir );
		info.curMoveCount = 0;
		info.moveSpeed    = DefaultMoveSpeed;
		info.stateBit     = 0;
		++mNumSnake;

		addSnakeMark( info );

		return &info;
	}

	Level::HitMask Level::hitTest( Vec2i const& pos , unsigned mask , int& hitResult )
	{
		MapData& data = mMap.getData( pos.x , pos.y );
		if ( mask & eTERRAIN_MASK )
		{
			if ( data.terrain )
			{
				hitResult = data.terrain;
				return eTERRAIN_MASK;
			}
		}
		if ( mask & eSNAKE_MASK )
		{
			if ( data.snake )
			{
				hitResult = data.snake;
				return eSNAKE_MASK;
			}
		}
		if ( mask & eFOOD_MASK )
		{
			for( FoodVec::iterator iter = mFoodVec.begin();
				iter != mFoodVec.end() ; ++iter )
			{
				if ( iter->pos == pos )
				{
					hitResult = iter->type;
					return eFOOD_MASK;
				}
			}

		}
		return eNO_HIT_MASK;
	}

	void Level::setupMap( int w , int h , MapType type )
	{
		assert( mNumSnake == 0 );
		mMapType = type;
		mMap.resize( w , h );
		MapData data;
		data.snake = 0;
		data.terrain = 0;
		mMap.fillValue( data );
	}


	void Level::addSnakeBodyMark( unsigned id , Vec2i const& pos )
	{
		if ( !mMap.checkRange( pos.x , pos.y ) )
			return ;
		mMap.getData( pos.x , pos.y ).snake |= BIT( id );
	}

	void Level::removeSnakeBodyMark( unsigned id , Vec2i const& pos )
	{
		if ( !mMap.checkRange( pos.x , pos.y ) )
			return;
		mMap.getData( pos.x , pos.y  ).snake &= ~BIT( id );
	}

	void Level::removeSnakeMark( SnakeInfo& info )
	{
		for( size_t i = 0 ; i < info.snake->getBodyLength() ; ++i )
		{
			Snake::Body const& body = info.snake->getBodyByIndex( i );
			removeSnakeBodyMark( info.id , body.pos );
		}
	}
	void Level::addSnakeMark( SnakeInfo& info )
	{
		for( unsigned i = 0 ; i < ( unsigned ) info.snake->getBodyLength() ; ++i )
		{
			Snake::Body const& body = info.snake->getBodyByIndex( i );
			addSnakeBodyMark( info.id , body.pos );
		}
	}

	void Level::removeAllSnake()
	{
		for( int i = 0 ; i < mNumSnake ; ++i )
		{
			removeSnakeMark( mSnakeInfo[i] );
		}
		mNumSnake = 0;
	}


	void Level::tick()
	{
		for( int i = 0 ; i < mNumSnake ; ++i )
		{
			SnakeInfo& info = mSnakeInfo[i];

			if ( info.stateBit & SS_DEAD )
				continue;

			info.curMoveCount += info.moveSpeed;
			while ( info.curMoveCount > MoveCount )
			{
				info.curMoveCount -= MoveCount;

				Vec2i tailPos = info.snake->getTail().pos;

				info.snake->moveStep();

				Vec2i headPos = info.snake->getHead().pos;
				switch( mMapType )
				{
				case eMAP_CLIFF:
					if ( !mMap.checkRange( headPos.x , headPos.y ) )
					{
						mListener->onCollideTerrain( info , TT_SIDE );
					}
					break;
				case eMAP_WARP_XY:
					info.snake->warpHeadPos( mMap.getSizeX() , mMap.getSizeY() );
					break;
				case eMAP_WARP_X:
					info.snake->warpHeadPos( mMap.getSizeX() , 0 );
					if ( !mMap.checkRange( headPos.x , headPos.y ) )
					{
						mListener->onCollideTerrain( info , TT_SIDE );
					}
					break;
				case eMAP_WARP_Y:
					info.snake->warpHeadPos( 0 , mMap.getSizeY() );
					if ( !mMap.checkRange( headPos.x , headPos.y ) )
					{
						mListener->onCollideTerrain( info , TT_SIDE );
					}
					break;
				}

				if ( tailPos != info.snake->getTail().pos )
				{
					removeSnakeBodyMark( info.id , tailPos );
				}

				if ( !( info.stateBit & SS_DEAD ) )
				{
					detectSnakeCollision( info );
					if ( !( info.stateBit & SS_DEAD ) )
						addSnakeBodyMark( info.id , info.snake->getHead().pos );
				}
			}
		}
	}

	void Level::addFood( Vec2i const& pos , int type )
	{
		FoodInfo info;
		info.pos  = pos;
		info.type = type;
		mFoodVec.push_back( info );
	}

	void Level::removeAllFood()
	{
		mFoodVec.clear();
	}

	void Level::removeFood( Vec2i const& pos )
	{
		for( FoodVec::iterator iter = mFoodVec.begin();
			 iter != mFoodVec.end(); )
		{
			if ( iter->pos == pos )
				iter = mFoodVec.erase( iter );
			else
				++iter;
		}
	}

	void Level::detectSnakeCollision( SnakeInfo& info )
	{
		Vec2i headPos = info.snake->getHead().pos;
		MapData& data = mMap.getData( headPos.x , headPos.y );

		if ( data.snake )
		{
			for( unsigned i = 0 ; i < (unsigned)mNumSnake ; ++i )
			{
				if ( data.snake & BIT( i ) )
					mListener->onCollideSnake( info , mSnakeInfo[i] );
			}
		}
		else if ( data.terrain )
		{
			mListener->onCollideTerrain( info , data.terrain );
		}
		else
		{
			Snake& snake = *info.snake;

			for( FoodVec::iterator iter = mFoodVec.begin();
				iter != mFoodVec.end() ; )
			{
				if ( iter->pos == snake.getHead().pos )
				{
					mListener->onEatFood( info , *iter );
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
		SnakeInfo& info = getSnakeInfo( id );
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
		switch( mMapType )
		{
		case eMAP_WARP_XY:
			if ( result.x < 0 )
				result.x += mMap.getSizeX();
			else if ( result.x >= mMap.getSizeX() )
				result.x -= mMap.getSizeX();
			if ( result.y < 0 )
				result.y += mMap.getSizeY();
			else if ( result.y >= mMap.getSizeY() )
				result.y -= mMap.getSizeY();
			break;
		case eMAP_WARP_X:
			if ( result.x < 0 )
				result.x += mMap.getSizeX();
			else if ( result.x >= mMap.getSizeX() )
				result.x -= mMap.getSizeX();
			break;
		case eMAP_WARP_Y:
			if ( result.y < 0 )
				result.y += mMap.getSizeY();
			else if ( result.y >= mMap.getSizeY() )
				result.y -= mMap.getSizeY();
			break;
		case eMAP_CLIFF:
			if ( !isVaildMapRange( result ) )
				return false;
			break;
		}
		return true;
	}

}//namespace GreedySnake