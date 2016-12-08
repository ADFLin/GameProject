#ifndef GreedySnakeGame_h__
#define GreedySnakeGame_h__

#include "GameInstance.h"
#include "GameControl.h"

#define GREEDY_SNAKE_NAME "Greedy Snake"
namespace GreedySnake
{

	class GameInstance : public IGameInstance
	{
	public:
		
		virtual void   beginPlay( StageModeType type, StageManager& manger );
		virtual void   deleteThis(){ delete this; }

	public:
		char const*                getName(){ return GREEDY_SNAKE_NAME;  }

		virtual GameController&    getController(){ return mController; }
		virtual StageBase*         createStage(unsigned id);
		virtual SettingHepler*     createSettingHelper( SettingHelperType type );
		virtual bool               getAttribValue( AttribValue& value );

		SimpleController   mController;
	};
}

#endif // GreedySnakeGame_h__
