#include "GreedySnakePCH.h"
#include "GreedySnakeScene.h"	

#include "RenderUtility.h"

namespace GreedySnake
{
	Scene::Scene( Mode& mode ) 
		:mMode( mode ) 
		,mLevel( this )
	{
		mIsOver = true;
		mMode.mScene = this;
	}

	void Scene::fireAction( ActionTrigger& trigger )
	{
		if ( !trigger.haveUpdateFrame() )
			return;

		for( int i = 0 ; i < mLevel.getSnakeNum() ; ++i )
		{
			SnakeInfo& info = mLevel.getSnakeInfo( i );
			trigger.setPort( info.id );
			fireSnakeAction( trigger );
		}
	}


	void Scene::fireSnakeAction( ActionTrigger& trigger )
	{
#define CHECK_MOVE_ACTION( ACT , DIR )\
	if ( trigger.detect( ACT )  )\
	{\
		changeSnakeMoveDir( trigger.getPort() , DIR );\
	}

		CHECK_MOVE_ACTION( ACT_GS_MOVE_E , DIR_EAST );
		CHECK_MOVE_ACTION( ACT_GS_MOVE_N , DIR_NORTH );
		CHECK_MOVE_ACTION( ACT_GS_MOVE_W , DIR_WEST );
		CHECK_MOVE_ACTION( ACT_GS_MOVE_S , DIR_SOUTH );

#undef CHECK_MOVE_ACTION
	}

	void Scene::productRandomFood( int type )
	{
		Level& level = getLevel();

		Vec2i mapSize = level.getMapSize();
		Vec2i pos;
		while( 1 )
		{
			pos.x = Global::RandomNet() % mapSize.x;
			pos.y = Global::RandomNet() % mapSize.y;

			int result;
			if ( !level.hitTest( pos , Level::eALL_MASK , result ) )
				break;
		}
		level.addFood( pos , type );
	}

	void Scene::drawSnake( Graphics2D& g , SnakeInfo& info , float dFrame )
	{
		float frac = ( info.curMoveCount + info.moveSpeed * dFrame ) / float( Level::MoveCount );
		int offset = int( BlockSize * frac );
		drawSnakeBody( g , *info.snake , Color::eYellow , offset );
	}

	void Scene::drawSnakeBody( Graphics2D& g , Snake& snake , int color , int offset )
	{
		assert( snake.getBodyLength() >= 2 );
		assert( offset >= 0 );

		//      base           
		//    _________
		//   |  _____  |  gap
		//   | |     | |
		//   | |     | |
		//   | |_____| |
		//   |_________|

		RenderUtility::setBrush( g , color );
		RenderUtility::setPen( g , color );

		Snake::Iterator iter = snake.getBodyIterator();

		Vec2i basePos;

		Snake::Body const& head = iter.getElement();
		basePos = BlockSize * head.pos + Vec2i( SnakeGap , SnakeGap );

		if ( offset  )
		{
			switch( head.dir )
			{
			case DIR_NORTH: 
				g.drawRect( basePos + Vec2i( 0 , -offset ) , Vec2i( SnakeWidth , offset ));
				break;
			case DIR_SOUTH:
				g.drawRect( basePos + Vec2i( 0 , SnakeWidth ) , Vec2i( SnakeWidth , offset ));
				break;
			case DIR_EAST:
				g.drawRect( basePos + Vec2i( SnakeWidth , 0 ) , Vec2i( offset , SnakeWidth ));
				break;
			case DIR_WEST:
				g.drawRect( basePos + Vec2i( -offset , 0 ) , Vec2i( offset , SnakeWidth ));
				break;
			}
		}

		g.drawRect( basePos , Vec2i( SnakeWidth , SnakeWidth ) );
		iter.goNext();

		while( 1 )
		{

			Snake::Body const& body = iter.getElement();

			iter.goNext();
			if ( !iter.haveMore() )
				break;

			basePos = BlockSize * body.pos + Vec2i( SnakeGap , SnakeGap );
			
			switch ( body.dir )
			{
			case DIR_SOUTH: 
				g.drawRect( basePos + Vec2i( 0 , SnakeWidth ) , Vec2i( SnakeWidth , 2 * SnakeGap ));
				break;
			case DIR_NORTH:
				g.drawRect( basePos + Vec2i( 0 , - 2 * SnakeGap ) , Vec2i( SnakeWidth , 2 * SnakeGap ));
				break;
			case DIR_WEST:
				g.drawRect( basePos + Vec2i( - 2 * SnakeGap , 0 ) , Vec2i( 2 * SnakeGap , SnakeWidth ));
				break;
			case DIR_EAST:
				g.drawRect( basePos + Vec2i( SnakeWidth , 0 ) , Vec2i( 2 * SnakeGap , SnakeWidth ));
				break;
			}

			g.drawRect( basePos , Vec2i( SnakeWidth , SnakeWidth ) );
		}

		Snake::Body const& tail = snake.getTail();
		basePos = BlockSize * tail.pos + Vec2i( SnakeGap , SnakeGap );

		if ( offset )
		{
			switch ( tail.dir )
			{
			case DIR_SOUTH: 
				g.drawRect( basePos + Vec2i( 0 , offset ) , Vec2i( SnakeWidth , BlockSize - offset ));
				break;
			case DIR_NORTH:
				g.drawRect( basePos + Vec2i( 0 , - 2 * SnakeGap ) , Vec2i( SnakeWidth , BlockSize - offset ));
				break;
			case DIR_WEST:
				g.drawRect( basePos + Vec2i( - 2 * SnakeGap , 0 ) , Vec2i( BlockSize - offset , SnakeWidth ));
				break;
			case DIR_EAST:
				g.drawRect( basePos + Vec2i( offset , 0 ) , Vec2i( BlockSize - offset , SnakeWidth ));
				break;
			}
		}
		else
		{
			switch ( tail.dir )
			{
			case DIR_SOUTH: 
				g.drawRect( basePos + Vec2i( 0 , SnakeWidth ) , Vec2i( SnakeWidth , 2 * SnakeGap ));
				break;
			case DIR_NORTH:
				g.drawRect( basePos + Vec2i( 0 , - 2 * SnakeGap ) , Vec2i( SnakeWidth , 2 * SnakeGap ));
				break;
			case DIR_WEST:
				g.drawRect( basePos + Vec2i( - 2 * SnakeGap , 0 ) , Vec2i( 2 * SnakeGap , SnakeWidth ));
				break;
			case DIR_EAST:
				g.drawRect( basePos + Vec2i( SnakeWidth , 0 ) , Vec2i( 2 * SnakeGap , SnakeWidth ));
				break;
			}
			g.drawRect( basePos , Vec2i( SnakeWidth , SnakeWidth ) );
		}
	}

