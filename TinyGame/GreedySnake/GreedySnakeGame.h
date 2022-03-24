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
		
		virtual void   beginPlay( StageManager& manger, EGameStageMode modeType ) override;

	public:
		char const*                getName(){ return GREEDY_SNAKE_NAME;  }

		virtual InputControl&      getInputControl() override { return mInputControl; }
		virtual StageBase*         createStage(unsigned id) override;
		virtual SettingHepler*     createSettingHelper( SettingHelperType type ) override;
		virtual bool               queryAttribute( GameAttribute& value ) override;

		DefaultInputControl   mInputControl;
	};
}

#endif // GreedySnakeGame_h__
