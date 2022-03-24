#ifndef CARGame_h__9c08f051_07b8_4f17_b7f0_9f5cf184db8e
#define CARGame_h__9c08f051_07b8_4f17_b7f0_9f5cf184db8e

#include "GameModule.h"

#define CAR_GAME_NAME "Carcassonne"

namespace CAR
{

	class GameModule : public IGameModule
	{
	public:
		GameModule();
		virtual ~GameModule();
		void  startupModule() override {}
		void  shutdownModule() override {}
		void  deleteThis() override { delete this; }

		void  enter() override{}
		void  exit() override{} 

		//
		void beginPlay( StageManager& manger, EGameStageMode modeType ) override;
	public:
		char const*           getName() override{ return CAR_GAME_NAME; }
		InputControl&         getInputControl() override{ return IGameModule::getInputControl(); }
		StageBase*            createStage( unsigned id ) override;
		SettingHepler*        createSettingHelper( SettingHelperType type ) override;
		bool                  queryAttribute( GameAttribute& value ) override;

		//old replay version
		ReplayTemplate*       createReplayTemplate( unsigned version ) override{ return nullptr; }

	public:
	};




}//namespace CAR

#endif // CARGame_h__9c08f051_07b8_4f17_b7f0_9f5cf184db8e
