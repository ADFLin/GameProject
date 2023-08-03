#include "GreedySnakePCH.h"
#include "GreedySnakeMode.h"
#include "GreedySnakeLevel.h"

#include "GamePlayer.h"

namespace GreedySnake
{
	void SurvivalMode::restart( bool beInit )
	{
		Level& level = getScene().getLevel();
		if ( beInit )
		{
			level.setupMap( Vec2i( 21 , 21 ) , MapBoundType::Cliff );
		}
		else
		{
			level.removeAllFood();
			level.removeAllSnake();
		}

		Snake* snake = level.addSnake( Vec2i(10,10) , 0 , 5 );
		getScene().productRandomFood(FOOD_GROW);
	}

	void SurvivalMode::setupLevel( IPlayerManager& playerManager )
	{
		Level& level = getScene().getLevel();

		for( auto iter = playerManager.createIterator(); iter; ++iter )
		{
			GamePlayer* player = iter.getElement();
			if( player->getType() == PT_SPECTATORS )
				continue;

			player->setActionPort( 0 );
		}
	}

	void SurvivalMode::onEatFood( Snake& snake , FoodInfo& food )
	{
		applyDefaultEffect(snake, food);
		getScene().productRandomFood(FOOD_GROW);
	}

	void SurvivalMode::onCollideSnake( Snake& snake , Snake& colSnake )
	{
		snake.stateBit |= SS_DEAD;
		getScene().setOver();
	}
	void SurvivalMode::onCollideTerrain( Snake& snake , int type )
	{
		snake.stateBit |= SS_DEAD;
		getScene().setOver();
	}


	void BattleMode::restart( bool beInit )
	{
		Level& level = getScene().getLevel();
		if ( beInit )
		{
			level.setupMap( Vec2i( 21 , 21 ) , MapBoundType::WarpXY);
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
			Snake* snake = level.addSnake( spawnPos[i] , DirType(i) , 3 );
		}

		mNumAlivePlayer = mNumPlayer;

		for( int i = 0; i < mNumPlayer ; ++i )
			getScene().productRandomFood(::Global::RandomNet() % NumFoodType);
	}

	void BattleMode::setupLevel( IPlayerManager& playerManager )
	{
		mNumPlayer = 0;
		for( auto iter = playerManager.createIterator(); iter; ++iter )
		{
			GamePlayer* player = iter.getElement();
			if( player->getType() == PT_SPECTATORS )
				continue;

			player->setActionPort( mNumPlayer );
			++mNumPlayer;
		}
	}

	void BattleMode::onCollideSnake( Snake& snake , Snake& colSnake )
	{
		//killSnake(snake);
	}

	void BattleMode::onCollideTerrain( Snake& snake , int type )
	{
		killSnake( snake );
	}

	void BattleMode::killSnake( Snake &snake )
	{
		Level& level = getScene().getLevel();

		getScene().killSnake( snake.id );
		--mNumAlivePlayer;

		if ( mNumAlivePlayer == 1 )
		{
			getScene().setOver();

			for( unsigned i = 0 ; i < (unsigned)level.getSnakeNum(); ++i )
			{
				Snake& snake = level.getSnake( i );
				if ( snake.stateBit & SS_DEAD )
					continue;

				mWinRound[ snake.id ] += 1;
				break;
			}
		}
	}

	void BattleMode::onEatFood( Snake& snake , FoodInfo& food )
	{
		applyDefaultEffect(snake, food);
		getScene().productRandomFood(::Global::RandomNet() % NumFoodType);
	}

}//namespace GreedySnake