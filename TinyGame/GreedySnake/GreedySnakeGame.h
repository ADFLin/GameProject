#ifndef GreedySnakeGame_h__
#define GreedySnakeGame_h__

#include "GameModule.h"
#include "GameControl.h"

#define GREEDY_SNAKE_NAME "Greedy Snake"
namespace GreedySnake
{

	class GameModule : public IGameModule
	{
	public:
		
		virtual void   beginPlay( StageManager& manger, StageModeType modeType ) override;
		virtual void   deleteThis(){ delete this; }

	public:
		char const*                getName(){ return GREEDY_SNAKE_NAME;  }

		virtual GameController&    getController() override { return mController; }
		virtual StageBase*         createStage(unsigned id) override;
		virtual SettingHepler*     createSettingHelper( SettingHelperType type ) override;
		virtual bool               getAttribValue( AttribValue& value ) override;

		SimpleController   mController;
	};
}

#endif // GreedySnakeGame_h__