	void Scene::render( Graphics2D& g , float dFrame )
	{


		Vec2i mapSize = mLevel.getMapSize();


		Vec2i rectSize = mLevel.getMapSize() * BlockSize;
		Vec2i offset = ( Global::getDrawEngine()->getScreenSize() - rectSize ) / 2 ;

		g.beginXForm();
		g.pushXForm();

		g.translateXForm( offset.x , offset.y );
		RenderUtility::setBrush( g , Color::eBlue );
		RenderUtility::setPen( g , Color::eBlue );
		g.drawRect( Vec2i(0,0) , rectSize );

		drawFood( g );

		for( int i = 0 ; i < mLevel.getSnakeNum() ; ++i )
		{
			drawSnake( g , mLevel.getSnakeInfo( i ) , dFrame );
		}


		g.popXForm();
		g.finishXForm();

		g.setTextColor( 0 , 255 , 125 );

		for( int i = 0 ; i < mapSize.x ; ++i )
		{
			for( int j = 0 ; j < mapSize.y ; ++j )
			{
				MapData const& data = mLevel.mMap.getData( i , j );

				FixString< 32 > str;
				str.format( "%u" , data.snake );
				g.drawText( BlockSize * Vec2i(i,j) + offset , str );
			}
		}
	}

	void Scene::tick()
	{
		if ( !mIsOver )
		{
			mMode.prevLevelTick();
			mLevel.tick();
			mMode.postLevelTick();
		}
	}

	void Scene::restart( bool beInit )
	{
		mIsOver = false;
		mMode.restart( beInit );
	}

	void Scene::onEatFood( SnakeInfo& info , FoodInfo& food )
	{
		mMode.onEatFood( info , food );
	}

	void Scene::onCollideSnake( SnakeInfo& snake , SnakeInfo& colSnake )
	{
		mMode.onCollideSnake( snake , colSnake );
	}

	void Scene::onCollideTerrain( SnakeInfo& snake , int type )
	{
		mMode.onCollideTerrain( snake , type );
	}

	void Scene::changeSnakeMoveDir( unsigned id , DirEnum dir )
	{
		mLevel.getSnakeInfo( id ).snake->changeMoveDir( dir );
	}

	void Scene::killSnake( unsigned id )
	{
		if ( !mLevel.addSnakeState( id , SS_DEAD ) )
			return;
	}

	void Scene::RenderVisitor::visit( FoodInfo const& info )
	{
		RenderUtility::setBrush( g , Color::eRed );
		g.drawCircle( BlockSize * info.pos + Vec2i( BlockSize / 2  , BlockSize / 2 ) , SnakeWidth / 2 );
	}

}//namespace GreedySnake
