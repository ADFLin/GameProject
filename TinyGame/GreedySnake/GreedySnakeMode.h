#ifndef GreedySnakeMode_h__
#define GreedySnakeMode_h__

#include "GreedySnakeScene.h"

class IPlayerManager;

namespace GreedySnake
{
	class SurvivalMode : public Mode
	{
		typedef Mode BaseClass;
	public:
		/*virtual */
		void restart( bool beInit  );
		void setupLevel( IPlayerManager& playerManager );
		void onEatFood( Snake& snake, FoodInfo& food );
		void onCollideSnake( Snake& snake , Snake& colSnake );
		void onCollideTerrain( Snake& snake , int type );
	};

	class BattleMode : public Mode
	{
		typedef Mode BaseClass;
	public:
		static int const MaxPlayerNum = 4;
		/*virtual */
		void restart( bool beInit );
		void setupLevel( IPlayerManager& playerManager );
		void prevLevelTick(){}
		void postLevelTick(){}
	protected:
		void onEatFood( Snake& snake , FoodInfo& food );
		void onCollideSnake( Snake& snake , Snake& colSnake );
		void onCollideTerrain( Snake& snake , int type );

	protected:
		void killSnake( Snake& snake );

		unsigned mCurRound;
		unsigned mWinRound[ gMaxPlayerNum ];
		unsigned mNumAlivePlayer;
		unsigned mNumPlayer;
	};

}//GreedySnake

#endif // GreedySnakeMode_h__
