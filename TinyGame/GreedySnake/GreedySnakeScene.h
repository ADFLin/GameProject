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
	};


	enum DirEnum
	{
		DIR_EAST  = 0,
		DIR_SOUTH = 1,
		DIR_WEST  = 2,
		DIR_NORTH = 3,
	};

	enum FoodType 
	{
		FOOD_GROW       ,
		FOOD_SPEED_SLOW ,
		FOOD_SPEED_UP   ,
		FOOD_CONFUSED   ,
	};

	class Mode
	{
	public:
		virtual ~Mode(){}
		virtual void restart( bool beInit ){}
		virtual void setupLevel( IPlayerManager& playerManager ) = 0;
		virtual void prevLevelTick(){}
		virtual void postLevelTick(){}
		virtual void onEatFood( SnakeInfo& info , FoodInfo& food ){}
		virtual void onCollideSnake( SnakeInfo& snake , SnakeInfo& colSnake ){}
		virtual void onCollideTerrain( SnakeInfo& snake , int type ){}
		Scene& getScene(){ return *mScene; }
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
		Level::MapType mapType;
		GameMode       mode;
		size_t         numPlayer;
	};


	class Scene : public Level::Listener
		        , public ActionEnumer
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
		void onEatFood( SnakeInfo& info , FoodInfo& food );
		void onCollideSnake( SnakeInfo& snake , SnakeInfo& colSnake );
		void onCollideTerrain( SnakeInfo& snake , int type );


		void  fireSnakeAction( ActionTrigger& trigger );
		void  fireAction( ActionTrigger& trigger );

		void  killSnake( unsigned id );

		void  changeSnakeMoveDir( unsigned id , DirEnum dir );

		class RenderVisitor
		{
		public:
			RenderVisitor( Graphics2D& _g ):g(_g){}
			void visit( FoodInfo const& info );

			Graphics2D& g;
		};

		static int const BlockSize  = 20;
		static int const SnakeWidth = 14;
		static int const SnakeGap   = ( BlockSize - SnakeWidth ) / 2;

		void productRandomFood( int type );

		void drawSnake( Graphics2D& g , SnakeInfo& info , float dFrame );
		void drawSnakeBody( Graphics2D& g , Snake& snake , int color , int offset );
		void drawFood( Graphics2D& g )
		{
			getLevel().visitFood( RenderVisitor( g ) );
		}

		Level& getLevel(){  return mLevel;  }
		void   setOver(){ mIsOver = true; }

		bool   isOver(){ return mIsOver; }

		bool    mIsOver;
		Level   mLevel;
		Mode&   mMode;
	};

}//namespace GreedySnake


#endif // GreedySnakeScene_h__
