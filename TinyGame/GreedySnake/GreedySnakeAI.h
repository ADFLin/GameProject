#ifndef GreedySnakeAI_h__
#define GreedySnakeAI_h__

#include "GamePlayer.h"

#include "TGrid2D.h"
#include "GreedySnakeScene.h"

namespace GreedySnake
{
	class SnakeAI : public AIBase
		          , public IActionInput
	{
	public:
		SnakeAI( Scene& scene , unsigned id );

		static void init( Scene& scene );

		virtual bool scanInput( bool beUpdateFrame );
		virtual bool checkAction( ActionParam& param );

		virtual IActionInput* getActionInput(){ return this; }

		DirType mMoveDir;

		int   calcBoundedCount( Vec2i const& pos , DirType dir );
		void  incTestCount();

		static unsigned const ColMask = Level::eTERRAIN_MASK | Level::eSNAKE_MASK;

		bool testCollision( Vec2i const& pos , DirType dir , Vec2i& resultPos );
		bool testCollision( Vec2i const& pos );

		DirType decideMoveDir();

		

		enum State
		{
			eSTATE_GO_DEST_POS ,
		};

		Snake& hostSnake;
		Scene& mScene;


	};

}//namespace GreedySnake

#endif // GreedySnakeAI_h__
