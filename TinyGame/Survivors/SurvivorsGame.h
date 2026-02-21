#ifndef SurvivorsGame_h__
#define SurvivorsGame_h__

#include "GameModule.h"
#include "GameControl.h"

#define SURVIVORS_NAME "Survivors"

namespace Survivors
{
	class GameModule : public IGameModule
	{
	public:
		void  enter() override {}
		void  exit() override {}
		void beginPlay(StageManager& manger, EGameMode modeType) override;

	public:
		virtual char const*           getName() override { return SURVIVORS_NAME; }
		virtual InputControl&         getInputControl() override { return mInputControl; }
		virtual StageBase*            createStage(unsigned id) override;
		virtual bool                  queryAttribute(GameAttribute& value) override;

		DefaultInputControl mInputControl;
	};

}//namespace Survivors

#endif // SurvivorsGame_h__
