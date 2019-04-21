#ifndef GreedySnakeScene_h__
#define GreedySnakeScene_h__

#include "GreedySnakeLevel.h"
#include "GameControl.h"

class IPlayerManager;

namespace GreedySnake
{
	class Scene;

	enum ActionKey
	{
		ACT_GS_MOVE_E ,
		ACT_GS_MOVE_S ,
		ACT_GS_MOVE_W ,
		ACT_GS_MOVE_N ,
		ACT_GS_CHANGE_DIR ,
	};


	enum FoodType 
	{
		FOOD_GROW       ,
		FOOD_SPEED_SLOW ,
		FOOD_SPEED_UP   ,
		FOOD_CONFUSED   ,

		NumFoodType ,
	};

	class Mode
	{
	public:
		virtual ~Mode(){}
		virtual void restart( bool beInit ){}
		virtual void setupLevel( IPlayerManager& playerManager ) = 0;
		virtual void prevLevelTick(){}
		virtual void postLevelTick(){}
		virtual void onEatFood( Snake& info , FoodInfo& food ){}
		virtual void onCollideSnake( Snake& snake , Snake& colSnake ){}
		virtual void onCollideTerrain( Snake& snake , int type ){}
		Scene& getScene(){ return *mScene; }


		void applyDefaultEffect(Snake& snake , FoodInfo const& food );
	private:
		friend class Scene;
		Scene* mScene;
	};

	enum GameMode
	{
		GM_SURIVAL ,
		GM_BATTLE  ,
	};

	struct GameInfo
	{
		MapBoundType   mapBoundType;
		GameMode       mode;
		size_t         numPlayer;
	};


	class Scene : public Level::Listener
		        , public IActionLanucher
	{
	public:
	
		Scene( Mode& mode );
		~Scene()
		{
		}

		void restart( bool beInit );

		void tick();
		void updateFrame( int frame )
		{

		}

		void render( Graphics2D& g , float dFrame );
		//Level::Listener
		void onEatFood( Snake& snake , FoodInfo& food );
		void onCollideSnake( Snake& snake , Snake& colSnake );
		void onCollideTerrain( Snake& snake , int type );
		//~Level::Listener

		void  fireSnakeAction(ActionPort port, ActionTrigger& trigger);
		void  fireAction( ActionTrigger& trigger );

		void  killSnake( unsigned id );

		static int const BlockSize  = 20;
		static int const SnakeWidth = 14;
		static int const SnakeGap   = ( BlockSize - SnakeWidth ) / 2;

		void productRandomFood( int type );

		void drawSnake( Graphics2D& g , Snake& snake, float dFrame );
		void drawSnakeBody(Graphics2D& g, Snake& snake, int color, int offset);
		void drawFood( Graphics2D& g );

		Level& getLevel(){  return mLevel;  }
		void   setOver(){ mIsOver = true; }

		bool   isOver(){ return mIsOver; }


		long    mFrame;
		bool    mIsOver;
		Level   mLevel;
		Mode&   mMode;
	};

}//namespace GreedySnake


#endif // GreedySnakeScene_h__
