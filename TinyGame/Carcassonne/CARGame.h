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
		bool  initialize() override{ return true; }
		void  cleanup() override{}
		void  enter() override{}
		void  exit() override{} 
		void  deleteThis() override{ delete this; }
		//
		void beginPlay( StageManager& manger, EGameStageMode modeType ) override;
	public:
		char const*           getName() override{ return CAR_GAME_NAME; }
		GameController&       getController() override{ return IGameModule::getController(); }
		StageBase*            createStage( unsigned id ) override;
		SettingHepler*        createSettingHelper( SettingHelperType type ) override;
		bool                  queryAttribute( GameAttribute& value ) override;

		//old replay version
		ReplayTemplate*       createReplayTemplate( unsigned version ) override{ return nullptr; }

	public:
	};




}//namespace CAR

#endif // CARGame_h__9c08f051_07b8_4f17_b7f0_9f5cf184db8e
