#ifndef GreedySnakeGame_h__
#define GreedySnakeGame_h__

#include "GamePackage.h"
#include "GameControl.h"

#define GREEDY_SNAKE_NAME "Greedy Snake"
namespace GreedySnake
{

	class CGamePackage : public IGamePackage
	{
	public:
		
		virtual void   beginPlay( GameType type, StageManager& manger );
		virtual void   deleteThis(){ delete this; }

	public:
		char const*                getName(){ return GREEDY_SNAKE_NAME;  }

		virtual GameController&    getController(){ return mController; }
		virtual GameSubStage*      createSubStage( unsigned id );
		virtual StageBase*         createStage( unsigned id ){ return NULL; }
		virtual SettingHepler*     createSettingHelper( SettingHelperType type );
		virtual bool               getAttribValue( AttribValue& value );

		SimpleController   mController;
	};
}

#endif // GreedySnakeGame_h__
