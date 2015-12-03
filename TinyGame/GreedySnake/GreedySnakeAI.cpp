#include "GreedySnakePCH.h"
#include "GreedySnakeAI.h"

namespace GreedySnake
{
	static TGrid2D< unsigned > sCheckMap;
	static unsigned sCheckCount;

	SnakeAI::SnakeAI( Scene& scene , unsigned id ) 
		:mScene( scene )
		,mInfo( scene.getLevel().getSnakeInfo( id ) )
	{

	}

	void SnakeAI::init( Scene& scene )
	{
		Vec2i size = scene.getLevel().getMapSize();
		sCheckMap.resize( size.x , size.y );
		sCheckMap.fillValue( 0 );
		sCheckCount = 0;
	}

	int SnakeAI::calcBoundedCount( Vec2i const& pos , DirType dir )
	{
		Vec2i testPos;
		if ( !mScene.getLevel().getMapPos( pos , dir , testPos ) )
			return 0;

		if ( sCheckCount == sCheckMap.getData( testPos.x , testPos.y ) )
			return 0;

		sCheckMap.getData( testPos.x , testPos.y ) = sCheckCount;

		if ( testCollision( testPos ) )
			return 0;

		int count = 1;
		if ( dir != DIR_WEST )
			count += calcBoundedCount( testPos , DIR_EAST );
		if ( dir != DIR_EAST )
			count += calcBoundedCount( testPos , DIR_WEST );
		if ( dir != DIR_SOUTH )
			count += calcBoundedCount( testPos , DIR_NORTH );
		if ( dir != DIR_NORTH )
			count += calcBoundedCount( testPos , DIR_SOUTH );

		return count;
	}

	void SnakeAI::incTestCount()
	{
		++sCheckCount;
		if ( sCheckCount == 0 )
		{
			sCheckMap.fillValue( 0 );
			sCheckCount = 1;
		}
	}

	bool SnakeAI::testCollision( Vec2i const& pos , DirType dir , Vec2i& resultPos )
	{
		Level& level = mScene.getLevel();
		if ( level.getMapPos( pos , dir , resultPos ) )
			return true;

		return testCollision( resultPos );
	}

	bool SnakeAI::testCollision( Vec2i const& pos )
	{
		Level& level = mScene.getLevel();

		assert( level.isVaildMapRange( pos ) );
		int hitResult = 0;
		switch( level.hitTest( pos , ColMask , hitResult ) )
		{
		case Level::eSNAKE_MASK:
			return true;
		case Level::eTERRAIN_MASK:
			if ( hitResult == TT_BLOCK )
				return true;
			break;
		}
		return false;
	}

	DirType SnakeAI::decideMoveDir()
	{
		Level& level = mScene.getLevel();
		Snake::Body const& head = mInfo.snake->getHead();
		DirType frontDir = head.dir;
		DirType leftDir  = ( head.dir + 3 ) % 4;
		DirType rightDir = ( head.dir + 1 ) % 4;

		Vec2i frontPos;
		if ( testCollision( head.pos , frontDir , frontPos ) )
		{
			Vec2i pos;
			if ( !testCollision( head.pos , leftDir , pos ) )
				return leftDir;

			if ( !testCollision( head.pos , rightDir , pos ) )
				return rightDir;
		}
		else
		{
			return frontDir;
		}

		int count[3];
		unsigned testCount[3];
		incTestCount();
		count[0] = calcBoundedCount( head.pos , frontDir );
		testCount[0] = sCheckCount;


		Vec2i pos;
		unsigned const mask = Level::eTERRAIN_MASK | Level::eTERRAIN_MASK;
		if ( !testCollision( head.pos , leftDir , pos ) )
		{
			unsigned c = sCheckMap.getData( pos.x , pos.y );
			if ( c == testCount[0] )
			{
				count[1] = count[0];
			}
			else
			{
				incTestCount();
				count[1] = calcBoundedCount( head.pos , leftDir );
			}
			testCount[1] = sCheckCount;
		}
		else
		{
			count[1] = 0;
			testCount[1] = 0;
		}

		if ( !testCollision( head.pos , rightDir , pos ) )
		{
			unsigned c = sCheckMap.getData( pos.x , pos.y );
			if ( c == testCount[0] )
			{
				count[2] = count[0];
			}
			else if ( c == testCount[1] )
			{
				count[2] = count[1];
			}
			else
			{
				incTestCount();
				count[2] = calcBoundedCount( head.pos , leftDir );
			}
			testCount[2] = sCheckCount;
		}
		else
		{
			count[2] = 0;
			testCount[2] = 0;
		}

		return 0;
	}

	bool SnakeAI::scanInput( bool beUpdateFrame )
	{
		if ( !beUpdateFrame )
			return false;

		if ( mInfo.curMoveCount + mInfo.moveSpeed < Level::MoveCount )
			return false;

		mMoveDir = decideMoveDir();

		return mMoveDir != mInfo.snake->getMoveDir();
	}

	bool SnakeAI::checkAction( ActionParam& param )
	{
		if ( param.port != mInfo.id )
			return false;
		return mMoveDir == param.act;
	}


}//namespace GreedySnake

