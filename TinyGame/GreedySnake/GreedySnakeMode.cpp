#include "GreedySnakePCH.h"
#include "GreedySnakeMode.h"

#include "GamePlayer.h"

namespace GreedySnake
{
	void SurvivalMode::restart( bool beInit )
	{
		Level& level = getScene().getLevel();
		if ( beInit )
		{
			level.setupMap( 21 , 21 , Level::eMAP_CLIFF );
		}
		else
		{
			level.removeAllFood();
			level.removeAllSnake();
		}

		SnakeInfo* snakeInfo = level.addSnake( Vec2i(10,10) , 0 , 5 );
		getScene().productRandomFood( FOOD_GROW );
	}

	void SurvivalMode::setupLevel( IPlayerManager& playerManager )
	{
		Level& level = getScene().getLevel();

		for( IPlayerManager::Iterator iter = playerManager.getIterator(); 
			 iter.haveMore() ; iter.goNext() )
		{
			GamePlayer* player = iter.getElement();
			if( player->getType() == PT_SPECTATORS )
				continue;

			player->getInfo().actionPort = 0;
		}
	}

	void SurvivalMode::onEatFood( SnakeInfo& info , FoodInfo& food )
	{
		info.snake->growBody();
		getScene().productRandomFood( FOOD_GROW );
	}

	void SurvivalMode::onCollideSnake( SnakeInfo& snake , SnakeInfo& colSnake )
	{
		snake.stateBit |= SS_DEAD;
		getScene().setOver();
	}
	void SurvivalMode::onCollideTerrain( SnakeInfo& snake , int type )
	{
		snake.stateBit |= SS_DEAD;
		getScene().setOver();
	}


	void BattleMode::restart( bool beInit )
	{
		Level& level = getScene().getLevel();
		if ( beInit )
		{
			level.setupMap( 21 , 21 , Level::eMAP_CLIFF );
			std::fill_n( mWinRound , gMaxPlayerNum , 0 );
		}
		else
		{
			level.removeAllFood();
			level.removeAllSnake();
		}

		int offset = 5;
		Vec2i const spawnPos[] = 
		{ 
			Vec2i( 10 - offset , 10 - offset ) ,
			Vec2i( 10 + offset , 10 + offset ) , 
			Vec2i( 10 - offset , 10 + offset ) , 
			Vec2i( 10 + offset , 10 - offset ) , 
		};
		for( unsigned i = 0 ; i < mNumPlayer ; ++i )
		{
			SnakeInfo* snakeInfo = level.addSnake( spawnPos[i] , DirType(i) , 3 );
		}

		mNumAlivePlayer = mNumPlayer;
	}

	void BattleMode::setupLevel( IPlayerManager& playerManager )
	{
		mNumPlayer = 0;
		for( IPlayerManager::Iterator iter = playerManager.getIterator(); 
			iter.haveMore() ; iter.goNext() )
		{
			GamePlayer* player = iter.getElement();
			if( player->getType() == PT_SPECTATORS )
				continue;

			player->getInfo().actionPort = mNumPlayer;
			++mNumPlayer;
		}
	}

	void BattleMode::onCollideSnake( SnakeInfo& snake , SnakeInfo& colSnake )
	{
		killSnake(snake);

	}

	void BattleMode::onCollideTerrain( SnakeInfo& snake , int type )
	{
		killSnake( snake );
	}

	void BattleMode::killSnake( SnakeInfo &snake )
	{
		Level& level = getScene().getLevel();

		getScene().killSnake( snake.id );
		--mNumAlivePlayer;

		if ( mNumAlivePlayer == 1 )
		{
			getScene().setOver();

			for( unsigned i = 0 ; i < (unsigned)level.getSnakeNum(); ++i )
			{
				SnakeInfo& info = level.getSnakeInfo( i );
				if ( info.stateBit & SS_DEAD )
					continue;

				mWinRound[ info.id ] += 1;
				break;
			}
		}
	}

	void BattleMode::onEatFood( SnakeInfo& info , FoodInfo& food )
	{

	}

}//namespace GreedySnake