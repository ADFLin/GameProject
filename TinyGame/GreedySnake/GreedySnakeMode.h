#ifndef GreedySnakeMode_h__
#define GreedySnakeMode_h__

#include "GreedySnakeScene.h"

class IPlayerManager;

namespace GreedySnake
{
	class SurvivalMode : public Mode
	{
	public:
		/*virtual */
		void restart( bool beInit  );
		void setupLevel( IPlayerManager& playerManager );
		void onEatFood( SnakeInfo& info , FoodInfo& food );
		void onCollideSnake( SnakeInfo& snake , SnakeInfo& colSnake );
		void onCollideTerrain( SnakeInfo& snake , int type );
	};

	class BattleMode : public Mode
	{
	public:
		/*virtual */
		void restart( bool beInit );
		void setupLevel( IPlayerManager& playerManager );
		void prevLevelTick(){}
		void postLevelTick(){}
	protected:
		void onEatFood( SnakeInfo& info , FoodInfo& food );
		void onCollideSnake( SnakeInfo& snake , SnakeInfo& colSnake );
		void onCollideTerrain( SnakeInfo& snake , int type );

	protected:
		void killSnake( SnakeInfo &snake );

		unsigned mCurRound;
		unsigned mWinRound[ gMaxPlayerNum ];
		unsigned mNumAlivePlayer;
		unsigned mNumPlayer;
	};

}//GreedySnake

#endif // GreedySnakeMode_h__
