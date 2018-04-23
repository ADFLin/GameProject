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
			Snake& snake = mLevel.getSnake( i );
			trigger.setPort( snake.id );
			fireSnakeAction( trigger );
		}
	}


	void Scene::fireSnakeAction( ActionTrigger& trigger )
	{
		Snake& snake = mLevel.getSnake(trigger.getPort());
#define CHECK_MOVE_ACTION( ACT , DIR )\
	if ( trigger.detect( ACT )  )\
	{\
		snake.changeMoveDir( DIR );\
		LogMsgF("%d : %u change dir %d", mFrame, snake.id, (int)DIR);\
	}

		CHECK_MOVE_ACTION( ACT_GS_MOVE_E , DIR_EAST );
		CHECK_MOVE_ACTION( ACT_GS_MOVE_N , DIR_NORTH );
		CHECK_MOVE_ACTION( ACT_GS_MOVE_W , DIR_WEST );
		CHECK_MOVE_ACTION( ACT_GS_MOVE_S , DIR_SOUTH );
		CHECK_MOVE_ACTION( ACT_GS_CHANGE_DIR, DirType((snake.getMoveDir() + 1) % 4));

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

	void Scene::drawSnake( Graphics2D& g , Snake& snake , float dFrame )
	{
		if( snake.stateBit & SS_DEAD )
			return;

		int ColorMap[4] = { Color::eYellow  , Color::eOrange , Color::eWhite , Color::eBlack };
		float frac = ( snake.moveCountAcc + snake.moveSpeed * dFrame ) / float( Level::MoveCount );
		int offset = int( BlockSize * frac );
		drawSnakeBody( g , snake , ColorMap[ snake.id ] , offset );
	}

	void Scene::drawSnakeBody( Graphics2D& g , Snake& snake , int color , int offset )
	{
		SnakeBody& snakeBody = snake.getBody();

		assert( snakeBody.getLength() >= 2 );
		assert( offset >= 0 );

		//      base           
		//    _________
		//   |  _____  |  gap
		//   | |     | |
		//   | |     | |
		//   | |_____| |
		//   |_________|

		RenderUtility::SetBrush( g , color );
		RenderUtility::SetPen( g , color );

		SnakeBody::Iterator iter = snakeBody.createIterator();

		Vec2i basePos;

		SnakeBody::Element const& head = iter.getElement();
		basePos = BlockSize * head.pos + Vec2i( SnakeGap , SnakeGap );

		if ( offset  )
		{
			switch( head.dir )
			{
			case DIR_NORTH: g.drawRect( basePos + Vec2i( 0 , -offset ) , Vec2i( SnakeWidth , offset )); break;
			case DIR_SOUTH: g.drawRect( basePos + Vec2i( 0 , SnakeWidth ) , Vec2i( SnakeWidth , offset )); break;
			case DIR_EAST:  g.drawRect( basePos + Vec2i( SnakeWidth , 0 ) , Vec2i( offset , SnakeWidth )); break;
			case DIR_WEST:  g.drawRect( basePos + Vec2i( -offset , 0 ) , Vec2i( offset , SnakeWidth )); break;
			}
		}

		g.drawRect( basePos , Vec2i( SnakeWidth , SnakeWidth ) );
		iter.goNext();

		while( 1 )
		{

			SnakeBody::Element const& bodyElement = iter.getElement();

			iter.goNext();
			if ( !iter.haveMore() )
				break;

			basePos = BlockSize * bodyElement.pos + Vec2i( SnakeGap , SnakeGap );
			
			switch ( bodyElement.dir )
			{
			case DIR_SOUTH: g.drawRect( basePos + Vec2i( 0 , SnakeWidth ) , Vec2i( SnakeWidth , 2 * SnakeGap )); break;
			case DIR_NORTH: g.drawRect( basePos + Vec2i( 0 , - 2 * SnakeGap ) , Vec2i( SnakeWidth , 2 * SnakeGap )); break;
			case DIR_WEST:  g.drawRect( basePos + Vec2i( - 2 * SnakeGap , 0 ) , Vec2i( 2 * SnakeGap , SnakeWidth )); break;
			case DIR_EAST:  g.drawRect( basePos + Vec2i( SnakeWidth , 0 ) , Vec2i( 2 * SnakeGap , SnakeWidth )); break;
			}

			g.drawRect( basePos , Vec2i( SnakeWidth , SnakeWidth ) );
		}

		SnakeBody::Element const& tail = snakeBody.getTail();
		basePos = BlockSize * tail.pos + Vec2i( SnakeGap , SnakeGap );

		if ( offset )
		{
			switch ( tail.dir )
			{
			case DIR_SOUTH:g.drawRect( basePos + Vec2i( 0 , offset ) , Vec2i( SnakeWidth , BlockSize - offset )); break;
			case DIR_NORTH:g.drawRect( basePos + Vec2i( 0 , - 2 * SnakeGap ) , Vec2i( SnakeWidth , BlockSize - offset )); break;
			case DIR_WEST: g.drawRect( basePos + Vec2i( - 2 * SnakeGap , 0 ) , Vec2i( BlockSize - offset , SnakeWidth )); break;
			case DIR_EAST: g.drawRect( basePos + Vec2i( offset , 0 ) , Vec2i( BlockSize - offset , SnakeWidth )); break;
			}
		}
		else
		{
			switch ( tail.dir )
			{
			case DIR_SOUTH: g.drawRect( basePos + Vec2i( 0 , SnakeWidth ) , Vec2i( SnakeWidth , 2 * SnakeGap )); break;
			case DIR_NORTH: g.drawRect( basePos + Vec2i( 0 , - 2 * SnakeGap ) , Vec2i( SnakeWidth , 2 * SnakeGap )); break;
			case DIR_WEST:  g.drawRect( basePos + Vec2i( - 2 * SnakeGap , 0 ) , Vec2i( 2 * SnakeGap , SnakeWidth )); break;
			case DIR_EAST:  g.drawRect( basePos + Vec2i( SnakeWidth , 0 ) , Vec2i( 2 * SnakeGap , SnakeWidth )); break;
			}
			g.drawRect( basePos , Vec2i( SnakeWidth , SnakeWidth ) );
		}
	}

	void Scene::drawFood(Graphics2D& g)
	{
		auto fun = [&g](FoodInfo const& info)
		{
			switch( info.type )
			{
			case FOOD_GROW:
				RenderUtility::SetBrush(g, Color::eRed);
				break;
			case FOOD_SPEED_UP:
				RenderUtility::SetBrush(g, Color::eGreen);
				break;
			case FOOD_SPEED_SLOW:
				RenderUtility::SetBrush(g, Color::ePurple);
				break;
			case FOOD_CONFUSED:
				RenderUtility::SetBrush(g, Color::eGray);
				break;
			}
			g.drawCircle(BlockSize * info.pos + Vec2i(BlockSize / 2, BlockSize / 2), SnakeWidth / 2);
		};
		getLevel().visitFood(fun);
	}

	void Scene::render(Graphics2D& g, float dFrame)
	{
		Vec2i mapSize = mLevel.getMapSize();
		Vec2i rectSize = mLevel.getMapSize() * BlockSize;
		Vec2i offset = ( Global::getDrawEngine()->getScreenSize() - rectSize ) / 2 ;

		g.beginXForm();
		g.pushXForm();

		g.translateXForm( offset.x , offset.y );
		RenderUtility::SetBrush( g , Color::eBlue );
		RenderUtility::SetPen( g , Color::eBlue );
		g.drawRect( Vec2i(0,0) , rectSize );

		drawFood( g );

		for( int i = 0 ; i < mLevel.getSnakeNum() ; ++i )
		{
			drawSnake( g , mLevel.getSnake( i ) , dFrame );
		}


		g.popXForm();
		g.finishXForm();

		g.setTextColor(Color3ub(0 , 255 , 125) );

		for( int i = 0 ; i < mapSize.x ; ++i )
		{
			for( int j = 0 ; j < mapSize.y ; ++j )
			{
				MapTileData const& tile = mLevel.mMap.getData( i , j );

				FixString< 32 > str;
				str.format( "%u" , tile.snakeMask );
				//g.drawText( BlockSize * Vec2i(i,j) + offset , Vec2i( BlockSize , BlockSize) , str );
				g.drawText(BlockSize * Vec2i(i, j) + offset + Vec2i( 5 , 5 ) , str);
			}
		}
	}

	void Scene::tick()
	{
		++mFrame;
		if ( !mIsOver )
		{
			mMode.prevLevelTick();
			mLevel.tick();
			mMode.postLevelTick();
		}
	}

	void Scene::restart( bool beInit )
	{
		mFrame = 0;
		mIsOver = false;
		mMode.restart( beInit );
	}

	void Scene::onEatFood( Snake& snake , FoodInfo& food )
	{
		mMode.onEatFood( snake , food );
	}

	void Scene::onCollideSnake( Snake& snake , Snake& colSnake )
	{
		mMode.onCollideSnake( snake , colSnake );
	}

	void Scene::onCollideTerrain( Snake& snake , int type )
	{
		mMode.onCollideTerrain( snake , type );
	}


	void Scene::killSnake( unsigned id )
	{
		if ( !mLevel.addSnakeState( id , SS_DEAD ) )
			return;
	}


	void Mode::applyDefaultEffect(Snake& snake, FoodInfo const& food)
	{
		switch( food.type )
		{
		case FOOD_GROW:
			snake.getBody().growBody();
			break;
		case FOOD_SPEED_UP:
			snake.stateBit ^= SS_INC_SPEED;
			break;
		case FOOD_SPEED_SLOW:
			snake.stateBit ^= SS_DEC_SPEED;
			break;
		case FOOD_CONFUSED:
			snake.stateBit ^= SS_CONFUSE;
			break;
		}
	}

}//namespace GreedySnake
